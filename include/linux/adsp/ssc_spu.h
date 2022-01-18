#ifndef SSC_SPU_HEADER
#define SSC_SPU_HEADER

#ifdef CONFIG_SUPPORT_SSC_SPU
#define VER_COMPARE_CNT		2
#define FILE_LEN            40
#define SLPI_VER_INFO		"/vendor/firmware_mnt/verinfo/slpi_ver.txt"
#define SLPI_SPU_VER_INFO	"/spu/sensorhub/slpi_ver.txt"
enum {
	SSC_SPU,
	SSC_ORI,
	SSC_ORI_AF_SPU_FAIL,
	SSC_CNT_MAX = SSC_ORI_AF_SPU_FAIL
};

enum {
	SSC_CL,
	SSC_YEAR,
	SSC_MONTH,
	SSC_DATE,
	SSC_HOUR,
	SSC_MIN,
	SSC_SEC,
	SSC_MSEC,
	SSC_VC_MAX
};
#endif

#endif  /* SSC_SPU_HEADER */
