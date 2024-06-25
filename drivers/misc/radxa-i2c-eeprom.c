/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2021 Red Hat, Inc. All Rights Reserved.
 * Copyright (C) 2024 Radxa Computer (Shenzhen) Co., Ltd.
 */

#include <common.h>
#include <command.h>
#include <i2c.h>
#include <vsprintf.h>
#include <radxa-i2c-eeprom.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <common.h>
#include <dm.h>
#include <i2c.h>
#include <log.h>
#include <errno.h>

struct eeprom_hats_header {
	char signature[MAGIC_NUMBER_BYTES];	/* ASCII table signature */
	u8 version;		/* EEPROM data format version */
				/* (0x00 reserved, 0x01 = first version) */
	u8 reversed;		/* 0x00, Reserved field */
	u16 numatoms;		/* total atoms in EEPROM */
	u32 eeplen;		/* total length in bytes of all eeprom data */
				/* (including this header) */
};

struct eeprom_hats_atom_header {
	u16 type;
	u16 count;
	u32 dlen;
};

/**
 * static eeprom: EEPROM layout for the ROCK platform I2C format
 */

struct eeprom_atom1_data {
	u8 uuid[16];
	u16 pid;
	u16 pver;
	u8 vslen;
	u8 pslen;

	char vstr[CONFIG_EEPROM_ATOM1_VSTR_SIZE];
	char pstr[CONFIG_EEPROM_ATOM1_PSTR_SIZE]; /* product SN */
};

struct eeprom_atom1 {
	struct eeprom_hats_atom_header header;
	struct eeprom_atom1_data data;
	u16 crc16;
};

struct eeprom_atom4_v1_data {
	u16 version;
	u8 bom_revision[BOM_ATOM4_BYTES];/* BOM version */
};

struct eeprom_atom4_v1 {
	struct eeprom_hats_atom_header header;
	struct eeprom_atom4_v1_data data;
	u16 crc16;
};

struct eeprom_atom5_data {
	u16 version;
	u8 sn_revision[SN_BYTES];	/* PCB version */
	u8 mac_addr[MAC_ADDR_BYTES];	/* Ethernet MAC */
};

struct eeprom_atom5 {
	struct eeprom_hats_atom_header header;
	struct eeprom_atom5_data data;
	u16 crc16;
};

/* Set to 1 if we've read EEPROM into memory
 * Set to -1 if EEPROM data is wrong
 */
static int has_been_read;

/**
 * helper struct for getting info from the local EEPROM copy.
 * most of the items are pointers to the eeprom_page1_buff.
 * ONLY serialnum is the u32 from the last 8 Bytes of product string
 */
struct eeprom_info_page1 {
	char *vstr;		/* Vendor string in ATOM1 */
	char *pstr;		/* product string in ATOM1 */
	u32 serialnum;		/* serial number from in product string*/
	u16 *version;		/* custom data version in ATOM4 */
	u8 *bom_revision;	/* BOM version in ATOM4 */
};
static struct eeprom_info_page1 einfo_page1;

struct eeprom_info_page2 {
	u16 *version;		/* custom data version in ATOM5 */
	u8 *sn_revision;		/* serial number from in product string*/
	u8 *mac_addr;		/*  MAC in ATOM5 */
};
static struct eeprom_info_page2 einfo_page2;

static uchar eeprom_buff[CONFIG_EEPROM_HATS_SIZE_MAX];

/**
 * is_match_magic() - Does the magic number match that of a ROCK EEPROM?
 *
 * @hats:		the pointer of eeprom_hats_header
 * Return:		status code, 0: Yes, non-0: NO
 */
static inline int is_match_magic(char *hats)
{
	return strncmp(hats, EEPROM_HATS_SIG, MAGIC_NUMBER_BYTES);
}
/**
 * calculate_crc16() - Calculate the current CRC for atom
 * Porting from  https://github.com/raspberrypi/hats, getcrc
 * @data:		the pointer of eeprom_hats_atom_header
 * @size:		total length in bytes of the entire atom
 * 			(type, count, dlen, data)
 * Return:		result: crc16 code
 */

