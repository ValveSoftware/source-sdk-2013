//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Base Rockets.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase_rocket.h"

// Server specific.
#ifdef GAME_DLL
#include "soundent.h"
#include "te_effect_dispatch.h"
#include "tf_fx.h"
#include "iscorer.h"
#include "tf_gamerules.h"
#include "func_nogrenades.h"
#include "tf_obj_sentrygun.h"

extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
extern void SendProxy_Angles( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
#endif

#ifdef CLIENT_DLL
#include "props_shared.h"
#include "usermessages.h"
#endif

//w_rocket_airstrike\w_rocket_airstrike.mdl
#define MINI_ROCKETS_MODEL					"models/weapons/w_models/w_rocket_airstrike/w_rocket_airstrike.mdl"

//=============================================================================
//
// TF Base Rocket tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFBaseRocket, DT_TFBaseRocket )

BEGIN_NETWORK_TABLE( CTFBaseRocket, DT_TFBaseRocket )
// Client specific.
#ifdef CLIENT_DLL
RecvPropVector( RECVINFO( m_vInitialVelocity ) ),

RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
RecvPropQAngles( RECVINFO_NAME( m_angNetworkAngles, m_angRotation ) ),
RecvPropInt( RECVINFO( m_iDeflected ) ),
RecvPropEHandle( RECVINFO( m_hLauncher ) ),

// Server specific.
#else
SendPropVector( SENDINFO( m_vInitialVelocity ), 12 /*nbits*/, 0 /*flags*/, -3000 /*low value*/, 3000 /*high value*/	),

SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
SendPropExclude( "DT_BaseEntity", "m_angRotation" ),

SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_COORD_MP_INTEGRAL|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
SendPropQAngles	(SENDINFO(m_angRotation), 6, SPROP_CHANGES_OFTEN, SendProxy_Angles ),
SendPropInt( SENDINFO( m_iDeflected ), 4, SPROP_UNSIGNED ),
SendPropEHandle( SENDINFO( m_hLauncher ) ),

#endif
END_NETWORK_TABLE()

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTFBaseRocket )
END_DATADESC()
#endif

