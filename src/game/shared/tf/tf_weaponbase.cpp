//========= Copyright Valve Corporation, All rights reserved. ============//
//
//	Weapons.
//
//=============================================================================
#include "cbase.h"
#include "in_buttons.h"
#include "takedamageinfo.h"
#include "tf_weaponbase.h"
#include "ammodef.h"
#include "tf_gamerules.h"
#include "eventlist.h"
#include "econ_item_system.h"
#include "activitylist.h"

#include "gcsdk/gcmsg.h"
#include "econ_gcmessages.h"
#include "tf_gcmessages.h"

#include "tf_weapon_wrench.h"

#include "passtime_convars.h"

// Server specific.
#if !defined( CLIENT_DLL )
#include "tf_player.h"
#include "tf_weapon_medigun.h"
#include "tf_gamestats.h"

#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_gamestats.h"
#include "ilagcompensationmanager.h"
#include "collisionutils.h"
#include "tf_team.h"
#include "tf_obj.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "particle_parse.h"
#include "tf_weaponbase_grenadeproj.h"
#include "tf_weapon_compound_bow.h"
#include "tf_projectile_arrow.h"
#include "tf_gamestats.h"
#include "bot/tf_bot_manager.h"
#include "bot/tf_bot.h"
#include "halloween/halloween_base_boss.h"
#include "tf_fx.h"
#include "tf_gamestats.h"
// Client specific.
#else
#include "c_tf_player.h"
#include "tf_viewmodel.h"
#include "hud_crosshair.h"
#include "c_tf_playerresource.h"
#include "clientmode_tf.h"
#include "r_efx.h"
#include "dlight.h"
#include "effect_dispatch_data.h"
#include "c_te_effect_dispatch.h"
#include "toolframework_client.h"
#include "hud_chat.h"
#include "econ_notifications.h"
#include "prediction.h"

// for spy material proxy
#include "tf_proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"

extern CTFWeaponInfo *GetTFWeaponInfo( int iWeapon );
#endif

#include "gc_clientsystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar tf_useparticletracers;
ConVar tf_scout_hype_pep_mod( "tf_scout_hype_pep_mod", "1.0", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_scout_hype_pep_max( "tf_scout_hype_pep_max", "99.0", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_scout_hype_pep_min_damage( "tf_scout_hype_pep_min_damage", "5.0", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

ConVar tf_weapon_criticals_nopred( "tf_weapon_criticals_nopred", "1.0", FCVAR_REPLICATED | FCVAR_CHEAT );

#ifdef _DEBUG
ConVar tf_weapon_criticals_anticheat( "tf_weapon_criticals_anticheat", "1.0", FCVAR_REPLICATED );
ConVar tf_weapon_criticals_debug( "tf_weapon_criticals_debug", "0.0", FCVAR_REPLICATED );
extern ConVar tf_weapon_criticals_force_random;
#endif // _DEBUG
extern ConVar tf_weapon_criticals_bucket_cap;
extern ConVar tf_weapon_criticals_bucket_bottom;

#ifdef CLIENT_DLL
extern ConVar cl_crosshair_file;
extern ConVar cl_flipviewmodels;
#endif

//=============================================================================
//
// Global functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool IsAmmoType( int iAmmoType, const char *pAmmoName )
{
	return GetAmmoDef()->Index( pAmmoName ) == iAmmoType;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void FindHullIntersection( const Vector &vecSrc, trace_t &tr, const Vector &mins, const Vector &maxs, CBaseEntity *pEntity )
{
	int	i, j, k;
	trace_t tmpTrace;
	Vector vecEnd;
	float distance = 1e6f;
	Vector minmaxs[2] = {mins, maxs};
	Vector vecHullEnd = tr.endpos;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc)*2);
	UTIL_TraceLine( vecSrc, vecHullEnd, MASK_SOLID, pEntity, COLLISION_GROUP_NONE, &tmpTrace );
	if ( tmpTrace.fraction < 1.0 )
	{
		tr = tmpTrace;
		return;
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			for ( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID, pEntity, COLLISION_GROUP_NONE, &tmpTrace );
				if ( tmpTrace.fraction < 1.0 )
				{
					float thisDistance = (tmpTrace.endpos - vecSrc).Length();
					if ( thisDistance < distance )
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}

//=============================================================================
//
// TFWeaponBase tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponBase, DT_TFWeaponBase )

#ifdef GAME_DLL
void* SendProxy_SendActiveLocalWeaponDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID );
void* SendProxy_SendNonLocalWeaponDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID );
#endif

//-----------------------------------------------------------------------------
// Purpose: Only sent to the player holding it.
//-----------------------------------------------------------------------------
BEGIN_NETWORK_TABLE_NOBASE( CTFWeaponBase, DT_LocalTFWeaponData )
#if defined( CLIENT_DLL )
	RecvPropTime( RECVINFO( m_flLastCritCheckTime ) ),
	RecvPropTime( RECVINFO( m_flReloadPriorNextFire ) ),
	RecvPropTime( RECVINFO( m_flLastFireTime ) ),
	RecvPropTime( RECVINFO( m_flEffectBarRegenTime ) ),
	RecvPropFloat( RECVINFO( m_flObservedCritChance ) ),
#else
	SendPropTime( SENDINFO( m_flLastCritCheckTime ) ),
	SendPropTime( SENDINFO( m_flReloadPriorNextFire ) ),
	SendPropTime( SENDINFO( m_flLastFireTime ) ),
	SendPropTime( SENDINFO( m_flEffectBarRegenTime ) ),
	SendPropFloat( SENDINFO( m_flObservedCritChance ), 16, SPROP_NOSCALE, 0.0, 100.0 ),
#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Variables sent at low precision to non-holding observers.
//-----------------------------------------------------------------------------
BEGIN_NETWORK_TABLE_NOBASE( CTFWeaponBase, DT_TFWeaponDataNonLocal )
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE( CTFWeaponBase, DT_TFWeaponBase )
// Client specific.
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bLowered ) ),
	RecvPropInt( RECVINFO( m_iReloadMode ) ),
	RecvPropBool( RECVINFO( m_bResetParity ) ), 
	RecvPropBool( RECVINFO( m_bReloadedThroughAnimEvent ) ),
	RecvPropBool( RECVINFO( m_bDisguiseWeapon ) ),
	RecvPropDataTable("LocalActiveTFWeaponData", 0, 0, &REFERENCE_RECV_TABLE(DT_LocalTFWeaponData)),
	RecvPropDataTable("NonLocalTFWeaponData", 0, 0, &REFERENCE_RECV_TABLE(DT_TFWeaponDataNonLocal)),
	RecvPropFloat( RECVINFO(m_flEnergy) ),
	RecvPropEHandle( RECVINFO( m_hExtraWearable ) ),
	RecvPropEHandle( RECVINFO( m_hExtraWearableViewModel ) ),
	RecvPropBool( RECVINFO( m_bBeingRepurposedForTaunt ) ),
	RecvPropInt( RECVINFO( m_nKillComboClass ) ),
	RecvPropInt( RECVINFO( m_nKillComboCount ) ),
	RecvPropFloat( RECVINFO( m_flInspectAnimEndTime ) ),
	RecvPropInt( RECVINFO( m_nInspectStage ) ),
	RecvPropInt( RECVINFO( m_iConsecutiveShots ) ),
