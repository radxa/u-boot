// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2015 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 * Copyright (c) 2016 Xilinx, Inc
 * Written by Michal Simek
 *
 * Based on ahci-uclass.c
 */

#define LOG_CATEGORY UCLASS_SCSI

//#include <types.h>
#include <ufs.h>
#include <./uinclude/scsi.h>
#include <device.h>
#include <./uinclude/blk.h>
#include "errno.h"


int scsi_exec(struct udevice *dev, struct scsi_cmd *pccb)
{
	struct scsi_ops *ops = &(dev->ufs_sc_ops);

	if (!ops->exec)
		return -ENOSYS;

	return ops->exec(dev, pccb);
}

int scsi_bus_reset(struct udevice *dev)
{
	struct scsi_ops *ops = &(dev->ufs_sc_ops);

	if (!ops->bus_reset)
		return -ENOSYS;

	return ops->bus_reset(dev);
}

