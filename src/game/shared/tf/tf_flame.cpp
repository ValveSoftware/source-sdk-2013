//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_flame.h"
#include "debugoverlay_shared.h"
#include "tf_gamerules.h"
#include "shot_manipulator.h"
#include "tf_weapon_flamethrower.h"

#ifdef GAME_DLL
#include "tf_player.h"
#include "tf_weapon_compound_bow.h"
#include "tf_logic_robot_destruction.h"
#include "ispatialpartition.h"
#include "tf_fx.h"
#endif // GAME_DLL

#ifdef CLIENT_DLL
#include "in_buttons.h"
#endif // CLIENT_DLL

const float tf_flame_burn_index_drain_rate = 1.25f;
const float tf_flame_burn_index_per_collide = 1.f;
const float tf_flame_burn_index_per_collide_remap_x = 10.f;
const float tf_flame_burn_index_per_collide_remap_y = 50.f;
const float tf_flame_burn_index_damage_scale_min = 0.5f;

ConVar tf_flame_dmg_mode_dist( "tf_flame_dmg_mode_dist", "0", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY | FCVAR_HIDDEN );

#ifdef WATERFALL_FLAMETHROWER_TEST
ConVar tf_flame_waterfall_speed_override( "tf_flame_waterfall_speed_override", "0", FCVAR_REPLICATED );
ConVar tf_flame_waterfall_drag_override( "tf_flame_waterfall_drag_override", "0", FCVAR_REPLICATED );
ConVar tf_flame_waterfall_gravity_override( "tf_flame_waterfall_gravity_override", "0", FCVAR_REPLICATED );
ConVar tf_flame_waterfall_lifetime_override( "tf_flame_waterfall_lifetime_override", "0", FCVAR_REPLICATED,
                                             "Time to live of flame damage entities." );
ConVar tf_flame_waterfall_spread( "tf_flame_waterfall_spread", "40", FCVAR_REPLICATED,
                                         "Spread angle of flame in waterfall mode." );
#endif // WATERFALL_FLAMETHROWER_TEST

extern ConVar tf_debug_flamethrower;
extern ConVar tf_flamethrower_boxsize;

#ifdef CLIENT_DLL
float tf_flame_particle_min_density = 0.01f;

#else // CLIENT_DLL

const float tf_flame_min_damage_scale = 0.5f;
const float tf_flame_maxdamagedist = 150.f;
const float tf_flame_mindamagedist = 300.f;
const float tf_flame_min_damage_scale_time = 0.5f;
const float tf_flame_min_damage_scale_time_cap = 0.5f;
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFFlameManager, DT_TFFlameManager );

BEGIN_NETWORK_TABLE( CTFFlameManager, DT_TFFlameManager )
#ifdef GAME_DLL
	SendPropEHandle( SENDINFO( m_hWeapon ) ),
	SendPropEHandle( SENDINFO( m_hAttacker ) ),

	SendPropFloat(	SENDINFO( m_flSpreadDegree ) ),
	SendPropFloat(	SENDINFO( m_flRedirectedFlameSizeMult ) ),
	SendPropFloat(	SENDINFO( m_flFlameStartSizeMult ) ),
	SendPropFloat(	SENDINFO( m_flFlameEndSizeMult ) ),
	SendPropFloat(	SENDINFO( m_flFlameIgnorePlayerVelocity ) ),
	SendPropFloat(	SENDINFO( m_flFlameReflectionAdditionalLifeTime ) ),
	SendPropFloat(	SENDINFO( m_flFlameReflectionDamageReduction ) ),
	SendPropInt(	SENDINFO( m_iMaxFlameReflectionCount ) ),
	SendPropInt(	SENDINFO( m_nShouldReflect ) ),
	SendPropFloat(	SENDINFO( m_flFlameSpeed ) ),
	SendPropFloat(	SENDINFO( m_flFlameLifeTime ) ),
	SendPropFloat(	SENDINFO( m_flRandomLifeTimeOffset ) ),
	SendPropFloat(	SENDINFO( m_flFlameGravity ) ),
	SendPropFloat(	SENDINFO( m_flFlameDrag ) ),
	SendPropFloat(	SENDINFO( m_flFlameUp ) ),

	SendPropBool(	SENDINFO( m_bIsFiring ) ),


#ifdef WATERFALL_FLAMETHROWER_TEST
	SendPropInt(	SENDINFO( m_iWaterfallMode ) ),
	SendPropArray(	SendPropEHandle( SENDINFO_ARRAY( m_hAdditionalFlameManagers ) ), m_hAdditionalFlameManagers ),
	SendPropInt(	SENDINFO( m_iStreamIndex ) ),
#endif // WATERFALL_FLAMETHROWER_TEST

#else
	RecvPropEHandle( RECVINFO( m_hWeapon ) ),
	RecvPropEHandle( RECVINFO( m_hAttacker ) ),

	RecvPropFloat(	RECVINFO( m_flSpreadDegree ) ),
	RecvPropFloat(	RECVINFO( m_flRedirectedFlameSizeMult ) ),
	RecvPropFloat(	RECVINFO( m_flFlameStartSizeMult ) ),
	RecvPropFloat(	RECVINFO( m_flFlameEndSizeMult ) ),
	RecvPropFloat(	RECVINFO( m_flFlameIgnorePlayerVelocity ) ),
	RecvPropFloat(	RECVINFO( m_flFlameReflectionAdditionalLifeTime ) ),
	RecvPropFloat(	RECVINFO( m_flFlameReflectionDamageReduction ) ),
	RecvPropInt(	RECVINFO( m_iMaxFlameReflectionCount ) ),
	RecvPropInt(	RECVINFO( m_nShouldReflect ) ),
	RecvPropFloat(	RECVINFO( m_flFlameSpeed ) ),
	RecvPropFloat(	RECVINFO( m_flFlameLifeTime ) ),
	RecvPropFloat(	RECVINFO( m_flRandomLifeTimeOffset ) ),
	RecvPropFloat(	RECVINFO( m_flFlameGravity ) ),
	RecvPropFloat(	RECVINFO( m_flFlameDrag ) ),
	RecvPropFloat(	RECVINFO( m_flFlameUp ) ),

	RecvPropBool(	RECVINFO( m_bIsFiring ) ),


