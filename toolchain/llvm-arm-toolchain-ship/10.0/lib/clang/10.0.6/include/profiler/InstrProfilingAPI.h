/*===-- InstrProfilingAPI.h - Profiling support library API routines ------===*\
|*
|* (c) 2015 Qualcomm Innovation Center, Inc. All rights reserved.
|*
|*===----------------------------------------------------------------------===*|
|
|* This file declares API functions used by the instrumentation based profiling
|* implementation.
|*
\*===----------------------------------------------------------------------===*/

#ifndef PROFILE_INSTRPROFILING_API_H
#define PROFILE_INSTRPROFILING_API_H

#ifdef __cplusplus
extern "C" {
#endif

/* llvm_start_profile - API for starting/resuming the profiler. This function
 * initializes all counters to 0.
 * Returns 0 for success and -1 for failure.
 */
#ifdef ENABLE_INSTR_PROFILE_APIS
int llvm_start_profile();
#else
static inline int llvm_start_profile() { return 0; }
#endif

/* llvm_stop_profile - API for stopping/pausing the profiler. This function
 * dumps all counters to profile output file and resets counters.
 * - Each shared library will write to the file specified in the compile flag
 * for that library. If the same file name is used, the results will be appended.
 * - If no filename is specified in the compile flag, the default path for
 * Android is /sdcard/default.profraw
 * Returns 0 for success and -1 for failure.
 */
#ifdef ENABLE_INSTR_PROFILE_APIS
int llvm_stop_profile();
#else
static inline int llvm_stop_profile() { return 0; }
#endif

#ifdef __cplusplus
} // extern C
#endif

#endif



