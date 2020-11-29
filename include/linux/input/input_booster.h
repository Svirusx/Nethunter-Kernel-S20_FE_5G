#ifndef _INPUT_BOOSTER_H_
#define _INPUT_BOOSTER_H_

#include <linux/pm_qos.h>
#include <linux/of.h>
#include <linux/cpufreq.h>

#ifdef CONFIG_DEBUG_BUS_VOTER
#include <linux/msm-bus.h>
#endif

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

#ifdef CONFIG_SCHED_HMP
#define USE_HMP_BOOST
#endif

#define ITAG	" [Input Booster] "
#define pr_booster(format, ...) { \
	if (debug_flag) \
		printk(ITAG format, ## __VA_ARGS__); \
}

#define MAX_MULTI_TOUCH_EVENTS		10
#define MAX_EVENTS			(MAX_MULTI_TOUCH_EVENTS * 10)

#define INPUT_BOOSTER_NULL	-1
#define INIT_ZERO	0


static unsigned int debug_flag = INIT_ZERO;
static unsigned int enable_event_booster = INIT_ZERO;

#define HEADGAGE "******"
#define TAILGAGE "****  "

#define GET_MAX_SIZE(param_size, orig_size) { \
	max_size = sizeof(param_size)/sizeof(orig_size); \
}

#if defined(CONFIG_ARCH_EXYNOS) //______________________________________________________________________________
#define set_qos(req, pm_qos_class, value) { \
	if (value) { \
		if (pm_qos_request_active(req)) {\
			pr_booster("[Input Booster2] %s      pm_qos_update_request : %d\n", glGage, value); \
			pm_qos_update_request(req, value); \
		} else { \
			pr_booster("[Input Booster2] %s      pm_qos_add_request : %d\n", glGage, value); \
			pm_qos_add_request(req, pm_qos_class, value); \
		} \
	} else { \
		pr_booster("[Input Booster2] %s      remove_qos\n", glGage); \
		remove_qos(req); \
	} \
}

#define remove_qos(req) { \
	if (pm_qos_request_active(req)) \
		pm_qos_remove_request(req); \
}

#ifdef USE_HMP_BOOST
#define set_hmp(enable)	 { \
	if (enable != current_hmp_boost) { \
		pr_booster("[Input Booster2] ******      set_hmp : %d ( %s )\n", enable, __func__); \
		if (set_hmp_boost(enable) < 0) {\
			pr_booster("[Input Booster2] ******            !!! fail to HMP !!!\n"); \
		} \
		current_hmp_boost = enable; \
	} \
}
#else
#define set_hmp(enable)
#endif

#define SET_BOOSTER  { \
	int freq = -1;\
	_this->level++; \
	MAX_T_INPUT_BOOSTER(freq, cpu_freq); \
	pr_booster("[Input Booster2] %s      set_freq_limit : %d\n", glGage, freq); \
	set_hmp(_this->param[_this->index].hmp_boost); \
	set_qos(&_this->cpu_qos, /*PM_QOS_CPU_FREQ_MIN*/PM_QOS_CLUSTER1_FREQ_MIN, _this->param[_this->index].cpu_freq);  \
	set_qos(&_this->kfc_qos, /*PM_QOS_KFC_FREQ_MIN*/PM_QOS_CLUSTER0_FREQ_MIN, _this->param[_this->index].kfc_freq);  \
	set_qos(&_this->mif_qos, PM_QOS_BUS_THROUGHPUT, _this->param[_this->index].mif_freq);  \
	set_qos(&_this->int_qos, PM_QOS_DEVICE_THROUGHPUT, _this->param[_this->index].int_freq);  \
}
#define REMOVE_BOOSTER  { \
	int freq = -1;\
	_this->level = -1; \
	MAX_T_INPUT_BOOSTER(freq, cpu_freq); \
	pr_booster("[Input Booster2] %s      set_freq_limit : %d\n", glGage, freq); \
	set_hmp(0);  \
	remove_qos(&_this->cpu_qos);  \
	remove_qos(&_this->kfc_qos);  \
	remove_qos(&_this->mif_qos);  \
	remove_qos(&_this->int_qos);  \
}
#define PROPERTY_BOOSTER(_device_param_, _dt_param_, _time_)  { \
	_device_param_.cpu_freq = _dt_param_.cpu_freq; \
	_device_param_.kfc_freq = _dt_param_.kfc_freq; \
	_device_param_.mif_freq = _dt_param_.mif_freq; \
	_device_param_.int_freq = _dt_param_.int_freq; \
	_device_param_.time = _dt_param_._time_; \
	_device_param_.hmp_boost = _dt_param_.hmp_boost; \
}
#define SYSFS_DEFINE(_A_, _B_)  { \
	_A_ cpu_freq _B_ ; \
	_A_ kfc_freq _B_ ; \
	_A_ mif_freq _B_ ; \
	_A_ int_freq _B_ ; \
	_A_ hmp_boost _B_ ; \
	_A_ head_time _B_ ; \
	_A_ tail_time _B_ ; \
	_A_ phase_time _B_ ; \
}
#define SYSFS_COPY_TO_FROM(_A_, _B_) { \
	_A_ cpu_freq = (*cpu_freq == (unsigned int)(-1)) ? _B_ cpu_freq : *cpu_freq; \
	_A_ kfc_freq = (*kfc_freq == (unsigned int)(-1)) ? _B_ kfc_freq : *kfc_freq; \
	_A_ mif_freq = (*mif_freq == (unsigned int)(-1)) ? _B_ mif_freq : *mif_freq; \
	_A_ int_freq = (*int_freq == (unsigned int)(-1)) ? _B_ int_freq : *int_freq; \
	_A_ hmp_boost = (*hmp_boost == (unsigned int)(-1)) ? _B_ hmp_boost : *hmp_boost; \
	_A_ head_time = (*head_time == (unsigned int)(-1)) ? _B_ head_time : *head_time; \
	_A_ tail_time = (*tail_time == (unsigned int)(-1)) ? _B_ tail_time : *tail_time; \
	_A_ phase_time = (*phase_time == (unsigned int)(-1)) ? _B_ phase_time : *phase_time; \
}
#define SYSFS_COPY_FROM(_A_) { \
	cpu_freq = _A_ cpu_freq; \
	kfc_freq = _A_ kfc_freq; \
	mif_freq = _A_ mif_freq; \
	int_freq = _A_ int_freq; \
	hmp_boost = _A_ hmp_boost; \
	head_time = _A_ head_time; \
	tail_time = _A_ tail_time; \
	phase_time = _A_ phase_time; \
}
#define DTSI_TO { \
	err |= of_property_read_u32_index(cnp, "input_booster,cpu_freqs", i, &dt_infor->param_tables[i].cpu_freq); \
	err |= of_property_read_u32_index(cnp, "input_booster,kfc_freqs", i, &dt_infor->param_tables[i].kfc_freq); \
	err |= of_property_read_u32_index(cnp, "input_booster,mif_freqs", i, &dt_infor->param_tables[i].mif_freq); \
	err |= of_property_read_u32_index(cnp, "input_booster,int_freqs", i, &dt_infor->param_tables[i].int_freq); \
	err |= of_property_read_u32_index(cnp, "input_booster,hmp_boost", i, &temp); dt_infor->param_tables[i].hmp_boost = (u8)temp; \
	err |= of_property_read_u32_index(cnp, "input_booster,head_times", i, &temp); dt_infor->param_tables[i].head_time = (u16)temp; \
	err |= of_property_read_u32_index(cnp, "input_booster,tail_times", i, &temp); dt_infor->param_tables[i].tail_time = (u16)temp; \
	err |= of_property_read_u32_index(cnp, "input_booster,phase_times", i, &temp); dt_infor->param_tables[i].phase_time = (u16)temp; \
}

