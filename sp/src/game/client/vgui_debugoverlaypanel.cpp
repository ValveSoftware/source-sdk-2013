//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "idebugoverlaypanel.h"
#include "overlaytext.h"
#include <vgui/IVGui.h>
#include "engine/ivdebugoverlay.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/Controls.h>
#include <vgui/IScheme.h>
#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CDebugOverlay : public vgui::Panel
{
	typedef vgui::Panel BaseClass;

public:
	CDebugOverlay( vgui::VPANEL parent );
	virtual ~CDebugOverlay( void );

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void Paint();
	virtual void OnTick( void );

	virtual bool ShouldDraw( void );

private:
	vgui::HFont			m_hFont;
};

//-----------------------------------------------------------------------------
// Purpose: Instances the overlay object
// Input  : *parent - 
//-----------------------------------------------------------------------------
CDebugOverlay::CDebugOverlay( vgui::VPANEL parent ) :
	BaseClass( NULL, "CDebugOverlay" )
{
	int w, h;
	vgui::surface()->GetScreenSize( w, h );
	SetParent( parent );
	SetSize( w, h );
	SetPos( 0, 0 );
	SetVisible( false );
	SetCursor( null );

	m_hFont = 0;
	SetFgColor( Color( 0, 0, 0, 0 ) );
	SetPaintBackgroundEnabled( false );

	// set the scheme before any child control is created
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL_TOOLS ), "resource/ClientScheme.res", "Client");
	SetScheme( scheme );
	
	vgui::ivgui()->AddTickSignal( GetVPanel(), 250 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDebugOverlay::~CDebugOverlay( void )
{
}

void CDebugOverlay::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	// Use a large font
//	m_hFont = pScheme->GetFont( "Default" );
	m_hFont = pScheme->GetFont( "DebugOverlay" );
	assert( m_hFont );

	int w, h;
	vgui::surface()->GetScreenSize( w, h );
	SetSize( w, h );
	SetPos( 0, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDebugOverlay::OnTick( void )
{
	bool bVisible = ShouldDraw();
	if ( IsVisible() != bVisible )
	{
		SetVisible( bVisible );
	}
}

bool CDebugOverlay::ShouldDraw( void )
{
	if ( debugoverlay->GetFirst() )
		return true;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Paints the 2D overlay items to the screen
//-----------------------------------------------------------------------------
void CDebugOverlay::Paint()
{
	OverlayText_t* pCurrText = debugoverlay->GetFirst();
	while (pCurrText) 
	{
		if ( pCurrText->text != NULL ) 
		{
			// --------------
			// Draw the text
			// --------------
			int r = pCurrText->r;
			int g = pCurrText->g;
			int b = pCurrText->b;
			int a = pCurrText->a;
			Vector screenPos;

			if (pCurrText->bUseOrigin)
			{
				if (!debugoverlay->ScreenPosition( pCurrText->origin, screenPos )) 
				{
					float xPos		= screenPos[0];
					float yPos		= screenPos[1]+ (pCurrText->lineOffset*13); // Line spacing;
					g_pMatSystemSurface->DrawColoredText( m_hFont, xPos, yPos, r, g, b, a, pCurrText->text );
				}
			}
			else
			{
				if (!debugoverlay->ScreenPosition( pCurrText->flXPos,pCurrText->flYPos, screenPos )) 
				{	
					float xPos		= screenPos[0];
					float yPos		= screenPos[1]+ (pCurrText->lineOffset*13); // Line spacing;
					g_pMatSystemSurface->DrawColoredText( m_hFont, xPos, yPos, r, g, b, a, pCurrText->text );
				}
			}
		}
		pCurrText = debugoverlay->GetNext( pCurrText );
	}

	debugoverlay->ClearDeadOverlays();
}

class CDebugOverlayPanel : public IDebugOverlayPanel
{
private:
	CDebugOverlay *debugOverlayPanel;
public:
	CDebugOverlayPanel( void )
	{
		debugOverlayPanel = NULL;
	}
	void Create( vgui::VPANEL parent )
	{
		debugOverlayPanel = new CDebugOverlay( parent );
	}
	void Destroy( void )
	{
		if ( debugOverlayPanel )
		{
			debugOverlayPanel->SetParent( (vgui::Panel *)NULL );
			delete debugOverlayPanel;
		}
	}
};

static CDebugOverlayPanel g_DebugOverlay;
IDebugOverlayPanel *debugoverlaypanel =  ( IDebugOverlayPanel * )&g_DebugOverlay;


void DebugDrawLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, int r, int g, int b, bool test, float duration )
{
	debugoverlay->AddLineOverlay( vecAbsStart + Vector( 0,0,0.1), vecAbsEnd + Vector( 0,0,0.1), r,g,b, test, duration );
}
