#
# (C) Copyright 2024 Rockchip Electronics Co., Ltd
#
# SPDX-License-Identifier:     GPL-2.0+
#

ifneq ($(CONFIG_TPL_BUILD)$(CONFIG_TPL_TINY_FRAMEWORK),yy)
ifneq ($(CONFIG_SPL_BUILD)$(CONFIG_SPL_DECOMP_HEADER),yy)
obj-y += syscon_rv1103b.o
endif
endif
obj-y += rv1103b.o
ifndef CONFIG_TPL_BUILD
obj-y += clk_rv1103b.o
endif
