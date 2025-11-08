//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_jar_gas.h"
#include "decals.h"
#include "debugoverlay_shared.h"
#include "tf_weaponbase_gun.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "dt_utlvector_recv.h"
#else
#include "soundent.h"
#include "te_effect_dispatch.h"
#include "tf_player.h"
#include "func_break.h"
#include "func_nogrenades.h"
#include "Sprite.h"
#include "tf_fx.h"
#include "tf_team.h"
#include "tf_gamestats.h"
#include "tf_gamerules.h"
#include "particle_parse.h"
#include "bone_setup.h"
#include "tf_flame.h"
#include "dt_utlvector_send.h"
#include "collisionutils.h"
#endif


IMPLEMENT_NETWORKCLASS_ALIASED( TFJarGas, DT_TFWeaponJarGas )

BEGIN_NETWORK_TABLE( CTFJarGas, DT_TFWeaponJarGas )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_weapon_jar_gas, CTFJarGas );
PRECACHE_WEAPON_REGISTER( tf_weapon_jar_gas );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_JarGas, DT_TFProjectile_JarGas )
BEGIN_NETWORK_TABLE( CTFProjectile_JarGas, DT_TFProjectile_JarGas )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_jar_gas, CTFProjectile_JarGas );
PRECACHE_WEAPON_REGISTER( tf_projectile_jar_gas );


IMPLEMENT_NETWORKCLASS_ALIASED( TFGasManager, DT_TFGasManager );

BEGIN_NETWORK_TABLE( CTFGasManager, DT_TFGasManager )
END_NETWORK_TABLE()

BEGIN_DATADESC( CTFGasManager )
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_gas_manager, CTFGasManager );

#ifdef CLIENT_DLL
gas_particle_t::gas_particle_t()
{
	m_nUniqueID = 0;
	m_hParticleEffect = NULL;
	m_hParticleOwner = NULL;
}

gas_particle_t::~gas_particle_t()
{
	Assert( m_hParticleOwner );
	if ( m_hParticleEffect.GetObject() && m_hParticleOwner )
	{
		m_hParticleOwner->ParticleProp()->StopEmission( m_hParticleEffect.GetObject() );
	}
}
#endif // CLIENT_DLL


