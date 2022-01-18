/*
 *  sec_adc.c
 *  Samsung Mobile Battery Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "include/sec_adc.h"

#define DEBUG

struct adc_list {
	const char *name;
	struct iio_channel *channel;
	bool is_used;
	int prev_value;
};

static struct adc_list batt_adc_list[SEC_BAT_ADC_CHANNEL_NUM] = {
	{.name = "adc-cable"},
	{.name = "adc-bat"},
	{.name = "adc-temp"},
	{.name = "adc-temp-amb"},
	{.name = "adc-full"},
	{.name = "adc-volt"},
	{.name = "adc-chg-temp"},
	{.name = "adc-in-bat"},
	{.name = "adc-dischg"},
	{.name = "adc-dischg-ntc"},
	{.name = "adc-wpc-temp"},
	{.name = "adc-slave-chg-temp"},
	{.name = "adc-usb-temp"},
	{.name = "adc-sub-bat"},
	{.name = "adc-blkt-temp"},
};

#ifdef CONFIG_SEC_EXT_THERMAL_MONITOR
static struct sec_battery_info *local_battery;
#endif /* CONFIG_SEC_EXT_THERMAL_MONITOR */

static void sec_bat_adc_ap_init(struct platform_device *pdev,
		struct sec_battery_info *battery)
{
	int i = 0;
	struct iio_channel *temp_adc;

	for (i = 0; i < SEC_BAT_ADC_CHANNEL_NUM; i++) {
		temp_adc = iio_channel_get(&pdev->dev, batt_adc_list[i].name);
		batt_adc_list[i].channel = temp_adc;
		batt_adc_list[i].is_used = !IS_ERR_OR_NULL(temp_adc);
	}

	for (i  = 0; i < SEC_BAT_ADC_CHANNEL_NUM; i++)
		pr_info("%s %s - %s\n",
			__func__, batt_adc_list[i].name, batt_adc_list[i].is_used ? "used" : "not used");
}

static int sec_bat_adc_ap_read(struct sec_battery_info *battery, int channel)
{
	int data = -1;
	int ret = 0;
	int retry_cnt = RETRY_CNT;

	if (batt_adc_list[channel].is_used) {
		do {
			ret = (batt_adc_list[channel].is_used) ?
			iio_read_channel_processed(batt_adc_list[channel].channel, &data) : 0;
			retry_cnt--;
		} while ((retry_cnt > 0) && (data < 0));
	}

	if (retry_cnt <= 0) {
		pr_err("%s: Error in ADC\n", __func__);
		data = batt_adc_list[channel].prev_value;
	} else
		batt_adc_list[channel].prev_value = data;

	return data;
}

static void sec_bat_adc_ap_exit(void)
{
	return;
}

static void sec_bat_adc_none_init(struct platform_device *pdev)
{
}

static int sec_bat_adc_none_read(int channel)
{
	return 0;
}

static void sec_bat_adc_none_exit(void)
{
}

static void sec_bat_adc_ic_init(struct platform_device *pdev)
{
}

static int sec_bat_adc_ic_read(int channel)
{
	return 0;
}

static void sec_bat_adc_ic_exit(void)
{
}
static int adc_read_type(struct sec_battery_info *battery, int channel)
{
	int adc = 0;

	switch (battery->pdata->temp_adc_type) {
	case SEC_BATTERY_ADC_TYPE_NONE:
		adc = sec_bat_adc_none_read(channel);
		break;
	case SEC_BATTERY_ADC_TYPE_AP:
		adc = sec_bat_adc_ap_read(battery, channel);
		break;
	case SEC_BATTERY_ADC_TYPE_IC:
		adc = sec_bat_adc_ic_read(channel);
		break;
	case SEC_BATTERY_ADC_TYPE_NUM:
		break;
	default:
		break;
	}
	pr_debug("[%s] [%d] ADC = %d\n", __func__, channel, adc);

	return adc;
}

