# SPDX-License-Identifier: GPL-2.0-or-later
include $(srctree)/${obj}/ksc_reg/ksc110/Makefile
include $(srctree)/${obj}/ksc_reg/ksc100/Makefile
obj-$(CONFIG_SUNXI_KSC) += sunxi_ksc.o

sunxi_ksc-objs := ksc_drv.o ksc.o ksc_mem.o

sunxi_ksc-$(CONFIG_MACH_SUN251IW1) += ksc_reg/ksc110/${ksc110_reg_obj}

sunxi_ksc-$(CONFIG_MACH_SUN50IW15) += ksc_reg/ksc100/${ksc100_reg_obj}


ccflags-y += -I$(srctree)/$(obj)/../common/