#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Jar *CTFJarGas::CreateJarProjectile( const Vector &position, const QAngle &angles, const Vector &velocity,
	const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo )
{
	RemoveJarGas( pOwner );
	return CTFProjectile_JarGas::Create( position, angles, velocity, angVelocity, pOwner, weaponInfo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFJarGas::GetAfterburnRateOnHit() const
{
	return TF_GAS_AFTERBURN_RATE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFJarGas::OnResourceMeterFilled()
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	pOwner->GiveAmmo( 1, m_iPrimaryAmmoType, false, kAmmoSource_ResourceMeter );
}
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFJarGas::GetProjectileSpeed( void )
{ 
	return TF_GAS_PROJ_SPEED;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_JarGas::Precache()
{
	PrecacheModel( TF_WEAPON_JAR_GAS_JAR_MODEL );
	PrecacheParticleSystem( "gas_can_blue" );
	PrecacheParticleSystem( "gas_can_red" );
	PrecacheScriptSound( TF_WEAPON_JARGAS_EXPLODE_SOUND );

	BaseClass::Precache();
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_JarGas* CTFProjectile_JarGas::Create( const Vector &position, const QAngle &angles,
	const Vector &velocity, const AngularImpulse &angVelocity,
	CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo )
{
	CTFProjectile_JarGas *pGrenade = static_cast< CTFProjectile_JarGas* >( CBaseEntity::CreateNoSpawn( "tf_projectile_jar_gas", position, angles, pOwner ) );
	if ( pGrenade )
	{
		// Set the pipebomb mode before calling spawn, so the model & associated vphysics get setup properly.
		pGrenade->SetPipebombMode();
		DispatchSpawn( pGrenade );

		pGrenade->InitGrenade( velocity, angVelocity, pOwner, weaponInfo );
		pGrenade->m_flFullDamage = 0;
		pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );
	}

	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_JarGas::SetCustomPipebombModel()
{
	SetModel( TF_WEAPON_JAR_GAS_JAR_MODEL );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_JarGas::Explode( trace_t *pTrace, int bitsDamageType )
{
	BaseClass::Explode( pTrace, bitsDamageType );

	SetModelName( NULL_STRING );//invisible
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_takedamage = DAMAGE_NO;

	// Pull out of the wall a bit.
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	CTFGasManager *pGasManager = NULL;
	CTFPlayer *pThrower = ToTFPlayer( GetThrower() );
	if ( pThrower )
	{
		Vector vecAbsOrigin = GetAbsOrigin();
		Vector vecManagerPos = vecAbsOrigin + Vector( 0, 0, TF_GAS_POINT_RADIUS * 1.2 );

		trace_t	tr;
		UTIL_TraceLine( vecAbsOrigin, vecManagerPos, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction != 1.0f )
		{
			vecManagerPos = vecAbsOrigin;
		}
		pGasManager = CTFGasManager::Create( pThrower, vecManagerPos );
		if ( pGasManager )
		{
			pGasManager->AddGas();
		}
	}

	SetContextThink( &CBaseGrenade::SUB_Remove, gpGlobals->curtime, "RemoveThink" );
	SetTouch( NULL );

	AddEffects( EF_NODRAW );
	SetAbsVelocity( vec3_origin );
}
#endif // GAME_DLL

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char* CTFJarGas::ModifyEventParticles( const char* token )
{
	if ( FStrEq( token, "energydrink_splash" ) )
	{
		CEconItemView *pItem = m_AttributeManager.GetItem();
		int iSystems = pItem->GetStaticData()->GetNumAttachedParticles( GetTeamNumber() );
		for ( int i = 0; i < iSystems; i++ )
		{
			attachedparticlesystem_t *pSystem = pItem->GetStaticData()->GetAttachedParticleData( GetTeamNumber(), i );
			if ( pSystem->iCustomType == 1 )
			{
				return pSystem->pszSystemName;
			}
		}
	}

	return BaseClass::ModifyEventParticles( token );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFJarGas::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner && pOwner->IsLocalPlayer() )
	{
		C_BaseEntity *pParticleEnt = pOwner->GetViewModel( 0 );
		if ( pParticleEnt )
		{
			pOwner->StopViewModelParticles( pParticleEnt );
		}
	}

	return BaseClass::Holster( pSwitchingTo );
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFJarGas::RemoveJarGas( CBaseCombatCharacter *pOwner )
{
	{
		CTFPlayer *pTFOwner = ToTFPlayer( pOwner );
		if ( pTFOwner )
		{
			pTFOwner->m_Shared.SetItemChargeMeter( LOADOUT_POSITION_SECONDARY, 0.f );
			pTFOwner->RemoveAmmo( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_iAmmoPerShot, m_iPrimaryAmmoType );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFJarGas::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFJarGas::CanAttack()
{
	{
		CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
		if ( pOwner )
		{
			if ( pOwner->m_Shared.GetItemChargeMeter( LOADOUT_POSITION_SECONDARY ) < 100.f )
				return false;
		}
	}

	return BaseClass::CanAttack();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFJarGas::ShouldUpdateMeter() const
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner )
		return pOwner->IsAlive();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGasManager::CTFGasManager()
{
	m_flLastMoveUpdate = -1.f;
	m_bKeepMovingPoints = true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGasManager::UpdateOnRemove( void )
{
#ifdef CLIENT_DLL
	m_hGasParticleEffects.RemoveAll();
#endif // CLIENT_DLL

	BaseClass::UpdateOnRemove();
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGasManager* CTFGasManager::Create( CBaseEntity *pOwner, const Vector& vPos )
{
	CTFGasManager *pGasManager = static_cast< CTFGasManager* >( CBaseEntity::Create( "tf_gas_manager", vPos, vec3_angle, pOwner ) );
	if ( pGasManager )
	{
		// Initialize the owner.
		pGasManager->SetOwnerEntity( pOwner );

		// Set team.
		pGasManager->ChangeTeam( pOwner->GetTeamNumber() );
	}

	return pGasManager;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGasManager::AddGas()
{
	int iCurrentTick = TIME_TO_TICKS( gpGlobals->curtime );
	while ( CanAddPoint() )
	{
		if ( !AddPoint( iCurrentTick ) )
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFGasManager::ShouldCollide( CBaseEntity *pEnt ) const
{
	if ( !pEnt->IsPlayer() )
		return false;

	if ( pEnt->GetTeamNumber() == GetTeamNumber() )
		return false;

	if ( TFGameRules() && TFGameRules()->IsTruceActive() )
		return false;

	if ( m_Touched.Find( pEnt ) != m_Touched.InvalidIndex() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGasManager::OnCollide( CBaseEntity *pEnt, int iPointIndex )
{

	CTFPlayer *pTFPlayer = assert_cast< CTFPlayer* >( pEnt );
	// add the condition if we can
	if ( !pTFPlayer->m_Shared.IsInvulnerable() && !pTFPlayer->m_Shared.InCond( TF_COND_PHASE ) && !pTFPlayer->m_Shared.InCond( TF_COND_PASSTIME_INTERCEPTION ) && pTFPlayer->CanGetWet() )
	{
		pTFPlayer->m_Shared.AddCond( TF_COND_GAS, 10.f, GetOwnerEntity() );
	}

	m_Touched.AddToTail( pTFPlayer );
}
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGasManager::Update()
{

	TM_ZONE_DEFAULT( TELEMETRY_LEVEL0 )

	bool bUpdatePoints = ( GetPointVec().Count() > 0 );

	// remove any gas points that shouldn't be included anymore
	FOR_EACH_VEC_BACK( GetPointVec(), i )
	{
		bool bShouldRemove = false;
		// expired
		if ( gpGlobals->curtime > GetPointVec()[i]->m_flSpawnTime + GetPointVec()[i]->m_flLifeTime )
		{
			bShouldRemove = true;
		}

		// in water?
		int nContents = UTIL_PointContents( GetPointVec()[i]->m_vecPosition );
		if ( ( nContents & MASK_WATER ) )
		{
			bShouldRemove = true;
		}

		if ( bShouldRemove )
		{
			RemovePoint( i );
		}
	}

	if ( GetPointVec().Count() <= 0 )
	{
		if ( bUpdatePoints )
		{
			SetContextThink( &CTFGasManager::SUB_Remove, gpGlobals->curtime, "RemoveThink" );
		}

		return;
	}

	// store the previous position
	FOR_EACH_VEC( GetPointVec(), i )
	{
		GetPointVec()[i]->m_vecPrevPosition = GetPointVec()[i]->m_vecPosition;
	}

	float flMinDistanceApart = ( TF_GAS_POINT_RADIUS * 1.9f );
	if ( m_flLastMoveUpdate <= 1.f )
	{
		m_flLastMoveUpdate = gpGlobals->curtime;
	}
	float flDelta = gpGlobals->curtime - m_flLastMoveUpdate;
	m_flLastMoveUpdate = gpGlobals->curtime;

	bool bAnyoneMoved = false;

	if ( m_bKeepMovingPoints )
	{
		// move them down
		FOR_EACH_VEC( GetPointVec(), i )
		{
			Vector vecDownDir = flDelta * Vector( 0.f, 0.f, -TF_GAS_POINT_RADIUS );

			trace_t	tr;
			Vector vecPos = GetPointVec()[i]->m_vecPosition + Vector( 0.f, 0.f, -TF_GAS_POINT_RADIUS ); // bottom of the sphere
			UTIL_TraceLine( vecPos, vecPos + vecDownDir, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
			if ( tr.fraction != 1.f )
				continue;

			GetPointVec()[i]->m_vecPosition += ( vecDownDir * TF_GAS_FALLRATE );
			bAnyoneMoved = true;
		}

		// now have them separate
		FOR_EACH_VEC( GetPointVec(), point1 )
		{
			int nNeighborCount = 0;
			Vector vecResult( 0, 0, 0 );

			FOR_EACH_VEC( GetPointVec(), point2 )
			{
				if ( point1 == point2 )
					continue;

				Vector vecDist = GetPointVec()[point2]->m_vecPosition - GetPointVec()[point1]->m_vecPosition;
				if ( vecDist.Length() < flMinDistanceApart )
				{
					vecResult.x += ( GetPointVec()[point2]->m_vecPosition.x - GetPointVec()[point1]->m_vecPosition.x );
					vecResult.y += ( GetPointVec()[point2]->m_vecPosition.y - GetPointVec()[point1]->m_vecPosition.y );
					vecResult.z += ( GetPointVec()[point2]->m_vecPosition.z - GetPointVec()[point1]->m_vecPosition.z );

					nNeighborCount++;
				}
			}

			if ( nNeighborCount > 0 )
			{
				vecResult.x /= nNeighborCount;
				vecResult.y /= nNeighborCount;
				vecResult.z /= nNeighborCount;

				Vector vecNewDir = vecResult.Normalized();
				vecNewDir *= ( TF_GAS_MOVE_DISTANCE * -1.f );

				// can we move there?
				trace_t	tr;
				UTIL_TraceLine( GetPointVec()[point1]->m_vecPosition, GetPointVec()[point1]->m_vecPosition + vecNewDir, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
				if ( tr.fraction != 1.f )
					continue;

				GetPointVec()[point1]->m_vecPosition += vecNewDir;
				bAnyoneMoved = true;
			}
		}
	}

// 	if ( bAnyoneMoved )
// 	{
// 		FOR_EACH_VEC( GetPointVec(), i )
// 		{
// #ifdef GAME_DLL
// 			DevMsg( "SERVER MOVE: index [%d], pos[ %f %f %f ]\n", i, XYZ( GetPointVec()[i]->m_vecPosition ) );
// #else
// 			DevMsg( "CLIENT MOVE: index [%d], pos[ %f %f %f ]\n", i, XYZ( GetPointVec()[i]->m_vecPosition ) );
// #endif
// 		}
// 	}

	m_bKeepMovingPoints = bAnyoneMoved;
#ifdef GAME_DLL
	// update bounds if necessary
	if ( bAnyoneMoved )
	{
		Vector vHullMin( MAX_COORD_FLOAT, MAX_COORD_FLOAT, MAX_COORD_FLOAT );
		Vector vHullMax( MIN_COORD_FLOAT, MIN_COORD_FLOAT, MIN_COORD_FLOAT );
		FOR_EACH_VEC( GetPointVec(), i )
		{
			tf_point_t *pPoint = GetPointVec()[i];
			float flRadius = GetRadius( pPoint );
			Vector vExtent( flRadius, flRadius, flRadius );
			VectorMin( vHullMin, pPoint->m_vecPosition - vExtent, vHullMin );
			VectorMax( vHullMax, pPoint->m_vecPosition + vExtent, vHullMax );
		}

		Vector vExtent = 0.5f * ( vHullMax - vHullMin );
		Vector vOrigin = vHullMin + vExtent;
		SetAbsOrigin( vOrigin );
		UTIL_SetSize( this, -vExtent, vExtent );
	}
#endif // GAME_DLL

#ifdef CLIENT_DLL
	// clean up gas that has gone away
	FOR_EACH_VEC_BACK( m_hGasParticleEffects, nIndex )
	{
		int nUniqueID = m_hGasParticleEffects[nIndex].m_nUniqueID;
		bool bFound = false;
		FOR_EACH_VEC( GetPointVec(), i )
		{
			if ( GetPointVec()[i]->m_nPointIndex == nUniqueID )
			{
				bFound = true;
				break;
			}
		}

		if ( !bFound )
		{
			m_hGasParticleEffects.Remove( nIndex );
		}
	}

	// add/update gas that's still around
	FOR_EACH_VEC( GetPointVec(), i )
	{
		int iFoundIndex = -1;
		FOR_EACH_VEC_BACK( m_hGasParticleEffects, iIndex )
		{
			if ( m_hGasParticleEffects[iIndex].m_nUniqueID == GetPointVec()[i]->m_nPointIndex )
			{
				iFoundIndex = iIndex;
				break;
			}
		}

		if ( iFoundIndex == -1 )
		{
			// create a new effect
			CSmartPtr<CNewParticleEffect> pEffect = ParticleProp()->Create( ( GetTeamNumber() == TF_TEAM_BLUE ) ? "gas_can_blue" : "gas_can_red", PATTACH_CUSTOMORIGIN, 0 );
			if ( pEffect.IsValid() && pEffect->IsValid() )
			{
				pEffect->SetControlPoint( 0, GetPointVec()[i]->m_vecPosition );

				int nNewIndex = m_hGasParticleEffects.AddToTail();
				m_hGasParticleEffects[nNewIndex].m_hParticleOwner = this;
				m_hGasParticleEffects[nNewIndex].m_hParticleEffect = pEffect;
				m_hGasParticleEffects[nNewIndex].m_nUniqueID = GetPointVec()[i]->m_nPointIndex;
			}
		}
		else
		{
			m_hGasParticleEffects[iFoundIndex].m_hParticleEffect->SetControlPoint( 0, GetPointVec()[i]->m_vecPosition );
		}
	}
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Vector CTFGasManager::GetInitialPosition() const
{
	float flDistance = TF_GAS_POINT_RADIUS;

#ifdef GAME_DLL
	Vector vecOrigin = GetAbsOrigin();
#else
	Vector vecOrigin = GetNetworkOrigin();
#endif

	return ( vecOrigin + Vector( m_randomStream.RandomFloat( flDistance * -1.f, flDistance ), m_randomStream.RandomFloat( flDistance * -1.f, flDistance ), 0 ) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFGasManager::OnPointHitWall( tf_point_t *pPoint, Vector &vecNewPos, Vector &vecNewVelocity, const trace_t& tr, float flDT )
{
	// set pos to some offset from the surface
	vecNewPos = tr.endpos + ( ( GetRadius( pPoint ) + 5 ) * tr.plane.normal );
	vecNewVelocity = pPoint->m_vecVelocity;

	return false;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGasManager::PostDataUpdate( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		BaseClass::PostDataUpdate( updateType );
		return;
	}

	// intentionally skip the BaseClass version after we're created
	CBaseEntity::PostDataUpdate( updateType );
}
#endif // CLIENT_DLL
