//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"

#include "../merasmus.h"
#include "merasmus_zap.h"
#include "merasmus_stunned.h"

ActionResult< CMerasmus > CMerasmusZap::OnStart( CMerasmus *me, Action< CMerasmus > *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_RANGE_ATTACK2 );
	m_zapTimer.Start( 1.3f );

	m_spellType = SpellType_t( RandomInt( 0, SPELL_COUNT - 1 ) );
	PlayCastSound( me );

	return Continue();
}


ActionResult< CMerasmus > CMerasmusZap::Update( CMerasmus *me, float interval )
{
	// Interupt if stunned
	if ( me->HasStunTimer() )
	{
		return ChangeTo( new CMerasmusStunned, "Stun Interupt!" );
	}

	if ( m_zapTimer.HasStarted() && m_zapTimer.IsElapsed() )
	{
		m_zapTimer.Invalidate();

		const float flSpellRange = 600.f + 50.f * ( me->GetLevel() - 1 );
		const int nTargetCount = 6 + ( me->GetLevel() - 1 );
		const float flMaxDamage = 50.f + ( 5 * (me->GetLevel() - 1) );
		const float flMinDamage = 20.f + ( 5 * (me->GetLevel() - 1) );

		if ( CMerasmus::Zap( me, "effect_staff", flSpellRange, flMinDamage, flMaxDamage, nTargetCount ) )
		{
			me->EmitSound( "Halloween.Merasmus_Spell" );
		}
	}

	if ( me->IsActivityFinished() )
	{
		return Done( "Zapped!" );
	}

	return Continue();
}


void CMerasmusZap::PlayCastSound( CMerasmus* me ) const
{
	CPVSFilter filter( me->WorldSpaceCenter() );
	switch ( m_spellType )
	{
	case SPELL_FIRE:
		{
			me->PlayLowPrioritySound( filter, "Halloween.MerasmusCastFireSpell" ); 
		}
		break;
	case SPELL_LAUNCH:
		{
			me->PlayLowPrioritySound( filter, "Halloween.MerasmusLaunchSpell" );
		}
		break;
	}
}

