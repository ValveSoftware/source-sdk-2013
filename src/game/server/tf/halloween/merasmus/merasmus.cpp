//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "nav_mesh/tf_nav_area.h"
#include "NextBot/Path/NextBotChasePath.h"
#include "econ_wearable.h"
#include "team_control_point_master.h"
#include "particle_parse.h"
#include "tf_weaponbase_merasmus_grenade.h"
#include "merasmus_dancer.h"
#include "tf_wheel_of_doom.h"
#include "soundenvelope.h"
#include "util.h"
#include "tf_obj_sentrygun.h"
#include "logicrelay.h"
#include "steamworks_gamestats.h"

#include "tf/halloween/ghost/ghost.h"

#include "player_vs_environment/monster_resource.h"

#include "merasmus_trick_or_treat_prop.h"
#include "merasmus.h"
#include "merasmus_behavior/merasmus_disguise.h"
#include "merasmus_behavior/merasmus_dying.h"
#include "merasmus_behavior/merasmus_reveal.h"
#include "merasmus_behavior/merasmus_teleport.h"

#include "rtime.h"
#include "gc_clientsystem.h"
#include "tf_gcmessages.h"

#include "tf_fx.h"

ConVar tf_merasmus_health_base( "tf_merasmus_health_base", "33750", FCVAR_CHEAT );
ConVar tf_merasmus_health_per_player( "tf_merasmus_health_per_player", "2500", FCVAR_CHEAT );
ConVar tf_merasmus_min_player_count( "tf_merasmus_min_player_count", "10", FCVAR_CHEAT );

ConVar tf_merasmus_lifetime( "tf_merasmus_lifetime", "120", FCVAR_CHEAT );

ConVar tf_merasmus_speed( "tf_merasmus_speed", "600", FCVAR_CHEAT );
ConVar tf_merasmus_speed_recovery_rate( "tf_merasmus_speed_recovery_rate", "100", FCVAR_CHEAT, "Movement units/second" );
ConVar tf_merasmus_chase_duration( "tf_merasmus_chase_duration", "7", FCVAR_CHEAT );
ConVar tf_merasmus_chase_range( "tf_merasmus_chase_range", "2000", FCVAR_CHEAT );

ConVar tf_merasmus_should_disguise_threshold( "tf_merasmus_should_disguise_threshold", "0.45f", FCVAR_CHEAT );
ConVar tf_merasmus_min_props_to_reveal( "tf_merasmus_min_props_to_reveal", "0.7f", FCVAR_CHEAT, "Percentage of total fake props players have to destroy before Merasmus reveals himself");

ConVar tf_merasmus_attack_range( "tf_merasmus_attack_range", "200", FCVAR_CHEAT );

ConVar tf_merasmus_health_regen_rate( "tf_merasmus_health_regen_rate", "0.001f", FCVAR_CHEAT, "Percentage of Max HP per sec that Merasmus will regenerate while in disguise" );

ConVar tf_merasmus_bomb_head_duration( "tf_merasmus_bomb_head_duration", "15.f", FCVAR_CHEAT );
ConVar tf_merasmus_bomb_head_per_team( "tf_merasmus_bomb_head_per_team", "1", FCVAR_CHEAT );

ConVar tf_merasmus_stun_duration( "tf_merasmus_stun_duration", "2.f", FCVAR_CHEAT );

extern ConVar tf_merasmus_spawn_interval;
extern ConVar tf_merasmus_spawn_interval_variation;

#define MERASMUS_MODEL_NAME			"models/bots/merasmus/merasmus.mdl"
#define MERASMUS_BOMB_MODEL			"models/props_lakeside_event/bomb_temp.mdl"

static const char* s_pszDisguiseProps[] =
{
	// "models/props_hydro/dumptruck.mdl",				// 265
	"models/props_halloween/pumpkin_02.mdl",
	"models/props_halloween/pumpkin_03.mdl",
	"models/egypt/palm_tree/palm_tree.mdl",
	"models/props_spytech/control_room_console01.mdl",
	"models/props_spytech/work_table001.mdl",
	// "models/egypt/tent/tent.mdl",					// 248
	"models/props_coalmines/boulder1.mdl",
	"models/props_coalmines/boulder2.mdl",
	"models/props_farm/concrete_block001.mdl",			// 152
	// "models/props_farm/tractor_tire001.mdl",			// requires offset
	"models/props_farm/welding_machine01.mdl",
	"models/props_medieval/medieval_resupply.mdl",
	"models/props_medieval/target/target.mdl",
	"models/props_swamp/picnic_table.mdl",
	"models/props_manor/baby_grand_01.mdl",				// 154
	"models/props_manor/bookcase_132_02.mdl",
	"models/props_manor/chair_01.mdl",
	"models/props_manor/couch_01.mdl",
	"models/props_manor/grandfather_clock_01.mdl",
	// "models/props_manor/tractor_01.mdl",				// 227
	// "models/props_gameplay/haybale.mdl",				// requires offset
	"models/props_viaduct_event/coffin_simple_closed.mdl",
	// "models/props_farm/wooden_barrel.mdl",			// requires offset
	"models/props_2fort/miningcrate001.mdl",
	"models/props_gameplay/resupply_locker.mdl",
	"models/props_2fort/oildrum.mdl",
	// "models/props_farm/wood_pile.mdl",				// requires offset
	"models/props_lakeside/wood_crate_01.mdl",
	// "models/props_farm/pallet001.mdl",				// requires offset
	"models/props_well/hand_truck01.mdl",
	"models/props_vehicles/mining_car_metal.mdl",
	"models/props_2fort/tire002.mdl",
	"models/props_well/computer_cart01.mdl",
	"models/egypt/palm_tree/palm_tree.mdl"
};


//-----------------------------------------------------------------------------------------------------
// The Horseless Headless Horseman
//-----------------------------------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( merasmus, CMerasmus );

IMPLEMENT_SERVERCLASS_ST( CMerasmus, DT_Merasmus )
	SendPropBool( SENDINFO( m_bRevealed ) ),
	SendPropBool( SENDINFO( m_bIsDoingAOEAttack ) ),
	SendPropBool( SENDINFO( m_bStunned ) ),
END_SEND_TABLE()


int CMerasmus::m_level = 1;

IMPLEMENT_AUTO_LIST( IMerasmusAutoList );

//-----------------------------------------------------------------------------------------------------
CMerasmus::CMerasmus()
{
	m_intention = new CMerasmusIntention( this );
	m_locomotor = new CMerasmusLocomotion( this );
	m_flyingLocomotor = new CMerasmusFlyingLocomotion( this );
	m_body = new CMerasmusBody( this );
	m_bRevealed = false;
	m_bIsDoingAOEAttack = false;

	m_wheel = NULL;

	m_stunTimer.Invalidate();
	m_bStunned = false;

	m_nBombHitCount = 0;

	m_hMerasmusRevealer = NULL;

	m_isFlying = false;
	m_isHiding=false;

	m_hHealthBar = g_pMonsterResource;
	ListenForGameEvent( "player_death" );
}


//-----------------------------------------------------------------------------------------------------
CMerasmus::~CMerasmus()
{
	if ( m_intention )
		delete m_intention;

	if ( m_locomotor )
		delete m_locomotor;

	if ( m_flyingLocomotor )
		delete m_flyingLocomotor;

	if ( m_body )
		delete m_body;

	// Make sure the health meter goes away
	if( m_hHealthBar.Get() )
	{
		m_hHealthBar->HideBossHealthMeter();
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "recalculate_truce" );
	if ( event )
	{
		gameeventmanager->FireEvent( event, true );
	}
}

