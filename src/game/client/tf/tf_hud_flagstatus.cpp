//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>

#include "c_playerresource.h"
#include "teamplay_round_timer.h"
#include "utlvector.h"
#include "entity_capture_flag.h"
#include "c_tf_player.h"
#include "c_team.h"
#include "c_tf_team.h"
#include "c_team_objectiveresource.h"
#include "tf_hud_objectivestatus.h"
#include "tf_spectatorgui.h"
#include "teamplayroundbased_gamerules.h"
#include "tf_gamerules.h"
#include "tf_hud_freezepanel.h"
#include "c_func_capture_zone.h"
#include "clientmode_shared.h"
#include "tf_hud_mediccallers.h"
#include "view.h"
#include "prediction.h"
#include "tf_logic_robot_destruction.h"

using namespace vgui;

DECLARE_BUILD_FACTORY( CTFArrowPanel );
DECLARE_BUILD_FACTORY( CTFFlagStatus );

DECLARE_HUDELEMENT( CTFFlagCalloutPanel );

ConVar tf_rd_flag_ui_mode( "tf_rd_flag_ui_mode", "3", FCVAR_DEVELOPMENTONLY, "When flags are stolen and not visible: 0 = Show outlines (glows), 1 = Show most valuable enemy flag (icons), 2 = Show all enemy flags (icons), 3 = Show all flags (icons)." );

extern ConVar tf_flag_caps_per_round;

