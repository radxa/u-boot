/*
 * (C) Copyright 2024 Rockchip Electronics Co., Ltd.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#ifndef _ASM_ARCH_GRF_RV1103B_H
#define _ASM_ARCH_GRF_RV1103B_H

#include <common.h>

/*
 * You can choose:
 * (1) Directly use concrete grf reg, like struct rv1103b_cpu_grf_reg.
 * (2) Add regs you need to struct rv1103b_grf, use rv1103b_grf directly.
 */
#define VEPU_GRF	0x20100000
#define NPU_GRF		0x20110000
#define VI_GRF		0x20120000
#define CPU_GRF		0x20130000
#define DDR_GRF		0x20140000
#define SYS_GRF		0x20150000
#define PMU_GRF		0x20160000
struct rv1103b_grf {
     uint32_t reserved0[(SYS_GRF + 0xA0 - VEPU_GRF)/ 4];
     uint32_t gmac_con0;                          /* address offset: 0x00a0 */
     uint32_t gmac_clk_con;                       /* address offset: 0x00a4 */
     uint32_t gmac_st;                            /* address offset: 0x00a8 */
     uint32_t reserved00ac;                       /* address offset: 0x00ac */
     uint32_t macphy_con0;                        /* address offset: 0x00b0 */
     uint32_t macphy_con1;                        /* address offset: 0x00b4 */
     uint32_t reserved1[(PMU_GRF + 0x10000 - (SYS_GRF + 0xB4)) / 4];
};

check_member(rv1103b_grf, macphy_con1, SYS_GRF + 0xB4 - VEPU_GRF);

/* grf_cpu register structure define */
struct rv1103b_cpu_grf_reg {
     uint32_t con0;                               /* address offset: 0x0000 */
     uint32_t mem_cfg_uhdspra;                    /* address offset: 0x0004 */
     uint32_t status;                             /* address offset: 0x0008 */
};

check_member(rv1103b_cpu_grf_reg, status, 0x0008);

/* grf_ddr register structure define */
struct rv1103b_ddr_grf_reg {
     uint32_t con0;                               /* address offset: 0x0000 */
     uint32_t con1;                               /* address offset: 0x0004 */
     uint32_t con2;                               /* address offset: 0x0008 */
     uint32_t con3;                               /* address offset: 0x000c */
     uint32_t con4;                               /* address offset: 0x0010 */
     uint32_t con5;                               /* address offset: 0x0014 */
     uint32_t con6;                               /* address offset: 0x0018 */
     uint32_t con7;                               /* address offset: 0x001c */
     uint32_t con8;                               /* address offset: 0x0020 */
     uint32_t con9;                               /* address offset: 0x0024 */
     uint32_t con10;                              /* address offset: 0x0028 */
     uint32_t con11;                              /* address offset: 0x002c */
     uint32_t con12;                              /* address offset: 0x0030 */
     uint32_t con13;                              /* address offset: 0x0034 */
     uint32_t con14;                              /* address offset: 0x0038 */
     uint32_t reserved003c[17];                   /* address offset: 0x003c */
     uint32_t probe_ctrl;                         /* address offset: 0x0080 */
     uint32_t reserved0084[39];                   /* address offset: 0x0084 */
     uint32_t status8;                            /* address offset: 0x0120 */
     uint32_t status9;                            /* address offset: 0x0124 */
};

check_member(rv1103b_ddr_grf_reg, status9, 0x0124);

/* grf_npu register structure define */
struct rv1103b_npu_grf_reg {
     uint32_t mem_con_spra;                       /* address offset: 0x0000 */
};

check_member(rv1103b_npu_grf_reg, mem_con_spra, 0x0000);

