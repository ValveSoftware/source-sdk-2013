// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "include/wrapper/cef_resource_manager.h"

#include <algorithm>
#include <vector>

#include "include/base/cef_macros.h"
#include "include/base/cef_weak_ptr.h"
#include "include/cef_parser.h"
#include "include/wrapper/cef_stream_resource_handler.h"
#include "include/wrapper/cef_zip_archive.h"

namespace {

#if defined(OS_WIN)
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif
  
// Returns |url| without the query or fragment components, if any.
std::string GetUrlWithoutQueryOrFragment(const std::string& url) {
  // Find the first instance of '?' or '#'.
  const size_t pos = std::min(url.find('?'), url.find('#'));
  if (pos != std::string::npos)
    return url.substr(0, pos);

  return url;
}

// Determine the mime type based on the |url| file extension.
std::string GetMimeType(const std::string& url) {
  std::string mime_type;
  const std::string& url_without_query = GetUrlWithoutQueryOrFragment(url);
  size_t sep = url_without_query.find_last_of(".");
  if (sep != std::string::npos) {
    mime_type = CefGetMimeType(url_without_query.substr(sep + 1));
    if (!mime_type.empty())
      return mime_type;
  }
  return "text/html";
}

// Default no-op filter.
std::string GetFilteredUrl(const std::string& url) {
  return url;
}


// Provider of fixed contents.
class ContentProvider : public CefResourceManager::Provider {
 public:
  ContentProvider(const std::string& url,
                  const std::string& content,
                  const std::string& mime_type)
    : url_(url),
      content_(content),
      mime_type_(mime_type) {
    DCHECK(!url.empty());
    DCHECK(!content.empty());
  }

  bool OnRequest(scoped_refptr<CefResourceManager::Request> request) OVERRIDE {
    CEF_REQUIRE_IO_THREAD();

    const std::string& url = request->url();
    if (url != url_) {
      // Not handled by this provider.
      return false;
    }

    CefRefPtr<CefStreamReader> stream =
        CefStreamReader::CreateForData(
            static_cast<void*>(const_cast<char*>(content_.data())),
            content_.length());

    // Determine the mime type a single time if it isn't already set.
    if (mime_type_.empty())
      mime_type_ = request->mime_type_resolver().Run(url);

    request->Continue(new CefStreamResourceHandler(mime_type_, stream));
    return true;
  }

 private:
  std::string url_;
  std::string content_;
  std::string mime_type_;

  DISALLOW_COPY_AND_ASSIGN(ContentProvider);
};


// Provider of contents loaded from a directory on the file system.
class DirectoryProvider : public CefResourceManager::Provider {
 public:
  DirectoryProvider(const std::string& url_path,
                    const std::string& directory_path)
    : url_path_(url_path),
      directory_path_(directory_path) {
    DCHECK(!url_path_.empty());
    DCHECK(!directory_path_.empty());

    // Normalize the path values.
    if (url_path_[url_path_.size() - 1] != '/')
      url_path_ += '/';
    if (directory_path_[directory_path_.size() - 1] != PATH_SEP)
      directory_path_ += PATH_SEP;
  }

  bool OnRequest(scoped_refptr<CefResourceManager::Request> request) OVERRIDE {
    CEF_REQUIRE_IO_THREAD();

    const std::string& url = request->url();
    if (url.find(url_path_) != 0U) {
      return false;
    }

    const std::string& file_path = GetFilePath(url);

    // Open |file_path| on the FILE thread.
    CefPostTask(TID_FILE,
        base::Bind(&DirectoryProvider::OpenOnFileThread, file_path, request));

    return true;
  }

 private:
  std::string GetFilePath(const std::string& url) {
    std::string path_part = url.substr(url_path_.length());
#if defined(OS_WIN)
    std::replace(path_part.begin(), path_part.end(), '/', '\\');
#endif
    return directory_path_ + path_part;
  }

