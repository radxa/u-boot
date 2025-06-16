/*
 * disp2/disp/disp_sys_intf.c
 *
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "de/bsp_display.h"
#include "disp_sys_intf.h"
#include "sunxi_disp_param_from_ini.h"
#include "asm/io.h"
#include <fs.h>
#include <search.h>
#include <sunxi_power/power_manage.h>

#ifdef CONFIG_FAT_WRITE
int do_fat_fswrite(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
#endif
extern int sunxi_partition_get_partno_byname(const char *part_name);
int disp_fat_load(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
/* cache flush flags */
#define  CACHE_FLUSH_I_CACHE_REGION       0
#define  CACHE_FLUSH_D_CACHE_REGION       1
#define  CACHE_FLUSH_CACHE_REGION         2
#define  CACHE_CLEAN_D_CACHE_REGION       3
#define  CACHE_CLEAN_FLUSH_D_CACHE_REGION 4
#define  CACHE_CLEAN_FLUSH_CACHE_REGION   5
#define  LCD_CONFIG_SIZE                  10
#define PARAM_SIZE  2048
#define PARAM_NUM   64
#if defined(CONFIG_AIOT_DISP_PARAM_UPDATE) && defined(CONFIG_FAT_WRITE)
#define LCD_NUM_MAX 5
#define DE_NUM_MAX 3
#define EXT_DISP_MAX 3
#endif

void mutex_destroy(struct mutex* lock)
{
	return;
}

/*
*******************************************************************************
*                     OSAL_CacheRangeFlush
*
* Description:
*    Cache flush
*
* Parameters:
*    address    :  start address to be flush
*    length     :  size
*    flags      :  flush flags
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void disp_sys_cache_flush(void*address, u32 length, u32 flags)
{
	if (address == NULL || length == 0) {
		return;
	}

	switch(flags) {
	case CACHE_FLUSH_I_CACHE_REGION:
	break;

	case CACHE_FLUSH_D_CACHE_REGION:
	break;

	case CACHE_FLUSH_CACHE_REGION:
	break;

	case CACHE_CLEAN_D_CACHE_REGION:
	break;

	case CACHE_CLEAN_FLUSH_D_CACHE_REGION:
	break;

	case CACHE_CLEAN_FLUSH_CACHE_REGION:
	break;

	default:
	break;
	}
	return;
}

/*
*******************************************************************************
*                     disp_sys_register_irq
*
* Description:
*    irq register
*
* Parameters:
*    irqno    	    ��input.  irq no
*    flags    	    ��input.
*    Handler  	    ��input.  isr handler
*    pArg 	        ��input.  para
*    DataSize 	    ��input.  len of para
*    prio	        ��input.    priority

*
* Return value:
*
*
* note:
*    typedef s32 (*ISRCallback)( void *pArg)��
*
*******************************************************************************
*/
int disp_sys_register_irq(u32 IrqNo, u32 Flags, void* Handler,void *pArg,u32 DataSize,u32 Prio)
{
	__inf("%s, irqNo=%d, Handler=0x%p, pArg=0x%p\n", __func__, IrqNo, Handler, pArg);
	irq_install_handler(IrqNo, (interrupt_handler_t *)Handler,  pArg);

	return 0;
}

/*
*******************************************************************************
*                     disp_sys_unregister_irq
*
* Description:
*    irq unregister
*
* Parameters:
*    irqno    	��input.  irq no
*    handler  	��input.  isr handler
*    Argment 	��input.    para
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void disp_sys_unregister_irq(u32 IrqNo, void* Handler, void *pArg)
{
	irq_free_handler(IrqNo);
}

/*
*******************************************************************************
*                     disp_sys_enable_irq
*
* Description:
*    enable irq
*
* Parameters:
*    irqno ��input.  irq no
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void disp_sys_enable_irq(u32 IrqNo)
{
	irq_enable(IrqNo);
}

/*
*******************************************************************************
*                     disp_sys_disable_irq
*
* Description:
*    disable irq
*
* Parameters:
*     irqno ��input.  irq no
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void disp_sys_disable_irq(u32 IrqNo)
{
	irq_disable(IrqNo);
}

void tasklet_init(struct tasklet_struct *tasklet, void (*func), unsigned long data)
{
	if ((NULL == tasklet) || (NULL == func)) {
		__wrn("tasklet_init, para is NULL, tasklet=0x%p, func=0x%p\n", tasklet, func);
		return ;
	}
	tasklet->func = func;
	tasklet->data = data;

	return ;
}

void tasklet_schedule(struct tasklet_struct *tasklet)
{
	if (NULL == tasklet) {
		__wrn("tasklet_schedule, para is NULL, tasklet=0x%p\n", tasklet);
		return ;
	}
	tasklet->func(tasklet->data);
}

typedef struct __disp_node_map
{
	char node_name[16];
	int  nodeoffset;
}disp_fdt_node_map_t;

static disp_fdt_node_map_t g_disp_fdt_node_map[] ={
	{FDT_DISP_PATH, -1},
#ifdef SUPPORT_HDMI
	{FDT_HDMI_PATH, -1},
#endif
	{FDT_LCD0_PATH, -1},
	{FDT_LCD1_PATH, -1},
	{FDT_LCD2_PATH, -1},
	{"/soc/lcd0_1", -1},
	{"/soc/lcd0_2", -1},
	{"/soc/lcd0_3", -1},
	{"/soc/lcd0_4", -1},
	{"/soc/lcd0_5", -1},
	{"/soc/lcd0_6", -1},
	{"/soc/lcd0_7", -1},
	{"/soc/lcd0_8", -1},
	{"/soc/lcd0_9", -1},
	{"/soc/lcd1_1", -1},
	{"/soc/lcd1_2", -1},
	{"/soc/lcd1_3", -1},
#ifdef CONFIG_DISP2_TV_AC200
	{FDT_AC200_PATH, -1},
#endif
	{FDT_BOOT_DISP_PATH, -1},
#ifdef SUPPORT_TV
	{FDT_TV0_PATH, -1},
	{FDT_TV1_PATH, -1},
#endif
#if defined(SUPPORT_EDP) && (defined(CONFIG_EDP_DISP2_SUNXI) || defined(CONFIG_EDP2_DISP2_SUNXI))
	{FDT_EDP0_PATH, -1},
	{FDT_EDP1_PATH, -1},
#endif
	{"",-1}
};

void disp_fdt_init(void)
{
	int i = 0;
	while(strlen(g_disp_fdt_node_map[i].node_name))
	{
		g_disp_fdt_node_map[i].nodeoffset =
			fdt_path_offset(working_fdt, g_disp_fdt_node_map[i].node_name);
		i++;
	}
}

int  disp_fdt_nodeoffset(char *main_name)
{
	int i = 0;
	for(i = 0; ; i++)
	{
		if( 0 == strcmp(g_disp_fdt_node_map[i].node_name, main_name))
		{
			return g_disp_fdt_node_map[i].nodeoffset;
		}
		if( 0 == strlen(g_disp_fdt_node_map[i].node_name) )
		{
			int node;

			DE_INF("[DISP] %s, main_name: %s cannot be found in g_disp_fdt_node_map.\n", __func__, main_name);
			node = fdt_path_offset(working_fdt, main_name);

			if (node != -1)
				return node;

			//last
			return -1;
		}
	}
	return -1;
}

int disp_get_compat_lcd_panel_num(int disp)
{
	char dts_node_path[] = "/soc/lcd0_0";
	int node;
	int i;
	int count = 0;

	dts_node_path[8] = '0' + disp;

	for (i = 1; i <= 9; i++) {
		dts_node_path[10] = '0' + i;
		node = fdt_path_offset(working_fdt, dts_node_path);
		if (node < 0) {
			break;
		}
		count++;
	}

	return count;
}
/*
*******************************************************************************
*                     disp_get_set_lcd_param_index_from_flash
*
* Description:
*    Get or set lcd_param_index from flash.
*
* Parameters:
*    is_set: true when set, false when get
*    idx: index to set, not used when get
*
* Return value:
*    >0 for index of lcd param
*     0 for using original lcd param--lcd0
*    -1 for err
* note:
*
*
*******************************************************************************
*/
int disp_get_set_lcd_param_index_from_flash(bool is_set, int *disp, int idx)
{
	char *argv[6];
	char part_num[16] = {0};
	char len[16] = {0};
	char addr[32] = {0};
	char file_name[32] = "lcd_compatible_index.txt";
	int partno = -1;
	char *buf;
	unsigned int lcd_param_index;
	int ret = -1;
	static int idx_get = -1;
	static int this_disp = -1;
	if (!is_set && idx_get != -1 && this_disp != -1) {
		*disp = this_disp;
		return idx_get;
	}

	partno = sunxi_partition_get_partno_byname("bootloader"); /*android*/
	if (partno < 0) {
		partno = sunxi_partition_get_partno_byname("boot-resource"); /*linux*/
		if (partno < 0) {
			printf("Get bootloader and boot-resource partition number fail!\n");
		ret = -1;
		goto exit;
		}
	}


	buf = kmalloc(LCD_CONFIG_SIZE, GFP_KERNEL | __GFP_ZERO);
	if (!buf) {
		printf("malloc memory fail!\n");
		return -1;
	}
	memset(buf, 0, LCD_CONFIG_SIZE);
	snprintf(part_num, 16, "0:%x", partno);
	snprintf(len, 16, "%ld", (ulong)LCD_CONFIG_SIZE);
	snprintf(addr, 32, "%lx", (ulong)buf);
	argv[1] = "sunxi_flash";
	argv[2] = part_num;
	argv[3] = addr;
	argv[4] = file_name;
	argv[5] = len;

	if (!is_set) {
		argv[0] = "fatload";
		if (disp_fat_load(0, 0, 6, argv)) {
			pr_error("disp_fat_load for lcd config failed\n");
			ret = -1;
			goto exit_free;
		}
		if (!strncmp("lcd", buf, 3)) {
			DE_INF("buf in flash : %s \n", buf);
			lcd_param_index = simple_strtoul(buf + 5, NULL, 10);
			*disp = simple_strtoul(buf + 3, NULL, 10);
			ret = idx_get = lcd_param_index;
			this_disp = *disp;
			if (lcd_param_index > 9) {
				pr_error("error: lcd_param_index must less than 10\n");
				ret = -1;
				goto exit_free;
			}

		} else {
			pr_error("lcd_param_index format err, should be lcdn, n=0,1,2,3...\n");
			ret = -1;
		}
	} else {
#ifdef CONFIG_COMPATIBLE_PANEL_RECORD
#ifdef CONFIG_FAT_WRITE
		if (idx < 0 || idx > 9) {
			ret = -1;
			pr_error("set_lcd_param_index_from_flash param err\n");
			goto exit_free;
		}
		memcpy(buf, "lcd0_0", 7);
		buf[3] = '0' + *disp;
		buf[5] = '0' + idx;
		argv[0] = "fatwrite";
		if (do_fat_fswrite(0, 0, 6, argv)) {
			pr_error("do_fat_fswrite for lcd config failed\n");
			ret = -1;
			goto exit_free;
		} else {
			ret = idx_get = idx;
			printf("save lcd compatible disp%d index %d to flash\n", *disp, idx);
		}
#else
		pr_error("please enable FAT_WRITE for lcd compatible first\n");
		ret = -1;
#endif
#else
		ret = idx_get = idx;
#endif
}
exit_free:
	kfree(buf);
exit:
return ret;

}

