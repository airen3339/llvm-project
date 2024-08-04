//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// ADDITIONAL_COMPILE_FLAGS: -O0 -g

// <charconv>

// from_chars_result from_chars(const char* first, const char* last,
//                              Float& value, chars_format fmt = chars_format::general)

#include <array>
#include <charconv>
#include <cmath>
#include <limits>

#include "charconv_test_helpers.h"
#include "test_macros.h"

template <class F>
void test_infinity(std::chars_format fmt) {
  const char* s = "-InFiNiTyXXX";
  { // I
    F value                       = 0.25;
    std::from_chars_result result = std::from_chars(s + 1, s + 2, value, fmt);

    assert(result.ec == std::errc::invalid_argument);
    assert(result.ptr == s + 1);
    assert(value == F(0.25));
  }
  { // In
    F value                       = 0.25;
    std::from_chars_result result = std::from_chars(s + 1, s + 3, value, fmt);

    assert(result.ec == std::errc::invalid_argument);
    assert(result.ptr == s + 1);
    assert(value == F(0.25));
  }
  { // InF
    F value                       = 0.25;
    std::from_chars_result result = std::from_chars(s + 1, s + 4, value, fmt);

    assert(result.ec == std::errc{});
    assert(result.ptr == s + 4);
    assert(value == std::numeric_limits<F>::infinity());
  }
  { // -InF
    F value                       = 0.25;
    std::from_chars_result result = std::from_chars(s, s + 4, value, fmt);

    assert(result.ec == std::errc{});
    assert(result.ptr == s + 4);
    assert(value == -std::numeric_limits<F>::infinity());
  }
  { // InFi
    F value                       = 0.25;
    std::from_chars_result result = std::from_chars(s + 1, s + 5, value, fmt);

    assert(result.ec == std::errc{});
    assert(result.ptr == s + 4);
    assert(value == std::numeric_limits<F>::infinity());
  }
  { // -InFiN
    F value                       = 0.25;
    std::from_chars_result result = std::from_chars(s, s + 6, value, fmt);

    assert(result.ec == std::errc{});
    assert(result.ptr == s + 4);
    assert(value == -std::numeric_limits<F>::infinity());
  }
  { // InFiNi
    F value                       = 0.25;
    std::from_chars_result result = std::from_chars(s + 1, s + 7, value, fmt);

    assert(result.ec == std::errc{});
    assert(result.ptr == s + 4);
    assert(value == std::numeric_limits<F>::infinity());
  }
  { // -InFiNiT
    F value                       = 0.25;
    std::from_chars_result result = std::from_chars(s, s + 8, value, fmt);

    assert(result.ec == std::errc{});
    assert(result.ptr == s + 4);
    assert(value == -std::numeric_limits<F>::infinity());
  }
  { // InFiNiTy
    F value                       = 0.25;
    std::from_chars_result result = std::from_chars(s + 1, s + 9, value, fmt);

    assert(result.ec == std::errc{});
    assert(result.ptr == s + 9);
    assert(value == std::numeric_limits<F>::infinity());
  }
  { // -InFiNiTy
    F value                       = 0.25;
    std::from_chars_result result = std::from_chars(s, s + 9, value, fmt);

    assert(result.ec == std::errc{});
    assert(result.ptr == s + 9);
    assert(value == -std::numeric_limits<F>::infinity());
  }
  { // InFiNiTyXXX
    F value                       = 0.25;
    std::from_chars_result result = std::from_chars(s + 1, s + 12, value, fmt);

    assert(result.ec == std::errc{});
    assert(result.ptr == s + 9);
    assert(value == std::numeric_limits<F>::infinity());
  }
  { // -InFiNiTyXXX
    F value                       = 0.25;
    std::from_chars_result result = std::from_chars(s, s + 12, value, fmt);

    assert(result.ec == std::errc{});
    assert(result.ptr == s + 9);
    assert(value == -std::numeric_limits<F>::infinity());
  }
}