#ifdef WATERFALL_FLAMETHROWER_TEST
	RecvPropInt(	RECVINFO( m_iWaterfallMode ) ),
	RecvPropArray(	RecvPropEHandle( RECVINFO( m_hAdditionalFlameManagers[0] ) ), m_hAdditionalFlameManagers ),
	RecvPropInt(	RECVINFO( m_iStreamIndex ) ),
#endif // WATERFALL_FLAMETHROWER_TEST

#endif
END_NETWORK_TABLE()

BEGIN_DATADESC( CTFFlameManager )
END_DATADESC()


#if defined( CLIENT_DLL )
BEGIN_PREDICTION_DATA( CTFFlameManager )
	DEFINE_PRED_FIELD( m_bIsFiring, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_flame_manager, CTFFlameManager );

IMPLEMENT_AUTO_LIST( ITFFlameManager );

CTFFlameManager::CTFFlameManager()
#ifdef GAME_DLL
	: m_mapEntitiesBurnt( DefLessFunc(EHANDLE) )
#endif // GAME_DLL
{
	// default attr
	m_flSpreadDegree = 0.f;
	m_flRedirectedFlameSizeMult = 1.f;
	m_flFlameStartSizeMult = 1.f;
	m_flFlameEndSizeMult = 1.f;
	m_flFlameIgnorePlayerVelocity = 0.f;
	m_flFlameReflectionAdditionalLifeTime = 0.f;
	m_flFlameReflectionDamageReduction = 1.f;
	m_iMaxFlameReflectionCount = 0;
	m_nShouldReflect = 0;
	m_flFlameSpeed = 0.f;
	m_flFlameLifeTime = 0.f;
	m_flRandomLifeTimeOffset = 0.f;
	m_flFlameGravity = 0.f;
	m_flFlameDrag = 0.f;
	m_flFlameUp = 0.f;

	m_bIsFiring = false;

#ifdef WATERFALL_FLAMETHROWER_TEST
	m_iWaterfallMode = 0;
	m_iStreamIndex = -1;
#endif // WATERFALL_FLAMETHROWER_TEST

#ifdef CLIENT_DLL
	m_nMuzzleAttachment = INVALID_PARTICLE_ATTACHMENT;
#endif // CLIENT_DLL
}

void CTFFlameManager::UpdateOnRemove( void )
{
#ifdef CLIENT_DLL
	RemoveAllParticles();
#endif // CLIENT_DLL

#ifdef GAME_DLL
#ifdef WATERFALL_FLAMETHROWER_TEST
	// This is temp -- if we want to ship this the multiple streams should be at flame manager level instead of
	// creating many entities/particle-systems/etc
	for ( int idx = 0; idx < m_hAdditionalFlameManagers.Count(); idx++ )
	{
		auto &hFM = m_hAdditionalFlameManagers[idx];
		if ( hFM )
		{
			UTIL_Remove( hFM );
			m_hAdditionalFlameManagers.Set( idx, NULL );
		}
	}
#endif // WATERFALL_FLAMETHROWER_TEST
#endif // GAME_DLL

	BaseClass::UpdateOnRemove();
}

void CTFFlameManager::StartFiring()
{
	if ( m_bIsFiring )
		return;

	m_bIsFiring = true;

#ifdef CLIENT_DLL
	OnFiringStateChange();
#endif // CLIENT_DLL
}

void CTFFlameManager::StopFiring()
{
	if ( !m_bIsFiring )
		return;

	m_bIsFiring = false;

#ifdef CLIENT_DLL
	OnFiringStateChange();
#endif // CLIENT_DLL
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlameManager::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_bOldFiring = m_bIsFiring;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlameManager::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_bOldFiring != m_bIsFiring )
	{
		OnFiringStateChange();
	}
}

#endif // CLIENT_DLL

void DebugFlame( const Vector& vStart, const Vector& vEnd, const Vector& vMin, const Vector& vMax, const Color& color )
{
	NDebugOverlay::Line( vStart, vEnd, 255, 255, 0, false, 0.0f );
	NDebugOverlay::SweptBox( vStart, vEnd, vMin, vMax, vec3_angle, color.r(), color.g(), color.b(), color.a(), 0.0f );
}

void CTFFlameManager::InitializePoint( tf_point_t *pPoint, int nPointIndex )
{
	BaseClass::InitializePoint( pPoint, nPointIndex );

	flame_point_t *pFlame = static_cast< flame_point_t * >( pPoint );
	if ( m_hAttacker )
	{
		pFlame->m_vecAttackerVelocity = m_hAttacker->GetLocalVelocity();
		pFlame->m_vecInitialPos = pPoint->m_vecPosition;
	}
}


Vector CTFFlameManager::GetInitialPosition() const
{
	if ( GetOwnerEntity() )
	{
		CTFFlameThrower *pFlameThrower = assert_cast< CTFFlameThrower* >( GetOwnerEntity() );
		if ( pFlameThrower )
		{
			return pFlameThrower->GetFlameOriginPos();
		}
	}

	return BaseClass::GetInitialPosition();
}


