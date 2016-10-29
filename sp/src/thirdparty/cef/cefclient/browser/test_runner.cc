// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/browser/test_runner.h"

#include <sstream>

#include "include/base/cef_bind.h"
#include "include/cef_parser.h"
#include "include/cef_task.h"
#include "include/cef_trace.h"
#include "include/cef_web_plugin.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_stream_resource_handler.h"
#include "cefclient/browser/binding_test.h"
#include "cefclient/browser/dialog_test.h"
#include "cefclient/browser/drm_test.h"
#include "cefclient/browser/main_context.h"
#include "cefclient/browser/preferences_test.h"
#include "cefclient/browser/resource.h"
#include "cefclient/browser/resource_util.h"
#include "cefclient/browser/response_filter_test.h"
#include "cefclient/browser/root_window_manager.h"
#include "cefclient/browser/scheme_test.h"
#include "cefclient/browser/urlrequest_test.h"
#include "cefclient/browser/window_test.h"

namespace client {
namespace test_runner {

namespace {

const char kTestOrigin[] = "http://tests/";

// Replace all instances of |from| with |to| in |str|.
std::string StringReplace(const std::string& str, const std::string& from,
                          const std::string& to) {
  std::string result = str;
  std::string::size_type pos = 0;
  std::string::size_type from_len = from.length();
  std::string::size_type to_len = to.length();
  do {
    pos = result.find(from, pos);
    if (pos != std::string::npos) {
      result.replace(pos, from_len, to);
      pos += to_len;
    }
  } while (pos != std::string::npos);
  return result;
}

void RunGetSourceTest(CefRefPtr<CefBrowser> browser) {
  class Visitor : public CefStringVisitor {
   public:
    explicit Visitor(CefRefPtr<CefBrowser> browser) : browser_(browser) {}
    virtual void Visit(const CefString& string) OVERRIDE {
      std::string source = StringReplace(string, "<", "&lt;");
      source = StringReplace(source, ">", "&gt;");
      std::stringstream ss;
      ss << "<html><body bgcolor=\"white\">Source:<pre>" << source <<
            "</pre></body></html>";
      browser_->GetMainFrame()->LoadString(ss.str(), "http://tests/getsource");
    }
   private:
    CefRefPtr<CefBrowser> browser_;
    IMPLEMENT_REFCOUNTING(Visitor);
  };

  browser->GetMainFrame()->GetSource(new Visitor(browser));
}

void RunGetTextTest(CefRefPtr<CefBrowser> browser) {
  class Visitor : public CefStringVisitor {
   public:
    explicit Visitor(CefRefPtr<CefBrowser> browser) : browser_(browser) {}
    virtual void Visit(const CefString& string) OVERRIDE {
      std::string text = StringReplace(string, "<", "&lt;");
      text = StringReplace(text, ">", "&gt;");
      std::stringstream ss;
      ss << "<html><body bgcolor=\"white\">Text:<pre>" << text <<
            "</pre></body></html>";
      browser_->GetMainFrame()->LoadString(ss.str(), "http://tests/gettext");
    }
   private:
    CefRefPtr<CefBrowser> browser_;
    IMPLEMENT_REFCOUNTING(Visitor);
  };

  browser->GetMainFrame()->GetText(new Visitor(browser));
}

void RunRequestTest(CefRefPtr<CefBrowser> browser) {
  // Create a new request
  CefRefPtr<CefRequest> request(CefRequest::Create());

  // Set the request URL
  request->SetURL("http://tests/request");

  // Add post data to the request.  The correct method and content-
  // type headers will be set by CEF.
  CefRefPtr<CefPostDataElement> postDataElement(CefPostDataElement::Create());
  std::string data = "arg1=val1&arg2=val2";
  postDataElement->SetToBytes(data.length(), data.c_str());
  CefRefPtr<CefPostData> postData(CefPostData::Create());
  postData->AddElement(postDataElement);
  request->SetPostData(postData);

  // Add a custom header
  CefRequest::HeaderMap headerMap;
  headerMap.insert(
      std::make_pair("X-My-Header", "My Header Value"));
  request->SetHeaderMap(headerMap);

  // Load the request
  browser->GetMainFrame()->LoadRequest(request);
}

void RunNewWindowTest(CefRefPtr<CefBrowser> browser) {
  MainContext::Get()->GetRootWindowManager()->CreateRootWindow(
      true,             // Show controls.
      browser->GetHost()->IsWindowRenderingDisabled(),
      CefRect(),        // Use default system size.
      std::string());   // Use default URL.
}

void RunPopupWindowTest(CefRefPtr<CefBrowser> browser) {
  browser->GetMainFrame()->ExecuteJavaScript(
      "window.open('http://www.google.com');", "about:blank", 0);
}

void RunPluginInfoTest(CefRefPtr<CefBrowser> browser) {
  class Visitor : public CefWebPluginInfoVisitor {
   public:
    explicit Visitor(CefRefPtr<CefBrowser> browser)
        : browser_(browser) {
      html_ = "<html><head><title>Plugin Info Test</title></head>"
              "<body bgcolor=\"white\">"
              "\n<b>Installed plugins:</b>";
    }
    ~Visitor() {
      html_ += "\n</body></html>";

      // Load the html in the browser.
      browser_->GetMainFrame()->LoadString(html_, "http://tests/plugin_info");
    }

    virtual bool Visit(CefRefPtr<CefWebPluginInfo> info, int count, int total)
        OVERRIDE {
      html_ +=  "\n<br/><br/>Name: " + info->GetName().ToString() +
                "\n<br/>Description: " + info->GetDescription().ToString() +
                "\n<br/>Version: " + info->GetVersion().ToString() +
                "\n<br/>Path: " + info->GetPath().ToString();
      return true;
    }

   private:
    std::string html_;
    CefRefPtr<CefBrowser> browser_;
    IMPLEMENT_REFCOUNTING(Visitor);
  };

  CefVisitWebPluginInfo(new Visitor(browser));
}

void ModifyZoom(CefRefPtr<CefBrowser> browser, double delta) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute on the UI thread.
    CefPostTask(TID_UI, base::Bind(&ModifyZoom, browser, delta));
    return;
  }