ulong hextoul(const char *cp, char **endp)
{
	return simple_strtoul(cp, endp, 16);
}

#define CRC16 0x8005

static u16 calculate_crc16(uchar* data, unsigned int size)
{
	int i, j = 0x0001;
	u16 out = 0, crc = 0;
	int bits_read = 0, bit_flag;
	/* Sanity check: */

	if((data == NULL) || size == 0)
		return 0;
	while(size > 0) {
		bit_flag = out >> 15;
		/* Get next bit: */
		out <<= 1;
		// item a) work from the least significant bits
		out |= (*data >> bits_read) & 1;
		/* Increment bit counter: */
		bits_read++;
		if(bits_read > 7) {
			bits_read = 0;
			data++;
			size--;
		}
		/* Cycle check: */
		if(bit_flag)
			out ^= CRC16;
	}

	// item b) "push out" the last 16 bits
	for (i = 0; i < 16; ++i) {
		bit_flag = out >> 15;
		out <<= 1;
		if(bit_flag)
			out ^= CRC16;
	}
	// item c) reverse the bits
	for (i = 0x8000; i != 0; i >>=1, j <<= 1) {
		if (i & out)
			crc |= j;
	}
	return crc;
}

/* This function should be called after each update to any EEPROM ATOM */
static inline void update_crc(struct eeprom_hats_atom_header *atom)
{
	uint atom_crc_offset = sizeof(struct eeprom_hats_atom_header) +
				atom->dlen - sizeof(u16);
	u16 *atom_crc_p = (void *) atom + atom_crc_offset;
	*atom_crc_p = calculate_crc16((uchar*) atom, atom_crc_offset);
}

/**
 * dump_raw_eeprom - display the raw contents of the EEPROM
 */
static void dump_raw_eeprom(u8 *e, unsigned int size)
{
	unsigned int i;

	printf("EEPROM dump: (0x%x bytes)\n", size);

	for (i = 0; i < size; i++) {
		if (!(i % 0x10))
			printf("%02X: ", i);
		printf("%02X ", e[i]);
		if (((i % 16) == 15) || (i == size - 1))
			printf("\n");
	}
	return;
}

static int hats_atom_crc_check(struct eeprom_hats_atom_header *atom)
{
	u16 atom_crc, data_crc;
	uint atom_crc_offset = sizeof(struct eeprom_hats_atom_header) +
				atom->dlen - sizeof(atom_crc);
	u16 *atom_crc_p = (void *)atom + atom_crc_offset;

		atom_crc = *atom_crc_p;

	data_crc = calculate_crc16((uchar *) atom, atom_crc_offset);

	if (atom_crc == data_crc)
		return 0;

	printf("EEPROM HATs: CRC ERROR in atom %x type %x, (%x!=%x)\n",
		atom->count, atom->type, atom_crc, data_crc);

	return -1;
}

static void *hats_get_atom(struct eeprom_hats_header *header, u16 type)
{

	struct eeprom_hats_atom_header *atom;

	void *hats_eeprom_max = (void *)header + header->eeplen;
	void *temp = (void *)header + sizeof(struct eeprom_hats_header);

	for (int numatoms = (int)header->numatoms; numatoms > 0; numatoms--) {
		atom = (struct eeprom_hats_atom_header *)temp;
		if (hats_atom_crc_check(atom))
			return NULL;
		if (atom->type == type)
			return (void *)atom;

		/* go to next atom */
		temp = (void *)atom + sizeof(struct eeprom_hats_atom_header) + atom->dlen;
		if (temp > hats_eeprom_max) {
			printf("EEPROM HATs: table overflow next@%p, max@%p\n",
				temp, hats_eeprom_max);
			break;
		}
	}
	/* fail to get atom */
	return NULL;
}

/**
 * show_eeprom - display the contents of the EEPROM
 */