  static void OpenOnFileThread(
      const std::string& file_path,
      scoped_refptr<CefResourceManager::Request> request) {
    CEF_REQUIRE_FILE_THREAD();

    CefRefPtr<CefStreamReader> stream =
        CefStreamReader::CreateForFile(file_path);

    // Continue loading on the IO thread.
    CefPostTask(TID_IO,
        base::Bind(&DirectoryProvider::ContinueOpenOnIOThread, request,
                   stream));
  }

  static void ContinueOpenOnIOThread(
      scoped_refptr<CefResourceManager::Request> request,
      CefRefPtr<CefStreamReader> stream) {
    CEF_REQUIRE_IO_THREAD();

    CefRefPtr<CefStreamResourceHandler> handler;
    if (stream.get()) {
      handler = new CefStreamResourceHandler(
          request->mime_type_resolver().Run(request->url()),
          stream);
    }
    request->Continue(handler);
  }

  std::string url_path_;
  std::string directory_path_;

  DISALLOW_COPY_AND_ASSIGN(DirectoryProvider);
};


// Provider of contents loaded from an archive file.
class ArchiveProvider : public CefResourceManager::Provider {
 public:
  ArchiveProvider(const std::string& url_path,
                  const std::string& archive_path,
                  const std::string& password)
    : url_path_(url_path),
      archive_path_(archive_path),
      password_(password),
      archive_load_started_(false),
      archive_load_ended_(false),
      ALLOW_THIS_IN_INITIALIZER_LIST(weak_ptr_factory_(this)) {
    DCHECK(!url_path_.empty());
    DCHECK(!archive_path_.empty());

    // Normalize the path values.
    if (url_path_[url_path_.size() - 1] != '/')
      url_path_ += '/';
  }

  bool OnRequest(scoped_refptr<CefResourceManager::Request> request) OVERRIDE {
    CEF_REQUIRE_IO_THREAD();

    const std::string& url = request->url();
    if (url.find(url_path_) != 0U) {
      // Not handled by this provider.
      return false;
    }

    if (!archive_load_started_) {
      // Initiate archive loading and queue the pending request.
      archive_load_started_ = true;
      pending_requests_.push_back(request);

      // Load the archive file on the FILE thread.
      CefPostTask(TID_FILE,
          base::Bind(&ArchiveProvider::LoadOnFileThread,
                     weak_ptr_factory_.GetWeakPtr(), archive_path_, password_));
      return true;
    }

    if (archive_load_started_ && !archive_load_ended_) {
      // The archive load has already started. Queue the pending request.
      pending_requests_.push_back(request);
      return true;
    }

    // Archive loading is done.
    return ContinueRequest(request);
  }

 private:
  static void LoadOnFileThread(base::WeakPtr<ArchiveProvider> ptr,
                               const std::string& archive_path,
                               const std::string& password) {
    CEF_REQUIRE_FILE_THREAD();

    CefRefPtr<CefZipArchive> archive;

    CefRefPtr<CefStreamReader> stream =
        CefStreamReader::CreateForFile(archive_path);
    if (stream.get()) {
      archive = new CefZipArchive;
      if (archive->Load(stream, password, true) == 0) {
        DLOG(WARNING) << "Empty archive file: " << archive_path;
        archive = NULL;
      }
    } else {
      DLOG(WARNING) << "Failed to load archive file: " << archive_path;
    }

    CefPostTask(TID_IO,
        base::Bind(&ArchiveProvider::ContinueOnIOThread, ptr, archive));
  }

  void ContinueOnIOThread(CefRefPtr<CefZipArchive> archive) {
    CEF_REQUIRE_IO_THREAD();

    archive_load_ended_ = true;
    archive_ = archive;

    if (!pending_requests_.empty()) {
      // Continue all pending requests.
      PendingRequests::const_iterator it = pending_requests_.begin();
      for (; it != pending_requests_.end(); ++it)
        ContinueRequest(*it);
      pending_requests_.clear();
    }
  }

