// SPDX-License-Identifier: GPL-2.0
/*
 * TODO: Add test description.
 */

#include <kunit/test.h>
#include <kunit/mock.h>
#include "../include/sec_battery.h"

extern bool sec_bat_get_cable_type(struct sec_battery_info *battery, int cable_source_type);
extern void sec_bat_get_charging_current_by_siop(
		struct sec_battery_info *battery, int *input_current, int *charging_current);
extern void sec_bat_get_charging_current_in_power_list(struct sec_battery_info *battery);
extern void sec_bat_get_input_current_in_power_list(struct sec_battery_info *battery);
extern int sec_bat_get_temp_by_temp_control_source(struct sec_battery_info *battery, enum sec_battery_temp_control_source tcs);
//extern u8 sec_bat_get_wireless20_power_class(struct sec_battery_info *battery);
extern int sec_bat_get_wireless_current(struct sec_battery_info *battery, int incurr);
extern void sec_bat_handle_tx_misalign(struct sec_battery_info *battery, bool trigger_misalign);
extern bool sec_bat_hv_wc_normal_mode_check(struct sec_battery_info *battery);
extern bool sec_bat_set_aging_step(struct sec_battery_info *battery, int step);
extern bool sec_bat_predict_wireless20_time_to_full_current(struct sec_battery_info *battery, int step);
extern void sec_bat_set_charging_status(struct sec_battery_info *battery, int status);
extern void sec_bat_set_decrease_iout(struct sec_battery_info *battery, bool last_delay);
extern void sec_bat_set_mfc_off(struct sec_battery_info *battery, bool need_ept);
extern void sec_bat_set_mfc_on(struct sec_battery_info *battery);
/*
 * This is the most fundamental element of KUnit, the test case. A test case
 * makes a set EXPECTATIONs and ASSERTIONs about the behavior of some code; if
 * any expectations or assertions are not met, the test fails; otherwise, the
 * test passes.
 *
 * In KUnit, a test case is just a function with the signature
 * `void (*)(struct test *)`. `struct test` is a context object that stores
 * information about the current test.
 */

#if !defined(CONFIG_UML)
/* NOTE: Target running TC must be in the #ifndef CONFIG_UML */
static void sec_battery_test_start(struct test *test)
{
	/*
	 * This is an EXPECTATION; it is how KUnit tests things. When you want
	 * to test a piece of code, you set some expectations about what the
	 * code should do. KUnit then runs the test and verifies that the code's
	 * behavior matched what was expected.
	 */
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

	cancel_delayed_work(&battery->monitor_work);
	__pm_relax(&battery->monitor_wake_lock);
}

static void sec_battery_test_end(struct test *test)
{
	/*
	 * This is an EXPECTATION; it is how KUnit tests things. When you want
	 * to test a piece of code, you set some expectations about what the
	 * code should do. KUnit then runs the test and verifies that the code's
	 * behavior matched what was expected.
	 */
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

	__pm_stay_awake(&battery->monitor_wake_lock);
	queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
}

static int fill_powerlist_pd20(PDIC_SINK_STATUS *sink_status)
{
	int i = 1;

	sink_status->power_list[i].max_voltage = 5000;
	sink_status->power_list[i].max_current = 3000;
	sink_status->power_list[i].accept = 1;
	sink_status->power_list[i++].apdo = 0;

	sink_status->power_list[i].max_voltage = 7000;
	sink_status->power_list[i].max_current = 2000;
	sink_status->power_list[i].accept = 1;
	sink_status->power_list[i++].apdo = 0;

	sink_status->power_list[i].max_voltage = 9000;
	sink_status->power_list[i].max_current = 1666;
	sink_status->power_list[i].accept = 1;
	sink_status->power_list[i++].apdo = 0;

	sink_status->power_list[i].max_voltage = 12000;
	sink_status->power_list[i].max_current = 2000;
	sink_status->power_list[i].accept = 1;
	sink_status->power_list[i++].apdo = 0;

	sink_status->has_apdo = false;

	sink_status->available_pdo_num = i-1;

	return i-1;
}

