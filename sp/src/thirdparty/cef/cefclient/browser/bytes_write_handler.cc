// Copyright (c) 2014 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/browser/bytes_write_handler.h"

#include <cstdio>
#include <cstdlib>

#include "include/wrapper/cef_helpers.h"

namespace client {

BytesWriteHandler::BytesWriteHandler(size_t grow)
    : grow_(grow),
      datasize_(grow),
      offset_(0) {
  DCHECK_GT(grow, 0U);
  data_ = malloc(grow);
  DCHECK(data_ != NULL);
}

BytesWriteHandler::~BytesWriteHandler() {
  if (data_)
    free(data_);
}

size_t BytesWriteHandler::Write(const void* ptr, size_t size, size_t n) {
  base::AutoLock lock_scope(lock_);
  size_t rv;
  if (offset_ + static_cast<int64>(size * n) >= datasize_ &&
      Grow(size * n) == 0) {
    rv = 0;
  } else {
    memcpy(reinterpret_cast<char*>(data_) + offset_, ptr, size * n);
    offset_ += size * n;
    rv = n;
  }

  return rv;
}

int BytesWriteHandler::Seek(int64 offset, int whence) {
  int rv = -1L;
  base::AutoLock lock_scope(lock_);
  switch (whence) {
  case SEEK_CUR:
    if (offset_ + offset > datasize_ || offset_ + offset < 0)
      break;
    offset_ += offset;
    rv = 0;
    break;
  case SEEK_END: {
    int64 offset_abs = std::abs(offset);
    if (offset_abs > datasize_)
      break;
    offset_ = datasize_ - offset_abs;
    rv = 0;
    break;
  }
  case SEEK_SET:
    if (offset > datasize_ || offset < 0)
      break;
    offset_ = offset;
    rv = 0;
    break;
  }

  return rv;
}

int64 BytesWriteHandler::Tell() {
  base::AutoLock lock_scope(lock_);
  return offset_;
}

int BytesWriteHandler::Flush() {
  return 0;
}

size_t BytesWriteHandler::Grow(size_t size) {
  base::AutoLock lock_scope(lock_);
  size_t rv;
  size_t s = (size > grow_ ? size : grow_);
  void* tmp = realloc(data_, datasize_ + s);
  DCHECK(tmp != NULL);
  if (tmp) {
    data_ = tmp;
    datasize_ += s;
    rv = datasize_;
  } else {
    rv = 0;
  }

  return rv;
}

}  // namespace client
