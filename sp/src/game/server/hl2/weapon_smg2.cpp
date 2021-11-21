//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "NPCevent.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "game.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponSMG2 : public CHLSelectFireMachineGun
{
public:
	DECLARE_CLASS( CWeaponSMG2, CHLSelectFireMachineGun );

	CWeaponSMG2();

	DECLARE_SERVERCLASS();

	const Vector	&GetBulletSpread( void );

	void			Precache( void );
	void			AddViewKick( void );
	void			Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	float			GetFireRate( void ) { return 0.1f; }
	int				CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	DECLARE_ACTTABLE();
};

IMPLEMENT_SERVERCLASS_ST(CWeaponSMG2, DT_WeaponSMG2)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_smg2, CWeaponSMG2 );
PRECACHE_WEAPON_REGISTER(weapon_smg2);

acttable_t	CWeaponSMG2::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SMG2, true },

#if EXPANDED_HL2_UNUSED_WEAPON_ACTIVITIES
	// Optional new NPC activities
	// (these should fall back to SMG animations when they don't exist on an NPC)
	{ ACT_RELOAD,					ACT_RELOAD_SMG2,			true },
	{ ACT_IDLE,						ACT_IDLE_SMG2,				true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG2,		true },

// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SMG2_RELAXED,			false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SMG2_STIMULATED,		false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_SMG2,			false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_SMG2_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_SMG2_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_SMG2,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_SMG2_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_SMG2_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_SMG2,				false },//always aims

// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG2_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_SMG2_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG2,			false },//always aims

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_SMG2_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_SMG2_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_SMG2,				false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_SMG2_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_SMG2_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_SMG2,				false },//always aims
//End readiness activities

	{ ACT_WALK,						ACT_WALK_SMG2,					true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_SMG2,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,					true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,				true },
	{ ACT_RUN,						ACT_RUN_SMG2,					true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_SMG2,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,					true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,				true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SMG2,	true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG2_LOW,		true },
	{ ACT_COVER_LOW,				ACT_COVER_SMG2_LOW,				false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_SMG2_LOW,			false },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG2_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG2,		true },

	{ ACT_ARM,						ACT_ARM_RIFLE,					false },
	{ ACT_DISARM,					ACT_DISARM_RIFLE,				false },

#if EXPANDED_HL2_COVER_ACTIVITIES
	{ ACT_RANGE_AIM_MED,			ACT_RANGE_AIM_SMG2_MED,			false },
	{ ACT_RANGE_ATTACK1_MED,		ACT_RANGE_ATTACK_SMG2_MED,		false },
#endif

#if EXPANDED_HL2DM_ACTIVITIES
	// HL2:DM activities (for third-person animations in SP)
	{ ACT_HL2MP_IDLE,                    ACT_HL2MP_IDLE_SMG2,                    false },
	{ ACT_HL2MP_RUN,                    ACT_HL2MP_RUN_SMG2,                    false },
	{ ACT_HL2MP_IDLE_CROUCH,            ACT_HL2MP_IDLE_CROUCH_SMG2,            false },
	{ ACT_HL2MP_WALK_CROUCH,            ACT_HL2MP_WALK_CROUCH_SMG2,            false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,    ACT_HL2MP_GESTURE_RANGE_ATTACK_SMG2,    false },
	{ ACT_HL2MP_GESTURE_RELOAD,            ACT_HL2MP_GESTURE_RELOAD_SMG2,        false },
	{ ACT_HL2MP_JUMP,                    ACT_HL2MP_JUMP_SMG2,                    false },
	{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_SMG2,					false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK2,	ACT_HL2MP_GESTURE_RANGE_ATTACK2_SMG2,    false },
#endif
#endif
};

IMPLEMENT_ACTTABLE(CWeaponSMG2);

//=========================================================
CWeaponSMG2::CWeaponSMG2( )
{
	m_fMaxRange1		= 2000;
	m_fMinRange1		= 32;

	m_iFireMode			= FIREMODE_3RNDBURST;
}

void CWeaponSMG2::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const Vector
//-----------------------------------------------------------------------------
const Vector &CWeaponSMG2::GetBulletSpread( void )
{
	static const Vector cone = VECTOR_CONE_10DEGREES;
	return cone;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponSMG2::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_SMG2:
		{
			Vector vecShootOrigin, vecShootDir;
			vecShootOrigin = pOperator->Weapon_ShootPosition( );

			CAI_BaseNPC *npc = pOperator->MyNPCPointer();
			ASSERT( npc != NULL );
			vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

			WeaponSound(SINGLE_NPC);
			pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2 );
			pOperator->DoMuzzleFlash();
			m_iClip1 = m_iClip1 - 1;
		}
		break;
		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSMG2::AddViewKick( void )
{
	#define	EASY_DAMPEN			0.5f
	#define	MAX_VERTICAL_KICK	2.0f	//Degrees
	#define	SLIDE_LIMIT			1.0f	//Seconds
	
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
		return;

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}