static int fill_powerlist_pd30(PDIC_SINK_STATUS *sink_status)
{
	int i = 1;

	sink_status->power_list[i].max_voltage = 5000;
	sink_status->power_list[i].max_current = 3000;
	sink_status->power_list[i].accept = 1;
	sink_status->power_list[i++].apdo = 0;

	sink_status->power_list[i].max_voltage = 9000;
	sink_status->power_list[i].max_current = 1666;
	sink_status->power_list[i].accept = 1;
	sink_status->power_list[i++].apdo = 0;

	sink_status->power_list[i].max_voltage = 12000;
	sink_status->power_list[i].max_current = 2000;
	sink_status->power_list[i].accept = 1;
	sink_status->power_list[i++].apdo = 0;

	sink_status->power_list[i].min_voltage = 3300;
	sink_status->power_list[i].max_voltage = 5900;
	sink_status->power_list[i].max_current = 3000;
	sink_status->power_list[i].accept = 1;
	sink_status->power_list[i++].apdo = 1;

	sink_status->power_list[i].min_voltage = 3300;
	sink_status->power_list[i].max_voltage = 1100;
	sink_status->power_list[i].max_current = 2250;
	sink_status->power_list[i].accept = 1;
	sink_status->power_list[i++].apdo = 1;

	sink_status->has_apdo = true;

	sink_status->available_pdo_num = i-1;

	return i-1;
}

extern int make_pd_list(struct sec_battery_info *battery);
static void sec_battery_make_pd_list(struct test *test)
{
	struct power_supply *psy;
	struct sec_battery_info *battery;
	PDIC_SINK_STATUS sink_status_backup;

	psy = power_supply_get_by_name("battery");
	if (!psy) {
		FAIL(test, "Fail to get battery psy");
		return;
	}
	battery = power_supply_get_drvdata(psy);

	sink_status_backup = battery->pdic_info.sink_status;

	fill_powerlist_pd20(&battery->pdic_info.sink_status);
	make_pd_list(battery);
	EXPECT_EQ(test, battery->pd_list.num_fpdo, 2);
	EXPECT_EQ(test, battery->pd_list.num_apdo, 0);
	EXPECT_EQ(test, battery->pd_list.max_pd_count, 2);

	fill_powerlist_pd30(&battery->pdic_info.sink_status);
	make_pd_list(battery);
	EXPECT_EQ(test, battery->pd_list.num_fpdo, 2);
	EXPECT_EQ(test, battery->pd_list.num_apdo, 2);
	EXPECT_EQ(test, battery->pd_list.max_pd_count, 4);

	battery->pdic_info.sink_status = sink_status_backup;
}

static void kunit_sec_bat_get_cable_type(struct test *test)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

	test_info(test, "START %s test\n", __func__);
	sec_bat_get_cable_type(battery, battery->pdata->cable_source_type);
	SUCCEED(test);
	test_info(test, "%s test done\n", __func__);
}

static void kunit_sec_bat_get_charging_current_by_siop(struct test *test)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

	int backup_siop_level = battery->siop_level;
	int backup_cable_type = battery->cable_type;
	int input_current = -1;
	int charging_current = -1;
	int test_siop_level[] = {100, 70, 10, 0};
	int i = 0;

	test_info(test, "START %s test\n", __func__);

	for (i = 0; i < sizeof(test_siop_level) / sizeof(int); i++) {
		battery->siop_level = test_siop_level[i];
		for(battery->cable_type = 0; battery->cable_type < SEC_BATTERY_CABLE_MAX; battery->cable_type++) {
			input_current = battery->pdata->charging_current[battery->cable_type].input_current_limit;
			charging_current = battery->pdata->charging_current[battery->cable_type].fast_charging_current;

			sec_bat_get_charging_current_by_siop(battery, &input_current, &charging_current);
			EXPECT_LE(test, input_current,
					battery->pdata->charging_current[battery->cable_type].input_current_limit);
			EXPECT_LE(test, charging_current,
					battery->pdata->charging_current[battery->cable_type].fast_charging_current);
		}
	}

	test_info(test, "%s test done\n", __func__);

	battery->siop_level = backup_siop_level;
	battery->cable_type = backup_cable_type;
}