static void show_eeprom(void)
{
	printf("\n--------EEPROM INFO--------\n");
	printf("Vendor: %s\n", einfo_page1.vstr);
	printf("Product full SN: %s\n", einfo_page1.pstr);
	printf("Data version: 0x%x\n", *einfo_page1.version);

	if (*einfo_page1.version == 1) {
		printf("BOM revision: %s\n", einfo_page1.bom_revision);

		for (int i = 0; i < 10; i++) {
			int idx = i * 6;
			printf("Ethernet%d MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n", i,
				einfo_page2.mac_addr[idx], einfo_page2.mac_addr[idx + 1],
				einfo_page2.mac_addr[idx + 2], einfo_page2.mac_addr[idx + 3],
				einfo_page2.mac_addr[idx + 4], einfo_page2.mac_addr[idx + 5]);
			}
	} else {
		printf("Custom data v%d is not supported\n", *einfo_page1.version);
	}

	printf("--------EEPROM INFO--------\n\n");
}

/**
 * parse_eeprom_info - parse the contents of the EEPROM
 * If everthing gose right,
 * 1, set has_been_read to 1
 * 2, display info
 *
 * If anything goes wrong,
 * 1, set has_been_read to -1
 * 2, dump data by hex for debug
 *
 * @buf:		the pointer of eeprom_hats_header in memory
 * Return:		status code, 0: Success, non-0: Fail
 *
 */

static int parse_eeprom_info(struct eeprom_hats_header *buf)
{
	struct eeprom_hats_atom_header *atom;
	void *atom_data;
	struct eeprom_atom1_data *atom1 = NULL;
	struct eeprom_atom4_v1_data *atom4_v1 = NULL;
	struct eeprom_atom5_data *atom5 = NULL;

	if (is_match_magic((char *)buf)) {
		printf("Not a ROCK EEPROM data format - magic error\n");
		goto error;
	};

	printf("ROCK EEPROM format v%u\n", buf->version);

	// parse atom1(verdor)
	atom = (struct eeprom_hats_atom_header *)
	hats_get_atom(buf, HATS_ATOM_VENDOR);
	if (atom) {
		atom_data = (void *)atom + sizeof(struct eeprom_hats_atom_header);
		atom1 = (struct eeprom_atom1_data *)atom_data;
		einfo_page1.vstr = atom1->vstr;
		einfo_page1.pstr = atom1->pstr;
		einfo_page1.serialnum = (u32)hextoul((void *)atom1->pstr +
					CONFIG_EEPROM_ATOM1_SN_OFFSET,
					NULL);
	} else {
		printf("fail to get vendor atom\n");
			goto error;
	};

	// parse atom4(custom)
	atom = (struct eeprom_hats_atom_header *)
	hats_get_atom(buf, HATS_ATOM_CUSTOM);
	if (atom) {
		atom_data = (void *)atom + sizeof(struct eeprom_hats_atom_header);
		atom4_v1 = (struct eeprom_atom4_v1_data *)atom_data;
		einfo_page1.version = &atom4_v1->version;
		if (*einfo_page1.version == 1) {
			einfo_page1.bom_revision = atom4_v1->bom_revision;
		}
	} else {
		printf("fail to get custom data atom\n");
		goto error;
	};

	// parse atom5
	atom = (struct eeprom_hats_atom_header *)
	hats_get_atom(buf, HATS_ATOM5_CUSTOM);
	if (atom) {
		atom_data = (void *)atom + sizeof(struct eeprom_hats_atom_header);
		atom5 = (struct eeprom_atom5_data *)atom_data;
		einfo_page2.version = &atom5->version;

		if (*einfo_page2.version == 1)
		{
			einfo_page2.sn_revision = atom5->sn_revision ;
			einfo_page2.mac_addr = atom5->mac_addr;
		}
	}
	else {
		printf("fail to get page2 data atom\n");
		goto error;
	};
	// everthing gose right
	has_been_read = 1;
	show_eeprom();
	return 0;
error:

	has_been_read = -1;
	dump_raw_eeprom(eeprom_buff, CONFIG_EEPROM_HATS_SIZE_MAX);
	return -1;
}

/**
 * read_eeprom() - read the EEPROM into memory, if it hasn't been read yet
 * @buf:		the pointer of eeprom data buff
 * Return:		status code, 0: Success, non-0: Fail
 * Note: depend on	CONFIG_SYS_EEPROM_BUS_NUM
 * 			CONFIG_SYS_I2C_EEPROM_ADDR
 * 			CONFIG_EEPROM_WP_OFFSET
 * 			CONFIG_EEPROM_HATS_SIZE_MAX
 */