#elif defined(CONFIG_ARCH_QCOM) //______________________________________________________________________________

#include <linux/msm-bus.h>
#include <linux/msm-bus-board.h>

#define MHZ_TO_BPS(mhz, w) ((uint64_t)mhz * 1000 * 1000 * w)

#if defined(CONFIG_ARCH_KONA)
#define NUM_BUS_TABLE 13
#define BUS_W 4	/* SM8250 DDR Voting('w' for DDR is 4) */

static int ab_ib_bus_vectors[NUM_BUS_TABLE][2] = {
{0, 0},		/* 0 */
{0, 200},	/* 1 */
{0, 300},	/* 2 */
{0, 451},	/* 3 */
{0, 547},	/* 4 */
{0, 681},	/* 5 */
{0, 768},	/* 6 */
{0, 1017},	/* 7 */
{0, 1353},	/* 8 */
{0, 1555},	/* 9 */
{0, 1804},	/* 10 */
{0, 2092},	/* 11 */
{0, 2736}	/* 12 */
};
#else	//CONFIG_ARCH_KONA
#define NUM_BUS_TABLE 1
#define BUS_W 0

static int ab_ib_bus_vectors[NUM_BUS_TABLE][2] = {
{0, 0},		/* 0 */
};
#endif	//set null for other chipset

static struct msm_bus_vectors touch_reg_bus_vectors[NUM_BUS_TABLE];

static void fill_bus_vector(void)
{
	int i = 0;

	for (i = 0; i < NUM_BUS_TABLE; i++) {
		touch_reg_bus_vectors[i].src = MSM_BUS_MASTER_AMPSS_M0;
		touch_reg_bus_vectors[i].dst = MSM_BUS_SLAVE_EBI_CH0;
		touch_reg_bus_vectors[i].ab = ab_ib_bus_vectors[i][0];
		touch_reg_bus_vectors[i].ib = MHZ_TO_BPS(ab_ib_bus_vectors[i][1], BUS_W);
	}
}

static int trans_freq_to_idx(int request_ddr_freq)
{
	int i = 0;

	if (request_ddr_freq <= 0)
		return 0;

	/* in case of null table, return 0*/
	if (NUM_BUS_TABLE == 1)
		return 0;

	for (i = 0; i < NUM_BUS_TABLE-1; i++) {
		if (request_ddr_freq > ab_ib_bus_vectors[i][1] &&
			request_ddr_freq <= ab_ib_bus_vectors[i+1][1]) {
			return (i+1);
		}
	}

	return 0;
}

static struct msm_bus_paths touch_reg_bus_usecases[ARRAY_SIZE(touch_reg_bus_vectors)];
static struct msm_bus_scale_pdata touch_reg_bus_scale_table = {
    .usecase = touch_reg_bus_usecases,
    .num_usecases = ARRAY_SIZE(touch_reg_bus_usecases),
    .name = "touch_bw",
};
static u32 bus_hdl;

#ifdef USE_HMP_BOOST
#define set_hmp(level)	 { \
	if (level != current_hmp_boost) { \
		if (level == 0) { \
			level = -current_hmp_boost; \
			current_hmp_boost = 0; \
		} else {\
			current_hmp_boost = level; \
		} \
		pr_booster("[Input Booster2] ******      set_hmp : %d ( %s )\n", level, __func__); \
		if (sched_set_boost(level) < 0) { \
			pr_booster("[Input Booster2] ******            !!! fail to HMP !!!\n"); \
		} \
	} \
}
#else
#define set_hmp(level)
#endif

