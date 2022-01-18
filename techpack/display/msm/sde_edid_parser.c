// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
 */

#include <drm/drm_edid.h>
#include <linux/hdmi.h>

#include "sde_kms.h"
#include "sde_edid_parser.h"

#define DBC_START_OFFSET 4
#define EDID_DTD_LEN 18

enum data_block_types {
	RESERVED,
	AUDIO_DATA_BLOCK,
	VIDEO_DATA_BLOCK,
	VENDOR_SPECIFIC_DATA_BLOCK,
	SPEAKER_ALLOCATION_DATA_BLOCK,
	VESA_DTC_DATA_BLOCK,
	RESERVED2,
	USE_EXTENDED_TAG
};

static u8 *sde_find_edid_extension(struct edid *edid, int ext_id)
{
	u8 *edid_ext = NULL;
	int i;

	/* No EDID or EDID extensions */
	if (edid == NULL || edid->extensions == 0)
		return NULL;

	/* Find CEA extension */
	for (i = 0; i < edid->extensions; i++) {
		edid_ext = (u8 *)edid + EDID_LENGTH * (i + 1);
		if (edid_ext[0] == ext_id)
			break;
	}

	if (i == edid->extensions)
		return NULL;

	return edid_ext;
}

static u8 *sde_find_cea_extension(struct edid *edid)
{
	return sde_find_edid_extension(edid, SDE_CEA_EXT);
}

static int
sde_cea_db_payload_len(const u8 *db)
{
	return db[0] & 0x1f;
}

static int
sde_cea_db_tag(const u8 *db)
{
	return db[0] >> 5;
}

static int
sde_cea_revision(const u8 *cea)
{
	return cea[1];
}

static int
sde_cea_db_offsets(const u8 *cea, int *start, int *end)
{
	/* Data block offset in CEA extension block */
	*start = 4;
	*end = cea[2];
	if (*end == 0)
		*end = 127;
	if (*end < 4 || *end > 127)
		return -ERANGE;
	return 0;
}

#define sde_for_each_cea_db(cea, i, start, end) \
for ((i) = (start); \
(i) < (end) && (i) + sde_cea_db_payload_len(&(cea)[(i)]) < (end); \
(i) += sde_cea_db_payload_len(&(cea)[(i)]) + 1)

static bool sde_cea_db_is_hdmi_hf_vsdb(const u8 *db)
{
	int hdmi_id;

	if (sde_cea_db_tag(db) != VENDOR_SPECIFIC_DATA_BLOCK)
		return false;

	if (sde_cea_db_payload_len(db) < 7)
		return false;

	hdmi_id = db[1] | (db[2] << 8) | (db[3] << 16);

	return hdmi_id == HDMI_FORUM_IEEE_OUI;
}

static u8 *sde_edid_find_extended_tag_block(struct edid *edid, int blk_id)
{
	u8 *db = NULL;
	u8 *cea = NULL;

	if (!edid) {
		SDE_ERROR("%s: invalid input\n", __func__);
		return NULL;
	}

	cea = sde_find_cea_extension(edid);

	if (cea && sde_cea_revision(cea) >= 3) {
		int i, start, end;

		if (sde_cea_db_offsets(cea, &start, &end))
			return NULL;

		sde_for_each_cea_db(cea, i, start, end) {
			db = &cea[i];
			if ((sde_cea_db_tag(db) == SDE_EXTENDED_TAG) &&
				(db[1] == blk_id))
				return db;
		}
	}
	return NULL;
}

static u8 *
sde_edid_find_block(struct edid *edid, int blk_id)
{
	u8 *db = NULL;
	u8 *cea = NULL;

	if (!edid) {
		SDE_ERROR("%s: invalid input\n", __func__);
		return NULL;
	}

	cea = sde_find_cea_extension(edid);

	if (cea && sde_cea_revision(cea) >= 3) {
		int i, start, end;

		if (sde_cea_db_offsets(cea, &start, &end))
			return NULL;

		sde_for_each_cea_db(cea, i, start, end) {
			db = &cea[i];
			if (sde_cea_db_tag(db) == blk_id)
				return db;
		}
	}
	return NULL;
}


static const u8 *_sde_edid_find_block(const u8 *in_buf, u32 start_offset,
	u8 type, u8 *len)
{
	/* the start of data block collection, start of Video Data Block */
	u32 offset = start_offset;
	u32 dbc_offset = in_buf[2];

	SDE_EDID_DEBUG("%s +", __func__);
	/*
	 * * edid buffer 1, byte 2 being 4 means no non-DTD/Data block
	 *   collection present.
	 * * edid buffer 1, byte 2 being 0 means no non-DTD/DATA block
	 *   collection present and no DTD data present.
	 */

	if ((dbc_offset == 0) || (dbc_offset == 4)) {
		SDE_EDID_DEBUG("EDID: no DTD or non-DTD data present\n");
		return NULL;
	}

	while (offset < dbc_offset) {
		u8 block_len = in_buf[offset] & 0x1F;

		if ((offset + block_len <= dbc_offset) &&
		    (in_buf[offset] >> 5) == type) {
			*len = block_len;
			SDE_EDID_DEBUG("block=%d found @ 0x%x w/ len=%d\n",
				type, offset, block_len);

			return in_buf + offset;
		}
		offset += 1 + block_len;
	}

	return NULL;
}

