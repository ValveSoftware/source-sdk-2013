//=============================================================================
//
// snowball! :)
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_snowball.h"

#ifdef GAME_DLL
#include "tf_player.h"
#include "studio.h"
#else
#include "c_tf_player.h"
#include "iclientmode.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tf_snowball_damage( "tf_snowball_damage", "3", FCVAR_REPLICATED | FCVAR_NOTIFY, "Damage dealt by a snowball to an enemy player, or a teammate if friendly fire is turned on" );
ConVar tf_snowball_recharge( "tf_snowball_recharge", "10", FCVAR_REPLICATED | FCVAR_NOTIFY, "Recharge time in seconds for snowball item" );

//=============================================================================
//
// Weapon Snowball tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFSnowball, DT_TFWeaponSnowball )
BEGIN_NETWORK_TABLE( CTFSnowball, DT_TFWeaponSnowball )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSnowball )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_snowball, CTFSnowball );
PRECACHE_WEAPON_REGISTER( tf_weapon_snowball );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Snowball, DT_TFProjectile_Snowball )
BEGIN_NETWORK_TABLE( CTFProjectile_Snowball, DT_TFProjectile_Snowball )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_snowball, CTFProjectile_Snowball );
PRECACHE_WEAPON_REGISTER( tf_projectile_snowball );

//=============================================================================
//
// Weapon Snowball functions.
//
CTFSnowball::CTFSnowball()
{
	m_bThrown = false;
	m_flSwitchBackTime = 0.0f;
#ifdef CLIENT_DLL
	m_hHandEffect = NULL;
	m_hHandEffectWeapon = NULL;
#endif // CLIENT_DLL
}

void CTFSnowball::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel( TF_SNOWBALL_MODEL );
	PrecacheScriptSound( "Snowball.KillJingle" );
	PrecacheScriptSound( "Snowball.Throw" );
	PrecacheScriptSound( "Snowball.Splat" );
	PrecacheParticleSystem( "snowball_trail" );
	PrecacheParticleSystem( "snowball_impact" );
	PrecacheParticleSystem( "snowball_1" );
}

bool CTFSnowball::CanDeploy( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && pPlayer->m_Shared.IsStealthed() )
		return false;

	return BaseClass::CanDeploy();
}

bool CTFSnowball::Deploy( void )
{
	m_bThrown = false;
	m_flSwitchBackTime = 0.0f;

	return BaseClass::Deploy();
}

float CTFSnowball::InternalGetEffectBarRechargeTime( void )
{
	return tf_snowball_recharge.GetFloat();
}

void CTFSnowball::GetProjectileEntityName( CAttribute_String *attrProjectileEntityName )
{
	attrProjectileEntityName->set_value( "tf_projectile_snowball" );
}

void CTFSnowball::PrimaryAttack( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer || m_bThrown )
		return;

	int iAmmo = pPlayer->GetAmmoCount( m_iPrimaryAmmoType );

	BaseClass::PrimaryAttack();

	if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) < iAmmo )
	{
		m_bThrown = true;
		m_flSwitchBackTime = gpGlobals->curtime + 0.5f;
	}
}

//-----------------------------------------------------------------------------
void CTFSnowball::ItemBusyFrame( void )
{
#ifdef CLIENT_DLL
	if ( m_hHandEffectWeapon && m_hHandEffect )
		return;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( IsFirstPersonView() )
	{
		m_hHandEffectWeapon = pPlayer->GetViewModel();
	}
	else
	{
		//only need this effect in first person since we're hiding the snowball model due to odd weapon_bone movement in the spell cast animations
		return;
	}

	if ( !m_hHandEffectWeapon )
		return;

	if ( UsingViewModel() && !g_pClientMode->ShouldDrawViewModel() )
	{
		// Prevent effects when the ViewModel is hidden with r_drawviewmodel=0
		return;
	}

	C_BaseAnimating *pBase = (C_BaseAnimating*)m_hHandEffectWeapon.Get();
	int iAttachment = pBase->C_BaseAnimating::LookupAttachment( "effect_hand_R" );

	// Start the muzzle flash, if a system hasn't already been started.
	if ( iAttachment > 0 )
	{
		m_hHandEffect = pBase->ParticleProp()->Create( "snowball_trail", PATTACH_POINT_FOLLOW, iAttachment );
	}
	else
	{
		if ( m_hHandEffect )
		{
			m_hHandEffectWeapon->ParticleProp()->StopEmission( m_hHandEffect );
			m_hHandEffectWeapon = NULL;
			m_hHandEffect = NULL;
		}
	}
#endif
}

