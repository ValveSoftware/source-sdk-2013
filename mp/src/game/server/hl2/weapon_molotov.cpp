//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is the molotov weapon
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include	"npcevent.h"
#include	"basehlcombatweapon.h"
#include	"basecombatcharacter.h"
#include	"ai_basenpc.h"
#include	"AI_Memory.h"
#include	"player.h"
#include	"gamerules.h"		// For g_pGameRules
#include	"weapon_molotov.h"
#include	"grenade_molotov.h"
#include	"in_buttons.h"
#include	"game.h"			
#include "vstdlib/random.h"
#include "movevars_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CWeaponMolotov )

	DEFINE_FIELD( m_nNumAmmoTypes, FIELD_INTEGER ),
	DEFINE_FIELD( m_bNeedDraw, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iThrowBits, FIELD_INTEGER ),
	DEFINE_FIELD( m_fNextThrowCheck, FIELD_TIME ),
	DEFINE_FIELD( m_vecTossVelocity, FIELD_VECTOR ),

	// Function Pointers
	DEFINE_FUNCTION( MolotovTouch ),

END_DATADESC()


IMPLEMENT_SERVERCLASS_ST(CWeaponMolotov, DT_WeaponMolotov)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_molotov, CWeaponMolotov );
PRECACHE_WEAPON_REGISTER(weapon_molotov);

acttable_t	CWeaponMolotov::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_THROW, true },
};
IMPLEMENT_ACTTABLE(CWeaponMolotov);


void CWeaponMolotov::Precache( void )
{
	PrecacheModel("models/props_junk/w_garb_beerbottle.mdl");	//<<TEMP>> need real model
	BaseClass::Precache();
}

void CWeaponMolotov::Spawn( void )
{
	// Call base class first
	BaseClass::Spawn();

	m_bNeedDraw		= true;

	SetModel( GetWorldModel() );
	UTIL_SetSize(this, Vector( -6, -6, -2), Vector(6, 6, 2));
}

//------------------------------------------------------------------------------
// Purpose : Override to use molotovs pickup touch function
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeaponMolotov::SetPickupTouch( void )
{
	SetTouch(MolotovTouch);
}