static void sde_edid_extract_vendor_id(struct sde_edid_ctrl *edid_ctrl)
{
	char *vendor_id;
	u32 id_codes;

	SDE_EDID_DEBUG("%s +", __func__);
	if (!edid_ctrl) {
		SDE_ERROR("%s: invalid input\n", __func__);
		return;
	}

	vendor_id = edid_ctrl->vendor_id;
	id_codes = ((u32)edid_ctrl->edid->mfg_id[0] << 8) +
		edid_ctrl->edid->mfg_id[1];

	vendor_id[0] = 'A' - 1 + ((id_codes >> 10) & 0x1F);
	vendor_id[1] = 'A' - 1 + ((id_codes >> 5) & 0x1F);
	vendor_id[2] = 'A' - 1 + (id_codes & 0x1F);
	vendor_id[3] = 0;
	SDE_EDID_DEBUG("vendor id is %s ", vendor_id);
	SDE_EDID_DEBUG("%s -", __func__);
}

static void sde_edid_set_y420_support(struct drm_connector *connector,
u32 video_format)
{
	u8 cea_mode = 0;
	struct drm_display_mode *mode;
	u32 mode_fmt_flags = 0;

	/* Need to add Y420 support flag to the modes */
	list_for_each_entry(mode, &connector->probed_modes, head) {
		/* Cache the format flags before clearing */
		mode_fmt_flags = mode->flags;
		/* Clear the RGB/YUV format flags before calling upstream API */
		mode->flags &= ~SDE_DRM_MODE_FLAG_FMT_MASK;
		cea_mode = drm_match_cea_mode(mode);
		/* Restore the format flags */
		mode->flags = mode_fmt_flags;
		if ((cea_mode != 0) && (cea_mode == video_format)) {
			SDE_EDID_DEBUG("%s found match for %d ", __func__,
			video_format);
			mode->flags |= DRM_MODE_FLAG_SUPPORTS_YUV;
		}
	}
}

static void sde_edid_parse_Y420CMDB(
struct drm_connector *connector, struct sde_edid_ctrl *edid_ctrl,
const u8 *db)
{
	u32 offset = 0;
	u8 cmdb_len = 0;
	u8 svd_len = 0;
	const u8 *svd = NULL;
	u32 i = 0, j = 0;
	u32 video_format = 0;

	if (!edid_ctrl) {
		SDE_ERROR("%s: edid_ctrl is NULL\n", __func__);
		return;
	}

	if (!db) {
		SDE_ERROR("%s: invalid input\n", __func__);
		return;
	}
	SDE_EDID_DEBUG("%s +\n", __func__);
	cmdb_len = db[0] & 0x1f;

	/* Byte 3 to L+1 contain SVDs */
	offset += 2;

	svd = sde_edid_find_block(edid_ctrl->edid, VIDEO_DATA_BLOCK);

	if (svd) {
		/*moving to the next byte as vic info begins there*/
		svd_len = svd[0] & 0x1f;
		++svd;
	}

	for (i = 0; i < svd_len; i++, j++) {
		video_format = *(svd + i) & 0x7F;
		if (cmdb_len == 1) {
			/* If cmdb_len is 1, it means all SVDs support YUV */
			sde_edid_set_y420_support(connector, video_format);
		} else if (db[offset] & (1 << j)) {
			sde_edid_set_y420_support(connector, video_format);

			if (j & 0x80) {
				j = j/8;
				offset++;
				if (offset >= cmdb_len)
					break;
			}
		}
	}

	SDE_EDID_DEBUG("%s -\n", __func__);

}

static void sde_edid_parse_Y420VDB(
struct drm_connector *connector, struct sde_edid_ctrl *edid_ctrl,
const u8 *db)
{
	u8 len = db[0] & 0x1f;
	u32 i = 0;
	u32 video_format = 0;

	if (!edid_ctrl) {
		SDE_ERROR("%s: invalid input\n", __func__);
		return;
	}

	SDE_EDID_DEBUG("%s +\n", __func__);

	/* Offset to byte 3 */
	db += 2;
	for (i = 0; i < len - 1; i++) {
		video_format = *(db + i) & 0x7F;
		/*
		 * mode was already added in get_modes()
		 * only need to set the Y420 support flag
		 */
		sde_edid_set_y420_support(connector, video_format);
	}
	SDE_EDID_DEBUG("%s -", __func__);
}

static void sde_edid_set_mode_format(
struct drm_connector *connector, struct sde_edid_ctrl *edid_ctrl)
{
	const u8 *db = NULL;
	struct drm_display_mode *mode;

	SDE_EDID_DEBUG("%s +\n", __func__);
	/* Set YUV mode support flags for YCbcr420VDB */
	db = sde_edid_find_extended_tag_block(edid_ctrl->edid,
			Y420_VIDEO_DATA_BLOCK);
	if (db)
		sde_edid_parse_Y420VDB(connector, edid_ctrl, db);
	else
		SDE_EDID_DEBUG("YCbCr420 VDB is not present\n");

