//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "tf_matchmaking_dashboard_notification.h"
#include "tf_matchmaking_dashboard.h"
#include "ienginevgui.h"
#include "clientmode_tf.h"
#include <vgui_controls/AnimationController.h>
#include "tf_matchmaking_dashboard_parent_manager.h"

using namespace vgui;
using namespace GCSDK;


CTFDashboardNotification::CTFDashboardNotification( ENotificationType eType,
													EAlignment eAlignment,
													float flLifetime,
													const char* pszName )
	: BaseClass( NULL, pszName )
	, m_eType ( eType )
	, m_eAlignment( eAlignment )
{
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ),
																 "resource/ClientScheme.res",
																 "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	AddActionSignalTarget( GetMMDashboard() );
	SetZPos( 15000 ); // "Really" high
	GetMMDashboardParentManager()->AddPanel( this );

	// We're going to tell ourselves to delete ourselves after a bit
	if ( flLifetime > 0.f )
	{
		SetToExpire( flLifetime );
	}
	PostActionSignal( new KeyValues( "NotificationCreated" ) );
}


CTFDashboardNotification::~CTFDashboardNotification()
{
	GetMMDashboardParentManager()->RemovePanel( this );
	PostActionSignal( new KeyValues( "NotificationCleared" ) );
}

void CTFDashboardNotification::SetToExpire( float flDelay )
{
	float flAlphaTime = Min( 1.f, flDelay );

	auto pAnim = g_pClientMode->GetViewportAnimationController();
	pAnim->RunAnimationCommand( this,
								"alpha",
								0,
								flDelay - flAlphaTime,
								flAlphaTime,
								AnimationController::INTERPOLATOR_LINEAR,
								0,
								true,
								false );

	PostMessage( this, new KeyValues( "Expire" ), flDelay );
}

void CTFDashboardNotification::OnExpire()
{
	MarkForDeletion();
}
