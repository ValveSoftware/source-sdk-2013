//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Hooks and classes for the support of humanoid NPCs with 
//			groovy facial animation capabilities, aka, "Actors"
//
//=============================================================================//


#include "cbase.h"
#include "AI_Interest_Target.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool CAI_InterestTarget_t::IsThis( CBaseEntity *pThis )
{
	return (pThis == m_hTarget);
};

const Vector &CAI_InterestTarget_t::GetPosition( void )
{
	if (m_eType	== LOOKAT_ENTITY && m_hTarget != NULL)
	{
		m_vecPosition = m_hTarget->EyePosition();
	}
	return m_vecPosition;
};

bool CAI_InterestTarget_t::IsActive( void )
{
	if (m_flEndTime < gpGlobals->curtime) return false;
	if (m_eType == LOOKAT_ENTITY && m_hTarget == NULL) return false;
	return true;
};

float CAI_InterestTarget_t::Interest( void )
{
	float t = (gpGlobals->curtime - m_flStartTime) / (m_flEndTime - m_flStartTime);

	if (t < 0.0f || t > 1.0f)
		return 0.0f;

	if (m_flRamp && t < 1 - m_flRamp)
	{
		//t = t / m_flRamp;
		t = 1.0 - ExponentialDecay( 0.2, m_flRamp, t );
		//t = 1.0 - ExponentialDecay( 0.01, 1 - m_flRamp, t );
	}
	else if (t > 1.0f - m_flRamp)
	{
		t = (1.0 - t) / m_flRamp;
		t = 3.0f * t * t - 2.0f * t * t * t;
	} 
	else
	{
		t = 1.0f;
	}
	// ramp
	t *= m_flInterest;

	return t;
}


void CAI_InterestTarget::Add( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp )
{
	int i;

	for (i = 0; i < Count(); i++)
	{
		CAI_InterestTarget_t &target = Element( i );

		if (target.m_hTarget == pTarget && target.m_flRamp == 0)
		{
			if (target.m_flStartTime == gpGlobals->curtime)
			{
				flImportance = MAX( flImportance, target.m_flInterest );
			}
			Remove( i );
			break;
		}
	}

	Add( CAI_InterestTarget_t::LOOKAT_ENTITY, pTarget, Vector( 0, 0, 0 ), flImportance, flDuration, flRamp );
}

void CAI_InterestTarget::Add( const Vector &vecPosition, float flImportance, float flDuration, float flRamp )
{
	int i;

	for (i = 0; i < Count(); i++)
	{
		CAI_InterestTarget_t &target = Element( i );

		if (target.m_vecPosition == vecPosition)
		{
			Remove( i );
			break;
		}
	}

	Add( CAI_InterestTarget_t::LOOKAT_POSITION, NULL, vecPosition, flImportance, flDuration, flRamp );
}

void CAI_InterestTarget::Add( CBaseEntity *pTarget, const Vector &vecPosition, float flImportance, float flDuration, float flRamp )
{
	int i;

	for (i = 0; i < Count(); i++)
	{
		CAI_InterestTarget_t &target = Element( i );

		if (target.m_hTarget == pTarget)
		{
			if (target.m_flStartTime == gpGlobals->curtime)
			{
				flImportance = MAX( flImportance, target.m_flInterest );
			}
			Remove( i );
			break;
		}
	}

	Add( CAI_InterestTarget_t::LOOKAT_BOTH, pTarget, vecPosition, flImportance, flDuration, flRamp );
}

void CAI_InterestTarget::Add( CAI_InterestTarget_t::CAI_InterestTarget_e type, CBaseEntity *pTarget, const Vector &vecPosition, float flImportance, float flDuration, float flRamp )
{
	int i = AddToTail();
	CAI_InterestTarget_t &target = Element( i );

	target.m_eType = type;
	target.m_hTarget = pTarget;
	target.m_vecPosition = vecPosition;
	target.m_flInterest = flImportance;
	target.m_flStartTime = gpGlobals->curtime;
	target.m_flEndTime = gpGlobals->curtime + flDuration;
	target.m_flRamp = flRamp / flDuration;
}