/*
*******************************************************************************
*                     disp_update_lcd_param
*
* Description:
*    Update lcd0 param with lcd0_<lcd_param_index> in dts for driver to reopen lcd driver.
*
* Parameters:
*    lcd_param_index : input, the index of compatible lcd param.
*
* Return value:
*    0 for success
*    -1 for err
* note:
*    if lcd_param_index = 0, we will use lcd0 param, and make no update.
*      lcd_param_index must <= 9.
*    lcd_param_index = -1 means using previous lcd_param_index;
*
*******************************************************************************
*/

static int to_update_disp_num;
static int to_update_lcd_compat_index;

int disp_update_lcd_param(int disp, int lcd_param_index, int is_set_index)
{
	int node;
	int level = 0;
	uint32_t tag;
	int  nodeoffset;
	int  nextoffset;
	const struct fdt_property *fdt_prop;
	const char *pathp;
	int prop_len;
	char dts_node_path[] = "/soc/lcd0_0";
	int len;
	int depth = 1;
	const void *nodep;
	unsigned int success_count = 0;
	unsigned int retry_skip_count = 0;
	u8 prop_tmp[32] = {0};
	u8 prop_name_tmp[32] = {0};
	int flash_idx = 0;
	char lcd_node_pre[32] = "/soc/lcd0";

	if (is_set_index == 0) {
#ifdef CONFIG_COMPATIBLE_PANEL
	    int disp_tmp = 0;

		flash_idx = disp_get_set_lcd_param_index_from_flash(READ, &disp_tmp, 0);
		if (flash_idx == -1) {  // first start.
			disp_get_set_lcd_param_index_from_flash(WRITE, &disp, to_update_lcd_compat_index);
			flash_idx = disp_get_set_lcd_param_index_from_flash(READ, &disp_tmp, 0);
		}
		lcd_param_index = flash_idx;

		disp = disp_tmp;
		/*if not using compatible panel, following operation is useless and wasting time, so just return.*/
		if (disp_get_compat_lcd_panel_num(disp) == 0) {
			return 0;
		}

		char *ctp_path;
		char val[2];
		int nodeoff;

		// set ctp fw idx
		nodeoff = fdt_path_offset(working_fdt, "/aliases");
		fdt_getprop_string(working_fdt, nodeoff, "ctp", &ctp_path);
		snprintf(val, 2, "%d", lcd_param_index);
		fdt_find_and_setprop(working_fdt, ctp_path, "ctp_fw_idx", val, 2, 1);
#else
		flash_idx = 0;
		lcd_param_index = flash_idx;
#endif
	} else if (is_set_index == 1) {
		to_update_disp_num = disp;
		to_update_lcd_compat_index = lcd_param_index;
		/*
		    now using uboot fdt, no need to update.
		*/
		return 0;
	}

	dts_node_path[8] = '0' + disp;
	dts_node_path[10] = '0' + lcd_param_index;
	if (lcd_param_index == 0) {
		/*use default lcd0 param*/
		return 0;
	}

	lcd_node_pre[8] = '0' + disp;
	nodeoffset = fdt_path_offset(working_fdt, dts_node_path);
	node = fdt_path_offset(working_fdt, lcd_node_pre);

	if (nodeoffset < 0 || node < 0) {
		/*
		 * Not found or something else bad happened.
		 */
		pr_error("nodeoffset: %d, node: %d \n", nodeoffset, node);
		pr_error("libfdt fdt_path_offset() for lcd\n");
		return -1;
	}

	while (level >= 0) {
		tag = fdt_next_tag(working_fdt, nodeoffset, &nextoffset);
		switch (tag) {
		case FDT_BEGIN_NODE:
			pathp = fdt_get_name(working_fdt, nodeoffset, NULL);
			if (level <= depth) {
				if (pathp == NULL)
					pathp = "/* NULL pointer error */";
				if (*pathp == '\0')
					pathp = "/";	/* root is nameless */
			}
			level++;
			break;
		case FDT_END_NODE:
			level--;
			if (level == 0) {
				level = -1;		/* exit the loop */
			}
			break;
		case FDT_PROP:
			if (success_count < retry_skip_count) {
				success_count++;
				break;
			}
			fdt_prop = fdt_offset_ptr(working_fdt, nodeoffset,
					sizeof(*fdt_prop));  // lcd_compat's property
			pathp    = fdt_string(working_fdt,
					fdt32_to_cpu(fdt_prop->nameoff));  // property_name
			prop_len      = fdt32_to_cpu(fdt_prop->len);
			nodep    = fdt_prop->data;  // property_data
			if (prop_len < 0) {
				printf ("libfdt fdt_getprop(): %s\n",
					fdt_strerror(prop_len));
				return -1;
			} else if (prop_len == 0) {
				/* the property has no value */
				if (level <= depth)
					printf("NULL value for lcd0 param\n");
			} else {
				if (level <= depth) {
					/*update this prop for lcd[disp]*/
					fdt_getprop(working_fdt, node, pathp, &len);
					success_count++;
					if (len != prop_len) {
						/*need memcpy and update node offset to avoid modify by fdt_setprop.*/
						level = 0;
						retry_skip_count = success_count;
						success_count = 0;
						memcpy(prop_name_tmp, pathp, strlen(pathp)+1);
						memcpy(prop_tmp, nodep, prop_len);
						fdt_setprop(working_fdt, node, pathp, prop_tmp, prop_len);
						nodeoffset = fdt_path_offset(working_fdt, dts_node_path);
						node = fdt_path_offset(working_fdt, lcd_node_pre);
						continue;
					} else {
						fdt_setprop(working_fdt, node, pathp, nodep, prop_len);
					}
				}
			}
			break;
		case FDT_NOP:
			break;
		case FDT_END:
			disp_fdt_init();
			return 0;
		default:
			if (level <= depth)
				printf("Unknown tag 0x%08X\n", tag);
			return 0;
		}
		nodeoffset = nextoffset;
	}

	disp_fdt_init();
	return 0;
}

