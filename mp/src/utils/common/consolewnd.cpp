//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <windows.h>
#include "consolewnd.h"


#pragma warning( disable : 4311 ) // warning C4311: 'reinterpret_cast' : pointer truncation from 'CConsoleWnd *const ' to 'LONG'
#pragma warning( disable : 4312 ) // warning C4312: 'type cast' : conversion from 'LONG' to 'CConsoleWnd *' of greater size

#define EDITCONTROL_BORDER_SIZE	5


// ------------------------------------------------------------------------------------------------ //
// Functions to manage the console window.
// ------------------------------------------------------------------------------------------------ //

class CConsoleWnd : public IConsoleWnd
{
public:
					CConsoleWnd();
					~CConsoleWnd();
	
	bool			Init( void *hInstance, int dialogResourceID, int editControlID, bool bVisible );
	void			Term();

	virtual void	Release();

	virtual void	SetVisible( bool bVisible );
	virtual bool	IsVisible() const;

	virtual void	PrintToConsole( const char *pMsg );
	virtual void	SetTitle( const char *pTitle );

	virtual void	SetDeleteOnClose( bool bDelete );


private:

	int				WindowProc( 
		HWND hwndDlg,  // handle to dialog box
		UINT uMsg,     // message
		WPARAM wParam, // first message parameter
		LPARAM lParam  // second message parameter
		);	
	
	static int		CALLBACK StaticWindowProc(
		HWND hwndDlg,  // handle to dialog box
		UINT uMsg,     // message
		WPARAM wParam, // first message parameter
		LPARAM lParam  // second message parameter
		);	

	void			RepositionEditControl();

	
private:	

	HWND			m_hWnd;
	HWND			m_hEditControl;
	bool			m_bVisible;
	bool			m_bDeleteOnClose;
	int				m_nCurrentChars;
};


CConsoleWnd::CConsoleWnd()
{
	m_hWnd = m_hEditControl = NULL;
	m_bVisible = false;
	m_bDeleteOnClose = false;
	m_nCurrentChars = 0;
}


CConsoleWnd::~CConsoleWnd()
{
	Term();
}

bool CConsoleWnd::Init( void *hInstance, int dialogResourceID, int editControlID, bool bVisible )
{
	// Create the window.
	m_hWnd = CreateDialog(
		(HINSTANCE)hInstance,
		MAKEINTRESOURCE( dialogResourceID ),
		NULL,
		&CConsoleWnd::StaticWindowProc );

	if ( !m_hWnd )
		return false;

	SetWindowLong( m_hWnd, GWL_USERDATA, reinterpret_cast< LONG >( this ) );
	if ( bVisible )
		ShowWindow( m_hWnd, SW_SHOW );

	// Get a handle to the edit control.
	m_hEditControl = GetDlgItem( m_hWnd, editControlID );
	if ( !m_hEditControl )
		return false;

	RepositionEditControl();

	m_bVisible = bVisible;		
	return true; 
}


void CConsoleWnd::Term()
{
	if ( m_hWnd )
	{
		DestroyWindow( m_hWnd );
		m_hWnd = NULL;
	}
}


void CConsoleWnd::Release()
{
	delete this;
}


