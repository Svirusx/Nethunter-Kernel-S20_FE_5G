// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2012-2019, The Linux Foundation. All rights reserved.
 */

#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <drm/drmP.h>
#include "dp_power.h"
#include "dp_catalog.h"
#include "dp_debug.h"

#ifdef CONFIG_SEC_DISPLAYPORT
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include "secdp.h"

#if defined(CONFIG_COMBO_REDRIVER_PTN36502)
#include <linux/combo_redriver/ptn36502.h>
#elif defined(CONFIG_COMBO_REDRIVER_PS5169)
#include <linux/combo_redriver/ps5169.h>
#endif

#endif

#define DP_CLIENT_NAME_SIZE	20

struct dp_power_private {
	struct dp_parser *parser;
	struct platform_device *pdev;
	struct clk *pixel_clk_rcg;
	struct clk *pixel_parent;
	struct clk *pixel1_clk_rcg;
	struct clk *pixel1_parent;

	struct dp_power dp_power;

	bool core_clks_on;
	bool link_clks_on;
	bool strm0_clks_on;
	bool strm1_clks_on;
#ifdef CONFIG_SEC_DISPLAYPORT
	bool aux_pullup_on;

	void (*redrv_onoff)(bool enable, int lane);
	void (*redrv_aux_ctrl)(int cross);
#endif
};

#ifdef CONFIG_SEC_DISPLAYPORT
struct dp_power_private *g_secdp_power;

#define DP_ENUM_STR(x)	#x

enum redriver_switch_t {
	REDRIVER_SWITCH_UNKNOWN = -1,
	REDRIVER_SWITCH_RESET   =  0,
	REDRIVER_SWITCH_CROSS,
	REDRIVER_SWITCH_THROU,
};
#endif

static int dp_power_regulator_init(struct dp_power_private *power)
{
	int rc = 0, i = 0, j = 0;
	struct platform_device *pdev;
	struct dp_parser *parser;

	parser = power->parser;
	pdev = power->pdev;

	for (i = DP_CORE_PM; !rc && (i < DP_MAX_PM); i++) {
		rc = msm_dss_config_vreg(&pdev->dev,
			parser->mp[i].vreg_config,
			parser->mp[i].num_vreg, 1);
		if (rc) {
			DP_ERR("failed to init vregs for %s\n",
				dp_parser_pm_name(i));
			for (j = i - 1; j >= DP_CORE_PM; j--) {
				msm_dss_config_vreg(&pdev->dev,
				parser->mp[j].vreg_config,
				parser->mp[j].num_vreg, 0);
			}

			goto error;
		}
	}
error:
	return rc;
}

static void dp_power_regulator_deinit(struct dp_power_private *power)
{
	int rc = 0, i = 0;
	struct platform_device *pdev;
	struct dp_parser *parser;

	parser = power->parser;
	pdev = power->pdev;

	for (i = DP_CORE_PM; (i < DP_MAX_PM); i++) {
		rc = msm_dss_config_vreg(&pdev->dev,
			parser->mp[i].vreg_config,
			parser->mp[i].num_vreg, 0);
		if (rc)
			DP_ERR("failed to deinit vregs for %s\n",
				dp_parser_pm_name(i));
	}
}

#ifdef CONFIG_SEC_DISPLAYPORT
extern struct regulator *aux_pullup_vreg;

/* factory use only
 * ref: qusb_phy_enable_power()
 */
static int secdp_aux_pullup_vreg_enable(bool on)
{
	int rc = 0;
	struct dp_power_private *power = g_secdp_power;
	struct regulator *aux_pu_vreg = aux_pullup_vreg;

	DP_DEBUG("+++, on(%d)\n", on);

	if (!aux_pu_vreg) {
		DP_ERR("vdda33 is null!\n");
		goto exit;
	}

#define QUSB2PHY_3P3_VOL_MIN		3072000 /* uV */
#define QUSB2PHY_3P3_VOL_MAX		3072000 /* uV */
#define QUSB2PHY_3P3_HPM_LOAD		30000	/* uA */

	if (on) {
		if (power->aux_pullup_on) {
			DP_INFO("already on\n");
			goto exit;
		}

		rc = regulator_set_load(aux_pu_vreg, QUSB2PHY_3P3_HPM_LOAD);
		if (rc < 0) {
			DP_ERR("Unable to set HPM of vdda33: %d\n", rc);
			goto exit;
		}

		rc = regulator_set_voltage(aux_pu_vreg, QUSB2PHY_3P3_VOL_MIN,
					QUSB2PHY_3P3_VOL_MAX);
		if (rc) {
			DP_ERR("Unable to set voltage for vdda33: %d\n", rc);
			goto put_vdda33_lpm;
		}

		rc = regulator_enable(aux_pu_vreg);
		if (rc) {
			DP_ERR("Unable to enable vdda33: %d\n", rc);
			goto unset_vdd33;
		}

		DP_INFO("on success\n");
		power->aux_pullup_on = true;
	} else {

		rc = regulator_disable(aux_pu_vreg);
		if (rc)
			DP_ERR("Unable to disable vdda33: %d\n", rc);

unset_vdd33:
		rc = regulator_set_voltage(aux_pu_vreg, 0,
				QUSB2PHY_3P3_VOL_MAX);
		if (rc)
			DP_ERR("Unable to set 0 voltage for vdda33: %d\n", rc);

put_vdda33_lpm:
		rc = regulator_set_load(aux_pu_vreg, 0);
		if (rc < 0)
			DP_ERR("Unable to set 0 HPM of vdda33: %d\n", rc);

		if (!rc)
			DP_INFO("off success\n");

		power->aux_pullup_on = false;
	}

exit:
	return rc;
}
#endif

