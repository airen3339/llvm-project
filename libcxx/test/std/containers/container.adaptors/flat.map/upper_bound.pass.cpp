//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <flat_map>

//       iterator upper_bound(const key_type& k);
// const_iterator upper_bound(const key_type& k) const;

#include <cassert>
#include <deque>
#include <flat_map>
#include <functional>
#include <utility>

#include "test_macros.h"
#include "min_allocator.h"

int main(int, char**)
{
  {
    using M = std::flat_map<int, char>;
    M m = {{1,'a'}, {2,'b'}, {4,'d'}, {5,'e'}, {8,'h'}};
    ASSERT_SAME_TYPE(decltype(m.upper_bound(0)), M::iterator);
    ASSERT_SAME_TYPE(decltype(std::as_const(m).upper_bound(0)), M::const_iterator);
    assert(m.upper_bound(0) == m.begin());
    assert(m.upper_bound(1) == m.begin() + 1);
    assert(m.upper_bound(2) == m.begin() + 2);
    assert(m.upper_bound(3) == m.begin() + 2);
    assert(m.upper_bound(4) == m.begin() + 3);
    assert(m.upper_bound(5) == m.begin() + 4);
    assert(m.upper_bound(6) == m.begin() + 4);
    assert(std::as_const(m).upper_bound(7) == m.begin() + 4);
    assert(std::as_const(m).upper_bound(8) == m.end());
    assert(std::as_const(m).upper_bound(9) == m.end());
  }
  {
    using M = std::flat_map<int, char, std::greater<int>, std::deque<int, min_allocator<int>>, std::deque<char, min_allocator<char>>>;
    M m = {{1,'a'}, {2,'b'}, {4,'d'}, {5,'e'}, {8,'h'}};
    ASSERT_SAME_TYPE(decltype(m.upper_bound(0)), M::iterator);
    ASSERT_SAME_TYPE(decltype(std::as_const(m).upper_bound(0)), M::const_iterator);
    assert(m.upper_bound(0) == m.end());
    assert(m.upper_bound(1) == m.end());
    assert(m.upper_bound(2) == m.begin() + 4);
    assert(m.upper_bound(3) == m.begin() + 3);
    assert(m.upper_bound(4) == m.begin() + 3);
    assert(m.upper_bound(5) == m.begin() + 2);
    assert(m.upper_bound(6) == m.begin() + 1);
    assert(m.upper_bound(7) == m.begin() + 1);
    assert(std::as_const(m).upper_bound(8) == m.begin() + 1);
    assert(std::as_const(m).upper_bound(9) == m.begin());
  }
  {
    using M = std::flat_map<bool, bool>;
    M m = {{true,false}, {false,true}};
    ASSERT_SAME_TYPE(decltype(m.upper_bound(0)), M::iterator);
    ASSERT_SAME_TYPE(decltype(std::as_const(m).upper_bound(0)), M::const_iterator);
    assert(m.upper_bound(true) == m.end());
    assert(m.upper_bound(false) == m.begin() + 1);
    m = {{true,true}};
    assert(m.upper_bound(true) == m.end());
    assert(m.upper_bound(false) == m.begin());
    m = {{false,false}};
    assert(std::as_const(m).upper_bound(true) == m.end());
    assert(std::as_const(m).upper_bound(false) == m.end());
    m.clear();
    assert(m.upper_bound(true) == m.end());
    assert(m.upper_bound(false) == m.end());
  }
  return 0;
}