  bool ContinueRequest(scoped_refptr<CefResourceManager::Request> request) {
    CefRefPtr<CefResourceHandler> handler;

    // |archive_| will be NULL if the archive file failed to load or was empty.
    if (archive_.get()) {
      const std::string& url = request->url();
      const std::string& relative_path = url.substr(url_path_.length());
      CefRefPtr<CefZipArchive::File> file = archive_->GetFile(relative_path);
      if (file.get()) {
        handler = new CefStreamResourceHandler(
            request->mime_type_resolver().Run(url),
            file->GetStreamReader());
      }
    }

    if (!handler.get())
      return false;

    request->Continue(handler);
    return true;
  }

  std::string url_path_;
  std::string archive_path_;
  std::string password_;

  bool archive_load_started_;
  bool archive_load_ended_;
  CefRefPtr<CefZipArchive> archive_;

  // List of requests that are pending while the archive is being loaded.
  typedef std::vector<scoped_refptr<CefResourceManager::Request> >
      PendingRequests;
  PendingRequests pending_requests_;

  // Must be the last member.
  base::WeakPtrFactory<ArchiveProvider> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArchiveProvider);
};

}  // namespace


// CefResourceManager::ProviderEntry implementation.

struct CefResourceManager::ProviderEntry {
  ProviderEntry(Provider* provider,
                int order,
                const std::string& identifier)
    : provider_(provider),
      order_(order),
      identifier_(identifier),
      deletion_pending_(false) {
  }

  SCOPED_PTR(Provider) provider_;
  int order_;
  std::string identifier_;

  // List of pending requests currently associated with this provider.
  RequestList pending_requests_;

  // True if deletion of this provider is pending.
  bool deletion_pending_;
};


// CefResourceManager::RequestState implementation.

CefResourceManager::RequestState::~RequestState() {
  // Always execute the callback.
  if (callback_.get())
    callback_->Continue(true);
}


// CefResourceManager::Request implementation.

void CefResourceManager::Request::Continue(
    CefRefPtr<CefResourceHandler> handler) {
  if (!CefCurrentlyOn(TID_IO)) {
    CefPostTask(TID_IO,
        base::Bind(&CefResourceManager::Request::Continue, this, handler));
    return;
  }

  if (!state_.get())
    return;

  // Disassociate |state_| immediately so that Provider::OnRequestCanceled is
  // not called unexpectedly if Provider::OnRequest calls this method and then
  // calls CefResourceManager::Remove*.
  CefPostTask(TID_IO,
      base::Bind(&CefResourceManager::Request::ContinueOnIOThread,
                 base::Passed(&state_), handler));
}

void CefResourceManager::Request::Stop() {
  if (!CefCurrentlyOn(TID_IO)) {
    CefPostTask(TID_IO,
        base::Bind(&CefResourceManager::Request::Stop, this));
    return;
  }

  if (!state_.get())
    return;

  // Disassociate |state_| immediately so that Provider::OnRequestCanceled is
  // not called unexpectedly if Provider::OnRequest calls this method and then
  // calls CefResourceManager::Remove*.
  CefPostTask(TID_IO,
      base::Bind(&CefResourceManager::Request::StopOnIOThread,
                 base::Passed(&state_)));
}

CefResourceManager::Request::Request(SCOPED_PTR(RequestState) state)
    : state_(MOVE_SCOPED_PTR(state)),
      params_(state_->params_) {
  CEF_REQUIRE_IO_THREAD();

  ProviderEntry* entry = *(state_->current_entry_pos_);
  // Should not be on a deleted entry.
  DCHECK(!entry->deletion_pending_);

  // Add this request to the entry's pending request list.
  entry->pending_requests_.push_back(this);
  state_->current_request_pos_ = --entry->pending_requests_.end();
}