#define COMPATIBLE_STR "i2c-eeprom"

int find_i2c_bus_by_compatible(const char *compatible, struct udevice **bus)
{
	struct udevice *dev;

	for (uclass_first_device(UCLASS_I2C, &dev);
		dev;
		uclass_next_device(&dev)) {

		printf("Checking device: %s\n", dev_read_name(dev));
		if (device_is_compatible(dev, compatible)) {
		printf("Found compatible device: %s\n", dev_read_name(dev));
			*bus = dev;
			return dev->seq;
		}
	}
	printf("fail to get i2c device.\n");
	return -ENODEV;
}


static int read_eeprom(uint8_t *buf)
{
	int ret;
	int i2c_bus_num;
	struct udevice *dev;

	if (has_been_read == 1)
		return 0;

	i2c_bus_num = find_i2c_bus_by_compatible("i2c-eeprom",&dev);

	printf("!!!! i2c_bus_num: %d\n", i2c_bus_num);
	ret = i2c_get_chip_for_busnum(i2c_bus_num,
				CONFIG_SYS_I2C_EEPROM_ADDR,
				CONFIG_SYS_I2C_EEPROM_ADDR_LEN,
					&dev);

	printf("!!!! i2c_get_chip_for_busnum device: %s\n", dev_read_name(dev));

	if (!ret) {
		ret = dm_i2c_read(dev, CONFIG_EEPROM_WP_OFFSET,
				buf, CONFIG_EEPROM_HATS_SIZE_MAX);
	}

	if (ret) {
		printf("fail to read EEPROM.\n");
		return ret;
	}

	return parse_eeprom_info((struct eeprom_hats_header *)buf);
}

static int set_mac_address_env(u8 *mac_addr)
{

	char env_name[32];	// Buffer to hold the environment variable name
	u8 mac_str[6];	 // Buffer to hold the MAC address string
	int indx = 0 ;
	int mac_env_num = 1;
	if (mac_addr == NULL) {
		printf("Invalid MAC address pointer.\n");
		return -1;
	}

	/* set mac address to env */
	for(int i=0; i<10; i++)
	{
		for(int tmp = 0; tmp<6; tmp++)
		{
			mac_str[tmp] = mac_addr[indx++];
		}

		if(!(mac_str[0]|mac_str[1]|mac_str[2]|mac_str[3]|mac_str[4]|mac_str[5]))
		continue;

		printf("Set Ethernet%d MAC address: %02X:%02X:%02X:%02X:%02X:%02X \n",mac_env_num,
		mac_str[0],mac_str[1],mac_str[2],mac_str[3],mac_str[4],mac_str[5]);

		if(i==0) {
			sprintf(env_name, "ethaddr");
			eth_env_set_enetaddr(env_name, mac_str);
		} else {
			sprintf(env_name, "eth%daddr", mac_env_num++);
			eth_env_set_enetaddr(env_name, mac_str);
		}
	}
	return 0;
}

/**
 * radxa_mac_read_from_eeprom() - read the MAC address & the serial number in EEPROM
 *
 * This function reads the MAC address and the serial number from EEPROM and
 * sets the appropriate environment variables for each one read.
 *
 * The environment variables are only set if they haven't been set already.
 * This ensures that any user-saved variables are never overwritten.
 *
 * If CONFIG_RADXA_ID_EEPROM is enabled, this function will be called in
 * "int rockchip_setup_macaddr(void)" of u-boot/arch/arm/mach-rockchip/misc.c.
 */

// Function to set MAC address environment variable
int radxa_mac_read_from_eeprom(void)
{
	/**
	 * try to fill the buff from EEPROM,
	 * always return SUCCESS, even some error happens.
	 */

	if(read_eeprom(eeprom_buff))
		return -1;

	if (!set_mac_address_env(einfo_page2.mac_addr))
		return -1;

	return 0;
}

// int do_mac(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
// {
// 	printf("This device does not support user programmable EEPROM.\n");
// 	return -1;
// }
