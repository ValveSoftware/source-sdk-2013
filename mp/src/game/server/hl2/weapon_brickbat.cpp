//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is the brickbat weapon
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "AI_Memory.h"
#include "player.h"
#include "gamerules.h"		// For g_pGameRules
#include "weapon_brickbat.h"
#include "grenade_brickbat.h"
#include "ammodef.h"
#include "in_buttons.h"
#include "game.h"			
#include "IEffects.h"
#include "vstdlib/random.h"
#include "baseviewmodel.h"
#include "movevars_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sk_npc_dmg_brickbat;
extern ConVar sk_plr_dmg_brickbat;

struct BrickbatAmmo_s
{
	const char	*m_sClassName;
	int			m_nAmmoType;
	int			m_nMaxCarry;
	const char	*m_sViewModel;
	const char	*m_sWorldModel;
};

BrickbatAmmo_s	BrickBatAmmoArray[NUM_BRICKBAT_AMMO_TYPES] =
{
	{ "grenade_rockbb",			BRICKBAT_ROCK,			5,	"models/weapons/v_bb_bottle.mdl",		"models/props_junk/Rock001a.mdl" },
	{ "grenade_beerbottle",		BRICKBAT_BOTTLE,		3,	"models/weapons/v_bb_bottle.mdl",		"models/weapons/w_bb_bottle.mdl" },
};

IMPLEMENT_SERVERCLASS_ST(CWeaponBrickbat, DT_WeaponBrickbat)
END_SEND_TABLE()

//LINK_ENTITY_TO_CLASS( weapon_brickbat, CWeaponBrickbat );
//PRECACHE_WEAPON_REGISTER(weapon_brickbat);

acttable_t	CWeaponBrickbat::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_THROW, true },
};
IMPLEMENT_ACTTABLE(CWeaponBrickbat);