static char *param_get(char *name, struct hsearch_data *htab)
{
    ENTRY e, *ep;

	e.key = name;
	e.data	= NULL;

    hsearch_r(e, FIND, &ep, htab, 0);

    return ep->data;
}
static int parse_disp_param(char *name, int partno, struct sunxi_disp_param *param)
{
	char part_info[16] = { 0 };
	void *data_buf = NULL;
	loff_t data_len;

	struct hsearch_data disp_param_htab;
	memset(&disp_param_htab, 0, sizeof(disp_param_htab));
	hcreate_r(PARAM_NUM, &disp_param_htab); /* support PARAM_NUM items */

	data_buf = malloc(PARAM_SIZE);
	snprintf(part_info, 16, "0:%x", partno);

	fs_set_blk_dev("sunxi_flash", part_info, FS_TYPE_FAT);
	if (fs_read(name, (ulong)data_buf, 0, 0, &data_len)) {
		hdestroy_r(&disp_param_htab);
		free(data_buf);
		return -1;
	}

	if (himport_r(&disp_param_htab, (char *)data_buf,
				data_len, '\n', 0, 0, 0, NULL) == 0)
		pr_err("param import failed: errno = %d\n", errno);

	param->cpu_id = param_get("cpu_id", &disp_param_htab);
	param->disp_main = param_get("disp_main", &disp_param_htab);
	param->disp_aux = param_get("disp_aux", &disp_param_htab);
	param->lcd_main_driver_name = param_get("lcd_main_driver_name", &disp_param_htab);
	param->lcd_aux_driver_name = param_get("lcd_aux_driver_name", &disp_param_htab);

	param->lcd_main_node = simple_strtoul(param_get("lcd_main_node", &disp_param_htab), NULL, 10);
	param->disp_main_x = simple_strtoul(param_get("disp_main_x", &disp_param_htab), NULL, 10);
	param->disp_main_y = simple_strtoul(param_get("disp_main_y", &disp_param_htab), NULL, 10);
	param->disp_main_dclk_freq_khz = simple_strtoul(param_get("disp_main_dclk_freq_khz", &disp_param_htab), NULL, 10);
	param->disp_main_hbp = simple_strtoul(param_get("disp_main_hbp", &disp_param_htab), NULL, 10);
	param->disp_main_hspw = simple_strtoul(param_get("disp_main_hspw", &disp_param_htab), NULL, 10);
	param->disp_main_hfp = simple_strtoul(param_get("disp_main_hfp", &disp_param_htab), NULL, 10);
	param->disp_main_vbp = simple_strtoul(param_get("disp_main_vbp", &disp_param_htab), NULL, 10);
	param->disp_main_vspw = simple_strtoul(param_get("disp_main_vspw", &disp_param_htab), NULL, 10);
	param->disp_main_vfp = simple_strtoul(param_get("disp_main_vfp", &disp_param_htab), NULL, 10);

	param->lcd_aux_node = simple_strtoul(param_get("lcd_aux_node", &disp_param_htab), NULL, 10);
	param->disp_aux_x = simple_strtoul(param_get("disp_aux_x", &disp_param_htab), NULL, 10);
	param->disp_aux_y = simple_strtoul(param_get("disp_aux_y", &disp_param_htab), NULL, 10);
	param->disp_aux_dclk_freq_khz = simple_strtoul(param_get("disp_aux_dclk_freq_khz", &disp_param_htab), NULL, 10);
	param->disp_aux_hbp = simple_strtoul(param_get("disp_aux_hbp", &disp_param_htab), NULL, 10);
	param->disp_aux_hspw = simple_strtoul(param_get("disp_aux_hspw", &disp_param_htab), NULL, 10);
	param->disp_aux_hfp = simple_strtoul(param_get("disp_aux_hfp", &disp_param_htab), NULL, 10);
	param->disp_aux_vbp = simple_strtoul(param_get("disp_aux_vbp", &disp_param_htab), NULL, 10);
	param->disp_aux_vspw = simple_strtoul(param_get("disp_aux_vspw", &disp_param_htab), NULL, 10);
	param->disp_aux_vfp = simple_strtoul(param_get("disp_aux_vfp", &disp_param_htab), NULL, 10);

	param->lvds_lane_count = simple_strtoul(param_get("lvds_lane_count", &disp_param_htab), NULL, 10);
	param->edp_lane_count = simple_strtoul(param_get("edp_lane_count", &disp_param_htab), NULL, 10);
	param->mipi_lane_count = simple_strtoul(param_get("mipi_lane_count", &disp_param_htab), NULL, 10);
	param->vbo_lane_count = simple_strtoul(param_get("vbo_lane_count", &disp_param_htab), NULL, 10);
	param->lvds_color_depth = simple_strtoul(param_get("lvds_color_depth", &disp_param_htab), NULL, 10);
	param->edp_color_depth = simple_strtoul(param_get("edp_color_depth", &disp_param_htab), NULL, 10);
	param->lvds_color_depth = simple_strtoul(param_get("lvds_color_depth", &disp_param_htab), NULL, 10);
	param->mipi_color_depth = simple_strtoul(param_get("mipi_color_depth", &disp_param_htab), NULL, 10);
	param->vbo_color_depth = simple_strtoul(param_get("vbo_color_depth", &disp_param_htab), NULL, 10);

	param->lcd_main_tcon_mode = simple_strtoul(param_get("lcd_main_tcon_mode", &disp_param_htab), NULL, 10);
	param->lcd_main_tcon_en_odd_even_div = simple_strtoul(param_get("lcd_main_tcon_en_odd_even_div", &disp_param_htab), NULL, 10);
	param->mipi_main_port_num = simple_strtoul(param_get("mipi_main_port_num", &disp_param_htab), NULL, 10);

	param->backlight_used = simple_strtoul(param_get("backlight_used", &disp_param_htab), NULL, 10);
	param->backlight_pol = simple_strtoul(param_get("backlight_pol", &disp_param_htab), NULL, 10);
	param->lcd_aux_tcon_mode = simple_strtoul(param_get("lcd_aux_tcon_mode", &disp_param_htab), NULL, 10);

	param->mipi_aux_port_num = simple_strtoul(param_get("mipi_aux_port_num", &disp_param_htab), NULL, 10);
	param->lcd_aux_tcon_en_odd_even_div = simple_strtoul(param_get("lcd_aux_tcon_en_odd_even_div", &disp_param_htab), NULL, 10);
	param->backlight2_used = simple_strtoul(param_get("backlight2_used", &disp_param_htab), NULL, 10);
	param->backlight2_pol = simple_strtoul(param_get("backlight2_pol", &disp_param_htab), NULL, 10);

	param->lcd_main_start_delay = simple_strtoul(param_get("lcd_main_start_delay", &disp_param_htab), NULL, 10);
	param->lcd_aux_start_delay = simple_strtoul(param_get("lcd_aux_start_delay", &disp_param_htab), NULL, 10);

	param->lvds_fmt = simple_strtoul(param_get("lvds_fmt", &disp_param_htab), NULL, 10);
	param->lvds_reverse = simple_strtoul(param_get("lvds_reverse", &disp_param_htab), NULL, 10);
	param->backlight_level = simple_strtoul(param_get("backlight_level", &disp_param_htab), NULL, 10);
	param->backlight2_level = simple_strtoul(param_get("backlight2_level", &disp_param_htab), NULL, 10);
	param->backlight_freq = simple_strtoul(param_get("backlight_freq", &disp_param_htab), NULL, 10);
	param->backlight2_freq = simple_strtoul(param_get("backlight2_freq", &disp_param_htab), NULL, 10);
	param->backlight_delayms = simple_strtoul(param_get("backlight_delayms", &disp_param_htab), NULL, 10);
	param->backlight2_delayms = simple_strtoul(param_get("backlight2_delayms", &disp_param_htab), NULL, 10);
	param->fb0_width = simple_strtoul(param_get("fb0_width", &disp_param_htab), NULL, 10);
	param->fb0_height = simple_strtoul(param_get("fb0_height", &disp_param_htab), NULL, 10);
	param->backlight_ch = simple_strtoul(param_get("backlight_ch", &disp_param_htab), NULL, 10);
	param->backlight2_ch = simple_strtoul(param_get("backlight2_ch", &disp_param_htab), NULL, 10);

	hdestroy_r(&disp_param_htab);
	free(data_buf);

	return 0;
}

