//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is a panel which is rendered image on top of an entity
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#ifndef VGUI_BITMAPPANEL_H
#define VGUI_BITMAPPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class KeyValues;
class BitmapImage;
struct Bitmap_t;

//-----------------------------------------------------------------------------
// This is a base class for a panel which always is rendered on top of an entity
//-----------------------------------------------------------------------------
class CBitmapPanel : public vgui::Panel
{
	typedef vgui::Panel BaseClass;

public:
	// constructor
	CBitmapPanel( );
	CBitmapPanel( vgui::Panel *pParent, const char *pName );
	~CBitmapPanel();

	// initialization
	bool Init( KeyValues* pInitData );

	// initialization from build-mode dialog style .res files
	virtual void ApplySettings(KeyValues *inResourceData);

	virtual void Paint( void );
	virtual void PaintBackground( void ) {}

	virtual void OnCursorEntered();
	virtual void OnCursorExited();

	// Setup for panels that aren't created by the commander overlay factory (i.e. aren't parsed from a keyvalues file)
	virtual void SetImage( BitmapImage *pImage );

	/// Set bitmap data directly
	virtual void SetBitmap( const Bitmap_t &bitmap );

	const char *GetMouseOverText( void );

private:
	enum
	{
		MAX_ENTITY_MOUSEOVER = 256
	};
	// The bitmap to render
	BitmapImage *m_pImage;
	int m_r, m_g, m_b, m_a;
	bool m_bOwnsImage;

	char			m_szMouseOverText[ MAX_ENTITY_MOUSEOVER ];

};

#endif //  VGUI_BITMAPPANEL_H
