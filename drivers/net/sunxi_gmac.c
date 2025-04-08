/*
* Allwinner GMAC driver.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#include <linux/types.h>
#include <common.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <memalign.h>
#include <net.h>
#include <clk/clk.h>
#include <malloc.h>
#include <linux/mii.h>
#include <netdev.h>
#include <errno.h>
#include <sys_config.h>
#include <linux/string.h>
#include <fdt_support.h>
#include <fdtdec.h>
#include <miiphy.h>
#include <phy.h>

#if defined(CONFIG_MACH_SUN55IW3)
#define SUNXI_GMAC_SYSCFG	SUNXI_SYSCTRL_BASE
#define SUNXI_GMAC_BASE		SUNXI_GMAC0_BASE
#else
#define SUNXI_GMAC_SYSCFG	SUNXI_SRAMC_BASE
#endif

#define SYSCFG_PHY_CLK		0x30
#define GMAC_CLK_REG		0x097c
#define GMAC_25M_CLK_REG	0x0970
#define GMAC_RST_BIT		16
#define GMAC_GATE_BIT		0
#define GMAC_25M_GATE_BIT	31

#define GMAC_BASIC_CTL0		0x00
#define GMAC_BASIC_CTL1		0x04
#define GMAC_INT_STA		0x08
#define GMAC_INT_EN		0x0C
#define GMAC_TX_CTL0		0x10
#define GMAC_TX_CTL1		0x14
#define GMAC_TX_FLOW_CTL	0x1C
#define GMAC_TX_DESC_LIST	0x20
#define GMAC_RX_CTL0		0x24
#define GMAC_RX_CTL1		0x28
#define GMAC_RX_DESC_LIST	0x34
#define GMAC_RX_FRM_FLT		0x38
#define GMAC_RX_HASH0		0x40
#define GMAC_RX_HASH1		0x44
#define GMAC_MDIO_ADDR		0x48
#define GMAC_MDIO_DATA		0x4C
#define GMAC_ADDR_HI(reg)	(0x50 + ((reg) << 3))
#define GMAC_ADDR_LO(reg)	(0x54 + ((reg) << 3))
#define GMAC_TX_DMA_STA		0xB0
#define GMAC_TX_CUR_DESC	0xB4
#define GMAC_TX_CUR_BUF		0xB8
#define GMAC_RX_DMA_STA		0xC0
#define GMAC_RX_CUR_DESC	0xC4
#define GMAC_RX_CUR_BUF		0xC8
#define GMAC_RGMII_STA		0xD0
#define MII_BUSY		0x00000001
#define MII_WRITE		0x00000002

#define GMAC_PHY_RGMII_MASK	0x00000004
#define GMAC_ETCS_RMII_MASK	0x00002003
#define GMAC_RGMII_INTCLK_MASK	0x00000002
#define GMAC_RMII_MASK		0x00002000
#define GMAC_TX_DELAY_MASK	0x07
#define GMAC_TX_DELAY_OFFSET	10
#define GMAC_RX_DELAY_MASK	0x1F
#define GMAC_RX_DELAY_OFFSET	5

#define GMAC_SOFT_RST		0x01
#define GMAC_TX_FRM_LEN_OFFSET	30
#define	GMAC_TX_EN_OFFSET	31
#define GMAC_TX_TH_128		0x100
#define GMAC_CHECK_CRC_OFFSET	27
#define GMAC_RX_EN_OFFSET	28
#define GMAC_RX_MODE		0x02
#define GMAC_RX_RECV_ALL	0x1
#define GMAC_BURST_LEN		0x8
#define GMAC_BURST_LEN_OFFSET	24
#define GMAC_RX_INT_MASK	0x2300
#define GMAC_RX_CLEAR_MASK	0x3F00
#define GMAC_PHY_SELECT_OFFSET	15
#define GMAC_TX_DMA_STOP	0x0
#define GMAC_TX_DMA_SUSPEND	0x6
#define GMAC_TX_DMA_MASK	0x7
#define GMAC_DMA_OWN_DESC	0x80000000
#define GMAC_TX_DMA_ONE_DESC	0x61000000
#define GMAC_TX_DMA_CRC_FULL	(0x3 << 27)
#define GMAC_TX_DMA_BUFF_SIZE(length)	(((1 << 11) -1) & length)
#define GMAC_TX_DMA_FLUSH_FIFO	0x00000001
#define GMAC_TX_DMA_EN		0x40000000
#define GMAC_TX_DMA_START	0x80000000
#define GMAC_RX_DMA_INT_CTL	0x81000000
#define GMAC_RX_DMA_BUFF_SIZE_MASK	((1 << 11) -1)
#define GMAC_RX_DMA_STOP	0x0
#define GMAC_RX_DMA_EN		0x40000000
#define GMAC_RX_DMA_START	0x80000000
#define GMAC_RX_DMA_MASK	0x7

#define GMAC_MDIO_MDC_DIV	(0x06 << 20)
#define GMAC_MDIO_PHYADDR_MASK	0x0001F000
#define GMAC_MDIO_PHYADDR_OFFSET	12
#define GMAC_MDIO_PHYREG_MASK	0x000007F0
#define GMAC_MDIO_PHYREG_OFFSET		4

#define GMAC_MAC_DUPLEX_MASK	0x01
#define GMAC_MAC_SPEED_MASK	0x0c
#define GMAC_MAC_SPEED_100M	0x0c
#define GMAC_MAC_SPEED_1000M	0x08

#define GMAC_PHY_ADDR_MAX	32
#define GMAC_PHY_REG_MASK	0xFFFF
#define GMAC_PHY_UNUSE_ID	0x1FFFFFFF

#ifndef SZ_2K
#define SZ_2K			0x00000800
#endif

enum rx_frame_status { /* IPC status */
	good_frame = 0,
	discard_frame = 1,
	csum_none = 2,
	llc_snap = 4,
};

