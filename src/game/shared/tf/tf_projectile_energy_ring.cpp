//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Energy Ring
//
//=============================================================================
#include "cbase.h"
#include "tf_projectile_energy_ring.h"
#include "tf_weapon_raygun.h"

#ifdef CLIENT_DLL
#include "c_basetempentity.h"
#include "c_te_legacytempents.h"
#include "c_te_effect_dispatch.h"
#include "input.h"
#include "c_tf_player.h"
#include "cliententitylist.h"
#endif

#ifdef GAME_DLL
#include "tf_player.h"
#include "tf_player_shared.h"
#include "particle_parse.h"
#include "tf_pumpkin_bomb.h"
#include "halloween/merasmus/merasmus_trick_or_treat_prop.h"
#include "tf_robot_destruction_robot.h"
#include "tf_generic_bomb.h"
#endif

#define ENERGY_RING_DISPATCH_EFFECT			"ClientProjectile_EnergyRing"
#define ENERGY_RING_DISPATCH_EFFECT_POMSON	"ClientProjectile_EnergyRingPomson"

const char* g_pszEnergyRingModel				( "models/weapons/w_models/w_drg_ball.mdl" );

const char* g_pszPomsonImpactFleshSound			( "Weapon_Pomson.ProjectileImpactWorld" );
const char* g_pszPomsonImpactWorldSound			( "Weapon_Pomson.ProjectileImpactFlesh" );
const char* g_pszPomsonTrailParticle			( "drg_pomson_projectile" );
const char* g_pszPomsonTrailParticleCrit		( "drg_pomson_projectile_crit" );

const char* g_pszBisonImpactFleshSound			( "Weapon_Bison.ProjectileImpactWorld" );
const char* g_pszBisonImpactWorldSound			( "Weapon_Bison.ProjectileImpactFlesh" );
const char* g_pszBisonTrailParticle				( "drg_bison_projectile" );
const char* g_pszBisonTrailParticleCrit			( "drg_bison_projectile_crit" );
												  
const char* g_pszEnergyProjectileImpactParticle	( "drg_pomson_impact" );
//=============================================================================
//
// TF Energy Ring Projectile functions
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_EnergyRing, DT_TFProjectile_EnergyRing )

BEGIN_NETWORK_TABLE( CTFProjectile_EnergyRing, DT_TFProjectile_EnergyRing )
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( tf_projectile_energy_ring, CTFProjectile_EnergyRing );
PRECACHE_WEAPON_REGISTER( tf_projectile_energy_ring );

short g_sModelIndexRing;
void PrecacheRing(void *pUser)
{
	g_sModelIndexRing = modelinfo->GetModelIndex( g_pszEnergyRingModel );
}
PRECACHE_REGISTER_FN(PrecacheRing);