/* grf_pmu register structure define */
struct rv1103b_pmu_grf_reg {
     uint32_t soc_con0;                           /* address offset: 0x0000 */
     uint32_t soc_con1;                           /* address offset: 0x0004 */
     uint32_t soc_con2;                           /* address offset: 0x0008 */
     uint32_t soc_con3;                           /* address offset: 0x000c */
     uint32_t soc_con4;                           /* address offset: 0x0010 */
     uint32_t soc_con5;                           /* address offset: 0x0014 */
     uint32_t soc_con6;                           /* address offset: 0x0018 */
     uint32_t soc_con7;                           /* address offset: 0x001c */
     uint32_t soc_con8;                           /* address offset: 0x0020 */
     uint32_t soc_con9;                           /* address offset: 0x0024 */
     uint32_t soc_con10;                          /* address offset: 0x0028 */
     uint32_t reserved002c;                       /* address offset: 0x002c */
     uint32_t soc_status0;                        /* address offset: 0x0030 */
     uint32_t reserved0034[3];                    /* address offset: 0x0034 */
     uint32_t men_con;                            /* address offset: 0x0040 */
     uint32_t reserved0044[3];                    /* address offset: 0x0044 */
     uint32_t soc_special0;                       /* address offset: 0x0050 */
     uint32_t reserved0054[3];                    /* address offset: 0x0054 */
     uint32_t soc_preroll_int_con;                /* address offset: 0x0060 */
     uint32_t reserved0064[103];                  /* address offset: 0x0064 */
     uint32_t os_reg0;                            /* address offset: 0x0200 */
     uint32_t os_reg1;                            /* address offset: 0x0204 */
     uint32_t os_reg2;                            /* address offset: 0x0208 */
     uint32_t os_reg3;                            /* address offset: 0x020c */
     uint32_t os_reg4;                            /* address offset: 0x0210 */
     uint32_t os_reg5;                            /* address offset: 0x0214 */
     uint32_t os_reg6;                            /* address offset: 0x0218 */
     uint32_t os_reg7;                            /* address offset: 0x021c */
     uint32_t os_reg8;                            /* address offset: 0x0220 */
     uint32_t os_reg9;                            /* address offset: 0x0224 */
     uint32_t os_reg10;                           /* address offset: 0x0228 */
     uint32_t os_reg11;                           /* address offset: 0x022c */
     uint32_t reset_function_status;              /* address offset: 0x0230 */
     uint32_t reset_function_clr;                 /* address offset: 0x0234 */
};

check_member(rv1103b_pmu_grf_reg, reset_function_clr, 0x0234);