template <class F>
void test_nan(std::chars_format fmt) {
  {
    const char* s = "-NaN(1_A)XXX";
    { // N
      F value                       = 0.25;
      std::from_chars_result result = std::from_chars(s + 1, s + 2, value, fmt);

      assert(result.ec == std::errc::invalid_argument);
      assert(result.ptr == s + 1);
      assert(value == F(0.25));
    }
    { // Na
      F value                       = 0.25;
      std::from_chars_result result = std::from_chars(s + 1, s + 3, value, fmt);

      assert(result.ec == std::errc::invalid_argument);
      assert(result.ptr == s + 1);
      assert(value == F(0.25));
    }
    { // NaN
      F value                       = 0.25;
      std::from_chars_result result = std::from_chars(s + 1, s + 4, value, fmt);

      assert(result.ec == std::errc{});
      assert(result.ptr == s + 4);
      assert(std::isnan(value));
      assert(!std::signbit(value));
    }
    { // -NaN
      F value                       = 0.25;
      std::from_chars_result result = std::from_chars(s + 0, s + 4, value, fmt);

      assert(result.ec == std::errc{});
      assert(result.ptr == s + 4);
      assert(std::isnan(value));
      assert(std::signbit(value));
    }
    { // NaN(
      F value                       = 0.25;
      std::from_chars_result result = std::from_chars(s + 1, s + 5, value, fmt);

      assert(result.ec == std::errc{});
      assert(result.ptr == s + 4);
      assert(std::isnan(value));
      assert(!std::signbit(value));
    }
    { // -NaN(1
      F value                       = 0.25;
      std::from_chars_result result = std::from_chars(s, s + 6, value, fmt);

      assert(result.ec == std::errc{});
      assert(result.ptr == s + 4);
      assert(std::isnan(value));
      assert(std::signbit(value));
    }
    { // NaN(1_
      F value                       = 0.25;
      std::from_chars_result result = std::from_chars(s + 1, s + 7, value, fmt);

      assert(result.ec == std::errc{});
      assert(result.ptr == s + 4);
      assert(std::isnan(value));
      assert(!std::signbit(value));
    }
    { // -NaN(1_A
      F value                       = 0.25;
      std::from_chars_result result = std::from_chars(s, s + 8, value, fmt);

      assert(result.ec == std::errc{});
      assert(result.ptr == s + 4);
      assert(std::isnan(value));
      assert(std::signbit(value));
    }
    { // NaN(1_A)
      F value                       = 0.25;
      std::from_chars_result result = std::from_chars(s + 1, s + 9, value, fmt);

      assert(result.ec == std::errc{});
      assert(result.ptr == s + 9);
      assert(std::isnan(value));
      assert(!std::signbit(value));
    }
    { // -NaN(1_A)
      F value                       = 0.25;
      std::from_chars_result result = std::from_chars(s, s + 9, value, fmt);

      assert(result.ec == std::errc{});
      assert(result.ptr == s + 9);
      assert(std::isnan(value));
      assert(std::signbit(value));
    }
    { // NaN(1_A)XXX
      F value                       = 0.25;
      std::from_chars_result result = std::from_chars(s + 1, s + 12, value, fmt);

      assert(result.ec == std::errc{});
      assert(result.ptr == s + 9);
      assert(std::isnan(value));
      assert(!std::signbit(value));
    }
    { // -NaN(1_A)XXX
      F value                       = 0.25;
      std::from_chars_result result = std::from_chars(s, s + 12, value, fmt);

      assert(result.ec == std::errc{});
      assert(result.ptr == s + 9);
      assert(std::isnan(value));
      assert(std::signbit(value));
    }
  }
  {
    const char* s                 = "NaN()";
    F value                       = 0.25;
    std::from_chars_result result = std::from_chars(s, s + std::strlen(s), value, fmt);

    assert(result.ec == std::errc{});
    assert(result.ptr == s + 5);
    assert(std::isnan(value));
    assert(!std::signbit(value));
  }
  { // validates a n-char-sequences with an invalid value
    std::array s = {'N', 'a', 'N', '(', ' ', ')'};
    s[4]         = 'a';
    {
      F value                       = 0.25;
      std::from_chars_result result = std::from_chars(s.data(), s.data() + s.size(), value, fmt);

      assert(result.ec == std::errc{});
      assert(result.ptr == s.data() + s.size());
      assert(std::isnan(value));
      assert(!std::signbit(value));
    }
    for (auto c : "!@#$%^&*(-=+[]{}|\\;:'\",./<>?~` \t\v\r\n") {
      F value                       = 0.25;
      s[4]                          = c;
      std::from_chars_result result = std::from_chars(s.data(), s.data() + s.size(), value, fmt);

      assert(result.ec == std::errc{});
      assert(result.ptr == s.data() + 3);
      assert(std::isnan(value));
      assert(!std::signbit(value));
    }
  }
}

template <class F>
void test_fmt_independent(std::chars_format fmt) {
  { // first == last
    F value                       = 0.25;
    std::from_chars_result result = std::from_chars(nullptr, nullptr, value, fmt);

    assert(result.ec == std::errc::invalid_argument);
    assert(result.ptr == nullptr);
    assert(value == F(0.25));
  }
  { // only a sign
    F value                       = 0.25;
    const char* s                 = "-";
    std::from_chars_result result = std::from_chars(s, s + std::strlen(s), value, fmt);

    assert(result.ec == std::errc::invalid_argument);
    assert(result.ptr == s);
    assert(value == F(0.25));
  }
  if (fmt != std::chars_format::scientific) {
    test_infinity<F>(fmt);
    test_nan<F>(fmt);
  } else {
    { // infinity
      F value                       = 0.25;
      const char* s                 = "inf";
      std::from_chars_result result = std::from_chars(s, s + std::strlen(s), value, fmt);

      assert(result.ec == std::errc::invalid_argument);
      assert(result.ptr == s);
      assert(value == F(0.25));
    }
    { // nan
      F value                       = 0.25;
      const char* s                 = "nan";
      std::from_chars_result result = std::from_chars(s, s + std::strlen(s), value, fmt);

      assert(result.ec == std::errc::invalid_argument);
      assert(result.ptr == s);
      assert(value == F(0.25));
    }
  }
  { // start with decimal separator
    F value                       = 0.25;
    const char* s                 = ".";
    std::from_chars_result result = std::from_chars(s, s + std::strlen(s), value, fmt);

    assert(result.ec == std::errc::invalid_argument);
    assert(result.ptr == s);
    assert(value == F(0.25));
  }
  { // Invalid sign
    F value                       = 0.25;
    const char* s                 = "+0.25";
    std::from_chars_result result = std::from_chars(s, s + std::strlen(s), value, fmt);

    assert(result.ec == std::errc::invalid_argument);
    assert(result.ptr == s);
    assert(value == F(0.25));
  }
}

