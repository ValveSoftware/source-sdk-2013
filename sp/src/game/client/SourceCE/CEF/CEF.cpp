#include "cbase.h"
//
//#include "CefCommon.h"
//#include "CEFApp.h"
//#include "CEF.h"
//
//#include "tier0/memdbgon.h"
//
//
//void VCefInit()
//{
//	UIMsg("[CEF] Initializing");
//
//	CefEnableHighDPISupport();
//
//	CefMainArgs args;
//
//	CefRefPtr<VCefApp> app(new VCefApp);
//
//	// Since this is the browser prcoess (no "type" arg set),
//	// this returns -1 here.
//	int exitCode = CefExecuteProcess(args, app, NULL);
//	if (exitCode >= 0) {
//		return;
//	}
//
//	CefSettings settings;
//	settings.no_sandbox	= TRUE;
//	settings.single_process = TRUE;
//	settings.multi_threaded_message_loop = FALSE;
//
//	settings.command_line_args_disabled = TRUE;
//
//	CefInitialize(args, settings, app, NULL);
//}
//
//void VCefTick()
//{
//	CefDoMessageLoopWork();
//}