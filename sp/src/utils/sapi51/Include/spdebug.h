/*******************************************************************************
* SPDebug.h *
*-----------*
*   Description:
*       This header file contains debug output services for SAPI5
*-------------------------------------------------------------------------------
*   Copyright (c) Microsoft Corporation. All rights reserved.
*******************************************************************************/

#pragma once

#include <TCHAR.h>
#include <crtdbg.h>

#ifdef ASSERT_WITH_STACK
#include "AssertWithStack.h"
#endif

const TCHAR g_szSpDebugKey[] = _T("SPDebug");
const TCHAR g_szSpDebugFuncTraceReportMode[] = _T("FuncTraceMode");
const TCHAR g_szSpDebugFuncTraceReportFile[] = _T("FuncTraceFile");
const TCHAR g_szSpDebugParamInfoReportMode[] = _T("ParamInfoMode");
const TCHAR g_szSpDebugParamInfoReportFile[] = _T("ParamInfoFile");
const TCHAR g_szSpDebugDumpInfoReportMode[] = _T("DumpInfoMode");
const TCHAR g_szSpDebugDumpInfoReportFile[] = _T("DumpInfoFile");
const TCHAR g_szSpDebugAssertReportMode[] = _T("AssertMode");
const TCHAR g_szSpDebugAssertReportFile[] = _T("AssertFile");
const TCHAR g_szSpDebugHRFailReportMode[] = _T("HRFailMode");
const TCHAR g_szSpDebugHRFailReportFile[] = _T("HRFailFile");

const TCHAR g_szSpDebugAssertSettingsReReadEachTime[] = _T("AssertSettingsReReadEachTime");
const TCHAR g_szSpDebugServerOnStart[] = _T("DebugServerOnStart");
const TCHAR g_szSpDebugClientOnStart[] = _T("DebugClientOnStart");

const TCHAR g_szSpDebugLog[] = _T("c:\\spdebug.log");

#ifdef _DEBUG

class CSpDebug
{
public:
    
    CSpDebug()
    {
        m_mutex = NULL;
        m_reportModePrev = -1;
        m_hfilePrev = NULL;
        Read();
    }

    ~CSpDebug()
    {
        if (m_mutex != NULL)
        {
            CloseHandle(m_mutex);
        }
    }
 
    BOOL FuncTrace(BOOL fEnter = TRUE)
    {
        return fEnter
            ? Enter(_CRT_WARN, m_FuncTraceMode, m_szFuncTraceFile)
            : Leave();
    }
    
    BOOL ParamInfo(BOOL fEnter = TRUE)
    {
        return fEnter
            ? Enter(_CRT_WARN, m_ParamInfoMode, m_szParamInfoFile)
            : Leave();
    }
    
    BOOL DumpInfo(BOOL fEnter = TRUE)
    {
        return fEnter
            ? Enter(_CRT_WARN, m_DumpInfoMode, m_szDumpInfoFile)
            : Leave();
    }
    
    BOOL Assert(BOOL fEnter = TRUE)
    {
        if (m_fAssertSettingsReReadEachTime)
            Read();

        return fEnter
            ? Enter(_CRT_ASSERT, m_AssertMode, m_szAssertFile)
            : Leave();
    }
    
    BOOL HRFail(BOOL fEnter = TRUE)
    {
        return fEnter
            ? Enter(_CRT_WARN, m_HRFailMode, m_szHRFailFile)
            : Leave();
    }
    
    BOOL DebugServerOnStart()
    {
        return m_fDebugServerOnStart;
    }
    
    BOOL DebugClientOnStart()
    {
        return m_fDebugClientOnStart;
    }
    
private:

