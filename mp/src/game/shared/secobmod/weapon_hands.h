//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef HL2MP_WEAPON_hands_H
#define HL2MP_WEAPON_hands_H
#pragma once


#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "weapon_hl2mpbasebasebludgeon.h"


#ifdef CLIENT_DLL
#define CWeaponhands C_Weaponhands
#endif

//-----------------------------------------------------------------------------
// CWeaponhands
//-----------------------------------------------------------------------------

class CWeaponhands : public CBaseHL2MPBludgeonWeapon
{
public:
	DECLARE_CLASS( CWeaponhands, CBaseHL2MPBludgeonWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponhands();

	float		GetRange( void );
	float		GetFireRate( void );

	void		AddViewKick( void );
	float		GetDamageForActivity( Activity hitActivity );
	void		SecondaryAttack( void )	{	return;	}

	void		Drop( const Vector &vecVelocity );

#ifdef SecobMod__Enable_Fixed_Multiplayer_AI	
	CWeaponhands( const CWeaponhands & ); 
#endif //SecobMod__Enable_Fixed_Multiplayer_AI


	// Animation event
#ifndef CLIENT_DLL
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#ifdef SecobMod__Enable_Fixed_Multiplayer_AI
	virtual int WeaponMeleeAttack1Condition( float flDot, float flDist ); 
#else
int WeaponMeleeAttack1Condition( float flDot, float flDist );
#endif //SecobMod__Enable_Fixed_Multiplayer_AI
private: 
	// Animation event handlers 
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#endif

};


#endif // HL2MP_WEAPON_hands_H

