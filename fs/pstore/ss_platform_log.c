/*
 * fs/pstore/ss_platform_log.c
 *
 * Copyright (C) 2016-2017 Samsung Electronics
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/sched/clock.h>
#include <linux/slab.h>
#include <asm/unaligned.h>

#include <linux/sec_debug.h>
#include <linux/sec_bootstat.h>

/* This defines are for PSTORE */
#define SS_LOGGER_LEVEL_HEADER		(1)
#define SS_LOGGER_LEVEL_PREFIX		(2)
#define SS_LOGGER_LEVEL_TEXT		(3)
#define SS_LOGGER_LEVEL_MAX		(4)
#define SS_LOGGER_SKIP_COUNT		(4)
#define SS_LOGGER_STRING_PAD		(1)
#define SS_LOGGER_HEADER_SIZE		(80)

#define SS_LOG_ID_MAIN			(0)
#define SS_LOG_ID_RADIO			(1)
#define SS_LOG_ID_EVENTS		(2)
#define SS_LOG_ID_SYSTEM		(3)
#define SS_LOG_ID_CRASH			(4)
#define SS_LOG_ID_KERNEL		(5)

#define MAX_BUFFER_SIZE	1024

struct ss_pmsg_log_header_t {
	uint8_t magic;
	uint16_t len;
	uint16_t uid;
	uint16_t pid;
} __attribute__((__packed__));

struct ss_android_log_header_t {
	unsigned char id;
	uint16_t tid;
	int32_t tv_sec;
	int32_t tv_nsec;
} __attribute__((__packed__));

struct ss_logger {
	uint16_t len;
	uint16_t id;
	uint16_t pid;
	uint16_t tid;
	uint16_t uid;
	uint16_t level;
	int32_t tv_sec;
	int32_t tv_nsec;
	char msg[0];
	char *buffer;
	void (*func_hook_logger)(const char*, size_t);
};

static struct ss_logger logger;

#ifdef CONFIG_SEC_EVENT_LOG
struct event_log_tag_t {
	int nTagNum;
	char *event_msg;
};

enum event_type {
	EVENT_TYPE_INT = 0,
	EVENT_TYPE_LONG,
	EVENT_TYPE_STRING,
	EVENT_TYPE_LIST,
	EVENT_TYPE_FLOAT,
};

