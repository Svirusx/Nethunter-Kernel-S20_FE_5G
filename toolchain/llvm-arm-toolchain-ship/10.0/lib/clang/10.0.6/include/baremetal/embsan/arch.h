//===-- arch.h --------------------------------------------------*- C++ -*-===//
//
// (c) 2014 Qualcomm Innovation Center, Inc. All rights reserved.
//
//===----------------------------------------------------------------------===//
// Architecture dependent declarations
//===----------------------------------------------------------------------===//

#ifndef CLANG_EMBSAN_ARCH_H
#define CLANG_EMBSAN_ARCH_H

#if __clang__
#else
#error Unsupported compiler
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __UINT16_TYPE__
#define __UINT16_TYPE__ unsigned __INT16_TYPE__
#endif

#ifndef __UINT32_TYPE__
#define __UINT32_TYPE__ unsigned __INT32_TYPE__
#endif

#ifndef __UINT64_TYPE__
#define __UINT64_TYPE__ unsigned __INT64_TYPE__
#endif

#ifndef __UINTPTR_TYPE__
#define __UINTPTR_TYPE__ unsigned __INTPTR_TYPE__
#endif

typedef __UINT16_TYPE__ clang_u16;
typedef __UINT32_TYPE__ clang_u32;
typedef __UINT64_TYPE__ clang_u64;
typedef __UINTPTR_TYPE__ clang_uptr;
typedef __UINTMAX_TYPE__ clang_umax;

typedef __INT16_TYPE__ clang_s16;
typedef __INT32_TYPE__ clang_s32;
typedef __INT64_TYPE__ clang_s64;
typedef __INTPTR_TYPE__ clang_sptr;
typedef __INTMAX_TYPE__ clang_smax;

#if CLANG_LONG_DOUBLE_FLTMAX
typedef long double clang_fltmax;
#else
typedef double clang_fltmax;
#endif

#if __arm__
#elif __aarch64__
#elif __x86_64__
#else
#error Unsupported architecture
#endif

#ifdef NDEBUG
  #define CLANG_ASSERTIONS_ON 0
#else
  #define CLANG_ASSERTIONS_ON 1
#endif

#define CLANG_TRAP() __builtin_trap()
#define CLANG_DEBUGTRAP() __builtin_debugtrap()
#define CLANG_UNREACHABLE() __builtin_unreachable()
#define CLANG_ABORT() __builtin_abort()

#if CLANG_ASSERTIONS_ON
#define CLANG_ASSERT(x) if (x) { TRAP(); }
#else
#define CLANG_ASSERT(x)
#endif


#ifdef __cplusplus
}
#endif

#endif