Vector CTFFlameManager::GetInitialVelocity() const
{
	if ( m_hAttacker && m_hAttacker->IsPlayer() )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( m_hAttacker );
		if ( pTFPlayer )
		{
			QAngle angFlame = pTFPlayer->EyeAngles();
#ifdef WATERFALL_FLAMETHROWER_TEST
			if ( m_iWaterfallMode && m_iStreamIndex >= 0 )
			{
				const int nAdditionalStreams = m_hAdditionalFlameManagers.Count();
				Assert( nAdditionalStreams == WATERFALL_FLAMETHROWER_STREAMS - 1 );
				Assert( nAdditionalStreams % 2 == 0 ); // Assuming an odd number of total streams
				// Total spread
				int nDegSpread = tf_flame_waterfall_spread.GetInt();
				// Variance per flame to fill out the fan effect
				int nPerFlameSpread = nDegSpread / WATERFALL_FLAMETHROWER_STREAMS;

				// The central stream is always firing forward, so insert a phantom offset halfway through the
				// range, then remap the range to the spread.
				int streamNum = m_iStreamIndex + ( m_iStreamIndex >= nAdditionalStreams / 2 );
				int degRandOffset = m_randomStream.RandomInt( -nPerFlameSpread / 2, nPerFlameSpread / 2 );

				int degStream = RemapValClamped( streamNum, 0, nAdditionalStreams, -nDegSpread / 2, nDegSpread / 2);
				angFlame[YAW] += degStream + degRandOffset;
			}
#endif // WATERFALL_FLAMETHROWER_TEST

			Vector vDirection;
			AngleVectors( angFlame, &vDirection );

			// apply spread to velocity if needed
			if ( m_flSpreadDegree > 0.f )
			{
				CShotManipulator shotManipulator( vDirection );
				float flSpreadDeg2Rad = DEG2RAD( m_flSpreadDegree );
				vDirection = shotManipulator.ApplySpread( Vector( flSpreadDeg2Rad, flSpreadDeg2Rad, flSpreadDeg2Rad ), 1.f, &m_randomStream );
			}
			return GetInitialSpeed() * vDirection;
		}
	}

	return BaseClass::GetInitialVelocity();
}

#ifdef GAME_DLL

class CFlameManagerHelper : public CAutoGameSystemPerFrame
{
public:
	CFlameManagerHelper( const char *pszName ) : CAutoGameSystemPerFrame( pszName )
	{
	}

	// called after entities think
	virtual void FrameUpdatePostEntityThink() OVERRIDE
	{
		FOR_EACH_VEC( ITFFlameManager::AutoList(), i )
		{
			CTFFlameManager *pFlameManager = static_cast< CTFFlameManager* >( ITFFlameManager::AutoList()[i] );
			pFlameManager->PostEntityThink();
		}
	}
};
static CFlameManagerHelper s_flameManagerHelper( "flame_manager_helper" );

void CTFFlameManager::PostEntityThink( void )
{
	// remove all entities that are no longer touching the flame
	FOR_EACH_MAP_FAST( m_mapEntitiesBurnt, i )
	{
		if ( gpGlobals->curtime - m_mapEntitiesBurnt[i].m_flLastBurnTime > m_flBurnFrequency )
		{
			m_mapEntitiesBurnt.RemoveAt( i );

			// go thru the map again to see if we need to remove anything else
			i = 0;
		}
	}

	FOR_EACH_MAP_FAST( m_mapEntitiesBurnt, i )
	{
		// Decay heat index (colliding refreshes it)
		// TODO(driller): factor in time since last think
		m_mapEntitiesBurnt[i].m_flHeatIndex = Max( m_mapEntitiesBurnt[i].m_flHeatIndex - tf_flame_burn_index_drain_rate, 0.f );
	}

	CTFFlameThrower *pFlameThrower = NULL;
	if ( m_hWeapon.Get() && m_hWeapon.Get()->GetWeaponID() == TF_WEAPON_FLAMETHROWER )
	{
		pFlameThrower = assert_cast< CTFFlameThrower* >( m_hWeapon.Get() );
	}

	if ( pFlameThrower )
	{
		pFlameThrower->ResetFlameHitCount();
		if ( m_mapEntitiesBurnt.Count() > 0 )
		{
			// this is to keep focus burn sound the same as the old flame
			pFlameThrower->IncrementFlameDamageCount();
			pFlameThrower->IncrementActiveFlameCount();
		}
	}
}

CTFFlameManager* CTFFlameManager::Create( CBaseEntity *pOwner, bool bIsAdditionalManager /*= false*/ )
{
	CTFFlameManager *pFlameManager = static_cast<CTFFlameManager*>( CBaseEntity::Create( "tf_flame_manager", vec3_origin, vec3_angle, pOwner ) );
	if ( pFlameManager )
	{
		// Initialize the owner.
		pFlameManager->SetOwnerEntity( pOwner );
		if ( pOwner->GetOwnerEntity() )
			pFlameManager->m_hAttacker = pOwner->GetOwnerEntity();
		else
			pFlameManager->m_hAttacker = pOwner;

		// Track total active flame entities
		pFlameManager->m_hWeapon = dynamic_cast< CTFWeaponBase* >( pOwner );

		// Set team.
		pFlameManager->ChangeTeam( pOwner->GetTeamNumber() );

		pFlameManager->HookAttributes();

#ifdef WATERFALL_FLAMETHROWER_TEST
		if ( !bIsAdditionalManager )
		{
			// Temp prototype hack, going to re-do properly if we want to keep this
			//
			// This is temp -- if we want to ship this the multiple streams should be at flame manager level instead of
			// creating many entities/particle-systems/etc
			if ( pFlameManager->m_iWaterfallMode )
			{
				for ( int idx = 0; idx < pFlameManager->m_hAdditionalFlameManagers.Count(); idx++ )
				{
					auto &hFM = pFlameManager->m_hAdditionalFlameManagers[idx];
					if ( !hFM )
					{
						CTFFlameManager *pAdditionalFlameManager = CTFFlameManager::Create( pOwner, true );
						if ( pAdditionalFlameManager )
						{
							pFlameManager->m_hAdditionalFlameManagers.Set( idx, pAdditionalFlameManager );
							pAdditionalFlameManager->m_iStreamIndex = idx;
						}
					}
				}
			}
		}
#endif // WATERFALL_FLAMETHROWER_TEST
	}

	return pFlameManager;
}