int flash_update_lcd_param(void)
{
	s32 ret = -1;
	char primary_key[25];
	struct sunxi_disp_param param;
	int node;
	int partno = -1;
	char *file_name = "display_param.cfg";

	partno = sunxi_partition_get_partno_byname("bootloader"); /*android*/
	if (partno < 0) {
		partno = sunxi_partition_get_partno_byname("boot-resource"); /*linux*/
		if (partno < 0) {
			pr_err("Get bootloader and boot-resource partition number fail!\n");
			return ret;
		}
	}
	if (parse_disp_param(file_name, partno, &param))
		return -1;

	pr_err("disp_main = %s  disp_aux = %s\n", param.disp_main, param.disp_aux);
	pr_err("lcd_main_node = %d  lcd_aux_node = %d\n", param.lcd_main_node, param.lcd_aux_node);
	if (param.lcd_main_node >= 0 && param.lcd_main_node <= 2) {
		sprintf(primary_key, "/soc/lcd%d", param.lcd_main_node);
		node = fdt_path_offset(working_fdt, primary_key);
		if (node < 0) {
			printf("Node not found\n");
			return -1;
		}
		pr_err("lcd_main_driver_name:%s \n", param.lcd_main_driver_name);
		if (fdt_setprop_string(working_fdt, node, "lcd_driver_name", param.lcd_main_driver_name) < 0) {
			printf("Failed to set property lcd_main_driver_name\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_x", param.disp_main_x) < 0) {
			printf("Failed to set property lcd_man_x\n");
			return -1;
		}

		if (fdt_setprop_u32(working_fdt, node, "lcd_y", param.disp_main_y) < 0) {
			printf("Failed to set property lcd_main_y\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_dclk_freq", param.disp_main_dclk_freq_khz / 1000) < 0) {
			printf("Failed to set property lcd_main_dclk_freq\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_hbp", param.disp_main_hbp + param.disp_main_hspw) < 0) {
			printf("Failed to set property lcd_main_hbp\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_hspw", param.disp_main_hspw) < 0) {
			printf("Failed to set property lcd_main_hspw\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_ht", param.disp_main_x + param.disp_main_hbp +
						param.disp_main_hspw + param.disp_main_hfp) < 0) {
			printf("Failed to set property lcd_main_ht\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_vbp", param.disp_main_vbp + param.disp_main_vspw) < 0) {
			printf("Failed to set property lcd_main_vbp\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_vspw", param.disp_main_vspw) < 0) {
			printf("Failed to set property lcd_main_vspw\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_vt", param.disp_main_y + param.disp_main_vbp +
						param.disp_main_vspw + param.disp_main_vfp) < 0) {
			printf("Failed to set property lcd_main_vt\n");
			return -1;
		}

		if (fdt_setprop_u32(working_fdt, node, "lcd_tcon_mode", param.lcd_main_tcon_mode) < 0) {
			printf("Failed to set property lcd_main_tcon_mode\n");
			return -1;
		}

		if (fdt_setprop_u32(working_fdt, node, "lcd_tcon_en_odd_even_div", param.lcd_main_tcon_en_odd_even_div) < 0) {
			printf("Failed to set property lcd_main_tcon_en_odd_even_drv\n");
			return -1;
		}

		if (fdt_setprop_u32(working_fdt, node, "lcd_start_delay", param.lcd_main_start_delay) < 0) {
			printf("Failed to set property lcd_main_start_delay\n");
			return -1;
		}
		if (!strncmp(param.disp_main, "lvds", 4)) {
			if (fdt_setprop_u32(working_fdt, node, "lcd_if", 3) < 0) {
				printf("Failed to set property lcd_main_if\n");
				return -1;
			}

			if (fdt_setprop_u32(working_fdt, node, "lcd_lvds_if", param.lvds_lane_count) < 0) {
				printf("Failed to set property lcd_main_lvds_if\n");
				return -1;
			}
			if (fdt_setprop_u32(working_fdt, node, "lcd_lvds_colordepth", param.lvds_color_depth) < 0) {
				printf("Failed to set property lcd_main_lvds_colordepth\n");
				return -1;
			}
			if (fdt_setprop_u32(working_fdt, node, "lcd_lvds_mode", param.lvds_fmt) < 0) {
				printf("Failed to set property lcd_main_lvds_mode\n");
				return -1;
			}
		}
		if (!strncmp(param.disp_main, "mipi", 4)) {

			if (fdt_setprop_u32(working_fdt, node, "lcd_if", 4) < 0) {
				printf("Failed to set property lcd_main_if\n");
				return -1;
			}

			if (fdt_setprop_u32(working_fdt, node, "lcd_dsi_if", 0) < 0) {
				printf("Failed to set property lcd_main_dsi_if\n");
				return -1;
			}
			if (fdt_setprop_u32(working_fdt, node, "lcd_dsi_lane", param.mipi_lane_count) < 0) {
				printf("Failed to set property lcd_main_dsi_lane\n");
				return -1;
			}
			if (fdt_setprop_u32(working_fdt, node, "lcd_dsi_format", param.mipi_color_depth) < 0) {
				printf("Failed to set property lcd_main_dsi_format\n");
				return -1;
			}
			if (fdt_setprop_u32(working_fdt, node, "lcd_dsi_port_num", param.mipi_main_port_num) < 0) {
				printf("Failed to set property lcd_main_dsi_port_num\n");
				return -1;
			}
		}

		//	if (!strncmp(param.disp_main, "hv", 2))
		//		info->lcd_if = 0;

		if (fdt_setprop_u32(working_fdt, node, "lcd_backlight", param.backlight_level) < 0) {
			printf("Failed to set property lcd_main_height\n");
			return -1;
		}

		if (fdt_setprop_u32(working_fdt, node, "lcd_pwm_used", param.backlight_used) < 0) {
			printf("Failed to set property lcd_pwm_used\n");
			return -1;
		}

		if (fdt_setprop_u32(working_fdt, node, "lcd_pwm_ch", param.backlight_ch) < 0) {
			printf("Failed to set property lcd_main_pwm_ch\n");
			return -1;
		}

		if (fdt_setprop_u32(working_fdt, node, "lcd_pwm_pol", param.backlight_pol) < 0) {
				printf("Failed to set property lcd_pwm_pol\n");
				return -1;
		}

		if (fdt_setprop_u32(working_fdt, node, "lcd_pwm_freq", param.backlight_freq) < 0) {
			printf("Failed to set property lcd_main_pwm_freq\n");
			return -1;
		}

		if (fdt_setprop_u32(working_fdt, node, "lcd_backlight_delay", param.backlight_delayms) < 0) {
			printf("Failed to set property lcd_backlight_delay\n");
			return -1;
		}

		node = fdt_path_offset(working_fdt, "/soc/disp");
		if (fdt_setprop_u32(working_fdt, node, "screen0_to_lcd_index", param.lcd_main_node) < 0) {
			printf("Failed to set property screen0_to_lcd_index\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "fb0_width", (param.fb0_width != 0) ?
						param.fb0_width : param.disp_main_x) < 0) {
			printf("Failed to set property fb0_width\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "fb0_height", (param.fb0_height != 0) ?
						param.fb0_height : param.disp_main_y) < 0) {
			printf("Failed to set property fb0_height\n");
			return -1;
		}
	}

	if (param.lcd_aux_node >= 0 && param.lcd_aux_node <= 2) {
		sprintf(primary_key, "/soc/lcd%d", param.lcd_aux_node);
		node = fdt_path_offset(working_fdt, primary_key);
		if (node < 0) {
			printf("Node not found\n");
			return -1;
		}
		pr_err("lcd_aux_driver_name:%s \n", param.lcd_aux_driver_name);
		if (fdt_setprop_string(working_fdt, node, "lcd_driver_name", param.lcd_aux_driver_name) < 0) {
			printf("Failed to set property lcd_aux_driver_name\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_x", param.disp_aux_x) < 0) {
			printf("Failed to set property lcd_aux_x\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_y", param.disp_aux_y) < 0) {
			printf("Failed to set property lcd_aux_y\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_dclk_freq", param.disp_aux_dclk_freq_khz / 1000) < 0) {
			printf("Failed to set property lcd_aux_dclk_freq\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_hbp", param.disp_aux_hbp + param.disp_aux_hspw) < 0) {
			printf("Failed to set property lcd_aux_hbp\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_hspw", param.disp_aux_hspw) < 0) {
			printf("Failed to set property lcd_aux_hspw\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_ht", param.disp_aux_x + param.disp_aux_hbp +
						param.disp_aux_hspw + param.disp_aux_hfp) < 0) {
			printf("Failed to set property lcd_aux_ht\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_vbp", param.disp_aux_vbp + param.disp_aux_vspw) < 0) {
			printf("Failed to set property lcd_aux_vbp\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_vspw", param.disp_aux_vspw) < 0) {
			printf("Failed to set property lcd_aux_vspw\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_vt", param.disp_aux_y + param.disp_aux_vbp +
						param.disp_aux_vspw + param.disp_aux_vfp) < 0) {
			printf("Failed to set property lcd_aux_vt\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_tcon_mode", param.lcd_aux_tcon_mode) < 0) {
			printf("Failed to set property lcd_aux_tcon_mode\n");
			return -1;
		}

		if (fdt_setprop_u32(working_fdt, node, "lcd_tcon_en_odd_even_div", param.lcd_aux_tcon_en_odd_even_div) < 0) {
			printf("Failed to set property lcd_aux_tcon_en_odd_even_drv\n");
			return -1;
		}

		if (fdt_setprop_u32(working_fdt, node, "lcd_start_delay", param.lcd_aux_start_delay) < 0) {
			printf("Failed to set property lcd_aux_start_delay\n");
			return -1;
		}
		if (!strncmp(param.disp_aux, "lvds", 4)) {
			if (fdt_setprop_u32(working_fdt, node, "lcd_if", 3) < 0) {
				printf("Failed to set property lcd_aux_if\n");
				return -1;
			}
			if (fdt_setprop_u32(working_fdt, node, "lcd_lvds_if", param.lvds_lane_count) < 0) {
				printf("Failed to set property lcd_aux_lvds_if\n");
				return -1;
			}
			if (fdt_setprop_u32(working_fdt, node, "lcd_lvds_colordepth", param.lvds_color_depth) < 0) {
				printf("Failed to set property lcd_aux_lvds_colordepth\n");
				return -1;
			}
			if (fdt_setprop_u32(working_fdt, node, "lcd_lvds_mode", param.lvds_fmt) < 0) {
				printf("Failed to set property lcd_aux_lvds_mode\n");
				return -1;
			}
		}
		if (!strncmp(param.disp_aux, "mipi", 4)) {
			if (fdt_setprop_u32(working_fdt, node, "lcd_if", 4) < 0) {
				printf("Failed to set property lcd_aux_if\n");
				return -1;
			}
			if (fdt_setprop_u32(working_fdt, node, "lcd_dsi_if", 0) < 0) {
				printf("Failed to set property lcd_aux_dsi_if\n");
				return -1;
			}
			if (fdt_setprop_u32(working_fdt, node, "lcd_dsi_lane", param.mipi_lane_count) < 0) {
				printf("Failed to set property lcd_aux_dsi_lane\n");
				return -1;
			}
			if (fdt_setprop_u32(working_fdt, node, "lcd_dsi_format", param.mipi_color_depth) < 0) {
				printf("Failed to set property lcd_aux_dsi_format\n");
				return -1;
			}
			if (fdt_setprop_u32(working_fdt, node, "lcd_dsi_port_num", param.mipi_aux_port_num) < 0) {
				printf("Failed to set property lcd_main_dsi_port_num\n");
				return -1;
			}
		}

		//	if (!strncmp(param.disp_aux, "hv", 2))
		//		info->lcd_if = 0;

		if (fdt_setprop_u32(working_fdt, node, "lcd_backlight", param.backlight2_level) < 0) {
			printf("Failed to set property lcd_height2\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_pwm_used", param.backlight2_used) < 0) {
			printf("Failed to set property lcd_pwm_used\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_pwm_ch", param.backlight2_ch) < 0) {
			printf("Failed to set property lcd_pwm_ch2\n");
			return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_pwm_pol", param.backlight2_pol) < 0) {
				printf("Failed to set property lcd_pwm_pol\n");
				return -1;
		}
		if (fdt_setprop_u32(working_fdt, node, "lcd_pwm_freq", param.backlight2_freq) < 0) {
			printf("Failed to set property lcd_pwm_freq2\n");
			return -1;
		}

		if (fdt_setprop_u32(working_fdt, node, "lcd_backlight_delay", param.backlight2_delayms) < 0) {
			printf("Failed to set property lcd_pwm_freq2\n");
			return -1;
		}
		node = fdt_path_offset(working_fdt, "/soc/disp");
		if (fdt_setprop_u32(working_fdt, node, "screen1_to_lcd_index", param.lcd_aux_node) < 0) {
			printf("Failed to set property screen1_to_lcd_index\n");
			return -1;
		}
	}

	return 0;
}
#if defined(CONFIG_AIOT_DISP_PARAM_UPDATE) && defined(CONFIG_FAT_WRITE)
static int lcd_param_update(const char *data)
{
	struct key_info *info;
	int node;
	int i = 0, j = 0;
	char group_name[20], primary_key[10];
	int info_cnt = 0;
	s32 value;
	const u32 *temp_pinctrl_0 = NULL;
	const u32 *temp_pinctrl_1 = NULL;
	int num_cell_0, num_cell_1;

	for (i = 0; i < LCD_NUM_MAX; i++) {
		sprintf(primary_key, "/soc/lcd%d", i);
		node = fdt_path_offset(working_fdt, primary_key);
		if (node < 0) {
			pr_debug("Node:%s not found\n", primary_key);
			continue;
		}

		sprintf(group_name, "lcd%d", i);
		info = ini_get_group_info(group_name, data);
		info_cnt = get_struct_count(info);
		if (info_cnt == 0)
			continue;

		for (j = 0; j < info_cnt; j++) {
			if (strcmp(info[j].key_name, "panel_driver_name") == 0 && info[j].key_value)
				fdt_setprop_string(working_fdt, node, "lcd_driver_name", info[j].key_value);

			if (strcmp(info[j].key_name, "status") == 0 && info[j].key_value)
				fdt_setprop_string(working_fdt, node, "status", info[j].key_value);

			if (strcmp(info[j].key_name, "used") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_used", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "dclk_freq") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_dclk_freq", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "ht") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_ht", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "x") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_x", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "hspw") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_hspw", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "hbp") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_hbp", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "vt") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_vt", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "y") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_y", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "vbp") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_vbp", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "vspw") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_vspw", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "tcon_mode") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_tcon_mode", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "tcon_en_odd_even_div") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_tcon_en_odd_even_div", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "start_delay") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_start_delay", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "backlight") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_backlight", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "backlight_delay") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_backlight_delay", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "pwm_used") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_pwm_used", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "pwm_ch") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_pwm_ch", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "pwm_freq") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_pwm_freq", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "pwm_pol") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_pwm_pol", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "pwm_max_limit") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_pwm_max_limit", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "if") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_if", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "convert_if") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_convert_if", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "lvds_if") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_lvds_if", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "lvds_colordepth") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_lvds_colordepth", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "lvds_mode") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_lvds_mode", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "dsi_if") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_dsi_if", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "dsi_lane") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_dsi_lane", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "dsi_format") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_dsi_fotmat", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "dsi_port_num") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_dsi_port_num", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "dsi_eotp") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_dsi_eotp", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "dsi_te") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_dsi_te", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "sync_pixel_num") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_sync_pixel_num", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "sync_line_num") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_sync_line_num", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "hv_clk_phase") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_hv_clk_phase", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "hv_sync_polarity") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_hv_sync_polarity", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "hv_data_polarity") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_hv_data_polarity", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "hv_srgb_seq") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_hv_srgb_seq", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "hv_syuv_seq") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_hv_syuv_seq", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "hv_syuv_fdly") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_hv_syuv_fdly", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "hv_if") == 0 && info[j].key_value)
				fdt_setprop_u32(working_fdt, node, "lcd_hv_if", simple_strtol(info[j].key_value, NULL, 10));

			if (strcmp(info[j].key_name, "lcd_port") == 0 && info[j].key_value) {
				if (strncmp(info[j].key_value, "lvds0", 5) == 0) {
					disp_sys_script_get_item(primary_key, "lvds0_pinctrl-0", &value, 1);
					fdt_setprop_u32(working_fdt, node, "pinctrl-0", value);

					disp_sys_script_get_item(primary_key, "lvds0_pinctrl-1", &value, 1);
					fdt_setprop_u32(working_fdt, node, "pinctrl-1", value);
				} else if (strncmp(info[j].key_value, "lvds1", 5) == 0) {
					disp_sys_script_get_item(primary_key, "lvds1_pinctrl-0", &value, 1);
					fdt_setprop_u32(working_fdt, node, "pinctrl-0", value);

					disp_sys_script_get_item(primary_key, "lvds1_pinctrl-1", &value, 1);
					fdt_setprop_u32(working_fdt, node, "pinctrl-1", value);
				} else if (strncmp(info[j].key_value, "dual_lvds0", 10) == 0) {
					temp_pinctrl_0 = fdt_getprop(working_fdt, node, "dual_lvds0_pinctrl-0", &num_cell_0);
					temp_pinctrl_1 = fdt_getprop(working_fdt, node, "dual_lvds0_pinctrl-1", &num_cell_1);

					fdt_setprop(working_fdt, node, "pinctrl-0", temp_pinctrl_0 + 1, num_cell_0);
					fdt_setprop(working_fdt, node, "pinctrl-1", temp_pinctrl_1 + 2, num_cell_1);
				} else if (strncmp(info[j].key_value, "dsi0", 4) == 0) {
					disp_sys_script_get_item(primary_key, "dsi0_pinctrl-0", &value, 1);
					fdt_setprop_u32(working_fdt, node, "pinctrl-0", value);

					disp_sys_script_get_item(primary_key, "dsi0_pinctrl-1", &value, 1);
					fdt_setprop_u32(working_fdt, node, "pinctrl-1", value);
				} else if (strncmp(info[j].key_value, "dual_dsi", 8) == 0) {
					temp_pinctrl_0 = fdt_getprop(working_fdt, node, "dual_dsi_pinctrl-0", &num_cell_0);
					temp_pinctrl_1 = fdt_getprop(working_fdt, node, "dual_dsi_pinctrl-1", &num_cell_1);

					fdt_setprop(working_fdt, node, "pinctrl-0", temp_pinctrl_0 + 1, num_cell_0);
					fdt_setprop(working_fdt, node, "pinctrl-1", temp_pinctrl_1 + 2, num_cell_1);
				} else if (strncmp(info[j].key_value, "dsi1", 4) == 0) {
					disp_sys_script_get_item(primary_key, "dsi1_pinctrl-0", &value, 1);
					fdt_setprop_u32(working_fdt, node, "pinctrl-0", value);

					disp_sys_script_get_item(primary_key, "dsi1_pinctrl-1", &value, 1);
					fdt_setprop_u32(working_fdt, node, "pinctrl-1", value);
				} else if (strncmp(info[j].key_value, "lvds2", 5) == 0) {
					disp_sys_script_get_item(primary_key, "lvds2_pinctrl-0", &value, 1);
					fdt_setprop_u32(working_fdt, node, "pinctrl-0", value);

					disp_sys_script_get_item(primary_key, "lvds2_pinctrl-1", &value, 1);
					fdt_setprop_u32(working_fdt, node, "pinctrl-1", value);
				} else if (strncmp(info[j].key_value, "lvds3", 5) == 0) {
					disp_sys_script_get_item(primary_key, "lvds3_pinctrl-0", &value, 1);
					fdt_setprop_u32(working_fdt, node, "pinctrl-0", value);

					disp_sys_script_get_item(primary_key, "lvds3_pinctrl-1", &value, 1);
					fdt_setprop_u32(working_fdt, node, "pinctrl-1", value);
				} else if (strncmp(info[j].key_value, "dual_lvds1", 10) == 0) {
					temp_pinctrl_0 = fdt_getprop(working_fdt, node, "dual_lvds1_pinctrl-0", &num_cell_0);
					temp_pinctrl_1 = fdt_getprop(working_fdt, node, "dual_lvds1_pinctrl-1", &num_cell_1);

					fdt_setprop(working_fdt, node, "pinctrl-0", temp_pinctrl_0 + 1, num_cell_0);
					fdt_setprop(working_fdt, node, "pinctrl-1", temp_pinctrl_1 + 2, num_cell_1);
				}
			}
		}
	}
	return 0;
}

