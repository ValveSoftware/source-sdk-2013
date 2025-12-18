//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_taunt.cpp
// Stand still and play a taunt animation
// Michael Booth, November 2009

#include "cbase.h"
#include "team.h"
#include "bot/tf_bot.h"
#include "bot/behavior/tf_bot_taunt.h"


ConVar tf_bot_taunt_stop_chance( "tf_bot_taunt_stop_chance", "30", FCVAR_NONE, "Percent chance a bot will stop taunting for looping/mimic taunts, checked every 5 seconds except in setup", true, 0.f, true, 100.f );


CTFBotTaunt::CTFBotTaunt( CTFPlayer *partner )
{
	m_partner = partner;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotTaunt::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	// wait a short random time so entire mob doesn't taunt in unison
	m_tauntTimer.Start( RandomFloat( 0, 1.f ) );
	m_didTaunt = false;
	m_currTauntTurnSpeed = 0.f;
	m_targetTauntYaw = 0.f;
	m_hasTauntYaw = false;

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotTaunt::Update( CTFBot *me, float interval )
{
	if ( m_tauntTimer.IsElapsed() )
	{
		const float tauntStopCheckInterval = 5.f;
		if ( m_didTaunt )
		{
			// Stop taunting when the timer is up
			if ( m_tauntEndTimer.HasStarted() && m_tauntEndTimer.IsElapsed() && me->m_Shared.GetTauntIndex() == TAUNT_LONG )
			{
				me->EndLongTaunt();
			}

			if ( !me->IsTaunting() )
			{
				return Done( "Taunt finished" );
			}

			CTFPlayer *partner = m_partner.Get();
			if ( partner && partner->IsTaunting() && me->CanMoveDuringTaunt() )
			{
				if ( m_tauntStopCheckTimer.HasStarted() && m_tauntStopCheckTimer.IsElapsed() )
				{
					// if not in setup, roll to stop taunting early
					if ( !TFGameRules()->InSetup() && RandomFloat( 0.0f, 100.0f ) < tf_bot_taunt_stop_chance.GetFloat() )
					{
						m_tauntEndTimer.Start( RandomFloat( 0.8f, 2.f ) );
						m_tauntStopCheckTimer.Invalidate();
					}
					else
					{
						m_tauntStopCheckTimer.Start( tauntStopCheckInterval );
					}
				}

				// store taunt yaw now that we are actually taunting
				if ( !m_hasTauntYaw )
				{
					m_targetTauntYaw = me->GetTauntYaw();
					m_hasTauntYaw = true;
				}

				const float nearbyRange = 100.f;
				const float maxFacingAngle = 10.f;

				// if outside range of the target, move towards them and turn to face them
				if ( me->IsRangeGreaterThan( partner, nearbyRange ) )
				{
					// Force move forwards
					me->PressForwardButton();

					Vector toPartner = partner->GetAbsOrigin() - me->GetAbsOrigin();
					toPartner.z = 0.f;
					// diff > maxFacingAngle or < -maxFacingAngle requires turning
					float diff = UTIL_AngleDiff( UTIL_VecToYaw( toPartner ), m_targetTauntYaw );

					float flTauntTurnAccelerationTime = me->GetTauntTurnAccelerationTime();
					// Ported from C_TFPlayer::CreateMove
					float flSign = fabsf( diff ) > maxFacingAngle ? 1.f : -1.f;
					float flMaxTurnSpeed = me->GetTauntTurnSpeed();
					if ( flTauntTurnAccelerationTime > 0.f )
					{
						m_currTauntTurnSpeed = clamp( m_currTauntTurnSpeed + flSign * ( interval / flTauntTurnAccelerationTime ) * flMaxTurnSpeed, 0.f, flMaxTurnSpeed );
					}
					else
					{
						m_currTauntTurnSpeed = flMaxTurnSpeed;
					}

					float flSmoothTurnSpeed = 0.f;
					if ( flMaxTurnSpeed > 0.f )
					{
						flSmoothTurnSpeed = SimpleSpline( m_currTauntTurnSpeed / flMaxTurnSpeed ) * flMaxTurnSpeed;
					}

					// mimics angMoveAngle handling from C_TFPlayer::CreateMove
					// target yaw accumulates even if current taunt yaw/AimHeadTowards hasn't caught up
					if ( diff > maxFacingAngle )
					{
						m_targetTauntYaw += flSmoothTurnSpeed * interval;
					}
					else if ( diff < -maxFacingAngle )
					{
						m_targetTauntYaw -= flSmoothTurnSpeed * interval;
					}

					// calculates a world point to look at based on target yaw so we get smooth turning
					Vector aimDir;
					AngleVectors( QAngle( 0.f, m_targetTauntYaw, 0.f ), &aimDir );
					me->GetBodyInterface()->AimHeadTowards( me->EyePosition() + 100.0f * aimDir, IBody::CRITICAL, interval, NULL, "Taunt partner" );
				}
				else
				{
					m_currTauntTurnSpeed = 0.f;
					m_targetTauntYaw = me->GetTauntYaw();
					me->ReleaseForwardButton();
				}
			}
			else
			{
				m_currTauntTurnSpeed = 0.f;
				me->ReleaseForwardButton();
			}

			const float maxRange = 800.f;

			// Start a timer to end our taunt if partner missing, partner not taunting, or can't see/too far from partner
			if ( !m_tauntEndTimer.HasStarted()
				&& ( !partner
					|| !partner->IsTaunting()
					|| !me->GetVisionInterface()->IsLineOfSightClearToEntity( partner )
					|| me->IsRangeGreaterThan( partner, maxRange ) ) )
			{
				m_tauntEndTimer.Start( RandomFloat( 0.8f, 2.f ) );
				m_tauntStopCheckTimer.Invalidate();
			}
		}
		else
		{
			me->HandleTauntCommand();
			m_didTaunt = true;
			m_tauntStopCheckTimer.Start( tauntStopCheckInterval );
		}
	}

	return Continue();
}
