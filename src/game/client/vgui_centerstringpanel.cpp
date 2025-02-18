//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include <stdarg.h>
#include "vguicenterprint.h"
#include "ivrenderview.h"
#include <vgui/IVGui.h>
#include "VGuiMatSurface/IMatSystemSurface.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#ifdef TF_CLIENT_DLL
static ConVar		scr_centertime( "scr_centertime", "5" );
#else
static ConVar		scr_centertime( "scr_centertime", "2" );
#endif

//-----------------------------------------------------------------------------
// Purpose: Implements Center String printing
//-----------------------------------------------------------------------------
class CCenterStringLabel : public vgui::Label
{
	DECLARE_CLASS_SIMPLE( CCenterStringLabel, vgui::Label );

public:
						CCenterStringLabel( vgui::VPANEL parent );
	virtual				~CCenterStringLabel( void );

	// vgui::Panel
	virtual void		ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void		OnTick( void );
	virtual bool		ShouldDraw( void );

	// CVGuiCenterPrint
	virtual void		SetTextColor( int r, int g, int b, int a );
	virtual void		Print( char *text );
	virtual void		Print( wchar_t *text );
	virtual void		ColorPrint( int r, int g, int b, int a, char *text );
	virtual void		ColorPrint( int r, int g, int b, int a, wchar_t *text );
	virtual void		Clear( void );

protected:
	MESSAGE_FUNC_INT_INT( OnScreenSizeChanged, "OnScreenSizeChanged", oldwide, oldtall );

private:
	void ComputeSize( void );

	vgui::HFont			m_hFont;

	float				m_flCentertimeOff;
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *parent - 
//-----------------------------------------------------------------------------
CCenterStringLabel::CCenterStringLabel( vgui::VPANEL parent ) : 
	BaseClass( NULL, "CCenterStringLabel", " " )
{
	SetParent( parent );
	ComputeSize();
	SetVisible( false );
	SetCursor( null );
	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );
	SetContentAlignment( vgui::Label::a_center );

	m_hFont = 0;
	SetFgColor( Color( 255, 255, 255, 255 ) );

	SetPaintBackgroundEnabled( false );

	m_flCentertimeOff = 0.0;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCenterStringLabel::~CCenterStringLabel( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Updates panel to handle the new screen size
//-----------------------------------------------------------------------------
void CCenterStringLabel::OnScreenSizeChanged(int iOldWide, int iOldTall)
{
	BaseClass::OnScreenSizeChanged(iOldWide, iOldTall);
	ComputeSize();
}

//-----------------------------------------------------------------------------
// Purpose: Computes panel's desired size and position
//-----------------------------------------------------------------------------
void CCenterStringLabel::ComputeSize( void )
{
	int w, h;
	w = ScreenWidth();
	h = ScreenHeight();

	int iHeight = (int)(h * 0.3);

	SetSize( w, iHeight );
	SetPos( 0, ( h * 0.35 ) - ( iHeight / 2 ) );
}

void CCenterStringLabel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	// Use a large font
	m_hFont = pScheme->GetFont( "Trebuchet24", true );
	assert( m_hFont );
	SetFont( m_hFont );

	int w, h;
	w = ScreenWidth();
	h = ScreenHeight();
	int iHeight = (int)(h * 0.3);
	SetSize( w, iHeight );
	SetPos( 0, ( h * 0.35 ) - ( iHeight / 2 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : r - 
//			g - 
//			b - 
//			a - 
//-----------------------------------------------------------------------------
void CCenterStringLabel::SetTextColor( int r, int g, int b, int a )
{
	SetFgColor( Color( r, g, b, a ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCenterStringLabel::Print( char *text )
{
	SetText( text );
	
	m_flCentertimeOff = scr_centertime.GetFloat() + gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCenterStringLabel::Print( wchar_t *text )
{
	SetText( text );
	
	m_flCentertimeOff = scr_centertime.GetFloat() + gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCenterStringLabel::ColorPrint( int r, int g, int b, int a, char *text )
{
	SetTextColor( r, g, b, a );
	Print( text );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCenterStringLabel::ColorPrint( int r, int g, int b, int a, wchar_t *text )
{
	SetTextColor( r, g, b, a );
	Print( text );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCenterStringLabel::Clear( void )
{
	m_flCentertimeOff = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCenterStringLabel::OnTick( void )
{
	bool bVisible = ShouldDraw();
	if ( IsVisible() != bVisible )
	{
		SetVisible( bVisible );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
// FIXME, this has dependencies on the engine that should go away
//-----------------------------------------------------------------------------
bool CCenterStringLabel::ShouldDraw( void )
{
	if ( engine->IsDrawingLoadingImage() )
	{
		return false;
	}

	if ( m_flCentertimeOff <= gpGlobals->curtime )
	{
		// not time to turn off the message yet
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
CCenterPrint::CCenterPrint( void )
{
	vguiCenterString = NULL;
}

void CCenterPrint::SetTextColor( int r, int g, int b, int a )
{
	if ( vguiCenterString )
	{
		vguiCenterString->SetTextColor( r, g, b, a );
	}
}

void CCenterPrint::Print( char *text )
{
	if ( vguiCenterString )
	{
		vguiCenterString->ColorPrint( 255, 255, 255, 255, text );
	}
}

void CCenterPrint::Print( wchar_t *text )
{
	if ( vguiCenterString )
	{
		vguiCenterString->ColorPrint( 255, 255, 255, 255, text );
	}
}

void CCenterPrint::ColorPrint( int r, int g, int b, int a, char *text )
{
	if ( vguiCenterString )
	{
		vguiCenterString->ColorPrint( r, g, b, a, text );
	}
}

void CCenterPrint::ColorPrint( int r, int g, int b, int a, wchar_t *text )
{
	if ( vguiCenterString )
	{
		vguiCenterString->ColorPrint( r, g, b, a, text );
	}
}

void CCenterPrint::Clear( void )
{
	if ( vguiCenterString )
	{
		vguiCenterString->Clear();
	}
}

void CCenterPrint::Create( vgui::VPANEL parent )
{
	if ( vguiCenterString )
	{
		Destroy();
	}

	vguiCenterString = new CCenterStringLabel( parent );
}

void CCenterPrint::Destroy( void )
{
	if ( vguiCenterString )
	{
		vguiCenterString->SetParent( (vgui::Panel *)NULL );
		delete vguiCenterString;
		vguiCenterString = NULL;
	}
}

static CCenterPrint g_CenterString;
CCenterPrint *internalCenterPrint = &g_CenterString;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CCenterPrint, ICenterPrint, VCENTERPRINT_INTERFACE_VERSION, g_CenterString );