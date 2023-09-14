//----=== qc_binary_log.h - Declarations and wrappers for code coverage ---===//
//
//
// (c) 2019 Qualcomm Innovation Center, Inc. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
// This file defines function to wrap the contents of a non-POD type in a
// struct.
//
//===----------------------------------------------------------------------===//

#ifndef __QC_BINARY_LOG_H
#define __QC_BINARY_LOG_H

template <class T>
struct __binary_log_struct { T *x; };

template <class T>
__binary_log_struct<T> __binary_log_record(T *x) {
  return __binary_log_struct<T> {x};
}

#endif /*  QC_BINARY_LOG_H */