static int edp_param_update(const char *data)
{
	struct key_info *info;
	int node;
	char group_name[20];
	int info_cnt = 0, i = 0;

	node = fdt_path_offset(working_fdt, "/soc/edp0");
	if (node < 0) {
		printf("Node edp0 not found\n");
		return -1;
	}

	sprintf(group_name, "edp");
	info = ini_get_group_info(group_name, data);
	info_cnt = get_struct_count(info);
	if (info_cnt == 0)
		return -1;

	for (i = 0; i < info_cnt; i++) {
		if (strcmp(info[i].key_name, "status") == 0 && info[i].key_value)
			fdt_setprop_string(working_fdt, node, "status", info[i].key_value);

		if (strcmp(info[i].key_name, "panel_driver_name") == 0 && info[i].key_value)
			fdt_setprop_string(working_fdt, node, "edp_panel_driver", info[i].key_value);

		if (strcmp(info[i].key_name, "timings_type") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_timings_type", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "ht") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_ht", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "x") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_x", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "hbp") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_hbp", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "hsw") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_hsw", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "hfp") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_hfp", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "vt") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_vt", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "y") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_y", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "vbp") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_vbp", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "vsw") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_vsw", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "vfp") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_vfp", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "hpolor") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_hpolor", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "vpolor") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_vpolor", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "fps") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_fps", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "ssc_en") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_ssc_en", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "ssc_mode") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_ssc_mode", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "audio_en") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_audio_en", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "color_depth") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_colordepth", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "color_format") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_color_fmt", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "lane_rate") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_lane_rate", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "lane_cnt") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_lane_cnt", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "lane0_sw") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_lane0_sw", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "lane0_pre") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_lane0_pre", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "lane1_sw") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_lane0_sw", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "lane1_pre") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_lane0_pre", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "lane2_sw") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_lane0_sw", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "lane2_pre") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_lane0_pre", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "lane3_sw") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_lane0_sw", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "lane3_pre") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_lane0_pre", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "efficient_training") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "efficient_training", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "panel_used") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_panel_used", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "pwm_used") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_pwm_used", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "pwm_freq") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_pwm_freq", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "pwm_pol") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_pwm_pol", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "default_backlight") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_default_backlight", simple_strtol(info[i].key_value, NULL, 10));
	}

	return 0;
}