#ifndef CONFIG_CPU_FREQ_LIMIT_USERSPACE
#define DVFS_TOUCH_ID	0
int set_freq_limit(unsigned long id, unsigned int freq)
{
	pr_err("%s is not yet implemented\n", __func__);
	return 0;
}
#endif

#ifdef CONFIG_ARCH_QCOM
static struct pm_qos_request lpm_bias_pm_qos_request;

#define SET_BOOSTER  { \
	int value = -1;\
	int ddr_new_value = 0; \
	_this->level++; \
	MAX_T_INPUT_BOOSTER(value, hmp_boost); \
	if (value == INPUT_BOOSTER_NULL) { \
		value = 0; \
	} \
	set_hmp(value); \
	MAX_T_INPUT_BOOSTER(value, cpu_freq); \
	pr_booster("[Input Booster2] %s      set freq limit : %d\n", glGage, value); \
	set_freq_limit(DVFS_TOUCH_ID, value);  \
	MAX_T_INPUT_BOOSTER(value, ddr_freq);  \
	ddr_new_value = trans_freq_to_idx(value); \
	msm_bus_scale_client_update_request(bus_hdl, ddr_new_value);\
	MAX_T_INPUT_BOOSTER(value, lpm_bias);  \
	if (value == INPUT_BOOSTER_NULL) { \
		value = 0; \
	} \
	pm_qos_update_request(&lpm_bias_pm_qos_request, value);\
}
#define REMOVE_BOOSTER  { \
	int value = -1;\
	_this->level = -1; \
	MAX_T_INPUT_BOOSTER(value, hmp_boost); \
	if (value == INPUT_BOOSTER_NULL) { \
		value = 0; \
	} \
	set_hmp(value); \
	MAX_T_INPUT_BOOSTER(value, cpu_freq); \
	pr_booster("[Input Booster2] %s      set freq limit : %d\n", glGage, value); \
	set_freq_limit(DVFS_TOUCH_ID, value);  \
	msm_bus_scale_client_update_request(bus_hdl, 0);\
	MAX_T_INPUT_BOOSTER(value, lpm_bias);  \
	if (value == INPUT_BOOSTER_NULL) { \
		value = 0; \
	} \
	pm_qos_update_request(&lpm_bias_pm_qos_request, value);\
}
#else
#define SET_BOOSTER  { \
	int value = -1;\
	_this->level++; \
	MAX_T_INPUT_BOOSTER(value, hmp_boost); \
	if (value == INPUT_BOOSTER_NULL) { \
		value = 0; \
	} \
	set_hmp(value); \
	MAX_T_INPUT_BOOSTER(value, cpu_freq); \
	pr_booster("[Input Booster2] %s      set_freq_limit : %d\n", glGage, value); \
	set_freq_limit(DVFS_TOUCH_ID, value);  \
}
#define REMOVE_BOOSTER  { \
	int value = -1;\
	_this->level = -1; \
	MAX_T_INPUT_BOOSTER(value, hmp_boost); \
	if (value == INPUT_BOOSTER_NULL) { \
		value = 0; \
	} \
	set_hmp(value); \
	MAX_T_INPUT_BOOSTER(value, cpu_freq); \
	pr_booster("[Input Booster2] %s      set_freq_limit : %d\n", glGage, value); \
	set_freq_limit(DVFS_TOUCH_ID, value);  \
}
#endif