    void Read()
    {
        HKEY hkeyDebug;
        RegCreateKeyEx(
            HKEY_CLASSES_ROOT, 
            g_szSpDebugKey, 
            0, 
            NULL, 
            0,
            KEY_READ | KEY_WRITE, 
            NULL, 
            &hkeyDebug, 
            NULL);
        if (hkeyDebug == NULL)
        {
            RegCreateKeyEx(
                HKEY_CLASSES_ROOT, 
                g_szSpDebugKey, 
                0, 
                NULL, 
                0,
                KEY_READ,
                NULL, 
                &hkeyDebug, 
                NULL);
        }
        
        DWORD dw = sizeof(m_fAssertSettingsReReadEachTime);
        if (RegQueryValueEx(
                hkeyDebug,
                g_szSpDebugAssertSettingsReReadEachTime,
                NULL,
                NULL,
                LPBYTE(&m_fAssertSettingsReReadEachTime),
                &dw) != ERROR_SUCCESS)
        {
            m_fAssertSettingsReReadEachTime = FALSE;
            RegSetValueEx(
                hkeyDebug,
                g_szSpDebugAssertSettingsReReadEachTime,
                NULL,
                REG_DWORD,
                LPBYTE(&m_fAssertSettingsReReadEachTime),
                sizeof(m_fAssertSettingsReReadEachTime));
        }
            
        ReadFor(
            hkeyDebug, 
            g_szSpDebugFuncTraceReportMode,
            g_szSpDebugFuncTraceReportFile, 
            &m_FuncTraceMode, 
            m_szFuncTraceFile, 
            0, 
            g_szSpDebugLog);
        ReadFor(
            hkeyDebug, 
            g_szSpDebugParamInfoReportMode, 
            g_szSpDebugParamInfoReportFile, 
            &m_ParamInfoMode, 
            m_szParamInfoFile, 
            0, 
            g_szSpDebugLog);
        ReadFor(
            hkeyDebug, 
            g_szSpDebugDumpInfoReportMode, 
            g_szSpDebugDumpInfoReportFile, 
            &m_DumpInfoMode, 
            m_szDumpInfoFile, 
            _CRTDBG_MODE_DEBUG, 
            g_szSpDebugLog);
        ReadFor(
            hkeyDebug, 
            g_szSpDebugAssertReportMode, 
            g_szSpDebugAssertReportFile, 
            &m_AssertMode, 
            m_szAssertFile, 
            _CRTDBG_MODE_WNDW,
            g_szSpDebugLog);
        ReadFor(
            hkeyDebug, 
            g_szSpDebugHRFailReportMode,
            g_szSpDebugHRFailReportFile, 
            &m_HRFailMode, 
            m_szHRFailFile, 
            _CRTDBG_MODE_DEBUG,
            g_szSpDebugLog);
        
        dw = sizeof(m_fDebugServerOnStart);
        if (RegQueryValueEx(
                hkeyDebug,
                g_szSpDebugServerOnStart,
                NULL,
                NULL,
                LPBYTE(&m_fDebugServerOnStart),
                &dw) != ERROR_SUCCESS)
        {
            m_fDebugServerOnStart = FALSE;
            RegSetValueEx(
                hkeyDebug,
                g_szSpDebugServerOnStart,
                NULL,
                REG_DWORD,
                LPBYTE(&m_fDebugServerOnStart),
                sizeof(m_fDebugServerOnStart));
        }
            
        dw = sizeof(m_fDebugClientOnStart);
        if (RegQueryValueEx(
                hkeyDebug,
                g_szSpDebugClientOnStart,
                NULL,
                NULL,
                LPBYTE(&m_fDebugClientOnStart),
                &dw) != ERROR_SUCCESS)
        {
            m_fDebugClientOnStart = FALSE;
            RegSetValueEx(
                hkeyDebug,
                g_szSpDebugClientOnStart,
                NULL,
                REG_DWORD,
                LPBYTE(&m_fDebugClientOnStart),
                sizeof(m_fDebugClientOnStart));
        }
        
        RegCloseKey(hkeyDebug);
    }

    void ReadFor(
            HKEY hkey, 
            const TCHAR * pszModeValueName, 
            const TCHAR * pszFileValueName, 
            DWORD * pdwModeValue,
            TCHAR * pszFileValue,
            DWORD dwDefaultModeValue,
            const TCHAR * pszDefaultFileValue)
    {
        DWORD dw = sizeof(*pdwModeValue);
        if (RegQueryValueEx(
                hkey,
                pszModeValueName,
                NULL,
                NULL,
                LPBYTE(pdwModeValue),
                &dw) != ERROR_SUCCESS)
        {
            *pdwModeValue = dwDefaultModeValue;
            RegSetValueEx(
                hkey,
                pszModeValueName,
                NULL,
                REG_DWORD,
                LPBYTE(pdwModeValue),
                sizeof(*pdwModeValue));
        }
        
        dw = MAX_PATH;
        if (RegQueryValueEx(
                hkey,
                pszFileValueName,
                NULL,
                NULL,
                LPBYTE(pszFileValue),
                &dw) != ERROR_SUCCESS)
        {
            _tcscpy(pszFileValue, pszDefaultFileValue);
            RegSetValueEx(
                hkey,
                pszFileValueName,
                NULL,
                REG_SZ,
                LPBYTE(pszFileValue),
                MAX_PATH);
        }
    }

