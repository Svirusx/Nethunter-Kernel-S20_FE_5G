/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */

#ifndef LINUX_RANGE_SENSOR_H
#define LINUX_RANGE_SENSOR_H

#define NUMBER_OF_ZONE   64
#define NUMBER_OF_TARGET  2

struct range_sensor_data_t {

#ifdef CONFIG_SENSOR_VL53L5_SUPPORT_DEPTH_16
   int16_t range_data[NUMBER_OF_ZONE][NUMBER_OF_TARGET];
#else
   int16_t range_data[NUMBER_OF_ZONE][NUMBER_OF_TARGET];
   uint8_t status[NUMBER_OF_ZONE][NUMBER_OF_TARGET];
#endif

};

#endif