/* NOTICE : it must have order. */
static struct event_log_tag_t event_tags[] = {
	{ 42, "answer" },
	{ 314, "pi" },
	{ 1003, "auditd" },
	{ 1004, "chatty" },
	{ 1005, "tag_def" },
	{ 1006, "liblog" },
	{ 2718, "e" },
	{ 2719, "configuration_changed" },
	{ 2720, "sync" },
	{ 2721, "cpu" },
	{ 2722, "battery_level" },
	{ 2723, "battery_status" },
	{ 2724, "power_sleep_requested" },
	{ 2725, "power_screen_broadcast_send" },
	{ 2726, "power_screen_broadcast_done" },
	{ 2727, "power_screen_broadcast_stop" },
	{ 2728, "power_screen_state" },
	{ 2729, "power_partial_wake_state" },
	{ 2730, "battery_discharge" },
	{ 2731, "power_soft_sleep_requested" },
	{ 2732, "storaged_disk_stats" },
	{ 2733, "storaged_emmc_info" },
	{ 2739, "battery_saver_mode" },
	{ 2740, "location_controller" },
	{ 2741, "force_gc" },
	{ 2742, "tickle" },
	{ 2747, "contacts_aggregation" },
	{ 2748, "cache_file_deleted" },
	{ 2749, "storage_state" },
	{ 2750, "notification_enqueue" },
	{ 2751, "notification_cancel" },
	{ 2752, "notification_cancel_all" },
	{ 2755, "fstrim_start" },
	{ 2756, "fstrim_finish" },
	{ 2802, "watchdog" },
	{ 2803, "watchdog_proc_pss" },
	{ 2804, "watchdog_soft_reset" },
	{ 2805, "watchdog_hard_reset" },
	{ 2806, "watchdog_pss_stats" },
	{ 2807, "watchdog_proc_stats" },
	{ 2808, "watchdog_scheduled_reboot" },
	{ 2809, "watchdog_meminfo" },
	{ 2810, "watchdog_vmstat" },
	{ 2811, "watchdog_requested_reboot" },
	{ 2820, "backup_data_changed" },
	{ 2821, "backup_start" },
	{ 2822, "backup_transport_failure" },
	{ 2823, "backup_agent_failure" },
	{ 2824, "backup_package" },
	{ 2825, "backup_success" },
	{ 2826, "backup_reset" },
	{ 2827, "backup_initialize" },
	{ 2828, "backup_requested" },
	{ 2829, "backup_quota_exceeded" },
	{ 2830, "restore_start" },
	{ 2831, "restore_transport_failure" },
	{ 2832, "restore_agent_failure" },
	{ 2833, "restore_package" },
	{ 2834, "restore_success" },
	{ 2840, "full_backup_package" },
	{ 2841, "full_backup_agent_failure" },
	{ 2842, "full_backup_transport_failure" },
	{ 2843, "full_backup_success" },
	{ 2844, "full_restore_package" },
	{ 2845, "full_backup_quota_exceeded" },
	{ 2846, "full_backup_cancelled" },
	{ 2850, "backup_transport_lifecycle" },
	{ 2851, "backup_transport_connection" },
	{ 2900, "rescue_note" },
	{ 2901, "rescue_level" },
	{ 2902, "rescue_success" },
	{ 2903, "rescue_failure" },
	{ 3000, "boot_progress_start" },
	{ 3010, "boot_progress_system_run" },
	{ 3020, "boot_progress_preload_start" },
	{ 3030, "boot_progress_preload_end" },
	{ 3040, "boot_progress_ams_ready" },
	{ 3050, "boot_progress_enable_screen" },
	{ 3060, "boot_progress_pms_start" },
	{ 3070, "boot_progress_pms_system_scan_start" },
	{ 3080, "boot_progress_pms_data_scan_start" },
	{ 3090, "boot_progress_pms_scan_end" },
	{ 3100, "boot_progress_pms_ready" },
	{ 3110, "unknown_sources_enabled" },
	{ 3120, "pm_critical_info" },
	{ 3121, "pm_package_stats" },
	{ 4000, "calendar_upgrade_receiver" },
	{ 4100, "contacts_upgrade_receiver" },
	{ 8000, "job_deferred_execution" },
	{ 20003, "dvm_lock_sample" },
	{ 20004, "art_hidden_api_access" },
	{ 27390, "battery_saving_stats" },
	{ 27391, "user_activity_timeout_override" },
	{ 27392, "battery_saver_setting" },
	{ 27500, "notification_panel_revealed" },
	{ 27501, "notification_panel_hidden" },
	{ 27510, "notification_visibility_changed" },
	{ 27511, "notification_expansion" },
	{ 27520, "notification_clicked" },
	{ 27521, "notification_action_clicked" },
	{ 27530, "notification_canceled" },
	{ 27531, "notification_visibility" },
	{ 27532, "notification_alert" },
	{ 27533, "notification_autogrouped" },
	{ 30001, "am_finish_activity" },
	{ 30002, "am_task_to_front" },
	{ 30003, "am_new_intent" },
	{ 30004, "am_create_task" },
	{ 30005, "am_create_activity" },
	{ 30006, "am_restart_activity" },
	{ 30007, "am_resume_activity" },
	{ 30008, "am_anr" },
	{ 30009, "am_activity_launch_time" },
	{ 30010, "am_proc_bound" },
	{ 30011, "am_proc_died" },
	{ 30012, "am_failed_to_pause" },
	{ 30013, "am_pause_activity" },
	{ 30014, "am_proc_start" },
	{ 30015, "am_proc_bad" },
	{ 30016, "am_proc_good" },
	{ 30017, "am_low_memory" },
	{ 30018, "am_destroy_activity" },
	{ 30019, "am_relaunch_resume_activity" },
	{ 30020, "am_relaunch_activity" },
	{ 30021, "am_on_paused_called" },
	{ 30022, "am_on_resume_called" },
	{ 30023, "am_kill" },
	{ 30024, "am_broadcast_discard_filter" },
	{ 30025, "am_broadcast_discard_app" },
	{ 30030, "am_create_service" },
	{ 30031, "am_destroy_service" },
	{ 30032, "am_process_crashed_too_much" },
	{ 30033, "am_drop_process" },
	{ 30034, "am_service_crashed_too_much" },
	{ 30035, "am_schedule_service_restart" },
	{ 30036, "am_provider_lost_process" },
	{ 30037, "am_process_start_timeout" },
	{ 30039, "am_crash" },
	{ 30040, "am_wtf" },
	{ 30041, "am_switch_user" },
	{ 30042, "am_activity_fully_drawn_time" },
	{ 30043, "am_set_resumed_activity" },
	{ 30044, "am_focused_stack" },
	{ 30045, "am_pre_boot" },
	{ 30046, "am_meminfo" },
	{ 30047, "am_pss" },
	{ 30048, "am_stop_activity" },
	{ 30049, "am_on_stop_called" },
	{ 30050, "am_mem_factor" },
	{ 30051, "am_user_state_changed" },
	{ 30052, "am_uid_running" },
	{ 30053, "am_uid_stopped" },
	{ 30054, "am_uid_active" },
	{ 30055, "am_uid_idle" },
	{ 30056, "am_stop_idle_service" },
	{ 30057, "am_on_create_called" },
	{ 30058, "am_on_restart_called" },
	{ 30059, "am_on_start_called" },
	{ 30060, "am_on_destroy_called" },
	{ 30061, "am_remove_task" },
	{ 30062, "am_on_activity_result_called" },
	{ 31000, "wm_no_surface_memory" },
	{ 31001, "wm_task_created" },
	{ 31002, "wm_task_moved" },
	{ 31003, "wm_task_removed" },
	{ 31004, "wm_stack_created" },
	{ 31005, "wm_home_stack_moved" },
	{ 31006, "wm_stack_removed" },
	{ 31007, "wm_boot_animation_done" },
	{ 32000, "imf_force_reconnect_ime" },
	{ 33000, "wp_wallpaper_crashed" },
	{ 34000, "device_idle" },
	{ 34001, "device_idle_step" },
	{ 34002, "device_idle_wake_from_idle" },
	{ 34003, "device_idle_on_start" },
	{ 34004, "device_idle_on_phase" },
	{ 34005, "device_idle_on_complete" },
	{ 34006, "device_idle_off_start" },
	{ 34007, "device_idle_off_phase" },
	{ 34008, "device_idle_off_complete" },
	{ 34009, "device_idle_light" },
	{ 34010, "device_idle_light_step" },
	{ 35000, "auto_brightness_adj" },
	{ 36000, "sysui_statusbar_touch" },
	{ 36001, "sysui_heads_up_status" },
	{ 36002, "sysui_fullscreen_notification" },
	{ 36003, "sysui_heads_up_escalation" },
	{ 36004, "sysui_status_bar_state" },
	{ 36010, "sysui_panelbar_touch" },
	{ 36020, "sysui_notificationpanel_touch" },
	{ 36021, "sysui_lockscreen_gesture" },
	{ 36030, "sysui_quickpanel_touch" },
	{ 36040, "sysui_panelholder_touch" },
	{ 36050, "sysui_searchpanel_touch" },
	{ 36060, "sysui_recents_connection" },
	{ 36070, "sysui_latency" },
	{ 40000, "volume_changed" },
	{ 40001, "stream_devices_changed" },
	{ 40100, "camera_gesture_triggered" },
	{ 50000, "menu_item_selected" },
	{ 50001, "menu_opened" },
	{ 50020, "connectivity_state_changed" },
	{ 50021, "wifi_state_changed" },
	{ 50022, "wifi_event_handled" },
	{ 50023, "wifi_supplicant_state_changed" },
	{ 50080, "ntp_success" },
	{ 50081, "ntp_failure" },
	{ 50100, "pdp_bad_dns_address" },
	{ 50101, "pdp_radio_reset_countdown_triggered" },
	{ 50102, "pdp_radio_reset" },
	{ 50103, "pdp_context_reset" },
	{ 50104, "pdp_reregister_network" },
	{ 50105, "pdp_setup_fail" },
	{ 50106, "call_drop" },
	{ 50107, "data_network_registration_fail" },
	{ 50108, "data_network_status_on_radio_off" },
	{ 50109, "pdp_network_drop" },
	{ 50110, "cdma_data_setup_failed" },
	{ 50111, "cdma_data_drop" },
	{ 50112, "gsm_rat_switched" },
	{ 50113, "gsm_data_state_change" },
	{ 50114, "gsm_service_state_change" },
	{ 50115, "cdma_data_state_change" },
	{ 50116, "cdma_service_state_change" },
	{ 50117, "bad_ip_address" },
	{ 50118, "data_stall_recovery_get_data_call_list" },
	{ 50119, "data_stall_recovery_cleanup" },
	{ 50120, "data_stall_recovery_reregister" },
	{ 50121, "data_stall_recovery_radio_restart" },
	{ 50122, "data_stall_recovery_radio_restart_with_prop" },
	{ 50123, "gsm_rat_switched_new" },
	{ 50125, "exp_det_sms_denied_by_user" },
	{ 50128, "exp_det_sms_sent_by_user" },
	{ 51100, "netstats_mobile_sample" },
	{ 51101, "netstats_wifi_sample" },
	{ 51200, "lockdown_vpn_connecting" },
	{ 51201, "lockdown_vpn_connected" },
	{ 51202, "lockdown_vpn_error" },
	{ 51300, "config_install_failed" },
	{ 51400, "ifw_intent_matched" },
	{ 51500, "idle_maintenance_window_start" },
	{ 51501, "idle_maintenance_window_finish" },
	{ 51600, "timezone_trigger_check" },
	{ 51610, "timezone_request_install" },
	{ 51611, "timezone_install_started" },
	{ 51612, "timezone_install_complete" },
	{ 51620, "timezone_request_uninstall" },
	{ 51621, "timezone_uninstall_started" },
	{ 51622, "timezone_uninstall_complete" },
	{ 51630, "timezone_request_nothing" },
	{ 51631, "timezone_nothing_complete" },
	{ 51690, "timezone_check_trigger_received" },
	{ 51691, "timezone_check_read_from_data_app" },
	{ 51692, "timezone_check_request_uninstall" },
	{ 51693, "timezone_check_request_install" },
	{ 51694, "timezone_check_request_nothing" },
	{ 52000, "db_sample" },
	{ 52001, "http_stats" },
	{ 52002, "content_query_sample" },
	{ 52003, "content_update_sample" },
	{ 52004, "binder_sample" },
	{ 52100, "unsupported_settings_query" },
	{ 52101, "persist_setting_error" },
	{ 53000, "harmful_app_warning_uninstall" },
	{ 53001, "harmful_app_warning_launch_anyway" },
	{ 60000, "viewroot_draw" },
	{ 60001, "viewroot_layout" },
	{ 60002, "view_build_drawing_cache" },
	{ 60003, "view_use_drawing_cache" },
	{ 60100, "sf_frame_dur" },
	{ 60110, "sf_stop_bootanim" },
	{ 65537, "exp_det_netlink_failure" },
	{ 70000, "screen_toggled" },
	{ 70101, "browser_zoom_level_change" },
	{ 70102, "browser_double_tap_duration" },
	{ 70150, "browser_snap_center" },
	{ 70151, "exp_det_attempt_to_call_object_getclass" },
	{ 70200, "aggregation" },
	{ 70201, "aggregation_test" },
	{ 70220, "gms_unknown" },
	{ 70301, "phone_ui_enter" },
	{ 70302, "phone_ui_exit" },
	{ 70303, "phone_ui_button_click" },
	{ 70304, "phone_ui_ringer_query_elapsed" },
	{ 70305, "phone_ui_multiple_query" },
	{ 71001, "qsb_start" },
	{ 71002, "qsb_click" },
	{ 71003, "qsb_search" },
	{ 71004, "qsb_voice_search" },
	{ 71005, "qsb_exit" },
	{ 71006, "qsb_latency" },
	{ 75000, "sqlite_mem_alarm_current" },
	{ 75001, "sqlite_mem_alarm_max" },
	{ 75002, "sqlite_mem_alarm_alloc_attempt" },
	{ 75003, "sqlite_mem_released" },
	{ 75004, "sqlite_db_corrupt" },
	{ 76001, "tts_speak_success" },
	{ 76002, "tts_speak_failure" },
	{ 76003, "tts_v2_speak_success" },
	{ 76004, "tts_v2_speak_failure" },
	{ 78001, "exp_det_dispatchCommand_overflow" },
	{ 80100, "bionic_event_memcpy_buffer_overflow" },
	{ 80105, "bionic_event_strcat_buffer_overflow" },
	{ 80110, "bionic_event_memmov_buffer_overflow" },
	{ 80115, "bionic_event_strncat_buffer_overflow" },
	{ 80120, "bionic_event_strncpy_buffer_overflow" },
	{ 80125, "bionic_event_memset_buffer_overflow" },
	{ 80130, "bionic_event_strcpy_buffer_overflow" },
	{ 80200, "bionic_event_strcat_integer_overflow" },
	{ 80205, "bionic_event_strncat_integer_overflow" },
	{ 80300, "bionic_event_resolver_old_response" },
	{ 80305, "bionic_event_resolver_wrong_server" },
	{ 80310, "bionic_event_resolver_wrong_query" },
	{ 81002, "dropbox_file_copy" },
	{ 90100, "exp_det_cert_pin_failure" },
	{ 90200, "lock_screen_type" },
	{ 90201, "exp_det_device_admin_activated_by_user" },
	{ 90202, "exp_det_device_admin_declined_by_user" },
	{ 90203, "exp_det_device_admin_uninstalled_by_user" },
	{ 90204, "settings_latency" },
	{ 201001, "system_update" },
	{ 201002, "system_update_user" },
	{ 202001, "vending_reconstruct" },
	{ 202901, "transaction_event" },
	{ 203001, "sync_details" },
	{ 203002, "google_http_request" },
	{ 204001, "gtalkservice" },
	{ 204002, "gtalk_connection" },
	{ 204003, "gtalk_conn_close" },
	{ 204004, "gtalk_heartbeat_reset" },
	{ 204005, "c2dm" },
	{ 205001, "setup_server_timeout" },
	{ 205002, "setup_required_captcha" },
	{ 205003, "setup_io_error" },
	{ 205004, "setup_server_error" },
	{ 205005, "setup_retries_exhausted" },
	{ 205006, "setup_no_data_network" },
	{ 205007, "setup_completed" },
	{ 205008, "gls_account_tried" },
	{ 205009, "gls_account_saved" },
	{ 205010, "gls_authenticate" },
	{ 205011, "google_mail_switch" },
	{ 206001, "snet" },
	{ 206003, "exp_det_snet" },
	{ 208000, "metrics_heartbeat" },
	{ 210001, "security_adb_shell_interactive" },
	{ 210002, "security_adb_shell_command" },
	{ 210003, "security_adb_sync_recv" },
	{ 210004, "security_adb_sync_send" },
	{ 210005, "security_app_process_start" },
	{ 210006, "security_keyguard_dismissed" },
	{ 210007, "security_keyguard_dismiss_auth_attempt" },
	{ 210008, "security_keyguard_secured" },
	{ 210009, "security_os_startup" },
	{ 210010, "security_os_shutdown" },
	{ 210011, "security_logging_started" },
	{ 210012, "security_logging_stopped" },
	{ 210013, "security_media_mounted" },
	{ 210014, "security_media_unmounted" },
	{ 210015, "security_log_buffer_size_critical" },
	{ 210016, "security_password_expiration_set" },
	{ 210017, "security_password_complexity_set" },
	{ 210018, "security_password_history_length_set" },
	{ 210019, "security_max_screen_lock_timeout_set" },
	{ 210020, "security_max_password_attempts_set" },
	{ 210021, "security_keyguard_disabled_features_set" },
	{ 210022, "security_remote_lock" },
	{ 210023, "security_wipe_failed" },
	{ 210024, "security_key_generated" },
	{ 210025, "security_key_imported" },
	{ 210026, "security_key_destroyed" },
	{ 210027, "security_user_restriction_added" },
	{ 210028, "security_user_restriction_removed" },
	{ 210029, "security_cert_authority_installed" },
	{ 210030, "security_cert_authority_removed" },
	{ 210031, "security_crypto_self_test_completed" },
	{ 210032, "security_key_integrity_violation" },
	{ 210033, "security_cert_validation_failure" },
	{ 230000, "service_manager_stats" },
	{ 230001, "service_manager_slow" },
	{ 275534, "notification_unautogrouped" },
	{ 300000, "arc_system_event" },
	{ 524287, "sysui_view_visibility" },
	{ 524288, "sysui_action" },
	{ 524290, "sysui_count" },
	{ 524291, "sysui_histogram" },
	{ 524292, "sysui_multi_action" },
	{ 525000, "commit_sys_config_file" },
	{ 1010000, "bt_hci_timeout" },
	{ 1010001, "bt_config_source" },
	{ 1010002, "bt_hci_unknown_type" },
	{ 1397638484, "snet_event_log" },
	{ 1937006964, "stats_log" },
};