//-----------------------------------------------------------------------------
void CTFSnowball::ItemHolsterFrame( void )
{
	BaseClass::ItemHolsterFrame();
	m_bThrown = false;
#ifdef CLIENT_DLL
	if ( !m_hHandEffectWeapon )
		return;

	// Stop the muzzle flash.
	if ( m_hHandEffect )
	{
		m_hHandEffectWeapon->ParticleProp()->StopEmission( m_hHandEffect );
		m_hHandEffectWeapon = NULL;
		m_hHandEffect = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
void CTFSnowball::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

#ifdef CLIENT_DLL
	if ( m_hHandEffect )
	{
		m_hHandEffectWeapon->ParticleProp()->StopEmission( m_hHandEffect );
		m_hHandEffectWeapon = NULL;
		m_hHandEffect = NULL;
	}
#endif

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !m_bThrown )
	{
		if ( m_flNextPrimaryAttack > gpGlobals->curtime )
			return;

		PrimaryAttack();

		if ( !m_bThrown )
		{
			m_bThrown = true;
			m_flSwitchBackTime = gpGlobals->curtime + 0.5f;
		}
		return;
	}

#ifdef GAME_DLL
	if ( m_flSwitchBackTime > gpGlobals->curtime )
		return;

	CBaseCombatWeapon *pLastWeapon = pPlayer->GetLastWeapon();
	if ( pLastWeapon && pLastWeapon != this && pPlayer->Weapon_CanSwitchTo( pLastWeapon ) )
	{
		pPlayer->Weapon_Switch( pLastWeapon );
	}
	else
	{
		pPlayer->SwitchToNextBestWeapon( this );
	}
#endif
}

//-----------------------------------------------------------------------------
/* static */ const char *CTFSnowball::GetHandEffect( CEconItemView *pItem )
{
	if ( pItem && pItem->GetSOCData() && ( pItem->GetSOCData()->GetQuality() == AE_SELFMADE || pItem->GetSOCData()->GetQuality() == AE_COMMUNITY ) )
	{
		return "community_sparkle";
	}
	return "snowball_1";
}

#ifdef GAME_DLL
float CTFProjectile_Snowball::GetDamage()
{
	return ( tf_snowball_damage.GetFloat() == 0.0f ? 0.001f : tf_snowball_damage.GetFloat() );
}

extern ConVar friendlyfire;

