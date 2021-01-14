#include <linux/input/input_booster.h>
#include <linux/msm-bus.h>
#include <linux/msm-bus-board.h>

u32 bus_hdl = 0;
struct pm_qos_request lpm_bias_pm_qos_request;
int release_value[MAX_RES_COUNT];
#define BUF_MAX	1024
#define MBPS_TO_BPS	1024 *1024

#define MHZ_TO_BPS(mhz, w) ((uint64_t)mhz * 1000 * 1000 * w)
#define CEILING_POS(X) ((X-(uint64_t)(X)) > 0 ? (uint64_t)(X+1) : (uint64_t)(X))
#define BPS_TO_MHZ(BPS, W) (CELIING_POS((BPS / 1000 / 1000 / W)))

#if defined(CONFIG_ARCH_KONA)
#define NUM_BUS_TABLE 13
#define BUS_W 4	/* SM8250 DDR Voting('w' for DDR is 4) */

void set_hmp(int level);

int ab_ib_bus_vectors[NUM_BUS_TABLE][2] = {
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

int ab_ib_bus_vectors[NUM_BUS_TABLE][2] = {
	{0, 0},		/* 0 */
};
#endif	//set null for other chipset


struct msm_bus_vectors touch_reg_bus_vectors[NUM_BUS_TABLE];
struct msm_bus_paths touch_reg_bus_usecases[ARRAY_SIZE(touch_reg_bus_vectors)];
struct msm_bus_scale_pdata touch_reg_bus_scale_table = {
	.usecase = touch_reg_bus_usecases,
	.num_usecases = ARRAY_SIZE(touch_reg_bus_usecases),
	.name = "touch_bw",
};

void fill_bus_vector(void) {
	int i = 0;
	for (i = 0; i < NUM_BUS_TABLE; i++) {
		touch_reg_bus_vectors[i].src = MSM_BUS_MASTER_AMPSS_M0;
		touch_reg_bus_vectors[i].dst = MSM_BUS_SLAVE_EBI_CH0;
		touch_reg_bus_vectors[i].ab = ab_ib_bus_vectors[i][0];
		touch_reg_bus_vectors[i].ib = MHZ_TO_BPS(ab_ib_bus_vectors[i][1], BUS_W);
	}
}

int trans_freq_to_idx(int request_ddr_freq) {
	int i = 0;

	if (request_ddr_freq <= 0) {
		return 0;
	}

	// in case of null table, return 0
	if (NUM_BUS_TABLE == 1) {
		return 0;
	}

	for (i = 0; i < NUM_BUS_TABLE-1; i++) {
		if (request_ddr_freq > ab_ib_bus_vectors[i][1] &&
			request_ddr_freq <= ab_ib_bus_vectors[i+1][1]) {
			return (i+1);
		}
	}

	return 0;
}

void ib_set_booster(int* qos_values)
{
	int value = -1;
	int ddr_new_value = 0;
	int res_type =0;

	for(res_type = 0; res_type < MAX_RES_COUNT; res_type++) {
		value = qos_values[res_type];

		if (value <= 0)
			continue;

		switch(res_type) {
		case CPUFREQ:
			set_freq_limit(DVFS_TOUCH_ID, value);
			pr_booster("ib_set_booster :: cpufreq value : %d", value);
			break;
		case DDRFREQ:
			ddr_new_value = trans_freq_to_idx(value);
			pr_booster("ib_set_booster :: ddr_new_value : %d", ddr_new_value);
			msm_bus_scale_client_update_request(bus_hdl, ddr_new_value);
			break;
		case HMPBOOST:
			set_hmp(value);
			pr_booster("ib_set_booster :: hmpboost value : %d", value);
			break;
		case LPMBIAS:
			pm_qos_update_request(&lpm_bias_pm_qos_request, value);
			pr_booster("ib_set_booster :: lpm bias value : %d", value);
			break;
		default:
			break;
		}
	}
}

void ib_release_booster(long *rel_flags)
{
	//cpufreq : -1, ddrfreq : 0, HMP : 0, lpm_bias = 0
	int flag;
	int ddr_new_value = 0;
	int value;
	int res_type = 0;

	for (res_type = 0; res_type < MAX_RES_COUNT; res_type++) {
		flag = rel_flags[res_type];
		if (flag <= 0)
			continue;

		value = release_value[res_type];

		switch (res_type) {
		case CPUFREQ:
			set_freq_limit(DVFS_TOUCH_ID, value);
			break;
		case DDRFREQ:
			ddr_new_value = trans_freq_to_idx(value);
			msm_bus_scale_client_update_request(bus_hdl, ddr_new_value);
			break;
		case HMPBOOST:
			set_hmp(value);
			break;
		case LPMBIAS:
			pm_qos_update_request(&lpm_bias_pm_qos_request, value);
			break;
		default:
			break;
		}
	}
}

void input_booster_init_vendor(int* release_val)
{
	int i = 0;

	pm_qos_add_request(&lpm_bias_pm_qos_request,
		PM_QOS_BIAS_HYST, PM_QOS_BIAS_HYST_DEFAULT_VALUE);

	fill_bus_vector();
	for (i = 0; i < touch_reg_bus_scale_table.num_usecases; i++) {
		touch_reg_bus_usecases[i].num_paths = 1;
		touch_reg_bus_usecases[i].vectors = &touch_reg_bus_vectors[i];
	}

	bus_hdl = msm_bus_scale_register_client(&touch_reg_bus_scale_table);

	for (i = 0; i < MAX_RES_COUNT; i++) {
		release_value[i] = release_val[i];
	}
}

void input_booster_exit_vendor()
{
	//msm_bus_scale_unregister_client(bus_hdl);
	pm_qos_remove_request(&lpm_bias_pm_qos_request);
}

#ifndef CONFIG_CPU_FREQ_LIMIT_USERSPACE
#define DVFS_TOUCH_ID	0
int set_freq_limit(unsigned long id, unsigned int freq)
{
	pr_err("%s is not yet implemented\n", __func__);
	return 0;
}
#endif

#ifdef USE_HMP_BOOST
void set_hmp(int level)
{
	if (level != current_hmp_boost) {
		if (level == 0) {
			level = -current_hmp_boost;
			current_hmp_boost = 0;
		} else {
			current_hmp_boost = level;
		}
		pr_booster("[Input Booster2] ******      set_hmp : %d ( %s )\n", level, __func__);
		if (sched_set_boost(level) < 0) {
			pr_booster("[Input Booster2] ******            !!! fail to HMP !!!\n"); \
		}
	}
}
#else
void set_hmp(int level)
{
	pr_booster("It does not use hmp\n");
}
#endif