  browser->GetHost()->SetZoomLevel(
      browser->GetHost()->GetZoomLevel() + delta);
}

const char kPrompt[] = "Prompt.";
const char kPromptFPS[] = "FPS";
const char kPromptDSF[] = "DSF";

// Handles execution of prompt results.
class PromptHandler : public CefMessageRouterBrowserSide::Handler {
 public:
  PromptHandler() {}

  // Called due to cefQuery execution.
  virtual bool OnQuery(CefRefPtr<CefBrowser> browser,
                       CefRefPtr<CefFrame> frame,
                       int64 query_id,
                       const CefString& request,
                       bool persistent,
                       CefRefPtr<Callback> callback) OVERRIDE {
    // Parse |request| which takes the form "Prompt.[type]:[value]".
    const std::string& request_str = request;
    if (request_str.find(kPrompt) != 0)
      return false;

    std::string type = request_str.substr(sizeof(kPrompt) - 1);
    size_t delim = type.find(':');
    if (delim == std::string::npos)
      return false;

    const std::string& value = type.substr(delim + 1);
    type = type.substr(0, delim);

    // Canceling the prompt dialog returns a value of "null".
    if (value != "null") {
      if (type == kPromptFPS)
        SetFPS(browser, atoi(value.c_str()));
      else if (type == kPromptDSF)
        SetDSF(browser, static_cast<float>(atof(value.c_str())));
    }

    // Nothing is done with the response.
    callback->Success(CefString());
    return true;
  }

 private:
  void SetFPS(CefRefPtr<CefBrowser> browser, int fps) {
    if (fps <= 0) {
      // Reset to the default value.
      CefBrowserSettings settings;
      MainContext::Get()->PopulateBrowserSettings(&settings);
      fps = settings.windowless_frame_rate;
    }

    browser->GetHost()->SetWindowlessFrameRate(fps);
  }

  void SetDSF(CefRefPtr<CefBrowser> browser, float dsf) {
    MainMessageLoop::Get()->PostClosure(
        base::Bind(&PromptHandler::SetDSFOnMainThread, browser, dsf));
  }

