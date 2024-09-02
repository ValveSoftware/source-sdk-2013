#ifndef _INC_TTSAPP
#define _INC_TTSAPP

#include <windows.h>        // System includes
#include <atlbase.h>		// ATL
#include <windowsx.h>
#include <commctrl.h>		// Common controls
#include <commdlg.h>
#include <richedit.h>		// Required for rich edit control
#include <richole.h>
#include "tom.h"
#include "textserv.h"
#include "resource.h"
#include <wchar.h>
#include <tchar.h>
#include <olectl.h>         // Required for showing property page
#include <sapi.h>           // SAPI includes
#include <sphelper.h>
#include <spuihelp.h>


// Constant definitions
#define MAX_SIZE                102400      //100K
#define NORM_SIZE               256
#define NUM_OUTPUTFORMATS       36
#define WM_TTSAPPCUSTOMEVENT       WM_APP          // Window message used for systhesis events
#define CHILD_CLASS             _T("TTSAppChildWin")  // Child window for blitting mouth to
#define WEYESNAR                14              // eye positions
#define WEYESCLO                15
#define NUM_PHONEMES            6
#define CHARACTER_WIDTH         128
#define CHARACTER_HEIGHT        128
#define MAX_FILE_PATH			256

//
// Prototypes for dialog procs
//
LPARAM CALLBACK ChildWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK About( HWND, UINT, WPARAM, LPARAM );

// Main Object used by app
class CTTSApp
{
public:
    CTTSApp(HINSTANCE hInstance) :
    m_hInst(hInstance), m_hWnd(0)
	{
        m_bPause                = FALSE; // pause audio?
        m_bStop                 = TRUE;  // stop audio?
        m_DefaultVolume         = 0;     // default volume
        m_DefaultRate           = 0;     // default rate
        m_DefaultFormatIndex    = 0;     // default output format
        m_pszwFileText          = NULL;  // text from file
        m_szWFileName[0]        = NULL;  // wide text from file
	}
	~CTTSApp() 
	{
        // delete any allocated memory
        if( m_pszwFileText )
        {
            delete m_pszwFileText;
        }
	}

    // Member Functions
    static LRESULT CALLBACK DlgProcMain (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    HRESULT     InitSapi();
    void        Stop();
    HRESULT     VoiceChange();
    HRESULT     ReadTheFile( TCHAR* szFileName, BOOL* bIsUnicode, WCHAR** ppszwBuff );
    void        UpdateEditCtlW( WCHAR* pszwText );
    BOOL        CallOpenFileDialog( TCHAR* szFileName, TCHAR* szFilter );
    BOOL        CallSaveFileDialog( TCHAR* szFileName, TCHAR* szFilter );
    HIMAGELIST  InitImageList();

//
//  Private methods
//
private:
    // Message handling member functions
    BOOL OnInitDialog( HWND hWnd );
    void MainHandleSynthEvent();
    void HandleScroll( HWND hCtl );
    void MainHandleCommand( int id, HWND hWndControl, UINT codeNotify );
    void MainHandleClose();
	void HandleSpeak ();
    void EnableSpeakButtons( BOOL fEnable );
    
//
//  Member data
//  
private:
    const HINSTANCE     m_hInst;
    HWND                m_hWnd;
    HWND                m_hChildWnd;
    CComPtr<ISpVoice>   m_cpVoice;
    CComPtr<ISpAudio>   m_cpOutAudio;
    BOOL                m_bPause;
    BOOL                m_bStop;
    USHORT              m_DefaultVolume;
    long                m_DefaultRate;
    int                 m_DefaultFormatIndex;
    WCHAR*              m_pszwFileText;
    WCHAR               m_szWFileName[MAX_FILE_PATH];
};

#endif // _INC_TTSAPP
