#include <linux/module.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/of.h>
#include <linux/uaccess.h>
#include <trace/events/power.h>
#include <linux/cgroup.h>
#include "perf_mgr.h"
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include "../../../techpack/display/msm/samsung/ss_panel_notify.h"

#define MAX_INSERT_DIGIT 4

extern int ss_panel_notifier_register(struct notifier_block *nb);
extern int ss_panel_notifier_unregister(struct notifier_block *nb);
extern unsigned long get_task_util(struct task_struct *p);
extern unsigned long get_max_capacity(int cpu);

static unsigned long calc_fps_required_util(unsigned long max_cap, unsigned long dur);
static struct task_fps_util_info *get_target_task(int tid);

static spinlock_t write_slock;
unsigned long fps_required_util;
struct list_head head_list;
int fps_task_count;
unsigned long us_frame_time;
int g_fps;
int fps_margin_percent;
int hold_frame_count;

static struct kobject *perf_kobject;

static long perf_mgr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *uarg = (void __user *)arg;
	long ret = -EINVAL;
	int target_tid;
	struct fps_info fps_info_val, ofi;
	struct task_struct *task, *tmp_task;

	//unsigned long rn_sum = 0, lttl_max_capa = 0;
	unsigned long rn_sum = 0;
	unsigned long duration = 0;
	struct task_fps_util_info *fi, *target_fi = NULL;
	unsigned long prev_fps_util, new_fps_util;
	unsigned long fiSize = 0;

	if (!uarg) {
		pr_err("[GPIS] : invalid user uarg!\n");
		return -EINVAL;
	}

	switch (cmd) {
	case PERF_MGR_FPS_NUM:
		return g_fps;

	/*
	 * There are two cases removing the task from the list.
	 * F/G Tasks changed and tasks killed.
	 */

	case PERF_MGR_PROCESS_KILL:
		if (copy_from_user(&target_tid, uarg, sizeof(int))) {
			pr_err("[GPIS] :prsc kill: fail to copy from user");
			return -EFAULT;
		}
		if (fps_task_count <= 1)
			break;

		 /* Sometimes All Task Clear in the list. */
		if (target_tid < 0) {
			rcu_read_lock();
			list_for_each_entry_rcu(fi, &head_list, list) {
				if (fi == NULL)
					continue;

				fi->last_update_frame = 0;
				fi->updated_fps_util = 0;
				task = find_task_by_vpid(fi->orig_fps_info.tid);

				if (task == NULL || fi->orig_fps_info.tid != task->pid)
					continue;

				get_task_struct(task);
				task->drawing_mig_boost = 0;
				put_task_struct(task);
			}
			rcu_read_unlock();
		} else {
			//Drawing Flag OFF on Task Struct
			rcu_read_lock();
			task = find_task_by_vpid(target_tid);
			if (task != NULL) {
				get_task_struct(task);
				task->drawing_flag = 0;
				put_task_struct(task);
			}
			rcu_read_unlock();

			fi = get_target_task(target_tid);
			if (fi == NULL)
				break;

			spin_lock(&write_slock);
			list_del_rcu(&(fi->list));
			spin_unlock(&write_slock);
			fps_task_count--;
			synchronize_rcu();
			kfree(fi);
		}
		trace_printk("[GPIS] ::: Delete TID is %d, Task Cnt : %d\n",
			target_tid, fps_task_count);
		break;

	/*
	 * New tasks start to drawing frames as top-app.
	 * we add all the new drawing tasks to lists.
	 */

	case PERF_MGR_TASK_ADD:
		if (copy_from_user(&fps_info_val, uarg, sizeof(struct fps_info))) {
			pr_err("[GPIS] : PERF_MGR_TASK_ADD: fail to copy data from user");
			return -EFAULT;
		}

		task = find_task_by_vpid(fps_info_val.tid);
		if (task == NULL)
			break;

		fi = get_target_task(fps_info_val.tid);
		/* task already exist on drawing task list */
		if (fi != NULL)
			break;

		fi = kmalloc(sizeof(struct task_fps_util_info), GFP_KERNEL);
		if (!fi)
			return -EAGAIN;

		task->drawing_flag = fps_info_val.group_id;

		fi->orig_fps_info.tid = fps_info_val.tid;
		fi->orig_fps_info.group_id = fps_info_val.group_id;
		fi->updated_fps_util = 0;
		fi->orig_fps_info.boosting_lvl = BOOST_OFF;

		spin_lock(&write_slock);
		list_add_tail_rcu(&(fi->list), &head_list);
		spin_unlock(&write_slock);
		fps_task_count++;
		trace_printk("[GPIS] ::: Add Tid : %d in Group %d, Cnt : %d\n",
			fi->orig_fps_info.tid, fps_info_val.group_id,
			fps_task_count);
		break;

	/*
	 * In case of drawing frames finished, F/W calls this command.
	 * It checks if it's jank situation.
	 * if so, calculating fps boosted util on target task.
	 */

	case PERF_MGR_FRAME_END:
		fiSize = sizeof(struct fps_info);

		if (copy_from_user(&fps_info_val, uarg, fiSize)) {
			pr_err("[GPIS] : FrameEnd: fail to copy from user");
			return -EFAULT;
		}
		task = find_task_by_vpid(fps_info_val.tid);

		if (list_empty(&head_list) || !task || !task->drawing_flag)
			break;

		rcu_read_lock();
		list_for_each_entry_rcu(fi, &head_list, list) {
			if (fi == NULL)
				continue;

			ofi = fi->orig_fps_info;

			if (ofi.tid == fps_info_val.tid)
				target_fi = fi;

		/*
		 *Find target task which is drawing the frame currently
		 *If the drawing task keeps boosting value more than 2 frames,
		 *we initialize boosting information of the task.
		 */

			tmp_task = find_task_by_vpid(ofi.tid);

			if (tmp_task == NULL ||
				tmp_task->drawing_flag != task->drawing_flag)
				continue;
				
			if (++(fi->last_update_frame) > hold_frame_count) {
				fi->last_update_frame = 0;
				fi->updated_fps_util = 0;
				tmp_task->drawing_mig_boost = 0;
			}

			rn_sum += get_task_util(tmp_task);
		}
		rcu_read_unlock();

		if (target_fi == NULL) {
			pr_err("[GPIS] PID %d not found. skip cal util\n",
				fps_info_val.tid);
			break;
		}

		/* We can choose the maximum value
		 * between current calculated value and previous saved value.
		 */
		prev_fps_util = target_fi->updated_fps_util;
		duration = (fps_info_val.boosting_lvl > BOOST_OFF) ?
			(us_frame_time * 1000) : fps_info_val.duration;
		new_fps_util = calc_fps_required_util(rn_sum, duration);

		if (new_fps_util > 0 &&
			fps_info_val.boosting_lvl != BOOST_LOW) {
			rcu_read_lock();
			list_for_each_entry_rcu(fi, &head_list, list) {
				ofi = fi->orig_fps_info;
				if (ofi.group_id == task->drawing_flag) {
					tmp_task = find_task_by_vpid(ofi.tid);
					if (tmp_task != NULL)
						tmp_task->drawing_mig_boost = 1;
				}
			}
			rcu_read_unlock();
		}

		/* Once we can choose boosted values between new and prev,
		 * initialize update info
		 */

		if (fps_info_val.boosting_lvl != BOOST_MID) {
			target_fi->updated_fps_util =
				max(target_fi->updated_fps_util, new_fps_util);
		} else {
			target_fi->updated_fps_util = 0;
		}

		if (target_fi->updated_fps_util != prev_fps_util)
			target_fi->last_update_frame = 0;

		trace_printk("[GPIS] FPS, Tid, Mig, CalUtil = %d, %d, %d, %lu\n",
			g_fps, target_fi->orig_fps_info.tid,
			task->drawing_mig_boost, fps_required_util);

		break;
	default:
		break;
	}
	return ret;
}

