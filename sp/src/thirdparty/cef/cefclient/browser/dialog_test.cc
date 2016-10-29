// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/browser/dialog_test.h"

#include <string>

#include "include/cef_browser.h"
#include "include/wrapper/cef_helpers.h"

namespace client {
namespace dialog_test {

namespace {

const char kTestUrl[] = "http://tests/dialogs";
const char kFileOpenMessageName[] = "DialogTest.FileOpen";
const char kFileOpenMultipleMessageName[] = "DialogTest.FileOpenMultiple";
const char kFileOpenFolderMessageName[] = "DialogTest.FileOpenFolder";
const char kFileSaveMessageName[] = "DialogTest.FileSave";

#if defined(OS_WIN)
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif

// Store persistent dialog state information.
class DialogState : public base::RefCountedThreadSafe<DialogState> {
 public:
  DialogState()
    : mode_(FILE_DIALOG_OPEN),
      last_selected_filter_(0),
      pending_(false) {}

  cef_file_dialog_mode_t mode_;
  int last_selected_filter_;
  CefString last_file_;
  bool pending_;

  DISALLOW_COPY_AND_ASSIGN(DialogState);
};

// Callback executed when the file dialog is dismissed.
class DialogCallback : public CefRunFileDialogCallback {
 public:
  DialogCallback(
      CefRefPtr<CefMessageRouterBrowserSide::Callback> router_callback,
      scoped_refptr<DialogState> dialog_state)
      : router_callback_(router_callback),
        dialog_state_(dialog_state) {
  }

  virtual void OnFileDialogDismissed(
      int last_selected_filter,
      const std::vector<CefString>& file_paths) OVERRIDE {
    CEF_REQUIRE_UI_THREAD();
    DCHECK(dialog_state_->pending_);

    if (!file_paths.empty()) {
      if (dialog_state_->mode_ != FILE_DIALOG_OPEN_FOLDER)
        dialog_state_->last_selected_filter_ = last_selected_filter;

      dialog_state_->last_file_ = file_paths[0];
      if (dialog_state_->mode_ == FILE_DIALOG_OPEN_FOLDER) {
        std::string last_file = dialog_state_->last_file_;
        if (last_file[last_file.length() - 1] != PATH_SEP) {
          // Add a trailing slash so we know it's a directory. Otherwise, file
          // dialogs will think the last path component is a file name.
          last_file += PATH_SEP;
          dialog_state_->last_file_ = last_file;
        }
      }
    }

    // Send a message back to the render process with the list of file paths.
    std::string response;
    for (int i = 0; i < static_cast<int>(file_paths.size()); ++i) {
      if (!response.empty())
        response += "|";  // Use a delimiter disallowed in file paths.
      response += file_paths[i];
    }

    router_callback_->Success(response);
    router_callback_ = NULL;

    dialog_state_->pending_ = false;
    dialog_state_ = NULL;
  }

 private:
  CefRefPtr<CefMessageRouterBrowserSide::Callback> router_callback_;
  scoped_refptr<DialogState> dialog_state_;

  IMPLEMENT_REFCOUNTING(DialogCallback);
  DISALLOW_COPY_AND_ASSIGN(DialogCallback);
};

// Handle messages in the browser process.
class Handler : public CefMessageRouterBrowserSide::Handler {
 public:
  Handler() {}

  // Called due to cefQuery execution in dialogs.html.
  virtual bool OnQuery(CefRefPtr<CefBrowser> browser,
                       CefRefPtr<CefFrame> frame,
                       int64 query_id,
                       const CefString& request,
                       bool persistent,
                       CefRefPtr<Callback> callback) OVERRIDE {
    CEF_REQUIRE_UI_THREAD();

    // Only handle messages from the test URL.
    const std::string& url = frame->GetURL();
    if (url.find(kTestUrl) != 0)
      return false;

    if (!dialog_state_.get())
      dialog_state_ = new DialogState;

    // Make sure we're only running one dialog at a time.
    DCHECK(!dialog_state_->pending_);

    std::vector<CefString> accept_filters;
    std::string title;

    const std::string& message_name = request;
    if (message_name == kFileOpenMessageName) {
      dialog_state_->mode_ = FILE_DIALOG_OPEN;
      title = "My Open Dialog";
    } else if (message_name == kFileOpenMultipleMessageName) {
      dialog_state_->mode_ = FILE_DIALOG_OPEN_MULTIPLE;
      title = "My Open Multiple Dialog";
    } else if (message_name == kFileOpenFolderMessageName) {
      dialog_state_->mode_ = FILE_DIALOG_OPEN_FOLDER;
      title = "My Open Folder Dialog";
    } else if (message_name == kFileSaveMessageName) {
      dialog_state_->mode_ = static_cast<cef_file_dialog_mode_t>(
          FILE_DIALOG_SAVE | FILE_DIALOG_OVERWRITEPROMPT_FLAG);
      title = "My Save Dialog";
    } else {
      NOTREACHED();
      return true;
    }

    if (dialog_state_->mode_ != FILE_DIALOG_OPEN_FOLDER) {
      // Build filters based on mime time.
      accept_filters.push_back("text/*");
    
      // Build filters based on file extension.
      accept_filters.push_back(".log");
      accept_filters.push_back(".patch");

      // Add specific filters as-is.
      accept_filters.push_back("Document Files|.doc;.odt");
      accept_filters.push_back("Image Files|.png;.jpg;.gif");
      accept_filters.push_back("PDF Files|.pdf");
    }

    dialog_state_->pending_ = true;

    browser->GetHost()->RunFileDialog(
        dialog_state_->mode_,
        title,
        dialog_state_->last_file_,
        accept_filters,
        dialog_state_->last_selected_filter_,
        new DialogCallback(callback, dialog_state_));

    return true;
  }

 private:
  scoped_refptr<DialogState> dialog_state_;

  DISALLOW_COPY_AND_ASSIGN(Handler);
};

}  // namespace

void CreateMessageHandlers(test_runner::MessageHandlerSet& handlers) {
  handlers.insert(new Handler());
}

}  // namespace dialog_test
}  // namespace client
