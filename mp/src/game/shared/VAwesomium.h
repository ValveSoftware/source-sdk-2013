#pragma once

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>

#include <vgui/ISurface.h>

#include <Awesomium/WebCore.h>
#include <Awesomium/BitmapSurface.h>

#include <Awesomium/WebString.h>
#include <Awesomium/STLHelpers.h>

#include <Awesomium/JSObject.h>
#include <Awesomium/JSValue.h>
#include <Awesomium/JSArray.h>

// NEVER DO THIS AT HOME KIDS, MULTIPLE INHERITANCE IS BAD BUT I'M YOUR WORST NIGHTMARE
class VAwesomium : public vgui::Panel, public Awesomium::JSMethodHandler, public Awesomium::WebViewListener::Load
{
	DECLARE_CLASS_SIMPLE(VAwesomium, vgui::Panel);

public:
	VAwesomium(vgui::Panel *parent, const char *panelName);
	~VAwesomium();
	void OpenURL(const char *address);
	void ExecuteJavaScript(const char *script, const char *frame_xpath);
	Awesomium::WebView* GetWebView(void);

	virtual void OnMethodCall(Awesomium::WebView* caller, unsigned int remote_object_id, const Awesomium::WebString& method_name, const Awesomium::JSArray& args){};
	virtual Awesomium::JSValue OnMethodCallWithReturnValue(Awesomium::WebView* caller, unsigned int remote_object_id, const Awesomium::WebString& method_name, const Awesomium::JSArray& args){return Awesomium::JSValue::Undefined();};

	virtual void OnBeginLoadingFrame(Awesomium::WebView* caller, int64 frame_id, bool is_main_frame, const Awesomium::WebURL& url, bool is_error_page){};
	virtual void OnFailLoadingFrame(Awesomium::WebView* caller, int64 frame_id, bool is_main_frame, const Awesomium::WebURL& url, int error_code, const Awesomium::WebString& error_desc){};
	virtual void OnFinishLoadingFrame(Awesomium::WebView* caller, int64 frame_id, bool is_main_frame, const Awesomium::WebURL& url){};
	virtual void OnDocumentReady(Awesomium::WebView* caller, const Awesomium::WebURL& url){};

protected:
	virtual void PerformLayout(void);
	virtual void Paint(void);
	virtual void Think(void);
	virtual void OnMousePressed(vgui::MouseCode code);
	virtual void OnMouseReleased(vgui::MouseCode code);
	virtual void OnKeyCodePressed(vgui::KeyCode code);
	virtual void OnKeyCodeReleased(vgui::KeyCode code);
	virtual void OnKeyTyped(wchar_t unichar);
	virtual void OnCursorMoved(int x, int y);
	virtual void OnMouseWheeled(int delta);
	virtual void OnRequestFocus(vgui::VPANEL subFocus, vgui::VPANEL defaultPanel);

private:
	void MouseButtonHelper(vgui::MouseCode code, bool isUp);
	void KeyboardButtonHelper(vgui::KeyCode code, bool isUp);
	void DrawBrowserView(void);
	void AllocateViewBuffer(void);
	int NearestPowerOfTwo(int v);
	void ResizeView(void);
	Awesomium::WebCore *m_WebCore;
	Awesomium::WebView *m_WebView;
	Awesomium::BitmapSurface *m_BitmapSurface;
	int m_iTextureId;
	int	m_iNearestPowerWidth;
	int m_iNearestPowerHeight;
	bool m_bHasLoaded;
	
	static int m_iNumberOfViews;
};
