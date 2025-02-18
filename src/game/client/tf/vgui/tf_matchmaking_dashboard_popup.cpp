//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_matchmaking_dashboard.h"
#include "tf_gamerules.h"
#include "ienginevgui.h"
#include "clientmode_tf.h"
#include "tf_hud_disconnect_prompt.h"
#include "tf_gc_client.h"
#include "tf_party.h"
#include <vgui/ISurface.h>
#include "vgui_avatarimage.h"
#include "tf_matchmaking_dashboard_popup.h"
#include "tf_matchmaking_dashboard_parent_manager.h"
#include "tf_partyclient.h"

using namespace vgui;
using namespace GCSDK;

ConVar tf_mm_dashboard_spew_enabled( "tf_mm_dashboard_spew_enabled", "0", FCVAR_ARCHIVE );
#define MMDashboardSpew(...)																		\
	do {																							\
		if ( tf_mm_dashboard_spew_enabled.GetBool() )												\
		{																							\
			ConColorMsg( Color( 187, 80, 255, 255 ), "MMDashboard:" __VA_ARGS__ );					\
		} 																							\
	} while(false)																					\

extern ConVar tf_mm_next_map_vote_time;


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFMatchmakingPopup::CTFMatchmakingPopup( const char* pszName )
	: CExpandablePanel( NULL, pszName )
	, m_bActive( false )
{
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	ivgui()->AddTickSignal( GetVPanel(), 100 ); 

	SetKeyBoardInputEnabled( false );

	// Josh:
	// Despite 'zpos' being set in all the .res files there is some
	// vgui bug going on that depends on that zpos being set
	// earlier for the tree traversal to be correct.
	// If it gets set higher later on, it seemingly doesn't account for its children
	// correctly in the tree later on. (See VPanel::SetZPos)
	// TF2 has a lot of UI, and it is VERY likely that something depends
	// actually depends on this stupid bug.
	// So... just doing this for now.
	SetZPos( 9999 );

	GetMMDashboardParentManager()->AddPanel( this );

	SetProportional( true );
	
	ListenForGameEvent( "rematch_failed_to_create" );
	ListenForGameEvent( "party_updated" );
}

CTFMatchmakingPopup::~CTFMatchmakingPopup()
{
	GetMMDashboardParentManager()->RemovePanel( this );
}

void CTFMatchmakingPopup::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetMouseInputEnabled( true );

	// This cannot ever be true or else things get weird when in-game
	SetKeyBoardInputEnabled( false );

	if ( m_bActive )
	{
		OnEnter();
	}
	else
	{
		OnExit();
	}
}

void CTFMatchmakingPopup::OnThink()
{
	BaseClass::OnThink();

	if ( m_bActive )
	{
		OnUpdate();
	}
}

void CTFMatchmakingPopup::OnTick()
{
	BaseClass::OnTick();

	bool bShouldBeActive = ShouldBeActve();
	if ( bShouldBeActive != m_bActive )
	{
		if ( bShouldBeActive )
		{
			m_bActive = true;
			OnEnter();
		}
		else
		{
			m_bActive = false;
			OnExit();
		}
	}

	SetMouseInputEnabled( ShouldBeActve() );
	SetKeyBoardInputEnabled( false ); // Never
}

void CTFMatchmakingPopup::FireGameEvent( IGameEvent *pEvent )
{
	if ( FStrEq( pEvent->GetName(), "party_updated" ) )
	{
		if ( ShouldBeActve() )
		{
			Update();
		}
	}
}

void CTFMatchmakingPopup::OnEnter()
{
	MMDashboardSpew( "Entering state %s\n", GetName() );

	Update();

	SetCollapsed( false );
}

void CTFMatchmakingPopup::OnUpdate() 
{
}

void CTFMatchmakingPopup::OnExit()
{
	MMDashboardSpew( "Exiting state %s\n", GetName() );
	SetCollapsed( true );
}


void CTFMatchmakingPopup::Update()
{
}
