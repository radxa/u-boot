// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2007-2031
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Aaron <leafy.myeh@allwinnertech.com>
 *
 * UFS driver for allwinner sunxi platform.
 */
#ifndef _LINUX_DMA_DIRECTION_H
#define _LINUX_DMA_DIRECTION_H
/*
 * These definitions mirror those in pci.h, so they can be used
 * interchangeably with their PCI_ counterparts.
 */
enum dma_data_direction {
	DMA_BIDIRECTIONAL = 0,
	DMA_TO_DEVICE = 1,
	DMA_FROM_DEVICE = 2,
	DMA_NONE = 3,
};
#endif
