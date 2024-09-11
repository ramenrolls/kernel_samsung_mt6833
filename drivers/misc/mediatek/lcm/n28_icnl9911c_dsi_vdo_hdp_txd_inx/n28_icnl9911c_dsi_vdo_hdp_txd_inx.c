
// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#define LOG_TAG "LCM"
#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif
#include "lcm_drv.h"
#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include "disp_dts_gpio.h"
#endif
#ifndef MACH_FPGA
#include <lcm_pmic.h>
#endif
#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif

extern char *saved_command_line;

static struct LCM_UTIL_FUNCS lcm_util;
#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define MDELAY(n)		(lcm_util.mdelay(n))
#define UDELAY(n)		(lcm_util.udelay(n))
#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
		lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
	  lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
		lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
/* #include <linux/jiffies.h> */
/* #include <linux/delay.h> */
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#endif
#define LCM_DSI_CMD_MODE									0
#define FRAME_WIDTH										(720)
#define FRAME_HEIGHT									(1600)
//+S96818AA1-1936,liyuhong1.wt,modify,2023/06/14,nt36528 modify the physical size of the screen
#define LCM_PHYSICAL_WIDTH								(70380)
#define LCM_PHYSICAL_HEIGHT								(156240)
//+S96818AA1-1936,liyuhong1.wt,modify,2023/06/14,nt36528 modify the physical size of the screen
#define REGFLAG_DELAY			0xFFFC
#define REGFLAG_UDELAY			0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD
#define REGFLAG_RESET_LOW		0xFFFE
#define REGFLAG_RESET_HIGH		0xFFFF
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};
static struct LCM_setting_table lcm_suspend_setting[] = {
	// SLEEP IN + DISPLAY OFF
	{0x26, 1,{0x08}},
	{0x28, 0,{0x00}},
	{REGFLAG_DELAY, 20,{}},
	{0x10, 0,{0x00}},
	{REGFLAG_DELAY, 120,{}},
};
static struct LCM_setting_table init_setting_vdo[] = {
	{0xF0, 2,{0x5A,0x59}},
	{0xF1, 2,{0xA5,0xA6}},
	{0xF6,1,{0x3F}},
	{0xB0,30,{0x83,0x82,0x86,0x87,0x06,0x07,0x07,0x08,0x33,0x33,0x33,0x33,0x05,0x00,0x00,0x6F,0x01,0x01,0x3F,0x05,0x04,0x03,0x02,0x01,0x02,0x03,0x04,0x00,0x00,0x00}},
	{0xB1,29,{0x13,0x8C,0x8D,0x00,0x05,0x00,0x00,0x6F,0x00,0x00,0x34,0x84,0x88,0x00,0x00,0x00,0x44,0x40,0x02,0x01,0x40,0x02,0x01,0x40,0x02,0x01,0x40,0x02,0x01}},
	{0xB2,17,{0x54,0xC4,0x82,0x8A,0x40,0x02,0x01,0x40,0x02,0x01,0x05,0x05,0x54,0x0C,0x0C,0x0D,0x0B}},
	{0xB3,31,{0x11,0x00,0x00,0x00,0x00,0x26,0x26,0x91,0xA2,0x33,0x44,0x3C,0x26,0x00,0x18,0x01,0x02,0x08,0x20,0x30,0x08,0x09,0x44,0x20,0x40,0x20,0x40,0x08,0x09,0x22,0x33}},
	{0xB4,28,{0x02,0x00,0x00,0x06,0x1F,0x1E,0x0C,0x0E,0x10,0x12,0x14,0x16,0x04,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0xFF,0xFF,0xFC,0x00,0x00,0x00}},
	{0xB5,28,{0x02,0x00,0x00,0x07,0x1F,0x1E,0x0D,0x0F,0x11,0x13,0x15,0x17,0x05,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0xFF,0xFF,0xFC,0x00,0x00,0x00}},
	{0xB8,24,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
	{0xBB,13,{0x01,0x05,0x09,0x11,0x0D,0x19,0x1D,0x55,0x25,0x69,0x00,0x21,0x25}},
	{0xBC,14,{0x00,0x00,0x00,0x00,0x02,0x20,0xFF,0x00,0x03,0x33,0x01,0x73,0x33,0x02}},
	{0xBD,10,{0xE9,0x02,0x4E,0xCF,0x72,0xA4,0x08,0x44,0xAE,0x15}},
	{0xBE,10,{0x6D,0x6D,0x46,0x5A,0x0C,0x77,0x43,0x07,0x0E,0x0E}},
	{0xBF, 8,{0x07,0x25,0x07,0x25,0x7F,0x00,0x11,0x04}},
	{0xC0, 9,{0x10,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0xFF,0x00}},
	{0xC1,19,{0xC0,0x20,0x20,0x96,0x04,0x48,0x48,0x04,0x2A,0x40,0x36,0x00,0x07,0xC0,0x10,0xFF,0x95,0x01,0xC0}},
	{0xC2, 6,{0xCC,0x01,0x10,0x00,0x01,0x30}},
	{0xC3, 9,{0x06,0x00,0xFF,0x00,0xFF,0x00,0x00,0x81,0x01}},
	{0xC4,11,{0x84,0x01,0x2B,0x41,0x00,0x3C,0x00,0x03,0x03,0x2E,0x00}},
	{0xC5,11,{0x03,0x1C,0xC0,0xC0,0x40,0x10,0x42,0x44,0x0F,0x09,0x14}},
	{0xC6,10,{0x87,0x96,0x2A,0x29,0x29,0x31,0x7F,0x34,0x08,0x04}},
	{0xC7,22,{0xFC,0xD9,0xBF,0xAA,0x84,0x67,0x39,0x8D,0x57,0x2A,0x05,0xD8,0x2D,0xFC,0xDB,0xA8,0x8B,0x61,0x1A,0x7F,0xF4,0x00}},
	{0xC8,22,{0xFC,0xD9,0xBF,0xAA,0x84,0x67,0x39,0x8D,0x57,0x2A,0x05,0xD8,0x2D,0xFC,0xDB,0xA8,0x8B,0x61,0x1A,0x7F,0xF4,0x00}},
	{0xCB, 1,{0x00}},
	{0xD0, 8,{0x80,0x0D,0xFF,0x0F,0x61,0x0B,0x08,0x04}},
	{0xD2, 1,{0x42}},
	{0xE0,26,{0x30,0x00,0x80,0x88,0x11,0x3F,0x22,0x62,0xDF,0xA0,0x04,0xCC,0x01,0xFF,0xF6,0xFF,0xF0,0xFD,0xFF,0xFD,0xF8,0xF5,0xFC,0xFC,0xFD,0xFF}},
	{0xE1,23,{0xEF,0xFE,0xFE,0xFE,0xFE,0xEE,0xF0,0x20,0x33,0xFF,0x00,0x00,0x6A,0x90,0xC0,0x0D,0x6A,0xF0,0x3E,0xFF,0x00,0x06,0x40}},
	{0xFA, 3,{0x45,0x93,0x01}},
	{0xFE, 4,{0xFF,0xFF,0xFF,0x40}},
	{0xF1, 2,{0x5A,0x59}},
	{0xF0, 2,{0xA5,0xA6}},
	{0x35, 1,{0x00}},
	{0x51, 2,{0xff,0xff}},
	{0x53, 1,{0x2C}},
	//{0x5E, 2,{0x00,0x0F}},
	{0x11, 0,{0x00}},
	{REGFLAG_DELAY, 110,{}},
	{0x29, 0,{0x00}},
	{REGFLAG_DELAY, 10,{}},
	{0x26, 1,{0x01}},
	{ REGFLAG_END_OF_TABLE, 0x00, {}}
};
static struct LCM_setting_table bl_level[] = {
	{ 0x51, 0x02, {0x0F,0xFF} },
	{ REGFLAG_END_OF_TABLE, 0x00, {} }
};
static void push_table(void *cmdq, struct LCM_setting_table *table,
	unsigned int count, unsigned char force_update)
{
	unsigned int i;
	unsigned cmd;
	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;
		switch (cmd) {
		case REGFLAG_DELAY:
			if (table[i].count <= 10)
				MDELAY(table[i].count);
			else
				MDELAY(table[i].count);
			break;
		case REGFLAG_UDELAY:
			UDELAY(table[i].count);
			break;
		case REGFLAG_END_OF_TABLE:
			break;
		default:
			dsi_set_cmdq_V22(cmdq, cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}
static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}
static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));
	params->type = LCM_TYPE_DSI;
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->physical_width = LCM_PHYSICAL_WIDTH/1000;
	params->physical_height = LCM_PHYSICAL_HEIGHT/1000;
	params->physical_width_um = LCM_PHYSICAL_WIDTH;
	params->physical_height_um = LCM_PHYSICAL_HEIGHT;
