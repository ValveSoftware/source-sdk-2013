//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:	'weapon' what lets the player controll the rollerbuddy.
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "NPCevent.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "entitylist.h"
#include "ndebugoverlay.h"
#include "soundent.h"
#include "engine/IEngineSound.h"
#include "rotorwash.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar thumpFrequency( "thumpfrequency", "2" );
ConVar thumpRadius( "thumpradius", "512" );


//=========================================================
//=========================================================
class CPortableThumper : public CBaseAnimating
{
	DECLARE_CLASS( CPortableThumper, CBaseAnimating );
private:

	void ThumpThink( void );
	void ThumperUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Precache( void );
	void Spawn( void );

	int ObjectCaps( void ) { return FCAP_IMPULSE_USE; }

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( portable_thumper, CPortableThumper );



void CPortableThumper::Precache( void )
{
	PrecacheModel( "models/fire_equipment/w_firehydrant.mdl" );
}

void CPortableThumper::Spawn( void )
{
	m_takedamage = DAMAGE_NO;

	SetModel( "models/fire_equipment/w_firehydrant.mdl" );

	Vector vecBBMin, vecBBMax;

	vecBBMin.z = 0;
	vecBBMin.x = -16;
	vecBBMin.y = -16;

	vecBBMax.z = 32;
	vecBBMax.x = 16;
	vecBBMax.y = 16;

	SetSolid( SOLID_BBOX );
	UTIL_SetSize( this, vecBBMin, vecBBMax );

	SetThink( ThumpThink );
	SetUse( ThumperUse );
	SetNextThink( gpGlobals->curtime + thumpFrequency.GetFloat() );
}

void CPortableThumper::ThumperUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !pActivator->IsPlayer() )
	{
		return;
	}

	CBasePlayer *pPlayer;

	pPlayer = (CBasePlayer *)pActivator;
	pPlayer->GiveNamedItem( "weapon_thumper" );

	UTIL_Remove( this );
}

void CPortableThumper::ThumpThink( void )
{
	EmitSound( "PortableThumper.ThumpSound" );

	UTIL_RotorWash( GetAbsOrigin() + Vector( 0, 0, 32 ), Vector( 0, 0, -1 ), 512 );	

	SetNextThink( gpGlobals->curtime + thumpFrequency.GetFloat() );

	CSoundEnt::InsertSound( SOUND_THUMPER, GetAbsOrigin(), thumpRadius.GetInt(), 0.2, this );
}

BEGIN_DATADESC( CPortableThumper )

	DEFINE_FUNCTION( ThumpThink ),
	DEFINE_FUNCTION( ThumperUse ),

END_DATADESC()






class CWeaponThumper: public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CWeaponThumper, CBaseHLCombatWeapon );
public:
	DECLARE_SERVERCLASS();
	void				Spawn( void );
	void				Precache( void );

	int					CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	void				PrimaryAttack( void );
	bool				Reload( void );
	void				DecrementAmmo( CBaseCombatCharacter *pOwner );
};

IMPLEMENT_SERVERCLASS_ST(CWeaponThumper, DT_WeaponThumper)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_thumper, CWeaponThumper );
PRECACHE_WEAPON_REGISTER(weapon_thumper);

void CWeaponThumper::Spawn( )
{
	BaseClass::Spawn();

	Precache( );

	UTIL_SetSize(this, Vector(-4,-4,-2),Vector(4,4,2));

	FallInit();// get ready to fall down
}

void CWeaponThumper::Precache( void )
{
	BaseClass::Precache();
	UTIL_PrecacheOther( "portable_thumper" );

	PrecacheScriptSound( "PortableThumper.ThumpSound" );

}

bool CWeaponThumper::Reload( void )
{
	WeaponIdle();
	return true;
}

void CWeaponThumper::PrimaryAttack( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	DecrementAmmo( pPlayer );

	trace_t tr;

	Vector vecStart, vecDir;

	Vector vecForward;
	Vector vecSpot;

	pPlayer->GetVectors( &vecForward, NULL, NULL );

	vecForward.z = 0.0;

	vecStart = pPlayer->WorldSpaceCenter() + vecForward * 64;

	UTIL_TraceLine( vecStart, vecStart - Vector( 0, 0, 128 ), MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr );

	if( tr.fraction != 1.0 )
	{
		Create( "portable_thumper", tr.endpos, vec3_angle, NULL );
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.5;
	}

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5;
}


void CWeaponThumper::DecrementAmmo( CBaseCombatCharacter *pOwner )
{
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
	
	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		pOwner->Weapon_Drop( this );
		UTIL_Remove(this);
	}
}

/*
//=========================================================
//=========================================================
class CWeaponThumper : public CBaseHLCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponThumper, CBaseHLCombatWeapon );

	CWeaponThumper();

	DECLARE_SERVERCLASS();

	void	Precache( void );
	bool	Deploy( void );

	int CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	void PrimaryAttack( void );
	void SecondaryAttack( void );

	DECLARE_ACTTABLE();
};

IMPLEMENT_SERVERCLASS_ST(CWeaponThumper, DT_WeaponThumper)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_thumper, CWeaponThumper );
PRECACHE_WEAPON_REGISTER(weapon_thumper);

acttable_t	CWeaponThumper::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_AR1, true },
};

IMPLEMENT_ACTTABLE(CWeaponThumper);

CWeaponThumper::CWeaponThumper( )
{
}

void CWeaponThumper::Precache( void )
{
	UTIL_PrecacheOther( "portable_thumper" );
	BaseClass::Precache();
}

bool CWeaponThumper::Deploy( void )
{
	bool fReturn;

	fReturn = BaseClass::Deploy();

	m_flNextPrimaryAttack = gpGlobals->curtime;
	m_flNextSecondaryAttack = gpGlobals->curtime;
	m_hOwner->m_flNextAttack = gpGlobals->curtime + 0.0;

	return fReturn;
}

void CWeaponThumper::PrimaryAttack( void )
{
	trace_t tr;

	Vector vecStart, vecDir;

	Vector vecForward;
	Vector vecSpot;

	m_hOwner->GetVectors( &vecForward, NULL, NULL );

	vecForward.z = 0.0;

	vecStart = m_hOwner->Center() + vecForward * 64;

	UTIL_TraceLine( vecStart, vecStart - Vector( 0, 0, 128 ), MASK_SHOT, m_hOwner, COLLISION_GROUP_NONE, &tr );

	if( tr.fraction != 1.0 )
	{
		Create( "portable_thumper", tr.endpos, vec3_origin, NULL );
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.5;
	}

	m_hOwner->m_iAmmo[m_iPrimaryAmmoType]++;
}

void CWeaponThumper::SecondaryAttack( void )
{
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
}
*/