static int dp_param_update(const char *data)
{
	struct key_info *info;
	int node;
	char group_name[20];
	int info_cnt = 0;
	int i;

	node = fdt_path_offset(working_fdt, "/soc/edp0");
	if (node < 0) {
		printf("Node:edp0 not found\n");
		return -1;
	}

	sprintf(group_name, "dp");
	info = ini_get_group_info(group_name, data);
	info_cnt = get_struct_count(info);
	if (info_cnt == 0)
		return -1;

	for (i = 0; i < info_cnt; i++) {
		if (strcmp(info[i].key_name, "compatible") == 0 && info[i].key_value)
			fdt_setprop_string(working_fdt, node, "compatible", info[i].key_value);

		if (strcmp(info[i].key_name, "status") == 0 && info[i].key_value)
			fdt_setprop_string(working_fdt, node, "status", info[i].key_value);

		if (strcmp(info[i].key_name, "ssc_en") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_ssc_en", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "ssc_mode") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_ssc_mode", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "audio_en") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_audio_en", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "color_depth") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_colordepth", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "color_format") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_color_fmt", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "lane_rate") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_lane_rate", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "lane_cnt") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_lane_cnt", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "lane0_sw") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_lane0_sw", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "lane0_pre") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_lane0_pre", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "lane1_sw") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_lane0_sw", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "lane1_pre") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_lane0_pre", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "lane2_sw") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_lane0_sw", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "lane2_pre") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_lane0_pre", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "lane3_sw") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_lane0_sw", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "lane3_pre") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "edp_lane0_pre", simple_strtol(info[i].key_value, NULL, 10));

		if (strcmp(info[i].key_name, "efficient_training") == 0 && info[i].key_value)
			fdt_setprop_u32(working_fdt, node, "efficient_training", simple_strtol(info[i].key_value, NULL, 10));
	}

	return 0;
}

static int hdmi_param_update(const char *data)
{
	struct key_info *info;
	int node;
	char group_name[20];
	int info_cnt = 0;
	int i = 0;

	node = fdt_path_offset(working_fdt, "/soc/hdmi");
	if (node < 0) {
		printf("Node hdmi not found\n");
		return -1;
	}

	sprintf(group_name, "hdmi");
	info = ini_get_group_info(group_name, data);
	info_cnt = get_struct_count(info);
	if (info_cnt == 0)
		return -1;

	for (i = 0; i < info_cnt; i++) {
		if (strcmp(info[i].key_name, "status") == 0 && info[i].key_value)
			fdt_setprop_string(working_fdt, node, "status", info[i].key_value);
	}

	return 0;
}

static int hwc_param_update(const char *data)
{
	struct key_info *info;
	char group_name[20], property_name[30];
	int i = 0, j = 0;
	int info_cnt = 0;
	int node;

	node = fdt_path_offset(working_fdt, "/soc/disp");
	if (node < 0) {
		printf("Node:disp not found\n");
		return -1;
	}

	sprintf(group_name, "primary_display");
	info = ini_get_group_info(group_name, data);
	info_cnt = get_struct_count(info);
	for (j = 0; j < info_cnt; j++) {
		if (strcmp(info[j].key_name, "display_type") == 0 && info[j].key_value)
			fdt_setprop_string(working_fdt, node, "primary_display_type", info[j].key_value);

		if (strcmp(info[j].key_name, "device_num") == 0 && info[j].key_value)
			fdt_setprop_u32(working_fdt, node, "display_device_num", simple_strtol(info[j].key_value, NULL, 10));

		if (strcmp(info[j].key_name, "de_id") == 0 && info[j].key_value)
			fdt_setprop_u32(working_fdt, node, "primary_de_id", simple_strtol(info[j].key_value, NULL, 10));

		if (strcmp(info[j].key_name, "framebuffer_width") == 0 && info[j].key_value)
			fdt_setprop_u32(working_fdt, node, "primary_framebuffer_width", simple_strtol(info[j].key_value, NULL, 10));

		if (strcmp(info[j].key_name, "framebuffer_height") == 0 && info[j].key_value)
			fdt_setprop_u32(working_fdt, node, "primary_framebuffer_height", simple_strtol(info[j].key_value, NULL, 10));

		if (strcmp(info[j].key_name, "dpix") == 0 && info[j].key_value)
			fdt_setprop_u32(working_fdt, node, "primary_dpix", simple_strtol(info[j].key_value, NULL, 10));

		if (strcmp(info[j].key_name, "dpiy") == 0 && info[j].key_value)
			fdt_setprop_u32(working_fdt, node, "primary_dpiy", simple_strtol(info[j].key_value, NULL, 10));
	}

	for (i = 0; i < EXT_DISP_MAX; i++) {
		sprintf(group_name, "extend%d_display", i);
		info = ini_get_group_info(group_name, data);
		info_cnt = get_struct_count(info);
		if (info_cnt == 0)
			continue;

		for (j = 0; j < info_cnt; j++) {
			if (strcmp(info[j].key_name, "display_type") == 0 && info[j].key_value) {
				sprintf(property_name, "extend%d_display_type", i);
				fdt_setprop_string(working_fdt, node, property_name, info[j].key_value);
			}

			if (strcmp(info[j].key_name, "de_id") == 0 && info[j].key_value) {
				sprintf(property_name, "extend%d_de_id", i);
				fdt_setprop_u32(working_fdt, node, property_name, simple_strtol(info[j].key_value, NULL, 10));
			}

			if (strcmp(info[j].key_name, "framebuffer_width") == 0 && info[j].key_value) {
				sprintf(property_name, "extend%d_framebuffer_width", i);
				fdt_setprop_u32(working_fdt, node, property_name, simple_strtol(info[j].key_value, NULL, 10));
			}

			if (strcmp(info[j].key_name, "framebuffer_height") == 0 && info[j].key_value) {
				sprintf(property_name, "extend%d_framebuffer_height", i);
				fdt_setprop_u32(working_fdt, node, property_name, simple_strtol(info[j].key_value, NULL, 10));
			}

			if (strcmp(info[j].key_name, "dpix") == 0 && info[j].key_value) {
				sprintf(property_name, "extend%d_dpix", i);
				fdt_setprop_u32(working_fdt, node, property_name, simple_strtol(info[j].key_value, NULL, 10));
			}

			if (strcmp(info[j].key_name, "dpiy") == 0 && info[j].key_value) {
				sprintf(property_name, "extend%d_dpiy", i);
				fdt_setprop_u32(working_fdt, node, property_name, simple_strtol(info[j].key_value, NULL, 10));
			}
		}
	}

	return 0;
}

