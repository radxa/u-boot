/*
 * (C) Copyright 2024 Rockchip Electronics Co., Ltd.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#ifndef _ASM_ARCH_IOC_RV1103B_H
#define _ASM_ARCH_IOC_RV1103B_H

#include <common.h>

/* pmuio0_ioc register structure define */
struct rv1103b_pmuio0_ioc_reg {
     uint32_t gpio0a_iomux_sel_0;                 /* address offset: 0x0000 */
     uint32_t gpio0a_iomux_sel_1;                 /* address offset: 0x0004 */
     uint32_t reserved0008[62];                   /* address offset: 0x0008 */
     uint32_t gpio0a_ds_0;                        /* address offset: 0x0100 */
     uint32_t gpio0a_ds_1;                        /* address offset: 0x0104 */
     uint32_t gpio0a_ds_2;                        /* address offset: 0x0108 */
     uint32_t gpio0a_ds_3;                        /* address offset: 0x010c */
     uint32_t reserved0110[60];                   /* address offset: 0x0110 */
     uint32_t gpio0a_pull;                        /* address offset: 0x0200 */
     uint32_t reserved0204[63];                   /* address offset: 0x0204 */
     uint32_t gpio0a_ie;                          /* address offset: 0x0300 */
     uint32_t reserved0304[63];                   /* address offset: 0x0304 */
     uint32_t gpio0a_smt;                         /* address offset: 0x0400 */
     uint32_t reserved0404[63];                   /* address offset: 0x0404 */
     uint32_t gpio0a_sus;                         /* address offset: 0x0500 */
     uint32_t reserved0504[63];                   /* address offset: 0x0504 */
     uint32_t gpio0a_sl;                          /* address offset: 0x0600 */
     uint32_t reserved0604[63];                   /* address offset: 0x0604 */
     uint32_t gpio0a_od;                          /* address offset: 0x0700 */
     uint32_t reserved0704[63];                   /* address offset: 0x0704 */
     uint32_t io_vsel;                            /* address offset: 0x0800 */
     uint32_t grf_jtag_con0;                      /* address offset: 0x0804 */
     uint32_t grf_jtag_con1;                      /* address offset: 0x0808 */
     uint32_t reserved080c[61];                   /* address offset: 0x080c */
     uint32_t xin_con;                            /* address offset: 0x0900 */
};

check_member(rv1103b_pmuio0_ioc_reg, xin_con, 0x0900);

/* pmuio1_ioc register structure define */
struct rv1103b_pmuio1_ioc_reg {
     uint32_t reserved0000[2];                    /* address offset: 0x0000 */
     uint32_t gpio0b_iomux_sel_0;                 /* address offset: 0x0008 */
     uint32_t gpio0b_iomux_sel_1;                 /* address offset: 0x000c */
     uint32_t reserved0010[64];                   /* address offset: 0x0010 */
     uint32_t gpio0b_ds_0;                        /* address offset: 0x0110 */
     uint32_t gpio0b_ds_1;                        /* address offset: 0x0114 */
     uint32_t gpio0b_ds_2;                        /* address offset: 0x0118 */
     uint32_t reserved011c[58];                   /* address offset: 0x011c */
     uint32_t gpio0b_pull;                        /* address offset: 0x0204 */
     uint32_t reserved0208[63];                   /* address offset: 0x0208 */
     uint32_t gpio0b_ie;                          /* address offset: 0x0304 */
     uint32_t reserved0308[63];                   /* address offset: 0x0308 */
     uint32_t gpio0b_smt;                         /* address offset: 0x0404 */
     uint32_t reserved0408[63];                   /* address offset: 0x0408 */
     uint32_t gpio0b_sus;                         /* address offset: 0x0504 */
     uint32_t reserved0508[63];                   /* address offset: 0x0508 */
     uint32_t gpio0b_sl;                          /* address offset: 0x0604 */
     uint32_t reserved0608[63];                   /* address offset: 0x0608 */
     uint32_t gpio0b_od;                          /* address offset: 0x0704 */
     uint32_t reserved0708[62];                   /* address offset: 0x0708 */
     uint32_t io_vsel;                            /* address offset: 0x0800 */
     uint32_t grf_jtag_con0;                      /* address offset: 0x0804 */
     uint32_t grf_jtag_con1;                      /* address offset: 0x0808 */
};

check_member(rv1103b_pmuio1_ioc_reg, grf_jtag_con1, 0x0808);