void CTFProjectile_Snowball::OnHit( CBaseEntity *pOther )
{
	if ( m_bHit )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	CTFPlayer *pThrower = ToTFPlayer( GetThrower() );

	if ( pPlayer && pThrower )
	{
		CTraceFilterIgnoreTeammates tracefilter( this, COLLISION_GROUP_NONE, GetTeamNumber() );
		trace_t trace;
		UTIL_TraceLine( GetAbsOrigin(), pPlayer->GetAbsOrigin(), ( MASK_SHOT & ~( CONTENTS_HITBOX ) ), &tracefilter, &trace );

		// Find the hitbox closest to where we've hit
		trace.hitgroup = HITGROUP_GENERIC;
		CStudioHdr *pStudioHdr = pPlayer->GetModelPtr();
		mstudiohitboxset_t *set = pStudioHdr ? pStudioHdr->pHitboxSet( pPlayer->GetHitboxSet() ) : NULL;
		if ( set )
		{
			const Vector &vecImpact = GetAbsOrigin();
			float flClosestDist = FLT_MAX;
			for ( int i = 0; i < set->numhitboxes; i++ )
			{
				mstudiobbox_t *pBox = set->pHitbox( i );

				Vector vecBonePos;
				QAngle angBone;
				pPlayer->GetBonePosition( pBox->bone, vecBonePos, angBone );

				Vector vecClosest;
				CalcClosestPointOnAABB( vecBonePos + pBox->bbmin, vecBonePos + pBox->bbmax, vecImpact, vecClosest );

				float flDist = vecClosest.DistToSqr( vecImpact );
				if ( flDist < flClosestDist )
				{
					flClosestDist = flDist;
					trace.hitgroup = pBox->group;
				}
			}
		}

		Vector vecToTarget;
		vecToTarget = pPlayer->WorldSpaceCenter() - this->WorldSpaceCenter();
		VectorNormalize( vecToTarget );

		// Apply Damage to Victim
		CTakeDamageInfo info;
		info.SetAttacker( pThrower );
		info.SetInflictor( this );
		info.SetWeapon( GetLauncher() );
		info.SetDamage( ( pPlayer->InSameTeam( pThrower ) && !friendlyfire.GetBool() ) ? 0.001f : GetDamage() );
		info.SetDamageCustom( GetCustomDamageType() );
		info.SetDamagePosition( GetAbsOrigin() );
		info.SetDamageType( info.GetDamage() < 0.1f ? DMG_GENERIC : DMG_CLUB ); //if snowball dealing no damage, no push force nor flinch
		CalculateMeleeDamageForce( &info, vecToTarget, GetAbsOrigin(), 0.0f );

		pPlayer->DispatchTraceAttack( info, vecToTarget, &trace );
		ApplyMultiDamage();
	}

	BaseClass::OnHit( pOther );
}
#endif // GAME_DLL

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// CEquipSnowballNotification
//-----------------------------------------------------------------------------
void CEquipSnowballNotification::Accept()
{
	m_bHasTriggered = true;

	CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();
	if ( !pLocalInv )
	{
		MarkForDeletion();
		return;
	}

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
	{
		MarkForDeletion();
		return;
	}

	// Default item becomes a snowball in this mode
	itemid_t iItemId = INVALID_ITEM_ID;

	if ( iItemId == INVALID_ITEM_ID )
	{
		iItemId = 0;

		static CSchemaItemDefHandle pItemDef_Snowball( "TF_WEAPON_SNOWBALL" );
		CEconItemView *pDefaultSnowball = TFInventoryManager()->GetBaseItemForClass( pLocalPlayer->GetPlayerClass()->GetClassIndex(), LOADOUT_POSITION_ACTION );
		if ( pDefaultSnowball )
		{
			if ( pDefaultSnowball->GetItemDefinition() == pItemDef_Snowball )
			{
				iItemId = pDefaultSnowball->GetItemID();
			}
		}
	}

	TFInventoryManager()->EquipItemInLoadout( pLocalPlayer->GetPlayerClass()->GetClassIndex(), LOADOUT_POSITION_ACTION, iItemId );

	// Tell the GC to tell server that we should respawn if we're in a respawn room

	MarkForDeletion();
}

//===========================================================================================
void CEquipSnowballNotification::UpdateTick()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer )
	{
		CTFSnowball *pSnowball = dynamic_cast<CTFSnowball*>( TFInventoryManager()->GetItemInLoadoutForClass( pLocalPlayer->GetPlayerClass()->GetClassIndex(), LOADOUT_POSITION_ACTION ) );
		if ( pSnowball )
		{
			MarkForDeletion();
		}
	}
}

#endif // CLIENT_DLL