//-----------------------------------------------------------------------------
// Purpose: Override so give correct ammo
// Input  : pOther - the entity that touched me
// Output :
//-----------------------------------------------------------------------------
void CWeaponMolotov::MolotovTouch( CBaseEntity *pOther )
{
	// ---------------------------------------------------
	//  First give weapon to touching entity if allowed
	// ---------------------------------------------------
	BaseClass::DefaultTouch(pOther);

	// ----------------------------------------------------
	//  Give molotov ammo if touching client
	// ----------------------------------------------------
	if (pOther->GetFlags() & FL_CLIENT)
	{
		// ------------------------------------------------
		//  If already owned weapon of this type remove me
		// ------------------------------------------------
		CBaseCombatCharacter* pBCC = ToBaseCombatCharacter( pOther );
		CWeaponMolotov* oldWeapon = (CWeaponMolotov*)pBCC->Weapon_OwnsThisType( GetClassname() );
		if (oldWeapon != this)
		{
			UTIL_Remove( this );
		}
		else
		{
			pBCC->GiveAmmo( 1, m_iSecondaryAmmoType );
			SetThink (NULL);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets event from anim stream and throws the object
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponMolotov::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_THROW:
		{
			CAI_BaseNPC *pNPC	= GetOwner()->MyNPCPointer();
			if (!pNPC)
			{
				return;
			}

			CBaseEntity *pEnemy = pNPC->GetEnemy();
			if (!pEnemy)
			{
				return;
			}

			Vector vec_target = pNPC->GetEnemyLKP();

			// -----------------------------------------------------
			//  Get position of throw
			// -----------------------------------------------------
			// If owner has a hand, set position to the hand bone position
			Vector launchPos;
			int iBIndex = pNPC->LookupBone("Bip01 R Hand");
			if (iBIndex != -1) 
			{
				Vector origin;
				QAngle angles;
				pNPC->GetBonePosition( iBIndex, launchPos, angles);
			}
			// Otherwise just set to in front of the owner
			else 
			{
				Vector vFacingDir = pNPC->BodyDirection2D( );
				vFacingDir = vFacingDir * 60.0; 
				launchPos = pNPC->GetAbsOrigin()+vFacingDir;
			}


			//Vector vecVelocity = VecCheckToss( pNPC, launchPos, vec_target, 1.0 );

			ThrowMolotov( launchPos, m_vecTossVelocity);

			// Drop the weapon and remove as no more ammo
			pNPC->Weapon_Drop( this );
			UTIL_Remove( this );
		}
		break;
		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CWeaponMolotov::ObjectInWay( void )
{
	CBaseCombatCharacter *pOwner  = GetOwner();
	if (!pOwner)
	{
		return false;
	}

	Vector vecSrc	 = pOwner->Weapon_ShootPosition( );
	Vector vecAiming = pOwner->BodyDirection2D( );

	trace_t tr;

	Vector	vecEnd = vecSrc + (vecAiming * 32);
	UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID, pOwner, COLLISION_GROUP_NONE, &tr );
	
	if (tr.fraction < 1.0)
	{
		// Don't block on a living creature
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
// Purpose: Override to allow throw w/o LOS
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CWeaponMolotov::WeaponLOSCondition(const Vector &ownerPos, const Vector &targetPos,bool bSetConditions)
{
	// <<TODO>> should test if can throw from present location here...
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Override to check throw
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CWeaponMolotov::WeaponRangeAttack1Condition( float flDot, float flDist )
{
	// If things haven't changed too much since last time
	// just return that previously calculated value
	if (gpGlobals->curtime < m_fNextThrowCheck )
	{
		return m_iThrowBits;
	}
	
	if ( flDist < m_fMinRange1) {
		m_iThrowBits = COND_TOO_CLOSE_TO_ATTACK;
	}
	else if (flDist > m_fMaxRange1) {
		m_iThrowBits = COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.5) {
		m_iThrowBits = COND_NOT_FACING_ATTACK;
	}

	// If moving, can't throw.
	else if ( m_flGroundSpeed != 0 )
	{
		m_iThrowBits = COND_NONE;
	}
	else {
		// Ok we should check again as some time has passed
		// This function is only used by NPC's so we can cast to a Base Monster
		CAI_BaseNPC *pNPC	= GetOwner()->MyNPCPointer();
		CBaseEntity *pEnemy = pNPC->GetEnemy();

		if (!pEnemy)
		{
			m_iThrowBits = COND_NONE;
		}
		// Get Enemy Position 
		Vector vecTarget;
		pEnemy->CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 0.0f ), &vecTarget );

		// Get Toss Vector
		Vector			throwStart  = pNPC->Weapon_ShootPosition();
		Vector			vecToss;
		CBaseEntity*	pBlocker	= NULL;
		float			throwDist	= (throwStart - vecTarget).Length();
		float			fGravity	= GetCurrentGravity();
		float			throwLimit	= pNPC->ThrowLimit(throwStart, vecTarget, fGravity, 35, WorldAlignMins(), WorldAlignMaxs(), pEnemy, &vecToss, &pBlocker);

		// If I can make the throw (or most of the throw)
		if (!throwLimit || (throwLimit != throwDist && throwLimit > 0.8*throwDist))
		{
			m_vecTossVelocity = vecToss;
			m_iThrowBits = COND_CAN_RANGE_ATTACK1;

		}
		else
		{
			m_iThrowBits = COND_NONE;
		}

	}
	// don't check again for a while.
	m_fNextThrowCheck = gpGlobals->curtime + 0.33; // 1/3 second.

	return m_iThrowBits;
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponMolotov::ThrowMolotov( const Vector &vecSrc, const Vector &vecVelocity)
{
	CGrenade_Molotov *pMolotov = (CGrenade_Molotov*)Create( "grenade_molotov", vecSrc, vec3_angle, GetOwner() );

	if (!pMolotov)
	{
		Msg("Couldn't make molotov!\n");
		return;
	}
	pMolotov->SetAbsVelocity( vecVelocity );

	// Tumble through the air
	QAngle angVel( random->RandomFloat ( -100, -500 ), random->RandomFloat ( -100, -500 ), random->RandomFloat ( -100, -500 ) ); 
	pMolotov->SetLocalAngularVelocity( angVel );

	pMolotov->SetThrower( GetOwner() );
	pMolotov->SetOwnerEntity( ((CBaseEntity*)GetOwner()) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponMolotov::PrimaryAttack( void )
{

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
	{
		return;
	}

	Vector vecSrc		= pPlayer->WorldSpaceCenter();
	Vector vecFacing	= pPlayer->BodyDirection3D( );
	vecSrc				= vecSrc + vecFacing * 18.0;
	// BUGBUG: is this some hack because it's not at the eye position????
	vecSrc.z		   += 24.0f;

	// Player may have turned to face a wall during the throw anim in which case
	// we don't want to throw the SLAM into the wall
	if (ObjectInWay())
	{
		vecSrc   = pPlayer->WorldSpaceCenter() + vecFacing * 5.0;
	}

	Vector vecAiming = pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	vecAiming.z += 0.20; // Raise up so passes through reticle

	ThrowMolotov(vecSrc, vecAiming*800);
	pPlayer->RemoveAmmo( 1, m_iSecondaryAmmoType );

	
	// Don't fire again until fire animation has completed
	//m_flNextPrimaryAttack = gpGlobals->curtime + CurSequenceDuration();
	//<<TEMP>> - till real animation is avaible
	m_flNextPrimaryAttack = gpGlobals->curtime + 1.0;
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.0;

	m_bNeedDraw = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponMolotov::SecondaryAttack( void )
{
	//<<TEMP>>
	// Hmmm... what should I do here?
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponMolotov::DrawAmmo( void )
{
	// -------------------------------------------
	// Make sure I have ammo of the current type
	// -------------------------------------------
	CBaseCombatCharacter *pOwner = GetOwner();
	if (pOwner->GetAmmoCount(m_iSecondaryAmmoType) <=0)
	{
		pOwner->Weapon_Drop( this );
		UTIL_Remove(this);
		return;
	}
	Msg("Drawing Molotov...\n");
	m_bNeedDraw = false;

	//<<TEMP>> - till real animation is avaible
	m_flNextPrimaryAttack	= gpGlobals->curtime + 2.0;
	m_flNextSecondaryAttack = gpGlobals->curtime + 2.0;

}

//-----------------------------------------------------------------------------
// Purpose: Override so shotgun can do mulitple reloads in a row
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponMolotov::ItemPostFrame( void )
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
	else if ((pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		// Uses secondary ammo only
		if (pOwner->GetAmmoCount(m_iSecondaryAmmoType))
		{
			PrimaryAttack();
		}
	}
	else if (m_bNeedDraw)
	{
		DrawAmmo();
	}
	else
	{
		WeaponIdle( );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponMolotov::CWeaponMolotov( void )
{
#ifdef _DEBUG
	m_vecTossVelocity.Init();
#endif

	m_fMinRange1	= 200;
	m_fMaxRange1	= 1000;
}