// Detaches and returns |state_| if the provider indicates that it will not
// handle the request. Note that |state_| may already be NULL if OnRequest
// executes a callback before returning, in which case execution will continue
// asynchronously in any case.
SCOPED_PTR(CefResourceManager::RequestState)
    CefResourceManager::Request::SendRequest() {
  CEF_REQUIRE_IO_THREAD();
  Provider* provider = (*state_->current_entry_pos_)->provider_.get();
  if (!provider->OnRequest(this))
    return MOVE_SCOPED_PTR(state_);
  return SCOPED_PTR(RequestState)();
}

bool CefResourceManager::Request::HasState() {
  CEF_REQUIRE_IO_THREAD();
  return (state_.get() != NULL);
}

// static
void CefResourceManager::Request::ContinueOnIOThread(
    SCOPED_PTR(RequestState) state,
    CefRefPtr<CefResourceHandler> handler) {
  CEF_REQUIRE_IO_THREAD();
  // The manager may already have been deleted.
  base::WeakPtr<CefResourceManager> manager = state->manager_;
  if (manager)
    manager->ContinueRequest(MOVE_SCOPED_PTR(state), handler);
}

// static
void CefResourceManager::Request::StopOnIOThread(
    SCOPED_PTR(RequestState) state) {
  CEF_REQUIRE_IO_THREAD();
  // The manager may already have been deleted.
  base::WeakPtr<CefResourceManager> manager = state->manager_;
  if (manager)
    manager->StopRequest(MOVE_SCOPED_PTR(state));
}


// CefResourceManager implementation.

CefResourceManager::CefResourceManager()
    : url_filter_(base::Bind(GetFilteredUrl)),
      mime_type_resolver_(base::Bind(GetMimeType)) {
}

CefResourceManager::~CefResourceManager() {
  CEF_REQUIRE_IO_THREAD();
  RemoveAllProviders();

  // Delete all entryies now. Requests may still be pending but they will not
  // call back into this manager due to the use of WeakPtr.
  if (!providers_.empty()) {
    ProviderEntryList::iterator it = providers_.begin();
    for (; it != providers_.end(); ++it)
      delete *it;
    providers_.clear();
  }
}

void CefResourceManager::AddContentProvider(const std::string& url,
                                            const std::string& content,
                                            const std::string& mime_type,
                                            int order,
                                            const std::string& identifier) {
  AddProvider(new ContentProvider(url, content, mime_type), order, identifier);
}

void CefResourceManager::AddDirectoryProvider(
    const std::string& url_path,
    const std::string& directory_path,
    int order,
    const std::string& identifier) {
  AddProvider(new DirectoryProvider(url_path, directory_path),
              order, identifier);
}
                    
void CefResourceManager::AddArchiveProvider(const std::string& url_path,
                                            const std::string& archive_path,
                                            const std::string& password,
                                            int order,
                                            const std::string& identifier) {
  AddProvider(new ArchiveProvider(url_path, archive_path, password),
              order, identifier);
}

void CefResourceManager::AddProvider(Provider* provider,
                                     int order,
                                     const std::string& identifier) {
  DCHECK(provider);
  if (!provider)
    return;

  if (!CefCurrentlyOn(TID_IO)) {
    CefPostTask(TID_IO,
        base::Bind(&CefResourceManager::AddProvider, this, provider, order,
                   identifier));
    return;
  }

  SCOPED_PTR(ProviderEntry) new_entry(
      new ProviderEntry(provider, order, identifier));

  if (providers_.empty()) {
    providers_.push_back(new_entry.release());
    return;
  }

  // Insert before the first entry with a higher |order| value.
  ProviderEntryList::iterator it = providers_.begin();
  for (; it != providers_.end(); ++it) {
    if ((*it)->order_ > order)
      break;
  }

  providers_.insert(it, new_entry.release());
}

void CefResourceManager::RemoveProviders(const std::string& identifier) {
  if (!CefCurrentlyOn(TID_IO)) {
    CefPostTask(TID_IO,
        base::Bind(&CefResourceManager::RemoveProviders, this, identifier));
    return;
  }

  if (providers_.empty())
    return;

  ProviderEntryList::iterator it = providers_.begin();
  while (it != providers_.end()) {
    if ((*it)->identifier_ == identifier)
      DeleteProvider(it, false);
    else
      ++it;
  }
}

