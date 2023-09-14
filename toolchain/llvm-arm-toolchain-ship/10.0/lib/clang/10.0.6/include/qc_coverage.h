//----=== qc_coverage.h - Declarations and wrappers for code coverage ----===//
//
//
// (c) 2017 Qualcomm Innovation Center, Inc. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
// This file declares prototypes of code coverage related functions for
// collecting and ontputting profiling results.
//
//===----------------------------------------------------------------------===//

#ifndef __QC_COVERAGE_H
#define __QC_COVERAGE_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif
// The declarations below are from <compiler-rt>/lib/profile/InstrProfiling.h

/*!
 * Get required size for profile buffer.
 */
uint64_t __llvm_profile_get_size_for_buffer(void);

/*!
 * Write instrumentation data to the given buffer.
 *
 * \pre \c Buffer is the start of a buffer at least as big as \a
 * __llvm_profile_get_size_for_buffer(). Buffer is realloc'ed in
 * case size has been grown.
 */
int __llvm_profile_write_buffer(char *Buffer);

/*!
 * Write instrumentation data to file
 * returns 0 for success.
 */
static int __qc_llvm_profile_write_file(const char *FileName) {
  uint64_t prof_size = __llvm_profile_get_size_for_buffer();
  char *prof_buf = (char *)malloc(prof_size);
  if (!prof_buf)
    return -1;
  if (__llvm_profile_write_buffer(prof_buf))
    return -2;
  FILE *prof_f = fopen(FileName, "wb");
  if (!prof_f)
    return -3;
  fwrite(prof_buf, 1, prof_size, prof_f);
  fclose(prof_f);
  return 0;
}

#if defined(__cplusplus)
}
#endif

#endif /*  QC_COVERAGE_H */
