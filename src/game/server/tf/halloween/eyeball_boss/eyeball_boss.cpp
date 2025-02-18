//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss.cpp
// The 2011 Halloween Boss
// Michael Booth, October 2011

#include "cbase.h"

#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "tf_projectile_arrow.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "tf_ammo_pack.h"
#include "nav_mesh/tf_nav_area.h"
#include "NextBot/Path/NextBotChasePath.h"
#include "econ_wearable.h"
#include "team_control_point_master.h"
#include "particle_parse.h"
#include "nav_mesh/tf_path_follower.h"
#include "tf_obj_sentrygun.h"
#include "bot/map_entities/tf_spawner.h"
#include "tf_fx.h"
#include "player_vs_environment/monster_resource.h"

#include "eyeball_boss.h"
#include "eyeball_behavior/eyeball_boss_behavior.h"
#include "halloween/zombie/zombie.h"


ConVar tf_eyeball_boss_debug( "tf_eyeball_boss_debug", "0", FCVAR_CHEAT );
ConVar tf_eyeball_boss_debug_orientation( "tf_eyeball_boss_debug_orientation", "0", FCVAR_CHEAT );

ConVar tf_eyeball_boss_lifetime( "tf_eyeball_boss_lifetime", "120", FCVAR_CHEAT );
ConVar tf_eyeball_boss_lifetime_spell( "tf_eyeball_boss_lifetime_spell", "8", FCVAR_CHEAT );

ConVar tf_eyeball_boss_speed( "tf_eyeball_boss_speed", "250", FCVAR_CHEAT );

ConVar tf_eyeball_boss_hover_height( "tf_eyeball_boss_hover_height", "200", FCVAR_CHEAT );

ConVar tf_eyeball_boss_acceleration( "tf_eyeball_boss_acceleration", "500", FCVAR_CHEAT );
ConVar tf_eyeball_boss_horiz_damping( "tf_eyeball_boss_horiz_damping", "2", FCVAR_CHEAT );
ConVar tf_eyeball_boss_vert_damping( "tf_eyeball_boss_vert_damping", "1", FCVAR_CHEAT );

ConVar tf_eyeball_boss_attack_range( "tf_eyeball_boss_attack_range", "750", FCVAR_CHEAT );

ConVar tf_eyeball_boss_health_base( "tf_eyeball_boss_health_base", "8000", FCVAR_CHEAT );
ConVar tf_eyeball_boss_health_per_player( "tf_eyeball_boss_health_per_player", "400", FCVAR_CHEAT );
extern ConVar tf_halloween_bot_min_player_count;

ConVar tf_eyeball_boss_health_at_level_2( "tf_eyeball_boss_health_at_level_2", "17000", FCVAR_CHEAT );
ConVar tf_eyeball_boss_health_per_level( "tf_eyeball_boss_health_per_level", "3000", FCVAR_CHEAT );


LINK_ENTITY_TO_CLASS( eyeball_boss, CEyeballBoss );

IMPLEMENT_SERVERCLASS_ST( CEyeballBoss, DT_EyeballBoss )

	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),	// client has its own orientation logic
	SendPropExclude( "DT_BaseEntity", "m_angAbsRotation" ),	// client has its own orientation logic
	SendPropVector( SENDINFO( m_lookAtSpot ), 0, SPROP_COORD ),
	SendPropInt( SENDINFO( m_attitude ) ),

END_SEND_TABLE()


int CEyeballBoss::m_level = 1;

IMPLEMENT_AUTO_LIST( IEyeballBossAutoList );

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
CEyeballBoss::CEyeballBoss()
{
	ALLOCATE_INTENTION_INTERFACE( CEyeballBoss );

	m_locomotor = new CEyeballBossLocomotion( this );
	m_body = new CEyeballBossBody( this );
	m_vision = new CDisableVision( this );

	m_eyeOffset = vec3_origin;
	m_target = NULL;
	m_rageTimer.Invalidate();
	m_victim = NULL;
	m_lookAtSpot = vec3_origin;
	m_attitude = EYEBALL_CALM;
	m_damageLimit = -1;
}