//-----------------------------------------------------------------------------------------------------
void CMerasmus::Precache()
{
	BaseClass::Precache();

	// always allow late precaching, so we don't pay the cost of the
	// Halloween Boss for the entire year

	bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	PrecacheMerasmus();

	CBaseEntity::SetAllowPrecache( bAllowPrecache );
}


void CMerasmus::PrecacheMerasmus()
{
	int model = PrecacheModel( MERASMUS_MODEL_NAME );
	PrecacheGibsForModel( model );

	// precache disguise prop list
	for ( int i=0; i<ARRAYSIZE( s_pszDisguiseProps ); ++i )
	{
		PrecacheModel( s_pszDisguiseProps[i] );
	}

	PrecacheModel( MERASMUS_BOMB_MODEL );
	PrecacheModel( "models/props_lakeside_event/bomb_temp_hat.mdl" ); // bomb head on player
	PrecacheModel( "models/props_halloween/bombonomicon.mdl" ); // bombonomicon hint to player

	// Boss VOs
	PrecacheScriptSound( "Halloween.MerasmusAppears" );
	PrecacheScriptSound( "Halloween.MerasmusBanish" );
	//PrecacheScriptSound( "Halloween.MerasmusCastBleedingSpell" );
	PrecacheScriptSound( "Halloween.MerasmusCastFireSpell" );
	PrecacheScriptSound( "Halloween.MerasmusCastJarateSpell" );
	PrecacheScriptSound( "Halloween.MerasmusCastJarateSpellRare" );
	PrecacheScriptSound( "Halloween.MerasmusLaunchSpell" );
	PrecacheScriptSound( "Halloween.MerasmusControlPoint" );
	PrecacheScriptSound( "Halloween.MerasmusDepart" );
	PrecacheScriptSound( "Halloween.MerasmusDepartRare" );
	PrecacheScriptSound( "Halloween.MerasmusDiscovered" );
	PrecacheScriptSound( "Halloween.MerasmusGrenadeThrow" );
	PrecacheScriptSound( "Halloween.MerasmusHidden" );
	PrecacheScriptSound( "Halloween.MerasmusHiddenRare" );
	PrecacheScriptSound( "Halloween.MerasmusHitByBomb" );
	PrecacheScriptSound( "Halloween.MerasmusHitByBombRare" );
	PrecacheScriptSound( "Halloween.MerasmusInitiateHiding" );
	PrecacheScriptSound( "Halloween.MerasmusStaffAttack" );
	PrecacheScriptSound( "Halloween.MerasmusStaffAttackRare" );
	PrecacheScriptSound( "Halloween.MerasmusTauntFakeProp" );

	//PrecacheScriptSound( "Halloween.MerasmusTeleport" );

	// Boss event sound effects
	PrecacheScriptSound( "Halloween.MerasmusBossSpawn" );
	PrecacheScriptSound( "Halloween.Merasmus_Death" );
	PrecacheScriptSound( "Halloween.EyeballBossEscapeSoon" );
	PrecacheScriptSound( "Halloween.EyeballBossEscapeImminent" );
	PrecacheScriptSound( "Halloween.EyeballBossEscaped" );
	PrecacheScriptSound( "Halloween.Merasmus_Float" );
	PrecacheScriptSound( "Halloween.Merasmus_Stun" );
	PrecacheScriptSound( "Halloween.Merasmus_Spell" );
	PrecacheScriptSound( "Halloween.Merasmus_Hiding_Explode" );

	PrecacheParticleSystem( "merasmus_spawn" ); // spawn
	PrecacheParticleSystem( "merasmus_tp" ); // puff effect
	PrecacheParticleSystem( "merasmus_blood" ); // when he takes damage
	PrecacheParticleSystem( "merasmus_blood_bits" ); // when he takes damage while stunned
	PrecacheParticleSystem( "merasmus_ambient_body" ); // glow around the body
	PrecacheParticleSystem( "merasmus_shoot" ); // when he casts spell
	PrecacheParticleSystem( "merasmus_book_attack" ); // big attack
	PrecacheParticleSystem( "merasmus_object_spawn" ); // object spawn
	PrecacheParticleSystem( "merasmus_zap" ); // zap!
	PrecacheParticleSystem( "merasmus_dazed" ); // stunned
	PrecacheParticleSystem( "merasmus_dazed_explosion" ); // bomb head explode

	// TEMP
	PrecacheScriptSound( "Halloween.HeadlessBossAxeHitFlesh" );
}


//-----------------------------------------------------------------------------------------------------
void CMerasmus::Spawn( void )
{
	Precache();

	SetModel( MERASMUS_MODEL_NAME );

	BaseClass::Spawn();

	PlayHighPrioritySound("Halloween.MerasmusAppears");

	m_isHiding=false;

	// scale the boss' health with the player count
	int totalPlayers = GetGlobalTFTeam( TF_TEAM_BLUE )->GetNumPlayers() + GetGlobalTFTeam( TF_TEAM_RED )->GetNumPlayers();

	int health = tf_merasmus_health_base.GetInt();
	if ( totalPlayers > tf_merasmus_min_player_count.GetInt() )
	{
		health += ( totalPlayers - tf_merasmus_min_player_count.GetInt() ) * tf_merasmus_health_per_player.GetInt();
	}

	CBaseEntity *pWheel = gEntList.FindEntityByName( NULL, "wheel_of_fortress" );
	if ( pWheel )
	{
		m_wheel = assert_cast< CWheelOfDoom* >( pWheel );
	}

	SetHealth( health );
	SetMaxHealth( health );

	m_homePos = GetAbsOrigin();

	m_damagePoseParameter = -1;

	SetBloodColor( DONT_BLEED );

	if ( m_pIdleSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pIdleSound );
		m_pIdleSound = NULL;
	}

	// Collect all of the players that are alive at this moment.  These are the players who will get
	// their hat leveled up when Merasmus dies.  We only want to give credit to these players to discourage
	// the strategy of spawning Merasmus with 10 people so his health is scaled for 10 people,
	// then have 22 other people connect and destroy him.
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector );
	FOR_EACH_VEC( playerVector, i )
	{
		m_startingAttackersVector.AddToTail( playerVector[i] );
	}

	CPVSFilter filter( GetAbsOrigin() );
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	m_pIdleSound = controller.SoundCreate( filter, entindex(), "Halloween.Merasmus_Float" );
	controller.Play( m_pIdleSound, 1.0, 100 );

	m_solidType = GetSolid();
	m_solidFlags = GetSolidFlags();

	const float flLifeTime = tf_merasmus_lifetime.GetFloat();
	m_lifeTimer.Start( flLifeTime );
	m_flLastWarnTime = 0;

	IGameEvent *event = gameeventmanager->CreateEvent( "merasmus_summoned" );
	if ( event )
	{
		event->SetInt( "level", GetLevel() );
		gameeventmanager->FireEvent( event );
	}
	TriggerLogicRelay( "boss_enter_relay", true );

	DispatchParticleEffect( "merasmus_spawn", GetAbsOrigin(), GetAbsAngles() );

	m_bossStats.ResetStats();

	event = gameeventmanager->CreateEvent( "recalculate_truce" );
	if ( event )
	{
		gameeventmanager->FireEvent( event, true );
	}
}