    BOOL Enter(int reportType, DWORD &reportMode, TCHAR * pszFile)
    {
        if (reportMode != 0)
        {
            // We'll hold the mutex, until the caller also calls Leave
            if (m_mutex == NULL)
            {
                m_mutex = CreateMutex(NULL, FALSE, _T("SpDebug"));
            }
            WaitForSingleObject(m_mutex, INFINITE);
            
            m_reportType = reportType;
            m_reportModePrev = _CrtSetReportMode(reportType, reportMode);
            if (reportMode & _CRTDBG_MODE_FILE)
            {
                HANDLE hfile = CreateFile(
                    pszFile, 
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_ALWAYS,
                    0,
                    NULL);
                SetFilePointer(hfile, 0, NULL, FILE_END);
                m_hfilePrev = (_HFILE)_CrtSetReportFile(reportType, (_HFILE)hfile);
            }
            
            return TRUE;
        }

        return FALSE;
    }

    BOOL Leave()
    {
        int reportMode = _CrtSetReportMode(m_reportType, m_reportModePrev);
        if (reportMode & _CRTDBG_MODE_FILE)
        {
            CloseHandle((_HFILE)_CrtSetReportFile(m_reportType, (_HFILE)m_hfilePrev));
        }
        
        ReleaseMutex(m_mutex);

        return TRUE;
    }
    
private:

    HANDLE m_mutex;
    
    int    m_reportType;
    int    m_reportModePrev;
    _HFILE m_hfilePrev;
    
    BOOL  m_fAssertSettingsReReadEachTime;
    
    DWORD m_FuncTraceMode;
    TCHAR  m_szFuncTraceFile[MAX_PATH + 1];
    DWORD m_ParamInfoMode;
    TCHAR  m_szParamInfoFile[MAX_PATH + 1];
    DWORD m_DumpInfoMode;
    TCHAR  m_szDumpInfoFile[MAX_PATH + 1];
    DWORD m_AssertMode;
    TCHAR  m_szAssertFile[MAX_PATH + 1];
    DWORD m_HRFailMode;
    TCHAR  m_szHRFailFile[MAX_PATH + 1];
    
    BOOL m_fDebugServerOnStart;
    BOOL m_fDebugClientOnStart;
};

inline CSpDebug *PSpDebug()
{
    static CSpDebug debug;
    return &debug;
}

class CSpFuncTrace
{
public:
  
    CSpFuncTrace(PCHAR pFuncName)
    {
        m_pFuncName = pFuncName;
        if (PSpDebug()->FuncTrace())
        {
            _RPT1( _CRT_WARN, "\nEntering Function: %s\n", m_pFuncName );
            PSpDebug()->FuncTrace(FALSE);
        }
    }
    
    ~CSpFuncTrace()
    {
        if (PSpDebug()->FuncTrace())
        {
            _RPT1( _CRT_WARN, "Leaving Function: %s\n", m_pFuncName );
            PSpDebug()->FuncTrace(FALSE);
        }
    }
    
private:

    PCHAR m_pFuncName;
};

#endif // _DEBUG

//=== User macros ==============================================================

#ifdef _DEBUG

#define SPDBG_FUNC(name)            \
    CSpFuncTrace functrace(name)

#if defined(ASSERT_WITH_STACK) && !defined(_WIN64)
#define SPDBG_REPORT_ON_FAIL(hr)                                                      \
    do                                                                                \
    {                                                                                 \
        HRESULT _hr = (hr);                                                           \
        if (FAILED(_hr) && PSpDebug()->HRFail())                                      \
        {                                                                             \
            SYSTEMTIME sysTime;                                                       \
            GetLocalTime(&sysTime);                                                   \
            CHAR pszHrWithTime[100];                                                  \
            sprintf(pszHrWithTime, "%lX\n\n%d.%d.%d %02d:%02d:%02d",            \
                _hr,                                                                  \
                sysTime.wMonth,sysTime.wDay,sysTime.wYear,                            \
                sysTime.wHour,sysTime.wMinute,sysTime.wSecond);                       \
            PCHAR pszStack =                                                          \
                (PCHAR)_alloca(                                                       \
                    cchMaxAssertStackLevelStringLen *                                 \
                         cfrMaxAssertStackLevels + 1);                                \
            GetStringFromStackLevels(0, 10, pszStack);                                \
            _RPT4(_CRT_WARN,                                                          \
                "%s(%d): Failed HR = %s\n\n%s\n",                                     \
                __FILE__,                                                             \
                __LINE__,                                                             \
                pszHrWithTime,                                                        \
                pszStack);                                                            \
            PSpDebug()->HRFail(FALSE);                                                \
        }                                                                             \
    } while (0)