//-----------------------------------------------------------------------------------------------------
CEyeballBoss::~CEyeballBoss()
{
	DEALLOCATE_INTENTION_INTERFACE;

	if ( m_vision )
		delete m_vision;

	if ( m_body )
		delete m_body;

	if ( m_locomotor )
		delete m_locomotor;

	IGameEvent *event = gameeventmanager->CreateEvent( "recalculate_truce" );
	if ( event )
	{
		gameeventmanager->FireEvent( event, true );
	}
}

void CEyeballBoss::PrecacheEyeballBoss()
{
	PrecacheModel( "models/props_halloween/halloween_demoeye.mdl" );
	PrecacheModel( "models/props_halloween/eyeball_projectile.mdl" );

	PrecacheScriptSound( "Halloween.EyeballBossIdle" );
	PrecacheScriptSound( "Halloween.EyeballBossBecomeAlert" );
	PrecacheScriptSound( "Halloween.EyeballBossAcquiredVictim" );
	PrecacheScriptSound( "Halloween.EyeballBossStunned" );
	PrecacheScriptSound( "Halloween.EyeballBossStunRecover" );
	PrecacheScriptSound( "Halloween.EyeballBossLaugh" );
	PrecacheScriptSound( "Halloween.EyeballBossBigLaugh" );
	PrecacheScriptSound( "Halloween.EyeballBossDie" );
	PrecacheScriptSound( "Halloween.EyeballBossEscapeSoon" );
	PrecacheScriptSound( "Halloween.EyeballBossEscapeImminent" );
	PrecacheScriptSound( "Halloween.EyeballBossEscaped" );
	PrecacheScriptSound( "Halloween.EyeballBossDie" );
	PrecacheScriptSound( "Halloween.EyeballBossTeleport" );
	PrecacheScriptSound( "Halloween.HeadlessBossSpawnRumble" );

	PrecacheScriptSound( "Halloween.EyeballBossBecomeEnraged" );
	PrecacheScriptSound( "Halloween.EyeballBossRage" );
	PrecacheScriptSound( "Halloween.EyeballBossCalmDown" );
	PrecacheScriptSound( "Halloween.spell_spawn_boss_disappear" );

	PrecacheScriptSound( "Halloween.MonoculusBossSpawn" );
	PrecacheScriptSound( "Halloween.MonoculusBossDeath" );

	PrecacheParticleSystem( "eyeboss_death" );
	PrecacheParticleSystem( "eyeboss_aura_angry" );
	PrecacheParticleSystem( "eyeboss_aura_grumpy" );
	PrecacheParticleSystem( "eyeboss_aura_calm" );
	PrecacheParticleSystem( "eyeboss_aura_stunned" );
	PrecacheParticleSystem( "eyeboss_tp_normal" );
	PrecacheParticleSystem( "eyeboss_tp_escape" );
	PrecacheParticleSystem( "eyeboss_team_red" );
	PrecacheParticleSystem( "eyeboss_team_blue" );
}

//-----------------------------------------------------------------------------------------------------
void CEyeballBoss::Precache()
{
	BaseClass::Precache();

	// always allow late precaching, so we don't pay the cost of the
	// Halloween Boss for the entire year

	bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	PrecacheEyeballBoss();

	CBaseEntity::SetAllowPrecache( bAllowPrecache );
}


