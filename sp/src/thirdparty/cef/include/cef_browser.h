// Copyright (c) 2012 Marshall A. Greenblatt. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the name Chromium Embedded
// Framework nor the names of its contributors may be used to endorse
// or promote products derived from this software without specific prior
// written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// ---------------------------------------------------------------------------
//
// The contents of this file must follow a specific format in order to
// support the CEF translator tool. See the translator.README.txt file in the
// tools directory for more information.
//

#ifndef CEF_INCLUDE_CEF_BROWSER_H_
#define CEF_INCLUDE_CEF_BROWSER_H_
#pragma once

#include "include/cef_base.h"
#include "include/cef_drag_data.h"
#include "include/cef_frame.h"
#include "include/cef_image.h"
#include "include/cef_navigation_entry.h"
#include "include/cef_process_message.h"
#include "include/cef_request_context.h"
#include <vector>

class CefBrowserHost;
class CefClient;


///
// Class used to represent a browser window. When used in the browser process
// the methods of this class may be called on any thread unless otherwise
// indicated in the comments. When used in the render process the methods of
// this class may only be called on the main thread.
///
/*--cef(source=library)--*/
class CefBrowser : public virtual CefBase {
 public:
  ///
  // Returns the browser host object. This method can only be called in the
  // browser process.
  ///
  /*--cef()--*/
  virtual CefRefPtr<CefBrowserHost> GetHost() =0;

  ///
  // Returns true if the browser can navigate backwards.
  ///
  /*--cef()--*/
  virtual bool CanGoBack() =0;

  ///
  // Navigate backwards.
  ///
  /*--cef()--*/
  virtual void GoBack() =0;

  ///
  // Returns true if the browser can navigate forwards.
  ///
  /*--cef()--*/
  virtual bool CanGoForward() =0;

  ///
  // Navigate forwards.
  ///
  /*--cef()--*/
  virtual void GoForward() =0;

  ///
  // Returns true if the browser is currently loading.
  ///
  /*--cef()--*/
  virtual bool IsLoading() =0;

  ///
  // Reload the current page.
  ///
  /*--cef()--*/
  virtual void Reload() =0;

  ///
  // Reload the current page ignoring any cached data.
  ///
  /*--cef()--*/
  virtual void ReloadIgnoreCache() =0;

  ///
  // Stop loading the page.
  ///
  /*--cef()--*/
  virtual void StopLoad() =0;

  ///
  // Returns the globally unique identifier for this browser.
  ///
  /*--cef()--*/
  virtual int GetIdentifier() =0;

  ///
  // Returns true if this object is pointing to the same handle as |that|
  // object.
  ///
  /*--cef()--*/
  virtual bool IsSame(CefRefPtr<CefBrowser> that) =0;

  ///
  // Returns true if the window is a popup window.
  ///
  /*--cef()--*/
  virtual bool IsPopup() =0;

  ///
  // Returns true if a document has been loaded in the browser.
  ///
  /*--cef()--*/
  virtual bool HasDocument() =0;

  ///
  // Returns the main (top-level) frame for the browser window.
  ///
  /*--cef()--*/
  virtual CefRefPtr<CefFrame> GetMainFrame() =0;

  ///
  // Returns the focused frame for the browser window.
  ///
  /*--cef()--*/
  virtual CefRefPtr<CefFrame> GetFocusedFrame() =0;

  ///
  // Returns the frame with the specified identifier, or NULL if not found.
  ///
  /*--cef(capi_name=get_frame_byident)--*/
  virtual CefRefPtr<CefFrame> GetFrame(int64 identifier) =0;

  ///
  // Returns the frame with the specified name, or NULL if not found.
  ///
  /*--cef(optional_param=name)--*/
  virtual CefRefPtr<CefFrame> GetFrame(const CefString& name) =0;

  ///
  // Returns the number of frames that currently exist.
  ///
  /*--cef()--*/
  virtual size_t GetFrameCount() =0;

  ///
  // Returns the identifiers of all existing frames.
  ///
  /*--cef(count_func=identifiers:GetFrameCount)--*/
  virtual void GetFrameIdentifiers(std::vector<int64>& identifiers) =0;

  ///
  // Returns the names of all existing frames.
  ///
  /*--cef()--*/
  virtual void GetFrameNames(std::vector<CefString>& names) =0;

