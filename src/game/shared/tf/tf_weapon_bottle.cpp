//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_bottle.h"
#include "decals.h"

// Client specific.
#ifdef CLIENT_DLL
#include "prediction.h"
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_fx.h"
#include "tf_gamerules.h"
#endif

//=============================================================================
//
// Weapon Breakable Melee tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFBreakableMelee, DT_TFWeaponBreakableMelee )

BEGIN_NETWORK_TABLE( CTFBreakableMelee, DT_TFWeaponBreakableMelee )
#if defined( CLIENT_DLL )
	RecvPropBool( RECVINFO( m_bBroken ), 0, CTFBreakableMelee::RecvProxy_Broken )
#else
	SendPropBool( SENDINFO( m_bBroken ) )
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBreakableMelee )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_bBroken, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nBody, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_INSENDTABLE )
#endif // CLIENT_DLL
END_PREDICTION_DATA()

//=============================================================================
//
// Weapon Bottle tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFBottle, DT_TFWeaponBottle )

BEGIN_NETWORK_TABLE( CTFBottle, DT_TFWeaponBottle )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBottle )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_bottle, CTFBottle );
PRECACHE_WEAPON_REGISTER( tf_weapon_bottle );

//=============================================================================
//
// Weapon Breakable Sign tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFBreakableSign, DT_TFWeaponBreakableSign )

BEGIN_NETWORK_TABLE( CTFBreakableSign, DT_TFWeaponBreakableSign )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBreakableSign )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_breakable_sign, CTFBreakableSign );
PRECACHE_WEAPON_REGISTER( tf_weapon_breakable_sign );

//=============================================================================
//
// Weapon Stickbomb tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFStickBomb, DT_TFWeaponStickBomb )

#ifdef CLIENT_DLL
void RecvProxy_Detonated( const CRecvProxyData *pData, void *pStruct, void *pOut );
#endif

BEGIN_NETWORK_TABLE( CTFStickBomb, DT_TFWeaponStickBomb )
#if defined( CLIENT_DLL )
	RecvPropInt( RECVINFO( m_iDetonated ), 0, RecvProxy_Detonated )
#else
	SendPropInt( SENDINFO( m_iDetonated ), 1, SPROP_UNSIGNED )
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFStickBomb )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_stickbomb, CTFStickBomb );
PRECACHE_WEAPON_REGISTER( tf_weapon_stickbomb );

#define TF_WEAPON_STICKBOMB_NORMAL_MODEL	"models/workshop/weapons/c_models/c_caber/c_caber.mdl"
#define TF_WEAPON_STICKBOMB_BROKEN_MODEL	"models/workshop/weapons/c_models/c_caber/c_caber_exploded.mdl"

//=============================================================================

#define TF_BREAKABLE_MELEE_BREAK_BODYGROUP 0
// Absolute body number of broken/not-broken since the server can't figure them out from the studiohdr.  Would only
// matter if we had other body groups going on anyway
#define TF_BREAKABLE_MELEE_BODY_NOTBROKEN 0
#define TF_BREAKABLE_MELEE_BODY_BROKEN 1

//=============================================================================
//
// Weapon Breakable Melee functions.
//

CTFBreakableMelee::CTFBreakableMelee()
{
	m_bBroken = false;
}

void CTFBreakableMelee::WeaponReset( void )
{
	BaseClass::WeaponReset();

	if ( !GetOwner() || !GetOwner()->IsAlive() )
	{
		m_bBroken = false;
	}
}

bool CTFBreakableMelee::DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt )
{
	bool bRet = BaseClass::DefaultDeploy( szViewModel, szWeaponModel, iActivity, szAnimExt );

	if ( bRet )
	{
		SwitchBodyGroups();
	}

	return bRet;
}

void CTFBreakableMelee::SwitchBodyGroups( void )
{
	int iState = 0;

	if ( m_bBroken == true )
	{
		iState = 1;
	}

#ifdef CLIENT_DLL
	// We'll successfully predict m_nBody along with m_bBroken, but this can be called outside prediction, in which case
	// we want to use the networked m_nBody value -- but still fixup our viewmodel which is clientside only.
	if ( prediction->InPrediction() )
		{ SetBodygroup( TF_BREAKABLE_MELEE_BREAK_BODYGROUP, iState ); }

	CTFPlayer *pTFPlayer = ToTFPlayer( GetOwner() );
	if ( pTFPlayer && pTFPlayer->GetActiveWeapon() == this )
	{
		C_BaseAnimating *pViewWpn = GetAppropriateWorldOrViewModel();
		if ( pViewWpn != this )
		{
			pViewWpn->SetBodygroup( TF_BREAKABLE_MELEE_BREAK_BODYGROUP, iState );
		}
	}
#else // CLIENT_DLL
	m_nBody = iState ? TF_BREAKABLE_MELEE_BODY_BROKEN : TF_BREAKABLE_MELEE_BODY_NOTBROKEN;
#endif // CLIENT_DLL
}

bool CTFBreakableMelee::UpdateBodygroups( CBaseCombatCharacter* pOwner, int iState )
{
	SwitchBodyGroups();

	return BaseClass::UpdateBodygroups( pOwner, iState );
}

void CTFBreakableMelee::Smack( void )
{
	BaseClass::Smack();

	if ( ConnectedHit() && IsCurrentAttackACrit() )
	{
		SetBroken( true );
	}
}

void CTFBreakableMelee::SetBroken( bool bBroken )
{ 
	m_bBroken = bBroken;
	SwitchBodyGroups();
}

