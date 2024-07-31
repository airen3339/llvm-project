//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <flat_map>

//       iterator begin();
// const_iterator begin() const;
//       iterator end();
// const_iterator end()   const;
//
// const_iterator         cbegin()  const;
// const_iterator         cend()    const;

#include <cassert>
#include <cstddef>
#include <deque>
#include <flat_map>
#include <functional>
#include <string>

#include "test_macros.h"

int main(int, char**) {
  {
    using M = std::flat_map<int, char, std::less<int>, std::deque<int>, std::deque<char>>;
    M m     = {{1, 'a'}, {2, 'b'}, {3, 'c'}, {4, 'd'}};
    ASSERT_SAME_TYPE(decltype(m.begin()), M::iterator);
    ASSERT_SAME_TYPE(decltype(m.cbegin()), M::const_iterator);
    ASSERT_SAME_TYPE(decltype(m.end()), M::iterator);
    ASSERT_SAME_TYPE(decltype(m.cend()), M::const_iterator);
    assert(m.size() == 4);
    assert(std::distance(m.begin(), m.end()) == 4);
    assert(std::distance(m.cbegin(), m.cend()) == 4);
    M::iterator i;                      // default-construct
    i                   = m.begin();    // move-assignment
    M::const_iterator k = i;            // converting constructor
    assert(i == k);                     // comparison
    for (int j = 1; j <= 4; ++j, ++i) { // pre-increment
      assert(i->first == j);            // operator->
      assert(i->second == 'a' + j - 1);
    }
    assert(i == m.end());
    for (int j = 4; j >= 1; --j) {
      --i; // pre-decrement
      assert((*i).first == j);
      assert((*i).second == 'a' + j - 1);
    }
    assert(i == m.begin());
  }
// std::string is not a sequence container
#if 0
  {
    using M = std::flat_map<short, char, std::less<>, std::deque<short>, std::string>;
    const M m = {{1,'a'}, {2,'b'}, {3,'c'}, {4,'d'}};
    ASSERT_SAME_TYPE(decltype(m.begin()), M::const_iterator);
    ASSERT_SAME_TYPE(decltype(m.cbegin()), M::const_iterator);
    ASSERT_SAME_TYPE(decltype(m.end()), M::const_iterator);
    ASSERT_SAME_TYPE(decltype(m.cend()), M::const_iterator);
    assert(m.size() == 4);
    assert(std::distance(m.begin(), m.end()) == 4);
    assert(std::distance(m.cbegin(), m.cend()) == 4);
    M::const_iterator i;  // default-construct 
    i = m.begin();  // move-assignment
    for (int j = 1; j <= 4; ++j, ++i) {  // pre-increment
      assert(i->first == j);
      assert(i->second == 'a' + j - 1);
    }
    assert(i == m.end());
    for (int j = 4; j >= 1; --j) {
      --i;  // pre-decrement
      assert((*i).first == j);
      assert((*i).second == 'a' + j - 1);
    }
    assert(i == m.begin());
  }
#endif
  {
    // N3644 testing
    using C = std::flat_map<int, char>;
    C::iterator ii1{}, ii2{};
    C::iterator ii4 = ii1;
    C::const_iterator cii{};
    assert(ii1 == ii2);
    assert(ii1 == ii4);
    assert(!(ii1 != ii2));

    assert((ii1 == cii));
    assert((cii == ii1));
    assert(!(ii1 != cii));
    assert(!(cii != ii1));
  }
  return 0;
}
