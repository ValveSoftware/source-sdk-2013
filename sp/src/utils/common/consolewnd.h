//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CONSOLEWND_H
#define CONSOLEWND_H
#ifdef _WIN32
#pragma once
#endif


class IConsoleWnd
{
public:
	virtual void	Release() = 0;
	
	// Print a message to the console.
	virtual void	PrintToConsole( const char *pMsg ) = 0;

	// Set the window title.
	virtual void	SetTitle( const char *pTitle ) = 0;

	// Show and hide the console window.
	virtual void	SetVisible( bool bVisible ) = 0;
	virtual bool	IsVisible() const = 0;

	// Normally, the window just hides itself when closed. You can use this to make the window 
	// automatically go away when they close it.
	virtual void	SetDeleteOnClose( bool bDelete ) = 0;
};


// Utility functions.

// This converts adds \r's where necessary and sends the text to the edit control.
void FormatAndSendToEditControl( void *hWnd, const char *pText );


IConsoleWnd* CreateConsoleWnd( void *hInstance, int dialogResourceID, int editControlID, bool bVisible );


#endif // CONSOLEWND_H
