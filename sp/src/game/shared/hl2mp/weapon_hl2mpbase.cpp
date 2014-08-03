//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "in_buttons.h"
#include "takedamageinfo.h"
#include "ammodef.h"
#include "hl2mp_gamerules.h"


#ifdef CLIENT_DLL
extern IVModelInfoClient* modelinfo;
#else
extern IVModelInfo* modelinfo;
#endif


#if defined( CLIENT_DLL )

	#include "vgui/ISurface.h"
	#include "vgui_controls/Controls.h"
	#include "c_hl2mp_player.h"
	#include "hud_crosshair.h"

#else

	#include "hl2mp_player.h"
	#include "vphysics/constraints.h"

#endif

#include "weapon_hl2mpbase.h"


// ----------------------------------------------------------------------------- //
// Global functions.
// ----------------------------------------------------------------------------- //

bool IsAmmoType( int iAmmoType, const char *pAmmoName )
{
	return GetAmmoDef()->Index( pAmmoName ) == iAmmoType;
}

static const char * s_WeaponAliasInfo[] = 
{
	"none",	//	WEAPON_NONE = 0,

	//Melee
	"shotgun",	//WEAPON_AMERKNIFE,
	
	NULL,		// end of list marker
};


// ----------------------------------------------------------------------------- //
// CWeaponHL2MPBase tables.
// ----------------------------------------------------------------------------- //

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponHL2MPBase, DT_WeaponHL2MPBase )

BEGIN_NETWORK_TABLE( CWeaponHL2MPBase, DT_WeaponHL2MPBase )

#ifdef CLIENT_DLL
  
#else
	// world weapon models have no aminations
  //	SendPropExclude( "DT_AnimTimeMustBeFirst", "m_flAnimTime" ),
//	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
//	SendPropExclude( "DT_LocalActiveWeaponData", "m_flTimeWeaponIdle" ),
#endif
	
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponHL2MPBase ) 
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_hl2mp_base, CWeaponHL2MPBase );


#ifdef GAME_DLL

	BEGIN_DATADESC( CWeaponHL2MPBase )

	END_DATADESC()

#endif

// ----------------------------------------------------------------------------- //
// CWeaponHL2MPBase implementation. 
// ----------------------------------------------------------------------------- //
CWeaponHL2MPBase::CWeaponHL2MPBase()
{
	SetPredictionEligible( true );
	AddSolidFlags( FSOLID_TRIGGER ); // Nothing collides with these but it gets touches.

	m_flNextResetCheckTime = 0.0f;
}


bool CWeaponHL2MPBase::IsPredicted() const
{ 
	return true;
}

void CWeaponHL2MPBase::WeaponSound( WeaponSound_t sound_type, float soundtime /* = 0.0f */ )
{
#ifdef CLIENT_DLL

		// If we have some sounds from the weapon classname.txt file, play a random one of them
		const char *shootsound = GetWpnData().aShootSounds[ sound_type ]; 
		if ( !shootsound || !shootsound[0] )
			return;

		CBroadcastRecipientFilter filter; // this is client side only
		if ( !te->CanPredict() )
			return;
				
		CBaseEntity::EmitSound( filter, GetPlayerOwner()->entindex(), shootsound, &GetPlayerOwner()->GetAbsOrigin() ); 
#else
		BaseClass::WeaponSound( sound_type, soundtime );
#endif
}


CBasePlayer* CWeaponHL2MPBase::GetPlayerOwner() const
{
	return dynamic_cast< CBasePlayer* >( GetOwner() );
}

CHL2MP_Player* CWeaponHL2MPBase::GetHL2MPPlayerOwner() const
{
	return dynamic_cast< CHL2MP_Player* >( GetOwner() );
}

#ifdef CLIENT_DLL
	
void CWeaponHL2MPBase::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( GetPredictable() && !ShouldPredict() )
		ShutdownPredictable();
}


bool CWeaponHL2MPBase::ShouldPredict()
{
	if ( GetOwner() && GetOwner() == C_BasePlayer::GetLocalPlayer() )
		return true;

	return BaseClass::ShouldPredict();
}


#else
	
void CWeaponHL2MPBase::Spawn()
{
	BaseClass::Spawn();

	// Set this here to allow players to shoot dropped weapons
	SetCollisionGroup( COLLISION_GROUP_WEAPON );
}