//---------------------------------------------------------------------------------------------
void RemoveAllBombHeadFromPlayers()
{
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
	CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

	for ( int i=0; i<playerVector.Count(); ++i )
	{
		if ( playerVector[i]->m_Shared.InCond( TF_COND_HALLOWEEN_BOMB_HEAD ) )
		{
			playerVector[i]->m_Shared.RemoveCond( TF_COND_HALLOWEEN_BOMB_HEAD );
		}
	}
}


void CMerasmus::UpdateOnRemove()
{
	Assert( TFGameRules() );
	if ( m_hHealthBar )
	{
		m_hHealthBar->HideBossHealthMeter();
	}

	if ( m_pIdleSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pIdleSound );
		m_pIdleSound = NULL;
	}

	// the boss is NULL, remove bomb head condition won't give players power play
	RemoveAllBombHeadFromPlayers();

	RemoveAllFakeProps();

	BaseClass::UpdateOnRemove();

	// Report Stats
	SW_ReportMerasmusStats();
}


//---------------------------------------------------------------------------------------------
float MerasmusModifyDamage( const CTakeDamageInfo &info )
{
	CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >( info.GetWeapon() );
	CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( info.GetInflictor() );
	CTFProjectile_SentryRocket *sentryRocket = dynamic_cast< CTFProjectile_SentryRocket * >( info.GetInflictor() );

	if ( sentry || sentryRocket )
	{
		return info.GetDamage() * 0.5f;
	}
	else if ( pWeapon )
	{
		switch( pWeapon->GetWeaponID() )
		{
		case TF_WEAPON_MINIGUN:
			return info.GetDamage() * 0.5f;

		case TF_WEAPON_SODA_POPPER:
			return info.GetDamage() * 1.5f;

		case TF_WEAPON_HANDGUN_SCOUT_PRIMARY:
			return info.GetDamage() * 1.75;

		case TF_WEAPON_SCATTERGUN:
		case TF_WEAPON_REVOLVER:
			return info.GetDamage() * 2.f;

		case TF_WEAPON_SNIPERRIFLE:
		case TF_WEAPON_SNIPERRIFLE_DECAP:
		case TF_WEAPON_SNIPERRIFLE_CLASSIC:
		case TF_WEAPON_COMPOUND_BOW:
		case TF_WEAPON_KNIFE:
		case TF_WEAPON_PEP_BRAWLER_BLASTER:
			return info.GetDamage() * 3.f;
		}
	}

	// unmodified
	return info.GetDamage();
}


//-----------------------------------------------------------------------------------------------------
int CMerasmus::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if ( !IsRevealed() )
	{
		return 0;
	}

	if ( IsSelf( info.GetAttacker() ) )
	{
		// don't injure myself
		return 0;
	}

	CTakeDamageInfo modifiedInfo = info;

	if ( RandomInt( 0, 30 ) == 0 )
	{
		//EmitSound( "Halloween.MerasmusHurt" ); << add new sound entry
	}

	if ( IsStunned() )
	{
		DispatchParticleEffect( "merasmus_blood", modifiedInfo.GetDamagePosition(), GetAbsAngles() );
	}
	else
	{
		DispatchParticleEffect( "merasmus_blood_bits", modifiedInfo.GetDamagePosition(), GetAbsAngles() );
	}

	modifiedInfo.SetDamage( MerasmusModifyDamage( modifiedInfo ) );
	if ( m_bIsDoingAOEAttack || m_bStunned )
	{
		modifiedInfo.AddDamageType( DMG_CRITICAL );
	}

	int result = BaseClass::OnTakeDamage_Alive( modifiedInfo );

	// update boss health meter
	float healthPercentage = (float)GetHealth() / (float)GetMaxHealth();

	if ( m_hHealthBar )
	{
		if ( healthPercentage <= 0.0f )
		{
			m_hHealthBar->HideBossHealthMeter();
		}
		else
		{
			m_hHealthBar->SetBossHealthPercentage( healthPercentage );
		}
	}

	// Stats Tracking
	CTFPlayer *pAttacker = ToTFPlayer( info.GetAttacker() );
	if ( pAttacker )
	{
		int iClass = pAttacker->GetPlayerClass()->GetClassIndex();
		if ( iClass > TF_CLASS_UNDEFINED && iClass < TF_LAST_NORMAL_CLASS )
		{
			m_bossStats.m_arrClassDamage[ iClass ] += info.GetDamage();
		}
	}
	return result;
}

//---------------------------------------------------------------------------------------------
void CMerasmus::FireGameEvent( IGameEvent *event)
{
	if ( !V_strcmp( event->GetName(), "player_death" ) )
	{
		// Collect Data
		int nDmgType = event->GetInt( "customkill", -1 );
		if ( nDmgType == TF_DMG_CUSTOM_MERASMUS_DECAPITATION || nDmgType == TF_DMG_CUSTOM_MERASMUS_ZAP )
		{
			m_bossStats.m_nStaffKills++;
		}
		else if ( nDmgType == TF_DMG_CUSTOM_MERASMUS_GRENADE  || nDmgType == TF_DMG_CUSTOM_MERASMUS_PLAYER_BOMB )
		{
			m_bossStats.m_nBombKills++;
		}
		else
		{
			// Treat as a PvPKill
			m_bossStats.m_nPvpKills++;
		}
	}
}

//---------------------------------------------------------------------------------------------
void CMerasmus::Update( void )
{
	BaseClass::Update();

	if ( m_damagePoseParameter < 0 )
	{
		m_damagePoseParameter = LookupPoseParameter( "damage" );
	}

	if ( m_damagePoseParameter >= 0 )
	{
		SetPoseParameter( m_damagePoseParameter, 1.0f - ( (float)GetHealth() / (float)GetMaxHealth() ) );
	}
}


Vector CMerasmus::GetCastPosition() const
{
	Vector vForward, vRight, vUp;
	AngleVectors( GetAbsAngles(), &vForward, &vRight, &vUp );
	return WorldSpaceCenter() + vForward * 60.f + 30.f * vRight + 60.f * vUp;
}


void RemoveAllGrenades( CMerasmus *me )
{
	const int maxCollectedEntities = 1024;
	CBaseEntity	*pObjects[ maxCollectedEntities ];
	int count = UTIL_EntitiesInSphere( pObjects, maxCollectedEntities, me->GetAbsOrigin(), 400, FL_GRENADE );

	for( int i = 0; i < count; ++i )
	{
		if ( pObjects[i] == NULL )
			continue;

		if ( pObjects[i]->IsPlayer() )
			continue;

		// Remove the enemy pipe 
		pObjects[i]->SetThink( &CBaseEntity::SUB_Remove );
		pObjects[i]->SetNextThink( gpGlobals->curtime );
		pObjects[i]->SetTouch( NULL );
		pObjects[i]->AddEffects( EF_NODRAW );
	}
}