#else
// Server specific.
	SendPropBool( SENDINFO( m_bLowered ) ),
	SendPropBool( SENDINFO( m_bResetParity ) ),
	SendPropInt( SENDINFO( m_iReloadMode ), 4, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bReloadedThroughAnimEvent ) ),
	SendPropBool( SENDINFO( m_bDisguiseWeapon ) ),
	SendPropDataTable("LocalActiveTFWeaponData", 0, &REFERENCE_SEND_TABLE(DT_LocalTFWeaponData), SendProxy_SendActiveLocalWeaponDataTable ),
	SendPropDataTable("NonLocalTFWeaponData", 0, &REFERENCE_SEND_TABLE(DT_TFWeaponDataNonLocal), SendProxy_SendNonLocalWeaponDataTable ),
	SendPropFloat( SENDINFO(m_flEnergy) ),
	SendPropEHandle( SENDINFO( m_hExtraWearable ) ),
	SendPropEHandle( SENDINFO( m_hExtraWearableViewModel ) ),
	SendPropBool( SENDINFO( m_bBeingRepurposedForTaunt ) ),
	SendPropInt( SENDINFO( m_nKillComboClass ), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nKillComboCount ), 2, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flInspectAnimEndTime ) ),
	SendPropInt( SENDINFO( m_nInspectStage ), -1, SPROP_VARINT ),
	SendPropInt( SENDINFO( m_iConsecutiveShots ), -1, SPROP_VARINT ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponBase ) 
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_bLowered, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iReloadMode, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bReloadedThroughAnimEvent, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bDisguiseWeapon, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( m_flLastCritCheckTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),	
	DEFINE_PRED_FIELD_TOL( m_flReloadPriorNextFire, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),	
	DEFINE_PRED_FIELD_TOL( m_flLastFireTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),	
	DEFINE_PRED_FIELD( m_bCurrentAttackIsCrit, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_iCurrentSeed, FIELD_INTEGER, 0 ),
	DEFINE_PRED_FIELD( m_flEnergy, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( m_flEffectBarRegenTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),	
	DEFINE_PRED_FIELD( m_bBeingRepurposedForTaunt, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

#ifdef GAME_DLL
BEGIN_ENT_SCRIPTDESC( CTFWeaponBase, CBaseCombatWeapon, "Team Fortress 2 Weapon" )
END_SCRIPTDESC();
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_base, CTFWeaponBase );

// Server specific.
#if !defined( CLIENT_DLL )

BEGIN_DATADESC( CTFWeaponBase )
	DEFINE_THINKFUNC( FallThink ),
END_DATADESC()

// Client specific
#else

ConVar cl_crosshaircolor( "cl_crosshaircolor", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
ConVar cl_dynamiccrosshair( "cl_dynamiccrosshair", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
ConVar cl_scalecrosshair( "cl_scalecrosshair", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
ConVar cl_crosshairalpha( "cl_crosshairalpha", "200", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );

int g_iScopeTextureID = 0;
int g_iScopeDustTextureID = 0;

#endif

ConVar tf_weapon_criticals( "tf_weapon_criticals", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Whether or not random crits are enabled" );

//=============================================================================
//
// TFWeaponBase shared functions.
//

// -----------------------------------------------------------------------------
// Purpose: Constructor.
// -----------------------------------------------------------------------------
CTFWeaponBase::CTFWeaponBase()
{
	SetPredictionEligible( true );

	// Nothing collides with these, but they get touch calls.
	AddSolidFlags( FSOLID_TRIGGER );

	// Weapons can fire underwater.
	m_bFiresUnderwater = true;
	m_bAltFiresUnderwater = true;

	// Initialize the weapon modes.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_iReloadMode.Set( TF_RELOAD_START );

	m_iAltFireHint = 0;
	m_bInAttack = false;
	m_bInAttack2 = false;
	m_flCritTime = 0;
	m_flLastCritCheckTime = 0;
	m_flLastRapidFireCritCheckTime = 0;
	m_iLastCritCheckFrame = 0;
	m_flObservedCritChance = 0.f;
	m_flLastFireTime = 0;
	m_flEffectBarRegenTime = 0;
	m_bCurrentAttackIsCrit = false;
	m_bCurrentCritIsRandom = false;
	m_bCurrentAttackIsDuringDemoCharge = false;
	m_iCurrentSeed = -1;
	m_flReloadPriorNextFire = 0;
	m_flLastDeployTime = 0;

	m_bDisguiseWeapon = false;

	m_flEnergy = Energy_GetMaxEnergy();

	m_iAmmoToAdd = 0;

#ifdef GAME_DLL
	m_iHitsInTime = 0;
	m_iProjectilesFiredInTime = 0;
	m_iConsecutiveKills = 0;
	m_iKillStreak = 0;
	m_flClipScale = 1.f;
#endif // GAME_DLL
	m_iConsecutiveShots = 0;
	
#ifdef CLIENT_DLL
	m_iCachedModelIndex = 0;
	m_iEjectBrassAttachpoint = -2;

	m_bInitViewmodelOffset = false;
	m_vecViewmodelOffset = vec3_origin;
#endif // CLIENT_DLL

	m_bBeingRepurposedForTaunt = false;

	m_nKillComboClass = 0;
	ClearKillComboCount();

	m_flLastPrimaryAttackTime = 0.f;
	m_eStrangeType = STRANGE_UNKNOWN;
	m_eStatTrakModuleType = MODULE_UNKNOWN;

	m_flInspectAnimEndTime = -1.f;
	m_nInspectStage = INSPECT_INVALID;
}

CTFWeaponBase::~CTFWeaponBase()
{
#ifdef CLIENT_DLL
	RemoveWorldmodelStatTrak();
	RemoveViewmodelStatTrak();
#endif
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::Spawn()
{
	// Called manually, because CBaseCombatWeapon::Spawn doesn't call back.
	InitializeAttributes();

	m_bBeingRepurposedForTaunt = false;
	m_nKillComboClass = 0;
	ClearKillComboCount();

	// Base class spawn.
	BaseClass::Spawn();

	// Set this here to allow players to shoot dropped weapons.
	SetCollisionGroup( COLLISION_GROUP_WEAPON );

	// Get the weapon information.
	WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( GetClassname() );
	Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );
	CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
	Assert( pWeaponInfo && "Failed to get CTFWeaponInfo in weapon spawn" );
	m_pWeaponInfo = pWeaponInfo;

	if ( GetPlayerOwner() )
	{
		ChangeTeam( GetPlayerOwner()->GetTeamNumber() );
	}

#ifdef GAME_DLL
	// Move it up a little bit, otherwise it'll be at the guy's feet, and its sound origin 
	// will be in the ground so its EmitSound calls won't do anything.
	Vector vecOrigin = GetAbsOrigin();
	SetAbsOrigin( Vector( vecOrigin.x, vecOrigin.y, vecOrigin.z + 5.0f ) );

	m_flRegenTime = 0.0f;

	m_hLastDrainVictim = NULL;
	m_lastDrainVictimTimer.Invalidate();
#endif

	m_szTracerName[0] = '\0';

	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem )
	{
		CEconItemDefinition* pData = pItem->GetStaticData();
		if ( pData && pData->GetSubType() )
		{
			SetSubType( pData->GetSubType() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::Activate( void )
{
	BaseClass::Activate();

	// Reset our clip, in case we've had it modified
	GiveDefaultAmmo();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::FallInit( void )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFWeaponBase::Precache()
{
	BaseClass::Precache();

	if ( GetMuzzleFlashModel() )
	{
		PrecacheModel( GetMuzzleFlashModel() );
	}

	const CTFWeaponInfo *pTFInfo = &GetTFWpnData();

	if ( pTFInfo->m_szExplosionSound[0] )
	{
		CBaseEntity::PrecacheScriptSound( pTFInfo->m_szExplosionSound );
	}

	if ( pTFInfo->m_szBrassModel[0] )
	{
		PrecacheModel( pTFInfo->m_szBrassModel );
	}

	if ( GetMuzzleFlashParticleEffect() )
	{
		PrecacheParticleSystem( GetMuzzleFlashParticleEffect() );
	}

	if ( pTFInfo->m_szExplosionEffect[0] )
	{
		PrecacheParticleSystem( pTFInfo->m_szExplosionEffect );
	}

	if ( pTFInfo->m_szExplosionPlayerEffect[0] )
	{
		PrecacheParticleSystem( pTFInfo->m_szExplosionPlayerEffect );
	}

	if ( pTFInfo->m_szExplosionWaterEffect[0] )
	{
		PrecacheParticleSystem( pTFInfo->m_szExplosionWaterEffect );
	}

	const char *pszTracerEffect = pTFInfo->m_szTracerEffect;
	const CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem->IsValid() )
	{
		const char *pszItemTracerEffect = pItem->GetStaticData()->GetTracerEffect( GetTeamNumber() );
		if ( pszItemTracerEffect )
		{
			pszTracerEffect = pszItemTracerEffect;
		}
	}
	if ( pszTracerEffect && pszTracerEffect[0] )
	{
		char pTracerEffect[128];
		char pTracerEffectCrit[128];

		Q_snprintf( pTracerEffect, sizeof(pTracerEffect), "%s_red", pszTracerEffect );
		Q_snprintf( pTracerEffectCrit, sizeof(pTracerEffectCrit), "%s_red_crit", pszTracerEffect );
		PrecacheParticleSystem( pTracerEffect );
		PrecacheParticleSystem( pTracerEffectCrit );

		Q_snprintf( pTracerEffect, sizeof(pTracerEffect), "%s_blue", pszTracerEffect );
		Q_snprintf( pTracerEffectCrit, sizeof(pTracerEffectCrit), "%s_blue_crit", pszTracerEffect );
		PrecacheParticleSystem( pTracerEffect );
		PrecacheParticleSystem( pTracerEffectCrit );
	}

	if ( TFGameRules() && TFGameRules()->GameModeUsesUpgrades() )
	{
		CBaseEntity::PrecacheScriptSound( "Weapon_Upgrade.DamageBonus1" );
		CBaseEntity::PrecacheScriptSound( "Weapon_Upgrade.DamageBonus2" );
		CBaseEntity::PrecacheScriptSound( "Weapon_Upgrade.DamageBonus3" );
		CBaseEntity::PrecacheScriptSound( "Weapon_Upgrade.DamageBonus4" );
	}

	PrecacheModel( "models/weapons/c_models/stattrack.mdl" );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const CTFWeaponInfo &CTFWeaponBase::GetTFWpnData() const
{
	const FileWeaponInfo_t *pWeaponInfo = &GetWpnData();
	const CTFWeaponInfo *pTFInfo = dynamic_cast< const CTFWeaponInfo* >( pWeaponInfo );
	Assert( pTFInfo );
	return *pTFInfo;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
int CTFWeaponBase::GetWeaponID( void ) const
{
	Assert( false ); 
	return TF_WEAPON_NONE; 
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBase::IsWeapon( int iWeapon ) const
{ 
	return GetWeaponID() == iWeapon; 
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
int	CTFWeaponBase::GetMaxClip1( void ) const
{
	if ( IsEnergyWeapon() )
	{
		return Energy_GetMaxEnergy();
	}

	// Handle the itemdef mod first...
	float flClip = BaseClass::GetMaxClip1();
	if ( flClip >= 0 )
	{
		CALL_ATTRIB_HOOK_INT( flClip, mult_clipsize );
	}

	// Now handle in-game sources, otherwise we get weird numbers on things like the FAN
	if ( flClip >= 0 )
	{
#ifdef GAME_DLL
		flClip *= m_flClipScale;
#endif

		CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
		if ( pPlayer )
		{
			// Blast weps (low clip counts)
			if ( IsBlastImpactWeapon() )
			{
				// MvM-specific upgrade attribute that handles rocket and grenade launchers
				int nProjectiles = 0;
				CALL_ATTRIB_HOOK_INT( nProjectiles, mult_clipsize_upgrade_atomic );

				// Clipsize increase on kills
				int iClipSizeOnKills = 0;
				CALL_ATTRIB_HOOK_INT( iClipSizeOnKills, clipsize_increase_on_kill );
				if ( iClipSizeOnKills )
				{
					nProjectiles += Min( pPlayer->m_Shared.GetDecapitations(), iClipSizeOnKills );		// max extra projectiles
				}

				if ( pPlayer->m_Shared.GetCarryingRuneType() == RUNE_HASTE )
				{
					flClip *= 2;
				}
				if ( !pPlayer->m_Shared.InCond( TF_COND_POWERUPMODE_DOMINANT ) && ( pPlayer->m_Shared.GetCarryingRuneType() == RUNE_PRECISION || pPlayer->m_Shared.GetCarryingRuneType() == RUNE_VAMPIRE ) )
				{
					flClip *= 1.5f;
				}

				return ( flClip + nProjectiles );
			}
			else
			{
				CALL_ATTRIB_HOOK_INT( flClip, mult_clipsize_upgrade );

				if ( pPlayer->m_Shared.GetCarryingRuneType() == RUNE_HASTE )
				{
					flClip *= 2;
				}
			}
		}
	}

	return flClip;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
int	CTFWeaponBase::GetDefaultClip1( void ) const
{
	int iDefault = GetWpnData().iDefaultClip1;
	return ( iDefault == 0 ) ? 0 : GetMaxClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::UsesPrimaryAmmo( void )
{
	if ( IsEnergyWeapon() )
		return false;
	else
		return CBaseCombatWeapon::UsesPrimaryAmmo();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const char *CTFWeaponBase::GetViewModel( int iViewModel ) const
{
	if ( GetPlayerOwner() == NULL )
		return BaseClass::GetViewModel();

	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );

	int iHandModelIndex = 0;
	if ( pPlayer )
	{
		//CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer, iHandModelIndex, override_hand_model_index );		// this is a cleaner way of doing it, but...
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer, iHandModelIndex, wrench_builds_minisentry );			// ...the gunslinger is the only thing that uses this attribute for now
	}

	const CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pPlayer && pItem->IsValid() && pItem->GetStaticData()->ShouldAttachToHands() )
	{
		// Should always be valid, because players without classes shouldn't be carrying items
		const char *pszHandModel = pPlayer->GetPlayerClass()->GetHandModelName( iHandModelIndex );
		Assert( pszHandModel );

		return pszHandModel;
	}

	return GetTFWpnData().szViewModel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFWeaponBase::GetWorldModel( void ) const
{
	const CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem->IsValid() )
	{
		if ( pItem->GetWorldDisplayModel() )
			return pItem->GetWorldDisplayModel();

		int iClass = 0;
		int iTeam = 0;
		CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
		if ( pPlayer )
		{
			iClass = pPlayer->GetPlayerClass()->GetClassIndex();
			iTeam = pPlayer->GetTeamNumber();
		}

		return pItem->GetPlayerDisplayModel( iClass, iTeam );
	}
	return BaseClass::GetWorldModel();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::IsInspectActivity( int iActivity )
{
	return iActivity == GetInspectActivity( INSPECT_START ) || iActivity == GetInspectActivity( INSPECT_IDLE ) || iActivity == GetInspectActivity( INSPECT_END );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Activity CTFWeaponBase::ActivityOverride( Activity baseAct, bool *pRequired )
{
	Activity iAct = BaseClass::ActivityOverride( baseAct, pRequired );

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return iAct;

	CUtlVector< CTFWeaponBase* > vecPassiveWeapons;
	if ( pPlayer->GetPassiveWeapons( vecPassiveWeapons ) )
	{
		// override with the first passive weapon that wants to override the baseAct
		FOR_EACH_VEC( vecPassiveWeapons, i )
		{
			CTFWeaponBase *pWpn = vecPassiveWeapons[i];
			if ( pWpn == this )
				continue;

			Activity iPassiveAct = pWpn->ActivityOverride( baseAct, pRequired );
			if ( iPassiveAct != baseAct )
			{
				return iPassiveAct;
			}
		}
	}

	return iAct;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
poseparamtable_t *CTFWeaponBase::GetPlayerPoseParamList( int &iPoseParamCount )
{
	const CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( GetOwnerEntity() && pItem && pItem->IsValid() )
	{
		int iTeam = GetOwnerEntity()->GetTeamNumber();
		iPoseParamCount = pItem->GetStaticData()->GetNumPlayerPoseParameters( iTeam );
		return pItem->GetStaticData()->GetPlayerPoseParameters( iTeam, 0 );
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
poseparamtable_t *CTFWeaponBase::GetItemPoseParamList( int &iPoseParamCount )
{
	const CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( GetOwnerEntity() && pItem && pItem->IsValid() )
	{
		int iTeam = GetOwnerEntity()->GetTeamNumber();
		iPoseParamCount = pItem->GetStaticData()->GetNumItemPoseParameters( iTeam );
		return pItem->GetStaticData()->GetItemPoseParameters( iTeam, 0 );
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::SendWeaponAnim( int iActivity )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return BaseClass::SendWeaponAnim( iActivity );

	if ( m_nInspectStage != INSPECT_INVALID )
	{
		if ( iActivity == GetActivity() )
			return true;

		// ignore idle anim while inspect anim is still playing
		if ( iActivity == ACT_VM_IDLE )	
		{
			return true;
		}

		// allow other activity to override the inspect
		if ( !IsInspectActivity( iActivity ) )
		{
			m_flInspectAnimEndTime = -1.f;
			m_nInspectStage = INSPECT_INVALID;
			return BaseClass::SendWeaponAnim( iActivity );
		}

		// let the idle loop while the inspect key is pressed
		if ( pPlayer->IsInspecting() && m_nInspectStage == INSPECT_IDLE )
			return true;
	}

	return BaseClass::SendWeaponAnim( iActivity );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::Equip( CBaseCombatCharacter *pOwner )
{
	SetOwner( pOwner );
	SetOwnerEntity( pOwner );
	ReapplyProvision();

	BaseClass::Equip( pOwner );

	// If we attach to our hands, we need to update our viewmodel when we get a new owner.
	UpdateHands();

	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem->IsValid() )
	{
		m_bFlipViewModel = pItem->GetStaticData()->ShouldFlipViewmodels();

		// Also precache the vision filtered display models here.
		if ( pItem->GetVisionFilteredDisplayModel() )
		{
			if ( modelinfo->GetModelIndex( pItem->GetVisionFilteredDisplayModel() ) == -1 ) 
			{
				tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s - Vision Filtered Display Model Late Precache", __FUNCTION__ );
				CBaseEntity::PrecacheModel( pItem->GetVisionFilteredDisplayModel() );
			}
		}

#ifdef GAME_DLL
		UpdateExtraWearables();
		CTFPlayer *pTFPlayer = ToTFPlayer( pOwner );
		if ( pTFPlayer )
		{
			pTFPlayer->ReapplyItemUpgrades(pItem);
		}
#endif // GAME_DLL
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::UpdateHands( void )
{
	const CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem->IsValid() && pItem->GetStaticData()->ShouldAttachToHands() )
	{
		m_iViewModelIndex = CBaseEntity::PrecacheModel( GetViewModel() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::EnableAttack()
{
	static CSchemaAttributeDefHandle pAttrDef_NoAttack( "no_attack" );
	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem->IsValid() )
	{
		pItem->GetAttributeList()->SetRuntimeAttributeValue( pAttrDef_NoAttack, 0.f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::DisableAttack()
{
	static CSchemaAttributeDefHandle pAttrDef_NoAttack( "no_attack" );
	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem->IsValid() )
	{
		pItem->GetAttributeList()->SetRuntimeAttributeValue( pAttrDef_NoAttack, 1.f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::EnableJump()
{
	static CSchemaAttributeDefHandle pAttrDef_NoJump( "no_jump" );
	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem->IsValid() )
	{
		pItem->GetAttributeList()->SetRuntimeAttributeValue( pAttrDef_NoJump, 0.f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::DisableJump()
{
	static CSchemaAttributeDefHandle pAttrDef_NoJump( "no_jump" );
	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem->IsValid() )
	{
		pItem->GetAttributeList()->SetRuntimeAttributeValue( pAttrDef_NoJump, 1.f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::EnableDuck()
{
	static CSchemaAttributeDefHandle pAttrDef_NoDuck( "no_duck" );
	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem->IsValid() )
	{
		pItem->GetAttributeList()->SetRuntimeAttributeValue( pAttrDef_NoDuck, 0.f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::DisableDuck()
{
	static CSchemaAttributeDefHandle pAttrDef_NoDuck( "no_duck" );
	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem->IsValid() )
	{
		pItem->GetAttributeList()->SetRuntimeAttributeValue( pAttrDef_NoDuck, 1.f );
	}
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::UpdateExtraWearables()
{
	CTFWearable *pOldWearable = m_hExtraWearable.Get();
	CTFWearable *pOldWearableVM = m_hExtraWearableViewModel.Get();

	if ( pOldWearable || pOldWearableVM )
	{
		CBaseCombatCharacter *pOwner = GetOwner();
		if ( pOwner )
		{
			if ( !( pOldWearable && pOldWearable->GetTeamNumber() != pOwner->GetTeamNumber() ) &&
				 !( pOldWearableVM && pOldWearableVM->GetTeamNumber() != pOwner->GetTeamNumber() ) )
			{
				// No need to destroy and recreate them, because they already match the owner's team
				return;
			}
		}

		RemoveExtraWearables();
	}

	bool bHasViewModel = false;

	CEconItemView *pEconItemView = GetAttributeContainer()->GetItem();
	if ( pEconItemView->GetExtraWearableViewModel() )
	{
		CTFWearable* pExtraWearableItem = dynamic_cast<CTFWearable*>( CreateEntityByName( "tf_wearable_vm" ) );
		if ( pExtraWearableItem )
		{
			if ( modelinfo->GetModelIndex( pEconItemView->GetExtraWearableViewModel() ) == -1 ) {
				tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s - View Model Late Precache", __FUNCTION__ );
				// Precaching may be needed here, because we allow virtually everything to be loaded on demand now.
				pExtraWearableItem->PrecacheModel( pEconItemView->GetExtraWearableViewModel() );
			}

			pExtraWearableItem->AddSpawnFlags( SF_NORESPAWN );
			pExtraWearableItem->SetAlwaysAllow( true );
			DispatchSpawn( pExtraWearableItem );
			pExtraWearableItem->GiveTo( GetOwner() );
			pExtraWearableItem->SetModel( pEconItemView->GetExtraWearableViewModel() );

			bHasViewModel = true;
			pExtraWearableItem->SetWeaponAssociatedWith( this );

			ExtraWearableViewModelEquipped( pExtraWearableItem );
		}
	}

	if ( pEconItemView->GetExtraWearableModel() )
	{
		CTFWearable* pExtraWearableItem = dynamic_cast<CTFWearable*>( CreateEntityByName( "tf_wearable" ) );
		if ( pExtraWearableItem )
		{
			if ( modelinfo->GetModelIndex( pEconItemView->GetExtraWearableModel() ) == -1 ) {
				tmZone(TELEMETRY_LEVEL0, TMZF_NONE, "%s - Model Late Precache", __FUNCTION__);

				// Precaching may be needed here, because we allow virtually everything to be loaded on demand now.
				pExtraWearableItem->PrecacheModel( pEconItemView->GetExtraWearableModel() );
			}

			pExtraWearableItem->AddSpawnFlags( SF_NORESPAWN );
			pExtraWearableItem->SetAlwaysAllow( true );
			DispatchSpawn( pExtraWearableItem );
			pExtraWearableItem->GiveTo( GetOwner() );
			pExtraWearableItem->SetModel( pEconItemView->GetExtraWearableModel() );

			if ( bHasViewModel )
			{
				// If it has a view model we need to have the weapon control visibility of this wearable
				pExtraWearableItem->SetWeaponAssociatedWith( this );
			}

			ExtraWearableEquipped( pExtraWearableItem );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::ExtraWearableEquipped( CTFWearable *pExtraWearableItem )
{
	Assert( m_hExtraWearable == NULL );
	Assert( pExtraWearableItem != NULL );

	m_hExtraWearable.Set( pExtraWearableItem );

	CBasePlayer *pPlayerOwner = dynamic_cast<CBasePlayer *>( GetOwner() );
	if ( pPlayerOwner )
	{
		pExtraWearableItem->Equip( pPlayerOwner );
	}
}

void CTFWeaponBase::ExtraWearableViewModelEquipped( CTFWearable *pExtraWearableItem )
{
	Assert( m_hExtraWearableViewModel == NULL );
	Assert( pExtraWearableItem != NULL );

	m_hExtraWearableViewModel.Set( pExtraWearableItem );

	CBasePlayer *pPlayerOwner = dynamic_cast<CBasePlayer *>( GetOwner() );
	if ( pPlayerOwner )
	{
		pExtraWearableItem->Equip( pPlayerOwner );
	}
}
#else
void CTFWeaponBase::UpdateExtraWearablesVisibility()
{
	if ( m_hExtraWearable.Get() )
	{
		m_hExtraWearable->ValidateModelIndex();
		m_hExtraWearable->UpdateVisibility();
		m_hExtraWearable->CreateShadow();
	}

	if ( m_hExtraWearableViewModel.Get() )
	{
		m_hExtraWearableViewModel->UpdateVisibility();
	}

	if ( m_viewmodelStatTrakAddon.Get() )
	{
		m_viewmodelStatTrakAddon->UpdateVisibility();
	}

	if ( m_worldmodelStatTrakAddon.Get() )
	{
		m_worldmodelStatTrakAddon->UpdateVisibility();
		m_worldmodelStatTrakAddon->CreateShadow();
	}
}
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::RemoveExtraWearables( void )
{
	if ( m_hExtraWearable )
	{
		m_hExtraWearable->RemoveFrom( GetOwnerEntity() );
		m_hExtraWearable = NULL;
	}

	if ( m_hExtraWearableViewModel )
	{
		m_hExtraWearableViewModel->RemoveFrom( GetOwnerEntity() );
		m_hExtraWearableViewModel = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::Drop( const Vector &vecVelocity )
{
#ifndef CLIENT_DLL
	if ( m_iAltFireHint )
	{
		CBasePlayer *pPlayer = GetPlayerOwner();
		if ( pPlayer )
		{
			pPlayer->StopHintTimer( m_iAltFireHint );
		}
	}
#endif

	BaseClass::Drop( vecVelocity );

	ReapplyProvision();

	RemoveExtraWearables();

#ifndef CLIENT_DLL
	// Never allow weapons to lie around on the ground
	UTIL_Remove( this );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::UpdateOnRemove( void )
{
	RemoveExtraWearables();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::CanHolster( void ) const
{
	// Honorbound weapons are unable to be holstered until they have killed someone
	// since the last time they were brought out. We ignore this logic for the first
	// block of time after a weapon is taken out to allow quickswitching.
	// only check the first block of time logic if the weapon is active weapon
	// Can always holster if you have enough life cause we'll take that away
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( pPlayer && ( pPlayer->GetActiveWeapon() != this || gpGlobals->curtime >= pPlayer->m_Shared.m_flFirstPrimaryAttack ) )
	{
		if ( IsHonorBound() && pPlayer->m_Shared.m_iKillCountSinceLastDeploy == 0 && pPlayer->GetHealth() <= 50 )
		{
#ifdef CLIENT_DLL
			pPlayer->EmitSound( "Player.DenyWeaponSelection" );
#endif
			return false;
		}
	}

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::StartHolsterAnim( void )
{
	Holster();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::Holster( CBaseCombatWeapon *pSwitchingTo )
{
#ifndef CLIENT_DLL
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( pPlayer && m_iAltFireHint )
	{
		pPlayer->StopHintTimer( m_iAltFireHint );
	}

	// Honorbound hurt yourself
	if ( pPlayer && ( pPlayer->GetActiveWeapon() != this || gpGlobals->curtime >= pPlayer->m_Shared.m_flFirstPrimaryAttack ) )
	{
		if ( IsHonorBound() && pPlayer->m_Shared.m_iKillCountSinceLastDeploy == 0 && pPlayer->GetHealth() > 0 && pPlayer->IsAlive() )
		{
			pPlayer->TakeDamage( CTakeDamageInfo( pPlayer, pPlayer, vec3_origin, pPlayer->WorldSpaceCenter(), 50.f, GetDamageType() | DMG_PREVENT_PHYSICS_FORCE ) );
		}
	}
#endif

	m_iReloadMode.Set( TF_RELOAD_START );

#ifdef GAME_DLL
	m_iHitsInTime = 0;
	m_iProjectilesFiredInTime = 0;
#endif // GAME_DLL
	m_iConsecutiveShots = 0;

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::Deploy( void )
{
#ifndef CLIENT_DLL
	if ( m_iAltFireHint )
	{
		CBasePlayer *pPlayer = GetPlayerOwner();
		if ( pPlayer )
		{
			pPlayer->StartHintTimer( m_iAltFireHint );
		}
	}
#endif

	m_iReloadMode.Set( TF_RELOAD_START );

	float flOriginalPrimaryAttack = m_flNextPrimaryAttack;
	float flOriginalSecondaryAttack = m_flNextSecondaryAttack;

	bool bDeploy = BaseClass::Deploy();

	if ( bDeploy )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
		if ( !pPlayer )
			return false;

		float flWeaponSwitchTime = 0.5f;

		// Overrides the anim length for calculating ready time.
		float flDeployTimeMultiplier = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer, flDeployTimeMultiplier, mult_deploy_time );
		CALL_ATTRIB_HOOK_FLOAT( flDeployTimeMultiplier, mult_single_wep_deploy_time );

		// don't apply mult_switch_from_wep_deploy_time attribute if the last weapon hasn't been deployed for more than 0.67 second to match to weapon script switch time
		// unless the player latched to a hook target, then allow switching right away
		CTFWeaponBase *pLastWeapon = dynamic_cast< CTFWeaponBase* >( pPlayer->GetLastWeapon() );
		bool bPowerupModeKnife = TFGameRules() && TFGameRules()->IsPowerupMode() && ( GetWeaponID() == TF_WEAPON_KNIFE );
		if ( pPlayer->GetGrapplingHookTarget() != NULL || ( pLastWeapon && gpGlobals->curtime - pLastWeapon->m_flLastDeployTime > flWeaponSwitchTime ) )
		{
			if ( !bPowerupModeKnife )
			{
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pLastWeapon, flDeployTimeMultiplier, mult_switch_from_wep_deploy_time );
			}
		}
		
		if ( pPlayer->m_Shared.InCond( TF_COND_BLASTJUMPING ) )
		{
			CALL_ATTRIB_HOOK_FLOAT( flDeployTimeMultiplier, mult_rocketjump_deploy_time );
		}

		int iIsSword = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pLastWeapon, iIsSword, is_a_sword );
		CALL_ATTRIB_HOOK_INT( iIsSword, is_a_sword );
		if ( iIsSword )
		{
			// swords deploy and holster 75% slower
			flDeployTimeMultiplier *= 1.75f;
		}


		if ( pPlayer->m_Shared.GetCarryingRuneType() == RUNE_AGILITY && !bPowerupModeKnife )
		{
			flDeployTimeMultiplier /= 5.0f;
		}

		int numHealers = pPlayer->m_Shared.GetNumHealers();
		if ( numHealers == 0 )
		{
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer, flDeployTimeMultiplier, mod_medic_healed_deploy_time );
		}
		
		flDeployTimeMultiplier = MAX( flDeployTimeMultiplier, 0.00001f );
		float flDeployTime = flWeaponSwitchTime * flDeployTimeMultiplier;
		float flPlaybackRate = Clamp( ( 1.f / flDeployTimeMultiplier ) * ( 0.67f / flWeaponSwitchTime ), -4.f, 12.f ); // clamp between the range that's defined in send table
		if ( pPlayer->GetViewModel(0) )
		{
			pPlayer->GetViewModel(0)->SetPlaybackRate( flPlaybackRate );
		}
		if ( pPlayer->GetViewModel(1) )
		{
			pPlayer->GetViewModel(1)->SetPlaybackRate( flPlaybackRate );
		}
		
		// Don't override primary attacks that are already further out than this. This prevents
		// people exploiting weapon switches to allow weapons to fire faster.
		m_flNextPrimaryAttack = MAX( flOriginalPrimaryAttack, gpGlobals->curtime + flDeployTime );
		m_flNextSecondaryAttack = MAX( flOriginalSecondaryAttack, m_flNextPrimaryAttack.Get() );

		pPlayer->SetNextAttack( m_flNextPrimaryAttack );

		m_flLastDeployTime = gpGlobals->curtime;

#ifdef GAME_DLL
		// Reset our deploy-lifetime kill counter.
		pPlayer->m_Shared.m_iKillCountSinceLastDeploy = 0;
		pPlayer->m_Shared.m_flFirstPrimaryAttack = m_flNextPrimaryAttack;
#endif // GAME_DLL

	}

	return bDeploy;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFWeaponBase::ForceWeaponSwitch() const
{
	// allow knockout rune to force switch to melee
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner && pOwner->m_Shared.GetCarryingRuneType() == RUNE_KNOCKOUT )
	{
		int iClass = pOwner->GetPlayerClass()->GetClassIndex();
		const CEconItemView *pItem = GetAttributeContainer()->GetItem();
		if ( pItem && pItem->GetStaticData()->GetLoadoutSlot( iClass ) == LOADOUT_POSITION_MELEE )
		{
			return true;
		}
	}

	// should force switch to this item
	int iForceWeaponSwitch = 0;
	CALL_ATTRIB_HOOK_INT( iForceWeaponSwitch, force_weapon_switch );
	return iForceWeaponSwitch != 0;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBase::Detach( void )
{
	BaseClass::Detach();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBase::OnActiveStateChanged( int iOldState )
{
	UpdateHiddenParentBodygroup( m_iState == WEAPON_IS_ACTIVE );

	// See if we need to reapply our provider based on our active state.
	int iProvideMode = 0;
	CALL_ATTRIB_HOOK_INT( iProvideMode, provide_on_active );
	if ( 1 == iProvideMode )
	{
		ReapplyProvision();
	}

	// Check for a speed mod change.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( pPlayer )
	{
		pPlayer->TeamFortress_SetSpeed();
	}

	CEconItemView *pScriptItem = GetAttributeContainer()->GetItem();
	if ( pScriptItem && pScriptItem->GetStaticData()->GetHideBodyGroupsDeployedOnly() )
	{
#ifdef CLIENT_DLL
		if ( pPlayer )
		{
			pPlayer->SetBodygroupsDirty();
		}
#else
		int iState = 0;
		if ( WeaponState() == WEAPON_IS_ACTIVE )
		{
			iState = 1;
		}

		if ( pPlayer )
		{
			UpdateBodygroups( pPlayer, iState );
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFWeaponBase::VisibleInWeaponSelection( void )
{
	if ( BaseClass::VisibleInWeaponSelection() == false )
	{
		return false;
	}
	if ( TFGameRules()->IsInTraining() )
	{
		ConVarRef training_can_select_weapon_primary	( "training_can_select_weapon_primary" );
		ConVarRef training_can_select_weapon_secondary	( "training_can_select_weapon_secondary" );
		ConVarRef training_can_select_weapon_melee		( "training_can_select_weapon_melee" );
		ConVarRef training_can_select_weapon_building	( "training_can_select_weapon_building" );
		ConVarRef training_can_select_weapon_pda		( "training_can_select_weapon_pda" );
		ConVarRef training_can_select_weapon_item1		( "training_can_select_weapon_item1" );
		ConVarRef training_can_select_weapon_item2		( "training_can_select_weapon_item2" );
		bool bVisible = true;
		switch ( GetTFWpnData().m_iWeaponType )
		{
		case TF_WPN_TYPE_PRIMARY:	bVisible = training_can_select_weapon_primary.GetBool(); break;
		case TF_WPN_TYPE_SECONDARY:	bVisible = training_can_select_weapon_secondary.GetBool(); break;
		case TF_WPN_TYPE_MELEE:		bVisible = training_can_select_weapon_melee.GetBool(); break;
		case TF_WPN_TYPE_BUILDING:	bVisible = training_can_select_weapon_building.GetBool(); break;
		case TF_WPN_TYPE_PDA:		bVisible = training_can_select_weapon_pda.GetBool(); break;
		case TF_WPN_TYPE_ITEM1:		bVisible = training_can_select_weapon_item1.GetBool(); break;
		case TF_WPN_TYPE_ITEM2:		bVisible = training_can_select_weapon_item2.GetBool(); break;
		} // switch
		return bVisible;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBase::UpdateHiddenParentBodygroup( bool bHide )
{
#ifdef GAME_DLL
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if (!pPlayer)
		return;

	const CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem->IsValid() )
	{
		// Old style hidden bodygroups (weapon only):
		int iHiddenBG = pItem->GetStaticData()->GetHiddenParentBodygroup( GetTeamNumber() );
		if ( iHiddenBG != -1 )
		{
			pPlayer->SetBodygroup( iHiddenBG, bHide );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::Misfire( void )
{
	CalcIsAttackCritical();
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::FireFullClipAtOnce( void )
{
	AssertMsg( 0, "weapon that has AutoFiresFullClipAllAtOnce should implement this function" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
void CTFWeaponBase::PrimaryAttack( void )
{
	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	if ( !CanAttack() )
		return;

	BaseClass::PrimaryAttack();

	if ( m_bReloadsSingly )
	{
		m_iReloadMode.Set( TF_RELOAD_START );
	}

	m_flLastPrimaryAttackTime = gpGlobals->curtime;

	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( pPlayer )
	{

		pPlayer->m_Shared.OnAttack();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CTFWeaponBase::SecondaryAttack( void )
{
	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;


	// Don't hook secondary for now.
	return;
}

//-----------------------------------------------------------------------------
// Purpose: Most calls use the prediction seed
//-----------------------------------------------------------------------------
void CTFWeaponBase::CalcIsAttackCritical( void)
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( gpGlobals->framecount == m_iLastCritCheckFrame )
		return;
	m_iLastCritCheckFrame = gpGlobals->framecount;

	m_bCurrentCritIsRandom = false;

#if !defined( CLIENT_DLL )
	// in training mode, the all bot team does not get crits
	if ( TFGameRules()->IsInTraining() )
	{
		if ( pPlayer->IsBot() && TheTFBots().IsAllBotTeam( pPlayer->GetTeamNumber() ) )
		{
			// Support critboosted even in no crit mode
			m_bCurrentAttackIsCrit = CalcIsAttackCriticalHelperNoCrits();
			return;
		}
	}

	if ( TFGameRules()->IsPVEModeActive() && TFGameRules()->IsPVEModeControlled( pPlayer ) )
	{
		// no crits for enemies in PvE

		// Support critboosted even in no crit mode
		m_bCurrentAttackIsCrit = CalcIsAttackCriticalHelperNoCrits();
		return;
	}
#endif

	if ( (TFGameRules()->State_Get() == GR_STATE_TEAM_WIN) && (TFGameRules()->GetWinningTeam() == pPlayer->GetTeamNumber()) )
	{
		m_bCurrentAttackIsCrit = true;
	}
	else if ( !AreRandomCritsEnabled() )
	{
		// Support critboosted even in no crit mode
		m_bCurrentAttackIsCrit = CalcIsAttackCriticalHelperNoCrits();
	}
	else
	{
		// call the weapon-specific helper method
		m_bCurrentAttackIsCrit = CalcIsAttackCriticalHelper();
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::CalcIsAttackCriticalHelperNoCrits()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

	return pPlayer->m_Shared.IsCritBoosted();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
ETFDmgCustom CTFWeaponBase::GetPenetrateType() const
{
	int iMode = 0;
	CALL_ATTRIB_HOOK_INT( iMode, projectile_penetration );


	return iMode >= 1
		 ? TF_DMG_CUSTOM_PENETRATE_ALL_PLAYERS
		 : TF_DMG_CUSTOM_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Weapon-specific helper method to calculate if attack is crit
//-----------------------------------------------------------------------------
bool CTFWeaponBase::CalcIsAttackCriticalHelper()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

	float flCritChance = 0.f;
	float flPlayerCritMult = pPlayer->GetCritMult();

	if ( !CanFireCriticalShot() )
		return false;

	// Crit boosted players fire all crits
	if ( pPlayer->m_Shared.IsCritBoosted() )
		return true;

	// For rapid fire weapons, allow crits while period is active
	bool bRapidFire = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_bUseRapidFireCrits;
	if ( bRapidFire && m_flCritTime > gpGlobals->curtime )
		return true;

	// --- Random crits from this point on ---
	
	// Monitor and enforce short-term random crit rate - via bucket

	// Figure out how much to add/remove from token bucket
	int nProjectilesPerShot = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nBulletsPerShot;
	if ( nProjectilesPerShot >= 1 )
	{
		CALL_ATTRIB_HOOK_FLOAT( nProjectilesPerShot, mult_bullets_per_shot );
	}
	else
	{
		nProjectilesPerShot = 1;
	}
	// Damage
	float flDamage = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nDamage;
	CALL_ATTRIB_HOOK_FLOAT( flDamage, mult_dmg );
	flDamage *= nProjectilesPerShot;
	AddToCritBucket( flDamage );

	bool bCrit = false;
	m_bCurrentCritIsRandom = true;
	int iRandom = 0;

	if ( bRapidFire )
	{
		// only perform one crit check per second for rapid fire weapons
		if ( tf_weapon_criticals_nopred.GetBool() )
		{
			if ( gpGlobals->curtime < m_flLastRapidFireCritCheckTime + 1.f )
				return false;

			m_flLastRapidFireCritCheckTime = gpGlobals->curtime;
		}
		else
		{
			if ( gpGlobals->curtime < m_flLastCritCheckTime + 1.f )
				return false;

			m_flLastCritCheckTime = gpGlobals->curtime;
		}

		// get the total crit chance (ratio of total shots fired we want to be crits)
		float flTotalCritChance = clamp( TF_DAMAGE_CRIT_CHANCE_RAPID * flPlayerCritMult, 0.01f, 0.99f );
		// get the fixed amount of time that we start firing crit shots for	
		float flCritDuration = TF_DAMAGE_CRIT_DURATION_RAPID;
		// calculate the amount of time, on average, that we want to NOT fire crit shots for in order to achieve the total crit chance we want
		float flNonCritDuration = ( flCritDuration / flTotalCritChance ) - flCritDuration;
		// calculate the chance per second of non-crit fire that we should transition into critting such that on average we achieve the total crit chance we want
		float flStartCritChance = 1 / flNonCritDuration;

		CALL_ATTRIB_HOOK_FLOAT( flStartCritChance, mult_crit_chance );

		// if base entity seed has changed since last calculation, reseed with new seed
		int iMask = ( entindex() << 8 ) | ( pPlayer->entindex() );
		int iSeed = CBaseEntity::GetPredictionRandomSeed() ^ iMask;
		if ( iSeed != m_iCurrentSeed )
		{
			m_iCurrentSeed = iSeed;
			RandomSeed( m_iCurrentSeed );
		}

		// see if we should start firing crit shots
		iRandom = RandomInt( 0, WEAPON_RANDOM_RANGE-1 );
		if ( iRandom < flStartCritChance * WEAPON_RANDOM_RANGE )
		{
			bCrit = true;
			flCritChance = flStartCritChance;
		}
	}
	else
	{
		// single-shot weapon, just use random pct per shot
		flCritChance = TF_DAMAGE_CRIT_CHANCE * flPlayerCritMult;
		CALL_ATTRIB_HOOK_FLOAT( flCritChance, mult_crit_chance );

		// mess with the crit chance seed so it's not based solely on the prediction seed
		int iMask = ( entindex() << 8 ) | ( pPlayer->entindex() );
		int iSeed = CBaseEntity::GetPredictionRandomSeed() ^ iMask;
		if ( iSeed != m_iCurrentSeed )
		{
			m_iCurrentSeed = iSeed;
			RandomSeed( m_iCurrentSeed );
		}

		iRandom = RandomInt( 0, WEAPON_RANDOM_RANGE - 1 );
		bCrit = ( iRandom < flCritChance * WEAPON_RANDOM_RANGE );
	}

#ifdef _DEBUG
	if ( tf_weapon_criticals_debug.GetBool() )
	{
#ifdef GAME_DLL
		DevMsg( "Roll (server): %i out of %f (crit: %d)\n", iRandom, ( flCritChance * WEAPON_RANDOM_RANGE ), bCrit );
#else
		if ( prediction->IsFirstTimePredicted() )
		{
			DevMsg( "\tRoll (client): %i out of %f (crit: %d)\n", iRandom, ( flCritChance * WEAPON_RANDOM_RANGE ), bCrit );
		}
#endif // GAME_DLL
	}

	// Force seed to always say yes
	if ( tf_weapon_criticals_force_random.GetInt() )
	{
		bCrit = true;
	}
#endif // _DEBUG
	
	// Track each check
#ifdef GAME_DLL
	m_nCritChecks++;
#else
	if ( prediction->IsFirstTimePredicted() )
	{
		m_nCritChecks++;
	}
#endif // GAME_DLL

	// Seed says crit.  Run it by the manager.
	if ( bCrit )
	{
		bool bAntiCheat = true;
#ifdef _DEBUG
		bAntiCheat = tf_weapon_criticals_anticheat.GetBool();
#endif // _DEBUG

		// Monitor and enforce long-term random crit rate - via stats
		if ( bAntiCheat )
		{
			if ( !CanFireRandomCriticalShot( flCritChance ) )
				return false;

			// Make sure rapid fire weapons can pay the cost of the entire period up-front
			if ( bRapidFire )
			{
				flDamage *= TF_DAMAGE_CRIT_DURATION_RAPID / m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;

				// Never try to drain more than cap
				int nBucketCap = tf_weapon_criticals_bucket_cap.GetInt();
				if ( flDamage * TF_DAMAGE_CRIT_MULTIPLIER > nBucketCap )
					flDamage = (float)nBucketCap / TF_DAMAGE_CRIT_MULTIPLIER;
			}

			bCrit = IsAllowedToWithdrawFromCritBucket( flDamage );
		}

		if ( bCrit && bRapidFire )
		{
			m_flCritTime = gpGlobals->curtime + TF_DAMAGE_CRIT_DURATION_RAPID;
		}
	}

	return bCrit;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this weapon has some ammo
//-----------------------------------------------------------------------------
bool CTFWeaponBase::HasAmmo( void )
{
	if ( IsEnergyWeapon() )
		return true;
	else
		return BaseClass::HasAmmo();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFWeaponBase::Reload( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

#ifdef GAME_DLL
	m_iHitsInTime = 0;
	m_iProjectilesFiredInTime = 0;
#endif // GAME_DLL
	m_iConsecutiveShots = 0;

	if ( IsEnergyWeapon() && !Energy_FullyCharged() )
	{
		return ReloadSingly();
	}

	// If we're not already reloading, check to see if we have ammo to reload and check to see if we are max ammo.
	if ( m_iReloadMode == TF_RELOAD_START ) 
	{
		// If I don't have any spare ammo, I can't reload
		if ( GetOwner()->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
			return false;

		if ( !CanOverload() && Clip1() >= GetMaxClip1() )
			return false;
	}

	// Reload one object at a time.
	if ( m_bReloadsSingly )
		return ReloadSingly();

	// Normal reload.
	DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::AbortReload( void )
{
	BaseClass::AbortReload();

	m_iReloadMode.Set( TF_RELOAD_START );

	// Make sure our reloading bodygroup is hidden (shells/grenades/etc)
	int indexR = FindBodygroupByName( "reload" );
	if ( indexR >= 0 )
	{
		SetBodygroup( indexR, 0 );
	}
}


//-----------------------------------------------------------------------------
// Is the weapon reloading right now?
bool CTFWeaponBase::IsReloading() const
{
	return m_iReloadMode != TF_RELOAD_START;
}

bool CTFWeaponBase::UsesCenterFireProjectile( void ) const
{
	int nCenterFireProjectile = 0;
	CALL_ATTRIB_HOOK_INT( nCenterFireProjectile, centerfire_projectile );

	return ( nCenterFireProjectile != 0 );
}

bool CTFWeaponBase::AutoFiresFullClip( void ) const
{
	int nAutoFiresFullClip = 0;
	CALL_ATTRIB_HOOK_INT( nAutoFiresFullClip, auto_fires_full_clip );

	return ( nAutoFiresFullClip != 0 );
}

bool CTFWeaponBase::AutoFiresFullClipAllAtOnce( void ) const
{
	int nAutoFiresFullClipAllAtOnce = 0;
	CALL_ATTRIB_HOOK_INT( nAutoFiresFullClipAllAtOnce, auto_fires_full_clip_all_at_once );

	return ( nAutoFiresFullClipAllAtOnce != 0 );
}

bool CTFWeaponBase::CanOverload( void ) const
{
	int nCanOverload = 0;
	CALL_ATTRIB_HOOK_INT( nCanOverload, can_overload );

	return ( nCanOverload != 0 );
}

float CTFWeaponBase::ApplyFireDelay( float flDelay ) const
{
	float flDelayMult = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flDelayMult, mult_postfiredelay );

	float flComboBoost = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT( flComboBoost, kill_combo_fire_rate_boost );
	flComboBoost *= GetKillComboCount();

	flDelayMult -= flComboBoost;

	// Haste Powerup Rune adds multiplier to fire delay time. Flare guns get double boost
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer && pPlayer->m_Shared.GetCarryingRuneType() == RUNE_HASTE )
	{
		if ( pPlayer->IsPlayerClass( TF_CLASS_PYRO ) && GetWeaponID() == TF_WEAPON_FLAREGUN )
		{
			flDelayMult *= 0.25f;
		}
		else if ( pPlayer->m_Shared.InCond( TF_COND_POWERUPMODE_DOMINANT ) )
		{
			flDelayMult *= 0.75f;
		}
		else
		{
			flDelayMult *= 0.50f;
		}
	}
	else if ( pPlayer && ( pPlayer->m_Shared.GetCarryingRuneType() == RUNE_KING || pPlayer->m_Shared.InCond( TF_COND_KING_BUFFED ) ) )
	{
		flDelayMult *= 0.75f;
	}

	return flDelay * flDelayMult;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFWeaponBase::ReloadSingly( void )
{
	// Don't reload.
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return false;

	// Get the current player.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

	// Anti Reload Cancelling Exploit (Beggers Bazooka)
	// Force attack if we try to reload when we have ammo in the clip
	if ( AutoFiresFullClip() && Clip1() > 0 && m_iReloadMode == TF_RELOAD_START )
	{
		PrimaryAttack();
		m_bFiringWholeClip = true;

#ifdef CLIENT_DLL
		pPlayer->SetFiredWeapon( true );
#endif
		return false;
	}

	int nAutoFiresWhenFull = 0;
	CALL_ATTRIB_HOOK_INT( nAutoFiresWhenFull, auto_fires_when_full );
	if ( nAutoFiresWhenFull && ( Clip1() == GetMaxClip1() || pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 ) )
	{
		PrimaryAttack();
		m_bFiringWholeClip = true;

#ifdef CLIENT_DLL
		pPlayer->SetFiredWeapon( true );
#endif
		return false;
	}

	// check to see if we're ready to reload
	switch ( m_iReloadMode )
	{
	case TF_RELOAD_START:
		{
			// Play weapon and player animations.
			if ( SendWeaponAnim( ACT_RELOAD_START ) )
			{
				SetReloadTimer( SequenceDuration() );
			}
			else
			{
				// Update the reload timers with script values.
				UpdateReloadTimers( true );
			}

			// Next reload the shells.
			m_iReloadMode.Set( TF_RELOADING );

			m_iReloadStartClipAmount = Clip1();

			return true;
		}
	case TF_RELOADING:
		{
			// Did we finish the reload start?  Now we can reload a rocket.
			if ( m_flTimeWeaponIdle > gpGlobals->curtime )
				return false;

			// Play weapon reload animations and sound.
			if ( Clip1() == m_iReloadStartClipAmount )
			{
				pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
			}
			else
			{
				pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD_LOOP );
			}

			m_bReloadedThroughAnimEvent = false;

			if ( SendWeaponAnim( ACT_VM_RELOAD ) )
			{
				if ( GetWeaponID() == TF_WEAPON_GRENADELAUNCHER )
				{
					SetReloadTimer( GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReload );
				}
				else
				{
					SetReloadTimer( SequenceDuration() );
				}
			}
			else
			{
				// Update the reload timers.
				UpdateReloadTimers( false );
			}

			// Play reload
#ifdef CLIENT_DLL
			if ( ShouldPlayClientReloadSound() )
				WeaponSound( RELOAD );
#else
			WeaponSound( RELOAD );
#endif

			// Next continue to reload shells?
			m_iReloadMode.Set( TF_RELOADING_CONTINUE );

			return true;
		}
	case TF_RELOADING_CONTINUE:
		{
			// Did we finish the reload start?  Now we can finish reloading the rocket.
			if ( m_flTimeWeaponIdle > gpGlobals->curtime )
				return false;

			IncrementAmmo();

			if ( IsEnergyWeapon() )
			{
				if ( Energy_FullyCharged() )
				{
					m_iReloadMode.Set( TF_RELOAD_FINISH );
				}
				else
				{
					m_iReloadMode.Set( TF_RELOADING );
				}
			}
			else
			{
				if ( ( !CanOverload() && ( Clip1() == GetMaxClip1() || pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 ) ) )
				{
					m_iReloadMode.Set( TF_RELOAD_FINISH );
				}
				else
				{
					m_iReloadMode.Set( TF_RELOADING );
				}
			}

			return true;
		}

	case TF_RELOAD_FINISH:
	default:
		{
			if ( SendWeaponAnim( ACT_RELOAD_FINISH ) )
			{
				// We're done, allow primary attack as soon as we like unless we're an energy weapon.
//				if ( IsEnergyWeapon() )
//				{
//					SetReloadTimer( SequenceDuration() );
//				}
			}

			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD_END );

			m_iReloadMode.Set( TF_RELOAD_START );
			return true;
		}
	}
}

void CTFWeaponBase::IncrementAmmo( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	// If we have ammo, remove ammo and add it to clip
	if ( !m_bReloadedThroughAnimEvent )
	{
		if ( IsEnergyWeapon() )
		{
			Energy_Recharge();
		}
		else if ( !CheckReloadMisfire() ) 
		{
			if ( pPlayer && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) > 0 )
			{
				m_iClip1 = MIN( ( m_iClip1 + 1 ), GetMaxClip1() );
				pPlayer->RemoveAmmo( 1, m_iPrimaryAmmoType );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CTFWeaponBase::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	if ( (pEvent->type & AE_TYPE_NEWEVENTSYSTEM) /*&& (pEvent->type & AE_TYPE_SERVER)*/ )
	{
		if ( pEvent->event == AE_WPN_INCREMENTAMMO )
		{
			IncrementAmmo();

			m_bReloadedThroughAnimEvent = true;
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFWeaponBase::GetInventoryModel( void )
{
	// Return the world model when displaying this item in the inventory
	const model_t *pWorldModel = modelinfo->GetModel( m_iWorldModelIndex );
	if ( pWorldModel )
		return modelinfo->GetModelName( pWorldModel );

	return NULL;//BaseClass::GetInventoryModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::NeedsReloadForAmmo1( int iClipSize1 ) const
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( pOwner )
	{
		// If you don't have clips, then don't try to reload them.
		if ( UsesClipsForAmmo1() )
		{
			// need to reload primary clip?
			int primary = MIN( iClipSize1 - m_iClip1, pOwner->GetAmmoCount( m_iPrimaryAmmoType ) );
			if ( primary != 0 )
				return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::NeedsReloadForAmmo2( int iClipSize2 ) const
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( pOwner )
	{
		if ( UsesClipsForAmmo2() )
		{
			// need to reload secondary clip?
			int secondary = MIN( iClipSize2 - m_iClip2, pOwner->GetAmmoCount( m_iSecondaryAmmoType ) );
			if ( secondary != 0 )
				return true;
		}
	}

	return false;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBase::DefaultReload( int iClipSize1, int iClipSize2, int iActivity )
{
	// The the owning local player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	// Setup and check for reload.
	bool bReloadPrimary = NeedsReloadForAmmo1( iClipSize1 );
	bool bReloadSecondary = NeedsReloadForAmmo2( iClipSize2 );

	// We didn't reload.
	if ( !( bReloadPrimary || bReloadSecondary )  )
		return false;

	// Play reload
#ifdef CLIENT_DLL
	if ( ShouldPlayClientReloadSound() )
		WeaponSound( RELOAD );
#else
	WeaponSound( RELOAD );
#endif

	// Play the player's reload animation
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );

	float flReloadTime;
	// First, see if we have a reload animation
	if ( SendWeaponAnim( iActivity ) )
	{
		// We consider the reload finished 0.2 sec before the anim is, so that players don't keep accidentally aborting their reloads
		flReloadTime = SequenceDuration() - 0.2;
	}
	else
	{
		// No reload animation. Use the script time.
		flReloadTime = GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReload;  
		if ( bReloadSecondary )
		{
			flReloadTime = GetTFWpnData().m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flTimeReload;  
		}
	}

	SetReloadTimer( flReloadTime );

	m_bInReload = true;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::UpdateReloadTimers( bool bStart )
{
	// Starting a reload?
	if ( bStart )
	{
		// Get the reload start time.
		SetReloadTimer( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeReloadStart );
	}
	// In reload.
	else
	{
		SetReloadTimer( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeReload );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::SetReloadTimer( float flReloadTime )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	float flBaseReloadTime = flReloadTime;

	CALL_ATTRIB_HOOK_FLOAT( flReloadTime, mult_reload_time );
	CALL_ATTRIB_HOOK_FLOAT( flReloadTime, mult_reload_time_hidden );
	CALL_ATTRIB_HOOK_FLOAT( flReloadTime, fast_reload );
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer, flReloadTime, hwn_mult_reload_time );

	//int iPanicAttack = 0;
	//CALL_ATTRIB_HOOK_INT( iPanicAttack, panic_attack );
	//if ( iPanicAttack ) 
	//{
	//	if ( pPlayer->GetHealth() < pPlayer->GetMaxHealth() * 0.33f )
	//	{
	//		flReloadTime *= 0.3f;
	//	}
	//	else if ( pPlayer->GetHealth() < pPlayer->GetMaxHealth() * 0.66f )
	//	{
	//		flReloadTime *= 0.6f;
	//	}
	//}

	// Haste Powerup Rune adds multiplier to reload time.
	if ( pPlayer->m_Shared.GetCarryingRuneType() == RUNE_HASTE )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_POWERUPMODE_DOMINANT ) )
		{
			flReloadTime *= 0.50f;
		}
		else
		{
			flReloadTime *= 0.20f;
		}
	}
	else if ( pPlayer->m_Shared.GetCarryingRuneType() == RUNE_KING || pPlayer->m_Shared.InCond( TF_COND_KING_BUFFED ) )
	{
		flReloadTime *= 0.75f;
	}

	flReloadTime *= GetReloadSpeedScale();


	int numHealers = pPlayer->m_Shared.GetNumHealers();
	if ( numHealers == 1 )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer, flReloadTime, mult_reload_time_while_healed );
	}

	flReloadTime = MAX( flReloadTime, 0.00001f );
	if ( pPlayer->GetViewModel(0) )
	{
		pPlayer->GetViewModel(0)->SetPlaybackRate( flBaseReloadTime / flReloadTime );
	}
	if ( pPlayer->GetViewModel(1) )
	{
		pPlayer->GetViewModel(1)->SetPlaybackRate( flBaseReloadTime / flReloadTime );
	}

	m_flReloadPriorNextFire = m_flNextPrimaryAttack;

	float flTime = gpGlobals->curtime + flReloadTime;

	// Set next player attack time (weapon independent).
	pPlayer->m_flNextAttack = flTime;

	// Set next weapon attack times (based on reloading).
	m_flNextPrimaryAttack = Max( flTime, (float)m_flReloadPriorNextFire);

	// Don't push out secondary attack, because our secondary fire
	// systems are all separate from primary fire (sniper zooming, demoman pipebomb detonating, etc)
	//m_flNextSecondaryAttack = flTime;

	// Set next idle time (based on reloading).
	SetWeaponIdleTime( flTime );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBase::PlayEmptySound()
{
	CPASAttenuationFilter filter( this );
	filter.UsePredictionRules();

	// TFTODO: Add default empty sound here!
//	EmitSound( filter, entindex(), "Default.ClipEmpty_Rifle" );

	return false;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::SendReloadEvents()
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	// Make the player play his reload animation.
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::ItemBusyFrame( void )
{
	// Call into the base ItemBusyFrame.
	BaseClass::ItemBusyFrame();

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
	{
		return;
	}

	if ( ( pOwner->m_nButtons & IN_ATTACK2 ) && /*m_bInReload == false &&*/ m_bInAttack2 == false )
	{
		pOwner->DoClassSpecialSkill();
		m_bInAttack2 = true;
	}
	else if ( !(pOwner->m_nButtons & IN_ATTACK2) && m_bInAttack2 )
	{
		m_bInAttack2 = false;
	}

	// Interrupt a reload on reload singly weapons.
	if ( ( pOwner->m_nButtons & IN_ATTACK ) && Clip1() > 0 )
	{
		bool bAbortReload = false;
		if ( m_bReloadsSingly )
		{
			if ( m_iReloadMode != TF_RELOAD_START )
			{
				m_iReloadMode.Set( TF_RELOAD_START );
				bAbortReload = true;
			}
		}
		else if ( m_bInReload )
		{
			// We don't let them abort before the next fire point, so they can't use reload to fire before they would have fired if they hadn't reloaded
			if ( gpGlobals->curtime >= m_flReloadPriorNextFire )
			{
				bAbortReload = true;
			}
		}

		if ( bAbortReload )
		{
			AbortReload();
			m_bInReload = false;
			pOwner->m_flNextAttack = gpGlobals->curtime;
			m_flNextPrimaryAttack = Max<float>( gpGlobals->curtime, m_flReloadPriorNextFire );
			SetWeaponIdleTime( gpGlobals->curtime + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeIdle );
		}
	}

#ifdef GAME_DLL

	// If we have an active-weapon-only regen, we accumulate regen time while active, so that
	// they can't avoid the regen/degen by weapon switching rapidly.
	ApplyItemRegen();

#endif

	CheckEffectBarRegen();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::ItemPostFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
	{
		return;
	}

	bool bNeedsReload = NeedsReloadForAmmo1( GetMaxClip1() ) || ( IsEnergyWeapon() && !Energy_FullyCharged() );

	// If we're not shooting, and we want to autoreload, press our reload key
	if ( !AutoFiresFullClip() && pOwner->ShouldAutoReload() && UsesClipsForAmmo1() && !(pOwner->m_nButtons & (IN_ATTACK|IN_ATTACK2)) && bNeedsReload )
	{
		pOwner->m_nButtons |= IN_RELOAD;
	}

	// debounce InAttack flags
	if ( m_bInAttack && !( pOwner->m_nButtons & IN_ATTACK ) )
	{
		m_bInAttack = false;
	}

	if ( m_bInAttack2 && !( pOwner->m_nButtons & IN_ATTACK2 ) )
	{
		m_bInAttack2 = false;
	}

#ifdef GAME_DLL

	// If we have an active-weapon-only regen, we accumulate regen time while active, so that
	// they can't avoid the regen/degen by weapon switching rapidly.
	ApplyItemRegen();

#endif

	CheckEffectBarRegen();

	// If we're lowered, we're not allowed to fire
	if ( m_bLowered )
		return;

	// Call the base item post frame.
	BaseClass::ItemPostFrame();

	// Check for reload singly interrupts.
	if ( m_bReloadsSingly )
	{
		ReloadSinglyPostFrame();
	}

	if ( AutoFiresFullClip() && AutoFiresFullClipAllAtOnce() && m_iClip1 == GetMaxClip1() )
	{
		FireFullClipAtOnce();
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Activity CTFWeaponBase::GetInspectActivity( TFWeaponInspectStage inspectStage )
{
	static struct InspectAct_t
	{
		loadout_positions_t loadoutSlot;
		Activity stages[INSPECT_STAGE_COUNT];
	} s_inspectActivities[] =
	{
		{
			LOADOUT_POSITION_PRIMARY,
			{
				ACT_PRIMARY_VM_INSPECT_START,
				ACT_PRIMARY_VM_INSPECT_IDLE,
				ACT_PRIMARY_VM_INSPECT_END
			}
		},
		{
			LOADOUT_POSITION_SECONDARY,
			{
				ACT_SECONDARY_VM_INSPECT_START,
				ACT_SECONDARY_VM_INSPECT_IDLE,
				ACT_SECONDARY_VM_INSPECT_END
			}
		},
		{
			LOADOUT_POSITION_MELEE,
			{
				ACT_MELEE_VM_INSPECT_START,
				ACT_MELEE_VM_INSPECT_IDLE,
				ACT_MELEE_VM_INSPECT_END
			}
		},
		{
			LOADOUT_POSITION_BUILDING,
			{
				ACT_BUILDING_VM_INSPECT_START,
				ACT_BUILDING_VM_INSPECT_IDLE,
				ACT_BUILDING_VM_INSPECT_END
			}
		},
	};

	loadout_positions_t iLoadoutSlot = LOADOUT_POSITION_INVALID;
	CTFPlayer *pOwner = GetTFPlayerOwner();
	const CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pOwner && pItem )
	{
		int iClass = pOwner->GetPlayerClass()->GetClassIndex();
		iLoadoutSlot = (loadout_positions_t)pItem->GetStaticData()->GetLoadoutSlot( iClass );
	}

	// default to primary slot
	Activity act = s_inspectActivities[0].stages[inspectStage];
	for ( int i=0; i < ARRAYSIZE( s_inspectActivities ); ++i )
	{
		if ( s_inspectActivities[i].loadoutSlot == iLoadoutSlot )
		{
			act = s_inspectActivities[i].stages[inspectStage];
			break;
		}
	}

	return act;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBase::HandleInspect()
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanInspect() )
		return;

	// first time pressing inspecting key
	if ( !m_bInspecting && pPlayer->IsInspecting() )
	{
		m_nInspectStage = INSPECT_INVALID;
		m_flInspectAnimEndTime = -1.f;
		if ( SendWeaponAnim( GetInspectActivity( INSPECT_START ) ) )
		{
			m_flInspectAnimEndTime = gpGlobals->curtime + SequenceDuration();
			m_nInspectStage = INSPECT_START;
		}
	}
	else if ( !pPlayer->IsInspecting() && m_nInspectStage == INSPECT_IDLE )
	{
		// transition from idle to end when the inspect button is released
		if ( SendWeaponAnim( GetInspectActivity( INSPECT_END ) ) )
		{
			m_flInspectAnimEndTime = gpGlobals->curtime + SequenceDuration();
			m_nInspectStage = INSPECT_END;
		}
	}
	else if ( m_nInspectStage != INSPECT_INVALID ) // inspecting
	{
		if ( gpGlobals->curtime > m_flInspectAnimEndTime )
		{
			if ( m_nInspectStage == INSPECT_START )
			{
				TFWeaponInspectStage inspectStage = pPlayer->IsInspecting() ? INSPECT_IDLE : INSPECT_END;
				// transition from start to idle, or end if the inspect button is released
				if ( SendWeaponAnim( GetInspectActivity( inspectStage ) ) )
				{
					m_flInspectAnimEndTime = gpGlobals->curtime + SequenceDuration();
					m_nInspectStage = inspectStage;
				}
			}
			else if ( m_nInspectStage == INSPECT_END )
			{
				m_flInspectAnimEndTime = -1.f;
				m_nInspectStage = INSPECT_INVALID;
				SendWeaponAnim( ACT_VM_IDLE );
			}
		}
	}

	m_bInspecting = pPlayer->IsInspecting();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFWeaponBase::GetNextSecondaryAttackDelay( void )
{
	// This is a little gross.  The demo needs fast-cycle secondary
	// attacks while holding down +attack2 and switching weapons
	// in order to properly handle timely sticky detonation.
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner && pOwner->IsPlayerClass( TF_CLASS_DEMOMAN ) )
	{
		return 0.1f;
	}

	return BaseClass::GetNextSecondaryAttackDelay();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBase::ItemHolsterFrame( void )
{
	BaseClass::ItemHolsterFrame();

	CheckEffectBarRegen();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::ReloadSinglyPostFrame( void )
{
	if ( m_flTimeWeaponIdle > gpGlobals->curtime )
		return;

	// if the clip is empty and we have ammo remaining,
	if ( IsEnergyWeapon() )
	{
		Reload();
	}
	else if ( ( !AutoFiresFullClip() && Clip1() == 0 && GetOwner()->GetAmmoCount( m_iPrimaryAmmoType ) > 0 ) ||
		// or we are already in the process of reloading but not finished
		( m_iReloadMode != TF_RELOAD_START ) )
	{
		// reload/continue reloading
		Reload();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::WeaponShouldBeLowered( void )
{
	// Can't be in the middle of another animation
	if ( GetIdealActivity() != ACT_VM_IDLE_LOWERED && GetIdealActivity() != ACT_VM_IDLE &&
		GetIdealActivity() != ACT_VM_IDLE_TO_LOWERED && GetIdealActivity() != ACT_VM_LOWERED_TO_IDLE )
		return false;

	if ( m_bLowered )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::Ready( void )
{
	// If we don't have the anim, just hide for now
	if ( SelectWeightedSequence( ACT_VM_IDLE_LOWERED ) == ACTIVITY_NOT_AVAILABLE )
	{
		RemoveEffects( EF_NODRAW );
	}

	m_bLowered = false;	
	SendWeaponAnim( ACT_VM_IDLE );

	// Prevent firing until our weapon is back up
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	pPlayer->m_flNextAttack = gpGlobals->curtime + SequenceDuration();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::Lower( void )
{
	AbortReload();

	// If we don't have the anim, just hide for now
	if ( SelectWeightedSequence( ACT_VM_IDLE_LOWERED ) == ACTIVITY_NOT_AVAILABLE )
	{
		AddEffects( EF_NODRAW );
	}

	m_bLowered = true;
	SendWeaponAnim( ACT_VM_IDLE_LOWERED );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Show/hide weapon and corresponding view model if any
// Input  : visible - 
//-----------------------------------------------------------------------------
void CTFWeaponBase::SetWeaponVisible( bool visible )
{
	if ( visible )
	{
		RemoveEffects( EF_NODRAW );
	}
	else
	{
		AddEffects( EF_NODRAW );
	}
	
#ifdef CLIENT_DLL
	UpdateVisibility();

	// Force an update
	PreDataUpdate( DATA_UPDATE_DATATABLE_CHANGED );
#endif

}

//-----------------------------------------------------------------------------
// Purpose: Allows the weapon to choose proper weapon idle animation
//-----------------------------------------------------------------------------
void CTFWeaponBase::WeaponIdle( void )
{
	//See if we should idle high or low
	if ( WeaponShouldBeLowered() )
	{
		// Move to lowered position if we're not there yet
		if ( GetActivity() != ACT_VM_IDLE_LOWERED && GetActivity() != ACT_VM_IDLE_TO_LOWERED && GetActivity() != ACT_TRANSITION )
		{
			SendWeaponAnim( ACT_VM_IDLE_LOWERED );
		}
		else if ( HasWeaponIdleTimeElapsed() )
		{
			// Keep idling low
			SendWeaponAnim( ACT_VM_IDLE_LOWERED );
		}
	}
	else
	{
		// See if we need to raise immediately
		if ( GetActivity() == ACT_VM_IDLE_LOWERED ) 
		{
			SendWeaponAnim( ACT_VM_IDLE );
		}
		else if ( HasWeaponIdleTimeElapsed() ) 
		{
			if ( !( m_bReloadsSingly && m_iReloadMode != TF_RELOAD_START ) )
			{
				SendWeaponAnim( ACT_VM_IDLE );
				m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();
			}

#ifdef GAME_DLL
			m_iHitsInTime = 0;
			m_iProjectilesFiredInTime = 0;
#endif // GAME_DLL
			m_iConsecutiveShots = 0;
		}
	}
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const char *CTFWeaponBase::GetMuzzleFlashModel( void )
{ 
	const char *pszModel = GetTFWpnData().m_szMuzzleFlashModel;

	if ( Q_strlen( pszModel ) > 0 )
	{
		return pszModel;
	}

	return NULL;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const char *CTFWeaponBase::GetMuzzleFlashParticleEffect( void )
{ 
	const char *pszPEffect = GetTFWpnData().m_szMuzzleFlashParticleEffect;

	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem->IsValid() )
	{
		const char *pszItemMuzzleEffect = pItem->GetStaticData()->GetMuzzleFlash( GetTeamNumber() );
		if ( pszItemMuzzleEffect )
		{
			pszPEffect = pszItemMuzzleEffect;
		}
	}

	if ( Q_strlen( pszPEffect ) > 0 )
	{
		return pszPEffect;
	}

	return NULL;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
float CTFWeaponBase::GetMuzzleFlashModelLifetime( void )
{ 
	return GetTFWpnData().m_flMuzzleFlashModelDuration;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFWeaponBase::GetTracerType( void )
{ 
	const char* pszTracerEffect = GetTFWpnData().m_szTracerEffect;
	if ( tf_useparticletracers.GetBool() )
	{
		CEconItemView *pItem = GetAttributeContainer()->GetItem();
		if ( pItem->IsValid() )
		{
			// Look for a replacement effect specified in the item's visual attributes.
			const char *pszItemTracerEffect = pItem->GetStaticData()->GetTracerEffect( GetTeamNumber() );
			if ( pszItemTracerEffect )
			{
				pszTracerEffect = pszItemTracerEffect;
			}
		}

		if ( pszTracerEffect && pszTracerEffect[0] )
		{
			if ( !m_szTracerName[0] )
			{
				Q_snprintf( m_szTracerName, MAX_TRACER_NAME, "%s_%s", pszTracerEffect, 
					(GetOwner() && GetOwner()->GetTeamNumber() == TF_TEAM_RED ) ? "red" : "blue" );
			}

			return m_szTracerName;
		}
	}

	if ( GetWeaponID() == TF_WEAPON_MINIGUN )
		return "BrightTracer";

	return BaseClass::GetTracerType();
}

//=============================================================================
//
// TFWeaponBase functions (Server specific).
//
#if !defined( CLIENT_DLL )

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::CheckRespawn()
{
	// Do not respawn.
	return;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBase::Respawn()
{
	// make a copy of this weapon that is invisible and inaccessible to players (no touch function). The weapon spawn/respawn code
	// will decide when to make the weapon visible and touchable.
	CBaseEntity *pNewWeapon = CBaseEntity::Create( GetClassname(), g_pGameRules->VecWeaponRespawnSpot( this ), GetAbsAngles(), GetOwner() );

	if ( pNewWeapon )
	{
		pNewWeapon->AddEffects( EF_NODRAW );// invisible for now
		pNewWeapon->SetTouch( NULL );// no touch
		pNewWeapon->SetThink( &CTFWeaponBase::AttemptToMaterialize );

		UTIL_DropToFloor( this, MASK_SOLID );

		// not a typo! We want to know when the weapon the player just picked up should respawn! This new entity we created is the replacement,
		// but when it should respawn is based on conditions belonging to the weapon that was taken.
		pNewWeapon->SetNextThink( gpGlobals->curtime + g_pGameRules->FlWeaponRespawnTime( this ) );
	}
	else
	{
		Msg( "Respawn failed to create %s!\n", GetClassname() );
	}

	return pNewWeapon;
}

// -----------------------------------------------------------------------------
// Purpose: Make a weapon visible and tangible.
// -----------------------------------------------------------------------------
void CTFWeaponBase::Materialize()
{
	if ( IsEffectActive( EF_NODRAW ) )
	{
		RemoveEffects( EF_NODRAW );
		DoMuzzleFlash();
	}

	AddSolidFlags( FSOLID_TRIGGER );

	SetThink ( &CTFWeaponBase::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 1 );
}

// -----------------------------------------------------------------------------
// Purpose: The item is trying to materialize, should it do so now or wait longer?
// -----------------------------------------------------------------------------
void CTFWeaponBase::AttemptToMaterialize()
{
	float flTime = g_pGameRules->FlWeaponTryRespawn( this );

	if ( flTime == 0 )
	{
		Materialize();
		return;
	}

	SetNextThink( gpGlobals->curtime + flTime );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::SetDieThink( bool bDie )
{
	if( bDie )
	{
		SetContextThink( &CTFWeaponBase::Die, gpGlobals->curtime + 30.0f, "DieContext" );
	}
	else
	{
		SetContextThink( NULL, gpGlobals->curtime, "DieContext" );
	}
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::Die( void )
{
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::WeaponReset( void )
{
	m_iReloadMode.Set( TF_RELOAD_START );

	m_bResetParity = !m_bResetParity;

	m_flEnergy = Energy_GetMaxEnergy();
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
const Vector &CTFWeaponBase::GetBulletSpread( void )
{
	static Vector cone = VECTOR_CONE_15DEGREES;
	return cone;
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::OnBulletFire( int iEnemyPlayersHit )
{
	if ( !iEnemyPlayersHit )
	{
		m_iConsecutiveKills = 0;
	}
	else
	{
		m_iHitsInTime++;
		m_flLastHitTime = gpGlobals->curtime;
	}
	// Todo: Expand to all projectiles.
	m_iProjectilesFiredInTime++;
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::OnPlayerKill( CTFPlayer *pVictim, const CTakeDamageInfo &info )
{
	m_iConsecutiveKills++;
	if ( pVictim )
	{
		int nClassIndex = pVictim->GetPlayerClass()->GetClassIndex();

		float fKillComboFireRateBoost = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT( fKillComboFireRateBoost, kill_combo_fire_rate_boost );
		if ( fKillComboFireRateBoost != 0.0f )
		{
			AddKillCombo( nClassIndex );

			if ( GetKillComboCount() == 1 )
			{
				// Yell when we switch class combo type
				CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
				if ( pOwner )
				{
					pOwner->SpeakConceptIfAllowed( MP_CONCEPT_COMBO_KILLED, CFmtStr( "victimclass:%s", g_aPlayerClassNames_NonLocalized[ nClassIndex ] ).Access() );
				}
			}
		}
	}
}

#else

void TE_DynamicLight( IRecipientFilter& filter, float delay,
					 const Vector* org, int r, int g, int b, int exponent, float radius, float time, float decay, int nLightIndex = LIGHT_INDEX_TE_DYNAMIC );

//=============================================================================
//
// TFWeaponBase functions (Client specific).
//


bool CTFWeaponBase::IsFirstPersonView()
{
	C_TFPlayer *pPlayerOwner = GetTFPlayerOwner();
	if ( pPlayerOwner == NULL )
	{
		return false;
	}
	return pPlayerOwner->InFirstPersonView();
}

bool CTFWeaponBase::UsingViewModel()
{
	C_TFPlayer *pPlayerOwner = GetTFPlayerOwner();
	bool bIsFirstPersonView = IsFirstPersonView();
	bool bUsingViewModel = bIsFirstPersonView && ( pPlayerOwner != NULL ) && !pPlayerOwner->ShouldDrawThisPlayer();
	return bUsingViewModel;
}

C_BaseAnimating *CTFWeaponBase::GetAppropriateWorldOrViewModel()
{
	C_TFPlayer *pPlayerOwner = GetTFPlayerOwner();
	if ( pPlayerOwner && UsingViewModel() )
	{
		// For w_* models the viewmodel itself is just arms+hands. And attached to them is the actual weapon.
		const CEconItemView *pItem = GetAttributeContainer()->GetItem();
		if ( pItem->IsValid() && pItem->GetStaticData()->ShouldAttachToHands() )
		{
			C_BaseAnimating *pVMAttach = GetViewmodelAttachment();
			if ( pVMAttach != NULL )
			{
				return pVMAttach;
			}
		}

		// Nope - it's a standard viewmodel.
		C_BaseAnimating *pViewModel = pPlayerOwner->GetViewModel();
		if ( pViewModel != NULL )
		{
			return pViewModel;
		}

		// No viewmodel, so just return the normal model.
		return this;
	}
	else
	{
		return this;
	}
}


void CTFWeaponBase::CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex )
{
	Vector vecOrigin;
	QAngle angAngles;

	if ( !pAttachEnt )
		return;

	if ( UsingViewModel() && !g_pClientMode->ShouldDrawViewModel() )
	{
		// Prevent effects when the ViewModel is hidden with r_drawviewmodel=0
		return;
	}

	int iMuzzleFlashAttachment = pAttachEnt->LookupAttachment( "muzzle" );

	const char *pszMuzzleFlashEffect = NULL;
	const char *pszMuzzleFlashModel = GetMuzzleFlashModel();
	const char *pszMuzzleFlashParticleEffect = GetMuzzleFlashParticleEffect();

	// Pick the right muzzleflash (3rd / 1st person)
	// (this uses IsFirstPersonView() rather than UsingViewModel() because even when NOT using the viewmodel, in 1st-person mode we still want the 1st-person muzzleflash effect)
	if ( IsFirstPersonView() )
	{
		pszMuzzleFlashEffect = GetMuzzleFlashEffectName_1st();
	}
	else
	{
		pszMuzzleFlashEffect = GetMuzzleFlashEffectName_3rd();
	}

	// If we have an attachment, then stick a light on it.
	if ( iMuzzleFlashAttachment > 0 && (pszMuzzleFlashEffect || pszMuzzleFlashModel || pszMuzzleFlashParticleEffect ) )
	{
		pAttachEnt->GetAttachment( iMuzzleFlashAttachment, vecOrigin, angAngles );

		// Muzzleflash light
/*
		CLocalPlayerFilter filter;
		TE_DynamicLight( filter, 0.0f, &vecOrigin, 255, 192, 64, 5, 70.0f, 0.05f, 70.0f / 0.05f, LIGHT_INDEX_MUZZLEFLASH );
*/

		if ( pszMuzzleFlashEffect )
		{
			// Using an muzzle flash dispatch effect
			CEffectData muzzleFlashData;
			muzzleFlashData.m_vOrigin = vecOrigin;
			muzzleFlashData.m_vAngles = angAngles;
			muzzleFlashData.m_hEntity = pAttachEnt->GetRefEHandle();
			muzzleFlashData.m_nAttachmentIndex = iMuzzleFlashAttachment;
			//muzzleFlashData.m_nHitBox = GetDODWpnData().m_iMuzzleFlashType;
			//muzzleFlashData.m_flMagnitude = GetDODWpnData().m_flMuzzleFlashScale;
			muzzleFlashData.m_flMagnitude = 0.2;
			DispatchEffect( pszMuzzleFlashEffect, muzzleFlashData );
		}

		if ( pszMuzzleFlashModel )
		{
			float flEffectLifetime = GetMuzzleFlashModelLifetime();

			// Using a model as a muzzle flash.
			if ( m_hMuzzleFlashModel[nIndex] )
			{
				// Increase the lifetime of the muzzleflash
				m_hMuzzleFlashModel[nIndex]->SetLifetime( flEffectLifetime );
			}
			else
			{
				m_hMuzzleFlashModel[nIndex] = C_MuzzleFlashModel::CreateMuzzleFlashModel( pszMuzzleFlashModel, pAttachEnt, iMuzzleFlashAttachment, flEffectLifetime );

				// FIXME: This is an incredibly brutal hack to get muzzle flashes positioned correctly for recording
				m_hMuzzleFlashModel[nIndex]->SetIs3rdPersonFlash( nIndex == 1 );
			}
		}

		if ( pszMuzzleFlashParticleEffect ) 
		{
			DispatchMuzzleFlash( pszMuzzleFlashParticleEffect, pAttachEnt );
		}
	}
}

void CTFWeaponBase::DispatchMuzzleFlash( const char* effectName, C_BaseEntity* pAttachEnt )
{
	DispatchParticleEffect( effectName, PATTACH_POINT_FOLLOW, pAttachEnt, "muzzle" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::ShouldDraw( void )
{
	C_BaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return true;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return true;

	if ( pOwner->IsPlayer() )
	{
		CTFPlayer *pTFOwner = ToTFPlayer( GetOwner() );
		if ( !pTFOwner )
			return true;

		if ( pTFOwner->m_Shared.IsControlStunned() )
			return false;

		// Ghosts dont have weapons
		if ( pTFOwner->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
			return false;

		if ( pTFOwner->m_Shared.GetDisguiseWeapon() )
		{
			if ( pTFOwner->m_Shared.InCond( TF_COND_DISGUISED ) )
			{
				int iLocalPlayerTeam = pLocalPlayer->GetTeamNumber();
				if ( pLocalPlayer->m_bIsCoaching && pLocalPlayer->m_hStudent )
				{
					iLocalPlayerTeam = pLocalPlayer->m_hStudent->GetTeamNumber();
				}
				// If we are disguised we may want to draw the disguise weapon.
				if ( iLocalPlayerTeam != pOwner->GetTeamNumber() && (iLocalPlayerTeam != TEAM_SPECTATOR) )
				{
					// We are a disguised enemy, so only draw the disguise weapon.
					if ( pTFOwner->m_Shared.GetDisguiseWeapon() != this )
					{
						return false;
					}
				}
				else
				{
					// We are a disguised friendly. Don't draw the disguise weapon.
					if ( m_bDisguiseWeapon )
					{
						return false;
					}
				}
			}
			else
			{
				// We are not disguised. Never draw the disguise weapon.
				if ( m_bDisguiseWeapon )
				{
					return false;
				}
			}
		}
	}

	return BaseClass::ShouldDraw();
}

void CTFWeaponBase::UpdateVisibility( void )
{
	BaseClass::UpdateVisibility();

	UpdateExtraWearablesVisibility();

	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner )
	{
		pOwner->SetBodygroupsDirty();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFWeaponBase::InternalDrawModel( int flags )
{
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	bool bNotViewModel = ( pOwner->ShouldDrawThisPlayer() );
	bool bUseInvulnMaterial = ( bNotViewModel && pOwner && pOwner->m_Shared.IsInvulnerable() && 
								( !pOwner->m_Shared.InCond( TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED ) || gpGlobals->curtime < pOwner->GetLastDamageTimeMvMOnly() + 2.0f ) );

	if ( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( *pOwner->GetInvulnMaterialRef() );
	}

	int ret = BaseClass::InternalDrawModel( flags );

	if ( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( NULL );
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	// Correct the ambient lighting position to match our owner entity
	if ( GetOwner() && pInfo )
	{
		pInfo->pLightingOrigin = &( GetOwner()->WorldSpaceCenter() );
	}

	return BaseClass::OnInternalDrawModel( pInfo );
}

void CTFWeaponBase::ProcessMuzzleFlashEvent( void )
{
	C_BaseAnimating *pAttachEnt = GetAppropriateWorldOrViewModel();
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );

	if ( pOwner == NULL )
		return;


	bool bDrawMuzzleFlashOnViewModel = ( pAttachEnt != this );
	{
		CRecordEffectOwner recordOwner( pOwner, bDrawMuzzleFlashOnViewModel );
		CreateMuzzleFlashEffects( pAttachEnt, 0 );
	}

	// Quasi-evil
	int nModelIndex = GetModelIndex();
	int nWorldModelIndex = GetWorldModelIndex();
	bool bInToolRecordingMode = ToolsEnabled() && clienttools->IsInRecordingMode();
	if ( bInToolRecordingMode && nModelIndex != nWorldModelIndex && pOwner->IsLocalPlayer() )
	{
		CRecordEffectOwner recordOwner( pOwner, false );

		SetModelIndex( nWorldModelIndex );
		CreateMuzzleFlashEffects( this, 1 );
		SetModelIndex( nModelIndex );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
bool CTFWeaponBase::ShouldPredict()
{
	if ( GetOwner() && GetOwner() == C_BasePlayer::GetLocalPlayer() )
	{
		return true;
	}

	return BaseClass::ShouldPredict();
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::WeaponReset( void )
{
	UpdateVisibility();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void CTFWeaponBase::PostDataUpdate( DataUpdateType_t updateType )
{
	UpdateModelIndex();

	BaseClass::PostDataUpdate( updateType );
}

void CTFWeaponBase::UpdateModelIndex()
{
	// We need to do this before the C_BaseAnimating code starts to drive
	// clientside animation sequences on this model, which will be using bad sequences for the world model.
	int iDesiredModelIndex = 0;
	C_BasePlayer *pOwner = ToBasePlayer(GetOwner());
	if ( !pOwner->ShouldDrawThisPlayer() )
	{
		iDesiredModelIndex = m_iViewModelIndex;
	}
	else
	{
		iDesiredModelIndex = GetWorldModelIndex();

		// Our world models never animate
		SetSequence( 0 );
	}

	if ( GetModelIndex() != iDesiredModelIndex )
	{
		SetModelIndex( iDesiredModelIndex );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::OnPreDataChanged( DataUpdateType_t type )
{
	BaseClass::OnPreDataChanged( type );

	m_bOldResetParity = m_bResetParity;

}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		ListenForGameEvent( "localplayer_changeteam" );
	}

	if ( GetPredictable() && !ShouldPredict() )
	{
		ShutdownPredictable();
	}

	//If its a world (held or dropped) model then set the correct skin color here.
	if ( m_nModelIndex == GetWorldModelIndex() )
	{
		m_nSkin = GetSkin();
	}

	if ( m_bResetParity != m_bOldResetParity )
	{
		WeaponReset();
	}

	UpdateParticleSystems();

	if ( m_iOldTeam != m_iTeamNum )
	{
		// Recompute our tracer name
		m_szTracerName[0] = '\0';
	}

	//if ( m_hExtraWearable.Get() && m_hExtraWearable->IsVisible() != IsVisible() )
	if ( m_hExtraWearable.Get() )
	{
		m_hExtraWearable->UpdateVisibility();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBase::FireGameEvent( IGameEvent *event )
{
	// If we were the active weapon, we need to update our visibility 
	// because we may switch visibility due to Spy disguises.
	const char *pszEventName = event->GetName();
	if ( Q_strcmp( pszEventName, "localplayer_changeteam" ) == 0 )
	{
		UpdateVisibility();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
int CTFWeaponBase::GetWorldModelIndex( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	// Guitar Riff taunt support.
	if ( pPlayer )
	{
		bool bReplaceModel = true;
		const char* pszCustomTauntProp = NULL;

		if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) && ( pPlayer->m_Shared.GetTauntIndex() == TAUNT_MISC_ITEM || pPlayer->m_Shared.GetTauntIndex() == TAUNT_LONG ) )
		{
			int iClass = pPlayer->GetPlayerClass()->GetClassIndex();

			CEconItemView *pMiscItemView = pPlayer->GetTauntEconItemView();
			if ( pMiscItemView && pMiscItemView->GetStaticData()->GetTauntData() )
			{
				// if prop has its own animation, don't replace weapon model
				if ( !pMiscItemView->GetStaticData()->GetTauntData()->GetPropIntroScene( iClass ) )
				{
					pszCustomTauntProp = pMiscItemView->GetStaticData()->GetTauntData()->GetProp( iClass );
				}
			}
		}

		if ( pszCustomTauntProp )
		{
			m_iWorldModelIndex = modelinfo->GetModelIndex( pszCustomTauntProp );
		}
		else
		{
			bReplaceModel = false;
		}

		if ( bReplaceModel )
		{
			return m_iWorldModelIndex;
		}
	}

	if ( m_iCachedModelIndex == 0 )
	{
		// Remember our normal world model index so we can quickly replace it later.
		m_iCachedModelIndex = modelinfo->GetModelIndex( GetWorldModel() );
	}

	// We aren't taunting, so we want to use the cached model index.
	if ( m_iWorldModelIndex != m_iCachedModelIndex )
	{
		m_iWorldModelIndex = m_iCachedModelIndex;
	}

	if ( pPlayer )
	{
		// if we're a spy and we're disguised, we also
		// want to disguise our weapon's world model

		CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pLocalPlayer )
			return 0;

		int iLocalTeam = pLocalPlayer->GetTeamNumber();

		// We only show disguise weapon to the enemy team when owner is disguised
		bool bUseDisguiseWeapon = ( pPlayer->GetTeamNumber() != iLocalTeam && iLocalTeam > LAST_SHARED_TEAM );

		if ( bUseDisguiseWeapon && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			CTFWeaponBase *pDisguiseWeapon = pPlayer->m_Shared.GetDisguiseWeapon();
			if ( !pDisguiseWeapon )
				return BaseClass::GetWorldModelIndex();
			if ( pDisguiseWeapon == this )
				return BaseClass::GetWorldModelIndex();
			else
				return pDisguiseWeapon->GetWorldModelIndex();
		}	
	}

	return BaseClass::GetWorldModelIndex();
}

bool CTFWeaponBase::ShouldDrawCrosshair( void )
{
	const char *crosshairfile = cl_crosshair_file.GetString();
	if ( !crosshairfile || !crosshairfile[0] )
	{
		// Default crosshair.
		return GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_bDrawCrosshair;
	}
	// Custom crosshair.
	return true;
}

void CTFWeaponBase::Redraw()
{
	if ( ShouldDrawCrosshair() && g_pClientMode->ShouldDrawCrosshair() )
	{
		DrawCrosshair();
	}
}

#endif

acttable_t s_acttablePrimary[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_PRIMARY,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_PRIMARY,				false },
	{ ACT_MP_DEPLOYED,			ACT_MP_DEPLOYED_PRIMARY,			false },
	{ ACT_MP_CROUCH_DEPLOYED,			ACT_MP_CROUCHWALK_DEPLOYED,	false },
	{ ACT_MP_CROUCH_DEPLOYED_IDLE,		ACT_MP_CROUCH_DEPLOYED_IDLE,	false },
	{ ACT_MP_RUN,				ACT_MP_RUN_PRIMARY,					false },
	{ ACT_MP_WALK,				ACT_MP_WALK_PRIMARY,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_PRIMARY,				false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_PRIMARY,			false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_PRIMARY,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_PRIMARY,			false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_PRIMARY,			false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_PRIMARY,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_PRIMARY,				false },
	{ ACT_MP_SWIM_DEPLOYED,		ACT_MP_SWIM_DEPLOYED_PRIMARY,		false },
	{ ACT_MP_DOUBLEJUMP_CROUCH,	ACT_MP_DOUBLEJUMP_CROUCH_PRIMARY,	false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_PRIMARY,	false },
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE_DEPLOYED,		ACT_MP_ATTACK_STAND_PRIMARY_DEPLOYED, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_PRIMARY,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE_DEPLOYED,	ACT_MP_ATTACK_CROUCH_PRIMARY_DEPLOYED,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_PRIMARY,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_PRIMARY,	false },

	{ ACT_MP_RELOAD_STAND,		ACT_MP_RELOAD_STAND_PRIMARY,		false },
	{ ACT_MP_RELOAD_STAND_LOOP,	ACT_MP_RELOAD_STAND_PRIMARY_LOOP,	false },
	{ ACT_MP_RELOAD_STAND_END,	ACT_MP_RELOAD_STAND_PRIMARY_END,	false },
	{ ACT_MP_RELOAD_CROUCH,		ACT_MP_RELOAD_CROUCH_PRIMARY,		false },
	{ ACT_MP_RELOAD_CROUCH_LOOP,ACT_MP_RELOAD_CROUCH_PRIMARY_LOOP,	false },
	{ ACT_MP_RELOAD_CROUCH_END,	ACT_MP_RELOAD_CROUCH_PRIMARY_END,	false },
	{ ACT_MP_RELOAD_SWIM,		ACT_MP_RELOAD_SWIM_PRIMARY,			false },
	{ ACT_MP_RELOAD_SWIM_LOOP,	ACT_MP_RELOAD_SWIM_PRIMARY_LOOP,	false },
	{ ACT_MP_RELOAD_SWIM_END,	ACT_MP_RELOAD_SWIM_PRIMARY_END,		false },
	{ ACT_MP_RELOAD_AIRWALK,	ACT_MP_RELOAD_AIRWALK_PRIMARY,		false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP,	ACT_MP_RELOAD_AIRWALK_PRIMARY_LOOP,	false },
	{ ACT_MP_RELOAD_AIRWALK_END,ACT_MP_RELOAD_AIRWALK_PRIMARY_END,	false },

	{ ACT_MP_GESTURE_FLINCH,	ACT_MP_GESTURE_FLINCH_PRIMARY, false },

	{ ACT_MP_GRENADE1_DRAW,		ACT_MP_PRIMARY_GRENADE1_DRAW,	false },
	{ ACT_MP_GRENADE1_IDLE,		ACT_MP_PRIMARY_GRENADE1_IDLE,	false },
	{ ACT_MP_GRENADE1_ATTACK,	ACT_MP_PRIMARY_GRENADE1_ATTACK,	false },
	{ ACT_MP_GRENADE2_DRAW,		ACT_MP_PRIMARY_GRENADE2_DRAW,	false },
	{ ACT_MP_GRENADE2_IDLE,		ACT_MP_PRIMARY_GRENADE2_IDLE,	false },
	{ ACT_MP_GRENADE2_ATTACK,	ACT_MP_PRIMARY_GRENADE2_ATTACK,	false },

	{ ACT_MP_ATTACK_STAND_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_SWIM_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE,	ACT_MP_ATTACK_STAND_GRENADE,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_PRIMARY,	false },

	{ ACT_MP_FALLING_STOMP,	ACT_MP_FALLING_STOMP_PRIMARY,	false },
};

acttable_t s_acttableSecondary[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_SECONDARY,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_SECONDARY,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_SECONDARY,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_SECONDARY,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_SECONDARY,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_SECONDARY,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_SECONDARY,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_SECONDARY,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_SECONDARY,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_SECONDARY,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_SECONDARY,				false },
	{ ACT_MP_DOUBLEJUMP_CROUCH,	ACT_MP_DOUBLEJUMP_CROUCH_SECONDARY,	false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_SECONDARY,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_SECONDARY,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_SECONDARY,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_SECONDARY,	false },

	{ ACT_MP_RELOAD_STAND,		ACT_MP_RELOAD_STAND_SECONDARY,		false },
	{ ACT_MP_RELOAD_STAND_LOOP,	ACT_MP_RELOAD_STAND_SECONDARY_LOOP,	false },
	{ ACT_MP_RELOAD_STAND_END,	ACT_MP_RELOAD_STAND_SECONDARY_END,	false },
	{ ACT_MP_RELOAD_CROUCH,		ACT_MP_RELOAD_CROUCH_SECONDARY,		false },
	{ ACT_MP_RELOAD_CROUCH_LOOP,ACT_MP_RELOAD_CROUCH_SECONDARY_LOOP,false },
	{ ACT_MP_RELOAD_CROUCH_END,	ACT_MP_RELOAD_CROUCH_SECONDARY_END,	false },
	{ ACT_MP_RELOAD_SWIM,		ACT_MP_RELOAD_SWIM_SECONDARY,		false },
	{ ACT_MP_RELOAD_SWIM_LOOP,	ACT_MP_RELOAD_SWIM_SECONDARY_LOOP,	false },
	{ ACT_MP_RELOAD_SWIM_END,	ACT_MP_RELOAD_SWIM_SECONDARY_END,	false },
	{ ACT_MP_RELOAD_AIRWALK,	ACT_MP_RELOAD_AIRWALK_SECONDARY,	false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP,	ACT_MP_RELOAD_AIRWALK_SECONDARY_LOOP,	false },
	{ ACT_MP_RELOAD_AIRWALK_END,ACT_MP_RELOAD_AIRWALK_SECONDARY_END,false },

	{ ACT_MP_GESTURE_FLINCH,	ACT_MP_GESTURE_FLINCH_SECONDARY, false },

	{ ACT_MP_GRENADE1_DRAW,		ACT_MP_SECONDARY_GRENADE1_DRAW,	false },
	{ ACT_MP_GRENADE1_IDLE,		ACT_MP_SECONDARY_GRENADE1_IDLE,	false },
	{ ACT_MP_GRENADE1_ATTACK,	ACT_MP_SECONDARY_GRENADE1_ATTACK,	false },
	{ ACT_MP_GRENADE2_DRAW,		ACT_MP_SECONDARY_GRENADE2_DRAW,	false },
	{ ACT_MP_GRENADE2_IDLE,		ACT_MP_SECONDARY_GRENADE2_IDLE,	false },
	{ ACT_MP_GRENADE2_ATTACK,	ACT_MP_SECONDARY_GRENADE2_ATTACK,	false },

	{ ACT_MP_ATTACK_STAND_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_SWIM_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE,	ACT_MP_ATTACK_STAND_GRENADE,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_SECONDARY,	false },
};

acttable_t s_acttablePrimary2[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_PRIMARY,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_PRIMARY,				false },
	{ ACT_MP_DEPLOYED,			ACT_MP_DEPLOYED_PRIMARY,			false },
	{ ACT_MP_CROUCH_DEPLOYED,			ACT_MP_CROUCHWALK_DEPLOYED,	false },
	{ ACT_MP_CROUCH_DEPLOYED_IDLE,		ACT_MP_CROUCH_DEPLOYED_IDLE,	false },
	{ ACT_MP_RUN,				ACT_MP_RUN_PRIMARY,					false },
	{ ACT_MP_WALK,				ACT_MP_WALK_PRIMARY,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_PRIMARY,				false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_PRIMARY,			false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_PRIMARY,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_PRIMARY,			false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_PRIMARY,			false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_PRIMARY,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_PRIMARY,				false },
	{ ACT_MP_SWIM_DEPLOYED,		ACT_MP_SWIM_DEPLOYED_PRIMARY,		false },
	{ ACT_MP_DOUBLEJUMP_CROUCH,	ACT_MP_DOUBLEJUMP_CROUCH_PRIMARY,	false },

	{ ACT_MP_ATTACK_STAND_PRIMARY_SUPER,	ACT_MP_ATTACK_STAND_PRIMARY_SUPER,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARY_SUPER,	ACT_MP_ATTACK_CROUCH_PRIMARY_SUPER,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARY_SUPER,		ACT_MP_ATTACK_SWIM_PRIMARY_SUPER,	false },
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_PRIMARY_ALT,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_PRIMARY_ALT,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_PRIMARY_ALT,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_PRIMARY,		false },

	{ ACT_MP_RELOAD_STAND,		ACT_MP_RELOAD_STAND_PRIMARY_ALT,		false },
	{ ACT_MP_RELOAD_STAND_LOOP,	ACT_MP_RELOAD_STAND_PRIMARY_LOOP_ALT,	false },
	{ ACT_MP_RELOAD_STAND_END,	ACT_MP_RELOAD_STAND_PRIMARY_END_ALT,	false },
	{ ACT_MP_RELOAD_CROUCH,		ACT_MP_RELOAD_CROUCH_PRIMARY_ALT,		false },
	{ ACT_MP_RELOAD_CROUCH_LOOP,ACT_MP_RELOAD_CROUCH_PRIMARY_LOOP_ALT,	false },
	{ ACT_MP_RELOAD_CROUCH_END,	ACT_MP_RELOAD_CROUCH_PRIMARY_END_ALT,	false },
	{ ACT_MP_RELOAD_SWIM,		ACT_MP_RELOAD_SWIM_PRIMARY_ALT,			false },
	{ ACT_MP_RELOAD_SWIM_LOOP,	ACT_MP_RELOAD_SWIM_PRIMARY_LOOP,		false },
	{ ACT_MP_RELOAD_SWIM_END,	ACT_MP_RELOAD_SWIM_PRIMARY_END,			false },
	{ ACT_MP_RELOAD_AIRWALK,	ACT_MP_RELOAD_AIRWALK_PRIMARY_ALT,		false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP,	ACT_MP_RELOAD_AIRWALK_PRIMARY_LOOP_ALT,	false },
	{ ACT_MP_RELOAD_AIRWALK_END,ACT_MP_RELOAD_AIRWALK_PRIMARY_END_ALT,	false },

	{ ACT_MP_ATTACK_STAND_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_SWIM_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE,	ACT_MP_ATTACK_STAND_GRENADE,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_PRIMARY,	false },
};

acttable_t s_acttableSecondary2[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_SECONDARY2,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_SECONDARY2,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_SECONDARY2,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_SECONDARY2,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_SECONDARY2,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_SECONDARY2,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_SECONDARY2,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_SECONDARY2,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_SECONDARY2,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_SECONDARY2,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_SECONDARY2,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_SECONDARY2,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_SECONDARY2,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_SECONDARY2,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_SECONDARY2,	false },

	{ ACT_MP_RELOAD_STAND,		ACT_MP_RELOAD_STAND_SECONDARY2,		false },
	{ ACT_MP_RELOAD_STAND_LOOP,	ACT_MP_RELOAD_STAND_SECONDARY2_LOOP,	false },
	{ ACT_MP_RELOAD_STAND_END,	ACT_MP_RELOAD_STAND_SECONDARY2_END,	false },
	{ ACT_MP_RELOAD_CROUCH,		ACT_MP_RELOAD_CROUCH_SECONDARY2,		false },
	{ ACT_MP_RELOAD_CROUCH_LOOP,ACT_MP_RELOAD_CROUCH_SECONDARY2_LOOP,false },
	{ ACT_MP_RELOAD_CROUCH_END,	ACT_MP_RELOAD_CROUCH_SECONDARY2_END,	false },
	{ ACT_MP_RELOAD_SWIM,		ACT_MP_RELOAD_SWIM_SECONDARY2,		false },
	{ ACT_MP_RELOAD_SWIM_LOOP,	ACT_MP_RELOAD_SWIM_SECONDARY2_LOOP,	false },
	{ ACT_MP_RELOAD_SWIM_END,	ACT_MP_RELOAD_SWIM_SECONDARY2_END,	false },
	{ ACT_MP_RELOAD_AIRWALK,	ACT_MP_RELOAD_AIRWALK_SECONDARY2,	false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP,	ACT_MP_RELOAD_AIRWALK_SECONDARY2_LOOP,	false },
	{ ACT_MP_RELOAD_AIRWALK_END,ACT_MP_RELOAD_AIRWALK_SECONDARY2_END,false },

	{ ACT_MP_ATTACK_STAND_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_SWIM_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE,	ACT_MP_ATTACK_STAND_GRENADE,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_SECONDARY,	false },
};

acttable_t s_acttableMelee[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_MELEE,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_MELEE,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_MELEE,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_MELEE,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_MELEE,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_MELEE,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_MELEE,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_MELEE,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_MELEE,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_MELEE,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_MELEE,				false },
	{ ACT_MP_DOUBLEJUMP_CROUCH,	ACT_MP_DOUBLEJUMP_CROUCH_MELEE,	false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_MELEE,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_MELEE,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_MELEE,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_MELEE,	false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE,	ACT_MP_ATTACK_STAND_MELEE_SECONDARY, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE,	ACT_MP_ATTACK_CROUCH_MELEE_SECONDARY,false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE,		ACT_MP_ATTACK_SWIM_MELEE,		false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE,	ACT_MP_ATTACK_AIRWALK_MELEE,	false },

	{ ACT_MP_GESTURE_FLINCH,	ACT_MP_GESTURE_FLINCH_MELEE, false },

	{ ACT_MP_GRENADE1_DRAW,		ACT_MP_MELEE_GRENADE1_DRAW,	false },
	{ ACT_MP_GRENADE1_IDLE,		ACT_MP_MELEE_GRENADE1_IDLE,	false },
	{ ACT_MP_GRENADE1_ATTACK,	ACT_MP_MELEE_GRENADE1_ATTACK,	false },
	{ ACT_MP_GRENADE2_DRAW,		ACT_MP_MELEE_GRENADE2_DRAW,	false },
	{ ACT_MP_GRENADE2_IDLE,		ACT_MP_MELEE_GRENADE2_IDLE,	false },
	{ ACT_MP_GRENADE2_ATTACK,	ACT_MP_MELEE_GRENADE2_ATTACK,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_MELEE,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_MELEE,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_MELEE,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_MELEE,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_MELEE,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_MELEE,	false },

	{ ACT_MP_FALLING_STOMP,	ACT_MP_FALLING_STOMP_MELEE,	false },
};

acttable_t s_acttableItem1[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_ITEM1,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_ITEM1,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_ITEM1,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_ITEM1,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_ITEM1,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_ITEM1,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_ITEM1,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_ITEM1,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_ITEM1,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_ITEM1,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_ITEM1,				false },
	{ ACT_MP_DOUBLEJUMP_CROUCH,	ACT_MP_DOUBLEJUMP_CROUCH_ITEM1,	false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_ITEM1,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_ITEM1,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_ITEM1,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_ITEM1,	false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE,	ACT_MP_ATTACK_STAND_ITEM1_SECONDARY, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE,	ACT_MP_ATTACK_CROUCH_ITEM1_SECONDARY,false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE,		ACT_MP_ATTACK_SWIM_ITEM1,		false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE,	ACT_MP_ATTACK_AIRWALK_ITEM1,	false },

	{ ACT_MP_DEPLOYED,						ACT_MP_DEPLOYED_ITEM1,	false },
	{ ACT_MP_DEPLOYED_IDLE,					ACT_MP_DEPLOYED_IDLE_ITEM1,	false },
	{ ACT_MP_CROUCH_DEPLOYED,				ACT_MP_CROUCHWALK_DEPLOYED_ITEM1,	false },
	{ ACT_MP_CROUCH_DEPLOYED_IDLE,			ACT_MP_CROUCH_DEPLOYED_IDLE_ITEM1,	false },
	//{ ACT_MP_ATTACK_STAND_PRIMARY_DEPLOYED,	ACT_MP_ATTACK_STAND_PRIMARY_DEPLOYED_ITEM1,	false },
	//{ ACT_MP_ATTACK_CROUCH_PRIMARY_DEPLOYED,ACT_MP_ATTACK_CROUCH_PRIMARY_DEPLOYED_ITEM1,	false },

	{ ACT_MP_GESTURE_FLINCH,	ACT_MP_GESTURE_FLINCH_ITEM1, false },

	{ ACT_MP_GRENADE1_DRAW,		ACT_MP_ITEM1_GRENADE1_DRAW,	false },
	{ ACT_MP_GRENADE1_IDLE,		ACT_MP_ITEM1_GRENADE1_IDLE,	false },
	{ ACT_MP_GRENADE1_ATTACK,	ACT_MP_ITEM1_GRENADE1_ATTACK,	false },
	{ ACT_MP_GRENADE2_DRAW,		ACT_MP_ITEM1_GRENADE2_DRAW,	false },
	{ ACT_MP_GRENADE2_IDLE,		ACT_MP_ITEM1_GRENADE2_IDLE,	false },
	{ ACT_MP_GRENADE2_ATTACK,	ACT_MP_ITEM1_GRENADE2_ATTACK,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_ITEM1,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_ITEM1,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_ITEM1,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_ITEM1,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_ITEM1,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_ITEM1,	false },
};

acttable_t s_acttableItem2[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_ITEM2,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_ITEM2,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_ITEM2,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_ITEM2,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_ITEM2,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_ITEM2,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_ITEM2,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_ITEM2,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_ITEM2,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_ITEM2,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_ITEM2,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_ITEM2,			false },
	{ ACT_MP_DOUBLEJUMP_CROUCH,	ACT_MP_DOUBLEJUMP_CROUCH_ITEM2,	false },

	{ ACT_MP_RELOAD_STAND,		ACT_MP_RELOAD_STAND_ITEM2,		false },
	{ ACT_MP_RELOAD_CROUCH,		ACT_MP_RELOAD_CROUCH_ITEM2,		false },
	{ ACT_MP_RELOAD_SWIM,		ACT_MP_RELOAD_SWIM_ITEM2,		false },
	{ ACT_MP_RELOAD_AIRWALK,	ACT_MP_RELOAD_AIRWALK_ITEM2,	false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_ITEM2,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_ITEM2,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_ITEM2,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_ITEM2,	false },

	{ ACT_MP_DEPLOYED,						ACT_MP_DEPLOYED_ITEM2,	false },
	{ ACT_MP_DEPLOYED_IDLE,					ACT_MP_DEPLOYED_IDLE_ITEM2,	false },
	{ ACT_MP_CROUCH_DEPLOYED,				ACT_MP_CROUCHWALK_DEPLOYED_ITEM2,	false },
	{ ACT_MP_CROUCH_DEPLOYED_IDLE,			ACT_MP_CROUCH_DEPLOYED_IDLE_ITEM2,	false },
	//{ ACT_MP_ATTACK_STAND_PRIMARY_DEPLOYED,	ACT_MP_ATTACK_STAND_PRIMARY_DEPLOYED_ITEM2,	false },
	//{ ACT_MP_ATTACK_CROUCH_PRIMARY_DEPLOYED,ACT_MP_ATTACK_CROUCH_PRIMARY_DEPLOYED_ITEM2,	false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE,	ACT_MP_ATTACK_STAND_ITEM2_SECONDARY, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE,	ACT_MP_ATTACK_CROUCH_ITEM2_SECONDARY,false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE,		ACT_MP_ATTACK_SWIM_ITEM2,		false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE,	ACT_MP_ATTACK_AIRWALK_ITEM2,	false },

	{ ACT_MP_GESTURE_FLINCH,	ACT_MP_GESTURE_FLINCH_ITEM2, false },

	{ ACT_MP_GRENADE1_DRAW,		ACT_MP_ITEM2_GRENADE1_DRAW,	false },
	{ ACT_MP_GRENADE1_IDLE,		ACT_MP_ITEM2_GRENADE1_IDLE,	false },
	{ ACT_MP_GRENADE1_ATTACK,	ACT_MP_ITEM2_GRENADE1_ATTACK,	false },
	{ ACT_MP_GRENADE2_DRAW,		ACT_MP_ITEM2_GRENADE2_DRAW,	false },
	{ ACT_MP_GRENADE2_IDLE,		ACT_MP_ITEM2_GRENADE2_IDLE,	false },
	{ ACT_MP_GRENADE2_ATTACK,	ACT_MP_ITEM2_GRENADE2_ATTACK,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_ITEM2,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_ITEM2,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_ITEM2,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_ITEM2,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_ITEM2,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_ITEM2,	false },
};

acttable_t s_acttableItem3[] =
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_ITEM3,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_ITEM3,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_ITEM3,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_ITEM3,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_ITEM3,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_ITEM3,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_ITEM3,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_ITEM3,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_ITEM3,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_ITEM3,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_ITEM3,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_ITEM3,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_ITEM3,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_ITEM3,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_ITEM3,	false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE,	ACT_MP_ATTACK_STAND_ITEM3_SECONDARY, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE,	ACT_MP_ATTACK_CROUCH_ITEM3_SECONDARY,false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE,		ACT_MP_ATTACK_SWIM_ITEM3,		false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE,	ACT_MP_ATTACK_AIRWALK_ITEM3,	false },

	{ ACT_MP_DEPLOYED,						ACT_MP_DEPLOYED_ITEM3,	false },
	{ ACT_MP_DEPLOYED_IDLE,					ACT_MP_DEPLOYED_IDLE_ITEM3,	false },
	{ ACT_MP_CROUCH_DEPLOYED,				ACT_MP_CROUCHWALK_DEPLOYED_ITEM3,	false },
	{ ACT_MP_CROUCH_DEPLOYED_IDLE,			ACT_MP_CROUCH_DEPLOYED_IDLE_ITEM3,	false },
};

acttable_t s_acttableItem4[] =
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_ITEM4,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_ITEM4,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_ITEM4,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_ITEM4,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_ITEM4,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_ITEM4,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_ITEM4,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_ITEM4,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_ITEM4,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_ITEM4,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_ITEM4,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_ITEM4,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_ITEM4,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_ITEM4,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_ITEM4,	false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE,	ACT_MP_ATTACK_STAND_ITEM4_SECONDARY, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE,	ACT_MP_ATTACK_CROUCH_ITEM4_SECONDARY,false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE,		ACT_MP_ATTACK_SWIM_ITEM4,		false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE,	ACT_MP_ATTACK_AIRWALK_ITEM4,	false },

	{ ACT_MP_DEPLOYED,						ACT_MP_DEPLOYED_ITEM4,	false },
	{ ACT_MP_DEPLOYED_IDLE,					ACT_MP_DEPLOYED_IDLE_ITEM4,	false },
	{ ACT_MP_CROUCH_DEPLOYED,				ACT_MP_CROUCHWALK_DEPLOYED_ITEM4,	false },
	{ ACT_MP_CROUCH_DEPLOYED_IDLE,			ACT_MP_CROUCH_DEPLOYED_IDLE_ITEM4,	false },
};

acttable_t s_acttableBuilding[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_BUILDING,			false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_BUILDING,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_BUILDING,			false },
	{ ACT_MP_WALK,				ACT_MP_WALK_BUILDING,			false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_BUILDING,		false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_BUILDING,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_BUILDING,			false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_BUILDING,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_BUILDING,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_BUILDING,		false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_BUILDING,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_BUILDING,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_BUILDING,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_BUILDING,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_BUILDING,	false },

	{ ACT_MP_ATTACK_STAND_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE_BUILDING,	false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE_BUILDING,	false },
	{ ACT_MP_ATTACK_SWIM_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE_BUILDING,	false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE,	ACT_MP_ATTACK_STAND_GRENADE_BUILDING,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_BUILDING,	false },
};

acttable_t s_acttablePDA[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_PDA,			false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_PDA,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_PDA,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_PDA,			false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_PDA,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_PDA,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_PDA,			false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_PDA,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_PDA,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_PDA,		false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_PDA,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_PDA, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_PDA, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_PDA,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_PDA,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_PDA,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_PDA,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_PDA,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_PDA,	false },
};

acttable_t s_acttableMeleeAllclass[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_MELEE_ALLCLASS,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_MELEE_ALLCLASS,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_MELEE_ALLCLASS,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_MELEE_ALLCLASS,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_MELEE_ALLCLASS,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_MELEE_ALLCLASS,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_MELEE_ALLCLASS,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_MELEE_ALLCLASS,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_MELEE_ALLCLASS,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_MELEE_ALLCLASS,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_MELEE_ALLCLASS,				false },
	{ ACT_MP_DOUBLEJUMP_CROUCH,	ACT_MP_DOUBLEJUMP_CROUCH_MELEE,	false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_MELEE_ALLCLASS,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_MELEE_ALLCLASS,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_MELEE_ALLCLASS,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_MELEE_ALLCLASS,	false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE,	ACT_MP_ATTACK_STAND_MELEE_SECONDARY, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE,	ACT_MP_ATTACK_CROUCH_MELEE_SECONDARY,false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE,		ACT_MP_ATTACK_SWIM_MELEE_ALLCLASS,		false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE,	ACT_MP_ATTACK_AIRWALK_MELEE_ALLCLASS,	false },

	{ ACT_MP_GESTURE_FLINCH,	ACT_MP_GESTURE_FLINCH_MELEE, false },

	{ ACT_MP_GRENADE1_DRAW,		ACT_MP_MELEE_GRENADE1_DRAW,	false },
	{ ACT_MP_GRENADE1_IDLE,		ACT_MP_MELEE_GRENADE1_IDLE,	false },
	{ ACT_MP_GRENADE1_ATTACK,	ACT_MP_MELEE_GRENADE1_ATTACK,	false },
	{ ACT_MP_GRENADE2_DRAW,		ACT_MP_MELEE_GRENADE2_DRAW,	false },
	{ ACT_MP_GRENADE2_IDLE,		ACT_MP_MELEE_GRENADE2_IDLE,	false },
	{ ACT_MP_GRENADE2_ATTACK,	ACT_MP_MELEE_GRENADE2_ATTACK,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_MELEE,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_MELEE,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_MELEE,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_MELEE,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_MELEE,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_MELEE,	false },
};

ConVar mp_forceactivityset( "mp_forceactivityset", "-1", FCVAR_CHEAT|FCVAR_REPLICATED|FCVAR_DEVELOPMENTONLY );

int CTFWeaponBase::GetActivityWeaponRole() const
{
	int iWeaponRole = GetTFWpnData().m_iWeaponType;

	const CEconItemView *pEconItemView = GetAttributeContainer()->GetItem();
	if ( pEconItemView )
	{
		int iMaybeOverrideAnimSlot = pEconItemView->GetAnimationSlot();
		if ( iMaybeOverrideAnimSlot >= 0 )
		{
			iWeaponRole = iMaybeOverrideAnimSlot;
		}
	}

	if ( mp_forceactivityset.GetInt() >= 0 )
	{
		iWeaponRole = mp_forceactivityset.GetInt();
	}

	return iWeaponRole;
}


acttable_t *CTFWeaponBase::ActivityList( int &iActivityCount )
{
	int iWeaponRole = GetActivityWeaponRole();

#ifdef CLIENT_DLL
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pPlayer->IsEnemyPlayer() )
	{
		CTFWeaponBase *pDisguiseWeapon = pPlayer->m_Shared.GetDisguiseWeapon();
		if ( pDisguiseWeapon && pDisguiseWeapon != this )
		{
			return pDisguiseWeapon->ActivityList( iActivityCount );
		}
	}
#endif

	acttable_t *pTable;

	switch( iWeaponRole )
	{
	case TF_WPN_TYPE_PRIMARY:
	default:
		iActivityCount = ARRAYSIZE( s_acttablePrimary );
		pTable = s_acttablePrimary;
		break;
	case TF_WPN_TYPE_SECONDARY:
		iActivityCount = ARRAYSIZE( s_acttableSecondary );
		pTable = s_acttableSecondary;
		break;
	case TF_WPN_TYPE_MELEE:
		iActivityCount = ARRAYSIZE( s_acttableMelee );
		pTable = s_acttableMelee;
		break;
	case TF_WPN_TYPE_BUILDING:
		iActivityCount = ARRAYSIZE( s_acttableBuilding );
		pTable = s_acttableBuilding;
		break;
	case TF_WPN_TYPE_PDA:
		iActivityCount = ARRAYSIZE( s_acttablePDA );
		pTable = s_acttablePDA;
		break;
	case TF_WPN_TYPE_ITEM1:
		iActivityCount = ARRAYSIZE( s_acttableItem1 );
		pTable = s_acttableItem1;
		break;
	case TF_WPN_TYPE_ITEM2:
		iActivityCount = ARRAYSIZE( s_acttableItem2 );
		pTable = s_acttableItem2;
		break;
	case TF_WPN_TYPE_ITEM3:
		iActivityCount = ARRAYSIZE( s_acttableItem3 );
		pTable = s_acttableItem3;
		break;
	case TF_WPN_TYPE_ITEM4:
		iActivityCount = ARRAYSIZE( s_acttableItem4 );
		pTable = s_acttableItem4;
		break;
	case TF_WPN_TYPE_MELEE_ALLCLASS:
		iActivityCount = ARRAYSIZE( s_acttableMeleeAllclass );
		pTable = s_acttableMeleeAllclass;
		break;
	case TF_WPN_TYPE_SECONDARY2:
		iActivityCount = ARRAYSIZE( s_acttableSecondary2 );
		pTable = s_acttableSecondary2;
		break;
	case TF_WPN_TYPE_PRIMARY2:
		iActivityCount = ARRAYSIZE( s_acttablePrimary2 );
		pTable = s_acttablePrimary2;
		break;
	}

	return pTable;
}

typedef struct
{
	int			baseAct;
	int			weaponAct;
	int			weaponRole;
} viewmodelacttable_t;
// Remaps viewmodel activities to specific ones for the weapon role.
// Needed this for weapons that bonemerge themselves to the hand models to create their viewmodel.
// The hand model needs to have all the animations, and be able to choose the right anims to play for the active weapon.
// We use this acttable to remap the base viewmodel anims to the right one for the weapon.
viewmodelacttable_t s_viewmodelacttable[] = 
{
	{ ACT_VM_DRAW,						ACT_PRIMARY_VM_DRAW,				TF_WPN_TYPE_PRIMARY		},
	{ ACT_VM_HOLSTER,					ACT_PRIMARY_VM_HOLSTER,				TF_WPN_TYPE_PRIMARY		},
	{ ACT_VM_IDLE,						ACT_PRIMARY_VM_IDLE,				TF_WPN_TYPE_PRIMARY		},
	{ ACT_VM_PULLBACK,					ACT_PRIMARY_VM_PULLBACK,			TF_WPN_TYPE_PRIMARY		},
	{ ACT_VM_PRIMARYATTACK,				ACT_PRIMARY_VM_PRIMARYATTACK,		TF_WPN_TYPE_PRIMARY		},
	{ ACT_VM_SECONDARYATTACK,			ACT_PRIMARY_VM_SECONDARYATTACK,		TF_WPN_TYPE_PRIMARY		},
	{ ACT_VM_RELOAD,					ACT_PRIMARY_VM_RELOAD,				TF_WPN_TYPE_PRIMARY		},
	{ ACT_RELOAD_START,					ACT_PRIMARY_RELOAD_START,			TF_WPN_TYPE_PRIMARY		},
	{ ACT_RELOAD_FINISH,				ACT_PRIMARY_RELOAD_FINISH,			TF_WPN_TYPE_PRIMARY		},
	{ ACT_VM_DRYFIRE,					ACT_PRIMARY_VM_DRYFIRE,				TF_WPN_TYPE_PRIMARY		},
	{ ACT_VM_IDLE_TO_LOWERED,			ACT_PRIMARY_VM_IDLE_TO_LOWERED,		TF_WPN_TYPE_PRIMARY		},
	{ ACT_VM_IDLE_LOWERED,				ACT_PRIMARY_VM_IDLE_LOWERED,		TF_WPN_TYPE_PRIMARY		},
	{ ACT_VM_LOWERED_TO_IDLE,			ACT_PRIMARY_VM_LOWERED_TO_IDLE,		TF_WPN_TYPE_PRIMARY		},
	{ ACT_MP_ATTACK_STAND_PREFIRE,		ACT_PRIMARY_ATTACK_STAND_PREFIRE,	TF_WPN_TYPE_PRIMARY		},
	{ ACT_MP_ATTACK_STAND_POSTFIRE,		ACT_PRIMARY_ATTACK_STAND_POSTFIRE,	TF_WPN_TYPE_PRIMARY		},
	{ ACT_MP_ATTACK_STAND_STARTFIRE,	ACT_PRIMARY_ATTACK_STAND_STARTFIRE,	TF_WPN_TYPE_PRIMARY		},
	{ ACT_MP_ATTACK_CROUCH_PREFIRE,		ACT_PRIMARY_ATTACK_CROUCH_PREFIRE,	TF_WPN_TYPE_PRIMARY		},
	{ ACT_MP_ATTACK_CROUCH_POSTFIRE,	ACT_PRIMARY_ATTACK_CROUCH_POSTFIRE,	TF_WPN_TYPE_PRIMARY		},
	{ ACT_MP_ATTACK_SWIM_PREFIRE,		ACT_PRIMARY_ATTACK_SWIM_PREFIRE,	TF_WPN_TYPE_PRIMARY		},
	{ ACT_MP_ATTACK_SWIM_POSTFIRE,		ACT_PRIMARY_ATTACK_SWIM_POSTFIRE,	TF_WPN_TYPE_PRIMARY		},

	{ ACT_VM_DRAW,						ACT_SECONDARY_VM_DRAW,				TF_WPN_TYPE_SECONDARY	},
	{ ACT_VM_HOLSTER,					ACT_SECONDARY_VM_HOLSTER,			TF_WPN_TYPE_SECONDARY	},
	{ ACT_VM_IDLE,						ACT_SECONDARY_VM_IDLE,				TF_WPN_TYPE_SECONDARY	},
	{ ACT_VM_PULLBACK,					ACT_SECONDARY_VM_PULLBACK,			TF_WPN_TYPE_SECONDARY	},
	{ ACT_VM_PRIMARYATTACK,				ACT_SECONDARY_VM_PRIMARYATTACK,		TF_WPN_TYPE_SECONDARY	},
	{ ACT_VM_SECONDARYATTACK,			ACT_SECONDARY_VM_SECONDARYATTACK,	TF_WPN_TYPE_SECONDARY	},
	{ ACT_VM_RELOAD,					ACT_SECONDARY_VM_RELOAD,			TF_WPN_TYPE_SECONDARY	},
	{ ACT_RELOAD_START,					ACT_SECONDARY_RELOAD_START,			TF_WPN_TYPE_SECONDARY	},
	{ ACT_RELOAD_FINISH,				ACT_SECONDARY_RELOAD_FINISH,		TF_WPN_TYPE_SECONDARY	},
	{ ACT_VM_DRYFIRE,					ACT_SECONDARY_VM_DRYFIRE,			TF_WPN_TYPE_SECONDARY	},
	{ ACT_VM_IDLE_TO_LOWERED,			ACT_SECONDARY_VM_IDLE_TO_LOWERED,	TF_WPN_TYPE_SECONDARY	},
	{ ACT_VM_IDLE_LOWERED,				ACT_SECONDARY_VM_IDLE_LOWERED,		TF_WPN_TYPE_SECONDARY	},
	{ ACT_VM_LOWERED_TO_IDLE,			ACT_SECONDARY_VM_LOWERED_TO_IDLE,	TF_WPN_TYPE_SECONDARY	},
	{ ACT_MP_ATTACK_STAND_PREFIRE,		ACT_SECONDARY_ATTACK_STAND_PREFIRE,	TF_WPN_TYPE_SECONDARY		},
	{ ACT_MP_ATTACK_STAND_POSTFIRE,		ACT_SECONDARY_ATTACK_STAND_POSTFIRE,	TF_WPN_TYPE_SECONDARY		},
	{ ACT_MP_ATTACK_STAND_STARTFIRE,	ACT_SECONDARY_ATTACK_STAND_STARTFIRE,	TF_WPN_TYPE_SECONDARY		},
	{ ACT_MP_ATTACK_CROUCH_PREFIRE,		ACT_SECONDARY_ATTACK_CROUCH_PREFIRE,	TF_WPN_TYPE_SECONDARY		},
	{ ACT_MP_ATTACK_CROUCH_POSTFIRE,	ACT_SECONDARY_ATTACK_CROUCH_POSTFIRE,	TF_WPN_TYPE_SECONDARY		},
	{ ACT_MP_ATTACK_SWIM_PREFIRE,		ACT_SECONDARY_ATTACK_SWIM_PREFIRE,	TF_WPN_TYPE_SECONDARY		},
	{ ACT_MP_ATTACK_SWIM_POSTFIRE,		ACT_SECONDARY_ATTACK_SWIM_POSTFIRE,	TF_WPN_TYPE_SECONDARY		},

	{ ACT_VM_DRAW,						ACT_MELEE_VM_DRAW,					TF_WPN_TYPE_MELEE		},
	{ ACT_VM_HOLSTER,					ACT_MELEE_VM_HOLSTER,				TF_WPN_TYPE_MELEE		},
	{ ACT_VM_IDLE,						ACT_MELEE_VM_IDLE,					TF_WPN_TYPE_MELEE		},
	{ ACT_VM_PULLBACK,					ACT_MELEE_VM_PULLBACK,				TF_WPN_TYPE_MELEE		},
	{ ACT_VM_PRIMARYATTACK,				ACT_MELEE_VM_PRIMARYATTACK,			TF_WPN_TYPE_MELEE		},
	{ ACT_VM_SECONDARYATTACK,			ACT_MELEE_VM_SECONDARYATTACK,		TF_WPN_TYPE_MELEE		},
	{ ACT_VM_RELOAD,					ACT_MELEE_VM_RELOAD,				TF_WPN_TYPE_MELEE		},
	{ ACT_VM_DRYFIRE,					ACT_MELEE_VM_DRYFIRE,				TF_WPN_TYPE_MELEE		},
	{ ACT_VM_IDLE_TO_LOWERED,			ACT_MELEE_VM_IDLE_TO_LOWERED,		TF_WPN_TYPE_MELEE		},
	{ ACT_VM_IDLE_LOWERED,				ACT_MELEE_VM_IDLE_LOWERED,			TF_WPN_TYPE_MELEE		},
	{ ACT_VM_LOWERED_TO_IDLE,			ACT_MELEE_VM_LOWERED_TO_IDLE,		TF_WPN_TYPE_MELEE		},
	{ ACT_VM_HITCENTER,					ACT_MELEE_VM_HITCENTER,				TF_WPN_TYPE_MELEE		},
	{ ACT_VM_SWINGHARD,					ACT_MELEE_VM_SWINGHARD,				TF_WPN_TYPE_MELEE		},
	{ ACT_MP_ATTACK_STAND_PREFIRE,		ACT_MELEE_ATTACK_STAND_PREFIRE,		TF_WPN_TYPE_MELEE		},
	{ ACT_MP_ATTACK_STAND_POSTFIRE,		ACT_MELEE_ATTACK_STAND_POSTFIRE,	TF_WPN_TYPE_MELEE		},
	{ ACT_MP_ATTACK_STAND_STARTFIRE,	ACT_MELEE_ATTACK_STAND_STARTFIRE,	TF_WPN_TYPE_MELEE		},
	{ ACT_MP_ATTACK_CROUCH_PREFIRE,		ACT_MELEE_ATTACK_CROUCH_PREFIRE,	TF_WPN_TYPE_MELEE		},
	{ ACT_MP_ATTACK_CROUCH_POSTFIRE,	ACT_MELEE_ATTACK_CROUCH_POSTFIRE,	TF_WPN_TYPE_MELEE		},
	{ ACT_MP_ATTACK_SWIM_PREFIRE,		ACT_MELEE_ATTACK_SWIM_PREFIRE,		TF_WPN_TYPE_MELEE		},
	{ ACT_MP_ATTACK_SWIM_POSTFIRE,		ACT_MELEE_ATTACK_SWIM_POSTFIRE,		TF_WPN_TYPE_MELEE		},

	// Scout Pack -- Bat Special State Support
	{ ACT_VM_DRAW_SPECIAL,				ACT_VM_DRAW_SPECIAL,				TF_WPN_TYPE_MELEE		},
	{ ACT_VM_HOLSTER_SPECIAL,			ACT_VM_HOLSTER_SPECIAL,				TF_WPN_TYPE_MELEE		},
	{ ACT_VM_IDLE_SPECIAL,				ACT_VM_IDLE_SPECIAL,				TF_WPN_TYPE_MELEE		},
	{ ACT_VM_PULLBACK_SPECIAL,			ACT_VM_PULLBACK_SPECIAL,			TF_WPN_TYPE_MELEE		},
	{ ACT_VM_PRIMARYATTACK_SPECIAL,		ACT_VM_PRIMARYATTACK_SPECIAL,		TF_WPN_TYPE_MELEE		},
	{ ACT_VM_SECONDARYATTACK_SPECIAL,	ACT_VM_SECONDARYATTACK_SPECIAL,		TF_WPN_TYPE_MELEE		},
	{ ACT_VM_HITCENTER_SPECIAL,			ACT_VM_HITCENTER_SPECIAL,			TF_WPN_TYPE_MELEE		},
	{ ACT_VM_SWINGHARD_SPECIAL,			ACT_VM_SWINGHARD_SPECIAL,			TF_WPN_TYPE_MELEE		},
	{ ACT_VM_IDLE_TO_LOWERED_SPECIAL,	ACT_VM_IDLE_TO_LOWERED_SPECIAL,		TF_WPN_TYPE_MELEE		},
	{ ACT_VM_IDLE_LOWERED_SPECIAL,		ACT_VM_IDLE_LOWERED_SPECIAL,		TF_WPN_TYPE_MELEE		},
	{ ACT_VM_LOWERED_TO_IDLE_SPECIAL,	ACT_VM_LOWERED_TO_IDLE_SPECIAL,		TF_WPN_TYPE_MELEE		},

	// Spy Pack -- New Knife Anims
	{ ACT_BACKSTAB_VM_DOWN,				ACT_BACKSTAB_VM_DOWN,				TF_WPN_TYPE_MELEE		},
	{ ACT_BACKSTAB_VM_UP,				ACT_BACKSTAB_VM_UP,					TF_WPN_TYPE_MELEE		},
	{ ACT_BACKSTAB_VM_IDLE,				ACT_BACKSTAB_VM_IDLE,				TF_WPN_TYPE_MELEE		},

	{ ACT_VM_DRAW,						ACT_PDA_VM_DRAW,					TF_WPN_TYPE_PDA			},
	{ ACT_VM_HOLSTER,					ACT_PDA_VM_HOLSTER,					TF_WPN_TYPE_PDA			},
	{ ACT_VM_IDLE,						ACT_PDA_VM_IDLE,					TF_WPN_TYPE_PDA			},
	{ ACT_VM_PULLBACK,					ACT_PDA_VM_PULLBACK,				TF_WPN_TYPE_PDA			},
	{ ACT_VM_PRIMARYATTACK,				ACT_PDA_VM_PRIMARYATTACK,			TF_WPN_TYPE_PDA			},
	{ ACT_VM_SECONDARYATTACK,			ACT_PDA_VM_SECONDARYATTACK,			TF_WPN_TYPE_PDA			},
	{ ACT_VM_RELOAD,					ACT_PDA_VM_RELOAD,					TF_WPN_TYPE_PDA			},
	{ ACT_VM_DRYFIRE,					ACT_PDA_VM_DRYFIRE,					TF_WPN_TYPE_PDA			},
	{ ACT_VM_IDLE_TO_LOWERED,			ACT_PDA_VM_IDLE_TO_LOWERED,			TF_WPN_TYPE_PDA			},
	{ ACT_VM_IDLE_LOWERED,				ACT_PDA_VM_IDLE_LOWERED,			TF_WPN_TYPE_PDA			},
	{ ACT_VM_LOWERED_TO_IDLE,			ACT_PDA_VM_LOWERED_TO_IDLE,			TF_WPN_TYPE_PDA			},

	// ITEM1
	{ ACT_VM_DRAW,						ACT_ITEM1_VM_DRAW,					TF_WPN_TYPE_ITEM1		},
	{ ACT_VM_HOLSTER,					ACT_ITEM1_VM_HOLSTER,				TF_WPN_TYPE_ITEM1		},
	{ ACT_VM_IDLE,						ACT_ITEM1_VM_IDLE,					TF_WPN_TYPE_ITEM1		},
	{ ACT_VM_PULLBACK,					ACT_ITEM1_VM_PULLBACK,				TF_WPN_TYPE_ITEM1		},
	{ ACT_VM_PRIMARYATTACK,				ACT_ITEM1_VM_PRIMARYATTACK,			TF_WPN_TYPE_ITEM1		},
	{ ACT_VM_SECONDARYATTACK,			ACT_ITEM1_VM_SECONDARYATTACK,		TF_WPN_TYPE_ITEM1		},
	{ ACT_VM_RELOAD,					ACT_ITEM1_VM_RELOAD,				TF_WPN_TYPE_ITEM1		},
	{ ACT_RELOAD_START,					ACT_ITEM1_RELOAD_START,				TF_WPN_TYPE_ITEM1		},
	{ ACT_RELOAD_FINISH,				ACT_ITEM1_RELOAD_FINISH,			TF_WPN_TYPE_ITEM1		},
	{ ACT_VM_DRYFIRE,					ACT_ITEM1_VM_DRYFIRE,				TF_WPN_TYPE_ITEM1		},
	{ ACT_VM_IDLE_TO_LOWERED,			ACT_ITEM1_VM_IDLE_TO_LOWERED,		TF_WPN_TYPE_ITEM1		},
	{ ACT_VM_IDLE_LOWERED,				ACT_ITEM1_VM_IDLE_LOWERED,			TF_WPN_TYPE_ITEM1		},
	{ ACT_VM_LOWERED_TO_IDLE,			ACT_ITEM1_VM_LOWERED_TO_IDLE,		TF_WPN_TYPE_ITEM1		},
	{ ACT_MP_ATTACK_STAND_PREFIRE,		ACT_ITEM1_ATTACK_STAND_PREFIRE,	TF_WPN_TYPE_ITEM1		},
	{ ACT_MP_ATTACK_STAND_POSTFIRE,		ACT_ITEM1_ATTACK_STAND_POSTFIRE,	TF_WPN_TYPE_ITEM1		},
	{ ACT_MP_ATTACK_STAND_STARTFIRE,	ACT_ITEM1_ATTACK_STAND_STARTFIRE,	TF_WPN_TYPE_ITEM1		},
	{ ACT_MP_ATTACK_CROUCH_PREFIRE,		ACT_ITEM1_ATTACK_CROUCH_PREFIRE,	TF_WPN_TYPE_ITEM1		},
	{ ACT_MP_ATTACK_CROUCH_POSTFIRE,	ACT_ITEM1_ATTACK_CROUCH_POSTFIRE,	TF_WPN_TYPE_ITEM1		},
	{ ACT_MP_ATTACK_SWIM_PREFIRE,		ACT_ITEM1_ATTACK_SWIM_PREFIRE,	TF_WPN_TYPE_ITEM1		},
	{ ACT_MP_ATTACK_SWIM_POSTFIRE,		ACT_ITEM1_ATTACK_SWIM_POSTFIRE,	TF_WPN_TYPE_ITEM1		},

	// ITEM2
	{ ACT_VM_DRAW,						ACT_ITEM2_VM_DRAW,					TF_WPN_TYPE_ITEM2		},
	{ ACT_VM_HOLSTER,					ACT_ITEM2_VM_HOLSTER,				TF_WPN_TYPE_ITEM2		},
	{ ACT_VM_IDLE,						ACT_ITEM2_VM_IDLE,					TF_WPN_TYPE_ITEM2		},
	{ ACT_VM_PULLBACK,					ACT_ITEM2_VM_PULLBACK,				TF_WPN_TYPE_ITEM2		},
	{ ACT_VM_PRIMARYATTACK,				ACT_ITEM2_VM_PRIMARYATTACK,			TF_WPN_TYPE_ITEM2		},
	{ ACT_VM_SECONDARYATTACK,			ACT_ITEM2_VM_SECONDARYATTACK,		TF_WPN_TYPE_ITEM2		},
	{ ACT_VM_RELOAD,					ACT_ITEM2_VM_RELOAD,				TF_WPN_TYPE_ITEM2		},
	{ ACT_VM_DRYFIRE,					ACT_ITEM2_VM_DRYFIRE,				TF_WPN_TYPE_ITEM2		},
	{ ACT_VM_IDLE_TO_LOWERED,			ACT_ITEM2_VM_IDLE_TO_LOWERED,		TF_WPN_TYPE_ITEM2		},
	{ ACT_VM_IDLE_LOWERED,				ACT_ITEM2_VM_IDLE_LOWERED,			TF_WPN_TYPE_ITEM2		},
	{ ACT_VM_LOWERED_TO_IDLE,			ACT_ITEM2_VM_LOWERED_TO_IDLE,		TF_WPN_TYPE_ITEM2		},
	{ ACT_MP_ATTACK_STAND_PREFIRE,		ACT_ITEM2_ATTACK_STAND_PREFIRE,	TF_WPN_TYPE_ITEM2		},
	{ ACT_MP_ATTACK_STAND_POSTFIRE,		ACT_ITEM2_ATTACK_STAND_POSTFIRE,	TF_WPN_TYPE_ITEM2		},
	{ ACT_MP_ATTACK_STAND_STARTFIRE,	ACT_ITEM2_ATTACK_STAND_STARTFIRE,	TF_WPN_TYPE_ITEM2		},
	{ ACT_MP_ATTACK_CROUCH_PREFIRE,		ACT_ITEM2_ATTACK_CROUCH_PREFIRE,	TF_WPN_TYPE_ITEM2		},
	{ ACT_MP_ATTACK_CROUCH_POSTFIRE,	ACT_ITEM2_ATTACK_CROUCH_POSTFIRE,	TF_WPN_TYPE_ITEM2		},
	{ ACT_MP_ATTACK_SWIM_PREFIRE,		ACT_ITEM2_ATTACK_SWIM_PREFIRE,	TF_WPN_TYPE_ITEM2		},
	{ ACT_MP_ATTACK_SWIM_POSTFIRE,		ACT_ITEM2_ATTACK_SWIM_POSTFIRE,	TF_WPN_TYPE_ITEM2		},

	// ITEM3
	{ ACT_VM_DRAW,						ACT_ITEM3_VM_DRAW,					TF_WPN_TYPE_ITEM3 },
	{ ACT_VM_HOLSTER,					ACT_ITEM3_VM_HOLSTER,				TF_WPN_TYPE_ITEM3 },
	{ ACT_VM_IDLE,						ACT_ITEM3_VM_IDLE,					TF_WPN_TYPE_ITEM3 },
	{ ACT_VM_PULLBACK,					ACT_ITEM3_VM_PULLBACK,				TF_WPN_TYPE_ITEM3 },
	{ ACT_VM_PRIMARYATTACK,				ACT_ITEM3_VM_PRIMARYATTACK,			TF_WPN_TYPE_ITEM3 },
	{ ACT_VM_SECONDARYATTACK,			ACT_ITEM3_VM_SECONDARYATTACK,		TF_WPN_TYPE_ITEM3 },
	{ ACT_VM_RELOAD,					ACT_ITEM3_VM_RELOAD,				TF_WPN_TYPE_ITEM3 },
	{ ACT_VM_DRYFIRE,					ACT_ITEM3_VM_DRYFIRE,				TF_WPN_TYPE_ITEM3 },
	{ ACT_VM_IDLE_TO_LOWERED,			ACT_ITEM3_VM_IDLE_TO_LOWERED,		TF_WPN_TYPE_ITEM3 },
	{ ACT_VM_IDLE_LOWERED,				ACT_ITEM3_VM_IDLE_LOWERED,			TF_WPN_TYPE_ITEM3 },
	{ ACT_VM_LOWERED_TO_IDLE,			ACT_ITEM3_VM_LOWERED_TO_IDLE,		TF_WPN_TYPE_ITEM3 },
	{ ACT_MP_ATTACK_STAND_PREFIRE,		ACT_ITEM3_ATTACK_STAND_PREFIRE,		TF_WPN_TYPE_ITEM3 },
	{ ACT_MP_ATTACK_STAND_POSTFIRE,		ACT_ITEM3_ATTACK_STAND_POSTFIRE,	TF_WPN_TYPE_ITEM3 },
	{ ACT_MP_ATTACK_STAND_STARTFIRE,	ACT_ITEM3_ATTACK_STAND_STARTFIRE,	TF_WPN_TYPE_ITEM3 },
	{ ACT_MP_ATTACK_CROUCH_PREFIRE,		ACT_ITEM3_ATTACK_CROUCH_PREFIRE,	TF_WPN_TYPE_ITEM3 },
	{ ACT_MP_ATTACK_CROUCH_POSTFIRE,	ACT_ITEM3_ATTACK_CROUCH_POSTFIRE,	TF_WPN_TYPE_ITEM3 },
	{ ACT_MP_ATTACK_SWIM_PREFIRE,		ACT_ITEM3_ATTACK_SWIM_PREFIRE,		TF_WPN_TYPE_ITEM3 },
	{ ACT_MP_ATTACK_SWIM_POSTFIRE,		ACT_ITEM3_ATTACK_SWIM_POSTFIRE,		TF_WPN_TYPE_ITEM3 },

	// ITEM4
	{ ACT_VM_DRAW,						ACT_ITEM4_VM_DRAW,					TF_WPN_TYPE_ITEM4 },
	{ ACT_VM_HOLSTER,					ACT_ITEM4_VM_HOLSTER,				TF_WPN_TYPE_ITEM4 },
	{ ACT_VM_IDLE,						ACT_ITEM4_VM_IDLE,					TF_WPN_TYPE_ITEM4 },
	{ ACT_VM_PULLBACK,					ACT_ITEM4_VM_PULLBACK,				TF_WPN_TYPE_ITEM4 },
	{ ACT_VM_PRIMARYATTACK,				ACT_ITEM4_VM_PRIMARYATTACK,			TF_WPN_TYPE_ITEM4 },
	{ ACT_VM_SECONDARYATTACK,			ACT_ITEM4_VM_SECONDARYATTACK,		TF_WPN_TYPE_ITEM4 },
	{ ACT_VM_RELOAD,					ACT_ITEM4_VM_RELOAD,				TF_WPN_TYPE_ITEM4 },
	{ ACT_VM_DRYFIRE,					ACT_ITEM4_VM_DRYFIRE,				TF_WPN_TYPE_ITEM4 },
	{ ACT_VM_IDLE_TO_LOWERED,			ACT_ITEM4_VM_IDLE_TO_LOWERED,		TF_WPN_TYPE_ITEM4 },
	{ ACT_VM_IDLE_LOWERED,				ACT_ITEM4_VM_IDLE_LOWERED,			TF_WPN_TYPE_ITEM4 },
	{ ACT_VM_LOWERED_TO_IDLE,			ACT_ITEM4_VM_LOWERED_TO_IDLE,		TF_WPN_TYPE_ITEM4 },
	{ ACT_MP_ATTACK_STAND_PREFIRE,		ACT_ITEM4_ATTACK_STAND_PREFIRE,		TF_WPN_TYPE_ITEM4 },
	{ ACT_MP_ATTACK_STAND_POSTFIRE,		ACT_ITEM4_ATTACK_STAND_POSTFIRE,	TF_WPN_TYPE_ITEM4 },
	{ ACT_MP_ATTACK_STAND_STARTFIRE,	ACT_ITEM4_ATTACK_STAND_STARTFIRE,	TF_WPN_TYPE_ITEM4 },
	{ ACT_MP_ATTACK_CROUCH_PREFIRE,		ACT_ITEM4_ATTACK_CROUCH_PREFIRE,	TF_WPN_TYPE_ITEM4 },
	{ ACT_MP_ATTACK_CROUCH_POSTFIRE,	ACT_ITEM4_ATTACK_CROUCH_POSTFIRE,	TF_WPN_TYPE_ITEM4 },
	{ ACT_MP_ATTACK_SWIM_PREFIRE,		ACT_ITEM4_ATTACK_SWIM_PREFIRE,		TF_WPN_TYPE_ITEM4 },
	{ ACT_MP_ATTACK_SWIM_POSTFIRE,		ACT_ITEM4_ATTACK_SWIM_POSTFIRE,		TF_WPN_TYPE_ITEM4 },

	{ ACT_VM_DRAW,						ACT_MELEE_ALLCLASS_VM_DRAW,					TF_WPN_TYPE_MELEE_ALLCLASS		},
	{ ACT_VM_HOLSTER,					ACT_MELEE_ALLCLASS_VM_HOLSTER,				TF_WPN_TYPE_MELEE_ALLCLASS		},
	{ ACT_VM_IDLE,						ACT_MELEE_ALLCLASS_VM_IDLE,					TF_WPN_TYPE_MELEE_ALLCLASS		},
	{ ACT_VM_PULLBACK,					ACT_MELEE_ALLCLASS_VM_PULLBACK,				TF_WPN_TYPE_MELEE_ALLCLASS		},
	{ ACT_VM_PRIMARYATTACK,				ACT_MELEE_ALLCLASS_VM_PRIMARYATTACK,		TF_WPN_TYPE_MELEE_ALLCLASS		},
	{ ACT_VM_SECONDARYATTACK,			ACT_MELEE_ALLCLASS_VM_SECONDARYATTACK,		TF_WPN_TYPE_MELEE_ALLCLASS		},
	{ ACT_VM_RELOAD,					ACT_MELEE_ALLCLASS_VM_RELOAD,				TF_WPN_TYPE_MELEE_ALLCLASS		},
	{ ACT_VM_DRYFIRE,					ACT_MELEE_ALLCLASS_VM_DRYFIRE,				TF_WPN_TYPE_MELEE_ALLCLASS		},
	{ ACT_VM_IDLE_TO_LOWERED,			ACT_MELEE_ALLCLASS_VM_IDLE_TO_LOWERED,		TF_WPN_TYPE_MELEE_ALLCLASS		},
	{ ACT_VM_IDLE_LOWERED,				ACT_MELEE_ALLCLASS_VM_IDLE_LOWERED,			TF_WPN_TYPE_MELEE_ALLCLASS		},
	{ ACT_VM_LOWERED_TO_IDLE,			ACT_MELEE_ALLCLASS_VM_LOWERED_TO_IDLE,		TF_WPN_TYPE_MELEE_ALLCLASS		},
	{ ACT_VM_HITCENTER,					ACT_MELEE_ALLCLASS_VM_HITCENTER,			TF_WPN_TYPE_MELEE_ALLCLASS		},
	{ ACT_VM_SWINGHARD,					ACT_MELEE_ALLCLASS_VM_SWINGHARD,			TF_WPN_TYPE_MELEE_ALLCLASS		},

	{ ACT_VM_DRAW,						ACT_SECONDARY2_VM_DRAW,					TF_WPN_TYPE_SECONDARY2	},
	{ ACT_VM_HOLSTER,					ACT_SECONDARY2_VM_HOLSTER,				TF_WPN_TYPE_SECONDARY2	},
	{ ACT_VM_IDLE,						ACT_SECONDARY2_VM_IDLE,					TF_WPN_TYPE_SECONDARY2	},
	{ ACT_VM_PULLBACK,					ACT_SECONDARY2_VM_PULLBACK,				TF_WPN_TYPE_SECONDARY2	},
	{ ACT_VM_PRIMARYATTACK,				ACT_SECONDARY2_VM_PRIMARYATTACK,		TF_WPN_TYPE_SECONDARY2	},
	{ ACT_VM_RELOAD,					ACT_SECONDARY2_VM_RELOAD,				TF_WPN_TYPE_SECONDARY2	},
	{ ACT_RELOAD_START,					ACT_SECONDARY2_RELOAD_START,			TF_WPN_TYPE_SECONDARY2	},
	{ ACT_RELOAD_FINISH,				ACT_SECONDARY2_RELOAD_FINISH,			TF_WPN_TYPE_SECONDARY2	},
	{ ACT_VM_DRYFIRE,					ACT_SECONDARY2_VM_DRYFIRE,				TF_WPN_TYPE_SECONDARY2	},
	{ ACT_VM_IDLE_TO_LOWERED,			ACT_SECONDARY2_VM_IDLE_TO_LOWERED,		TF_WPN_TYPE_SECONDARY2	},
	{ ACT_VM_IDLE_LOWERED,				ACT_SECONDARY2_VM_IDLE_LOWERED,			TF_WPN_TYPE_SECONDARY2	},
	{ ACT_VM_LOWERED_TO_IDLE,			ACT_SECONDARY2_VM_LOWERED_TO_IDLE,		TF_WPN_TYPE_SECONDARY2	},

	{ ACT_VM_DRAW,						ACT_PRIMARY_VM_DRAW,					TF_WPN_TYPE_PRIMARY2		},
	{ ACT_VM_HOLSTER,					ACT_PRIMARY_VM_HOLSTER,					TF_WPN_TYPE_PRIMARY2		},
	{ ACT_VM_IDLE,						ACT_PRIMARY_VM_IDLE,					TF_WPN_TYPE_PRIMARY2		},
	{ ACT_VM_PULLBACK,					ACT_PRIMARY_VM_PULLBACK,				TF_WPN_TYPE_PRIMARY2		},
	{ ACT_VM_PRIMARYATTACK,				ACT_PRIMARY_VM_PRIMARYATTACK,			TF_WPN_TYPE_PRIMARY2		},
	{ ACT_VM_RELOAD,					ACT_PRIMARY_VM_RELOAD_3,				TF_WPN_TYPE_PRIMARY2		},
	{ ACT_RELOAD_START,					ACT_PRIMARY_RELOAD_START_3,				TF_WPN_TYPE_PRIMARY2		},
	{ ACT_RELOAD_FINISH,				ACT_PRIMARY_RELOAD_FINISH_3,			TF_WPN_TYPE_PRIMARY2		},
	{ ACT_VM_DRYFIRE,					ACT_PRIMARY_VM_DRYFIRE,					TF_WPN_TYPE_PRIMARY2		},
	{ ACT_VM_IDLE_TO_LOWERED,			ACT_PRIMARY_VM_IDLE_TO_LOWERED,			TF_WPN_TYPE_PRIMARY2		},
	{ ACT_VM_IDLE_LOWERED,				ACT_PRIMARY_VM_IDLE_LOWERED,			TF_WPN_TYPE_PRIMARY2		},
	{ ACT_VM_LOWERED_TO_IDLE,			ACT_PRIMARY_VM_LOWERED_TO_IDLE,			TF_WPN_TYPE_PRIMARY2		},
};

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
Activity CTFWeaponBase::TranslateViewmodelHandActivityInternal( Activity actBase )
{
	CEconItemView *pEconItemView = GetAttributeContainer()->GetItem();
	if ( pEconItemView && pEconItemView->IsValid() && GetOwnerEntity() )
	{
		Activity translatedActivity = pEconItemView->GetStaticData()->GetActivityOverride( GetOwnerEntity()->GetTeamNumber(), actBase );
		if ( translatedActivity != actBase )
			return translatedActivity;
	}

	int iWeaponRole = GetViewModelWeaponRole();

	if ( pEconItemView )
	{
		int iMaybeOverrideAnimSlot = pEconItemView->GetAnimationSlot();
		if ( iMaybeOverrideAnimSlot >= 0 )
		{
			iWeaponRole = iMaybeOverrideAnimSlot;
		}
	}

	viewmodelacttable_t *pTable = s_viewmodelacttable;
	for ( int i = 0; i < ARRAYSIZE(s_viewmodelacttable); i++ )
	{
		const viewmodelacttable_t &act = pTable[i];
		if ( actBase == act.baseAct && act.weaponRole == iWeaponRole )
			return (Activity)act.weaponAct;
	}
	return actBase;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
CBasePlayer *CTFWeaponBase::GetPlayerOwner() const
{
	return dynamic_cast<CBasePlayer*>( GetOwner() );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
CTFPlayer *CTFWeaponBase::GetTFPlayerOwner() const
{
	return dynamic_cast<CTFPlayer*>( GetOwner() );
}

#ifdef CLIENT_DLL
// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
C_BaseEntity *CTFWeaponBase::GetWeaponForEffect()
{
	return GetAppropriateWorldOrViewModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ShadowType_t CTFWeaponBase::ShadowCastType( void ) 
{
	// Some weapons (fists) don't actually get set to NODRAW when holstered so we
	// need some extra checks
	if ( IsEffectActive( EF_NODRAW | EF_NOSHADOW ) || m_iState != WEAPON_IS_ACTIVE )
		return SHADOWS_NONE;

	return BaseClass::ShadowCastType();
}
#endif

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBase::CanAttack()
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( pPlayer )
		return pPlayer->CanAttack( GetCanAttackFlags() );

	return false;
}


//-----------------------------------------------------------------------------
bool CTFWeaponBase::CanFireCriticalShot( bool bIsHeadshot, CBaseEntity *pTarget /*= NULL*/ )
{
#ifdef GAME_DLL
	CTFPlayer *player = GetTFPlayerOwner();

	if ( TFGameRules()->IsPVEModeControlled( player ) )
	{
		// scenario bots cant crit (unless they always do)
		CTFBot *bot = ToTFBot( player );
		return ( bot && bot->HasAttribute( CTFBot::ALWAYS_CRIT ) );
	}

#ifdef TF_CREEP_MODE
	if ( TFGameRules()->IsCreepWaveMode() && player )
	{
		CTFBot *bot = ToTFBot( player );

		if ( bot && bot->HasAttribute( CTFBot::IS_NPC ) )
		{
			// creeps can't crit
			return false;
		}
	}
#endif // TF_CREEP_MODE
#endif

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::CanFireRandomCriticalShot( float flCritChance )
{
#ifdef GAME_DLL
	// Todo: Create a version of this in tf_weaponbase_melee

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	PlayerStats_t *pPlayerStats = CTF_GameStats.FindPlayerStats( pPlayer );
	if ( pPlayerStats )
	{
		// Compare total damage done against total crit damage done.  If this
		// ratio is out of range for the expected crit chance, deny the crit.
		int nRandomRangedCritDamage = pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE_RANGED_CRIT_RANDOM];
		int nTotalDamage = pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE_RANGED];

		// Early out
		if ( !nTotalDamage )
			return true;

		float flNormalizedDamage = (float)nRandomRangedCritDamage / TF_DAMAGE_CRIT_MULTIPLIER;
		m_flObservedCritChance.Set( flNormalizedDamage / ( flNormalizedDamage + ( nTotalDamage - nRandomRangedCritDamage ) ) );

		// DevMsg ( "SERVER: CritChance: %f Observed: %f\n", flCritChance, m_flObservedCritChance.Get() );
	}
#else
		// DevMsg ( "CLIENT: CritChance: %f Observed: %f\n", flCritChance, m_flObservedCritChance.Get() );
#endif // GAME_DLL
	
	if ( m_flObservedCritChance.Get() > flCritChance + 0.1f )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CTFWeaponBase::GetShootSound( int iIndex ) const
{
	const CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem->IsValid() )
	{
		int nTeam = GetTeamNumber();

		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && nTeam == TF_TEAM_PVE_INVADERS )
		{
			CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
			if ( pPlayer && pPlayer->IsMiniBoss() )
			{
				// Not a real team - just a define used in replacing visuals via itemdefs ("visuals_mvm")
				nTeam = TF_TEAM_PVE_INVADERS_GIANTS;
			}
		}
		const char *pszSound = pItem->GetStaticData()->GetWeaponReplacementSound( nTeam, (WeaponSound_t)iIndex );
		if ( pszSound )
			return pszSound;
	}

	return BaseClass::GetShootSound(iIndex);
}

//-----------------------------------------------------------------------------
// Purpose: Owner is stunned.
//-----------------------------------------------------------------------------
void CTFWeaponBase::OnControlStunned( void )
{
	// Abort reloading.
	AbortReload();

	// Hide the weapon.
	SetWeaponVisible( false );
}

#if defined( CLIENT_DLL )

static ConVar	cl_bobcycle( "cl_bobcycle","0.8", FCVAR_CHEAT );
static ConVar	cl_bobup( "cl_bobup","0.5", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: Helper function to calculate head bob
//-----------------------------------------------------------------------------
float CalcViewModelBobHelper( CBasePlayer *player, BobState_t *pBobState )
{
	Assert( pBobState );
	if ( !pBobState )
		return 0;

	float	cycle;

	// Don't allow zeros, because we divide by them.
	float flBobup = cl_bobup.GetFloat();
	if ( flBobup <= 0 )
	{
		flBobup = 0.01;
	}
	float flBobCycle = cl_bobcycle.GetFloat();
	if ( flBobCycle <= 0 )
	{
		flBobCycle = 0.01;
	}

	//NOTENOTE: For now, let this cycle continue when in the air, because it snaps badly without it

	if ( ( !gpGlobals->frametime ) || ( player == NULL ) )
	{
		//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
		return 0.0f;// just use old value
	}

	//Find the speed of the player
	float speed = player->GetLocalVelocity().Length2D();
	float flmaxSpeedDelta = MAX( 0, (gpGlobals->curtime - pBobState->m_flLastBobTime ) * 320.0f );

	// don't allow too big speed changes
	speed = clamp( speed, pBobState->m_flLastSpeed-flmaxSpeedDelta, pBobState->m_flLastSpeed+flmaxSpeedDelta );
	speed = clamp( speed, -320.f, 320.f );

	pBobState->m_flLastSpeed = speed;

	//FIXME: This maximum speed value must come from the server.
	//		 MaxSpeed() is not sufficient for dealing with sprinting - jdw

	float bob_offset = RemapVal( speed, 0, 320, 0.0f, 1.0f );

	pBobState->m_flBobTime += ( gpGlobals->curtime - pBobState->m_flLastBobTime ) * bob_offset;
	pBobState->m_flLastBobTime = gpGlobals->curtime;

	//Calculate the vertical bob
	cycle = pBobState->m_flBobTime - (int)(pBobState->m_flBobTime/flBobCycle)*flBobCycle;
	cycle /= flBobCycle;

	if ( cycle < flBobup )
	{
		cycle = M_PI * cycle / flBobup;
	}
	else
	{
		cycle = M_PI + M_PI*(cycle-flBobup)/(1.0 - flBobup);
	}

	pBobState->m_flVerticalBob = speed*0.005f;
	pBobState->m_flVerticalBob = pBobState->m_flVerticalBob*0.3 + pBobState->m_flVerticalBob*0.7*sin(cycle);

	pBobState->m_flVerticalBob = clamp( pBobState->m_flVerticalBob, -7.0f, 4.0f );

	//Calculate the lateral bob
	cycle = pBobState->m_flBobTime - (int)(pBobState->m_flBobTime/flBobCycle*2)*flBobCycle*2;
	cycle /= flBobCycle*2;

	if ( cycle < flBobup )
	{
		cycle = M_PI * cycle / flBobup;
	}
	else
	{
		cycle = M_PI + M_PI*(cycle-flBobup)/(1.0 - flBobup);
	}

	pBobState->m_flLateralBob = speed*0.005f;
	pBobState->m_flLateralBob = pBobState->m_flLateralBob*0.3 + pBobState->m_flLateralBob*0.7*sin(cycle);
	pBobState->m_flLateralBob = clamp( pBobState->m_flLateralBob, -7.0f, 4.0f );

	//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to add head bob
//-----------------------------------------------------------------------------
void AddViewModelBobHelper( Vector &origin, QAngle &angles, BobState_t *pBobState )
{
	Assert( pBobState );
	if ( !pBobState )
		return;

	Vector	forward, right;
	AngleVectors( angles, &forward, &right, NULL );

	// Apply bob, but scaled down to 40%
	VectorMA( origin, pBobState->m_flVerticalBob * 0.4f, forward, origin );

	// Z bob a bit more
	origin[2] += pBobState->m_flVerticalBob * 0.1f;

	// bob the angles
	angles[ ROLL ]	+= pBobState->m_flVerticalBob * 0.5f;
	angles[ PITCH ]	-= pBobState->m_flVerticalBob * 0.4f;
	angles[ YAW ]	-= pBobState->m_flLateralBob  * 0.3f;

	VectorMA( origin, pBobState->m_flLateralBob * 0.2f, right, origin );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFWeaponBase::CalcViewmodelBob( void )
{
	CBasePlayer *player = ToBasePlayer( GetOwner() );
	//Assert( player );
	BobState_t *pBobState = GetBobState();
	if ( pBobState )
		return ::CalcViewModelBobHelper( player, pBobState );
	else
		return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&angles - 
//			viewmodelindex - 
//-----------------------------------------------------------------------------
void CTFWeaponBase::AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles )
{
	// call helper functions to do the calculation
	BobState_t *pBobState = GetBobState();
	if ( pBobState )
	{
		CalcViewmodelBob();
		::AddViewModelBobHelper( origin, angles, pBobState );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the head bob state for this weapon, which is stored
//			in the view model.  Note that this this function can return
//			NULL if the player is dead or the view model is otherwise not present.
//-----------------------------------------------------------------------------
BobState_t *CTFWeaponBase::GetBobState()
{
	// get the view model for this weapon
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
		return NULL;
	CBaseViewModel *baseViewModel = pOwner->GetViewModel( m_nViewModelIndex );
	if ( baseViewModel == NULL )
		return NULL;
	CTFViewModel *viewModel = dynamic_cast<CTFViewModel *>(baseViewModel);
	Assert( viewModel );

	// get the bob state out of the view model
	return &( viewModel->GetBobState() );
}

#endif // defined( CLIENT_DLL )

//-----------------------------------------------------------------------------
// Purpose: Used for spy invisiblity material, skin overrides, and team colors
//-----------------------------------------------------------------------------
int CTFWeaponBase::GetSkin()
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return 0;

	int iTeamNumber = pPlayer->GetTeamNumber();

#if defined( CLIENT_DLL )
	// Run client-only "is the viewer on the same team as the wielder" logic. Assumed to
	// always be false on the server.
	CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return 0;

	int iLocalTeam = pLocalPlayer->GetTeamNumber();
	
	// We only show disguise weapon to the enemy team when owner is disguised
	bool bUseDisguiseWeapon = ( iTeamNumber != iLocalTeam && iLocalTeam > LAST_SHARED_TEAM );

	if ( bUseDisguiseWeapon && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		if ( pLocalPlayer != pPlayer )
		{
			iTeamNumber = pPlayer->m_Shared.GetDisguiseTeam();
		}
	}
#endif // defined( CLIENT_DLL )

	// See if the item wants to override the skin
	int nSkin = GetSkinOverride();						// give custom gameplay code a chance to set whatever

	if ( nSkin == -1 )
	{
		const CEconItemView *pItem = GetAttributeContainer()->GetItem();
		if ( pItem->IsValid() )
		{
			nSkin = pItem->GetSkin( iTeamNumber );			// if we didn't have custom code, fall back to the item definition
		}
	}

	// If it didn't, fall back to the base skins
	if ( nSkin == -1 )
	{
		switch( iTeamNumber )
		{
		case TF_TEAM_RED:
			nSkin = 0;
			break;
		case TF_TEAM_BLUE:
			nSkin = 1;
			break;
		default:
			nSkin = 0;
			break;
		}
	}

	return nSkin;
}

#if defined( CLIENT_DLL )

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options )
{
	if ( event == 6002 && ShouldEjectBrass() )
	{
		if ( UsingViewModel() && !g_pClientMode->ShouldDrawViewModel() )
		{
			// Prevent effects when the ViewModel is hidden with r_drawviewmodel=0
			return true;
		}

		CEffectData data;
		// Look for 'eject_brass' attachment first instead of using options which is a seemingly magic number
		if ( m_iEjectBrassAttachpoint == -2 )
		{
			m_iEjectBrassAttachpoint = pViewModel->LookupAttachment( "eject_brass" );
		}

		if ( m_iEjectBrassAttachpoint > 0 )
		{
			pViewModel->GetAttachment( m_iEjectBrassAttachpoint, data.m_vOrigin, data.m_vAngles );
		}
		else
		{
			pViewModel->GetAttachment( atoi(options), data.m_vOrigin, data.m_vAngles );
		}
		data.m_nDamageType = GetAttributeContainer()->GetItem() ? GetAttributeContainer()->GetItem()->GetItemDefIndex() : 0;
		data.m_nHitBox = GetWeaponID();
		DispatchEffect( "TF_EjectBrass", data );
		return true;
	}
	if ( event == AE_WPN_INCREMENTAMMO )
	{
		IncrementAmmo();

		m_bReloadedThroughAnimEvent = true;

		return true;
	}

	return BaseClass::OnFireEvent( pViewModel, origin, angles, event, options );
}

//-----------------------------------------------------------------------------
// Purpose: Used for spy invisiblity material
//-----------------------------------------------------------------------------
class CWeaponInvisProxy : public CBaseInvisMaterialProxy
{
public:
	virtual void OnBind( C_BaseEntity *pBaseEntity ) OVERRIDE;
};


extern ConVar tf_teammate_max_invis;
//-----------------------------------------------------------------------------
// Purpose: 
// Input :
//-----------------------------------------------------------------------------
void CWeaponInvisProxy::OnBind( C_BaseEntity *pBaseEntity )
{
	if( !m_pPercentInvisible )
		return;

	C_BaseEntity *pMoveParent = pBaseEntity->GetMoveParent();
	if ( !pMoveParent )
	{
		m_pPercentInvisible->SetFloatValue( 0.0f );
		return;
	}

	if ( !pMoveParent->IsPlayer() )
	{
		C_TFPlayer *pOwningPlayer = ToTFPlayer( pMoveParent->GetOwnerEntity() );
		if ( pOwningPlayer )
		{
			// mimic the owner's invisibility
			float flInvis = pOwningPlayer->GetEffectiveInvisibilityLevel();
			m_pPercentInvisible->SetFloatValue( flInvis );
		}
		else
		{
			m_pPercentInvisible->SetFloatValue( 0.0f );
		}

		return;
	}

	CTFPlayer *pPlayer = ToTFPlayer( pMoveParent );
	Assert( pPlayer );

	float flInvis = pPlayer->GetEffectiveInvisibilityLevel();
	m_pPercentInvisible->SetFloatValue( flInvis );
}

EXPOSE_INTERFACE( CWeaponInvisProxy, IMaterialProxy, "weapon_invis" IMATERIAL_PROXY_INTERFACE_VERSION );

#endif // CLIENT_DLL

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ConVar tf_dev_marked_for_death_lifetime( "tf_dev_marked_for_death_lifetime", "15.0", FCVAR_DEVELOPMENTONLY );
ConVar tf_dev_health_on_damage_recover_percentage( "tf_dev_health_on_damage_recover_percentage", "0.35", FCVAR_DEVELOPMENTONLY );

void CTFWeaponBase::ApplyOnHitAttributes( CBaseEntity *pVictimBaseEntity, CTFPlayer *pAttacker, const CTakeDamageInfo &info )
{
	if ( !pAttacker )
		return;

	CTFPlayer *pVictim = ToTFPlayer( pVictimBaseEntity );

	// Ammo on hit
	int iModAmmoOnHit = 0;
	CALL_ATTRIB_HOOK_INT( iModAmmoOnHit, add_onhit_addammo );
	if ( iModAmmoOnHit > 0 )
	{
		// this will save the value so we can add it after we're doing firing 
		// the projectile and have subtracted the ammo for the current shot
		float flPercentage = (float)iModAmmoOnHit / 100.0f;

		// No ammo for disguised Spies that are NOT stealthed so you can't use this to check for Spies
		if ( pVictim && 
			 pVictim->IsPlayerClass( TF_CLASS_SPY ) && 
			 pVictim->m_Shared.InCond( TF_COND_DISGUISED ) && 
			 !( pVictim->m_Shared.IsStealthed() || pVictim->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) ) )
		{
			flPercentage = 0.0f;
		}

		m_iAmmoToAdd += (int)( flPercentage * info.GetDamage() );
	}

	int iExtraDamageOnHit = 0;
	CALL_ATTRIB_HOOK_INT( iExtraDamageOnHit, extra_damage_on_hit );
	if ( iExtraDamageOnHit )
	{
		// Adds 'Heads'. Reusing this data field
		int iDecap = pAttacker->m_Shared.GetDecapitations();
		pAttacker->m_Shared.SetDecapitations( Min( 200, iDecap + iExtraDamageOnHit ) );
	}

	// Everything else is only for player enemies or Halloween bosses
	// We don't want buildables or the tank doing things like giving health or increasing ubercharge
	if ( !( pVictim || dynamic_cast< CHalloweenBaseBoss* >( pVictimBaseEntity ) ) )
	{
		return;
	}

	bool bIsSpyRevealed = false;

	if ( pVictim )
	{
		// Reveal cloaked Spy on hit
		if ( pVictim->IsPlayerClass( TF_CLASS_SPY ) && pVictim->m_Shared.IsStealthed() )
		{
			int iRevealCloakedSpyOnHit = 0;
			CALL_ATTRIB_HOOK_INT( iRevealCloakedSpyOnHit, reveal_cloaked_victim_on_hit );
			if ( iRevealCloakedSpyOnHit > 0 )
			{
				pVictim->RemoveInvisibility();
				bIsSpyRevealed = true;
			}
		}

		// Reveal disguised Spy on hit
		if ( pVictim->IsPlayerClass( TF_CLASS_SPY ) && pVictim->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			int iRevealDisguisedSpyOnHit = 0;
			CALL_ATTRIB_HOOK_INT( iRevealDisguisedSpyOnHit, reveal_disguised_victim_on_hit );
			if ( iRevealDisguisedSpyOnHit > 0 )
			{
				pVictim->RemoveDisguise();
				bIsSpyRevealed = true;
			}
		}

		if ( bIsSpyRevealed )
		{
			color32 colorHit = { 255, 255, 255, 255 };
			UTIL_ScreenFade( pVictim, colorHit, 0.25f, 0.1f, FFADE_IN );

	//		pVictim->EmitSound( "Weapon_DRG_Wrench.RevealSpy" );
		}

		// On hit attributes don't work when you shoot disguised spies
		if ( pVictim->m_Shared.InCond( TF_COND_DISGUISED ) )
			return;
	}

	// Or from burn damage
	if ( (info.GetDamageType() & DMG_BURN) )
		return;

	// Heal on hits
	int iModHealthOnHit = 0;
	CALL_ATTRIB_HOOK_INT( iModHealthOnHit, add_onhit_addhealth );
	if ( iModHealthOnHit )
	{
		// Scale Health mod with damage dealt, input being the maximum amount of health possible
		float flScale = Clamp( info.GetDamage() / info.GetBaseDamage(), 0.f, 1.0f );
		iModHealthOnHit = Max( 3, (int)( (float)iModHealthOnHit * flScale ) );
	}

	// Charge meter on hit
	float flChargeRefill = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT( flChargeRefill, charge_meter_on_hit );
	if ( flChargeRefill > 0 )
	{
		if ( pAttacker->m_Shared.GetCarryingRuneType() != RUNE_NONE )
		{
			flChargeRefill *= 0.2f;
		}
		pAttacker->m_Shared.SetDemomanChargeMeter( pAttacker->m_Shared.GetDemomanChargeMeter() + flChargeRefill * 100.0f );
	}

	// Speed on hit
	int iSpeedBoostOnHit = 0;
	CALL_ATTRIB_HOOK_INT( iSpeedBoostOnHit, speed_boost_on_hit );
	if ( iSpeedBoostOnHit )
	{
		pAttacker->m_Shared.AddCond( TF_COND_SPEED_BOOST, iSpeedBoostOnHit );
	}

	if ( pVictim )
	{
		if ( pVictim->m_Shared.InCond( TF_COND_MAD_MILK ) )
		{
			int nAmount = info.GetDamage() * 0.6f;
			iModHealthOnHit += nAmount;

			CTFPlayer *pProvider = ToTFPlayer( pVictim->m_Shared.GetConditionProvider( TF_COND_MAD_MILK ) );
			if ( pProvider )
			{
				// Only give points for the portion they're responsible for
				if ( pProvider != pAttacker )
				{
					CTF_GameStats.Event_PlayerHealedOtherAssist( pProvider, nAmount );
				}

				// Show in the medic's UI as primary healing
				IGameEvent *event = gameeventmanager->CreateEvent( "player_healed" );
				if ( event )
				{
					event->SetInt( "priority", 1 );	// HLTV event priority
					event->SetInt( "patient", pAttacker->GetUserID() );
					event->SetInt( "healer", pProvider->GetUserID() );
					event->SetInt( "amount", iModHealthOnHit );
					gameeventmanager->FireEvent( event );
				}

				// Give them a little bit of Uber
				CWeaponMedigun *pMedigun = static_cast<CWeaponMedigun *>( pProvider->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN ) );
				if ( pMedigun )
				{
					int iHealedAmount = Max( Min( (int)pAttacker->GetMaxHealth() - (int)pAttacker->GetHealth(), nAmount ), 0 );

					// On Mediguns, per frame, the amount of uber added is based on 
					// Default heal rate is 24per second, we scale based on that and frametime
					pMedigun->AddCharge( (iHealedAmount / 24.0f ) * gpGlobals->frametime );
				}
			}
		}
	}

	if ( pAttacker->m_Shared.InCond( TF_COND_REGENONDAMAGEBUFF ) )
	{
		int nAmount = info.GetDamage() * tf_dev_health_on_damage_recover_percentage.GetFloat();
		iModHealthOnHit += nAmount;

		// Increment provider's healing assist stat
		CTFPlayer *pProvider = ToTFPlayer( pAttacker->m_Shared.GetConditionProvider( TF_COND_REGENONDAMAGEBUFF ) );
		if ( pProvider && pProvider != pAttacker )
		{
			// Only give points for the portion they're responsible for
			CTF_GameStats.Event_PlayerHealedOtherAssist( pProvider, nAmount );
		}
	}

	if ( iModHealthOnHit )
	{
		if ( iModHealthOnHit > 0 )
		{
			int iHealed = pAttacker->TakeHealth( iModHealthOnHit, DMG_GENERIC );

			// Increment attacker's healing stat
			if ( iHealed )
			{
				CTF_GameStats.Event_PlayerHealedOther( pAttacker, iHealed );
			}
		}
		else 
		{
			pAttacker->TakeDamage( CTakeDamageInfo( pAttacker, this, (iModHealthOnHit * -1), DMG_GENERIC ) );
		}

		IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
		if ( event )
		{
			event->SetInt( "amount", iModHealthOnHit );
			event->SetInt( "entindex", pAttacker->entindex() );
			item_definition_index_t healingItemDef = INVALID_ITEM_DEF_INDEX;
			if ( GetAttributeContainer() && GetAttributeContainer()->GetItem() )
			{
				healingItemDef = GetAttributeContainer()->GetItem()->GetItemDefIndex();
			}
			event->SetInt( "weapon_def_index", healingItemDef );
			gameeventmanager->FireEvent( event ); 
		}
	}

	// Add ubercharge on hit
	if ( pAttacker->IsPlayerClass( TF_CLASS_MEDIC ) )
	{
		float flUberChargeBonus = 0;
		CALL_ATTRIB_HOOK_FLOAT( flUberChargeBonus, add_onhit_ubercharge );
		if ( flUberChargeBonus )
		{
			CWeaponMedigun *pMedigun = (CWeaponMedigun *)pAttacker->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );
			if ( pMedigun )
			{
				if ( TFGameRules() && TFGameRules()->IsPowerupMode() )
				{
					if ( pAttacker->m_Shared.GetCarryingRuneType() != RUNE_NONE )
					{
						flUberChargeBonus *= 0.2;
					}
					else 
						flUberChargeBonus *= 0.4;
				}
				pMedigun->AddCharge( flUberChargeBonus );
			}
		}
	}
	
	// Lower rage on hit.
	if ( pAttacker->IsPlayerClass( TF_CLASS_SOLDIER ) || pAttacker->IsPlayerClass( TF_CLASS_PYRO ) )
	{
		int iRageOnHit = 0;
		CALL_ATTRIB_HOOK_INT( iRageOnHit, rage_on_hit );
		pAttacker->m_Shared.ModifyRage( iRageOnHit );
	}

	// rune charge on hit
	if ( pAttacker->m_Shared.CanRuneCharge() )
	{
		const float flMaxRuneCharge = 400.f;
		float flAdd = (float)info.GetDamage() * ( 100.f /  flMaxRuneCharge );
		pAttacker->m_Shared.SetRuneCharge( pAttacker->m_Shared.GetRuneCharge() + flAdd );
	}

	// Increase Boost on hit
	int iBoostOnDamage = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pAttacker, iBoostOnDamage, boost_on_damage );

	if ( iBoostOnDamage != 0 )
	{
		float fHype = MIN( tf_scout_hype_pep_max.GetFloat(), pAttacker->m_Shared.GetScoutHypeMeter() + ( MAX( tf_scout_hype_pep_min_damage.GetFloat(), info.GetDamage() ) / tf_scout_hype_pep_mod.GetFloat() ) );
		pAttacker->m_Shared.SetScoutHypeMeter( fHype );
		pAttacker->TeamFortress_SetSpeed();
	}

	// Procs!
	if( pVictim )
	{
		// Detemine weapon speed
		float flFireDelay = ApplyFireDelay( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay );
		
		// Proc chance for AOE Heal
		float flPPM = 0.f;
		CALL_ATTRIB_HOOK_FLOAT( flPPM, aoe_heal_chance );
		float flProcChance = flFireDelay * (flPPM / 60.f);
		
		if( RandomFloat() < flProcChance )
		{
			pAttacker->m_Shared.AddCond( TF_COND_RADIUSHEAL_ON_DAMAGE, 1.0f );
		}

		// Proc chance for crit boost
		flPPM = 0.f;
		CALL_ATTRIB_HOOK_FLOAT( flPPM, crits_on_damage );
		flProcChance = flFireDelay * (flPPM / 60.f);
		if( RandomFloat() < flProcChance )
		{
			pAttacker->m_Shared.AddCond( TF_COND_CRITBOOSTED_CARD_EFFECT, 3 );
		}


		// Proc chance for stun
		flPPM = 0.f;
		CALL_ATTRIB_HOOK_FLOAT( flPPM, stun_on_damage );
		flProcChance = flFireDelay * (flPPM / 60.f);
		
		if( RandomFloat() < flProcChance )
		{
			pVictim->m_Shared.StunPlayer( 3.0, 1.f, TF_STUN_MOVEMENT | TF_STUN_CONTROLS, pAttacker );
		}


		// Proc chance for AOE Blast
		flPPM = 0.f;
		CALL_ATTRIB_HOOK_FLOAT ( flPPM, aoe_blast_on_damage );
		flProcChance = flFireDelay * (flPPM / 60.f);

		if ( (RandomFloat() < flProcChance) )
		{
			// Stun the source
			float flStunDuration = 2.f;
			float flStunAmt = 1.f;
			pVictim->m_Shared.StunPlayer( flStunDuration, flStunAmt, TF_STUN_MOVEMENT | TF_STUN_CONTROLS | TF_STUN_NO_EFFECTS, pAttacker );

			pVictim->m_Shared.MakeBleed( ToTFPlayer( pAttacker ), NULL, flStunDuration, 75 );

			// Generate an explosion and look for nearby bots
			float flDmgRange = 100.f;

			const int nMaxEnts = 12;
			CBaseEntity	*pObjects[ nMaxEnts ];
			CTFPlayer* pPrevTFPlayer = NULL;
			int nCount = UTIL_EntitiesInSphere( pObjects, nMaxEnts, pVictim->GetAbsOrigin(), flDmgRange, FL_CLIENT );
			for ( int i = 0; i < nCount; i++ )
			{
				if ( !pObjects[i] )
					continue;

				if ( !pObjects[i]->IsAlive() )
					continue;

				if ( pObjects[i]->GetTeamNumber() != pVictim->GetTeamNumber() )
					continue;

				if ( !FVisible( pObjects[i], MASK_OPAQUE ) )
					continue;

				CTFPlayer *pTFPlayer = static_cast<CTFPlayer *>( pObjects[i] );
				if ( !pTFPlayer )
					continue;

				if ( pTFPlayer == pVictim )
					continue;

				if ( !pTFPlayer->IsBot() )
					continue;

				if ( pVictim->m_Shared.InCond( TF_COND_PHASE ) || pVictim->m_Shared.InCond( TF_COND_PASSTIME_INTERCEPTION ) )
					continue;

				if ( pVictim->m_Shared.IsInvulnerable() )
					continue;

				// Stun
				pTFPlayer->m_Shared.StunPlayer( flStunDuration, flStunAmt, TF_STUN_MOVEMENT | TF_STUN_CONTROLS | TF_STUN_NO_EFFECTS, pAttacker );

				// DoT
				pTFPlayer->m_Shared.MakeBleed( ToTFPlayer( pAttacker ), NULL, flStunDuration, 75.f );

				// Shoot a beam at them
				CPVSFilter filter( pTFPlayer->WorldSpaceCenter() );
				Vector vStart = pPrevTFPlayer == NULL ? pVictim->EyePosition() : pPrevTFPlayer->EyePosition();
				Vector vEnd = pTFPlayer->EyePosition();
				te_tf_particle_effects_control_point_t controlPoint = { PATTACH_ABSORIGIN, vEnd };
				TE_TFParticleEffectComplex( filter, 0.0f, "dxhr_arm_muzzleflash", vStart, QAngle( 0, 0, 0 ), NULL, &controlPoint, pTFPlayer, PATTACH_CUSTOMORIGIN );

				pTFPlayer->EmitSound( "Weapon_Upgrade.ExplosiveHeadshot" );
				pPrevTFPlayer = pTFPlayer;
			}
		}

	}

	// Damage bonus on hit
	// Disabled because we have no attributes that use it
	/*
	float flAddDamageDoneBonusOnHit = 0;
	CALL_ATTRIB_HOOK_FLOAT( flAddDamageDoneBonusOnHit, addperc_ondmgdone_tmpbuff );
	if ( flAddDamageDoneBonusOnHit )
	{
		pAttacker->m_Shared.AddTmpDamageBonus( flAddDamageDoneBonusOnHit, 10.0 );
	}
	*/

	if ( pVictim )
	{
		int iRageStun = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pAttacker, iRageStun, generate_rage_on_dmg );
		if ( iRageStun && pAttacker->m_Shared.IsRageDraining() )
		{
			// MvM: Heavies can purchase a rage-based knockback+stun effect
			if ( pAttacker->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
			{
				int iStunFlags = TF_STUN_MOVEMENT | TF_STUN_NO_EFFECTS;
				pVictim->m_Shared.StunPlayer( 0.25f, 1.f, iStunFlags, pAttacker );
			}
		}

		// Slow enemy on hit, unless they're being healed by a medic
		if ( !pVictim->m_Shared.InCond( TF_COND_HEALTH_BUFF ) )
		{
			float flSlowEnemy = 0.0;
			CALL_ATTRIB_HOOK_FLOAT( flSlowEnemy, mult_onhit_enemyspeed );
			if ( flSlowEnemy )
			{
				if ( RandomFloat() < flSlowEnemy )
				{
					// Adjust the stun amount based on distance to the target
					// close range full stun, falls off to zero at 1536 (1024 window size)
					Vector vecDistance = pVictim->GetAbsOrigin() - pAttacker->GetAbsOrigin();
					float flStunAmount = RemapValClamped( vecDistance.LengthSqr(), (512.0f * 512.0f), (1536.0f * 1536.0f), 0.60f, 0.0f );

					pVictim->m_Shared.StunPlayer( 0.2, flStunAmount, TF_STUN_MOVEMENT, pAttacker );
				}
			}

			flSlowEnemy = 0.0;
			CALL_ATTRIB_HOOK_FLOAT( flSlowEnemy, mult_onhit_enemyspeed_major );
			if ( flSlowEnemy )
			{
				pVictim->m_Shared.StunPlayer( flSlowEnemy, 0.4, TF_STUN_MOVEMENT, pAttacker );
			}
		}

		// Mark for death on hit.
		int iMarkForDeath = 0;
		CALL_ATTRIB_HOOK_INT( iMarkForDeath, mark_for_death );
		if ( iMarkForDeath )
		{
			// Note: this logic isn't perfect, and can do non-obvious things in certain situations. For example,
			// imagine that we've got two scouts -- if the first scout marks someone, and then the second scout marks
			// the same guy, and then the first scout marks someone else, the original victim will lose his marked-
			// for-death status. Conditions don't have any concept of owner. This could be manually tracked for this
			// condition if it becomes a problem.
			if ( pAttacker->m_pMarkedForDeathTarget != NULL && pAttacker->m_pMarkedForDeathTarget->m_Shared.InCond( TF_COND_MARKEDFORDEATH ) )
			{
				pAttacker->m_pMarkedForDeathTarget->m_Shared.RemoveCond( TF_COND_MARKEDFORDEATH );
			}

			float flDuration = pVictim->IsMiniBoss() ? tf_dev_marked_for_death_lifetime.GetFloat() / 2 : tf_dev_marked_for_death_lifetime.GetFloat();
			pVictim->m_Shared.AddCond( TF_COND_MARKEDFORDEATH, flDuration, pAttacker );

			pAttacker->m_pMarkedForDeathTarget = pVictim;

			// ACHIEVEMENT_TF_MVM_SCOUT_MARK_FOR_DEATH
			if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
			{
				if ( pAttacker->IsPlayerClass( TF_CLASS_SCOUT ) && ( GetWeaponID() == TF_WEAPON_BAT_WOOD ) )
				{
					if ( pVictim->IsBot() && ( pVictim->GetTeamNumber() == TF_TEAM_PVE_INVADERS ) )
					{
						IGameEvent *event = gameeventmanager->CreateEvent( "mvm_scout_marked_for_death" );
						if ( event )
						{
							event->SetInt( "player", pAttacker->entindex() );
							gameeventmanager->FireEvent( event );
						}
					}
				}
			}
		}

		// Stun airborne enemies who are half a body length higher than attacker
		bool bIsVictimAirborne = !( pVictim->GetFlags() & FL_ONGROUND ) && ( pVictim->GetWaterLevel() == WL_NotInWater );

		int iStunWaistHighAirborne = 0;
		CALL_ATTRIB_HOOK_INT( iStunWaistHighAirborne, stun_waist_high_airborne );
		if ( iStunWaistHighAirborne > 0 && bIsVictimAirborne )
		{
			if ( pVictim->WorldSpaceCenter().z >= pAttacker->EyePosition().z )
			{
				// right in the jimmy!
				pVictim->m_Shared.StunPlayer( iStunWaistHighAirborne, 0.5f, TF_STUN_LOSER_STATE | TF_STUN_BOTH, pAttacker );
				pVictim->EmitSound( "Halloween.PlayerScream" );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: When owner of this weapon is hit
//-----------------------------------------------------------------------------
void CTFWeaponBase::ApplyOnInjuredAttributes( CTFPlayer *pVictim, CTFPlayer *pAttacker, const CTakeDamageInfo &info )
{
	if ( CanDeploy() )
	{
		int iBecomeFireproofOnHitByFire = 0;
		CALL_ATTRIB_HOOK_INT( iBecomeFireproofOnHitByFire, become_fireproof_on_hit_by_fire );
		if ( iBecomeFireproofOnHitByFire > 0 && ( ( info.GetDamageType() & DMG_BURN ) || ( info.GetDamageType() & DMG_IGNITE ) ) )
		{
			pVictim->m_Shared.AddCond( TF_COND_FIRE_IMMUNE, 1.f );

			if ( pVictim->m_Shared.InCond( TF_COND_BURNING ) )
			{
				pVictim->EmitSound( "TFPlayer.FlameOut" );
				pVictim->m_Shared.RemoveCond( TF_COND_BURNING );
			}
			// STAGING_SPY
			pVictim->m_Shared.AddCond( TF_COND_AFTERBURN_IMMUNE, iBecomeFireproofOnHitByFire );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::ApplyPostHitEffects( const CTakeDamageInfo &info, CTFPlayer *pVictim )
{
	bool bDidDrain = false;

	CTFPlayer *pAttacker = ToTFPlayer( info.GetAttacker() );
	if ( !pAttacker || !pVictim )
		return;

	// only drain a victim once per shot, even with penetrating weapons
	if ( pVictim != m_hLastDrainVictim || m_lastDrainVictimTimer.IsElapsed() )
	{
		// Subtract victim's Medigun charge on hit
		if ( pVictim->IsPlayerClass( TF_CLASS_MEDIC ) )
		{
			int iSubtractVictimMedigunChargeOnHit = 0;
			CALL_ATTRIB_HOOK_INT( iSubtractVictimMedigunChargeOnHit, subtract_victim_medigun_charge_onhit );
			if ( iSubtractVictimMedigunChargeOnHit > 0 )
			{
				CWeaponMedigun *pMedigun = (CWeaponMedigun *)pVictim->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );
				if ( pMedigun && !pMedigun->IsReleasingCharge() )
				{
					// STAGING_ENGY
					// Scale drain after 512 Hu to 1536Hu ( 50% drain at 1024, 0 drain at 1536 units )
					Vector toEnt = pVictim->GetAbsOrigin() - pAttacker->GetAbsOrigin();
					if ( toEnt.LengthSqr() > Square( 512.0f ) )
					{
						iSubtractVictimMedigunChargeOnHit *= RemapValClamped( toEnt.LengthSqr(), (512.0f * 512.0f), (1536.0f * 1536.0f), 1.0f, 0.0f );
					}	

					pMedigun->SubtractCharge( iSubtractVictimMedigunChargeOnHit / 100.0f );
					bDidDrain = true;
				}
			}
		}

		// Subtract victim's cloak on hit
		if ( pVictim->IsPlayerClass( TF_CLASS_SPY ) )
		{
			int iSubtractVictimCloakOnHit = 0;
			CALL_ATTRIB_HOOK_INT( iSubtractVictimCloakOnHit, subtract_victim_cloak_on_hit );
			if ( iSubtractVictimCloakOnHit > 0 )
			{
				// STAGING_ENGY
				// Scale drain after 512 Hu to 1536Hu ( 50% drain at 1024, 0 drain at 1536 units )
				Vector toEnt = pVictim->GetAbsOrigin() - pAttacker->GetAbsOrigin();
				if ( toEnt.LengthSqr() > Square( 512.0f ) )
				{
					iSubtractVictimCloakOnHit *= RemapValClamped( toEnt.LengthSqr(), (512.0f * 512.0f), (1536.0f * 1536.0f), 1.0f, 0.0f );
				}

				float flCloak = pVictim->m_Shared.GetSpyCloakMeter();
				flCloak -= iSubtractVictimCloakOnHit;
				if ( flCloak < 0.0f )
				{
					flCloak = 0.0f;
				}

				pVictim->m_Shared.SetSpyCloakMeter( flCloak );
				bDidDrain = true;
			}
		}

		// don't play effects to attacker if he hit a disguised/cloaked spy
		if ( !pVictim->m_Shared.InCond( TF_COND_DISGUISED ) &&
			 !pVictim->m_Shared.IsStealthed() )
		{
			if ( bDidDrain )
			{
				DispatchParticleEffect( "drg_pomson_impact_drain", PATTACH_POINT, pVictim, "head", GetParticleColor( 1 ), GetParticleColor( 2 ) );
				if ( pAttacker )
				{
					// play drain sound effect, louder for the attacker
					EmitSound_t params;
					params.m_flSoundTime = 0;
					params.m_pSoundName = "Weapon_Pomson.DrainedVictim";
					params.m_pflSoundDuration = 0;

					CPASFilter filter( pVictim->GetAbsOrigin() );
					filter.RemoveRecipient( pAttacker );
					EmitSound( filter, pVictim->entindex(), params );

					CSingleUserRecipientFilter attackerFilter( pAttacker );
					EmitSound( attackerFilter, pAttacker->entindex(), params );
				}

				m_hLastDrainVictim = pVictim;
				m_lastDrainVictimTimer.Start( 0.3f );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Deliberately disabled to prevent players picking up fallen weapons.
	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::DisguiseWeaponThink( void )
{
	// Periodically check to make sure we are valid.
	// Disguise weapons are attached to a player, but not managed through the owned weapons list.
	CTFPlayer *pTFOwner = ToTFPlayer( GetOwner() );
	if ( !pTFOwner )
	{
		// We must have an owner to be valid.
		Drop( Vector( 0,0,0 ) );
		return;
	}

	if ( pTFOwner->m_Shared.GetDisguiseWeapon() != this )
	{
		// The owner's disguise weapon must be us, otherwise we are invalid.
		Drop( Vector( 0,0,0 ) );
		return;
	}

	SetContextThink( &CTFWeaponBase::DisguiseWeaponThink, gpGlobals->curtime + 0.5, "DisguiseWeaponThink" );
}

#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::IsViewModelFlipped( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

#ifdef GAME_DLL
	if ( m_bFlipViewModel != pPlayer->m_bFlipViewModels )
	{
		return true;
	}
#else
	if ( m_bFlipViewModel != cl_flipviewmodels.GetBool() )
	{
		return true;
	}
#endif

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::ReapplyProvision( void )
{
	// Disguise items never provide
	if ( m_bDisguiseWeapon )
	{
#ifdef GAME_DLL
		UpdateModelToClass();
#endif
		return;
	}

	int iProvideMode = 0;
	CALL_ATTRIB_HOOK_INT( iProvideMode, provide_on_active );
	if ( 1 == iProvideMode )
	{
		if ( m_iState == WEAPON_IS_ACTIVE )
		{
			// We are active, provide to our owner.
			BaseClass::ReapplyProvision();
		}
		else
		{
			// We aren't active so stop providing to our owner.
			GetAttributeManager()->StopProvidingTo( GetPlayerOwner() );
			m_hOldProvidee = NULL;
		}
	}
	else
	{
		BaseClass::ReapplyProvision();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return the origin & angles for a projectile fired from the player's gun
//-----------------------------------------------------------------------------
void CTFWeaponBase::GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates /* = true */, float flEndDist /* = 2000 */)
{
	// @todo third person code!!

	// Flip the firing offset if our view model is flipped.
	if ( IsViewModelFlipped() )
	{
		vecOffset.y *= -1;
	}

	int iCenterFireProjectile = 0;
	CALL_ATTRIB_HOOK_INT( iCenterFireProjectile, centerfire_projectile );
	if ( iCenterFireProjectile == 1 )
	{
		vecOffset.y = 0;
	}

	QAngle angSpread = GetSpreadAngles();
	Vector vecForward, vecRight, vecUp;
	AngleVectors( angSpread, &vecForward, &vecRight, &vecUp );

	Vector vecShootPos = pPlayer->Weapon_ShootPosition();

	// Estimate end point
	Vector endPos = vecShootPos + vecForward * flEndDist;	

	// Trace forward and find what's in front of us, and aim at that
	trace_t tr;

	if ( bHitTeammates )
	{
		CTraceFilterSimple traceFilter( pPlayer, COLLISION_GROUP_NONE );
		ITraceFilter *pFilterChain = NULL;

		CTraceFilterIgnoreFriendlyCombatItems traceFilterCombatItem( pPlayer, COLLISION_GROUP_NONE, GetTeamNumber() );
		if ( TFGameRules() && TFGameRules()->GameModeUsesUpgrades() )
		{
			// Ignore teammates and their (physical) upgrade items in MvM
			pFilterChain = &traceFilterCombatItem;
		}

		CTraceFilterChain traceFilterChain( &traceFilter, pFilterChain );
		UTIL_TraceLine( vecShootPos, endPos, MASK_SOLID, &traceFilterChain, &tr );
	}
	else
	{
		CTraceFilterIgnoreTeammates filter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
		UTIL_TraceLine( vecShootPos, endPos, MASK_SOLID, &filter, &tr );
	}

	// Offset actual start point
	*vecSrc = vecShootPos + (vecForward * vecOffset.x) + (vecRight * vecOffset.y) + (vecUp * vecOffset.z);

	// Find angles that will get us to our desired end point
	// Only use the trace end if it wasn't too close, which results
	// in visually bizarre forward angles
	if ( tr.fraction > 0.1 )
	{
		VectorAngles( tr.endpos - *vecSrc, *angForward );
	}
	else
	{
		VectorAngles( endPos - *vecSrc, *angForward );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
QAngle CTFWeaponBase::GetSpreadAngles( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	Assert( pOwner );

	QAngle angEyes = pOwner->EyeAngles();

	float flSpreadAngle = 0.0f; 
	CALL_ATTRIB_HOOK_FLOAT( flSpreadAngle, projectile_spread_angle );
	if ( flSpreadAngle )
	{
		QAngle angSpread = RandomAngle( -flSpreadAngle, flSpreadAngle );
		angSpread.z = 0.0f;

		if ( TFGameRules() && TFGameRules()->GameModeUsesUpgrades() )
		{
			if ( CanOverload() && AutoFiresFullClip() && Clip1() == 1 && !m_bFiringWholeClip )
			{
				float flTimeSinceLastAttack = gpGlobals->curtime - GetLastPrimaryAttackTime();
				if ( flTimeSinceLastAttack < 0.9f )
				{
					// Punish upgraded single-fire spam for this class of weapon
					float flPenaltyAngle = RemapValClamped( flTimeSinceLastAttack, 0.4f, 0.9f, 6.f, 1.f );
					angSpread += RandomAngle( -flPenaltyAngle, flPenaltyAngle );
				}
			}
		}

		angEyes += angSpread;
	}

	return angEyes;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::CanPerformSecondaryAttack() const
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );

	// Demo shields are allowed to charge whenever
	if ( pOwner->m_Shared.HasDemoShieldEquipped() )
		return true;

	return BaseClass::CanPerformSecondaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::AreRandomCritsEnabled( void )
{
	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsPowerupMode() )
			return false;

		const IMatchGroupDescription *pMatchDesc = GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() );
		if ( pMatchDesc )
			return pMatchDesc->BUsesRandomCrits();
	}

	return tf_weapon_criticals.GetBool();
}


#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::ChangeTeam( int iTeamNum )
{
	BaseClass::ChangeTeam( iTeamNum );

	// We need to set the team for our econ item view as well
	if ( GetAttributeContainer() && GetAttributeContainer()->GetItem() )
	{
		GetAttributeContainer()->GetItem()->SetTeamNumber( GetTeamNumber() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::DeflectProjectiles()
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return false;

	if ( pOwner->GetWaterLevel() == WL_Eyes )
		return false;

	lagcompensation->StartLagCompensation( pOwner, pOwner->GetCurrentCommand() );

	Vector vecEye = pOwner->EyePosition();
	Vector vecForward, vecRight, vecUp;
	AngleVectors( pOwner->EyeAngles(), &vecForward, &vecRight, &vecUp );
	Vector vecCenter = vecEye + vecForward * GetDeflectionRadius();

	// Get a list of entities in the box defined by vecSize at VecCenter.
	// We will then try to deflect everything in the box.
	const int maxCollectedEntities = 64;
	CBaseEntity	*pObjects[ maxCollectedEntities ];
	int count = UTIL_EntitiesInSphere( pObjects, maxCollectedEntities, vecCenter, GetDeflectionRadius(), FL_CLIENT | FL_GRENADE );

	//NDebugOverlay::Sphere( vecCenter, GetDeflectionRadius(), 0, 255, 0, 40, 3 );

	bool bDeflected = false;
	bool bDeflectedPlayer = false;

	int iEnemyTeam = GetEnemyTeam( pOwner->GetTeamNumber() );
	bool bTruce = TFGameRules() && TFGameRules()->IsTruceActive() && pOwner->IsTruceValidForEnt();

	for ( int i = 0; i < count; i++ )
	{
		if ( pObjects[i] == pOwner )
			continue;

		if ( pObjects[i]->IsPlayer() && pObjects[i]->GetTeamNumber() == TEAM_SPECTATOR )
			continue;

		if ( pOwner->FVisible( pObjects[i], MASK_SOLID ) == false )
			continue;

		if ( bTruce && ( pObjects[i]->GetTeamNumber() == iEnemyTeam ) )
			continue;

		if ( !pObjects[i]->IsDeflectable() && !FClassnameIs( pObjects[i], "prop_physics" ) )
			continue;

		if ( pObjects[i]->IsPlayer() == true )
		{
			CTFPlayer *pTarget = ToTFPlayer( pObjects[i] );
			if ( pTarget )
			{
				bool bRes = DeflectPlayer( pTarget, pOwner, vecForward );
				bDeflectedPlayer |= bRes;
				bDeflected |= bRes;
			}
		}
		else
		{
			bDeflected |= DeflectEntity( pObjects[i], pOwner, vecForward );
		}
	}

	if ( bDeflected )
	{
		pOwner->SpeakConceptIfAllowed( MP_CONCEPT_DEFLECTED, "victim:0" );
		PlayDeflectionSound( bDeflectedPlayer );
	}

	lagcompensation->FinishLagCompensation( pOwner );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::DeflectPlayer( CTFPlayer *pTarget, CTFPlayer *pOwner, Vector &vecForward )
{
	return true;
}

//-----------------------------------------------------------------------------
// This filter checks against friendly players, buildings, shields
//-----------------------------------------------------------------------------
class CTraceFilterDeflection : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterDeflection, CTraceFilterSimple );
	
	CTraceFilterDeflection( const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam ) 
		: CTraceFilterSimple( passentity, collisionGroup ), m_iIgnoreTeam( iIgnoreTeam )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *passentity, int contentsMask ) OVERRIDE
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( passentity );
		if ( !pEntity )
			return false;

		if ( pEntity->IsPlayer() )
			return false;

		if ( pEntity->IsBaseObject() )
			return false;

		if ( pEntity->IsCombatItem() )
			return false;

		return BaseClass::ShouldHitEntity( passentity, contentsMask );
	}

	int m_iIgnoreTeam;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::DeflectEntity( CBaseEntity *pTarget, CTFPlayer *pOwner, Vector &vecForward )
{
	Assert( pTarget );
	Assert( pOwner );

	Vector vecEye = pOwner->EyePosition();
	Vector vecVel = pTarget->GetAbsVelocity();

	// apply an impulse instead if this is a prop physics object
	if ( FClassnameIs( pTarget, "prop_physics" ) )
	{
		IPhysicsObject *pPhysicsObject = pTarget->VPhysicsGetObject();
		if ( pPhysicsObject && pTarget->CollisionProp() )
		{
			Vector vecDir = pTarget->WorldSpaceCenter() - vecEye;
			VectorNormalize( vecDir );
			float flVel = 50.0f * CTFWeaponBase::DeflectionForce( pTarget->CollisionProp()->OBBSize(), 90, 12.0f );
			pPhysicsObject->ApplyForceOffset( vecDir * flVel, vecEye );
		}
		return true;
	}

	int iAOEDeflection = 0;
	CALL_ATTRIB_HOOK_INT( iAOEDeflection, aoe_deflection );
	Vector vecDir;
	if ( iAOEDeflection )
	{
		vecDir = pTarget->WorldSpaceCenter() - pOwner->WorldSpaceCenter();
	}
	else
	{
		CTraceFilterDeflection filter( pOwner, COLLISION_GROUP_NONE, pOwner->GetTeamNumber() );
		trace_t tr;
		UTIL_TraceLine( vecEye, vecEye + vecForward * MAX_TRACE_LENGTH, MASK_SOLID, &filter, &tr );
		vecDir = tr.endpos - pTarget->WorldSpaceCenter();
	}
	VectorNormalize( vecDir );

	// Send the entity back where it came.
	// If we want per-entity physical deflection behavior this could move into ::Deflected
	IPhysicsObject *pPhysicsObject = pTarget->VPhysicsGetObject();
	AngularImpulse angularimp;
	if ( pPhysicsObject )
	{
		pPhysicsObject->GetVelocity( &vecVel, &angularimp );
	}
	float flVel = vecVel.Length();
	vecVel = flVel * vecDir;
	if ( pPhysicsObject )
	{
		if ( pPhysicsObject->IsMotionEnabled() == false )
		{
			vecDir = pTarget->WorldSpaceCenter() - pOwner->WorldSpaceCenter();
			VectorNormalize( vecDir );

			vecVel = flVel * vecDir;
		}

		pPhysicsObject->EnableMotion( true );
		pPhysicsObject->SetVelocity( &vecVel, &angularimp );
	}
	else
	{
		pTarget->SetAbsVelocity( vecVel );
	}

	// Perform entity specific deflection behavior like team changing.
	pTarget->Deflected( pOwner, vecDir );

	QAngle newAngles;
	VectorAngles( vecDir, newAngles );
	pTarget->SetAbsAngles( newAngles );

	pOwner->AwardAchievement( ACHIEVEMENT_TF_PYRO_REFLECT_PROJECTILES );

	CDisablePredictionFiltering disabler;
	DispatchParticleEffect( "deflect_fx", PATTACH_ABSORIGIN_FOLLOW, pTarget );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Static deflection helper.
//-----------------------------------------------------------------------------
float CTFWeaponBase::DeflectionForce( const Vector &size, float damage, float scale )
{ 
	float force = damage * ((48 * 48 * 82.0) / (size.x * size.y * size.z)) * scale;

	if ( force > 1000.0) 
	{
		force = 1000.0;
	}

	return force;
}

//-----------------------------------------------------------------------------
// Purpose: Static deflection helper.
//-----------------------------------------------------------------------------
void CTFWeaponBase::SendObjectDeflectedEvent( CTFPlayer *pNewOwner, CTFPlayer *pPrevOwner, int iWeaponID, CBaseAnimating *pObject )
{
	if ( pNewOwner && pPrevOwner )
	{
		IGameEvent * event = gameeventmanager->CreateEvent( "object_deflected" );
		if ( event )
		{
			event->SetInt( "userid", pNewOwner->GetUserID() );
			event->SetInt( "ownerid", pPrevOwner->GetUserID() );
			event->SetInt( "weaponid", iWeaponID );

			// Community request. We don't use object_entindex, but some server plugins do.
			event->SetInt( "object_entindex", pObject ? pObject->entindex() : 0 );

			gameeventmanager->FireEvent( event );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:  Separate Regen function to handle item-specific cases
//-----------------------------------------------------------------------------
void CTFWeaponBase::ApplyItemRegen( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	m_flRegenTime += gpGlobals->frametime;
	if ( m_flRegenTime > 1.0f )
	{
		m_flRegenTime -= 1.0;

		float flRegenAmount = 0;
		CALL_ATTRIB_HOOK_FLOAT( flRegenAmount, active_item_health_regen );
		if ( (int)flRegenAmount != 0 )
		{
			pOwner->TakeDamage( CTakeDamageInfo( pOwner, pOwner, vec3_origin, WorldSpaceCenter(), (int)flRegenAmount * -1, DMG_GENERIC ) );

			IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
			if ( event )
			{
				event->SetInt( "amount", (int)flRegenAmount );
				event->SetInt( "entindex", pOwner->entindex() );
				item_definition_index_t healingItemDef = INVALID_ITEM_DEF_INDEX;
				if ( GetAttributeContainer() && GetAttributeContainer()->GetItem() )
				{
					healingItemDef = GetAttributeContainer()->GetItem()->GetItemDefIndex();
				}
				event->SetInt( "weapon_def_index", healingItemDef );
				gameeventmanager->FireEvent( event ); 
			}
		}
	}
}


kill_eater_event_t CTFWeaponBase::GetKillEaterKillEventType() const
{
	uint32 unEventType = kKillEaterEvent_PlayerKill;
	CALL_ATTRIB_HOOK_INT( unEventType, kill_eater_kill_type );
	return (kill_eater_event_t)unEventType;
}

#endif // GAME_DLL

bool CTFWeaponBase::IsSilentKiller()
{
	int iSilentKiller = 0;
	CALL_ATTRIB_HOOK_INT( iSilentKiller, set_silent_killer );
	if ( iSilentKiller == 1 )
		return true;
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose: Ensures that a player's correct body groups are enabled on client respawn.
//-----------------------------------------------------------------------------
void CTFWeaponBase::UpdateWeaponBodyGroups( CTFPlayer* pPlayer, bool bHandleDeployedBodygroups )
{
	if ( !pPlayer )
		return;

	for ( int i = 0; i < pPlayer->WeaponCount(); i++) 
	{
		CTFWeaponBase *pWpn = ( CTFWeaponBase *) pPlayer->GetWeapon(i);
		if ( !pWpn )
			continue;

		// If this weapon if repurposed for a taunt, dont modify bodygroups.  This is so
		// things like the Heavy's boxing gloves can change to a different model (ie. a guitar)
		// and then his hands will draw like normal
		if( pWpn->IsBeingRepurposedForTaunt() )
			continue;

		// Dynamic models which are not yet rendering do not modify bodygroups
		if ( pWpn->IsDynamicModelLoading() )
			continue;

		// These are updated later or have already been updated.
		CEconItemView *pScriptItem = pWpn->GetAttributeContainer()->GetItem();
		const bool bHideBodygroupsDeployedOnly = pScriptItem ? pScriptItem->GetStaticData()->GetHideBodyGroupsDeployedOnly() : false;

		if ( bHideBodygroupsDeployedOnly != bHandleDeployedBodygroups )
			continue;

		// If we're supposed to hide bodygroups when deployed and we aren't deployed, don't do anything.
		if ( bHideBodygroupsDeployedOnly && pPlayer->GetActiveWeapon() != pWpn )
			continue;
		
		pWpn->UpdateBodygroups( pPlayer, 1 );
	}
}


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Weapon Level Notification
//-----------------------------------------------------------------------------
class CTFKillEaterNotification : public CEconNotification
{
public:
	CTFKillEaterNotification( const CSteamID& KillerID, const wchar_t *wszWeaponName, const wchar_t *wszLevelName )
		: CEconNotification() 
	{
		SetLifetime( 20.0f );

		SetSteamID( KillerID );

		SetText( "#TF_HUD_Event_KillEater_Leveled" );

		AddStringToken( "weapon_name", wszWeaponName );

		AddStringToken( "rank_name", wszLevelName );

		SetSoundFilename( "misc/happy_birthday.wav" );
	}

	virtual EType NotificationType() { return eType_Basic; }
};

//-----------------------------------------------------------------------------
// Purpose: GC Msg handler to receive the server response that we've killed a player.
//-----------------------------------------------------------------------------
class CGCPlayerKilledResponse : public GCSDK::CGCClientJob
{
public:
	CGCPlayerKilledResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg<CMsgGCIncrementKillCountResponse> msg( pNetPacket );

		if ( !steamapicontext || !steamapicontext->SteamFriends() || !steamapicontext->SteamUser() || !steamapicontext->SteamUtils() )
			return true;

		item_definition_index_t unItemDef = msg.Body().item_def();
		const CEconItemDefinition* pItemDef = ItemSystem()->GetStaticDataForItemByDefIndex( unItemDef );
		if ( !pItemDef )
			return true;

		const char *pszKillerName = InventoryManager()->PersonaName_Get( msg.Body().killer_account_id() );
		if ( !pszKillerName )
			return true;

		wchar_t wszPlayerName[1024];
		g_pVGuiLocalize->ConvertANSIToUnicode( pszKillerName, wszPlayerName, sizeof(wszPlayerName) );

		wchar_t* wszWeaponName = g_pVGuiLocalize->Find( pItemDef->GetItemBaseName() );

		uint32 unLevelBlock = msg.Body().level_type();
		const char *pszLevelBlockName = GetItemSchema()->GetKillEaterScoreTypeLevelingDataName( unLevelBlock );
		
		const CItemLevelingDefinition *pLevelDef = GetItemSchema()->GetItemLevelForScore( pszLevelBlockName, msg.Body().num_kills() );
		if ( !pLevelDef )
			return true;

		wchar_t* wszLevelName = g_pVGuiLocalize->Find( pLevelDef->GetNameLocalizationKey() );

		// Kyle says: the notifications were annoying people so instead of doing a full
		//			  flashy thing we display a HUD message for everyone except the guy whose
		//			  weapon it is. Basically, *you* get the flashy notification that your
		//			  weapon leveled up, but everyone else just gets the HUD text.
		if ( steamapicontext->SteamUser()->GetSteamID().GetAccountID() == msg.Body().killer_account_id() )
		{
			NotificationQueue_Add( new CTFKillEaterNotification( CSteamID( msg.Body().killer_account_id(), GetUniverse(), k_EAccountTypeIndividual ), wszWeaponName, wszLevelName ) );
		}

		// Everyone gets the HUD notification text.
		CBaseHudChat *pHUDChat = (CBaseHudChat *)GET_HUDELEMENT( CHudChat );
		if ( pHUDChat )
		{
			wchar_t wszNotification[1024]=L"";
			g_pVGuiLocalize->ConstructString_safe( wszNotification, 
				g_pVGuiLocalize->Find( "#TF_HUD_Event_KillEater_Leveled_Chat" ), 
				3, wszPlayerName, wszWeaponName, wszLevelName );

			char szAnsi[1024];
			g_pVGuiLocalize->ConvertUnicodeToANSI( wszNotification, szAnsi, sizeof(szAnsi) );

			pHUDChat->Printf( CHAT_FILTER_NONE, "%s", szAnsi );
		}

		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCPlayerKilledResponse, "CGCPlayerKilledResponse", k_EMsgGC_IncrementKillCountResponse, GCSDK::k_EServerTypeGCClient );
#endif

bool WeaponID_IsSniperRifle( int iWeaponID )
{
	if ( iWeaponID == TF_WEAPON_SNIPERRIFLE ||
		iWeaponID == TF_WEAPON_SNIPERRIFLE_DECAP || 
		iWeaponID == TF_WEAPON_SNIPERRIFLE_CLASSIC )
		return true;
	else
		return false;
}

bool WeaponID_IsSniperRifleOrBow( int iWeaponID )
{
	if ( iWeaponID == TF_WEAPON_COMPOUND_BOW )
		return true;
	else
		return WeaponID_IsSniperRifle( iWeaponID );
}

bool CTFWeaponBase::IsPassiveWeapon( void ) const
{
	int iPassiveWeapon = 0;
	CALL_ATTRIB_HOOK_INT( iPassiveWeapon, is_passive_weapon );

	return iPassiveWeapon != 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFWeaponBase::Energy_GetMaxEnergy( void ) const
{
	// This is a terrible hack to support clip size upgrades.
	// Basically -- figure out the desired number of shots,
	// and return the amount of energy required for that.

	int iNumShots = ENERGY_WEAPON_MAX_CHARGE / Energy_GetShotCost();
	CALL_ATTRIB_HOOK_FLOAT( iNumShots, mult_clipsize_upgrade );

	return ( iNumShots * Energy_GetShotCost() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFWeaponBase::Energy_FullyCharged( void ) const
{
	if ( m_flEnergy >= Energy_GetMaxEnergy() )
		return true;
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFWeaponBase::Energy_HasEnergy( void )
{
	if ( m_flEnergy >= Energy_GetShotCost() )
		return true;
	else
		return false;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBase::Energy_DrainEnergy( void )
{
	Energy_DrainEnergy( Energy_GetShotCost() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBase::Energy_DrainEnergy( float flDrain )
{
	m_flEnergy -= flDrain;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFWeaponBase::Energy_Recharge( void )
{
	m_flEnergy += Energy_GetRechargeCost();
	if ( Energy_FullyCharged() )
	{
		m_flEnergy = Energy_GetMaxEnergy();
		return true;
	}
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBase::WeaponRegenerate( void )
{
	m_flEnergy = Energy_GetMaxEnergy();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBase::FinishReload( void )
{
	if ( IsEnergyWeapon() )
	{
		m_bInReload = false;
		return;
	}

	BaseClass::FinishReload();

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		int iAttr = 0;
		CALL_ATTRIB_HOOK_INT( iAttr, last_shot_crits );
		if ( iAttr )
		{
			if ( m_iClip1 == 1 )
			{
				pPlayer->m_Shared.AddCond( TF_COND_CRITBOOSTED );
			}
			else
			{
				pPlayer->m_Shared.RemoveCond( TF_COND_CRITBOOSTED );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBase::CheckReload( void )
{
	if ( IsEnergyWeapon() )
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		if ( !pOwner )
			return;

		if ( !Energy_HasEnergy() )
		{
			Reload();
			return;
		}

		if ((m_bInReload) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
		{
			if ( pOwner->m_nButtons & (IN_ATTACK | IN_ATTACK2) && Energy_HasEnergy() )
			{
				m_bInReload = false;
				return;
			}

			if ( !Energy_FullyCharged() )
			{
				Reload();
			}
			else
			{
				FinishReload();
				m_flNextPrimaryAttack	= gpGlobals->curtime;
				m_flNextSecondaryAttack = gpGlobals->curtime;
			}
		}
	}
	else
	{
		BaseClass::CheckReload();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the current bar state (will return a value from 0.0 to 1.0)
//-----------------------------------------------------------------------------
float CTFWeaponBase::GetEffectBarProgress( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && (pPlayer->GetAmmoCount( GetEffectBarAmmo() ) < pPlayer->GetMaxAmmo( GetEffectBarAmmo() )) )
	{
		float flTime = GetEffectBarRechargeTime();
		float flProgress = (flTime - (m_flEffectBarRegenTime - gpGlobals->curtime)) / flTime;
		return flProgress;
	}

	return 1.f;
}

//-----------------------------------------------------------------------------
// Purpose: Start the regeneration bar charging from this moment in time
//-----------------------------------------------------------------------------
void CTFWeaponBase::StartEffectBarRegen( void )
{
	// Only reset regen if its less then curr time or we were full
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	bool bWasFull = false;
	if ( pPlayer && (pPlayer->GetAmmoCount( GetEffectBarAmmo() ) + 1 == pPlayer->GetMaxAmmo( GetEffectBarAmmo() ) ) )
	{
		bWasFull = true;
	}

	if ( m_flEffectBarRegenTime < gpGlobals->curtime || bWasFull ) 
	{
		m_flEffectBarRegenTime = gpGlobals->curtime + GetEffectBarRechargeTime();
	}	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBase::CheckEffectBarRegen( void ) 
{ 
	if ( !m_flEffectBarRegenTime )
		return;
	
	// If we're full stop the timer.  Fixes a bug with "double" throws after respawning or touching a supply cab
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer->GetAmmoCount( GetEffectBarAmmo() ) == pPlayer->GetMaxAmmo( GetEffectBarAmmo() ) )
	{
		m_flEffectBarRegenTime = 0;
		return;
	}
	
	if ( m_flEffectBarRegenTime < gpGlobals->curtime ) 
	{
		m_flEffectBarRegenTime = 0;
		EffectBarRegenFinished();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBase::EffectBarRegenFinished( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && (pPlayer->GetAmmoCount( GetEffectBarAmmo() ) < pPlayer->GetMaxAmmo( GetEffectBarAmmo() )) )
	{
#ifdef GAME_DLL
		pPlayer->GiveAmmo( 1, GetEffectBarAmmo(), true );
#endif

#ifdef GAME_DLL
		// If we still have more ammo space, recharge
		if ( pPlayer->GetAmmoCount( GetEffectBarAmmo() ) < pPlayer->GetMaxAmmo( GetEffectBarAmmo() ) )
#else
		// On the client, we assume we'll get 1 more ammo as soon as the server updates us, so only restart if that still won't make us full.
		if ( pPlayer->GetAmmoCount( GetEffectBarAmmo() ) + 1 < pPlayer->GetMaxAmmo( GetEffectBarAmmo() ) )
#endif
		{
			StartEffectBarRegen();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CTFWeaponBase::GetParticleColor( int iColor )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return Vector(0,0,0);

	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( !pItem->IsValid() )
		return Vector(0,0,0);

	int iModifiedRGB = pItem->GetModifiedRGBValue( pOwner->GetTeamNumber() == TF_TEAM_BLUE );

	if ( iModifiedRGB > 0 )
	{
		Color clr = Color( ((iModifiedRGB & 0xFF0000) >> 16), ((iModifiedRGB & 0xFF00) >> 8), (iModifiedRGB & 0xFF) );

		float fColorMod = 1.f;
		if ( iColor == 2 )
		{
			fColorMod = 0.5f;
		}

		Vector vResult;
		vResult.x = clamp( fColorMod * clr.r() * (1.f/255), 0.f, 1.0f );
		vResult.y = clamp( fColorMod * clr.g() * (1.f/255), 0.f, 1.0f );
		vResult.z = clamp( fColorMod * clr.b() * (1.f/255), 0.f, 1.0f );
		return vResult;
	}

	if ( iColor == 1 )
	{
		if ( pOwner->GetTeamNumber() == TF_TEAM_RED )
			return TF_PARTICLE_WEAPON_RED_1;
		else
			return TF_PARTICLE_WEAPON_BLUE_1;
	}
	else
	{
		if ( pOwner->GetTeamNumber() == TF_TEAM_RED )
			return TF_PARTICLE_WEAPON_RED_2;
		else
			return TF_PARTICLE_WEAPON_BLUE_2;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::CanBeCritBoosted( void )
{
	int iNoCritBoost = 0;
	CALL_ATTRIB_HOOK_FLOAT( iNoCritBoost, no_crit_boost );
	return iNoCritBoost == 0;
}

bool CTFWeaponBase::CanHaveRevengeCrits( void )
{
	int iSapperCrits = 0;
	CALL_ATTRIB_HOOK_INT( iSapperCrits, sapper_kills_collect_crits );
	if ( iSapperCrits != 0 )
		return true;

	int iExtinguishCrits = 0;
	CALL_ATTRIB_HOOK_INT( iExtinguishCrits, extinguish_revenge );
	if ( iExtinguishCrits != 0 )
		return true;

	int iRevengeCrits = 0;
	CALL_ATTRIB_HOOK_INT( iRevengeCrits, sentry_killed_revenge );
	if ( iRevengeCrits )
		return true;
		
	return false;
}

const CEconItemView *CTFWeaponBase::GetTauntItem() const
{
	const CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem && pItem->IsValid() && pItem->GetItemDefinition()->GetTauntData() )
	{
		return pItem;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:  This is an accent sound that plays in addition to the base shoot sound
//-----------------------------------------------------------------------------
void CTFWeaponBase::PlayUpgradedShootSound( const char *pszSound )
{
	if ( TFGameRules()->GameModeUsesUpgrades() )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
		if ( pOwner )
		{
			float flDmgMod = 1.f;
			CALL_ATTRIB_HOOK_FLOAT( flDmgMod, mult_dmg );
			if ( flDmgMod > 1.f )
			{
				// This is pretty hacky as it assumes a cap of +100% damage for picking
				// sounds -- anything more and the 1-4 scale below falls apart.
				int nLevel = RemapValClamped( flDmgMod, 1.f, 1.8f, 1.f, 4.f );
				const char *pszSoundname = CFmtStr( "%s%d", pszSound, nLevel );

				CSoundParameters params;
				if ( !GetParametersForSound( pszSoundname, params, NULL ) )
					return;

				CPASAttenuationFilter filter( GetOwner(), params.soundlevel );
				if ( IsPredicted() && CBaseEntity::GetPredictionPlayer() )
				{
					filter.UsePredictionRules();
				}

				EmitSound( filter, pOwner->entindex(), pszSoundname );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:  Is this honorbound weapon?
//-----------------------------------------------------------------------------
bool CTFWeaponBase::IsHonorBound( void ) const
{
	int iHonorbound = 0;
	CALL_ATTRIB_HOOK_INT( iHonorbound, honorbound );
	return iHonorbound != 0;
}

EWeaponStrangeType_t CTFWeaponBase::GetStrangeType()
{
	// verify stattrak module and add if necessary
	if ( m_eStrangeType == STRANGE_UNKNOWN )
	{
		CEconItemView *pItem = GetAttributeContainer()->GetItem();
		if ( !pItem )
			return STRANGE_UNKNOWN;

		int iStrangeType = -1;
		for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
		{
			if ( pItem->FindAttribute( GetKillEaterAttr_Score( i ) ) )
			{
				iStrangeType = i;
				break;
			}
		}

		m_eStrangeType = iStrangeType == -1 ? STRANGE_NOT_STRANGE : STRANGE_IS_STRANGE;
	}

	return m_eStrangeType;
}

bool CTFWeaponBase::BHasStatTrakModule()
{
	if ( m_eStatTrakModuleType == MODULE_UNKNOWN )
	{
		CEconItemView *pItem = GetAttributeContainer()->GetItem();
		if ( !pItem )
			return false;

		EWeaponStrangeType_t eStrangeType = GetStrangeType();
		if ( eStrangeType != STRANGE_IS_STRANGE)
		{
			m_eStatTrakModuleType = MODULE_NONE;
			return false;
		}

		// Does it have a module
		if ( GetStattrak( pItem ) )
		{
			m_eStatTrakModuleType = MODULE_FOUND;
			return true;
		}
	}

	if ( m_eStatTrakModuleType != MODULE_FOUND )
		return false;

	return true;

}
#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
void CTFWeaponBase::UpdateAllViewmodelAddons( void )
{
	C_TFPlayer *pPlayer = ToTFPlayer( GetOwner() );

	// Remove any view model add ons if we're spectating.
	if ( !pPlayer )
	{
		RemoveViewmodelStatTrak();
		return;
	}

	// econ-related addons follow, so bail out if we can't get at the econitemview
	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( !pItem )
	{
		RemoveViewmodelStatTrak();
		return;
	}

	if ( GetStrangeType() > -1 )
	{
		CSteamID HolderSteamID;
		pPlayer->GetSteamID( &HolderSteamID );
		AddStatTrakModel( pItem, m_eStrangeType, HolderSteamID.GetAccountID() );
	}
	else
	{
		RemoveViewmodelStatTrak();
	}
}

// StatTrak Module Testing
void CTFWeaponBase::AddStatTrakModel( CEconItemView *pItem, int nStatTrakType, AccountID_t holderAcctId )
{
	// Already has module, just early out
	if ( m_viewmodelStatTrakAddon && m_viewmodelStatTrakAddon.Get() && m_viewmodelStatTrakAddon->GetMoveParent() )
	{
		return;
	}

	// Something is missing, remove and return
	if ( !pItem )
	{
		RemoveViewmodelStatTrak();
		RemoveWorldmodelStatTrak();
		return;
	}
	
	if ( GetStrangeType() != STRANGE_IS_STRANGE )
	{
		RemoveViewmodelStatTrak();
		RemoveWorldmodelStatTrak();
		return;
	}

	if ( !BHasStatTrakModule() )
	{
		RemoveViewmodelStatTrak();
		RemoveWorldmodelStatTrak();
		return;
	}

	// Get Module Data
	CAttribute_String attrModule;
	if ( !GetStattrak( pItem, &attrModule ) )
	{
		RemoveViewmodelStatTrak();
		RemoveWorldmodelStatTrak();
		return;
	}

	float flScale = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flScale, weapon_stattrak_module_scale );

	// Skin
	int nSkin = pItem->GetTeamNumber() - TF_TEAM_RED;
	if ( pItem->GetAccountID() != holderAcctId )
	{
		nSkin += 2;	// sad skin
	}

	// View Model / third person
	if ( GetViewmodelAttachment() )
	{
		// Already has a module, early out
		if ( !( m_viewmodelStatTrakAddon && m_viewmodelStatTrakAddon.Get() && m_viewmodelStatTrakAddon->GetMoveParent() ) )
		{
			RemoveViewmodelStatTrak();
		
			CTFWeaponAttachmentModel *pStatTrakEnt = new class CTFWeaponAttachmentModel;
			if ( pStatTrakEnt )
			{
				pStatTrakEnt->InitializeAsClientEntity( attrModule.value().c_str(), RENDER_GROUP_VIEW_MODEL_OPAQUE );
				
				pStatTrakEnt->Init( GetViewmodelAttachment(), this, true );
				pStatTrakEnt->m_nSkin = nSkin;
				m_viewmodelStatTrakAddon = pStatTrakEnt;
				
				if ( cl_flipviewmodels.GetBool() )
				{
					pStatTrakEnt->SetBodygroup( 1, 1 ); // use a special mirror-image stattrak module that appears correct for lefties
					flScale *= -1.0f;					// flip scale
				}

				pStatTrakEnt->SetModelScale( flScale );
				//RemoveEffects( EF_NODRAW );
			}
		}
	}

	// World Model
	if ( !(m_worldmodelStatTrakAddon && m_worldmodelStatTrakAddon.Get() && m_worldmodelStatTrakAddon->GetMoveParent() ) )
	{
		RemoveWorldmodelStatTrak();

		CTFWeaponAttachmentModel *pStatTrakEnt = new class CTFWeaponAttachmentModel;
		if ( pStatTrakEnt )
		{
			pStatTrakEnt->InitializeAsClientEntity( attrModule.value().c_str(), RENDER_GROUP_OPAQUE_ENTITY );
			pStatTrakEnt->SetModelScale( flScale );
			pStatTrakEnt->Init( this, this, false );
			pStatTrakEnt->m_nSkin = nSkin;
			m_worldmodelStatTrakAddon = pStatTrakEnt;
			
			
			//	//if ( !cl_flipviewmodels.GetBool() )
			//	//{
			//	//	pStatTrakEnt->SetBodygroup( 0, 1 ); // use a special mirror-image stattrak module that appears correct for lefties
			//	//}

			//RemoveEffects( EF_NODRAW );
		}
	}
	
}

//-----------------------------------------------------------------------------
void CTFWeaponBase::RemoveViewmodelStatTrak( void )
{
	if ( m_viewmodelStatTrakAddon.Get() )
	{
		m_viewmodelStatTrakAddon->Remove();
		m_viewmodelStatTrakAddon = NULL;
	}
}

//-----------------------------------------------------------------------------
void CTFWeaponBase::RemoveWorldmodelStatTrak( void )
{
	if ( m_worldmodelStatTrakAddon )
	{
		m_worldmodelStatTrakAddon->Remove();
		m_worldmodelStatTrakAddon = NULL;
	}
}

//-----------------------------------------------------------------------------
const Vector& CTFWeaponBase::GetViewmodelOffset()
{
	if ( !m_bInitViewmodelOffset )
	{
		CAttribute_String attr_min_viewmodel_offset;
		CALL_ATTRIB_HOOK_STRING( attr_min_viewmodel_offset, min_viewmodel_offset );
		const char* pszMinViewmodelOffset = attr_min_viewmodel_offset.value().c_str();
		if ( pszMinViewmodelOffset && *pszMinViewmodelOffset )
		{
			UTIL_StringToVector( m_vecViewmodelOffset.Base(), pszMinViewmodelOffset );
		}

		m_bInitViewmodelOffset = true;
	}

	return m_vecViewmodelOffset;
}

//-----------------------------------------------------------------------------
// CTFWeaponAttachmentModel
//-----------------------------------------------------------------------------
void CTFWeaponAttachmentModel::Init( CBaseEntity *pParent, CTFWeaponBase *pAssociatedWeapon, bool bIsViewModel )
{
	SetParent( pParent );
	SetLocalOrigin( vec3_origin );
	UpdatePartitionListEntry();
	CollisionProp()->MarkPartitionHandleDirty();
	//UpdateVisibility();
	SetWeaponAssociatedWith( pAssociatedWeapon );

	AddEffects( EF_BONEMERGE );
	AddEffects( EF_BONEMERGE_FASTCULL );
	AddEffects( EF_NODRAW );

	m_bIsViewModelAttachment = bIsViewModel;
}

//-----------------------------------------------------------------------------
bool CTFWeaponAttachmentModel::ShouldDraw( void )
{
	// Follow my associated weapon
	if ( !m_hWeaponAssociatedWith.Get() )
		return false;

	// some code is overriding the weapon model (taunt), don't show the attachment model
	if ( m_hWeaponAssociatedWith->IsUsingOverrideModel() )
		return false;

	if ( m_hWeaponAssociatedWith->IsFirstPersonView() && !m_bIsViewModelAttachment )
	{
		return false;
	}

	bool bShouldDraw = m_hWeaponAssociatedWith->ShouldDraw();
	if ( bShouldDraw )
	{
		return !m_bIsViewModelAttachment;
	}
	return false;

	//if ( pWeapon )
	//{
	//	// If the weapon isn't active, don't draw
	//	if ( pOwner && pOwner->GetActiveWeapon() != pWeapon )
	//	{
	//		return false;
	//	}

	//	if ( !IsViewModelWearable() )
	//	{
	//		// If it's the 3rd person wearable, don't draw it when the weapon is hidden
	//		if ( !pWeapon->ShouldDraw() )
	//		{
	//			return false;
	//		}
	//	}

	//	// If the weapon is being repurposed for a taunt dont draw.
	//	// The Brutal Legend taunt changes your weapon's model to be the guitar,
	//	// but we dont want things like bot-killer skulls or festive lights
	//	// to continue to draw
	//	if ( pWeapon->IsBeingRepurposedForTaunt() )
	//	{
	//		return false;
	//	}
	//}
	//
}

#endif // CLIENT_DLL