static struct task_fps_util_info *get_target_task(pid_t tid)
{

	struct task_fps_util_info *fi = NULL, *ret_fi = NULL;

	if (list_empty(&head_list))
		return NULL;

	rcu_read_lock();
	list_for_each_entry_rcu(fi, &head_list, list) {
		if (fi != NULL && fi->orig_fps_info.tid == tid) {
			ret_fi = fi;
			break;
		}
	}
	rcu_read_unlock();

	return ret_fi;
}

unsigned long get_fps_req_util(void)
{
	return fps_required_util;
}
EXPORT_SYMBOL_GPL(get_fps_req_util);

unsigned long get_max_fps_util(int group_id)
{

	struct task_fps_util_info *fi = NULL;
	unsigned long max_util = 0;

	if (fps_task_count <= 1 || list_empty(&head_list))
		return 0;

	rcu_read_lock();
		list_for_each_entry_rcu(fi, &head_list, list) {
				if (fi != NULL &&
					fi->orig_fps_info.group_id == group_id &&
					fi->updated_fps_util > max_util)
					max_util = fi->updated_fps_util;
		}
	rcu_read_unlock();

	return max_util;
}
EXPORT_SYMBOL_GPL(get_max_fps_util);

unsigned long calc_fps_required_util(unsigned long rn_sum, unsigned long dur)
{

	unsigned long required_cap;

	unsigned long us_scale_dur = dur / 1000;	// ns -> us
	unsigned long required_rate = 0;
	unsigned long margin = 0;

	if (fps_margin_percent > 0)
		margin = (us_frame_time * (fps_margin_percent * 10)) >> 10;

	if (g_fps == 0 || us_frame_time == 0)
		return 0;

	required_rate = (us_scale_dur * FP_SCALE) / (us_frame_time - margin);

	if (required_rate <= (1 * FP_SCALE))
		return 0;
	else
		required_cap =  required_rate * rn_sum / FP_SCALE;

	if (required_cap > 1024)
		required_cap = 1024;

	return required_cap;
}

