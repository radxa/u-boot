/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2013 Google, Inc
 *
 * (C) Copyright 2012
 * Pavel Herrmann <morpheus.ibis@gmail.com>
 * Marek Vasut <marex@denx.de>
 */

#ifndef SUNXI_UFS_DM_DEVICE_H
#define SUNXI_UFS_DM_DEVICE_H

/**
 * struct udevice - An instance of a driver
 *
 * This holds information about a device, which is a driver bound to a
 * particular port or peripheral (essentially a driver instance).
 *
 * A device will come into existence through a 'bind' call, either due to
 * a U_BOOT_DRVINFO() macro (in which case plat is non-NULL) or a node
 * in the device tree (in which case of_offset is >= 0). In the latter case
 * we translate the device tree information into plat in a function
 * implemented by the driver of_to_plat method (called just before the
 * probe method if the device has a device tree node.
 *
 * All three of plat, priv and uclass_priv can be allocated by the
 * driver, or you can use the auto members of struct driver and
 * struct uclass_driver to have driver model do this automatically.
 *
 * @driver: The driver used by this device
 * @name: Name of device, typically the FDT node name
 * @plat_: Configuration data for this device (do not access outside driver
 *	model)
 * @parent_plat_: The parent bus's configuration data for this device (do not
 *	access outside driver model)
 * @uclass_plat_: The uclass's configuration data for this device (do not access
 *	outside driver model)
 * @driver_data: Driver data word for the entry that matched this device with
 *		its driver
 * @parent: Parent of this device, or NULL for the top level device
 * @priv_: Private data for this device (do not access outside driver model)
 * @uclass: Pointer to uclass for this device
 * @uclass_priv_: The uclass's private data for this device (do not access
 *	outside driver model)
 * @parent_priv_: The parent's private data for this device (do not access
 *	outside driver model)
 * @uclass_node: Used by uclass to link its devices
 * @child_head: List of children of this device
 * @sibling_node: Next device in list of all devices
 * @flags_: Flags for this device `DM_FLAG_...` (do not access outside driver
 *	model)
 * @seq_: Allocated sequence number for this device (-1 = none). This is set up
 * when the device is bound and is unique within the device's uclass. If the
 * device has an alias in the devicetree then that is used to set the sequence
 * number. Otherwise, the next available number is used. Sequence numbers are
 * used by certain commands that need device to be numbered (e.g. 'mmc dev').
 * (do not access outside driver model)
 * @node_: Reference to device tree node for this device (do not access outside
 *	driver model)
 * @devres_head: List of memory allocations associated with this device.
 *		When CONFIG_DEVRES is enabled, devm_kmalloc() and friends will
 *		add to this list. Memory so-allocated will be freed
 *		automatically when the device is removed / unbound
 * @dma_offset: Offset between the physical address space (CPU's) and the
 *		device's bus address space
 */
struct udevice {
/*
    const struct driver *driver;
*/
/*
	void *plat_;
	void *parent_plat_;
	void *uclass_plat_;
	ulong driver_data;
	struct udevice *parent;
	void *priv_;
	struct uclass *uclass;
*/
/*
	struct udevice *parent;
	void *uclass_priv_;
	void *uclass_plat_;
	struct udevice *child;
*/
    /*
	void *parent_priv_;
	struct list_head uclass_node;
	struct list_head child_head;
	struct list_head sibling_node;
#if !CONFIG_IS_ENABLED(OF_PLATDATA_RT)
	u32 flags_;
#endif
	int seq_;
#if CONFIG_IS_ENABLED(OF_REAL)
	ofnode node_;
#endif
#ifdef CONFIG_DEVRES
	struct list_head devres_head;
#endif
#if CONFIG_IS_ENABLED(DM_DMA)
	ulong dma_offset;
#endif
*/
	struct ufs_hba uhba;
	struct ufs_hba_ops sunxi_pltfm_hba_ops;

	struct scsi_plat sc_plat;
	struct scsi_ops ufs_sc_ops;

	void *bd;
	void *scsi_blk_ops;
};
#endif
