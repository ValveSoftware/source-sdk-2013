//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: static_prop - don't move, don't animate, don't do anything.
//			physics_prop - move, take damage, but don't animate
//
//===========================================================================//


#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_props.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tf_soccer_ball_up_max( "tf_soccer_ball_up_max", "350", FCVAR_CHEAT );
ConVar tf_soccer_ball_multiplier( "tf_soccer_ball_multiplier", "4", FCVAR_CHEAT );
ConVar tf_soccer_ball_min_speed( "tf_soccer_ball_min_speed", "30", FCVAR_CHEAT );

// Ranges from ~ .3 (side swipe) to 1 (full head on hit). When it's >= this value,
//  treat the hit as a full frontal 1.0 collision.
ConVar tf_soccer_front_hit_range( "tf_soccer_front_hit_range", ".95", FCVAR_CHEAT );

extern ConVar tf_halloween_kart_dash_speed;
extern ConVar tf_halloween_kart_normal_speed;


LINK_ENTITY_TO_CLASS( prop_soccer_ball, CPropSoccerBall );

BEGIN_DATADESC( CPropSoccerBall )
	DEFINE_KEYFIELD( m_iszTriggers, FIELD_STRING, "trigger_name" ),
END_DATADESC()


void CPropSoccerBall::Precache()
{
	PrecacheScriptSound( "BumperCar.HitBall" );
}

void CPropSoccerBall::Spawn()
{
	BaseClass::Spawn();

	SetSolid( SOLID_VPHYSICS );		// Use our vphys model for collision
	SetSolidFlags( FSOLID_TRIGGER ); // Generate Touch functions, but dont collide
	SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS ); // Need this one too so players dont get stopped

	SetTouch( &CPropSoccerBall::BallTouch );
	SetContextThink( &CPropSoccerBall::TriggerTouchThink, gpGlobals->curtime + 0.2f, "TriggerTouchThink" );
}

// Here's the deal.  The ball is a trigger, but triggers are not allowed to touch other triggers.  To get around this,
// we're going to specify the names of the triggers we actually want to touch and then we're going to manually try to 
// touch them.  Our collision system is a vortex of insanity.
void CPropSoccerBall::TriggerTouchThink()
{
	FOR_EACH_VEC( m_vecTriggers, i )
	{
		if ( m_vecTriggers[i]->PointIsWithin( GetAbsOrigin() ) )
		{
			m_vecTriggers[i]->StartTouch( this );
			m_vecTriggers[i]->EndTouch( this );
		}
	}

	SetContextThink( &CPropSoccerBall::TriggerTouchThink, gpGlobals->curtime + 0.2f, "TriggerTouchThink" );
}

void CPropSoccerBall::Activate()
{
	CBaseTrigger* pTrigger = NULL;
	do
	{
		pTrigger = dynamic_cast<CBaseTrigger *> ( gEntList.FindEntityByName( pTrigger, m_iszTriggers.ToCStr() ) );
		if ( pTrigger )
		{
			m_vecTriggers.AddToTail( pTrigger );
		}
	} while ( pTrigger );

	BaseClass::Activate();
}

bool CPropSoccerBall::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	TestHitboxes( ray, 0xFFFFFFFF, trace );

	if ( trace.DidHit() )
	{
		IPhysicsObject *pObj = VPhysicsGetObject();
		if ( pObj )
			pObj->Wake();
	}

	return false;
}

void CPropSoccerBall::BallTouch( CBaseEntity *pOther )
{
	if ( gpGlobals->curtime < m_flNextAllowedImpactTime )
		return;

	CTFPlayer *pTFPlayer = ToTFPlayer( pOther );
	if ( pTFPlayer )
	{
		const float flSoccerBallMultiplier = tf_soccer_ball_multiplier.GetFloat();

		// Get player direction and speed.
		Vector vPlayer( pOther->GetAbsVelocity().x, pOther->GetAbsVelocity().y, 0.0f );
		float flSpeed = vPlayer.Length2D();
		vPlayer.NormalizeInPlace();

		// Linearly scale up based on kart speed.
		float fUp = tf_soccer_ball_up_max.GetFloat();
		float fUpScale = flSpeed / tf_halloween_kart_dash_speed.GetFloat();
		fUp = Clamp( fUp * fUpScale, 5.0f, fUp ) * flSoccerBallMultiplier;

		// Get vector to ball from player.
		Vector vToBall = GetAbsOrigin() - pOther->GetAbsOrigin();
		vToBall.z = 0.0f;
		vToBall.NormalizeInPlace();

		// cosTheta ranges from about .3 (side swipe) to 1 (full head on hit).
		float cosTheta = Max( 0.1f, vToBall.Dot( vPlayer ) );

		// Scale speed based on incident angle and soccer ball multiplier hack.
		flSpeed = Max( flSpeed * cosTheta, tf_soccer_ball_min_speed.GetFloat() );
		flSpeed *= flSoccerBallMultiplier;

		Vector vecVelocity;
		if ( cosTheta >= tf_soccer_front_hit_range.GetFloat() )
		{
			// Front hit - snag player velocity and direction and use that.
			//DevMsg( "%s cosTheta: %.2f front hit\n", __FUNCTION__, cosTheta );
			vecVelocity = vPlayer;
		}
		else
		{
			// Side swipe. Scale vector by player speed and hit angle.
			//DevMsg( "%s cosTheta: %.2f side hit\n", __FUNCTION__, cosTheta );
			vecVelocity = vToBall;
		}

		vecVelocity *= flSpeed;
		vecVelocity.z = fUp;

		IPhysicsObject *pObj = VPhysicsGetObject();
		pObj->Wake();
		pObj->AddVelocity( &vecVelocity, NULL );
		m_flNextAllowedImpactTime = gpGlobals->curtime + 0.1f;

		EmitSound( "BumperCar.HitBall" );

		ChangeTeam( pTFPlayer->GetTeamNumber() );
		m_hLastToucher = pTFPlayer;
	}
}