static int dp_power_regulator_ctrl(struct dp_power_private *power, bool enable)
{
	int rc = 0, i = 0, j = 0;
	struct dp_parser *parser;

	parser = power->parser;

	for (i = DP_CORE_PM; i < DP_MAX_PM; i++) {
		rc = msm_dss_enable_vreg(
			parser->mp[i].vreg_config,
			parser->mp[i].num_vreg, enable);
		if (rc) {
			DP_ERR("failed to '%s' vregs for %s\n",
					enable ? "enable" : "disable",
					dp_parser_pm_name(i));
			if (enable) {
				for (j = i-1; j >= DP_CORE_PM; j--) {
					msm_dss_enable_vreg(
					parser->mp[j].vreg_config,
					parser->mp[j].num_vreg, 0);
				}
			}
			goto error;
		}
	}

#ifdef CONFIG_SEC_DISPLAYPORT
	secdp_aux_pullup_vreg_enable(enable);
#endif

error:
	return rc;
}

static int dp_power_pinctrl_set(struct dp_power_private *power, bool active)
{
	int rc = -EFAULT;
	struct pinctrl_state *pin_state;
	struct dp_parser *parser;

	parser = power->parser;

	if (IS_ERR_OR_NULL(parser->pinctrl.pin))
		return 0;

	if (parser->no_aux_switch && parser->lphw_hpd) {
		pin_state = active ? parser->pinctrl.state_hpd_ctrl
				: parser->pinctrl.state_hpd_tlmm;
		if (!IS_ERR_OR_NULL(pin_state)) {
			rc = pinctrl_select_state(parser->pinctrl.pin,
				pin_state);
			if (rc) {
				DP_ERR("cannot direct hpd line to %s\n",
					active ? "ctrl" : "tlmm");
				return rc;
			}
		}
	}

	if (parser->no_aux_switch)
		return 0;

	pin_state = active ? parser->pinctrl.state_active
				: parser->pinctrl.state_suspend;
	if (!IS_ERR_OR_NULL(pin_state)) {
		rc = pinctrl_select_state(parser->pinctrl.pin,
				pin_state);
		if (rc)
			DP_ERR("can not set %s pins\n",
			       active ? "dp_active"
			       : "dp_sleep");
	} else {
		DP_ERR("invalid '%s' pinstate\n",
		       active ? "dp_active"
		       : "dp_sleep");
	}

	return rc;
}

static int dp_power_clk_init(struct dp_power_private *power, bool enable)
{
	int rc = 0;
	struct device *dev;
	enum dp_pm_type module;

	dev = &power->pdev->dev;

	if (enable) {
		for (module = DP_CORE_PM; module < DP_MAX_PM; module++) {
			struct dss_module_power *pm =
				&power->parser->mp[module];

			if (!pm->num_clk)
				continue;

			rc = msm_dss_get_clk(dev, pm->clk_config, pm->num_clk);
			if (rc) {
				DP_ERR("failed to get %s clk. err=%d\n",
					dp_parser_pm_name(module), rc);
				goto exit;
			}
		}

		power->pixel_clk_rcg = devm_clk_get(dev, "pixel_clk_rcg");
		if (IS_ERR(power->pixel_clk_rcg)) {
			DP_DEBUG("Unable to get DP pixel clk RCG\n");
			power->pixel_clk_rcg = NULL;
		}

		power->pixel_parent = devm_clk_get(dev, "pixel_parent");
		if (IS_ERR(power->pixel_parent)) {
			DP_DEBUG("Unable to get DP pixel RCG parent\n");
			power->pixel_parent = NULL;
		}

		power->pixel1_clk_rcg = devm_clk_get(dev, "pixel1_clk_rcg");
		if (IS_ERR(power->pixel1_clk_rcg)) {
			DP_DEBUG("Unable to get DP pixel1 clk RCG\n");
			power->pixel1_clk_rcg = NULL;
		}

		power->pixel1_parent = devm_clk_get(dev, "pixel1_parent");
		if (IS_ERR(power->pixel1_parent)) {
			DP_DEBUG("Unable to get DP pixel1 RCG parent\n");
			power->pixel1_parent = NULL;
		}
	} else {
		if (power->pixel_parent)
			devm_clk_put(dev, power->pixel_parent);

		if (power->pixel_clk_rcg)
			devm_clk_put(dev, power->pixel_clk_rcg);

		if (power->pixel1_parent)
			devm_clk_put(dev, power->pixel1_parent);

		if (power->pixel1_clk_rcg)
			devm_clk_put(dev, power->pixel1_clk_rcg);

		for (module = DP_CORE_PM; module < DP_MAX_PM; module++) {
			struct dss_module_power *pm =
				&power->parser->mp[module];

			if (!pm->num_clk)
				continue;

			msm_dss_put_clk(pm->clk_config, pm->num_clk);
		}
	}
exit:
	return rc;
}