typedef union {
	struct {
		/* Tx descriptor0 */
		u32 deferred:1;         /* Deferred bit (only half-duplex) */
		u32 under_err:1;        /* Underflow error */
		u32 ex_deferral:1;      /* Excessive deferral */
		u32 coll_cnt:4;         /* Collision count */
		u32 vlan_tag:1;         /* VLAN Frame */

		u32 ex_coll:1;          /* Excessive collision */
		u32 late_coll:1;        /* Late collision */
		u32 no_carr:1;          /* No carrier */
		u32 loss_carr:1;        /* Loss of collision */

		u32 ipdat_err:1;        /* IP payload error */
		u32 frm_flu:1;          /* Frame flushed */
		u32 jab_timeout:1;      /* Jabber timeout */
		u32 err_sum:1;          /* Error summary */

		u32 iphead_err:1;       /* IP header error */
		u32 ttss:1;             /* Transmit time stamp status */
		u32 reserved0:13;
		u32 own:1;              /* Own bit. CPU:0, DMA:1 */
	} tx;

	struct {
		/* Rx desctriptor0 */
		u32 chsum_err:1;        /* Payload checksum error */
		u32 crc_err:1;          /* CRC error */
		u32 dribbling:1;        /* Dribble bit error */
		u32 mii_err:1;          /* Received error (bit3) */

		u32 recv_wt:1;          /* Received watchdog timeout */
		u32 frm_type:1;         /* Frame type */
		u32 late_coll:1;        /* Late Collision */
		u32 ipch_err:1;         /* IPv header checksum error (bit7) */

		u32 last_desc:1;        /* Laset descriptor */
		u32 first_desc:1;       /* First descriptor */
		u32 vlan_tag:1;         /* VLAN Tag */
		u32 over_err:1;         /* Overflow error (bit11) */

		u32 len_err:1;          /* Length error */
		u32 sou_filter:1;       /* Source address filter fail */
		u32 desc_err:1;         /* Descriptor error */
		u32 err_sum:1;          /* Error summary (bit15) */

		u32 frm_len:14;         /* Frame length */
		u32 des_filter:1;       /* Destination address filter fail */
		u32 own:1;              /* Own bit. CPU:0, DMA:1 */
	} rx;

	u32 all;
} desc0_u;

typedef union {
	struct {
		/* TDES1 */
		u32 buf1_size:11;       /* Transmit buffer1 size */
		u32 buf2_size:11;       /* Transmit buffer2 size */
		u32 ttse:1;             /* Transmit time stamp enable */
		u32 dis_pad:1;          /* Disable pad (bit23) */

		u32 adr_chain:1;        /* Second address chained */
		u32 end_ring:1;         /* Transmit end of ring */
		u32 crc_dis:1;          /* Disable CRC */
		u32 cic:2;              /* Checksum insertion control (bit27:28) */
		u32 first_sg:1;         /* First Segment */
		u32 last_seg:1;         /* Last Segment */
		u32 interrupt:1;        /* Interrupt on completion */
	} tx;

	struct {
		/* RDES1 */
		u32 buf1_size:11;       /* Received buffer1 size */
		u32 buf2_size:11;       /* Received buffer2 size */
		u32 reserved1:2;

		u32 adr_chain:1;        /* Second address chained */
		u32 end_ring:1;         /* Received end of ring */
		u32 reserved2:5;
		u32 dis_ic:1;           /* Disable interrupt on completion */
	} rx;

	u32 all;
} desc1_u;