/* sec_bat_get_input_current_in_power_list */
/* sec_bat_get_charging_current_in_power_list */
static void kunit_sec_bat_get_input_charging_current_in_power_list(struct test *test)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

	int backup_current_pdo_num = battery->pdic_info.sink_status.current_pdo_num;
	int backup_max_voltage = battery->pdic_info.sink_status.power_list[0].max_voltage;
	int backup_max_current = battery->pdic_info.sink_status.power_list[0].max_current;
	int backup_input_current_limit_pdic = battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC].input_current_limit;
	int backup_input_current_limit_apdo = battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC_APDO].input_current_limit;
	int backup_charging_current_pdic = battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC].fast_charging_current;
	int backup_charging_current_apdo = battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC_APDO].fast_charging_current;
	int backup_now_isApdo = battery->pd_list.now_isApdo;
	int backup_step_charging_status = battery->step_charging_status;
	int backup_wire_status = battery->wire_status;
	int test_voltage[] = {5000, 5000, 9000, 9000, 12000};
	int test_current[] = {2000, 3000, 2000, 3000, 2500};
	int i = 0;

	test_info(test, "START %s test\n", __func__);

	if (sizeof(test_voltage) != sizeof(test_current))
		FAIL(test, "Test cases does not match. Please check test_voltage and test_current\n");

	battery->pd_list.now_isApdo = 0;
	battery->step_charging_status = -1;
	battery->pdic_info.sink_status.current_pdo_num = 0;
	battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC].input_current_limit = 0;
	battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC_APDO].input_current_limit = 0;

	for (i = 0; i < sizeof(test_voltage) / sizeof(int); i++) {
		battery->wire_status = SEC_BATTERY_CABLE_PDIC;
		battery->pdic_info.sink_status.power_list[0].max_voltage = test_voltage[i];
		battery->pdic_info.sink_status.power_list[0].max_current = test_current[i];
		sec_bat_get_charging_current_in_power_list(battery);
		EXPECT_LE(test,
				battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC].fast_charging_current,
				battery->pdata->max_charging_current);

		battery->wire_status = SEC_BATTERY_CABLE_PDIC_APDO;
		sec_bat_get_charging_current_in_power_list(battery);
		EXPECT_LE(test,
				battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC_APDO].fast_charging_current,
				battery->pdata->max_charging_current);

		sec_bat_get_input_current_in_power_list(battery);
		EXPECT_EQ(test, battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC].input_current_limit,
				battery->pdic_info.sink_status.power_list[0].max_current);
		EXPECT_EQ(test, battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC_APDO].input_current_limit,
				battery->pdic_info.sink_status.power_list[0].max_current);
	}

	test_info(test, "%s test done\n", __func__);

	battery->pdic_info.sink_status.current_pdo_num = backup_current_pdo_num;
	battery->pdic_info.sink_status.power_list[0].max_voltage = backup_max_voltage;
	battery->pdic_info.sink_status.power_list[0].max_current = backup_max_current;
	battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC].fast_charging_current = backup_charging_current_pdic;
	battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC_APDO].fast_charging_current = backup_charging_current_apdo;
	battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC].input_current_limit = backup_input_current_limit_pdic;
	battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC_APDO].input_current_limit = backup_input_current_limit_apdo;
	battery->pd_list.now_isApdo = backup_now_isApdo;
	battery->step_charging_status = backup_step_charging_status;
	battery->wire_status = backup_wire_status;
}