//-----------------------------------------------------------------------------------------------------
void CEyeballBoss::Spawn( void )
{
	Precache();

	BaseClass::Spawn();

	SetModel( "models/props_halloween/halloween_demoeye.mdl" );

	int health = tf_eyeball_boss_health_base.GetInt();

	if ( m_level > 1 )
	{
		// the Boss was defeated last time - he's tougher this time
		health = tf_eyeball_boss_health_at_level_2.GetInt();

		health += tf_eyeball_boss_health_per_level.GetInt() * ( m_level - 2 );
	}
	else
	{
		// scale the boss' health with the player count
		int totalPlayers = GetGlobalTFTeam( TF_TEAM_BLUE )->GetNumPlayers() + GetGlobalTFTeam( TF_TEAM_RED )->GetNumPlayers();

		if ( totalPlayers > tf_halloween_bot_min_player_count.GetInt() )
		{
			health += ( totalPlayers - tf_halloween_bot_min_player_count.GetInt() ) * tf_eyeball_boss_health_per_player.GetInt();
		}
	}

	SetHealth( health );
	SetMaxHealth( health );

	m_homePos = GetAbsOrigin();

	Vector mins( -50, -50, -50 );
	Vector maxs( 50, 50, 50 );
	CollisionProp()->SetSurroundingBoundsType( USE_SPECIFIED_BOUNDS, &mins, &maxs );
	CollisionProp()->SetCollisionBounds( mins, maxs );

	m_lookAtSpot = vec3_origin;

	CBaseEntity *spawnPoint = NULL;
	while( ( spawnPoint = gEntList.FindEntityByClassname( spawnPoint, "info_target" ) ) != NULL )
	{
		if ( FStrEq( STRING( spawnPoint->GetEntityName() ), "spawn_boss_alt" ) )
		{
			m_spawnSpotVector.AddToTail( spawnPoint );
		}
	}

	if ( m_spawnSpotVector.Count() == 0 )
	{
		Warning( "No info_target entities named 'spawn_boss_alt' found!\n" );
	}

	// show Boss' health meter on HUD
	if ( IsSpell() )
	{
		// this will force particle effect on the boss
		m_attitude = GetTeamNumber() == TF_TEAM_RED ? EYEBALL_ANGRY : EYEBALL_CALM;
	}
	else
	{
		if ( g_pMonsterResource )
		{
			g_pMonsterResource->SetBossHealthPercentage( 1.0f );
		}

		m_attitude = EYEBALL_CALM;
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "recalculate_truce" );
	if ( event )
	{
		gameeventmanager->FireEvent( event, true );
	}
}


//---------------------------------------------------------------------------------------------
void CEyeballBoss::Update( void )
{
	BaseClass::Update();

	m_attitude = EYEBALL_CALM;

	if ( IsEnraged() )
	{
		if ( IsSpell() )
		{
			m_attitude = GetTeamNumber() == TF_TEAM_RED ? EYEBALL_ANGRY : EYEBALL_CALM;
			m_nSkin = GetTeamNumber() == TF_TEAM_RED ? EYEBALL_TEAM_RED : EYEBALL_TEAM_BLUE;
		}
		else
		{
			m_nSkin = EYEBALL_RED_SKIN;
		}

		int angryPoseParameter = LookupPoseParameter( "anger" );
		if ( angryPoseParameter >= 0 )
		{
			SetPoseParameter( angryPoseParameter, 1 );
		}
	}
	else if ( IsGrumpy() )
	{
		m_nSkin = EYEBALL_NORMAL_SKIN;

		int angryPoseParameter = LookupPoseParameter( "anger" );
		if ( angryPoseParameter >= 0 )
		{
			SetPoseParameter( angryPoseParameter, 0.4f );
		}
	}
	else
	{
		m_nSkin = EYEBALL_NORMAL_SKIN;

		int angryPoseParameter = LookupPoseParameter( "anger" );
		if ( angryPoseParameter >= 0 )
		{
			SetPoseParameter( angryPoseParameter, 0 );
		}
	}
}


//---------------------------------------------------------------------------------------------
void CEyeballBoss::UpdateOnRemove( void )
{
	// In regular TF gameplay, g_pMonsterResource should always be non-null. The null check helps some server plugins though.
	Assert( g_pMonsterResource != NULL );
	if ( g_pMonsterResource )
	{
		g_pMonsterResource->HideBossHealthMeter();
	}

	BaseClass::UpdateOnRemove();
}


//---------------------------------------------------------------------------------------------
void CEyeballBoss::JarateNearbyPlayers( float range )
{
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
	CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

	for( int i=0; i<playerVector.Count(); ++i )
	{
		if ( IsRangeLessThan( playerVector[i], range ) && 
			IsLineOfSightClear( playerVector[i], CBaseCombatCharacter::IGNORE_ACTORS ) )
		{
			playerVector[i]->m_Shared.AddCond( TF_COND_URINE, 10.0f );
		}
	}
}


//---------------------------------------------------------------------------------------------
float EyeballBossModifyDamage( const CTakeDamageInfo &info )
{
	CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >( info.GetWeapon() );
	CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( info.GetInflictor() );
	CTFProjectile_SentryRocket *sentryRocket = dynamic_cast< CTFProjectile_SentryRocket * >( info.GetInflictor() );

	if ( sentry || sentryRocket )
	{
		return info.GetDamage() * 0.25f;
	}
	else if ( pWeapon )
	{
		switch( pWeapon->GetWeaponID() )
		{
		case TF_WEAPON_FLAMETHROWER:
			return info.GetDamage() * 0.5f;

		case TF_WEAPON_MINIGUN:
			return info.GetDamage() * 0.25f;
		}
	}

	// unmodified
	return info.GetDamage();
}