  ///
  // Send a message to the specified |target_process|. Returns true if the
  // message was sent successfully.
  ///
  /*--cef()--*/
  virtual bool SendProcessMessage(CefProcessId target_process,
                                  CefRefPtr<CefProcessMessage> message) =0;
};


///
// Callback interface for CefBrowserHost::RunFileDialog. The methods of this
// class will be called on the browser process UI thread.
///
/*--cef(source=client)--*/
class CefRunFileDialogCallback : public virtual CefBase {
 public:
  ///
  // Called asynchronously after the file dialog is dismissed.
  // |selected_accept_filter| is the 0-based index of the value selected from
  // the accept filters array passed to CefBrowserHost::RunFileDialog.
  // |file_paths| will be a single value or a list of values depending on the
  // dialog mode. If the selection was cancelled |file_paths| will be empty.
  ///
  /*--cef(index_param=selected_accept_filter,optional_param=file_paths)--*/
  virtual void OnFileDialogDismissed(
      int selected_accept_filter,
      const std::vector<CefString>& file_paths) =0;
};


///
// Callback interface for CefBrowserHost::GetNavigationEntries. The methods of
// this class will be called on the browser process UI thread.
///
/*--cef(source=client)--*/
class CefNavigationEntryVisitor : public virtual CefBase {
 public:
  ///
  // Method that will be executed. Do not keep a reference to |entry| outside of
  // this callback. Return true to continue visiting entries or false to stop.
  // |current| is true if this entry is the currently loaded navigation entry.
  // |index| is the 0-based index of this entry and |total| is the total number
  // of entries.
  ///
  /*--cef()--*/
  virtual bool Visit(CefRefPtr<CefNavigationEntry> entry,
                     bool current,
                     int index,
                     int total) =0;
};


///
// Callback interface for CefBrowserHost::PrintToPDF. The methods of this class
// will be called on the browser process UI thread.
///
/*--cef(source=client)--*/
class CefPdfPrintCallback : public virtual CefBase {
 public:
  ///
  // Method that will be executed when the PDF printing has completed. |path|
  // is the output path. |ok| will be true if the printing completed
  // successfully or false otherwise.
  ///
  /*--cef()--*/
  virtual void OnPdfPrintFinished(const CefString& path, bool ok) =0;
};


///
// Callback interface for CefBrowserHost::DownloadImage. The methods of this
// class will be called on the browser process UI thread.
///
/*--cef(source=client)--*/
class CefDownloadImageCallback : public virtual CefBase {
 public:
  ///
  // Method that will be executed when the image download has completed.
  // |image_url| is the URL that was downloaded and |http_status_code| is the
  // resulting HTTP status code. |image| is the resulting image, possibly at
  // multiple scale factors, or empty if the download failed.
  ///
  /*--cef(optional_param=image)--*/
  virtual void OnDownloadImageFinished(
     const CefString& image_url,
     int http_status_code,
     CefRefPtr<CefImage> image) =0;
};


///
// Class used to represent the browser process aspects of a browser window. The
// methods of this class can only be called in the browser process. They may be
// called on any thread in that process unless otherwise indicated in the
// comments.
///
/*--cef(source=library)--*/
class CefBrowserHost : public virtual CefBase {
 public:
  typedef cef_drag_operations_mask_t DragOperationsMask;
  typedef cef_file_dialog_mode_t FileDialogMode;
  typedef cef_mouse_button_type_t MouseButtonType;
  typedef cef_paint_element_type_t PaintElementType;

  ///
  // Create a new browser window using the window parameters specified by
  // |windowInfo|. All values will be copied internally and the actual window
  // will be created on the UI thread. If |request_context| is empty the
  // global request context will be used. This method can be called on any
  // browser process thread and will not block.
  ///
  /*--cef(optional_param=client,optional_param=url,
          optional_param=request_context)--*/
  static bool CreateBrowser(const CefWindowInfo& windowInfo,
                            CefRefPtr<CefClient> client,
                            const CefString& url,
                            const CefBrowserSettings& settings,
                            CefRefPtr<CefRequestContext> request_context);