static void kunit_sec_bat_get_temp_by_temp_control_source(struct test *test)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

	test_info(test, "START %s test\n", __func__);
	EXPECT_EQ(test, sec_bat_get_temp_by_temp_control_source(battery, TEMP_CONTROL_SOURCE_CHG_THM), battery->chg_temp);
	EXPECT_EQ(test, sec_bat_get_temp_by_temp_control_source(battery, TEMP_CONTROL_SOURCE_USB_THM), battery->usb_temp);
	EXPECT_EQ(test, sec_bat_get_temp_by_temp_control_source(battery, TEMP_CONTROL_SOURCE_WPC_THM), battery->wpc_temp);
	EXPECT_EQ(test, sec_bat_get_temp_by_temp_control_source(battery, TEMP_CONTROL_SOURCE_NONE), battery->temperature);
	EXPECT_EQ(test, sec_bat_get_temp_by_temp_control_source(battery, TEMP_CONTROL_SOURCE_BAT_THM), battery->temperature);
	test_info(test, "%s test done\n", __func__);
}

static void kunit_sec_bat_get_wireless20_power_class(struct test *test)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

	int backup_cable_type = battery->cable_type;

	test_info(test, "START %s test\n", __func__);
	battery->cable_type = SEC_BATTERY_CABLE_PREPARE_WIRELESS_20;
	EXPECT_EQ(test, sec_bat_get_wireless20_power_class(battery), battery->wc20_power_class);
	battery->cable_type = SEC_BATTERY_CABLE_WIRELESS;
	EXPECT_EQ(test, sec_bat_get_wireless20_power_class(battery), SEC_WIRELESS_RX_POWER_CLASS_1);
	test_info(test, "%s test done\n", __func__);

	battery->cable_type = backup_cable_type;
}

static void kunit_sec_bat_get_wireless_current(struct test *test)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	int incurr = 0;
	int backup_cable_type = battery->cable_type;
	int backup_sleep_mode = sleep_mode;
	int backup_chg_limit = battery->chg_limit;
	int backup_siop_level = battery->siop_level;
	int backup_lcd_status = battery->lcd_status;
	int backup_status = battery->status;
	int backup_charging_mode = battery->charging_mode;
	int backup_capacity = battery->capacity;
	int backup_wpc_input_limit_by_tx_check = battery->pdata->wpc_input_limit_by_tx_check;

	test_info(test, "START %s test\n", __func__);

	/* WPC_SLEEP_MODE */
	battery->cable_type = SEC_BATTERY_CABLE_HV_WIRELESS;
	sleep_mode = 1;
	for (incurr = 3000; incurr > 0; incurr -= 500)
		EXPECT_LE(test, sec_bat_get_wireless_current(battery, incurr), battery->pdata->sleep_mode_limit_current);

	sleep_mode = backup_sleep_mode;

	/* WPC_TEMP_MODE */
	battery->chg_limit = 1;

	battery->siop_level = 100;
	battery->lcd_status = 0;
	for (incurr = 3000; incurr > 0; incurr -= 500) {
		if (incurr > battery->pdata->wpc_input_limit_current) {
			battery->cable_type = SEC_BATTERY_CABLE_WIRELESS_TX;
			battery->pdata->wpc_input_limit_by_tx_check = 1;
			EXPECT_LE(test, sec_bat_get_wireless_current(battery, incurr),
					battery->pdata->wpc_input_limit_current_by_tx);
			battery->pdata->wpc_input_limit_by_tx_check = backup_wpc_input_limit_by_tx_check;

			battery->cable_type = SEC_BATTERY_CABLE_WIRELESS;
			EXPECT_LE(test, sec_bat_get_wireless_current(battery, incurr), battery->pdata->wpc_input_limit_current);
		}
	}

	battery->siop_level = 70;
	battery->lcd_status = 1;
	for (incurr = 3000; incurr > 0; incurr -= 500)
		EXPECT_LE(test, sec_bat_get_wireless_current(battery, incurr), battery->pdata->wpc_lcd_on_input_limit_current);

	battery->chg_limit = backup_chg_limit;
	battery->siop_level = backup_siop_level;
	battery->lcd_status = backup_lcd_status;

	/* Full-Additional state */
	battery->status = POWER_SUPPLY_STATUS_FULL;
	battery->charging_mode = SEC_BATTERY_CHARGING_2ND;
	for (incurr = 3000; incurr > 0; incurr -= 500)
		EXPECT_LE(test, sec_bat_get_wireless_current(battery, incurr),
				battery->pdata->siop_hv_wireless_input_limit_current);

	battery->status = backup_status;
	battery->charging_mode = backup_charging_mode;

	/* Hero Stand Pad CV */
	battery->capacity = 100;
	battery->cable_type = SEC_BATTERY_CABLE_WIRELESS_STAND;
	for (incurr = 3000; incurr > 0; incurr -= 500)
		EXPECT_LE(test, sec_bat_get_wireless_current(battery, incurr), battery->pdata->wc_hero_stand_cv_current);

	battery->cable_type = SEC_BATTERY_CABLE_WIRELESS_HV_STAND;
	battery->chg_limit = 1;
	for (incurr = 3000; incurr > 0; incurr -= 500)
		EXPECT_LE(test, sec_bat_get_wireless_current(battery, incurr), battery->pdata->wc_hero_stand_cv_current);

	battery->chg_limit = 0;
	for (incurr = 3000; incurr > 0; incurr -= 500)
		EXPECT_LE(test, sec_bat_get_wireless_current(battery, incurr), battery->pdata->wc_hero_stand_hv_cv_current);

	battery->chg_limit = backup_chg_limit;
	battery->cable_type = backup_cable_type;
	battery->capacity = backup_capacity;

	/* Full-None state && SIOP_LEVEL 100 */
	battery->siop_level = 100;
	battery->lcd_status = 0;
	battery->status = POWER_SUPPLY_STATUS_FULL;
	battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
	for (incurr = 3000; incurr > 0; incurr -= 500)
		EXPECT_EQ(test, sec_bat_get_wireless_current(battery, incurr), battery->pdata->wc_full_input_limit_current);

	battery->siop_level = backup_siop_level;
	battery->lcd_status = backup_lcd_status;
	battery->status = backup_status;
	battery->charging_mode = backup_charging_mode;

	test_info(test, "%s test done\n", __func__);

	battery->cable_type = backup_cable_type;

}

