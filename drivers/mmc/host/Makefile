# SPDX-License-Identifier: GPL-2.0+
#
#
obj-$(CONFIG_MMC_SUNXI)		+= sunxi_mmc_host_common.o
obj-$(CONFIG_MACH_SUN8IW21)		+= sun8iw21_unique.o
obj-y				+= sunxi_mmc_host_tm0.o sunxi_mmc_host_tm1.o \
 				   sunxi_mmc_host_tm4.o sunxi_mmc_host_tm5.o \
 				   sunxi_mmc_clk_origin.o
ifndef CONFIG_MACH_SUN8IW20
obj-y 				+= sunxi_mmc_clk_common.o sunxi_mmc_clk_typ1.o
endif
obj-$(CONFIG_SUNXI_FPGA_PLATFORM) += sunxi_mmc_clk_fpga.o
