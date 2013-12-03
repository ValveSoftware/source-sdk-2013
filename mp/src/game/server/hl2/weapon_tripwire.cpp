//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "util.h"
#include "basehlcombatweapon.h"
#include "player.h"
#include "gamerules.h"
#include "grenade_tripwire.h"
#include "grenade_satchel.h"
#include "entitylist.h"
#include "weapon_tripwire.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CWeapon_Tripwire )

	DEFINE_FIELD( m_bNeedReload, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bClearReload, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bAttachTripwire, FIELD_BOOLEAN ),

	// Function Pointers
	DEFINE_FUNCTION( TripwireTouch ),

END_DATADESC()


IMPLEMENT_SERVERCLASS_ST(CWeapon_Tripwire, DT_Weapon_Tripwire)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_tripwire, CWeapon_Tripwire );

// BUGBUG: Enable this when the script & resources are checked in.
//PRECACHE_WEAPON_REGISTER(weapon_tripwire);

acttable_t	CWeapon_Tripwire::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_TRIPWIRE, true },
};

IMPLEMENT_ACTTABLE(CWeapon_Tripwire);


void CWeapon_Tripwire::Spawn( )
{
	UTIL_Remove(this);
	return;

	BaseClass::Spawn();

	Precache( );

	UTIL_SetSize(this, Vector(-4,-4,-2),Vector(4,4,2));


	FallInit();// get ready to fall down

	SetThink( NULL );

	// Give one piece of default ammo when first picked up
	m_iClip2 = 1;
}

void CWeapon_Tripwire::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "Weapon_Tripwire.Attach" );

	UTIL_PrecacheOther( "npc_tripwire" );
}

//------------------------------------------------------------------------------
// Purpose : Override to use tripwire's pickup touch function
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeapon_Tripwire::SetPickupTouch( void )
{
	SetTouch(TripwireTouch);
}