static const char *find_tag_name_from_id(int id)
{
	int l = 0;
	int r = ARRAY_SIZE(event_tags) - 1;
	int mid = 0;

	while (l <= r) {
		mid = (l + r) / 2;

		if (event_tags[mid].nTagNum == id)
			return event_tags[mid].event_msg;
		else if (event_tags[mid].nTagNum < id)
			l = mid + 1;
		else
			r = mid - 1;
	}

	return NULL;
}

static off_t parse_buffer(char *buffer, unsigned char type, off_t pos)
{
	int buf_len;
	char *buf = NULL;
	off_t next = pos;

	buf = kzalloc(MAX_BUFFER_SIZE, GFP_KERNEL);
	switch (type) {
	case EVENT_TYPE_INT:
	{
		int val = get_unaligned((int *)&buffer[pos]);

		next += sizeof(int);
		buf_len = scnprintf(buf, MAX_BUFFER_SIZE, "%d", val);
		logger.func_hook_logger(buf, buf_len);
	}
	break;
	case EVENT_TYPE_LONG:
	{
		long long val = get_unaligned((long long *)&buffer[pos]);

		next += sizeof(long long);
		buf_len = scnprintf(buf, MAX_BUFFER_SIZE, "%lld", val);
		logger.func_hook_logger(buf, buf_len);
	}
	break;
	case EVENT_TYPE_FLOAT:
	{
		buffer += sizeof(float);
	}
	break;
	case EVENT_TYPE_STRING:
	{
		unsigned int len = get_unaligned((unsigned int *)&buffer[pos]);
		unsigned int len_to_copy =
			min(len, (unsigned int)(MAX_BUFFER_SIZE - 1));

		pos += sizeof(unsigned int);
		next += sizeof(unsigned int) + len;

		memcpy(buf, &buffer[pos], len_to_copy);
		logger.func_hook_logger(buf, len_to_copy);
	}
	break;
	}

	kfree(buf);
	return next;
}
#endif /* CONFIG_SEC_EVENT_LOG */