  static void SetDSFOnMainThread(CefRefPtr<CefBrowser> browser, float dsf) {
    RootWindow::GetForBrowser(browser->GetIdentifier())->
        SetDeviceScaleFactor(dsf);
  }
};

void Prompt(CefRefPtr<CefBrowser> browser,
            const std::string& type,
            const std::string& label,
            const std::string& default_value) {
  // Prompt the user for a new value. Works as follows:
  // 1. Show a prompt() dialog via JavaScript.
  // 2. Pass the result to window.cefQuery().
  // 3. Handle the result in PromptHandler::OnQuery.
  const std::string& code =
      "window.cefQuery({'request': '" + std::string(kPrompt) + type +
      ":' + prompt('" + label + "', '" + default_value + "')});";
  browser->GetMainFrame()->ExecuteJavaScript(
      code, browser->GetMainFrame()->GetURL(), 0);
}

void PromptFPS(CefRefPtr<CefBrowser> browser) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute on the UI thread.
    CefPostTask(TID_UI, base::Bind(&PromptFPS, browser));
    return;
  }

  // Format the default value string.
  std::stringstream ss;
  ss << browser->GetHost()->GetWindowlessFrameRate();

  Prompt(browser, kPromptFPS, "Enter FPS", ss.str());
}

void PromptDSF(CefRefPtr<CefBrowser> browser) {
  if (!MainMessageLoop::Get()->RunsTasksOnCurrentThread()) {
    // Execute on the main thread.
    MainMessageLoop::Get()->PostClosure(base::Bind(&PromptDSF, browser));
    return;
  }

  // Format the default value string.
  std::stringstream ss;
  ss << RootWindow::GetForBrowser(browser->GetIdentifier())->
            GetDeviceScaleFactor();

  Prompt(browser, kPromptDSF, "Enter Device Scale Factor", ss.str());
}

void BeginTracing() {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute on the UI thread.
    CefPostTask(TID_UI, base::Bind(&BeginTracing));
    return;
  }

  CefBeginTracing(CefString(), NULL);
}

void EndTracing(CefRefPtr<CefBrowser> browser) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute on the UI thread.
    CefPostTask(TID_UI, base::Bind(&EndTracing, browser));
    return;
  }

  class Client : public CefEndTracingCallback,
                 public CefRunFileDialogCallback {
   public:
    explicit Client(CefRefPtr<CefBrowser> browser)
        : browser_(browser) {
      RunDialog();
    }

    void RunDialog() {
      static const char kDefaultFileName[] = "trace.txt";
      std::string path = MainContext::Get()->GetDownloadPath(kDefaultFileName);
      if (path.empty())
        path = kDefaultFileName;

      // Results in a call to OnFileDialogDismissed.
      browser_->GetHost()->RunFileDialog(
          static_cast<cef_file_dialog_mode_t>(
              FILE_DIALOG_SAVE | FILE_DIALOG_OVERWRITEPROMPT_FLAG),
          CefString(),  // title
          path,
          std::vector<CefString>(),  // accept_filters
          0,  // selected_accept_filter
          this);
    }

    void OnFileDialogDismissed(
        int selected_accept_filter,
        const std::vector<CefString>& file_paths) OVERRIDE {
      if (!file_paths.empty()) {
        // File selected. Results in a call to OnEndTracingComplete.
        CefEndTracing(file_paths.front(), this);
      } else {
        // No file selected. Discard the trace data.
        CefEndTracing(CefString(), NULL);
      }
    }

    void OnEndTracingComplete(
        const CefString& tracing_file) OVERRIDE {
      Alert(browser_,
            "File \"" + tracing_file.ToString() + "\" saved successfully.");
    }

   private:
    CefRefPtr<CefBrowser> browser_;

    IMPLEMENT_REFCOUNTING(Client);
  };

  new Client(browser);
}