static int dp_power_clk_set_rate(struct dp_power_private *power,
		enum dp_pm_type module, bool enable)
{
	int rc = 0;
	struct dss_module_power *mp;

	if (!power) {
		DP_ERR("invalid power data\n");
		rc = -EINVAL;
		goto exit;
	}

	mp = &power->parser->mp[module];

	if (enable) {
		rc = msm_dss_clk_set_rate(mp->clk_config, mp->num_clk);
		if (rc) {
			DP_ERR("failed to set clks rate.\n");
			goto exit;
		}

		rc = msm_dss_enable_clk(mp->clk_config, mp->num_clk, 1);
		if (rc) {
			DP_ERR("failed to enable clks\n");
			goto exit;
		}
	} else {
		rc = msm_dss_enable_clk(mp->clk_config, mp->num_clk, 0);
		if (rc) {
			DP_ERR("failed to disable clks\n");
				goto exit;
		}
	}
exit:
	return rc;
}

static int dp_power_clk_enable(struct dp_power *dp_power,
		enum dp_pm_type pm_type, bool enable)
{
	int rc = 0;
	struct dss_module_power *mp;
	struct dp_power_private *power;

	if (!dp_power) {
		DP_ERR("invalid power data\n");
		rc = -EINVAL;
		goto error;
	}

	power = container_of(dp_power, struct dp_power_private, dp_power);

	mp = &power->parser->mp[pm_type];

	if (pm_type >= DP_MAX_PM) {
		DP_ERR("unsupported power module: %s\n",
				dp_parser_pm_name(pm_type));
		return -EINVAL;
	}

	if (enable) {
		if (pm_type == DP_CORE_PM && power->core_clks_on) {
			DP_DEBUG("core clks already enabled\n");
			return 0;
		}

		if ((pm_type == DP_STREAM0_PM) && (power->strm0_clks_on)) {
			DP_DEBUG("strm0 clks already enabled\n");
			return 0;
		}

		if ((pm_type == DP_STREAM1_PM) && (power->strm1_clks_on)) {
			DP_DEBUG("strm1 clks already enabled\n");
			return 0;
		}

		if ((pm_type == DP_CTRL_PM) && (!power->core_clks_on)) {
			DP_DEBUG("Need to enable core clks before link clks\n");

			rc = dp_power_clk_set_rate(power, pm_type, enable);
			if (rc) {
				DP_ERR("failed to enable clks: %s. err=%d\n",
					dp_parser_pm_name(DP_CORE_PM), rc);
				goto error;
			} else {
				power->core_clks_on = true;
			}
		}

		if (pm_type == DP_LINK_PM && power->link_clks_on) {
			DP_DEBUG("links clks already enabled\n");
			return 0;
		}
	}

#ifdef CONFIG_SEC_DISPLAYPORT
	if (!enable) {
		/* consider below abnormal sequence :
		 * CCIC_NOTIFY_ATTACH
		 * -> no CCIC_NOTIFY_ID_DP_LINK_CONF, no CCIC_NOTIFY_ID_DP_HPD
		 * -> CCIC_NOTIFY_DETACH
		 */
		if ((pm_type == DP_CORE_PM) && (!power->core_clks_on)) {
			DP_DEBUG("core clks already disabled\n");
			return 0;
		}

		if ((pm_type == DP_CTRL_PM) && (!power->link_clks_on)) {
			DP_DEBUG("links clks already disabled\n");
			return 0;
		}

		if ((pm_type == DP_STREAM0_PM) && (!power->strm0_clks_on)) {
			DP_DEBUG("strm0 clks already disabled\n");
			return 0;
		}

		if ((pm_type == DP_STREAM1_PM) && (!power->strm1_clks_on)) {
			DP_DEBUG("strm1 clks already disabled\n");
			return 0;
		}

		if (pm_type == DP_LINK_PM && !power->link_clks_on) {
			DP_DEBUG("links clks already disabled\n");
			return 0;
		}
	}
#endif

