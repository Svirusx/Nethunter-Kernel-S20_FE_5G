/*
 * Universal Flash Storage Feature Support
 *
 * Copyright (C) 2017-2018 Samsung Electronics Co., Ltd.
 *
 * Authors:
 *	Yongmyung Lee <ymhungry.lee@samsung.com>
 *	Jinyoung Choi <j-young.choi@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * See the COPYING file in the top-level directory or visit
 * <http://www.gnu.org/licenses/gpl-2.0.html>
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * This program is provided "AS IS" and "WITH ALL FAULTS" and
 * without warranty of any kind. You are solely responsible for
 * determining the appropriateness of using and distributing
 * the program and assume all risks associated with your exercise
 * of rights with respect to the program, including but not limited
 * to infringement of third party rights, the risks and costs of
 * program errors, damage to or loss of data, programs or equipment,
 * and unavailability or interruption of operations. Under no
 * circumstances will the contributor of this Program be liable for
 * any damages of any kind arising from your use or distribution of
 * this program.
 *
 * The Linux Foundation chooses to take subject only to the GPLv2
 * license terms, and distributes only under these terms.
 */

#include "ufsfeature.h"
#include "ufshcd.h"

#if defined(CONFIG_UFSHPB)
#include "ufshpb.h"
#endif

/*
 * ufs feature common functions.
 */
static int ufsf_read_desc(struct ufs_hba *hba, u8 desc_id, u8 desc_index,
			  u8 selector, u8 *desc_buf, u32 size)
{
	int err = 0;

	pm_runtime_get_sync(hba->dev);

	err = ufshcd_query_descriptor_retry(hba, UPIU_QUERY_OPCODE_READ_DESC,
					    desc_id, desc_index,
					    selector,
					    desc_buf, &size);
	if (err)
		ERR_MSG("reading Device Desc failed. err = %d", err);

	pm_runtime_put_sync(hba->dev);

	return err;
}

static int ufsf_read_dev_desc(struct ufsf_feature *ufsf, u8 selector)
{
	u8 desc_buf[UFSF_QUERY_DESC_DEVICE_MAX_SIZE];
	int ret;

	ret = ufsf_read_desc(ufsf->hba, QUERY_DESC_IDN_DEVICE, 0, selector,
			     desc_buf, UFSF_QUERY_DESC_DEVICE_MAX_SIZE);
	if (ret)
		return ret;

	ufsf->num_lu = desc_buf[DEVICE_DESC_PARAM_NUM_LU];
	INIT_INFO("device lu count %d", ufsf->num_lu);

	INIT_INFO("sel=%u length=%u(0x%x) bSupport=0x%.2x, extend=0x%.2x_%.2x",
		  selector, desc_buf[DEVICE_DESC_PARAM_LEN],
		  desc_buf[DEVICE_DESC_PARAM_LEN],
		  desc_buf[DEVICE_DESC_PARAM_UFS_FEAT],
		  desc_buf[DEVICE_DESC_PARAM_EX_FEAT_SUP+2],
		  desc_buf[DEVICE_DESC_PARAM_EX_FEAT_SUP+3]);

#if defined(CONFIG_UFSHPB)
	ufshpb_get_dev_info(&ufsf->hpb_dev_info, desc_buf);
#endif
	return 0;
}

static int ufsf_read_geo_desc(struct ufsf_feature *ufsf, u8 selector)
{
	u8 geo_buf[UFSF_QUERY_DESC_GEOMETRY_MAX_SIZE];
	int ret;

	ret = ufsf_read_desc(ufsf->hba, QUERY_DESC_IDN_GEOMETRY, 0, selector,
			     geo_buf, UFSF_QUERY_DESC_GEOMETRY_MAX_SIZE);
	if (ret)
		return ret;

#if defined(CONFIG_UFSHPB)
	if (ufsf->hpb_dev_info.hpb_device)
		ufshpb_get_geo_info(&ufsf->hpb_dev_info, geo_buf);
#endif
	return 0;
}