void PrintToPDF(CefRefPtr<CefBrowser> browser) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute on the UI thread.
    CefPostTask(TID_UI, base::Bind(&PrintToPDF, browser));
    return;
  }

  class Client : public CefPdfPrintCallback,
                 public CefRunFileDialogCallback {
   public:
    explicit Client(CefRefPtr<CefBrowser> browser)
        : browser_(browser) {
      RunDialog();
    }

    void RunDialog() {
      static const char kDefaultFileName[] = "output.pdf";
      std::string path = MainContext::Get()->GetDownloadPath(kDefaultFileName);
      if (path.empty())
        path = kDefaultFileName;

      std::vector<CefString> accept_filters;
      accept_filters.push_back(".pdf");

      // Results in a call to OnFileDialogDismissed.
      browser_->GetHost()->RunFileDialog(
          static_cast<cef_file_dialog_mode_t>(
              FILE_DIALOG_SAVE | FILE_DIALOG_OVERWRITEPROMPT_FLAG),
          CefString(),  // title
          path,
          accept_filters,
          0,  // selected_accept_filter
          this);
    }

    void OnFileDialogDismissed(
        int selected_accept_filter,
        const std::vector<CefString>& file_paths) OVERRIDE {
      if (!file_paths.empty()) {
        CefPdfPrintSettings settings;

        // Show the URL in the footer.
        settings.header_footer_enabled = true;
        CefString(&settings.header_footer_url) =
            browser_->GetMainFrame()->GetURL();

        // Print to the selected PDF file.
        browser_->GetHost()->PrintToPDF(file_paths[0], settings, this);
      }
    }

    void OnPdfPrintFinished(const CefString& path, bool ok) OVERRIDE {
      Alert(browser_, "File \"" + path.ToString() +"\" " +
          (ok ? "saved successfully." : "failed to save."));
    }

   private:
    CefRefPtr<CefBrowser> browser_;

    IMPLEMENT_REFCOUNTING(Client);
  };

  new Client(browser);
}

void RunOtherTests(CefRefPtr<CefBrowser> browser) {
  browser->GetMainFrame()->LoadURL("http://tests/other_tests");
}

// Provider that dumps the request contents.
class RequestDumpResourceProvider : public CefResourceManager::Provider {
 public:
  explicit RequestDumpResourceProvider(const std::string& url)
    : url_(url) {
    DCHECK(!url.empty());
  }

  bool OnRequest(scoped_refptr<CefResourceManager::Request> request) OVERRIDE {
    CEF_REQUIRE_IO_THREAD();

    const std::string& url = request->url();
    if (url != url_) {
      // Not handled by this provider.
      return false;
    }

    const std::string& dump = DumpRequestContents(request->request());
    std::string str = "<html><body bgcolor=\"white\"><pre>" + dump +
                      "</pre></body></html>";
    CefRefPtr<CefStreamReader> stream =
        CefStreamReader::CreateForData(
            static_cast<void*>(const_cast<char*>(str.c_str())),
            str.size());
    DCHECK(stream.get());
    request->Continue(new CefStreamResourceHandler("text/html", stream));
    return true;
  }

 private:
  std::string url_;

  DISALLOW_COPY_AND_ASSIGN(RequestDumpResourceProvider);
};

// Add a file extension to |url| if none is currently specified.
std::string RequestUrlFilter(const std::string& url) {
  if (url.find(kTestOrigin) != 0U) {
    // Don't filter anything outside of the test origin.
    return url;
  }

  // Identify where the query or fragment component, if any, begins.
  size_t suffix_pos = url.find('?');
  if (suffix_pos == std::string::npos)
    suffix_pos = url.find('#');

  std::string url_base, url_suffix;
  if (suffix_pos == std::string::npos) {
    url_base = url;
  } else {
    url_base = url.substr(0, suffix_pos);
    url_suffix = url.substr(suffix_pos);
  }

  // Identify the last path component.
  size_t path_pos = url_base.rfind('/');
  if (path_pos == std::string::npos)
    return url;

  const std::string& path_component = url_base.substr(path_pos);

  // Identify if a file extension is currently specified.
  size_t ext_pos = path_component.rfind(".");
  if (ext_pos != std::string::npos)
    return url;

  // Rebuild the URL with a file extension.
  return url_base + ".html" + url_suffix;
}

}  // namespace