  ///
  // Create a new browser window using the window parameters specified by
  // |windowInfo|. If |request_context| is empty the global request context
  // will be used. This method can only be called on the browser process UI
  // thread.
  ///
  /*--cef(optional_param=client,optional_param=url,
          optional_param=request_context)--*/
  static CefRefPtr<CefBrowser> CreateBrowserSync(
      const CefWindowInfo& windowInfo,
      CefRefPtr<CefClient> client,
      const CefString& url,
      const CefBrowserSettings& settings,
      CefRefPtr<CefRequestContext> request_context);

  ///
  // Returns the hosted browser object.
  ///
  /*--cef()--*/
  virtual CefRefPtr<CefBrowser> GetBrowser() =0;

  ///
  // Request that the browser close. The JavaScript 'onbeforeunload' event will
  // be fired. If |force_close| is false the event handler, if any, will be
  // allowed to prompt the user and the user can optionally cancel the close.
  // If |force_close| is true the prompt will not be displayed and the close
  // will proceed. Results in a call to CefLifeSpanHandler::DoClose() if the
  // event handler allows the close or if |force_close| is true. See
  // CefLifeSpanHandler::DoClose() documentation for additional usage
  // information.
  ///
  /*--cef()--*/
  virtual void CloseBrowser(bool force_close) =0;

  ///
  // Helper for closing a browser. Call this method from the top-level window
  // close handler. Internally this calls CloseBrowser(false) if the close has
  // not yet been initiated. This method returns false while the close is
  // pending and true after the close has completed. See CloseBrowser() and
  // CefLifeSpanHandler::DoClose() documentation for additional usage
  // information. This method must be called on the browser process UI thread.
  ///
  /*--cef()--*/
  virtual bool TryCloseBrowser() =0;

  ///
  // Set whether the browser is focused.
  ///
  /*--cef()--*/
  virtual void SetFocus(bool focus) =0;

  ///
  // Retrieve the window handle for this browser. If this browser is wrapped in
  // a CefBrowserView this method should be called on the browser process UI
  // thread and it will return the handle for the top-level native window.
  ///
  /*--cef()--*/
  virtual CefWindowHandle GetWindowHandle() =0;

  ///
  // Retrieve the window handle of the browser that opened this browser. Will
  // return NULL for non-popup windows or if this browser is wrapped in a
  // CefBrowserView. This method can be used in combination with custom handling
  // of modal windows. 
  ///
  /*--cef()--*/
  virtual CefWindowHandle GetOpenerWindowHandle() =0;

  ///
  // Returns true if this browser is wrapped in a CefBrowserView.
  ///
  /*--cef()--*/
  virtual bool HasView() =0;

  ///
  // Returns the client for this browser.
  ///
  /*--cef()--*/
  virtual CefRefPtr<CefClient> GetClient() =0;

  ///
  // Returns the request context for this browser.
  ///
  /*--cef()--*/
  virtual CefRefPtr<CefRequestContext> GetRequestContext() =0;

  ///
  // Get the current zoom level. The default zoom level is 0.0. This method can
  // only be called on the UI thread.
  ///
  /*--cef()--*/
  virtual double GetZoomLevel() =0;

  ///
  // Change the zoom level to the specified value. Specify 0.0 to reset the
  // zoom level. If called on the UI thread the change will be applied
  // immediately. Otherwise, the change will be applied asynchronously on the
  // UI thread.
  ///
  /*--cef()--*/
  virtual void SetZoomLevel(double zoomLevel) =0;

  ///
  // Call to run a file chooser dialog. Only a single file chooser dialog may be
  // pending at any given time. |mode| represents the type of dialog to display.
  // |title| to the title to be used for the dialog and may be empty to show the
  // default title ("Open" or "Save" depending on the mode). |default_file_path|
  // is the path with optional directory and/or file name component that will be
  // initially selected in the dialog. |accept_filters| are used to restrict the
  // selectable file types and may any combination of (a) valid lower-cased MIME
  // types (e.g. "text/*" or "image/*"), (b) individual file extensions (e.g.
  // ".txt" or ".png"), or (c) combined description and file extension delimited
  // using "|" and ";" (e.g. "Image Types|.png;.gif;.jpg").
  // |selected_accept_filter| is the 0-based index of the filter that will be
  // selected by default. |callback| will be executed after the dialog is
  // dismissed or immediately if another dialog is already pending. The dialog
  // will be initiated asynchronously on the UI thread.
  ///
  /*--cef(optional_param=title,optional_param=default_file_path,
          optional_param=accept_filters,index_param=selected_accept_filter)--*/
  virtual void RunFileDialog(FileDialogMode mode,
                             const CefString& title,
                             const CefString& default_file_path,
                             const std::vector<CefString>& accept_filters,
                             int selected_accept_filter,
                             CefRefPtr<CefRunFileDialogCallback> callback) =0;