static int ufsf_read_unit_desc(struct ufsf_feature *ufsf, int lun, u8 selector)
{
	u8 unit_buf[UFSF_QUERY_DESC_UNIT_MAX_SIZE];
	int lu_enable, ret = 0;

	ret = ufsf_read_desc(ufsf->hba, QUERY_DESC_IDN_UNIT, lun, selector,
			     unit_buf, UFSF_QUERY_DESC_UNIT_MAX_SIZE);
	if (ret) {
		ERR_MSG("read unit desc failed. ret %d", ret);
		goto out;
	}

	lu_enable = unit_buf[UNIT_DESC_PARAM_LU_ENABLE];
	if (!lu_enable)
		return 0;

#if defined(CONFIG_UFSHPB)
	if (ufsf->hpb_dev_info.hpb_device) {
		ret = ufshpb_get_lu_info(ufsf, lun, unit_buf);
		if (ret == -ENOMEM)
			goto out;
	}
#endif
out:
	return ret;
}

int ufsf_hpb_dt_check(struct ufs_hba *hba)
{
        struct device_node *node = hba->dev->of_node;
        struct ufsf_feature *ufsf = &hba->ufsf;
        int dt_hpb_enable = 1;
        bool dev_hpb_enable = 0;
        int err;

        if ((ufsf->hpb_dev_info.hpb_ver < UFSHPB_HPBEN_VER) ||
                        (ufsf->hpb_dev_info.hpb_ver > UFSHPB_MAX_VER))
                return dt_hpb_enable;

        err = of_property_read_u32(node, "hpb-enable", &dt_hpb_enable);
        if (err) {
                ERR_MSG("of_property_read_u32 failed...err %d", err);
                dt_hpb_enable = 0;
        }

        pm_runtime_get_sync(hba->dev);
        err = ufshcd_query_flag_retry(hba, UPIU_QUERY_OPCODE_READ_FLAG,
                        QUERY_FLAG_IDN_HPB_EN, &dev_hpb_enable);
        if (err) {
                ERR_MSG("read flag [0x%.2X] failed...err %d",
                                QUERY_FLAG_IDN_HPB_EN, err);
		goto query_fail;
	}

        INIT_INFO("HPB enable DT %d - Dev %d", dt_hpb_enable, dev_hpb_enable);

        if (!dt_hpb_enable && dev_hpb_enable) {
                err = ufshcd_query_flag_retry(hba, UPIU_QUERY_OPCODE_CLEAR_FLAG,
                                QUERY_FLAG_IDN_HPB_EN, NULL);
                if (err) {
                        ERR_MSG("clear flag [0x%.2X] failed...err%d",
                                        QUERY_FLAG_IDN_HPB_EN, err);
                        goto query_fail;
                }
        } else if (dt_hpb_enable && !dev_hpb_enable) {
                err = ufshcd_query_flag_retry(hba, UPIU_QUERY_OPCODE_SET_FLAG,
                                QUERY_FLAG_IDN_HPB_EN, NULL);
                if (err) {
                        ERR_MSG("set flag [0x%.2X] failed...err %d",
                                        QUERY_FLAG_IDN_HPB_EN, err);
                        goto query_fail;
                }
        }
        pm_runtime_put_sync(hba->dev);

        return dt_hpb_enable;
query_fail:
        pm_runtime_put_sync(hba->dev);
        return 0;
}