#ifdef CLIENT_DLL
/* static */ void CTFBreakableMelee::RecvProxy_Broken( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_TFBreakableMelee* pWeapon = ( C_TFBreakableMelee*) pStruct;

	if ( !!pData->m_Value.m_Int != pWeapon->m_bBroken )
	{
		pWeapon->m_bBroken = !!pData->m_Value.m_Int;
		pWeapon->SwitchBodyGroups();
	}
}
#endif // CLIENT_DLL

CTFStickBomb::CTFStickBomb()
: CTFBreakableMelee()
{
	m_iDetonated = 0;
}

void CTFStickBomb::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( TF_WEAPON_STICKBOMB_NORMAL_MODEL );
	PrecacheModel( TF_WEAPON_STICKBOMB_BROKEN_MODEL );
}

void CTFStickBomb::Smack( void )
{
	CTFWeaponBaseMelee::Smack();

	// Stick bombs detonate once, on impact.
	if ( m_iDetonated == 0 && ConnectedHit() )
	{
		m_iDetonated = 1;
		m_bBroken = true;
		SwitchBodyGroups();

#ifdef GAME_DLL
		CTFPlayer *pTFPlayer = ToTFPlayer( GetOwner() );
		if ( pTFPlayer )
		{
			Vector vecForward; 
			AngleVectors( pTFPlayer->EyeAngles(), &vecForward );
			Vector vecSwingStart = pTFPlayer->Weapon_ShootPosition();
			Vector vecSwingEnd = vecSwingStart + vecForward * GetSwingRange();

			Vector explosion = vecSwingStart;

			CPVSFilter filter( explosion );
			
			// Halloween Spell
			int iHalloweenSpell = 0;
			int iCustomParticleIndex = INVALID_STRING_INDEX;
			if ( TF_IsHolidayActive( kHoliday_HalloweenOrFullMoon ) )
			{
				CALL_ATTRIB_HOOK_INT_ON_OTHER( this, iHalloweenSpell, halloween_pumpkin_explosions );
				if ( iHalloweenSpell > 0 )
				{
					iCustomParticleIndex = GetParticleSystemIndex( "halloween_explosion" );
				}
			}

			TE_TFExplosion( filter, 0.0f, explosion, Vector(0,0,1), TF_WEAPON_GRENADELAUNCHER, pTFPlayer->entindex(), -1, SPECIAL1, iCustomParticleIndex );

			int dmgType = DMG_BLAST | DMG_USEDISTANCEMOD | DMG_MELEE;
			if ( IsCurrentAttackACrit() )
				dmgType |= DMG_CRITICAL;

			float flDamage = 75.0f;
			CALL_ATTRIB_HOOK_FLOAT( flDamage, mult_dmg );

			CTakeDamageInfo info( pTFPlayer, pTFPlayer, this, explosion, explosion, flDamage, dmgType, TF_DMG_CUSTOM_STICKBOMB_EXPLOSION, &explosion );

			float flRadius = 100.f;
			CALL_ATTRIB_HOOK_FLOAT( flRadius, mult_explosion_radius );

			CTFRadiusDamageInfo radiusinfo( &info, explosion, flRadius );
			TFGameRules()->RadiusDamage( radiusinfo );
		}
#endif
	}
}

void CTFStickBomb::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_iDetonated = 0;

	SwitchBodyGroups();
}

void CTFStickBomb::WeaponRegenerate( void )
{
	BaseClass::WeaponRegenerate();

	m_iDetonated = 0;

	SetContextThink( &CTFStickBomb::SwitchBodyGroups, gpGlobals->curtime + 0.01f, "SwitchBodyGroups" );
}

void CTFStickBomb::SwitchBodyGroups( void )
{
#ifdef CLIENT_DLL
	if ( !GetViewmodelAttachment() )
		return;

	if ( m_iDetonated == 1 )
	{
		GetViewmodelAttachment()->SetModel( TF_WEAPON_STICKBOMB_BROKEN_MODEL );
	}
	else
	{
		GetViewmodelAttachment()->SetModel( TF_WEAPON_STICKBOMB_NORMAL_MODEL );
	}
#endif
}

const char *CTFStickBomb::GetWorldModel( void ) const
{
	if ( m_iDetonated == 1 )
	{
		return TF_WEAPON_STICKBOMB_BROKEN_MODEL;
	}
	else
	{
		return BaseClass::GetWorldModel();
	}
}

#ifdef CLIENT_DLL
int CTFStickBomb::GetWorldModelIndex( void )
{
	if ( !modelinfo )
		return BaseClass::GetWorldModelIndex();

	if ( m_iDetonated == 1 )
	{
		m_iWorldModelIndex = modelinfo->GetModelIndex( TF_WEAPON_STICKBOMB_BROKEN_MODEL );
		return m_iWorldModelIndex;
	}
	else
	{
		m_iWorldModelIndex = modelinfo->GetModelIndex( TF_WEAPON_STICKBOMB_NORMAL_MODEL );
		return m_iWorldModelIndex;
	}
}
#endif

#ifdef CLIENT_DLL

void RecvProxy_Detonated( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_TFStickBomb* pBomb = (C_TFStickBomb*) pStruct;

	if ( pData->m_Value.m_Int != pBomb->GetDetonated() )
	{
		pBomb->SetDetonated( pData->m_Value.m_Int );
		pBomb->SwitchBodyGroups();
	}
}

#endif