  ///
  // Download the file at |url| using CefDownloadHandler.
  ///
  /*--cef()--*/
  virtual void StartDownload(const CefString& url) =0;

  ///
  // Download |image_url| and execute |callback| on completion with the images
  // received from the renderer. If |is_favicon| is true then cookies are not
  // sent and not accepted during download. Images with density independent
  // pixel (DIP) sizes larger than |max_image_size| are filtered out from the
  // image results. Versions of the image at different scale factors may be
  // downloaded up to the maximum scale factor supported by the system. If there
  // are no image results <= |max_image_size| then the smallest image is resized
  // to |max_image_size| and is the only result. A |max_image_size| of 0 means
  // unlimited. If |bypass_cache| is true then |image_url| is requested from the
  // server even if it is present in the browser cache.
  ///
  /*--cef()--*/
  virtual void DownloadImage(const CefString& image_url,
                             bool is_favicon,
                             uint32 max_image_size,
                             bool bypass_cache,
                             CefRefPtr<CefDownloadImageCallback> callback) =0;

  ///
  // Print the current browser contents.
  ///
  /*--cef()--*/
  virtual void Print() =0;

  ///
  // Print the current browser contents to the PDF file specified by |path| and
  // execute |callback| on completion. The caller is responsible for deleting
  // |path| when done. For PDF printing to work on Linux you must implement the
  // CefPrintHandler::GetPdfPaperSize method.
  ///
  /*--cef(optional_param=callback)--*/
  virtual void PrintToPDF(const CefString& path,
                          const CefPdfPrintSettings& settings,
                          CefRefPtr<CefPdfPrintCallback> callback) =0;

  ///
  // Search for |searchText|. |identifier| can be used to have multiple searches
  // running simultaniously. |forward| indicates whether to search forward or
  // backward within the page. |matchCase| indicates whether the search should
  // be case-sensitive. |findNext| indicates whether this is the first request
  // or a follow-up. The CefFindHandler instance, if any, returned via
  // CefClient::GetFindHandler will be called to report find results.
  ///
  /*--cef()--*/
  virtual void Find(int identifier, const CefString& searchText,
                    bool forward, bool matchCase, bool findNext) =0;

  ///
  // Cancel all searches that are currently going on.
  ///
  /*--cef()--*/
  virtual void StopFinding(bool clearSelection) =0;

  ///
  // Open developer tools (DevTools) in its own browser. The DevTools browser
  // will remain associated with this browser. If the DevTools browser is
  // already open then it will be focused, in which case the |windowInfo|,
  // |client| and |settings| parameters will be ignored. If |inspect_element_at|
  // is non-empty then the element at the specified (x,y) location will be
  // inspected. The |windowInfo| parameter will be ignored if this browser is
  // wrapped in a CefBrowserView.
  ///
  /*--cef(optional_param=windowInfo,optional_param=client,
          optional_param=settings,optional_param=inspect_element_at)--*/
  virtual void ShowDevTools(const CefWindowInfo& windowInfo,
                            CefRefPtr<CefClient> client,
                            const CefBrowserSettings& settings,
                            const CefPoint& inspect_element_at) =0;

  ///
  // Explicitly close the associated DevTools browser, if any.
  ///
  /*--cef()--*/
  virtual void CloseDevTools() =0;

  ///
  // Returns true if this browser currently has an associated DevTools browser.
  // Must be called on the browser process UI thread.
  ///
  /*--cef()--*/
  virtual bool HasDevTools() =0;

  ///
  // Retrieve a snapshot of current navigation entries as values sent to the
  // specified visitor. If |current_only| is true only the current navigation
  // entry will be sent, otherwise all navigation entries will be sent.
  ///
  /*--cef()--*/
  virtual void GetNavigationEntries(
      CefRefPtr<CefNavigationEntryVisitor> visitor,
      bool current_only) =0;

