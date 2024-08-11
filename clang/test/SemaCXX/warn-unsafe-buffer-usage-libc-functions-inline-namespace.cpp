// RUN: %clang_cc1 -std=c++20 -Wno-all -Wunsafe-buffer-usage \
// RUN:            -verify %s

namespace std {
  inline namespace __1 {
  template< class InputIt, class OutputIt >
  OutputIt copy( InputIt first, InputIt last,
		 OutputIt d_first );

  struct iterator{};
  template<typename T>
  struct span {
    T * ptr;
    T * data();
    unsigned size_bytes();
    unsigned size();
    iterator begin() const noexcept;
    iterator end() const noexcept;
  };

  template<typename T>
  struct basic_string {
    T* p;
    T *c_str();
    T *data();
    unsigned size_bytes();
  };

  typedef basic_string<char> string;
  typedef basic_string<wchar_t> wstring;

  // C function under std:
  void memcpy();
  void strcpy();
  int snprintf( char* buffer, unsigned buf_size, const char* format, ... );
  }
}

void f(char * p, char * q, std::span<char> s) {
  std::memcpy();              // expected-warning{{function memcpy introduces unsafe buffer access}}
  std::strcpy();              // expected-warning{{function strcpy introduces unsafe buffer access}}
  std::__1::memcpy();              // expected-warning{{function memcpy introduces unsafe buffer access}}
  std::__1::strcpy();              // expected-warning{{function strcpy introduces unsafe buffer access}}

  /* Test printfs */
  std::snprintf(s.data(), s.size(), "%s%d", "hello", *p); // expected-warning{{function snprintf introduces unsafe buffer access}} expected-note{{use 'std::string::c_str' as pointer to guarantee null-termination}}
  std::__1::snprintf(s.data(), s.size(), "%s%d", "hello", *p); // expected-warning{{function snprintf introduces unsafe buffer access}} expected-note{{use 'std::string::c_str' as pointer to guarantee null-termination}}
  std::snprintf(s.data(), s.size_bytes(), "%s%d", "hello", *p); // no warn
  std::__1::snprintf(s.data(), s.size_bytes(), "%s%d", "hello", *p); // no warn
}

void v(std::string s1) {
  std::snprintf(s1.data(), s1.size_bytes(), "%s%d", s1.c_str(), 0); // no warn
  std::__1::snprintf(s1.data(), s1.size_bytes(), "%s%d", s1.c_str(), 0); // no warn
}

void g(char *begin, char *end, char *p, std::span<char> s) {
  std::copy(begin, end, p); // no warn
  std::copy(s.begin(), s.end(), s.begin()); // no warn
}