#ifdef _DEBUG
ConVar tf_rocket_show_radius( "tf_rocket_show_radius", "0", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Render rocket radius." );
#endif

//=============================================================================
//
// Shared (client/server) functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFBaseRocket::CTFBaseRocket()
{
	m_vInitialVelocity.Init();
	m_iDeflected = 0;
	
// Client specific.
#ifdef CLIENT_DLL

	m_flSpawnTime = 0.0f;
	m_iCachedDeflect = false;
	
// Server specific.
#else

	m_flDamage = 0.0f;
	m_flDestroyableTime = 0.0f;
	m_bStunOnImpact = false;
	m_flDamageForceScale = 1.0f;

#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFBaseRocket::~CTFBaseRocket()
{}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseRocket::Precache( void )
{
	BaseClass::Precache();
	PrecacheParticleSystem( "Explosion_ShockWave_01" );
	PrecacheParticleSystem( "ExplosionCore_Wall_Jumper" );
	PrecacheModel( MINI_ROCKETS_MODEL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseRocket::Spawn( void )
{
	BaseClass::Spawn();

	// Precache.
	Precache();
	UseClientSideAnimation();
	
	if ( GetLauncher() )
	{
		int iMiniRocket = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( GetLauncher(), iMiniRocket, mini_rockets );
		if ( iMiniRocket )
		{
			SetModel( MINI_ROCKETS_MODEL );
		}
	}

// Client specific.
#ifdef CLIENT_DLL

	m_flSpawnTime = gpGlobals->curtime;

// Server specific.
#else

	//Derived classes must have set model.
	Assert( GetModel() );	

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLY, MOVECOLLIDE_FLY_CUSTOM );
	AddEFlags( EFL_NO_WATER_VELOCITY_CHANGE );
	AddEffects( EF_NOSHADOW );

	SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS );

	UTIL_SetSize( this, -Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	ResetSequence( LookupSequence("idle") );

	// Setup attributes.
	m_takedamage = DAMAGE_NO;
	SetGravity( 0.0f );

	// Setup the touch and think functions.
	SetTouch( &CTFBaseRocket::RocketTouch );
	SetNextThink( gpGlobals->curtime );

	AddFlag( FL_GRENADE );

	m_flDestroyableTime = gpGlobals->curtime + TF_ROCKET_DESTROYABLE_TIMER;
	m_bCritical = false;

#endif
}

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseRocket::PostDataUpdate( DataUpdateType_t type )
{
	// Pass through to the base class.
	BaseClass::PostDataUpdate( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		// Now stick our initial velocity and angles into the interpolation history.
		CInterpolatedVar<Vector> &interpolator = GetOriginInterpolator();
		interpolator.ClearHistory();

		CInterpolatedVar<QAngle> &rotInterpolator = GetRotationInterpolator();
		rotInterpolator.ClearHistory();

		float flChangeTime = GetLastChangeTime( LATCH_SIMULATION_VAR );

		// Add a sample 1 second back.
		Vector vCurOrigin = GetLocalOrigin() - m_vInitialVelocity;
		interpolator.AddToHead( flChangeTime - 1.0f, &vCurOrigin, false );

		QAngle vCurAngles = GetLocalAngles();
		rotInterpolator.AddToHead( flChangeTime - 1.0f, &vCurAngles, false );

		// Add the current sample.
		vCurOrigin = GetLocalOrigin();
		interpolator.AddToHead( flChangeTime, &vCurOrigin, false );

		rotInterpolator.AddToHead( flChangeTime - 1.0, &vCurAngles, false );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseRocket::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);

	if ( updateType == DATA_UPDATE_CREATED || m_iCachedDeflect != GetDeflected() )
	{
		CreateTrails();		
	}

	m_iCachedDeflect = GetDeflected();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFBaseRocket::DrawModel( int flags )
{
	// During the first 0.2 seconds of our life, don't draw ourselves.
	if ( gpGlobals->curtime - m_flSpawnTime < 0.2f )
		return 0;

	return BaseClass::DrawModel( flags );
}

//=============================================================================
//
// Server specific functions.
//
#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFBaseRocket *CTFBaseRocket::Create( CBaseEntity *pLauncher, const char *pszClassname, const Vector &vecOrigin, 
									  const QAngle &vecAngles, CBaseEntity *pOwner )
{
	CTFBaseRocket *pRocket = static_cast<CTFBaseRocket*>( CBaseEntity::Create( pszClassname, vecOrigin, vecAngles, pOwner ) );
	if ( !pRocket )
		return NULL;

	pRocket->SetLauncher( pLauncher );

	// Initialize the owner.
	pRocket->SetOwnerEntity( pOwner );

	// Spawn.
	pRocket->Spawn();

	// Setup the initial velocity.
	Vector vecForward, vecRight, vecUp;
	AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

	float flLaunchSpeed = 1100.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pLauncher, flLaunchSpeed, mult_projectile_speed );

	// Hack: This attribute represents a bucket of attributes - one of which is projectile speed.
	// If the concept works we'll make the "bucket" system directly modify the attributes instead.
	if ( pOwner )
	{
		// if the owner is a Sentry, Check its owner
		int iRocketSpecialist = 0;
		CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun*>( pOwner );
		if ( pSentry )
		{
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pSentry->GetOwner(), iRocketSpecialist, rocket_specialist );
		}
		else
		{
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pOwner, iRocketSpecialist, rocket_specialist );
		}

		if ( iRocketSpecialist )
		{
			flLaunchSpeed *= RemapValClamped( iRocketSpecialist, 1.f, 4.f, 1.15f, 1.6f );
			flLaunchSpeed = Min( flLaunchSpeed, 3000.f );
		}
	}

	CTFPlayer *pTFOwner = ToTFPlayer( pRocket->GetOwnerPlayer() );

	if ( pTFOwner )
	{
		pRocket->SetTruceValidForEnt( pTFOwner->IsTruceValidForEnt() );

		if ( pTFOwner->m_Shared.GetCarryingRuneType() == RUNE_PRECISION )
		{
			flLaunchSpeed = 3000.f;
		}
	}

	Vector vecVelocity = vecForward * flLaunchSpeed;
	pRocket->SetAbsVelocity( vecVelocity );	
	pRocket->SetupInitialTransmittedGrenadeVelocity( vecVelocity );

	// Setup the initial angles.
	QAngle angles;
	VectorAngles( vecVelocity, angles );
	pRocket->SetAbsAngles( angles );

	// Set team.
	pRocket->ChangeTeam( pOwner->GetTeamNumber() );

	return pRocket;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseRocket::RocketTouch( CBaseEntity *pOther )
{
	// Verify a correct "other."
	Assert( pOther );
	bool bShield = pOther->IsCombatItem() && !InSameTeam( pOther );
	if ( pOther->IsSolidFlagSet( FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS ) && !bShield )
		return;

	// Handle hitting skybox (disappear).
	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
	if( pTrace->surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	trace_t trace;
	memcpy( &trace, pTrace, sizeof( trace_t ) );
	Explode( &trace, pOther );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CTFBaseRocket::PhysicsSolidMaskForEntity( void ) const
{ 
	int teamContents = 0;

	if ( !CanCollideWithTeammates() )
	{
		// Only collide with the other team
		teamContents = ( GetTeamNumber() == TF_TEAM_RED ) ? CONTENTS_BLUETEAM : CONTENTS_REDTEAM;
	}
	else
	{
		// Collide with both teams
		teamContents = CONTENTS_REDTEAM | CONTENTS_BLUETEAM;
	}

	return BaseClass::PhysicsSolidMaskForEntity() | teamContents;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBaseRocket::ShouldNotDetonate( void )
{
	return InNoGrenadeZone( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseRocket::Destroy( bool bBlinkOut, bool bBreakRocket )
{
	if ( bBreakRocket )
	{
		CPVSFilter filter( GetAbsOrigin() );
		UserMessageBegin( filter, "BreakModelRocketDud" );
		WRITE_SHORT( GetModelIndex() );
		WRITE_VEC3COORD( GetAbsOrigin() );
		WRITE_ANGLES( GetAbsAngles() );
		MessageEnd();
	}

	// Kill it
	SetThink( &BaseClass::SUB_Remove );
	SetNextThink( gpGlobals->curtime );
	SetTouch( NULL );
	AddEffects( EF_NODRAW );

	if ( bBlinkOut )
	{
		// Sprite flash
		CSprite *pGlowSprite = CSprite::SpriteCreate( NOGRENADE_SPRITE, GetAbsOrigin(), false );
		if ( pGlowSprite )
		{
			pGlowSprite->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxFadeFast );
			pGlowSprite->SetThink( &CSprite::SUB_Remove );
			pGlowSprite->SetNextThink( gpGlobals->curtime + 1.0 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseRocket::Explode( trace_t *pTrace, CBaseEntity *pOther )
{
	if ( ShouldNotDetonate() )
	{
		Destroy( true );
		return;
	}

	// Save this entity as enemy, they will take 100% damage.
	m_hEnemy = pOther;

	// Invisible.
	SetModelName( NULL_STRING );
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;

	// Pull out a bit.
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	// Play explosion sound and effect.
	Vector vecOrigin = GetAbsOrigin();
	CPVSFilter filter( vecOrigin );
	
	// Halloween Spell Effect Check
	int iHalloweenSpell = 0;
	int iCustomParticleIndex = INVALID_STRING_INDEX;
	item_definition_index_t ownerWeaponDefIndex = INVALID_ITEM_DEF_INDEX;
	// if the owner is a Sentry, Check its owner
	CBaseEntity *pPlayerOwner = GetOwnerPlayer();

	if ( TF_IsHolidayActive( kHoliday_HalloweenOrFullMoon ) )
	{
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayerOwner, iHalloweenSpell, halloween_pumpkin_explosions );
		if ( iHalloweenSpell > 0 )
		{
			iCustomParticleIndex = GetParticleSystemIndex( "halloween_explosion" );
		}
	}

	int iNoSelfBlastDamage = 0;
	CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >( GetOriginalLauncher() );
	if ( pWeapon )
	{
		ownerWeaponDefIndex = pWeapon->GetAttributeContainer()->GetItem()->GetItemDefIndex();

		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iNoSelfBlastDamage, no_self_blast_dmg );
		if ( iNoSelfBlastDamage )
		{
			iCustomParticleIndex = GetParticleSystemIndex( "ExplosionCore_Wall_Jumper" );
		}
	}
	
	int iLargeExplosion = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayerOwner, iLargeExplosion, use_large_smoke_explosion );
	if ( iLargeExplosion > 0 )
	{
		DispatchParticleEffect( "explosionTrail_seeds_mvm", GetAbsOrigin(), GetAbsAngles() );
		DispatchParticleEffect( "fluidSmokeExpl_ring_mvm", GetAbsOrigin(), GetAbsAngles() );
	}

	TE_TFExplosion( filter, 0.0f, vecOrigin, pTrace->plane.normal, GetWeaponID(), pOther->entindex(), ownerWeaponDefIndex, SPECIAL1, iCustomParticleIndex );

	CSoundEnt::InsertSound ( SOUND_COMBAT, vecOrigin, 1024, 3.0 );

	// Damage.
	CBaseEntity *pAttacker = GetOwnerEntity();
	IScorer *pScorerInterface = dynamic_cast<IScorer*>( pAttacker );
	if ( pScorerInterface )
	{
		pAttacker = pScorerInterface->GetScorer();
	}
	else if ( pAttacker && pAttacker->GetOwnerEntity() )
	{
		pAttacker = pAttacker->GetOwnerEntity();
	}

	float flRadius = GetRadius();

	if ( pAttacker ) // No attacker, deal no damage. Otherwise we could potentially kill teammates.
	{
		CTFPlayer *pTarget = ToTFPlayer( GetEnemy() );
		if ( pTarget )
		{
			// Rocket Specialist
			CheckForStunOnImpact( pTarget );

			RecordEnemyPlayerHit( pTarget, true );
		}

		CTakeDamageInfo info( this, pAttacker, GetOriginalLauncher(), vec3_origin, vecOrigin, GetDamage(), GetDamageType(), GetDamageCustom() );
		CTFRadiusDamageInfo radiusinfo( &info, vecOrigin, flRadius, NULL, TF_ROCKET_RADIUS_FOR_RJS, GetDamageForceScale() );
		TFGameRules()->RadiusDamage( radiusinfo );
	}


	// Don't decal players with scorch.
	if ( !pOther->IsPlayer() && ( iNoSelfBlastDamage == 0 ) )
	{
		UTIL_DecalTrace( pTrace, "Scorch" );
	}

	// Remove the rocket.
	UTIL_Remove( this );

	return;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFBaseRocket::CheckForStunOnImpact( CTFPlayer* pTarget )
{
	if ( !m_bStunOnImpact )
		return;

	CTFPlayer *pAttacker = ToTFPlayer( GetOwnerPlayer() );
	if ( !pAttacker )
		return;

	int iRocketSpecialist = GetStunLevel();
	if ( !iRocketSpecialist )
		return;

	// Stun
	float flStunAmount = pTarget->IsMiniBoss() ? 0.85f : 1.f;
	float flStunTime = RemapValClamped( iRocketSpecialist, 1.f, 4.f, 0.5f, 0.75f );

	pTarget->SetAbsVelocity( vec3_origin );
	pTarget->m_Shared.StunPlayer( flStunTime, flStunAmount, TF_STUN_MOVEMENT | TF_STUN_NO_EFFECTS, pAttacker );

	if ( TFGameRules()->IsMannVsMachineMode() && pTarget->IsBot() && ( pAttacker->GetTeamNumber() == TF_TEAM_PVE_DEFENDERS ) )
	{
		pAttacker->AwardAchievement( ACHIEVEMENT_TF_MVM_ROCKET_SPECIALIST_STUN_GRIND );
	}


	// Effect
	CPVSFilter filter( GetAbsOrigin() );
	TE_TFParticleEffect( filter, 0.0, "mvm_soldier_shockwave", GetAbsOrigin(), QAngle( 0, 0, 0 ) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFBaseRocket::GetStunLevel( void )
{
	CTFPlayer *pAttacker = ToTFPlayer( GetOwnerPlayer() );
	if ( !pAttacker )
		return 0;

	int iRocketSpecialist = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pAttacker, iRocketSpecialist, rocket_specialist );
	return iRocketSpecialist;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBaseRocket::GetRadius() 
{ 
	float flRadius = TF_ROCKET_RADIUS;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_hLauncher, flRadius, mult_explosion_radius );

	CBaseEntity *pAttacker = GetOwnerPlayer();
	if ( pAttacker )
	{
		int iRocketSpecialist = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pAttacker, iRocketSpecialist, rocket_specialist );
		if ( iRocketSpecialist )
		{
			bool bDirectHit = ( GetEnemy() && GetEnemy()->GetTeamNumber() != pAttacker->GetTeamNumber() && 
								( GetEnemy()->IsPlayer() || GetEnemy()->MyCombatCharacterPointer() ) );
			// If we have the Rocket Specialist attribute and hit an enemy combatant directly...
			if ( bDirectHit )
			{
				// Increased blast radius
				flRadius *= RemapValClamped( iRocketSpecialist, 1.f, 4.f, 1.15f, 1.6f );
				m_bStunOnImpact = true;
			}
		}

		CTFPlayer *pTFPlayer = ToTFPlayer( pAttacker );
		// Airstrike gets a small blast radius penalty while Rjing
		if ( pTFPlayer && pTFPlayer->m_Shared.InCond( TF_COND_BLASTJUMPING ) )
		{
			// Using this attr to key in the AirStrike
			float flRocketJumpAttackBonus = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pAttacker, flRocketJumpAttackBonus, rocketjump_attackrate_bonus );
			if ( flRocketJumpAttackBonus != 1.0f )
			{
				flRadius *= 0.80;
			}
		}
	}

	return flRadius; 
}


//-----------------------------------------------------------------------------
// Checks if the owner is a sentry gun, if so returns the sentry guns owner
//-----------------------------------------------------------------------------
CBaseEntity *CTFBaseRocket::GetOwnerPlayer( void ) const
{
	// if the owner is a Sentry, Check its owner
	CBaseEntity *pOwner = GetOwnerEntity();
	CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun*>( pOwner );

	if ( pSentry )
	{
		return pSentry->GetOwner();
	}
	return pOwner;
}

#endif

#if defined( CLIENT_DLL )

//-----------------------------------------------------------------------------
// Receive the BreakModelRocketDud user message
//-----------------------------------------------------------------------------
USER_MESSAGE( BreakModelRocketDud )
{
	int nModelIndex = (int)msg.ReadShort();
	CUtlVector<breakmodel_t>	aGibs;
	BuildGibList( aGibs, nModelIndex, 1.0f, COLLISION_GROUP_NONE );
	if ( !aGibs.Count() )
		return;

	// Get the origin & angles
	Vector vecOrigin, vecForward;
	QAngle vecAngles;
	msg.ReadBitVec3Coord( vecOrigin );
	msg.ReadBitAngles( vecAngles );

	AngleVectors( vecAngles, &vecForward );

	Vector vecBreakVelocity = Vector(0,0,300) + vecForward*-400;
	AngularImpulse angularImpulse( 0, RandomFloat( -500, -3000 ), 0 );
	breakablepropparams_t breakParams( vecOrigin, vecAngles, vecBreakVelocity, angularImpulse );
	breakParams.impactEnergyScale = 1.0f;

	CreateGibsFromList( aGibs, nModelIndex, NULL, breakParams, NULL, -1 , false, true );
}

#endif