void RunTest(CefRefPtr<CefBrowser> browser, int id) {
  if (!browser)
    return;

  switch (id) {
    case ID_TESTS_GETSOURCE:
       RunGetSourceTest(browser);
      break;
    case ID_TESTS_GETTEXT:
      RunGetTextTest(browser);
      break;
    case ID_TESTS_WINDOW_NEW:
      RunNewWindowTest(browser);
      break;
    case ID_TESTS_WINDOW_POPUP:
      RunPopupWindowTest(browser);
      break;
    case ID_TESTS_REQUEST:
      RunRequestTest(browser);
      break;
    case ID_TESTS_PLUGIN_INFO:
      RunPluginInfoTest(browser);
      break;
    case ID_TESTS_ZOOM_IN:
      ModifyZoom(browser, 0.5);
      break;
    case ID_TESTS_ZOOM_OUT:
      ModifyZoom(browser, -0.5);
      break;
    case ID_TESTS_ZOOM_RESET:
      browser->GetHost()->SetZoomLevel(0.0);
      break;
    case ID_TESTS_OSR_FPS:
      PromptFPS(browser);
      break;
    case ID_TESTS_OSR_DSF:
      PromptDSF(browser);
      break;
    case ID_TESTS_TRACING_BEGIN:
      BeginTracing();
      break;
    case ID_TESTS_TRACING_END:
      EndTracing(browser);
      break;
    case ID_TESTS_PRINT:
      browser->GetHost()->Print();
      break;
    case ID_TESTS_PRINT_TO_PDF:
      PrintToPDF(browser);
      break;
    case ID_TESTS_OTHER_TESTS:
      RunOtherTests(browser);
      break;
  }
}

std::string DumpRequestContents(CefRefPtr<CefRequest> request) {
  std::stringstream ss;

  ss << "URL: " << std::string(request->GetURL());
  ss << "\nMethod: " << std::string(request->GetMethod());

  CefRequest::HeaderMap headerMap;
  request->GetHeaderMap(headerMap);
  if (headerMap.size() > 0) {
    ss << "\nHeaders:";
    CefRequest::HeaderMap::const_iterator it = headerMap.begin();
    for (; it != headerMap.end(); ++it) {
      ss << "\n\t" << std::string((*it).first) << ": " <<
          std::string((*it).second);
    }
  }

  CefRefPtr<CefPostData> postData = request->GetPostData();
  if (postData.get()) {
    CefPostData::ElementVector elements;
    postData->GetElements(elements);
    if (elements.size() > 0) {
      ss << "\nPost Data:";
      CefRefPtr<CefPostDataElement> element;
      CefPostData::ElementVector::const_iterator it = elements.begin();
      for (; it != elements.end(); ++it) {
        element = (*it);
        if (element->GetType() == PDE_TYPE_BYTES) {
          // the element is composed of bytes
          ss << "\n\tBytes: ";
          if (element->GetBytesCount() == 0) {
            ss << "(empty)";
          } else {
            // retrieve the data.
            size_t size = element->GetBytesCount();
            char* bytes = new char[size];
            element->GetBytes(size, bytes);
            ss << std::string(bytes, size);
            delete [] bytes;
          }
        } else if (element->GetType() == PDE_TYPE_FILE) {
          ss << "\n\tFile: " << std::string(element->GetFile());
        }
      }
    }
  }

  return ss.str();
}

std::string GetDataURI(const std::string& data,
                       const std::string& mime_type) {
  return "data:" + mime_type + ";base64," +
      CefURIEncode(CefBase64Encode(data.data(), data.size()), false).ToString();
}

