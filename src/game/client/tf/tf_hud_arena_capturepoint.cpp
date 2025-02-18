//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/EditablePanel.h>
#include "tf_imagepanel.h"
#include "tf_gamerules.h"
#include "c_tf_team.h"
#include "tf_hud_freezepanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

extern ConVar tf_arena_preround_time;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudArenaCapPointCountdown : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudArenaCapPointCountdown, EditablePanel );

public:
	CHudArenaCapPointCountdown( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual void	OnTick( void );
	virtual bool	ShouldDraw( void );
	virtual bool	IsVisible( void );


private:

	bool			m_bFire5SecRemain;
	bool			m_bFire4SecRemain;
	bool			m_bFire3SecRemain;
	bool			m_bFire2SecRemain;
	bool			m_bFire1SecRemain;
	bool			m_bFire0SecRemain;

};

DECLARE_HUDELEMENT( CHudArenaCapPointCountdown );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudArenaCapPointCountdown::CHudArenaCapPointCountdown( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudArenaCapPointCountdown" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	vgui::ivgui()->AddTickSignal( GetVPanel() );
	SetVisible( true );

	m_bFire5SecRemain = true;
	m_bFire4SecRemain = true;
	m_bFire3SecRemain = true;
	m_bFire2SecRemain = true;
	m_bFire1SecRemain = true;
	m_bFire0SecRemain = true;

	RegisterForRenderGroup( "mid" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaCapPointCountdown::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudArenaCapPointCountdown.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	SetVisible( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudArenaCapPointCountdown::IsVisible( void )
{
	if ( IsInFreezeCam() == true )
		return false;

	if ( ShouldDraw() == false )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaCapPointCountdown::OnTick( void )
{
	BaseClass::OnTick();

	if ( TFGameRules() == NULL || ( TFGameRules() && TFGameRules()->IsInArenaMode() == false ) )
	{
		SetVisible( false );
		return;
	}

	if ( TFGameRules()->State_Get() != GR_STATE_STALEMATE || ShouldDraw() == false )
	{
		SetVisible( false );
		return;
	}

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
	{
		SetVisible( false );
		return;
	}

	int iTimeLeft = ceil( TFGameRules()->GetCapturePointTime() - gpGlobals->curtime );
	

	if ( iTimeLeft > 5 || iTimeLeft <= 0 )
	{
		if ( iTimeLeft <= 0 && m_bFire0SecRemain )
		{
			m_bFire0SecRemain = false;
			pLocalPlayer->EmitSound( "Announcer.AM_CapEnabledRandom" );
		}

		m_bFire5SecRemain = true;
		m_bFire4SecRemain = true;
		m_bFire3SecRemain = true;
		m_bFire2SecRemain = true;
		m_bFire1SecRemain = true;
	
		SetVisible( false );
		return;
	}


	SetVisible( true );

	wchar_t wzTimeLeft[128];
	_snwprintf( wzTimeLeft, ARRAYSIZE( wzTimeLeft ), L"%i", iTimeLeft );

	SetDialogVariable( "capturetime", wzTimeLeft );

	if ( iTimeLeft <= 5 && m_bFire5SecRemain )
	{
		m_bFire5SecRemain = false;
		pLocalPlayer->EmitSound( "Announcer.RoundBegins5Seconds" );
	}
	else if ( iTimeLeft <= 4 && m_bFire4SecRemain )
	{
		m_bFire4SecRemain = false;
		pLocalPlayer->EmitSound( "Announcer.RoundBegins4Seconds" );
	}
	else if ( iTimeLeft <= 3 && m_bFire3SecRemain )
	{
		m_bFire3SecRemain = false;
		pLocalPlayer->EmitSound( "Announcer.RoundBegins3Seconds" );
	}
	else if ( iTimeLeft <= 2 && m_bFire2SecRemain )
	{
		m_bFire2SecRemain = false;
		pLocalPlayer->EmitSound( "Announcer.RoundBegins2Seconds" );
	}
	else if ( iTimeLeft <= 1 && m_bFire1SecRemain )
	{
		m_bFire1SecRemain = false;
		m_bFire0SecRemain = true;
		pLocalPlayer->EmitSound( "Announcer.RoundBegins1Seconds" );
	}
}

bool CHudArenaCapPointCountdown::ShouldDraw( void )
{
	return CHudElement::ShouldDraw();
}