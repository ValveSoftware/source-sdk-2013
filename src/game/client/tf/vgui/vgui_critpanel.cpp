//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include "iclientmode.h"
#include "tf_shareddefs.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>
#include "VGuiMatSurface/IMatSystemSurface.h"

#include "c_tf_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar cl_showcrit( "cl_showcrit", "0", FCVAR_DEVELOPMENTONLY, "Debug! Draw crit values above the ammo count." );

//-----------------------------------------------------------------------------
// Purpose: Critical meter panel.
//-----------------------------------------------------------------------------
class CCriticalPanel : public CHudElement, public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CCriticalPanel, vgui::EditablePanel );

	CCriticalPanel( const char *pElementName );
	virtual			~CCriticalPanel( void );

	virtual void Reset();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool ShouldDraw( void );

protected:

	virtual void OnThink();

private:

	void InitCritData( void );

	// vgui
	vgui::HFont		m_hFont;
	vgui::Label		*m_pTextLabel;

	float			m_flNextThink;

	// Critical Data.
	struct CriticalData_t 
	{
		float	m_flCurrent;
		float	m_flAverage;
		float	m_flLow;
		float	m_flHigh;
	};

	bool			m_bInitData;
	CriticalData_t	m_CritData;
};

DECLARE_HUDELEMENT( CCriticalPanel );

#define CRITICAL_PANEL_WIDTH	300
#define CRITICAL_BLEND_WEIGHT   0.1f

//-----------------------------------------------------------------------------
// Purpose: Constructor.
// Input  : *parent - parent VGUI window
//-----------------------------------------------------------------------------
CCriticalPanel::CCriticalPanel( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "CriticalPanel" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	SetScheme( "ClientScheme" );

	SetVisible( false );
	SetCursor( null );

	SetFgColor( Color( 0, 0, 0, 255 ) );
	SetPaintBackgroundEnabled( false );

	m_pTextLabel = new vgui::Label( this, "CriticalPanelLabel", "" );
	m_hFont = 0;

	// Have we initialize the criticl data yet?
	m_bInitData = false;

	m_flNextThink = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CCriticalPanel::~CCriticalPanel( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCriticalPanel::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCriticalPanel::InitCritData( void )
{
	m_CritData.m_flCurrent = -1.0f;
	m_CritData.m_flAverage = -1.0f;
	m_CritData.m_flLow = -1.0f;
	m_CritData.m_flHigh = -1.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCriticalPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pTextLabel->SetBounds( 0.0f, 0.0f, GetWide(), GetTall() );
	m_pTextLabel->SetFont( pScheme->GetFont( "DefaultSmall" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CCriticalPanel::ShouldDraw( void )
{
	return false;

	if ( ( !cl_showcrit.GetInt() || ( gpGlobals->absoluteframetime <= 0 ) ) &&
		 ( !cl_showcrit.GetInt() ) )
	{
		m_bInitData = false;
		return false;
	}

	if ( !m_bInitData )
	{
		m_bInitData = true;
		InitCritData();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GetCritColor( float flCritMult, unsigned char ucColor[3] )
{
	ucColor[0] = 255; ucColor[1] = 255; ucColor[2] = 0;

	if ( flCritMult < 5.0f )
	{
		ucColor[1] = 0;
	}
	else if ( flCritMult > 5.0f )
	{
		ucColor[0] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCriticalPanel::OnThink()
{
	// Get the local TF player.
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	if ( cl_showcrit.GetInt() )
	{
		// Get the current critical multiplier.
		float flCritMult = pPlayer->GetCritMult() * TF_DAMAGE_CRIT_CHANCE;
		flCritMult *= 100.0f;

		if ( m_CritData.m_flAverage < 0.0f )
		{
			// Initial data.
			m_CritData.m_flCurrent = flCritMult;
			m_CritData.m_flAverage = flCritMult;
			m_CritData.m_flLow = m_CritData.m_flAverage;
			m_CritData.m_flHigh = m_CritData.m_flAverage;
		} 
		else
		{				
			// Average over time.
			m_CritData.m_flCurrent = flCritMult;
			m_CritData.m_flAverage *= ( 1.0f - CRITICAL_BLEND_WEIGHT ) ;
			m_CritData.m_flAverage += ( flCritMult * CRITICAL_BLEND_WEIGHT );
		}

		// Adjust for highs and lows.
		m_CritData.m_flLow = MIN( m_CritData.m_flLow, flCritMult );
		m_CritData.m_flHigh = MAX( m_CritData.m_flHigh, flCritMult );

		unsigned char ucColor[3];
		GetCritColor( flCritMult, ucColor );
		m_pTextLabel->SetFgColor( Color( ucColor[0], ucColor[1], ucColor[2], 255 ) );

		char szCriticalText[256];
		//Q_snprintf( szCriticalText, sizeof( szCriticalText ), "Crit: %3.2f (A:%3.2f, L:%3.2f, H:%3.2f)",
		//						m_CritData.m_flCurrent, m_CritData.m_flAverage, m_CritData.m_flLow, m_CritData.m_flHigh );
		Q_snprintf( szCriticalText, sizeof( szCriticalText ), "Crit Chance: %3.2f", m_CritData.m_flCurrent );

		m_pTextLabel->SetText( szCriticalText );

		m_flNextThink = gpGlobals->curtime + 0.01f;
	}
	else
	{
		m_flNextThink = gpGlobals->curtime + 0.1f;
	}
}

#if 0
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
//-----------------------------------------------------------------------------
void CCriticalPanel::Paint() 
{
	if ( cl_showcrit.GetInt() )
	{
		unsigned char ucColor[3];
		GetCritColor( m_CritData.m_flCurrent, ucColor );
		g_pMatSystemSurface->DrawColoredText( m_hFont, 0, 2, ucColor[0], ucColor[1], ucColor[2], 255, 
			"%2.2f (Avg:%2.2f, Low:%2.2f, High:%2.2f)", 
			m_CritData.m_flCurrent, m_CritData.m_flAverage, m_CritData.m_flLow, m_CritData.m_flHigh );
	}
}
#endif
