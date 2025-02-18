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
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include "tf_weaponbase.h"
#include "c_tf_projectile_arrow.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudBowChargeMeter : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudBowChargeMeter, EditablePanel );

public:
	CHudBowChargeMeter( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );
	virtual void	Init( void );
	virtual void	FireGameEvent( IGameEvent *event );

private:
	vgui::ContinuousProgressBar *m_pChargeMeter;
};

DECLARE_HUDELEMENT( CHudBowChargeMeter );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudBowChargeMeter::CHudBowChargeMeter( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudBowCharge" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pChargeMeter = new ContinuousProgressBar( this, "ChargeMeter" );

	SetHiddenBits( HIDEHUD_MISCSTATUS | HIDEHUD_PIPES_AND_CHARGE );

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	RegisterForRenderGroup( "inspect_panel" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBowChargeMeter::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudBowCharge.res" );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBowChargeMeter::Init( void )
{
	ListenForGameEvent( "arrow_impact" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBowChargeMeter::FireGameEvent( IGameEvent *event )
{
	if ( !event )
		return;

	const char *pszEventName = event->GetName();

	if ( FStrEq( pszEventName, "arrow_impact" ) )
	{
		int attachedEntity = event->GetInt( "attachedEntity" );
		C_BaseFlex *pFlex = dynamic_cast<C_BaseFlex*>( ClientEntityList().GetEnt( attachedEntity ) );
		if ( !pFlex )
			return;

		// Create a client side arrow and have it attach itself.
		C_TFProjectile_Arrow *pArrow = new C_TFProjectile_Arrow;
		if ( !pArrow )
			return;

		int boneIndexAttached = event->GetInt( "boneIndexAttached" );
		Vector bonePosition( 
			event->GetFloat( "bonePositionX"),
			event->GetFloat( "bonePositionY"),
			event->GetFloat( "bonePositionZ") );
		QAngle boneAngles( 
			event->GetFloat( "boneAnglesX"),
			event->GetFloat( "boneAnglesY"),
			event->GetFloat( "boneAnglesZ") );

		const char* pszModelName = NULL;
		int type = event->GetInt( "projectileType" );
		float flScale = 1.0f;

		switch ( type )
		{
		case TF_PROJECTILE_STICKY_BALL:
			pszModelName = g_pszArrowModels[MODEL_SNOWBALL];
			break;
		case TF_PROJECTILE_ARROW:
			pszModelName = g_pszArrowModels[MODEL_ARROW_REGULAR];
			break;
		case TF_PROJECTILE_BUILDING_REPAIR_BOLT:
			pszModelName = g_pszArrowModels[MODEL_ARROW_BUILDING_REPAIR];
			break;
		case TF_PROJECTILE_FESTIVE_ARROW:
			pszModelName = g_pszArrowModels[MODEL_FESTIVE_ARROW_REGULAR];
			break;
		case TF_PROJECTILE_HEALING_BOLT:
			{
				pszModelName = g_pszArrowModels[MODEL_SYRINGE];
				// pull the syringe back slightly
				Vector vForward;
				AngleVectors( boneAngles, &vForward );
				bonePosition = bonePosition - (vForward * 6.0f);
				flScale = 1.6f;
			}
			break;
		case TF_PROJECTILE_FESTIVE_HEALING_BOLT:
			{
				pszModelName = g_pszArrowModels[MODEL_FESTIVE_HEALING_BOLT];
				// pull the syringe back slightly
				Vector vForward;
				AngleVectors( boneAngles, &vForward );
				bonePosition = bonePosition - ( vForward * 1.0f );
				flScale = 1.4f;
			}
			break;
		case TF_PROJECTILE_BREAD_MONSTER:
		case TF_PROJECTILE_BREADMONSTER_JARATE:
		case TF_PROJECTILE_BREADMONSTER_MADMILK:
			{
				pszModelName = g_pszArrowModels[MODEL_BREAD_MONSTER];
				// pull the syringe back slightly
				Vector vForward;
				AngleVectors( boneAngles, &vForward );
				bonePosition = bonePosition - ( vForward * 1.0f );
				flScale = 2.5f;
				pArrow->SetLifeTime( 10.0f );
				if ( event->GetBool( "isCrit" ) )
				{
					flScale = RandomFloat( 3.0f, 5.0f );
				}
				break;
			}
		default:
			Warning( " Unsupported Projectile type on event arrow_impact - %d", type );
			return;
		}
		
		pArrow->InitializeAsClientEntity( pszModelName, RENDER_GROUP_OPAQUE_ENTITY );
		pArrow->SetModelScale( flScale );

		CTFPlayer *pPlayer = ToTFPlayer( ClientEntityList().GetEnt( event->GetInt( "shooter" ) ) );
		if ( pPlayer )
		{
			pArrow->m_nSkin = ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE ) ? 1 : 0;
		}

		pArrow->AttachEntityToBone( pFlex, boneIndexAttached, bonePosition, boneAngles );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudBowChargeMeter::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer || !pPlayer->IsPlayerClass( TF_CLASS_SNIPER ) || !pPlayer->IsAlive() )
	{
		return false;
	}

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();

	if ( !pWpn )
	{
		return false;
	}

	int iWeaponID = pWpn->GetWeaponID();

	if ( iWeaponID != TF_WEAPON_COMPOUND_BOW )
	{
		return false;
	}

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBowChargeMeter::OnTick( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return;

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
	ITFChargeUpWeapon *pChargeupWeapon = dynamic_cast< ITFChargeUpWeapon *>( pWpn );

	if ( !pWpn || !pChargeupWeapon )
		return;

	if ( m_pChargeMeter )
	{
		float flChargeMaxTime = pChargeupWeapon->GetChargeMaxTime();

		if ( flChargeMaxTime != 0 )
		{
			float flChargeBeginTime = pChargeupWeapon->GetChargeBeginTime();

			if ( flChargeBeginTime > 0 )
			{
				float flTimeCharged = MAX( 0, gpGlobals->curtime - flChargeBeginTime );
				flTimeCharged = MIN( flTimeCharged, 1.f );
				float flPercentCharged = MIN( 1.0, flTimeCharged / flChargeMaxTime );

				m_pChargeMeter->SetProgress( flPercentCharged );
			}
			else
			{
				m_pChargeMeter->SetProgress( 0.0f );
			}
		}
	}
}