void CefResourceManager::RemoveAllProviders() {
  if (!CefCurrentlyOn(TID_IO)) {
    CefPostTask(TID_IO,
        base::Bind(&CefResourceManager::RemoveAllProviders, this));
    return;
  }

  if (providers_.empty())
    return;

  ProviderEntryList::iterator it = providers_.begin();
  while (it != providers_.end())
    DeleteProvider(it, true);
}

void CefResourceManager::SetMimeTypeResolver(const MimeTypeResolver& resolver) {
  if (!CefCurrentlyOn(TID_IO)) {
    CefPostTask(TID_IO,
        base::Bind(&CefResourceManager::SetMimeTypeResolver, this, resolver));
    return;
  }

  if (!resolver.is_null())
    mime_type_resolver_ = resolver;
  else
    mime_type_resolver_ = base::Bind(GetMimeType);
}

void CefResourceManager::SetUrlFilter(const UrlFilter& filter) {
  if (!CefCurrentlyOn(TID_IO)) {
    CefPostTask(TID_IO,
        base::Bind(&CefResourceManager::SetUrlFilter, this, filter));
    return;
  }

  if (!filter.is_null())
    url_filter_ = filter;
  else
    url_filter_ = base::Bind(GetFilteredUrl);
}

cef_return_value_t CefResourceManager::OnBeforeResourceLoad(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefRequest> request,
    CefRefPtr<CefRequestCallback> callback) {
  CEF_REQUIRE_IO_THREAD();

  // Find the first provider that is not pending deletion.
  ProviderEntryList::iterator current_entry_pos = providers_.begin();
  GetNextValidProvider(current_entry_pos);
 
  if (current_entry_pos == providers_.end()) {
    // No providers so continue the request immediately.
    return RV_CONTINUE;
  }

  SCOPED_PTR(RequestState) state(new RequestState);

  if (!weak_ptr_factory_.get()) {
    // WeakPtrFactory instances need to be created and destroyed on the same
    // thread. This object performs most of its work on the IO thread and will
    // be destroyed on the IO thread so, now that we're on the IO thread,
    // properly initialize the WeakPtrFactory.
    weak_ptr_factory_.reset(new base::WeakPtrFactory<CefResourceManager>(this));
  }

  state->manager_ = weak_ptr_factory_->GetWeakPtr();
  state->callback_ = callback;

  state->params_.url_ =
      GetUrlWithoutQueryOrFragment(url_filter_.Run(request->GetURL()));
  state->params_.browser_ = browser;
  state->params_.frame_ = frame;
  state->params_.request_ = request;
  state->params_.url_filter_ = url_filter_;
  state->params_.mime_type_resolver_ = mime_type_resolver_;

  state->current_entry_pos_ = current_entry_pos;

  // If the request is potentially handled we need to continue asynchronously.
  return SendRequest(MOVE_SCOPED_PTR(state)) ? RV_CONTINUE_ASYNC : RV_CONTINUE;
}

CefRefPtr<CefResourceHandler> CefResourceManager::GetResourceHandler(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefRequest> request) {
  CEF_REQUIRE_IO_THREAD();

  if (pending_handlers_.empty())
    return NULL;

  CefRefPtr<CefResourceHandler> handler;

  PendingHandlersMap::iterator it =
      pending_handlers_.find(request->GetIdentifier());
  if (it != pending_handlers_.end()) {
    handler = it->second;
    pending_handlers_.erase(it);
  }

  return handler;
}

