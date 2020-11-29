/*
 * Copyright (c) 2017, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "dp_debug.h"
#include "dp_display.h"
#include "sde_edid_parser.h"
#include "secdp.h"

static u8 g_test_edid[] = {
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x09, 0xD1, 0x54, 0x7F, 0x45, 0x54, 0x00, 0x00,
	0x14, 0x1B, 0x01, 0x03, 0x80, 0x46, 0x28, 0x78, 0x2E, 0xDF, 0x50, 0xA3, 0x54, 0x35, 0xB5, 0x26,
	0x0F, 0x50, 0x54, 0xA5, 0x6B, 0x80, 0xD1, 0xC0, 0x81, 0xC0, 0x81, 0x00, 0x81, 0x80, 0xA9, 0xC0,
	0xB3, 0x00, 0x01, 0x01, 0x01, 0x01, 0x51, 0xD0, 0x00, 0xA0, 0xF0, 0x70, 0x3E, 0x80, 0x30, 0x20,
	0x35, 0x00, 0xBA, 0x89, 0x21, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x53, 0x35, 0x48,
	0x30, 0x31, 0x38, 0x39, 0x31, 0x53, 0x4C, 0x30, 0x0A, 0x20, 0x00, 0x00, 0x00, 0xFD, 0x00, 0x18,
	0x4C, 0x1E, 0x8C, 0x3C, 0x00, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFC,
	0x00, 0x42, 0x65, 0x6E, 0x51, 0x20, 0x53, 0x57, 0x33, 0x32, 0x30, 0x0A, 0x20, 0x20, 0x01, 0x42,
	0x02, 0x03, 0x45, 0xF1, 0x56, 0x61, 0x60, 0x5D, 0x5E, 0x5F, 0x10, 0x05, 0x04, 0x03, 0x02, 0x07,
	0x06, 0x0F, 0x1F, 0x20, 0x21, 0x22, 0x14, 0x13, 0x12, 0x16, 0x01, 0x23, 0x09, 0x07, 0x07, 0xE6,
	0x06, 0x05, 0x01, 0x60, 0x5A, 0x44, 0x6D, 0x03, 0x0C, 0x00, 0x10, 0x00, 0x38, 0x44, 0x20, 0x00,
	0x60, 0x01, 0x02, 0x03, 0x67, 0xD8, 0x5D, 0xC4, 0x01, 0x78, 0x80, 0x01, 0xE4, 0x0F, 0x03, 0x00,
	0x00, 0xE3, 0x05, 0xC3, 0x00, 0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C, 0x45,
	0x00, 0xBA, 0x89, 0x21, 0x00, 0x00, 0x1E, 0x56, 0x5E, 0x00, 0xA0, 0xA0, 0xA0, 0x29, 0x50, 0x30,
	0x20, 0x35, 0x00, 0xBA, 0x89, 0x21, 0x00, 0x00, 0x1A, 0xF4, 0x51, 0x00, 0xA0, 0xF0, 0x70, 0x19,
	0x80, 0x30, 0x20, 0x35, 0x00, 0xBA, 0x89, 0x21, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0xAB,
};

static struct secdp_display_timing g_parsed_res[] = {
	/* active_h, active_v, refresh_rate, interlaced */
	{0, 640,  480, 60, false},		/* 640x480  60hz */
	{0, 640,  480, 75, false},		/* 640x480  75hz */
	{0, 720,  400, 70, false},		/* 720x400  70hz */
	{0, 720,  480, 60, true},		/* 720x480i 60hz */
	{0, 720,  480, 60, false},		/* 720x480  60hz */

	{0, 720,  576, 50, true},		/* 720x576i 50hz */
	{0, 720,  576, 50, false},		/* 720x576  50hz */
	{0, 800,  600, 60, false},		/* 800x600  60hz */
	{0, 800,  600, 75, false},		/* 800x600  75hz */
	{0, 832,  624, 75, false},		/* 832x624  75hz */

	{0, 1024,  768, 60, false},		/* 1024x768 60hz */
	{0, 1024,  768, 75, false},		/* 1024x768 75hz */
	{0, 1152,  864, 75, false},		/* 1152x864 75hz */
	{0, 1280,  720, 50, false},		/* 1280x720 50hz */
	{0, 1280,  720, 60, false},		/* 1280x720 60hz */

	{0, 1280,  800, 60, false},		/* 1280x800  60hz */
	{0, 1280, 1024, 60, false},		/* 1280x1024 60hz */
	{0, 1280, 1024, 75, false},		/* 1280x1024 75hz */
	{0, 1440,  480, 60, false},		/* 1440x480  60hz */
	{0, 1600,  900, 60, false},		/* 1600x900  60hz */

	{0, 1680, 1050, 60, false},		/* 1680x1050  60hz */
	{0, 1920, 1080, 50, true},		/* 1920x1080i 50hz */
	{0, 1920, 1080, 60, true},		/* 1920x1080i 60hz */
	{0, 1920, 1080, 24, false},		/* 1920x1080  24hz */
	{0, 1920, 1080, 25, false},		/* 1920x1080  25hz */

	{0, 1920, 1080, 30, false},		/* 1920x1080 30hz */
	{0, 1920, 1080, 50, false},		/* 1920x1080 50hz */
	{0, 1920, 1080, 60, false},		/* 1920x1080 60hz */
	{0, 2560, 1440, 60, false},		/* 2560x1440 60hz */
	{0, 3840, 2160, 24, false},		/* 3840x2160 24hz */

	{0, 3840, 2160, 25, false},		/* 3840x2160 25hz */
	{0, 3840, 2160, 30, false},		/* 3840x2160 30hz */
	{0, 3840, 2160, 50, false},		/* 3840x2160 50hz */
	{0, 3840, 2160, 60, false},		/* 3840x2160 60hz */
};

