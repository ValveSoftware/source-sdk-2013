/******************************************************************************
*   DictPad.h 
*       This module contains the base definitions for the DictPad SAPI 5 
*       premier app.
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/
#pragma once

#include "resource.h"
#include "candidatelist.h"

// Flags that determine the state of the app
typedef enum DPFLAGS
{
    DP_DICTATION_MODE     = ( 1L << 0 ),    // Toggles between dictation and command mode
    DP_WHOLE_WORD_SEL     = ( 1L << 1 ),    // Indicates that whole words should be selected
    DP_MICROPHONE_ON      = ( 1L << 2 ),    // Indicates that the "mic" is on (really that 
                                            // the appropriate grammars are active
    DP_SHARED_RECOGNIZER  = ( 1L << 3 ),    // Shared reco engine (false if engine is inproc)
    DP_IS_SPEAKING        = ( 1L << 4 ),    // Indicates that we are in the midst of a playback
    DP_GRAMMARS_ACTIVE    = ( 1L << 5 ),    // Indicates the the "mic" is on
    DP_JUST_PASTED_TEXT   = ( 1L << 6 ),    // Indicates that text has just been pasted
    DP_SKIP_UPDATE        = ( 1L << 7 )     // Indicates that selection changes should not be processed
} DPFLAGS;

// There are three grammars loaded
typedef enum GRAMMARIDS
{
    GID_DICTATION,      // ID for the dictation grammar
    GID_DICTATIONCC,    // ID for the C&C grammar that's active during dictation
    GID_CC              // ID for the C&C grammar that's active when dictation is not
};

// State having to do with the text selection
typedef struct SELINFO
{
    SELCHANGE selchange;    // SELCHANGE struct
    long lTextLen;          // Total text length
}   SELINFO;

// State having to do with an ongoing playback
typedef struct SPEAKINFO
{
    long lSelStart, lSelEnd;
    ITextRange *pSpeakRange;
    PTEXTRUNNODE pCurrentNode;
    ULONG ulCurrentStream;              // The stream number we are currently on
}   SPEAKINFO;

// A way of mapping between recognized voice command and window messages.
// The details of what gets executed in response to voice-enable menus is very consistent and predictable.
// Effectively, we will send a message to the client window, simulating the menu item itself.
// This structure is used for bundling these details.
// An instance of this map is created in dictpad_sapi.cpp
struct PROPERTYMAP
{
    DWORD   dwPropertyID;   // The specific property Id
    UINT    uiMessage;      // Speicific msg that will be sent to the apps client window 
    WPARAM  wParam;         // First param - value is dependent upon the msg
    LPARAM  lParam;         // Second param - value is dependent upon the msg
};

// Constants
#define MAX_LOADSTRING          1000
#define WM_DICTRECOEVENT    WM_USER + 1
#define WM_CCRECOEVENT      WM_USER + 2
#define WM_TTSEVENT         WM_USER + 3
#define WM_STOPUPDATE       WM_USER + 4
#define WM_STARTUPDATE      WM_USER + 5
#define WM_UPDATEALTSBUTTON WM_USER + 6

class CDictationPad
{
public:
    CDictationPad( HINSTANCE hInst = NULL );
    ~CDictationPad();

    // Functions for running Dictpad (in dictpad.cpp)
    BOOL Initialize( int nCmdShow, LPSTR lpCmdLine );
    int Run( void );

private:
    // In dictpad.cpp:

    // Window proc
    static LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
    
    // Initialization methods
    BOOL InitializeWindow( HWND hWnd );
    void SetTooltipText( LPARAM lParam );
    
    // Methods to process typed input and other notifications from the edit window
    HRESULT UpdateList( long lStart, long lEnd );   // Updates the list by inserting a block
    void ProcessMsgFilter( MSGFILTER *pMsgFilter ); // Handle the various notificaton events
                                                    // that we were interested in
    HRESULT ProcessSelChange( SELCHANGE *pSelChange);                     
                                                    // Handle notification of selection change
                                                    // (this is how keyboard input gets processed)

    // Playback
    HRESULT DoPlay();                               // Executes a playback
    HRESULT StartSpeaking( long lStartSpeakRange,   // Called before a playback starts
                            long lEndSpeakRange);                           
    void EndSpeaking();                             // Called when a playback is through

    // File new/open/save/close
    HRESULT DoFileNew();                            // Opens a new file 
    HRESULT DoFileOpen( LPTSTR lpFileName );        // Opens a file from GetOpenFileName
    HRESULT DoFileSave( bool fTextOnly = false );   // Saves a file in the appropriate format
    HRESULT DoFileSaveAs();                         // Gets a name from GetSaveFileName and saves
    HRESULT DoFileClose();                          // Closes the file if there is currently one open

    // In dictpad_sapi.cpp:

    // Initialization methods
    HRESULT InitializeSAPIObjs();                   // Set up the SAPI objects
    HRESULT InitSAPICallback( HWND hWnd );          // Hook up the client window for SAPI notifications
    HRESULT LoadGrammars();                         // Load the various grammars
    
    // SR and TTS notification functions
    bool SRDictEventHandler( void );                // Handle notifications from the dictation context
    bool SRCCEventHandler( void );                  // Handle notifications from the C&C context
    void TTSEventHandler( void );                   // Handle notifications from the voice
    void SetSREventInterest( bool fOn );            // Sets/unsets interest in SR notification events

    // Methods to process events from the SR engine
    bool ProcessDictationModeCommands( ISpRecoResult &rResult );
                                                    // Process commands recognized while in dictation mode
    bool ProcessCommandModeCommands( ISpRecoResult &rResult );
                                                    // Process commands recognized while in command mode
    bool ProcessDictation( ISpRecoResult &rResult, int eEventId );
                                                    // Process dictation recognitions
    bool ProcessDictationHypothesis( ISpRecoResult &rResult );
                                                    // Process dictation hypotheses

    // Switching between the two recognition contexts 
    HRESULT SetMode( bool fDictationMode );         // Switches between dictation and command modes

    // Controlling the "mic" (really, whether grammar rules are active)
    HRESULT SetGrammarState( BOOL bOn );            // Sets the grammar rules to the desired state

    // Add/delete Words UI
    HRESULT RunAddDeleteUI();                       // Starts up the Add/Delete words UI
                                                    // with in params as appropriate
private:
    // Win32-related handles
    HACCEL m_hAccelTable;               // handle to the accelerators
    HINSTANCE m_hInst;                  // handle to the current instance
    HMODULE m_hRtfLib;                  // handle to the rich edit control dll
    HWND    m_hClient;                  // handle to the app's client window
    HWND    m_hEdit;                    // handle to the rich edit control
    HWND    m_hToolBar;                 // handle to the toolbar
    HWND    m_hStatusBar;               // handle to the status bar
    HFONT   m_hFont;                    // handle to the current font
    HWND    m_hAltsButton;              // handle to alternates UI button

    // Application state
    DWORD   m_dwFlags;                  // DPFLAGS (see above)
    SELINFO m_LastSelInfo;              // Information on the last selection
    SELINFO m_CurSelInfo;               // Information on the current selection
    SPEAKINFO m_SpeakInfo;              // Information about the current speaking state
    TCHAR *m_pszFile;                   // Name of the current file


    // Richedit/TOM 
    CComPtr<IRichEditOle> m_cpRichEdit;         // OLE interface to the rich edit control
    CComPtr<ITextDocument> m_cpTextDoc;
    CComPtr<ITextSelection> m_cpTextSel;
    
    // SAPI objects
    CComPtr<ISpRecognizer> m_cpRecoEngine;    // SR engine
    CComPtr<ISpRecoContext> m_cpDictRecoCtxt;   // Recognition context for dictation
    CComPtr<ISpRecoContext> m_cpCCRecoCtxt;     // Recognition context for C&C
    CComPtr<ISpRecoGrammar> m_cpDictGrammar;    // Dictation grammar 
    CComPtr<ISpRecoGrammar> m_cpDictCCGrammar;  // Grammar for the few commands that are accessible while dictating
    CComPtr<ISpRecoGrammar> m_cpCCGrammar;      // Grammar for full command & control mode
    CComPtr<ISpVoice> m_cpVoice;                // TTS voice

    // Event interests
    const ULONGLONG m_ullDictInterest;          // Events in which Dictpad will be interested in
    const ULONGLONG m_ullCCInterest;
    
    // Classes related to CDictationPad
    CTextRunList *m_pTextRunList;               // List of dictated and non-dictated runs (textrunlist.cpp)
    CRecoEventMgr *m_pRecoEventMgr;             // Handles placement of recognized text (recomgr.cpp)
    CCandidateList *m_pCandidateList;           // Handles alternates UI (candidatelist.cpp)
};


//--- Function Prototypes -----------------------------------------------------

// In dictpad.cpp
LRESULT CALLBACK About( HWND, UINT, WPARAM, LPARAM );
DWORD CALLBACK EditStreamCallbackReadIn( DWORD dwCookie, LPBYTE pbBuff, ULONG cb, ULONG *pcb );       
DWORD CALLBACK EditStreamCallbackWriteOut( DWORD dwCookie, LPBYTE pbBuff, ULONG cb, ULONG *pcb ); 
int MessageBoxFromResource( HWND hWnd, UINT uID, LPCTSTR lpCaption, UINT uType );      

// In dictpad_sapi.cpp
void HighlightAndBringIntoView( ITextDocument &rTextDoc, long lStart, long lEnd );
void DumpCommandToScreen( HWND hwndClient, ISpPhrase &rPhrase );