	rc = dp_power_clk_set_rate(power, pm_type, enable);
	if (rc) {
		DP_ERR("failed to '%s' clks for: %s. err=%d\n",
			enable ? "enable" : "disable",
			dp_parser_pm_name(pm_type), rc);
			goto error;
	}

	if (pm_type == DP_CORE_PM)
		power->core_clks_on = enable;
	else if (pm_type == DP_STREAM0_PM)
		power->strm0_clks_on = enable;
	else if (pm_type == DP_STREAM1_PM)
		power->strm1_clks_on = enable;
	else if (pm_type == DP_LINK_PM)
		power->link_clks_on = enable;

	/*
	 * This log is printed only when user connects or disconnects
	 * a DP cable. As this is a user-action and not a frequent
	 * usecase, it is not going to flood the kernel logs. Also,
	 * helpful in debugging the NOC issues.
	 */
	DP_INFO("core:%s link:%s strm0:%s strm1:%s\n",
		power->core_clks_on ? "on" : "off",
		power->link_clks_on ? "on" : "off",
		power->strm0_clks_on ? "on" : "off",
		power->strm1_clks_on ? "on" : "off");
error:
	return rc;
}

static int dp_power_request_gpios(struct dp_power_private *power)
{
	int rc = 0, i;
	struct device *dev;
	struct dss_module_power *mp;
	static const char * const gpio_names[] = {
		"aux_enable", "aux_sel", "usbplug_cc",
	};

	if (!power) {
		DP_ERR("invalid power data\n");
		return -EINVAL;
	}

	dev = &power->pdev->dev;
	mp = &power->parser->mp[DP_CORE_PM];

	for (i = 0; i < ARRAY_SIZE(gpio_names); i++) {
		unsigned int gpio = mp->gpio_config[i].gpio;

		if (gpio_is_valid(gpio)) {
			rc = devm_gpio_request(dev, gpio, gpio_names[i]);
			if (rc) {
				DP_ERR("request %s gpio failed, rc=%d\n",
					       gpio_names[i], rc);
				goto error;
			}
		}
	}
	return 0;
error:
	for (i = 0; i < ARRAY_SIZE(gpio_names); i++) {
		unsigned int gpio = mp->gpio_config[i].gpio;

		if (gpio_is_valid(gpio))
			gpio_free(gpio);
	}
	return rc;
}

static bool dp_power_find_gpio(const char *gpio1, const char *gpio2)
{
	return !!strnstr(gpio1, gpio2, strlen(gpio1));
}

#ifndef CONFIG_SEC_DISPLAYPORT
static void dp_power_set_gpio(struct dp_power_private *power, bool flip)
{
	int i;
	struct dss_module_power *mp = &power->parser->mp[DP_CORE_PM];
	struct dss_gpio *config = mp->gpio_config;

	for (i = 0; i < mp->num_gpio; i++) {
		if (dp_power_find_gpio(config->gpio_name, "aux-sel"))
			config->value = flip;

		if (gpio_is_valid(config->gpio)) {
			DP_DEBUG("gpio %s, value %d\n", config->gpio_name,
				config->value);

			if (dp_power_find_gpio(config->gpio_name, "aux-en") ||
			    dp_power_find_gpio(config->gpio_name, "aux-sel"))
				gpio_direction_output(config->gpio,
					config->value);
			else
				gpio_set_value(config->gpio, config->value);

		}
		config++;
	}
}
#else
int secdp_power_request_gpios(struct dp_power *dp_power)
{
	int rc;
	struct dp_power_private *power;

	if (!dp_power) {
		DP_ERR("invalid power data\n");
		rc = -EINVAL;
		goto exit;
	}

	power = container_of(dp_power, struct dp_power_private, dp_power);
	rc = dp_power_request_gpios(power);

exit:
	return rc;
}

#if defined(CONFIG_COMBO_REDRIVER_PTN36502) || defined(CONFIG_COMBO_REDRIVER_PS5169)
static inline char *secdp_redriver_switch_to_string(int event)
{
	switch (event) {
	case REDRIVER_SWITCH_UNKNOWN:
		return DP_ENUM_STR(REDRIVER_SWITCH_UNKNOWN);
	case REDRIVER_SWITCH_RESET:
		return DP_ENUM_STR(REDRIVER_SWITCH_RESET);
	case REDRIVER_SWITCH_CROSS:
		return DP_ENUM_STR(REDRIVER_SWITCH_CROSS);
	case REDRIVER_SWITCH_THROU:
		return DP_ENUM_STR(REDRIVER_SWITCH_THROU);
	default:
		return "unknown";
	}
}
#endif