void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFArrowPanel::CTFArrowPanel( Panel *parent, const char *name ) : vgui::Panel( parent, name )
{
	m_RedMaterial.Init( "hud/objectives_flagpanel_compass_red", TEXTURE_GROUP_VGUI ); 
	m_BlueMaterial.Init( "hud/objectives_flagpanel_compass_blue", TEXTURE_GROUP_VGUI ); 
	m_NeutralMaterial.Init( "hud/objectives_flagpanel_compass_grey", TEXTURE_GROUP_VGUI ); 
	m_NeutralRedMaterial.Init( "hud/objectives_flagpanel_compass_grey_with_red", TEXTURE_GROUP_VGUI ); 

	m_RedMaterialNoArrow.Init( "hud/objectives_flagpanel_compass_red_noArrow", TEXTURE_GROUP_VGUI ); 
	m_BlueMaterialNoArrow.Init( "hud/objectives_flagpanel_compass_blue_noArrow", TEXTURE_GROUP_VGUI ); 

	m_pMaterial = m_NeutralMaterial;
	m_bUseRed = false;
	m_flNextColorSwitch = 0.0f;

	ivgui()->AddTickSignal( GetVPanel(), 100 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFArrowPanel::OnTick( void )
{
	if ( !m_hEntity.Get() )
		return;

	C_BaseEntity *pEnt = m_hEntity.Get();
	m_pMaterial = m_NeutralMaterial;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		if ( m_bUseRed )
		{
			m_pMaterial = m_NeutralRedMaterial;
		}
		else
		{
			m_pMaterial = m_NeutralMaterial;
		}

		if ( pEnt && TFGameRules()->GetMannVsMachineAlarmStatus() == true ) 
		{
			CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag *>( pEnt );
			if ( pFlag && pFlag->IsStolen() )
			{
				if ( m_flNextColorSwitch < gpGlobals->curtime )
				{
					m_flNextColorSwitch = gpGlobals->curtime + 0.2f;
					m_bUseRed = !m_bUseRed;
				}
			}
			else
			{
				m_bUseRed = false;
			}
		}
		else
		{
			m_bUseRed = false;
		}
	}
	else
	{
		// figure out what material we need to use
		if ( pEnt->GetTeamNumber() == TF_TEAM_RED )
		{
			m_pMaterial = m_RedMaterial;

			if ( pLocalPlayer && ( pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE ) )
			{
				// is our target a player?
				C_BaseEntity *pTargetEnt = pLocalPlayer->GetObserverTarget();
				if ( pTargetEnt && pTargetEnt->IsPlayer() )
				{
					// does our target have the flag and are they carrying the flag we're currently drawing?
					C_TFPlayer *pTarget = static_cast< C_TFPlayer* >( pTargetEnt );
					if ( pTarget->HasTheFlag() && ( pTarget->GetItem() == pEnt ) )
					{
						m_pMaterial = m_RedMaterialNoArrow;
					}
				}
			}
		}
		else if ( pEnt->GetTeamNumber() == TF_TEAM_BLUE )
		{
			m_pMaterial = m_BlueMaterial;

			if ( pLocalPlayer && ( pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE ) )
			{
				// is our target a player?
				C_BaseEntity *pTargetEnt = pLocalPlayer->GetObserverTarget();
				if ( pTargetEnt && pTargetEnt->IsPlayer() )
				{
					// does our target have the flag and are they carrying the flag we're currently drawing?
					C_TFPlayer *pTarget = static_cast< C_TFPlayer* >( pTargetEnt );
					if ( pTarget->HasTheFlag() && ( pTarget->GetItem() == pEnt ) )
					{
						m_pMaterial = m_BlueMaterialNoArrow;
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFArrowPanel::GetAngleRotation( void )
{
	float flRetVal = 0.0f;

	C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	C_BaseEntity *pEnt = m_hEntity.Get();

	if ( pPlayer && pEnt )
	{
		QAngle vangles;
		Vector eyeOrigin;
		float zNear, zFar, fov;

		pPlayer->CalcView( eyeOrigin, vangles, zNear, zFar, fov );

		Vector vecFlag = pEnt->WorldSpaceCenter() - eyeOrigin;
		vecFlag.z = 0;
		vecFlag.NormalizeInPlace();

		Vector forward, right, up;
		AngleVectors( vangles, &forward, &right, &up );
		forward.z = 0;
		right.z = 0;
		forward.NormalizeInPlace();
		right.NormalizeInPlace();

		float dot = DotProduct( vecFlag, forward );
		float angleBetween = acos( dot );

		dot = DotProduct( vecFlag, right );

		if ( dot < 0.0f )
		{
			angleBetween *= -1;
		}

		flRetVal = RAD2DEG( angleBetween );
	}

	return flRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFArrowPanel::Paint()
{
	int x = 0;
	int y = 0;
	ipanel()->GetAbsPos( GetVPanel(), x, y );
	int nWidth = GetWide();
	int nHeight = GetTall();

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PushMatrix(); 

	VMatrix panelRotation;
	panelRotation.Identity();
	MatrixBuildRotationAboutAxis( panelRotation, Vector( 0, 0, 1 ), GetAngleRotation() );
//	MatrixRotate( panelRotation, Vector( 1, 0, 0 ), 5 );
	panelRotation.SetTranslation( Vector( x + nWidth/2, y + nHeight/2, 0 ) );
	pRenderContext->LoadMatrix( panelRotation );

	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_pMaterial );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.TexCoord2f( 0, 0, 0 );
	meshBuilder.Position3f( -nWidth/2, -nHeight/2, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.TexCoord2f( 0, 1, 0 );
	meshBuilder.Position3f( nWidth/2, -nHeight/2, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.TexCoord2f( 0, 1, 1 );
	meshBuilder.Position3f( nWidth/2, nHeight/2, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.TexCoord2f( 0, 0, 1 );
	meshBuilder.Position3f( -nWidth/2, nHeight/2, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();

	pMesh->Draw();
	pRenderContext->PopMatrix();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFArrowPanel::IsVisible( void )
{
	if( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFlagStatus::CTFFlagStatus( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pArrow = new CTFArrowPanel( this, "Arrow" );
	m_pStatusIcon = new CTFImagePanel( this, "StatusIcon" );
	m_pBriefcase = new CTFImagePanel( this, "Briefcase" );
	m_hEntity = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagStatus::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions = NULL;

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		pConditions = new KeyValues( "conditions" );

		if ( pConditions )
		{
			AddSubKeyNamed( pConditions, "if_mvm" );
		}
	}

	// load control settings...
	LoadControlSettings( "resource/UI/FlagStatus.res", NULL, NULL, pConditions );

	if ( pConditions )
	{
		pConditions->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlagStatus::IsVisible( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer && pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagStatus::UpdateStatus( void )
{
	if ( m_hEntity.Get() )
	{
		CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag *>( m_hEntity.Get() );

		if ( pFlag )
		{
			const char *pszImage = "../hud/objectives_flagpanel_ico_flag_home";
			const char *pszBombImage = "../hud/bomb_dropped";

			if ( pFlag->IsDropped() )
			{
				pszImage = "../hud/objectives_flagpanel_ico_flag_dropped";
			}
			else if ( pFlag->IsStolen() )
			{
				pszImage = "../hud/objectives_flagpanel_ico_flag_moving";
				pszBombImage = "../hud/bomb_carried";
			}

			if ( m_pStatusIcon )
			{
				m_pStatusIcon->SetImage( pszImage );
			}

			if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && m_pBriefcase )
			{
				m_pBriefcase->SetImage( pszBombImage );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudFlagObjectives::CTFHudFlagObjectives( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pCarriedImage = NULL;
	m_pPlayingTo = NULL;
	m_bFlagAnimationPlayed = false;
	m_bCarryingFlag = false;
	m_pSpecCarriedImage = NULL;
	m_pPoisonImage = NULL;
	m_pPoisonTimeLabel = NULL;

	m_pRedFlag = new CTFFlagStatus( this, "RedFlag" );
	m_pBlueFlag = new CTFFlagStatus( this, "BlueFlag" );

	m_bPlayingHybrid_CTF_CP = false;
	m_bPlayingSpecialDeliveryMode = false;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 250 );

	ListenForGameEvent( "flagstatus_update" );

	m_nNumValidFlags = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudFlagObjectives::IsVisible( void )
{
	if( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions = NULL;

	bool bHybrid = TFGameRules() && TFGameRules()->IsPlayingHybrid_CTF_CP();
	bool bMVM = TFGameRules() && TFGameRules()->IsMannVsMachineMode();
	bool bSpecialDeliveryMode = TFGameRules() && TFGameRules()->IsPlayingSpecialDeliveryMode();

	int nNumFlags = 0;

	if ( m_pRedFlag && m_pRedFlag->GetEntity() != NULL )
	{
		nNumFlags++;
	}
	
	if ( m_pBlueFlag && m_pBlueFlag->GetEntity() != NULL )
	{
		nNumFlags++;
	}

	if ( nNumFlags == 2 && m_pRedFlag->GetEntity() == m_pBlueFlag->GetEntity() )
	{
		// They're both pointing at the same flag! There's really only 1
		nNumFlags = 1;
	}

	if ( nNumFlags == 0 )
	{
		pConditions = new KeyValues( "conditions" );
		if ( pConditions )
		{
			AddSubKeyNamed( pConditions, "if_no_flags" );
		}

		if ( bSpecialDeliveryMode )
		{
			AddSubKeyNamed( pConditions, "if_specialdelivery" );
		}
	}
	else
	{
		if ( bHybrid || ( nNumFlags == 1 ) || bMVM || bSpecialDeliveryMode )
		{
			pConditions = new KeyValues( "conditions" );
			if ( pConditions )
			{
				if ( bHybrid )
				{
					AddSubKeyNamed( pConditions, "if_hybrid" );
				}

				if ( nNumFlags == 1 || bSpecialDeliveryMode )
				{
					AddSubKeyNamed( pConditions, "if_hybrid_single" );
				}
				else if ( nNumFlags == 2 )
				{
					AddSubKeyNamed( pConditions, "if_hybrid_double" );
				}

				if ( bMVM )
				{
					AddSubKeyNamed( pConditions, "if_mvm" );
				}

				if ( bSpecialDeliveryMode )
				{
					AddSubKeyNamed( pConditions, "if_specialdelivery" );
				}
			}
		}
	}
	
	// load control settings...
	LoadControlSettings( "resource/UI/HudObjectiveFlagPanel.res", NULL, NULL, pConditions );

	m_pCarriedImage = dynamic_cast<ImagePanel *>( FindChildByName( "CarriedImage" ) );
	m_pPlayingTo = dynamic_cast<CExLabel *>( FindChildByName( "PlayingTo" ) );
	m_pPlayingToBG = FindChildByName( "PlayingToBG" );

	m_pCapturePoint = dynamic_cast<CTFArrowPanel *>( FindChildByName( "CaptureFlag" ) );

	m_pSpecCarriedImage = dynamic_cast<ImagePanel *>( FindChildByName( "SpecCarriedImage" ) );

	m_pPoisonImage = dynamic_cast<ImagePanel *>( FindChildByName( "PoisonIcon" ) );
	m_pPoisonTimeLabel = dynamic_cast<CExLabel *>( FindChildByName( "PoisonTimeLabel" ) );

	// outline is always on, so we need to init the alpha to 0
	vgui::Panel *pOutline = FindChildByName( "OutlineImage" );
	if ( pOutline )
	{
		pOutline->SetAlpha( 0 );
	}

	if ( pConditions )
	{
		pConditions->deleteThis();
	}

	UpdateStatus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::Reset()
{
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutlineHide" );

	UpdateStatus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::SetPlayingToLabelVisible( bool bVisible )
{
	if ( m_pPlayingTo && m_pPlayingToBG )
	{
		if ( m_pPlayingTo->IsVisible() != bVisible )
		{
			m_pPlayingTo->SetVisible( bVisible );
		}

		if ( m_pPlayingToBG->IsVisible() != bVisible )
		{
			m_pPlayingToBG->SetVisible( bVisible );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::OnTick()
{
	int nNumValidFlags = 0;

	// check that our blue panel still points at a valid flag
	if ( m_pBlueFlag && m_pBlueFlag->GetEntity() )
	{
		CCaptureFlag *pFlag = dynamic_cast< CCaptureFlag* >( m_pBlueFlag->GetEntity() );
		if ( !pFlag || pFlag->IsDisabled() )
		{
			m_pBlueFlag->SetEntity( NULL );
		}
	}

	// check that our red panel still points at a valid flag
	if ( m_pRedFlag && m_pRedFlag->GetEntity() )
	{
		CCaptureFlag *pFlag = dynamic_cast< CCaptureFlag* >( m_pRedFlag->GetEntity() );
		if ( !pFlag || pFlag->IsDisabled() )
		{
			m_pRedFlag->SetEntity( NULL );
		}
	}

	// iterate through the flags to set their position in our HUD
	for ( int i = 0; i<ICaptureFlagAutoList::AutoList().Count(); i++ )
	{
		CCaptureFlag *pFlag = static_cast< CCaptureFlag* >( ICaptureFlagAutoList::AutoList()[i] );

		if ( !pFlag->IsDisabled() || pFlag->IsVisibleWhenDisabled() )
		{
			if ( pFlag->GetTeamNumber() == TF_TEAM_RED )
			{
				if ( m_pRedFlag )
				{
					bool bNeedsUpdate = m_pRedFlag->GetEntity() != pFlag;
					m_pRedFlag->SetEntity( pFlag );
					if ( bNeedsUpdate )
					{
						UpdateStatus();
					}
				}
			}
			else if ( pFlag->GetTeamNumber() == TF_TEAM_BLUE )
			{
				if ( m_pBlueFlag )
				{
					bool bNeedsUpdate = m_pBlueFlag->GetEntity() != pFlag;
					m_pBlueFlag->SetEntity( pFlag );
					if ( bNeedsUpdate )
					{
						UpdateStatus();
					}
				}
			}
			else if ( pFlag->GetTeamNumber() == TEAM_UNASSIGNED )
			{
				if ( m_pBlueFlag && !m_pBlueFlag->GetEntity() )
				{
					m_pBlueFlag->SetEntity( pFlag );

					if ( !m_pBlueFlag->IsVisible() )
					{
						m_pBlueFlag->SetVisible( true );
					}

					if ( m_pRedFlag && m_pRedFlag->IsVisible()  )
					{
						m_pRedFlag->SetVisible( false );
					}
				}
				else if ( m_pRedFlag && !m_pRedFlag->GetEntity() )
				{
					// make sure both panels aren't pointing at the same entity
					if ( !m_pBlueFlag || ( pFlag != m_pBlueFlag->GetEntity() ) )
					{
						m_pRedFlag->SetEntity( pFlag );
							
						if ( !m_pRedFlag->IsVisible() )
						{
							m_pRedFlag->SetVisible( true );
						}
					}
				}
			}

			nNumValidFlags++;
		}

		// VGUI callout panels
		if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() && CTFRobotDestructionLogic::GetRobotDestructionLogic()->GetType() == CTFRobotDestructionLogic::TYPE_ROBOT_DESTRUCTION )
		{
			if ( tf_rd_flag_ui_mode.GetInt() && !pFlag->IsDisabled() && !pFlag->IsHome() )
			{
				Vector vecLocation = pFlag->GetAbsOrigin() + Vector( 0.f, 0.f, 18.f );
				CTFFlagCalloutPanel::AddFlagCalloutIfNotFound( pFlag, FLT_MAX, vecLocation );
			}
		}
	}

	if ( m_nNumValidFlags != nNumValidFlags )
	{
		m_nNumValidFlags = nNumValidFlags;
		InvalidateLayout( false, true );
	}

	// are we playing captures for rounds?
	if ( !TFGameRules() || ( !TFGameRules()->IsPlayingHybrid_CTF_CP() && !TFGameRules()->IsPlayingSpecialDeliveryMode() && !TFGameRules()->IsMannVsMachineMode() ) )
	{
		if ( tf_flag_caps_per_round.GetInt() > 0 )
		{
			C_TFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
			if ( pTeam )
			{
				SetDialogVariable( "bluescore", pTeam->GetFlagCaptures() );
			}

			pTeam = GetGlobalTFTeam( TF_TEAM_RED );
			if ( pTeam )
			{
				SetDialogVariable( "redscore", pTeam->GetFlagCaptures() );
			}

			SetPlayingToLabelVisible( true );
			SetDialogVariable( "rounds", tf_flag_caps_per_round.GetInt() );
		}
		else // we're just playing straight score
		{
			C_TFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
			if ( pTeam )
			{
				SetDialogVariable( "bluescore", pTeam->Get_Score() );
			}

			pTeam = GetGlobalTFTeam( TF_TEAM_RED );
			if ( pTeam )
			{
				SetDialogVariable( "redscore", pTeam->Get_Score() );
			}

			SetPlayingToLabelVisible( false );
		}
	}

	// check the local player to see if they're spectating, OBS_MODE_IN_EYE, and the target entity is carrying the flag
	bool bSpecCarriedImage = false;
	CCaptureFlag *pPoisonFlag = NULL;
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		if ( pPlayer->GetObserverMode() == OBS_MODE_IN_EYE )
		{
			// does our target have the flag?
			C_BaseEntity *pEnt = pPlayer->GetObserverTarget();
			if ( pEnt && pEnt->IsPlayer() )
			{
				C_TFPlayer *pTarget = static_cast< C_TFPlayer* >( pEnt );
				if ( pTarget->HasTheFlag() )
				{
					bSpecCarriedImage = true;
					if ( pTarget->GetTeamNumber() == TF_TEAM_RED )
					{
						if ( m_pSpecCarriedImage )
						{
							m_pSpecCarriedImage->SetImage( "../hud/objectives_flagpanel_carried_blue" );
						}
					}
					else
					{
						if ( m_pSpecCarriedImage )
						{
							m_pSpecCarriedImage->SetImage( "../hud/objectives_flagpanel_carried_red" );
						}
					}
				}
			}
		}

		if ( pPlayer->HasTheFlag() && TFGameRules()->IsPowerupMode() )
		{
			CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag *>( pPlayer->GetItem() );
			if ( pFlag )
			{
				pPoisonFlag = pFlag;
			}
		}
	}

	if ( m_pSpecCarriedImage )
	{
		m_pSpecCarriedImage->SetVisible( bSpecCarriedImage );
	}

	if ( m_pPoisonImage )
	{
		m_pPoisonImage->SetVisible( pPoisonFlag && pPoisonFlag->IsPoisonous() );
	}

	if ( m_pPoisonTimeLabel )
	{
		m_pPoisonTimeLabel->SetVisible( pPoisonFlag && pPoisonFlag->GetPoisonTime() > 0.f && !pPoisonFlag->IsPoisonous() );
		if ( m_pPoisonTimeLabel->IsVisible() )
		{
			int nNumSecondsToPoisonous = pPoisonFlag->GetPoisonTime() - gpGlobals->curtime;
			m_pPoisonTimeLabel->SetText( CFmtStr( "%d", nNumSecondsToPoisonous ) );
		}
	}

	if ( TFGameRules() )
	{
		if ( m_bPlayingHybrid_CTF_CP != TFGameRules()->IsPlayingHybrid_CTF_CP() )
		{
			m_bPlayingHybrid_CTF_CP = TFGameRules()->IsPlayingHybrid_CTF_CP();
			InvalidateLayout( false, true );
		}

		if ( m_bPlayingSpecialDeliveryMode != TFGameRules()->IsPlayingSpecialDeliveryMode() )
		{
			m_bPlayingSpecialDeliveryMode = TFGameRules()->IsPlayingSpecialDeliveryMode();
			InvalidateLayout( false, true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::SetCarriedImage( const char *pchIcon )
{
	if ( m_pCarriedImage )
	{
		m_pCarriedImage->SetImage( pchIcon );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::UpdateStatus( C_BasePlayer *pNewOwner /*= NULL*/, C_BaseEntity *pFlagEntity /*= NULL*/ )
{
	C_TFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );

	// are we carrying a flag?
	CCaptureFlag *pPlayerFlag = NULL;
	if ( pLocalPlayer && pLocalPlayer->HasItem() && pLocalPlayer->GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		if ( !pNewOwner || pNewOwner == pLocalPlayer )
		{
			pPlayerFlag = dynamic_cast< CCaptureFlag* >( pLocalPlayer->GetItem() );
		}
	}

	if ( !pPlayerFlag && pLocalPlayer && pLocalPlayer == pNewOwner )
	{
		pPlayerFlag = dynamic_cast< CCaptureFlag* >( pFlagEntity );
	}

	if ( pPlayerFlag )
	{
		m_bCarryingFlag = true;

		// make sure the panels are on, set the initial alpha values, 
		// set the color of the flag we're carrying, and start the animations
		if ( m_pBlueFlag && m_pBlueFlag->IsVisible() )
		{
			m_pBlueFlag->SetVisible( false );
		}

		if ( m_pRedFlag && m_pRedFlag->IsVisible() )
		{
			m_pRedFlag->SetVisible( false );
		}

		if ( m_pCarriedImage && !m_pCarriedImage->IsVisible() )
		{
			int nTeam;
			if ( pPlayerFlag->GetType() == TF_FLAGTYPE_ATTACK_DEFEND || 
				 pPlayerFlag->GetType() == TF_FLAGTYPE_TERRITORY_CONTROL || 
				 pPlayerFlag->GetType() == TF_FLAGTYPE_INVADE || 
				 pPlayerFlag->GetType() == TF_FLAGTYPE_RESOURCE_CONTROL )
			{
				nTeam = ( ( GetLocalPlayerTeam() == TF_TEAM_BLUE ) ? ( TF_TEAM_BLUE ) : ( TF_TEAM_RED ) );
			}
			else
			{
				// normal CTF behavior (carrying the enemy flag)
				nTeam = ( ( GetLocalPlayerTeam() == TF_TEAM_RED ) ? ( TF_TEAM_BLUE ) : ( TF_TEAM_RED ) );
			}


			char szImage[ MAX_PATH ];
			pPlayerFlag->GetHudIcon( nTeam, szImage, sizeof( szImage ) );

			SetCarriedImage( szImage );
			m_pCarriedImage->SetVisible( true );
		}

		if ( !m_bFlagAnimationPlayed )
		{
			m_bFlagAnimationPlayed = true;
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutline" );
		}

		if ( m_pCapturePoint && !m_pCapturePoint->IsVisible() )
		{
			m_pCapturePoint->SetVisible( true );
		}

		if ( pLocalPlayer && m_pCapturePoint )
		{
			// go through all the capture zones and find ours
			for ( int i = 0; i<ICaptureZoneAutoList::AutoList().Count(); i++ )
			{
				C_CaptureZone *pCaptureZone = static_cast< C_CaptureZone* >( ICaptureZoneAutoList::AutoList()[i] );
				if ( !pCaptureZone->IsDormant() )
				{
					if ( pCaptureZone->GetTeamNumber() == pLocalPlayer->GetTeamNumber() && !pCaptureZone->IsDisabled() )
					{
						m_pCapturePoint->SetEntity( pCaptureZone );
					}
				}
			}
		}
	}
	else
	{
		// were we carrying the flag?
		if ( m_bCarryingFlag )
		{
			m_bCarryingFlag = false;
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutline" );
		}

		m_bFlagAnimationPlayed = false;

		if ( m_pCarriedImage && m_pCarriedImage->IsVisible() )
		{
			m_pCarriedImage->SetVisible( false );
		}

		if ( m_pCapturePoint && m_pCapturePoint->IsVisible() )
		{
			m_pCapturePoint->SetVisible( false );
		}

		if ( m_pBlueFlag )
		{
			if ( m_pBlueFlag->GetEntity() != NULL )
			{
				if ( !m_pBlueFlag->IsVisible() )
				{
					m_pBlueFlag->SetVisible( true );
				}
				
				m_pBlueFlag->UpdateStatus();
			}
			else
			{
				if ( m_pBlueFlag->IsVisible() )
				{
					m_pBlueFlag->SetVisible( false );
				}
			}
		}

		if ( m_pRedFlag )
		{
			if ( m_pRedFlag->GetEntity() != NULL )
			{
				if ( !m_pRedFlag->IsVisible() )
				{
					m_pRedFlag->SetVisible( true );
				}

				m_pRedFlag->UpdateStatus();
			}
			else
			{
				if ( m_pRedFlag->IsVisible() )
				{
					m_pRedFlag->SetVisible( false );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();

	if ( FStrEq( eventName, "flagstatus_update" ) )
	{
		int nVictimID = event->GetInt( "userid" );
		C_BasePlayer *pNewOwner = USERID2PLAYER( nVictimID );

		int nFlagEntIndex = event->GetInt( "entindex" );
		C_BaseEntity *pFlagEntity = ClientEntityList().GetEnt( nFlagEntIndex );

		UpdateStatus( pNewOwner, pFlagEntity );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define FLAG_CALLER_WIDE ( XRES( 30 ) )
#define FLAG_CALLER_TALL ( YRES( 30 ) )
#define FLAG_CALLER_ARROW_WIDE ( XRES( 8 ) )
#define FLAG_CALLER_ARROW_TALL ( YRES( 10 ) )
#define FLAG_CALLER_DISPLAY_ENEMY_ONE 1
#define FLAG_CALLER_DISPLAY_ENEMY_ALL 2
#define FLAG_CALLER_DISPLAY_ALL 3

CUtlVector< CTFFlagCalloutPanel* > CTFFlagCalloutPanel::m_FlagCalloutPanels;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFlagCalloutPanel::CTFFlagCalloutPanel( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, pElementName )
{
	m_FlagCalloutPanels.AddToTail( this );

	SetParent( g_pClientMode->GetViewport() );

	RegisterForRenderGroup( "mid" );
	RegisterForRenderGroup( "commentary" );
	SetHiddenBits( HIDEHUD_MISCSTATUS );

	// SetBounds( 0, 0, FLAG_CALLER_WIDE, FLAG_CALLER_TALL );
	vgui::ivgui()->AddTickSignal( GetVPanel() );

	m_pFlagCalloutPanel = new CTFImagePanel( this, "FlagCalloutPanel" );
	m_pFlagValueLabel = new Label( this, "FlagValueLabel", "" );
	m_pFlagStatusIcon = new CTFImagePanel( this,  "StatusIcon" );

	m_flRemoveTime = 1.f;
	m_flFirstDisplayTime = 1.f;
	m_pArrowMaterial = NULL;
	m_iDrawArrow = DRAW_ARROW_UP;
	m_bFlagVisible = false;		// On screen, line-of-sight

	m_flPrevScale = 0.f;
	m_nPanelWideOrig = 0;
	m_nPanelTallOrig = 0;
	m_nLabelWideOrig = 0;
	m_nLabelTallOrig = 0;
	m_nIconWideOrig = 0;
	m_nIconTallOrig = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFlagCalloutPanel::~CTFFlagCalloutPanel( void )
{
	bool bFound = false;
	FOR_EACH_VEC_BACK( m_FlagCalloutPanels, i )
	{
		if ( m_FlagCalloutPanels[i] == this )
		{
			m_FlagCalloutPanels.Remove( i );
			bFound = true;
			break;
		}
	}

	// We should have found the panel and returned earlier
	Assert( bFound );

	if ( m_pArrowMaterial )
	{
		m_pArrowMaterial->DecrementReferenceCount();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlagCalloutPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/FlagCalloutPanel.res" );

	if ( m_pArrowMaterial )
	{
		m_pArrowMaterial->DecrementReferenceCount();
	}
	m_pArrowMaterial = materials->FindMaterial( "HUD/medic_arrow", TEXTURE_GROUP_VGUI );
	m_pArrowMaterial->IncrementReferenceCount();

	if ( !m_pFlagCalloutPanel )
		return;

	if ( !m_pFlagValueLabel )
		return;

	if ( !m_pFlagStatusIcon )
		return;

	m_pFlagCalloutPanel->GetSize( m_nPanelWideOrig, m_nPanelTallOrig );
	m_pFlagValueLabel->GetSize( m_nLabelWideOrig, m_nLabelTallOrig );
	m_pFlagStatusIcon->GetSize( m_nIconWideOrig, m_nIconTallOrig );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagCalloutPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();

	// SetSize( FLAG_CALLER_WIDE, FLAG_CALLER_TALL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagCalloutPanel::GetCalloutPosition( const Vector &vecDelta, float flRadius, float *xpos, float *ypos, float *flRotation )
{
	// Player Data
	QAngle playerAngles = MainViewAngles();

	Vector forward, right, up( 0.f, 0.f, 1.f );
	AngleVectors( playerAngles, &forward, NULL, NULL );
	forward.z = 0.f;
	VectorNormalize( forward );
	CrossProduct( up, forward, right );
	float front = DotProduct( vecDelta, forward );
	float side = DotProduct( vecDelta, right );
	*xpos = flRadius * -side;
	*ypos = flRadius * -front;

	// Get the rotation (yaw)
	*flRotation = atan2( *xpos, *ypos ) + M_PI;
	*flRotation *= 180.f / M_PI;

	float yawRadians = -( *flRotation ) * M_PI / 180.f;
	float ca = cos( yawRadians );
	float sa = sin( yawRadians );

	// Rotate it around the circle
	*xpos = (int)( ( ScreenWidth() / 2 ) + ( flRadius * sa ) );
	*ypos = (int)( ( ScreenHeight() / 2 ) - ( flRadius * ca ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagCalloutPanel::OnTick( void )
{
	int nDisplayMode = tf_rd_flag_ui_mode.GetInt();

	// Panels self-manage their existence and visibility
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer || !m_hFlag || m_hFlag->IsHome() || m_hFlag->IsDisabled() || !nDisplayMode )
	{
		MarkForDeletion();
		return;
	}
	
	bool bShouldDraw = ShouldShowFlagIconToLocalPlayer();

	// Only show the most valuable enemy flag in this mode
	if ( nDisplayMode == FLAG_CALLER_DISPLAY_ENEMY_ONE )
	{
		int nHighestValue = 0;
		CCaptureFlag *pMostValuableFlag = NULL;

		for ( int i = 0; i < ICaptureFlagAutoList::AutoList().Count(); ++i )
		{
			CCaptureFlag *pFlag = static_cast< CCaptureFlag* >( ICaptureFlagAutoList::AutoList()[i] );
			if ( pFlag && pFlag->GetPointValue() > nHighestValue )
			{
				if ( pFlag->IsDisabled() )
					continue;

				if ( pFlag->IsHome() )
					continue;

				if ( pFlag->InSameTeam( pLocalTFPlayer ) )
					continue;

				if ( pFlag->GetPointValue() > nHighestValue )
				{
					nHighestValue = pFlag->GetPointValue();
					pMostValuableFlag = pFlag;
				}
			}
		}

		// If we're not it
		if ( pMostValuableFlag != m_hFlag )
			bShouldDraw = false;
	}

	if ( IsVisible() != bShouldDraw )
	{
		if ( !IsVisible() )
		{
			m_flFirstDisplayTime = gpGlobals->curtime;
			m_flPrevScale = 0.f;
		}

		SetVisible( bShouldDraw );
	}
	if ( IsEnabled() != bShouldDraw )
	{
		SetEnabled( bShouldDraw );
	}

	if ( !bShouldDraw )
		return;

	bool bCarried = ( !m_hFlag->IsDropped() && m_hFlag->GetPrevOwner() );
	if ( bCarried && !prediction->IsFirstTimePredicted() )
		return;

	// Adjust scale based on distance
	Vector vecDistance = m_hFlag->GetAbsOrigin() - pLocalTFPlayer->GetAbsOrigin();
	ScaleAndPositionCallout( RemapValClamped( vecDistance.LengthSqr(), ( 1000.f * 1000.f ), ( 4000.f * 4000.f ), 1.f, 0.6f ) );

	// Reposition the callout based on our target's position
	int iX, iY;
	Vector vecTarget = ( bCarried ) ? m_hFlag->GetPrevOwner()->GetAbsOrigin() : m_hFlag->GetAbsOrigin();
	Vector vecDelta = vecTarget - MainViewOrigin();
	bool bOnScreen = GetVectorInHudSpace( vecTarget, iX, iY );
	int nHalfWidth = GetWide() / 2;

	if ( !bOnScreen || iX < nHalfWidth || iX > ScreenWidth() - nHalfWidth )
	{
		// Only show side panel for a short period of time in this mode
		if ( TFGameRules() && TFGameRules()->IsPlayingRobotDestructionMode() && gpGlobals->curtime > m_flFirstDisplayTime + 5.f )
		{
			m_iDrawArrow = DRAW_ARROW_UP;
			SetAlpha( 0 );
		}
		else
		{
			// It's off the screen. Position the callout.
			VectorNormalize( vecDelta );
			float xpos, ypos;
			float flRotation;
			float flRadius = YRES( 100 );
			GetCalloutPosition( vecDelta, flRadius, &xpos, &ypos, &flRotation );

			iX = xpos;
			iY = ypos;

			Vector vCenter = m_hFlag->WorldSpaceCenter( );
			if ( MainViewRight().Dot( vCenter - MainViewOrigin() ) > 0 )
			{
				m_iDrawArrow = DRAW_ARROW_RIGHT;
			}
			else
			{
				m_iDrawArrow = DRAW_ARROW_LEFT;
			}

			// Move the icon there
			SetPos( iX - nHalfWidth, iY - ( GetTall() / 2 ) );
			SetAlpha( 128 );
		}
	}
	else
	{
		// On screen
		// If our target isn't visible, we draw transparently
		trace_t	tr;
		UTIL_TraceLine( vecTarget, MainViewOrigin(), MASK_VISIBLE, NULL, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction >= 1.f )
		{
			m_bFlagVisible = true;
			SetAlpha( 0 );
			return;
		}
		else
		{
			m_iDrawArrow = DRAW_ARROW_UP;
			SetAlpha( 128 );
			SetPos( iX - nHalfWidth, iY - ( GetTall() / 2 ) );
		}
	}

	m_bFlagVisible = false;

	if ( !m_pFlagCalloutPanel )
		return;

	if ( !m_pFlagValueLabel )
		return;

	if ( !m_pFlagStatusIcon )
		return;

	m_pFlagCalloutPanel->SetImage( m_hFlag->GetTeamNumber() == TF_TEAM_BLUE ? "../hud/obj_briefcase_blue" : "../hud/obj_briefcase_red" );
	m_pFlagValueLabel->SetText( CFmtStr( "%i", m_hFlag->GetPointValue() ) );

	const char *pszImage = "../hud/objectives_flagpanel_ico_flag_home";
	if ( m_hFlag->IsDropped() )
	{
		pszImage = "../hud/objectives_flagpanel_ico_flag_dropped";
	}
	else if ( m_hFlag->IsStolen() )
	{
		pszImage = "../hud/objectives_flagpanel_ico_flag_moving";
	}
	m_pFlagStatusIcon->SetImage( pszImage );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagCalloutPanel::PaintBackground( void )
{
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return;

	if ( !m_hFlag )
	{
		SetAlpha( 0 );
		return;
	}

	BaseClass::PaintBackground();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagCalloutPanel::Paint( void )
{
	// Don't draw side panels if our target is visible. The particle effect will be doing it for us.
	if ( m_bFlagVisible )
		return;

	BaseClass::Paint();

	if ( m_iDrawArrow == DRAW_ARROW_UP )
		return;

	float uA, uB, yA, yB;
	int x, y;
	GetPos( x, y );
	if ( m_iDrawArrow == DRAW_ARROW_LEFT )
	{
		uA = 1.f;
		uB = 0.f;
		yA = 0.f;
		yB = 1.f;
		x -= FLAG_CALLER_ARROW_WIDE;
	}
	else
	{
		uA = 0.f;
		uB = 1.f;
		yA = 0.f;
		yB = 1.f;
		x += m_pFlagCalloutPanel->GetWide();
	}

	int iyindent = ( GetTall() - FLAG_CALLER_ARROW_TALL ) * 0.5f;
	y += iyindent;

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_pArrowMaterial );
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Position3f( x, y, 0.f );
	meshBuilder.TexCoord2f( 0, uA, yA );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x + FLAG_CALLER_ARROW_WIDE, y, 0.f );
	meshBuilder.TexCoord2f( 0, uB, yA );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x + FLAG_CALLER_ARROW_WIDE, y + FLAG_CALLER_ARROW_TALL, 0.f );
	meshBuilder.TexCoord2f( 0, uB, yB );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x, y + FLAG_CALLER_ARROW_TALL, 0.f );
	meshBuilder.TexCoord2f( 0, uA, yB );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagCalloutPanel::SetFlag( CCaptureFlag *pFlag, float flDuration, Vector &vecOffset )
{
	m_hFlag = pFlag;
	m_flRemoveTime = gpGlobals->curtime + flDuration;
	m_vecOffset = vecOffset;
	m_flFirstDisplayTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFlagCalloutPanel *CTFFlagCalloutPanel::AddFlagCalloutIfNotFound( CCaptureFlag *pFlag, float flDuration, Vector &vecLocation )
{
	// How this system works:
	// CTFHudFlagObjectives::OnTick() will attempt to create one panel per-flag that is stolen.
	// CTFFlagCalloutPanel::OnTick() tries to manage whether or not the panel is visible, based on the UI mode.

	// See if we have a panel for this flag already
	FOR_EACH_VEC_BACK( m_FlagCalloutPanels, i )
	{
		if ( m_FlagCalloutPanels[i]->m_hFlag == pFlag )
		{
			return NULL;
		}
	}

	CTFFlagCalloutPanel *pCallout = new CTFFlagCalloutPanel( "FlagCalloutHUD" );
	if ( pCallout )
	{
		pCallout->SetFlag( pFlag, flDuration, vecLocation );
	}
	return pCallout;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlagCalloutPanel::ShouldShowFlagIconToLocalPlayer( void )
{
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return false;

	int nDisplayMode = tf_rd_flag_ui_mode.GetInt();

	// In "show all" mode, don't show flags on the local player's team that are being carried
	if ( m_hFlag->IsStolen() && 
		 m_hFlag->InSameTeam( pLocalTFPlayer ) &&
		 nDisplayMode == FLAG_CALLER_DISPLAY_ALL )
		return false;

	// In all other modes, don't show flags on the local player's team
	if ( m_hFlag->InSameTeam( pLocalTFPlayer ) &&
		nDisplayMode < FLAG_CALLER_DISPLAY_ALL )
		return false;
	
	// Don't show the player running this flag
	if ( m_hFlag->IsStolen() && pLocalTFPlayer == m_hFlag->GetPrevOwner() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagCalloutPanel::ScaleAndPositionCallout( float flScale /*= 1.f*/  )
{
	if ( flScale == m_flPrevScale )
		return;

	SetSize( ( FLAG_CALLER_WIDE * flScale ), ( FLAG_CALLER_TALL * flScale ) );
	
	if ( !m_pFlagCalloutPanel )
		return;

	if ( !m_pFlagValueLabel )
		return;

	if ( !m_pFlagStatusIcon )
		return;

	// Briefcase - top-left
	m_pFlagCalloutPanel->SetSize( ( m_nPanelWideOrig * flScale ), ( m_nPanelTallOrig * flScale ) );
	m_pFlagCalloutPanel->SetPos( 0, 0 );
	
	// Label - centered
	m_pFlagValueLabel->SetSize( ( m_nLabelWideOrig * flScale ), ( m_nLabelTallOrig * flScale ) );
	m_pFlagValueLabel->SetPos( ( m_pFlagCalloutPanel->GetWide() - m_pFlagValueLabel->GetWide() ) * 0.5f, ( m_pFlagCalloutPanel->GetWide() - m_pFlagValueLabel->GetTall() ) * 0.65f );
	
	// Icon - lower-right
	m_pFlagStatusIcon->SetSize( ( m_nIconWideOrig * flScale ), ( m_nIconTallOrig * flScale ) );
	m_pFlagStatusIcon->SetPos( ( m_pFlagCalloutPanel->GetWide() - m_pFlagStatusIcon->GetWide() ) * 1.05f, ( m_pFlagCalloutPanel->GetWide() - m_pFlagStatusIcon->GetTall() ) * 0.85f );

	m_flPrevScale = flScale;
}