int disp_updata_param(void)
{
	struct key_info *info;
	char group_name[20], property_name[30];
	char *ini_file = "disp_config.ini";
	const char *data = NULL;
	int i = 0, j = 0;
	int info_cnt = 0;
	int node;
	int output_type = 0;

	node = fdt_path_offset(working_fdt, "/soc/disp");
	if (node < 0) {
		printf("Node:disp not found\n");
		return -1;
	}

	data = get_file_from_partiton(ini_file, "bootloader");
	if (!data) {
		data = get_file_from_partiton(ini_file, "boot-resource");
		if (!data) {
			pr_err("Get bootloader and boot-resource partition number fail!\n");
			return -1;
		}
	}
	for (i = 0; i < DE_NUM_MAX; i++) {
		sprintf(group_name, "disp%d", i);
		info = ini_get_group_info(group_name, data);
		info_cnt = get_struct_count(info);
		if (info_cnt == 0)
			continue;

		for (j = 0; j < info_cnt; j++) {
			if (i == 0) {
				if (strcmp(info[j].key_name, "dev_num") == 0 && info[j].key_value) {
					sprintf(property_name, "dev_num");
					fdt_setprop_u32(working_fdt, node, property_name, simple_strtol(info[j].key_value, NULL, 10));
				}

				if (strcmp(info[j].key_name, "chn_cfg_mode") == 0 && info[j].key_value) {
					sprintf(property_name, "chn_cfg_mode");
					fdt_setprop_u32(working_fdt, node, property_name, simple_strtol(info[j].key_value, NULL, 10));
				}
			}

			if (strcmp(info[j].key_name, "lcd_index") == 0 && info[j].key_value) {
				sprintf(property_name, "screen%d_to_lcd_index", i);
				fdt_setprop_u32(working_fdt, node, property_name, simple_strtol(info[j].key_value, NULL, 10));
			}

			if (strcmp(info[j].key_name, "output_mode") == 0 && info[j].key_value) {
				sprintf(property_name, "screen%d_output_mode", i);
				fdt_setprop_u32(working_fdt, node, property_name, simple_strtol(info[j].key_value, NULL, 10));
				sprintf(property_name, "dev%d_output_mode", i);
				fdt_setprop_u32(working_fdt, node, property_name, simple_strtol(info[j].key_value, NULL, 10));
			}

			if (strcmp(info[j].key_name, "output_type") == 0 && info[j].key_value) {
				sprintf(property_name, "screen%d_output_type", i);
				fdt_setprop_u32(working_fdt, node, property_name, simple_strtol(info[j].key_value, NULL, 10));

				switch (simple_strtol(info[j].key_value, NULL, 10)) {
				case 0:
					output_type = DISP_OUTPUT_TYPE_NONE;
					break;
				case 1:
					output_type = DISP_OUTPUT_TYPE_LCD;
					break;
				case 2:
					output_type = DISP_OUTPUT_TYPE_TV;
					break;
				case 3:
					output_type = DISP_OUTPUT_TYPE_HDMI;
					break;
				case 4:
					output_type = DISP_OUTPUT_TYPE_VGA;
					break;
				case 6:
					output_type = DISP_OUTPUT_TYPE_EDP;
					break;
				default:
					output_type = DISP_OUTPUT_TYPE_NONE;
					break;
				}
				sprintf(property_name, "dev%d_output_type", i);
				fdt_setprop_u32(working_fdt, node, property_name, output_type);
			}

			if (strcmp(info[j].key_name, "do_hpd") == 0 && info[j].key_value) {
				sprintf(property_name, "dev%d_do_hpd", i);
				fdt_setprop_u32(working_fdt, node, property_name, simple_strtol(info[j].key_value, NULL, 10));
			}

			if (strcmp(info[j].key_name, "screen_id") == 0 && info[j].key_value) {
				sprintf(property_name, "dev%d_screen_id", i);
				fdt_setprop_u32(working_fdt, node, property_name, simple_strtol(info[j].key_value, NULL, 10));
			}

			if (strcmp(info[j].key_name, "fb_width") == 0 && info[j].key_value) {
				sprintf(property_name, "fb%d_width", i);
				fdt_setprop_u32(working_fdt, node, property_name, simple_strtol(info[j].key_value, NULL, 10));
			}

			if (strcmp(info[j].key_name, "fb_height") == 0 && info[j].key_value) {
				sprintf(property_name, "fb%d_height", i);
				fdt_setprop_u32(working_fdt, node, property_name, simple_strtol(info[j].key_value, NULL, 10));
			}
		}
	}

	lcd_param_update(data);
	hdmi_param_update(data);
	edp_param_update(data);
	dp_param_update(data);
	hwc_param_update(data);

	return 0;
}
#endif

int disp_lcd_param_update_to_kernel(void)
{
#if defined(CONFIG_AIOT_DISP_PARAM_UPDATE) && defined(CONFIG_FAT_WRITE)
	disp_updata_param();
#endif
//	flash_update_lcd_param();
	return disp_update_lcd_param(to_update_disp_num, to_update_lcd_compat_index, UPDATE_PARAM);
}

/* type: 0:invalid, 1: int; 2:str, 3: gpio, 4: iommu master id */
int disp_sys_script_get_item(char *main_name, char *sub_name, int value[], int type)
{
	int node;
	int ret = 0;
	user_gpio_set_t  gpio_info;
	disp_gpio_set_t  *gpio_list;
	node = disp_fdt_nodeoffset(main_name);
        if (node < 0) {
		DE_DBG("fdt get node offset faill: %s\n", main_name);
		return ret;
        }

        if (1 == type) {
          if (fdt_getprop_u32(working_fdt, node, sub_name, (uint32_t *)value) >=
              0)
            ret = type;
        } else if (2 == type) {
		const char *str;
                if (fdt_getprop_string(working_fdt, node, sub_name,
                                       (char **)&str) >= 0) {
                  ret = type;
                  memcpy((void *)value, str, strlen(str) + 1);
                }
        } else if (3 == type) {
		ret = fdt_get_one_gpio_by_offset(node, sub_name, &gpio_info);
		if (ret >= 0) {
			if (ret == 4) {
				gpio_list = (disp_gpio_set_t  *)value;
				gpio_list->port = gpio_info.port;
				gpio_list->port_num = gpio_info.port_num;
				gpio_list->mul_sel = 1;
				gpio_list->drv_level = 1;
				gpio_list->pull = 0;
				gpio_list->data = gpio_info.mul_sel;
			} else {
				gpio_list = (disp_gpio_set_t  *)value;
				gpio_list->port = gpio_info.port;
				gpio_list->port_num = gpio_info.port_num;
				gpio_list->mul_sel = gpio_info.mul_sel;
				gpio_list->drv_level = gpio_info.drv_level;
				gpio_list->pull = gpio_info.pull;
				gpio_list->data = gpio_info.data;
			}

			memcpy(gpio_info.gpio_name, sub_name, strlen(sub_name)+1);
			debug("%s.%s gpio=%d,mul_sel=%d,data:%d\n",main_name, sub_name, gpio_list->gpio, gpio_list->mul_sel, gpio_list->data);
			ret = type;
		}
	} else if (type == 4) {
		if (fdtdec_get_int_array_count(working_fdt, node, sub_name, (uint32_t *)value, 1)) {
		       debug("of_property_read_u32_index %s.%s fail\n",
			     main_name, sub_name);
			ret = -1;
		} else
			ret = type;
	}

	return ret;
}

