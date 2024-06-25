#define CONFIG_SYS_EEPROM_BUS_NUM		0

/* Magic number at the first four bytes of EEPROM HATs */

#define EEPROM_HATS_SIG	"RADX" /* Radxa ROCK */

/*
 * MAGIC_NUMBER_BYTES: number of bytes used by the magic number
 */
#define MAGIC_NUMBER_BYTES			4

/*
 * MAC_ADDR_BYTES: number of bytes used by the Ethernet MAC address
 */
#define MAC_ADDR_BYTES				60

/*
 * SN_BYTES: length of sn string
*/
#define SN_BYTES				20

/*
 * Atom Types
 * 0x0000 = invalid
 * 0x0001 = vendor info
 * 0x0002 = GPIO map
 * 0x0003 = Linux device tree blob
 * 0x0004 = manufacturer custom data
 * 0x0005-0xfffe = reserved for future use
 * 0xffff = invalid
 */
#define HATS_ATOM_INVALID	0x0000
#define HATS_ATOM_VENDOR	0x0001
#define HATS_ATOM_GPIO		0x0002
#define HATS_ATOM_DTB		0x0003
#define HATS_ATOM_CUSTOM	0x0004
#define HATS_ATOM5_CUSTOM	0x0005
#define HATS_ATOM_INVALID_END	0xffff

#define BOM_ATOM4_BYTES	8

#define CONFIG_EEPROM_HATS_SIZE_MAX	256	/* Header + Atom1&4&5(v1) */
#define CONFIG_EEPROM_WP_OFFSET		256	/* Read only field */
#define CONFIG_EEPROM_ATOM1_PSTR_SIZE	32
#define CONFIG_EEPROM_ATOM1_SN_OFFSET	23
#define CONFIG_EEPROM_ATOM1_VSTR_SIZE	32
#define CONFIG_SYS_I2C_EEPROM_ADDR	0x50
#define CONFIG_SYS_I2C_EEPROM_ADDR_LEN	1

int radxa_mac_read_from_eeprom(void);