void CWeaponHL2MPBase::Materialize( void )
{
	if ( IsEffectActive( EF_NODRAW ) )
	{
		// changing from invisible state to visible.
		EmitSound( "AlyxEmp.Charge" );
		
		RemoveEffects( EF_NODRAW );
		DoMuzzleFlash();
	}

	if ( HasSpawnFlags( SF_NORESPAWN ) == false )
	{
		VPhysicsInitNormal( SOLID_BBOX, GetSolidFlags() | FSOLID_TRIGGER, false );
		SetMoveType( MOVETYPE_VPHYSICS );

		HL2MPRules()->AddLevelDesignerPlacedObject( this );
	}

	if ( HasSpawnFlags( SF_NORESPAWN ) == false )
	{
		if ( GetOriginalSpawnOrigin() == vec3_origin )
		{
			m_vOriginalSpawnOrigin = GetAbsOrigin();
			m_vOriginalSpawnAngles = GetAbsAngles();
		}
	}

	SetPickupTouch();

	SetThink (NULL);
}

int CWeaponHL2MPBase::ObjectCaps()
{
	return BaseClass::ObjectCaps() & ~FCAP_IMPULSE_USE;
}

#endif

void CWeaponHL2MPBase::FallInit( void )
{
#ifndef CLIENT_DLL
	SetModel( GetWorldModel() );
	VPhysicsDestroyObject();

	if ( HasSpawnFlags( SF_NORESPAWN ) == false )
	{
		SetMoveType( MOVETYPE_NONE );
		SetSolid( SOLID_BBOX );
		AddSolidFlags( FSOLID_TRIGGER );

		UTIL_DropToFloor( this, MASK_SOLID );
	}
	else
	{
		if ( !VPhysicsInitNormal( SOLID_BBOX, GetSolidFlags() | FSOLID_TRIGGER, false ) )
		{
			SetMoveType( MOVETYPE_NONE );
			SetSolid( SOLID_BBOX );
			AddSolidFlags( FSOLID_TRIGGER );
		}
		else
		{
	#if !defined( CLIENT_DLL )
			// Constrained start?
			if ( HasSpawnFlags( SF_WEAPON_START_CONSTRAINED ) )
			{
				//Constrain the weapon in place
				IPhysicsObject *pReferenceObject, *pAttachedObject;
				
				pReferenceObject = g_PhysWorldObject;
				pAttachedObject = VPhysicsGetObject();

				if ( pReferenceObject && pAttachedObject )
				{
					constraint_fixedparams_t fixed;
					fixed.Defaults();
					fixed.InitWithCurrentObjectState( pReferenceObject, pAttachedObject );
					
					fixed.constraint.forceLimit	= lbs2kg( 10000 );
					fixed.constraint.torqueLimit = lbs2kg( 10000 );

					IPhysicsConstraint *pConstraint = GetConstraint();

					pConstraint = physenv->CreateFixedConstraint( pReferenceObject, pAttachedObject, NULL, fixed );

					pConstraint->SetGameData( (void *) this );
				}
			}
	#endif //CLIENT_DLL
		}
	}

	SetPickupTouch();
	
	SetThink( &CBaseCombatWeapon::FallThink );

	SetNextThink( gpGlobals->curtime + 0.1f );

#endif
}

const CHL2MPSWeaponInfo &CWeaponHL2MPBase::GetHL2MPWpnData() const
{
	const FileWeaponInfo_t *pWeaponInfo = &GetWpnData();
	const CHL2MPSWeaponInfo *pHL2MPInfo;

	#ifdef _DEBUG
		pHL2MPInfo = dynamic_cast< const CHL2MPSWeaponInfo* >( pWeaponInfo );
		Assert( pHL2MPInfo );
	#else
		pHL2MPInfo = static_cast< const CHL2MPSWeaponInfo* >( pWeaponInfo );
	#endif

	return *pHL2MPInfo;
}
void CWeaponHL2MPBase::FireBullets( const FireBulletsInfo_t &info )
{
	FireBulletsInfo_t modinfo = info;

	modinfo.m_iPlayerDamage = GetHL2MPWpnData().m_iPlayerDamage;

	BaseClass::FireBullets( modinfo );
}


#if defined( CLIENT_DLL )

#include "c_te_effect_dispatch.h"

#define NUM_MUZZLE_FLASH_TYPES 4

bool CWeaponHL2MPBase::OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options )
{
	return BaseClass::OnFireEvent( pViewModel, origin, angles, event, options );
}


void UTIL_ClipPunchAngleOffset( QAngle &in, const QAngle &punch, const QAngle &clip )
{
	QAngle	final = in + punch;

	//Clip each component
	for ( int i = 0; i < 3; i++ )
	{
		if ( final[i] > clip[i] )
		{
			final[i] = clip[i];
		}
		else if ( final[i] < -clip[i] )
		{
			final[i] = -clip[i];
		}

		//Return the result
		in[i] = final[i] - punch[i];
	}
}

#endif

