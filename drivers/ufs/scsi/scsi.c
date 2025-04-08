// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2001
 * Denis Peter, MPL AG Switzerland
 */

/*tools/toolchain/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabi/lib/gcc/arm-linux-gnueabi/7.2.1/include/stdint-gcc.h
 * has conflict on uintptr_t with linux/types.h, undef it here
 * */
#undef __UINTPTR_TYPE__
#include <common.h>
#include <linux/kernel.h>
#include <types.h>
#include <ufs.h>
#include <./uinclude/scsi.h>
#include <device.h>
#include <./uinclude/blk.h>
#include <log.h>
#include "errno.h"
#include "stdint.h"
#include "part.h"
#include <device_compat.h>
#include <string.h>
#include <sunxi-ufs-trace.h>
#include <linux/unaligned/be_byteshift.h>

#define READ_CAPACITY
int blk_create_devicef(struct udevice *parent, const char *drv_name,
		       const char *name, int if_type, int devnum, int blksz,
		       lbaint_t lba, struct udevice **devp);


static struct scsi_cmd tempccb;	/* temporary scsi command buffer */

static unsigned char tempbuff[512]; /* temporary data buffer */

/* almost the maximum amount of the scsi_ext command.. */
//#define SCSI_MAX_BLK 0xFFFF

/*limit to 16 block(64k).if no limit,use max boot0 668k,
 * simulate time is too long,so only simulate max 64k
 * on one time transfer,because we can't run on fpga.by lixiang
 * */

#define SCSI_MAX_BLK (128)
#define SCSI_LBA48_READ	0xFFFFFFF

static void scsi_print_error(struct scsi_cmd *pccb)
{
	/* Dummy function that could print an error for debugging */
	int i = 0;

	dev_err(NULL, "scsi cmd");
	for (i = 0; i < 16; i++) {
		dev_err(NULL, "%02X ", pccb->cmd[i]);
	}
	dev_err(NULL, "\n");

	dev_err(NULL, "scsi cmd len %d,data len %lu\n", pccb->cmdlen, pccb->datalen);
	dev_err(NULL, "scsi first 32 byte data");
	for (i = 0; (i < (pccb->datalen)) && (i < 32); i++) {
		dev_err(NULL, "%02X ", pccb->pdata[i]);
	}
	dev_err(NULL, "\n");

	dev_err(NULL, "scsi last 32 byte data");
	if (pccb->datalen > 32) {
		for (i = pccb->datalen - 32; i < pccb->datalen; i++) {
			dev_err(NULL, "%02X ", pccb->pdata[i]);
		}
	}
	dev_err(NULL, "\n");

}

#ifdef CONFIG_SYS_64BIT_LBA
void scsi_setup_read16(struct scsi_cmd *pccb, lbaint_t start,
		       unsigned long blocks)
{
	pccb->cmd[0] = SCSI_READ16;
	pccb->cmd[1] = pccb->lun << 5;
	pccb->cmd[2] = (unsigned char)(start >> 56) & 0xff;
	pccb->cmd[3] = (unsigned char)(start >> 48) & 0xff;
	pccb->cmd[4] = (unsigned char)(start >> 40) & 0xff;
	pccb->cmd[5] = (unsigned char)(start >> 32) & 0xff;
	pccb->cmd[6] = (unsigned char)(start >> 24) & 0xff;
	pccb->cmd[7] = (unsigned char)(start >> 16) & 0xff;
	pccb->cmd[8] = (unsigned char)(start >> 8) & 0xff;
	pccb->cmd[9] = (unsigned char)start & 0xff;
	pccb->cmd[10] = 0;
	pccb->cmd[11] = (unsigned char)(blocks >> 24) & 0xff;
	pccb->cmd[12] = (unsigned char)(blocks >> 16) & 0xff;
	pccb->cmd[13] = (unsigned char)(blocks >> 8) & 0xff;
	pccb->cmd[14] = (unsigned char)blocks & 0xff;
	pccb->cmd[15] = 0;
	pccb->cmdlen = 16;
	pccb->msgout[0] = SCSI_IDENTIFY; /* NOT USED */
	debug("scsi_setup_read16: cmd: %02X %02X startblk %02X%02X%02X%02X%02X%02X%02X%02X blccnt %02X%02X%02X%02X\n",
	      pccb->cmd[0], pccb->cmd[1],
	      pccb->cmd[2], pccb->cmd[3], pccb->cmd[4], pccb->cmd[5],
	      pccb->cmd[6], pccb->cmd[7], pccb->cmd[8], pccb->cmd[9],
	      pccb->cmd[11], pccb->cmd[12], pccb->cmd[13], pccb->cmd[14]);
}
#endif

