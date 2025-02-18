//========= Copyright Valve Corporation, All rights reserved. ============//
// bot_npc_archer.cpp
// A NextBot non-player derived archer
// Michael Booth, November 2010

#include "cbase.h"

#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "tf_projectile_arrow.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "nav_mesh/tf_nav_area.h"
#include "bot_npc_archer.h"
#include "NextBot/Path/NextBotChasePath.h"
#include "econ_wearable.h"
#include "team_control_point_master.h"
#include "particle_parse.h"
#include "CRagdollMagnet.h"
#include "NextBot/Behavior/BehaviorMoveTo.h"

ConVar tf_bot_npc_archer_health( "tf_bot_npc_archer_health", "100", FCVAR_CHEAT );

ConVar tf_bot_npc_archer_speed( "tf_bot_npc_archer_speed", "100", FCVAR_CHEAT );

ConVar tf_bot_npc_archer_shoot_interval( "tf_bot_npc_archer_shoot_interval", "2", FCVAR_CHEAT ); // 2
ConVar tf_bot_npc_archer_arrow_damage( "tf_bot_npc_archer_arrow_damage", "75", FCVAR_CHEAT );


//-----------------------------------------------------------------
// The Bot NPC
//-----------------------------------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( bot_npc_archer, CBotNPCArcher );

PRECACHE_REGISTER( bot_npc_archer );


//-----------------------------------------------------------------------------------------------------
CBotNPCArcher::CBotNPCArcher()
{
	ALLOCATE_INTENTION_INTERFACE( CBotNPCArcher );

	m_locomotor = new NextBotGroundLocomotion( this );
	m_body = new CBotNPCBody( this );

	m_eyeOffset = vec3_origin;
	m_homePos = vec3_origin;
}


//-----------------------------------------------------------------------------------------------------
CBotNPCArcher::~CBotNPCArcher()
{
	DEALLOCATE_INTENTION_INTERFACE;

	if ( m_locomotor )
		delete m_locomotor;

	if ( m_body )
		delete m_body;
}

//-----------------------------------------------------------------------------------------------------
void CBotNPCArcher::Precache()
{
	BaseClass::Precache();

	PrecacheModel( "models/player/sniper.mdl" );
	PrecacheModel( "models/weapons/c_models/c_bow/c_bow.mdl" );
}


//-----------------------------------------------------------------------------------------------------
void CBotNPCArcher::Spawn( void )
{
	BaseClass::Spawn();

	SetModel( "models/player/sniper.mdl" );

	m_bow = (CBaseAnimating *)CreateEntityByName( "prop_dynamic" );
	if ( m_bow )
	{
		m_bow->SetModel( "models/weapons/c_models/c_bow/c_bow.mdl" );

		// bonemerge into our model
		m_bow->FollowEntity( this, true );
	}

	int health = tf_bot_npc_archer_health.GetInt();
	SetHealth( health );
	SetMaxHealth( health );

	ChangeTeam( TF_TEAM_RED );

	Vector headPos;
	QAngle headAngles;
	if ( GetAttachment( "head", headPos, headAngles ) )
	{
		m_eyeOffset = headPos - GetAbsOrigin();
	}

	m_homePos = GetAbsOrigin();
}


//---------------------------------------------------------------------------------------------
unsigned int CBotNPCArcher::PhysicsSolidMaskForEntity( void ) const
{ 
	// Only collide with the other team
	int teamContents = ( GetTeamNumber() == TF_TEAM_RED ) ? CONTENTS_BLUETEAM : CONTENTS_REDTEAM;

	return BaseClass::PhysicsSolidMaskForEntity() | teamContents;
}


//---------------------------------------------------------------------------------------------
bool CBotNPCArcher::ShouldCollide( int collisionGroup, int contentsMask ) const
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
//---------------------------------------------------------------------------------------------
class CBotNPCArcherSurrender : public Action< CBotNPCArcher >
{
public:
	virtual ActionResult< CBotNPCArcher >	OnStart( CBotNPCArcher *me, Action< CBotNPCArcher > *priorAction );
	virtual const char *GetName( void ) const	{ return "Surrender"; }		// return name of this action
};