typedef struct dma_desc {
       desc0_u desc0;	/* 1st: Status */
       desc1_u desc1;	/* 2nd: Buffer Size */
       u32 *desc2;	/* 3rd: Buffer Addr */
       u32 *desc3;	/* 4th: Next Desc */
} __attribute__((packed)) dma_desc_t;

struct sunxi_gmac {
	struct dma_desc *dma_desc_tx;
	struct dma_desc *dma_desc_rx;
	unsigned char *rx_packet;
	unsigned char *rx_backup_packet;
	phy_interface_t phy_interface;

#define SUNXI_EXTERNAL_PHY	1
#define SUNXI_INTERNAL_PHY	0
	u32 phy_type;		/* 0:internel phy, 1: externel phy */

#define SUNXI_PHY_USE_CLK25M	0
#define SUNXI_PHY_USE_EXT_OSC	1
	u32 phy_clk_type;	/* 0:phy clk use soc 25m, 1:phy clk use osc25m */

	int gmac_node;
	u32 tx_delay;
	u32 rx_delay;
};

/* #define PKT_DEBUG */
#ifdef PKT_DEBUG
static void pkt_hex_dump(char *prefix_str, void *pkt, unsigned int len)
{
	int i;
	unsigned char *data = (unsigned char *)pkt;
	for (i = 0; i < len; i++) {
		if (!(i % 16))
			printf("\n%s %08x:", prefix_str, i);

		printf(" %02x", data[i]);
	}
	printf("\n");
}
#else
#define pkt_hex_dump(a, b, c)  {}
#endif

static u16 __sunxi_gmac_phy_read(struct eth_device *dev, u8 phy_addr, u16 reg)
{
	u32 value = 0;
	int timeout;

	value |= GMAC_MDIO_MDC_DIV;
	value |= (((phy_addr << GMAC_MDIO_PHYADDR_OFFSET) & GMAC_MDIO_PHYADDR_MASK) |
			((reg << GMAC_MDIO_PHYREG_OFFSET) & GMAC_MDIO_PHYREG_MASK) |
					MII_BUSY);

	writel(value, (void *)(u32)(dev->iobase + GMAC_MDIO_ADDR));

	timeout = get_timer(0) + 5 * CONFIG_SYS_HZ;
	while (readl((void *)(u32)(dev->iobase + GMAC_MDIO_ADDR)) & MII_BUSY) {
		if (get_timer(0) > timeout) {
			printf("Error: %s Mii operation timeout\n", __func__);
			break;
		}
	}

	return readl((void *)(u32)(dev->iobase + GMAC_MDIO_DATA));
}

static void __sunxi_gmac_phy_write(struct eth_device *dev, u8 phy_addr, u8 reg, u16 data)
{
	u32 value = 0;
	int timeout;

	value |= GMAC_MDIO_MDC_DIV;
	value |= (((phy_addr << GMAC_MDIO_PHYADDR_OFFSET) & GMAC_MDIO_PHYADDR_MASK) |
			((reg << GMAC_MDIO_PHYREG_OFFSET) & GMAC_MDIO_PHYREG_MASK) |
					MII_WRITE | MII_BUSY);

	writel(data, (void *)(u32)dev->iobase + GMAC_MDIO_DATA);
	writel(value, (void *)(u32)dev->iobase + GMAC_MDIO_ADDR);

	/* Until operation is complete and exiting */
	timeout = get_timer(0) + 5 * CONFIG_SYS_HZ;
	while (readl((void *)(u32)(dev->iobase + GMAC_MDIO_ADDR)) & MII_BUSY) {
		if (get_timer(0) > timeout) {
			printf("Error: %s Mii operation timeout\n", __func__);
			break;
		}
	}
}

int sunxi_gmac_phy_read(const char *devname, u8 phy_adr, u8 reg, u16 *value)
{
	struct eth_device *dev;

	dev = eth_get_dev_by_name(devname);
	*value = __sunxi_gmac_phy_read(dev, phy_adr, reg);

	return 0;
}

