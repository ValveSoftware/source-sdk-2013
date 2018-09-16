/******************************************************************************
*   Common.h 
*       This module contains the definitions used by all modules in the
*       coffee application.         
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/

#define NORMAL_LOADSTRING  100                  // Normal size of loaded strings
#define MAX_LOADSTRING  256                     // Normal size of loaded strings
#define GRAMMARID1      161                     // Arbitrary grammar id
#define WM_RECOEVENT    WM_USER+190             // Arbitrary user defined message for reco callback
#define WM_GOTOCOUNTER      WM_USER+202         // Notification to go to counter pane
#define WM_INITPANE         WM_USER+203         // Notification for any pane to initialize
#define WM_GOTOOFFICE       WM_USER+204         // Notification to go to office pane
#define WM_ESPRESSOORDER    WM_USER+210         // Notification that an order has been received
#define WM_DIDNTUNDERSTAND  WM_USER+211         // Notification that we got a false recognition
#define WM_MANAGEEMPLOYEES  WM_USER+212         // Notification to go to employee management pane
#define WM_HEARTHEVOICE     WM_USER+213         // The User wants to hear the selected employee speak
#define WM_MISCCOMMAND      WM_USER+214         // Notification that something described under "other commands" was recognized
#define WM_TTSVOICESEL      WM_USER+215         // Notification that the user selected a voice
#define MY_RULE_ID      458                     // Arbitrary rule id
#define MAX_ID_ARRAY    7                       // Max number of ids in espresso rule
#define MINMAX_WIDTH    640                     // Window width
#define MINMAX_HEIGHT   480                     // Window height
#define TIMEOUT         12000                   // Timer fires on this interval in ms
#define MIN_ORDER_INTERVAL   2500               // Minimum utterance time for a false reco to be
                                                // considered a possible order
#define DYN_TTSVOICERULE     1001               // ID for the dynamic tts voice rule

typedef LRESULT (*PMSGHANDLER) (HWND, UINT, WPARAM, LPARAM );  // typedef msg handler

typedef struct tagID_TEXT
{
    ULONG   ulId;
    WCHAR   *pwstrCoMemText;
}ID_TEXT;