static inline void __ss_logger_level_header(char *buffer, size_t count)
{
	char *logbuf = logger.buffer;
	struct tm tmBuf;
	u64 tv_kernel;
	int logbuf_len;
	u64 rem_nsec;

#ifndef CONFIG_SEC_EVENT_LOG
	if (logger.id == SS_LOG_ID_EVENTS)
		return;
#endif

	tv_kernel = local_clock();
	rem_nsec = do_div(tv_kernel, 1000000000);
	time_to_tm(logger.tv_sec, 0, &tmBuf);

	logbuf_len = scnprintf(logbuf, SS_LOGGER_HEADER_SIZE,
			"\n[%5llu.%06llu][%d:%16s] %02d-%02d "
			"%02d:%02d:%02d.%03d %5d %5d  ",
			(unsigned long long)tv_kernel,
			(unsigned long long)rem_nsec / 1000,
			raw_smp_processor_id(), current->comm,
			tmBuf.tm_mon + 1, tmBuf.tm_mday,
			tmBuf.tm_hour, tmBuf.tm_min,
			tmBuf.tm_sec, logger.tv_nsec / 1000000,
			logger.pid, logger.tid);

	logger.func_hook_logger(logbuf, logbuf_len - 1);
}

static inline void __ss_logger_level_prefix(char *buffer, size_t count)
{
	char *logbuf = logger.buffer;
	const char *kPrioChars = "!.VDIWEFS";
	size_t prio = (size_t)logger.msg[0];

	if (logger.id == SS_LOG_ID_EVENTS)
		return;

	logbuf[0] = prio < strlen(kPrioChars) ?
		kPrioChars[prio] : '?';

#ifdef CONFIG_SEC_EVENT_LOG
	logger.msg[0] = 0xff;
#endif
	logger.func_hook_logger(logbuf, 1);
}