BEGIN_DATADESC( CWeaponBrickbat )

	DEFINE_FIELD( m_bNeedDraw, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bNeedThrow, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iThrowBits, FIELD_INTEGER ),
	DEFINE_FIELD( m_fNextThrowCheck, FIELD_TIME ),
	DEFINE_FIELD( m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_ARRAY( m_nAmmoCount, FIELD_INTEGER, NUM_BRICKBAT_AMMO_TYPES ),
	DEFINE_KEYFIELD( m_iCurrentAmmoType, FIELD_INTEGER, "BrickbatType" ),

	// Function Pointers
	DEFINE_FUNCTION( BrickbatTouch ),

END_DATADESC()

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeaponBrickbat::Precache( void )
{
	for (int i=0;i<ARRAYSIZE(BrickBatAmmoArray);i++)
	{
		PrecacheModel(BrickBatAmmoArray[i].m_sWorldModel);
		PrecacheModel(BrickBatAmmoArray[i].m_sViewModel);
	}

	UTIL_PrecacheOther("grenade_molotov");

	BaseClass::Precache();
}

void CWeaponBrickbat::Spawn( void )
{
	m_bNeedDraw		= true;
	m_bNeedThrow	= false;

	for (int i=0;i<NUM_BRICKBAT_AMMO_TYPES;i++)
	{
		m_nAmmoCount[i] = 0;
	}

	// Call base class first
	BaseClass::Spawn();

	// Deactivate the trigger bounds so we can pick it up with the physgun
	CollisionProp()->UseTriggerBounds( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CWeaponBrickbat::GetViewModel( int viewmodelindex /*=0*/ )
{
	return BrickBatAmmoArray[m_iCurrentAmmoType].m_sViewModel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CWeaponBrickbat::GetWorldModel( void )
{
	return BrickBatAmmoArray[m_iCurrentAmmoType].m_sWorldModel;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CWeaponBrickbat::Deploy( void )
{
	SetModel( GetViewModel() );
	m_bNeedDraw		= false;
	m_bNeedThrow	= false;
	return DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), ACT_VM_DRAW, (char*)GetAnimPrefix() );
}

//------------------------------------------------------------------------------
// Purpose : Override to use brickbats pickup touch function
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeaponBrickbat::SetPickupTouch( void )
{
	SetTouch( BrickbatTouch );
}


//-----------------------------------------------------------------------------
// Purpose: Override so give correct ammo
// Input  : pOther - the entity that touched me
// Output :
//-----------------------------------------------------------------------------
void CWeaponBrickbat::BrickbatTouch( CBaseEntity *pOther )
{
	// ---------------------------------------------------
	//  First give weapon to touching entity if allowed
	//  Skip ammo given portion by setting clips to zero
	//  and handle ammo giving here
	// ---------------------------------------------------
	BaseClass::DefaultTouch(pOther);

	//FIXME: This ammo handling code is a bit bogus, need a real solution if brickbats are going to live

	/*
	// ----------------------------------------------------
	//  Give brickbat ammo if touching client
	// ----------------------------------------------------
	if (pOther->GetFlags() & FL_CLIENT)
	{
		CBaseCombatCharacter* pBCC = ToBaseCombatCharacter( pOther );

		// Exit if game rules say I can't have any more of this ammo type.
		if ( g_pGameRules->CanHaveAmmo( pBCC, m_iPrimaryAmmoType ) == false )
			return;

		// ------------------------------------------------
		//  If already owned weapon of this type remove me
		// ------------------------------------------------
		CWeaponBrickbat* oldWeapon = (CWeaponBrickbat*)pBCC->Weapon_OwnsThisType( GetClassname() );
		
		// Remove physics object if is one
		VPhysicsDestroyObject();

		if ( ( oldWeapon != NULL ) && ( oldWeapon != this ) )
		{
			// Only pick up if not at max ammo amount
			if (oldWeapon->m_nAmmoCount[m_iCurrentAmmoType] < BrickBatAmmoArray[m_iCurrentAmmoType].m_nMaxCarry)
			{
				oldWeapon->m_nAmmoCount[m_iCurrentAmmoType]++;
				pBCC->GiveAmmo( 1, oldWeapon->m_iPrimaryAmmoType ); 
				UTIL_Remove( this );
			}
		}
		else
		{
			// Only pick up if not at max ammo amount
			if (m_nAmmoCount[m_iCurrentAmmoType] < BrickBatAmmoArray[m_iCurrentAmmoType].m_nMaxCarry)
			{
				m_nAmmoCount[m_iCurrentAmmoType]++;
				pBCC->GiveAmmo( 1, m_iPrimaryAmmoType ); 

				SetThink (NULL);
			}
		}

		// -----------------------------------------------------
		// Switch to this weapon if the only weapon I own
		// -----------------------------------------------------
		if (!pBCC->GetActiveWeapon() && pBCC->GetActiveWeapon() != this)
		{
			pBCC->Weapon_Switch(oldWeapon);
		}
	}
	*/
}


//-----------------------------------------------------------------------------
// Purpose: Gets event from anim stream and throws the object
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponBrickbat::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_THROW:
		{
			CAI_BaseNPC *pNPC = GetOwner()->MyNPCPointer();

			if (!pNPC)
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
			if (iBIndex != -1) {
				Vector origin;
				QAngle angles;
				pNPC->GetBonePosition( iBIndex, launchPos, angles);
			}
			// Otherwise just set to in front of the owner
			else {
				Vector vFacingDir = pNPC->BodyDirection2D( );
				vFacingDir = vFacingDir * 60.0; 
				launchPos = pNPC->GetLocalOrigin()+vFacingDir;
			}

			ThrowBrickbat( launchPos, m_vecTossVelocity, sk_npc_dmg_brickbat.GetFloat());

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
bool CWeaponBrickbat::ObjectInWay( void )
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
bool CWeaponBrickbat::WeaponLOSCondition(const Vector &ownerPos, const Vector &targetPos,bool bSetConditions)
{
	// <<TODO>> should test if can throw from present location here...
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Override to check throw
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CWeaponBrickbat::WeaponRangeAttack1Condition( float flDot, float flDist )
{
	// If things haven't changed too much since last time
	// just return that previously calculated value
	if (gpGlobals->curtime < m_fNextThrowCheck )
	{
		return m_iThrowBits;
	}
	
	if ( flDist < m_fMinRange1) 
	{
		m_iThrowBits = COND_TOO_CLOSE_TO_ATTACK;
	}
	else if (flDist > m_fMaxRange1) 
	{
		m_iThrowBits = COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.5) 
	{
		m_iThrowBits = COND_NOT_FACING_ATTACK;
	}

	// If moving, can't throw.
	else if ( m_flGroundSpeed != 0 )
	{
		m_iThrowBits = COND_NONE;
	}
	else 
	{
		// Ok we should check again as some time has passed
		// This function is only used by NPC's so we can cast to a Base Monster
		CAI_BaseNPC *pNPC	= GetOwner()->MyNPCPointer();
		CBaseEntity *pEnemy = pNPC->GetEnemy();

		if (!pEnemy)
		{
			return COND_NONE;
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
void CWeaponBrickbat::ThrowBrickbat( Vector vecSrc, Vector vecVelocity, float damage)
{
	CGrenade_Brickbat *pBrickbat = (CGrenade_Brickbat*)Create( BrickBatAmmoArray[m_iCurrentAmmoType].m_sClassName, vecSrc, vec3_angle, GetOwner() );

	if (!pBrickbat)
	{
		Msg("Brickbat type (%s) not defined!\n",BrickBatAmmoArray[m_iCurrentAmmoType].m_sClassName);
		return;
	}

	AngularImpulse vecAngVel;
	// Tumble through the air
	vecAngVel.x = random->RandomFloat ( -100, -500 );
	vecAngVel.z = random->RandomFloat ( -100, -500 );
	vecAngVel.y = random->RandomFloat ( -100, -500 );

	// If physically simulated
	IPhysicsObject *pPhysicsObject = pBrickbat->VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->AddVelocity( &vecVelocity, &vecAngVel );
	}
	// Otherwise
	else
	{
		pBrickbat->SetAbsVelocity( vecVelocity );

		QAngle angVel;
		AngularImpulseToQAngle( vecAngVel, angVel );
		pBrickbat->SetLocalAngularVelocity( angVel );
	}

	pBrickbat->SetThrower( GetOwner() );
	pBrickbat->SetOwnerEntity( ((CBaseEntity*)GetOwner()) );
	pBrickbat->SetDamage(damage);

	m_nAmmoCount[m_iCurrentAmmoType]--;

	m_bNeedThrow = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponBrickbat::PrimaryAttack( void )
{

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
	{
		return;
	}

	SendWeaponAnim(ACT_VM_PULLBACK);
	
	// Don't fire again until fire animation has completed
	float flSequenceEndTime = gpGlobals->curtime + SequenceDuration();
	pPlayer->m_flNextAttack = m_flNextPrimaryAttack = m_flNextSecondaryAttack = flSequenceEndTime;

	m_bNeedThrow = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponBrickbat::Throw( void )
{

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
	{
		return;
	}

	Vector vecSrc		= pPlayer->WorldSpaceCenter();
	Vector vecFacing	= pPlayer->BodyDirection3D( );
	vecSrc				= vecSrc + vecFacing * 18.0;
	vecSrc.z		   += 24.0f;

	// Player may have turned to face a wall during the throw anim in which case
	// we don't want to throw the SLAM into the wall
	if (ObjectInWay())
	{
		vecSrc = pPlayer->WorldSpaceCenter() + vecFacing * 5.0;
	}

	Vector vecAiming = pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	vecAiming.z += 0.20; // Raise up so passes through reticle

	ThrowBrickbat(vecSrc, vecAiming*800, sk_plr_dmg_brickbat.GetFloat());
	pPlayer->RemoveAmmo( 1, m_iPrimaryAmmoType );

	SendWeaponAnim(ACT_VM_THROW);
	
	// Don't fire again until fire animation has completed
	float flSequenceEndTime = gpGlobals->curtime + SequenceDuration();
	pPlayer->m_flNextAttack = m_flNextPrimaryAttack = m_flNextSecondaryAttack = flSequenceEndTime;

	m_bNeedThrow = false;
	m_bNeedDraw	 = true;
}
//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponBrickbat::SecondaryAttack( void )
{
	int counter = 0;
	while (counter < NUM_BRICKBAT_AMMO_TYPES)
	{
		m_iCurrentAmmoType = ((++m_iCurrentAmmoType)%NUM_BRICKBAT_AMMO_TYPES);

		// If I've found a category with ammo stop looking
		if (m_nAmmoCount[m_iCurrentAmmoType] > 0)
		{
			DrawAmmo();
			return;
		}
		counter++;
	}
	// I'm out of all ammo types
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponBrickbat::DrawAmmo( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	// -------------------------------------------
	// Make sure I have ammo of the current type
	// -------------------------------------------
	int counter = 0;
	while (m_nAmmoCount[m_iCurrentAmmoType] <=0)
	{
		m_iCurrentAmmoType = ((++m_iCurrentAmmoType)%NUM_BRICKBAT_AMMO_TYPES);
		counter++;

		// ----------------------------------------------------
		// No ammo of any types so drop the weapon and destroy
		// ----------------------------------------------------
		if (counter >= NUM_BRICKBAT_AMMO_TYPES)
		{
			pOwner->Weapon_Drop( this, NULL, NULL );
			UTIL_Remove(this);
			return;
		}
	}
	SetModel( BrickBatAmmoArray[m_iCurrentAmmoType].m_sViewModel);
	CBaseViewModel *vm = pOwner->GetViewModel();
	if ( vm )
	{
		vm->SetModel( BrickBatAmmoArray[m_iCurrentAmmoType].m_sViewModel );
	}

	//Msg("Drawing %s...\n",BrickBatAmmoArray[m_iCurrentAmmoType].m_sClassName);
	m_bNeedDraw = false;

	SendWeaponAnim(ACT_VM_DRAW);

	// Don't fire again until fire animation has completed
	float flSequenceEndTime = gpGlobals->curtime + SequenceDuration();
	pOwner->m_flNextAttack = m_flNextPrimaryAttack = m_flNextSecondaryAttack = flSequenceEndTime;
}

//-----------------------------------------------------------------------------
// Purpose: Override so shotgun can do mulitple reloads in a row
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponBrickbat::ItemPostFrame( void )
{
	/*  HANDY FOR DEBUG
	for (int i=0;i<NUM_BRICKBAT_AMMO_TYPES;i++)
	{
		Msg("%i %s",m_nAmmoCount[i],BrickBatAmmoArray[i].m_sClassName);
		if (i==m_iCurrentAmmoType)
		{
			Msg("**");
		}
		Msg("\n");
	}
	*/

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if (!pOwner)
	{
		return;
	}

	if (m_bNeedThrow)
	{
		Throw();
	}
	else if ((pOwner->m_nButtons & IN_ATTACK2) && (m_flNextSecondaryAttack <= gpGlobals->curtime))
	{
		SecondaryAttack();
	}
	else if ((pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		// Uses secondary ammo only
		if (pOwner->GetAmmoCount(m_iPrimaryAmmoType))
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
		SendWeaponAnim( ACT_VM_IDLE );
		//pOwner->m_flNextAttack = gpGlobals->curtime + SequenceDuration();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponBrickbat::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	if ( info.GetDamageType() & DMG_BULLET)
	{
		if ( BrickBatAmmoArray[m_iCurrentAmmoType].m_nAmmoType == BRICKBAT_ROCK )
		{
			g_pEffects->Ricochet(ptr->endpos,ptr->plane.normal);
		}	
	}
	BaseClass::TraceAttack( info, vecDir, ptr );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponBrickbat::CWeaponBrickbat( void )
{
#ifdef _DEBUG
	m_vecTossVelocity.Init();
#endif

	m_fMinRange1	= 200;
	m_fMaxRange1	= 1000;
}
