//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP_SRC_INCLUDE_FROM_CHARS_FLOATING_POINT_H
#define _LIBCPP_SRC_INCLUDE_FROM_CHARS_FLOATING_POINT_H

// This header is in the shared LLVM-libc header library.
#include "shared/str_to_float.h"

#include <__assert>
#include <__config>
#include <cctype>
#include <charconv>
#include <concepts>
#include <limits>
#include <type_traits>

// Included for the _Floating_type_traits class
#include "to_chars_floating_point.h"

_LIBCPP_BEGIN_NAMESPACE_STD

// Parses an infinity string.
// Valid strings are case insentitive and contain INF or INFINITY.
//
// - __first is the first argument to std::from_chars. When the string is invalid
//   this value is returned as ptr in the result.
// - __last is the last argument of std::from_chars.
// - __value is the value argument of std::from_chars,
// - __ptr is the current position is the input string. This is points beyond
//   the initial I character.
// - __negative whether a valid string represents -inf or +inf.
template <floating_point _Tp>
from_chars_result __from_chars_floating_point_inf(
    const char* const __first, const char* __last, _Tp& __value, const char* __ptr, bool __negative) {
  if (__last - __ptr < 2) [[unlikely]]
    return {__first, errc::invalid_argument};

  if (std::tolower(__ptr[0]) != 'n' || std::tolower(__ptr[1]) != 'f') [[unlikely]]
    return {__first, errc::invalid_argument};

  __ptr += 2;

  // At this point the result is valid and contains INF.
  // When the remaining part contains INITY this will be consumed. Otherwise
  // only INF is consumed. For example INFINITZ will consume INF and ignore
  // INITZ.

  if (__last - __ptr >= 5              //
      && std::tolower(__ptr[0]) == 'i' //
      && std::tolower(__ptr[1]) == 'n' //
      && std::tolower(__ptr[2]) == 'i' //
      && std::tolower(__ptr[3]) == 't' //
      && std::tolower(__ptr[4]) == 'y')
    __ptr += 5;

  if constexpr (numeric_limits<_Tp>::has_infinity) {
    if (__negative)
      __value = -std::numeric_limits<_Tp>::infinity();
    else
      __value = std::numeric_limits<_Tp>::infinity();

    return {__ptr, std::errc{}};
  } else {
    return {__ptr, errc::result_out_of_range};
  }
}

// Parses an infinita string.
// Valid strings are case insentitive and contain INF or INFINITY.
//
// - __first is the first argument to std::from_chars. When the string is invalid
//   this value is returned as ptr in the result.
// - __last is the last argument of std::from_chars.
// - __value is the value argument of std::from_chars,
// - __ptr is the current position is the input string. This is points beyond
//   the initial N character.
// - __negative whether a valid string represents -nan or +nan.
template <floating_point _Tp>
from_chars_result __from_chars_floating_point_nan(
    const char* const __first, const char* __last, _Tp& __value, const char* __ptr, bool __negative) {
  if (__last - __ptr < 2) [[unlikely]]
    return {__first, errc::invalid_argument};

  if (std::tolower(__ptr[0]) != 'a' || std::tolower(__ptr[1]) != 'n') [[unlikely]]
    return {__first, errc::invalid_argument};

  __ptr += 2;

  // At this point the result is valid and contains NAN. When the remaining
  // part contains ( n-char-sequence_opt ) this will be consumed. Otherwise
  // only NAN is consumed. For example NAN(abcd will consume NAN and ignore
  // (abcd.
  if (__last - __ptr >= 2 && __ptr[0] == '(') {
    size_t __offset = 1;
    do {
      if (__ptr[__offset] == ')') {
        __ptr += __offset + 1;
        break;
      }
      if (__ptr[__offset] != '_' && !std::isalnum(__ptr[__offset]))
        break;
      ++__offset;
    } while (__ptr + __offset != __last);
  }

  if (__negative)
    __value = -std::numeric_limits<_Tp>::quiet_NaN();
  else
    __value = std::numeric_limits<_Tp>::quiet_NaN();

  return {__ptr, std::errc{}};
}