void CMerasmus::OnRevealed(bool bPlaySound)
{
	RecordDisguiseTime();
	m_bRevealed = true;
	m_isHiding = false;
	m_nRevealedHealth = GetHealth();

	if (bPlaySound)
	{
		PlayHighPrioritySound( "Halloween.MerasmusDiscovered" );
	}

	RemoveEffects( EF_NOINTERP | EF_NODRAW );

	DispatchParticleEffect( "merasmus_spawn", WorldSpaceCenter(), GetAbsAngles() );

	// don't collide with anything, we do our own collision detection in our think method
	SetSolid( m_solidType );
	SetSolidFlags( m_solidFlags );

	RemoveAllFakeProps();
	TFGameRules()->PushAllPlayersAway( GetAbsOrigin(), 400.f, 500.f, TF_TEAM_RED );
	TFGameRules()->PushAllPlayersAway( GetAbsOrigin(), 400.f, 500.f, TF_TEAM_BLUE );
	RemoveAllGrenades( this );

	// give player who found merasmus buff
	if ( m_hMerasmusRevealer )
	{
		// condition was removed by blowing up merasmus, you get buff award
		const float buffDuration = 10.0f;
		m_hMerasmusRevealer->m_Shared.AddCond( TF_COND_CRITBOOSTED_PUMPKIN, buffDuration );
		m_hMerasmusRevealer->m_Shared.AddCond( TF_COND_SPEED_BOOST, buffDuration );
		m_hMerasmusRevealer->m_Shared.AddCond( TF_COND_INVULNERABLE, buffDuration );
	}
	m_hMerasmusRevealer = NULL;

	// show Boss' health meter on HUD
	if ( m_hHealthBar )
	{
		float healthPercentage = (float)GetHealth() / (float)GetMaxHealth();
		m_hHealthBar->SetBossHealthPercentage( healthPercentage );
	}

	// face towards a nearby player
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
	CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

	float closeRangeSq = FLT_MAX;
	CTFPlayer *close = NULL;

	for( int i=0; i<playerVector.Count(); ++i )
	{
		CTFPlayer *player = playerVector[i];

		float rangeSq = GetRangeSquaredTo( player );
		if ( rangeSq < closeRangeSq )
		{
			closeRangeSq = rangeSq;
			close = player;
		}
	}

	QAngle facingAngle;

	if ( close )
	{
		Vector toPlayer = close->GetAbsOrigin() - GetAbsOrigin();
		toPlayer.z = 0.0f;
		toPlayer.NormalizeInPlace();

		VectorAngles( toPlayer, Vector(0,0,1), facingAngle );
	}
	else
	{
		facingAngle.x = 0.0f;
		facingAngle.y = RandomFloat( 0.0f, 360.0f );
		facingAngle.z = 0.0f;
	}

	SetAbsAngles( facingAngle );
}


bool CMerasmus::ShouldReveal() const
{
	int nDestroyedProps = 0;
	for ( int i=0; i<m_fakePropVector.Count(); ++i )
	{
		if ( m_fakePropVector[i] == NULL )
		{
			nDestroyedProps++;
		}
	}
	return nDestroyedProps >= m_nDestroyedPropsToReveal;
}


bool CMerasmus::IsNextKilledPropMerasmus() const
{
	int nDestroyedProps = 0;
	for ( int i=0; i<m_fakePropVector.Count(); ++i )
	{
		if ( m_fakePropVector[i] == NULL )
		{
			nDestroyedProps++;
		}
	}
	return nDestroyedProps + 1 == m_nDestroyedPropsToReveal;
}


void CMerasmus::OnDisguise()
{
	m_flStartDisguiseTime = gpGlobals->curtime;
	m_bRevealed = false;
	m_isHiding = true;

	StopAOEAttack();

	AddEffects( EF_NOINTERP | EF_NODRAW );

	DispatchParticleEffect( "merasmus_tp", WorldSpaceCenter(), GetAbsAngles() );

	// don't collide with anything, we do our own collision detection in our think method
	SetSolid( SOLID_NONE );
	SetSolidFlags( FSOLID_NOT_SOLID );

	// fake randomness of when Merasmus should reveal
	m_nDestroyedPropsToReveal = RandomInt( MAX( 1, tf_merasmus_min_props_to_reveal.GetFloat() * m_fakePropVector.Count() ), m_fakePropVector.Count() );
}


bool CMerasmus::ShouldDisguise() const
{
	if ( GetHealth() <= 0 )
	{
		return false;
	}

	float flLostHealthPercentage = (float)( m_nRevealedHealth - GetHealth() ) / (float)GetMaxHealth();
	return flLostHealthPercentage > tf_merasmus_should_disguise_threshold.GetFloat();
}


CTFWeaponBaseGrenadeProj* CMerasmus::CreateMerasmusGrenade( const Vector& vPosition, const Vector& vVelocity, CBaseCombatCharacter* pOwner, float fScale )
{
	QAngle qAngles = RandomAngle( 0, 360 );
	CTFWeaponBaseMerasmusGrenade *pGrenade = static_cast<CTFWeaponBaseMerasmusGrenade*>( CBaseEntity::Create( "tf_weaponbase_merasmus_grenade", vPosition, qAngles, pOwner ) );
	if ( pGrenade )
	{
		pGrenade->SetModel( MERASMUS_BOMB_MODEL );
		DispatchSpawn( pGrenade );
		pGrenade->InitGrenade( vVelocity, AngularImpulse( 600, random->RandomInt( -1200, 1200 ), 0 ), pOwner, 50 * fScale, 300.f * fScale );
		pGrenade->SetDetonateTimerLength( 2.f );
		pGrenade->SetModelScale( fScale );
		pGrenade->SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS );  // we want to use collision_group_rockets so we don't ever collide with players
	}

	return pGrenade;
}


const char* CMerasmus::GetRandomPropModelName()
{
	int which = RandomInt( 0, ARRAYSIZE( s_pszDisguiseProps ) - 1 );
	return s_pszDisguiseProps[ which ];
}


void CMerasmus::PushPlayer( CTFPlayer* pPlayer, float flPushForce ) const
{
	// send the player flying
	// make sure we push players up and away
	Vector toPlayer = pPlayer->EyePosition() - GetAbsOrigin();
	toPlayer.z = 0.0f;
	toPlayer.NormalizeInPlace();
	toPlayer.z = 1.0f;

	Vector push = flPushForce * toPlayer;

	pPlayer->ApplyAbsVelocityImpulse( push );
}


void CMerasmus::AddStun( CTFPlayer* pPlayer )
{
	if ( !IsRevealed() )
	{
		// don't let bomb head player explode on merasmus while disguise
		return;
	}

	// first stun
	if ( !IsStunned() )
	{
		CPVSFilter filter( WorldSpaceCenter() );
		if (RandomInt( 1, 10) == 9 )
		{
			PlayLowPrioritySound( filter, "Halloween.MerasmusHitByBombRare" ); 
		}
		else
		{
			PlayLowPrioritySound( filter, "Halloween.MerasmusHitByBomb" ); 
		}
	}

	// buff the player that stunned me
	const float buffDuration = 10.0f;
	pPlayer->m_Shared.AddCond( TF_COND_CRITBOOSTED_PUMPKIN, buffDuration );
	pPlayer->m_Shared.AddCond( TF_COND_SPEED_BOOST, buffDuration );
	pPlayer->m_Shared.AddCond( TF_COND_INVULNERABLE, buffDuration );

	pPlayer->m_Shared.RemoveCond( TF_COND_STUNNED );
	pPlayer->m_Shared.RemoveCond( TF_COND_HALLOWEEN_BOMB_HEAD );
	pPlayer->MerasmusPlayerBombExplode( false );
			 
	PushPlayer( pPlayer, 300.f );
	DispatchParticleEffect( "merasmus_dazed_explosion", WorldSpaceCenter(), GetAbsAngles() );

	IGameEvent *pEvent = gameeventmanager->CreateEvent( "merasmus_stunned" );
	if ( pEvent )
	{
		pEvent->SetInt( "player", pPlayer->GetUserID() );
		gameeventmanager->FireEvent( pEvent, true );
	}

	// don't stun while doing AOE
	if ( !m_bIsDoingAOEAttack )
	{
		m_nBombHitCount++;
		m_stunTimer.Start( tf_merasmus_stun_duration.GetFloat() );
	}
}