#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
#endif
	params->dsi.switch_mode_enable = 0;
	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = 256;
	/* video mode timing */
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.vertical_sync_active = 4;
	params->dsi.vertical_backporch = 32;
	params->dsi.vertical_frontporch = 150;
	//params->dsi.vertical_frontporch_for_low_power = 540;/*disable dynamic frame rate*/
	params->dsi.vertical_active_line = FRAME_HEIGHT;
	params->dsi.horizontal_sync_active = 4;
	params->dsi.horizontal_backporch = 72;
	params->dsi.horizontal_frontporch = 72;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_range = 4;
	params->dsi.ssc_disable = 1;
#ifndef CONFIG_FPGA_EARLY_PORTING
#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 282;	/* this value must be in MTK suggested table */
#else
	params->dsi.PLL_CLOCK = 295;	/* this value must be in MTK suggested table */
	params->dsi.data_rate = 590;
#endif
	//params->dsi.PLL_CK_CMD = 220;
	//params->dsi.PLL_CK_VDO = 255;
#else
	params->dsi.pll_div1 = 0;
	params->dsi.pll_div2 = 0;
	params->dsi.fbk_div = 0x1;
#endif
	/* mipi hopping setting params */
	params->dsi.dynamic_switch_mipi = 1;
	params->dsi.data_rate_dyn = 610;
	params->dsi.PLL_CLOCK_dyn = 305;
	params->dsi.horizontal_sync_active_dyn = 4;
	params->dsi.horizontal_backporch_dyn = 84;
	params->dsi.horizontal_frontporch_dyn  = 88;

	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