void ufsf_device_check(struct ufs_hba *hba)
{
	struct ufsf_feature *ufsf = &hba->ufsf;
	int ret, lun;

	ufsf->slave_conf_cnt = 0;

	ufsf->hba = hba;

	ret = ufsf_read_dev_desc(ufsf, UFSFEATURE_SELECTOR);
	if (ret)
		return;

        ret = ufsf_hpb_dt_check(hba);
        if (!ret)
                goto dt_hpb_disable;

	ret = ufsf_read_geo_desc(ufsf, UFSFEATURE_SELECTOR);
	if (ret)
		return;

	seq_scan_lu(lun) {
		ret = ufsf_read_unit_desc(ufsf, lun, UFSFEATURE_SELECTOR);
		if (ret == -ENOMEM)
			goto out_free_mem;
	}

	return;
out_free_mem:
#if defined(CONFIG_UFSHPB)
	seq_scan_lu(lun) {
		kfree(ufsf->ufshpb_lup[lun]);
		ufsf->ufshpb_lup[lun] = NULL;
	}

	/* don't call init handler */
	ufsf->ufshpb_state = HPB_FAILED;
#endif
	return;
dt_hpb_disable:
#if defined(CONFIG_UFSHPB)
        /* don't call init handler */
        ufsf->ufshpb_state = HPB_FAILED;
#endif
}

static void ufsf_print_query_buf(unsigned char *field, int size)
{
	unsigned char buf[255];
	int count = 0;
	int i;

	count += snprintf(buf, 8, "(0x00):");

	for (i = 0; i < size; i++) {
		count += snprintf(buf + count, 4, " %.2X", field[i]);

		if ((i + 1) % 16 == 0) {
			buf[count] = '\n';
			buf[count + 1] = '\0';
			printk("%s", buf);
			count = 0;
			count += snprintf(buf, 8, "(0x%.2X):", i + 1);
		} else if ((i + 1) % 4 == 0)
			count += snprintf(buf + count, 3, " :");
	}
	buf[count] = '\n';
	buf[count + 1] = '\0';
	printk("%s", buf);
}

inline int ufsf_check_query(__u32 opcode)
{
	return (opcode & 0xffff0000) >> 16 == UFSFEATURE_QUERY_OPCODE;
}

int ufsf_query_ioctl(struct ufsf_feature *ufsf, int lun, void __user *buffer,
		     struct ufs_ioctl_query_data *ioctl_data, u8 selector)
{
	unsigned char *kernel_buf;
	int opcode;
	int err = 0;
	int index = 0;
	int length = 0;
	unsigned int buf_len = 0;

	opcode = ioctl_data->opcode & 0xffff;

	INFO_MSG("op %u idn %u sel %u size %u(0x%X)", opcode, ioctl_data->idn,
		 selector, ioctl_data->buf_size, ioctl_data->buf_size);

	buf_len = (ioctl_data->idn == QUERY_DESC_IDN_STRING) ?
		IOCTL_DEV_CTX_MAX_SIZE : QUERY_DESC_MAX_SIZE;

	if (buf_len < ioctl_data->buf_size) {
		ERR_MSG("Invalid ioctl_data->buf_size : %u, buf_len : %u", ioctl_data->buf_size, buf_len);
		err = -EINVAL;
		goto out;
	}

	kernel_buf = kzalloc(buf_len, GFP_KERNEL);
	if (!kernel_buf) {
		err = -ENOMEM;
		goto out;
	}

	switch (opcode) {
	case UPIU_QUERY_OPCODE_WRITE_DESC:
		err = copy_from_user(kernel_buf, buffer +
				     sizeof(struct ufs_ioctl_query_data),
				     ioctl_data->buf_size);
		INFO_MSG("buf size %d", ioctl_data->buf_size);
		ufsf_print_query_buf(kernel_buf, ioctl_data->buf_size);
		if (err)
			goto out_release_mem;
		break;

	case UPIU_QUERY_OPCODE_READ_DESC:
		switch (ioctl_data->idn) {
		case QUERY_DESC_IDN_UNIT:
			if (!ufs_is_valid_unit_desc_lun(lun)) {
				ERR_MSG("No unit descriptor for lun 0x%x", lun);
				err = -EINVAL;
				goto out_release_mem;
			}
			index = lun;
			INFO_MSG("read lu desc lun: %d", index);
			break;

		case QUERY_DESC_IDN_STRING:
#if defined(CONFIG_UFSHPB)
			if (!ufs_is_valid_unit_desc_lun(lun)) {
				ERR_MSG("No unit descriptor for lun 0x%x", lun);
				err = -EINVAL;
				goto out_release_mem;
			}
			err = ufshpb_issue_req_dev_ctx(ufsf->ufshpb_lup[lun],
						       kernel_buf,
						       ioctl_data->buf_size);
			if (err < 0)
				goto out_release_mem;

			goto copy_buffer;
#endif
		case QUERY_DESC_IDN_DEVICE:
		case QUERY_DESC_IDN_GEOMETRY:
		case QUERY_DESC_IDN_CONFIGURATION:
			break;

		default:
			ERR_MSG("invalid idn %d", ioctl_data->idn);
			err = -EINVAL;
			goto out_release_mem;
		}
		break;
	default:
		ERR_MSG("invalid opcode %d", opcode);
		err = -EINVAL;
		goto out_release_mem;
	}