static inline void __ss_logger_level_text_event_log(char *buffer, size_t count)
{
#ifdef CONFIG_SEC_EVENT_LOG
	int buf_len;
	char buf[MAX_BUFFER_SIZE] = { '\0', };
	unsigned int tag_id = *(unsigned int *)buffer;
	const char *tag_name;

	tag_name = find_tag_name_from_id(tag_id);

	if (count == 4 && tag_name) {
		buf_len = scnprintf(buf, MAX_BUFFER_SIZE, "# %s", tag_name);
		logger.func_hook_logger(buf, buf_len);
	} else {
		if (*buffer == EVENT_TYPE_LONG ||
		    *buffer == EVENT_TYPE_INT ||
		    *buffer == EVENT_TYPE_FLOAT ||
		    *buffer == EVENT_TYPE_STRING)
			parse_buffer(buffer, *buffer, 1);
		else if (*buffer == EVENT_TYPE_LIST) {
			size_t items = (size_t)buffer[1];
			size_t i;
			off_t pos = 0;
			unsigned char type;

			pos += 2;

			logger.func_hook_logger("[", 1);
			for (i = 0; i < items; i++) {
				if (unlikely(pos > count)) {
					pr_warn("out-of-bound error %zu / %zu",
							pos, count);
					break;
				}

				type = buffer[pos++];

				pos = parse_buffer(buffer, type, pos);
				if (i < items -1)
					logger.func_hook_logger(",", 1);
			}
			logger.func_hook_logger("]", 1);
		}

		logger.msg[0] = 0xff; /* dummy value; */
	}
#endif /* CONFIG_SEC_EVENT_LOG */
}

