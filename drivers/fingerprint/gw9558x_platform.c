#include "fingerprint.h"
#include "gw9558x_common.h"

int gw9558_register_platform_variable(struct gf_device *gf_dev)
{
	return 0;
}

int gw9558_unregister_platform_variable(struct gf_device *gf_dev)
{
	return 0;
}

void gw9558_spi_setup_conf(struct gf_device *gf_dev, u32 bits)
{
#ifndef ENABLE_SENSORS_FPRINT_SECURE
	gf_dev->spi->bits_per_word = 8 * bits;
	if (gf_dev->prev_bits_per_word != gf_dev->spi->bits_per_word) {
		if (spi_setup(gf_dev->spi))
			pr_err("failed to setup spi conf\n");
		pr_info("prev-bpw:%d, bpw:%d\n",
				gf_dev->prev_bits_per_word, gf_dev->spi->bits_per_word);
		gf_dev->prev_bits_per_word = gf_dev->spi->bits_per_word;
	}
#endif
}

int gw9558_spi_clk_enable(struct gf_device *gf_dev)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	if (!gf_dev->enabled_clk) {
		wake_lock(&gf_dev->wake_lock);
		gf_dev->enabled_clk = true;
	}
#endif
	return 0;
}

int gw9558_spi_clk_disable(struct gf_device *gf_dev)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	if (gf_dev->enabled_clk) {
		wake_unlock(&gf_dev->wake_lock);
		gf_dev->enabled_clk = false;
	}
#endif
	return 0;
}

int gw9558_set_cpu_speedup(struct gf_device *gf_dev, int onoff)
{
	int retval = 0;

#if 0
	int retry_cnt = 0;

	if (gf_dev->min_cpufreq_limit) {
		if (onoff) {
			pr_info("FP_CPU_SPEEDUP ON\n");
			pm_qos_add_request(&gf_dev->pm_qos, PM_QOS_CPU_DMA_LATENCY, 0);
			do {
				retval = set_freq_limit(DVFS_FINGER_ID, gf_dev->min_cpufreq_limit);
				retry_cnt++;
				if (retval) {
					pr_err("booster start failed. (%d) retry: %d\n",
					retval, retry_cnt);
					usleep_range(500, 510);
				}
			} while (retval && retry_cnt < 7);
		} else {
			pr_info("FP_CPU_SPEEDUP OFF\n");
			retval = set_freq_limit(DVFS_FINGER_ID, -1);
			if (retval)
				pr_err("booster stop failed. (%d)\n", retval);
			pm_qos_remove_request(&gf_dev->pm_qos);
		}
	}
#else
	pr_info("FP_CPU_SPEEDUP does not set\n");
#endif
	return retval;
}

int gw9558_pin_control(struct gf_device *gf_dev, bool pin_set)
{
	return 0;
}

