/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <malloc.h>
#include <asm/arch/dma.h>
#include <asm/arch/gic.h>
#include <asm/arch/clock.h>
#include <asm/io.h>

#if 0
#define dma_dbg(fmt, arg...)	printf("[%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##arg)
#define sunxi_boot_dma_debug
#else
#define dma_dbg(fmt, arg...)
#endif

#ifdef CONFIG_MACH_SUN8IW18
#define SUNXI_DMA_MAX     10
#elif CONFIG_MACH_SUN60IW2
#define SUNXI_DMA_MAX	  16
#else
#define SUNXI_DMA_MAX     4
#endif

#define DMA_VERSION_REG_V2 0x30003

static int dma_int_cnt = 0;
static int dma_init = -1;
static sunxi_dma_source   dma_channal_source[SUNXI_DMA_MAX];


static void sunxi_dma_reg_func(void *p)
{
	int i;
	uint pending;
	unsigned int ver_reg;
	sunxi_dma_reg *dma_reg = (sunxi_dma_reg *)SUNXI_DMA_BASE;


	ver_reg = readl(&dma_reg->version);

	if (ver_reg < DMA_VERSION_REG_V2) {
		for (i = 0; i < 8 && i < SUNXI_DMA_MAX; i++) {
			pending = (DMA_PKG_END_INT << (i * 4));
			if (readl(&dma_reg->irq_pending0) & pending) {
				writel(pending, &dma_reg->irq_pending0);
				if (dma_channal_source[i].dma_func.m_func != NULL)
					dma_channal_source[i].dma_func.m_func(dma_channal_source[i].dma_func.m_data);
			}
		}
		for (i = 8; i < SUNXI_DMA_MAX; i++) {
			pending = (DMA_PKG_END_INT << ((i - 8) * 4));
			if (readl(&dma_reg->irq_pending1) & pending) {
				writel(pending, &dma_reg->irq_pending1);
				if (dma_channal_source[i].dma_func.m_func != NULL)
					dma_channal_source[i].dma_func.m_func(dma_channal_source[i].dma_func.m_data);
			}
		}
	} else if (ver_reg >= DMA_VERSION_REG_V2) {
#if defined(DMA_VERSION_V2)
		for (i = 0; i < SUNXI_DMA_MAX; i++) {
			if (readl(&(dma_reg->channal[i].irq_pend_reg)) & DMA_PKG_END_INT) {
				writel(DMA_PKG_END_INT, &(dma_reg->channal[i].irq_pend_reg));
				if (dma_channal_source[i].dma_func.m_func != NULL)
					dma_channal_source[i].dma_func.m_func(dma_channal_source[i].dma_func.m_data);
			}
		}
#endif
	}
}