static void scsi_setup_inquiry(struct scsi_cmd *pccb)
{
	pccb->cmd[0] = SCSI_INQUIRY;
	pccb->cmd[1] = pccb->lun << 5;
	pccb->cmd[2] = 0;
	pccb->cmd[3] = 0;
	if (pccb->datalen > 255)
		pccb->cmd[4] = 255;
	else
		pccb->cmd[4] = (unsigned char)pccb->datalen;
	pccb->cmd[5] = 0;
	pccb->cmdlen = 6;
	pccb->msgout[0] = SCSI_IDENTIFY; /* NOT USED */
}

static void scsi_setup_read_ext(struct scsi_cmd *pccb, lbaint_t start,
				unsigned short blocks)
{
	pccb->cmd[0] = SCSI_READ10;
	pccb->cmd[1] = 0;
	pccb->cmd[2] = (unsigned char)(start >> 24) & 0xff;
	pccb->cmd[3] = (unsigned char)(start >> 16) & 0xff;
	pccb->cmd[4] = (unsigned char)(start >> 8) & 0xff;
	pccb->cmd[5] = (unsigned char)start & 0xff;
	pccb->cmd[6] = 0;
	pccb->cmd[7] = (unsigned char)(blocks >> 8) & 0xff;
	pccb->cmd[8] = (unsigned char)blocks & 0xff;
	pccb->cmd[9] = 0;
	pccb->cmdlen = 10;
	pccb->msgout[0] = SCSI_IDENTIFY; /* NOT USED */
	debug("scsi_setup_read_ext: cmd: %02X %02X startblk %02X%02X%02X%02X blccnt %02X%02X\n",
	      pccb->cmd[0], pccb->cmd[1],
	      pccb->cmd[2], pccb->cmd[3], pccb->cmd[4], pccb->cmd[5],
	      pccb->cmd[7], pccb->cmd[8]);
}

static void scsi_setup_write_ext(struct scsi_cmd *pccb, lbaint_t start,
				 unsigned short blocks)
{
	pccb->cmd[0] = SCSI_WRITE10;
	pccb->cmd[1] = pccb->lun << 5;
	pccb->cmd[2] = (unsigned char)(start >> 24) & 0xff;
	pccb->cmd[3] = (unsigned char)(start >> 16) & 0xff;
	pccb->cmd[4] = (unsigned char)(start >> 8) & 0xff;
	pccb->cmd[5] = (unsigned char)start & 0xff;
	pccb->cmd[6] = 0;
	pccb->cmd[7] = ((unsigned char)(blocks >> 8)) & 0xff;
	pccb->cmd[8] = (unsigned char)blocks & 0xff;
	pccb->cmd[9] = 0;
	pccb->cmdlen = 10;
	pccb->msgout[0] = SCSI_IDENTIFY;  /* NOT USED */
	debug("%s: cmd: %02X %02X startblk %02X%02X%02X%02X blccnt %02X%02X\n",
	      __func__,
	      pccb->cmd[0], pccb->cmd[1],
	      pccb->cmd[2], pccb->cmd[3], pccb->cmd[4], pccb->cmd[5],
	      pccb->cmd[7], pccb->cmd[8]);
}