bool CTFFlameManager::AddPoint( int iCurrentTick )
{
	bool bRet = BaseClass::AddPoint( iCurrentTick );

#ifdef WATERFALL_FLAMETHROWER_TEST
	// Temp prototype hack, going to re-do properly if we want to keep this
	//
	// This is temp -- if we want to ship this the multiple streams should be at flame manager level instead of
	// creating many entities/particle-systems/etc
	if ( m_iWaterfallMode )
	{
		for ( int idx = 0; idx < m_hAdditionalFlameManagers.Count(); idx++ )
		{
			auto &hFM = m_hAdditionalFlameManagers[idx];
			if ( hFM )
			{
				bRet &= hFM->AddPoint( iCurrentTick );
			}
		}
	}
#endif // WATERFALL_FLAMETHROWER_TEST
	
	return bRet;
}


void CTFFlameManager::HookAttributes()
{
	// cache all attrs
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(	m_hAttacker,	m_flSpreadDegree,						flame_spread_degree );
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(	m_hAttacker,	m_flRedirectedFlameSizeMult,			redirected_flame_size_mult );
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(	m_hAttacker,	m_flFlameStartSizeMult,					mult_flame_size );
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(	m_hAttacker,	m_flFlameEndSizeMult,					mult_end_flame_size );
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(	m_hAttacker,	m_flFlameIgnorePlayerVelocity,			flame_ignore_player_velocity );
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(	m_hAttacker,	m_flFlameReflectionAdditionalLifeTime,	flame_reflection_add_life_time );
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(	m_hAttacker,	m_flFlameReflectionDamageReduction,		reflected_flame_dmg_reduction );
	CALL_ATTRIB_HOOK_INT_ON_OTHER(		m_hAttacker,	m_iMaxFlameReflectionCount,				max_flame_reflection_count );
	CALL_ATTRIB_HOOK_INT_ON_OTHER(		m_hAttacker,	m_nShouldReflect,						flame_reflect_on_collision );
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(	m_hAttacker,	m_flFlameSpeed,							flame_speed );
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(	m_hAttacker,	m_flFlameLifeTime,						flame_lifetime );
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(	m_hAttacker,	m_flRandomLifeTimeOffset,				flame_random_life_time_offset );
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(	m_hAttacker,	m_flFlameGravity,						flame_gravity );
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(	m_hAttacker,	m_flFlameDrag,							flame_drag );
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(	m_hAttacker,	m_flFlameUp,							flame_up_speed );


#ifdef WATERFALL_FLAMETHROWER_TEST
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(	m_hAttacker,	m_iWaterfallMode,						flame_waterfall );
#endif // WATERFALL_FLAMETHROWER_TEST
}

void CTFFlameManager::UpdateDamage( int iDmgType, float flDamage, float flBurnFrequency, bool bCritFromBehind )
{
	m_iDmgType = iDmgType;
	m_flDamage = flDamage;
	m_flBurnFrequency = flBurnFrequency;
	m_bCritFromBehind = bCritFromBehind;

#ifdef WATERFALL_FLAMETHROWER_TEST
	if ( m_iWaterfallMode )
	{
		for ( int idx = 0; idx < m_hAdditionalFlameManagers.Count(); idx++ )
		{
			auto &hFM = m_hAdditionalFlameManagers[idx];
			if ( hFM )
			{
				hFM->UpdateDamage( iDmgType, flDamage, flBurnFrequency, bCritFromBehind );
			}
		}
	}
#endif // WATERFALL_FLAMETHROWER_TEST
}

float CTFFlameManager::GetFlameDamageScale( const tf_point_t* pPoint, CTFPlayer *pTFTarget /*= NULL*/ ) const
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	const flame_point_t *pFlame = static_cast< const flame_point_t * >( pPoint );
	
	float flDamageScale = 1.f;

	// Distance-based calculation is what we shipped with
	if ( tf_flame_dmg_mode_dist.GetBool() )
	{
		float flDistSqr = pFlame->m_vecPosition.DistToSqr( pFlame->m_vecInitialPos );
		float flMaxDamageDistSqr = Square( tf_flame_maxdamagedist );
		float flMinDamageDistSqr = Square( tf_flame_mindamagedist );
		flDamageScale = RemapValClamped( flDistSqr, flMaxDamageDistSqr, flMinDamageDistSqr, 1.0f, tf_flame_min_damage_scale );
	}
	// Lifetime-based
	else
	{
		float flTimeAlive = gpGlobals->curtime - pFlame->m_flSpawnTime;
		float flLifeMax = pFlame->m_flLifeTime * tf_flame_min_damage_scale_time_cap;
		flDamageScale = RemapValClamped( flTimeAlive, 0.f, flLifeMax, 1.f, tf_flame_min_damage_scale_time );
	}

	if ( pTFTarget 
		)
	{
		float flIndexMod = 1.f;
		auto iEntIndex = m_mapEntitiesBurnt.Find( pTFTarget );
		if ( iEntIndex != m_mapEntitiesBurnt.InvalidIndex() )
		{
			flIndexMod = RemapValClamped( m_mapEntitiesBurnt[iEntIndex].m_flHeatIndex, 
										  tf_flame_burn_index_per_collide_remap_x, tf_flame_burn_index_per_collide_remap_y, 
										  tf_flame_burn_index_damage_scale_min, 1.f );
		}

		flDamageScale *= flIndexMod;
	}

	// should we reduce damage based on reflection?
	for ( int i = 0; i<pPoint->m_nHitWall; ++i )
	{
		flDamageScale *= ReflectionDamageReduction();
	}

	return flDamageScale;
}

