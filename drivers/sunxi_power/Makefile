# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (c) 2009 Wind River Systems, Inc.
# Tom Rix <Tom.Rix at windriver.com>
obj-$(CONFIG_SUNXI_POWER)   	+= axp.o
obj-$(CONFIG_SUNXI_PMU)		+= pmu.o
obj-$(CONFIG_SUNXI_BMU)		+= bmu.o
obj-$(CONFIG_SUNXI_PMU_EXT)	+= pmu_ext.o
obj-$(CONFIG_SUNXI_PMU_GENERAL)	+= pmu_general.o
obj-$(CONFIG_SUNXI_BMU_EXT)	+= bmu_ext.o
#PMU
ifdef CONFIG_SUNXI_PMU
obj-$(CONFIG_AXPNULL_POWER)	+= pmu_axpnull.o
obj-$(CONFIG_AXP858_POWER)      += pmu_axp858.o
obj-$(CONFIG_AXP2101_POWER)      += pmu_axp2101.o
obj-$(CONFIG_AXP2202_POWER)      += pmu_axp2202.o
obj-$(CONFIG_AXP8191_POWER)      += pmu_axp8191.o
obj-$(CONFIG_SUNXI_AXP152_POWER)      += pmu_axp152.o
obj-$(CONFIG_AXP1530_POWER)      += pmu_axp1530.o
obj-$(CONFIG_AXP81X_POWER)	+= pmu_axp81X.o
obj-$(CONFIG_AXP806_POWER)      += pmu_axp806.o
obj-$(CONFIG_AXP221_POWER_SUNXI)      += pmu_axp221.o
endif
#BMU
ifdef CONFIG_SUNXI_BMU
obj-$(CONFIG_AXP515_POWER)     += bmu_axp515.o
obj-$(CONFIG_AXP517_POWER)     += bmu_axp517.o
obj-$(CONFIG_AXP2101_POWER)      += bmu_axp2101.o
obj-$(CONFIG_AXP2202_POWER)      += bmu_axp2202.o
obj-$(CONFIG_AXP81X_POWER)      += bmu_axp81X.o
obj-$(CONFIG_SUNXI_AXP152_POWER)	+= bmu_axp152.o
obj-$(CONFIG_AXPNULL_POWER)     += bmu_axpnull.o
obj-$(CONFIG_AXP221_POWER_SUNXI)      += bmu_axp221.o
obj-$(CONFIG_AXP2601_POWER)      += bmu_axp2601.o
endif
#PMU_EXT
ifdef CONFIG_SUNXI_PMU_EXT
obj-$(CONFIG_TCS4838_POWER)      += pmu_tcs4838.o
obj-$(CONFIG_SY8827G_POWER)      += pmu_sy8827g.o
obj-$(CONFIG_AXP1530_POWER)      += pmu_axp1530_ext.o
endif
#PMU_GENERAL
ifdef CONFIG_SUNXI_PMU_GENERAL
obj-$(CONFIG_OCP2131_POWER)      += pmu_ocp2131.o
obj-$(CONFIG_AW37501_POWER)      += pmu_aw37501.o
endif
#BMU_EXT
ifdef CONFIG_SUNXI_BMU_EXT
obj-$(CONFIG_ETA6973_POWER)      += bmu_eta6973.o
obj-$(CONFIG_AXP519_POWER)      += bmu_axp519.o
endif