ulong scsi_read(struct udevice *dev, lbaint_t blknr, lbaint_t blkcnt,
		       void *buffer)
{
	struct blk_desc *block_dev = dev->bd;
	struct udevice *bdev = dev;
//	struct scsi_plat *uc_plat = &dev->sc_plat;
	lbaint_t start, blks, max_blks;
	unsigned long buf_addr;
	unsigned short smallblks = 0;
	struct scsi_cmd *pccb = (struct scsi_cmd *)&tempccb;

	/* Setup device */
	pccb->target = block_dev->target;
	pccb->lun = block_dev->lun;
	buf_addr = (unsigned long)buffer;
	start = blknr;
	blks = blkcnt;
	max_blks = SCSI_MAX_BLK;

	debug("\nscsi_read: dev %d startblk " LBAF
	      ", blccnt " LBAF " buffer %lx\n",
	      block_dev->devnum, start, blks, (unsigned long)buffer);
	do {
		pccb->pdata = (unsigned char *)buf_addr;
		pccb->dma_dir = DMA_FROM_DEVICE;
#ifdef CONFIG_SYS_64BIT_LBA
		if (start > SCSI_LBA48_READ) {
			unsigned long blocks;
			blocks = min_t(lbaint_t, blks, max_blks);
			pccb->datalen = block_dev->blksz * blocks;
			scsi_setup_read16(pccb, start, blocks);
			start += blocks;
			blks -= blocks;
		} else
#endif
		if (blks > max_blks) {
			pccb->datalen = block_dev->blksz * max_blks;
			smallblks = max_blks;
			scsi_setup_read_ext(pccb, start, smallblks);
			start += max_blks;
			blks -= max_blks;
		} else {
			pccb->datalen = block_dev->blksz * blks;
			smallblks = (unsigned short)blks;
			scsi_setup_read_ext(pccb, start, smallblks);
			start += blks;
			blks = 0;
		}
		debug("scsi_read_ext: startblk " LBAF
		      ", blccnt %x buffer %lX\n",
		      start, smallblks, buf_addr);
		if (scsi_exec(bdev, pccb)) {
			scsi_print_error(pccb);
			//blkcnt -= blks;
			blkcnt = 0;
			break;
		}
		buf_addr += pccb->datalen;
	} while (blks != 0);
	debug("scsi_read_ext: end startblk " LBAF
	      ", blccnt %x buffer %lX\n", start, smallblks, buf_addr);
	return blkcnt;
}

/*******************************************************************************
 * scsi_write
 */

ulong scsi_write(struct udevice *dev, lbaint_t blknr, lbaint_t blkcnt,
			const void *buffer)
{
	struct blk_desc *block_dev = dev->bd;
	struct udevice *bdev = dev;
	struct scsi_plat *uc_plat = &(dev->sc_plat);
	lbaint_t start, blks, max_blks;
	uintptr_t buf_addr;
	unsigned short smallblks;
	struct scsi_cmd *pccb = (struct scsi_cmd *)&tempccb;

	/* Setup device */
	pccb->target = block_dev->target;
	pccb->lun = block_dev->lun;
	buf_addr = (unsigned long)buffer;
	start = blknr;
	blks = blkcnt;
	if (uc_plat->max_bytes_per_req)
		max_blks = uc_plat->max_bytes_per_req / block_dev->blksz;
	else
		max_blks = SCSI_MAX_BLK;

	debug("\n%s: dev %d startblk " LBAF ", blccnt " LBAF " buffer %lx\n",
	      __func__, block_dev->devnum, start, blks, (unsigned long)buffer);
	do {
		pccb->pdata = (unsigned char *)buf_addr;
		pccb->dma_dir = DMA_TO_DEVICE;
		if (blks > max_blks) {
			pccb->datalen = block_dev->blksz * max_blks;
			smallblks = max_blks;
			scsi_setup_write_ext(pccb, start, smallblks);
			start += max_blks;
			blks -= max_blks;
		} else {
			pccb->datalen = block_dev->blksz * blks;
			smallblks = (unsigned short)blks;
			scsi_setup_write_ext(pccb, start, smallblks);
			start += blks;
			blks = 0;
		}
		debug("%s: startblk " LBAF ", blccnt %x buffer %lx\n",
		      __func__, start, smallblks, buf_addr);
		if (scsi_exec(bdev, pccb)) {
			scsi_print_error(pccb);
			blkcnt -= blks;
			break;
		}
		buf_addr += pccb->datalen;
	} while (blks != 0);
	debug("%s: end startblk " LBAF ", blccnt %x buffer %lX\n",
	      __func__, start, smallblks, buf_addr);
	return blkcnt;
}

static void scsi_setup_sync_cache(struct scsi_cmd *pccb)
{
	pccb->cmd[0] = SCSI_SYNC_CACHE;
	/*
	** Leave the rest of the command zero to indicate
	** flush everything.
	**/
	memset(&(pccb->cmd[1]), 0, 9);
	pccb->cmdlen = 10;
	pccb->datalen = 0;
	pccb->msgout[0] = SCSI_IDENTIFY; /* NOT USED */
}