//-----------------------------------------------------------------------------
// Purpose: Override so give correct ammo
// Input  : pOther - the entity that touched me
// Output :
//-----------------------------------------------------------------------------
void CWeapon_Tripwire::TripwireTouch( CBaseEntity *pOther )
{
	// ---------------------------------------------------
	//  First give weapon to touching entity if allowed
	// ---------------------------------------------------
	BaseClass::DefaultTouch(pOther);

	// ----------------------------------------------------
	//  Give ammo if touching client
	// ----------------------------------------------------
	if (pOther->GetFlags() & FL_CLIENT)
	{
		// ------------------------------------------------
		//  If already owned weapon of this type remove me
		// ------------------------------------------------
		CBaseCombatCharacter* pBCC = ToBaseCombatCharacter( pOther );
		CWeapon_Tripwire* oldWeapon = (CWeapon_Tripwire*)pBCC->Weapon_OwnsThisType( GetClassname() );
		if (oldWeapon != this)
		{
			UTIL_Remove( this );
		}
		else
		{
			pBCC->GiveAmmo( 1, m_iSecondaryAmmoType );
			SetThink(NULL);
		}
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CWeapon_Tripwire::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	SetThink(NULL);
	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: Tripwire has no reload, but must call weapon idle to update state
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CWeapon_Tripwire::Reload( void )
{
	WeaponIdle( );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeapon_Tripwire::PrimaryAttack( void )
{
	CBaseCombatCharacter *pOwner  = GetOwner();
	if (!pOwner)
	{ 
		return;
	}

	if (pOwner->GetAmmoCount(m_iSecondaryAmmoType) <= 0)
	{
		return;
	}

	if (CanAttachTripwire())
	{
		StartTripwireAttach();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Secondary attack does nothing
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeapon_Tripwire::SecondaryAttack( void )
{
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeapon_Tripwire::TripwireAttach( void )
{
	CBaseCombatCharacter *pOwner  = GetOwner();
	if (!pOwner)
	{
		return;
	}

	m_bAttachTripwire = false;

	Vector vecSrc	 = pOwner->Weapon_ShootPosition();
	Vector vecAiming = pOwner->BodyDirection2D( );

	trace_t tr;

	UTIL_TraceLine( vecSrc, vecSrc + (vecAiming * 128), MASK_SOLID, pOwner, COLLISION_GROUP_NONE, &tr );
	
	if (tr.fraction < 1.0)
	{
		CBaseEntity *pEntity = tr.m_pEnt;
		if (pEntity && !(pEntity->GetFlags() & FL_CONVEYOR))
		{
			QAngle angles;
			VectorAngles(tr.plane.normal, angles);
			angles.x += 90;
			
			CBaseEntity *pEnt = CBaseEntity::Create( "npc_tripwire", tr.endpos + tr.plane.normal * 3, angles, NULL );

			CTripwireGrenade *pMine = (CTripwireGrenade *)pEnt;
			pMine->SetThrower( GetOwner() );

			pOwner->RemoveAmmo( 1, m_iSecondaryAmmoType );

			EmitSound( "Weapon_Tripwire.Attach" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeapon_Tripwire::StartTripwireAttach( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
	{
		return;
	}

	Vector vecSrc	 = pPlayer->Weapon_ShootPosition();
	Vector vecAiming = pPlayer->BodyDirection3D( );

	trace_t tr;

	UTIL_TraceLine( vecSrc, vecSrc + (vecAiming * 128), MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
	
	if (tr.fraction < 1.0)
	{
		// ALERT( at_console, "hit %f\n", tr.flFraction );

		CBaseEntity *pEntity = tr.m_pEnt;
		if (pEntity && !(pEntity->GetFlags() & FL_CONVEYOR))
		{
			// player "shoot" animation
			pPlayer->SetAnimation( PLAYER_ATTACK1 );

			// -----------------------------------------
			//  Play attach animation
			// -----------------------------------------
			SendWeaponAnim(ACT_SLAM_TRIPMINE_ATTACH);

			m_bNeedReload		= true;
			m_bAttachTripwire	= true;
		}
		else
		{
			// ALERT( at_console, "no deploy\n" );
		}
	}
	m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
	m_flNextSecondaryAttack	= gpGlobals->curtime + SequenceDuration();
//	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CWeapon_Tripwire::CanAttachTripwire( void )
{
	CBaseCombatCharacter *pOwner  = GetOwner();
	if (!pOwner)
	{
		return false;
	}

	Vector vecSrc	 = pOwner->Weapon_ShootPosition();
	Vector vecAiming = pOwner->BodyDirection2D( );

	trace_t tr;

	Vector	vecEnd = vecSrc + (vecAiming * 42);
	UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID, pOwner, COLLISION_GROUP_NONE, &tr );
	
	if (tr.fraction < 1.0)
	{
		// Don't attach to a living creature
		if (tr.m_pEnt)
		{
			CBaseEntity *pEntity = tr.m_pEnt;
			CBaseCombatCharacter *pBCC		= ToBaseCombatCharacter( pEntity );
			if (pBCC)
			{
				return false;
			}
		}
		return true;
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeapon_Tripwire::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
	{
		return;
	}

	if ((pOwner->m_nButtons & IN_ATTACK2) && (m_flNextSecondaryAttack <= gpGlobals->curtime))
	{
		SecondaryAttack();
	}
	else if (!m_bNeedReload && (pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		PrimaryAttack();
	}

	// -----------------------
	//  No buttons down
	// -----------------------
	else 
	{
		WeaponIdle( );
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeapon_Tripwire::WeaponIdle( void )
{
	// Ready to switch animations?
 	if ( HasWeaponIdleTimeElapsed() )
	{
		if (m_bClearReload)
		{
			m_bNeedReload  = false;
			m_bClearReload = false;
		}
		CBaseCombatCharacter *pOwner  = GetOwner();
		if (!pOwner)
		{
			return;
		}

		int iAnim = 0;

		if (m_bAttachTripwire)
		{
			TripwireAttach();
			iAnim = ACT_SLAM_TRIPMINE_ATTACH2;
		}	
		else if (m_bNeedReload)
		{	
			// If owner had ammo draw the correct tripwire type
			if (pOwner->GetAmmoCount(m_iSecondaryAmmoType) > 0)
			{
				iAnim = ACT_SLAM_TRIPMINE_DRAW;
				m_bClearReload			= true;
			}
			else
			{
				pOwner->Weapon_Drop( this );
				UTIL_Remove(this);
			}
		}
		else if (pOwner->GetAmmoCount(m_iSecondaryAmmoType) <= 0)
		{
			pOwner->Weapon_Drop( this );
			UTIL_Remove(this);
		}

		// If I don't need to reload just do the appropriate idle
		else
		{
			iAnim = ACT_SLAM_TRIPMINE_IDLE;
		}
		SendWeaponAnim( iAnim );
	}
}

bool CWeapon_Tripwire::Deploy( void )
{
	CBaseCombatCharacter *pOwner  = GetOwner();
	if (!pOwner)
	{
		return false;
	}

	SetModel( GetViewModel() );

	// ------------------------------
	// Pick the right draw animation
	// ------------------------------
	int iActivity;

	// If detonator is already armed
	m_bNeedReload = false;
	iActivity = ACT_SLAM_STICKWALL_ND_DRAW; 
	return DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), iActivity, (char*)GetAnimPrefix() );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CWeapon_Tripwire::CWeapon_Tripwire(void)
{
	m_bNeedReload			= true;
	m_bClearReload			= false;
	m_bAttachTripwire		= false;
}
