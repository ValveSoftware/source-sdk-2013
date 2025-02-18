//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "nav_mesh/tf_nav_area.h"
#include "particle_parse.h"
#include "player_vs_environment/monster_resource.h"

#include "../merasmus.h"
#include "../merasmus_trick_or_treat_prop.h"
#include "merasmus_disguise.h"
#include "merasmus_reveal.h"

ConVar tf_merasmus_disguise_debug( "tf_merasmus_disguise_debug", "0", FCVAR_CHEAT );

//---------------------------------------------------------------------------------------------
ActionResult< CMerasmus >	CMerasmusDisguise::OnStart( CMerasmus *me, Action< CMerasmus > *priorAction )
{
	m_bSpawnedProps = false;

	TryToDisguiseSpawn( me );
	
	m_flStartRegenTime = gpGlobals->curtime;
	m_nStartRegenHealth = me->GetHealth();

	me->PlayHighPrioritySound( "Halloween.MerasmusInitiateHiding" ); 
	RandomDisguiseTauntTimer();
	
	m_findPropsFailTimer.Start( 3 );

	// set boss inactive
	g_pMonsterResource->SetBossState( 1 );

	return Continue();
}


//----------------------------------------------------------------------------------
ActionResult< CMerasmus >	CMerasmusDisguise::Update( CMerasmus *me, float interval )
{
	if ( me->ShouldLeave() )
	{
		return Done();
	}
	me->LeaveWarning();

	if ( !m_bSpawnedProps )
	{
		if ( m_findPropsFailTimer.HasStarted() && m_findPropsFailTimer.IsElapsed() )
		{
			// Couldn't find props in time - skip
			return Done();
		}

		if ( !m_findSpawnPositionTime.IsElapsed() )
		{
			// not ready yet
			return Continue();
		}

		TryToDisguiseSpawn( me );

		return Continue();
	}

	if ( m_disguiseTauntTimer.IsElapsed() )
	{
		if (RandomInt(0,10) == 0)
		{
			me->PlayHighPrioritySound( "Halloween.MerasmusHiddenRare" );
		}
		else
		{
			me->PlayHighPrioritySound( "Halloween.MerasmusHidden" );
		}

		RandomDisguiseTauntTimer();
	}

	// regen health while disguise
	if ( me->GetHealth() < me->GetMaxHealth() )
	{
		float flHealthRegenPerSec = tf_merasmus_health_regen_rate.GetFloat() * me->GetMaxHealth() * ( me->GetLevel() - 1 );
		int nNewHealth = MIN( ( gpGlobals->curtime - m_flStartRegenTime ) * flHealthRegenPerSec + m_nStartRegenHealth, me->GetMaxHealth() );
		me->SetHealth( nNewHealth );

		// show Boss' health meter on HUD
		if ( g_pMonsterResource )
		{
			float healthPercentage = (float)me->GetHealth() / (float)me->GetMaxHealth();
			g_pMonsterResource->SetBossHealthPercentage( healthPercentage );
		}
	}

	// should I come out from disguise?
	if ( me->ShouldReveal() )
	{
		return Done( "Revealed!" );
	}

	return Continue();
}


void CMerasmusDisguise::OnEnd( CMerasmus *me, Action< CMerasmus > *nextAction )
{
	if ( me->ShouldLeave() )
	{
		me->OnLeaveWhileInPropForm();
	}

	// set boss active
	g_pMonsterResource->SetBossState( 0 );

	me->OnRevealed();
}


QAngle GetRandomPropAngles( CTFNavArea* pArea )
{
	Vector vNormal;
	pArea->ComputeNormal( &vNormal );
	Vector vForward = pArea->GetRandomPoint() - pArea->GetCenter();
	QAngle qAngles;
	VectorAngles( vForward, vNormal, qAngles );

	return qAngles;
}


void CMerasmusDisguise::TryToDisguiseSpawn( CMerasmus *me )
{
	m_findSpawnPositionTime.Start( 1 );

	// face towards a nearby player
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
	CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

	// pick a random spot
	CUtlVector< CTFNavArea * > candidateAreaVector;
	for( int i=0; i<TheNavAreas.Count(); ++i )
	{
		CTFNavArea *area = (CTFNavArea *)TheNavAreas[i];

		if ( !area->HasFuncNavPrefer() )
		{
			// don't spawn outside nav prefer
			continue;
		}

		// don't use small nav areas
		const float goodSize = 150.f;
		if ( area->GetSizeX() < goodSize || area->GetSizeY() < goodSize )
		{
			continue;
		}

		// don't use area containing player
		if ( area->GetPlayerCount( TF_TEAM_BLUE ) || area->GetPlayerCount( TF_TEAM_RED ) )
		{
			continue;
		}

		// don't use slope area
// 		Vector vNormal;
// 		area->ComputeNormal( &vNormal );
// 		if ( vNormal.z < 0.9f )
// 		{
// 			continue;
// 		}

		candidateAreaVector.AddToTail( area );
	}

	if ( candidateAreaVector.Count() == 0 )
	{
		// no place to spawn (!)
		return;
	}

	// spread out the area
	CUtlVector< CTFNavArea * > spawnAreaVector;
	SelectSeparatedShuffleSet< CTFNavArea >( 10, 500.f, candidateAreaVector, &spawnAreaVector );

	if ( spawnAreaVector.Count() == 0 )
	{
		// no place to spawn (!)
		return;
	}

	if ( tf_merasmus_disguise_debug.GetBool() )
	{
		for ( int i=0; i<spawnAreaVector.Count(); ++i )
		{
			// draw all potential areas
			spawnAreaVector[i]->DrawFilled( 0, 255, 0, 0, 30.f );
		}
	}

	// spawn random props
	int nRandomTrickOrTreatProps = spawnAreaVector.Count();
	for ( int i=0; i<nRandomTrickOrTreatProps; ++i )
	{
		int propSpawnID = RandomInt( 0, spawnAreaVector.Count()-1 );

		CTFMerasmusTrickOrTreatProp* pFakeProp = CTFMerasmusTrickOrTreatProp::Create( spawnAreaVector[ propSpawnID ]->GetCenter(), GetRandomPropAngles( spawnAreaVector[ propSpawnID ] ) );
		me->AddFakeProp( pFakeProp );

		spawnAreaVector.FastRemove( propSpawnID );
	}

	me->OnDisguise();
	m_bSpawnedProps = true;
}


void CMerasmusDisguise::RandomDisguiseTauntTimer()
{
	m_disguiseTauntTimer.Start( RandomFloat( 10.f, 25.f ) );
}