static void scsi_setup_format_unit(struct scsi_cmd *pccb)
{
	pccb->cmd[0] = SCSI_FORMAT;
	/*
	** Leave the rest of the command zero to indicate
	** format all logical unit except rpmb.
	**/
	memset(&(pccb->cmd[1]), 0, 5);
	pccb->cmdlen = 6;
	pccb->datalen = 0;
	pccb->msgout[0] = SCSI_IDENTIFY; /* NOT USED */
}

static void scsi_setup_unmap(struct scsi_cmd *pccb)
{
	memset(&(pccb->cmd[0]), 0, 10);
	pccb->cmd[0] = SCSI_UNMAP;
	pccb->cmd[8] = 24;/**PARAMETER LIST LENGTHPARAMETER LIST LENGTH LSB**/
	pccb->cmdlen = 10;
	pccb->msgout[0] = SCSI_IDENTIFY; /* NOT USED */
}



int scsi_format_unit(struct udevice *dev)
{
	struct blk_desc *block_dev = dev->bd;
	struct udevice *bdev = dev;
	struct scsi_cmd *pccb = (struct scsi_cmd *)&tempccb;
	int i = 0;

	/* Setup device */
	pccb->target = block_dev->target;
	pccb->dma_dir = DMA_NONE;
	pccb->lun = 0xd0;/*ufs device well know unit lun field value**/
	scsi_setup_format_unit(pccb);
	/*Try 2 times to clear unit attention**/
	for (i = 0; i < 3; i++) {
		if (scsi_exec(bdev, pccb)) {
			scsi_print_error(pccb);
		} else {
			break;
		}
	}

	if (i == 3)
		return -EINVAL;

	dev_info(dev->uhb.dev, "format unit\n");
	return 0;
}

int scsi_unmap(struct udevice *dev, u64 start_lba, u32 nr_blocks)
{
	struct blk_desc *block_dev = dev->bd;
	struct udevice *bdev = dev;
	struct scsi_cmd *pccb = (struct scsi_cmd *)&tempccb;
	unsigned char *buf = NULL;

	/* Setup device */
	pccb->target = block_dev->target;
	pccb->lun = 0x0;

	pccb->dma_dir = DMA_TO_DEVICE;
	pccb->datalen = 24;
	pccb->pdata = (unsigned char *)&tempbuff;
	memset(pccb->pdata, 0, 24);
	buf = pccb->pdata;
	/**UNMAP parameter list**/
	put_unaligned_be16(6 + 16, &buf[0]);/*UNMAP DATA LENG*/
	put_unaligned_be16(16, &buf[2]);/*UNMAP BLOCK DESCRIPTOR DATA LENGTH**/
	put_unaligned_be64(start_lba, &buf[8]);/*UNMAP LOGICAL BLOCK ADDRESS*/
	put_unaligned_be32(nr_blocks, &buf[16]);/*NUMBER OF LOGICAL BLOCKS*/

	scsi_setup_unmap(pccb);
	if (scsi_exec(bdev, pccb)) {
		scsi_print_error(pccb);
		dev_err(dev->uhb.dev, "Unmap start %llu, blocks %d, failed\n", start_lba, nr_blocks);
		return -EINVAL;
	}
	dev_info(dev->uhb.dev, "Unmap start %llu, blocks %d\n", start_lba, nr_blocks);
	return 0;
}

int scsi_sync_cache(struct udevice *dev)
{
	struct blk_desc *block_dev = dev->bd;
	struct udevice *bdev = dev;
	struct scsi_cmd *pccb = (struct scsi_cmd *)&tempccb;

	/* Setup device */
	pccb->target = block_dev->target;
	pccb->lun = block_dev->lun;
	pccb->dma_dir = DMA_NONE;
	scsi_setup_sync_cache(pccb);
	if (scsi_exec(bdev, pccb)) {
		scsi_print_error(pccb);
		return -EINVAL;
	}
	dev_info(NULL, "sync cache\n");
	return 0;
}



/* copy src to dest, skipping leading and trailing blanks
 * and null terminate the string
 */