#ifdef CONFIG_MTK_ROUND_CORNER_SUPPORT
	params->round_corner_en = 0;
	params->corner_pattern_width = 720;
	params->corner_pattern_height = 32;
#endif
}
enum Color {
	LOW,
	HIGH
};
static void lcm_init_power(void)
{
	int ret = 0;
	pr_debug("[LCM]icnl9911c_txd lcm_init_power !\n");
    lcm_reset_pin(LOW);
	ret = lcm_power_enable();
    MDELAY(11);
}

//extern bool chipone_gestrue_status;
static void lcm_suspend_power(void)
{
	//if (chipone_gestrue_status == 1) {
	//	pr_debug("[LCM] icnl9911c_txd lcm_suspend_power chipone_gestrue_status = 1!\n");
	//} else {
		int ret  = 0;
		pr_debug("[LCM] icnl9911c_txd lcm_suspend_power chipone_gestrue_status = 0!\n");
		//lcm_reset_pin(LOW);
		MDELAY(2);
		ret = lcm_power_disable();
		MDELAY(10);
	//}
}
static void lcm_resume_power(void)
{
	pr_debug("[LCM]icnl9911c_txd lcm_resume_power !\n");
	lcm_init_power();
}

unsigned char g_lcm_f6_reg_txd = 0x00;
static void get_lcm_fb_cmdline(void)
{
	int ret = 0;
	char *lcm_f6_reg_start=NULL;
    char *temp;
	char lcm_f6_reg[8]={'\0'};
	lcm_f6_reg_start = strstr(saved_command_line, "lcm_f6_reg=");
	if(lcm_f6_reg_start == NULL){
		pr_info("[LCM]command_line have non lcm_f6_reg info.\n");
        return;
	}
	temp = lcm_f6_reg_start + strlen("lcm_f6_reg=");
	memcpy(lcm_f6_reg, temp, 4);
    pr_info("[LCM kernel]Read LCM_F6_REG from command_line is %s.\n", lcm_f6_reg);
	ret =  kstrtou8(lcm_f6_reg, 0, &g_lcm_f6_reg_txd);
	if (ret != 0){
		pr_info("[LCM]Convert lcm_f6_reg string to unsigned int error.\n");
	}
	pr_info("[LCM kernel] lcm_f6_reg = 0x%02x.\n", g_lcm_f6_reg_txd);
}