bool CTFFlameManager::BCanBurnEntityThisFrame( CBaseEntity *pEnt ) const
{
	auto iBurnt = m_mapEntitiesBurnt.Find( pEnt );
	if ( iBurnt == m_mapEntitiesBurnt.InvalidIndex() )
		return true;

	float flLastBurnTime = m_mapEntitiesBurnt[iBurnt].m_flLastBurnTime;
	return ( gpGlobals->curtime - flLastBurnTime >= m_flBurnFrequency );
}

void CTFFlameManager::SetHitTarget( void )
{
	if ( !m_hWeapon.Get() )
		return;

	if ( m_hWeapon.Get()->GetWeaponID() == TF_WEAPON_FLAMETHROWER )
	{
		assert_cast< CTFFlameThrower* >( m_hWeapon.Get() )->SetHitTarget();
	}
}

bool CFlameEntityEnum::EnumEntity( IHandleEntity *pHandleEntity )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	CBaseEntity *pEnt = static_cast< CBaseEntity* >( pHandleEntity );

	// Ignore collisions with the shooter
	if ( pEnt == m_pShooter )
		return true;

	if ( pEnt->IsPlayer() && pEnt->IsAlive() )
	{
		m_Targets.AddToTail( pEnt );
	}
	else if ( pEnt->MyNextBotPointer() && pEnt->IsAlive() )
	{
		// add non-player bots
		m_Targets.AddToTail( pEnt );
	}
	else if ( pEnt->IsBaseObject() && m_pShooter->GetTeamNumber() != pEnt->GetTeamNumber() )
	{
		// only add enemy objects
		m_Targets.AddToTail( pEnt );
	}
	else if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() && m_pShooter->GetTeamNumber() != pEnt->GetTeamNumber() && FClassnameIs( pEnt, "tf_robot_destruction_robot" ) )
	{
		// only add enemy robots
		m_Targets.AddToTail( pEnt );
	}
	else if ( FClassnameIs( pEnt, "func_breakable" ) || FClassnameIs( pEnt, "tf_pumpkin_bomb" ) || FClassnameIs( pEnt, "tf_merasmus_trick_or_treat_prop" ) || FClassnameIs( pEnt, "tf_generic_bomb" ) )
	{
		m_Targets.AddToTail( pEnt );
	}

	return true;
}

bool CTFFlameManager::IsValidBurnTarget( CBaseEntity *pEntity ) const
{
	// is our attacker still valid?
	if ( !m_hAttacker.Get() )
		return false;

	// Ignore collisions with the shooter
	if ( pEntity == m_hAttacker )
		return false;

	if ( pEntity->IsPlayer() && pEntity->IsAlive() )
	{
		return true;
	}
	else if ( pEntity->MyNextBotPointer() && pEntity->IsAlive() )
	{
		// add non-player bots
		return true;
	}
	else if ( pEntity->IsBaseObject() && m_hAttacker->GetTeamNumber() != pEntity->GetTeamNumber() )
	{
		// only add enemy objects
		return true;
	}
	else if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() && m_hAttacker->GetTeamNumber() != pEntity->GetTeamNumber() && FClassnameIs( pEntity, "tf_robot_destruction_robot" ) )
	{
		// only add enemy robots
		return true;
	}
	else if ( FClassnameIs( pEntity, "func_breakable" ) || FClassnameIs( pEntity, "tf_pumpkin_bomb" ) || FClassnameIs( pEntity, "tf_merasmus_trick_or_treat_prop" ) || FClassnameIs( pEntity, "tf_generic_bomb" ) )
	{
		return true;
	}

	return false;
}

bool CTFFlameManager::ShouldCollide( CBaseEntity *pEnt ) const
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	if ( !IsValidBurnTarget( pEnt ) )
		return false;

#ifdef WATERFALL_FLAMETHROWER_TEST
	if ( m_iWaterfallMode )
	{
		for ( int idx = 0; idx < m_hAdditionalFlameManagers.Count(); idx++ )
		{
			auto &hFM = m_hAdditionalFlameManagers[idx];
			if ( hFM && !hFM->BCanBurnEntityThisFrame( pEnt ) )
			{
				return false;
			}
		}
	}
#endif // WATERFALL_FLAMETHROWER_TEST

	return true;
}

