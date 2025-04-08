/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2000-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#ifndef UFS_BLK_H
#define UFS_BLK_H

#if 0

#ifdef CONFIG_SYS_64BIT_LBA
typedef uint64_t lbaint_t;
#define LBAFlength "ll"
#else
typedef unsigned long long lbaint_t;
#define LBAFlength "l"
#endif
#define LBAF "%" LBAFlength "x"
#define LBAFU "%" LBAFlength "u"

#define BLK_VEN_SIZE		40
#define BLK_PRD_SIZE		20
#define BLK_REV_SIZE		8

#define PART_FORMAT_PCAT	0x1
#define PART_FORMAT_GPT		0x2

/* Interface types: */
enum if_type {
	IF_TYPE_UNKNOWN = 0,
	IF_TYPE_IDE,
	IF_TYPE_SCSI,
	IF_TYPE_ATAPI,
	IF_TYPE_USB,
	IF_TYPE_DOC,
	IF_TYPE_MMC,
	IF_TYPE_SD,
	IF_TYPE_SATA,
	IF_TYPE_HOST,
	IF_TYPE_NVME,
	IF_TYPE_EFI_LOADER,
	IF_TYPE_PVBLOCK,
	IF_TYPE_VIRTIO,
	IF_TYPE_EFI_MEDIA,

	IF_TYPE_COUNT,			/* Number of interface types */
};


/*
 * With driver model (CONFIG_BLK) this is uclass platform data, accessible
 * with dev_get_uclass_plat(dev)
 */
struct blk_desc {
	/*
	 * TODO: With driver model we should be able to use the parent
	 * device's uclass instead.
	 */
	int		devnum;		/* device number */
	unsigned char	part_type;	/* partition type */
	unsigned char	target;		/* target SCSI ID */
	unsigned char	lun;		/* target LUN */
	unsigned char	hwpart;		/* HW partition, e.g. for eMMC */
	unsigned char	type;		/* device type */
	unsigned char	removable;	/* removable device */
#ifdef CONFIG_LBA48
	/* device can use 48bit addr (ATA/ATAPI v7) */
	unsigned char	lba48;
#endif
	lbaint_t	lba;		/* number of blocks */
	unsigned long	blksz;		/* block size */
/*	int		log2blksz;*/	/* for convenience: log2(blksz) */
//	char		vendor[BLK_VEN_SIZE + 1]; /* device vendor string */
//	char		product[BLK_PRD_SIZE + 1]; /* device product number */
//	char		revision[BLK_REV_SIZE + 1]; /* firmware revision */
#if 1
	/*
	 * For now we have a few functions which take struct blk_desc as a
	 * parameter. This field allows them to look up the associated
	 * device. Once these functions are removed we can drop this field.
	 */
	//struct udevice *bdev;
#else
	unsigned long	(*block_read)(struct blk_desc *block_dev,
				      lbaint_t start,
				      lbaint_t blkcnt,
				      void *buffer);
	unsigned long	(*block_write)(struct blk_desc *block_dev,
				       lbaint_t start,
				       lbaint_t blkcnt,
				       const void *buffer);
	unsigned long	(*block_erase)(struct blk_desc *block_dev,
				       lbaint_t start,
				       lbaint_t blkcnt);
	void		*priv;		/* driver private struct pointer */
#endif
};

#define BLOCK_CNT(size, blk_desc) (PAD_COUNT(size, blk_desc->blksz))
#define PAD_TO_BLOCKSIZE(size, blk_desc) \
	(PAD_SIZE(size, blk_desc->blksz))
#endif

#if !CONFIG_IS_ENABLED(BLK) && !CONFIG_IS_ENABLED(AW_BLK)
/* Operations on block devices */
struct blk_ops {
	/**
	 * read() - read from a block device
	 *
	 * @dev:	Device to read from
	 * @start:	Start block number to read (0=first)
	 * @blkcnt:	Number of blocks to read
	 * @buffer:	Destination buffer for data read
	 * @return number of blocks read, or -ve error number (see the
	 * IS_ERR_VALUE() macro
	 */
	unsigned long (*read)(struct udevice *dev, lbaint_t start,
			      lbaint_t blkcnt, void *buffer);

	/**
	 * write() - write to a block device
	 *
	 * @dev:	Device to write to
	 * @start:	Start block number to write (0=first)
	 * @blkcnt:	Number of blocks to write
	 * @buffer:	Source buffer for data to write
	 * @return number of blocks written, or -ve error number (see the
	 * IS_ERR_VALUE() macro
	 */
	unsigned long (*write)(struct udevice *dev, lbaint_t start,
			       lbaint_t blkcnt, const void *buffer);

	/**
	 * erase() - erase a section of a block device
	 *
	 * @dev:	Device to (partially) erase
	 * @start:	Start block number to erase (0=first)
	 * @blkcnt:	Number of blocks to erase
	 * @return number of blocks erased, or -ve error number (see the
	 * IS_ERR_VALUE() macro
	 */
	unsigned long (*erase)(struct udevice *dev, lbaint_t start,
			       lbaint_t blkcnt);

	/**
	 * select_hwpart() - select a particular hardware partition
	 *
	 * Some devices (e.g. MMC) can support partitioning at the hardware
	 * level. This is quite separate from the normal idea of
	 * software-based partitions. MMC hardware partitions must be
	 * explicitly selected. Once selected only the region of the device
	 * covered by that partition is accessible.
	 *
	 * The MMC standard provides for two boot partitions (numbered 1 and 2),
	 * rpmb (3), and up to 4 addition general-purpose partitions (4-7).
	 *
	 * @desc:	Block device to update
	 * @hwpart:	Hardware partition number to select. 0 means the raw
	 *		device, 1 is the first partition, 2 is the second, etc.
	 * @return 0 if OK, -ve on error
	 */
	int (*select_hwpart)(struct udevice *dev, int hwpart);
};
#endif

#endif
