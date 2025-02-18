//========= Copyright Valve Corporation, All rights reserved. ============//
// bot_npc_decoy.cpp
// A NextBot non-player decoy that imitates a real player
// Michael Booth, January 2011

#include "cbase.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "nav_mesh/tf_nav_area.h"
#include "bot_npc_decoy.h"
#include "econ_wearable.h"

LINK_ENTITY_TO_CLASS( bot_npc_decoy, CBotNPCDecoy );
PRECACHE_REGISTER( bot_npc_decoy );

ConVar tf_decoy_lifetime( "tf_decoy_lifetime", "5", FCVAR_CHEAT, "The lifetime of a decoy, in seconds" );


//-----------------------------------------------------------------------------------------------------
CBotNPCDecoy::CBotNPCDecoy()
{
	ALLOCATE_INTENTION_INTERFACE( CBotNPCDecoy );

	m_locomotor = new CBotNPCDecoyLocomotion( this );
	m_body = new CBotNPCBody( this );

	m_eyeOffset = vec3_origin;
}


//-----------------------------------------------------------------------------------------------------
CBotNPCDecoy::~CBotNPCDecoy()
{
	DEALLOCATE_INTENTION_INTERFACE;

	if ( m_locomotor )
		delete m_locomotor;

	if ( m_body )
		delete m_body;
}

//-----------------------------------------------------------------------------------------------------
void CBotNPCDecoy::Precache()
{
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------------------------------
void CBotNPCDecoy::Spawn( void )
{
	BaseClass::Spawn();

	SetCollisionGroup( COLLISION_GROUP_NONE );
	SetSolid( SOLID_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );

	CTFPlayer *owner = ToTFPlayer( GetOwnerEntity() );
	if ( !owner )
	{
		Warning( "Decoy spawned without an owner\n" );
		return;
	}

	int ownerClass = owner->m_Shared.InCond( TF_COND_DISGUISED ) ? owner->m_Shared.GetDisguiseClass() : owner->GetPlayerClass()->GetClassIndex();
	int ownerTeam = owner->m_Shared.InCond( TF_COND_DISGUISED ) ? owner->m_Shared.GetDisguiseTeam() : owner->GetTeamNumber();

	SetModel( GetPlayerClassData( ownerClass )->m_szModelName );
	ChangeTeam( ownerTeam );

	if ( ownerTeam == TF_TEAM_BLUE )
	{
		m_nSkin = 1;
	}
	else
	{
		m_nSkin = 0;
	}

	SetAbsOrigin( owner->GetAbsOrigin() );
	SetAbsAngles( owner->GetAbsAngles() );
	SetAbsVelocity( owner->GetAbsVelocity() );

	Vector headPos;
	QAngle headAngles;
	if ( GetAttachment( "head", headPos, headAngles ) )
	{
		m_eyeOffset = headPos - GetAbsOrigin();
	}

	CTFWeaponBase *theirWeapon = owner->m_Shared.GetDisguiseWeapon();
	if ( !theirWeapon )
	{
		theirWeapon = owner->GetActiveTFWeapon();
	}

	if ( theirWeapon )
	{
		CBaseAnimating *weapon = (CBaseAnimating *)CreateEntityByName( "prop_dynamic" );
		if ( weapon )
		{
			weapon->SetModel( theirWeapon->GetWorldModel() );

			// bonemerge the weapon into our model
			weapon->FollowEntity( this, true );

			// choose the appropriate run animation for this weapon
			switch( theirWeapon->GetTFWpnData().m_iWeaponType )
			{
			case TF_WPN_TYPE_PRIMARY:
				m_runActivity = ACT_MP_RUN_PRIMARY;
				break;

			case TF_WPN_TYPE_SECONDARY:
				m_runActivity = ACT_MP_RUN_SECONDARY;
				break;

			case TF_WPN_TYPE_MELEE:
			default:
				m_runActivity = ACT_MP_RUN_MELEE;
				break;
			}
		}
	}
}


//---------------------------------------------------------------------------------------------
unsigned int CBotNPCDecoy::PhysicsSolidMaskForEntity( void ) const
{ 
	// Only collide with the other team
	int teamContents = ( GetTeamNumber() == TF_TEAM_RED ) ? CONTENTS_BLUETEAM : CONTENTS_REDTEAM;

	return BaseClass::PhysicsSolidMaskForEntity() | teamContents;
}


//---------------------------------------------------------------------------------------------
bool CBotNPCDecoy::ShouldCollide( int collisionGroup, int contentsMask ) const
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
float CBotNPCDecoyLocomotion::GetRunSpeed( void ) const
{
	CTFPlayer *owner = ToTFPlayer( GetBot()->GetEntity()->GetOwnerEntity() );
	if ( !owner )
	{
		return 0.0f;
	}

	int ownerClass = owner->m_Shared.InCond( TF_COND_DISGUISED ) ? owner->m_Shared.GetDisguiseClass() : owner->GetPlayerClass()->GetClassIndex();
	return GetPlayerClassData( ownerClass )->m_flMaxSpeed;
}


//---------------------------------------------------------------------------------------------
// return maximum acceleration of locomotor
float CBotNPCDecoyLocomotion::GetMaxAcceleration( void ) const
{
	return 1500.0f;
}


//---------------------------------------------------------------------------------------------
// return maximum deceleration of locomotor
float CBotNPCDecoyLocomotion::GetMaxDeceleration( void ) const
{
	return 1500.0f;
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCDecoyBehavior : public Action< CBotNPCDecoy >
{
public:
	virtual ActionResult< CBotNPCDecoy > OnStart( CBotNPCDecoy *me, Action< CBotNPCDecoy > *priorAction )
	{
		m_timer.Start( tf_decoy_lifetime.GetFloat() );

		// play running animation
		if ( !me->GetBodyInterface()->IsActivity( me->GetRunActivity() ) )
		{
			me->GetBodyInterface()->StartActivity( me->GetRunActivity() );
		}

		return Continue(); 
	}

	virtual ActionResult< CBotNPCDecoy > Update( CBotNPCDecoy *me, float interval )
	{
		if ( m_timer.IsElapsed() )
		{
			// we're out of time
			UTIL_Remove( me );
			return Done( "Lifetime expired" );
		}

		CTFPlayer *owner = ToTFPlayer( me->GetOwnerEntity() );
		if ( !owner )
		{
			UTIL_Remove( me );
			return Done( "No owner!" );
		}

		Vector forward;
		me->GetVectors( &forward, NULL, NULL );

		me->GetLocomotionInterface()->SetDesiredSpeed( FLT_MAX );	// this is just a rate limiter
		me->GetLocomotionInterface()->Run();
		me->GetLocomotionInterface()->Approach( me->GetAbsOrigin() + 100.0f * forward );

		return Continue();
	}

	virtual const char *GetName( void ) const	{ return "Behavior"; }		// return name of this action

private:
	CountdownTimer m_timer;
};


IMPLEMENT_INTENTION_INTERFACE( CBotNPCDecoy, CBotNPCDecoyBehavior );