static void adc_init_type(struct platform_device *pdev,
			  struct sec_battery_info *battery)
{
	switch (battery->pdata->temp_adc_type) {
	case SEC_BATTERY_ADC_TYPE_NONE:
		sec_bat_adc_none_init(pdev);
		break;
	case SEC_BATTERY_ADC_TYPE_AP:
		sec_bat_adc_ap_init(pdev, battery);
		break;
	case SEC_BATTERY_ADC_TYPE_IC:
		sec_bat_adc_ic_init(pdev);
		break;
	case SEC_BATTERY_ADC_TYPE_NUM:
		break;
	default:
		break;
	}
}

static void adc_exit_type(struct sec_battery_info *battery)
{
	switch (battery->pdata->temp_adc_type) {
	case SEC_BATTERY_ADC_TYPE_NONE:
		sec_bat_adc_none_exit();
		break;
	case SEC_BATTERY_ADC_TYPE_AP:
		sec_bat_adc_ap_exit();
		break;
	case SEC_BATTERY_ADC_TYPE_IC:
		sec_bat_adc_ic_exit();
		break;
	case SEC_BATTERY_ADC_TYPE_NUM:
		break;
	default:
		break;
	}
}

int sec_bat_get_adc_data(struct sec_battery_info *battery,
			int adc_ch, int count)
{
	int adc_data = 0;
	int adc_max = 0;
	int adc_min = 0xFFFF;
	int adc_total = 0;
	int i = 0;

	if (count < 3)
		count = 3;

	for (i = 0; i < count; i++) {
		mutex_lock(&battery->adclock);
#ifdef CONFIG_OF
		adc_data = adc_read_type(battery, adc_ch);
#else
		adc_data = adc_read_type(battery->pdata, adc_ch);
#endif
		mutex_unlock(&battery->adclock);

		if (i != 0) {
			if (adc_data > adc_max)
				adc_max = adc_data;
			else if (adc_data < adc_min)
				adc_min = adc_data;
		} else {
			adc_max = adc_data;
			adc_min = adc_data;
		}
		adc_total += adc_data;
	}

	return (adc_total - adc_max - adc_min) / (count - 2);
}

int sec_bat_get_charger_type_adc
				(struct sec_battery_info *battery)
{
	/* It is true something valid is
	connected to the device for charging.
	By default this something is considered to be USB.*/
	int result = SEC_BATTERY_CABLE_USB;

	int adc = 0;
	int i = 0;

	/* Do NOT check cable type when cable_switch_check() returns false
	 * and keep current cable type
	 */
	if (battery->pdata->cable_switch_check &&
	    !battery->pdata->cable_switch_check())
		return battery->cable_type;

	adc = sec_bat_get_adc_data(battery,
		SEC_BAT_ADC_CHANNEL_CABLE_CHECK,
		battery->pdata->adc_check_count);

	/* Do NOT check cable type when cable_switch_normal() returns false
	 * and keep current cable type
	 */
	if (battery->pdata->cable_switch_normal &&
	    !battery->pdata->cable_switch_normal())
		return battery->cable_type;

	for (i = 0; i < SEC_BATTERY_CABLE_MAX; i++)
		if ((adc > battery->pdata->cable_adc_value[i].min) &&
			(adc < battery->pdata->cable_adc_value[i].max))
			break;
	if (i >= SEC_BATTERY_CABLE_MAX)
		dev_err(battery->dev,
			"%s : default USB\n", __func__);
	else
		result = i;

	dev_dbg(battery->dev, "%s : result(%d), adc(%d)\n",
		__func__, result, adc);

	return result;
}

bool sec_bat_get_value_by_adc(
				struct sec_battery_info *battery,
				enum sec_battery_adc_channel channel,
				union power_supply_propval *value,
				enum sec_battery_temp_check check_type)
{
	int temp = 0;
	int temp_adc;
	int low = 0;
	int high = 0;
	int mid = 0;
	const sec_bat_adc_table_data_t *temp_adc_table = {0 , };
	unsigned int temp_adc_table_size = 0;

	if (check_type == SEC_BATTERY_TEMP_CHECK_FAKE) {
		value->intval = 300;
		return true;
	}

	temp_adc = sec_bat_get_adc_data(battery, channel, battery->pdata->adc_check_count);
	if (temp_adc < 0)
		return false;