// Send the request to providers in order until one potentially handles it or we
// run out of providers. Returns true if the request is potentially handled.
bool CefResourceManager::SendRequest(SCOPED_PTR(RequestState) state) {
  bool potentially_handled = false;

  do {
    // Should not be on the last provider entry.
    DCHECK(state->current_entry_pos_ != providers_.end());
    scoped_refptr<Request> request = new Request(MOVE_SCOPED_PTR(state));

    // Give the provider an opportunity to handle the request.
    state = request->SendRequest();
    if (state.get()) {
      // The provider will not handle the request. Move to the next provider if
      // any.
      if (!IncrementProvider(state.get()))
        StopRequest(MOVE_SCOPED_PTR(state));
    } else {
      potentially_handled = true;
    }
  } while (state.get());

  return potentially_handled;
}

void CefResourceManager::ContinueRequest(
    SCOPED_PTR(RequestState) state,
    CefRefPtr<CefResourceHandler> handler) {
  CEF_REQUIRE_IO_THREAD();

  if (handler.get()) {
    // The request has been handled. Associate the request ID with the handler.
    pending_handlers_.insert(
        std::make_pair(state->params_.request_->GetIdentifier(), handler));
    StopRequest(MOVE_SCOPED_PTR(state));
  } else {
    // Move to the next provider if any.
    if (IncrementProvider(state.get()))
      SendRequest(MOVE_SCOPED_PTR(state));
    else
      StopRequest(MOVE_SCOPED_PTR(state));
  }
}

void CefResourceManager::StopRequest(SCOPED_PTR(RequestState) state) {
  CEF_REQUIRE_IO_THREAD();

  // Detach from the current provider.
  DetachRequestFromProvider(state.get());

  // Delete the state object and execute the callback.
  state.reset();
}

// Move state to the next provider if any and return true if there are more
// providers.
bool CefResourceManager::IncrementProvider(RequestState* state) {
  // Identify the next provider.
  ProviderEntryList::iterator next_entry_pos = state->current_entry_pos_;
  GetNextValidProvider(++next_entry_pos);

  // Detach from the current provider.
  DetachRequestFromProvider(state);

  if (next_entry_pos != providers_.end()) {
    // Update the state to reference the new provider entry.
    state->current_entry_pos_ = next_entry_pos;
    return true;
  }

  return false;
}

// The new provider, if any, should be determined before calling this method.
void CefResourceManager::DetachRequestFromProvider(RequestState* state) {
  if (state->current_entry_pos_ != providers_.end()) {
    // Remove the association from the current provider entry.
    ProviderEntryList::iterator current_entry_pos =
        state->current_entry_pos_;
    ProviderEntry* current_entry = *(current_entry_pos);
    current_entry->pending_requests_.erase(state->current_request_pos_);

    if (current_entry->deletion_pending_ &&
        current_entry->pending_requests_.empty()) {
      // Delete the current provider entry now.
      providers_.erase(current_entry_pos);
      delete current_entry;
    }

    // Set to the end for error checking purposes.
    state->current_entry_pos_ = providers_.end();
  }
}

// Move to the next provider that is not pending deletion.
void CefResourceManager::GetNextValidProvider(
    ProviderEntryList::iterator& iterator) {
  while (iterator != providers_.end() && (*iterator)->deletion_pending_) {
    ++iterator;
  }
}

void CefResourceManager::DeleteProvider(ProviderEntryList::iterator& iterator,
                                        bool stop) {
  CEF_REQUIRE_IO_THREAD();

  ProviderEntry* current_entry = *(iterator);

  if (current_entry->deletion_pending_)
    return;

  if (!current_entry->pending_requests_.empty()) {
    // Don't delete the provider entry until all pending requests have cleared.
    current_entry->deletion_pending_ = true;

    // Continue pending requests immediately.
    RequestList::iterator it = current_entry->pending_requests_.begin();
    for (; it != current_entry->pending_requests_.end(); ++it) {
      const scoped_refptr<Request>& request = *it;
      if (request->HasState()) {
        if (stop)
          request->Stop();
        else
          request->Continue(NULL);
        current_entry->provider_->OnRequestCanceled(request);
      }
    }

    ++iterator;
  } else {
    // Delete the provider entry now.
    iterator = providers_.erase(iterator);
    delete current_entry;
  }
}
