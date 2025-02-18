//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"

#include "particle_parse.h"
#include "tf/halloween/eyeball_boss/teleport_vortex.h"
#include "player_vs_environment/monster_resource.h"
#include "tf_gamerules.h"
#include "nav_mesh/tf_nav_area.h"

#include "../merasmus.h"
#include "merasmus_teleport.h"
#include "merasmus_aoe_attack.h"


CMerasmusTeleport::CMerasmusTeleport( bool bShouldAOE, bool bGoToCap )
	: m_bShouldAOE( bShouldAOE ), m_bShouldGoToCap( bGoToCap )
{
}


//---------------------------------------------------------------------------------------------
ActionResult< CMerasmus > CMerasmusTeleport::OnStart( CMerasmus *me, Action< CMerasmus > *priorAction )
{
	// teleport out
	m_state = TELEPORTING_OUT;
	me->GetBodyInterface()->StartActivity( ACT_SHIELD_DOWN );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CMerasmus > CMerasmusTeleport::Update( CMerasmus *me, float interval )
{
	if ( me->IsActivityFinished() )
	{
		switch( m_state )
		{
		case TELEPORTING_OUT:
			{
				DispatchParticleEffect( "merasmus_tp", me->WorldSpaceCenter(), me->GetAbsAngles() );

				me->AddEffects( EF_NOINTERP | EF_NODRAW );

				me->SetAbsOrigin( GetTeleportPosition( me ) );

				// wait on the other side for a moment
				m_state = TELEPORTING_IN;
			}
			break;

		case TELEPORTING_IN:
			{
				me->RemoveEffects( EF_NOINTERP | EF_NODRAW );

				DispatchParticleEffect( "merasmus_tp", me->WorldSpaceCenter(), me->GetAbsAngles() );

				me->GetBodyInterface()->StartActivity( ACT_SHIELD_UP );

				m_state = DONE;
			}
			break;

		case DONE:
			{
				if ( m_bShouldAOE )
				{
					m_bShouldAOE = false;
					return SuspendFor( new CMerasmusAOEAttack, "AOE Attack!" );
				}
			}
			return Done();
		}
	}

	return Continue();
}


Vector CMerasmusTeleport::GetTeleportPosition( CMerasmus *me ) const
{
	Vector vGroundOffset( 0, 0, 75.0f );
	if ( m_bShouldGoToCap )
	{
		return me->GetHomePosition() + vGroundOffset;
	}
	else
	{
		// pick a random spot
		const float goodSize = 100.f;
		CUtlVector< CTFNavArea * > spawnAreaVector;
		for( int i=0; i<TheNavAreas.Count(); ++i )
		{
			CTFNavArea *area = (CTFNavArea *)TheNavAreas[i];

			if ( !area->HasFuncNavPrefer() )
			{
				// don't spawn outside nav prefer
				continue;
			}

			// don't use small nav areas
			if ( area->GetSizeX() < goodSize || area->GetSizeY() < goodSize )
			{
				continue;
			}

			// don't use area containing player
			if ( area->GetPlayerCount( TF_TEAM_BLUE ) || area->GetPlayerCount( TF_TEAM_RED ) )
			{
				continue;
			}

			spawnAreaVector.AddToTail( area );
		}

		if ( spawnAreaVector.Count() )
		{
			int which = RandomInt( 0, spawnAreaVector.Count() - 1 );
			return spawnAreaVector[ which ]->GetCenter();
		}
		else
		{
			return me->GetHomePosition() + vGroundOffset;
		}
	}
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
ActionResult< CMerasmus > CMerasmusEscape::OnStart( CMerasmus *me, Action< CMerasmus > *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_FLY );

	if (RandomInt(0,10) == 0)
	{
		me->PlayHighPrioritySound( "Halloween.MerasmusDepartRare" );
	}
	else
	{
		me->PlayHighPrioritySound( "Halloween.MerasmusDepart" );
	}

	UTIL_LogPrintf( "HALLOWEEN: merasmus_escaped (max_dps %3.2f) (health %d) (level %d)\n", me->GetMaxInjuryRate(), me->GetHealth(), me->GetLevel() );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CMerasmus > CMerasmusEscape::Update( CMerasmus *me, float interval )
{
	if ( me->IsActivityFinished() )
	{
		Vector vPos;
		QAngle qAngles;
		me->GetAttachment( "effect_robe", vPos, qAngles );
		DispatchParticleEffect( "merasmus_tp", vPos, qAngles );

		if ( g_pMonsterResource )
		{
			g_pMonsterResource->HideBossHealthMeter();
		}

		IGameEvent *event = gameeventmanager->CreateEvent( "merasmus_escaped" );
		if ( event )
		{
			event->SetInt( "level", me->GetLevel() );
			gameeventmanager->FireEvent( event );
		}
		me->TriggerLogicRelay( "boss_exit_relay" );

		// reset back to normal level
		me->ResetLevel();

		me->StartRespawnTimer();

		UTIL_Remove( me );

		return Done();
	}

	return Continue();
}
