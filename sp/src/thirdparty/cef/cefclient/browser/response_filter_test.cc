// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/browser/response_filter_test.h"

#include <algorithm>
#include <sstream>
#include <string>

#include "include/base/cef_logging.h"
#include "include/cef_command_line.h"
#include "cefclient/common/client_switches.h"

namespace client {
namespace response_filter_test {

namespace {

const char kTestUrl[] = "http://tests/response_filter";
const char kFindString[] = "REPLACE_THIS_STRING";
const char kReplaceString[] = "This is the replaced string!";

// Helper for passing params to Write().
#define WRITE_PARAMS data_out_ptr, data_out_size, data_out_written

// Filter the contents of response_filter.html by replacing all instances of
// |kFindString| with |kReplaceString|. Pass the `--enable-filter-testing`
// command-line flag (which shrinks the buffer size to 32 bytes) to better test
// the logic in this implementation.
class FindReplaceResponseFilter : public CefResponseFilter {
 public:
  FindReplaceResponseFilter()
      : find_match_offset_(0U),
        replace_overflow_size_(0U),
        replace_count_(0U) {
  }

  bool InitFilter() OVERRIDE {
    const size_t find_size = sizeof(kFindString) - 1;
    const size_t replace_size = sizeof(kReplaceString) - 1;

    // Determine a reasonable amount of space for find/replace overflow. For
    // example, the amount of space required if the search string is
    // found/replaced 10 times (plus space for the count).
    if (replace_size > find_size)
      replace_overflow_size_ = (replace_size - find_size + 3) * 10;

    return true;
  }

  FilterStatus Filter(void* data_in,
                      size_t data_in_size,
                      size_t& data_in_read,
                      void* data_out,
                      size_t data_out_size,
                      size_t& data_out_written) OVERRIDE {
    DCHECK((data_in_size == 0U && !data_in) || (data_in_size > 0U && data_in));
    DCHECK_EQ(data_in_read, 0U);
    DCHECK(data_out);
    DCHECK_GT(data_out_size, 0U);
    DCHECK_EQ(data_out_written, 0U);

    // All data will be read.
    data_in_read = data_in_size;

    const size_t find_size = sizeof(kFindString) - 1;

    const char* data_in_ptr = static_cast<char*>(data_in);
    char* data_out_ptr = static_cast<char*>(data_out);

    // Reset the overflow.
    std::string old_overflow;
    if (!overflow_.empty()) {
      old_overflow = overflow_;
      overflow_.clear();
    }

    const size_t likely_out_size =
        data_in_size + replace_overflow_size_ + old_overflow.size();
    if (data_out_size < likely_out_size) {
      // We'll likely need to use the overflow buffer. Size it appropriately.
      overflow_.reserve(likely_out_size - data_out_size);
    }

    if (!old_overflow.empty()) {
      // Write the overflow from last time.
      Write(old_overflow.c_str(), old_overflow.size(), WRITE_PARAMS);
    }

    // Evaluate each character in the input buffer. Track how many characters in
    // a row match kFindString. If kFindString is completely matched then write
    // kReplaceString. Otherwise, write the input characters as-is.
    for (size_t i = 0U; i < data_in_size; ++i) {
      if (data_in_ptr[i] == kFindString[find_match_offset_]) {
        // Matched the next character in the find string.
        if (++find_match_offset_ == find_size) {
          // Complete match of the find string. Write the replace string.
          std::stringstream ss;
          ss << ++replace_count_ << ". " << kReplaceString;
          const std::string& replace_str = ss.str();
          Write(replace_str.c_str(), replace_str.size(), WRITE_PARAMS);

          // Start over looking for a match.
          find_match_offset_ = 0;
        }
        continue;
      }

      // Character did not match the find string.
      if (find_match_offset_ > 0) {
        // Write the portion of the find string that has matched so far.
        Write(kFindString, find_match_offset_, WRITE_PARAMS);

        // Start over looking for a match.
        find_match_offset_ = 0;
      }

      // Write the current character.
      Write(&data_in_ptr[i], 1, WRITE_PARAMS);
    }

    // If a match is currently in-progress we need more data. Otherwise, we're
    // done.
    return find_match_offset_ > 0 ?
        RESPONSE_FILTER_NEED_MORE_DATA : RESPONSE_FILTER_DONE;
  }

