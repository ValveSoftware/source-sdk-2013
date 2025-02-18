//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Base Rockets.
//
//=============================================================================//
#include "cbase.h"
#include "tf_wearable_weapons.h"
#include "tf_gamerules.h"
#include "engine/ivdebugoverlay.h"
#include "tf_weapon_medigun.h"

#ifdef CLIENT_DLL
#include "tf_weapon_sword.h"
#include "renamed_recvtable_compat.h"

NOTE_RENAMED_RECVTABLE( DT_TFWearableItemDemoShield, DT_TFWearableDemoShield )
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( tf_wearable_demoshield, CTFWearableDemoShield );
IMPLEMENT_NETWORKCLASS_ALIASED( TFWearableDemoShield, DT_TFWearableDemoShield )

// Network Table --
BEGIN_NETWORK_TABLE( CTFWearableDemoShield, DT_TFWearableDemoShield )
END_NETWORK_TABLE()
// -- Network Table

// Data Desc --
BEGIN_DATADESC( CTFWearableDemoShield )
END_DATADESC()
// -- Data Desc

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWearableDemoShield::CTFWearableDemoShield() : CTFWearable()
{
#ifdef GAME_DLL
	m_bImpactedSomething = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearableDemoShield::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "DemoCharge.HitWorld" );
	PrecacheScriptSound( "DemoCharge.HitFlesh" );
	PrecacheScriptSound( "DemoCharge.HitFleshRange" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearableDemoShield::DoSpecialAction( CTFPlayer *pPlayer )
{
	if ( CanCharge( pPlayer ) )
	{
		DoCharge( pPlayer );
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearableDemoShield::EndSpecialAction( CTFPlayer *pPlayer )
{
	int nAttackNotCancelCharge = 0;
	CALL_ATTRIB_HOOK_INT( nAttackNotCancelCharge, attack_not_cancel_charge );
	if ( nAttackNotCancelCharge > 0 )
	{
		return;
	}

	if ( pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
	{
		pPlayer->m_Shared.CalcChargeCrit();
		pPlayer->m_Shared.SetDemomanChargeMeter( 0 );
		pPlayer->m_Shared.RemoveCond( TF_COND_SHIELD_CHARGE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWearableDemoShield::CanCharge( CTFPlayer *pPlayer )
{
	// 6/14/2011 - Demos can now charge while in the air to counter pyro control.
//	if ( !(pPlayer->GetFlags() & FL_ONGROUND) )
//		return false;

	if ( TFGameRules() && (TFGameRules()->State_Get() == GR_STATE_PREROUND) )
		return false;

	if ( pPlayer->m_Shared.IsLoser() )
		return false;

	if ( pPlayer->m_Shared.InCond( TF_COND_STUNNED ) )
		return false;

	if ( pPlayer->IsTaunting() )
		return false;

	if ( pPlayer->GetGrapplingHookTarget() )
		return false;

	if ( pPlayer->m_Shared.GetDemomanChargeMeter() == 100.f )
		return true;

	if ( TFGameRules() && TFGameRules()->IsPasstimeMode() && pPlayer->m_Shared.HasPasstimeBall() )
		return false;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearableDemoShield::DoCharge( CTFPlayer *pPlayer )
{
#ifdef GAME_DLL
	float flChargeTime = 1.5f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer, flChargeTime, mod_charge_time );
	pPlayer->m_Shared.AddCond( TF_COND_SHIELD_CHARGE, flChargeTime );

	m_bImpactedSomething = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearableDemoShield::ShieldBash( CTFPlayer *pPlayer, float flCurrentChargeMeter )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

#ifdef GAME_DLL

	if ( m_bImpactedSomething )
		return;

	m_bImpactedSomething = true;

	// Setup the swing range.
	Vector vecForward; 
	AngleVectors( pOwner->EyeAngles(), &vecForward );
	Vector vecStart = pOwner->Weapon_ShootPosition();
	Vector vecEnd = vecStart + vecForward * 48;

	// See if we hit anything.
	trace_t trace;
	UTIL_TraceHull( vecStart, vecEnd, -Vector(24,24,24), Vector(24,24,24),
		MASK_SOLID, pOwner, COLLISION_GROUP_NONE, &trace );

	// Play an impact sound.
	bool bImpactDamage = false;
	if ( trace.m_pEnt )
	{
		const char* pszSoundName = "";
		if ( trace.m_pEnt->IsPlayer() )
		{
			bImpactDamage = true;
			pszSoundName = "DemoCharge.HitFleshRange";
		}
		else
		{
			pszSoundName = "DemoCharge.HitWorld";
		}
		pOwner->EmitSound( pszSoundName );
	}

	// Apply impact damage, if any.
	if ( bImpactDamage )
	{
		float flBashDamage = CalculateChargeDamage( flCurrentChargeMeter );
		CTakeDamageInfo info;
		info.SetAttacker( pOwner );
		info.SetInflictor( this ); 
		info.SetWeapon( this );
		info.SetDamage( flBashDamage );
		info.SetDamageCustom( TF_DMG_CUSTOM_CHARGE_IMPACT );
		info.SetDamageForce( GetShieldDamageForce( flCurrentChargeMeter ) );
		info.SetDamagePosition( trace.endpos );
		info.SetDamageType( DMG_CLUB );

		Vector dir;
		AngleVectors( pOwner->GetAbsAngles(), &dir );
		trace.m_pEnt->DispatchTraceAttack( info, dir, &trace );
		ApplyMultiDamage();

		// Calculate charge crit if we did any bash damage
		pOwner->m_Shared.CalcChargeCrit();
	}

	UTIL_ScreenShake( pOwner->WorldSpaceCenter(), 25.0, 150.0, 1.0, 750, SHAKE_START );

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFWearableDemoShield::CalculateChargeDamage( float flCurrentChargeMeter )
{
	float flImpactDamage = RemapValClamped( flCurrentChargeMeter, 90.0f, 40.0f, 15.0f, 50.0f );

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return flImpactDamage;

	// Cap at 5 decapitations for dmg bonus
	int iDecaps = Min( pOwner->m_Shared.GetDecapitations(), 5 );
	if ( iDecaps > 0 )
	{
		flImpactDamage *= (1.0f + iDecaps * 0.1f );
	}

	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, flImpactDamage, charge_impact_damage );

	return flImpactDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CTFWearableDemoShield::GetShieldDamageForce( float flCurrentChargeMeter )
{
	Vector vecVelocity = GetAbsVelocity();
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->GetVelocity( &vecVelocity, NULL );
		VectorNormalize( vecVelocity );
	}

	return (vecVelocity * CalculateChargeDamage( flCurrentChargeMeter ) );
}

//-----------------------------------------------------------------------------
// Purpose: Attaches the item to the player.
//-----------------------------------------------------------------------------
void CTFWearableDemoShield::Equip( CBasePlayer* pOwner )
{
	BaseClass::Equip( pOwner );
	if ( !CanEquip( pOwner ) )
		return;

	CTFPlayer *pTFPlayer = ToTFPlayer( pOwner );
	if ( pTFPlayer )
	{
		pTFPlayer->m_Shared.SetShieldEquipped( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Remove item from the player.
//-----------------------------------------------------------------------------
void CTFWearableDemoShield::UnEquip( CBasePlayer* pOwner )
{
	BaseClass::UnEquip( pOwner );

	CTFPlayer *pTFPlayer = ToTFPlayer( pOwner );
	if ( pTFPlayer )
	{
		pTFPlayer->m_Shared.SetShieldEquipped( false );
	}
}


LINK_ENTITY_TO_CLASS( tf_wearable_razorback, CTFWearableRazorback );
IMPLEMENT_NETWORKCLASS_ALIASED( TFWearableRazorback, DT_TFWearableRazorback )

// Network Table --
BEGIN_NETWORK_TABLE( CTFWearableRazorback, DT_TFWearableRazorback )
END_NETWORK_TABLE()
// -- Network Table

// Data Desc --
BEGIN_DATADESC( CTFWearableRazorback )
END_DATADESC()
// -- Data Desc

void CTFWearableRazorback::OnResourceMeterFilled()
{
#ifdef GAME_DLL
	// unhide the model when meter is filled
	if ( IsEffectActive( EF_NODRAW ) )
	{
		RemoveEffects( EF_NODRAW );
	}
#endif // GAME_DLL
}
