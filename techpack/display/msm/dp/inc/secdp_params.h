// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2020, The Linux Foundation. All rights reserved.
 */
#ifndef _SECDP_PARAMS_H
#define _SECDP_PARAMS_H

#if defined(CONFIG_SEC_C1Q_PROJECT)
#include "secdp_params_c1q.h"
#elif defined(CONFIG_SEC_C2Q_PROJECT)
#include "secdp_params_c2q.h"
#elif defined(CONFIG_SEC_GTS7L_PROJECT)
#include "secdp_params_gts7l.h"
#elif defined(CONFIG_SEC_GTS7XL_PROJECT)
#include "secdp_params_gts7xl.h"
#elif defined(CONFIG_SEC_F2Q_PROJECT)
#include "secdp_params_f2q.h"
#elif defined(CONFIG_SEC_R8Q_PROJECT)
#include "secdp_params_r8q.h"
#elif defined(CONFIG_SEC_X1Q_PROJECT)
#include "secdp_params_x1q.h"
#elif defined(CONFIG_SEC_Y2Q_PROJECT)
#include "secdp_params_y2q.h"
#elif defined(CONFIG_SEC_Z3Q_PROJECT)
#include "secdp_params_z3q.h"
#else
#include "secdp_params_default.h"
#endif

#endif/*_SECDP_PARAMS_H*/