/*
static void scsi_ident_cpy(unsigned char *dest, unsigned char *src,
			   unsigned int len)
{
	int start, end;

	start = 0;
	while (start < len) {
		if (src[start] != ' ')
			break;
		start++;
	}
	end = len-1;
	while (end > start) {
		if (src[end] != ' ')
			break;
		end--;
	}
	for (; start <= end; start++)
		*dest ++= src[start];
	*dest = '\0';
}
*/
#ifdef READ_CAPACITY
int scsi_read_capacity(struct udevice *dev, struct scsi_cmd *pccb,
			      lbaint_t *capacity, unsigned long *blksz)
{
	*capacity = 0;

	memset(pccb->cmd, '\0', sizeof(pccb->cmd));
	pccb->cmd[0] = SCSI_RD_CAPAC10;
	pccb->cmd[1] = pccb->lun << 5;
	pccb->cmdlen = 10;
	pccb->msgout[0] = SCSI_IDENTIFY; /* NOT USED */

	pccb->datalen = 8;
	pccb->dma_dir = DMA_FROM_DEVICE;
	if (scsi_exec(dev, pccb))
		return 1;

	*capacity = ((lbaint_t)pccb->pdata[0] << 24) |
		    ((lbaint_t)pccb->pdata[1] << 16) |
		    ((lbaint_t)pccb->pdata[2] << 8)  |
		    ((lbaint_t)pccb->pdata[3]);

	if (*capacity != 0xffffffff) {
		/* Read capacity (10) was sufficient for this drive. */
		*blksz = ((unsigned long)pccb->pdata[4] << 24) |
			 ((unsigned long)pccb->pdata[5] << 16) |
			 ((unsigned long)pccb->pdata[6] << 8)  |
			 ((unsigned long)pccb->pdata[7]);
		return 0;
	}

	/* Read capacity (10) was insufficient. Use read capacity (16). */
	memset(pccb->cmd, '\0', sizeof(pccb->cmd));
	pccb->cmd[0] = SCSI_RD_CAPAC16;
	pccb->cmd[1] = 0x10;
	pccb->cmdlen = 16;
	pccb->msgout[0] = SCSI_IDENTIFY; /* NOT USED */

	/*ufs read capacity 16 parameter data is 32 byte*/
	pccb->datalen = 32;
	pccb->dma_dir = DMA_FROM_DEVICE;
	if (scsi_exec(dev, pccb))
		return 1;

	*capacity = ((uint64_t)pccb->pdata[0] << 56) |
		    ((uint64_t)pccb->pdata[1] << 48) |
		    ((uint64_t)pccb->pdata[2] << 40) |
		    ((uint64_t)pccb->pdata[3] << 32) |
		    ((uint64_t)pccb->pdata[4] << 24) |
		    ((uint64_t)pccb->pdata[5] << 16) |
		    ((uint64_t)pccb->pdata[6] << 8)  |
		    ((uint64_t)pccb->pdata[7]);

	/*ufs read capacity 16 parameter data logical block length in bytes is only 4 byte*/
	*blksz = ((uint64_t)pccb->pdata[8] << 24) |
			((uint64_t)pccb->pdata[9] << 16) |
			((uint64_t)pccb->pdata[10] << 8)  |
			((uint64_t)pccb->pdata[11]);

	return 0;
}
#endif

/*
 * Some setup (fill-in) routines
 */
static void scsi_setup_test_unit_ready(struct scsi_cmd *pccb)
{
	pccb->cmd[0] = SCSI_TST_U_RDY;
	pccb->cmd[1] = pccb->lun << 5;
	pccb->cmd[2] = 0;
	pccb->cmd[3] = 0;
	pccb->cmd[4] = 0;
	pccb->cmd[5] = 0;
	pccb->cmdlen = 6;
	pccb->msgout[0] = SCSI_IDENTIFY; /* NOT USED */
}

static void scsi_setup_start(struct scsi_cmd *pccb)
{
	pccb->cmd[0] = SCSI_START_STOP;
	pccb->cmd[1] = 1;/* Return immediately */
	pccb->cmd[2] = 0;
	pccb->cmd[3] = 0;
	pccb->cmd[4] = 1;/* Start spin cycle */
	pccb->cmd[5] = 0;
	pccb->cmdlen = 6;
	pccb->msgout[0] = SCSI_IDENTIFY; /* NOT USED */
}

/**
 * scsi_init_dev_desc_priv - initialize only SCSI specific blk_desc properties
 *
 * @dev_desc: Block device description pointer
 */
static void scsi_init_dev_desc_priv(struct blk_desc *dev_desc)
{
	dev_desc->target = 0xff;
	dev_desc->lun = 0xff;
/*	dev_desc->log2blksz =
		LOG2_INVALID(typeof(dev_desc->log2blksz));*/
/*	dev_desc->type = DEV_TYPE_UNKNOWN;
	dev_desc->vendor[0] = 0;
	dev_desc->product[0] = 0;
	dev_desc->revision[0] = 0;
	dev_desc->removable = false;
*/
}

