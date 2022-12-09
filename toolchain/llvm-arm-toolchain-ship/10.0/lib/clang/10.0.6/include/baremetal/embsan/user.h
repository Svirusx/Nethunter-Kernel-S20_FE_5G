//===-- user.h --------------------------------------------------*- C++ -*-===//
//
// (c) 2014 Qualcomm Innovation Center, Inc. All rights reserved.
//
//===----------------------------------------------------------------------===//
// Prototypes for user supplied functions.
//===----------------------------------------------------------------------===//

#ifndef CLANG_EMBSAN_USER_H
#define CLANG_EMBSAN_USER_H

#include "embsan/arch.h"

#ifdef __cplusplus
extern "C" {
#endif

enum clang_diag_kind {
  clang_unknown_kind = 0,

  // integer overflow
  clang_add_overflow_diag_kind,
  clang_sub_overflow_diag_kind,
  clang_mul_overflow_diag_kind,
  clang_negate_overflow_diag_kind,
  clang_divrem_overflow_diag_kind,

  // float overflow
  clang_float_cast_overflow_diag_kind,

  // other integer
  clang_shift_out_of_bounds_diag_kind,

  // integer subtypes (e.g. enum, bool)
  clang_load_invalid_int_subtype_value_diag_kind,

  // type mismatch
  clang_load_type_mismatch_diag_kind,
  clang_store_type_mismatch_diag_kind,
  clang_reference_binding_type_mismatch_diag_kind,
  clang_member_access_type_mismatch_diag_kind,
  clang_member_call_type_mismatch_diag_kind,
  clang_constructor_call_type_mismatch_diag_kind,
  clang_downcast_pointer_type_mismatch_diag_kind,
  clang_downcast_reference_type_mismatch_diag_kind,
  clang_upcast_type_mismatch_diag_kind,
  clang_upcast_to_virtual_base_type_mismatch_diag_kind,
  clang_function_call_type_mismatch_diag_kind,

  // array bounds
  clang_array_index_out_of_bounds_diag_kind,
  clang_vla_bound_not_positive_diag_kind,

  // other
  clang_unreachable_diag_kind,
  clang_missing_return_diag_kind,
  clang_nonnull_arg_diag_kind,
  clang_nonnull_return_diag_kind
};

//==============================================================================
// clang_begin_diag - Initiate the generation of a diagnostic message.
//
//   'kind' indicates the type of violation that has occurred. See the
//   clang_diag_kind enumeration above.
//
//   'return_address' indicates the area of code that was executing when the
//   violation was detected and possibly how it got there. It is a
//   zero-terminated array of return addresses describing the call stack,
//   starting with the return address for the initial function called to handle
//   the violation. 'clang_return_address_count' determines the depth of this
//   array.
//
//   'file' and 'line' indicate the source where the violation occurred. These
//   may be null and 0 respectively if no source location information is
//   available.
//
//   Returns a value uniquely identifying this diagnostic instance. This will be
//   passed as 'diag_instance' in subsequent calls to clang_print and
//   clang_end_diag.
//
// clang_print - Output 'str' as part of diagnostic 'diag_instance'.
//
// clang_end_diag - Complete the generation of diagnostic 'diag_instance'.
//
// clang_return_address_count - return the number of return addresses to pass to
//   clang_begin_diag. Implementation of this function is optional. The default
//   implementation returns 1. Counts from 1 to 3 are supported. Counts other
//   than 1 may not be reliable.
//
//==============================================================================
clang_uptr clang_begin_diag(
  enum clang_diag_kind kind,
  void *return_address[],
  const char* file, unsigned int line);
void clang_print(clang_uptr diag_instance, const char *str);
void clang_end_diag(clang_uptr diag_instance);
int clang_return_address_count(void);
#ifdef __cplusplus
}
#endif

#endif