#if defined(CONFIG_COMBO_REDRIVER_PTN36502)
static void secdp_ptn36502_aux_ctrl(int cross)
{
	DP_DEBUG("+++ cross: %s\n", secdp_redriver_switch_to_string(cross));

	switch (cross) {
	case REDRIVER_SWITCH_CROSS:
		ptn36502_config(AUX_CROSS_MODE, 0);
		break;
	case REDRIVER_SWITCH_THROU:
		ptn36502_config(AUX_THRU_MODE, 0);
		break;
	case REDRIVER_SWITCH_RESET:
		ptn36502_config(SAFE_STATE, 0);
		break;
	default:
		DP_INFO("unknown: %d\n", cross);
		break;
	}
}

static void secdp_ptn36502_onoff(bool enable, int lane)
{
	DP_DEBUG("+++ enable(%d), lane(%d)\n", enable, lane);

	if (enable) {
		int val = -1;

		if (lane == 2)
			ptn36502_config(DP2_LANE_USB3_MODE, 1);
		else if (lane == 4)
			ptn36502_config(DP4_LANE_MODE, 1);
		else {
			DP_ERR("error! unknown lane: %d\n", lane);
			goto exit;
		}

		val = ptn36502_i2c_read(Chip_ID);
		DP_INFO("Chip_ID:  0x%x\n", val);
		val = ptn36502_i2c_read(Chip_Rev);
		DP_INFO("Chip_Rev: 0x%x\n", val);
	} else {
		ptn36502_config(SAFE_STATE, 0);
	}

exit:
	return;
}
#elif defined(CONFIG_COMBO_REDRIVER_PS5169)
static void secdp_ps5169_aux_ctrl(int cross)
{
	/*
	 * ps5169 does not support AUX switching function.
	 * It needs to be done by AUX switch IC
	 */
	DP_DEBUG("+++ cross: %s, do nothing!\n",
		secdp_redriver_switch_to_string(cross));
}

static void secdp_ps5169_onoff(bool enable, int lane)
{
	DP_DEBUG("+++ enable(%d), lane(%d)\n", enable, lane);

	if (enable) {
		if (lane == 2)
			ps5169_config(DP2_LANE_USB_MODE, 1);
		else if (lane == 4)
			ps5169_config(DP_ONLY_MODE, 1);
		else {
			DP_ERR("error! unknown lane: %d\n", lane);
			goto exit;
		}

		DP_INFO("Chip_ID1:  0x%x, Chip_Rev1: 0x%x\n",
			ps5169_i2c_read(Chip_ID1), ps5169_i2c_read(Chip_Rev1));
		DP_INFO("Chip_ID2:  0x%x, Chip_Rev2: 0x%x\n",
			ps5169_i2c_read(Chip_ID2), ps5169_i2c_read(Chip_Rev2));
	} else {
		ps5169_config(CLEAR_STATE, 0);
	}

exit:
	return;
}
#endif

void secdp_redriver_onoff(bool enable, int lane)
{
	struct dp_power_private *power = g_secdp_power;

	if (power && power->redrv_onoff)
		power->redrv_onoff(enable, lane);
}

static void secdp_redriver_aux_ctrl(int cross)
{
	struct dp_power_private *power = g_secdp_power;

	if (power && power->redrv_aux_ctrl)
		power->redrv_aux_ctrl(cross);
}

static void secdp_redriver_register(struct dp_power_private *power)
{
	int use_redrv;

	if (!power || !power->parser) {
		DP_ERR("invalid power!\n");
		goto end;
	}

	use_redrv = power->parser->use_redrv;
	DP_DEBUG("++ use_redrv(%d)\n", use_redrv);

	if (!use_redrv) {
		DP_INFO("nothing registered!\n");
		goto end;
	}

#if defined(CONFIG_COMBO_REDRIVER_PTN36502)
	power->redrv_onoff = secdp_ptn36502_onoff;
	power->redrv_aux_ctrl = secdp_ptn36502_aux_ctrl;
	DP_INFO("ptn36502 API registered!\n");
#elif defined (CONFIG_COMBO_REDRIVER_PS5169)
	power->redrv_onoff = secdp_ps5169_onoff;
	power->redrv_aux_ctrl = secdp_ps5169_aux_ctrl;
	DP_INFO("ps5169 API registered!\n");
#endif

end:
	return;
}

/* turn on EDP_AUX switch
 * ===================================================
 * | usbplug-cc(dir) | orientation | flip  | aux-sel |
 * ===================================================
 * |        0        |     CC1     | false |    0    |
 * |        1        |     CC2     | true  |    1    |
 * ===================================================
 */
