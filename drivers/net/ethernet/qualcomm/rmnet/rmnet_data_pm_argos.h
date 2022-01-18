/*
 * rmnet_data_pm_argos.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */


#ifndef _RMNET_DATA_PM_ARGOS_H_
#define _RMNET_DATA_PM_ARGOS_H_

#include <linux/netdevice.h>
#include "rmnet_config.h"

struct rmnet_data_pm_ops {
    void (*boost_rps) (unsigned long speed);
    void (*pnd_chain) (unsigned long speed);
    void (*gro_count) (unsigned long speed);
    void (*tx_aggr) (unsigned long speed);
    void (*pm_qos) (unsigned long speed);
};

struct rmnet_data_pm_config {
    struct net_device *real_dev;
    struct rmnet_data_pm_ops *ops;
};

#if defined (CONFIG_IPA3)
extern void ipa3_set_napi_chained_rx(bool enable);
#else
static void ipa3_set_napi_chained_rx(bool enable)
{
    return 0;
}
#endif

#if defined (CONFIG_MHI_NETDEV)
extern void mhi_set_napi_chained_rx(struct net_device *dev, bool enable);
#else
static void mhi_set_napi_chained_rx(struct net_device *dev, bool enable)
{
    return 0;
}
#endif

#endif /* _RMNET_DATA_PM_ARGOS_H_ */