void sunxi_dma_init(void)
{
	int i;
	unsigned int ver_reg;
	sunxi_dma_reg *const dma_reg = (sunxi_dma_reg *)SUNXI_DMA_BASE;
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

	if (dma_init > 0)
		return ;

#if defined(CONFIG_SUNXI_VERSION1)
	setbits_le32(&ccm->ahb_gate0, 1 << AHB_GATE_OFFSET_DMA);
	setbits_le32(&ccm->ahb_reset0_cfg, 1 << AHB_GATE_OFFSET_DMA);

#elif defined(CONFIG_MACH_SUN60IW2)

	/* dma : mbus gate enable reg */
	setbits_le32(&ccm->mbus_gate_en_reg, 1 << 0);

	/* dma reset */
	setbits_le32(&ccm->dma_gate_reset, 1 << DMA_RST_OFS);

	/* dma gating */
	setbits_le32(&ccm->dma_gate_reset, 1 << DMA_GATING_OFS);
#else
	/* dma : mbus clock gating */
	setbits_le32(&ccm->mbus_gate, 1 << 0);

	/* dma reset */
	setbits_le32(&ccm->dma_gate_reset, 1 << DMA_RST_OFS);

	/* dma gating */
	setbits_le32(&ccm->dma_gate_reset, 1 << DMA_GATING_OFS);

#if defined(CONFIG_SOUND_SUNXI_DSP_DMAC)

	sunxi_dsp_ccu_reg *const dsp_ccm = (sunxi_dsp_ccu_reg *)SUNXI_DSP_CCM_BASE;

	/* dma : mbus clock gating */
	setbits_le32(&dsp_ccm->mclk_gating_cfg_reg, 1 << 0);  // dma mclk enable

	setbits_le32(&dsp_ccm->mclk_gating_cfg_reg, 1 << 1);  // dsp mclk enable

	/* dma reset */
	setbits_le32(&dsp_ccm->dsp_dma_brg_reg, 1 << DMA_RST_OFS);

	/* dma gating */
	setbits_le32(&dsp_ccm->dsp_dma_brg_reg, 1 << DMA_GATING_OFS);
#endif

#endif

	ver_reg = readl(&dma_reg->version);
	if (ver_reg == 0) {
		printf("The sunxi dma version = 0, please check the error!\n");
		return;
	}

	if (ver_reg < DMA_VERSION_REG_V2) {
		writel(0, &dma_reg->irq_en0);
		writel(0, &dma_reg->irq_en1);

		writel(0xffffffff, &dma_reg->irq_pending0);
		writel(0xffffffff, &dma_reg->irq_pending1);
	}

	/*auto MCLK gating  disable*/
	clrsetbits_le32(&dma_reg->auto_gate, 0x7 << 0, 0x7 << 0);

	memset((void *)dma_channal_source, 0, SUNXI_DMA_MAX * sizeof(struct sunxi_dma_source_t));

	for (i = 0; i < SUNXI_DMA_MAX; i++) {
#if defined(DMA_VERSION_V2)
		if (ver_reg >= DMA_VERSION_REG_V2) {
			writel(0, &(dma_reg->channal[i].irq_en_reg));
			writel(0xf, &(dma_reg->channal[i].irq_pend_reg));
		}
#endif

		dma_channal_source[i].used = 0;
		dma_channal_source[i].channal = &(dma_reg->channal[i]);
		dma_channal_source[i].desc  =
			(sunxi_dma_desc *)malloc_align(sizeof(sunxi_dma_desc), CONFIG_SYS_CACHELINE_SIZE);
	}

	irq_install_handler(AW_IRQ_DMA, sunxi_dma_reg_func, 0);

	dma_int_cnt = 0;
	dma_init = 1;

	return ;
}

void sunxi_dma_exit(void)
{
	int i;
	ulong hdma;
	unsigned int ver_reg;
	sunxi_dma_reg *dma_reg = (sunxi_dma_reg *)SUNXI_DMA_BASE;
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

	ver_reg = readl(&dma_reg->version);

	/* free dma channal if other module not free it */
	for (i = 0; i < SUNXI_DMA_MAX; i++) {
		if (dma_channal_source[i].used == 1) {
			hdma = (ulong)&dma_channal_source[i];
			sunxi_dma_disable_int(hdma);
			sunxi_dma_free_int(hdma);
			writel(0, &dma_channal_source[i].channal->enable);
			dma_channal_source[i].used   = 0;
		}
	}
	/* irq disable */
	if (ver_reg < DMA_VERSION_REG_V2) {
		writel(0, &dma_reg->irq_en0);
		writel(0, &dma_reg->irq_en1);

		writel(0xffffffff, &dma_reg->irq_pending0);
		writel(0xffffffff, &dma_reg->irq_pending1);
	} else if (ver_reg >= DMA_VERSION_REG_V2) {
#if defined(DMA_VERSION_V2)
		for (i = 0; i < SUNXI_DMA_MAX; i++) {
			writel(0, &(dma_reg->channal[i].irq_en_reg));
			writel(0xf, &(dma_reg->channal[i].irq_pend_reg));
		}
#endif
	}

	irq_free_handler(AW_IRQ_DMA);

#if defined(CONFIG_SUNXI_VERSION1)
	clrbits_le32(&ccm->ahb_gate0, 1 << AHB_GATE_OFFSET_DMA);
#else
	/* close dma clock when dma exit */
	clrbits_le32(&ccm->dma_gate_reset, 1 << DMA_GATING_OFS | 1 << DMA_RST_OFS);
#endif

	dma_init--;

	dma_dbg("dma exit\n");

}

ulong sunxi_dma_request_from_last(uint dmatype)
{
	int   i;

	for (i = SUNXI_DMA_MAX - 1; i >= 0; i--) {
		if (dma_channal_source[i].used == 0) {
			dma_channal_source[i].used = 1;
			dma_channal_source[i].channal_count = i;
			return (ulong)&dma_channal_source[i];
		}
	}

	return 0;
}

