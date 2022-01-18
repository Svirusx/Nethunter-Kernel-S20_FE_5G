#ifndef _KDP_H
#define _KDP_H

#ifndef __ASSEMBLY__
#ifndef LINKER_SCRIPT
#include <linux/rkp.h>

/***************** KDP_CRED *****************/
#define CRED_JAR_RO		"cred_jar_ro"
#define TSEC_JAR		"tsec_jar"
#define VFSMNT_JAR		"vfsmnt_cache"

#define rocred_uc_read(x) atomic_read(x->use_cnt)
#define rocred_uc_inc(x)  atomic_inc(x->use_cnt)
#define rocred_uc_dec_and_test(x) atomic_dec_and_test(x->use_cnt)
#define rocred_uc_inc_not_zero(x) atomic_inc_not_zero(x->use_cnt)
#define rocred_uc_set(x,v) atomic_set(x->use_cnt,v)

extern int rkp_cred_enable;
extern char __rkp_ro_start[], __rkp_ro_end[];
extern struct cred init_cred;
extern struct task_security_struct init_sec;
extern int security_integrity_current(void);

/* uH KDP Command ID */
enum __KDP_CMD_ID{
	RKP_KDP_X40 = 0x40,
	RKP_KDP_X41 = 0x41,
	RKP_KDP_X42 = 0x42,
	RKP_KDP_X43 = 0x43,
	RKP_KDP_X44 = 0x44,
	RKP_KDP_X45 = 0x45,
	RKP_KDP_X46 = 0x46,
	RKP_KDP_X47 = 0x47,
	RKP_KDP_X48 = 0x48,
	RKP_KDP_X49 = 0x49,
	RKP_KDP_X4A = 0x4A,
	RKP_KDP_X4B = 0x4B,
	RKP_KDP_X4C = 0x4C,
	RKP_KDP_X4D = 0x4D,
	RKP_KDP_X4E = 0x4E,
	RKP_KDP_X4F = 0x4F,
	RKP_KDP_X50 = 0x50,
	RKP_KDP_X51 = 0x51,
	RKP_KDP_X52 = 0x52,
	RKP_KDP_X53 = 0x53,
	RKP_KDP_X54 = 0x54,
	RKP_KDP_X55 = 0x55,
	RKP_KDP_X56 = 0x56,
	RKP_KDP_X60 = 0x60,
};

typedef struct kdp_init_struct {
	u32 credSize;
	u32 sp_size;
	u32 pgd_mm;
	u32 uid_cred;
	u32 euid_cred;
	u32 gid_cred;
	u32 egid_cred;
	u32 bp_pgd_cred;
	u32 bp_task_cred;
	u32 type_cred;
	u32 security_cred;
	u32 usage_cred;
	u32 cred_task;
	u32 mm_task;
	u32 pid_task;
	u32 rp_task;
	u32 comm_task;
	u32 bp_cred_secptr;
	u32 task_threadinfo;
	u64 verifiedbootstate;
	struct {
		u64 selinux_enforcing_va;
		u64 ss_initialized_va;
	}selinux;
} kdp_init_t;

/*Check whether the address belong to Cred Area*/
static inline u8 rkp_ro_page(unsigned long addr)
{
	if(!rkp_cred_enable)
		return (u8)0;
	if((addr == ((unsigned long)&init_cred)) || 
		(addr == ((unsigned long)&init_sec)))
		return (u8)1;
	else
		return rkp_is_pg_protected(addr);
}


/***************** KDP_NS *****************/
#ifdef CONFIG_KDP_NS
typedef struct ns_param {
	u32 ns_buff_size;
	u32 ns_size;
	u32 bp_offset;
	u32 sb_offset;
	u32 flag_offset;
	u32 data_offset;
}ns_param_t;

#define rkp_ns_fill_params(nsparam,buff_size,size,bp,sb,flag,data)	\
do {						\
	nsparam.ns_buff_size = (u64)buff_size;		\
	nsparam.ns_size  = (u64)size;		\
	nsparam.bp_offset = (u64)bp;		\
	nsparam.sb_offset = (u64)sb;		\
	nsparam.flag_offset = (u64)flag;		\
	nsparam.data_offset = (u64)data;		\
} while(0)
#endif


/***************** KDP_DMAP *****************/
#ifdef CONFIG_KDP_DMAP
static inline void dmap_prot(u64 addr,u64 order,u64 val)
{
	if(rkp_cred_enable)
		uh_call(UH_APP_RKP, RKP_KDP_X4A, order, val, 0, 0);
}
#endif

#endif // LINKER_SCRIPT
#endif //__ASSEMBLY__
#endif //_KDP_H