int sunxi_gmac_phy_write(const char *devname, u8 phy_adr, u8 reg, u16 data)
{
	struct eth_device *dev;

	dev = eth_get_dev_by_name(devname);
	__sunxi_gmac_phy_write(dev, phy_adr, reg, data);

	return 0;
}

static int sunxi_gmac_hardware_init(struct sunxi_gmac *chip)
{
	u32 value;
	int ret;

	value = readl(SUNXI_GMAC_SYSCFG + SYSCFG_PHY_CLK);

	/* Write phy type */
	if (chip->phy_type == SUNXI_EXTERNAL_PHY)
		value &= ~(1 << GMAC_PHY_SELECT_OFFSET);
	else
		value |= (1 << GMAC_PHY_SELECT_OFFSET);

	/* Only support RGMII/RMII */
	if (chip->phy_interface == PHY_INTERFACE_MODE_RGMII)
		value |= GMAC_PHY_RGMII_MASK;
	else
		value &= (~GMAC_PHY_RGMII_MASK);

	/* Write RGMII ETCS ret */
	value &= (~GMAC_ETCS_RMII_MASK);
	if (chip->phy_interface == PHY_INTERFACE_MODE_RGMII)
		value |= GMAC_RGMII_INTCLK_MASK;
	else if (chip->phy_interface == PHY_INTERFACE_MODE_RMII)
		value |= GMAC_RMII_MASK;

	/*
	 * Adjust Tx/Rx clock delay
	 * Tx clock delay: 0~7
	 * Rx clock delay: 0~31
	 */
	value &= ~(GMAC_TX_DELAY_MASK << GMAC_TX_DELAY_OFFSET);
	value |= ((chip->tx_delay & GMAC_TX_DELAY_MASK) << GMAC_TX_DELAY_OFFSET);
	value &= ~(GMAC_RX_DELAY_MASK << GMAC_RX_DELAY_OFFSET);
	value |= ((chip->rx_delay & GMAC_RX_DELAY_MASK) << GMAC_RX_DELAY_OFFSET);

	writel(value, SUNXI_GMAC_SYSCFG + SYSCFG_PHY_CLK);

	/* enable gmac clk */
	value = readl(SUNXI_CCM_BASE + GMAC_CLK_REG);
	value |= (1 << GMAC_RST_BIT);
	value |= (1 << GMAC_GATE_BIT);
	writel(value, SUNXI_CCM_BASE + GMAC_CLK_REG);

	/* Open phy25m clk */
	if (chip->phy_clk_type == SUNXI_PHY_USE_CLK25M) {
		value = readl(SUNXI_CCM_BASE + GMAC_25M_CLK_REG);
		value |= (3 << (GMAC_25M_GATE_BIT - 1));
		writel(value, SUNXI_CCM_BASE + GMAC_25M_CLK_REG);
	}

	ret = fdt_set_all_pin_by_offset(chip->gmac_node, "pinctrl-0");
	if (ret) {
		printf("Error: gmac set pin failed\n");
		return -EIO;
	}

	return 0;
}

static int sunxi_gmac_send(struct eth_device *dev, void *packet, int length)
{
	struct sunxi_gmac *chip = dev->priv;
	dma_desc_t *tx_p = chip->dma_desc_tx;
	u32 value, send_status;
	int timeout;

	/* Get transmit status */
	send_status = readl((void *)(u32)(dev->iobase + GMAC_TX_DMA_STA)) & GMAC_TX_DMA_MASK;

	/* Wait the Prev packet compled and timeout flush it */
	timeout = get_timer(0) + 5 * CONFIG_SYS_HZ;
	while (tx_p->desc0.tx.own || (send_status != GMAC_TX_DMA_STOP
			&& send_status != GMAC_TX_DMA_SUSPEND)) {
		invalidate_dcache_range((ulong)tx_p, (ulong)((ulong)tx_p + ALIGN(sizeof(*tx_p), CACHE_LINE_SIZE)));
		if (get_timer(0) > timeout) {
			printf("Error: Gmac send timeout\n");
			break;
		}
	}

	/* configure transmit dma descriptor */
	tx_p->desc0.all = GMAC_DMA_OWN_DESC;  /* Set Own */
	tx_p->desc1.all = GMAC_TX_DMA_ONE_DESC;
	tx_p->desc1.all |= GMAC_TX_DMA_CRC_FULL;  /* CIC Full */
	tx_p->desc1.all |= GMAC_TX_DMA_BUFF_SIZE(length);
	tx_p->desc2 = (u32 *)packet;

	flush_cache((unsigned long)tx_p, ALIGN(sizeof(*tx_p), CACHE_LINE_SIZE));
	flush_cache((unsigned long)packet, ALIGN(128, CACHE_LINE_SIZE));

	/* flush Transmit FIFO */
	value = readl((void *)(u32)(dev->iobase + GMAC_TX_CTL1));
	value |= GMAC_TX_DMA_FLUSH_FIFO;
	writel(value, (void *)(u32)(dev->iobase + GMAC_TX_CTL1));

	pkt_hex_dump("TX", (void *)packet, 64);

	/* Enable transmit and Poll transmit */
	value = readl((void *)(u32)(dev->iobase + GMAC_TX_CTL1));
	if (send_status == GMAC_TX_DMA_STOP)
		value |= GMAC_TX_DMA_EN;
	else
		value |= GMAC_TX_DMA_START;
	writel(value, (void *)(unsigned long)(dev->iobase + GMAC_TX_CTL1));

	return 0;
}

