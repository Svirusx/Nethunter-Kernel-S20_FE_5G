/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 * Authors:	James Gleeson <jagleeso@gmail.com>
 *		Wenbo Shen <wenbo.s@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/cfp.h>

#ifdef CONFIG_CFP_ROPP_SYSREGKEY
/*
 * use fixed key for debugging purpose
 * Hypervisor will generate and load the key
 */
u64 ropp_master_key = 0x55555555;
#endif

#if (defined CONFIG_CFP_ROPP_FIXKEY) || (defined SYSREG_DEBUG)
u64 ropp_fixed_key = 0x1988020519880205;
#endif
