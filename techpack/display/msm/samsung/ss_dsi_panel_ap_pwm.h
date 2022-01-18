/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef SS_DSI_PANEL_AP_PWM_H
#define SS_DSI_PANEL_AP_PWM_H

#include <linux/gpio.h>
#include <linux/of_gpio.h>

#define GP_CLK_M_DEFAULT			1
#define GP_CLK_N_DEFAULT                  127
#define GP_CLK_D_DEFAULT			127

extern void __iomem *virt_mmss_gp_base;

#define __inp(port) ioread8(port)
#define __inpw(port) ioread16(port)
#define __inpdw(port) ioread32(port)
#define __outp(port, val) iowrite8(val, port)
#define __outpw(port, val) iowrite16(val, port)
#define __outpdw(port, val) iowrite32(val, port)

#define in_dword(addr)              (__inpdw(addr))
#define in_dword_masked(addr, mask) (__inpdw(addr) & (mask))
#define out_dword(addr, val)        __outpdw(addr, val)
#define out_dword_masked(io, mask, val, shadow)  { \
	(void) out_dword(io, \
	((shadow & (unsigned int)(~(mask))) | ((unsigned int)((val) & (mask))))) \
}

#define out_dword_masked_ns(io, mask, val, current_reg_content) { \
	(void) out_dword(io, \
	((current_reg_content & (unsigned int)(~(mask))) \
	| ((unsigned int)((val) & (mask))))) \
}

#define HWIO_CAMSS_GP0_CBCR_ADDR ((void __iomem *)(virt_mmss_gp_base + 0))	/* MMSS_CC_CAMSS_GP0_CBCR */
#define HWIO_GP0_CMD_RCGR_ADDR ((void __iomem *)(virt_mmss_gp_base + 4))	/* MMSS_CC_GP0_CMD_RCGR */
#define HWIO_GP0_CFG_RCGR_ADDR ((void __iomem *)(virt_mmss_gp_base + 8))	/* MMSS_CC_GP0_CFG_RCGR */
#define HWIO_GP_M_REG_ADDR ((void __iomem *)(virt_mmss_gp_base + 0xC))		/* MMSS_CC_GP0_M */
#define HWIO_GP_NS_REG_ADDR ((void __iomem *)(virt_mmss_gp_base + 0x10))	/* MMSS_CC_GP0_N */
#define HWIO_GP_D_REG_ADDR ((void __iomem *)(virt_mmss_gp_base + 0x14))		/* MMSS_CC_GP0_D */

#define HWIO_GP_MD_REG_RMSK		        0xffffffff
#define HWIO_GP_NS_REG_RMSK		        0xffffffff
#define HWIO_GP_MD_REG_M_VAL_BMSK		0xff
#define HWIO_GP_MD_REG_M_VAL_SHFT		0
#define HWIO_GP_MD_REG_D_VAL_BMSK		0xff
#define HWIO_GP_MD_REG_D_VAL_SHFT		0
#define HWIO_GP_NS_REG_GP_N_VAL_BMSK	0xff
#define HWIO_GP_SRC_SEL_VAL_BMSK		0x700
#define HWIO_GP_SRC_SEL_VAL1_SHFT		0  /*set 1*/
#define HWIO_GP_SRC_SEL_VAL0_SHFT		8  /*set 0*/
#define HWIO_GP_SRC_DIV_VAL_BMSK		0x1f
#define HWIO_GP_SRC_DIV_VAL_SHFT		0
#define HWIO_GP_MODE_VAL_BMSK			0x3000
#define HWIO_GP_MODE_VAL_SHFT			12

#define HWIO_CLK_ENABLE_VAL_BMSK	     0x1
#define HWIO_CLK_ENABLE_VAL_SHFT	     0
#define HWIO_UPDATE_VAL_BMSK	         0x1
#define HWIO_UPDATE_VAL_SHFT	         0
#define HWIO_ROOT_EN_VAL_BMSK	         0x2
#define HWIO_ROOT_EN_VAL_SHFT	         1

#define HWIO_GP0_CMD_RCGR_IN		in_dword_masked(HWIO_GP0_CMD_RCGR_ADDR, HWIO_GP_NS_REG_RMSK)
#define HWIO_GP0_CMD_RCGR_OUTM(m, v)	out_dword_masked_ns(HWIO_GP0_CMD_RCGR_ADDR, m, v, HWIO_GP0_CMD_RCGR_IN)

#define HWIO_GP0_CFG_RCGR_IN		in_dword_masked(HWIO_GP0_CFG_RCGR_ADDR, HWIO_GP_NS_REG_RMSK)
#define HWIO_GP0_CFG_RCGR_OUTM(m, v)	out_dword_masked_ns(HWIO_GP0_CFG_RCGR_ADDR, m, v, HWIO_GP0_CFG_RCGR_IN)

#define HWIO_CAMSS_GP0_CBCR_IN		in_dword_masked(HWIO_CAMSS_GP0_CBCR_ADDR, HWIO_GP_NS_REG_RMSK)
#define HWIO_CAMSS_GP0_CBCR_OUTM(m, v)	out_dword_masked_ns(HWIO_CAMSS_GP0_CBCR_ADDR, m, v, HWIO_CAMSS_GP0_CBCR_IN)

#define HWIO_GP_D_REG_IN		in_dword_masked(HWIO_GP_D_REG_ADDR, HWIO_GP_MD_REG_RMSK)
#define HWIO_GP_D_REG_OUTM(m, v)	out_dword_masked_ns(HWIO_GP_D_REG_ADDR, m, v, HWIO_GP_D_REG_IN)

#define HWIO_GP_M_REG_IN		in_dword_masked(HWIO_GP_M_REG_ADDR, HWIO_GP_MD_REG_RMSK)
#define HWIO_GP_M_REG_OUTM(m, v)	out_dword_masked_ns(HWIO_GP_M_REG_ADDR, m, v, HWIO_GP_M_REG_IN)

#define HWIO_GP_NS_REG_IN		in_dword_masked(HWIO_GP_NS_REG_ADDR, HWIO_GP_NS_REG_RMSK)
#define HWIO_GP_NS_REG_OUTM(m, v)	out_dword_masked_ns(HWIO_GP_NS_REG_ADDR, m, v, HWIO_GP_NS_REG_IN)

#define __msmhwio_outm(hwiosym, mask, val)  HWIO_##hwiosym##_OUTM(mask, val)
#define HWIO_OUTM(hwiosym, mask, val)	__msmhwio_outm(hwiosym, mask, val)
#endif