#define PROPERTY_BOOSTER(_device_param_, _dt_param_, _time_)  { \
	_device_param_.cpu_freq = _dt_param_.cpu_freq; \
	_device_param_.ddr_freq = _dt_param_.ddr_freq; \
	_device_param_.time = _dt_param_._time_; \
	_device_param_.hmp_boost = _dt_param_.hmp_boost; \
	_device_param_.lpm_bias = _dt_param_.lpm_bias; \
}
#define SYSFS_DEFINE(_A_, _B_)  { \
	_A_ cpu_freq _B_ ; \
	_A_ ddr_freq _B_ ; \
	_A_ hmp_boost _B_ ; \
	_A_ lpm_bias _B_ ; \
	_A_ head_time _B_ ; \
	_A_ tail_time _B_ ; \
}
#define SYSFS_COPY_TO_FROM(_A_, _B_) { \
	_A_ cpu_freq = (*cpu_freq == (unsigned int)(-1)) ? _B_ cpu_freq : *cpu_freq; \
	_A_ ddr_freq = (*ddr_freq == (unsigned int)(-1)) ? _B_ ddr_freq : *ddr_freq; \
	_A_ hmp_boost = (*hmp_boost == (unsigned int)(-1)) ? _B_ hmp_boost : *hmp_boost; \
	_A_ lpm_bias = (*lpm_bias == (unsigned int)(-1)) ? _B_ lpm_bias : *lpm_bias; \
	_A_ head_time = (*head_time == (unsigned int)(-1)) ? _B_ head_time : *head_time; \
	_A_ tail_time = (*tail_time == (unsigned int)(-1)) ? _B_ tail_time : *tail_time; \
}
#define SYSFS_COPY_FROM(_A_) { \
	cpu_freq = _A_ cpu_freq; \
	ddr_freq = _A_ ddr_freq; \
	hmp_boost = _A_ hmp_boost; \
	lpm_bias = _A_ lpm_bias; \
	head_time = _A_ head_time; \
	tail_time = _A_ tail_time; \
}
#define DTSI_TO { \
	err |= of_property_read_u32_index(cnp, "input_booster,cpu_freqs", i, &dt_infor->param_tables[i].cpu_freq); \
	err |= of_property_read_u32_index(cnp, "input_booster,ddr_freqs", i, &dt_infor->param_tables[i].ddr_freq); \
	err |= of_property_read_u32_index(cnp, "input_booster,hmp_boost", i, &temp); dt_infor->param_tables[i].hmp_boost = (u8)temp; \
	err |= of_property_read_u32_index(cnp, "input_booster,lpm_bias", i, &temp); dt_infor->param_tables[i].lpm_bias = (u8)temp; \
	err |= of_property_read_u32_index(cnp, "input_booster,head_times", i, &temp); dt_infor->param_tables[i].head_time = (u16)temp; \
	err |= of_property_read_u32_index(cnp, "input_booster,tail_times", i, &temp); dt_infor->param_tables[i].tail_time = (u16)temp; \
}
#endif //______________________________________________________________________________
#define GET_BOOSTER_PARAM(_GENDER_, _HEAD_PARAM_, _TAIL_PARAM_) { \
	int levels[][3] = { \
		{1, 2, 0}, \
		{2, 2, 3}, \
		{3, 1, 1}, \
		{4, 1, 2} }; \
	int j, k; \
	for (j = 0; j < (int)(sizeof(levels)/(3*sizeof(int))); j++) {\
		if ((_GENDER_->pDT->nlevels > 2 && levels[j][0] == _GENDER_->level) || (_GENDER_->pDT->nlevels == 1 && j == 2) || (_GENDER_->pDT->nlevels == 2 && j == 3)) { \
			if (levels[j][1] > 0) { \
				for (k = 0; k < _GENDER_->pDT->nlevels; k++) { \
					if (levels[j][1] == _GENDER_->pDT->param_tables[k].ilevels) { \
						_HEAD_PARAM_ = (_GENDER_->pDT->param_tables[k].head_time > 0) ? &_GENDER_->pDT->param_tables[k] : NULL; \
						break; \
					} \
				} \
			} \
			if (levels[j][2] > 0) { \
				for (k = 0; k < dt_gender->pDT->nlevels; k++) { \
					if (levels[j][2] == dt_gender->pDT->param_tables[k].ilevels) { \
						_TAIL_PARAM_ = &_GENDER_->pDT->param_tables[k]; \
						break; \
					} \
				} \
			} \
			break; \
		} \
	} \
}
#define CHANGE_BOOSTER { \
	struct t_ib_dev_tree_param *head_param = NULL, *tail_param = NULL; \
	GET_BOOSTER_PARAM(dt_gender, head_param, tail_param) \
	memset(dt_gender->pBooster->param, 0x00, sizeof(struct t_input_booster_param)*2); \
	if (head_param != NULL) { \
		PROPERTY_BOOSTER(dt_gender->pBooster->param[0], (*head_param), head_time) \
	} \
	if (tail_param != NULL) { \
		PROPERTY_BOOSTER(dt_gender->pBooster->param[1], (*tail_param), tail_time) \
	} \
}