inline ActionResult< CBotNPCArcher > CBotNPCArcherSurrender::OnStart( CBotNPCArcher *me, Action< CBotNPCArcher > *priorAction )
{
	CBaseAnimating *bow = me->GetBow();
	if ( bow )
	{
		bow->AddEffects( EF_NODRAW );
	}
	
	me->GetBodyInterface()->StartActivity( ACT_MP_STAND_LOSERSTATE );

	return Continue();
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCArcherShootBow : public Action< CBotNPCArcher >
{
public:
	CBotNPCArcherShootBow( CTFPlayer *target )
	{
		m_target = target;
	}

	virtual ActionResult< CBotNPCArcher >	OnStart( CBotNPCArcher *me, Action< CBotNPCArcher > *priorAction );
	virtual ActionResult< CBotNPCArcher >	Update( CBotNPCArcher *me, float interval );

	virtual const char *GetName( void ) const	{ return "ShootBow"; }		// return name of this action

private:
	CHandle< CTFPlayer > m_target;
};


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPCArcher >	CBotNPCArcherShootBow::OnStart( CBotNPCArcher *me, Action< CBotNPCArcher > *priorAction )
{
	if ( !m_target )
	{
		return Done( "No target" );
	}

	me->GetLocomotionInterface()->FaceTowards( m_target->WorldSpaceCenter() );
	me->AddGesture( ACT_MP_ATTACK_STAND_ITEM2 );

	// fire arrow
	const float arrowSpeed = 2000.0f;
	const float arrowGravity = 0.2f;

	Vector muzzleOrigin;
	QAngle muzzleAngles;
	if ( me->GetBow()->GetAttachment( "muzzle", muzzleOrigin, muzzleAngles ) == false )
	{
		return Done( "No muzzle attachment!" );
	}

	// lead target
	float range = me->GetRangeTo( m_target->EyePosition() );
	float flightTime = range / arrowSpeed;

	Vector aimSpot = m_target->EyePosition() + m_target->GetAbsVelocity() * flightTime;

	Vector to = aimSpot - muzzleOrigin;
	VectorAngles( to, muzzleAngles );

	CTFProjectile_Arrow *arrow = CTFProjectile_Arrow::Create( muzzleOrigin, muzzleAngles, arrowSpeed, arrowGravity, TF_PROJECTILE_ARROW, me, me );
	if ( arrow )
	{
		arrow->SetLauncher( me );
		arrow->SetCritical( false );

		arrow->SetDamage( tf_bot_npc_archer_arrow_damage.GetFloat() );

		me->EmitSound( "Weapon_CompoundBow.Single" );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPCArcher >	CBotNPCArcherShootBow::Update( CBotNPCArcher *me, float interval )
{
	if ( me->IsSequenceFinished() )
	{
		return Done();
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCArcherGuardSpot : public Action< CBotNPCArcher >
{
public:
	virtual ActionResult< CBotNPCArcher >	OnStart( CBotNPCArcher *me, Action< CBotNPCArcher > *priorAction )
	{
		me->GetBodyInterface()->StartActivity( ACT_MP_STAND_ITEM2 );

		return Continue();
	}

	CTFPlayer *GetVictim( CBotNPCArcher *me )
	{
		CUtlVector< CTFPlayer * > playerVector;
		CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS );

		CTFPlayer *closeVictim = NULL;
		float victimRangeSq = FLT_MAX;

		for( int i=0; i<playerVector.Count(); ++i )
		{
			float rangeSq = me->GetRangeSquaredTo( playerVector[i] );
			if ( rangeSq < victimRangeSq )
			{
				if ( playerVector[i]->m_Shared.IsStealthed() )
				{
					continue;
				}

				if ( me->IsLineOfSightClear( playerVector[i] ) )
				{
					closeVictim = playerVector[i];
					victimRangeSq = rangeSq;
				}
			}
		}

		return closeVictim;
	}

	virtual ActionResult< CBotNPCArcher >	Update( CBotNPCArcher *me, float interval )
	{
		if ( TFGameRules()->GetActiveBoss() == NULL )
		{
			// the Boss has been defeated - give up
			return ChangeTo( new CBotNPCArcherSurrender, "The Boss is dead! I give up!" );
		}

		CTFPlayer *victim = GetVictim( me );

		if ( victim )
		{
			// look at visible victim out of range
			me->GetLocomotionInterface()->FaceTowards( victim->WorldSpaceCenter() );

			if ( m_shootTimer.IsElapsed() )
			{
				m_shootTimer.Start( tf_bot_npc_archer_shoot_interval.GetFloat() );

				return SuspendFor( new CBotNPCArcherShootBow( victim ), "Fire!" );
			}
		}

		if ( me->GetLocomotionInterface()->IsAttemptingToMove() )
		{
			// play running animation
			if ( !me->GetBodyInterface()->IsActivity( ACT_MP_DEPLOYED_IDLE_ITEM2 ) )
			{
				me->GetBodyInterface()->StartActivity( ACT_MP_DEPLOYED_IDLE_ITEM2 );
			}
		}
		else
		{
			// standing still
			if ( !me->GetBodyInterface()->IsActivity( ACT_MP_STAND_ITEM2 ) )
			{
				me->GetBodyInterface()->StartActivity( ACT_MP_STAND_ITEM2 );
			}
		}

		return Continue();
	}

	virtual const char *GetName( void ) const	{ return "GuardSpot"; }		// return name of this action

private:
	CountdownTimer m_shootTimer;
};


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCArcherMoveToMark : public Action< CBotNPCArcher >
{
public:
	virtual ActionResult< CBotNPCArcher >	OnStart( CBotNPCArcher *me, Action< CBotNPCArcher > *priorAction )
	{
		ShortestPathCost cost;
		m_path.Compute( me, me->GetHomePosition(), cost );

		me->GetBodyInterface()->StartActivity( ACT_MP_RUN_ITEM2 );

		return Continue();
	}

	virtual ActionResult< CBotNPCArcher >	Update( CBotNPCArcher *me, float interval )
	{
		m_path.Update( me );

		if ( !m_path.IsValid() )
		{
			return ChangeTo( new CBotNPCArcherGuardSpot, "Reached my mark" );
		}

		return Continue();
	}

	virtual const char *GetName( void ) const	{ return "MoveToMark"; }		// return name of this action

private:
	PathFollower m_path;
};

	
//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCArcherBehavior : public Action< CBotNPCArcher >
{
public:
	virtual Action< CBotNPCArcher > *InitialContainedAction( CBotNPCArcher *me )	
	{
		return new CBotNPCArcherMoveToMark;
	}

	virtual ActionResult< CBotNPCArcher > Update( CBotNPCArcher *me, float interval )
	{
		return Continue();
	}

	virtual EventDesiredResult< CBotNPCArcher > OnKilled( CBotNPCArcher *me, const CTakeDamageInfo &info )
	{ 
		// Calculate death force
		Vector forceVector = me->CalcDamageForceVector( info );

		// See if there's a ragdoll magnet that should influence our force.
		CRagdollMagnet *magnet = CRagdollMagnet::FindBestMagnet( me );
		if ( magnet )
		{
			forceVector += magnet->GetForceVector( me );
		}

		me->BecomeRagdoll( info, forceVector );

		return TryDone();
	}

	virtual const char *GetName( void ) const	{ return "Behavior"; }		// return name of this action
};


IMPLEMENT_INTENTION_INTERFACE( CBotNPCArcher, CBotNPCArcherBehavior );