EXPORT_SYMBOL(disp_sys_script_get_item);

int disp_sys_get_ic_ver(void)
{
    return 0;
}

int disp_sys_gpio_request(disp_gpio_set_t *gpio_list, u32 group_count_max)
{
	user_gpio_set_t gpio_info;
	gpio_info.port = gpio_list->port;
	gpio_info.port_num = gpio_list->port_num;
	gpio_info.mul_sel = gpio_list->mul_sel;
	gpio_info.drv_level = gpio_list->drv_level;
	gpio_info.data = gpio_list->data;

        __inf("disp_sys_gpio_request, port:%d, port_num:%d, mul_sel:%d, "
              "pull:%d, drv_level:%d, data:%d\n",
              gpio_list->port, gpio_list->port_num, gpio_list->mul_sel,
              gpio_list->pull, gpio_list->drv_level, gpio_list->data);
        /*TODO:different name*/
	return sunxi_gpio_request(&gpio_info, group_count_max);
}
EXPORT_SYMBOL(disp_sys_gpio_request);

int disp_sys_gpio_request_simple(disp_gpio_set_t *gpio_list, u32 group_count_max)
{
	int ret = 0;
	user_gpio_set_t gpio_info;
	gpio_info.port = gpio_list->port;
	gpio_info.port_num = gpio_list->port_num;
	gpio_info.mul_sel = gpio_list->mul_sel;
	gpio_info.drv_level = gpio_list->drv_level;
	gpio_info.data = gpio_list->data;

	__inf("OSAL_GPIO_Request, port:%d, port_num:%d, mul_sel:%d, "\
		"pull:%d, drv_level:%d, data:%d\n",
		gpio_list->port, gpio_list->port_num, gpio_list->mul_sel,
		gpio_list->pull, gpio_list->drv_level, gpio_list->data);
	ret = gpio_request_early(&gpio_info, group_count_max,1);
	return ret;
}
int disp_sys_gpio_release(int p_handler, s32 if_release_to_default_status)
{
	if(p_handler != 0xffff)
	{
		gpio_release(p_handler, if_release_to_default_status);
	}
	return 0;
}
EXPORT_SYMBOL(disp_sys_gpio_release);

/* direction: 0:input, 1:output */
int disp_sys_gpio_set_direction(u32 p_handler, u32 direction, const char *gpio_name)
{
	return gpio_set_one_pin_io_status(p_handler, direction, gpio_name);
}

int disp_sys_gpio_get_value(u32 p_handler, const char *gpio_name)
{
	return gpio_read_one_pin_value(p_handler, gpio_name);
}

int disp_sys_gpio_set_value(u32 p_handler, u32 value_to_gpio, const char *gpio_name)
{
	return gpio_write_one_pin_value(p_handler, value_to_gpio, gpio_name);
}

extern int fdt_set_all_pin(const char* node_path,const char* pinctrl_name);
int disp_sys_pin_set_state(char *dev_name, char *name)
{
	int ret = -1;

	if (!strcmp(name, DISP_PIN_STATE_ACTIVE))
		ret = fdt_set_all_pin(dev_name, "pinctrl-0");
	else
		ret = fdt_set_all_pin(dev_name, "pinctrl-1");

	if (ret != 0)
		printf("%s, fdt_set_all_pin, ret=%d\n", __func__, ret);
	return ret;
}
EXPORT_SYMBOL(disp_sys_pin_set_state);

char *disp_sys_power_get(int nodeoffset, const char *name)
{
	const char *power_name;

	power_name = fdt_get_regulator_name(nodeoffset, name);

	if (power_name == NULL) {
		printf("power_name:%s parse fail!\n", power_name);
		return NULL;
	}

	if (strlen(power_name) > 16) {
		printf("power_name:%sout of lenth, keep in 16 bytes!\n", power_name);
		return NULL;
	}

	return (char *)power_name;
}

int disp_sys_power_set_voltage(char *name, u32 vol)
{
	int ret = 0;
	if (0 == strlen(name)) {
		return 0;
	}
#if defined(CONFIG_SUNXI_PMU)
	/*TODO:bmu*/
	ret = pmu_set_voltage(name, vol, -1);
	if (!ret)
		__wrn("enable power %s, ret=%d\n", name, ret);
#ifdef CONFIG_SUNXI_PMU_EXT
	if (ret != 0) {
		ret = pmu_ext_set_voltage(name, vol, -1);
		if (!ret)
			__wrn("enable power_ext %s, ret=%d\n", name, ret);
	}
#endif
#else
	__wrn("SUNXI_POWER is not enabled!\n");
#endif

	return ret;
}

int disp_sys_power_get_voltage(char *name)
{
	int ret = 0;
	if (0 == strlen(name)) {
		return 0;
	}
#if defined(CONFIG_SUNXI_PMU)
	/*TODO:bmu*/
	ret = pmu_get_voltage(name);
	if (!ret)
		__wrn("enable power %s, ret=%d\n", name, ret);
#ifdef CONFIG_SUNXI_PMU_EXT
	if (ret != 0) {
		ret = pmu_ext_get_voltage(name);
		if (!ret)
			__wrn("enable power_ext %s, ret=%d\n", name, ret);
	}
#endif
#else
	__wrn("SUNXI_POWER is not enabled!\n");
#endif

	return ret;
}

int disp_sys_power_enable(char *name)
{
	int ret = 0;
	if (0 == strlen(name)) {
		return 0;
	}
#if defined(CONFIG_SUNXI_PMU)
	/*TODO:bmu*/
	ret = pmu_set_voltage(name, 0, 1);
	if (!ret)
		__wrn("enable power %s, ret=%d\n", name, ret);
#ifdef CONFIG_SUNXI_PMU_EXT
	if (ret != 0) {
		ret = pmu_ext_set_voltage(name, 0, 1);
		if (!ret)
			__wrn("enable power_ext %s, ret=%d\n", name, ret);
	}
#endif
#else
	__wrn("SUNXI_POWER is not enabled!\n");
#endif

	return ret;
}
EXPORT_SYMBOL(disp_sys_power_enable);

int disp_sys_power_disable(char *name)
{
	int ret = 0;
	/*TODO:bmu*/
#if 0
	/*
	 * NOTE: DO NOT enable this.
	 * Because compatible panel driver may call this,
	 * which may cause other modules get wrong when using
	 * the same regulator.
	 */
	ret = pmu_set_voltage(name, 0, 0);
	if(!ret)
		__wrn("disable power %s, ret=%d\n", name, ret);
#endif
	return ret;
}
EXPORT_SYMBOL(disp_sys_power_disable);

#if defined(CONFIG_PWM_SUNXI) || defined(CONFIG_PWM_SUNXI_NEW)
uintptr_t disp_sys_pwm_request(u32 pwm_id)
{
	pwm_request(pwm_id, "lcd");
	return (pwm_id + 0x100);
}

int disp_sys_pwm_free(uintptr_t p_handler)
{
	return 0;
}

int disp_sys_pwm_enable(uintptr_t p_handler)
{
	int ret = 0;
	int pwm_id = p_handler - 0x100;

	ret = pwm_enable(pwm_id);
	return ret;

}

int disp_sys_pwm_disable(uintptr_t p_handler)
{
	int ret = 0;
	int pwm_id = p_handler - 0x100;

	pwm_disable(pwm_id);
	return ret;
}

int disp_sys_pwm_config(uintptr_t p_handler, int duty_ns, int period_ns)
{
	int ret = 0;
	int pwm_id = p_handler - 0x100;

	ret = pwm_config(pwm_id, duty_ns, period_ns);
	return ret;
}

int disp_sys_pwm_set_polarity(uintptr_t p_handler, int polarity)
{
	int ret = 0;
	int pwm_id = p_handler - 0x100;

	ret = pwm_set_polarity(pwm_id, polarity);
	return ret;
}
#else
uintptr_t disp_sys_pwm_request(u32 pwm_id)
{
	return 0;
}

int disp_sys_pwm_free(uintptr_t p_handler)
{
	return 0;
}

int disp_sys_pwm_enable(uintptr_t p_handler)
{
	return 0;
}

int disp_sys_pwm_disable(uintptr_t p_handler)
{
	return 0;
}

int disp_sys_pwm_config(uintptr_t p_handler, int duty_ns, int period_ns)
{
	return 0;
}

int disp_sys_pwm_set_polarity(uintptr_t p_handler, int polarity)
{
	return 0;
}
#endif