ulong sunxi_dma_request(uint dmatype)
{
	int   i;

	for (i = 0; i < SUNXI_DMA_MAX; i++) {
		if (dma_channal_source[i].used == 0) {
			dma_channal_source[i].used = 1;
			dma_channal_source[i].channal_count = i;
			return (ulong)&dma_channal_source[i];
		}
	}

	return 0;
}

int sunxi_dma_release(ulong hdma)
{
	struct sunxi_dma_source_t  *dma_source = (struct sunxi_dma_source_t *)hdma;

	if (!dma_source->used)
		return -1;

	sunxi_dma_disable_int(hdma);
	sunxi_dma_free_int(hdma);

	dma_source->used   = 0;

	return 0;
}


int sunxi_dma_setting(ulong hdma, sunxi_dma_set *cfg)
{

	uint   commit_para;
	sunxi_dma_set     *dma_set = cfg;
	sunxi_dma_source  *dma_source = (sunxi_dma_source *)hdma;
	sunxi_dma_desc    *desc = dma_source->desc;
	uint channal_addr  = (ulong)(&(dma_set->channal_cfg));

	if (!dma_source->used)
		return -1;

	if (dma_set->loop_mode)
		desc->link = (ulong)(&dma_source->desc);
	else
		desc->link = SUNXI_DMA_LINK_NULL;

	commit_para  = (dma_set->wait_cyc & 0xff);
	commit_para |= (dma_set->data_block_size & 0xff) << 8;

	writel(commit_para, &desc->commit_para);
	writel(readl((volatile void __iomem *)(ulong)channal_addr), &desc->config);

	dma_dbg("config [%x] commit_para[%x] \n", readl(&desc->config), readl(&desc->commit_para));

	return 0;
}


int sunxi_dma_start(ulong hdma, uint saddr, uint daddr, uint bytes)
{
	sunxi_dma_source  	  *dma_source = (sunxi_dma_source *)hdma;
	sunxi_dma_channal_reg *channal = dma_source->channal;
	sunxi_dma_desc    *desc = dma_source->desc;

	if (!dma_source->used)
		return -1;

	/*config desc */
	writel(saddr, &desc->source_addr);
	writel(daddr, &desc->dest_addr);
	writel(bytes, &desc->byte_count);

	flush_cache((ulong)desc,
		    ALIGN(sizeof(sunxi_dma_desc), CONFIG_SYS_CACHELINE_SIZE));

	dma_dbg("source_addr [%x] dest_addr [%x]  byte_count [%x]\n", readl(&desc->source_addr), readl(&desc->dest_addr), readl(&desc->byte_count));

	/* start dma */
	writel((ulong)(desc), &channal->desc_addr);
	writel(1, &channal->enable);

#if defined(sunxi_boot_dma_debug)

	sunxi_dma_reg *const dma_reg = (sunxi_dma_reg *)SUNXI_DMA_BASE;
	unsigned int ver_reg;
	int i = 0;

	ver_reg = readl(&dma_reg->version);

	if (ver_reg < DMA_VERSION_REG_V2) {
		for (i = 0; i < 5; i++) {

			dma_dbg("desc [%p], channal [%p],  dma_reg [%p], auto_gate addr [%p], desc_addr addr [%p] \n \
					irq_en0 [%x], irq_pending0 [%x], auto_gate [%x], status [%x] \n  \
					enable [%x], desc_addr [%x], config [%x], cur_src_addr [%x]   \n  \
					cur_dst_addr [%x], left_bytes [%x], parameters [%x], mode [%x]\n", \
					desc,  channal, dma_reg, &dma_reg->auto_gate, &channal->desc_addr, \
					readl(&dma_reg->irq_en0), readl(&dma_reg->irq_pending0), readl(&dma_reg->auto_gate), readl(&dma_reg->status), \
					readl(&channal->enable), readl(&channal->desc_addr), readl(&channal->config), readl(&channal->cur_src_addr), \
					readl(&channal->cur_dst_addr), readl(&channal->left_bytes), readl(&channal->parameters), readl(&channal->mode));

			mdelay(200);
		}
	} else if (ver_reg >= DMA_VERSION_REG_V2) {
#if defined(DMA_VERSION_V2)
		for (i = 0; i < 5; i++) {

			dma_dbg("desc [%p], channal [%p],  dma_reg [%p], auto_gate addr [%p], desc_addr addr [%p] \n \
					irq_en_reg [%x], irq_pend_reg [%x], auto_gate [%x], status [%x] \n  \
					enable [%x], desc_addr [%x], config [%x], cur_src_addr [%x]   \n  \
					cur_dst_addr [%x], left_bytes [%x], parameters [%x], mode [%x]\n", \
					desc,  channal, dma_reg, &dma_reg->auto_gate, &channal->desc_addr, \
					readl(&channal->irq_en_reg), readl(&channal->irq_pend_reg), readl(&dma_reg->auto_gate), readl(&dma_reg->status), \
					readl(&channal->enable), readl(&channal->desc_addr), readl(&channal->config), readl(&channal->cur_src_addr), \
					readl(&channal->cur_dst_addr), readl(&channal->left_bytes), readl(&channal->parameters), readl(&channal->mode));

			mdelay(200);
		}
#endif
	}
#endif

	return 0;
}