/* vccio3_ioc register structure define */
struct rv1103b_vccio3_ioc_reg {
     uint32_t reserved0000[8];                    /* address offset: 0x0000 */
     uint32_t gpio1a_iomux_sel_0;                 /* address offset: 0x0020 */
     uint32_t gpio1a_iomux_sel_1;                 /* address offset: 0x0024 */
     uint32_t reserved0028[70];                   /* address offset: 0x0028 */
     uint32_t gpio1a_ds_0;                        /* address offset: 0x0140 */
     uint32_t gpio1a_ds_1;                        /* address offset: 0x0144 */
     uint32_t gpio1a_ds_2;                        /* address offset: 0x0148 */
     uint32_t reserved014c[49];                   /* address offset: 0x014c */
     uint32_t gpio1a_pull;                        /* address offset: 0x0210 */
     uint32_t reserved0214[63];                   /* address offset: 0x0214 */
     uint32_t gpio1a_ie;                          /* address offset: 0x0310 */
     uint32_t reserved0314[63];                   /* address offset: 0x0314 */
     uint32_t gpio1a_smt;                         /* address offset: 0x0410 */
     uint32_t reserved0414[63];                   /* address offset: 0x0414 */
     uint32_t gpio1a_sus;                         /* address offset: 0x0510 */
     uint32_t reserved0514[63];                   /* address offset: 0x0514 */
     uint32_t gpio1a_sl;                          /* address offset: 0x0610 */
     uint32_t reserved0614[63];                   /* address offset: 0x0614 */
     uint32_t gpio1a_od;                          /* address offset: 0x0710 */
     uint32_t reserved0714[59];                   /* address offset: 0x0714 */
     uint32_t io_vsel_vccio3;                     /* address offset: 0x0800 */
};

check_member(rv1103b_vccio3_ioc_reg, io_vsel_vccio3, 0x0800);

/* vccio4_ioc register structure define */
struct rv1103b_vccio4_ioc_reg {
     uint32_t reserved0000[9];                    /* address offset: 0x0000 */
     uint32_t gpio1a_iomux_sel_1;                 /* address offset: 0x0024 */
     uint32_t gpio1b_iomux_sel_0;                 /* address offset: 0x0028 */
     uint32_t gpio1b_iomux_sel_1;                 /* address offset: 0x002c */
     uint32_t reserved0030[71];                   /* address offset: 0x0030 */
     uint32_t gpio1a_ds_3;                        /* address offset: 0x014c */
     uint32_t gpio1b_ds_0;                        /* address offset: 0x0150 */
     uint32_t gpio1b_ds_1;                        /* address offset: 0x0154 */
     uint32_t gpio1b_ds_2;                        /* address offset: 0x0158 */
     uint32_t reserved015c[45];                   /* address offset: 0x015c */
     uint32_t gpio1a_pull;                        /* address offset: 0x0210 */
     uint32_t gpio1b_pull;                        /* address offset: 0x0214 */
     uint32_t reserved0218[62];                   /* address offset: 0x0218 */
     uint32_t gpio1a_ie;                          /* address offset: 0x0310 */
     uint32_t gpio1b_ie;                          /* address offset: 0x0314 */
     uint32_t reserved0318[62];                   /* address offset: 0x0318 */
     uint32_t gpio1a_smt;                         /* address offset: 0x0410 */
     uint32_t gpio1b_smt;                         /* address offset: 0x0414 */
     uint32_t reserved0418[62];                   /* address offset: 0x0418 */
     uint32_t gpio1a_sus;                         /* address offset: 0x0510 */
     uint32_t gpio1b_sus;                         /* address offset: 0x0514 */
     uint32_t reserved0518[62];                   /* address offset: 0x0518 */
     uint32_t gpio1a_sl;                          /* address offset: 0x0610 */
     uint32_t gpio1b_sl;                          /* address offset: 0x0614 */
     uint32_t reserved0618[62];                   /* address offset: 0x0618 */
     uint32_t gpio1a_od;                          /* address offset: 0x0710 */
     uint32_t gpio1b_od;                          /* address offset: 0x0714 */
     uint32_t reserved0718[58];                   /* address offset: 0x0718 */
     uint32_t io_vsel_vccio4;                     /* address offset: 0x0800 */
};

check_member(rv1103b_vccio4_ioc_reg, io_vsel_vccio4, 0x0800);

