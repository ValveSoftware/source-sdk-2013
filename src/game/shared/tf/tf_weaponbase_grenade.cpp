//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Base Grenade.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase.h"
#include "tf_weaponbase_grenade.h"
#include "tf_gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "in_buttons.h"	
#include "tf_weaponbase_grenadeproj.h"
#include "eventlist.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "items.h"
#endif

#define GRENADE_TIMER	1.5f			// seconds

//=============================================================================
//
// TF Grenade tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponBaseGrenade, DT_TFWeaponBaseGrenade )

BEGIN_NETWORK_TABLE( CTFWeaponBaseGrenade, DT_TFWeaponBaseGrenade )
// Client specific.
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bPrimed ) ),
	RecvPropFloat( RECVINFO( m_flThrowTime ) ),
	RecvPropBool( RECVINFO( m_bThrow ) ),
// Server specific.
#else
	SendPropBool( SENDINFO( m_bPrimed ) ),
	SendPropTime( SENDINFO( m_flThrowTime ) ),
	SendPropBool( SENDINFO( m_bThrow ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponBaseGrenade )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_bPrimed, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flThrowTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bThrow, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weaponbase_grenade, CTFWeaponBaseGrenade );

//=============================================================================
//
// TF Grenade functions.
//

CTFWeaponBaseGrenade::CTFWeaponBaseGrenade()
{
}

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenade::Spawn( void )
{
	BaseClass::Spawn();

	SetViewModelIndex( 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenade::Precache()
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBaseGrenade::IsPrimed( void )
{
	return m_bPrimed;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBaseGrenade::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
		Prime();
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenade::Prime() 
{
	CTFWeaponInfo weaponInfo = GetTFWpnData();
	m_flThrowTime = gpGlobals->curtime + weaponInfo.m_flPrimerTime;
	m_bPrimed = true;

#ifndef CLIENT_DLL
	if ( GetWeaponID() != TF_WEAPON_GRENADE_SMOKE_BOMB )
	{
		// Get the player owning the weapon.
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
		if ( !pPlayer )
			return;

		pPlayer->RemoveInvisibility();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenade::Throw() 
{
	if ( !m_bPrimed )
		return;

	m_bPrimed = false;
	m_bThrow = false;

	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

#ifdef GAME_DLL
	// Calculate the time remaining.
	float flTime = m_flThrowTime - gpGlobals->curtime;
	bool bExplodingInHand = ( flTime <= 0.0f );

	// Players who are dying may not have their death state set, so check that too
	bool bExplodingOnDeath = ( !pPlayer->IsAlive() || pPlayer->StateGet() == TF_STATE_DYING );

	Vector vecSrc, vecThrow;
	vecSrc = pPlayer->Weapon_ShootPosition();

	if ( bExplodingInHand || bExplodingOnDeath )
	{
		vecThrow = vec3_origin;
	}
	else
	{
		// Determine the throw angle and velocity.
		QAngle angThrow = pPlayer->LocalEyeAngles();
		if ( angThrow.x < 90.0f )
		{
			angThrow.x = -10.0f + angThrow.x * ( ( 90.0f + 10.0f ) / 90.0f );
		}
		else
		{
			angThrow.x = 360.0f - angThrow.x;
			angThrow.x = -10.0f + angThrow.x * -( ( 90.0f - 10.0f ) / 90.0f );
		}

		// Adjust for the lowering of the spawn point
		angThrow.x -= 10;

		float flVelocity = ( 90.0f - angThrow.x ) * 8.0f;
		if ( flVelocity > 950.0f )
		{
			flVelocity = 950.0f;
		}

		Vector vForward, vRight, vUp;
		AngleVectors( angThrow, &vForward, &vRight, &vUp );

		// Throw from the player's left hand position.
		vecSrc += vForward * 16.0f + vRight * -8.0f + vUp * -20.0f;

		vecThrow = vForward * flVelocity;
	}

#if 0
	// Debug!!!
	char str[256];
	Q_snprintf( str, sizeof( str ),"GrenadeTime = %f\n", flTime );
	NDebugOverlay::ScreenText( 0.5f, 0.38f, str, 255, 255, 255, 255, 2.0f );
#endif

	QAngle vecAngles = RandomAngle( 0, 360 );

	// Create the projectile and send in the time remaining.
	if ( !bExplodingInHand )
	{
		EmitGrenade( vecSrc, vecAngles, vecThrow, AngularImpulse( 600, random->RandomInt( -1200, 1200 ), 0 ), pPlayer, flTime );
	}
	else
	{
		// We're holding onto an exploding grenade
		CTFWeaponBaseGrenadeProj *pGrenade = EmitGrenade( vecSrc, vecAngles, vecThrow, AngularImpulse( 600, random->RandomInt( -1200, 1200 ), 0 ), pPlayer, 0.0 );
		if ( pGrenade )
		{
			pGrenade->Detonate();
		}
	}

	// The grenade is about to be destroyed, so it won't be able to holster.
	// Handle the viewmodel hiding for it.
	if ( bExplodingInHand || bExplodingOnDeath )
	{
		SendWeaponAnim( ACT_VM_IDLE );
		CBaseViewModel *vm = pPlayer->GetViewModel( 1 );
		if ( vm )
		{
			vm->AddEffects( EF_NODRAW );
		}
	}
#endif

	// Reset the throw time
	m_flThrowTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBaseGrenade::ShouldDetonate( void )
{
	return ( m_flThrowTime != 0.0f ) && ( m_flThrowTime < gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenade::ItemPostFrame()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( m_bPrimed )
	{
		// Is our timer up? If so, blow up immediately
		if ( ShouldDetonate() )
		{
			Throw();
			return;
		}

		if ( !m_bThrow && !( pPlayer->m_nButtons & IN_GRENADE1 || pPlayer->m_nButtons & IN_GRENADE2 ) )
		{
			// Start throwing
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_GRENADE );
			m_bThrow = true;
		}		
	}

	if ( m_bThrow )
	{
		if ( GetActivity() != ACT_VM_PRIMARYATTACK )
		{
			// Start the throw animation
			if ( !SendWeaponAnim( ACT_VM_PRIMARYATTACK ) )
			{
				Throw();
			}
		}
		else if ( HasWeaponIdleTimeElapsed() )
		{
			// The Throw call here exists solely to catch the lone case of thirdperson where the 
			// viewmodel isn't being drawn, and hence the anim event doesn't trigger and force a throw.
			// In all other cases, it'll do nothing because the grenade has already been thrown.
			Throw();
		}
		return;
	}

	if ( !m_bPrimed && !m_bThrow )
	{
		// Once we've finished being holstered, we'll be hidden. When that happens,
		// tell our player that we're all done with the grenade throw.
		if ( IsEffectActive(EF_NODRAW) )
		{
			pPlayer->FinishThrowGrenade();
			return;
		}

		// We've been thrown. Go away.
		if ( HasWeaponIdleTimeElapsed() )
		{
			Holster();
		}
	}

	// Go straight to idle anim when deploy is done
	if ( m_flTimeWeaponIdle <= gpGlobals->curtime )
	{
		SendWeaponAnim( ACT_VM_IDLE );
	}
}

bool CTFWeaponBaseGrenade::ShouldLowerMainWeapon( void )
{
	return GetTFWpnData().m_bLowerWeapon;
}

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBaseGrenade::ShouldDraw( void )
{
	if ( !BaseClass::ShouldDraw() )
	{
		// Grenades need to be visible whenever they're being primed & thrown
		if ( !m_bPrimed )
			return false;

		// Don't draw primed grenades for local player in first person players
		if ( !(ToPlayer(GetOwner())->ShouldDrawThisPlayer()) )
			return false;
	}

	return true;
}

//=============================================================================
//
// Server specific functions.
//
#else

BEGIN_DATADESC( CTFWeaponBaseGrenade )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenade::HandleAnimEvent( animevent_t *pEvent )
{
	if ( (pEvent->type & AE_TYPE_NEWEVENTSYSTEM) )
	{
		if ( pEvent->event == AE_WPN_PRIMARYATTACK )
		{
			Throw();
			return;
		}
	}

	BaseClass::HandleAnimEvent( pEvent );	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj *CTFWeaponBaseGrenade::EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iFlags )
{
	Assert( 0 && "CBaseCSGrenade::EmitGrenade should not be called. Make sure to implement this in your subclass!\n" );
	return NULL;
}

#endif