static inline void __ss_logger_level_text(char *buffer, size_t count)
{
	//move to local buffer from pmsg buffer
	memcpy(logger.buffer, buffer, count);

	if (logger.id == SS_LOG_ID_EVENTS)
		__ss_logger_level_text_event_log(logger.buffer, count);
	else {
		char *eatnl = &logger.buffer[count - SS_LOGGER_STRING_PAD];
		logger.buffer[count - 1] = '\0';

		if (count == SS_LOGGER_SKIP_COUNT && *eatnl != '\0')
			return;

		if (count > 1 && !strncmp(logger.buffer, "!@", 2 /*strlen("!@")*/)) {
			/* To prevent potential buffer overrun
			 * put a null at the end of the buffer if required
			 */
			if (logger.buffer[count - 1] != '\0')
				logger.buffer[count - 1] = '\0';

			/* FIXME: print without a module and a function name */
			printk(KERN_INFO "%s\n", logger.buffer);
#ifdef CONFIG_SEC_BOOTSTAT
			if (count > 5 &&
			    !strncmp(logger.buffer, "!@Boot", 6 /*strlen("!@Boot")*/))
				sec_boot_stat_add(logger.buffer);
#endif
		}

		logger.func_hook_logger(logger.buffer, count - 1);
	}
}

