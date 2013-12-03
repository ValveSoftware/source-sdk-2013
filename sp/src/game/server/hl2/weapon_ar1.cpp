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
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DAMAGE_PER_SECOND 10

#define MAX_SETTINGS	5

float RateOfFire[ MAX_SETTINGS ] = 
{
	0.1,
	0.2,
	0.5,
	0.7,
	1.0,
};

float Damage[ MAX_SETTINGS ] =
{
	2,
	4,
	10,
	14,
	20,
};


//=========================================================
//=========================================================
class CWeaponAR1 : public CHLMachineGun
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponAR1, CHLMachineGun );

	DECLARE_SERVERCLASS();

	CWeaponAR1();

	int m_ROF;

	void	Precache( void );
	bool	Deploy( void );

	float GetFireRate( void ) {return RateOfFire[ m_ROF ];}

	int CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	void SecondaryAttack( void );

	virtual void FireBullets( const FireBulletsInfo_t &info );

	virtual const Vector& GetBulletSpread( void )
	{
		static const Vector cone = VECTOR_CONE_10DEGREES;
		return cone;
	}

	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
	{
		switch( pEvent->event )
		{
			case EVENT_WEAPON_AR1:
			{
				Vector vecShootOrigin, vecShootDir;
				vecShootOrigin = pOperator->Weapon_ShootPosition( );

				CAI_BaseNPC *npc = pOperator->MyNPCPointer();
				ASSERT( npc != NULL );
				
				vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

				WeaponSound(SINGLE_NPC);
				pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2 );
				pOperator->DoMuzzleFlash();
			}
			break;
			default:
				CBaseCombatWeapon::Operator_HandleAnimEvent( pEvent, pOperator );
				break;
		}
	}
	DECLARE_ACTTABLE();
};

IMPLEMENT_SERVERCLASS_ST(CWeaponAR1, DT_WeaponAR1)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_ar1, CWeaponAR1 );
PRECACHE_WEAPON_REGISTER(weapon_ar1);

acttable_t	CWeaponAR1::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_AR1, true },
};

IMPLEMENT_ACTTABLE(CWeaponAR1);

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CWeaponAR1 )

	DEFINE_FIELD( m_ROF,			FIELD_INTEGER ),

END_DATADESC()


CWeaponAR1::CWeaponAR1( )
{
	m_ROF = 0;
}

void CWeaponAR1::Precache( void )
{
	BaseClass::Precache();
}

bool CWeaponAR1::Deploy( void )
{
	//CBaseCombatCharacter *pOwner  = m_hOwner;
	return BaseClass::Deploy();
}


//=========================================================
//=========================================================
void CWeaponAR1::FireBullets( const FireBulletsInfo_t &info )
{
	if(CBasePlayer *pPlayer = ToBasePlayer( GetOwner() ))
	{
		pPlayer->FireBullets( info );
	}
}


void CWeaponAR1::SecondaryAttack( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( pPlayer )
	{
		pPlayer->m_nButtons &= ~IN_ATTACK2;
	}

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.1;

	m_ROF += 1;

	if( m_ROF == MAX_SETTINGS )
	{
		m_ROF = 0;
	}

	int i;

	Msg( "\n" );
	for( i = 0 ; i < MAX_SETTINGS ; i++ )
	{
		if( i == m_ROF )
		{
			Msg( "|" );
		}
		else
		{
			Msg( "-" );
		}
	}
	Msg( "\n" );
}