  ///
  // Set whether mouse cursor change is disabled.
  ///
  /*--cef()--*/
  virtual void SetMouseCursorChangeDisabled(bool disabled) =0;

  ///
  // Returns true if mouse cursor change is disabled.
  ///
  /*--cef()--*/
  virtual bool IsMouseCursorChangeDisabled() =0;

  ///
  // If a misspelled word is currently selected in an editable node calling
  // this method will replace it with the specified |word|.
  ///
  /*--cef()--*/
  virtual void ReplaceMisspelling(const CefString& word) =0;

  ///
  // Add the specified |word| to the spelling dictionary.
  ///
  /*--cef()--*/
  virtual void AddWordToDictionary(const CefString& word) =0;

  ///
  // Returns true if window rendering is disabled.
  ///
  /*--cef()--*/
  virtual bool IsWindowRenderingDisabled() =0;

  ///
  // Notify the browser that the widget has been resized. The browser will first
  // call CefRenderHandler::GetViewRect to get the new size and then call
  // CefRenderHandler::OnPaint asynchronously with the updated regions. This
  // method is only used when window rendering is disabled.
  ///
  /*--cef()--*/
  virtual void WasResized() =0;

  ///
  // Notify the browser that it has been hidden or shown. Layouting and
  // CefRenderHandler::OnPaint notification will stop when the browser is
  // hidden. This method is only used when window rendering is disabled.
  ///
  /*--cef()--*/
  virtual void WasHidden(bool hidden) =0;

  ///
  // Send a notification to the browser that the screen info has changed. The
  // browser will then call CefRenderHandler::GetScreenInfo to update the
  // screen information with the new values. This simulates moving the webview
  // window from one display to another, or changing the properties of the
  // current display. This method is only used when window rendering is
  // disabled.
  ///
  /*--cef()--*/
  virtual void NotifyScreenInfoChanged() =0;

  ///
  // Invalidate the view. The browser will call CefRenderHandler::OnPaint
  // asynchronously. This method is only used when window rendering is
  // disabled.
  ///
  /*--cef()--*/
  virtual void Invalidate(PaintElementType type) =0;

  ///
  // Send a key event to the browser.
  ///
  /*--cef()--*/
  virtual void SendKeyEvent(const CefKeyEvent& event) =0;

  ///
  // Send a mouse click event to the browser. The |x| and |y| coordinates are
  // relative to the upper-left corner of the view.
  ///
  /*--cef()--*/
  virtual void SendMouseClickEvent(const CefMouseEvent& event,
                                   MouseButtonType type,
                                   bool mouseUp, int clickCount) =0;

  ///
  // Send a mouse move event to the browser. The |x| and |y| coordinates are
  // relative to the upper-left corner of the view.
  ///
  /*--cef()--*/
  virtual void SendMouseMoveEvent(const CefMouseEvent& event,
                                  bool mouseLeave) =0;

  ///
  // Send a mouse wheel event to the browser. The |x| and |y| coordinates are
  // relative to the upper-left corner of the view. The |deltaX| and |deltaY|
  // values represent the movement delta in the X and Y directions respectively.
  // In order to scroll inside select popups with window rendering disabled
  // CefRenderHandler::GetScreenPoint should be implemented properly.
  ///
  /*--cef()--*/
  virtual void SendMouseWheelEvent(const CefMouseEvent& event,
                                   int deltaX, int deltaY) =0;

  ///
  // Send a focus event to the browser.
  ///
  /*--cef()--*/
  virtual void SendFocusEvent(bool setFocus) =0;

  ///
  // Send a capture lost event to the browser.
  ///
  /*--cef()--*/
  virtual void SendCaptureLostEvent() =0;

  ///
  // Notify the browser that the window hosting it is about to be moved or
  // resized. This method is only used on Windows and Linux.
  ///
  /*--cef()--*/
  virtual void NotifyMoveOrResizeStarted() =0;

  ///
  // Returns the maximum rate in frames per second (fps) that CefRenderHandler::
  // OnPaint will be called for a windowless browser. The actual fps may be
  // lower if the browser cannot generate frames at the requested rate. The
  // minimum value is 1 and the maximum value is 60 (default 30). This method
  // can only be called on the UI thread.
  ///
  /*--cef()--*/
  virtual int GetWindowlessFrameRate() =0;