static int sunxi_gmac_rx_status(dma_desc_t *p)
{
	int ret = good_frame;

	if (p->desc0.rx.last_desc == 0)
		ret = discard_frame;

	if (p->desc0.rx.frm_type && (p->desc0.rx.chsum_err
			|| p->desc0.rx.ipch_err))
		ret = discard_frame;

	if (p->desc0.rx.err_sum)
		ret = discard_frame;

	if (p->desc0.rx.len_err)
		ret = discard_frame;

	if (p->desc0.rx.mii_err)
		ret = discard_frame;

	return ret;
}

static int sunxi_gmac_recv(struct eth_device *dev)
{
	struct sunxi_gmac *chip = dev->priv;
	dma_desc_t *rx_p = chip->dma_desc_rx;
	u32 len, recv_stat, value;

	invalidate_dcache_range((ulong)rx_p, (ulong)((ulong)rx_p + ALIGN(sizeof(*rx_p), CACHE_LINE_SIZE)));

	if (rx_p->desc0.rx.own)
		return 0;

	invalidate_dcache_range((ulong)(chip->rx_packet), (ulong)((ulong)(chip->rx_packet) + ALIGN(SZ_2K, CACHE_LINE_SIZE)));

	/*
	 * FIXME: to prevent the Rx buffer that we are handling
	 * from being overwrited.
	 */
	memcpy(chip->rx_backup_packet, chip->rx_packet, SZ_2K);

	recv_stat = readl((void *)(u32)(dev->iobase + GMAC_INT_STA));
	if (!(recv_stat & GMAC_RX_INT_MASK))
		goto fill;

	writel(GMAC_RX_CLEAR_MASK, (void *)(u32)(dev->iobase + GMAC_INT_STA));

	recv_stat = sunxi_gmac_rx_status(rx_p);
	if (recv_stat != discard_frame) {
		if (recv_stat != llc_snap)
			len = (rx_p->desc0.rx.frm_len - 4);
		else
			len = rx_p->desc0.rx.frm_len;

		pkt_hex_dump("RX", (void *)chip->rx_backup_packet, 64);

		flush_cache((unsigned long)(chip->rx_packet), ALIGN(len, CACHE_LINE_SIZE));

		net_process_received_packet((uchar *)(chip->rx_backup_packet), len);
	} else {
		/* Just need to clear 64 bits header */
		memset((chip->rx_packet), 0, 64);

		flush_cache((unsigned long)(chip->rx_packet), ALIGN(64, CACHE_LINE_SIZE));
	}

fill:
	rx_p->desc0.all = GMAC_DMA_OWN_DESC;
	rx_p->desc1.all = GMAC_RX_DMA_INT_CTL;
	rx_p->desc1.all |= GMAC_RX_DMA_BUFF_SIZE_MASK;
	rx_p->desc2 = (void *)chip->rx_packet;

	flush_cache((unsigned long)rx_p, ALIGN(sizeof(*rx_p), CACHE_LINE_SIZE));

	recv_stat = readl((void *)(u32)(dev->iobase + GMAC_RX_DMA_STA)) & GMAC_RX_DMA_MASK;
	/* Enable receive and poll it */
	value = readl((void *)(u32)(dev->iobase + GMAC_RX_CTL1));
	if (recv_stat == GMAC_RX_DMA_STOP)
		value |= GMAC_RX_DMA_EN;
	else
		value |= GMAC_RX_DMA_START;
	writel(value, (void *)(u32)(dev->iobase + GMAC_RX_CTL1));

	return 0;
}

