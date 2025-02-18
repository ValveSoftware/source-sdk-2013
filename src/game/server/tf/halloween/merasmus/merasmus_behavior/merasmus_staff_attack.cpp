//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"

#include "tf_gamerules.h"
#include "tf_player.h"

#include "../merasmus.h"
#include "merasmus_staff_attack.h"
#include "merasmus_stunned.h"

CMerasmusStaffAttack::CMerasmusStaffAttack( CTFPlayer* pTarget )
{
	m_hTarget = pTarget;
}


ActionResult< CMerasmus >	CMerasmusStaffAttack::OnStart( CMerasmus *me, Action< CMerasmus > *priorAction )
{
	// smooth out the bot's path following by moving toward a point farther down the path
	m_path.SetMinLookAheadDistance( 100.0f );

	int iLayer = me->AddGesture( ACT_MP_ATTACK_STAND_MELEE );
	float flDuration = me->GetLayerDuration( iLayer );
	m_staffSwingTimer.Start( flDuration );
	m_hitTimer.Start( 0.5f * flDuration );

	if ( RandomInt( 0, 2 ) == 0 )
	{
		CPVSFilter filter( me->WorldSpaceCenter() );
		if ( RandomInt( 1, 5 ) == 1 )
		{
			me->PlayLowPrioritySound( filter, "Halloween.MerasmusStaffAttackRare" );
		}
		else
		{
			me->PlayLowPrioritySound( filter, "Halloween.MerasmusStaffAttack" ); 
		}
	}

	return Continue();
}


ActionResult< CMerasmus >	CMerasmusStaffAttack::Update( CMerasmus *me, float interval )
{
	// Interupt if stunned
	if ( me->HasStunTimer() )
	{
		return ChangeTo( new CMerasmusStunned, "Stun Interupt!" );
	}

	if ( m_hitTimer.HasStarted() && m_hitTimer.IsElapsed() )
	{
		m_hitTimer.Invalidate();

		if ( m_hTarget != NULL )
		{
			Vector forward;
			me->GetVectors( &forward, NULL, NULL );

			Vector toVictim = m_hTarget->WorldSpaceCenter() - me->WorldSpaceCenter();
			toVictim.NormalizeInPlace();

			// looser tolerance as victim gets closer
			const float closeRange = 100.0f;
			float range = me->GetRangeTo( m_hTarget );
			float closeness = ( range < closeRange ) ? 0.0f : ( range - closeRange ) / ( tf_merasmus_attack_range.GetFloat() - closeRange );
			float hitAngle = 0.0f + closeness * 0.27f;

			if ( DotProduct( forward, toVictim ) > hitAngle )
			{
				if ( me->IsRangeLessThan( m_hTarget, 0.9f * tf_merasmus_attack_range.GetFloat() ) )
				{
					if ( me->IsLineOfSightClear( m_hTarget ) )
					{
						// CHOP!
						CTakeDamageInfo info( me, me, 70, DMG_CLUB, TF_DMG_CUSTOM_MERASMUS_DECAPITATION );
						CalculateMeleeDamageForce( &info, toVictim, me->WorldSpaceCenter(), 5.0f );
						m_hTarget->TakeDamage( info );

						CPVSFilter filter( me->WorldSpaceCenter() );
						me->PlayLowPrioritySound( filter, "Halloween.HeadlessBossAxeHitFlesh" );

						me->PushPlayer( m_hTarget, 500.f );
					}
				}
			}
		}
	}

	if ( m_hTarget )
	{
		if ( me->IsRangeGreaterThan( m_hTarget, 100.f ) || !me->IsLineOfSightClear( m_hTarget ) )
		{
			if ( m_path.GetAge() > 1.0f )
			{
				CMerasmusPathCost cost( me );
				m_path.Compute( me, m_hTarget, cost );
			}

			m_path.Update( me );
		}

		me->GetLocomotionInterface()->FaceTowards( m_hTarget->WorldSpaceCenter() );
	}

	if ( m_staffSwingTimer.IsElapsed() )
	{
		return Done();
	}

	return Continue();
}
