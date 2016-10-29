// Copyright (c) 2014 The Chromium Embedded Framework Authors.
// Portions copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "include/base/cef_logging.h"

#if defined(OS_WIN)
#include <windows.h>
#include <algorithm>
#include <sstream>
#elif defined(OS_POSIX)
#include <errno.h>
#include <stdio.h>
#include <string.h>
#endif

#include "include/internal/cef_string_types.h"

namespace cef {
namespace logging {

namespace {

#if defined(OS_POSIX)
// From base/posix/safe_strerror.cc

#if defined(__GLIBC__) || defined(OS_NACL)
#define USE_HISTORICAL_STRERRO_R 1
#else
#define USE_HISTORICAL_STRERRO_R 0
#endif

#if USE_HISTORICAL_STRERRO_R && defined(__GNUC__)
// GCC will complain about the unused second wrap function unless we tell it
// that we meant for them to be potentially unused, which is exactly what this
// attribute is for.
#define POSSIBLY_UNUSED __attribute__((unused))
#else
#define POSSIBLY_UNUSED
#endif

#if USE_HISTORICAL_STRERRO_R
// glibc has two strerror_r functions: a historical GNU-specific one that
// returns type char *, and a POSIX.1-2001 compliant one available since 2.3.4
// that returns int. This wraps the GNU-specific one.
static void POSSIBLY_UNUSED wrap_posix_strerror_r(
    char *(*strerror_r_ptr)(int, char *, size_t),
    int err,
    char *buf,
    size_t len) {
  // GNU version.
  char *rc = (*strerror_r_ptr)(err, buf, len);
  if (rc != buf) {
    // glibc did not use buf and returned a static string instead. Copy it
    // into buf.
    buf[0] = '\0';
    strncat(buf, rc, len - 1);
  }
  // The GNU version never fails. Unknown errors get an "unknown error" message.
  // The result is always null terminated.
}
#endif  // USE_HISTORICAL_STRERRO_R

// Wrapper for strerror_r functions that implement the POSIX interface. POSIX
// does not define the behaviour for some of the edge cases, so we wrap it to
// guarantee that they are handled. This is compiled on all POSIX platforms, but
// it will only be used on Linux if the POSIX strerror_r implementation is
// being used (see below).
static void POSSIBLY_UNUSED wrap_posix_strerror_r(
    int (*strerror_r_ptr)(int, char *, size_t),
    int err,
    char *buf,
    size_t len) {
  int old_errno = errno;
  // Have to cast since otherwise we get an error if this is the GNU version
  // (but in such a scenario this function is never called). Sadly we can't use
  // C++-style casts because the appropriate one is reinterpret_cast but it's
  // considered illegal to reinterpret_cast a type to itself, so we get an
  // error in the opposite case.
  int result = (*strerror_r_ptr)(err, buf, len);
  if (result == 0) {
    // POSIX is vague about whether the string will be terminated, although
    // it indirectly implies that typically ERANGE will be returned, instead
    // of truncating the string. We play it safe by always terminating the
    // string explicitly.
    buf[len - 1] = '\0';
  } else {
    // Error. POSIX is vague about whether the return value is itself a system
    // error code or something else. On Linux currently it is -1 and errno is
    // set. On BSD-derived systems it is a system error and errno is unchanged.
    // We try and detect which case it is so as to put as much useful info as
    // we can into our message.
    int strerror_error;  // The error encountered in strerror
    int new_errno = errno;
    if (new_errno != old_errno) {
      // errno was changed, so probably the return value is just -1 or something
      // else that doesn't provide any info, and errno is the error.
      strerror_error = new_errno;
    } else {
      // Either the error from strerror_r was the same as the previous value, or
      // errno wasn't used. Assume the latter.
      strerror_error = result;
    }
    // snprintf truncates and always null-terminates.
    snprintf(buf,
             len,
             "Error %d while retrieving error %d",
             strerror_error,
             err);
  }
  errno = old_errno;
}

void safe_strerror_r(int err, char *buf, size_t len) {
  if (buf == NULL || len <= 0) {
    return;
  }
  // If using glibc (i.e., Linux), the compiler will automatically select the
  // appropriate overloaded function based on the function type of strerror_r.
  // The other one will be elided from the translation unit since both are
  // static.
  wrap_posix_strerror_r(&strerror_r, err, buf, len);
}

std::string safe_strerror(int err) {
  const int buffer_size = 256;
  char buf[buffer_size];
  safe_strerror_r(err, buf, sizeof(buf));
  return std::string(buf);
}
#endif  // defined(OS_POSIX)

}  // namespace

// MSVC doesn't like complex extern templates and DLLs.
#if !defined(COMPILER_MSVC)
// Explicit instantiations for commonly used comparisons.
template std::string* MakeCheckOpString<int, int>(
    const int&, const int&, const char* names);
template std::string* MakeCheckOpString<unsigned long, unsigned long>(
    const unsigned long&, const unsigned long&, const char* names);
template std::string* MakeCheckOpString<unsigned long, unsigned int>(
    const unsigned long&, const unsigned int&, const char* names);
template std::string* MakeCheckOpString<unsigned int, unsigned long>(
    const unsigned int&, const unsigned long&, const char* names);
template std::string* MakeCheckOpString<std::string, std::string>(
    const std::string&, const std::string&, const char* name);
#endif

#if defined(OS_WIN)
LogMessage::SaveLastError::SaveLastError() : last_error_(::GetLastError()) {
}

LogMessage::SaveLastError::~SaveLastError() {
  ::SetLastError(last_error_);
}
#endif  // defined(OS_WIN)

LogMessage::LogMessage(const char* file, int line, LogSeverity severity)
    : severity_(severity), file_(file), line_(line) {
}

LogMessage::LogMessage(const char* file, int line, std::string* result)
    : severity_(LOG_FATAL), file_(file), line_(line) {
  stream_ << "Check failed: " << *result;
  delete result;
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
                       std::string* result)
    : severity_(severity), file_(file), line_(line) {
  stream_ << "Check failed: " << *result;
  delete result;
}

LogMessage::~LogMessage() {
  stream_ << std::endl;
  std::string str_newline(stream_.str());
  cef_log(file_, line_, severity_, str_newline.c_str());
}

#if defined(OS_WIN)
// This has already been defined in the header, but defining it again as DWORD
// ensures that the type used in the header is equivalent to DWORD. If not,
// the redefinition is a compile error.
typedef DWORD SystemErrorCode;
#endif

SystemErrorCode GetLastSystemErrorCode() {
#if defined(OS_WIN)
  return ::GetLastError();
#elif defined(OS_POSIX)
  return errno;
#else
#error Not implemented
#endif
}

#if defined(OS_WIN)
std::string SystemErrorCodeToString(SystemErrorCode error_code) {
  const int error_message_buffer_size = 256;
  char msgbuf[error_message_buffer_size];
  DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
  DWORD len = FormatMessageA(flags, NULL, error_code, 0, msgbuf,
                             arraysize(msgbuf), NULL);
  std::stringstream ss;
  if (len) {
    std::string s(msgbuf);
    // Messages returned by system end with line breaks.
    s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
    ss << s << " (0x" << std::hex << error_code << ")";
  } else {
    ss << "Error (0x" << std::hex << GetLastError() <<
          ") while retrieving error. (0x" << error_code << ")";
  }
  return ss.str();
}
#elif defined(OS_POSIX)
std::string SystemErrorCodeToString(SystemErrorCode error_code) {
  return safe_strerror(error_code);
}
#else
#error Not implemented
#endif

#if defined(OS_WIN)
Win32ErrorLogMessage::Win32ErrorLogMessage(const char* file,
                                           int line,
                                           LogSeverity severity,
                                           SystemErrorCode err)
    : err_(err),
      log_message_(file, line, severity) {
}

Win32ErrorLogMessage::~Win32ErrorLogMessage() {
  stream() << ": " << SystemErrorCodeToString(err_);
}
#elif defined(OS_POSIX)
ErrnoLogMessage::ErrnoLogMessage(const char* file,
                                 int line,
                                 LogSeverity severity,
                                 SystemErrorCode err)
    : err_(err),
      log_message_(file, line, severity) {
}

ErrnoLogMessage::~ErrnoLogMessage() {
  stream() << ": " << SystemErrorCodeToString(err_);
}
#endif  // OS_WIN

std::ostream& operator<<(std::ostream& out, const wchar_t* wstr) {
  cef_string_utf8_t str = {0};
  std::wstring tmp_str(wstr);
  cef_string_wide_to_utf8(wstr, tmp_str.size(), &str);
  out << str.str;
  cef_string_utf8_clear(&str);
  return out;
}

}  // namespace logging
}  // namespace cef