void CMerasmus::OnBeginStun()
{
	EmitSound( "Halloween.Merasmus_Stun" );

	m_bStunned = true;
}


void CMerasmus::OnEndStun()
{
	m_stunTimer.Invalidate();
	m_bStunned = false;
}


void CMerasmus::AddFakeProp( CTFMerasmusTrickOrTreatProp* pFakeProp )
{
	m_fakePropVector.AddToTail( pFakeProp );
}


void CMerasmus::RemoveAllFakeProps()
{
	for ( int i=0; i<m_fakePropVector.Count(); ++i )
	{
		if ( m_fakePropVector[i] != NULL )
		{
			UTIL_Remove( m_fakePropVector[i] );
		}
	}
	m_fakePropVector.RemoveAll();
}


void BombHeadForTeam( int nTeam, int nBombHeadPlayers )
{
	// decrease nBombHeadPlayers by the number of existing bomb heads
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, nTeam, COLLECT_ONLY_LIVING_PLAYERS );

	if ( playerVector.Count() <= 0 )
	{
		// everyone on this team is dead
		return;
	}

	for( int n=0; n<nBombHeadPlayers; ++n )
	{
		// find the living player who was a bombhead the longest time ago and give them the bomb
		CTFPlayer *pVictim = NULL;
		float oldestTimeStamp = -1.0f;

		for( int i=0; i<playerVector.Count(); ++i )
		{
			CTFPlayer *pPlayer = playerVector[i];

			if ( pPlayer->GetTimeSinceWasBombHead() > oldestTimeStamp && 
				 !pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_BOMB_HEAD ) &&
				 pPlayer->GetLastKnownArea() &&
				 pPlayer->GetLastKnownArea()->HasFuncNavPrefer() )
			{
				pVictim = pPlayer;
				oldestTimeStamp = pPlayer->GetTimeSinceWasBombHead();
			}
		}

		if ( !pVictim )
		{
			// no victims available - try again next time
			return;
		}

		// give this victim the bomb
		float flBuffDuration = tf_merasmus_bomb_head_duration.GetFloat();
		pVictim->m_Shared.StunPlayer( tf_merasmus_bomb_head_duration.GetFloat(), 0.f, TF_STUN_LOSER_STATE );
		pVictim->m_Shared.AddCond( TF_COND_HALLOWEEN_BOMB_HEAD, flBuffDuration );
		pVictim->m_Shared.AddCond( TF_COND_SPEED_BOOST, flBuffDuration );

		pVictim->SetBombHeadTimestamp();

		// notify player they are a bomb
		ClientPrint( pVictim, HUD_PRINTCENTER, "#TF_HALLOWEEN_MERASMUS_YOU_ARE_BOMB", pVictim->GetPlayerName() );
	}
}


void CMerasmus::BombHeadMode()
{
	int nBombHeadPlayers = tf_merasmus_bomb_head_per_team.GetInt();
	BombHeadForTeam( TF_TEAM_RED, nBombHeadPlayers );
	BombHeadForTeam( TF_TEAM_BLUE, nBombHeadPlayers );
}


bool CMerasmus::ShouldLeave() const
{
	return m_lifeTimer.IsElapsed();
}


void CMerasmus::LeaveWarning()
{
	if ( m_lifeTimer.GetRemainingTime() < 10.0f && m_flLastWarnTime > 10.0f )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "merasmus_escape_warning" );
		if ( event )
		{
			event->SetInt( "level", GetLevel() );
			event->SetInt( "time_remaining", 10 );
			gameeventmanager->FireEvent( event );
		}
	}
	else if ( m_lifeTimer.GetRemainingTime() < 30.0f && m_flLastWarnTime > 30.0f )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "merasmus_escape_warning" );
		if ( event )
		{
			event->SetInt( "level", GetLevel() );
			event->SetInt( "time_remaining", 30 );
			gameeventmanager->FireEvent( event );
		}
	}
	else if ( m_lifeTimer.GetRemainingTime() < 60.0f && m_flLastWarnTime > 60.0f )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "merasmus_escape_warning" );
		if ( event )
		{
			event->SetInt( "level", GetLevel() );
			event->SetInt( "time_remaining", 60 );
			gameeventmanager->FireEvent( event );
		}
	}
	m_flLastWarnTime = m_lifeTimer.GetRemainingTime();
}


void CMerasmus::OnLeaveWhileInPropForm()
{
	CUtlVector< CBaseEntity* > validProps;
	for ( int i=0; i<m_fakePropVector.Count(); ++i )
	{
		if ( m_fakePropVector[i] != NULL )
		{
			validProps.AddToTail( m_fakePropVector[i] );
		}
	}

	if ( validProps.Count() )
	{
		int which = RandomInt( 0, validProps.Count() -1 );
		SetAbsOrigin( validProps[ which ]->GetAbsOrigin() );
	}
}


void CMerasmus::TriggerLogicRelay( const char* pszLogicRelayName, bool bSpawn /*= false*/ )
{
	CLogicRelay* pLogicRelay = assert_cast< CLogicRelay* >( gEntList.FindEntityByName( NULL, pszLogicRelayName ) );
	if ( pLogicRelay )
	{
		inputdata_t data;
		data.pCaller = this;
		data.pActivator = this;
		pLogicRelay->InputTrigger( data );

		if ( bSpawn )
		{
			SetAbsOrigin( pLogicRelay->GetAbsOrigin() );
		}
	}
}


void CMerasmus::PlayLowPrioritySound( IRecipientFilter &filter, const char* pszSoundEntryName )
{
	CSoundParameters params;
	if ( CBaseEntity::GetParametersForSound( pszSoundEntryName, params, NULL ) )
	{
		EmitSound_t es( params );
		es.m_nFlags |= SND_DO_NOT_OVERWRITE_EXISTING_ON_CHANNEL;
		EmitSound( filter, entindex(), es );
	}
}

void CMerasmus::PlayHighPrioritySound( const char* pszSoundEntryName )
{
	CBroadcastRecipientFilter filter;
	CSoundParameters params;
	if ( CBaseEntity::GetParametersForSound( pszSoundEntryName, params, NULL ) )
	{
		EmitSound_t es( params );
		EmitSound( filter, entindex(), es );
	}
}

//---------------------------------------------------------------------------------------------
void CMerasmus::RecordDisguiseTime( )
{
	if ( m_flStartDisguiseTime == 0 )
		return;

	float flTime = ( gpGlobals->curtime - m_flStartDisguiseTime );

	if ( m_bossStats.m_flPropHuntTime1 == 0 )
	{
		m_bossStats.m_flPropHuntTime1 = flTime;
	}
	else
	{
		m_bossStats.m_flPropHuntTime2 = flTime;
	}

	m_flStartDisguiseTime = 0;
}

void CMerasmus::StartRespawnTimer() const 
{
	if( TFGameRules() )
	{
		if( GetLevel() <= 3 )
		{
			TFGameRules()->StartHalloweenBossTimer( tf_merasmus_spawn_interval.GetFloat() ,tf_merasmus_spawn_interval_variation.GetFloat() );
		}
		else
		{
			TFGameRules()->StartHalloweenBossTimer( 60.f );
		}
	}
}