template <floating_point _Tp>
from_chars_result __from_chars_floating_point_decimal(
    const char* const __first,
    const char* __last,
    _Tp& __value,
    chars_format __fmt,
    const char* __ptr,
    bool __negative) {
  using _Traits    = _Floating_type_traits<_Tp>;
  using _Uint_type = typename _Traits::_Uint_type;
  ptrdiff_t length = __last - __first;
  _LIBCPP_ASSERT_INTERNAL(length > 0, "");

  const char* src = __ptr; // rename to match the libc code copied for this section.

  _Uint_type mantissa            = 0;
  int exponent                   = 0;
  bool truncated                 = false;
  bool seen_digit                = false;
  bool after_decimal             = false;
  size_t index                   = 0;
  const size_t BASE              = 10;
  constexpr char EXPONENT_MARKER = 'e';
  constexpr char DECIMAL_POINT   = '.';

  // The loop fills the mantissa with as many digits as it can hold
  const _Uint_type bitstype_max_div_by_base = numeric_limits<_Uint_type>::max() / BASE;
  while (index < static_cast<size_t>(length)) {
    if (LIBC_NAMESPACE::internal::isdigit(src[index])) {
      uint32_t digit = src[index] - '0';
      seen_digit     = true;

      if (mantissa < bitstype_max_div_by_base) {
        mantissa = (mantissa * BASE) + digit;
        if (after_decimal) {
          --exponent;
        }
      } else {
        if (digit > 0)
          truncated = true;
        if (!after_decimal)
          ++exponent;
      }

      ++index;
      continue;
    }
    if (src[index] == DECIMAL_POINT) {
      if (after_decimal) {
        break; // this means that src[index] points to a second decimal point, ending the number.
      }
      after_decimal = true;
      ++index;
      continue;
    }
    // The character is neither a digit nor a decimal point.
    break;
  }

  if (!seen_digit)
    return {src + index, {}};

  if (index < static_cast<size_t>(length) && LIBC_NAMESPACE::internal::tolower(src[index]) == EXPONENT_MARKER) {
    bool has_sign = false;
    if (index + 1 < static_cast<size_t>(length) && (src[index + 1] == '+' || src[index + 1] == '-')) {
      has_sign = true;
    }
    if (index + 1 + static_cast<size_t>(has_sign) < static_cast<size_t>(length) &&
        LIBC_NAMESPACE::internal::isdigit(src[index + 1 + static_cast<size_t>(has_sign)])) {
      ++index;
      auto result = LIBC_NAMESPACE::internal::strtointeger<int32_t>(src + index, 10);
      // if (result.has_error())
      //   output.error = result.error;
      int32_t add_to_exponent = result.value;
      index += result.parsed_len;

      // Here we do this operation as int64 to avoid overflow.
      int64_t temp_exponent = static_cast<int64_t>(exponent) + static_cast<int64_t>(add_to_exponent);

      // If the result is in the valid range, then we use it. The valid range is
      // also within the int32 range, so this prevents overflow issues.
      if (temp_exponent > LIBC_NAMESPACE::fputil::FPBits<_Tp>::MAX_BIASED_EXPONENT) {
        exponent = LIBC_NAMESPACE::fputil::FPBits<_Tp>::MAX_BIASED_EXPONENT;
      } else if (temp_exponent < -LIBC_NAMESPACE::fputil::FPBits<_Tp>::MAX_BIASED_EXPONENT) {
        exponent = -LIBC_NAMESPACE::fputil::FPBits<_Tp>::MAX_BIASED_EXPONENT;
      } else {
        exponent = static_cast<int32_t>(temp_exponent);
      }
    }
  }

  LIBC_NAMESPACE::internal::ExpandedFloat<_Tp> expanded_float = {0, 0};
  if (mantissa != 0) {
    auto temp = LIBC_NAMESPACE::shared::decimal_exp_to_float<_Tp>(
        {mantissa, exponent}, truncated, LIBC_NAMESPACE::internal::RoundDirection::Nearest, src, length);
    expanded_float = temp.num;
    // Note: there's also an error value in temp.error. I'm not doing that error handling right now though.
  }

  auto result = LIBC_NAMESPACE::fputil::FPBits<_Tp>();
  result.set_mantissa(expanded_float.mantissa);
  result.set_biased_exponent(expanded_float.exponent);
  if (__negative)
    __value = -result.get_val();
  else
    __value = result.get_val();
  return {src + index, {}};
}

template <floating_point _Tp>
from_chars_result
__from_chars_floating_point(const char* const __first, const char* __last, _Tp& __value, chars_format __fmt) {
  if (__first == __last) [[unlikely]]
    return {__first, errc::invalid_argument};

  const char* __ptr = __first;

  // skip whitespace
  while (std::isspace(*__ptr)) {
    ++__ptr;
    if (__ptr == __last) [[unlikely]]
      return {__first, errc::invalid_argument}; // is this valid??
  }

  bool __negative = *__ptr == '-';
  if (__negative) {
    ++__ptr;
    if (__ptr == __last) [[unlikely]]
      return {__first, errc::invalid_argument};
  }

  if (!std::isdigit(*__ptr)) {
    // TODO Evaluate the other implementations
    // [charconv.from.chars]/6.2
    //   if fmt has chars_format::scientific set but not chars_format::fixed,
    //   the otherwise optional exponent part shall appear;
    // Since INF/NAN do not have an exponent this value is not valid.
    // See LWG3456
    if (__fmt == chars_format::scientific)
      return {__first, errc::invalid_argument};

    switch (std::tolower(*__ptr)) {
    case 'i':
      return __from_chars_floating_point_inf(__first, __last, __value, __ptr + 1, __negative);
    case 'n':
      if constexpr (numeric_limits<_Tp>::has_quiet_NaN)
        return __from_chars_floating_point_nan(__first, __last, __value, __ptr + 1, __negative);
      [[fallthrough]];
    default:
      return {__first, errc::invalid_argument};
    }
  }

#if 1
  _LIBCPP_ASSERT_INTERNAL(__fmt == std::chars_format::general, "");
#else
  if (__fmt == chars_format::hex)
    return std::__from_chars_floating_point_hex(__first, __last, __value);
#endif

  return std::__from_chars_floating_point_decimal(__first, __last, __value, __fmt, __ptr, __negative);
}

_LIBCPP_END_NAMESPACE_STD

#endif //_LIBCPP_SRC_INCLUDE_FROM_CHARS_FLOATING_POINT_H
