//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#if !defined( VGUICENTERPRINT_H )
#define VGUICENTERPRINT_H
#ifdef _WIN32
#pragma once
#endif

#include "ivguicenterprint.h"
#include <vgui/VGUI.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
namespace vgui
{
class Panel;
}

class CCenterStringLabel;
class CCenterPrint : public ICenterPrint
{
private:
	CCenterStringLabel	*vguiCenterString;

public:
						CCenterPrint( void );

	virtual void		Create( vgui::VPANEL parent );
	virtual void		Destroy( void );
	
	virtual void		SetTextColor( int r, int g, int b, int a );
	virtual void		Print( char *text );
	virtual void		Print( wchar_t *text );
	virtual void		ColorPrint( int r, int g, int b, int a, char *text );
	virtual void		ColorPrint( int r, int g, int b, int a, wchar_t *text );
	virtual void		Clear( void );
};

extern CCenterPrint *internalCenterPrint;

#endif // VGUICENTERPRINT_H