static int ss_combine_pmsg(char *buffer, size_t count, unsigned int level)
{
	switch (level) {
	case SS_LOGGER_LEVEL_HEADER:
		__ss_logger_level_header(buffer, count);
		break;
	case SS_LOGGER_LEVEL_PREFIX:
		__ss_logger_level_prefix(buffer, count);
		break;
	case SS_LOGGER_LEVEL_TEXT:
		__ss_logger_level_text(buffer, count);
		break;
	default:
		pr_warn("Unknown logger level : %u\n", level);
		break;
	}
	logger.func_hook_logger(" ", 1);

	return 0;
}

int ss_hook_pmsg(char *buffer, size_t count)
{
	struct ss_android_log_header_t *header;
	struct ss_pmsg_log_header_t *pmsg_header;

	if (unlikely(!logger.buffer))
		return -ENOMEM;

	switch (count) {
	case sizeof(struct ss_pmsg_log_header_t):
		pmsg_header = (struct ss_pmsg_log_header_t *)buffer;
		if (pmsg_header->magic != 'l') {
			ss_combine_pmsg(buffer, count, SS_LOGGER_LEVEL_TEXT);
		} else {
			/* save logger data */
			logger.pid = pmsg_header->pid;
			logger.uid = pmsg_header->uid;
			logger.len = pmsg_header->len;
		}
		break;
	case sizeof(struct ss_android_log_header_t):
		/* save logger data */
		header = (struct ss_android_log_header_t *)buffer;
		logger.id = header->id;
		logger.tid = header->tid;
		logger.tv_sec = header->tv_sec;
		logger.tv_nsec  = header->tv_nsec;
		if (logger.id > 7) {
			/* write string */
			ss_combine_pmsg(buffer, count, SS_LOGGER_LEVEL_TEXT);
		} else {
			/* write header */
			ss_combine_pmsg(buffer, count, SS_LOGGER_LEVEL_HEADER);
		}
		break;
	case sizeof(unsigned char):
		logger.msg[0] = buffer[0];
		/* write char for prefix */
		ss_combine_pmsg(buffer, count, SS_LOGGER_LEVEL_PREFIX);
		break;
	default:
		/* write string */
		ss_combine_pmsg(buffer, count, SS_LOGGER_LEVEL_TEXT);
		break;
	}

	return 0;
}

static unsigned long long mem_address;
module_param(mem_address, ullong, 0400);
MODULE_PARM_DESC(mem_address,
		"base of reserved RAM used to store platform log");