static void kunit_sec_bat_handle_tx_misalign(struct test *test)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

	unsigned int backup_tx_event = battery->tx_event;

	test_info(test, "START %s test\n", __func__);

	battery->tx_misalign_cnt = 1;
	sec_bat_handle_tx_misalign(battery, 1);
	EXPECT_EQ(test, (battery->tx_retry_case & SEC_BAT_TX_RETRY_MISALIGN) ? true : false, true);

	battery->tx_misalign_cnt = 3;
	sec_bat_handle_tx_misalign(battery, 1);
	EXPECT_EQ(test, (battery->tx_retry_case & SEC_BAT_TX_RETRY_MISALIGN) ? true : false, false);

	battery->tx_misalign_cnt = 0;
	battery->tx_misalign_passed_time = 0;
	battery->tx_event = backup_tx_event;

	test_info(test, "%s test done\n", __func__);
}

static void kunit_sec_bat_hv_wc_normal_mode_check(struct test *test)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

	int backup_sleep_mode = sleep_mode;

	test_info(test, "START %s test\n", __func__);

	sleep_mode = 1;
	EXPECT_EQ(test, sec_bat_hv_wc_normal_mode_check(battery), true);

	sleep_mode = backup_sleep_mode;

	test_info(test, "%s test done\n", __func__);
}

static void kunit_sec_bat_init_chg_work(struct test *test)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

	int backup_cable_type = battery->cable_type;
	unsigned int backup_misc_event = battery->misc_event;
	unsigned int backup_charger_mode = battery->charger_mode;


	test_info(test, "START %s test\n", __func__);

	battery->cable_type = SEC_BATTERY_CABLE_NONE;
	battery->misc_event = 0;
	queue_delayed_work(battery->monitor_wqueue, &battery->init_chg_work, 0);

	SUCCEED(test);
	battery->cable_type = backup_cable_type;
	battery->misc_event = backup_misc_event;
	sec_bat_set_charge(battery, backup_charger_mode);

	test_info(test, "%s test done\n", __func__);
}