/* vccio6_ioc register structure define */
struct rv1103b_vccio6_ioc_reg {
     uint32_t reserved0000[16];                   /* address offset: 0x0000 */
     uint32_t gpio2a_iomux_sel_0;                 /* address offset: 0x0040 */
     uint32_t gpio2a_iomux_sel_1;                 /* address offset: 0x0044 */
     uint32_t gpio2b_iomux_sel_0;                 /* address offset: 0x0048 */
     uint32_t reserved004c[77];                   /* address offset: 0x004c */
     uint32_t gpio2a_ds_0;                        /* address offset: 0x0180 */
     uint32_t gpio2a_ds_1;                        /* address offset: 0x0184 */
     uint32_t gpio2a_ds_2;                        /* address offset: 0x0188 */
     uint32_t gpio2a_ds_3;                        /* address offset: 0x018c */
     uint32_t gpio2b_ds_0;                        /* address offset: 0x0190 */
     uint32_t gpio2b_ds_1;                        /* address offset: 0x0194 */
     uint32_t reserved0198[34];                   /* address offset: 0x0198 */
     uint32_t gpio2a_pull;                        /* address offset: 0x0220 */
     uint32_t gpio2b_pull;                        /* address offset: 0x0224 */
     uint32_t reserved0228[62];                   /* address offset: 0x0228 */
     uint32_t gpio2a_ie;                          /* address offset: 0x0320 */
     uint32_t gpio2b_ie;                          /* address offset: 0x0324 */
     uint32_t reserved0328[62];                   /* address offset: 0x0328 */
     uint32_t gpio2a_smt;                         /* address offset: 0x0420 */
     uint32_t gpio2b_smt;                         /* address offset: 0x0424 */
     uint32_t reserved0428[62];                   /* address offset: 0x0428 */
     uint32_t gpio2a_sus;                         /* address offset: 0x0520 */
     uint32_t gpio2b_sus;                         /* address offset: 0x0524 */
     uint32_t reserved0528[62];                   /* address offset: 0x0528 */
     uint32_t gpio2a_sl;                          /* address offset: 0x0620 */
     uint32_t gpio2b_sl;                          /* address offset: 0x0624 */
     uint32_t reserved0628[62];                   /* address offset: 0x0628 */
     uint32_t gpio2a_od;                          /* address offset: 0x0720 */
     uint32_t gpio2b_od;                          /* address offset: 0x0724 */
     uint32_t reserved0728[54];                   /* address offset: 0x0728 */
     uint32_t io_vsel_vccio6;                     /* address offset: 0x0800 */
     uint32_t misc_con;                           /* address offset: 0x0804 */
     uint32_t reserved0808;                       /* address offset: 0x0808 */
     uint32_t saradc_con0;                        /* address offset: 0x080c */
     uint32_t saradc_con1;                        /* address offset: 0x0810 */
};

check_member(rv1103b_vccio6_ioc_reg, saradc_con1, 0x0810);

/* vccio7_ioc register structure define */
struct rv1103b_vccio7_ioc_reg {
     uint32_t reserved0000[11];                   /* address offset: 0x0000 */
     uint32_t gpio1b_iomux_sel_1;                 /* address offset: 0x002c */
     uint32_t gpio1c_iomux_sel_0;                 /* address offset: 0x0030 */
     uint32_t gpio1c_iomux_sel_1;                 /* address offset: 0x0034 */
     uint32_t gpio1d_iomux_sel_0;                 /* address offset: 0x0038 */
     uint32_t gpio1d_iomux_sel_1;                 /* address offset: 0x003c */
     uint32_t reserved0040[70];                   /* address offset: 0x0040 */
     uint32_t gpio1b_ds_2;                        /* address offset: 0x0158 */
     uint32_t gpio1b_ds_3;                        /* address offset: 0x015c */
     uint32_t gpio1c_ds_0;                        /* address offset: 0x0160 */
     uint32_t reserved0164[44];                   /* address offset: 0x0164 */
     uint32_t gpio1b_pull;                        /* address offset: 0x0214 */
     uint32_t gpio1c_pull;                        /* address offset: 0x0218 */
     uint32_t reserved021c[62];                   /* address offset: 0x021c */
     uint32_t gpio1b_ie;                          /* address offset: 0x0314 */
     uint32_t gpio1c_ie;                          /* address offset: 0x0318 */
     uint32_t reserved031c[62];                   /* address offset: 0x031c */
     uint32_t gpio1b_smt;                         /* address offset: 0x0414 */
     uint32_t gpio1c_smt;                         /* address offset: 0x0418 */
     uint32_t reserved041c[62];                   /* address offset: 0x041c */
     uint32_t gpio1b_sus;                         /* address offset: 0x0514 */
     uint32_t gpio1c_sus;                         /* address offset: 0x0518 */
     uint32_t reserved051c[62];                   /* address offset: 0x051c */
     uint32_t gpio1b_sl;                          /* address offset: 0x0614 */
     uint32_t gpio1c_sl;                          /* address offset: 0x0618 */
     uint32_t reserved061c[62];                   /* address offset: 0x061c */
     uint32_t gpio1b_od;                          /* address offset: 0x0714 */
     uint32_t gpio1c_od;                          /* address offset: 0x0718 */
     uint32_t reserved071c[58];                   /* address offset: 0x071c */
     uint32_t io_vsel_vccio7;                     /* address offset: 0x0804 */
     uint32_t misc_con;                           /* address offset: 0x0808 */
     uint32_t sdcard_io_con;                      /* address offset: 0x080c */
     uint32_t jtag_m2_con;                        /* address offset: 0x0810 */
};

check_member(rv1103b_vccio7_ioc_reg, jtag_m2_con, 0x0810);

#endif /* _ASM_ARCH_IOC_RV1103B_H */