	switch (channel) {
	case SEC_BAT_ADC_CHANNEL_TEMP:
		temp_adc_table = battery->pdata->temp_adc_table;
		temp_adc_table_size =
			battery->pdata->temp_adc_table_size;
		battery->temp_adc = temp_adc;
		break;
	case SEC_BAT_ADC_CHANNEL_TEMP_AMBIENT:
		temp_adc_table = battery->pdata->temp_amb_adc_table;
		temp_adc_table_size =
			battery->pdata->temp_amb_adc_table_size;
		battery->temp_ambient_adc = temp_adc;
		break;
	case SEC_BAT_ADC_CHANNEL_USB_TEMP:
		temp_adc_table = battery->pdata->usb_temp_adc_table;
		temp_adc_table_size =
			battery->pdata->usb_temp_adc_table_size;
		battery->usb_temp_adc = temp_adc;
		break;
	case SEC_BAT_ADC_CHANNEL_CHG_TEMP:
		temp_adc_table = battery->pdata->chg_temp_adc_table;
		temp_adc_table_size =
		battery->pdata->chg_temp_adc_table_size;
		battery->chg_temp_adc = temp_adc;
		break;
	case SEC_BAT_ADC_CHANNEL_WPC_TEMP:
		temp_adc_table = battery->pdata->wpc_temp_adc_table;
		temp_adc_table_size =
			battery->pdata->wpc_temp_adc_table_size;
		battery->wpc_temp_adc = temp_adc;
		break;
	case SEC_BAT_ADC_CHANNEL_SLAVE_CHG_TEMP:
		temp_adc_table = battery->pdata->slave_chg_temp_adc_table;
		temp_adc_table_size =
			battery->pdata->slave_chg_temp_adc_table_size;
		battery->slave_chg_temp_adc = temp_adc;
		break;
	case SEC_BAT_ADC_CHANNEL_SUB_BAT_TEMP:
		temp_adc_table = battery->pdata->sub_bat_temp_adc_table;
		temp_adc_table_size =
			battery->pdata->sub_bat_temp_adc_table_size;
		battery->sub_bat_temp_adc = temp_adc;
		break;		
	case SEC_BAT_ADC_CHANNEL_BLKT_TEMP:
		temp_adc_table = battery->pdata->blkt_temp_adc_table;
		temp_adc_table_size =
			battery->pdata->blkt_temp_adc_table_size;
		battery->blkt_temp_adc = temp_adc;
		break;
	default:
		dev_err(battery->dev,
			"%s: Invalid Property\n", __func__);
		return false;
	}

	if (temp_adc_table[0].adc >= temp_adc) {
		temp = temp_adc_table[0].data;
		goto temp_by_adc_goto;
	} else if (temp_adc_table[temp_adc_table_size-1].adc <= temp_adc) {
		temp = temp_adc_table[temp_adc_table_size-1].data;
		goto temp_by_adc_goto;
	}

	high = temp_adc_table_size - 1;

	while (low <= high) {
		mid = (low + high) / 2;
		if (temp_adc_table[mid].adc > temp_adc)
			high = mid - 1;
		else if (temp_adc_table[mid].adc < temp_adc)
			low = mid + 1;
		else {
			temp = temp_adc_table[mid].data;
			goto temp_by_adc_goto;
		}
	}

	temp = temp_adc_table[high].data;
	temp += ((temp_adc_table[low].data - temp_adc_table[high].data) *
		 (temp_adc - temp_adc_table[high].adc)) /
		(temp_adc_table[low].adc - temp_adc_table[high].adc);

temp_by_adc_goto:
	value->intval = temp;

	dev_dbg(battery->dev,
		"%s:[%d] Temp(%d), Temp-ADC(%d)\n",
		__func__,channel, temp, temp_adc);

	return true;
}