/**
 * scsi_detect_dev - Detect scsi device
 *
 * @target: target id
 * @lun: target lun
 * @dev_desc: block device description
 *
 * The scsi_detect_dev detects and fills a dev_desc structure when the device is
 * detected.
 *
 * Return: 0 on success, error value otherwise
 */
static int scsi_detect_dev(struct udevice *dev, int target, int lun,
			   struct blk_desc *dev_desc)
{
	unsigned char perq = 0;
#ifdef READ_CAPACITY
	lbaint_t capacity = 0;
	unsigned long blksz = 0;
#endif
	struct scsi_cmd *pccb = (struct scsi_cmd *)&tempccb;
	int count = 0, err = 0, spintime = 0, try_cnt = 0;

	pccb->target = target;
	pccb->lun = lun;
	pccb->pdata = (unsigned char *)&tempbuff;
	/* from ufs 3.0 spec:
	 * First 36 bytes are defined for UFS as standard
	 * Requesting 36 bytes will result in the device returning the complete record
	 * the Device Server shall return a number of bytes equal to the minimum between
	 * 36 and the value specified in the Allocation Length
	 */
	pccb->datalen = 36;
	pccb->dma_dir = DMA_FROM_DEVICE;
	scsi_setup_inquiry(pccb);

	/*
	* INQUIRY should not yield UNIT_ATTENTION
	* but many buggy devices do so anyway.(msg from kernel)
	*/
	for (count = 0; count < 3; count++) {
		if (scsi_exec(dev, pccb)) {
			if (pccb->contr_stat == SCSI_SEL_TIME_OUT) {
				/*
				  * selection timeout => assuming no
					* device present
				*/
				debug("Selection timeout ID %d\n",
					pccb->target);
				err =  -ETIMEDOUT;
			}
			scsi_print_error(pccb);
			err =  -ENODEV;
		} else {
			err = 0;
			break;
		}
	}
	if (err) {
		sunxi_ufs_trace_point(SUNXI_UFS_INQ_ERR);
		return err;
	}

	perq = tempbuff[0];
/*	modi = tempbuff[1];*/
	if ((perq & 0x1f) == 0x1f)
		return -ENODEV; /* skip unknown devices */

//	if ((modi & 0x80) == 0x80) /* drive is removable */
//		dev_desc->removable = true;
	/* get info for this device */
/*
	scsi_ident_cpy((unsigned char *)dev_desc->vendor,
		       &tempbuff[8], 8);
	scsi_ident_cpy((unsigned char *)dev_desc->product,
		       &tempbuff[16], 16);
	scsi_ident_cpy((unsigned char *)dev_desc->revision,
		       &tempbuff[32], 4);
*/
	dev_desc->target = pccb->target;
	dev_desc->lun = pccb->lun;

	for (try_cnt = 0; try_cnt < 3; try_cnt++) {
		for (count = 0; count < 3; count++) {
			pccb->datalen = 0;
			pccb->dma_dir = DMA_NONE;
			//printf("dma none\n");
			scsi_setup_test_unit_ready(pccb);
			err = scsi_exec(dev, pccb);
			if (!err)
				break;
		}
		if (err) {
/*
		if (dev_desc->removable) {
			dev_desc->type = perq;
			goto removable;
		}
*/
			/*
			 * According to ufs3.0 7.4.4 logical unit power conditon,
			 * "All logical units shall be in the active power condition
			 * after power on or any type of reset event"
			 * So we no nedd to use start cmd to change lun to active power condition
			 * but kernel code sd_spinup_disk has use start cmd when device sense key is not ready
			 * To be save,add we add start cmd on fist time test unit try faild no matter what error for for simplicity
			 * ,and try 3 times, wait up to 3 second.kernel code wait 100 second,it is too long
			 * for brom.
			 */
			if (!spintime) {
				spintime = 1;
				pccb->datalen = 0;
				scsi_setup_start(pccb);
				scsi_exec(dev, pccb);
			}
			//scsi_print_error(pccb);
			//return -EINVAL;
		} else
			break;
		/* Wait 1 second for next try */

		printf("test unit try again after start cmd\n");
		mdelay(1000);
	}
	if (err) {
		sunxi_ufs_trace_point(SUNXI_UFS_TESTREADY_ERR);
		return -EINVAL;
	}


#ifdef READ_CAPACITY
	if (scsi_read_capacity(dev, pccb, &capacity, &blksz)) {
		scsi_print_error(pccb);
		return -EINVAL;
	}
	/*From ufs spec,read capacity cmd only read :
	 * "last addressable block on medium under control of logical unit (LU)"
	 * not the number of blocks.
	 * Also in kernel,if we not plus one,kernel will report
	 * "GPT:Alternate GPT header not at the end of the disk"**/
	capacity++;
	dev_info(NULL, "blk size %ld,capacity %ld block\n", blksz, capacity);
#endif
	/*no need cheak lba,if over lba,read will failed*/
	//dev_desc->lba = UINT_MAX;
	dev_desc->lba = capacity;
	/*only support 4k block size,ufs use min block size 4k,if over 4k,
	 * should repartition to make ufs device use 4k block size
	 * */
	dev_desc->blksz = 4096;
	/*dev_desc->log2blksz = LOG2(dev_desc->blksz);*/
	dev_desc->type = perq;
	//removable:
	return 0;
}