	length = ioctl_data->buf_size;

	err = ufshcd_query_descriptor_retry(ufsf->hba, opcode, ioctl_data->idn,
					    index, selector, kernel_buf,
					    &length);
	if (err)
		goto out_release_mem;

#if defined(CONFIG_UFSHPB)
copy_buffer:
#endif
	if (opcode == UPIU_QUERY_OPCODE_READ_DESC) {
		err = copy_to_user(buffer, ioctl_data,
				   sizeof(struct ufs_ioctl_query_data));
		if (err)
			ERR_MSG("Failed copying back to user.");

		err = copy_to_user(buffer + sizeof(struct ufs_ioctl_query_data),
				   kernel_buf, ioctl_data->buf_size);
		if (err)
			ERR_MSG("Fail: copy rsp_buffer to user space.");
	}
out_release_mem:
	kfree(kernel_buf);
out:
	return err;
}

inline bool ufsf_is_valid_lun(int lun)
{
	return lun < UFS_UPIU_MAX_GENERAL_LUN;
}

/*
 * Wrapper functions for ufshpb.
 */
#if defined(CONFIG_UFSHPB)
inline int ufsf_hpb_prepare_pre_req(struct ufsf_feature *ufsf,
				    struct scsi_cmnd *cmd, int lun)
{
	if (ufsf->ufshpb_state == HPB_PRESENT)
		return ufshpb_prepare_pre_req(ufsf, cmd, lun);
	return -ENODEV;
}

inline int ufsf_hpb_prepare_add_lrbp(struct ufsf_feature *ufsf, int add_tag)
{
	if (ufsf->ufshpb_state == HPB_PRESENT)
		return ufshpb_prepare_add_lrbp(ufsf, add_tag);
	return -ENODEV;
}

inline void ufsf_hpb_end_pre_req(struct ufsf_feature *ufsf,
				 struct request *req)
{
	ufshpb_end_pre_req(ufsf, req);
}

inline void ufsf_hpb_change_lun(struct ufsf_feature *ufsf,
				struct ufshcd_lrb *lrbp)
{
	int ctx_lba = LI_EN_32(lrbp->cmd->cmnd + 2);

	if (ufsf->ufshpb_state == HPB_PRESENT &&
	    ufsf->issue_ioctl == true && ctx_lba == READ10_DEBUG_LBA) {
		lrbp->lun = READ10_DEBUG_LUN;
		INFO_MSG("lun 0x%X lba 0x%X", lrbp->lun, ctx_lba);
	}
}

inline void ufsf_hpb_prep_fn(struct ufsf_feature *ufsf,
			     struct ufshcd_lrb *lrbp)
{
	if (ufsf->ufshpb_state == HPB_PRESENT
	    && ufsf->issue_ioctl == false)
		ufshpb_prep_fn(ufsf, lrbp);
}

inline void ufsf_hpb_noti_rb(struct ufsf_feature *ufsf, struct ufshcd_lrb *lrbp)
{
	if (ufsf->ufshpb_state == HPB_PRESENT)
		ufshpb_rsp_upiu(ufsf, lrbp);
}

inline void ufsf_hpb_wakeup_worker_on_idle(struct ufsf_feature *ufsf)
{
	if (ufsf->ufshpb_state == HPB_PRESENT)
		ufshpb_wakeup_worker_on_idle(ufsf);
}