void CConsoleWnd::SetVisible( bool bVisible )
{
	ShowWindow( m_hWnd, bVisible ? SW_RESTORE : SW_HIDE );
	
	if ( bVisible )
	{
		ShowWindow( m_hWnd, SW_SHOW );
		SetWindowPos( m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
		UpdateWindow( m_hWnd );
		
		int nLen = (int)SendMessage( m_hEditControl, EM_GETLIMITTEXT, 0, 0 );
		SendMessage( m_hEditControl, EM_SETSEL, nLen, nLen );
	}
	else
	{
		SetWindowPos( m_hWnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW | SWP_NOOWNERZORDER );
	}
	
	m_bVisible = bVisible;
}


bool CConsoleWnd::IsVisible() const
{
	return m_bVisible;
}


void CConsoleWnd::PrintToConsole( const char *pMsg )
{								
	if ( m_nCurrentChars >= 16*1024 )
	{
		// Clear the edit control otherwise it'll stop outputting anything.
		m_nCurrentChars = 0;

		int nLen = (int)SendMessage( m_hEditControl, EM_GETLIMITTEXT, 0, 0 );
		SendMessage( m_hEditControl, EM_SETSEL, 0, nLen );
		SendMessage( m_hEditControl, EM_REPLACESEL, FALSE, (LPARAM)"" );
	}		

	FormatAndSendToEditControl( m_hEditControl, pMsg );
	m_nCurrentChars += (int)strlen( pMsg );
}


void CConsoleWnd::SetTitle( const char *pTitle )
{
	SetWindowText( m_hWnd, pTitle );
}


int	CConsoleWnd::WindowProc( 
	HWND hwndDlg,  // handle to dialog box
	UINT uMsg,     // message
	WPARAM wParam, // first message parameter
	LPARAM lParam  // second message parameter
	)
{
	lParam = lParam; // avoid compiler warning
	
	if ( hwndDlg != m_hWnd )
		return false;

	switch ( uMsg )
	{
		case WM_SYSCOMMAND:
		{
			if ( wParam == SC_CLOSE )
			{
				if ( m_bDeleteOnClose )
				{
					Release();
				}
				else
				{
					SetVisible( false );
					return true;
				}
			}
		}
		break;

		case WM_SHOWWINDOW:
		{
			m_bVisible = (wParam != 0);
		}
		break;

		case WM_SIZE:
		case WM_INITDIALOG:
		{
			RepositionEditControl();
		}
		break;
	}
	
	return false;	
}


int	CConsoleWnd::StaticWindowProc(
	HWND hwndDlg,  // handle to dialog box
	UINT uMsg,     // message
	WPARAM wParam, // first message parameter
	LPARAM lParam  // second message parameter
	)
{
	CConsoleWnd *pDlg = (CConsoleWnd*)GetWindowLong( hwndDlg, GWL_USERDATA );
	if ( pDlg )
		return pDlg->WindowProc( hwndDlg, uMsg, wParam, lParam );
	else
		return false;
}


void CConsoleWnd::RepositionEditControl()
{
	RECT rcMain;
	GetClientRect( m_hWnd, &rcMain );

	RECT rcNew;
	rcNew.left = rcMain.left + EDITCONTROL_BORDER_SIZE;
	rcNew.right = rcMain.right - EDITCONTROL_BORDER_SIZE;
	rcNew.top = rcMain.top + EDITCONTROL_BORDER_SIZE;
	rcNew.bottom = rcMain.bottom - EDITCONTROL_BORDER_SIZE;

	SetWindowPos( 
		m_hEditControl,
		NULL,
		rcNew.left,
		rcNew.top,
		rcNew.right - rcNew.left,
		rcNew.bottom - rcNew.top,
		SWP_NOZORDER );
}


void CConsoleWnd::SetDeleteOnClose( bool bDelete )
{
	m_bDeleteOnClose = bDelete;
}


// ------------------------------------------------------------------------------------ //
// Module interface.
// ------------------------------------------------------------------------------------ //

void SendToEditControl( HWND hEditControl, const char *pText )
{
	int nLen = (int)SendMessage( hEditControl, EM_GETLIMITTEXT, 0, 0 );
	SendMessage( hEditControl, EM_SETSEL, nLen, nLen );
	SendMessage( hEditControl, EM_REPLACESEL, FALSE, (LPARAM)pText );
}


void FormatAndSendToEditControl( void *hWnd, const char *pText )
{
	HWND hEditControl = (HWND)hWnd;

	// Translate \n to \r\n.
	char outMsg[1024];
	const char *pIn = pText;
	char *pOut = outMsg;
	while ( *pIn )
	{
		if ( *pIn == '\n' )
		{
			*pOut = '\r';
			pOut++;
		}
		*pOut = *pIn;

		++pIn;
		++pOut;

		if ( pOut - outMsg >= 1020 )
		{
			*pOut = 0;
			SendToEditControl( hEditControl, outMsg );
			pOut = outMsg;
		}
	}
	*pOut = 0;
	SendToEditControl( hEditControl, outMsg );
}


IConsoleWnd* CreateConsoleWnd( void *hInstance, int dialogResourceID, int editControlID, bool bVisible )
{
	CConsoleWnd *pWnd = new CConsoleWnd;

	if ( pWnd->Init( hInstance, dialogResourceID, editControlID, bVisible ) )
	{
		return pWnd;
	}
	else
	{
		pWnd->Release();
		return NULL;
	}
}