#define INIT_BOOSTER(_DEVICE_) { \
	_DEVICE_##_booster.input_booster_state = input_booster_idle_state; \
	INIT_DELAYED_WORK(&_DEVICE_##_booster.ib_timeout_work[0], TIMEOUT_FUNC(_DEVICE_)); \
	INIT_DELAYED_WORK(&_DEVICE_##_booster.ib_timeout_work[1], TIMEOUT_FUNC(_DEVICE_)); \
	INIT_WORK(&_DEVICE_##_booster.input_booster_set_booster_work, SET_BOOSTER_FUNC(_DEVICE_)); \
	mutex_init(&_DEVICE_##_booster.lock); \
	_DEVICE_##_booster.change_on_release = 0; \
	_DEVICE_##_booster.multi_events = 0; \
	{ \
		int i; \
		for (i = 0; i < (int)(sizeof(_DEVICE_##_booster.param)/sizeof(struct t_input_booster_param)); i++) { \
			_DEVICE_##_booster.level = -1; \
		} \
		for (i = 0; i < ndevice_in_dt; i++) { \
			if (is_ib_init_succeed() && \
					device_tree_infor[i].type == _DEVICE_##_booster_dt.type) { \
				struct t_ib_dev_tree_gender *dt_gender = &_DEVICE_##_booster_dt; \
				dt_gender->pDT = &device_tree_infor[i]; \
				dt_gender->pBooster = &_DEVICE_##_booster; \
				CHANGE_BOOSTER \
				break; \
			} \
		} \
	} \
}

#define TIMEOUT_FUNC(_DEVICE_) input_booster_##_DEVICE_##_timeout_work_func

#define DECLARE_TIMEOUT_FUNC(_DEVICE_) \
static void input_booster_##_DEVICE_##_timeout_work_func(struct work_struct *work)  \
{ \
	struct t_input_booster *_this = &_DEVICE_##_booster; \
	int param_max = sizeof(_this->param)/sizeof(struct t_input_booster_param), temp_index = -1;  \
	mutex_lock(&_this->lock); \
	pr_booster("[Input Booster] %s           Timeout : changed  index : %d (%s)\n", HEADGAGE, _this->index, __func__); \
	if (_this->index >= 2 && delayed_work_pending(&_this->ib_timeout_work[_this->index-2])) { \
		pr_booster("[SimSim] index 2 over and head not finished"); \
		mutex_unlock(&_this->lock); \
		return; \
	} \
	if (_this->index == param_max && delayed_work_pending(&_this->ib_timeout_work[_this->index-1])) { \
		pr_booster("[SimSim] index is 2 and tail not finished"); \
		temp_index = _this->index; \
		_this->index = (_this->index) ? _this->index-1 : 0; \
	} \
	pr_booster("[Input Booster] %s           Timeout : changed  index : %d (%s)\n", HEADGAGE, _this->index, __func__); \
	if (_this->index < param_max) { \
		pr_booster("[Input Booster] %s           Timeout : changed  index : %d, time : %d (%s)\n", HEADGAGE, _this->index, _this->param[_this->index].time, __func__); \
		pr_booster("[Input Booster] %s           cpu : %d (%s)\n", TAILGAGE, _this->param[_this->index].cpu_freq, __func__); \
		if (_this->param[(_this->index) ? _this->index-1 : 0].time > 0) { \
			SET_BOOSTER; \
			if (_this->change_on_release) { \
				schedule_delayed_work(&_this->ib_timeout_work[_this->index], msecs_to_jiffies(_this->param[_this->index].time)); \
				_this->index++; \
				CHANGE_STATE_TO(idle); \
			} \
		} \
		_this->index = (temp_index >= 0) ? temp_index : _this->index; \
	} else { \
		pr_booster("[Input Booster] Timeout : completed   param_max : %d (%s)\n", param_max, __func__); \
		pr_booster("[Input Booster]\n"); \
		REMOVE_BOOSTER; \
		_this->index = 0; \
		CHANGE_STATE_TO(idle); \
	} \
	mutex_unlock(&_this->lock); \
}

#define SET_BOOSTER_FUNC(_DEVICE_) input_booster_##_DEVICE_##_set_booster_work_func

#define DECLARE_SET_BOOSTER_FUNC(_DEVICE_) \
static void input_booster_##_DEVICE_##_set_booster_work_func(struct work_struct *work)  \
{ \
	struct t_input_booster *_this = (struct t_input_booster *)(&_DEVICE_##_booster); \
	mutex_lock(&_this->lock); \
	_this->input_booster_state(_this, _this->event_type); \
	mutex_unlock(&_this->lock); \
}

#define DECLARE_STATE_FUNC(_STATE_) void input_booster_##_STATE_##_state(void *__this, int input_booster_event)

#define CHANGE_STATE_TO(_STATE_) {_this->input_booster_state = input_booster_##_STATE_##_state; }

#define RUN_BOOSTER(_EVENT_,_BOOSTER_,_BOOSTER_DT_) { \
	if (_BOOSTER_DT_->level > 0) { \
		_BOOSTER_->event_type = _EVENT_; \
		if (_EVENT_ == BOOSTER_ON) { \
			_BOOSTER_->level = -1; \
		} \
		(_EVENT_ == BOOSTER_ON)  ? _BOOSTER_->multi_events++ : _BOOSTER_->multi_events--; \
		schedule_work(&_BOOSTER_->input_booster_set_booster_work); \
	} \
}

//+++++++++++++++++++++++++++++++++++++++++++++++  STRUCT & VARIABLE FOR SYSFS  +++++++++++++++++++++++++++++++++++++++++++++++//
#define SYSFS_CLASS(_ATTR_, _ARGU_, _COUNT_) \
	static ssize_t input_booster_sysfs_class_show_##_ATTR_(struct class *dev, struct class_attribute *attr, char *buf) \
	{ \
		struct t_ib_dev_tree_gender *dt_gender = &touch_booster_dt; \
		ssize_t ret; int level; \
		unsigned int enable_event = 0; \
		unsigned int debug_level = 0; \
		unsigned int cpu_freq = 0; \
		unsigned int ddr_freq = 0; \
		unsigned int hmp_boost = 0; \
		unsigned int lpm_bias = 0; \
		unsigned int head_time = 0; \
		unsigned int tail_time = 0; \
		struct t_ib_dev_tree_param *head_param = NULL, *tail_param = NULL; \
		GET_BOOSTER_PARAM(dt_gender, head_param, tail_param) \
		enable_event = enable_event_booster; \
		debug_level = debug_flag; \
		level = dt_gender->level; \
		if (strcmp(#_ATTR_, "head") == 0 && head_param != NULL) { \
			SYSFS_COPY_FROM(head_param->) \
		} \
		if (strcmp(#_ATTR_, "tail") == 0 && tail_param != NULL) { \
			SYSFS_COPY_FROM(tail_param->) \
		} \
		ret = sprintf _ARGU_; \
		pr_booster("[Input Booster8] %s buf : %s\n", __func__, buf); \
		return ret; \
	} \
	static ssize_t input_booster_sysfs_class_store_##_ATTR_(struct class *dev, struct class_attribute *attr, const char *buf, size_t count) \
	{ \
		struct t_ib_dev_tree_gender *dt_gender = &touch_booster_dt; \
		int level[1] = {-1}, len; \
		unsigned int enable_event[1] = {-1}; \
		unsigned int debug_level[1] = {-1}; \
		unsigned int cpu_freq[1] = {-1}; \
		unsigned int ddr_freq[1] = {-1}; \
		unsigned int hmp_boost[1] = {-1}; \
		unsigned int lpm_bias[1] = {-1}; \
		unsigned int head_time[1] = {-1}; \
		unsigned int tail_time[1] = {-1}; \
		struct t_ib_dev_tree_param *head_param = NULL, *tail_param = NULL; \
		GET_BOOSTER_PARAM(dt_gender, head_param, tail_param) \
		len = sscanf _ARGU_; \
		pr_booster("[Input Booster8] %s buf : %s\n", __func__, buf); \
		if (sscanf _ARGU_ != _COUNT_) { \
			return count; \
		} \
		enable_event_booster = (*enable_event == (unsigned int)(-1)) ? enable_event_booster : *enable_event; \
		debug_flag = (*debug_level == (unsigned int)(-1)) ? debug_flag : *debug_level; \
		dt_gender->level = (*level == (unsigned int)(-1)) ? dt_gender->level : *level; \
		if (*head_time != (unsigned int)(-1) && head_param != NULL) { \
			SYSFS_COPY_TO_FROM(head_param->, head_param->) \
		} \
		if (*tail_time != (unsigned int)(-1) && tail_param != NULL) { \
			SYSFS_COPY_TO_FROM(tail_param->, tail_param->) \
		} \
		CHANGE_BOOSTER \
		return count; \
	} \
	static struct class_attribute class_attr_##_ATTR_ = __ATTR(_ATTR_, S_IRUGO | S_IWUSR, input_booster_sysfs_class_show_##_ATTR_, input_booster_sysfs_class_store_##_ATTR_);

#define SYSFS_DEVICE(_ATTR_, _ARGU_, _COUNT_) \
	static ssize_t input_booster_sysfs_device_show_##_ATTR_(struct device *dev, struct device_attribute *attr, char *buf) \
	{ \
		struct t_ib_dev_tree_gender *dt_gender = dev_get_drvdata(dev); \
		ssize_t ret = 0; \
		int level, Arg_count = _COUNT_; \
		unsigned int debug_level = 0; \
		unsigned int cpu_freq = 0; \
		unsigned int ddr_freq = 0; \
		unsigned int hmp_boost = 0; \
		unsigned int lpm_bias = 0; \
		unsigned int head_time = 0; \
		unsigned int tail_time = 0; \
		struct t_ib_dev_tree_param *head_param = NULL, *tail_param = NULL; \
		if (dt_gender == NULL) { \
			return  ret; \
		} \
		GET_BOOSTER_PARAM(dt_gender, head_param, tail_param) \
		if (Arg_count == 1) { \
			level = dt_gender->level; \
			ret = sprintf _ARGU_; \
			pr_booster("[Input Booster8] %s buf : %s\n", __func__, buf); \
		} else { \
			if (head_param != NULL) { \
				level = head_param->ilevels; \
				SYSFS_COPY_FROM(head_param->) \
				ret = sprintf _ARGU_; \
				buf = buf + ret - 1; \
			} \
			*buf = '|'; \
			*(buf+1) = '\n';\
			if (tail_param != NULL) { \
				buf = buf + 1; \
				level = tail_param->ilevels; \
				SYSFS_COPY_FROM(tail_param->) \
				ret += sprintf _ARGU_; \
			} \
			pr_booster("[Input Booster8] %s buf : %s\n", __func__, buf); \
		} \
		return  ret; \
	} \
	static ssize_t input_booster_sysfs_device_store_##_ATTR_(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) \
	{ \
		struct t_ib_dev_tree_gender *dt_gender = dev_get_drvdata(dev); \
		struct t_ib_dev_tree_info *dt_infor = (dt_gender) ? dt_gender->pDT : NULL; \
		int level[1] = {-1}, len; \
		unsigned int debug_level[1] = {-1}; \
		unsigned int cpu_freq[1] = {-1}; \
		unsigned int ddr_freq[1] = {-1}; \
		unsigned int hmp_boost[1] = {-1}; \
		unsigned int lpm_bias[1] = {-1}; \
		unsigned int head_time[1] = {-1}; \
		unsigned int tail_time[1] = {-1}; \
		len = sscanf _ARGU_; \
		pr_booster("[Input Booster8] %s buf : %s\n", __func__, buf); \
		if (dt_infor == NULL) { \
			return  count; \
		} \
		if (len != _COUNT_) { \
			pr_booster("### Keep this format : [level cpu_freq kfc_freq mif_freq int_freq hmp_boost lpm_bias] (Ex: 1 1600000 0 1500000 667000 333000 1 5###\n"); \
			pr_booster("### Keep this format : [level head_time tail_time phase_time] (Ex: 1 130 500 50 ###\n"); \
			pr_booster("### Keep this format : [type value] (Ex: 2 1 ###\n"); \
			return count; \
		} \
		if (level[0] >= 0) { \
			int Arg_count = _COUNT_; \
			if (Arg_count == 1) { \
				dt_gender->level = level[0]; \
			} else { \
				int k; \
				for (k = 0; k < dt_infor->nlevels; k++) { \
					if (level[0] == dt_infor->param_tables[k].ilevels) { \
						SYSFS_COPY_TO_FROM(dt_infor->param_tables[k]., dt_infor->param_tables[k].) \
						pr_booster("[Input Booster8] %s time : %d %d\n", __func__, dt_infor->param_tables[*level].head_time, dt_infor->param_tables[k].tail_time); \
					} \
				} \
			} \
			CHANGE_BOOSTER \
		} \
		return count; \
	} \
	static DEVICE_ATTR(_ATTR_, S_IRUGO | S_IWUSR, input_booster_sysfs_device_show_##_ATTR_, input_booster_sysfs_device_store_##_ATTR_);

#define INIT_SYSFS_CLASS(_CLASS_) { \
		int ret = class_create_file(sysfs_class, &class_attr_##_CLASS_); \
		if (ret) { \
			pr_booster("[Input Booster] Failed to create class\n"); \
			class_destroy(sysfs_class); \
			return; \
		} \
	}
#define INIT_SYSFS_DEVICE(_DEVICE_) { \
		struct device   *sysfs_dev; int ret = 0;\
		sysfs_dev = device_create(sysfs_class, NULL, 0, &_DEVICE_##_booster_dt, #_DEVICE_); \
		if (IS_ERR(sysfs_dev)) { \
			ret = IS_ERR(sysfs_dev); \
			pr_booster("[Input Booster] Failed to create %s sysfs device[%d]\n", #_DEVICE_, ret); \
			return; \
		} \
		ret = sysfs_create_group(&sysfs_dev->kobj, &dvfs_attr_group); \
		if (ret) { \
			pr_booster("[Input Booster] Failed to create %s sysfs group\n", #_DEVICE_); \
			return; \
		} \
	}
//-----------------------------------------------  STRUCT & VARIABLE FOR SYSFS  -----------------------------------------------//

enum booster_mode_on_off {
	BOOSTER_OFF = 0,
	BOOSTER_ON,
};

enum {
	NONE_TYPE_DEVICE = -1,
	TOUCH = 0,
	TOUCH_KEY,
	KEYBOARD,
	SPEN,
	HOVER,
	KEY,
	MOUSE,
	MOUSH_WHEEL,
	MULTI_TOUCH,
	MAX_BOOSTER_CNT
};

struct input_value input_events[MAX_EVENTS+1];

struct t_input_booster_param {
#if defined(CONFIG_ARCH_EXYNOS) //______________________________________________________________________________
	u32 cpu_freq;
	u32 kfc_freq;
	u32 mif_freq;
	u32 int_freq;

	u16 time;

	u8 hmp_boost;
	u8 dummy;
#elif defined(CONFIG_ARCH_QCOM) //______________________________________________________________________________
	u32 cpu_freq;
	u32 ddr_freq;

	u8 hmp_boost;
	u32 lpm_bias;
	u16 time;
#endif //______________________________________________________________________________
};

struct t_input_booster {
	struct mutex lock;
	struct t_input_booster_param param[2];

#if defined(CONFIG_ARCH_EXYNOS) //______________________________________________________________________________
	struct pm_qos_request	cpu_qos;
	struct pm_qos_request	kfc_qos;
	struct pm_qos_request	mif_qos;
	struct pm_qos_request	int_qos;
#endif //______________________________________________________________________________

	struct delayed_work     ib_timeout_work[2];
	struct work_struct      input_booster_set_booster_work;

	int index;
	int multi_events;
	int event_type;
	int change_on_release;
	int level;

	void (*input_booster_state)(void *__this, int input_booster_event);
};

//+++++++++++++++++++++++++++++++++++++++++++++++  STRUCT & VARIABLE FOR DEVICE TREE  +++++++++++++++++++++++++++++++++++++++++++++++//
struct t_ib_dev_tree_param {
	u8		ilevels;

#if defined(CONFIG_ARCH_EXYNOS) //______________________________________________________________________________
	u8		hmp_boost;

	u16		head_time;
	u16		tail_time;
	u16		phase_time;

	u32		cpu_freq;
	u32		kfc_freq;
	u32		mif_freq;
	u32		int_freq;
#elif defined(CONFIG_ARCH_QCOM) //______________________________________________________________________________
	u8		hmp_boost;
	u32		lpm_bias;

	u16		head_time;
	u16		tail_time;

	u32		cpu_freq;
	u32		ddr_freq;
#endif //______________________________________________________________________________
};

struct t_ib_dev_tree_info {
	const char     *label;
	int     type;
	int     nlevels;

	struct t_ib_dev_tree_param      *param_tables;
};

struct t_ib_dev_tree_gender {
	int type;
	int level;
	struct t_input_booster	*pBooster;
	struct t_ib_dev_tree_info *pDT;
};

//______________________________________________________________________________	<<< in DTSI file >>>
//______________________________________________________________________________	input_booster,type = <4>;	/* BOOSTER_DEVICE_KEYBOARD */
//______________________________________________________________________________
struct t_ib_dev_tree_gender	key_booster_dt = {0, 1,};		// type : 0,  level : 1
struct t_ib_dev_tree_gender	touchkey_booster_dt = {1, 1,};		// type : 1,  level : 1
struct t_ib_dev_tree_gender	touch_booster_dt = {2, 2,};		// type : 2,  level : 2
struct t_ib_dev_tree_gender	multitouch_booster_dt = {3, 1,};		// type : 3,  level : 1
struct t_ib_dev_tree_gender	keyboard_booster_dt = {4, 1,};		// type : 4,  level : 1
struct t_ib_dev_tree_gender	mouse_booster_dt = {5, 1,};		// type : 5,  level : 1
struct t_ib_dev_tree_gender	mouse_wheel_booster_dt = {6, 1,};	// type : 6,  level : 1
struct t_ib_dev_tree_gender	hover_booster_dt = {7, 1,};		// type : 7,  level : 1
struct t_ib_dev_tree_gender	pen_booster_dt = {8, 1,};			// type : 8,  level : 1
struct t_ib_dev_tree_info	*device_tree_infor;

int ndevice_in_dt;
//----------------------------------------------  STRUCT & VARIABLE FOR DEVICE TREE  ----------------------------------------------//

//+++++++++++++++++++++++++++++++++++++++++++++++  STRUCT & VARIABLE FOR SYSFS  +++++++++++++++++++++++++++++++++++++++++++++++//
SYSFS_CLASS(enable_event, (buf, "%u\n", enable_event), 1)

#if defined(CONFIG_ARCH_EXYNOS) //______________________________________________________________________________
SYSFS_CLASS(debug_level, (buf, "%u\n", debug_level), 1)
SYSFS_CLASS(head, (buf, "%d %u %u %u %u %u\n", head_time, cpu_freq, kfc_freq, mif_freq, int_freq, hmp_boost), 6)
SYSFS_CLASS(tail, (buf, "%d %u %u %u %u %u\n", tail_time, cpu_freq, kfc_freq, mif_freq, int_freq, hmp_boost), 6)
SYSFS_CLASS(level, (buf, "%d\n", level), 1)
SYSFS_DEVICE(level, (buf, "%d\n", level), 1)
SYSFS_DEVICE(freq, (buf, "%d %u %u %u %u %u\n", level, cpu_freq, kfc_freq, mif_freq, int_freq, hmp_boost), 6)
SYSFS_DEVICE(time, (buf, "%d %u %u %u\n", level, head_time, tail_time, phase_time), 4)
#elif defined(CONFIG_ARCH_QCOM) //______________________________________________________________________________
SYSFS_CLASS(debug_level, (buf, "%u\n", debug_level), 1)
SYSFS_CLASS(head, (buf, "%u %u %u %u %u\n", head_time, cpu_freq, ddr_freq, hmp_boost, lpm_bias), 5)
SYSFS_CLASS(tail, (buf, "%u %u %u %u %u\n", tail_time, cpu_freq, ddr_freq, hmp_boost, lpm_bias), 5)
SYSFS_CLASS(level, (buf, "%d\n", level), 1)
SYSFS_DEVICE(level, (buf, "%d\n", level), 1)
SYSFS_DEVICE(freq, (buf, "%d %u %u %u %u\n", level, cpu_freq, ddr_freq, hmp_boost, lpm_bias), 5)
SYSFS_DEVICE(time, (buf, "%d %u %u\n", level, head_time, tail_time), 3)
#endif //______________________________________________________________________________
void input_booster_run_booster_on(void);
void input_booster_run_booster_off(void);
static ssize_t input_booster_sysfs_device_store_control(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int value;

	if (sscanf(buf, "%u", &value) != 1) {
		pr_booster("### Keep this format : [type value] (Ex: 2 1 ###\n");
		return count;
	}

	pr_booster("[Input Booster8] %s buf : %s\n", __func__, buf);

	if(value == 1) {
		input_booster_run_booster_on();
	}

	if(value == 0) {
		input_booster_run_booster_off();
	}

	return count;
}
static DEVICE_ATTR(control, S_IRUGO | S_IWUSR, NULL, input_booster_sysfs_device_store_control);
static struct attribute *dvfs_attributes[] = {
	&dev_attr_level.attr,
	&dev_attr_freq.attr,
	&dev_attr_time.attr,
	&dev_attr_control.attr,
	NULL,
};

static struct attribute_group dvfs_attr_group = {
	.attrs = dvfs_attributes,
};

//----------------------------------------------  STRUCT & VARIABLE FOR SYSFS  ----------------------------------------------//

char *glGage = HEADGAGE;
int current_hmp_boost = INIT_ZERO;

struct t_input_booster	key_booster;
struct t_input_booster	touchkey_booster;
struct t_input_booster	touch_booster;
struct t_input_booster	multitouch_booster;
struct t_input_booster	keyboard_booster;
struct t_input_booster	mouse_booster;
struct t_input_booster	mouse_wheel_booster;
struct t_input_booster	hover_booster;
struct t_input_booster	pen_booster;

struct t_input_booster *t_input_boosters[] = {
	&key_booster,
	&touchkey_booster,
	&touch_booster,
	&multitouch_booster,
	&keyboard_booster,
	&mouse_booster,
	&mouse_wheel_booster,
	&hover_booster,
	&pen_booster
};

struct ib_info {
	struct t_input_booster *input_booster;
	struct t_ib_dev_tree_gender *input_booster_dt;
};

#define MAX_T_INPUT_BOOSTER(ref, _PARAM_) { \
		int i = 0; \
		int max = INPUT_BOOSTER_NULL; \
		for (i = 0; i < (int)(sizeof(t_input_boosters)/sizeof(struct t_input_booster *)); i++) { \
			if (t_input_boosters[i]->level >= 0 && t_input_boosters[i]->level < (int)(sizeof(t_input_boosters[i]->param)/sizeof(struct t_input_booster_param))) { \
				pr_booster("[Input Booster3] %s booster type : %d    level : %d    value : %d\n", #_PARAM_, i, t_input_boosters[i]->level, t_input_boosters[i]->param[t_input_boosters[i]->level]._PARAM_); \
				if (max < (int)(t_input_boosters[i]->param[t_input_boosters[i]->level]._PARAM_)) { \
					max = (int)(t_input_boosters[i]->param[t_input_boosters[i]->level]._PARAM_); \
				} \
			} \
		} \
		if (max == INPUT_BOOSTER_NULL) { \
			ref = INPUT_BOOSTER_NULL; \
		} else { \
			ref = max; \
		} \
		pr_booster("[Input Booster3] %s max value : %d\n", #_PARAM_, max); \
	} \

int input_count = 0, key_back = 0, key_home = 0, key_recent = 0;

void input_booster_idle_state(void *__this, int input_booster_event);
void input_booster_press_state(void *__this, int input_booster_event);
void input_booster(struct input_dev *dev);
void input_booster_init(void);

void input_booster_run_booster_on(void)
{
	touch_booster.input_booster_state = input_booster_idle_state;

	struct t_ib_dev_tree_gender *trgt_booster_dt = &touch_booster_dt;
	struct t_input_booster *trgt_booster = &touch_booster;

	RUN_BOOSTER(BOOSTER_ON,trgt_booster,trgt_booster_dt);
}

void input_booster_run_booster_off(void)
{
	struct t_ib_dev_tree_gender *trgt_booster_dt = &touch_booster_dt;
	struct t_input_booster *trgt_booster = &touch_booster;
	RUN_BOOSTER(BOOSTER_OFF,trgt_booster,trgt_booster_dt);
}

int is_ib_init_succeed()
{
	return (device_tree_infor != NULL) ? 1 : 0;
}
#endif