  ///
  // Set the maximum rate in frames per second (fps) that CefRenderHandler::
  // OnPaint will be called for a windowless browser. The actual fps may be
  // lower if the browser cannot generate frames at the requested rate. The
  // minimum value is 1 and the maximum value is 60 (default 30). Can also be
  // set at browser creation via CefBrowserSettings.windowless_frame_rate.
  ///
  /*--cef()--*/
  virtual void SetWindowlessFrameRate(int frame_rate) =0;

  ///
  // Get the NSTextInputContext implementation for enabling IME on Mac when
  // window rendering is disabled.
  ///
  /*--cef(default_retval=NULL)--*/
  virtual CefTextInputContext GetNSTextInputContext() =0;

  ///
  // Handles a keyDown event prior to passing it through the NSTextInputClient
  // machinery.
  ///
  /*--cef()--*/
  virtual void HandleKeyEventBeforeTextInputClient(CefEventHandle keyEvent) =0;

  ///
  // Performs any additional actions after NSTextInputClient handles the event.
  ///
  /*--cef()--*/
  virtual void HandleKeyEventAfterTextInputClient(CefEventHandle keyEvent) =0;

  ///
  // Call this method when the user drags the mouse into the web view (before
  // calling DragTargetDragOver/DragTargetLeave/DragTargetDrop).
  // |drag_data| should not contain file contents as this type of data is not
  // allowed to be dragged into the web view. File contents can be removed using
  // CefDragData::ResetFileContents (for example, if |drag_data| comes from
  // CefRenderHandler::StartDragging).
  // This method is only used when window rendering is disabled.
  ///
  /*--cef()--*/
  virtual void DragTargetDragEnter(CefRefPtr<CefDragData> drag_data,
                                  const CefMouseEvent& event,
                                  DragOperationsMask allowed_ops) =0;

  ///
  // Call this method each time the mouse is moved across the web view during
  // a drag operation (after calling DragTargetDragEnter and before calling
  // DragTargetDragLeave/DragTargetDrop).
  // This method is only used when window rendering is disabled.
  ///
  /*--cef()--*/
  virtual void DragTargetDragOver(const CefMouseEvent& event,
                                  DragOperationsMask allowed_ops) =0;

  ///
  // Call this method when the user drags the mouse out of the web view (after
  // calling DragTargetDragEnter).
  // This method is only used when window rendering is disabled.
  ///
  /*--cef()--*/
  virtual void DragTargetDragLeave() =0;

  ///
  // Call this method when the user completes the drag operation by dropping
  // the object onto the web view (after calling DragTargetDragEnter).
  // The object being dropped is |drag_data|, given as an argument to
  // the previous DragTargetDragEnter call.
  // This method is only used when window rendering is disabled.
  ///
  /*--cef()--*/
  virtual void DragTargetDrop(const CefMouseEvent& event) =0;

  ///
  // Call this method when the drag operation started by a
  // CefRenderHandler::StartDragging call has ended either in a drop or
  // by being cancelled. |x| and |y| are mouse coordinates relative to the
  // upper-left corner of the view. If the web view is both the drag source
  // and the drag target then all DragTarget* methods should be called before
  // DragSource* mthods.
  // This method is only used when window rendering is disabled.
  ///
  /*--cef()--*/
  virtual void DragSourceEndedAt(int x, int y, DragOperationsMask op) =0;

  ///
  // Call this method when the drag operation started by a
  // CefRenderHandler::StartDragging call has completed. This method may be
  // called immediately without first calling DragSourceEndedAt to cancel a
  // drag operation. If the web view is both the drag source and the drag
  // target then all DragTarget* methods should be called before DragSource*
  // mthods.
  // This method is only used when window rendering is disabled.
  ///
  /*--cef()--*/
  virtual void DragSourceSystemDragEnded() =0;

  ///
  // Returns the current visible navigation entry for this browser. This method
  // can only be called on the UI thread.
  ///
  /*--cef()--*/
  virtual CefRefPtr<CefNavigationEntry> GetVisibleNavigationEntry() =0;
};

#endif  // CEF_INCLUDE_CEF_BROWSER_H_
