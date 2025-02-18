//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Engineer's Laser Pointer
//
//=============================================================================//
#include "cbase.h" 
#include "tf_fx_shared.h"
#include "tf_weapon_laser_pointer.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "view.h"
#include "beamdraw.h"
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>
#include "vgui_controls/Controls.h"
#include "hud_crosshair.h"
#include "functionproxy.h"
#include "materialsystem/imaterialvar.h"
#include "toolframework_client.h"
#include "input.h"
#include "sourcevr/isourcevirtualreality.h"

// forward declarations
void ToolFramework_RecordMaterialParams( IMaterial *pMaterial );
#else
#include "tf_gamerules.h"
#include "tf_obj_sentrygun.h"
#endif

#define TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC	50.0
#define TF_WEAPON_SNIPERRIFLE_UNCHARGE_PER_SEC	75.0
#define	TF_WEAPON_SNIPERRIFLE_DAMAGE_MIN		50
#define TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX		150
#define TF_WEAPON_SNIPERRIFLE_RELOAD_TIME		1.5f
#define TF_WEAPON_SNIPERRIFLE_ZOOM_TIME			0.3f

#define TF_WEAPON_SNIPERRIFLE_NO_CRIT_AFTER_ZOOM_TIME	0.2f

#define LASER_DOT_SPRITE_RED		"effects/sniperdot_red.vmt"
#define LASER_DOT_SPRITE_BLUE		"effects/sniperdot_blue.vmt"

//=============================================================================
//
// Weapon Laser Pointer tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFLaserPointer, DT_TFLaserPointer )

BEGIN_NETWORK_TABLE_NOBASE( CTFLaserPointer, DT_LaserPointerLocalData )
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE( CTFLaserPointer, DT_TFLaserPointer )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFLaserPointer )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_laser_pointer, CTFLaserPointer );
PRECACHE_WEAPON_REGISTER( tf_weapon_laser_pointer );