template <class F>
struct test_basics {
  void operator()() {
    for (auto fmt :
         {std::chars_format::scientific, std::chars_format::fixed, std::chars_format::hex, std::chars_format::general})
      test_fmt_independent<F>(fmt);
  }
};

template <class F>
struct test_general {
  void operator()() {
    std::from_chars_result r;
    F x;

    { // number followed by non-numeric valies
      const char* s = "001x";

      // the expected form of the subject sequence is a nonempty sequence of
      // decimal digits optionally containing a decimal-point character, then
      // an optional exponent part as defined in 6.4.4.3, excluding any digit
      // separators (6.4.4.2); (C23 7.24.1.5)
      r = std::from_chars(s, s + std::strlen(s), x);
      assert(r.ec == std::errc{});
      assert(r.ptr == s + 3);
      assert(x == F(1.0));
    }

    { // double deciamal point
      const char* s = "1.25.78";

      // This number is halfway between two float values.
      r = std::from_chars(s, s + std::strlen(s), x);
      assert(r.ec == std::errc{});
      assert(r.ptr == s + 4);
      assert(x == F(1.25));
    }

    { // exponenent no sign
      const char* s = "1.5e10";

      r = std::from_chars(s, s + std::strlen(s), x);
      assert(r.ec == std::errc{});
      assert(r.ptr == s + 6);
      assert(x == F(1.5e10));
    }
    { // exponenent + sign
      const char* s = "1.5e+10";

      r = std::from_chars(s, s + std::strlen(s), x);
      assert(r.ec == std::errc{});
      assert(r.ptr == s + 7);
      assert(x == F(1.5e10));
    }
    { // exponenent - sign
      const char* s = "1.5e-10";

      r = std::from_chars(s, s + std::strlen(s), x);
      assert(r.ec == std::errc{});
      assert(r.ptr == s + 7);
      assert(x == F(1.5e-10));
    }
    { // exponent double sign
      const char* s = "1.25e++12";

      r = std::from_chars(s, s + std::strlen(s), x);
      assert(r.ec == std::errc{});
      assert(r.ptr == s + 4);
      assert(x == F(1.25));
    }
    { // double exponent
      const char* s = "1.25e0e12";

      r = std::from_chars(s, s + std::strlen(s), x);
      assert(r.ec == std::errc{});
      assert(r.ptr == s + 6);
      assert(x == F(1.25));
    }
    { // This number is halfway between two float values.
      const char* s = "20040229";

      r = std::from_chars(s, s + std::strlen(s), x);
      assert(r.ec == std::errc{});
      assert(r.ptr == s + 8);
      assert(x == F(20040229));
    }
    { // Shifting mantissa exponent and no exponent
      const char* s = "123.456";

      r = std::from_chars(s, s + std::strlen(s), x);
      assert(r.ec == std::errc{});
      assert(r.ptr == s + 7);
      assert(x == F(1.23456e2));
    }
    { // Shifting mantissa exponent and an exponent
      const char* s = "123.456e3";

      r = std::from_chars(s, s + std::strlen(s), x);
      assert(r.ec == std::errc{});
      assert(r.ptr == s + 9);
      assert(x == F(1.23456e5));
    }
    { // Mantissa overflow
      {
        const char* s = "0.111111111111111111111111111111111111111111";

        r = std::from_chars(s, s + std::strlen(s), x);
        assert(r.ec == std::errc{});
        assert(r.ptr == s + std::strlen(s));
        assert(x == F(0.111111111111111111111111111111111111111111));
      }
      {
        const char* s = "111111111111.111111111111111111111111111111111111111111";

        r = std::from_chars(s, s + std::strlen(s), x);
        assert(r.ec == std::errc{});
        assert(r.ptr == s + std::strlen(s));
        assert(x == F(111111111111.111111111111111111111111111111111111111111));
      }
    }
    { // Leading whitespace
      const char* s = " \t\v\r\n0.25";

      r = std::from_chars(s, s + std::strlen(s), x);
      assert(r.ec == std::errc{});
      assert(r.ptr == s + std::strlen(s));
      assert(x == F(0.25));
    }
    { // Negative value
      const char* s = "-0.25";

      r = std::from_chars(s, s + std::strlen(s), x);
      assert(r.ec == std::errc{});
      assert(r.ptr == s + std::strlen(s));
      assert(x == F(-0.25));
    }
  }
};

int main(int, char**) {
  run<test_basics>(all_floats);
  run<test_general>(all_floats);

  return 0;
}