#ifdef GAME_DLL
ConVar tf_bison_tick_time( "tf_bison_tick_time", "0.025", FCVAR_CHEAT );
#endif


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_EnergyRing::CTFProjectile_EnergyRing()
{
	m_vecPrevPos = vec3_origin;

#ifdef GAME_DLL
	m_flLastHitTime = 0.f;
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CTFProjectile_EnergyRing::GetProjectileModelName( void )
{
	return g_pszEnergyRingModel;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFProjectile_EnergyRing::GetGravity( void )
{
	return 0.f;
}

float CTFProjectile_EnergyRing::GetInitialVelocity( void )
{
	return 1200.f; 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_EnergyRing *CTFProjectile_EnergyRing::Create( CTFWeaponBaseGun *pLauncher, const Vector &vecOrigin, const QAngle& vecAngles, float fSpeed, float fGravity, 
														    CBaseEntity *pOwner, CBaseEntity *pScorer, Vector vColor1, Vector vColor2, bool bCritical )
{
	CTFProjectile_EnergyRing *pRing = NULL;
	
#ifdef GAME_DLL
	Vector vecForward, vecRight, vecUp;
	AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

	pRing = static_cast<CTFProjectile_EnergyRing*>( CBaseEntity::Create( "tf_projectile_energy_ring", vecOrigin, vecAngles, pOwner ) );
	if ( !pRing )
		return NULL;

	// Initialize the owner.
	pRing->SetOwnerEntity( pOwner );
	pRing->SetLauncher( pLauncher );

	pRing->SetScorer( pScorer );

	// Spawn.
	pRing->Spawn();

	Vector vecVelocity = vecForward * pRing->GetInitialVelocity();
	pRing->SetAbsVelocity( vecVelocity );	

	// Setup the initial angles.
	QAngle angles;
	VectorAngles( vecVelocity, angles );
	pRing->SetAbsAngles( angles );

	// Set team.
	pRing->ChangeTeam( pOwner->GetTeamNumber() );

	if ( pScorer )
	{
		pRing->SetTruceValidForEnt( pScorer->IsTruceValidForEnt() );
	}
#endif

#ifdef CLIENT_DLL
	// This is silly code to support demos when the client created its own effects
	// for the Pomson and Righteous Bison
	CTFRaygun* pRaygun = assert_cast< CTFRaygun* >( pLauncher );

	if ( pRaygun && !pRaygun->UseNewProjectileCode() )
	{
		if ( pRaygun->GetWeaponID() == TF_WEAPON_DRG_POMSON )
		{
			pRing = static_cast<CTFProjectile_EnergyRing*>( CTFBaseProjectile::Create( "tf_projectile_energy_ring", vecOrigin, vecAngles, pOwner, 
																					   1200.f, g_sModelIndexRing, 
																					   ENERGY_RING_DISPATCH_EFFECT_POMSON, pScorer, bCritical, vColor1, vColor2 ) );
		}
		else
		{
			pRing = static_cast<CTFProjectile_EnergyRing*>( CTFBaseProjectile::Create( "tf_projectile_energy_ring", vecOrigin, vecAngles, pOwner, 
																					   1200.f, g_sModelIndexRing, 
																					   ENERGY_RING_DISPATCH_EFFECT, pScorer, bCritical, vColor1, vColor2 ) );
		}

		if ( pRing )
		{
			pRing->SetRenderMode( kRenderNone );
			pRing->SetSolidFlags( FSOLID_TRIGGER | FSOLID_NOT_SOLID );
			pRing->SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS );
		}
	}
#endif

	return pRing;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyRing::Spawn()
{	
	BaseClass::Spawn();

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLY, MOVECOLLIDE_FLY_CUSTOM );
	SetRenderMode( kRenderNone	);
	SetSolidFlags( FSOLID_TRIGGER | FSOLID_NOT_SOLID );
	SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyRing::Precache()
{
	PrecacheParticleSystem( g_pszEnergyProjectileImpactParticle );

	PrecacheParticleSystem( g_pszBisonTrailParticle );
	PrecacheParticleSystem( g_pszBisonTrailParticleCrit );
	PrecacheScriptSound( g_pszBisonImpactWorldSound );
	PrecacheScriptSound( g_pszBisonImpactFleshSound );

	PrecacheParticleSystem( g_pszPomsonTrailParticle );
	PrecacheParticleSystem( g_pszPomsonTrailParticleCrit );
	PrecacheScriptSound( g_pszPomsonImpactWorldSound );
	PrecacheScriptSound( g_pszPomsonImpactFleshSound );

	BaseClass::Precache();
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyRing::ProjectileTouch( CBaseEntity *pOther )
{
	// Verify a correct "other."
	Assert( pOther );
	if ( !pOther || 
		 !pOther->IsSolid() ||
		 pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) ||
		 ( pOther->GetCollisionGroup() == TFCOLLISION_GROUP_RESPAWNROOMS ) ||
		 pOther->IsFuncLOD() )
	{
		return;
	}

	CBaseEntity* pOwner = GetOwnerEntity();
	// Don't shoot ourselves
	if ( pOwner == pOther )
		return;

	// Handle hitting skybox (disappear).
	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
	if ( pTrace->surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	// pass through ladders
	if ( pTrace->surface.flags & CONTENTS_LADDER )
		return;

	if ( !ShouldTouchNonWorldSolid( pOther, pTrace ) )
		return;

	// The stuff we collide with
	bool bCombatEntity = pOther->IsPlayer() || 
						 pOther->IsBaseObject() || 
						 pOther->IsCombatCharacter() || 
						 pOther->IsCombatItem() ||
						 pOther->IsProjectileCollisionTarget();

	if ( bCombatEntity )
	{
		// Bison projectiles shouldn't collide with friendly things
		if ( ShouldPenetrate() && ( pOther->InSameTeam( this ) || ( gpGlobals->curtime - m_flLastHitTime ) < tf_bison_tick_time.GetFloat() ) )
			return;

		m_flLastHitTime = gpGlobals->curtime;

		const int nDamage = GetDamage();

		CTakeDamageInfo info( this, pOwner, GetLauncher(), nDamage, GetDamageType(), TF_DMG_CUSTOM_PLASMA );
		info.SetReportedPosition( pOwner->GetAbsOrigin() );
		info.SetDamagePosition( pTrace->endpos );

		if ( info.GetDamageType() & DMG_CRITICAL )
		{
			info.SetCritType( CTakeDamageInfo::CRIT_FULL );
		}

		trace_t traceAttack;
		UTIL_TraceLine( WorldSpaceCenter(), pOther->WorldSpaceCenter(), MASK_SOLID|CONTENTS_HITBOX, this, COLLISION_GROUP_NONE, &traceAttack );

		pOther->DispatchTraceAttack( info, GetAbsVelocity(), &traceAttack );

		ApplyMultiDamage();

		// Get a position on whatever we hit
		Vector vecDelta = pOther->GetAbsOrigin() - GetAbsOrigin();
		Vector vecNormalVel = GetAbsVelocity().Normalized();
		Vector vecNewPos = ( DotProduct( vecDelta, vecNormalVel ) * vecNormalVel ) + GetAbsOrigin();

		PlayImpactEffects( vecNewPos, pOther->IsPlayer() );

		if ( ShouldPenetrate() )
			return;
		
		UTIL_Remove( this );
		return;
	}

	if ( pOther->IsWorld() )
	{
		SetAbsVelocity( vec3_origin	);
		AddSolidFlags( FSOLID_NOT_SOLID );
	}

	PlayImpactEffects( pTrace->endpos, false );
	
	// Remove by default.  Fixes this entity living forever on things like doors.
	UTIL_Remove( this );
}

void CTFProjectile_EnergyRing::ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity )
{
	PlayImpactEffects( trace.endpos, false );
	
	// Remove by default.  Fixes this entity living forever on things like doors.
	UTIL_Remove( this );
}

void CTFProjectile_EnergyRing::PlayImpactEffects( const Vector& vecPos, bool bHitFlesh )
{
	CTFWeaponBaseGun* pTFGun = dynamic_cast< CTFWeaponBaseGun* >( GetLauncher() );
	if ( pTFGun )
	{
		DispatchParticleEffect( g_pszEnergyProjectileImpactParticle, vecPos, GetAbsAngles(), pTFGun->GetParticleColor( 1 ), pTFGun->GetParticleColor( 2 ), true, NULL, 0 );
		const char* pszSoundString = NULL;
		if ( ShouldPenetrate() )
		{
			pszSoundString = bHitFlesh ? g_pszBisonImpactFleshSound : g_pszBisonImpactWorldSound;
		}
		else
		{
			pszSoundString = bHitFlesh ? g_pszPomsonImpactFleshSound : g_pszPomsonImpactWorldSound;
		}
		EmitSound( pszSoundString );
	}
}

#else

void CTFProjectile_EnergyRing::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CNewParticleEffect* pEffect = ParticleProp()->Create( GetTrailParticleName(), PATTACH_ABSORIGIN_FOLLOW );
		CTFWeaponBaseGun* pTFGun = dynamic_cast< CTFWeaponBaseGun* >( GetLauncher() );
		if ( pEffect && pTFGun )
		{
			pEffect->SetControlPoint( CUSTOM_COLOR_CP1, pTFGun->GetParticleColor( 0 ) );
			pEffect->SetControlPoint( CUSTOM_COLOR_CP2, pTFGun->GetParticleColor( 1 ) );
		}
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFProjectile_EnergyRing::GetDamage()
{
	return ShouldPenetrate() ? 20.f : 60.f;
}

bool CTFProjectile_EnergyRing::ShouldPenetrate() const
{
	int iPenetrate = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( GetOwnerEntity(), iPenetrate, energy_weapon_penetration );

	return iPenetrate != 0;
}

const char*	CTFProjectile_EnergyRing::GetTrailParticleName() const
{
	if ( ShouldPenetrate() )	// Righteous Bison
	{
		return IsCritical() ? g_pszBisonTrailParticleCrit : g_pszBisonTrailParticle;
	}
	else // Pomson
	{
		return IsCritical() ? g_pszPomsonTrailParticleCrit : g_pszPomsonTrailParticle;
	}
}



//-----------------------------------------------------------------------------
// The following is legacy code to support old demos
//-----------------------------------------------------------------------------

#ifdef CLIENT_DLL
void CreateClientSideEnergyRing( const char* pszStandardParticle, const char* pszCritParticle, const CEffectData &data, int nFlags )
{
	C_BaseEntity *entity = ClientEntityList().GetBaseEntityFromHandle( data.m_hEntity );
	if ( !entity )
	{
		return;
	}

	C_TFPlayer *pPlayer = dynamic_cast< C_TFPlayer * >( entity->GetOwnerEntity() );
	if ( pPlayer )
	{
		C_LocalTempEntity *pRing = ClientsideProjectileCallback( data, 0.f );
		if ( pRing )
		{
			bool bCritical = ( ( data.m_nDamageType & DMG_CRITICAL ) != 0 );
			CNewParticleEffect* pEffect = pRing->AddParticleEffect( bCritical ? pszCritParticle : pszStandardParticle );
			if ( pEffect )
			{
				pEffect->SetControlPoint( CUSTOM_COLOR_CP1, data.m_CustomColors.m_vecColor1 );
				pEffect->SetControlPoint( CUSTOM_COLOR_CP2, data.m_CustomColors.m_vecColor2 );
			}

			pRing->AddEffects( EF_NOSHADOW );
			pRing->flags = nFlags;
			pRing->SetRenderMode( kRenderNone );
			pRing->SetSolidFlags( FSOLID_TRIGGER | FSOLID_NOT_SOLID );
			pRing->SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Bison effect callback
//-----------------------------------------------------------------------------
void ClientsideProjectileRingCallback( const CEffectData &data )
{
	CreateClientSideEnergyRing( g_pszBisonTrailParticle, g_pszBisonTrailParticleCrit, data, FTENT_COLLIDEKILL | FTENT_COLLIDEPROPS | FTENT_ATTACHTOTARGET | FTENT_ALIGNTOMOTION | FTENT_CLIENTSIDEPARTICLES );
}

DECLARE_CLIENT_EFFECT( ENERGY_RING_DISPATCH_EFFECT, ClientsideProjectileRingCallback );


//-----------------------------------------------------------------------------
// Purpose: Pomson effect callback
//-----------------------------------------------------------------------------
void ClientsideProjectileRingPomsonCallback( const CEffectData &data )
{
	CreateClientSideEnergyRing( g_pszPomsonTrailParticle, g_pszPomsonTrailParticleCrit, data, FTENT_COLLIDEALL | FTENT_USEFASTCOLLISIONS | FTENT_ATTACHTOTARGET | FTENT_ALIGNTOMOTION | FTENT_CLIENTSIDEPARTICLES );
}

DECLARE_CLIENT_EFFECT( ENERGY_RING_DISPATCH_EFFECT_POMSON, ClientsideProjectileRingPomsonCallback );

#endif