static void kunit_sec_bat_set_aging_step(struct test *test)
{
        struct power_supply *psy = power_supply_get_by_name("battery");
        struct sec_battery_info *battery = power_supply_get_drvdata(psy);

        int backup_age_step = battery->pdata->age_step;
        unsigned int backup_chg_float_voltage = battery->pdata->chg_float_voltage;
        unsigned int backup_swelling_normal_float_voltage = battery->pdata->swelling_normal_float_voltage;
        unsigned int backup_recharge_condition_vcell = battery->pdata->recharge_condition_vcell;
        unsigned int backup_full_condition_soc = battery->pdata->full_condition_soc;
        unsigned int backup_full_condition_vcell = battery->pdata->full_condition_vcell;

        int i = 0;

	test_info(test, "START %s test\n", __func__);

        for(i = 0; i < battery->pdata->num_age_step; i++) {
                sec_bat_set_aging_step(battery, i);
                EXPECT_EQ(test, battery->pdata->age_step, i);
                EXPECT_EQ(test, battery->pdata->chg_float_voltage, battery->pdata->age_data[i].float_voltage);
                EXPECT_EQ(test, battery->pdata->swelling_normal_float_voltage, battery->pdata->chg_float_voltage);
                EXPECT_EQ(test, battery->pdata->recharge_condition_vcell, battery->pdata->age_data[i].recharge_condition_vcell);
                EXPECT_EQ(test, battery->pdata->full_condition_soc, battery->pdata->age_data[i].full_condition_soc);
                EXPECT_EQ(test, battery->pdata->full_condition_vcell, battery->pdata->age_data[i].full_condition_vcell);
        }

        battery->pdata->age_step = backup_age_step;
        battery->pdata->chg_float_voltage = backup_chg_float_voltage;
        battery->pdata->swelling_normal_float_voltage = backup_swelling_normal_float_voltage;
        battery->pdata->recharge_condition_vcell = backup_recharge_condition_vcell;
        battery->pdata->full_condition_soc = backup_full_condition_soc;
        battery->pdata->full_condition_vcell = backup_full_condition_vcell;

	test_info(test, "%s test done\n", __func__);
}

static void kunit_sec_bat_predict_wireless20_time_to_full_current(struct test *test)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

        unsigned int backup_ttf_predict_wc20_charge_current = battery->ttf_predict_wc20_charge_current;

        int i;

	test_info(test, "START %s test\n", __func__);

        for(i = 0; i < SEC_WIRELESS_RX_POWER_MAX; i++) {
                sec_bat_predict_wireless20_time_to_full_current(battery, i);
                EXPECT_EQ(test, battery->ttf_predict_wc20_charge_current, battery->pdata->wireless_power_info[i].ttf_charge_current);
        }
        
        battery->ttf_predict_wc20_charge_current = backup_ttf_predict_wc20_charge_current;

	test_info(test, "%s test done\n", __func__);
}

static void kunit_sec_bat_set_charging_status(struct test *test)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

        int backup_status = battery->status;
        int i = 0;

	test_info(test, "START %s test\n", __func__);

        for (i = 0; i <= POWER_SUPPLY_STATUS_FULL; i++) {
                sec_bat_set_charging_status(battery, i);
                EXPECT_EQ(test, i, battery->status);
        }

        battery->status = backup_status;
        sec_bat_set_charging_status(battery, battery->status);
        
	test_info(test, "%s test done\n", __func__);
}

static void kunit_sec_bat_set_decrease_iout(struct test *test)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	union power_supply_propval value = {0, };

        int backup_input_current = battery->input_current;

	test_info(test, "START %s test\n", __func__);

        sec_bat_set_decrease_iout(battery, false);

        psy_do_property(battery->pdata->charger_name, get,
                        POWER_SUPPLY_PROP_CURRENT_MAX, value);

        EXPECT_LE(test, backup_input_current, value.intval);

        value.intval = backup_input_current;
	psy_do_property(battery->pdata->charger_name, set,
		POWER_SUPPLY_EXT_PROP_PAD_VOLT_CTRL, value);


	test_info(test, "%s test done\n", __func__);

}