#ifdef CONFIG_SEC_EXT_THERMAL_MONITOR
int sec_bat_convert_adc_to_temp(unsigned int adc_ch, int temp_adc)
{
	enum sec_battery_adc_channel channel = 0;
	int temp = 25000;
	int low = 0;
	int high = 0;
	int mid = 0;
	const sec_bat_adc_table_data_t *temp_adc_table = {0 , };
	unsigned int temp_adc_table_size = 0;

	if(!local_battery) {
		pr_info("%s: battery data is not ready yet\n", __func__);
		goto temp_to_adc_goto;
	}

	if (adc_ch == 0x4d &&
		local_battery->pdata->usb_thermal_source == SEC_BATTERY_THERMAL_SOURCE_ADC)
		channel = SEC_BAT_ADC_CHANNEL_USB_TEMP;
	else if (adc_ch == 0x52 &&
		local_battery->pdata->thermal_source == SEC_BATTERY_THERMAL_SOURCE_ADC)
		channel = SEC_BAT_ADC_CHANNEL_TEMP;
	else
		goto temp_to_adc_goto;

	switch (channel) {
	case SEC_BAT_ADC_CHANNEL_USB_TEMP:
		temp_adc_table = local_battery->pdata->usb_temp_adc_table;
		temp_adc_table_size =
			local_battery->pdata->usb_temp_adc_table_size;
		break;
	case SEC_BAT_ADC_CHANNEL_TEMP:
		temp_adc_table = local_battery->pdata->temp_adc_table;
		temp_adc_table_size =
			local_battery->pdata->temp_adc_table_size;
		break;
	default:
		dev_err(local_battery->dev,
			"%s: Invalid Property\n", __func__);
		goto temp_to_adc_goto;
	}

	if (temp_adc_table[0].adc >= temp_adc) {
		temp = temp_adc_table[0].data;
		goto temp_to_adc_goto;
	} else if (temp_adc_table[temp_adc_table_size-1].adc <= temp_adc) {
		temp = temp_adc_table[temp_adc_table_size-1].data;
		goto temp_to_adc_goto;
	}

	high = temp_adc_table_size - 1;

	while (low <= high) {
		mid = (low + high) / 2;
		if (temp_adc_table[mid].adc > temp_adc)
			high = mid - 1;
		else if (temp_adc_table[mid].adc < temp_adc)
			low = mid + 1;
		else {
			temp = temp_adc_table[mid].data;
			goto temp_to_adc_goto;
		}
	}

	temp = temp_adc_table[high].data;
	temp += ((temp_adc_table[low].data - temp_adc_table[high].data) *
		 (temp_adc - temp_adc_table[high].adc)) /
		(temp_adc_table[low].adc - temp_adc_table[high].adc);

temp_to_adc_goto:
	return (temp == 25000 ? temp : temp * 100);
}
EXPORT_SYMBOL(sec_bat_convert_adc_to_temp);

int sec_bat_get_thr_voltage(unsigned int adc_ch, int temp)
{
	enum sec_battery_adc_channel channel = 0;
	int volt = 0;
	int low = 0;
	int high = 0;
	int mid = 0;
	const sec_bat_adc_table_data_t *temp_adc_table = {0 , };
	unsigned int temp_adc_table_size = 0;

	if(!local_battery) {
		pr_info("%s: battery data is not ready yet\n", __func__);
		goto get_thr_voltage_goto;
	}

	if (adc_ch == 0x4d &&
		local_battery->pdata->usb_thermal_source == SEC_BATTERY_THERMAL_SOURCE_ADC)
		channel = SEC_BAT_ADC_CHANNEL_USB_TEMP;
	else if (adc_ch == 0x52 &&
		local_battery->pdata->thermal_source == SEC_BATTERY_THERMAL_SOURCE_ADC)
		channel = SEC_BAT_ADC_CHANNEL_TEMP;
	else
		goto get_thr_voltage_goto;

	switch (channel) {
	case SEC_BAT_ADC_CHANNEL_USB_TEMP:
		temp_adc_table = local_battery->pdata->usb_temp_adc_table;
		temp_adc_table_size =
			local_battery->pdata->usb_temp_adc_table_size;
		break;
	case SEC_BAT_ADC_CHANNEL_TEMP:
		temp_adc_table = local_battery->pdata->temp_adc_table;
		temp_adc_table_size =
			local_battery->pdata->temp_adc_table_size;
		break;
	default:
		dev_err(local_battery->dev,
			"%s: Invalid Property\n", __func__);
		goto get_thr_voltage_goto;
	}

 	if (temp > 900 || temp < -200) {
		dev_err(local_battery->dev,
			"%s: unsupported temperature\n", __func__);
		goto get_thr_voltage_goto;
	}

	high = temp_adc_table_size - 1;

	while (low <= high) {
		mid = (low + high) / 2;
		if (temp_adc_table[mid].data < temp)
			high = mid - 1;
		else if (temp_adc_table[mid].data > temp)
			low = mid + 1;
		else {
			volt = temp_adc_table[mid].adc / 1000;
			goto get_thr_voltage_goto;
		}
	}

	volt = temp_adc_table[mid].adc / 1000;

get_thr_voltage_goto:
	return volt;
}
EXPORT_SYMBOL(sec_bat_get_thr_voltage);
#endif /* CONFIG_SEC_EXT_THERMAL_MONITOR */