	/* Set RGB supported on all modes where YUV is not set */
	list_for_each_entry(mode, &connector->probed_modes, head) {
		if (!(mode->flags & DRM_MODE_FLAG_SUPPORTS_YUV))
			mode->flags |= DRM_MODE_FLAG_SUPPORTS_RGB;
	}


	db = sde_edid_find_extended_tag_block(edid_ctrl->edid,
			Y420_CAPABILITY_MAP_DATA_BLOCK);
	if (db)
		sde_edid_parse_Y420CMDB(connector, edid_ctrl, db);
	else
		SDE_EDID_DEBUG("YCbCr420 CMDB is not present\n");

	SDE_EDID_DEBUG("%s -\n", __func__);
}

static void _sde_edid_update_dc_modes(
struct drm_connector *connector, struct sde_edid_ctrl *edid_ctrl)
{
	int i, start, end;
	u8 *edid_ext, *hdmi;
	struct drm_display_info *disp_info;
	u32 hdmi_dc_yuv_modes = 0;

	SDE_EDID_DEBUG("%s +\n", __func__);

	if (!connector || !edid_ctrl) {
		SDE_ERROR("invalid input\n");
		return;
	}

	disp_info = &connector->display_info;

	edid_ext = sde_find_cea_extension(edid_ctrl->edid);

	if (!edid_ext) {
		SDE_DEBUG("no cea extension\n");
		return;
	}

	if (sde_cea_db_offsets(edid_ext, &start, &end))
		return;

	sde_for_each_cea_db(edid_ext, i, start, end) {
		if (sde_cea_db_is_hdmi_hf_vsdb(&edid_ext[i])) {

			hdmi = &edid_ext[i];

			if (sde_cea_db_payload_len(hdmi) < 7)
				continue;

			if (hdmi[7] & DRM_EDID_YCBCR420_DC_30) {
				hdmi_dc_yuv_modes |= DRM_EDID_YCBCR420_DC_30;
				SDE_EDID_DEBUG("Y420 30-bit supported\n");
			}

			if (hdmi[7] & DRM_EDID_YCBCR420_DC_36) {
				hdmi_dc_yuv_modes |= DRM_EDID_YCBCR420_DC_36;
				SDE_EDID_DEBUG("Y420 36-bit supported\n");
			}

			if (hdmi[7] & DRM_EDID_YCBCR420_DC_48) {
				hdmi_dc_yuv_modes |= DRM_EDID_YCBCR420_DC_36;
				SDE_EDID_DEBUG("Y420 48-bit supported\n");
			}
		}
	}

	disp_info->edid_hdmi_dc_modes |= hdmi_dc_yuv_modes;

	SDE_EDID_DEBUG("%s -\n", __func__);
}

#if defined(CONFIG_SEC_DISPLAYPORT)
struct sde_edid_ctrl *g_edid_ctrl;

int secdp_get_audio_ch(void)
{
	if (g_edid_ctrl)
		return g_edid_ctrl->audio_channel_info;

	return 0;
}
#endif

static void _sde_edid_extract_audio_data_blocks(
	struct sde_edid_ctrl *edid_ctrl)
{
	u8 len = 0;
	u8 adb_max = 0;
	const u8 *adb = NULL;
	u32 offset = DBC_START_OFFSET;
	u8 *cea = NULL;
#if defined(CONFIG_SEC_DISPLAYPORT)
	u16 audio_ch = 0;
	u32 bit_rate = 0;
	const u8 *adb_temp = NULL;
	u8 len_temp = 0;
	u8 *in_buf;
#endif

	if (!edid_ctrl) {
		SDE_ERROR("invalid edid_ctrl\n");
		return;
	}

#if defined(CONFIG_SEC_DISPLAYPORT)
	in_buf = (u8 *)edid_ctrl->edid;
	if (in_buf[3] & (1<<6)) {
		pr_info("%s: default audio format\n", __func__);
		edid_ctrl->audio_channel_info |= 2;
	}
#endif

	SDE_EDID_DEBUG("%s +", __func__);
	cea = sde_find_cea_extension(edid_ctrl->edid);
	if (!cea) {
		SDE_DEBUG("CEA extension not found\n");
		return;
	}

	edid_ctrl->adb_size = 0;

	memset(edid_ctrl->audio_data_block, 0,
		sizeof(edid_ctrl->audio_data_block));

	do {
		len = 0;
		adb = _sde_edid_find_block(cea, offset, AUDIO_DATA_BLOCK,
			&len);

		if ((adb == NULL) || (len > MAX_AUDIO_DATA_BLOCK_SIZE ||
			adb_max >= MAX_NUMBER_ADB)) {
			if (!edid_ctrl->adb_size) {
				SDE_DEBUG("No/Invalid Audio Data Block\n");
				return;
			}

			continue;
		}

#if defined(CONFIG_SEC_DISPLAYPORT)
		adb_temp = adb;
		len_temp = len;
		while (len > 0) {
			if (adb[1]>>3 == 1) {
				audio_ch |= (1 << (adb[1] & 0x7));
				if ((adb[1] & 0x7) > 0x04)
					audio_ch |= 0x20;
				if (adb[3] & 0x07) {
					bit_rate = adb[3] & 0x7;
					bit_rate |= (adb[2] & 0x7F) << 3;
				}
			}
			len -= 3;
			adb += 3;
		}
		adb = adb_temp;
		len = len_temp;
#endif
		memcpy(edid_ctrl->audio_data_block + edid_ctrl->adb_size,
			adb + 1, len);
		offset = (adb - cea) + 1 + len;

		edid_ctrl->adb_size += len;
		adb_max++;
	} while (adb);

#if defined(CONFIG_SEC_DISPLAYPORT)
	edid_ctrl->audio_channel_info |= (bit_rate << 16);
	edid_ctrl->audio_channel_info |= audio_ch;
	pr_info("%s: Displayport Audio info : 0x%x\n", __func__,
			edid_ctrl->audio_channel_info);
#endif
	SDE_EDID_DEBUG("%s -", __func__);
}

