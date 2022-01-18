#ifndef LINUX_SEC_SUMMARY_COREINFO_H
#define LINUX_SEC_SUMMARY_COREINFO_H

#include <linux/linkage.h>
#include <linux/elfcore.h>
#include <linux/elf.h>

#define SUMMARY_CORE_NOTE_NAME			"CORE"
#define SUMMARY_CORE_NOTE_HEAD_BYTES		ALIGN(sizeof(struct elf_note), 4)

#define SUMMARY_CORE_NOTE_NAME_BYTES		ALIGN(sizeof(SUMMARY_CORE_NOTE_NAME), 4)
#define SUMMARY_CORE_NOTE_DESC_BYTES		ALIGN(sizeof(struct elf_prstatus), 4)

#define SUMMARY_CORE_NOTE_BYTES			((SUMMARY_CORE_NOTE_HEAD_BYTES * 2) + SUMMARY_CORE_NOTE_NAME_BYTES + SUMMARY_CORE_NOTE_DESC_BYTES)

#define SUMMARY_COREINFO_BYTES			PAGE_SIZE
#define SUMMARY_COREINFO_NOTE_NAME		"SUMMARY_COREINFO"
#define SUMMARY_COREINFO_NOTE_NAME_BYTES	ALIGN(sizeof(SUMMARY_COREINFO_NOTE_NAME), 4)
#define SUMMARY_COREINFO_NOTE_SIZE		((SUMMARY_CORE_NOTE_HEAD_BYTES * 2) + SUMMARY_COREINFO_NOTE_NAME_BYTES + SUMMARY_COREINFO_BYTES)

#define SUMMARY_COREINFO_SYMBOL(name)		summary_coreinfo_append_str("SYMBOL(%s)=0x%llx\n", #name, (unsigned long long)&name)
#define SUMMARY_COREINFO_SYMBOL_ARRAY(name)	summary_coreinfo_append_str("SYMBOL(%s)=0x%llx\n", #name, (unsigned long long)name)
#define SUMMARY_COREINFO_SIZE(name)		summary_coreinfo_append_str("SIZE(%s)=%zu\n", #name, sizeof(name))
#define SUMMARY_COREINFO_STRUCT_SIZE(name)	summary_coreinfo_append_str("SIZE(%s)=%zu\n", #name, sizeof(struct name))
#define SUMMARY_COREINFO_OFFSET(name, field)	summary_coreinfo_append_str("OFFSET(%s.%s)=%zu\n", #name, #field, (size_t)offsetof(struct name, field))
#define SUMMARY_COREINFO_LENGTH(name, value)	summary_coreinfo_append_str("LENGTH(%s)=%llu\n", #name, (unsigned long long)value)
#define SUMMARY_COREINFO_NUMBER(name)		summary_coreinfo_append_str("NUMBER(%s)=%lld\n", #name, (long long)name)

void summary_coreinfo_append_str(const char *fmt, ...);

#ifdef CONFIG_SEC_DEBUG_MODULE_INFO
#ifdef CONFIG_MODULES_TREE_LOOKUP
extern void sec_debug_coreinfo_module(void);
#endif
#endif

#endif /* LINUX_SEC_SUMMARY_COREINFO_H */