//---------------------------------------------------------------------------------------------
void CMerasmus::SW_ReportMerasmusStats( void )
{
	if ( !GCClientSystem() )
		return;

	static uint8 unEventCounter = 0;
	
	GCSDK::CProtoBufMsg<CMsgHalloween_Merasmus2012> msg( k_EMsgGC_Halloween_Merasmus2012 );
	msg.Body().set_time_submitted( CRTime::RTime32TimeCur() );
	msg.Body().set_is_valve_server( false );
	msg.Body().set_boss_level( GetLevel() );
	msg.Body().set_spawned_health( GetMaxHealth() );
	msg.Body().set_remaining_health( GetHealth() );
	msg.Body().set_life_time( (int)m_lifeTimer.GetElapsedTime() );					// Amount of time in seconds, boss was alive for
	msg.Body().set_bomb_kills( m_bossStats.m_nBombKills );				// Kills from Bombs
	msg.Body().set_staff_kills( m_bossStats.m_nStaffKills );					// kills from staff attack
	msg.Body().set_pvp_kills( m_bossStats.m_nPvpKills );						// Number of kills from players while Boss is out (Jerk factor)
	msg.Body().set_prophunt_time1( m_bossStats.m_flPropHuntTime1 );
	msg.Body().set_prophunt_time2( m_bossStats.m_flPropHuntTime2 );

	msg.Body().set_dmg_scout( m_bossStats.m_arrClassDamage[ TF_CLASS_SCOUT ] );			// Amount of damage done by each class
	msg.Body().set_dmg_sniper( m_bossStats.m_arrClassDamage[ TF_CLASS_SNIPER] );	
	msg.Body().set_dmg_soldier( m_bossStats.m_arrClassDamage[ TF_CLASS_SOLDIER] );	
	msg.Body().set_dmg_demo( m_bossStats.m_arrClassDamage[ TF_CLASS_DEMOMAN] );	
	msg.Body().set_dmg_medic( m_bossStats.m_arrClassDamage[ TF_CLASS_MEDIC ] );	
	msg.Body().set_dmg_heavy( m_bossStats.m_arrClassDamage[ TF_CLASS_HEAVYWEAPONS ] );	
	msg.Body().set_dmg_pyro( m_bossStats.m_arrClassDamage[ TF_CLASS_PYRO ] );	
	msg.Body().set_dmg_spy( m_bossStats.m_arrClassDamage[ TF_CLASS_SPY ] );	
	msg.Body().set_dmg_engineer( m_bossStats.m_arrClassDamage[ TF_CLASS_ENGINEER ] );	

	// Class counts
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector ); 
	
	int nClassCounts[ TF_LAST_NORMAL_CLASS ];
	V_memset( nClassCounts, 0, sizeof( nClassCounts ) );
	FOR_EACH_VEC( playerVector, index )
	{
		int iClass = playerVector[index]->GetPlayerClass()->GetClassIndex();
		if ( iClass > TF_CLASS_UNDEFINED && iClass < TF_LAST_NORMAL_CLASS )
		{
			nClassCounts[iClass]++;
		}
	}

	msg.Body().set_scout_count( nClassCounts[TF_CLASS_SCOUT] );				// Class and player break down at the point of boss despawn
	msg.Body().set_sniper_count( nClassCounts[TF_CLASS_SNIPER] );
	msg.Body().set_solider_count( nClassCounts[TF_CLASS_SOLDIER] );
	msg.Body().set_demo_count( nClassCounts[TF_CLASS_DEMOMAN] );
	msg.Body().set_medic_count( nClassCounts[TF_CLASS_MEDIC] );
	msg.Body().set_heavy_count( nClassCounts[TF_CLASS_HEAVYWEAPONS] );
	msg.Body().set_pyro_count( nClassCounts[TF_CLASS_PYRO] );
	msg.Body().set_spy_count( nClassCounts[TF_CLASS_SPY] );
	msg.Body().set_engineer_count( nClassCounts[TF_CLASS_ENGINEER] );

	GCClientSystem()->BSendMessage( msg );
	

// OGS Version 
//
//#if !defined(NO_STEAM)
//	KeyValues* pKVData = new KeyValues( "TF2Halloween2012MerasmusBossStats" );
//
//	// Auto Values
//	// ID
//	// SessionID
//	// TimeSubmitted
//	//pKVData->SetBool( "IsValveServer", false );
//
//	pKVData->SetInt( "BossLevel", GetLevel() );
//	pKVData->SetInt( "SpawnedHealth", GetMaxHealth() );
//	pKVData->SetInt( "RemainingHealth", GetHealth() );							// 0 == Boss was killed
//
//	pKVData->SetInt( "LifeTime", (int)m_lifeTimer.GetElapsedTime() );			// Amount of time in seconds, boss was alive for
//	pKVData->SetInt( "BombKills", m_bossStats.m_nBombKills );					// Kills from Bombs
//	pKVData->SetInt( "StaffKills", m_bossStats.m_nStaffKills );					// kills from staff account
//	pKVData->SetInt( "PvPKills", m_bossStats.m_nPvpKills );						// Number of kills from players while Boss is out (Jerk factor)
//	pKVData->SetInt( "PropHuntTime1", m_bossStats.m_flPropHuntTime1 );		
//	pKVData->SetInt( "PropHuntTime2", m_bossStats.m_flPropHuntTime2 );
//
//	// Class Damage
//	pKVData->SetInt( "DmgFromScout", m_bossStats.m_arrClassDamage[ TF_CLASS_SCOUT ] );			// Amount of damage done by each class
//	pKVData->SetInt( "DmgFromSniper", m_bossStats.m_arrClassDamage[ TF_CLASS_SNIPER ] );
//	pKVData->SetInt( "DmgFromSoldier", m_bossStats.m_arrClassDamage[ TF_CLASS_SOLDIER ] );
//	pKVData->SetInt( "DmgFromDemo", m_bossStats.m_arrClassDamage[ TF_CLASS_DEMOMAN ] );
//	pKVData->SetInt( "DmgFromMedic", m_bossStats.m_arrClassDamage[ TF_CLASS_MEDIC ] );
//	pKVData->SetInt( "DmgFromHeavy", m_bossStats.m_arrClassDamage[ TF_CLASS_HEAVYWEAPONS ] );
//	pKVData->SetInt( "DmgFromPyro", m_bossStats.m_arrClassDamage[ TF_CLASS_PYRO ] );
//	pKVData->SetInt( "DmgFromSpy", m_bossStats.m_arrClassDamage[ TF_CLASS_SPY ] );
//	pKVData->SetInt( "DmgFromEngineer", m_bossStats.m_arrClassDamage[ TF_CLASS_ENGINEER ] );
//
//	// Class counts
//	CUtlVector< CTFPlayer * > playerVector;
//	CollectPlayers( &playerVector ); 
//
//	int nClassCounts[ TF_LAST_NORMAL_CLASS ];
//	V_memset( nClassCounts, 0, sizeof( nClassCounts ) );
//	FOR_EACH_VEC( playerVector, index )
//	{
//		int iClass = playerVector[index]->GetPlayerClass()->GetClassIndex();
//		if ( iClass > TF_CLASS_UNDEFINED && iClass < TF_LAST_NORMAL_CLASS )
//		{
//			nClassCounts[iClass]++;
//		}
//	}
//
//	pKVData->SetInt( "ScoutCount", nClassCounts[TF_CLASS_SCOUT] );				// Class and player break down at the point of boss despawn
//	pKVData->SetInt( "SniperCount", nClassCounts[TF_CLASS_SNIPER] );
//	pKVData->SetInt( "SoldierCount", nClassCounts[TF_CLASS_SOLDIER] );
//	pKVData->SetInt( "DemoCount", nClassCounts[TF_CLASS_DEMOMAN] );
//	pKVData->SetInt( "MedicCount", nClassCounts[TF_CLASS_MEDIC] );
//	pKVData->SetInt( "HeavyCount", nClassCounts[TF_CLASS_HEAVYWEAPONS] );
//	pKVData->SetInt( "PyroCount", nClassCounts[TF_CLASS_PYRO] );
//	pKVData->SetInt( "SpyCount", nClassCounts[TF_CLASS_SPY] );
//	pKVData->SetInt( "EngineerCount", nClassCounts[TF_CLASS_ENGINEER] );
//
//	//GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
//#endif

	m_bossStats.ResetStats();
}


