//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"

#include "../merasmus.h"
#include "merasmus_stunned.h"
#include "merasmus_teleport.h"

#include "tf_player.h"

ActionResult< CMerasmus > CMerasmusStunned::OnStart( CMerasmus *me, Action< CMerasmus > *priorAction )
{
	m_nStunStage = STUN_BEGIN;
	
	int iLayer = me->AddGesture( ACT_MP_STUN_BEGIN );
	float flDuration = me->GetLayerDuration( iLayer );
	m_stunAnimationTimer.Start( flDuration );

	me->OnBeginStun();

	return Continue();
}


ActionResult< CMerasmus > CMerasmusStunned::Update( CMerasmus *me, float interval )
{
	// finished?
	if ( m_nStunStage == STUN_END && m_stunFinishTimer.IsElapsed() )
	{
		if ( me->ShouldDisguise() )
		{
			return Done();
		}

		if ( me->GetBombHitCount() >= 3 )
		{
			return ChangeTo( new CMerasmusTeleport( true, true ), "Teleport AOE!" );
		}
		else
		{
			return ChangeTo( new CMerasmusTeleport( false, false ), "Teleport to new area!" );
		}
	}

	if ( m_stunAnimationTimer.IsElapsed() )
	{
		bool bStunned = me->HasStunTimer();

		// reset animation if stunned
		if ( bStunned )
		{
			m_nStunStage = STUN_BEGIN;
		}

		switch ( m_nStunStage )
		{
		case STUN_BEGIN:
			{
				int iLayer = me->AddGesture( ACT_MP_STUN_MIDDLE );
				float flDuration = me->GetLayerDuration( iLayer );
				m_stunAnimationTimer.Start( flDuration );
				if ( !bStunned )
				{
					m_nStunStage = STUN_MID;
				}
			}
			break;
		case STUN_MID:
			{
				int iLayer = me->AddGesture( ACT_MP_STUN_END );
				float flDuration = me->GetLayerDuration( iLayer );
				m_stunAnimationTimer.Start( flDuration );

				m_nStunStage = STUN_END;
				m_stunFinishTimer.Start( flDuration + 0.5f );
			}
			break;
		}
	}

	return Continue();
}


void CMerasmusStunned::OnEnd( CMerasmus *me, Action< CMerasmus > *nextAction )
{
	me->OnEndStun();
}
