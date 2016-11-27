// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "include/base/cef_string16.h"

#if defined(OS_POSIX)
#if defined(WCHAR_T_IS_UTF16)

#error This file should not be used on 2-byte wchar_t systems
// If this winds up being needed on 2-byte wchar_t systems, either the
// definitions below can be used, or the host system's wide character
// functions like wmemcmp can be wrapped.

#elif defined(WCHAR_T_IS_UTF32)

#include <cstring>
#include <ostream>

#include "include/internal/cef_string_types.h"

namespace base {

int c16memcmp(const char16* s1, const char16* s2, size_t n) {
  // We cannot call memcmp because that changes the semantics.
  while (n-- > 0) {
    if (*s1 != *s2) {
      // We cannot use (*s1 - *s2) because char16 is unsigned.
      return ((*s1 < *s2) ? -1 : 1);
    }
    ++s1;
    ++s2;
  }
  return 0;
}

size_t c16len(const char16* s) {
  const char16 *s_orig = s;
  while (*s) {
    ++s;
  }
  return s - s_orig;
}

const char16* c16memchr(const char16* s, char16 c, size_t n) {
  while (n-- > 0) {
    if (*s == c) {
      return s;
    }
    ++s;
  }
  return 0;
}

char16* c16memmove(char16* s1, const char16* s2, size_t n) {
  return static_cast<char16*>(memmove(s1, s2, n * sizeof(char16)));
}

char16* c16memcpy(char16* s1, const char16* s2, size_t n) {
  return static_cast<char16*>(memcpy(s1, s2, n * sizeof(char16)));
}

char16* c16memset(char16* s, char16 c, size_t n) {
  char16 *s_orig = s;
  while (n-- > 0) {
    *s = c;
    ++s;
  }
  return s_orig;
}

std::ostream& operator<<(std::ostream& out, const string16& str) {
  cef_string_utf8_t cef_str = {0};
  cef_string_utf16_to_utf8(str.c_str(), str.size(), &cef_str);
  out << cef_str.str;
  cef_string_utf8_clear(&cef_str);
  return out;
}

void PrintTo(const string16& str, std::ostream* out) {
  *out << str;
}

}  // namespace base

template class std::basic_string<base::char16, base::string16_char_traits>;

#endif  // WCHAR_T_IS_UTF32
#endif  // OS_POSIX