void CollectTargets( CBaseCombatCharacter *pCaster, float flSpellRange, int nTargetTeam, int nMaxTarget, CUtlVector< CHandle< CBaseEntity > > &vecTargets )
{
	vecTargets.RemoveAll();

	// collect everyone
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, nTargetTeam, COLLECT_ONLY_LIVING_PLAYERS );

	CUtlVector< CTFPlayer * > candidateTargets;
	for ( int i=0; i<playerVector.Count(); ++i )
	{
		CTFPlayer *pPlayer = playerVector[i];
		Vector toPlayer = pCaster->EyePosition() - pPlayer->WorldSpaceCenter();
		if ( toPlayer.IsLengthLessThan( flSpellRange ) && pCaster->IsLineOfSightClear( pPlayer ) )
		{
			candidateTargets.AddToTail( pPlayer );
		}
	}

	while ( candidateTargets.Count() != 0 && vecTargets.Count() != nMaxTarget )
	{
		int which = RandomInt( 0, candidateTargets.Count() - 1 );
		vecTargets.AddToTail( candidateTargets[ which ] );
		candidateTargets.FastRemove( which );
	}

	// find sentry in range
	for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *pObj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[i] );
		if ( pObj->ObjectType() == OBJ_SENTRYGUN )
		{
			Vector toSentry = pCaster->EyePosition() - pObj->WorldSpaceCenter();
			if ( toSentry.IsLengthLessThan( flSpellRange ) && pCaster->IsLineOfSightClear( pObj ) )
			{
				vecTargets.AddToTail( pObj );
			}
		}
	}
}


void CastSpell( CBaseCombatCharacter* pCaster, const char* pszCastingAttachmentName, float flSpellRange, float flMinDamage, float flMaxDamage, CBaseEntity* pTarget )
{
	float flSpellTime = 5.f;

	if ( pTarget->IsPlayer() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pTarget );
		pPlayer->m_Shared.SelfBurn( flSpellTime );
		pPlayer->ApplyAbsVelocityImpulse( 1000.f * Vector( 0, 0, 1 ) );

		Vector toPlayer = pCaster->EyePosition() - pPlayer->WorldSpaceCenter();
		float flDistSqr = toPlayer.LengthSqr();

		float flDmg = RemapValClamped( flDistSqr, 100.f, Square( 0.5f * flSpellRange ), flMaxDamage, flMinDamage );
		CTakeDamageInfo info( pCaster, pCaster, flDmg, DMG_BURN, TF_DMG_CUSTOM_MERASMUS_ZAP );
		pPlayer->TakeDamage( info );
	}
	else if ( pTarget->IsBaseObject() )
	{
		CBaseObject *pObj = static_cast< CBaseObject* >( pTarget );
		pObj->DetonateObject();
	}

	// Shoot a beam at them
	CReliableBroadcastRecipientFilter filter;
	Vector vStartPos;
	QAngle qStartAngles;
	pCaster->GetAttachment( pszCastingAttachmentName, vStartPos, qStartAngles );
	Vector vEnd = pTarget->EyePosition();
	te_tf_particle_effects_control_point_t controlPoint = { PATTACH_ABSORIGIN, vEnd };
	TE_TFParticleEffectComplex( filter, 0.0f, "merasmus_zap", vStartPos, qStartAngles, NULL, &controlPoint, pCaster, PATTACH_CUSTOMORIGIN );
}


/*static*/ bool CMerasmus::Zap( CBaseCombatCharacter *pCaster, const char* pszCastingAttachmentName, float flSpellRange, float flMinDamage, float flMaxDamage, int nMaxTarget, int nTargetTeam /*= TEAM_ANY*/ )
{
	CUtlVector< CHandle< CBaseEntity > > vecTargets;
	CollectTargets( pCaster, flSpellRange, nTargetTeam, nMaxTarget, vecTargets );

	if ( vecTargets.Count() == 0 )
		return false;

	for ( int i=0; i<vecTargets.Count(); ++i )
	{
		CBaseEntity *pTarget = vecTargets[i];
		if ( pTarget )
		{
			CastSpell( pCaster, pszCastingAttachmentName, flSpellRange, flMinDamage, flMaxDamage, pTarget );
		}
	}

	return true;
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CMerasmusBehavior : public Action< CMerasmus >
{
public:
	virtual Action< CMerasmus > *InitialContainedAction( CMerasmus *me )	
	{
		return new CMerasmusReveal;
	}

	virtual ActionResult< CMerasmus >	OnStart( CMerasmus *me, Action< CMerasmus > *priorAction )
	{
		return Continue();
	}

	virtual ActionResult< CMerasmus >	Update( CMerasmus *me, float interval )
	{
		if ( !me->IsAlive() )
		{
			if ( !me->WasSpawnedByCheats() )
			{
				// award achievement to everyone who injured me within the last few seconds
				const float deathTime = 5.0f;
				const CUtlVector< CMerasmus::AttackerInfo > &attackerVector = me->GetAttackerVector();
				for( int i=0; i<attackerVector.Count(); ++i )
				{
					if ( attackerVector[i].m_attacker != NULL && 
						gpGlobals->curtime - attackerVector[i].m_timestamp < deathTime )
					{
						CReliableBroadcastRecipientFilter filter;
						UTIL_SayText2Filter( filter, attackerVector[i].m_attacker, false, "#TF_Halloween_Merasmus_Killers", attackerVector[i].m_attacker->GetPlayerName() );

						if ( TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_LAKESIDE ) )
						{
							attackerVector[i].m_attacker->AwardAchievement( ACHIEVEMENT_TF_HALLOWEEN_MERASMUS_KILL );
						}
					}
				}

				// Award hat levels based on Merasmus' level when he dies, but only to people who were
				// around when Merasmus spawned.
				const CUtlVector< CHandle<CTFPlayer> >& vecStartingAttackers = me->GetStartingAttackers();
				if ( GCClientSystem() )
				{
					// GC message
					// Notify the GC that this occurred to possibly level up your hat if you have one
					GCSDK::CProtoBufMsg<CMsgUpdateHalloweenMerasmusLootLevel> msg( k_EMsgGC_Halloween_UpdateMerasmusLootLevel );
					msg.Body().set_merasmus_level( me->GetLevel() );

					FOR_EACH_VEC( vecStartingAttackers, i )
					{
						CTFPlayer* pPlayer = vecStartingAttackers[i];
						if ( pPlayer )
						{
							CSteamID steamID;
							if ( pPlayer->GetSteamID( &steamID ) && steamID.IsValid() && steamID.BIndividualAccount() )
							{
								CMsgUpdateHalloweenMerasmusLootLevel_Player *pMsgPlayer = msg.Body().add_players();
								pMsgPlayer->set_steam_id( steamID.ConvertToUint64() );
							}
							
						}
					}
					GCClientSystem()->BSendMessage( msg );
				}
			}

			// nobody is IT any longer
			TFGameRules()->SetIT( NULL );

			return ChangeTo( new CMerasmusDying, "I am dead!" );
		}
		else
		{
			me->LeaveWarning();

			if ( me->ShouldLeave() && !me->IsStunned() && !me->IsFlying() )
			{
				return ChangeTo( new CMerasmusEscape, "Escaping..." );
			}
		}

		return Continue();
	}


	virtual EventDesiredResult< CMerasmus > OnInjured( CMerasmus *me, const CTakeDamageInfo &info )
	{
		if ( me->ShouldDisguise() && me->IsRevealed() && !me->IsStunned() && !me->IsFlying() ) 
		{
			return TrySuspendFor( new CMerasmusDisguise, RESULT_IMPORTANT, "Disguise" );
		}

		return TryContinue();
	}

	virtual const char *GetName( void ) const	{ return "Merasmus Behavior"; }		// return name of this action

private:

	
};


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
CMerasmusIntention::CMerasmusIntention( CMerasmus *me ) : IIntention( me )
{ 
	m_behavior = new Behavior< CMerasmus >( new CMerasmusBehavior ); 
}