/*
 * (re)-scan the scsi bus and reports scsi device info
 * to the user if mode = 1
 */
static int do_scsi_scan_one(struct udevice *dev, int id, int lun, bool verbose)
{
	//int ret;
	struct udevice *bdev;
	struct blk_desc bd;
	struct blk_desc *bdesc;
	//char str[10];

	/*
	 * detect the scsi driver to get information about its geometry (block
	 * size, number of blocks) and other parameters (ids, type, ...)
	 */
	scsi_init_dev_desc_priv(&bd);
	if (scsi_detect_dev(dev, id, lun, &bd))
		return -ENODEV;

	/*
	* Create only one block device and do detection
	* to make sure that there won't be a lot of
	* block devices created
	*/
/*
	ret = blk_create_devicef(dev, "scsi_blk", NULL, IF_TYPE_SCSI, -1,
			bd.blksz, bd.lba, &bdev);
	if (ret) {
		debug("Can't create device\n");
		return ret;
	}
*/
	bdev = dev;
	bdesc = bdev->bd;
	bdesc->target = id;
	bdesc->lun = lun;
//	bdesc->removable = bd.removable;
	bdesc->type = bd.type;

	/*the same function like blk_create_devicef at our environment*/
	bdesc->devnum = 0;
	bdesc->blksz = bd.blksz;
	bdesc->lba = bd.lba;
	/*
	memcpy(&bdesc->vendor, &bd.vendor, sizeof(bd.vendor));
	memcpy(&bdesc->product, &bd.product, sizeof(bd.product));
	memcpy(&bdesc->revision, &bd.revision,	sizeof(bd.revision));
	if (IS_ENABLED(CONFIG_SYS_BIG_ENDIAN)) {
		ata_swap_buf_le16((u16 *)&bdesc->vendor, sizeof(bd.vendor) / 2);
		ata_swap_buf_le16((u16 *)&bdesc->product, sizeof(bd.product) / 2);
		ata_swap_buf_le16((u16 *)&bdesc->revision, sizeof(bd.revision) / 2);
	}
*/
	if (verbose) {
		dev_dbg(NULL, "Device %d: \n", bdesc->devnum);
		//dev_print(bdesc);
	}
	return 0;
}

int scsi_scan_dev(struct udevice *dev, bool verbose)
{
	/*struct scsi_plat *uc_plat;*/ /* scsi controller plat */
	int ret = 0;
	int i = 0;
	int lun = 0;

	/* Get controller plat */
	/*uc_plat = dev_get_uclass_plat(dev);*/
	/*uc_plat = dev->uclass_plat_;*/
/*
	for (i = 0; i < uc_plat->max_id; i++)
		for (lun = 0; lun < uc_plat->max_lun; lun++)
*/
			ret = do_scsi_scan_one(dev, i, lun, verbose);

	return ret;
}


/*
int scsi_scan(struct udevice *udev, bool verbose)
{
	struct udevice *dev = udev;
	int ret;

	if (verbose)
		printf("scanning bus for devices...\n");

	ret = scsi_scan_dev(dev, verbose);
	if (ret)
		return ret;

	return 0;
}
*/