static void drm_mode_remove(struct drm_connector *connector,
			struct drm_display_mode *mode)
{
	list_del(&mode->head);
	drm_mode_destroy(connector->dev, mode);
}

bool secdp_unit_test_edid_parse(void)
{
	int rc, i, parsed_res_cnt = 0, table_size;
	bool ret = false;
	struct sde_edid_ctrl *edid_ctrl = NULL;
	struct drm_display_mode *mode, *t;
	struct drm_connector *connector;

	connector = secdp_get_connector();
	if (!connector) {
		DP_ERR("fail to get connector\n");
		goto exit;
	}

	table_size = ARRAY_SIZE(g_parsed_res);

	edid_ctrl = sde_edid_init();
	if (!edid_ctrl) {
		DP_ERR("edid_ctrl alloc failed\n");
		goto exit;
	}

	mutex_lock(&connector->dev->mode_config.mutex);

	edid_ctrl->edid = (struct edid *)g_test_edid;
	rc = _sde_edid_update_modes(connector, edid_ctrl);
	DP_INFO("_sde_edid_update_modes, rc: %d\n", rc);

	/* init g_parsed_res */
	for (i = 0; i < table_size; i++)
		g_parsed_res[i].supported = false;

	/* check resolutions */
	list_for_each_entry(mode, &connector->probed_modes, head) {
		DP_INFO("checking %s @ %d Hz..\n", mode->name, drm_mode_vrefresh(mode));
		for (i = 0; i < table_size; i++) {
			bool interlaced = !!(mode->flags & DRM_MODE_FLAG_INTERLACE);

			if (g_parsed_res[i].active_h == mode->hdisplay &&
				g_parsed_res[i].active_v == mode->vdisplay &&
				g_parsed_res[i].refresh_rate == drm_mode_vrefresh(mode) &&
				g_parsed_res[i].interlaced == interlaced) {

				/*all conditions are met, mark it as supported*/
				g_parsed_res[i].supported = true;
			}
		}
	}

	list_for_each_entry_safe(mode, t, &connector->probed_modes, head)
		drm_mode_remove(connector, mode);

	mutex_unlock(&connector->dev->mode_config.mutex);
	kfree(edid_ctrl);

	/* count how many resolutions are marked as supported */
	for (i = 0; i < table_size; i++) {
		if (g_parsed_res[i].supported)
			parsed_res_cnt++;
	}

	/* check if num of supported resolutions are found without errors */
	if (parsed_res_cnt != table_size) {
		DP_ERR("count is not matched! res_cnt: %d, table_size: %d\n",
			parsed_res_cnt, table_size);
		goto exit;
	}

	ret = true;
exit:
	DP_INFO("returns %s\n", ret ? "true" : "false");
	return ret;
}