void CTFFlameManager::OnCollide( CBaseEntity *pEnt, int iPointIndex )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	CBaseEntity *pAttacker = m_hAttacker;
	if ( !pAttacker )
		return;

	const flame_point_t *pFlame = static_cast<const flame_point_t *>( GetPointVec()[iPointIndex] );

	// Do this each touch - even if we're not doing damage - to generate a heat index (rough approximation of density/accuracy - per-target)
	int iEntIndex = m_mapEntitiesBurnt.Find( pEnt );
	if ( iEntIndex != m_mapEntitiesBurnt.InvalidIndex() )
	{
		float flTimeAlive = gpGlobals->curtime - pFlame->m_flSpawnTime;

		float flAmount = RemapValClamped( flTimeAlive, 0.f, 0.02f, ( tf_flame_burn_index_per_collide * 2.f ), tf_flame_burn_index_per_collide );

		m_mapEntitiesBurnt[iEntIndex].m_flHeatIndex += flAmount;
	}

	// if we already burn this entity, check if we can burn it again
	if ( !BCanBurnEntityThisFrame( pEnt ) )
		return;

	if ( pEnt->IsPlayer() && pEnt->InSameTeam( pAttacker ) )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pEnt );

		// Only care about Snipers
		if ( !pPlayer->IsPlayerClass(TF_CLASS_SNIPER) )
			return;

		// Does he have the bow?
		CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
		if ( pWpn && pWpn->GetWeaponID() == TF_WEAPON_COMPOUND_BOW )
		{
			CTFCompoundBow *pBow = static_cast<CTFCompoundBow*>( pWpn );
			pBow->SetArrowAlight( true );
		}
	}
	else
	{
		SetHitTarget();

		int iDamageType = m_iDmgType;
		CTFPlayer *pVictim = NULL;

		if ( pEnt->IsPlayer() )
		{
			pVictim = ToTFPlayer( pEnt );

			// get direction of touching flame from initial pos to current pos
			Vector vFlameGeneralDir = ( pFlame->m_vecPosition - pFlame->m_vecInitialPos ).Normalized();

			Vector vOtherForward;
			AngleVectors( pEnt->GetAbsAngles(), &vOtherForward ); 
			vOtherForward.z = 0;
			vOtherForward.NormalizeInPlace();

			const float flBehindThreshold = 0.8f;

			// check if we're behind the victim
			if ( DotProduct( vFlameGeneralDir, vOtherForward ) > flBehindThreshold )
			{
				if ( m_bCritFromBehind == true )
				{
					iDamageType |= DMG_CRITICAL;
				}

				if ( pVictim )
				{
					pVictim->HandleAchievement_Pyro_BurnFromBehind( ToTFPlayer( pAttacker ) );
				}
			}

			// Pyro-specific
			if ( pAttacker->IsPlayer() && pVictim )
			{
				CTFPlayer *pPlayerAttacker = ToTFPlayer( pAttacker );
				if ( pPlayerAttacker && pPlayerAttacker->IsPlayerClass( TF_CLASS_PYRO ) )
				{
					// burn the victim while taunting?
					if ( pVictim->m_Shared.InCond( TF_COND_TAUNTING ) )
					{
						static CSchemaItemDefHandle flipTaunt( "Flippin' Awesome Taunt" );
						// if I'm the one being flipped, and getting lit on fire
						if ( !pVictim->IsTauntInitiator() && pVictim->GetTauntEconItemView() && pVictim->GetTauntEconItemView()->GetItemDefinition() == flipTaunt )
						{
							pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_PYRO_IGNITE_PLAYER_BEING_FLIPPED );
						}
					}

					pVictim->m_Shared.AddCond( TF_COND_HEALING_DEBUFF, 2.f, pAttacker );
				}
			}
		}

		// make sure damage is at least 1
		float flDamageScale = GetFlameDamageScale( pFlame, pVictim );
		float flDamage = MAX( flDamageScale * m_flDamage, 1.f );
		CTakeDamageInfo info( GetOwnerEntity(), pAttacker, GetOwnerEntity(), flDamage, iDamageType, TF_DMG_CUSTOM_BURNING );
		info.SetReportedPosition( pAttacker->GetAbsOrigin() );

		if ( info.GetDamageType() & DMG_CRITICAL )
		{
			info.SetCritType( CTakeDamageInfo::CRIT_FULL );
		}

		// terrible hack for flames hitting the Merasmus props to get the particle effect in the correct position
		if ( TFGameRules() && TFGameRules()->GetActiveBoss() && ( TFGameRules()->GetActiveBoss()->GetBossType() == HALLOWEEN_BOSS_MERASMUS ) )
		{
			info.SetDamagePosition( GetAbsOrigin() );
		}

		// Track hits for the Flamethrower, which is used to change the weapon sound based on hit ratio
		/*if ( m_hFlameThrower )
		{
			m_bBurnedEnemy = true;
			m_hFlameThrower->IncrementFlameDamageCount();
		}*/

		// We collided with pEnt, so try to find a place on their surface to show blood
		trace_t pTrace;
		UTIL_TraceLine( WorldSpaceCenter(), pEnt->WorldSpaceCenter(), MASK_SOLID|CONTENTS_HITBOX, this, COLLISION_GROUP_NONE, &pTrace );

		pEnt->DispatchTraceAttack( info, GetAbsVelocity(), &pTrace );
		ApplyMultiDamage();
	}

	// add ent to burn list
	if ( iEntIndex != m_mapEntitiesBurnt.InvalidIndex() )
	{
		m_mapEntitiesBurnt[ iEntIndex ].m_flLastBurnTime = gpGlobals->curtime;
	}
	else
	{
		m_mapEntitiesBurnt.Insert( pEnt, { gpGlobals->curtime, tf_flame_burn_index_per_collide } );
	}
}

#endif // GAME_DLL

float CTFFlameManager::GetFlameSizeMult( const tf_point_t *pPoint ) const
{
	float flLife = gpGlobals->curtime - pPoint->m_flSpawnTime;
	float flSizeMult = RemapValClamped( flLife, 0.f, pPoint->m_flLifeTime, GetStartSizeMult(), GetEndSizeMult() );
	if ( pPoint->m_nHitWall > 0 )
	{
		flSizeMult *= m_flRedirectedFlameSizeMult;
	}
	return flSizeMult;
}


float CTFFlameManager::GetStartSizeMult() const
{
	return m_flFlameStartSizeMult;
}


float CTFFlameManager::GetEndSizeMult() const
{
	return m_flFlameEndSizeMult;
}


bool CTFFlameManager::ShouldIgnorePlayerVelocity() const
{
	return m_flFlameIgnorePlayerVelocity != 0.f;
}


float CTFFlameManager::ReflectionAdditionalLifeTime() const
{
	return m_flFlameReflectionAdditionalLifeTime;
}


float CTFFlameManager::ReflectionDamageReduction() const
{
	return m_flFlameReflectionDamageReduction;
}

