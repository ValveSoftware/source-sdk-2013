// Copyright (c) 2010 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "include/wrapper/cef_zip_archive.h"

#include <algorithm>

#include "include/base/cef_logging.h"
#include "include/base/cef_macros.h"
#include "include/base/cef_scoped_ptr.h"
#include "include/cef_stream.h"
#include "include/cef_zip_reader.h"
#include "include/wrapper/cef_byte_read_handler.h"

#if defined(OS_LINUX)
#include <wctype.h>
#endif

namespace {

// Convert |str| to lowercase in a Unicode-friendly manner.
CefString ToLower(const CefString& str) {
  std::wstring wstr = str;
  std::transform(wstr.begin(), wstr.end(), wstr.begin(), towlower);
  return wstr;
}

class CefZipFile : public CefZipArchive::File {
 public:
  CefZipFile() : data_size_(0) {}

  bool Initialize(size_t data_size) {
    data_.reset(new unsigned char[data_size]);
    if (data_) {
      data_size_ = data_size;
      return true;
    } else {
      DLOG(ERROR) << "Failed to allocate " << data_size << " bytes of memory";
      data_size_ = 0;
      return false;
    }
  }

  virtual const unsigned char* GetData() const OVERRIDE { return data_.get(); }

  virtual size_t GetDataSize() const OVERRIDE { return data_size_; }

  virtual CefRefPtr<CefStreamReader> GetStreamReader() const OVERRIDE {
    CefRefPtr<CefReadHandler> handler(
        new CefByteReadHandler(data_.get(), data_size_,
                               const_cast<CefZipFile*>(this)));
    return CefStreamReader::CreateForHandler(handler);
  }

  unsigned char* data() { return data_.get(); }

 private:
  size_t data_size_;
  SCOPED_PTR(unsigned char[]) data_;

  IMPLEMENT_REFCOUNTING(CefZipFile);
  DISALLOW_COPY_AND_ASSIGN(CefZipFile);
};

}  // namespace

// CefZipArchive implementation

CefZipArchive::CefZipArchive() {
}

CefZipArchive::~CefZipArchive() {
}

size_t CefZipArchive::Load(CefRefPtr<CefStreamReader> stream,
                           const CefString& password,
                           bool overwriteExisting) {
  base::AutoLock lock_scope(lock_);

  CefRefPtr<CefZipReader> reader(CefZipReader::Create(stream));
  if (!reader.get())
    return 0;

  if (!reader->MoveToFirstFile())
    return 0;

  FileMap::iterator it;
  size_t count = 0;

  do {
    const size_t size = static_cast<size_t>(reader->GetFileSize());
    if (size == 0) {
      // Skip directories and empty files.
      continue;
    }

    if (!reader->OpenFile(password))
      break;

    const CefString& name = ToLower(reader->GetFileName());

    it = contents_.find(name);
    if (it != contents_.end()) {
      if (overwriteExisting)
        contents_.erase(it);
      else  // Skip files that already exist.
        continue;
    }

    CefRefPtr<CefZipFile> contents = new CefZipFile();
    if (!contents->Initialize(size))
      continue;
    unsigned char* data = contents->data();
    size_t offset = 0;

    // Read the file contents.
    do {
     offset += reader->ReadFile(data + offset, size - offset);
    } while (offset < size && !reader->Eof());

    DCHECK(offset == size);

    reader->CloseFile();
    count++;

    // Add the file to the map.
    contents_.insert(std::make_pair(name, contents.get()));
  } while (reader->MoveToNextFile());

  return count;
}

void CefZipArchive::Clear() {
  base::AutoLock lock_scope(lock_);
  contents_.clear();
}

size_t CefZipArchive::GetFileCount() const {
  base::AutoLock lock_scope(lock_);
  return contents_.size();
}

bool CefZipArchive::HasFile(const CefString& fileName) const {
  base::AutoLock lock_scope(lock_);
  FileMap::const_iterator it = contents_.find(ToLower(fileName));
  return (it != contents_.end());
}

CefRefPtr<CefZipArchive::File> CefZipArchive::GetFile(
    const CefString& fileName) const {
  base::AutoLock lock_scope(lock_);
  FileMap::const_iterator it = contents_.find(ToLower(fileName));
  if (it != contents_.end())
    return it->second;
  return NULL;
}

bool CefZipArchive::RemoveFile(const CefString& fileName) {
  base::AutoLock lock_scope(lock_);
  FileMap::iterator it = contents_.find(ToLower(fileName));
  if (it != contents_.end()) {
    contents_.erase(it);
    return true;
  }
  return false;
}

size_t CefZipArchive::GetFiles(FileMap& map) const {
  base::AutoLock lock_scope(lock_);
  map = contents_;
  return contents_.size();
}