static unsigned long long mem_size;
module_param(mem_size, ullong, 0400);
MODULE_PARM_DESC(mem_size,
		"size of reserved RAM used to store platform log");

static char *platform_log_buf;
static size_t platform_log_idx;

static inline void ss_hook_logger(const char *buf, size_t size)
{
	size_t f_len, s_len, remain_space;
	size_t idx;

	if (unlikely(!platform_log_buf))
		return;

	idx = platform_log_idx % mem_size;
	remain_space = mem_size - idx;
	f_len = min(size, remain_space);
	memcpy_toio(&(platform_log_buf[idx]), buf, f_len);

	s_len = size - f_len;
	if (unlikely(s_len))
		memcpy_toio(platform_log_buf, &buf[f_len], s_len);

	platform_log_idx += size;
}

struct ss_plog_platform_data {
	size_t mem_size;
	phys_addr_t mem_address;
};

static int ss_plog_parse_dt(struct platform_device *pdev,
		struct ss_plog_platform_data *pdata)
{
	struct device_node *mem_np;
	struct resource res;
	int err;

	dev_dbg(&pdev->dev, "using Device Tree\n");

	mem_np = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	if (!mem_np) {
		dev_err(&pdev->dev,
			"failed to locate DT /reserved-memory resource\n");
		return -EINVAL;
	}

	err = of_address_to_resource(mem_np, 0, &res);
	if (err)
		return err;

	pdata->mem_size = (size_t)resource_size(&res);
	pdata->mem_address = (phys_addr_t)res.start;

	return 0;
}

static int ss_plog_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ss_plog_platform_data *pdata = dev->platform_data;
	void __iomem *va;
	int err = -EINVAL;

	if (dev->of_node && !pdata) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
		if (unlikely(!pdata)) {
			err = -ENOMEM;
			return err;
		}

		err = ss_plog_parse_dt(pdev, pdata);
		if (unlikely(err < 0))
			goto fail_out_parse_dt;
	}

	if (unlikely(!pdata->mem_size)) {
		pr_err("The memory size must be non-zero\n");
		err = -ENOMEM;
		goto fail_out_check_mem_size;
	}

	mem_size = pdata->mem_size;
	mem_address = pdata->mem_address;

	pr_info("attached 0x%llxx@0x%llx\n", mem_size, mem_address);

	platform_set_drvdata(pdev, pdata);

	if (!request_mem_region((resource_size_t)mem_address,
				(resource_size_t)mem_size, "ss_plog")) {
		pr_err("request mem region (0x%llx@0x%llx) failed\n",
			mem_size, mem_address);
		goto fail_request_mem_region;
	}

	if (sec_debug_is_enabled())
		va = ioremap_wc((phys_addr_t)mem_address, (size_t)mem_size);
	else
		va = ioremap_cache((phys_addr_t)mem_address, (size_t)mem_size);
	if (unlikely(!va)) {
		pr_err("Failed to remap plaform log region\n");
		err = -ENOMEM;
		goto fail_remap_mem_region;
	}

	platform_log_buf = va;
	platform_log_idx = 0;

	logger.func_hook_logger = ss_hook_logger;
	logger.buffer = vmalloc(PAGE_SIZE * 3);
	if (unlikely(!logger.buffer)) {
		pr_err("Failed to alloc memory for buffer\n");
		err = -ENOMEM;
		goto fail_alloc_buffer;
	}
	pr_info("logger buffer alloc address: 0x%pK\n", logger.buffer);

	return 0;

fail_alloc_buffer:
	iounmap(va);

fail_remap_mem_region:
	release_mem_region((resource_size_t)mem_address,
			(resource_size_t)mem_size);

fail_out_parse_dt:
fail_out_check_mem_size:
fail_request_mem_region:
	devm_kfree(&pdev->dev, pdata);

	return err;
}

static const struct of_device_id dt_match[] = {
	{ .compatible = "samsung,ss_plog" },
	{}
};

static struct platform_driver ss_plog_driver = {
	.probe		= ss_plog_probe,
	.driver		= {
		.name		= "ss_plog",
		.of_match_table	= dt_match,
	},
};

static int __init ss_plog_init(void)
{
	return platform_driver_register(&ss_plog_driver);
}
device_initcall(ss_plog_init);

static void __exit ss_plog_exit(void)
{
	platform_driver_unregister(&ss_plog_driver);
}
module_exit(ss_plog_exit);
