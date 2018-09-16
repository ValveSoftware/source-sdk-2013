/****************************************************************************
*   ComponentVersionDlg.cpp
*       This module contains the implementation details of the 
*       "Component Version" dialog that may be accessible from all of the
*       SAPI SDK samples and tools.
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
*****************************************************************************/
#include "stdafx.h"
#include "SapiSDKCommon.h"
#include "SapiSDKCommonResources.h"


/**********************************************************************
* ComponentVersionsDlgProc *
*--------------------------*
*   Description:  
*       The DlgProc for the ComponentVersions dlg. It fills the edit
*       control with the version & path info of all the .dll's & .exe's
*       that are loaded by the app that is using us. 
*
*       If you wish to have other .dll's & .exe's included, then just
*       add their names to the s_aszModules_c array.  The rest happens
*       automatically.
*
*   Return:
*       TRUE if have processed the message
*       FALSE otherwise
**********************************************************************/
LRESULT CALLBACK ComponentVersionsDlgProc( HWND hDlg, UINT uiMessage, WPARAM wParam, LPARAM lParam )
{
    //--- basic dlg proc switch statement
    switch( uiMessage )
    {
        case WM_INITDIALOG:
			{
                //--- this array contains the full list of .dll's & .exe's that we will interogate.
                //    just add to this list when you want to add another module to the output
                static const LPCTSTR s_aszModules_c[] = 
                {
                    _T("dictpad.exe"),
                    _T("reco.exe"),
                    _T("speak.exe"),
                    _T("sapi.cpl"),
                    _T("srsvr.exe"),
                    _T("ttshello.exe"),
                    _T("wavtotext.exe"),
                    _T("wintts.exe"),
                    _T("sapi.dll"),
                    _T("spttseng.dll"),
                    _T("spcwfe.DLL"),
                    _T("spsreng.DLL"),
                    _T("spsr.DLL"),
                    _T("spsrx.DLL"),
                    _T("gramcomp.dll"),
                    _T("Lexicon.dll"),
                    _T("advapi32.DLL"),
                    _T("atl.DLL"),
                    _T("comctl32.DLL"),
                    _T("gdi32.DLL"),
                    _T("icap.DLL"),	
                    _T("kernel32.DLL"),
                    _T("lz32.DLL"),
                    _T("mfc42.DLL"),	
                    _T("mfc42d.DLL"),	
                    _T("mfc42u.DLL"),	
                    _T("mfc42ud.DLL"),
                    _T("msasm32.DLL"),
                    _T("msvcrt.DLL"),
                    _T("msxml.DLL"),
                    _T("ntdll.DLL"),
                    _T("ole32.DLL"),
                    _T("oleaut32.DLL"),
                    _T("riched32.DLL"),
                    _T("rpcrt.DLL"),
                    _T("rpcrt4.DLL"),
                    _T("shell32.DLL"),
                    _T("shfolder.DLL"),
                    _T("shlwapi.DLL"),
                    _T("user32.DLL"),
                    _T("urlmon.DLL"),
                    _T("version.DLL"),
                    _T("winmm.DLL")
                };
                static const int s_iNumModules_c = sizeof( s_aszModules_c ) / sizeof( s_aszModules_c[ 0 ] );

                TCHAR acFinalBuff[10000];   
                acFinalBuff[ 0 ] = L'\0';

                //--- spin thru all the listed modules to find the ones that are loaded by the current app
                for( int i = 0;  i < s_iNumModules_c;  ++i )
                {
                    //--- main discovery point - is the current module being used, or not
			        HMODULE hModule = GetModuleHandle( s_aszModules_c[ i ] );
			        if( hModule )
			        {
                        //--- the current module is being used, get it's path
				        TCHAR acModulePath[ _MAX_PATH ];
				        DWORD dwSize = GetModuleFileName( hModule, acModulePath, sizeof( acModulePath ) );
				        _ASSERTE( 0 < dwSize );

                        //--- now that we have the file, get the version info size from that file.  If the 
                        //    size is non-trivial, then the file contains legitimate version info
				        DWORD dwDummy;
				        dwSize = GetFileVersionInfoSize( const_cast< LPTSTR >( acModulePath ), &dwDummy );
			            TCHAR acBuff[1000];  
				        if( 0 < dwSize )
                        {
                            //--- real version info exists for the current module - get it
				            char *pcVersionInfo = new char[ dwSize ];
				            _ASSERTE( NULL != pcVersionInfo );
				            BOOL fSuccess = GetFileVersionInfo( const_cast< LPTSTR >( acModulePath ), 
                                                                0, dwSize, pcVersionInfo );
				            _ASSERTE( fSuccess );

                            //--- now convert the version info into something intelligible
				            VS_FIXEDFILEINFO	*pFixedVersionInfo;
				            UINT				uiFixedVersionSize;
				            fSuccess = VerQueryValue( pcVersionInfo, _T( "\\" ), 
										              reinterpret_cast< LPVOID * >( &pFixedVersionInfo ), 
										              &uiFixedVersionSize );
				            _ASSERTE( fSuccess );

				            //--- esnure we have a correct structure version!
				            _ASSERTE( uiFixedVersionSize == sizeof( VS_FIXEDFILEINFO ) );

                            //--- format the module name, version info & module path all nice and pretty
				            _stprintf( acBuff, _T( "%-15.15s: %3d.%02d.%02d.%04d   %s\r\n" ), 
                                s_aszModules_c[ i ],
					            HIWORD( pFixedVersionInfo->dwProductVersionMS ), 
					            LOWORD( pFixedVersionInfo->dwProductVersionMS ),
					            HIWORD( pFixedVersionInfo->dwProductVersionLS ), 
					            LOWORD( pFixedVersionInfo->dwProductVersionLS ),
                                acModulePath );
       				        
                            //--- clean-up
                            delete [] pcVersionInfo;
                        }
                        else
                        {
                            //--- no version info, but the module itself, as well as it's path, are still interesting
                            //    to know
                            _stprintf( acBuff, _T( "%-15.15s: <no version info>   %s\r\n" ), 
                                s_aszModules_c[ i ],
                                acModulePath );
                        }

                        //--- accummulate all the info in a single buffer 
                        if( ( _tcslen( acFinalBuff ) + _tcslen( acBuff ) ) < ( sizeof( acFinalBuff ) - 1 ) )
                        {
                            //--- plenty of room
                            _tcscat( acFinalBuff, acBuff );
                        }
                        else
                        {
                            //--- we just escaped a buffer overflow...
                            _tcscpy( acFinalBuff, _T( "<buffer too small>" ) );
                            break;
                        }
			        }
                }

                //--- send the fully populated buffer to the edit control
                HWND hEdit = ::GetDlgItem( hDlg, IDC_VERSION_EDIT );
                ::SetWindowText( hEdit, acFinalBuff );
            }
            return TRUE;

        case WM_SIZE:
            {
                //--- as the dlg resizes, have the edit control follow the client area's size
                RECT rect;
                ::GetClientRect( hDlg, &rect );
                HWND hEdit = ::GetDlgItem( hDlg, IDC_VERSION_EDIT );
                ::SetWindowPos( hEdit, NULL, rect.left, rect.top, 
                                LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER | SWP_NOACTIVATE );
            }
            return TRUE;

        case WM_COMMAND:
            //--- kill the dialog when we get canceled by the user
            if( IDCANCEL == LOWORD( wParam ) )
            {
                EndDialog( hDlg, LOWORD( wParam ));
                return TRUE;
            }
            break;
    }

    //--- we didn't process this msg, let the default behavior prevail
    return FALSE;

} /* ComponentVersions */