int panel_timing_changed_data_notify(struct notifier_block *nb,
	unsigned long event_type, void *data)
{
	struct panel_dms_data *dms_data = data;

	if (event_type == PANEL_EVENT_VRR_CHANGED) {
		// get fps information
		trace_printk("[GPIS] fps changed from %d to %d\n:",
			g_fps, dms_data->fps);
		g_fps = dms_data->fps;
		us_frame_time = 1000000 / g_fps;
	}
	return 0;
}
static struct notifier_block panel_timing_changed_data_notifier = {
	.notifier_call = panel_timing_changed_data_notify,
	.priority = 1,
};

static int perf_mgr_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int perf_mgr_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations perf_mgr_fops = {

	.owner = THIS_MODULE,
	.unlocked_ioctl = perf_mgr_ioctl,
	.open = perf_mgr_open,
	.release = perf_mgr_release,
};

static struct miscdevice perf_mgr_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "perf_manager",
	.fops = &perf_mgr_fops,
};

static ssize_t fps_margin_percent_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
		return snprintf(buf, MAX_INSERT_DIGIT, "%d\n", fps_margin_percent);
}

static ssize_t fps_margin_percent_store(struct kobject *kobj,
			struct kobj_attribute *attr, const char *buf, size_t n)
{
		sscanf(buf, "%du", &fps_margin_percent);
		return n;
}

perf_attr(fps_margin_percent);

static ssize_t hold_frame_count_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
		return snprintf(buf, MAX_INSERT_DIGIT, "%d\n", hold_frame_count);
}

static ssize_t hold_frame_count_store(struct kobject *kobj,
			struct kobj_attribute *attr, const char *buf, size_t n)
{
		sscanf(buf, "%du", &hold_frame_count);
		return n;
}

perf_attr(hold_frame_count);

static struct attribute *g[] = {
	&fps_margin_percent_attr.attr,
	&hold_frame_count_attr.attr,
	NULL,
};

static const struct attribute_group attr_group = {
	.attrs = g,
};

static int __init perf_mgr_dev_init(void)
{

	int err;
	struct task_fps_util_info *s;

	err = misc_register(&perf_mgr_device);

	if (err)
		return err;

	ss_panel_notifier_register(&panel_timing_changed_data_notifier);

	//Tunable Sysfs Init.
	perf_kobject = kobject_create_and_add("gpis", NULL);
	if (!perf_kobject)
		return -ENOMEM;
	err = sysfs_create_group(perf_kobject, &attr_group);
	if (err) {
		pr_err("[GPIS] Failed to create sysfs in /sys/gpis\n");
		return err;
	}

	INIT_LIST_HEAD(&head_list);
	spin_lock_init(&write_slock);

	s = kmalloc(sizeof(struct task_fps_util_info), GFP_KERNEL);
	if (s == NULL)
		return -EAGAIN;

	fps_required_util = 0;
	fps_task_count = 0;
	us_frame_time = 0;
	g_fps = 0;
	fps_margin_percent = 30;
	hold_frame_count = 2;

	s->orig_fps_info.tid = 0;
	s->orig_fps_info.duration = 0;
	s->updated_fps_util = 0;
	s->running_cpu = 9999;
	fps_task_count++;
	list_add_tail(&(s->list), &head_list);

	return 0;
}

static void  __exit perf_mgr_dev_exit(void)
{
	misc_deregister(&perf_mgr_device);
	ss_panel_notifier_unregister(&panel_timing_changed_data_notifier);
}

module_init(perf_mgr_dev_init);
module_exit(perf_mgr_dev_exit);
