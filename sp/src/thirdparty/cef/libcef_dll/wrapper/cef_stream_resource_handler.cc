// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "include/wrapper/cef_stream_resource_handler.h"

#include <algorithm>

#include "include/base/cef_bind.h"
#include "include/base/cef_logging.h"
#include "include/base/cef_macros.h"
#include "include/cef_callback.h"
#include "include/cef_request.h"
#include "include/cef_stream.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

// Class that represents a readable/writable character buffer.
class CefStreamResourceHandler::Buffer {
 public:
  Buffer()
      : size_(0),
        bytes_requested_(0),
        bytes_written_(0),
        bytes_read_(0) {
  }

  void Reset(int new_size) {
    if (size_ < new_size) {
      size_ = new_size;
      buffer_.reset(new char[size_]);
      DCHECK(buffer_);
    }
    bytes_requested_ = new_size;
    bytes_written_ = 0;
    bytes_read_ = 0;
  }

  bool IsEmpty() const {
    return (bytes_written_ == 0);
  }

  bool CanRead() const {
    return (bytes_read_ < bytes_written_);
  }

  int WriteTo(void* data_out, int bytes_to_read) {
    const int write_size =
        std::min(bytes_to_read, bytes_written_ - bytes_read_);
    if (write_size > 0) {
      memcpy(data_out, buffer_ .get() + bytes_read_, write_size);
      bytes_read_ += write_size;
    }
    return write_size;
  }

  int ReadFrom(CefRefPtr<CefStreamReader> reader) {
    // Read until the buffer is full or until Read() returns 0 to indicate no
    // more data.
    int bytes_read;
    do {
      bytes_read = static_cast<int>(
          reader->Read(buffer_.get() + bytes_written_, 1,
                       bytes_requested_ - bytes_written_));
      bytes_written_ += bytes_read;
    } while (bytes_read != 0 && bytes_written_ < bytes_requested_);

    return bytes_written_;
  }

 private:
  SCOPED_PTR(char[]) buffer_;
  int size_;
  int bytes_requested_;
  int bytes_written_;
  int bytes_read_;

  DISALLOW_COPY_AND_ASSIGN(Buffer);
};

CefStreamResourceHandler::CefStreamResourceHandler(
    const CefString& mime_type,
    CefRefPtr<CefStreamReader> stream)
    : status_code_(200),
      status_text_("OK"),
      mime_type_(mime_type),
      stream_(stream)
#if DCHECK_IS_ON()
      , buffer_owned_by_file_thread_(false)
#endif
{
  DCHECK(!mime_type_.empty());
  DCHECK(stream_.get());
  read_on_file_thread_ = stream_->MayBlock();
}

CefStreamResourceHandler::CefStreamResourceHandler(
    int status_code,
    const CefString& status_text,
    const CefString& mime_type,
    CefResponse::HeaderMap header_map,
    CefRefPtr<CefStreamReader> stream)
    : status_code_(status_code),
      status_text_(status_text),
      mime_type_(mime_type),
      header_map_(header_map),
      stream_(stream)
#if DCHECK_IS_ON()
      , buffer_owned_by_file_thread_(false)
#endif
{
  DCHECK(!mime_type_.empty());
  DCHECK(stream_.get());
  read_on_file_thread_ = stream_->MayBlock();
}

CefStreamResourceHandler::~CefStreamResourceHandler() {
}

bool CefStreamResourceHandler::ProcessRequest(CefRefPtr<CefRequest> request,
                                              CefRefPtr<CefCallback> callback) {
  callback->Continue();
  return true;
}

void CefStreamResourceHandler::GetResponseHeaders(
    CefRefPtr<CefResponse> response,
    int64& response_length,
    CefString& redirectUrl) {
  response->SetStatus(status_code_);
  response->SetStatusText(status_text_);
  response->SetMimeType(mime_type_);

  if (!header_map_.empty())
    response->SetHeaderMap(header_map_);

  response_length = -1;
}

bool CefStreamResourceHandler::ReadResponse(void* data_out,
                                            int bytes_to_read,
                                            int& bytes_read,
                                            CefRefPtr<CefCallback> callback) {
  DCHECK_GT(bytes_to_read, 0);

  if (read_on_file_thread_) {
#if DCHECK_IS_ON()
    DCHECK(!buffer_owned_by_file_thread_);
#endif
    if (buffer_ && (buffer_->CanRead() || buffer_->IsEmpty())) {
      if (buffer_->CanRead()) {
        // Provide data from the buffer.
        bytes_read = buffer_->WriteTo(data_out, bytes_to_read);
        return (bytes_read > 0);
      } else {
        // End of the steam.
        bytes_read = 0;
        return false;
      }
    } else {
      // Perform another read on the file thread.
      bytes_read = 0;
#if DCHECK_IS_ON()
      buffer_owned_by_file_thread_ = true;
#endif
      CefPostTask(TID_FILE,
          base::Bind(&CefStreamResourceHandler::ReadOnFileThread, this,
                     bytes_to_read, callback));
      return true;
    }
  } else {
    // Read until the buffer is full or until Read() returns 0 to indicate no
    // more data.
    bytes_read = 0;
    int read = 0;
    do {
      read = static_cast<int>(
          stream_->Read(static_cast<char*>(data_out) + bytes_read, 1,
                        bytes_to_read - bytes_read));
      bytes_read += read;
    } while (read != 0 && bytes_read < bytes_to_read);

    return (bytes_read > 0);
  }
}

void CefStreamResourceHandler::Cancel() {
}

void CefStreamResourceHandler::ReadOnFileThread(
    int bytes_to_read,
    CefRefPtr<CefCallback> callback) {
  CEF_REQUIRE_FILE_THREAD();
#if DCHECK_IS_ON()
  DCHECK(buffer_owned_by_file_thread_);
#endif

  if (!buffer_)
    buffer_.reset(new Buffer());
  buffer_->Reset(bytes_to_read);
  buffer_->ReadFrom(stream_);

#if DCHECK_IS_ON()
  buffer_owned_by_file_thread_ = false;
#endif
  callback->Continue();
}