/* grf_sys register structure define */
struct rv1103b_sys_grf_reg {
     uint32_t peri_con0;                          /* address offset: 0x0000 */
     uint32_t peri_con1;                          /* address offset: 0x0004 */
     uint32_t peri_con2;                          /* address offset: 0x0008 */
     uint32_t peri_hprot2_con;                    /* address offset: 0x000c */
     uint32_t peri_status;                        /* address offset: 0x0010 */
     uint32_t reserved0014[3];                    /* address offset: 0x0014 */
     uint32_t audio_con0;                         /* address offset: 0x0020 */
     uint32_t audio_con1;                         /* address offset: 0x0024 */
     uint32_t reserved0028[2];                    /* address offset: 0x0028 */
     uint32_t usbotg_con0;                        /* address offset: 0x0030 */
     uint32_t usbotg_con1;                        /* address offset: 0x0034 */
     uint32_t reserved0038[2];                    /* address offset: 0x0038 */
     uint32_t usbotg_status0;                     /* address offset: 0x0040 */
     uint32_t usbotg_status1;                     /* address offset: 0x0044 */
     uint32_t usbotg_status2;                     /* address offset: 0x0048 */
     uint32_t reserved004c;                       /* address offset: 0x004c */
     uint32_t usbphy_con0;                        /* address offset: 0x0050 */
     uint32_t usbphy_con1;                        /* address offset: 0x0054 */
     uint32_t usbphy_con2;                        /* address offset: 0x0058 */
     uint32_t usbphy_con3;                        /* address offset: 0x005c */
     uint32_t usbphy_status;                      /* address offset: 0x0060 */
     uint32_t reserved0064[3];                    /* address offset: 0x0064 */
     uint32_t saradc_con;                         /* address offset: 0x0070 */
     uint32_t tsadc_con;                          /* address offset: 0x0074 */
     uint32_t otp_con;                            /* address offset: 0x0078 */
     uint32_t reserved007c;                       /* address offset: 0x007c */
     uint32_t mem_con_spra;                       /* address offset: 0x0080 */
     uint32_t mem_con_dpra;                       /* address offset: 0x0084 */
     uint32_t mem_con_rom;                        /* address offset: 0x0088 */
     uint32_t mem_con_gate;                       /* address offset: 0x008c */
     uint32_t biu_con0;                           /* address offset: 0x0090 */
     uint32_t reserved0094;                       /* address offset: 0x0094 */
     uint32_t biu_status0;                        /* address offset: 0x0098 */
     uint32_t biu_status1;                        /* address offset: 0x009c */
     uint32_t gmac_con0;                          /* address offset: 0x00a0 */
     uint32_t gmac_clk_con;                       /* address offset: 0x00a4 */
     uint32_t gmac_st;                            /* address offset: 0x00a8 */
     uint32_t reserved00ac;                       /* address offset: 0x00ac */
     uint32_t macphy_con0;                        /* address offset: 0x00b0 */
     uint32_t macphy_con1;                        /* address offset: 0x00b4 */
     uint32_t reserved00b8[18];                   /* address offset: 0x00b8 */
     uint32_t usbotg_sig_detect_con;              /* address offset: 0x0100 */
     uint32_t usbotg_sig_detect_status;           /* address offset: 0x0104 */
     uint32_t usbotg_sig_detect_clr;              /* address offset: 0x0108 */
     uint32_t reserved010c;                       /* address offset: 0x010c */
     uint32_t usbotg_linestate_detect_con;        /* address offset: 0x0110 */
     uint32_t usbotg_disconnect_detect_con;       /* address offset: 0x0114 */
     uint32_t usbotg_bvalid_detect_con;           /* address offset: 0x0118 */
     uint32_t usbotg_id_detect_con;               /* address offset: 0x011c */
     uint32_t reserved0120[56];                   /* address offset: 0x0120 */
     uint32_t cache_peri_addr_start;              /* address offset: 0x0200 */
     uint32_t cache_peri_addr_end;                /* address offset: 0x0204 */
     uint32_t hpmcu_code_addr_start;              /* address offset: 0x0208 */
     uint32_t hpmcu_sram_addr_start;              /* address offset: 0x020c */
     uint32_t hpmcu_exsram_addr_start;            /* address offset: 0x0210 */
     uint32_t hpmcu_cache_misc;                   /* address offset: 0x0214 */
     uint32_t hpmcu_cache_status;                 /* address offset: 0x0218 */
     uint32_t reserved021c[377];                  /* address offset: 0x021c */
     uint32_t chip_id;                            /* address offset: 0x0800 */
     uint32_t chip_version;                       /* address offset: 0x0804 */
};

check_member(rv1103b_sys_grf_reg, chip_version, 0x0804);

/* grf_vepu register structure define */
struct rv1103b_vepu_grf_reg {
     uint32_t mem_con_spra;                       /* address offset: 0x0000 */
     uint32_t mem_con_dpra;                       /* address offset: 0x0004 */
};

check_member(rv1103b_vepu_grf_reg, mem_con_dpra, 0x0004);

/* grf_vi register structure define */
struct rv1103b_vi_grf_reg {
     uint32_t mem_con_spra;                       /* address offset: 0x0000 */
     uint32_t mem_con_dpra;                       /* address offset: 0x0004 */
     uint32_t reserved0008;                       /* address offset: 0x0008 */
     uint32_t vi_hprot2_con;                      /* address offset: 0x000c */
     uint32_t status;                             /* address offset: 0x0010 */
     uint32_t csiphy_con;                         /* address offset: 0x0014 */
     uint32_t csiphy_status;                      /* address offset: 0x0018 */
     uint32_t reserved001c;                       /* address offset: 0x001c */
     uint32_t misc_con;                           /* address offset: 0x0020 */
     uint32_t sdmmc_det_cnt;                      /* address offset: 0x0024 */
     uint32_t sdmmc_sig_detect_con;               /* address offset: 0x0028 */
     uint32_t sdmmc_sig_detect_status;            /* address offset: 0x002c */
     uint32_t sdmmc_status_clr;                   /* address offset: 0x0030 */
};

check_member(rv1103b_vi_grf_reg, sdmmc_status_clr, 0x0030);

#endif /*  _ASM_ARCH_GRF_RV1103B_H  */