static void secdp_power_set_gpio(bool flip)
{
	int i;
	/*int dir = (flip == false) ? 0 : 1;*/
	struct dp_power_private *power = g_secdp_power;
	struct dss_module_power *mp = &power->parser->mp[DP_CORE_PM];
	struct dss_gpio *config;
	struct dp_parser *parser;
	bool sel_val = false;

	parser = power->parser;

	DP_DEBUG("flip(%d), aux_sel_inv(%d), use_redrv(%d)\n",
		flip, parser->aux_sel_inv, parser->use_redrv);

	if (parser->aux_sel_inv)
		sel_val = true;

	config = mp->gpio_config;
	for (i = 0; i < mp->num_gpio; i++) {
		if (gpio_is_valid(config->gpio)) {
			if (dp_power_find_gpio(config->gpio_name, "aux-sel")) {
				if (parser->use_redrv == SECDP_REDRV_PTN36502)
					gpio_direction_output(config->gpio, 0);
				else /* SECDP_REDRV_PS5169 or SECDP_REDRV_NONE */
					gpio_direction_output(config->gpio,
						(!flip ? sel_val : !sel_val));

				usleep_range(100, 120);
				DP_INFO("%s -> set %d\n", config->gpio_name,
					gpio_get_value(config->gpio));
				break;
			}
		}
		config++;
	}

	usleep_range(100, 120);
	config = mp->gpio_config;
	for (i = 0; i < mp->num_gpio; i++) {
		if (gpio_is_valid(config->gpio)) {
			if (dp_power_find_gpio(config->gpio_name, "aux-en")) {
				gpio_direction_output(config->gpio, 0);
				DP_INFO("%s -> %d\n", config->gpio_name,
					gpio_get_value(config->gpio));
				break;
			}
		}
		config++;
	}
}

/* turn off EDP_AUX switch */
static void secdp_power_unset_gpio(void)
{
	int i;
	struct dp_power_private *power = g_secdp_power;
	struct dss_module_power *mp = &power->parser->mp[DP_CORE_PM];
	struct dss_gpio *config;

	DP_DEBUG("+++\n");

	config = mp->gpio_config;
	for (i = 0; i < mp->num_gpio; i++) {
		if (gpio_is_valid(config->gpio)) {
			if (dp_power_find_gpio(config->gpio_name, "aux-en")) {
				gpio_direction_output(config->gpio, 1);
				DP_INFO("%s -> 1\n", config->gpio_name);
				break;
			}
		}
		config++;
	}

	config = mp->gpio_config;
	for (i = 0; i < mp->num_gpio; i++) {
		if (gpio_is_valid(config->gpio)) {
			if (dp_power_find_gpio(config->gpio_name, "aux-sel")) {
				gpio_direction_output(config->gpio, 0);
				DP_INFO("%s -> 0\n", config->gpio_name);
				break;
			}
		}
		config++;
	}
}

/*
 * @aux_sel : 1 or 0
 */
void secdp_config_gpios_factory(int aux_sel, bool on)
{
	struct dp_power_private *power = g_secdp_power;
	struct dp_parser *parser;

	DP_DEBUG("+++ (%d,%d)\n", aux_sel, on);

	parser = power->parser;

	if (on) {
		secdp_aux_pullup_vreg_enable(true);
		secdp_power_set_gpio(aux_sel);

		if (aux_sel == 1)
			secdp_redriver_aux_ctrl(REDRIVER_SWITCH_CROSS);
		else if (aux_sel == 0)
			secdp_redriver_aux_ctrl(REDRIVER_SWITCH_THROU);
		else
			DP_ERR("unknown <%d>\n", aux_sel);
	} else {
		secdp_redriver_aux_ctrl(REDRIVER_SWITCH_RESET);
		secdp_power_unset_gpio();
		secdp_aux_pullup_vreg_enable(false);
	}
}

enum plug_orientation secdp_get_plug_orientation(void)
{
	int i, dir;
	struct dp_power_private *power = g_secdp_power;
	struct dss_module_power *mp = &power->parser->mp[DP_CORE_PM];
	struct dss_gpio *config = mp->gpio_config;
	struct dp_parser *parser;

	parser = power->parser;
	DP_INFO("+++ cc_dir_inv: %d\n", parser->cc_dir_inv);

	for (i = 0; i < mp->num_gpio; i++) {
		if (gpio_is_valid(config->gpio)) {
			if (dp_power_find_gpio(config->gpio_name,
					"usbplug-cc")) {
				dir = gpio_get_value(config->gpio);
				if (parser->cc_dir_inv)
					dir = !dir;
				DP_INFO("orientation: %s\n",
					!dir ? "CC1" : "CC2");
				if (dir == 0)
					return ORIENTATION_CC1;
				else /* if (dir == 1) */
					return ORIENTATION_CC2;
			}
		}
		config++;
	}

