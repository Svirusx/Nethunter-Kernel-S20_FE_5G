//===---- arm_memcpy_bias.h - Prototype declarations of optimized memcpy --===//
//
//
// (c) 2013 Qualcomm Innovation Center, Inc. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
// This file declares prototypes of optimized memcpy for special cases,
// where enabling such optimizations requires changing application source code
// from calling memcpy to calling __builtin_neon_memcpy_X as needed.
// This optimization uses NEON vector load and vector store intructions,
// NEON support is required.
//
//===----------------------------------------------------------------------===//

#ifndef __ARM_MEMCPY_BIAS_H
#define __ARM_MEMCPY_BIAS_H

#ifndef __ARM_NEON__
#error "NEON support not enabled"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void *__builtin_neon_memcpy_1024(void *, const void *, size_t);

#ifdef __cplusplus
}
#endif

#endif