//=============================================================================
//
// Weapon Laser Pointer functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFLaserPointer::CTFLaserPointer()
{
#ifdef GAME_DLL
	m_hLaserDot = NULL;
#endif

	m_flNextAttack = 0.f;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFLaserPointer::~CTFLaserPointer()
{
// Server specific.
#ifdef GAME_DLL
	DestroyLaserDot();
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFLaserPointer::Precache()
{
	BaseClass::Precache();

	PrecacheModel( LASER_DOT_SPRITE_RED );
	PrecacheModel( LASER_DOT_SPRITE_BLUE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFLaserPointer::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
	#ifdef GAME_DLL
		SetContextThink( &CTFLaserPointer::CreateLaserDot, gpGlobals->curtime + 0.5f, "CREATE_LASER_DOT" );
	#endif

		m_bDeployed = true;

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFLaserPointer::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( BaseClass::Holster( pSwitchingTo ) )
	{
	#ifdef GAME_DLL
		DestroyLaserDot();
	#endif

		m_bDeployed = false;

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFLaserPointer::ItemPostFrame( void )
{
	if ( !m_bDeployed )
		return;

	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

#ifdef GAME_DLL
	if ( m_hLaserDot )
	{
		UpdateLaserDot();
	}
#endif

	BaseClass::ItemPostFrame();

	// Return to idle.
	if ( GetIdealActivity() == ACT_ITEM1_VM_RELOAD && !( pPlayer->m_nButtons & IN_ATTACK ) )
	{
		SendWeaponAnim( ACT_ITEM1_RELOAD_FINISH );
		if ( gpGlobals->curtime - m_flStartedFiring > 5.f )
		{
			m_bDoHandIdle = true;
			m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();
		}
	}
}

void CTFLaserPointer::WeaponIdle( void )
{
#ifdef GAME_DLL
	if ( m_bDoHandIdle && !WeaponShouldBeLowered() && HasWeaponIdleTimeElapsed() )
	{
		m_bDoHandIdle = false;
		SendWeaponAnim( ACT_ITEM1_VM_IDLE_2 );
		m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();
		return;
	}
#endif

	BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFLaserPointer::PrimaryAttack( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;
#ifdef GAME_DLL
	CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun*>( pPlayer->GetObjectOfType( OBJ_SENTRYGUN ) );
	if ( !pSentry )
		return;

	pSentry->FireNextFrame();

	if ( GetIdealActivity() != ACT_ITEM1_VM_RELOAD )
	{
		m_flStartedFiring = gpGlobals->curtime;
	}

	SendWeaponAnim( ACT_ITEM1_VM_RELOAD );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFLaserPointer::SecondaryAttack( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;
#ifdef GAME_DLL
	CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun*>( pPlayer->GetObjectOfType( OBJ_SENTRYGUN ) );
	if ( !pSentry )
		return;

	if ( pSentry->GetUpgradeLevel() == 3 )
	{
		pSentry->FireRocketNextFrame();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFLaserPointer::CreateLaserDot( void )
{
#ifdef GAME_DLL
	if ( !m_bDeployed )
		return;

	if ( m_hLaserDot )
		return;

	CBaseCombatCharacter *pPlayer = GetOwner();
	if ( !pPlayer )
		return;

	m_hLaserDot = CLaserDot::Create( GetAbsOrigin(), pPlayer, true );
	m_hLaserDot->ChangeTeam( pPlayer->GetTeamNumber() );

	UpdateLaserDot();
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFLaserPointer::DestroyLaserDot( void )
{
#ifdef GAME_DLL
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( pPlayer )
	{
		CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun*>( pPlayer->GetObjectOfType( OBJ_SENTRYGUN ) );
		if ( pSentry )
		{
			pSentry->ClearTarget();
		}
	}

	if ( m_hLaserDot )
	{
		UTIL_Remove( m_hLaserDot );
		m_hLaserDot = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFLaserPointer::UpdateLaserDot( void )
{
#ifdef GAME_DLL
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	Vector vecMuzzlePos = pPlayer->Weapon_ShootPosition();
	Vector forward;
	pPlayer->EyeVectors( &forward );
	Vector vecEndPos = vecMuzzlePos + ( forward * MAX_TRACE_LENGTH );

	trace_t	trace;
	CTraceFilterIgnoreTeammatesAndTeamObjects filter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
	UTIL_TraceLine( vecMuzzlePos, vecEndPos, MASK_SOLID, &filter, &trace );

	if ( m_hLaserDot )
	{
		CBaseEntity *pEntity = NULL;
		if ( trace.DidHitNonWorldEntity() )
		{
			pEntity = trace.m_pEnt;
			if ( !pEntity || !pEntity->m_takedamage )
			{
				pEntity = NULL;
			}
			else if ( pEntity->IsPlayer() )
			{
				// We lased a player target. We want to auto-aim on this guy for a short period of time.
				CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun*>( pPlayer->GetObjectOfType( OBJ_SENTRYGUN ) );
				if ( pSentry )
				{
					pSentry->SetAutoAimTarget( ToTFPlayer( pEntity ) );
				}
			}
		}

		m_hLaserDot->Update( pEntity, trace.endpos, trace.plane.normal );
	}
#endif
}

//=============================================================================
//
// Laser Dot functions.
//

IMPLEMENT_NETWORKCLASS_ALIASED( LaserDot, DT_LaserDot )

BEGIN_NETWORK_TABLE( CLaserDot, DT_LaserDot )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( env_laserdot, CLaserDot );

BEGIN_DATADESC( CLaserDot )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CLaserDot::CLaserDot( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CLaserDot::~CLaserDot( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CLaserDot* CLaserDot::Create( const Vector &origin, CBaseEntity *pOwner, bool bVisibleDot )
{
#ifdef CLIENT_DLL
	return NULL;
#else
	CLaserDot *pDot = static_cast<CLaserDot*>( CBaseEntity::Create( "env_laserdot", origin, QAngle( 0.0f, 0.0f, 0.0f ) ) );
	if ( !pDot )
		return NULL;

	pDot->SetMoveType( MOVETYPE_NONE );
	pDot->AddSolidFlags( FSOLID_NOT_SOLID );
	pDot->AddEffects( EF_NOSHADOW );
	UTIL_SetSize( pDot, -Vector( 4.0f, 4.0f, 4.0f ), Vector( 4.0f, 4.0f, 4.0f ) );

	pDot->SetOwnerEntity( pOwner );

	pDot->AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

	return pDot;
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CLaserDot::DrawModel( int flags )
{
	// Get the owning player.
	C_TFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( !pPlayer )
		return -1;

	// Get the sprite rendering position.
	Vector vecEndPos;

	float flSize = 6.0;

	if ( !pPlayer->IsDormant() )
	{
		Vector vecAttachment, vecDir;

		float flDist = MAX_TRACE_LENGTH;

		// Always draw the dot in front of our faces when in first-person.
		if ( pPlayer->IsLocalPlayer() )
		{
			// Take our view position and orientation
			vecAttachment = CurrentViewOrigin();
			vecDir = CurrentViewForward();

			if ( UseVR() )
			{
				// It will basically be a copy of CSniperDot::GetRenderingPositions in tf_weapon_sniperrife.cpp
				Assert ( !"Ask Joe Ludwig to fix CLaserDot::DrawModel() for VR." );
			}

			// Clamp the forward distance for the sniper's firstperson
			flDist = 384;

			flSize = 2.0;
		}
		else
		{
			// Take the owning player eye position and direction.
			vecAttachment = pPlayer->EyePosition();
			QAngle angles = pPlayer->EyeAngles();
			AngleVectors( angles, &vecDir );
		}

		trace_t	trace;
		CTraceFilterIgnoreTeammatesAndTeamObjects filter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
		UTIL_TraceLine( vecAttachment, vecAttachment + ( vecDir * flDist ), MASK_SOLID, &filter, &trace );

		// Backup off the hit plane, towards the source
		vecEndPos = trace.endpos + vecDir * -4;
	}
	else
	{
		// Just use our position if we can't predict it otherwise.
		vecEndPos = GetAbsOrigin();
	}

	// Draw our laser dot in space.
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_hSpriteMaterial, this );

	float flLifeTime = gpGlobals->curtime - m_flChargeStartTime;
	float flStrength = RemapValClamped( flLifeTime, 0.0, TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX / TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC, 0.1, 1.0 );

	color32 innercolor = { 255, 255, 255, 255 };
	color32 outercolor = { 255, 255, 255, 128 };

	DrawSprite( vecEndPos, flSize, flSize, outercolor );
	DrawSprite( vecEndPos, flSize * flStrength, flSize * flStrength, innercolor );

	// Successful.
	return 1;
}
#endif