//---------------------------------------------------------------------------------------------
int CEyeballBoss::OnTakeDamage_Alive( const CTakeDamageInfo &rawInfo )
{
	CTakeDamageInfo info = rawInfo;

	if ( IsSelf( info.GetAttacker() ) )
	{
		// don't injure myself
		return 0;
	}

	// have we reached our damage limit?
	if ( m_damageLimit == 0 )
	{
		return 0;
	}

	if ( IsSpell() )
	{
		return 0;
	}

	int beforeHealth = GetHealth();

	info.SetDamage( EyeballBossModifyDamage( info ) );

	int result = BaseClass::OnTakeDamage_Alive( info );

	// update boss health meter
	float healthPercentage = (float)GetHealth() / (float)GetMaxHealth();

	if ( g_pMonsterResource )
	{
		if ( healthPercentage <= 0.0f )
		{
			g_pMonsterResource->HideBossHealthMeter();
		}
		else
		{
			g_pMonsterResource->SetBossHealthPercentage( healthPercentage );
		}
	}

	// do we have a damage limit?
	if ( m_damageLimit >= 0 )
	{
		int actualDamage = beforeHealth - GetHealth();

		m_damageLimit -= actualDamage;

		if ( m_damageLimit < 0 )
		{
			m_damageLimit = 0;
		}
	}

	return result;
}


//---------------------------------------------------------------------------------------------
bool CEyeballBoss::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}



//-----------------------------------------------------------------------------
// Update our last known nav area directly underneath us (since we fly)
//-----------------------------------------------------------------------------
void CEyeballBoss::UpdateLastKnownArea( void )
{
	if ( TheNavMesh->IsGenerating() )
	{
		ClearLastKnownArea();
		return;
	}

	// find the area we are directly standing in
	CNavArea *area = TheNavMesh->GetNearestNavArea( this, GETNAVAREA_CHECK_LOS, 500.0f );
	if ( !area )
		return;

	// make sure we can actually use this area - if not, consider ourselves off the mesh
	if ( !IsAreaTraversable( area ) )
		return;

	if ( area != m_lastNavArea )
	{
		// player entered a new nav area
		if ( m_lastNavArea )
		{
			m_lastNavArea->DecrementPlayerCount( m_registeredNavTeam, entindex() );
			m_lastNavArea->OnExit( this, area );
		}

		m_registeredNavTeam = GetTeamNumber();
		area->IncrementPlayerCount( m_registeredNavTeam, entindex() );
		area->OnEnter( this, m_lastNavArea );

		OnNavAreaChanged( area, m_lastNavArea );

		m_lastNavArea = area;
	}
}


//---------------------------------------------------------------------------------------------
CBaseCombatCharacter *CEyeballBoss::GetVictim( void ) const
{
	if ( m_victim == NULL )
		return NULL;

	if ( !m_victim->IsAlive() )
		return NULL;

	if ( IsInPurgatory( m_victim ) )
		return NULL;

	return m_victim;
}


