//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef IG15_H
#define IG15_H
#ifdef _WIN32
#pragma once
#endif

typedef void * G15_HANDLE;

typedef enum
{
    G15_BUTTON_1, G15_BUTTON_2, G15_BUTTON_3, G15_BUTTON_4
} G15SoftButton;

typedef enum
{
    G15_SMALL, G15_MEDIUM, G15_BIG
} G15TextSize;

typedef enum
{
    G15_SCROLLING_TEXT, G15_STATIC_TEXT, G15_ICON, G15_PROGRESS_BAR, G15_UNKNOWN
} G15ObjectType;

class IG15
{
public:
	virtual void		GetLCDSize( int &w, int &h ) = 0;
	
	// w, h should match the return value from GetLCDSize!!!

	// Creates the underlying object
	virtual bool		Init( char const *name ) = 0;
	// Destroys the underlying object
	virtual void		Shutdown() = 0;

	virtual bool		IsConnected() = 0;

	// Add/remove
	virtual G15_HANDLE	AddText(G15ObjectType type, G15TextSize size, int alignment, int maxLengthPixels) = 0;
	virtual G15_HANDLE	AddIcon( void *icon, int sizeX, int sizeY) = 0;
	virtual void		RemoveAndDestroyObject( G15_HANDLE hObject ) = 0;

	// Change
	virtual int			SetText(G15_HANDLE handle, char const * text) = 0;
	virtual int			SetOrigin(G15_HANDLE handle, int x, int y) = 0;
	virtual int			SetVisible(G15_HANDLE handle, bool visible) = 0;

	virtual bool		ButtonTriggered(int button) = 0;
	virtual void		UpdateLCD( unsigned int dwTimestamp) = 0;
};

#define G15_INTERFACE_VERSION "G15_INTERFACE_VERSION001"

#endif // IG15_H