static void _sde_edid_extract_speaker_allocation_data(
	struct sde_edid_ctrl *edid_ctrl)
{
	u8 len;
	const u8 *sadb = NULL;
	u8 *cea = NULL;
#ifdef CONFIG_SEC_DISPLAYPORT
	u16 speaker_allocation = 0;
#endif

	if (!edid_ctrl) {
		SDE_ERROR("invalid edid_ctrl\n");
		return;
	}
	SDE_EDID_DEBUG("%s +", __func__);
	cea = sde_find_cea_extension(edid_ctrl->edid);
	if (!cea) {
		SDE_DEBUG("CEA extension not found\n");
		return;
	}

	sadb = _sde_edid_find_block(cea, DBC_START_OFFSET,
		SPEAKER_ALLOCATION_DATA_BLOCK, &len);
	if ((sadb == NULL) || (len != MAX_SPKR_ALLOC_DATA_BLOCK_SIZE)) {
		SDE_DEBUG("No/Invalid Speaker Allocation Data Block\n");
		return;
	}

	memcpy(edid_ctrl->spkr_alloc_data_block, sadb + 1, len);
	edid_ctrl->sadb_size = len;
#ifdef CONFIG_SEC_DISPLAYPORT
	speaker_allocation |= (sadb[1] & 0x7F);
#endif

	SDE_EDID_DEBUG("speaker alloc data SP byte = %08x %s%s%s%s%s%s%s\n",
		sadb[1],
		(sadb[1] & BIT(0)) ? "FL/FR," : "",
		(sadb[1] & BIT(1)) ? "LFE," : "",
		(sadb[1] & BIT(2)) ? "FC," : "",
		(sadb[1] & BIT(3)) ? "RL/RR," : "",
		(sadb[1] & BIT(4)) ? "RC," : "",
		(sadb[1] & BIT(5)) ? "FLC/FRC," : "",
		(sadb[1] & BIT(6)) ? "RLC/RRC," : "");
	SDE_EDID_DEBUG("%s -", __func__);

#ifdef CONFIG_SEC_DISPLAYPORT
	edid_ctrl->audio_channel_info |= (speaker_allocation << 8);
#endif
}

struct sde_edid_ctrl *sde_edid_init(void)
{
	struct sde_edid_ctrl *edid_ctrl = NULL;

	SDE_EDID_DEBUG("%s +\n", __func__);
	edid_ctrl = kzalloc(sizeof(*edid_ctrl), GFP_KERNEL);
	if (!edid_ctrl) {
		SDE_ERROR("edid_ctrl alloc failed\n");
		return NULL;
	}
	memset((edid_ctrl), 0, sizeof(*edid_ctrl));
	SDE_EDID_DEBUG("%s -\n", __func__);
	return edid_ctrl;
}

void sde_free_edid(void **input)
{
	struct sde_edid_ctrl *edid_ctrl = (struct sde_edid_ctrl *)(*input);

	SDE_EDID_DEBUG("%s +", __func__);
	kfree(edid_ctrl->edid);
	edid_ctrl->edid = NULL;
}

void sde_edid_deinit(void **input)
{
	struct sde_edid_ctrl *edid_ctrl = (struct sde_edid_ctrl *)(*input);

	SDE_EDID_DEBUG("%s +", __func__);
	sde_free_edid((void *)&edid_ctrl);
	kfree(edid_ctrl);
	SDE_EDID_DEBUG("%s -", __func__);
}

#ifdef CONFIG_SEC_DISPLAYPORT
/* edid_cea_modes array copied from */
/* kernel/drivers/gpu/drm/drm_edid.c */