static void lcm_init(void)
{
	pr_debug("[LCM] icnl9911c_txd lcm_init\n");
	lcm_reset_pin(HIGH);
	MDELAY(5);
	lcm_reset_pin(LOW);
	MDELAY(5);
	lcm_reset_pin(HIGH);
	MDELAY(10);
    LCM_LOGI("lcm config: 0xf6 = 0x%x\n", g_lcm_f6_reg_txd);
    init_setting_vdo[2].para_list[0] = g_lcm_f6_reg_txd;
	push_table(NULL, init_setting_vdo, sizeof(init_setting_vdo) / sizeof(struct LCM_setting_table), 1);
	pr_debug("[LCM] icnl9911c_txd----init success ----\n");
}
static void lcm_suspend(void)
{
	pr_debug("[LCM] lcm_suspend\n");
	push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
}
static void lcm_resume(void)
{
	pr_debug("[LCM] lcm_resume\n");
    get_lcm_fb_cmdline();
	lcm_init();
}
static unsigned int lcm_compare_id(void)
{
	return 1;
}
static unsigned int lcm_ata_check(unsigned char *buffer)
{
#ifndef BUILD_LK
	unsigned int ret = 0;
	unsigned int x0 = FRAME_WIDTH / 4;
	unsigned int x1 = FRAME_WIDTH * 3 / 4;
	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);
	unsigned int data_array[3];
	unsigned char read_buf[4];
	pr_debug("ATA check size = 0x%x,0x%x,0x%x,0x%x\n", x0_MSB, x0_LSB, x1_MSB, x1_LSB);
	data_array[0] = 0x0005390A;	/* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	data_array[0] = 0x00043700;	/* read id return two byte,version and id */
	dsi_set_cmdq(data_array, 1, 1);
	read_reg_v2(0x2A, read_buf, 4);
	if ((read_buf[0] == x0_MSB) && (read_buf[1] == x0_LSB)
	    && (read_buf[2] == x1_MSB) && (read_buf[3] == x1_LSB))
		ret = 1;
	else
		ret = 0;
	x0 = 0;
	x1 = FRAME_WIDTH - 1;
	x0_MSB = ((x0 >> 8) & 0xFF);
	x0_LSB = (x0 & 0xFF);
	x1_MSB = ((x1 >> 8) & 0xFF);
	x1_LSB = (x1 & 0xFF);
	data_array[0] = 0x0005390A;	/* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	return ret;
#else
	return 0;
#endif
}
static void lcm_setbacklight_cmdq(void* handle,unsigned int level)
{
	unsigned int bl_lvl;
	bl_lvl = wingtech_bright_to_bl(level,255,10,4095,48);
	pr_debug("%s,incl9911c_txd backlight: level = %d,bl_lvl=%d\n", __func__, level,bl_lvl);
	// set 12bit
	bl_level[0].para_list[0] = (bl_lvl & 0xF00) >> 8;
	bl_level[0].para_list[1] = (bl_lvl & 0xFF);
	pr_debug("incl9911c_txd backlight: para_list[0]=%x,para_list[1]=%x\n",bl_level[0].para_list[0],bl_level[0].para_list[1]);
	push_table(handle, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}
struct LCM_DRIVER n28_icnl9911c_dsi_vdo_hdp_txd_inx_drv = {
	.name = "n28_icnl9911c_dsi_vdo_hdp_txd_inx",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id = lcm_compare_id,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.set_backlight_cmdq = lcm_setbacklight_cmdq,
	.ata_check = lcm_ata_check,
};