int sunxi_dma_stop(ulong hdma)
{
	sunxi_dma_source *dma_source = (sunxi_dma_source *)hdma;
	sunxi_dma_channal_reg *channal = dma_source->channal;

	if (!dma_source->used)
		return -1;
	writel(0, &channal->enable);

	dma_dbg("dma stop\n");

	return 0;
}

int sunxi_dma_querystatus(ulong hdma)
{
	uint  channal_count;
	sunxi_dma_source *dma_source = (sunxi_dma_source *)hdma;
	sunxi_dma_reg    *dma_reg = (sunxi_dma_reg *)SUNXI_DMA_BASE;

	if (!dma_source->used)
		return -1;

	channal_count = dma_source->channal_count;

	return (readl(&dma_reg->status) >> channal_count) & 0x01;
}

int sunxi_dma_install_int(ulong hdma, interrupt_handler_t dma_reg_func, void *p)
{
	sunxi_dma_source     *dma_channal = (sunxi_dma_source *)hdma;
	sunxi_dma_reg    *dma_reg  = (sunxi_dma_reg *)SUNXI_DMA_BASE;
	uint  channal_count;
	unsigned int ver_reg;

	if (!dma_channal->used)
		return -1;

	ver_reg = readl(&dma_reg->version);

	channal_count = dma_channal->channal_count;

	if (ver_reg < DMA_VERSION_REG_V2) {
		if (channal_count < 8)
			writel((7 << channal_count * 4), &dma_reg->irq_pending0);
		else
			writel((7 << (channal_count - 8) * 4), &dma_reg->irq_pending1);
	} else if (ver_reg >= DMA_VERSION_REG_V2) {
#if defined(DMA_VERSION_V2)
		sunxi_dma_channal_reg *channal = dma_channal->channal;
		writel(7, &channal->irq_pend_reg);
#endif
	}

	if (!dma_channal->dma_func.m_func) {
		dma_channal->dma_func.m_func = dma_reg_func;
		dma_channal->dma_func.m_data = p;
	} else {
		printf("dma 0x%lx int is used already, you have to free it first\n", hdma);
	}

	return 0;
}

int sunxi_dma_enable_int(ulong hdma)
{
	sunxi_dma_source     *dma_channal = (sunxi_dma_source *)hdma;
	sunxi_dma_reg    *dma_status  = (sunxi_dma_reg *)SUNXI_DMA_BASE;
	uint  channal_count;
	unsigned int ver_reg;

	if (!dma_channal->used)
		return -1;

	ver_reg = readl(&dma_status->version);

	channal_count = dma_channal->channal_count;

	if (ver_reg < DMA_VERSION_REG_V2) {
		if (channal_count < 8) {
			if (readl(&dma_status->irq_en0) & (DMA_PKG_END_INT << channal_count * 4)) {
				printf("dma 0x%lx int is avaible already\n", hdma);
				return 0;
			}
			setbits_le32(&dma_status->irq_en0, (DMA_PKG_END_INT << channal_count * 4));
		} else {
			if (readl(&dma_status->irq_en1) & (DMA_PKG_END_INT << (channal_count - 8) * 4)) {
				printf("dma 0x%lx int is avaible already\n", hdma);
				return 0;
			}
			setbits_le32(&dma_status->irq_en1, (DMA_PKG_END_INT << (channal_count - 8) * 4));
		}
	}

	if (ver_reg >= DMA_VERSION_REG_V2) {
#if defined(DMA_VERSION_V2)
		sunxi_dma_channal_reg *channal = dma_channal->channal;
		if (readl(&channal->irq_en_reg) & DMA_PKG_END_INT) {
			printf("dma 0x%lx int is avaible already\n", hdma);
			return 0;
		}
		setbits_le32(&channal->irq_en_reg, DMA_PKG_END_INT);
#endif
	}

	if (!dma_int_cnt)
		irq_enable(AW_IRQ_DMA);

	dma_int_cnt ++;

	return 0;
}

