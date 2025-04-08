# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2006
#lixiang <lixiang@allwinnertech.com>


ccflags-y += -I$(src)/uinclude/linux/
ccflags-y += -I$(src)/uinclude/dm/
ccflags-y += -I$(src)/uinclude/
ccflags-y += -I$(src)/
ccflags-y += -I$(src)/trace/


obj-$(CONFIG_UFS) += ufs.o
obj-$(CONFIG_UFS) 		+= sunxi-ufs-platform.o ufs_main.o ufs-uclass.o
obj-$(CONFIG_UFS)                 += scsi/scsi-uclass.o
obj-$(CONFIG_UFS)                 += scsi/scsi.o
obj-$(CONFIG_UFS)                 += trace/sunxi-ufs-trace.o


