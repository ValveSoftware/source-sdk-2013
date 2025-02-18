//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "hud.h"
#include "iclientmode.h"
#include "c_baseobject.h"
#include "c_tf_player.h"
#include "ienginevgui.h"
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include <vgui/IVGui.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/AnimationController.h>
#include "game_controls/IconPanel.h"
#include "teamplay_round_timer.h"

#include "tf_hud_building_status.h"

#include "c_obj_sentrygun.h"
#include "c_obj_dispenser.h"
#include "c_obj_teleporter.h"
#include "c_obj_sapper.h"

#include "tf_gamerules.h"
#include "tf_logic_halloween_2014.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_ObjectSentrygun;

extern CUtlVector<int> g_TeamRoundTimers;

using namespace vgui;

ConVar tf_hud_num_building_alert_beeps( "tf_hud_num_building_alert_beeps", "2", FCVAR_ARCHIVE, "Number of times to play warning sound when a new alert displays on building hud objects", true, 0, false, 0 );

//============================================================================

DECLARE_BUILD_FACTORY( CBuildingHealthBar );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBuildingHealthBar::CBuildingHealthBar(Panel *parent, const char *panelName) : vgui::ProgressBar( parent, panelName )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuildingHealthBar::Paint()
{
	if ( _progress < 0.5 )
	{
		SetFgColor( m_cLowHealthColor );
	}
	else
	{
		SetFgColor( m_cHealthColor );
	}

	BaseClass::Paint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuildingHealthBar::PaintBackground()
{
	// save progress and real fg color
	float flProgress = _progress;
	Color fgColor = GetFgColor();

	// stuff our fake info
	_progress = 1.0;
	SetFgColor( GetBgColor() );

	BaseClass::Paint();

	// restore actual progress / color
	_progress = flProgress;
	SetFgColor( fgColor );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuildingHealthBar::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetBgColor(GetSchemeColor("BuildingHealthBar.BgColor", pScheme));
	m_cHealthColor = GetSchemeColor("BuildingHealthBar.Health", pScheme);
	m_cLowHealthColor = GetSchemeColor("BuildingHealthBar.LowHealth", pScheme);
	SetBorder(NULL);
}

//============================================================================

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBuildingStatusItem::CBuildingStatusItem( Panel *parent, const char *szLayout, int iObjectType, int iObjectMode=0 ) :
BaseClass( parent, "BuildingStatusItem" )
{
	SetProportional( true );

	// Save our layout file for re-loading
	Q_strncpy( m_szLayout, szLayout, sizeof(m_szLayout) );

	// load control settings...
	LoadControlSettings( szLayout );

	SetPositioned( false );

	m_pObject = NULL;
	m_iObjectType = iObjectType;
	m_iObjectMode = iObjectMode;

	m_pBuiltPanel = new vgui::EditablePanel( this, "BuiltPanel" );
	m_pNotBuiltPanel = new vgui::EditablePanel( this, "NotBuiltPanel" );

	// sub panels
	m_pBuildingPanel = new vgui::EditablePanel( m_pBuiltPanel, "BuildingPanel" );
	m_pRunningPanel = new vgui::EditablePanel( m_pBuiltPanel, "RunningPanel" );

	// Shared between All sub panels
	m_pBackground = new CIconPanel( this, "Background" );

	// Running and Building sub panels only
	m_pHealthBar = new CBuildingHealthBar( m_pBuiltPanel, "Health" );
	m_pHealthBar->SetSegmentInfo( YRES(1), YRES(3) );
	m_pHealthBar->SetProgressDirection( ProgressBar::PROGRESS_NORTH );
	m_pHealthBar->SetBarInset( 0 );

	m_pBuildingProgress = new vgui::ContinuousProgressBar( m_pBuildingPanel, "BuildingProgress" ); 

	m_pAlertTray = new CBuildingStatusAlertTray( m_pBuiltPanel, "AlertTray" );
	m_pWrenchIcon = new CIconPanel( m_pBuiltPanel, "WrenchIcon" );
	m_pSapperIcon = new CIconPanel( m_pBuiltPanel, "SapperIcon" );

	m_pUpgradeIcons[0] = new CIconPanel( m_pBuiltPanel, "Icon_Upgrade_1" );
	m_pUpgradeIcons[1] = new CIconPanel( m_pBuiltPanel, "Icon_Upgrade_2" );
	m_pUpgradeIcons[2] = new CIconPanel( m_pBuiltPanel, "Icon_Upgrade_3" );

	m_iUpgradeLevel = 1;

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuildingStatusItem::ApplySchemeSettings( IScheme *pScheme )
{
	// This lets us use hud_reloadscheme to reload the status items
	int x, y;
	GetPos( x, y );

	LoadControlSettings( m_szLayout );

	SetPos( x, y );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: Calc visibility of subpanels
//-----------------------------------------------------------------------------
void CBuildingStatusItem::PerformLayout( void )
{
	BaseClass::PerformLayout();

	C_BaseObject *pObj = m_pObject.Get();

	m_bActive = ( pObj != NULL );

	m_pHealthBar->SetVisible( m_bActive );

	m_pNotBuiltPanel->SetVisible( !m_bActive );
	m_pBuiltPanel->SetVisible( m_bActive );

	if ( pObj )
	{
		// redo the background
		m_pBackground->SetIcon( GetBackgroundImage() );

		if ( pObj->IsBuilding() )
		{
			m_pBuildingPanel->SetVisible( true );
			m_pRunningPanel->SetVisible( false );

			m_pUpgradeIcons[0]->SetVisible( false );
			m_pUpgradeIcons[1]->SetVisible( false );
			m_pUpgradeIcons[2]->SetVisible( false );
		}
		else
		{
			m_pBuildingPanel->SetVisible( false );
			m_pRunningPanel->SetVisible( true );

			int iUpgradeLevel = pObj->GetUpgradeLevel();

			Assert( iUpgradeLevel >= 1 && iUpgradeLevel <= 3 );

			m_pUpgradeIcons[0]->SetVisible( false );
			m_pUpgradeIcons[1]->SetVisible( false );
			m_pUpgradeIcons[2]->SetVisible( false );

			// show the correct upgrade level icon
			if ( !pObj->IsMiniBuilding() )
			{
				m_pUpgradeIcons[iUpgradeLevel-1]->SetVisible( true );
			}
		}
	}
	else
	{
		// redo the background
		m_pBackground->SetIcon( GetInactiveBackgroundImage() );

		if ( m_pAlertTray->IsTrayOut() )
		{
			m_pAlertTray->HideTray();
			m_pWrenchIcon->SetVisible( false );
			m_pSapperIcon->SetVisible( false );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CBuildingStatusItem::LevelInit( void )
{
	if ( m_pAlertTray )
		m_pAlertTray->LevelInit();
}

//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CBuildingStatusItem::SetObject( C_BaseObject *pObj )
{
	m_pObject = pObj;

	Assert( !pObj || ( pObj && !pObj->IsMarkedForDeletion() ) );

	if ( !pObj )
	{
		m_pAlertTray->HideTray();
		m_pWrenchIcon->SetVisible( false );
		m_pSapperIcon->SetVisible( false );
		m_pAlertTray->SetAlertType( BUILDING_HUD_ALERT_NONE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuildingStatusItem::Paint( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuildingStatusItem::PaintBackground( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CBuildingStatusItem::GetBackgroundImage( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	const char *pResult = "obj_status_background_blue";

	if ( !pLocalPlayer )
	{
		Assert( 0 );
		return pResult;
	}

	switch( pLocalPlayer->GetTeamNumber() )
	{
	case TF_TEAM_BLUE:
		pResult = "obj_status_background_blue";
		break;
	case TF_TEAM_RED:
		pResult = "obj_status_background_red";
		break;
	default:
		break;
	}

	return pResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CBuildingStatusItem::GetInactiveBackgroundImage( void )
{
	return "obj_status_background_disabled";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuildingStatusItem::OnTick()
{
	// We only tick while active and with a valid built object
	C_BaseObject *pObj = GetRepresentativeObject();

	if ( !pObj )	// implies not active
	{
		if ( m_bActive )
		{
			// we lost our object. force relayout to inactive mode
			InvalidateLayout();

			// tell our parent that we're gone
			IGameEvent *event = gameeventmanager->CreateEvent( "building_info_changed" );
			if ( event )
			{
				event->SetInt( "building_type", GetRepresentativeObjectType() );
				event->SetInt( "object_mode", GetRepresentativeObjectMode() );
				gameeventmanager->FireEventClientSide( event );
			}
		}

		// We don't want to tick while inactive regardless
		return;
	}

	float flHealth = (float)pObj->GetHealth() / (float)pObj->GetMaxHealth();

	m_pHealthBar->SetProgress( flHealth );

	if ( pObj->IsBuilding() )
	{
		m_pBuildingPanel->SetVisible( true );
		m_pRunningPanel->SetVisible( false );

		m_pBuildingProgress->SetProgress( pObj->GetPercentageConstructed() );
	}
	else
	{
		m_pBuildingPanel->SetVisible( false );
		m_pRunningPanel->SetVisible( true );
	}

	// what is our current alert state?
	BuildingHudAlert_t alertLevel = pObj->GetBuildingAlertLevel();

	if ( alertLevel <= BUILDING_HUD_ALERT_NONE )
	{
		// if the tray is out, hide it
		if ( m_pAlertTray->IsTrayOut() )
		{
			m_pAlertTray->HideTray();
			m_pWrenchIcon->SetVisible( false );
			m_pSapperIcon->SetVisible( false );
		}
	}
	else
	{
		m_pWrenchIcon->SetVisible( false );
		m_pSapperIcon->SetVisible( false );

		bool bShowAlertTray = false;
		bool bAlertTrayFullyDeployed = m_pAlertTray->GetPercentDeployed() >= 1.0f;
		switch( alertLevel )
		{
			// show low ammo for normal sentry and mini-sentry
		case BUILDING_HUD_ALERT_LOW_AMMO:
		case BUILDING_HUD_ALERT_VERY_LOW_AMMO:
			bShowAlertTray = true;
			m_pWrenchIcon->SetVisible( bAlertTrayFullyDeployed );
			break;

			// do not show low health for the disposable mini-sentry
		case BUILDING_HUD_ALERT_LOW_HEALTH:
		case BUILDING_HUD_ALERT_VERY_LOW_HEALTH:
			bShowAlertTray = pObj->IsDisposableBuilding() == false;
			m_pWrenchIcon->SetVisible( bAlertTrayFullyDeployed && bShowAlertTray );
			break;

			// always show when being sapped
		case BUILDING_HUD_ALERT_SAPPER:
			bShowAlertTray = true;
			m_pSapperIcon->SetVisible( bAlertTrayFullyDeployed );
			break;

		default:
			bShowAlertTray = false;
			break;
		}

		if ( bShowAlertTray && !pObj->IsDisposableBuilding() )
		{
			if ( !m_pAlertTray->IsTrayOut() )
			{
				m_pAlertTray->ShowTray();
			}
			m_pAlertTray->SetAlertType( alertLevel );
		}
		else
		{
			if ( m_pAlertTray->IsTrayOut() )
			{
				m_pAlertTray->HideTray();
			}
			m_pAlertTray->SetAlertType( BUILDING_HUD_ALERT_NONE );
		}
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseObject *CBuildingStatusItem::GetRepresentativeObject( void )
{
	if ( !m_bActive )
	{
		return NULL;
	}
	else
	{
		return m_pObject.Get();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBuildingStatusItem::GetRepresentativeObjectType( void )
{
	return m_iObjectType;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBuildingStatusItem::GetRepresentativeObjectMode( void )
{
	return m_iObjectMode;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBuildingStatusItem::GetObjectPriority( void )
{
	int nPriority = GetObjectInfo( GetRepresentativeObjectType() )->m_iDisplayPriority;

	// MvM hack to sort buildings properly since we can have more than one sentry via upgrades
	if ( GetRepresentativeObjectType() == OBJ_SENTRYGUN && GetRepresentativeObjectMode() == MODE_SENTRYGUN_DISPOSABLE )
	{
		nPriority = 0;
	}

	return nPriority;	
}

//============================================================================


DECLARE_BUILD_FACTORY( CBuildingStatusAlertTray );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBuildingStatusAlertTray::CBuildingStatusAlertTray(Panel *parent, const char *panelName) : BaseClass( parent, panelName )
{
	m_pAlertPanelMaterial = NULL;
	m_flAlertDeployedPercent = 0.0f;
	m_bIsTrayOut = false;

	m_pAlertPanelHudTexture = NULL;
	m_pAlertPanelMaterial = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuildingStatusAlertTray::ApplySettings( KeyValues *inResourceData )
{
	m_pAlertPanelHudTexture = gHUD.GetIcon( inResourceData->GetString( "icon", "" ) );

	if ( m_pAlertPanelHudTexture )
	{
		m_pAlertPanelMaterial = materials->FindMaterial( m_pAlertPanelHudTexture->szTextureFile, TEXTURE_GROUP_VGUI );
	}

	BaseClass::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuildingStatusAlertTray::Paint( void )
{
	// Paint the alert tray
	if ( !m_pAlertPanelMaterial || !m_pAlertPanelHudTexture )
	{
		return;
	}

	int x = 0;
	int y = 0;
	ipanel()->GetAbsPos(GetVPanel(), x,y );
	int iWidth = GetWide();
	int iHeight = GetTall();

	// Position the alert panel image based on the deployed percent
	float flXa = m_pAlertPanelHudTexture->texCoords[0];
	float flXb = m_pAlertPanelHudTexture->texCoords[2];
	float flYa = m_pAlertPanelHudTexture->texCoords[1];
	float flYb = m_pAlertPanelHudTexture->texCoords[3];

	float flMaskXa = flXa;
	float flMaskXb = flXb;
	float flMaskYa = flYa;
	float flMaskYb = flYb;

	float flFrameDelta = ( flXb - flXa ) * ( 1.0 - m_flAlertDeployedPercent );

	flXa += flFrameDelta;
	flXb += flFrameDelta; 

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_pAlertPanelMaterial );
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true );

	int r, g, b, a;
	r = a = 255;

	switch( m_lastAlertType )
	{
	case BUILDING_HUD_ALERT_VERY_LOW_AMMO:
	case BUILDING_HUD_ALERT_VERY_LOW_HEALTH:
		g = b = (int)( 127.0f + 127.0f * cos( gpGlobals->curtime * 2.0f * M_PI * 0.5 ) );
		break;

	case BUILDING_HUD_ALERT_SAPPER:
		g = b = (int)( 127.0f + 127.0f * cos( gpGlobals->curtime * 2.0f * M_PI * 1.5 ) );
		break;

	case BUILDING_HUD_ALERT_LOW_AMMO:
	case BUILDING_HUD_ALERT_LOW_HEALTH:
	case BUILDING_HUD_ALERT_NONE:
	default:
		g = b = 255;
		break;
	}

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Position3f( x, y, 0.0f );
	meshBuilder.TexCoord2f( 0, flXa, flYa );
	meshBuilder.TexCoord2f( 1, flMaskXa, flMaskYa );
	meshBuilder.Color4ub( r, g, b, a );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x + iWidth, y, 0.0f );
	meshBuilder.TexCoord2f( 0, flXb, flYa );
	meshBuilder.TexCoord2f( 1, flMaskXb, flMaskYa );
	meshBuilder.Color4ub( r, g, b, a );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x + iWidth, y + iHeight, 0.0f );
	meshBuilder.TexCoord2f( 0, flXb, flYb );
	meshBuilder.TexCoord2f( 1, flMaskXb, flMaskYb );
	meshBuilder.Color4ub( r, g, b, a );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x, y + iHeight, 0.0f );
	meshBuilder.TexCoord2f( 0, flXa, flYb );
	meshBuilder.TexCoord2f( 1, flMaskXa, flMaskYb );
	meshBuilder.Color4ub( r, g, b, a );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

void CBuildingStatusAlertTray::PaintBackground( void )
{
}

void CBuildingStatusAlertTray::ShowTray( void )
{
	if ( m_bIsTrayOut == false )
	{
		m_flAlertDeployedPercent = 0.0;
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "deployed", 1.0, 0.0, 0.3, AnimationController::INTERPOLATOR_LINEAR );

		m_bIsTrayOut = true;
	}
}

void CBuildingStatusAlertTray::HideTray( void )
{
	if ( m_bIsTrayOut == true )
	{
		m_flAlertDeployedPercent = 1.0;
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "deployed", 0.0, 0.0, 0.3, AnimationController::INTERPOLATOR_LINEAR );

		m_bIsTrayOut = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CBuildingStatusAlertTray::LevelInit( void )
{
	m_bIsTrayOut = false;
	m_flAlertDeployedPercent = 0.0f;
}

void CBuildingStatusAlertTray::SetAlertType( BuildingHudAlert_t alertLevel )
{	
	m_lastAlertType = alertLevel;
}

//============================================================================

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBuildingStatusItem_SentryGun::CBuildingStatusItem_SentryGun( Panel *parent ) :
CBuildingStatusItem( parent, "resource/UI/hud_obj_sentrygun.res", OBJ_SENTRYGUN, MODE_SENTRYGUN_NORMAL )
{
	m_pShellsProgress = new vgui::ContinuousProgressBar( GetRunningPanel(), "Shells" );
	m_pRocketsProgress = new vgui::ContinuousProgressBar( GetRunningPanel(), "Rockets" );
	m_pUpgradeProgress = new vgui::ContinuousProgressBar( GetRunningPanel(), "Upgrade" );

	m_pRocketIcon = new vgui::ImagePanel( GetRunningPanel(), "RocketIcon" );
	m_pUpgradeIcon = new CIconPanel( GetRunningPanel(), "UpgradeIcon" );

	m_pSentryIcons[0] = new CIconPanel( this, "Icon_Sentry_1" );
	m_pSentryIcons[1] = new CIconPanel( this, "Icon_Sentry_2" );
	m_pSentryIcons[2] = new CIconPanel( this, "Icon_Sentry_3" );

	m_iUpgradeLevel = 1;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuildingStatusItem_SentryGun::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	m_cLowAmmoColor = scheme->GetColor( "LowHealthRed", Color(255,0,0,255) );
	m_cNormalAmmoColor = scheme->GetColor( "ProgressOffWhite", Color(255,255,255,255) );
}

//-----------------------------------------------------------------------------
// Purpose: Calc visibility of subpanels
//-----------------------------------------------------------------------------
void CBuildingStatusItem_SentryGun::PerformLayout( void )
{
	BaseClass::PerformLayout();

	C_ObjectSentrygun *pSentrygun = dynamic_cast<C_ObjectSentrygun *>( GetRepresentativeObject() );

	if ( !pSentrygun || ( pSentrygun && pSentrygun->IsDisposableBuilding() ) )
	{
		return;
	}

	GetRunningPanel()->SetDialogVariable( "numkills", pSentrygun->GetKills() );
	GetRunningPanel()->SetDialogVariable( "numassists", pSentrygun->GetAssists() );

	int iShells, iMaxShells;
	int iRockets, iMaxRockets;
	pSentrygun->GetAmmoCount( iShells, iMaxShells, iRockets, iMaxRockets );

	// Shells label
	float flShells = (float)iShells / (float)iMaxShells;
	m_pShellsProgress->SetProgress( flShells );

	if ( flShells < 0.25f )
	{
		m_pShellsProgress->SetFgColor( m_cLowAmmoColor );
	}
	else
	{
		m_pShellsProgress->SetFgColor( m_cNormalAmmoColor );
	}

	// Rockets label
	float flRockets = (float)iRockets / (float)SENTRYGUN_MAX_ROCKETS;
	m_pRocketsProgress->SetProgress( flRockets );

	if ( flRockets < 0.25f )
	{
		m_pRocketsProgress->SetFgColor( m_cLowAmmoColor );
	}
	else
	{
		m_pRocketsProgress->SetFgColor( m_cNormalAmmoColor );
	}

	int iUpgradeLevel = pSentrygun->GetUpgradeLevel();

	Assert( iUpgradeLevel >= 1 && iUpgradeLevel <= 3 );

	// show the correct icon
	m_pSentryIcons[0]->SetVisible( false );
	m_pSentryIcons[1]->SetVisible( false );
	m_pSentryIcons[2]->SetVisible( false );
	m_pSentryIcons[iUpgradeLevel-1]->SetVisible( true );

	// upgrade progress
	int iMetal = pSentrygun->GetUpgradeMetal();
	int iMetalRequired = pSentrygun->GetUpgradeMetalRequired();
	float flUpgrade = (float)iMetal / (float)iMetalRequired;
	m_pUpgradeProgress->SetProgress( flUpgrade );

	// upgrade label only in 1 or 2
	m_pUpgradeIcon->SetVisible( iUpgradeLevel < 3 );
	m_pUpgradeProgress->SetVisible( iUpgradeLevel < 3 );

	// rockets label only in 3
	m_pRocketIcon->SetVisible( iUpgradeLevel == 3 );
	m_pRocketsProgress->SetVisible( iUpgradeLevel == 3 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuildingStatusItem_SentryGun::OnTick()
{
	BaseClass::OnTick();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CBuildingStatusItem_SentryGun::GetBackgroundImage( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	const char *pResult = "obj_status_background_tall_blue";

	if ( !pLocalPlayer )
	{
		return pResult;
	}

	switch( pLocalPlayer->GetTeamNumber() )
	{
	case TF_TEAM_BLUE:
		pResult = "obj_status_background_tall_blue";
		break;
	case TF_TEAM_RED:
		pResult = "obj_status_background_tall_red";
		break;
	default:
		break;
	}

	return pResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CBuildingStatusItem_SentryGun::GetInactiveBackgroundImage( void )
{
	return "obj_status_background_tall_disabled";
}

//============================================================================

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBuildingStatusItem_SentryGun_Disposable::CBuildingStatusItem_SentryGun_Disposable( Panel *parent ) :
CBuildingStatusItem( parent, "resource/UI/hud_obj_sentrygun_disp.res", OBJ_SENTRYGUN, MODE_SENTRYGUN_DISPOSABLE )
{
	m_pShellsProgress = new vgui::ContinuousProgressBar( GetRunningPanel(), "Shells" );

	m_pUpgradeIcon = new CIconPanel( GetRunningPanel(), "UpgradeIcon" );

	m_pSentryIcons[0] = new CIconPanel( this, "Icon_Sentry_1" );
	m_pSentryIcons[1] = new CIconPanel( this, "Icon_Sentry_2" );
	m_pSentryIcons[2] = new CIconPanel( this, "Icon_Sentry_3" );

	m_iUpgradeLevel = 1;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuildingStatusItem_SentryGun_Disposable::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	m_cLowAmmoColor = scheme->GetColor( "LowHealthRed", Color(255,0,0,255) );
	m_cNormalAmmoColor = scheme->GetColor( "ProgressOffWhite", Color(255,255,255,255) );
}

//-----------------------------------------------------------------------------
// Purpose: Calc visibility of subpanels
//-----------------------------------------------------------------------------
void CBuildingStatusItem_SentryGun_Disposable::PerformLayout( void )
{
	BaseClass::PerformLayout();

	C_ObjectSentrygun *pSentrygun = dynamic_cast<C_ObjectSentrygun *>( GetRepresentativeObject() );

	if ( !pSentrygun || ( pSentrygun && !pSentrygun->IsDisposableBuilding() ) )
	{
		return;
	}

	GetRunningPanel()->SetDialogVariable( "numkills", pSentrygun->GetKills() );
	GetRunningPanel()->SetDialogVariable( "numassists", pSentrygun->GetAssists() );

	int iShells, iMaxShells;
	int iRockets, iMaxRockets;
	pSentrygun->GetAmmoCount( iShells, iMaxShells, iRockets, iMaxRockets );

	// Shells label
	float flShells = (float)iShells / (float)iMaxShells;
	m_pShellsProgress->SetProgress( flShells );

	if ( flShells < 0.25f )
	{
		m_pShellsProgress->SetFgColor( m_cLowAmmoColor );
	}
	else
	{
		m_pShellsProgress->SetFgColor( m_cNormalAmmoColor );
	}

	int iUpgradeLevel = pSentrygun->GetUpgradeLevel();

	Assert( iUpgradeLevel >= 1 && iUpgradeLevel <= 3 );

	// show the correct icon
	m_pSentryIcons[0]->SetVisible( false );
	m_pSentryIcons[1]->SetVisible( false );
	m_pSentryIcons[2]->SetVisible( false );
	m_pSentryIcons[iUpgradeLevel-1]->SetVisible( true );

	m_pUpgradeIcon->SetEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuildingStatusItem_SentryGun_Disposable::OnTick()
{
	BaseClass::OnTick();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CBuildingStatusItem_SentryGun_Disposable::GetBackgroundImage( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	const char *pResult = "obj_status_background_tall_blue";

	if ( !pLocalPlayer )
	{
		return pResult;
	}

	switch( pLocalPlayer->GetTeamNumber() )
	{
	case TF_TEAM_BLUE:
		pResult = "obj_status_background_blue";
		break;
	case TF_TEAM_RED:
		pResult = "obj_status_background_red";
		break;
	default:
		break;
	}

	return pResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CBuildingStatusItem_SentryGun_Disposable::GetInactiveBackgroundImage( void )
{
	return "obj_status_background_disabled";
}

//============================================================================

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBuildingStatusItem_Dispenser::CBuildingStatusItem_Dispenser( Panel *parent ) :
CBuildingStatusItem( parent, "resource/UI/hud_obj_dispenser.res", OBJ_DISPENSER )
{
	m_pAmmoProgress = new vgui::ContinuousProgressBar( GetRunningPanel(), "Ammo" );
	m_pUpgradeProgress = new vgui::ContinuousProgressBar( GetRunningPanel(), "Upgrade" );

	m_pUpgradeIcon = new CIconPanel( GetRunningPanel(), "UpgradeIcon" );

}

//-----------------------------------------------------------------------------
// Purpose: Calc visibility of subpanels
//-----------------------------------------------------------------------------
void CBuildingStatusItem_Dispenser::PerformLayout( void )
{
	BaseClass::PerformLayout();

	C_ObjectDispenser *pDispenser = static_cast<C_ObjectDispenser*>(GetRepresentativeObject());

	if ( !pDispenser )
	{
		return;
	}

	int iAmmo = pDispenser->GetMetalAmmoCount();

	float flMaxMetal = pDispenser->IsMiniBuilding() ? MINI_DISPENSER_MAX_METAL : DISPENSER_MAX_METAL_AMMO;
	float flProgress = (float)iAmmo / flMaxMetal;
	m_pAmmoProgress->SetProgress( flProgress );

	int iUpgradeLevel = pDispenser->GetUpgradeLevel();

	Assert( iUpgradeLevel >= 1 && iUpgradeLevel <= 3 );

	// upgrade progress
	int iMetal = pDispenser->GetUpgradeMetal();
	int iMetalRequired = pDispenser->GetUpgradeMetalRequired();
	flProgress = (float)iMetal / (float)iMetalRequired;
	
	m_pUpgradeProgress->SetProgress( flProgress );
	
	// upgrade label only in 1 or 2
	bool bShowUpgradeInfo = iUpgradeLevel < 3;
	m_pUpgradeIcon->SetVisible( bShowUpgradeInfo );
	m_pUpgradeProgress->SetVisible( bShowUpgradeInfo );
}



//============================================================================

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBuildingStatusItem_TeleporterEntrance::CBuildingStatusItem_TeleporterEntrance( Panel *parent ) :
CBuildingStatusItem( parent, "resource/UI/hud_obj_tele_entrance.res", OBJ_TELEPORTER, MODE_TELEPORTER_ENTRANCE )
{
	// Panel and children when we are charging
	m_pChargingPanel = new vgui::EditablePanel( GetRunningPanel(), "ChargingPanel" );
	m_pRechargeTimer = new vgui::ContinuousProgressBar( m_pChargingPanel, "Recharge" );

	// Panel and children when we are fully charged
	m_pFullyChargedPanel = new vgui::EditablePanel( GetRunningPanel(), "FullyChargedPanel" );

	m_iTimesUsed = -1;	// force first update of 0
	m_iTeleporterState = -1;

	m_pUpgradeProgress = new vgui::ContinuousProgressBar( GetRunningPanel(), "Upgrade" );
	m_pUpgradeIcon = new CIconPanel( GetRunningPanel(), "UpgradeIcon" );

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuildingStatusItem_TeleporterEntrance::OnTick( void )
{
	// We only tick while active and with a valid built object
	C_ObjectTeleporter *pTeleporter = static_cast<C_ObjectTeleporter*>(GetRepresentativeObject());

	if ( pTeleporter && IsActive() )
	{
		if ( pTeleporter->GetState() == TELEPORTER_STATE_RECHARGING )
		{
			// Update the recharge
			float flMaxRecharge = pTeleporter->GetCurrentRechargeDuration();
			float flChargeTime = pTeleporter->GetChargeTime();
			m_pRechargeTimer->SetProgress( 1.0 - ( flChargeTime / flMaxRecharge ) );
		}
	}

	BaseClass::OnTick();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuildingStatusItem_TeleporterEntrance::PerformLayout( void )
{
	BaseClass::PerformLayout();

	// We only tick while active and with a valid built object
	C_ObjectTeleporter *pTeleporter = static_cast<C_ObjectTeleporter*>(GetRepresentativeObject());

	if ( !IsActive() || !pTeleporter )
	{
		return;
	}

	bool bRecharging = ( pTeleporter->GetState() == TELEPORTER_STATE_RECHARGING );

	m_pChargingPanel->SetVisible( bRecharging );
	m_pFullyChargedPanel->SetVisible( !bRecharging );

	// How many times has this teleporter been used?
	m_pFullyChargedPanel->SetDialogVariable( "timesused", pTeleporter->GetTimesUsed() );		

	int iUpgradeLevel = pTeleporter->GetUpgradeLevel();

	Assert( iUpgradeLevel >= 1 && iUpgradeLevel <= 3 );

	// upgrade progress
	int iMetal = pTeleporter->GetUpgradeMetal();
	int iMetalRequired = pTeleporter->GetUpgradeMetalRequired();
	float flUpgrade = (float)iMetal / (float)iMetalRequired;
	m_pUpgradeProgress->SetProgress( flUpgrade );

	// upgrade label only in 1 or 2
	m_pUpgradeIcon->SetVisible( iUpgradeLevel < 3 );
	m_pUpgradeProgress->SetVisible( iUpgradeLevel < 3 );
}

//============================================================================

CBuildingStatusItem_TeleporterExit::CBuildingStatusItem_TeleporterExit( Panel *parent ) :
CBuildingStatusItem( parent, "resource/UI/hud_obj_tele_exit.res", OBJ_TELEPORTER, MODE_TELEPORTER_EXIT )
{
	m_pUpgradeProgress = new vgui::ContinuousProgressBar( GetRunningPanel(), "Upgrade" );
	m_pUpgradeIcon = new CIconPanel( GetRunningPanel(), "UpgradeIcon" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuildingStatusItem_TeleporterExit::PerformLayout( void )
{
	BaseClass::PerformLayout();

	// We only tick while active and with a valid built object
	C_ObjectTeleporter *pTeleporter = static_cast<C_ObjectTeleporter*>(GetRepresentativeObject());

	if ( !IsActive() || !pTeleporter )
	{
		return;
	}

	int iUpgradeLevel = pTeleporter->GetUpgradeLevel();

	Assert( iUpgradeLevel >= 1 && iUpgradeLevel <= 3 );

	// upgrade progress
	int iMetal = pTeleporter->GetUpgradeMetal();
	int iMetalRequired = pTeleporter->GetUpgradeMetalRequired();
	float flUpgrade = (float)iMetal / (float)iMetalRequired;
	m_pUpgradeProgress->SetProgress( flUpgrade );

	// upgrade label only in 1 or 2
	m_pUpgradeIcon->SetVisible( iUpgradeLevel < 3 );
	m_pUpgradeProgress->SetVisible( iUpgradeLevel < 3 );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBuildingStatusItem_Sapper::CBuildingStatusItem_Sapper( Panel *parent ) :
CBuildingStatusItem( parent, "resource/UI/hud_obj_sapper.res", OBJ_ATTACHMENT_SAPPER )
{
	// health of target building
	m_pTargetHealthBar = new ContinuousProgressBar( GetRunningPanel(), "TargetHealth" );

	// image of target building 
	m_pTargetIcon = new CIconPanel( GetRunningPanel(), "TargetIcon" );

	// force first think to set the icon
	m_iTargetType = -1;
}

void CBuildingStatusItem_Sapper::PerformLayout( void )
{
	BaseClass::PerformLayout();

	// We only tick while active and with a valid built object
	C_ObjectSapper *pSapper = static_cast<C_ObjectSapper*>(GetRepresentativeObject());

	// only visible 
	SetVisible( pSapper != NULL );

	if ( !IsActive() || !pSapper )
	{
		return;
	}

	C_BaseObject *pTarget = pSapper->GetParentObject();

	if ( pTarget )
	{
		float flHealth = (float)pTarget->GetHealth() / (float)pTarget->GetMaxHealth();

		m_pTargetHealthBar->SetProgress( flHealth );

		int iTargetType = pTarget->GetType();

		if ( m_iTargetType != iTargetType )
		{
			m_pTargetIcon->SetIcon( pTarget->GetHudStatusIcon() );

			m_iTargetType = iTargetType;
		}
	}
	else
	{
		m_pTargetHealthBar->SetProgress( 0.0f );
	}
}


//============================================================================


DECLARE_HUDELEMENT( CHudBuildingStatusContainer_Spy );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudBuildingStatusContainer_Spy::CHudBuildingStatusContainer_Spy( const char *pElementName ) :
BaseClass( "BuildingStatus_Spy" )
{
	AddBuildingPanel( OBJ_ATTACHMENT_SAPPER );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudBuildingStatusContainer_Spy::ShouldDraw( void )
{
	// Don't draw in freezecam
	C_TFPlayer *pPlayer = CTFPlayer::GetLocalTFPlayer();
	if ( !pPlayer || !pPlayer->IsPlayerClass( TF_CLASS_SPY ) || pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM )
	{
		return false;
	}

	if ( pPlayer->GetTeamNumber() <= TEAM_SPECTATOR )
	{
		return false;
	}

	return CHudElement::ShouldDraw();
}

//============================================================================

DECLARE_HUDELEMENT( CHudBuildingStatusContainer_Engineer );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudBuildingStatusContainer_Engineer::CHudBuildingStatusContainer_Engineer( const char *pElementName ) :
BaseClass( "BuildingStatus_Engineer" )
{
	AddBuildingPanel( OBJ_SENTRYGUN, MODE_SENTRYGUN_NORMAL );
	AddBuildingPanel( OBJ_DISPENSER );
	AddBuildingPanel( OBJ_TELEPORTER, MODE_TELEPORTER_ENTRANCE );
	AddBuildingPanel( OBJ_TELEPORTER, MODE_TELEPORTER_EXIT );
	AddBuildingPanel( OBJ_SENTRYGUN, MODE_SENTRYGUN_DISPOSABLE );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 500 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudBuildingStatusContainer_Engineer::ShouldDraw( void )
{
	// Don't draw in freezecam
	C_TFPlayer *pPlayer = CTFPlayer::GetLocalTFPlayer();
	if ( !pPlayer || !pPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) || pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM )
		return false;

	if ( pPlayer->GetTeamNumber() <= TEAM_SPECTATOR )
		return false;

	if ( CTFMinigameLogic::GetMinigameLogic() && CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame() )
		return false;

	if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBuildingStatusContainer_Engineer::OnTick()
{
	BaseClass::OnTick();
	
	if ( !ShouldDraw() )
		return;

	C_TFPlayer *pLocalPlayer = CTFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer )
	{
		bool bDisposableSentriesVisible = false;

		if ( TFGameRules() && TFGameRules()->GameModeUsesUpgrades() )
		{
			int nDisposableSentries = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pLocalPlayer, nDisposableSentries, engy_disposable_sentries );
			if ( nDisposableSentries )
			{
				bDisposableSentriesVisible = true;
			}
		}


		for ( int i = 0 ; i < m_BuildingPanels.Count() ; i++ )
		{
			CBuildingStatusItem *pItem = m_BuildingPanels.Element( i );

			if ( pItem && ( pItem->GetRepresentativeObjectType() == OBJ_SENTRYGUN ) && ( pItem->GetRepresentativeObjectMode() == MODE_SENTRYGUN_DISPOSABLE ) )
			{
				if ( pItem->IsVisible() != bDisposableSentriesVisible )
				{
					pItem->SetVisible( bDisposableSentriesVisible );
				}

				break;
			}

		}
	}
}

//============================================================================

// order the buildings in our m_BuildingsList by their object priority
typedef CBuildingStatusItem *BUILDINGSTATUSITEM_PTR;
static bool BuildingOrderLessFunc( const BUILDINGSTATUSITEM_PTR &left, const BUILDINGSTATUSITEM_PTR &right )
{
	return ( left->GetObjectPriority() <= right->GetObjectPriority() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudBuildingStatusContainer::CHudBuildingStatusContainer( const char *pElementName ) :
CHudElement( pElementName ), BaseClass( NULL, pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS | HIDEHUD_BUILDING_STATUS );

	SetProportional(true);

	ListenForGameEvent( "building_info_changed" );

	m_BuildingPanels.SetLessFunc( BuildingOrderLessFunc );

	m_AlertLevel = BUILDING_HUD_ALERT_NONE;
	m_flNextBeep = 0;
	m_iNumBeepsToBeep = 0;

	// for beeping
	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudBuildingStatusContainer::ShouldDraw( void )
{
	// Don't draw in freezecam
	C_TFPlayer *pPlayer = CTFPlayer::GetLocalTFPlayer();
	if ( pPlayer && pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM )
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBuildingStatusContainer::LevelInit( void )
{
	CHudElement::LevelInit();

	for ( int i = 0; i < m_BuildingPanels.Count(); i++ )
	{
		CBuildingStatusItem *pItem = m_BuildingPanels.Element(i);

		if ( pItem )
		{
			pItem->LevelInit();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create the appropriate info panel for the object
//-----------------------------------------------------------------------------
CBuildingStatusItem *CHudBuildingStatusContainer::CreateItemPanel( int iObjectType, int iObjectMode )
{
	CBuildingStatusItem *pBuildingItem = NULL;
	
	switch( iObjectType )
	{
	case OBJ_SENTRYGUN:
		if ( iObjectMode == 0 )
		{
			pBuildingItem = new CBuildingStatusItem_SentryGun( this );
		}
		else
		{
			pBuildingItem = new CBuildingStatusItem_SentryGun_Disposable( this );
		}
		break;
	case OBJ_DISPENSER:
		pBuildingItem = new CBuildingStatusItem_Dispenser( this );
		break;
	case OBJ_TELEPORTER:
		if ( iObjectMode == 0 )
		{
			pBuildingItem = new CBuildingStatusItem_TeleporterEntrance( this );
		}
		else if ( iObjectMode == 1 )
		{
			pBuildingItem = new CBuildingStatusItem_TeleporterExit( this );
		}
		break;
	case OBJ_ATTACHMENT_SAPPER:
		pBuildingItem = new CBuildingStatusItem_Sapper( this );
		break;
	default:
		pBuildingItem = NULL;
		break;
	}
	
	Assert( pBuildingItem );

	return pBuildingItem;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBuildingStatusContainer::AddBuildingPanel( int iObjectType, int iObjectMode )
{
	CBuildingStatusItem *pBuildingItem = CreateItemPanel( iObjectType, iObjectMode );

	Assert( pBuildingItem );

	pBuildingItem->SetPos( 0, 0 );
	pBuildingItem->InvalidateLayout();

	m_BuildingPanels.Insert( pBuildingItem );

	RepositionObjectPanels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBuildingStatusContainer::UpdateAllBuildings( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	for ( int i = 0; i < m_BuildingPanels.Count(); i++ )
	{
		CBuildingStatusItem *pItem = m_BuildingPanels.Element(i);

		if ( pItem )
		{
			// find the item that represents this building type
			C_BaseObject *pObj = NULL;
			if ( pObj )
			{
				// find the object
				pObj = pLocalPlayer->GetObjectOfType( pItem->GetRepresentativeObjectType(), pItem->GetRepresentativeObjectMode() );

				pItem->SetObject( pObj );

				pItem->InvalidateLayout( true );
				RecalculateAlertState();
			}
		}
	}
}


void CHudBuildingStatusContainer::OnBuildingChanged( int iBuildingType, int iBuildingMode, bool bBuildingIsDead )
{
	bool bFound = false;
	for ( int i = 0; i < m_BuildingPanels.Count() && !bFound; i++ )
	{
		CBuildingStatusItem *pItem = m_BuildingPanels.Element(i);

		if ( pItem && pItem->GetRepresentativeObjectType() == iBuildingType && pItem->GetRepresentativeObjectMode() == iBuildingMode )
		{
			// find the item that represents this building type
			C_BaseObject *pObj = NULL;

			// find the object
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pLocalPlayer )
			{
				pObj = pLocalPlayer->GetObjectOfType( iBuildingType, iBuildingMode );
				pItem->SetObject( pObj );
				pItem->InvalidateLayout( true );
				bFound = true;

				RecalculateAlertState();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBuildingStatusContainer::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	SetPaintBackgroundEnabled( false );

	RepositionObjectPanels();
}

//-----------------------------------------------------------------------------
// Purpose: Contents of object list has changed, reposition the panels
//-----------------------------------------------------------------------------
void CHudBuildingStatusContainer::RepositionObjectPanels( void )
{
	float flXPos = XRES(9);
	float flYPos = YRES(9);

	float flTeleEntranceY = YRES(9);
	float flTeleExitY = YRES(9);

	// Regular Panels
	for ( int i = 0; i < m_BuildingPanels.Count(); i++ )
	{
		CBuildingStatusItem *pItem = m_BuildingPanels.Element(i);

		if ( pItem )
		{
			// set position directly
			pItem->SetPos( flXPos, flYPos );

			// do not increment for speed pad (this is a minor hack)
			// OBJ_TELEPORTER, MODE_TELEPORTER_SPEED
			if ( pItem->GetRepresentativeObjectType() == OBJ_TELEPORTER )
			{
				switch ( pItem->GetRepresentativeObjectMode() )
				{
					case MODE_TELEPORTER_ENTRANCE:
						flTeleEntranceY = flYPos;
						flYPos += pItem->GetTall();
						break;
					case MODE_TELEPORTER_EXIT:
						flTeleExitY = flYPos;
						flYPos += pItem->GetTall();
						break;
				}
			}
			else
			{
				flYPos += pItem->GetTall();	// the fade around the panels gives a gap
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBuildingStatusContainer::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "building_info_changed" ) == 0 )
	{
		int iBuildingType = event->GetInt( "building_type" );
		int iBuildingMode = event->GetInt( "object_mode" );

		if ( iBuildingType >= 0 )
		{
			bool bRemove = ( event->GetInt( "remove" ) > 0 );

			OnBuildingChanged( iBuildingType, iBuildingMode, bRemove );
		}
		else
		{
			UpdateAllBuildings();
		}
	}
	else
	{
		CHudElement::FireGameEvent( event );
	}
}

void CHudBuildingStatusContainer::RecalculateAlertState( void )
{
	BuildingHudAlert_t maxAlertLevel = BUILDING_HUD_ALERT_NONE;

	// find our highest warning level
	for ( int i = 0; i < m_BuildingPanels.Count(); i++ )
	{
		CBuildingStatusItem *pItem = m_BuildingPanels.Element(i);
		C_BaseObject * pObj = pItem->GetRepresentativeObject();

		if ( pObj )
		{
			BuildingHudAlert_t alertLevel = pObj->GetBuildingAlertLevel();
			if ( alertLevel > maxAlertLevel )
			{
				if ( pObj->IsMiniBuilding() && alertLevel != BUILDING_HUD_ALERT_LOW_HEALTH && alertLevel != BUILDING_HUD_ALERT_VERY_LOW_HEALTH && alertLevel != BUILDING_HUD_ALERT_SAPPER )
					continue;
				maxAlertLevel = alertLevel;
			}
		}
	}

	if ( maxAlertLevel != m_AlertLevel )
	{
		if ( maxAlertLevel >= BUILDING_HUD_ALERT_VERY_LOW_AMMO )
		{
			m_flNextBeep = gpGlobals->curtime;	// beep asap
			m_iNumBeepsToBeep = tf_hud_num_building_alert_beeps.GetInt();
		}

		m_AlertLevel = maxAlertLevel;
	}
}

void CHudBuildingStatusContainer::OnTick( void )
{
	if ( m_AlertLevel >= BUILDING_HUD_ALERT_VERY_LOW_AMMO &&
		gpGlobals->curtime >= m_flNextBeep && 
		m_iNumBeepsToBeep > 0 )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( !pLocalPlayer )
			return;

		pLocalPlayer->EmitSound( "Hud.Warning" );

		switch( m_AlertLevel )
		{
		case BUILDING_HUD_ALERT_VERY_LOW_AMMO:
		case BUILDING_HUD_ALERT_VERY_LOW_HEALTH:
			m_flNextBeep = gpGlobals->curtime + 2.0f;
			m_iNumBeepsToBeep--;
			break;

		case BUILDING_HUD_ALERT_SAPPER:
			m_flNextBeep = gpGlobals->curtime + 1.0f;
			// don't decrement beeps, we want them to go on forever
			break;
		}
	}
}