//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Heavily armored combine infantry
//
//=============================================================================

#include "cbase.h"
#include "npc_combines.h"
#include "weapon_physcannon.h"
#include "hl2_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	sk_combine_armored_health( "sk_combine_armored_health","0");
ConVar	sk_combine_armored_kick( "sk_combine_armored_kick","0");

//-----------------------------------------------------------------------------
// Purpose: Heavily armored combine infantry
//-----------------------------------------------------------------------------
class CArmorPiece : public CBaseAnimating
{
	DECLARE_CLASS( CArmorPiece, CBaseAnimating );
public: 
	void Spawn( void )
	{
		BaseClass::Spawn();
		Precache();

		SetModel( STRING(GetModelName()) );

		CreateVPhysics();
	}

	void Precache( void )
	{
		PrecacheModel( STRING(GetModelName()) );
	}

	bool CreateVPhysics( void )
	{
		SetSolid( SOLID_VPHYSICS );
		IPhysicsObject *pPhysicsObject = VPhysicsInitShadow( false, false );

		if ( !pPhysicsObject )
		{
			SetSolid( SOLID_NONE );
			SetMoveType( MOVETYPE_NONE );
			Warning("ERROR!: Can't create physics object for %s\n", STRING( GetModelName() ) );
		}

		return true;
	}
};

LINK_ENTITY_TO_CLASS( combine_armor_piece, CArmorPiece );

//-----------------------------------------------------------------------------
// Purpose: Heavily armored combine infantry
//-----------------------------------------------------------------------------
class CNPC_Combine_Armored : public CNPC_CombineS
{
	DECLARE_CLASS( CNPC_Combine_Armored, CNPC_CombineS );
public: 
	void		Spawn( void );
	void		Precache( void );

	void		SpawnArmorPieces( void );
};

LINK_ENTITY_TO_CLASS( npc_combine_armored, CNPC_Combine_Armored );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Combine_Armored::Spawn( void )
{
	Precache();
	SetModel( STRING( GetModelName() ) );

	SetHealth( sk_combine_armored_health.GetFloat() );
	SetMaxHealth( sk_combine_armored_health.GetFloat() );
	SetKickDamage( sk_combine_armored_kick.GetFloat() );

	CapabilitiesAdd( bits_CAP_ANIMATEDFACE );
	CapabilitiesAdd( bits_CAP_MOVE_SHOOT );
	CapabilitiesAdd( bits_CAP_DOORS_GROUP );

	BaseClass::Spawn();

	SpawnArmorPieces();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Combine_Armored::Precache()
{
	if( !GetModelName() )
	{
		SetModelName( MAKE_STRING( "models/armored_soldier.mdl" ) );
	}

	PrecacheModel( STRING( GetModelName() ) );

	UTIL_PrecacheOther( "combine_armor_piece" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Combine_Armored::SpawnArmorPieces( void )
{
	struct armorpiecepositions_t
	{
		char	*pszAttachment;
		char	*pszModel;
	};

	armorpiecepositions_t ArmorPiecesPositions[] =
	{
		{ "attach_L_Thigh_armor",		"models/combine_armor.mdl" },
		{ "attach_L_UpperArm_armor",	"models/combine_armor.mdl" },
		{ "attach_R_Thigh_armor",		"models/combine_armor.mdl" },  
		{ "attach_R_UpperArm_armor",	"models/combine_armor.mdl" },
		{ "attach_chest_armor",			"models/combine_armor_chest.mdl" },
	};

	for ( int i = 0; i < ARRAYSIZE(ArmorPiecesPositions); i++ )
	{
		CArmorPiece *pArmor = (CArmorPiece *)CBaseEntity::CreateNoSpawn( "combine_armor_piece", GetAbsOrigin(), GetAbsAngles(), this );
		pArmor->SetModelName( MAKE_STRING(ArmorPiecesPositions[i].pszModel) );
		pArmor->SetParent( this, LookupAttachment(ArmorPiecesPositions[i].pszAttachment) );
		pArmor->SetLocalOrigin( vec3_origin );
		pArmor->SetLocalAngles( vec3_angle );
		DispatchSpawn( pArmor );
		pArmor->Activate();
	}
}
