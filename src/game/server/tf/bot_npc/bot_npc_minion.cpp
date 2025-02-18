//========= Copyright Valve Corporation, All rights reserved. ============//
// bot_npc_minion.cpp
// Minions for the Boss
// Michael Booth, November 2010

#include "cbase.h"

#ifdef TF_RAID_MODE

#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "tf_projectile_arrow.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "tf_ammo_pack.h"
#include "nav_mesh/tf_nav_area.h"
#include "bot_npc_minion.h"
#include "NextBot/Path/NextBotChasePath.h"
#include "econ_wearable.h"
#include "team_control_point_master.h"
#include "particle_parse.h"
#include "nav_mesh/tf_path_follower.h"
#include "tf_obj_sentrygun.h"
#include "bot/map_entities/tf_spawner.h"

#define MINION_LIGHT_ON 0
#define MINION_LIGHT_OFF 1

ConVar tf_bot_npc_minion_health( "tf_bot_npc_minion_health", "60"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_minion_speed( "tf_bot_npc_minion_speed", "250"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_minion_acceleration( "tf_bot_npc_minion_acceleration", "500"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_minion_horiz_damping( "tf_bot_npc_minion_horiz_damping", "2"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_minion_vert_damping( "tf_bot_npc_minion_vert_damping", "1"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_minion_stun_charge( "tf_bot_npc_minion_stun_charge", "0.4"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_minion_ammo_count( "tf_bot_npc_minion_ammo_count", "100"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_minion_debug( "tf_bot_npc_minion_debug", "0"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_minion_notice_threat_range( "tf_bot_npc_minion_notice_threat_range", "500"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_minion_stun_range( "tf_bot_npc_minion_stun_range", "100"/*,FCVAR_CHEAT */ );
ConVar tf_bot_npc_minion_stun_charge_up_time( "tf_bot_npc_minion_stun_charge_up_time", "1.5"/*,FCVAR_CHEAT */ );
ConVar tf_bot_npc_minion_stun_kill_time( "tf_bot_npc_minion_stun_kill_time", "7"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_minion_invuln_duration( "tf_bot_npc_minion_invuln_duration", "3"/*, FCVAR_CHEAT*/ );



LINK_ENTITY_TO_CLASS( bot_npc_minion, CBotNPCMinion );

PRECACHE_REGISTER( bot_npc_minion );

IMPLEMENT_SERVERCLASS_ST( CBotNPCMinion, DT_BotNPCMinion )

	SendPropEHandle( SENDINFO( m_stunTarget ) ),

END_SEND_TABLE()


//-----------------------------------------------------------------------------------------------------
class CTFSpawnTemplateStunDrone : public CTFSpawnTemplate
{
public:
	virtual CBaseEntity *Instantiate( void ) const
	{
		return CreateEntityByName( "bot_npc_minion" );
	}
};

LINK_ENTITY_TO_CLASS( tf_template_stun_drone, CTFSpawnTemplateStunDrone );




//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
CBotNPCMinion::CBotNPCMinion()
{
	ALLOCATE_INTENTION_INTERFACE( CBotNPCMinion );

	m_locomotor = new CNextBotFlyingLocomotion( this );
	m_body = new CBotNPCBody( this );
	m_vision = new CDisableVision( this );

	m_eyeOffset = vec3_origin;
	m_target = NULL;
	m_stunTarget = NULL;
	m_isAlert = false;
}


//-----------------------------------------------------------------------------------------------------
CBotNPCMinion::~CBotNPCMinion()
{
	DEALLOCATE_INTENTION_INTERFACE;

	if ( m_vision )
		delete m_vision;

	if ( m_locomotor )
		delete m_locomotor;

	if ( m_body )
		delete m_body;
}

//-----------------------------------------------------------------------------------------------------
void CBotNPCMinion::Precache()
{
	BaseClass::Precache();

 	PrecacheModel( "models/props_swamp/bug_zapper.mdl" );
// 	PrecacheModel( "models/combine_helicopter/helicopter_bomb01.mdl" );
//	PrecacheModel( "models/props_lights/spotlight001a.mdl" );

	PrecacheScriptSound( "Minion.Ping.Roam" );
	PrecacheScriptSound( "Minion.Ping.Acquire" );
	PrecacheScriptSound( "Minion.Bounce" );
//	PrecacheScriptSound( "Minion.Explode" );
	PrecacheScriptSound( "Minion.ChargeUpStun" );
	PrecacheScriptSound( "Minion.Stun" );
	PrecacheScriptSound( "Minion.StunKill" );
	PrecacheScriptSound( "Minion.Building" );
	PrecacheScriptSound( "Minion.Recognize" );
	PrecacheScriptSound( "Minion.Notice" );

	PrecacheParticleSystem( "cart_flashinglight" );
}


//-----------------------------------------------------------------------------------------------------
void CBotNPCMinion::Spawn( void )
{
	BaseClass::Spawn();

	SetModel( "models/props_swamp/bug_zapper.mdl" );

	int health = tf_bot_npc_minion_health.GetInt();
	SetHealth( health );
	SetMaxHealth( health );

	ChangeTeam( TF_TEAM_RED );

	// this flag lets flamethrowers deflect me
	AddFlag( FL_GRENADE );

	m_eyeOffset = Vector( 0, 0, 20.0f );
	m_lastKnownTargetPosition = vec3_origin;
	m_isAlert = false;

	m_invulnTimer.Start( tf_bot_npc_minion_invuln_duration.GetFloat() );
}


//---------------------------------------------------------------------------------------------
ConVar tf_bot_npc_minion_dmg_mult_sentry( "tf_bot_npc_minion_dmg_mult_sentry", "0.5"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_minion_dmg_mult_sniper( "tf_bot_npc_minion_dmg_mult_sniper", "2"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_minion_dmg_mult_arrow( "tf_bot_npc_minion_dmg_mult_arrow", "3"/*, FCVAR_CHEAT*/ );

float MinionModifyDamage( const CTakeDamageInfo &info )
{
	CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >( info.GetWeapon() );
	CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( info.GetInflictor() );

	if ( sentry )
	{
		return info.GetDamage() * tf_bot_npc_minion_dmg_mult_sentry.GetFloat();
	}
	else if ( pWeapon )
	{
		switch( pWeapon->GetWeaponID() )
		{
		case TF_WEAPON_SNIPERRIFLE:
		case TF_WEAPON_SNIPERRIFLE_DECAP:
		case TF_WEAPON_SNIPERRIFLE_CLASSIC:
			return info.GetDamage() * tf_bot_npc_minion_dmg_mult_sniper.GetFloat();

		case TF_WEAPON_COMPOUND_BOW:
			return info.GetDamage() * tf_bot_npc_minion_dmg_mult_arrow.GetFloat();
		}
	}

	// unmodified
	return info.GetDamage();
}


//---------------------------------------------------------------------------------------------
int CBotNPCMinion::OnTakeDamage_Alive( const CTakeDamageInfo &rawInfo )
{
	if ( !m_invulnTimer.IsElapsed() )
	{
		// invulnerable for a moment after spawning
		return 0;
	}

	CTakeDamageInfo info = rawInfo;

	info.SetDamage( MinionModifyDamage( info ) );

	BecomeAlert();

	// fire event for client combat text, beep, etc.
	IGameEvent *event = gameeventmanager->CreateEvent( "npc_hurt" );
	if ( event )
	{
		event->SetInt( "entindex", entindex() );
		event->SetInt( "health", MAX( 0, GetHealth() ) );
		event->SetInt( "damageamount", info.GetDamage() );
		event->SetBool( "crit", ( info.GetDamageType() & DMG_CRITICAL ) ? true : false );

		CTFPlayer *attackerPlayer = ToTFPlayer( info.GetAttacker() );
		if ( attackerPlayer )
		{
			event->SetInt( "attacker_player", attackerPlayer->GetUserID() );

			if ( attackerPlayer->GetActiveTFWeapon() )
			{
				event->SetInt( "weaponid", attackerPlayer->GetActiveTFWeapon()->GetWeaponID() );
			}
			else
			{
				event->SetInt( "weaponid", 0 );
			}
		}
		else
		{
			// hurt by world
			event->SetInt( "attacker_player", 0 );
			event->SetInt( "weaponid", 0 );
		}

		gameeventmanager->FireEvent( event );
	}

	return BaseClass::OnTakeDamage_Alive( info );
}


//---------------------------------------------------------------------------------------------
void CBotNPCMinion::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	BecomeAlert();

	if ( pDeflectedBy )
	{
		GetLocomotionInterface()->Deflect( pDeflectedBy );
	}
}


//---------------------------------------------------------------------------------------------
unsigned int CBotNPCMinion::PhysicsSolidMaskForEntity( void ) const
{ 
	// Only collide with the other team
	int teamContents = ( GetTeamNumber() == TF_TEAM_RED ) ? CONTENTS_BLUETEAM : CONTENTS_REDTEAM;

	return BaseClass::PhysicsSolidMaskForEntity() | teamContents;
}


//---------------------------------------------------------------------------------------------
bool CBotNPCMinion::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		switch( GetTeamNumber() )
		{
		case TF_TEAM_RED:
			if ( !( contentsMask & CONTENTS_REDTEAM ) )
				return false;
			break;

		case TF_TEAM_BLUE:
			if ( !( contentsMask & CONTENTS_BLUETEAM ) )
				return false;
			break;
		}
	}

	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}


//---------------------------------------------------------------------------------------------
void CBotNPCMinion::BecomeAmmoPack( void )
{
	int iPrimary = tf_bot_npc_minion_ammo_count.GetInt();
	int iSecondary = tf_bot_npc_minion_ammo_count.GetInt();
	int iMetal = tf_bot_npc_minion_ammo_count.GetInt();

	// Create the ammo pack.
	//CTFAmmoPack *pAmmoPack = CTFAmmoPack::Create( GetAbsOrigin(), GetAbsAngles(), NULL, "models/props_swamp/bug_zapper.mdl" );
	CTFAmmoPack *pAmmoPack = CTFAmmoPack::Create( GetAbsOrigin(), GetAbsAngles(), NULL, "models/items/ammopack_medium.mdl" );
	if ( pAmmoPack )
	{
		Vector vel = GetAbsVelocity();
		pAmmoPack->SetInitialVelocity( vel );
		pAmmoPack->m_nSkin = MINION_LIGHT_OFF;

		// Give the ammo pack some health, so that trains can destroy it.
		pAmmoPack->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
		pAmmoPack->m_takedamage = DAMAGE_YES;		
		pAmmoPack->SetHealth( 900 );

		pAmmoPack->SetBodygroup( 1, 1 );

		DispatchSpawn( pAmmoPack );

		// Fill up the ammo pack.
		pAmmoPack->GiveAmmo( iPrimary, TF_AMMO_PRIMARY );
		pAmmoPack->GiveAmmo( iSecondary, TF_AMMO_SECONDARY );
		pAmmoPack->GiveAmmo( iMetal, TF_AMMO_METAL );
	}
}


//---------------------------------------------------------------------------------------------
void CBotNPCMinion::BecomeAlert( void )
{
	m_isAlert = true;
	m_nSkin = MINION_LIGHT_ON;
}


//---------------------------------------------------------------------------------------------
//
// Find the closest living player not already being targeted by another minion
//
CTFPlayer *CBotNPCMinion::FindTarget( void )
{
	CUtlVector< CBotNPCMinion * > minionVector;
	CBotNPCMinion *minion = NULL;
	while( ( minion = (CBotNPCMinion *)gEntList.FindEntityByClassname( minion, "bot_npc_minion" ) ) != NULL )
	{
		minionVector.AddToTail( minion );
	}

	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS );

	CTFPlayer *closeVictim = NULL;
	float victimRangeSq = FLT_MAX;

	for( int i=0; i<playerVector.Count(); ++i )
	{
		if ( !playerVector[i]->IsAlive() )
			continue;

		float rangeSq = GetRangeSquaredTo( playerVector[i] );
		if ( rangeSq < victimRangeSq )
		{
			if ( playerVector[i]->m_Shared.IsStealthed() )
			{
				continue;
			}

			// is any other minion targeting this player?
			bool isTargetAvailable = true;
			for( int m=0; m<minionVector.Count(); ++m )
			{
				CBotNPCMinion *peer = minionVector[m];

				if ( peer != this && peer->HasTarget() && peer->GetTarget() == playerVector[i] )
				{
					isTargetAvailable = false;
					break;
				}
			}

			if ( isTargetAvailable )
			{
				closeVictim = playerVector[i];
				victimRangeSq = rangeSq;
			}
		}
	}
	
	return closeVictim;
}


//---------------------------------------------------------------------------------------------
void CBotNPCMinion::UpdateTarget( void )
{
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS );

	CTFPlayer *closestPlayer = NULL;
	float closestRangeSq = IsAlert() ? FLT_MAX : ( tf_bot_npc_minion_notice_threat_range.GetFloat() * tf_bot_npc_minion_notice_threat_range.GetFloat() );

	for( int i=0; i<playerVector.Count(); ++i )
	{
		CTFPlayer *player = playerVector[i];

		float rangeSq = ( player->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
		if ( rangeSq < closestRangeSq )
		{
			if ( IsLineOfSightClear( player, IGNORE_ACTORS ) )
			{
				closestPlayer = player;
				closestRangeSq = rangeSq;
			}
		}
	}

	if ( closestPlayer )
	{
		if ( closestPlayer != GetTarget() )
		{
			EmitSound( "Minion.Notice" );
			SetTarget( closestPlayer );
		}

		m_lastKnownTargetPosition = closestPlayer->GetAbsOrigin();
	}
	else
	{
		SetTarget( NULL );
	}
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCMinionHoldStunVictim : public Action< CBotNPCMinion >
{
public:
	virtual ActionResult< CBotNPCMinion >	OnStart( CBotNPCMinion *me, Action< CBotNPCMinion > *priorAction );
	virtual ActionResult< CBotNPCMinion >	Update( CBotNPCMinion *me, float interval );
	virtual void OnEnd( CBotNPCMinion *me, Action< CBotNPCMinion > *nextAction );

	virtual const char *GetName( void ) const	{ return "HoldStunVictim"; }		// return name of this action

private:
	CTFPathFollower m_path;
	CountdownTimer m_restunTimer;
	CountdownTimer m_killTimer;
};


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPCMinion > CBotNPCMinionHoldStunVictim::OnStart( CBotNPCMinion *me, Action< CBotNPCMinion > *priorAction )
{
	CTFPlayer *target = me->GetTarget();

	if ( !target )
	{
		return Done( "No target" );
	}

	me->StartStunEffects( target );

	me->EmitSound( "Minion.Stun" );

	m_restunTimer.Invalidate();
	m_killTimer.Start( tf_bot_npc_minion_stun_kill_time.GetFloat() );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPCMinion > CBotNPCMinionHoldStunVictim::Update( CBotNPCMinion *me, float interval )
{
	CTFPlayer *target = me->GetTarget();

	if ( !target )
	{
		return Done( "No target" );
	}

	me->GetLocomotionInterface()->FaceTowards( target->WorldSpaceCenter() );

	if ( me->IsRangeGreaterThan( target, tf_bot_npc_minion_stun_range.GetFloat() ) )
	{
		return Done( "Target out of stun range" );
	}

	// if we've held them long enough, they die
	if ( m_killTimer.IsElapsed() )
	{
		me->EmitSound( "Minion.StunKill" );

		CTakeDamageInfo info( me, me, 1000.0f, DMG_ENERGYBEAM, TF_DMG_CUSTOM_NONE );

		Vector toTarget = target->WorldSpaceCenter() - me->WorldSpaceCenter();
		toTarget.z = 0.5f;
		toTarget.NormalizeInPlace();

		CalculateMeleeDamageForce( &info, toTarget, me->WorldSpaceCenter(), 1.0f );
		target->TakeDamage( info );

		return Done( "My work here is done." );
	}

	// stun them!
	// if they are ubered we overload and explode from the energy feedback
	if ( target->m_Shared.InCond( TF_COND_INVULNERABLE ) )
	{
		CTakeDamageInfo info( target, target, me->GetMaxHealth() + 10.0f, DMG_ENERGYBEAM, TF_DMG_CUSTOM_NONE );

		Vector fromVictim = me->WorldSpaceCenter() - target->WorldSpaceCenter();
		fromVictim.NormalizeInPlace();

		CalculateMeleeDamageForce( &info, fromVictim, me->WorldSpaceCenter(), 1.0f );
		me->TakeDamage( info );

		return Done( "Destroyed by target's Uber" );
	}

	// periodically restart the stun effect
	if ( m_restunTimer.IsElapsed() )
	{
		const float stunTime = 1.0f;
		const float speedReduction = 0.5f;

		int stunFlags = TF_STUN_LOSER_STATE | TF_STUN_MOVEMENT | TF_STUN_CONTROLS;
		target->m_Shared.StunPlayer( stunTime + 0.5f, speedReduction, stunFlags, NULL );
		m_restunTimer.Start( stunTime );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CBotNPCMinionHoldStunVictim::OnEnd( CBotNPCMinion *me, Action< CBotNPCMinion > *nextAction )
{
	me->EndStunEffects();
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCMinionStartStunAttack : public Action< CBotNPCMinion >
{
public:
	virtual ActionResult< CBotNPCMinion >	OnStart( CBotNPCMinion *me, Action< CBotNPCMinion > *priorAction );
	virtual ActionResult< CBotNPCMinion >	Update( CBotNPCMinion *me, float interval );

	virtual const char *GetName( void ) const	{ return "StartStunAttack"; }		// return name of this action

private:
	CTFPathFollower m_path;
	CountdownTimer m_stunTimer;
};


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPCMinion > CBotNPCMinionStartStunAttack::OnStart( CBotNPCMinion *me, Action< CBotNPCMinion > *priorAction )
{
	// light on
	me->m_nSkin = MINION_LIGHT_ON;

	me->EmitSound( "Minion.ChargeUpStun" );
	m_stunTimer.Start( tf_bot_npc_minion_stun_charge_up_time.GetFloat() );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPCMinion > CBotNPCMinionStartStunAttack::Update( CBotNPCMinion *me, float interval )
{
	CTFPlayer *target = me->GetTarget();

	if ( !target )
	{
		return Done( "No target" );
	}

	me->GetLocomotionInterface()->FaceTowards( target->WorldSpaceCenter() );

	if ( me->IsRangeGreaterThan( target, tf_bot_npc_minion_stun_range.GetFloat() ) )
	{
		return Done( "Target out of stun range" );
	}

	if ( m_stunTimer.IsElapsed() )
	{
		// stun 'em
		return ChangeTo( new CBotNPCMinionHoldStunVictim, "Stun successful" );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCMinionReadyToStun : public Action< CBotNPCMinion >
{
public:
	virtual ActionResult< CBotNPCMinion >	Update( CBotNPCMinion *me, float interval );

	virtual const char *GetName( void ) const	{ return "ReadyToStun"; }		// return name of this action
};


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPCMinion > CBotNPCMinionReadyToStun::Update( CBotNPCMinion *me, float interval )
{
	CTFPlayer *target = me->GetTarget();

	if ( target )
	{
		me->GetLocomotionInterface()->FaceTowards( target->WorldSpaceCenter() );

		if ( me->IsRangeLessThan( target, tf_bot_npc_minion_stun_range.GetFloat() ) )
		{
			return SuspendFor( new CBotNPCMinionStartStunAttack, "Charging stun..." );
		}
	}

	return Continue();
}



//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCMinionApproachTarget : public Action< CBotNPCMinion >
{
public:
	virtual Action< CBotNPCMinion > *InitialContainedAction( CBotNPCMinion *me );

	virtual ActionResult< CBotNPCMinion >	OnStart( CBotNPCMinion *me, Action< CBotNPCMinion > *priorAction );
	virtual ActionResult< CBotNPCMinion >	Update( CBotNPCMinion *me, float interval );
	virtual void OnEnd( CBotNPCMinion *me, Action< CBotNPCMinion > *nextAction );

	virtual const char *GetName( void ) const	{ return "ApproachTarget"; }		// return name of this action

private:
	CTFPathFollower m_path;
	CountdownTimer m_initialStunTimer;
	CountdownTimer m_chooseTargetTimer;
	CountdownTimer m_pingTimer;
};


//---------------------------------------------------------------------------------------------
Action< CBotNPCMinion > *CBotNPCMinionApproachTarget::InitialContainedAction( CBotNPCMinion *me )
{
	return new CBotNPCMinionReadyToStun;
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPCMinion > CBotNPCMinionApproachTarget::OnStart( CBotNPCMinion *me, Action< CBotNPCMinion > *priorAction )
{
	if ( !me->HasTarget() )
	{
		return Done( "No initial target" );
	}

	m_pingTimer.Start( 1.0f );

	// light on
	me->m_nSkin = MINION_LIGHT_ON;

	me->EmitSound( "Minion.Recognize" );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPCMinion > CBotNPCMinionApproachTarget::Update( CBotNPCMinion *me, float interval )
{
	if ( !me->HasTarget() )
	{
		me->GetLocomotionInterface()->FaceTowards( me->GetLastKnownTargetPosition() );

		if ( ( me->GetAbsOrigin() - me->GetLastKnownTargetPosition() ).AsVector2D().IsLengthLessThan( 20.0f ) )
		{
			// reached last know position of threat - we lost them
			return Done( "Lost our target" );
		}

		me->EndStunEffects();
	}

	// approach
	if ( !m_path.IsValid() || m_path.GetAge() > 1.0f )
	{
		ShortestPathCost cost;
		m_path.Compute( me, me->GetLastKnownTargetPosition(), cost );
	}

	me->GetLocomotionInterface()->SetDesiredSpeed( tf_bot_npc_minion_speed.GetFloat() );

	if ( me->IsLineOfSightClear( me->GetLastKnownTargetPosition() ) )
	{
		// move directly toward our goal
		me->GetLocomotionInterface()->Approach( me->GetLastKnownTargetPosition() );
	}
	else
	{
		m_path.Update( me );
	}

	if ( m_pingTimer.IsElapsed() )
	{
		m_pingTimer.Reset();
		me->EmitSound( "Minion.Ping.Acquire" );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CBotNPCMinionApproachTarget::OnEnd( CBotNPCMinion *me, Action< CBotNPCMinion > *nextAction )
{
	me->EndStunEffects();
	me->m_nSkin = MINION_LIGHT_OFF;
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCMinionNotice : public Action< CBotNPCMinion >
{
public:
	virtual ActionResult< CBotNPCMinion > OnStart( CBotNPCMinion *me, Action< CBotNPCMinion > *priorAction );
	virtual ActionResult< CBotNPCMinion > Update( CBotNPCMinion *me, float interval );

	virtual const char *GetName( void ) const	{ return "Notice"; }		// return name of this action

private:
	CountdownTimer m_timer;
};

ConVar tf_bot_npc_minion_notice_duration( "tf_bot_npc_minion_notice_duration", "1"/*, FCVAR_CHEAT */ );


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPCMinion > CBotNPCMinionNotice::OnStart( CBotNPCMinion *me, Action< CBotNPCMinion > *priorAction )
{
	m_timer.Start( tf_bot_npc_minion_notice_duration.GetFloat() );

	me->BecomeAlert();

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPCMinion > CBotNPCMinionNotice::Update( CBotNPCMinion *me, float interval )
{
	if ( me->HasTarget() )
	{
		me->GetLocomotionInterface()->FaceTowards( me->GetTarget()->WorldSpaceCenter() );
	}

	if ( m_timer.IsElapsed() )
	{
		return ChangeTo( new CBotNPCMinionApproachTarget, "Acquiring target" );
	}

	return Continue();
}



//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCMinionRoam : public Action< CBotNPCMinion >
{
public:
	virtual ActionResult< CBotNPCMinion > OnStart( CBotNPCMinion *me, Action< CBotNPCMinion > *priorAction );
	virtual ActionResult< CBotNPCMinion > Update( CBotNPCMinion *me, float interval );

	virtual const char *GetName( void ) const	{ return "Roam"; }		// return name of this action

private:
	CountdownTimer m_pingTimer;
	CTFNavArea *m_goalArea;
	CountdownTimer m_waitTimer;

	void UpdateGoal( CBotNPCMinion *me );
};


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPCMinion > CBotNPCMinionRoam::OnStart( CBotNPCMinion *me, Action< CBotNPCMinion > *priorAction )
{
	m_goalArea = NULL;
	m_pingTimer.Invalidate();
	m_waitTimer.Invalidate();

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPCMinion > CBotNPCMinionRoam::Update( CBotNPCMinion *me, float interval )
{
	if ( me->HasTarget() )
	{
		return SuspendFor( new CBotNPCMinionNotice, "Target found..." );
	}

	// light off
	me->m_nSkin = MINION_LIGHT_OFF;

	if ( m_goalArea == NULL || 
		 me->GetLastKnownArea() == m_goalArea || 
		 m_goalArea->IsOverlapping( me->WorldSpaceCenter() ) ||
		 ( me->WorldSpaceCenter() - m_goalArea->GetCenter() ).AsVector2D().IsLengthLessThan( 20.0f ) )
	{
		// at goal
		if ( m_waitTimer.HasStarted() )
		{
			if ( m_waitTimer.IsElapsed() )
			{
				// time to find a new goal
				UpdateGoal( me );
				m_waitTimer.Invalidate();
			}
		}
		else
		{
			m_waitTimer.Start( RandomFloat( 3.0f, 5.0f ) );
		}
	}

	if ( m_goalArea )
	{
		me->GetLocomotionInterface()->SetDesiredSpeed( tf_bot_npc_minion_speed.GetFloat() );
		me->GetLocomotionInterface()->Approach( m_goalArea->GetCenter() );

		if ( tf_bot_npc_minion_debug.GetBool() )
		{
			NDebugOverlay::Line( me->WorldSpaceCenter(), m_goalArea->GetCenter(), 0, 255, 0, true, 0.1f );
		}
	}

	if ( m_pingTimer.IsElapsed() )
	{
		m_pingTimer.Start( RandomFloat( 0.9f, 1.1f ) * 3.0f );
		me->EmitSound( "Minion.Ping.Roam" );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CBotNPCMinionRoam::UpdateGoal( CBotNPCMinion *me )
{
	CNavArea *myArea = me->GetLastKnownArea();

	if ( myArea )
	{
		CUtlVector< CNavArea * > adjVector;
		myArea->CollectAdjacentAreas( &adjVector );

		if ( adjVector.Count() > 0 )
		{
			m_goalArea = (CTFNavArea *)adjVector[ RandomInt( 0, adjVector.Count()-1 ) ];
		}
	}
	else
	{
		CBaseCombatCharacter *boss = me->GetOwnerEntity() ? me->GetOwnerEntity()->MyCombatCharacterPointer() : NULL;
		if ( boss )
		{
			m_goalArea = (CTFNavArea *)boss->GetLastKnownArea();
		}
	}
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCMinionIdle : public Action< CBotNPCMinion >
{
public:
	virtual ActionResult< CBotNPCMinion > OnStart( CBotNPCMinion *me, Action< CBotNPCMinion > *priorAction );
	virtual ActionResult< CBotNPCMinion > Update( CBotNPCMinion *me, float interval );

	virtual const char *GetName( void ) const	{ return "Idle"; }		// return name of this action

private:
	CountdownTimer m_pingTimer;
};


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPCMinion > CBotNPCMinionIdle::OnStart( CBotNPCMinion *me, Action< CBotNPCMinion > *priorAction )
{
	m_pingTimer.Start( 3.0f );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPCMinion > CBotNPCMinionIdle::Update( CBotNPCMinion *me, float interval )
{
	if ( me->HasTarget() )
	{
		return SuspendFor( new CBotNPCMinionNotice, "Target found..." );
	}

	// light off
	me->m_nSkin = MINION_LIGHT_OFF;

	if ( m_pingTimer.IsElapsed() )
	{
		m_pingTimer.Reset();
		me->EmitSound( "Minion.Ping.Roam" );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
ConVar tf_bot_npc_minion_init_vel( "tf_bot_npc_minion_init_vel", "250"/*, FCVAR_CHEAT*/ );


class CBotNPCMinionBehavior : public Action< CBotNPCMinion >
{
public:
	virtual Action< CBotNPCMinion > *InitialContainedAction( CBotNPCMinion *me )	
	{
		if ( TFGameRules()->GetActiveBoss() )
		{
			return new CBotNPCMinionRoam;
		}

		// minions in the wild just hover in place
		return new CBotNPCMinionIdle;
	}

	virtual ActionResult< CBotNPCMinion > OnStart( CBotNPCMinion *me, Action< CBotNPCMinion > *priorAction )
	{ 
		Vector initVelocity;

		float s, c;
		SinCos( RandomFloat( -M_PI, M_PI ), &s, &c );

		initVelocity.x = c * tf_bot_npc_minion_init_vel.GetFloat();
		initVelocity.y = s * tf_bot_npc_minion_init_vel.GetFloat();
		initVelocity.z = 0.0f;

		static_cast< CNextBotFlyingLocomotion * >( me->GetLocomotionInterface() )->SetVelocity( initVelocity );

		return Continue(); 
	}

	virtual ActionResult< CBotNPCMinion > Update( CBotNPCMinion *me, float interval )
	{
		me->UpdateTarget();

		return Continue();
	}

	virtual EventDesiredResult< CBotNPCMinion > OnKilled( CBotNPCMinion *me, const CTakeDamageInfo &info )
	{ 
		me->BecomeAmmoPack();

		DispatchParticleEffect( "asplode_hoodoo_embers", me->GetAbsOrigin(), me->GetAbsAngles() );
		me->EmitSound( "Minion.Explode" );
		UTIL_Remove( me );

		return TryDone();
	}

	virtual const char *GetName( void ) const	{ return "Behavior"; }		// return name of this action
};


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
IMPLEMENT_INTENTION_INTERFACE( CBotNPCMinion, CBotNPCMinionBehavior );


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
CNextBotFlyingLocomotion::CNextBotFlyingLocomotion( INextBot *bot ) : ILocomotion( bot )
{
	Reset();
}


//---------------------------------------------------------------------------------------------
CNextBotFlyingLocomotion::~CNextBotFlyingLocomotion()
{
}


//---------------------------------------------------------------------------------------------
// (EXTEND) reset to initial state
void CNextBotFlyingLocomotion::Reset( void )
{
	m_velocity = vec3_origin;
	m_acceleration = vec3_origin;
	m_desiredSpeed = 0.0f;
	m_currentSpeed = 0.0f;
	m_forward = vec3_origin;
	m_desiredAltitude = 50.0f;
}


//---------------------------------------------------------------------------------------------
void CNextBotFlyingLocomotion::MaintainAltitude( void )
{
	CBaseCombatCharacter *me = GetBot()->GetEntity();

	trace_t result;
	//CTraceFilterSimple filter( me, COLLISION_GROUP_NONE );
	CTraceFilterSimpleClassnameList filter( me, COLLISION_GROUP_NONE );
	filter.AddClassnameToIgnore( "bot_npc_minion" );

	// find ceiling
	TraceHull( me->GetAbsOrigin(), me->GetAbsOrigin() + Vector( 0, 0, 1000.0f ), 
			   me->WorldAlignMins(), me->WorldAlignMaxs(), 
			   GetBot()->GetBodyInterface()->GetSolidMask(), &filter, &result );

	float ceiling = result.endpos.z - me->GetAbsOrigin().z;

	// trace wider hull to account for nearby ledges we want to float over
	TraceHull( me->GetAbsOrigin() + Vector( 0, 0, ceiling ),
			   me->GetAbsOrigin() + Vector( 0, 0, -1000.0f ), 
			   Vector( 2.0f * me->WorldAlignMins().x, 2.0f * me->WorldAlignMins().y, me->WorldAlignMins().z ), 
			   Vector( 2.0f * me->WorldAlignMaxs().x, 2.0f * me->WorldAlignMaxs().y, me->WorldAlignMaxs().z ), 
			   GetBot()->GetBodyInterface()->GetSolidMask(), &filter, &result );

	float groundZ = result.endpos.z;

	float currentAltitude = me->GetAbsOrigin().z - groundZ;
	float error = m_desiredAltitude - currentAltitude;
	float accelZ = clamp( error, -tf_bot_npc_minion_acceleration.GetFloat(), tf_bot_npc_minion_acceleration.GetFloat() );

	m_acceleration.z += accelZ;
}


ConVar tf_bot_npc_minion_avoid_range( "tf_bot_npc_minion_avoid_range", "100"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_minion_avoid_force( "tf_bot_npc_minion_avoid_force", "100"/*, FCVAR_CHEAT*/ );


//---------------------------------------------------------------------------------------------
// (EXTEND) update internal state
void CNextBotFlyingLocomotion::Update( void )
{
	CBaseCombatCharacter *me = GetBot()->GetEntity();
	const float deltaT = GetUpdateInterval();

	Vector pos = me->GetAbsOrigin();

	// always maintain altitude, even if not trying to move (ie: no Approach call)
	MaintainAltitude();

	m_forward = m_velocity;
	m_currentSpeed = m_forward.NormalizeInPlace();

	Vector damping( tf_bot_npc_minion_horiz_damping.GetFloat(), tf_bot_npc_minion_horiz_damping.GetFloat(), tf_bot_npc_minion_vert_damping.GetFloat() );
	Vector totalAccel = m_acceleration - m_velocity * damping;

	// avoid other minions
	CBaseEntity *minion = NULL;
	while( ( minion = gEntList.FindEntityByClassname( minion, "bot_npc_minion" ) ) != NULL )
	{
		if ( me == minion )
			continue;

		Vector toPeer = minion->GetAbsOrigin() - me->GetAbsOrigin();
		toPeer.z = 0.0f;
		float range = toPeer.NormalizeInPlace();

		if ( range < tf_bot_npc_minion_avoid_range.GetFloat() )
		{
			totalAccel += -tf_bot_npc_minion_avoid_force.GetFloat() * toPeer;
		}
	}

	m_velocity += totalAccel * deltaT;
	me->SetAbsVelocity( m_velocity );

	pos += m_velocity * deltaT;

	// check for collisions along move	
	trace_t result;
	CTraceFilterSkipClassname filter( me, "bot_npc_minion", COLLISION_GROUP_NONE );
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
			me->EmitSound( "Minion.Bounce" );
		}
	}

	GetBot()->GetEntity()->SetAbsOrigin( result.endpos );

	m_acceleration = vec3_origin;
}


//---------------------------------------------------------------------------------------------
// (EXTEND) move directly towards the given position
void CNextBotFlyingLocomotion::Approach( const Vector &goalPos, float goalWeight )
{
	Vector flyGoal = goalPos;
	flyGoal.z += m_desiredAltitude;

	Vector toGoal = flyGoal - GetBot()->GetEntity()->GetAbsOrigin();
	// altitude is handled in Update()
	toGoal.z = 0.0f;
	toGoal.NormalizeInPlace();

	m_acceleration += tf_bot_npc_minion_acceleration.GetFloat() * toGoal;
}


//---------------------------------------------------------------------------------------------
void CNextBotFlyingLocomotion::SetDesiredSpeed( float speed )
{
	m_desiredSpeed = speed;
}


//---------------------------------------------------------------------------------------------
float CNextBotFlyingLocomotion::GetDesiredSpeed( void ) const
{
	return m_desiredSpeed;
}


//---------------------------------------------------------------------------------------------
void CNextBotFlyingLocomotion::SetDesiredAltitude( float height )
{
	m_desiredAltitude = height;
}


//---------------------------------------------------------------------------------------------
float CNextBotFlyingLocomotion::GetDesiredAltitude( void ) const
{
	return m_desiredAltitude;
}


ConVar tf_bot_npc_minion_deflect_range( "tf_bot_npc_minion_deflect_range", "300"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_minion_deflect_force( "tf_bot_npc_minion_deflect_force", "2000"/*, FCVAR_CHEAT*/ );


//---------------------------------------------------------------------------------------------
void CNextBotFlyingLocomotion::Deflect( CBaseEntity *deflector )
{
	if ( deflector )
	{
		Vector fromDeflector = GetBot()->GetEntity()->WorldSpaceCenter() - deflector->EyePosition();
		float range = fromDeflector.NormalizeInPlace();

		if ( range < tf_bot_npc_minion_deflect_range.GetFloat() )
		{
			m_velocity += ( 1.0f - ( range / tf_bot_npc_minion_deflect_range.GetFloat() ) ) * tf_bot_npc_minion_deflect_force.GetFloat() * fromDeflector;
		}
	}
}

#endif // TF_RAID_MODE