static void kunit_sec_bat_set_mfc_onoff(struct test *test)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

        int backup_val = gpio_get_value(battery->pdata->wpc_en);
        int new_val = 0;

	test_info(test, "START %s test\n", __func__);

        sec_bat_set_mfc_on(battery);
        new_val = gpio_get_value(battery->pdata->wpc_en);
        EXPECT_EQ(test, new_val, 0);

        sec_bat_set_mfc_off(battery, false);
        new_val = gpio_get_value(battery->pdata->wpc_en);
        EXPECT_EQ(test, new_val, 1);

        gpio_direction_output(battery->pdata->wpc_en, backup_val);

	test_info(test, "%s test done\n", __func__);
}
#endif

/* NOTE: UML TC */
static void sec_battery_test_bar(struct test *test)
{
	/* Test cases for UML */
	return;
}

/*
 * This is run once before each test case, see the comment on
 * example_test_module for more information.
 */
static int sec_battery_test_init(struct test *test)
{
	return 0;
}

/*
 * This is run once after each test case, see the comment on example_test_module
 * for more information.
 */
static void sec_battery_test_exit(struct test *test)
{
}

/*
 * Here we make a list of all the test cases we want to add to the test module
 * below.
 */
static struct test_case sec_battery_test_cases[] = {
	/*
	 * This is a helper to create a test case object from a test case
	 * function; its exact function is not important to understand how to
	 * use KUnit, just know that this is how you associate test cases with a
	 * test module.
	 */
#if !defined(CONFIG_UML)
	/* NOTE: Target running TC */

	TEST_CASE(sec_battery_test_start),
	/* test_start has cancel_delayed_work of monitor_work */

	TEST_CASE(sec_battery_make_pd_list),
	TEST_CASE(kunit_sec_bat_get_cable_type),
	TEST_CASE(kunit_sec_bat_get_charging_current_by_siop),
	TEST_CASE(kunit_sec_bat_get_input_charging_current_in_power_list),
	TEST_CASE(kunit_sec_bat_get_temp_by_temp_control_source),
	TEST_CASE(kunit_sec_bat_get_wireless20_power_class),
	TEST_CASE(kunit_sec_bat_get_wireless_current),
	TEST_CASE(kunit_sec_bat_handle_tx_misalign),
	TEST_CASE(kunit_sec_bat_hv_wc_normal_mode_check),
	TEST_CASE(kunit_sec_bat_init_chg_work),

	TEST_CASE(kunit_sec_bat_set_aging_step),
	TEST_CASE(kunit_sec_bat_predict_wireless20_time_to_full_current),
	TEST_CASE(kunit_sec_bat_set_charging_status),
	TEST_CASE(kunit_sec_bat_set_decrease_iout),
	TEST_CASE(kunit_sec_bat_set_mfc_onoff),

	TEST_CASE(sec_battery_test_end),
	/* test_end has queue_delayed_work of monitor_work */

#endif
	/* NOTE: UML TC */
	TEST_CASE(sec_battery_test_bar),
	{},
};

/*
 * This defines a suite or grouping of tests.
 *
 * Test cases are defined as belonging to the suite by adding them to
 * `test_cases`.
 *
 * Often it is desirable to run some function which will set up things which
 * will be used by every test; this is accomplished with an `init` function
 * which runs before each test case is invoked. Similarly, an `exit` function
 * may be specified which runs after every test case and can be used to for
 * cleanup. For clarity, running tests in a test module would behave as follows:
 *
 * module.init(test);
 * module.test_case[0](test);
 * module.exit(test);
 * module.init(test);
 * module.test_case[1](test);
 * module.exit(test);
 * ...;
 */
static struct test_module sec_battery_test_module = {
	.name = "sec_battery_test",
	.init = sec_battery_test_init,
	.exit = sec_battery_test_exit,
	.test_cases = sec_battery_test_cases,
};

/*
 * This registers the above test module telling KUnit that this is a suite of
 * tests that need to be run.
 */
module_test(sec_battery_test_module);