bool sec_bat_check_vf_adc(struct sec_battery_info *battery)
{
	int adc = 0;

	adc = sec_bat_get_adc_data(battery,
		SEC_BAT_ADC_CHANNEL_BAT_CHECK,
		battery->pdata->adc_check_count);

	if (adc < 0) {
		dev_err(battery->dev, "%s: VF ADC error\n", __func__);
		adc = battery->check_adc_value;
	} else
		battery->check_adc_value = adc;

	if ((battery->check_adc_value <= battery->pdata->check_adc_max) &&
		(battery->check_adc_value >= battery->pdata->check_adc_min)) {
		return true;
	} else {
		dev_info(battery->dev, "%s: adc (%d)\n", __func__, battery->check_adc_value);
		return false;
	}
}

#if defined(CONFIG_DIRECT_CHARGING)
int sec_bat_get_direct_chg_temp_adc(struct sec_battery_info *battery,
			int adc_data, int count)
{
	int temp = 0;
	int temp_adc = 0;
	int low = 0;
	int high = 0;
	int mid = 0;
	const sec_bat_adc_table_data_t *temp_adc_table = {0 , };
	unsigned int temp_adc_table_size = 0;

	if (battery->pdata->dchg_temp_check_type == SEC_BATTERY_TEMP_CHECK_FAKE) {
		temp = 300;
		goto direct_chg_temp_goto;
	}

	temp_adc = adc_data;
	if (temp_adc < 0)
		return 0;

	temp_adc_table = battery->pdata->dchg_temp_adc_table;
	temp_adc_table_size =
		battery->pdata->dchg_temp_adc_table_size;
	battery->dchg_temp_adc = temp_adc;

	if (temp_adc_table[0].adc >= temp_adc) {
			temp = temp_adc_table[0].data;
			goto direct_chg_temp_goto;
	} else if (temp_adc_table[temp_adc_table_size-1].adc <= temp_adc) {
		temp = temp_adc_table[temp_adc_table_size-1].data;
		goto direct_chg_temp_goto;
	}
	
	high = temp_adc_table_size - 1;
	while (low <= high) {
		mid = (low + high) / 2;
		if (temp_adc_table[mid].adc > temp_adc)
			high = mid - 1;
		else if (temp_adc_table[mid].adc < temp_adc)
			low = mid + 1;
		else {
			temp = temp_adc_table[mid].data;
			goto direct_chg_temp_goto;
		}
	}

	temp = temp_adc_table[high].data;
	temp += ((temp_adc_table[low].data - temp_adc_table[high].data) *
		 (temp_adc - temp_adc_table[high].adc)) /
		(temp_adc_table[low].adc - temp_adc_table[high].adc);

direct_chg_temp_goto:
	dev_info(battery->dev,
			"%s: temp(%d), direct-chg-temp-ADC(%d)\n",
			__func__, temp, temp_adc);

	return temp;
}
#endif

void adc_init(struct platform_device *pdev, struct sec_battery_info *battery)
{
	adc_init_type(pdev, battery);
#ifdef CONFIG_SEC_EXT_THERMAL_MONITOR
	local_battery = battery;
#endif /* CONFIG_SEC_EXT_THERMAL_MONITOR */
}

void adc_exit(struct sec_battery_info *battery)
{
	adc_exit_type(battery);
}