#else // ASSERT_WITH_STACK & !_WIN64
#define SPDBG_REPORT_ON_FAIL(hr)                                                      \
    do                                                                                \
    {                                                                                 \
        HRESULT _hr = (hr);                                                           \
        if (FAILED(_hr) && PSpDebug()->HRFail())                                      \
        {                                                                             \
            _RPT3(_CRT_WARN, "%s(%d): Failed HR = %lX\n", __FILE__, __LINE__, (_hr) );\
            PSpDebug()->HRFail(FALSE);                                                \
        }                                                                             \
    } while (0)
#endif // ASSERT_WITH_STACK

#define SPDBG_ASSERT(expr)                  \
    do                                      \
    {                                       \
        if (!(expr))                        \
        {                                   \
            if (PSpDebug()->Assert())       \
            {                               \
                _ASSERTE( expr );           \
                PSpDebug()->Assert(FALSE);  \
            }                               \
        }                                   \
    }                                       \
    while (0)

#define SPDBG_VERIFY(expr)  \
    SPDBG_ASSERT(expr)

#define SPDBG_PMSG0(format)                                    \
    do                                                         \
    {                                                          \
        if (PSpDebug()->ParamInfo())                           \
        {                                                      \
            _RPT0(_CRT_WARN, format);                          \
            PSpDebug()->ParamInfo(FALSE);                      \
        }                                                      \
    } while (0)
#define SPDBG_PMSG1(format, arg1)                              \
    do                                                         \
    {                                                          \
        if (PSpDebug()->ParamInfo())                           \
        {                                                      \
            _RPT1(_CRT_WARN, format, arg1);                    \
            PSpDebug()->ParamInfo(FALSE);                      \
        }                                                      \
    } while (0)
#define SPDBG_PMSG2(format, arg1, arg2)                        \
    do                                                         \
    {                                                          \
        if (PSpDebug()->ParamInfo())                           \
        {                                                      \
            _RPT2(_CRT_WARN, format, arg1, arg2);              \
            PSpDebug()->ParamInfo(FALSE);                      \
        }                                                      \
    } while (0)
#define SPDBG_PMSG3(format, arg1, arg2, arg3)                  \
    do                                                         \
    {                                                          \
        if (PSpDebug()->ParamInfo())                           \
        {                                                      \
            _RPT3(_CRT_WARN, format, arg1, arg2, arg3);        \
            PSpDebug()->ParamInfo(FALSE);                      \
        }                                                      \
    } while (0)
#define SPDBG_PMSG4(format, arg1, arg2, arg3, arg4)            \
    do                                                         \
    {                                                          \
        if (PSpDebug()->ParamInfo())                           \
        {                                                      \
            _RPT4(_CRT_WARN, format, arg1, arg2, arg3, arg4);  \
            PSpDebug()->ParamInfo(FALSE);                      \
        }                                                      \
    } while (0)
    
#define SPDBG_DMSG0(format)                                    \
    do                                                         \
    {                                                          \
        if (PSpDebug()->DumpInfo())                            \
        {                                                      \
            _RPT0(_CRT_WARN, format);                          \
            PSpDebug()->DumpInfo(FALSE);                       \
        }                                                      \
    } while (0)
#define SPDBG_DMSG1(format, arg1)                              \
    do                                                         \
    {                                                          \
        if (PSpDebug()->DumpInfo())                            \
        {                                                      \
            _RPT1(_CRT_WARN, format, arg1);                    \
            PSpDebug()->DumpInfo(FALSE);                       \
        }                                                      \
    } while (0)