	/*cannot be here*/
	return ORIENTATION_NONE;
}

bool secdp_get_clk_status(enum dp_pm_type type)
{
	struct dp_power_private *power = g_secdp_power;
	bool ret = false;

	switch (type) {
	case DP_CORE_PM:
		ret = power->core_clks_on;
		break;
	case DP_STREAM0_PM:
		ret = power->strm0_clks_on;
		break;
	case DP_STREAM1_PM:
		ret = power->strm1_clks_on;
		break;
	case DP_LINK_PM:
		ret = power->link_clks_on;
		break;
	default:
		DP_ERR("invalid type:%d\n", type);
		break;
	}

	return ret;
}
#endif

static int dp_power_config_gpios(struct dp_power_private *power, bool flip,
					bool enable)
{
#ifndef CONFIG_SEC_DISPLAYPORT
	int rc = 0, i;
#endif
	struct dss_module_power *mp;
	struct dss_gpio *config;

	if (power->parser->no_aux_switch)
		return 0;

	mp = &power->parser->mp[DP_CORE_PM];
	config = mp->gpio_config;

	if (enable) {
#ifndef CONFIG_SEC_DISPLAYPORT
		rc = dp_power_request_gpios(power);
		if (rc) {
			DP_ERR("gpio request failed\n");
			return rc;
		}

		dp_power_set_gpio(power, flip);
#else
		secdp_power_set_gpio(flip);
#endif
	} else {
#ifndef CONFIG_SEC_DISPLAYPORT
		for (i = 0; i < mp->num_gpio; i++) {
			if (gpio_is_valid(config[i].gpio)) {
				gpio_set_value(config[i].gpio, 0);
				gpio_free(config[i].gpio);
			}
		}
#else
		secdp_power_unset_gpio();
#endif
	}

	return 0;
}

static int dp_power_client_init(struct dp_power *dp_power,
	struct sde_power_handle *phandle, struct drm_device *drm_dev)
{
	int rc = 0;
	struct dp_power_private *power;

	if (!drm_dev) {
		DP_ERR("invalid drm_dev\n");
		return -EINVAL;
	}

	power = container_of(dp_power, struct dp_power_private, dp_power);

	rc = dp_power_regulator_init(power);
	if (rc) {
		DP_ERR("failed to init regulators\n");
		goto error_power;
	}

	rc = dp_power_clk_init(power, true);
	if (rc) {
		DP_ERR("failed to init clocks\n");
		goto error_clk;
	}
	dp_power->phandle = phandle;
	dp_power->drm_dev = drm_dev;

#ifdef CONFIG_SEC_DISPLAYPORT
	rc = dp_power_pinctrl_set(power, false);
	if (rc) {
		DP_ERR("failed to set pinctrl state\n");
		goto error_client;
	}
#endif
	return 0;

#ifdef CONFIG_SEC_DISPLAYPORT
error_client:
	dp_power_clk_init(power, false);
#endif
error_clk:
	dp_power_regulator_deinit(power);
error_power:
	return rc;
}

static void dp_power_client_deinit(struct dp_power *dp_power)
{
	struct dp_power_private *power;

	if (!dp_power) {
		DP_ERR("invalid power data\n");
		return;
	}

	power = container_of(dp_power, struct dp_power_private, dp_power);

	dp_power_clk_init(power, false);
	dp_power_regulator_deinit(power);
}

static int dp_power_set_pixel_clk_parent(struct dp_power *dp_power, u32 strm_id)
{
	int rc = 0;
	struct dp_power_private *power;

	if (!dp_power || strm_id >= DP_STREAM_MAX) {
		DP_ERR("invalid power data. stream %d\n", strm_id);
		rc = -EINVAL;
		goto exit;
	}

	power = container_of(dp_power, struct dp_power_private, dp_power);

	if (strm_id == DP_STREAM_0) {
		if (power->pixel_clk_rcg && power->pixel_parent)
			clk_set_parent(power->pixel_clk_rcg,
					power->pixel_parent);
	} else if (strm_id == DP_STREAM_1) {
		if (power->pixel1_clk_rcg && power->pixel1_parent)
			clk_set_parent(power->pixel1_clk_rcg,
					power->pixel1_parent);
	}
exit:
	return rc;
}