static const struct drm_display_mode edid_cea_modes[] = {
	/* 0 - dummy, VICs start at 1 */
	{ },
	/* 1 - 640x480@60Hz */
	{ DRM_MODE("640x480", DRM_MODE_TYPE_DRIVER, 25175, 640, 656,
		   752, 800, 0, 480, 490, 492, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 2 - 720x480@60Hz */
	{ DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 27000, 720, 736,
		   798, 858, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 3 - 720x480@60Hz */
	{ DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 27000, 720, 736,
		   798, 858, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 4 - 1280x720@60Hz */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 1390,
		   1430, 1650, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 5 - 1920x1080i@60Hz */
	{ DRM_MODE("1920x1080i", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2008,
		   2052, 2200, 0, 1080, 1084, 1094, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC |
			DRM_MODE_FLAG_INTERLACE),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 6 - 720(1440)x480i@60Hz */
	{ DRM_MODE("720x480i", DRM_MODE_TYPE_DRIVER, 13500, 720, 739,
		   801, 858, 0, 480, 488, 494, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 7 - 720(1440)x480i@60Hz */
	{ DRM_MODE("720x480i", DRM_MODE_TYPE_DRIVER, 13500, 720, 739,
		   801, 858, 0, 480, 488, 494, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 8 - 720(1440)x240@60Hz */
	{ DRM_MODE("720x240", DRM_MODE_TYPE_DRIVER, 13500, 720, 739,
		   801, 858, 0, 240, 244, 247, 262, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_DBLCLK),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 9 - 720(1440)x240@60Hz */
	{ DRM_MODE("720x240", DRM_MODE_TYPE_DRIVER, 13500, 720, 739,
		   801, 858, 0, 240, 244, 247, 262, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_DBLCLK),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 10 - 2880x480i@60Hz */
	{ DRM_MODE("2880x480i", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2956,
		   3204, 3432, 0, 480, 488, 494, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 11 - 2880x480i@60Hz */
	{ DRM_MODE("2880x480i", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2956,
		   3204, 3432, 0, 480, 488, 494, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 12 - 2880x240@60Hz */
	{ DRM_MODE("2880x240", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2956,
		   3204, 3432, 0, 240, 244, 247, 262, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 13 - 2880x240@60Hz */
	{ DRM_MODE("2880x240", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2956,
		   3204, 3432, 0, 240, 244, 247, 262, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 14 - 1440x480@60Hz */
	{ DRM_MODE("1440x480", DRM_MODE_TYPE_DRIVER, 54000, 1440, 1472,
		   1596, 1716, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 15 - 1440x480@60Hz */
	{ DRM_MODE("1440x480", DRM_MODE_TYPE_DRIVER, 54000, 1440, 1472,
		   1596, 1716, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 16 - 1920x1080@60Hz */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2008,
		   2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 17 - 720x576@50Hz */
	{ DRM_MODE("720x576", DRM_MODE_TYPE_DRIVER, 27000, 720, 732,
		   796, 864, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 18 - 720x576@50Hz */
	{ DRM_MODE("720x576", DRM_MODE_TYPE_DRIVER, 27000, 720, 732,
		   796, 864, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 19 - 1280x720@50Hz */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 1720,
		   1760, 1980, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 20 - 1920x1080i@50Hz */
	{ DRM_MODE("1920x1080i", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2448,
		   2492, 2640, 0, 1080, 1084, 1094, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC |
			DRM_MODE_FLAG_INTERLACE),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 21 - 720(1440)x576i@50Hz */
	{ DRM_MODE("720x576i", DRM_MODE_TYPE_DRIVER, 13500, 720, 732,
		   795, 864, 0, 576, 580, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 22 - 720(1440)x576i@50Hz */
	{ DRM_MODE("720x576i", DRM_MODE_TYPE_DRIVER, 13500, 720, 732,
		   795, 864, 0, 576, 580, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 23 - 720(1440)x288@50Hz */
	{ DRM_MODE("720x288", DRM_MODE_TYPE_DRIVER, 13500, 720, 732,
		   795, 864, 0, 288, 290, 293, 312, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_DBLCLK),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 24 - 720(1440)x288@50Hz */
	{ DRM_MODE("720x288", DRM_MODE_TYPE_DRIVER, 13500, 720, 732,
		   795, 864, 0, 288, 290, 293, 312, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_DBLCLK),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 25 - 2880x576i@50Hz */
	{ DRM_MODE("2880x576i", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2928,
		   3180, 3456, 0, 576, 580, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 26 - 2880x576i@50Hz */
	{ DRM_MODE("2880x576i", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2928,
		   3180, 3456, 0, 576, 580, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 27 - 2880x288@50Hz */
	{ DRM_MODE("2880x288", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2928,
		   3180, 3456, 0, 288, 290, 293, 312, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 28 - 2880x288@50Hz */
	{ DRM_MODE("2880x288", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2928,
		   3180, 3456, 0, 288, 290, 293, 312, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 29 - 1440x576@50Hz */
	{ DRM_MODE("1440x576", DRM_MODE_TYPE_DRIVER, 54000, 1440, 1464,
		   1592, 1728, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 30 - 1440x576@50Hz */
	{ DRM_MODE("1440x576", DRM_MODE_TYPE_DRIVER, 54000, 1440, 1464,
		   1592, 1728, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 31 - 1920x1080@50Hz */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2448,
		   2492, 2640, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 32 - 1920x1080@24Hz */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2558,
		   2602, 2750, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 24, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 33 - 1920x1080@25Hz */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2448,
		   2492, 2640, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 25, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 34 - 1920x1080@30Hz */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2008,
		   2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 30, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 35 - 2880x480@60Hz */
	{ DRM_MODE("2880x480", DRM_MODE_TYPE_DRIVER, 108000, 2880, 2944,
		   3192, 3432, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 36 - 2880x480@60Hz */
	{ DRM_MODE("2880x480", DRM_MODE_TYPE_DRIVER, 108000, 2880, 2944,
		   3192, 3432, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 37 - 2880x576@50Hz */
	{ DRM_MODE("2880x576", DRM_MODE_TYPE_DRIVER, 108000, 2880, 2928,
		   3184, 3456, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 38 - 2880x576@50Hz */
	{ DRM_MODE("2880x576", DRM_MODE_TYPE_DRIVER, 108000, 2880, 2928,
		   3184, 3456, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 39 - 1920x1080i@50Hz */
	{ DRM_MODE("1920x1080i", DRM_MODE_TYPE_DRIVER, 72000, 1920, 1952,
		   2120, 2304, 0, 1080, 1126, 1136, 1250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 40 - 1920x1080i@100Hz */
	{ DRM_MODE("1920x1080i", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2448,
		   2492, 2640, 0, 1080, 1084, 1094, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC |
			DRM_MODE_FLAG_INTERLACE),
	  .vrefresh = 100, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 41 - 1280x720@100Hz */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 148500, 1280, 1720,
		   1760, 1980, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 100, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 42 - 720x576@100Hz */
	{ DRM_MODE("720x576", DRM_MODE_TYPE_DRIVER, 54000, 720, 732,
		   796, 864, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 100, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 43 - 720x576@100Hz */
	{ DRM_MODE("720x576", DRM_MODE_TYPE_DRIVER, 54000, 720, 732,
		   796, 864, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 100, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 44 - 720(1440)x576i@100Hz */
	{ DRM_MODE("720x576i", DRM_MODE_TYPE_DRIVER, 27000, 720, 732,
		   795, 864, 0, 576, 580, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	  .vrefresh = 100, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 45 - 720(1440)x576i@100Hz */
	{ DRM_MODE("720x576i", DRM_MODE_TYPE_DRIVER, 27000, 720, 732,
		   795, 864, 0, 576, 580, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	  .vrefresh = 100, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 46 - 1920x1080i@120Hz */
	{ DRM_MODE("1920x1080i", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2008,
		   2052, 2200, 0, 1080, 1084, 1094, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC |
			DRM_MODE_FLAG_INTERLACE),
	  .vrefresh = 120, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 47 - 1280x720@120Hz */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 148500, 1280, 1390,
		   1430, 1650, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 120, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 48 - 720x480@120Hz */
	{ DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 54000, 720, 736,
		   798, 858, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 120, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 49 - 720x480@120Hz */
	{ DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 54000, 720, 736,
		   798, 858, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 120, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 50 - 720(1440)x480i@120Hz */
	{ DRM_MODE("720x480i", DRM_MODE_TYPE_DRIVER, 27000, 720, 739,
		   801, 858, 0, 480, 488, 494, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	  .vrefresh = 120, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 51 - 720(1440)x480i@120Hz */
	{ DRM_MODE("720x480i", DRM_MODE_TYPE_DRIVER, 27000, 720, 739,
		   801, 858, 0, 480, 488, 494, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	  .vrefresh = 120, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 52 - 720x576@200Hz */
	{ DRM_MODE("720x576", DRM_MODE_TYPE_DRIVER, 108000, 720, 732,
		   796, 864, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 200, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 53 - 720x576@200Hz */
	{ DRM_MODE("720x576", DRM_MODE_TYPE_DRIVER, 108000, 720, 732,
		   796, 864, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 200, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 54 - 720(1440)x576i@200Hz */
	{ DRM_MODE("720x576i", DRM_MODE_TYPE_DRIVER, 54000, 720, 732,
		   795, 864, 0, 576, 580, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	  .vrefresh = 200, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 55 - 720(1440)x576i@200Hz */
	{ DRM_MODE("720x576i", DRM_MODE_TYPE_DRIVER, 54000, 720, 732,
		   795, 864, 0, 576, 580, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	  .vrefresh = 200, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 56 - 720x480@240Hz */
	{ DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 108000, 720, 736,
		   798, 858, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 240, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 57 - 720x480@240Hz */
	{ DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 108000, 720, 736,
		   798, 858, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 240, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 58 - 720(1440)x480i@240 */
	{ DRM_MODE("720x480i", DRM_MODE_TYPE_DRIVER, 54000, 720, 739,
		   801, 858, 0, 480, 488, 494, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	  .vrefresh = 240, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3, },
	/* 59 - 720(1440)x480i@240 */
	{ DRM_MODE("720x480i", DRM_MODE_TYPE_DRIVER, 54000, 720, 739,
		   801, 858, 0, 480, 488, 494, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
			DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	  .vrefresh = 240, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 60 - 1280x720@24Hz */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 59400, 1280, 3040,
		   3080, 3300, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 24, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 61 - 1280x720@25Hz */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 3700,
		   3740, 3960, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 25, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 62 - 1280x720@30Hz */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 3040,
		   3080, 3300, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 30, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 63 - 1920x1080@120Hz */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 297000, 1920, 2008,
		   2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	 .vrefresh = 120, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 64 - 1920x1080@100Hz */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 297000, 1920, 2448,
		   2492, 2640, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	 .vrefresh = 100, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 65 - 1280x720@24Hz */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 59400, 1280, 3040,
		   3080, 3300, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 24, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 66 - 1280x720@25Hz */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 3700,
		   3740, 3960, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 25, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 67 - 1280x720@30Hz */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 3040,
		   3080, 3300, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 30, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 68 - 1280x720@50Hz */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 1720,
		   1760, 1980, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 69 - 1280x720@60Hz */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 1390,
		   1430, 1650, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 70 - 1280x720@100Hz */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 148500, 1280, 1720,
		   1760, 1980, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 100, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 71 - 1280x720@120Hz */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 148500, 1280, 1390,
		   1430, 1650, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 120, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 72 - 1920x1080@24Hz */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2558,
		   2602, 2750, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 24, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 73 - 1920x1080@25Hz */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2448,
		   2492, 2640, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 25, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 74 - 1920x1080@30Hz */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2008,
		   2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 30, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 75 - 1920x1080@50Hz */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2448,
		   2492, 2640, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 76 - 1920x1080@60Hz */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2008,
		   2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 77 - 1920x1080@100Hz */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 297000, 1920, 2448,
		   2492, 2640, 0, 1080, 1084, 1094, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	 .vrefresh = 100, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 78 - 1920x1080@120Hz */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 297000, 1920, 2008,
		   2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	 .vrefresh = 120, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 79 - 1680x720@24Hz */
	{ DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 59400, 1680, 3040,
		3080, 3300, 0, 720, 725, 730, 750, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 24, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 80 - 1680x720@25Hz */
	{ DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 59400, 1680, 2908,
		2948, 3168, 0, 720, 725, 730, 750, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 25, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 81 - 1680x720@30Hz */
	{ DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 59400, 1680, 2380,
		2420, 2640, 0, 720, 725, 730, 750, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 30, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 82 - 1680x720@50Hz */
	{ DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 82500, 1680, 1940,
		1980, 2200, 0, 720, 725, 730, 750, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 83 - 1680x720@60Hz */
	{ DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 99000, 1680, 1940,
		1980, 2200, 0, 720, 725, 730, 750, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 84 - 1680x720@100Hz */
	{ DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 165000, 1680, 1740,
		1780, 2000, 0, 720, 725, 730, 825, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 100, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 85 - 1680x720@120Hz */
	{ DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 198000, 1680, 1740,
		1780, 2000, 0, 720, 725, 730, 825, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 120, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 86 - 2560x1080@24Hz */
	{ DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 99000, 2560, 3558,
		3602, 3750, 0, 1080, 1084, 1089, 1100, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 24, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 87 - 2560x1080@25Hz */
	{ DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 90000, 2560, 3008,
		3052, 3200, 0, 1080, 1084, 1089, 1125, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 25, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 88 - 2560x1080@30Hz */
	{ DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 118800, 2560, 3328,
		3372, 3520, 0, 1080, 1084, 1089, 1125, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 30, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 89 - 2560x1080@50Hz */
	{ DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 185625, 2560, 3108,
		3152, 3300, 0, 1080, 1084, 1089, 1125, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 90 - 2560x1080@60Hz */
	{ DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 198000, 2560, 2808,
		2852, 3000, 0, 1080, 1084, 1089, 1100, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 91 - 2560x1080@100Hz */
	{ DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 371250, 2560, 2778,
		2822, 2970, 0, 1080, 1084, 1089, 1250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 100, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 92 - 2560x1080@120Hz */
	{ DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 495000, 2560, 3108,
		3152, 3300, 0, 1080, 1084, 1089, 1250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 120, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27, },
	/* 93 - 3840x2160p@24Hz 16:9 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 297000, 3840, 5116,
		5204, 5500, 0, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 24, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,},
	/* 94 - 3840x2160p@25Hz 16:9 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 297000, 3840, 4896,
		4984, 5280, 0, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 25, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9},
	/* 95 - 3840x2160p@30Hz 16:9 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 297000, 3840, 4016,
		4104, 4400, 0, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 30, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9},
	/* 96 - 3840x2160p@50Hz 16:9 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 594000, 3840, 4896,
		4984, 5280, 0, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9},
	/* 97 - 3840x2160p@60Hz 16:9 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 594000, 3840, 4016,
		4104, 4400, 0, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9},
	/* 98 - 4096x2160p@24Hz 256:135 */
	{ DRM_MODE("4096x2160", DRM_MODE_TYPE_DRIVER, 297000, 4096, 5116,
		5204, 5500, 0, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 24, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_256_135},
	/* 99 - 4096x2160p@25Hz 256:135 */
	{ DRM_MODE("4096x2160", DRM_MODE_TYPE_DRIVER, 297000, 4096, 5064,
		5152, 5280, 0, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 25, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_256_135},
	/* 100 - 4096x2160p@30Hz 256:135 */
	{ DRM_MODE("4096x2160", DRM_MODE_TYPE_DRIVER, 297000, 4096, 4184,
		4272, 4400, 0, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 30, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_256_135},
	/* 101 - 4096x2160p@50Hz 256:135 */
	{ DRM_MODE("4096x2160", DRM_MODE_TYPE_DRIVER, 594000, 4096, 5064,
		5152, 5280, 0, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_256_135},
	/* 102 - 4096x2160p@60Hz 256:135 */
	{ DRM_MODE("4096x2160", DRM_MODE_TYPE_DRIVER, 594000, 4096, 4184,
		4272, 4400, 0, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_256_135},
	/* 103 - 3840x2160p@24Hz 64:27 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 297000, 3840, 5116,
		5204, 5500, 0, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 24, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27},
	/* 104 - 3840x2160p@25Hz 64:27 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 297000, 3840, 4016,
		4104, 4400, 0, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 25, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27},
	/* 105 - 3840x2160p@30Hz 64:27 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 297000, 3840, 4016,
		4104, 4400, 0, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 30, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27},
	/* 106 - 3840x2160p@50Hz 64:27 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 594000, 3840, 4896,
		4984, 5280, 0, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 50, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27},
	/* 107 - 3840x2160p@60Hz 64:27 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 594000, 3840, 4016,
		4104, 4400, 0, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27},
};

char *secdp_vic_to_string(int vic)
{
	static char temp[30];
	int res_cnt = ARRAY_SIZE(edid_cea_modes);

	if (res_cnt <= vic)
		return NULL;

	scnprintf(temp, 30, "%s %dhz\n",
			edid_cea_modes[vic].name,
			edid_cea_modes[vic].vrefresh);
	return temp;
}

int forced_resolution;
#endif

int _sde_edid_update_modes(struct drm_connector *connector,
	void *input)
{
	int rc = 0;
	struct sde_edid_ctrl *edid_ctrl = (struct sde_edid_ctrl *)(input);
#ifdef CONFIG_SEC_DISPLAYPORT
	struct drm_device *dev = connector->dev;
	struct drm_display_mode *newmode;
#endif

	SDE_EDID_DEBUG("%s +", __func__);
	if (edid_ctrl->edid) {
		drm_connector_update_edid_property(connector,
			edid_ctrl->edid);

#ifdef CONFIG_SEC_DISPLAYPORT
		if (forced_resolution > 0) {
			newmode = drm_mode_duplicate(dev, &edid_cea_modes[forced_resolution]);
			if (!newmode) {
				pr_err("%s: duplicate failed!\n", __func__);
				return rc;
			}

			drm_mode_probed_add(connector, newmode);
			rc = 1;
			SDE_EDID_DEBUG("%s --", __func__);
			return rc;
		}
#endif

		rc = drm_add_edid_modes(connector, edid_ctrl->edid);
		sde_edid_set_mode_format(connector, edid_ctrl);
		_sde_edid_update_dc_modes(connector, edid_ctrl);
		SDE_EDID_DEBUG("%s -", __func__);
		return rc;
	}

	drm_connector_update_edid_property(connector, NULL);
	SDE_EDID_DEBUG("%s null edid -", __func__);
	return rc;
}

u8 sde_get_edid_checksum(void *input)
{
	struct sde_edid_ctrl *edid_ctrl = (struct sde_edid_ctrl *)(input);
	struct edid *edid = NULL, *last_block = NULL;
	u8 *raw_edid = NULL;

	if (!edid_ctrl || !edid_ctrl->edid) {
		SDE_ERROR("invalid edid input\n");
		return 0;
	}

	edid = edid_ctrl->edid;

	raw_edid = (u8 *)edid;
#ifndef CONFIG_SEC_DISPLAYPORT
	raw_edid += (edid->extensions * EDID_LENGTH);
#else
	/* fix Prevent_CXX Major defect.
	 * Dangerous cast: Pointer "raw_edid"( 8 ) to int( 4 ).
	 */
	raw_edid = raw_edid + (edid->extensions * EDID_LENGTH);
#endif
	last_block = (struct edid *)raw_edid;

	if (last_block)
		return last_block->checksum;

	SDE_ERROR("Invalid block, no checksum\n");
	return 0;
}

bool sde_detect_hdmi_monitor(void *input)
{
	struct sde_edid_ctrl *edid_ctrl = (struct sde_edid_ctrl *)(input);

	return drm_detect_hdmi_monitor(edid_ctrl->edid);
}

void sde_parse_edid(void *input)
{
	struct sde_edid_ctrl *edid_ctrl;

	if (!input) {
		SDE_ERROR("Invalid input\n");
		return;
	}

	edid_ctrl = (struct sde_edid_ctrl *)(input);

	if (edid_ctrl->edid) {

#ifdef CONFIG_SEC_DISPLAYPORT
		edid_ctrl->audio_channel_info = 1 << 26;
#endif

		sde_edid_extract_vendor_id(edid_ctrl);
		_sde_edid_extract_audio_data_blocks(edid_ctrl);
		_sde_edid_extract_speaker_allocation_data(edid_ctrl);
	} else {
		SDE_ERROR("edid not present\n");
	}
}

void sde_get_edid(struct drm_connector *connector,
				  struct i2c_adapter *adapter, void **input)
{
	struct sde_edid_ctrl *edid_ctrl = (struct sde_edid_ctrl *)(*input);
#ifdef CONFIG_SEC_DISPLAYPORT
	g_edid_ctrl = edid_ctrl;
#endif

	edid_ctrl->edid = drm_get_edid(connector, adapter);
	SDE_EDID_DEBUG("%s +\n", __func__);

	if (!edid_ctrl->edid)
		SDE_ERROR("EDID read failed\n");

	if (edid_ctrl->edid)
		sde_parse_edid(edid_ctrl);

	SDE_EDID_DEBUG("%s -\n", __func__);
};