int sunxi_dma_disable_int(ulong hdma)
{
	sunxi_dma_source     *dma_channal = (sunxi_dma_source *)hdma;
	sunxi_dma_reg    *dma_reg  = (sunxi_dma_reg *)SUNXI_DMA_BASE;
	uint  channal_count;
	unsigned int ver_reg;

	if (!dma_channal->used)
		return -1;

	ver_reg = readl(&dma_reg->version);

	channal_count = dma_channal->channal_count;

	if (ver_reg < DMA_VERSION_REG_V2) {
		if (channal_count < 8) {
			if (!(readl(&dma_reg->irq_en0) & (DMA_PKG_END_INT << channal_count * 4))) {
				debug("dma 0x%lx int is not used yet\n", hdma);
				return 0;
			}
			clrbits_le32(&dma_reg->irq_en0, (DMA_PKG_END_INT << channal_count * 4));
		} else {
			if (!(readl((volatile void __iomem *)(ulong)dma_reg->irq_en1) & (DMA_PKG_END_INT << (channal_count - 8) * 4))) {
				debug("dma 0x%lx int is not used yet\n", hdma);
				return 0;
			}
			clrbits_le32(&dma_reg->irq_en1, (DMA_PKG_END_INT << (channal_count - 8) * 4));
		}
	}

	if (ver_reg >= DMA_VERSION_REG_V2) {
#if defined(DMA_VERSION_V2)
		sunxi_dma_channal_reg *channal = dma_channal->channal;
		if (!(readl(&channal->irq_en_reg) & DMA_PKG_END_INT)) {
			debug("dma 0x%lx int is not used yet!\n", hdma);
			return 0;
		}
		clrbits_le32(&channal->irq_en_reg, DMA_PKG_END_INT);
#endif
	}

	//disable golbal int
	if (dma_int_cnt > 0)
		dma_int_cnt --;

	if (!dma_int_cnt)
		irq_disable(AW_IRQ_DMA);

	return 0;
}

int sunxi_dma_free_int(ulong hdma)
{
	sunxi_dma_source     *dma_channal = (sunxi_dma_source *)hdma;
	sunxi_dma_reg    *dma_status  = (sunxi_dma_reg *)SUNXI_DMA_BASE;
	sunxi_dma_reg *const dma_reg = (sunxi_dma_reg *)SUNXI_DMA_BASE;
	uint  channal_count;
	unsigned int ver_reg;

	if (!dma_channal->used)
		return -1;

	ver_reg = readl(&dma_reg->version);

	channal_count = dma_channal->channal_count;

	if (ver_reg < DMA_VERSION_REG_V2) {
		if (channal_count < 8)
			writel((7 << channal_count), &dma_status->irq_pending0);
		else
			writel((7 << (channal_count - 8)), &dma_status->irq_pending1);
	} else if (ver_reg >= DMA_VERSION_REG_V2) {
#if defined(DMA_VERSION_V2)
		sunxi_dma_channal_reg *channal = dma_channal->channal;
		writel(0, &channal->irq_pend_reg);
#endif
	}

	if (dma_channal->dma_func.m_func) {
		dma_channal->dma_func.m_func = NULL;
		dma_channal->dma_func.m_data = NULL;
	} else {
		dma_dbg("dma 0x%lx int is free, you do not need to free it again\n", hdma);
		return -1;
	}

	return 0;
}