static u64 dp_power_clk_get_rate(struct dp_power *dp_power, char *clk_name)
{
	size_t i;
	enum dp_pm_type j;
	struct dss_module_power *mp;
	struct dp_power_private *power;
	bool clk_found = false;
	u64 rate = 0;

	if (!clk_name) {
		DP_ERR("invalid pointer for clk_name\n");
		return 0;
	}

	power = container_of(dp_power, struct dp_power_private, dp_power);
	mp = &dp_power->phandle->mp;
	for (i = 0; i < mp->num_clk; i++) {
		if (!strcmp(mp->clk_config[i].clk_name, clk_name)) {
			rate = clk_get_rate(mp->clk_config[i].clk);
			clk_found = true;
			break;
		}
	}

	for (j = DP_CORE_PM; j < DP_MAX_PM && !clk_found; j++) {
		mp = &power->parser->mp[j];
		for (i = 0; i < mp->num_clk; i++) {
			if (!strcmp(mp->clk_config[i].clk_name, clk_name)) {
				rate = clk_get_rate(mp->clk_config[i].clk);
				clk_found = true;
				break;
			}
		}
	}

	return rate;
}

static int dp_power_init(struct dp_power *dp_power, bool flip)
{
	int rc = 0;
	struct dp_power_private *power;

	if (!dp_power) {
		DP_ERR("invalid power data\n");
		rc = -EINVAL;
		goto exit;
	}

	DP_DEBUG("+++\n");

	power = container_of(dp_power, struct dp_power_private, dp_power);

	rc = dp_power_regulator_ctrl(power, true);
	if (rc) {
		DP_ERR("failed to enable regulators\n");
		goto exit;
	}

	rc = dp_power_pinctrl_set(power, true);
	if (rc) {
		DP_ERR("failed to set pinctrl state\n");
		goto err_pinctrl;
	}

	rc = dp_power_config_gpios(power, flip, true);
	if (rc) {
		DP_ERR("failed to enable gpios\n");
		goto err_gpio;
	}

	rc = pm_runtime_get_sync(dp_power->drm_dev->dev);
	if (rc < 0) {
		DP_ERR("Power resource enable failed\n");
		goto err_sde_power;
	}

	rc = dp_power_clk_enable(dp_power, DP_CORE_PM, true);
	if (rc) {
		DP_ERR("failed to enable DP core clocks\n");
		goto err_clk;
	}

	return 0;

err_clk:
	pm_runtime_put_sync(dp_power->drm_dev->dev);
err_sde_power:
	dp_power_config_gpios(power, flip, false);
err_gpio:
	dp_power_pinctrl_set(power, false);
err_pinctrl:
	dp_power_regulator_ctrl(power, false);
exit:
	return rc;
}

static int dp_power_deinit(struct dp_power *dp_power)
{
	int rc = 0;
	struct dp_power_private *power;

	if (!dp_power) {
		DP_ERR("invalid power data\n");
		rc = -EINVAL;
		goto exit;
	}

	DP_DEBUG("+++\n");

	power = container_of(dp_power, struct dp_power_private, dp_power);

	if (power->link_clks_on)
		dp_power_clk_enable(dp_power, DP_LINK_PM, false);

	dp_power_clk_enable(dp_power, DP_CORE_PM, false);
	pm_runtime_put_sync(dp_power->drm_dev->dev);

	dp_power_config_gpios(power, false, false);
	dp_power_pinctrl_set(power, false);
	dp_power_regulator_ctrl(power, false);
exit:
	return rc;
}

struct dp_power *dp_power_get(struct dp_parser *parser)
{
	int rc = 0;
	struct dp_power_private *power;
	struct dp_power *dp_power;

	if (!parser) {
		DP_ERR("invalid input\n");
		rc = -EINVAL;
		goto error;
	}

	power = devm_kzalloc(&parser->pdev->dev, sizeof(*power), GFP_KERNEL);
	if (!power) {
		rc = -ENOMEM;
		goto error;
	}

	power->parser = parser;
	power->pdev = parser->pdev;

	dp_power = &power->dp_power;

	dp_power->init = dp_power_init;
	dp_power->deinit = dp_power_deinit;
	dp_power->clk_enable = dp_power_clk_enable;
	dp_power->set_pixel_clk_parent = dp_power_set_pixel_clk_parent;
	dp_power->clk_get_rate = dp_power_clk_get_rate;
	dp_power->power_client_init = dp_power_client_init;
	dp_power->power_client_deinit = dp_power_client_deinit;

#ifdef CONFIG_SEC_DISPLAYPORT
	secdp_redriver_register(power);
	g_secdp_power = power;
#endif

	return dp_power;
error:
	return ERR_PTR(rc);
}

void dp_power_put(struct dp_power *dp_power)
{
	struct dp_power_private *power = NULL;

	if (!dp_power)
		return;

	power = container_of(dp_power, struct dp_power_private, dp_power);

	devm_kfree(&power->pdev->dev, power);

#ifdef CONFIG_SEC_DISPLAYPORT
	g_secdp_power = NULL;
#endif
}