int sunxi_gmac_write_hwaddr(struct eth_device *dev)
{
	unsigned char *addr;
	u32 mac;

	addr = &(dev->enetaddr[0]);
	mac  = (addr[5] << 8) | addr[4];
	writel(mac, (void *)(u32)(dev->iobase + GMAC_ADDR_HI(0)));

	mac  = (addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | addr[0];
	writel(mac, (void *)(u32)(dev->iobase + GMAC_ADDR_LO(0)));

	return 0;
}

static int sunxi_gmac_reset(void *iobase, int n)
{
	u32 value;

	value = readl(iobase + GMAC_BASIC_CTL1);
	value |= GMAC_SOFT_RST;
	writel(value, iobase + GMAC_BASIC_CTL1);

	udelay(n);

	return !!(readl(iobase + GMAC_BASIC_CTL1) & GMAC_SOFT_RST);
}

static int sunxi_gmac_phy_init(struct eth_device *dev)
{
	struct sunxi_gmac *chip = dev->priv;
	u32 phy_addr = 0, value, phy_id;
	u16 phy_val, phy_id_high, phy_id_low;
	int i;

	/* scan mdio bus, find phy addr */
	for (i = 0; i < GMAC_PHY_ADDR_MAX; i++) {
		phy_id_high = (u16)(__sunxi_gmac_phy_read(dev, i, MII_PHYSID1)
							& GMAC_PHY_REG_MASK);
		phy_id_low = (u16)(__sunxi_gmac_phy_read(dev, i, MII_PHYSID2) & GMAC_PHY_REG_MASK);

		phy_id = (phy_id_high << 16) | phy_id_low;

		if ((phy_id & GMAC_PHY_UNUSE_ID) == GMAC_PHY_UNUSE_ID)
			continue;

		phy_addr = i;
		break;
	}

	/* close autoneg and force to 1000Mbps/100Mbps */
	phy_val = __sunxi_gmac_phy_read(dev, phy_addr, MII_BMCR);
	phy_val &= ~BMCR_ANENABLE;
	phy_val &= ~BMCR_SPEED1000;
	if (chip->phy_interface == PHY_INTERFACE_MODE_RGMII)
		phy_val |= BMCR_SPEED1000;
	else
		phy_val |= BMCR_SPEED100;
	phy_val |= BMCR_FULLDPLX;
	__sunxi_gmac_phy_write(dev, phy_addr, MII_BMCR, phy_val);

	/* set mac speed to match phy speed */
	phy_val = __sunxi_gmac_phy_read(dev, phy_addr, MII_BMCR);
	value = readl((void *)(u32)(dev->iobase + GMAC_BASIC_CTL0));
	if (phy_val & BMCR_FULLDPLX)
		value |= GMAC_MAC_DUPLEX_MASK;
	else
		value &= ~GMAC_MAC_DUPLEX_MASK;

	value &= ~GMAC_MAC_SPEED_MASK;
	if (phy_val & BMCR_SPEED100)
		value |= GMAC_MAC_SPEED_100M;
	else if (!(phy_val & BMCR_SPEED1000))
		value |= GMAC_MAC_SPEED_1000M;
	writel(value, (void *)(u32)(dev->iobase + GMAC_BASIC_CTL0));

	/* type out phy ability */
	phy_val = __sunxi_gmac_phy_read(dev, phy_addr, MII_BMCR);
	printf("%s   Speed: %dMbps, Mode: %s, Loopback: %s\n",
				dev->name, ((phy_val & BMCR_SPEED1000) ? 1000 :
				((phy_val & BMCR_SPEED100) ? 100 : 10)),
				(phy_val & BMCR_FULLDPLX) ? "Full duplex" : "Half duplex",
				(phy_val & BMCR_LOOPBACK) ? "YES" : "NO");

	return 0;
}

static int sunxi_gmac_init(struct eth_device *dev, bd_t *bis)
{
	struct sunxi_gmac *chip = dev->priv;
	u32 value;
	int ret;

	ret = sunxi_gmac_reset((void *)(u32)dev->iobase, 1000);
	if (ret) {
		printf("Error: gmac reset failed, please check mac and phy clk\n");
		return -EINVAL;
	}

	/* init phy and set mac speed */
	sunxi_gmac_phy_init(dev);

	/* Initialize core */
	value = readl((void *)(u32)(dev->iobase + GMAC_TX_CTL0));
	value |= (1 << GMAC_TX_FRM_LEN_OFFSET);
	value |= (1 << GMAC_TX_EN_OFFSET);
	writel(value, (void *)(u32)(dev->iobase + GMAC_TX_CTL0));

	value = readl((void *)(u32)(dev->iobase + GMAC_TX_CTL1));
	/*
	 * Some platform Tx fifo is 1K, but ethernet packet is 1500,
	 * so set the dma trigger level to 128 to avoid Tx fifo overflow.
	 */
	value |= GMAC_TX_TH_128;
	writel(value, (void *)(u32)(dev->iobase + GMAC_TX_CTL1));

	value = readl((void *)(u32)(dev->iobase + GMAC_RX_CTL0));
	value |= (1 << GMAC_CHECK_CRC_OFFSET);		/* Enable CRC & IPv4 Header Checksum */
	/*
	 * Enable rx
	 * Set rx frm len
	 * Enable Jumbo frm
	 * Enable strip_fcs
	 */
	value |= (0x0F << GMAC_RX_EN_OFFSET);
	writel(value, (void *)(u32)(dev->iobase + GMAC_RX_CTL0));

	value = readl((void *)(u32)(dev->iobase + GMAC_RX_CTL1));
	value |= GMAC_RX_MODE;
	writel(value, (void *)(u32)(dev->iobase + GMAC_RX_CTL1));

	/* GMAC frame filter */
	value = readl((void *)(u32)(dev->iobase + GMAC_RX_FRM_FLT));
	value |= GMAC_RX_RECV_ALL;
	writel(value, (void *)(u32)(dev->iobase + GMAC_RX_FRM_FLT));

	/* Burst should be 8 */
	value = readl((void *)(u32)(dev->iobase + GMAC_BASIC_CTL1));
	value |= (GMAC_BURST_LEN << GMAC_BURST_LEN_OFFSET);
	writel(value, (void *)(u32)(dev->iobase + GMAC_BASIC_CTL1));

	/* Seth hardware address */
	if (dev->write_hwaddr)
		dev->write_hwaddr(dev);

	/* Disable all interrupt of dma */
	writel(0x00UL, (void *)(u32)(dev->iobase + GMAC_INT_EN));

	memset((void *)chip->dma_desc_tx, 0, sizeof(dma_desc_t));
	memset((void *)chip->dma_desc_rx, 0, sizeof(dma_desc_t));

	chip->dma_desc_tx->desc3 = (void *)chip->dma_desc_tx;  /* There is only one TX-DMA-Descriptor on the linked-list */
	chip->dma_desc_rx->desc3 = (void *)chip->dma_desc_rx;  /* There is only one RX-DMA-Descriptor on the linked-list */

	flush_cache((unsigned long)chip->dma_desc_tx, ALIGN(sizeof(*chip->dma_desc_tx), CACHE_LINE_SIZE));
	flush_cache((unsigned long)chip->dma_desc_rx, ALIGN(sizeof(*chip->dma_desc_rx), CACHE_LINE_SIZE));

	writel((ulong)chip->dma_desc_tx, (void *)(u32)(dev->iobase + GMAC_TX_DESC_LIST));
	writel((ulong)chip->dma_desc_rx, (void *)(u32)(dev->iobase + GMAC_RX_DESC_LIST));

	return 0;
}

static void sunxi_gmac_halt(struct eth_device *dev)
{
	u32 value;

	/* Disable transmit component */
	value = readl((void *)(u32)(dev->iobase + GMAC_TX_CTL0));
	value &= ~GMAC_TX_DMA_START;
	writel(value, (void *)(u32)(dev->iobase + GMAC_TX_CTL0));

	/* Disable received component */
	value = readl((void *)(u32)(dev->iobase + GMAC_RX_CTL0));
	value &= ~GMAC_RX_DMA_START;
	writel(value, (void *)(u32)(dev->iobase + GMAC_RX_CTL0));
}

static int sunxi_gmac_resource_get(struct sunxi_gmac *chip)
{
	char *phy_mode;
	int ret, i;

	chip->gmac_node = fdt_path_offset(working_fdt, "gmac0");
	if (chip->gmac_node < 0) {
		printf("Error: gmac get node failed\n");
		return -EINVAL;
	}

	ret = fdt_getprop_string(working_fdt, chip->gmac_node, "phy-mode", &phy_mode);
	if (ret < 0) {
		printf("Error: gmac get phy-mode failed\n");
		return -EINVAL;
	}

	/* convert string to enum */
	for (i = 0; i < ARRAY_SIZE(phy_interface_strings); i++) {
		if (!strcmp(phy_interface_strings[i], (const char *)phy_mode))
			break;
	}

	if (i != PHY_INTERFACE_MODE_NONE) {
		chip->phy_interface = i;
		printf("Info: gmac phy mode = %s, phy_interface = %d\n", phy_mode, chip->phy_interface);
	} else {
		printf("Error: gmac no phy interface support\n");
		return -EINVAL;
	}

	ret = fdt_getprop_u32(working_fdt, chip->gmac_node, "tx-delay", &chip->tx_delay);
	if (ret < 0) {
		printf("Error: gmac get tx-delay failed\n");
		return -EINVAL;
	}

	ret = fdt_getprop_u32(working_fdt, chip->gmac_node, "rx-delay", &chip->rx_delay);
	if (ret < 0) {
		printf("Error: gmac get rx-delay failed\n");
		return -EINVAL;
	}

	/* TODO:not support ac300 now */
#ifdef CONFIG_SUNXI_EPHY_AC300
	chip->phy_type = SUNXI_INTERNAL_PHY;
#else
	chip->phy_type = SUNXI_EXTERNAL_PHY;
#endif

	ret = fdt_getprop_u32(working_fdt, chip->gmac_node, "sunxi,phy-clk-type", &chip->phy_clk_type);
	if (ret < 0) {
		printf("Error: gmac get phy-clk-type failed\n");
		return -EINVAL;
	}

	return 0;
}

int sunxi_gmac_initialize(void)
{
	struct eth_device *dev;
	struct sunxi_gmac *chip;
	int ret;

	dev = (struct eth_device *)malloc(sizeof(*dev));
	if (!dev) {
		ret = -ENOMEM;
		goto err_dev;
	}

	memset(dev, 0, sizeof(*dev));
	strcpy(dev->name, "eth0");

	chip = (struct sunxi_gmac *)malloc(sizeof(*chip));
	if (!chip) {
		ret = -ENOMEM;
		goto err_chip;
	}

	chip->dma_desc_tx = (struct dma_desc *)
		memalign(CONFIG_SYS_CACHELINE_SIZE, sizeof(*(chip->dma_desc_tx)));
	if (!chip->dma_desc_tx) {
		ret = -ENOMEM;
		goto err_dma_tx;
	}

	chip->dma_desc_rx = (struct dma_desc *)
		memalign(CONFIG_SYS_CACHELINE_SIZE, sizeof(*(chip->dma_desc_rx)));
	if (!chip->dma_desc_rx) {
		ret = -ENOMEM;
		goto err_dma_rx;
	}

	chip->rx_packet = (unsigned char *)memalign(CONFIG_SYS_CACHELINE_SIZE, SZ_2K);
	if (!chip->rx_packet) {
		ret = -ENOMEM;
		goto err_rx_packet;
	}

	chip->rx_backup_packet = (unsigned char *)memalign(CONFIG_SYS_CACHELINE_SIZE, SZ_2K);
	if (!chip->rx_backup_packet) {
		ret = -ENOMEM;
		goto err_rx_backup_packet;
	}

	/* attach allwinner private gmac struct to eth device */
	dev->priv = chip;

	ret = sunxi_gmac_resource_get(chip);
	if (ret)
		goto err_resource_get;

	ret = sunxi_gmac_hardware_init(chip);
	if (ret)
		goto err_hardware_init;

	dev->iobase = SUNXI_GMAC_BASE;
	dev->init = sunxi_gmac_init;
	dev->halt = sunxi_gmac_halt;
	dev->send = sunxi_gmac_send;
	dev->recv = sunxi_gmac_recv;
	dev->write_hwaddr = sunxi_gmac_write_hwaddr;

	eth_register(dev);
	miiphy_register(dev->name, sunxi_gmac_phy_read, sunxi_gmac_phy_write);

	return 0;

err_hardware_init:
	/* nothing to do */
err_resource_get:
	free(chip->rx_backup_packet);
err_rx_backup_packet:
	free(chip->rx_packet);
err_rx_packet:
	free(chip->dma_desc_rx);
err_dma_rx:
	free(chip->dma_desc_tx);
err_dma_tx:
	free(chip);
err_chip:
	free(dev);
err_dev:
	return ret;
}
