//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Nail
//
//=============================================================================
#include "cbase.h"
#include "tf_projectile_flare.h"
#include "soundent.h"
#include "tf_fx.h"
#include "tf_player.h"
#include "tf_weapon_flaregun.h"
#include "tf_gamerules.h"

//=============================================================================
//
// TF Flare Projectile functions (Server specific).
//
#define FLARE_MODEL					"models/weapons/w_models/w_flaregun_shell.mdl"
#define FLARE_GRAVITY				0.3f
#define FLARE_SPEED					2000.0f

#define FLARE_THINK_CONTEXT			"CTFProjectile_FlareThink"

LINK_ENTITY_TO_CLASS( tf_projectile_flare, CTFProjectile_Flare );
PRECACHE_WEAPON_REGISTER( tf_projectile_flare );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Flare, DT_TFProjectile_Flare )

BEGIN_NETWORK_TABLE( CTFProjectile_Flare, DT_TFProjectile_Flare )
	SendPropBool( SENDINFO( m_bCritical ) ),
END_NETWORK_TABLE()

BEGIN_DATADESC( CTFProjectile_Flare )
DEFINE_THINKFUNC( ImpactThink ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Flare::CTFProjectile_Flare()
{
	m_bIsFromTaunt = false;
	m_bCritical = false;
	m_bImpact = false;
	m_flImpactTime = 0.0f;
	m_flNextSeekUpdate = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Flare::~CTFProjectile_Flare()
{

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Flare *CTFProjectile_Flare::Create( CBaseEntity *pLauncher, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer )
{
	CTFProjectile_Flare *pFlare = static_cast<CTFProjectile_Flare*>( CBaseEntity::CreateNoSpawn( "tf_projectile_flare", vecOrigin, vecAngles, pOwner ) );
	if ( !pFlare )
		return NULL;

	pFlare->SetLauncher( pLauncher );

	// Initialize the owner.
	pFlare->SetOwnerEntity( pOwner );

	// Set team.
	pFlare->ChangeTeam( pOwner->GetTeamNumber() );

	// Save the scoring player.
	pFlare->SetScorer( pScorer );

	// Spawn.
	DispatchSpawn( pFlare );

	// Setup the initial velocity.
	Vector vecForward, vecRight, vecUp;
	AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

	float flLaunchSpeed = pFlare->GetProjectileSpeed();

	Vector vecVelocity = vecForward * flLaunchSpeed;
	pFlare->SetAbsVelocity( vecVelocity );	
	pFlare->SetupInitialTransmittedGrenadeVelocity( vecVelocity );

	float flGravity = FLARE_GRAVITY;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pLauncher, flGravity, mult_projectile_speed );
	pFlare->SetGravity( flGravity );

	// Setup the initial angles.
	QAngle angles;
	VectorAngles( vecVelocity, angles );
	pFlare->SetAbsAngles( angles );

	return pFlare;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::Spawn()
{
	SetModel( FLARE_MODEL );
	BaseClass::Spawn();

	float flHeatSeekPower = GetHeatSeekPower();

	if ( flHeatSeekPower > 0.0f )
	{
		SetMoveType( MOVETYPE_CUSTOM, MOVECOLLIDE_DEFAULT );
		SetGravity( FLARE_GRAVITY );
	}
	else
	{
		SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
		SetGravity( FLARE_GRAVITY );
	}

	// Set team.
	m_nSkin = ( GetTeamNumber() == TF_TEAM_BLUE ) ? 1 : 0;

	m_flCreationTime = gpGlobals->curtime;

	CTFPlayer *pScorer = ToTFPlayer( GetScorer() );
	if ( pScorer && pScorer->IsTaunting() && m_flCreationTime >= pScorer->GetTauntAttackTime() )
	{
		m_bIsFromTaunt = true;
	}

	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		// If there's anything solid between the flare and the attacker, just fizzle it.
		// We could change how we spawn flares in CTFWeaponBaseGun::FireFlare(), but it
		// would change how flares fire.  Maybe that's OK?
		trace_t trace;
		CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
		UTIL_TraceLine( pOwner->EyePosition(), GetAbsOrigin(), MASK_SOLID_BRUSHONLY, &traceFilter, &trace );
		if ( trace.fraction < 1.f && ( !trace.m_pEnt || trace.m_pEnt->m_takedamage == DAMAGE_NO ) )
		{
			Detonate( true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::Precache()
{
	PrecacheModel( FLARE_MODEL );
	PrecacheParticleSystem( "flaregun_trail_red" );
	PrecacheParticleSystem( "flaregun_trail_crit_red" );
	PrecacheParticleSystem( "flaregun_trail_blue" );
	PrecacheParticleSystem( "flaregun_trail_crit_blue" );
	PrecacheParticleSystem( "drg_manmelter_projectile" );
	PrecacheParticleSystem( "scorchshot_trail_red" );
	PrecacheParticleSystem( "scorchshot_trail_crit_red" );
	PrecacheParticleSystem( "scorchshot_trail_blue" );
	PrecacheParticleSystem( "scorchshot_trail_crit_blue" );
	PrecacheParticleSystem( "Explosions_MA_FlyingEmbers" );

	PrecacheParticleSystem( "pyrovision_flaregun_trail_blue" );
	PrecacheParticleSystem( "pyrovision_flaregun_trail_red" );
	PrecacheParticleSystem( "pyrovision_flaregun_trail_crit_blue" );
	PrecacheParticleSystem( "pyrovision_flaregun_trail_crit_red" );
	PrecacheParticleSystem( "pyrovision_scorchshot_trail_blue" );
	PrecacheParticleSystem( "pyrovision_scorchshot_trail_red" );
	PrecacheParticleSystem( "pyrovision_scorchshot_trail_crit_blue" );
	PrecacheParticleSystem( "pyrovision_scorchshot_trail_crit_red" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::SetScorer( CBaseEntity *pScorer )
{
	m_Scorer = pScorer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBasePlayer *CTFProjectile_Flare::GetScorer( void )
{
	return dynamic_cast<CBasePlayer *>( m_Scorer.Get() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFProjectile_Flare::GetDamageType() 
{ 
	int iDmgType = BaseClass::GetDamageType();
	if ( m_bCritical )
	{
		iDmgType |= DMG_CRITICAL;
	}

	return iDmgType;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::Explode( trace_t *pTrace, CBaseEntity *pOther )
{
	Vector vecOrigin = GetAbsOrigin();

	CBaseEntity *pAttacker = GetOwnerEntity();
	IScorer *pScorerInterface = dynamic_cast<IScorer*>( pAttacker );
	if ( pScorerInterface )
	{
		pAttacker = pScorerInterface->GetScorer();
	}

	CTFPlayer *pTFVictim = ToTFPlayer( pOther );

	CTFFlareGun *pFlareGun = dynamic_cast< CTFFlareGun* >( GetLauncher() );
	if ( pFlareGun )
	{
		if ( pFlareGun->GetFlareGunType() == FLAREGUN_SCORCHSHOT )
		{
			// When the scorch shot hits a player...
			if ( pTFVictim )
			{
				// Now it only collides with the world
				SetCollisionGroup( COLLISION_GROUP_DEBRIS );

				Vector vVelocity = GetAbsVelocity();

				// Check if burning before damage
				bool bIsBurningVictim = pTFVictim->m_Shared.InCond( TF_COND_BURNING );
				int iDamageType = GetDamageType();
				
				// Prevent the normal push force cause we are going to add it
				iDamageType |= DMG_PREVENT_PHYSICS_FORCE;

				// Damage the player to push them back
				CTakeDamageInfo info( this, pAttacker, m_hLauncher, vec3_origin, vecOrigin, GetDamage(), iDamageType, m_bIsFromTaunt ? TF_DMG_CUSTOM_FLARE_PELLET : 0 );
				pTFVictim->TakeDamage( info );

				bool bIsEnemy = pAttacker && pTFVictim->GetTeamNumber() != pAttacker->GetTeamNumber();
				
				if ( !pTFVictim->m_Shared.IsImmuneToPushback() && bIsEnemy )
				{
					Vector vecToTarget;
					vecToTarget = vVelocity;
					VectorNormalize( vecToTarget );
					vecToTarget.z = 1.0;

					// apply airblast - Apply stun if they are effectively grounded so we can knock them up
					if ( !pTFVictim->m_Shared.InCond( TF_COND_KNOCKED_INTO_AIR ) )
					{
						pTFVictim->m_Shared.StunPlayer( 0.5, 1.f, TF_STUN_MOVEMENT, ToTFPlayer( pAttacker ) );
					}
					
					float flForce = bIsBurningVictim ? 400.0f : 100.0f;
					pTFVictim->ApplyGenericPushbackImpulse( vecToTarget * flForce, ToTFPlayer( pAttacker ) );
				}

				// It loses almost all of its speed and pops into the air
				vVelocity.x *= 0.07f;
				vVelocity.y *= 0.07f;
				vVelocity.z = 100.0f;
				SetAbsVelocity( vVelocity + RandomVector( -2.0f, 2.0f ) );

				// Point the new direction and randomly flip
				QAngle angForward;
				VectorAngles( vVelocity, angForward );
				SetAbsAngles( angForward );

				QAngle angRotation = RandomAngle( 180.0f, 720.0f );
				angRotation.x *= ( RandomInt( 0, 1 ) == 0 ? 1 : -1 );
				angRotation.y *= ( RandomInt( 0, 1 ) == 0 ? 1 : -1 );
				angRotation.z *= ( RandomInt( 0, 1 ) == 0 ? 1 : -1 );

				SetLocalAngularVelocity( angRotation );

				CPVSFilter filter( vecOrigin );
				EmitSound( filter, entindex(), "Rubber.BulletImpact" );

				// Save this entity as enemy, they will take 100% damage.
				if ( m_hEnemy.Get() == NULL )
				{
					m_hEnemy = pTFVictim;
				}

				return;
			}
		}
	}

	// If we've already got an impact time, don't impact again.
	if ( m_flImpactTime > 0.0 )
		return;

	// Save this entity as enemy, they will take 100% damage.
	if ( m_hEnemy.Get() == NULL )
	{
		m_hEnemy = pOther;
	}

	if ( !pTFVictim )
	{
		m_bImpact = true;
	}

	// Invisible.
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;

	bool bDetonate = false;
	bool bNoRandomCrit = false;
	if ( pFlareGun )
	{
		switch ( pFlareGun->GetFlareGunType() )
		{
		case FLAREGUN_DETONATE:
			bDetonate = true;
			break;

		case FLAREGUN_GRORDBORT:
			bNoRandomCrit = true;
			break;

		case FLAREGUN_SCORCHSHOT:
			bDetonate = true;
			bNoRandomCrit = true;
			break;
		}
	}

	// Flares that hit a burning player crit, unless it's a detonate flare - they mini-crit
	if ( pTFVictim && pTFVictim->m_Shared.InCond( TF_COND_BURNING ) && !bDetonate && !bNoRandomCrit )
	{
		m_bCritical = true;
	}

	CTakeDamageInfo info( this, pAttacker, m_hLauncher, vec3_origin, vecOrigin, GetDamage(), GetDamageType(), TF_DMG_CUSTOM_BURNING_FLARE );
	pOther->TakeDamage( info );

	// Remove the flare.
	if ( m_bImpact )
	{
		SetMoveType( MOVETYPE_FLY );
		SetAbsVelocity( vec3_origin );

		m_vecImpactNormal = pTrace->plane.normal;
		m_flImpactTime = gpGlobals->curtime + 0.1f;

		// Stick into and object and fizzle a little while.
		SetContextThink( &CTFProjectile_Flare::ImpactThink, gpGlobals->curtime, FLARE_THINK_CONTEXT );

		// Only do this for the Detonator
		if ( bDetonate )
		{
			// Scorch Shot can still light others in this case
			Detonate( pFlareGun->GetFlareGunType() != FLAREGUN_SCORCHSHOT );
		}
	}
	else
	{
		// Impact player sound.
		CPVSFilter filter( vecOrigin );
		EmitSound( filter, pOther->entindex(), "TFPlayer.FlareImpact" );

		SendDeathNotice();
		UTIL_Remove( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Custom explode for air burst flare
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::Explode_Air( trace_t *pTrace, int bitsDamageType, bool bSelfOnly )
{
	// Invisible.
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;

	// Play explosion sound and effect.
	Vector vecOrigin = GetAbsOrigin();
	CPVSFilter filter( vecOrigin );

	CBaseEntity *pAttacker = GetOwnerEntity();
	int nDefID = -1;
	WeaponSound_t nSound = SPECIAL1;
	if ( pAttacker )
	{
		CTFFlareGun *pFlareGun = dynamic_cast<CTFFlareGun*>( ToTFPlayer( pAttacker )->GetActiveWeapon() );
		if ( pFlareGun )
		{
			CEconItemView *pItem = pFlareGun->GetAttributeContainer()->GetItem();
			nDefID = pItem->GetItemDefIndex();
		}

		// Damage.
		IScorer *pScorerInterface = dynamic_cast<IScorer*>( pAttacker );
		if ( pScorerInterface )
		{
			pAttacker = pScorerInterface->GetScorer();
		}

		float flRadius = bSelfOnly ? 0.f : GetRadius();

		if ( bSelfOnly )
		{
			bitsDamageType |= DMG_BLAST;
			nSound = SPECIAL2;
		}

		CTakeDamageInfo info( this, pAttacker, m_hLauncher, vec3_origin, vecOrigin, GetDamage(), bitsDamageType | DMG_HALF_FALLOFF, TF_DMG_CUSTOM_FLARE_EXPLOSION );
		CTFRadiusDamageInfo radiusinfo( &info, vecOrigin, flRadius, NULL, TF_FLARE_RADIUS_FOR_FJS );
		TFGameRules()->RadiusDamage( radiusinfo );
	}

	const char *pszParticle = bSelfOnly ? "Explosions_MA_FlyingEmbers" : "ExplosionCore_MidAir_Flare";
	TE_TFExplosion( filter, 0.0f, vecOrigin, pTrace->plane.normal, GetWeaponID(), entindex(), nDefID, nSound );
	TE_TFParticleEffect( filter, 0.0f, pszParticle, vecOrigin, pTrace->plane.normal, vec3_angle );
	CSoundEnt::InsertSound ( SOUND_COMBAT, vecOrigin, 1024, 3.0 );

	SendDeathNotice();
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: Alt-fire air burst flare
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::Detonate( bool bSelfOnly )
{
	trace_t		tr;
	Vector		vecSpot;

	SetThink( NULL );

	vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, & tr);

	Explode_Air( &tr, GetDamageType(), bSelfOnly );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFProjectile_Flare::GetRadius( void ) 
{ 
	float flRadius = TF_FLARE_DET_RADIUS;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_hLauncher, flRadius, mult_explosion_radius );
	return flRadius; 
}

//-----------------------------------------------------------------------------
// Purpose: Let the flaregun know we're gone
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::SendDeathNotice( void ) 
{ 
	CBaseEntity *pAttacker = GetOwnerEntity();
	if ( !pAttacker )
		return;

	CTFFlareGun *pFlareGun = dynamic_cast<CTFFlareGun*>( ToTFPlayer( pAttacker )->GetActiveWeapon() );
	if ( pFlareGun && pFlareGun->GetFlareGunType() == FLAREGUN_DETONATE )
	{
		pFlareGun->DeathNotice( this );
	}
}	

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::ImpactThink( void )
{
	if ( gpGlobals->curtime > m_flImpactTime )
	{
		// If we hit anything other than the player create an impact - effect, sound, etc.
		if ( m_hEnemy.Get() )
		{
			Vector vecOrigin = GetAbsOrigin();
			CPVSFilter filter( vecOrigin );
			TE_TFExplosion( filter, 0.0f, vecOrigin, m_vecImpactNormal, GetWeaponID(), m_hEnemy.Get()->entindex() );
		}

		SendDeathNotice();
		UTIL_Remove( this );
		SetContextThink( NULL, 0, FLARE_THINK_CONTEXT );
	}
	else
	{
		SetContextThink( &CTFProjectile_Flare::ImpactThink, gpGlobals->curtime + 0.1f, FLARE_THINK_CONTEXT );
	}
}

void CTFProjectile_Flare::PerformCustomPhysics( Vector *pNewPosition, Vector *pNewVelocity, QAngle *pNewAngles, QAngle *pNewAngVelocity )
{
	if ( m_flNextSeekUpdate < gpGlobals->curtime )
	{
		CTFPlayer *pBestTarget = NULL;
		const float flMaxSeekDistanceSqr = ( 1024.0f * 1024.0f );
		float flBestDistance = flMaxSeekDistanceSqr;

		// Loop through players and attempt to find a seek target
		int i;
		for ( i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( !pPlayer )
				continue;

			bool bBurning = pPlayer->m_Shared.InCond( TF_COND_BURNING );

			if ( !bBurning || 
				 pPlayer->InSameTeam( this ) || 
				 ( pPlayer->m_Shared.GetDisguiseTeam() == GetTeamNumber() && !bBurning ) ||
				 ( pPlayer->m_Shared.IsStealthed() && !bBurning ) || 
				 pPlayer->GetTeamNumber() == TEAM_SPECTATOR ||
				 !pPlayer->IsAlive() )
			{
				continue;
			}

			Vector vToTarget = pPlayer->WorldSpaceCenter() - GetAbsOrigin();
			VectorNormalize( vToTarget );

			Vector vForward = *pNewVelocity;
			VectorNormalize( vForward );

			if ( vToTarget.Dot( vForward ) < -0.25f )
				continue;

			float flDistanceSqr = pPlayer->WorldSpaceCenter().DistToSqr( GetAbsOrigin() );
			if ( flBestDistance > flDistanceSqr )
			{
				trace_t tr;
				UTIL_TraceLine( pPlayer->WorldSpaceCenter(), GetAbsOrigin(), MASK_SOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, &tr );
				if ( !tr.DidHit() || tr.m_pEnt == this )
				{
					flBestDistance = flDistanceSqr;
					pBestTarget = pPlayer;
				}
			}
		}

		float flHeatSeekPower = GetHeatSeekPower();

		QAngle angToTarget = *pNewAngles;

		if ( pBestTarget )
		{
			Vector vToTarget = pBestTarget->WorldSpaceCenter() - GetAbsOrigin();
			VectorAngles( vToTarget, angToTarget );
		}

		const float flUpdatesPerSecond = 4.0f;

		if ( angToTarget != *pNewAngles )
		{
			pNewAngVelocity->x = Approach( UTIL_AngleDiff( angToTarget.x, pNewAngles->x ) * flUpdatesPerSecond, pNewAngVelocity->x, flHeatSeekPower );
			pNewAngVelocity->y = Approach( UTIL_AngleDiff( angToTarget.y, pNewAngles->y ) * flUpdatesPerSecond, pNewAngVelocity->y, flHeatSeekPower );
			pNewAngVelocity->z = Approach( UTIL_AngleDiff( angToTarget.z, pNewAngles->z ) * flUpdatesPerSecond, pNewAngVelocity->z, flHeatSeekPower );

			const float flMaxAngularVelocity = 360.0f;
			pNewAngVelocity->x = clamp( pNewAngVelocity->x, -flMaxAngularVelocity, flMaxAngularVelocity );
			pNewAngVelocity->y = clamp( pNewAngVelocity->y, -flMaxAngularVelocity, flMaxAngularVelocity );
			pNewAngVelocity->z = clamp( pNewAngVelocity->z, -flMaxAngularVelocity, flMaxAngularVelocity );
		}

		m_flNextSeekUpdate = gpGlobals->curtime + ( 1.0f / flUpdatesPerSecond );
	}

	*pNewAngles += *pNewAngVelocity * gpGlobals->frametime;

	Vector vForward;
	AngleVectors( *pNewAngles, &vForward );
	*pNewVelocity = vForward * GetProjectileSpeed();

	*pNewPosition += *pNewVelocity * gpGlobals->frametime;
}

//-----------------------------------------------------------------------------
// Purpose: Flare was deflected.
//-----------------------------------------------------------------------------
void CTFProjectile_Flare::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	CTFPlayer *pTFDeflector = ToTFPlayer( pDeflectedBy );
	if ( !pTFDeflector )
		return;

	ChangeTeam( pTFDeflector->GetTeamNumber() );
	SetLauncher( pTFDeflector->GetActiveWeapon() );

	CTFPlayer* pOldOwner = ToTFPlayer( GetOwnerEntity() );
	SetOwnerEntity( pTFDeflector );

	if ( pOldOwner )
	{
		pOldOwner->SpeakConceptIfAllowed( MP_CONCEPT_DEFLECTED, "projectile:1,victim:1" );
	}

	if ( pTFDeflector->m_Shared.IsCritBoosted() )
	{
		SetCritical( true );
	}

	CTFWeaponBase::SendObjectDeflectedEvent( pTFDeflector, pOldOwner, GetWeaponID(), this );

	IncrementDeflected();
	SetScorer( pTFDeflector );
}

float CTFProjectile_Flare::GetProjectileSpeed( void ) const
{
	float flLaunchSpeed = FLARE_SPEED;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_hLauncher, flLaunchSpeed, mult_projectile_speed );

	return flLaunchSpeed;
}

float CTFProjectile_Flare::GetHeatSeekPower( void ) const
{
	float flHeatSeekPower = 0.0;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_hLauncher, flHeatSeekPower, mod_projectile_heat_seek_power );

	return flHeatSeekPower;
}
