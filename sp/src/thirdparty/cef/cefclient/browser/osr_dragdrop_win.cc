// Copyright (c) 2014 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/browser/osr_dragdrop_win.h"

#if defined(CEF_USE_ATL)

#include <shellapi.h>
#include <shlobj.h>
#include <windowsx.h>

#include <algorithm>
#include <string>

#include "include/wrapper/cef_helpers.h"
#include "cefclient/browser/bytes_write_handler.h"
#include "cefclient/browser/resource.h"
#include "cefclient/browser/util_win.h"

namespace client {

namespace {

DWORD DragOperationToDropEffect(CefRenderHandler::DragOperation allowed_ops) {
  DWORD effect = DROPEFFECT_NONE;
  if (allowed_ops & DRAG_OPERATION_COPY)
    effect |= DROPEFFECT_COPY;
  if (allowed_ops & DRAG_OPERATION_LINK)
    effect |= DROPEFFECT_LINK;
  if (allowed_ops & DRAG_OPERATION_MOVE)
    effect |= DROPEFFECT_MOVE;
  return effect;
}

CefRenderHandler::DragOperationsMask DropEffectToDragOperation(DWORD effect) {
  DWORD operation = DRAG_OPERATION_NONE;
  if (effect & DROPEFFECT_COPY)
    operation |= DRAG_OPERATION_COPY;
  if (effect & DROPEFFECT_LINK)
    operation |= DRAG_OPERATION_LINK;
  if (effect & DROPEFFECT_MOVE)
    operation |= DRAG_OPERATION_MOVE;
  return static_cast<CefRenderHandler::DragOperationsMask>(operation);
}

CefMouseEvent ToMouseEvent(POINTL p, DWORD key_state, HWND hWnd) {
  CefMouseEvent ev;
  POINT screen_point = { p.x, p.y };
  ScreenToClient(hWnd, &screen_point);
  ev.x = screen_point.x;
  ev.y = screen_point.y;
  ev.modifiers = GetCefMouseModifiers(key_state);
  return ev;
}


void GetStorageForBytes(STGMEDIUM* storage, const void* data, size_t bytes) {
  HANDLE handle = GlobalAlloc(GPTR, static_cast<int>(bytes));
  if (handle) {
    memcpy(handle, data, bytes);
  }

  storage->hGlobal = handle;
  storage->tymed = TYMED_HGLOBAL;
  storage->pUnkForRelease = NULL;
}

template <typename T>
void GetStorageForString(STGMEDIUM* stgmed, const std::basic_string<T>& data) {
  GetStorageForBytes(stgmed, data.c_str(),
      (data.size() + 1) * sizeof(std::basic_string<T>::value_type));
}

void GetStorageForFileDescriptor(STGMEDIUM* storage,
                                 const std::wstring& file_name) {
  DCHECK(!file_name.empty());
  HANDLE hdata = GlobalAlloc(GPTR, sizeof(FILEGROUPDESCRIPTOR));

  FILEGROUPDESCRIPTOR* descriptor =
      reinterpret_cast<FILEGROUPDESCRIPTOR*>(hdata);
  descriptor->cItems = 1;
  descriptor->fgd[0].dwFlags = FD_LINKUI;
  wcsncpy_s(descriptor->fgd[0].cFileName, MAX_PATH, file_name.c_str(),
      std::min(file_name.size(), static_cast<size_t>(MAX_PATH - 1u)));

  storage->tymed = TYMED_HGLOBAL;
  storage->hGlobal = hdata;
  storage->pUnkForRelease = NULL;
}

// Helper method for converting from text/html to MS CF_HTML.
// Documentation for the CF_HTML format is available at
// http://msdn.microsoft.com/en-us/library/aa767917(VS.85).aspx
std::string HtmlToCFHtml(const std::string& html, const std::string& base_url) {
  if (html.empty())
    return std::string();

#define MAX_DIGITS 10
#define MAKE_NUMBER_FORMAT_1(digits) MAKE_NUMBER_FORMAT_2(digits)
#define MAKE_NUMBER_FORMAT_2(digits) "%0" #digits "u"
#define NUMBER_FORMAT MAKE_NUMBER_FORMAT_1(MAX_DIGITS)

  static const char* header = "Version:0.9\r\n"
                              "StartHTML:" NUMBER_FORMAT "\r\n"
                              "EndHTML:" NUMBER_FORMAT "\r\n"
                              "StartFragment:" NUMBER_FORMAT "\r\n"
                              "EndFragment:" NUMBER_FORMAT "\r\n";
  static const char* source_url_prefix = "SourceURL:";

  static const char* start_markup = "<html>\r\n<body>\r\n<!--StartFragment-->";
  static const char* end_markup = "<!--EndFragment-->\r\n</body>\r\n</html>";

  // Calculate offsets
  size_t start_html_offset = strlen(header) - strlen(NUMBER_FORMAT) * 4 +
      MAX_DIGITS * 4;
  if (!base_url.empty()) {
    start_html_offset += strlen(source_url_prefix) + base_url.length()
        + 2;  // Add 2 for \r\n.
  }
  size_t start_fragment_offset = start_html_offset + strlen(start_markup);
  size_t end_fragment_offset = start_fragment_offset + html.length();
  size_t end_html_offset = end_fragment_offset + strlen(end_markup);
  char raw_result[1024];
  _snprintf(raw_result, sizeof(1024),
      header,
      start_html_offset,
      end_html_offset,
      start_fragment_offset,
      end_fragment_offset);
  std::string result = raw_result;
  if (!base_url.empty()) {
    result.append(source_url_prefix);
    result.append(base_url);
    result.append("\r\n");
  }
  result.append(start_markup);
  result.append(html);
  result.append(end_markup);

#undef MAX_DIGITS
#undef MAKE_NUMBER_FORMAT_1
#undef MAKE_NUMBER_FORMAT_2
#undef NUMBER_FORMAT

  return result;
}

void CFHtmlExtractMetadata(const std::string& cf_html,
                           std::string* base_url,
                           size_t* html_start,
                           size_t* fragment_start,
                           size_t* fragment_end) {
  // Obtain base_url if present.
  if (base_url) {
    static std::string src_url_str("SourceURL:");
    size_t line_start = cf_html.find(src_url_str);
    if (line_start != std::string::npos) {
      size_t src_end = cf_html.find("\n", line_start);
      size_t src_start = line_start + src_url_str.length();
      if (src_end != std::string::npos && src_start != std::string::npos) {
        *base_url = cf_html.substr(src_start, src_end - src_start);
      }
    }
  }

  // Find the markup between "<!--StartFragment-->" and "<!--EndFragment-->".
  // If the comments cannot be found, like copying from OpenOffice Writer,
  // we simply fall back to using StartFragment/EndFragment bytecount values
  // to determine the fragment indexes.
  std::string cf_html_lower = cf_html;
  size_t markup_start = cf_html_lower.find("<html", 0);
  if (html_start) {
    *html_start = markup_start;
  }
  size_t tag_start = cf_html.find("<!--StartFragment", markup_start);
  if (tag_start == std::string::npos) {
    static std::string start_fragment_str("StartFragment:");
    size_t start_fragment_start = cf_html.find(start_fragment_str);
    if (start_fragment_start != std::string::npos) {
      *fragment_start = static_cast<size_t>(atoi(cf_html.c_str() +
        start_fragment_start + start_fragment_str.length()));
    }

    static std::string end_fragment_str("EndFragment:");
    size_t end_fragment_start = cf_html.find(end_fragment_str);
    if (end_fragment_start != std::string::npos) {
      *fragment_end = static_cast<size_t>(atoi(cf_html.c_str() +
        end_fragment_start + end_fragment_str.length()));
    }
  } else {
    *fragment_start = cf_html.find('>', tag_start) + 1;
    size_t tag_end = cf_html.rfind("<!--EndFragment", std::string::npos);
    *fragment_end = cf_html.rfind('<', tag_end);
  }
}

void CFHtmlToHtml(const std::string& cf_html,
                  std::string* html,
                  std::string* base_url) {
  size_t frag_start = std::string::npos;
  size_t frag_end = std::string::npos;

  CFHtmlExtractMetadata(cf_html, base_url, NULL, &frag_start, &frag_end);

  if (html && frag_start != std::string::npos &&
      frag_end != std::string::npos) {
    *html = cf_html.substr(frag_start, frag_end - frag_start);
  }
}

const DWORD moz_url_format = ::RegisterClipboardFormat(L"text/x-moz-url");
const DWORD html_format = ::RegisterClipboardFormat(L"HTML Format");
const DWORD file_desc_format =
    ::RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
const DWORD file_contents_format =
    ::RegisterClipboardFormat(CFSTR_FILECONTENTS);

bool DragDataToDataObject(CefRefPtr<CefDragData> drag_data,
                          IDataObject** data_object) {
  const int kMaxDataObjects = 10;
  FORMATETC fmtetcs[kMaxDataObjects];
  STGMEDIUM stgmeds[kMaxDataObjects];
  FORMATETC fmtetc = { 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
  int curr_index = 0;
  CefString text = drag_data->GetFragmentText();
  if (!text.empty()) {
    fmtetc.cfFormat = CF_UNICODETEXT;
    fmtetcs[curr_index] = fmtetc;
    GetStorageForString(&stgmeds[curr_index], text.ToWString());
    curr_index++;
  }
  if (drag_data->IsLink() && !drag_data->GetLinkURL().empty()) {
    std::wstring x_moz_url_str = drag_data->GetLinkURL().ToWString();
    x_moz_url_str += '\n';
    x_moz_url_str += drag_data->GetLinkTitle().ToWString();
    fmtetc.cfFormat = moz_url_format;
    fmtetcs[curr_index] = fmtetc;
    GetStorageForString(&stgmeds[curr_index], x_moz_url_str);
    curr_index++;
  }
  CefString html = drag_data->GetFragmentHtml();
  if (!html.empty()) {
    CefString base_url = drag_data->GetFragmentBaseURL();
    std::string cfhtml = HtmlToCFHtml(html.ToString(), base_url.ToString());
    fmtetc.cfFormat = html_format;
    fmtetcs[curr_index] = fmtetc;
    GetStorageForString(&stgmeds[curr_index], cfhtml);
    curr_index++;
  }

  size_t bufferSize = drag_data->GetFileContents(NULL);
  if (bufferSize) {
    CefRefPtr<BytesWriteHandler> handler = new BytesWriteHandler(bufferSize);
    CefRefPtr<CefStreamWriter> writer =
        CefStreamWriter::CreateForHandler(handler.get());
    drag_data->GetFileContents(writer);
    DCHECK_EQ(handler->GetDataSize(), static_cast<int64>(bufferSize));
    CefString fileName = drag_data->GetFileName();
    GetStorageForFileDescriptor(&stgmeds[curr_index], fileName.ToWString());
    fmtetc.cfFormat = file_desc_format;
    fmtetcs[curr_index] = fmtetc;
    curr_index++;
    GetStorageForBytes(&stgmeds[curr_index], handler->GetData(),
        handler->GetDataSize());
    fmtetc.cfFormat = file_contents_format;
    fmtetcs[curr_index] = fmtetc;
    curr_index++;
  }
  DCHECK_LT(curr_index, kMaxDataObjects);

  CComPtr<IDataObject> obj =
      DataObjectWin::Create(fmtetcs, stgmeds, curr_index);
  (*data_object) = obj.Detach();
  return true;
}

CefRefPtr<CefDragData> DataObjectToDragData(IDataObject* data_object) {
  CefRefPtr<CefDragData> drag_data = CefDragData::Create();
  IEnumFORMATETC* enumFormats = NULL;
  HRESULT res = data_object->EnumFormatEtc(DATADIR_GET, &enumFormats);
  if (res != S_OK)
    return drag_data;
  enumFormats->Reset();
  const int kCelt = 10;

  ULONG celtFetched;
  do {
    celtFetched = kCelt;
    FORMATETC rgelt[kCelt];
    res = enumFormats->Next(kCelt, rgelt, &celtFetched);
    for (unsigned i = 0; i < celtFetched; i++) {
      CLIPFORMAT format = rgelt[i].cfFormat;
      if (!(format == CF_UNICODETEXT ||
          format == CF_TEXT ||
          format == moz_url_format ||
          format == html_format ||
          format == CF_HDROP)
          || rgelt[i].tymed != TYMED_HGLOBAL)
        continue;
      STGMEDIUM medium;
      if (data_object->GetData(&rgelt[i], &medium) == S_OK) {
        if (!medium.hGlobal) {
          ReleaseStgMedium(&medium);
          continue;
        }
        void* hGlobal = GlobalLock(medium.hGlobal);
        if (!hGlobal) {
          ReleaseStgMedium(&medium);
          continue;
        }
        if (format == CF_UNICODETEXT) {
          CefString text;
          text.FromWString((std::wstring::value_type*)hGlobal);
          drag_data->SetFragmentText(text);
        } else if (format == CF_TEXT) {
          CefString text;
          text.FromString((std::string::value_type*)hGlobal);
          drag_data->SetFragmentText(text);
        } else if (format == moz_url_format) {
          std::wstring html((std::wstring::value_type*)hGlobal);
          size_t pos = html.rfind('\n');
          CefString url(html.substr(0, pos));
          CefString title(html.substr(pos + 1));
          drag_data->SetLinkURL(url);
          drag_data->SetLinkTitle(title);
        } else if (format == html_format) {
          std::string cf_html((std::string::value_type*)hGlobal);
          std::string base_url;
          std::string html;
          CFHtmlToHtml(cf_html, &html, &base_url);
          drag_data->SetFragmentHtml(html);
          drag_data->SetFragmentBaseURL(base_url);
        }
        if (format == CF_HDROP) {
          HDROP hdrop = (HDROP)hGlobal;
          const int kMaxFilenameLen = 4096;
          const unsigned num_files = DragQueryFileW(hdrop, 0xffffffff, 0, 0);
          for (unsigned int x = 0; x < num_files; ++x) {
            wchar_t filename[kMaxFilenameLen];
            if (!DragQueryFileW(hdrop, x, filename, kMaxFilenameLen))
              continue;
            WCHAR* name = wcsrchr(filename, '\\');
            drag_data->AddFile(filename, (name ? name + 1 : filename));
          }
        }
        if (medium.hGlobal)
          GlobalUnlock(medium.hGlobal);
        if (format == CF_HDROP)
          DragFinish((HDROP)hGlobal);
        else
          ReleaseStgMedium(&medium);
      }
    }
  } while (res == S_OK);
  enumFormats->Release();
  return drag_data;
}

}  // namespace


CComPtr<DropTargetWin> DropTargetWin::Create(OsrDragEvents* callback,
                                             HWND hWnd) {
  return CComPtr<DropTargetWin>(new DropTargetWin(callback, hWnd));
}

HRESULT DropTargetWin::DragEnter(IDataObject* data_object,
                                 DWORD key_state,
                                 POINTL cursor_position,
                                 DWORD* effect) {
  if (!callback_)
    return E_UNEXPECTED;

  CefRefPtr<CefDragData> drag_data = current_drag_data_;
  if (!drag_data) {
    drag_data = DataObjectToDragData(data_object);
  }
  CefMouseEvent ev = ToMouseEvent(cursor_position, key_state, hWnd_);
  CefBrowserHost::DragOperationsMask mask = DropEffectToDragOperation(*effect);
  mask = callback_->OnDragEnter(drag_data, ev, mask);
  *effect = DragOperationToDropEffect(mask);
  return S_OK;
}

CefBrowserHost::DragOperationsMask DropTargetWin::StartDragging(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDragData> drag_data,
    CefRenderHandler::DragOperationsMask allowed_ops,
    int x, int y) {
  CComPtr<IDataObject> dataObject;
  DWORD resEffect = DROPEFFECT_NONE;
  if (DragDataToDataObject(drag_data, &dataObject)) {
    CComPtr<IDropSource> dropSource = DropSourceWin::Create();
    DWORD effect = DragOperationToDropEffect(allowed_ops);
    current_drag_data_ = drag_data->Clone();
    current_drag_data_->ResetFileContents();
    HRESULT res = DoDragDrop(dataObject, dropSource, effect, &resEffect);
    if (res != DRAGDROP_S_DROP)
      resEffect = DROPEFFECT_NONE;
    current_drag_data_ = NULL;
  }
  return DropEffectToDragOperation(resEffect);
}

HRESULT DropTargetWin::DragOver(DWORD key_state,
                                POINTL cursor_position,
                                DWORD* effect) {
  if (!callback_)
    return E_UNEXPECTED;
  CefMouseEvent ev = ToMouseEvent(cursor_position, key_state, hWnd_);
  CefBrowserHost::DragOperationsMask mask = DropEffectToDragOperation(*effect);
  mask = callback_->OnDragOver(ev, mask);
  *effect = DragOperationToDropEffect(mask);
  return S_OK;
}

HRESULT DropTargetWin::DragLeave() {
  if (!callback_)
    return E_UNEXPECTED;
  callback_->OnDragLeave();
  return S_OK;
}

HRESULT DropTargetWin::Drop(IDataObject* data_object,
                            DWORD key_state,
                            POINTL cursor_position,
                            DWORD* effect) {
  if (!callback_)
    return E_UNEXPECTED;
  CefMouseEvent ev = ToMouseEvent(cursor_position, key_state, hWnd_);
  CefBrowserHost::DragOperationsMask mask = DropEffectToDragOperation(*effect);
  mask = callback_->OnDrop(ev, mask);
  *effect = DragOperationToDropEffect(mask);
  return S_OK;
}

CComPtr<DropSourceWin> DropSourceWin::Create() {
  return CComPtr<DropSourceWin>(new DropSourceWin());
}

HRESULT DropSourceWin::GiveFeedback(DWORD dwEffect) {
  return DRAGDROP_S_USEDEFAULTCURSORS;
}

HRESULT DropSourceWin::QueryContinueDrag(BOOL fEscapePressed,
                                         DWORD grfKeyState) {
  if (fEscapePressed) {
    return DRAGDROP_S_CANCEL;
  }

  if (!(grfKeyState & MK_LBUTTON)) {
    return DRAGDROP_S_DROP;
  }

  return S_OK;
}

HRESULT DragEnumFormatEtc::CreateEnumFormatEtc(UINT cfmt,
    FORMATETC* afmt,
    IEnumFORMATETC** ppEnumFormatEtc) {
  if (cfmt == 0 || afmt == 0 || ppEnumFormatEtc == 0)
    return E_INVALIDARG;

  *ppEnumFormatEtc = new DragEnumFormatEtc(afmt, cfmt);

  return (*ppEnumFormatEtc) ? S_OK : E_OUTOFMEMORY;
}

HRESULT DragEnumFormatEtc::Next(ULONG celt,
                                FORMATETC* pFormatEtc,
                                ULONG* pceltFetched) {
  ULONG copied = 0;

  // copy the FORMATETC structures into the caller's buffer
  while (m_nIndex < m_nNumFormats && copied < celt) {
    DeepCopyFormatEtc(&pFormatEtc[copied], &m_pFormatEtc[m_nIndex]);
    copied++;
    m_nIndex++;
  }

  // store result
  if (pceltFetched != 0)
    *pceltFetched = copied;

  // did we copy all that was requested?
  return (copied == celt) ? S_OK : S_FALSE;
}
HRESULT DragEnumFormatEtc::Skip(ULONG celt) {
  m_nIndex += celt;
  return (m_nIndex <= m_nNumFormats) ? S_OK : S_FALSE;
}
HRESULT DragEnumFormatEtc::Reset(void) {
  m_nIndex = 0;
  return S_OK;
}
HRESULT DragEnumFormatEtc::Clone(IEnumFORMATETC** ppEnumFormatEtc) {
  HRESULT hResult;

  // make a duplicate enumerator
  hResult = CreateEnumFormatEtc(m_nNumFormats, m_pFormatEtc, ppEnumFormatEtc);

  if (hResult == S_OK) {
    // manually set the index state
    reinterpret_cast<DragEnumFormatEtc*>(*ppEnumFormatEtc)->m_nIndex = m_nIndex;
  }

  return hResult;
}

DragEnumFormatEtc::DragEnumFormatEtc(FORMATETC* pFormatEtc, int nNumFormats) {
  AddRef();

  m_nIndex = 0;
  m_nNumFormats = nNumFormats;
  m_pFormatEtc = new FORMATETC[nNumFormats];

  // make a new copy of each FORMATETC structure
  for (int i = 0; i < nNumFormats; i++) {
    DeepCopyFormatEtc(&m_pFormatEtc[i], &pFormatEtc[i]);
  }
}
DragEnumFormatEtc::~DragEnumFormatEtc() {
  // first free any DVTARGETDEVICE structures
  for (ULONG i = 0; i < m_nNumFormats; i++) {
    if (m_pFormatEtc[i].ptd)
      CoTaskMemFree(m_pFormatEtc[i].ptd);
  }

  // now free the main array
  delete[] m_pFormatEtc;
}

void DragEnumFormatEtc::DeepCopyFormatEtc(FORMATETC* dest, FORMATETC* source) {
  // copy the source FORMATETC into dest
  *dest = *source;
  if (source->ptd) {
    // allocate memory for the DVTARGETDEVICE if necessary
    dest->ptd = reinterpret_cast<DVTARGETDEVICE*>(
        CoTaskMemAlloc(sizeof(DVTARGETDEVICE)));

    // copy the contents of the source DVTARGETDEVICE into dest->ptd
    *(dest->ptd) = *(source->ptd);
  }
}

CComPtr<DataObjectWin> DataObjectWin::Create(FORMATETC* fmtetc,
                                             STGMEDIUM* stgmed,
                                             int count) {
  return CComPtr<DataObjectWin>(new DataObjectWin(fmtetc, stgmed, count));
}

HRESULT DataObjectWin::GetDataHere(FORMATETC* pFormatEtc, STGMEDIUM* pmedium) {
  return E_NOTIMPL;
}

HRESULT DataObjectWin::QueryGetData(FORMATETC* pFormatEtc) {
  return (LookupFormatEtc(pFormatEtc) == -1) ? DV_E_FORMATETC : S_OK;
}

HRESULT DataObjectWin::GetCanonicalFormatEtc(FORMATETC* pFormatEct,
                                             FORMATETC* pFormatEtcOut) {
  pFormatEtcOut->ptd = NULL;
  return E_NOTIMPL;
}

HRESULT DataObjectWin::SetData(FORMATETC* pFormatEtc,
                               STGMEDIUM* pMedium,
                               BOOL fRelease) {
  return E_NOTIMPL;
}

HRESULT DataObjectWin::DAdvise(FORMATETC* pFormatEtc,
                               DWORD advf,
                               IAdviseSink*,
                               DWORD*) {
  return E_NOTIMPL;
}

HRESULT DataObjectWin::DUnadvise(DWORD dwConnection) {
  return E_NOTIMPL;
}

HRESULT DataObjectWin::EnumDAdvise(IEnumSTATDATA **ppEnumAdvise) {
  return E_NOTIMPL;
}

HRESULT DataObjectWin::EnumFormatEtc(DWORD dwDirection,
                                     IEnumFORMATETC** ppEnumFormatEtc) {
  return DragEnumFormatEtc::CreateEnumFormatEtc(m_nNumFormats, m_pFormatEtc,
      ppEnumFormatEtc);
}

HRESULT DataObjectWin::GetData(FORMATETC* pFormatEtc, STGMEDIUM* pMedium) {
  int idx;

  // try to match the specified FORMATETC with one of our supported formats
  if ((idx = LookupFormatEtc(pFormatEtc)) == -1)
    return DV_E_FORMATETC;

  // found a match - transfer data into supplied storage medium
  pMedium->tymed = m_pFormatEtc[idx].tymed;
  pMedium->pUnkForRelease = 0;

  // copy the data into the caller's storage medium
  switch (m_pFormatEtc[idx].tymed) {
  case TYMED_HGLOBAL:
    pMedium->hGlobal = DupGlobalMem(m_pStgMedium[idx].hGlobal);
    break;

  default:
    return DV_E_FORMATETC;
  }
  return S_OK;
}

HGLOBAL DataObjectWin::DupGlobalMem(HGLOBAL hMem) {
  DWORD   len = GlobalSize(hMem);
  PVOID   source = GlobalLock(hMem);
  PVOID   dest = GlobalAlloc(GMEM_FIXED, len);

  memcpy(dest, source, len);
  GlobalUnlock(hMem);
  return dest;
}

int DataObjectWin::LookupFormatEtc(FORMATETC* pFormatEtc) {
  // check each of our formats in turn to see if one matches
  for (int i = 0; i < m_nNumFormats; i++) {
    if ((m_pFormatEtc[i].tymed    &  pFormatEtc->tymed) &&
      m_pFormatEtc[i].cfFormat == pFormatEtc->cfFormat &&
      m_pFormatEtc[i].dwAspect == pFormatEtc->dwAspect) {
      // return index of stored format
      return i;
    }
  }

  // error, format not found
  return -1;
}

DataObjectWin::DataObjectWin(FORMATETC* fmtetc, STGMEDIUM* stgmed, int count)
    : ref_count_(0) {
  m_nNumFormats = count;

  m_pFormatEtc = new FORMATETC[count];
  m_pStgMedium = new STGMEDIUM[count];

  for (int i = 0; i < count; i++) {
    m_pFormatEtc[i] = fmtetc[i];
    m_pStgMedium[i] = stgmed[i];
  }
}

}  // namespace client

#endif  // defined(CEF_USE_ATL)