//---------------------------------------------------------------------------------------------
CBaseCombatCharacter *CEyeballBoss::FindClosestVisibleVictim( void )
{
	CBaseCombatCharacter *victim = NULL;
	float victimRangeSq = FLT_MAX;

	CUtlVector< CTFPlayer * > playerVector;
	int nTargetTeam = TEAM_ANY;
	if ( IsSpell() )
	{
		nTargetTeam = GetTeamNumber() == TF_TEAM_RED ? TF_TEAM_BLUE : TF_TEAM_RED;

		for ( int i=0; i<TFGameRules()->GetBossCount(); ++i )
		{
			CBaseCombatCharacter *pBoss = TFGameRules()->GetActiveBoss( i );
			if ( pBoss && !IsSelf( pBoss ) && pBoss->GetTeamNumber() != GetTeamNumber() )
			{
				float rangeSq = ( pBoss->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
				if ( rangeSq < victimRangeSq )
				{
					if ( IsLineOfSightClear( pBoss ) )
					{
						victim = pBoss;
						victimRangeSq = rangeSq;
					}
				}
			}
		}
	}
	CollectPlayers( &playerVector, nTargetTeam, COLLECT_ONLY_LIVING_PLAYERS );

	for( int i=0; i<playerVector.Count(); ++i )
	{
		CTFPlayer *player = playerVector[i];

		if ( IsInPurgatory( player ) )
			continue;

		if ( player->m_Shared.IsStealthed() )
		{
			if ( !player->m_Shared.InCond( TF_COND_BURNING ) &&
				 !player->m_Shared.InCond( TF_COND_URINE ) &&
				 !player->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) &&
				 !player->m_Shared.InCond( TF_COND_BLEEDING ) )
			{
				// cloaked spies are invisible to us
				continue;
			}
		}

		// ignore player who disguises as my team
		if ( player->m_Shared.InCond( TF_COND_DISGUISED ) && player->m_Shared.GetDisguiseTeam() == GetTeamNumber() )
		{
			continue;
		}

		// ignore ghost players
		if ( player->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
		{
			continue;
		}

		float rangeSq = ( player->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
		if ( rangeSq < victimRangeSq )
		{
			if ( IsLineOfSightClear( player ) )
			{
				victim = player;
				victimRangeSq = rangeSq;
			}
		}
	}

	for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject* pObj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[i] );
		if ( pObj->GetTeamNumber() == GetTeamNumber() )
		{
			continue;
		}

		if ( pObj->ObjectType() == OBJ_SENTRYGUN )
		{
			float rangeSq = ( pObj->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
			if ( rangeSq < victimRangeSq )
			{
				if ( IsLineOfSightClear( pObj ) )
				{
					victim = pObj;
					victimRangeSq = rangeSq;
				}
			}
		}
	}

	// find closest zombie
	for ( int i=0; i<IZombieAutoList::AutoList().Count(); ++i )
	{
		CZombie* pZombie = static_cast< CZombie* >( IZombieAutoList::AutoList()[i] );
		if ( pZombie->GetTeamNumber() == GetTeamNumber() )
		{
			continue;
		}

		float rangeSq = GetRangeSquaredTo( pZombie );
		if ( rangeSq < victimRangeSq )
		{
			if ( IsLineOfSightClear( pZombie ) )
			{
				victim = pZombie;
				victimRangeSq = rangeSq;
			}
		}
	}

	return victim;
}


//---------------------------------------------------------------------------------------------
const Vector &CEyeballBoss::PickNewSpawnSpot( void ) const
{
	static Vector spot;

	if ( m_spawnSpotVector.Count() == 0 )
	{
		spot = GetAbsOrigin();
	}
	else
	{
		spot = m_spawnSpotVector[ RandomInt( 0, m_spawnSpotVector.Count()-1 ) ]->GetAbsOrigin();
	}

	return spot;
}


//---------------------------------------------------------------------------------------------
void CEyeballBoss::BecomeEnraged( float duration )
{
	if ( !IsEnraged() )
	{
		EmitSound( "Halloween.EyeballBossBecomeEnraged" );
	}

	m_rageTimer.Start( duration );
}


//---------------------------------------------------------------------------------------------
void CEyeballBoss::LogPlayerInteraction( const char *verb, CTFPlayer *player )
{
	if ( !player || !verb )
		return;

	if ( !player->GetTeam() )
		return;

	CTFWeaponBase *weapon = player->GetActiveTFWeapon();
	const char *weaponLogName = NULL;

	if ( weapon )
	{
		weaponLogName = WeaponIdToAlias( weapon->GetWeaponID() );

		CEconItemView *pItem = weapon->GetAttributeContainer()->GetItem();

		if ( pItem && pItem->GetStaticData() )
		{
			if ( pItem->GetStaticData()->GetLogClassname() )
			{
				weaponLogName = pItem->GetStaticData()->GetLogClassname();
			}
		}
	}

	UTIL_LogPrintf( "HALLOWEEN: \"%s<%i><%s><%s>\" %s with \"%s\" (attacker_position \"%d %d %d\")\n",  
					player->GetPlayerName(),
					player->GetUserID(),
					player->GetNetworkIDString(),
					player->GetTeam()->GetName(),
					verb,
					weaponLogName ? weaponLogName : "NoWeapon",
					(int)player->GetAbsOrigin().x, 
					(int)player->GetAbsOrigin().y,
					(int)player->GetAbsOrigin().z );
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
IMPLEMENT_INTENTION_INTERFACE( CEyeballBoss, CEyeballBossBehavior );


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
CEyeballBossLocomotion::CEyeballBossLocomotion( INextBot *bot ) : ILocomotion( bot )
{
	Reset();
}


//---------------------------------------------------------------------------------------------
CEyeballBossLocomotion::~CEyeballBossLocomotion()
{
}


//---------------------------------------------------------------------------------------------
// (EXTEND) reset to initial state
void CEyeballBossLocomotion::Reset( void )
{
	m_velocity = vec3_origin;
	m_acceleration = vec3_origin;
	m_desiredSpeed = 0.0f;
	m_currentSpeed = 0.0f;
	m_forward = vec3_origin;
	m_desiredAltitude = tf_eyeball_boss_hover_height.GetFloat();
}


#ifdef LOW_FLOAT_BUT_HANGS_UP_ON_LEDGES
//---------------------------------------------------------------------------------------------
void CEyeballBossLocomotion::MaintainAltitude( void )
{
	CBaseCombatCharacter *me = GetBot()->GetEntity();

	trace_t result;
	CTraceFilterSimpleClassnameList filter( me, COLLISION_GROUP_NONE );
	filter.AddClassnameToIgnore( "eyeball_boss" );

	UTIL_TraceLine( me->GetAbsOrigin(), me->GetAbsOrigin() + Vector( 0, 0, -2000.0f ), MASK_PLAYERSOLID_BRUSHONLY, &filter, &result );

	float groundZ = result.endpos.z;

	float currentAltitude = me->GetAbsOrigin().z - groundZ;

	float desiredAltitude = GetDesiredAltitude();

	float error = desiredAltitude - currentAltitude;

	float accelZ = clamp( error, -tf_eyeball_boss_acceleration.GetFloat(), tf_eyeball_boss_acceleration.GetFloat() );

	m_acceleration.z += accelZ;
}
#endif

#define HI_FLOATING
#ifdef HI_FLOATING
//---------------------------------------------------------------------------------------------
void CEyeballBossLocomotion::MaintainAltitude( void )
{
	CBaseCombatCharacter *me = GetBot()->GetEntity();

	if ( !me->IsAlive() )
	{
		m_acceleration.x = 0.0f;
		m_acceleration.y = 0.0f;
		m_acceleration.z = -300.0f;

		return;
	}

	trace_t result;
	CTraceFilterSimpleClassnameList filter( me, COLLISION_GROUP_NONE );
	filter.AddClassnameToIgnore( "eyeball_boss" );

	// find ceiling
	TraceHull( me->GetAbsOrigin(), me->GetAbsOrigin() + Vector( 0, 0, 1000.0f ), 
			   me->WorldAlignMins(), me->WorldAlignMaxs(), 
			   GetBot()->GetBodyInterface()->GetSolidMask(), &filter, &result );

	float ceiling = result.endpos.z - me->GetAbsOrigin().z;

	Vector aheadXY;
	
	if ( IsAttemptingToMove() )
	{
		aheadXY.x = m_forward.x;
		aheadXY.y = m_forward.y;
		aheadXY.z = 0.0f;
		aheadXY.NormalizeInPlace();
	}
	else
	{
		aheadXY = vec3_origin;
	}

	TraceHull( me->GetAbsOrigin() + Vector( 0, 0, ceiling ) + aheadXY * 50.0f,
			   me->GetAbsOrigin() + Vector( 0, 0, -2000.0f ) + aheadXY * 50.0f,
			   Vector( 1.25f * me->WorldAlignMins().x, 1.25f * me->WorldAlignMins().y, me->WorldAlignMins().z ), 
			   Vector( 1.25f * me->WorldAlignMaxs().x, 1.25f * me->WorldAlignMaxs().y, me->WorldAlignMaxs().z ), 
			   GetBot()->GetBodyInterface()->GetSolidMask(), &filter, &result );

	float groundZ = result.endpos.z;

	float currentAltitude = me->GetAbsOrigin().z - groundZ;

	float desiredAltitude = GetDesiredAltitude();

	float error = desiredAltitude - currentAltitude;

	float accelZ = clamp( error, -tf_eyeball_boss_acceleration.GetFloat(), tf_eyeball_boss_acceleration.GetFloat() );

	m_acceleration.z += accelZ;
}
#endif

//---------------------------------------------------------------------------------------------
// (EXTEND) update internal state
void CEyeballBossLocomotion::Update( void )
{
	CBaseCombatCharacter *me = GetBot()->GetEntity();
	const float deltaT = GetUpdateInterval();

	Vector pos = me->GetAbsOrigin();

	// always maintain altitude, even if not trying to move (ie: no Approach call)
	MaintainAltitude();

	m_forward = m_velocity;
	m_currentSpeed = m_forward.NormalizeInPlace();

	Vector damping( tf_eyeball_boss_horiz_damping.GetFloat(), tf_eyeball_boss_horiz_damping.GetFloat(), tf_eyeball_boss_vert_damping.GetFloat() );
	Vector totalAccel = m_acceleration - m_velocity * damping;

	m_velocity += totalAccel * deltaT;
	me->SetAbsVelocity( m_velocity );

	pos += m_velocity * deltaT;

	// check for collisions along move	
	trace_t result;
	CTraceFilterSkipClassname filter( me, "eyeball_boss", COLLISION_GROUP_NONE );
	Vector from = me->GetAbsOrigin();
	Vector to = pos;
	Vector desiredGoal = to;
	Vector resolvedGoal;
	int recursionLimit = 3;

	int hitCount = 0;
	Vector surfaceNormal = vec3_origin;

	bool didHitWorld = false;

	while( true )
	{
		TraceHull( from, desiredGoal, me->WorldAlignMins(), me->WorldAlignMaxs(), GetBot()->GetBodyInterface()->GetSolidMask(), &filter, &result );

		if ( !result.DidHit() )
		{
			resolvedGoal = pos;
			break;
		}

		if ( result.DidHitWorld() )
		{
			didHitWorld = true;
		}

		++hitCount;
		surfaceNormal += result.plane.normal;

		// If we hit really close to our target, then stop
		if ( !result.startsolid && desiredGoal.DistToSqr( result.endpos ) < 1.0f )
		{
			resolvedGoal = result.endpos;
			break;
		}

		if ( result.startsolid )
		{
			// stuck inside solid; don't move
			resolvedGoal = me->GetAbsOrigin();
			break;
		}

		if ( --recursionLimit <= 0 )
		{
			// reached recursion limit, no more adjusting allowed
			resolvedGoal = result.endpos;
			break;
		}

		// slide off of surface we hit
		Vector fullMove = desiredGoal - from;
		Vector leftToMove = fullMove * ( 1.0f - result.fraction );

		float blocked = DotProduct( result.plane.normal, leftToMove );

		Vector unconstrained = fullMove - blocked * result.plane.normal;

		// check for collisions along remainder of move
		// But don't bother if we're not going to deflect much
		Vector remainingMove = from + unconstrained;
		if ( remainingMove.DistToSqr( result.endpos ) < 1.0f )
		{
			resolvedGoal = result.endpos;
			break;
		}

		desiredGoal = remainingMove;
	}

	if ( hitCount > 0 )
	{
		surfaceNormal.NormalizeInPlace();

		// bounce
		m_velocity = m_velocity - 2.0f * DotProduct( m_velocity, surfaceNormal ) * surfaceNormal;

		if ( didHitWorld )
		{
			//me->EmitSound( "Minion.Bounce" );
		}
	}

	GetBot()->GetEntity()->SetAbsOrigin( result.endpos );

	m_acceleration = vec3_origin;
}


//---------------------------------------------------------------------------------------------
// (EXTEND) move directly towards the given position
void CEyeballBossLocomotion::Approach( const Vector &goalPos, float goalWeight )
{
	Vector flyGoal = goalPos;
	flyGoal.z += m_desiredAltitude;

	Vector toGoal = flyGoal - GetBot()->GetEntity()->GetAbsOrigin();
	// altitude is handled in Update()
	toGoal.z = 0.0f;
	toGoal.NormalizeInPlace();

	m_acceleration += tf_eyeball_boss_acceleration.GetFloat() * toGoal;
}


//---------------------------------------------------------------------------------------------
void CEyeballBossLocomotion::SetDesiredSpeed( float speed )
{
	m_desiredSpeed = speed;
}


//---------------------------------------------------------------------------------------------
float CEyeballBossLocomotion::GetDesiredSpeed( void ) const
{
	return m_desiredSpeed;
}


//---------------------------------------------------------------------------------------------
void CEyeballBossLocomotion::SetDesiredAltitude( float height )
{
	m_desiredAltitude = height;
}


//---------------------------------------------------------------------------------------------
float CEyeballBossLocomotion::GetDesiredAltitude( void ) const
{
	return m_desiredAltitude;
}


//---------------------------------------------------------------------------------------------
// Face along path. Since we float, only face horizontally.
void CEyeballBossLocomotion::FaceTowards( const Vector &target )
{
	CBaseCombatCharacter *me = GetBot()->GetEntity();

	Vector toTarget = target - me->WorldSpaceCenter();
	toTarget.z = 0.0f;

	QAngle angles;
	VectorAngles( toTarget, angles );

	me->SetAbsAngles( angles );
}


//---------------------------------------------------------------------------------------------
// return position of "feet" - the driving point where the bot contacts the ground
// for this floating boss, "feet" refers to the ground directly underneath him
const Vector &CEyeballBossLocomotion::GetFeet( void ) const
{
	static Vector feet;
	CBaseCombatCharacter *me = GetBot()->GetEntity();

	trace_t result;
	CTraceFilterSimpleClassnameList filter( me, COLLISION_GROUP_NONE );
	filter.AddClassnameToIgnore( "eyeball_boss" );

	feet = me->GetAbsOrigin();

	UTIL_TraceLine( feet, feet + Vector( 0, 0, -2000.0f ), MASK_PLAYERSOLID_BRUSHONLY, &filter, &result );

	feet.z = result.endpos.z;

	return feet;
}



//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
CEyeballBossBody::CEyeballBossBody( INextBot *bot ) : CBotNPCBody( bot ) 
{
	m_leftRightPoseParameter = -1;
	m_upDownPoseParameter = -1;
	m_lookAtSpot = vec3_origin;
}


//---------------------------------------------------------------------------------------------
void CEyeballBossBody::Update( void )
{
	CBaseCombatCharacter *me = GetBot()->GetEntity();

	// track client-side rotation
	Vector myForward;
	me->GetVectors( &myForward, NULL, NULL );

	const float myApproachRate = 3.0f; // 1.0f;

	Vector toTarget = m_lookAtSpot - me->WorldSpaceCenter();
	toTarget.NormalizeInPlace();

	myForward += toTarget * myApproachRate * GetUpdateInterval();
	myForward.NormalizeInPlace();

	QAngle myNewAngles;
	VectorAngles( myForward, myNewAngles );

	me->SetAbsAngles( myNewAngles );

	if ( tf_eyeball_boss_debug.GetBool() )
	{
		NDebugOverlay::Line( me->WorldSpaceCenter(), me->WorldSpaceCenter() + 150.0f * myForward, 255, 255, 0, true, 0.1f );
	}

	// move the animation ahead in time	
	me->StudioFrameAdvance();
	me->DispatchAnimEvents( me );
}


//---------------------------------------------------------------------------------------------
// Aim the bot's head towards the given goal
void CEyeballBossBody::AimHeadTowards( const Vector &lookAtPos, LookAtPriorityType priority, float duration, INextBotReply *replyWhenAimed, const char *reason )
{
	CEyeballBoss *me = (CEyeballBoss *)GetBot()->GetEntity();

	m_lookAtSpot = lookAtPos;
	me->SetLookAtTarget( lookAtPos );
}


//---------------------------------------------------------------------------------------------
// Continually aim the bot's head towards the given subject
void CEyeballBossBody::AimHeadTowards( CBaseEntity *subject, LookAtPriorityType priority, float duration, INextBotReply *replyWhenAimed, const char *reason )
{
	CEyeballBoss *me = (CEyeballBoss *)GetBot()->GetEntity();

	me->SetLookAtTarget( subject->EyePosition() );

	if ( !subject )
		return;

	m_lookAtSpot = subject->EyePosition();
}