 private:
  inline void Write(const char* str,
                    size_t str_size,
                    char*& data_out_ptr,
                    size_t data_out_size,
                    size_t& data_out_written) {
    // Number of bytes remaining in the output buffer.
    const size_t remaining_space = data_out_size - data_out_written;
    // Maximum number of bytes we can write into the output buffer.
    const size_t max_write = std::min(str_size, remaining_space);

    // Write the maximum portion that fits in the output buffer.
    if (max_write == 1) {
      // Small optimization for single character writes.
      *data_out_ptr = str[0];
      data_out_ptr += 1;
      data_out_written += 1;
    } else if (max_write > 1) {
      memcpy(data_out_ptr, str, max_write);
      data_out_ptr += max_write;
      data_out_written += max_write;
    }

    if (max_write < str_size) {
      // Need to write more bytes than will fit in the output buffer. Store the
      // remainder in the overflow buffer.
      overflow_ += std::string(str + max_write, str_size - max_write);
    }
  }

  // The portion of the find string that is currently matching.
  size_t find_match_offset_;

  // The likely amount of overflow.
  size_t replace_overflow_size_;

  // Overflow from the output buffer.
  std::string overflow_;

  // Number of times the the string was found/replaced.
  size_t replace_count_;

  IMPLEMENT_REFCOUNTING(FindReplaceResponseFilter);
};

// Filter that writes out all of the contents unchanged.
class PassThruResponseFilter : public CefResponseFilter {
 public:
  PassThruResponseFilter() {}

  bool InitFilter() OVERRIDE {
    return true;
  }

  FilterStatus Filter(void* data_in,
                      size_t data_in_size,
                      size_t& data_in_read,
                      void* data_out,
                      size_t data_out_size,
                      size_t& data_out_written) OVERRIDE {
    DCHECK((data_in_size == 0U && !data_in) || (data_in_size > 0U && data_in));
    DCHECK_EQ(data_in_read, 0U);
    DCHECK(data_out);
    DCHECK_GT(data_out_size, 0U);
    DCHECK_EQ(data_out_written, 0U);

    // All data will be read.
    data_in_read = data_in_size;

    // Write out the contents unchanged.
    data_out_written = std::min(data_in_read, data_out_size);
    if (data_out_written > 0)
      memcpy(data_out, data_in, data_out_written);

    return RESPONSE_FILTER_DONE;
  }

 private:
  IMPLEMENT_REFCOUNTING(PassThruResponseFilter);
};

// Returns true if |url| starts with the value specified via the `--filter-url`
// command-line flag.
bool MatchesFilterURL(const std::string& url) {
  CefRefPtr<CefCommandLine> command_line =
      CefCommandLine::GetGlobalCommandLine();
  if (command_line->HasSwitch(switches::kFilterURL)) {
    const std::string& filter_url =
        command_line->GetSwitchValue(switches::kFilterURL);
    return url.find(filter_url) == 0;
  }
  return false;
}

}  // namespace

CefRefPtr<CefResponseFilter> GetResourceResponseFilter(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefRequest> request,
    CefRefPtr<CefResponse> response) {
  // Use the find/replace filter on the test URL.
  const std::string& url = request->GetURL();
  if (url.find(kTestUrl) == 0)
    return new FindReplaceResponseFilter();

  if (MatchesFilterURL(url))
    return new PassThruResponseFilter();

  return NULL;
}

}  // namespace response_filter_test
}  // namespace client
