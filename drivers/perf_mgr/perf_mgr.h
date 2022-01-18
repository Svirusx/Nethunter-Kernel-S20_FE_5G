#include <linux/ioctl.h>
#include <linux/types.h>

#define PERF_MGR_MAGIC 'p'
#define FP_SCALE 1024

#define BOOST_OFF 0
#define BOOST_LOW 1
#define BOOST_MID 2
#define BOOST_HIGH 3

enum {
	TASK_ADD =1,
	FRAME_END,
	FPS_NUM,
	PROCESS_KILL,
};

struct fps_info {
	pid_t tid;
	int group_id;
	int boosting_lvl;
	int64_t duration;
};

struct task_fps_util_info{
	struct fps_info orig_fps_info;
	unsigned long updated_fps_util;
	int running_cpu;
	int last_update_frame;
	struct list_head list;
};

#define PERF_MGR_FPS_NUM				_IOWR(PERF_MGR_MAGIC, FPS_NUM, int*)
#define PERF_MGR_PROCESS_KILL			_IOWR(PERF_MGR_MAGIC, PROCESS_KILL, int*)
#define PERF_MGR_TASK_ADD			_IOWR(PERF_MGR_MAGIC, TASK_ADD, int*)
#define PERF_MGR_FRAME_END				_IOWR(PERF_MGR_MAGIC, FRAME_END, struct fps_info)

#define perf_attr(_name) \
static struct kobj_attribute _name##_attr = {	\
	.attr	= {									\
			.name = __stringify(_name),			\
			.mode = 0664,						\
	},											\
	.show	= _name##_show,						\
	.store	= _name##_store,					\
}
#define power_attr_ro(_name) \
static struct kobj_attribute _name##_attr = {	\
	.attr	= {									\
			.name = __stringify(_name),			\
			.mode = S_IRUGO,					\
	},											\
	.show	= _name##_show,						\
}