inline void ufsf_hpb_reset_lu(struct ufsf_feature *ufsf)
{
	ufsf->ufshpb_state = HPB_RESET;
	schedule_work(&ufsf->ufshpb_reset_work);
}

inline void ufsf_hpb_reset_host(struct ufsf_feature *ufsf)
{
	if (ufsf->ufshpb_state == HPB_PRESENT)
		ufsf->ufshpb_state = HPB_RESET;
}

inline void ufsf_hpb_init(struct ufsf_feature *ufsf)
{
	if (ufsf->hpb_dev_info.hpb_device &&
	    ufsf->ufshpb_state == HPB_NEED_INIT) {
                INIT_DELAYED_WORK(&ufsf->ufshpb_init_work, ufshpb_init_handler);
                schedule_delayed_work(&ufsf->ufshpb_init_work,
                                      msecs_to_jiffies(10000));
	}
}

inline void ufsf_hpb_reset(struct ufsf_feature *ufsf)
{
	if (ufsf->hpb_dev_info.hpb_device &&
	    ufsf->ufshpb_state == HPB_RESET) {
		schedule_work(&ufsf->ufshpb_reset_work);
	}
}

inline void ufsf_hpb_suspend(struct ufsf_feature *ufsf)
{
	/*
	 * if suspend failed, pm could call the suspend function again,
	 * in this case, ufshpb state already had been changed to SUSPEND state.
	 * so, we will not call ufshpb_suspend.
	 * */
	if (ufsf->ufshpb_state == HPB_PRESENT)
		ufshpb_suspend(ufsf);
}

inline void ufsf_hpb_resume(struct ufsf_feature *ufsf)
{
	if (ufsf->ufshpb_state == HPB_SUSPEND ||
	    ufsf->ufshpb_state == HPB_PRESENT) {
		if (ufsf->ufshpb_state == HPB_PRESENT)
			WARNING_MSG("warning.. hpb state PRESENT in resuming");
		ufshpb_resume(ufsf);
	}
}

inline void ufsf_hpb_release(struct ufsf_feature *ufsf)
{
	ufshpb_release(ufsf, HPB_NEED_INIT);
}

inline void ufsf_hpb_set_init_state(struct ufsf_feature *ufsf)
{
	ufsf->ufshpb_state = HPB_NEED_INIT;
}
#else
inline int ufsf_hpb_prepare_pre_req(struct ufsf_feature *ufsf,
				    struct scsi_cmnd *cmd, int lun)
{
	return 0;
}

inline int ufsf_hpb_prepare_add_lrbp(struct ufsf_feature *ufsf, int add_tag)
{
	return 0;
}

inline void ufsf_hpb_end_pre_req(struct ufsf_feature *ufsf,
				 struct request *req) {}
inline void ufsf_hpb_change_lun(struct ufsf_feature *ufsf,
				struct ufshcd_lrb *lrbp) {}
inline void ufsf_hpb_prep_fn(struct ufsf_feature *ufsf,
			     struct ufshcd_lrb *lrbp) {}
inline void ufsf_hpb_wakeup_worker_on_idle(struct ufsf_feature *ufsf) {}
inline void ufsf_hpb_noti_rb(struct ufsf_feature *ufsf,
			     struct ufshcd_lrb *lrbp) {}
inline void ufsf_hpb_reset_lu(struct ufsf_feature *ufsf) {}
inline void ufsf_hpb_reset_host(struct ufsf_feature *ufsf) {}
inline void ufsf_hpb_init(struct ufsf_feature *ufsf) {}
inline void ufsf_hpb_reset(struct ufsf_feature *ufsf) {}
inline void ufsf_hpb_suspend(struct ufsf_feature *ufsf) {}
inline void ufsf_hpb_resume(struct ufsf_feature *ufsf) {}
inline void ufsf_hpb_release(struct ufsf_feature *ufsf) {}
inline void ufsf_hpb_set_init_state(struct ufsf_feature *ufsf) {}
#endif