int CTFFlameManager::GetMaxFlameReflectionCount() const
{

	return m_iMaxFlameReflectionCount;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlameManager::OnClientPointAdded( const tf_point_t *pNewestPoint )
{
	//DevMsg( "OnClientPointAdded\n" );
	// ignore new points if we don't have particle effect
	if ( !m_hParticleEffect )
	{
		return;
	}

	const flame_point_t* pNewestFlame = static_cast< const flame_point_t * >( pNewestPoint );
	if ( pNewestFlame )
	{
		UpdateWeaponParticleControlPoint( pNewestFlame );
		UpdateFlameParticleControlPoint( pNewestFlame );
		
		// set active CP to the newest point
		int nTargetCP = pNewestFlame->m_nPointIndex + 1;
		Assert( nTargetCP != 0 );
		m_hParticleEffect->SetControlPointIndex( nTargetCP );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlameManager::UpdateWeaponParticleControlPoint( const flame_point_t *pNewestFlame )
{
	if ( !m_hParticleEffect )
		return;

	if ( !m_hWeapon )
		return;

	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s: Update particle", __FUNCTION__ );

	const float flStartRadius = 3.f;
	Vector vMuzzlePos;
	QAngle qMuzzleAng;
	if ( m_nMuzzleAttachment == INVALID_PARTICLE_ATTACHMENT )
	{
		m_nMuzzleAttachment = m_hWeapon->LookupAttachment( "muzzle" );
	}
	m_hWeapon->GetAttachment( m_nMuzzleAttachment, vMuzzlePos, qMuzzleAng );
	if ( m_hParticleEffect )
	{
		Vector vMuzzleForward, vMuzzleRight, vMuzzleUp;
		AngleVectors( qMuzzleAng, &vMuzzleForward, &vMuzzleRight, &vMuzzleUp );
		m_hParticleEffect->SetControlPoint( 0, vMuzzlePos );
		m_hParticleEffect->SetControlPointOrientation( 0, vMuzzleForward, vMuzzleRight, vMuzzleUp );
		m_hParticleEffect->SetControlPointDensity( 0, 1.f );
		m_hParticleEffect->SetControlPointRadius( 0, flStartRadius );

		Vector vVelocity = pNewestFlame ? pNewestFlame->m_vecAttackerVelocity + pNewestFlame->m_vecVelocity : vec3_origin;
		m_hParticleEffect->SetControlPointVelocity( 0, vVelocity );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlameManager::UpdateFlameParticleControlPoint( const flame_point_t *pFlame )
{
	Assert( pFlame );

	float flRadius = GetRadius( pFlame );
	float flAge = gpGlobals->curtime - pFlame->m_flSpawnTime;

	if ( m_hParticleEffect )
	{
		int iControlPoint = pFlame->m_nPointIndex + 1;
		Assert( iControlPoint != 0 );

		m_hParticleEffect->SetControlPoint( iControlPoint, pFlame->m_vecPosition );

		float flDensity = RemapValClamped( flAge, 0.f, pFlame->m_flLifeTime, 1.f, tf_flame_particle_min_density );
		m_hParticleEffect->SetControlPointDensity( iControlPoint, flDensity );

		m_hParticleEffect->SetControlPointRadius( iControlPoint, flRadius );

		m_hParticleEffect->SetControlPointDuration( iControlPoint, pFlame->m_flLifeTime );

		Vector vecVelocity = pFlame->m_vecAttackerVelocity + pFlame->m_vecVelocity;
		m_hParticleEffect->SetControlPointVelocity( iControlPoint, vecVelocity );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlameManager::OnFiringStateChange()
{
	// stop emission for old particle if needed particles
	if ( m_hParticleEffect && !m_hParticleEffect->m_bEmissionStopped )
	{
		//DevMsg( "stop old particle\n" );
		ParticleProp()->StopEmission( m_hParticleEffect );
		m_hOldParticleEffects.AddToTail( m_hParticleEffect );
	}

	if ( m_bIsFiring )
	{
		//DevMsg( "start new particle\n" );
		CTFFlameThrower *pFlameThrower = NULL;
		if ( m_hWeapon )
		{
			pFlameThrower = dynamic_cast< CTFFlameThrower* >( m_hWeapon.Get() );
		}

		const char *pszParticleName = pFlameThrower ? pFlameThrower->GetParticleEffectName() : "new_flame";
		m_hParticleEffect = ParticleProp()->Create( pszParticleName, PATTACH_CUSTOMORIGIN, 0 );
		if ( !m_hParticleEffect )
		{
			Warning( "Failed to create %s particles.", pszParticleName );
		}
		else
		{
			UpdateWeaponParticleControlPoint( NULL );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlameManager::RemoveAllParticles()
{
	if ( m_hParticleEffect )
	{
		ParticleProp()->StopEmissionAndDestroyImmediately( m_hParticleEffect );
		m_hParticleEffect = NULL;
	}
	ParticleProp()->StopEmission( NULL, false, true );
}

#endif // CLIENT_DLL

void CTFFlameManager::Update()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	BaseClass::Update();

	if ( tf_debug_flamethrower.GetBool() )
	{
#ifdef GAME_DLL
		Color color( 0, 0, 255, 100 );
#else
		Color color( 255, 0, 0, 100 );
#endif
		FOR_EACH_VEC( GetPointVec(), i )
		{
			const flame_point_t *pFlame = static_cast< const flame_point_t * >( GetPointVec()[i] );

			float flRadius = GetRadius( pFlame );
			Vector vMins = flRadius * Vector( -1, -1, -1 );
			Vector vMaxs = flRadius * Vector( 1, 1, 1 );

			DebugFlame( pFlame->m_vecPrevPosition, pFlame->m_vecPosition, vMins, vMaxs, color );
		}
	}

#ifdef GAME_DLL
#else // GAME_DLL

	if ( GetPointVec().IsEmpty() )
	{
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s: Empty stop particle immediate", __FUNCTION__ );
		if ( !m_bIsFiring )
		{
			RemoveAllParticles();
		}
		return;
	}

	// remove old particles if they're no longer active
	FOR_EACH_VEC_BACK( m_hOldParticleEffects, i )
	{
		if ( m_hOldParticleEffects[i]->m_nActiveParticles == 0 )
		{
			ParticleProp()->StopEmission( m_hOldParticleEffects[i] );
			m_hOldParticleEffects[i] = NULL;
			m_hOldParticleEffects.Remove( i );
		}
	}

	const flame_point_t* pNewestFlame = static_cast< const flame_point_t * >( GetPointVec()[ GetPointVec().Count() - 1 ] );
	UpdateWeaponParticleControlPoint( pNewestFlame );

	FOR_EACH_VEC( GetPointVec(), i )
	{
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s: FOR_EACH_VEC( GetPointVec() ) ", __FUNCTION__ );
		const flame_point_t *pFlame = static_cast< const flame_point_t * >( GetPointVec()[i] );
		UpdateFlameParticleControlPoint( pFlame );
	}

#endif // CLIENT_DLL
}

bool CTFFlameManager::OnPointHitWall( tf_point_t *pPoint, Vector &vecNewPos, Vector &vecNewVelocity, const trace_t& tr, float flDT )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	// bounce too many tiems
	if ( GetMaxFlameReflectionCount() > 0 && pPoint->m_nHitWall > GetMaxFlameReflectionCount() )
	{
		return true;
	}

	flame_point_t *pFlame = static_cast< flame_point_t* >( pPoint );

	pFlame->m_flLifeTime += ReflectionAdditionalLifeTime();

	Vector vecNewDirVelocity = vecNewVelocity - DotProduct( tr.plane.normal, vecNewVelocity ) * tr.plane.normal;
	if ( m_nShouldReflect > 0 )
	{
		// stomp with reflection
		vecNewDirVelocity = 2.f * vecNewDirVelocity - vecNewVelocity;
	}

	// once we change direction, just ignore the attacker vel
	pFlame->m_vecAttackerVelocity = vec3_origin;


	// update the new velocity and new pos
	vecNewVelocity = vecNewDirVelocity;

	// set pos to some offset from the surface
	vecNewPos = tr.endpos + GetRadius( pFlame ) * tr.plane.normal;

	// offset in a new direction
	vecNewPos += ( 1.f - tr.fraction ) * flDT * vecNewDirVelocity;

	return false;
}

void CTFFlameManager::ModifyAdditionalMovementInfo( tf_point_t *pPoint, float flDT )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	if ( ShouldIgnorePlayerVelocity() )
	{
		flame_point_t *pFlame = static_cast< flame_point_t * >( pPoint );

		float flAttackerSpeed = pFlame->m_vecAttackerVelocity.NormalizeInPlace();
		flAttackerSpeed = Clamp( flAttackerSpeed - flDT * GetDrag() * flAttackerSpeed, 0.f, flAttackerSpeed );
		pFlame->m_vecAttackerVelocity *= flAttackerSpeed;
	}
}

float CTFFlameManager::GetInitialSpeed() const
{

#ifdef WATERFALL_FLAMETHROWER_TEST
	if ( m_iWaterfallMode )
	{
		float flOverride = tf_flame_waterfall_speed_override.GetFloat();
		if ( flOverride != 0.f )
		{
			return flOverride;
		}
	}
#endif // WATERFALL_FLAMETHROWER_TEST

	return m_flFlameSpeed;
}

float CTFFlameManager::GetLifeTime() const
{

	float flFlameLifeTime = m_flFlameLifeTime;

#ifdef WATERFALL_FLAMETHROWER_TEST
	if ( m_iWaterfallMode )
	{
		float flOverride = tf_flame_waterfall_lifetime_override.GetFloat();
		if ( flOverride != 0.f )
		{
			flFlameLifeTime = flOverride;
		}
	}
#endif // WATERFALL_FLAMETHROWER_TEST

	return flFlameLifeTime + m_randomStream.RandomFloat( -m_flRandomLifeTimeOffset, m_flRandomLifeTimeOffset );
}

float CTFFlameManager::GetGravity() const
{
#ifdef WATERFALL_FLAMETHROWER_TEST
	if ( m_iWaterfallMode )
	{
		float flOverride = tf_flame_waterfall_gravity_override.GetFloat();
		if ( flOverride != 0.f )
		{
			return flOverride;
		}
	}
#endif // WATERFALL_FLAMETHROWER_TEST


	return m_flFlameGravity;
}

float CTFFlameManager::GetDrag() const
{
#ifdef WATERFALL_FLAMETHROWER_TEST
	if ( m_iWaterfallMode )
	{
		float flOverride = tf_flame_waterfall_drag_override.GetFloat();
		if ( flOverride != 0.f )
		{
			return flOverride;
		}
	}
#endif // WATERFALL_FLAMETHROWER_TEST


	return m_flFlameDrag;
}

Vector CTFFlameManager::GetAdditionalVelocity( const tf_point_t *pPoint ) const
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	float flFlameUp = m_flFlameUp;


	const flame_point_t *pFlame = static_cast< const flame_point_t * >( pPoint );

	// only add attacker vel in the direction of base vel
	Vector vecBaseDir = pFlame->m_vecVelocity.Normalized();
	Vector vecAttackerVelocity = DotProduct( pFlame->m_vecAttackerVelocity, vecBaseDir ) * vecBaseDir;

	return Vector( 0, 0, flFlameUp ) + vecAttackerVelocity;
}

float CTFFlameManager::GetRadius( const tf_point_t *pPoint ) const
{
	return tf_flamethrower_boxsize.GetFloat() * GetFlameSizeMult( pPoint );
}