std::string GetErrorString(cef_errorcode_t code) {
  // Case condition that returns |code| as a string.
  #define CASE(code) case code: return #code

  switch (code) {
    CASE(ERR_NONE);
    CASE(ERR_FAILED);
    CASE(ERR_ABORTED);
    CASE(ERR_INVALID_ARGUMENT);
    CASE(ERR_INVALID_HANDLE);
    CASE(ERR_FILE_NOT_FOUND);
    CASE(ERR_TIMED_OUT);
    CASE(ERR_FILE_TOO_BIG);
    CASE(ERR_UNEXPECTED);
    CASE(ERR_ACCESS_DENIED);
    CASE(ERR_NOT_IMPLEMENTED);
    CASE(ERR_CONNECTION_CLOSED);
    CASE(ERR_CONNECTION_RESET);
    CASE(ERR_CONNECTION_REFUSED);
    CASE(ERR_CONNECTION_ABORTED);
    CASE(ERR_CONNECTION_FAILED);
    CASE(ERR_NAME_NOT_RESOLVED);
    CASE(ERR_INTERNET_DISCONNECTED);
    CASE(ERR_SSL_PROTOCOL_ERROR);
    CASE(ERR_ADDRESS_INVALID);
    CASE(ERR_ADDRESS_UNREACHABLE);
    CASE(ERR_SSL_CLIENT_AUTH_CERT_NEEDED);
    CASE(ERR_TUNNEL_CONNECTION_FAILED);
    CASE(ERR_NO_SSL_VERSIONS_ENABLED);
    CASE(ERR_SSL_VERSION_OR_CIPHER_MISMATCH);
    CASE(ERR_SSL_RENEGOTIATION_REQUESTED);
    CASE(ERR_CERT_COMMON_NAME_INVALID);
    CASE(ERR_CERT_DATE_INVALID);
    CASE(ERR_CERT_AUTHORITY_INVALID);
    CASE(ERR_CERT_CONTAINS_ERRORS);
    CASE(ERR_CERT_NO_REVOCATION_MECHANISM);
    CASE(ERR_CERT_UNABLE_TO_CHECK_REVOCATION);
    CASE(ERR_CERT_REVOKED);
    CASE(ERR_CERT_INVALID);
    CASE(ERR_CERT_END);
    CASE(ERR_INVALID_URL);
    CASE(ERR_DISALLOWED_URL_SCHEME);
    CASE(ERR_UNKNOWN_URL_SCHEME);
    CASE(ERR_TOO_MANY_REDIRECTS);
    CASE(ERR_UNSAFE_REDIRECT);
    CASE(ERR_UNSAFE_PORT);
    CASE(ERR_INVALID_RESPONSE);
    CASE(ERR_INVALID_CHUNKED_ENCODING);
    CASE(ERR_METHOD_NOT_SUPPORTED);
    CASE(ERR_UNEXPECTED_PROXY_AUTH);
    CASE(ERR_EMPTY_RESPONSE);
    CASE(ERR_RESPONSE_HEADERS_TOO_BIG);
    CASE(ERR_CACHE_MISS);
    CASE(ERR_INSECURE_RESPONSE);
    default:
      return "UNKNOWN";
  }
}

void SetupResourceManager(CefRefPtr<CefResourceManager> resource_manager) {
  if (!CefCurrentlyOn(TID_IO)) {
    // Execute on the browser IO thread.
    CefPostTask(TID_IO, base::Bind(SetupResourceManager, resource_manager));
    return;
  }

  const std::string& test_origin = kTestOrigin;

  // Add the URL filter.
  resource_manager->SetUrlFilter(base::Bind(RequestUrlFilter));

  // Add provider for resource dumps.
  resource_manager->AddProvider(
      new RequestDumpResourceProvider(test_origin + "request.html"),
      0, std::string());

  // Add provider for bundled resource files.
#if defined(OS_WIN)
  // Read resources from the binary.
  resource_manager->AddProvider(CreateBinaryResourceProvider(test_origin),
                                100, std::string());
#elif defined(OS_POSIX)
  // Read resources from a directory on disk.
  std::string resource_dir;
  if (GetResourceDir(resource_dir)) {
    resource_manager->AddDirectoryProvider(test_origin, resource_dir,
                                           100, std::string());
  }
#endif
}

void Alert(CefRefPtr<CefBrowser> browser, const std::string& message) {
  // Escape special characters in the message.
  std::string msg = StringReplace(message, "\\", "\\\\");
  msg = StringReplace(msg, "'", "\\'");

  // Execute a JavaScript alert().
  CefRefPtr<CefFrame> frame = browser->GetMainFrame();
  frame->ExecuteJavaScript("alert('" + msg + "');", frame->GetURL(), 0);
}

void CreateMessageHandlers(MessageHandlerSet& handlers) {
  handlers.insert(new PromptHandler);

  // Create the binding test handlers.
  binding_test::CreateMessageHandlers(handlers);

  // Create the dialog test handlers.
  dialog_test::CreateMessageHandlers(handlers);

  // Create the drm test handlers.
  drm_test::CreateMessageHandlers(handlers);

  // Create the preferences test handlers.
  preferences_test::CreateMessageHandlers(handlers);

  // Create the urlrequest test handlers.
  urlrequest_test::CreateMessageHandlers(handlers);

  // Create the window test handlers.
  window_test::CreateMessageHandlers(handlers);
}

void RegisterSchemeHandlers() {
  // Register the scheme handler.
  scheme_test::RegisterSchemeHandlers();
}

CefRefPtr<CefResponseFilter> GetResourceResponseFilter(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefRequest> request,
    CefRefPtr<CefResponse> response) {
  // Create the response filter.
  return response_filter_test::GetResourceResponseFilter(browser, frame,
                                                         request, response);
}

}  // namespace test_runner
}  // namespace client