CMerasmusIntention::~CMerasmusIntention()
{
	delete m_behavior;
}

void CMerasmusIntention::Reset( void )
{ 
	delete m_behavior; 
	m_behavior = new Behavior< CMerasmus >( new CMerasmusBehavior );
}

void CMerasmusIntention::Update( void )
{
	m_behavior->Update( static_cast< CMerasmus * >( GetBot() ), GetUpdateInterval() ); 
}

// is this a place we can be?
QueryResultType CMerasmusIntention::IsPositionAllowed( const INextBot *meBot, const Vector &pos ) const
{
	return ANSWER_YES;
}



//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
void CMerasmusLocomotion::Update( void )
{
	CMerasmus *me = (CMerasmus *)GetBot()->GetEntity();

	if ( me->IsFlying() )
	{
		// don't update this locomotor since the flying locomotor is active
		return;
	}

	NextBotGroundLocomotion::Update();
}

float CMerasmusLocomotion::GetRunSpeed( void ) const
{
	return tf_merasmus_speed.GetFloat();
}


//---------------------------------------------------------------------------------------------
// if delta Z is greater than this, we have to jump to get up
float CMerasmusLocomotion::GetStepHeight( void ) const
{
	return 18.0f;
}


//---------------------------------------------------------------------------------------------
// return maximum height of a jump
float CMerasmusLocomotion::GetMaxJumpHeight( void ) const
{
	return 18.0f;
}


//---------------------------------------------------------------------------------------------
// Return max rate of yaw rotation
float CMerasmusLocomotion::GetMaxYawRate( void ) const
{
	return 200.0f;
}


//---------------------------------------------------------------------------------------------
bool CMerasmusLocomotion::ShouldCollideWith( const CBaseEntity *object ) const
{
	if ( !object )
		return false;

	// Don't collide with players
	return object->IsPlayer() ? false : true;
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
#define MERASMUS_ACCELERATION 250.0f //500.0f

CMerasmusFlyingLocomotion::CMerasmusFlyingLocomotion( INextBot *bot ) : ILocomotion( bot )
{
	Reset();
}


//---------------------------------------------------------------------------------------------
CMerasmusFlyingLocomotion::~CMerasmusFlyingLocomotion()
{
}


//---------------------------------------------------------------------------------------------
// (EXTEND) reset to initial state
void CMerasmusFlyingLocomotion::Reset( void )
{
	m_velocity = vec3_origin;
	m_acceleration = vec3_origin;
	m_currentSpeed = 0.0f;
	m_forward = vec3_origin;
	m_desiredAltitude = 50.0f;
}


//---------------------------------------------------------------------------------------------
void CMerasmusFlyingLocomotion::MaintainAltitude( void )
{
	CBaseCombatCharacter *me = GetBot()->GetEntity();

	float groundZ;
	TheNavMesh->GetSimpleGroundHeight( me->GetAbsOrigin(), &groundZ );

	float currentAltitude = me->GetAbsOrigin().z - groundZ;
	float error = m_desiredAltitude - currentAltitude;
	float accelZ = clamp( error, -MERASMUS_ACCELERATION, MERASMUS_ACCELERATION );

	m_acceleration.z += accelZ;
}


//---------------------------------------------------------------------------------------------
// (EXTEND) update internal state
void CMerasmusFlyingLocomotion::Update( void )
{
	CMerasmus *me = (CMerasmus *)GetBot()->GetEntity();
	const float deltaT = GetUpdateInterval();

	if ( !me->IsFlying() )
	{
		// not flying - let the other locomotor run
		return;
	}

	Vector pos = me->GetAbsOrigin();

	// always maintain altitude, even if not trying to move (ie: no Approach call)
	MaintainAltitude();

	m_forward = m_velocity;
	m_currentSpeed = m_forward.NormalizeInPlace();

	Vector damping( 1.0f, 1.0f, 1.0f );
	Vector totalAccel = m_acceleration - m_velocity * damping;

	m_velocity += totalAccel * deltaT;
	me->SetAbsVelocity( m_velocity );

	pos += m_velocity * deltaT;

	// Merasmus doesn't collide with players and floats between valid nav areas
	// so skip the collision checking
	GetBot()->GetEntity()->SetAbsOrigin( pos );
	m_acceleration = vec3_origin;
}


//---------------------------------------------------------------------------------------------
// (EXTEND) move directly towards the given position
void CMerasmusFlyingLocomotion::Approach( const Vector &goalPos, float goalWeight )
{
	Vector flyGoal = goalPos;
	flyGoal.z += m_desiredAltitude;

	Vector toGoal = flyGoal - GetBot()->GetEntity()->GetAbsOrigin();
	// altitude is handled in Update()
	toGoal.z = 0.0f;
	toGoal.NormalizeInPlace();

	m_acceleration += MERASMUS_ACCELERATION * toGoal;
}


//---------------------------------------------------------------------------------------------
float CMerasmusFlyingLocomotion::GetDesiredSpeed( void ) const
{
	return tf_merasmus_speed.GetFloat();
}


//---------------------------------------------------------------------------------------------
void CMerasmusFlyingLocomotion::SetDesiredAltitude( float height )
{
	m_desiredAltitude = height;
}


//---------------------------------------------------------------------------------------------
float CMerasmusFlyingLocomotion::GetDesiredAltitude( void ) const
{
	return m_desiredAltitude;
}

//---------------------------------------------------------------------------------------------
bool CMerasmusFlyingLocomotion::ShouldCollideWith( const CBaseEntity *object ) const
{
	if ( !object )
		return false;

	// Don't collide with players
	return object->IsPlayer() ? false : true;
}

//---------------------------------------------------------------------------------------------
void CMerasmusFlyingLocomotion::FaceTowards( const Vector &target )
{
	CMerasmus *me = (CMerasmus *)GetBot()->GetEntity();
	const float deltaT = GetUpdateInterval();

	QAngle angles = me->GetLocalAngles();

	float desiredYaw = UTIL_VecToYaw( target - GetFeet() );

	float angleDiff = UTIL_AngleDiff( desiredYaw, angles.y );

	const float maxYawRate = 100.0f;
	float deltaYaw = maxYawRate * deltaT;

	if ( angleDiff < -deltaYaw )
	{
		angles.y -= deltaYaw;
	}
	else if ( angleDiff > deltaYaw )
	{
		angles.y += deltaYaw;
	}
	else
	{
		angles.y += angleDiff;
	}

	me->SetLocalAngles( angles );
}