#define SPDBG_DMSG2(format, arg1, arg2)                        \
    do                                                         \
    {                                                          \
        if (PSpDebug()->DumpInfo())                            \
        {                                                      \
            _RPT2(_CRT_WARN, format, arg1, arg2);              \
            PSpDebug()->DumpInfo(FALSE);                       \
        }                                                      \
    } while (0)
#define SPDBG_DMSG3(format, arg1, arg2, arg3)                  \
    do                                                         \
    {                                                          \
        if (PSpDebug()->DumpInfo())                            \
        {                                                      \
            _RPT3(_CRT_WARN, format, arg1, arg2, arg3);        \
            PSpDebug()->DumpInfo(FALSE);                       \
        }                                                      \
    } while (0)
#define SPDBG_DMSG4(format, arg1, arg2, arg3, arg4)            \
    do                                                         \
    {                                                          \
        if (PSpDebug()->DumpInfo())                            \
        {                                                      \
            _RPT4(_CRT_WARN, format, arg1, arg2, arg3, arg4);  \
            PSpDebug()->DumpInfo(FALSE);                       \
        }                                                      \
    } while (0)

#define SPDBG_RETURN(hr)                \
    {                                   \
        HRESULT __hr = (hr);            \
        if (FAILED(__hr))               \
        {                               \
            SPDBG_REPORT_ON_FAIL(__hr); \
        }                               \
        return __hr;                    \
    }                                                    

#define SPDBG_DEBUG_SERVER_ON_START()           \
    {                                           \
        if (PSpDebug()->DebugServerOnStart())   \
        {                                       \
            if (MessageBox(                     \
                    GetDesktopWindow(),         \
                    _T("Attach Debugger to the SAPI Server process?"),   \
                    _T("SAPI"),                 \
                    MB_YESNO) == IDYES)         \
            {                                   \
                USES_CONVERSION;                \
                TCHAR szCommand[MAX_PATH + 1];  \
                wsprintf(                       \
                    szCommand,                  \
                    _T("msdev -p %d"),          \
                    GetCurrentProcessId());     \
                system(T2A(szCommand));         \
            }                                   \
        }                                       \
    }

#define SPDBG_DEBUG_CLIENT_ON_START()           \
    {                                           \
        if (PSpDebug()->DebugClientOnStart())   \
        {                                       \
            TCHAR szModule[MAX_PATH + 1];       \
            szModule[0] = '\0';                 \
            TCHAR * pszSapiServer =             \
                _T("sapisvr.exe");              \
            GetModuleFileName(                  \
                NULL,                           \
                szModule,                       \
                MAX_PATH);                      \
            if ((_tcslen(szModule) <=           \
                    _tcslen(pszSapiServer) ||   \
                 _tcsicmp(                      \
                    szModule +                  \
                        _tcslen(szModule) -     \
                        _tcslen(pszSapiServer), \
                    pszSapiServer) != 0) &&     \
                MessageBox(                     \
                    GetDesktopWindow(),         \
                    _T("Attach Debugger to the SAPI Client process?"),   \
                    _T("SAPI"),                 \
                    MB_YESNO) == IDYES)         \
            {                                   \
                USES_CONVERSION;                \
                TCHAR szCommand[MAX_PATH + 1];  \
                wsprintf(                       \
                    szCommand,                  \
                    _T("msdev -p %d"),          \
                    GetCurrentProcessId());     \
                system(T2A(szCommand));         \
            }                                   \
        }                                       \
    }
        
#else // _DEBUG

#define SPDBG_FUNC(name)
#define SPDBG_REPORT_ON_FAIL(hr)
#define SPDBG_ASSERT(expr)
#define SPDBG_VERIFY(expr) (expr)
#define SPDBG_PMSG0(format)
#define SPDBG_PMSG1(format, arg1)
#define SPDBG_PMSG2(format, arg1, arg2)
#define SPDBG_PMSG3(format, arg1, arg2, arg3)
#define SPDBG_PMSG4(format, arg1, arg2, arg3, arg4)
#define SPDBG_DMSG0(format)
#define SPDBG_DMSG1(format, arg1)
#define SPDBG_DMSG2(format, arg1, arg2)
#define SPDBG_DMSG3(format, arg1, arg2, arg3)
#define SPDBG_DMSG4(format, arg1, arg2, arg3, arg4)
#define SPDBG_RETURN(hr) return (hr)
#define SPDBG_DEBUG_SERVER_ON_START()
#define SPDBG_DEBUG_CLIENT_ON_START()

#endif // _DEBUG
