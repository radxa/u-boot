// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2021 Red Hat, Inc. All Rights Reserved.
 * Copyright (C) 2024 Radxa Computer (Shenzhen) Co., Ltd.
 */

#include <dm.h>
#include <env.h>
#include <i2c_eeprom.h>
#include <net.h>
#include <radxa-i2c-eeprom.h>
#include <asm/unaligned.h>
#include <linux/kernel.h>
#include <linux/string.h>

#define RADXA_EEPROM_SIZE	256
#define RADXA_HEADER_SIZE	12
#define RADXA_ATOM_HEADER_SIZE	8
#define RADXA_MAC_ATOM		5
#define RADXA_MAC_COUNT		10
#define RADXA_SN_SIZE		20

/* HAT atom CRC: CRC-16/ARC with a zero initial value. */
static u16 calculate_crc16(const u8 *data, unsigned int size)
{
	u16 crc = 0;
	unsigned int i;

	while (size--) {
		crc ^= *data++;
		for (i = 0; i < 8; i++)
			crc = (crc >> 1) ^ ((crc & 1) ? 0xa001 : 0);
	}

	return crc;
}

static int parse_eeprom_macs(const u8 *buf, size_t size, u8 *macs)
{
	const u8 *mac_data = NULL;
	size_t offset = RADXA_HEADER_SIZE;
	u32 length, dlen;
	u16 count, type, crc;
	unsigned int i;

	if (size < RADXA_HEADER_SIZE || memcmp(buf, "RADX", 4))
		return -EINVAL;
	if (buf[4] != 1 || buf[5])
		return -EPROTONOSUPPORT;
	count = get_unaligned_le16(buf + 6);
	length = get_unaligned_le32(buf + 8);
	if (length < RADXA_HEADER_SIZE || length > size || !count)
		return -EINVAL;

	for (i = 0; i < count; i++) {
		if (length - offset < RADXA_ATOM_HEADER_SIZE)
			return -EINVAL;
		type = get_unaligned_le16(buf + offset);
		dlen = get_unaligned_le32(buf + offset + 4);
		if (dlen < sizeof(u16) ||
		    dlen > length - offset - RADXA_ATOM_HEADER_SIZE)
			return -EINVAL;
		crc = get_unaligned_le16(buf + offset +
					 RADXA_ATOM_HEADER_SIZE + dlen - sizeof(u16));
		if (crc != calculate_crc16(buf + offset,
					  RADXA_ATOM_HEADER_SIZE + dlen - sizeof(u16)))
			return -EBADMSG;
		if (type == RADXA_MAC_ATOM) {
			/* Version 1 may append board-specific fields after the MACs. */
			if (mac_data || dlen < sizeof(u16) + RADXA_SN_SIZE +
			    RADXA_MAC_COUNT * ARP_HLEN + sizeof(u16))
				return -EINVAL;
			if (get_unaligned_le16(buf + offset + RADXA_ATOM_HEADER_SIZE) != 1)
				return -EPROTONOSUPPORT;
			mac_data = buf + offset + RADXA_ATOM_HEADER_SIZE +
				   sizeof(u16) + RADXA_SN_SIZE;
		}
		offset += RADXA_ATOM_HEADER_SIZE + dlen;
	}
	if (offset != length || !mac_data)
		return -EINVAL;
	/* Do not suppress the platform fallback without a usable primary MAC. */
	if (!is_valid_ethaddr(mac_data))
		return -ENODATA;
	memcpy(macs, mac_data, RADXA_MAC_COUNT * ARP_HLEN);

	return 0;
}

static int set_mac_address_env(const u8 *macs)
{
	u8 existing[ARP_HLEN];
	char name[16];
	unsigned int i;
	int ret;

	for (i = 0; i < RADXA_MAC_COUNT; i++) {
		if (!is_valid_ethaddr(macs + i * ARP_HLEN))
			continue;
		if (i)
			snprintf(name, sizeof(name), "eth%uaddr", i);
		else
			strcpy(name, "ethaddr");
		/* Keep valid addresses explicitly supplied by the environment. */
		if (eth_env_get_enetaddr(name, existing))
			continue;
		ret = eth_env_set_enetaddr(name, macs + i * ARP_HLEN);
		if (ret)
			return ret;
	}

	return 0;
}

int radxa_mac_read_from_eeprom(void)
{
	static const unsigned int offsets[] = { 0, 0x100 };
	u8 buf[RADXA_EEPROM_SIZE], macs[RADXA_MAC_COUNT * ARP_HLEN];
	struct udevice *dev;
	unsigned int i;
	int ret, size;

	for (ret = uclass_first_device_check(UCLASS_I2C_EEPROM, &dev); dev;
	     ret = uclass_next_device_check(&dev)) {
		if (ret)
			continue;
		size = i2c_eeprom_size(dev);
		if (size < RADXA_EEPROM_SIZE)
			continue;
		for (i = 0; i < ARRAY_SIZE(offsets); i++) {
			if (offsets[i] + sizeof(buf) > (unsigned int)size)
				continue;
			/* The generic driver handles 24C04/08/16 address overflow. */
			ret = i2c_eeprom_read(dev, offsets[i], buf, sizeof(buf));
			if (ret)
				continue;
			ret = parse_eeprom_macs(buf, sizeof(buf), macs);
			if (ret)
				continue;
			return set_mac_address_env(macs);
		}
	}

	return -ENODATA;
}
