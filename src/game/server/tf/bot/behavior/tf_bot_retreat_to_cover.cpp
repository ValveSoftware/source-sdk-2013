//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_move_to_cover.cpp
// Retreat to local cover from known threats
// Michael Booth, June 2009

#include "cbase.h"
#include "tf_player.h"
#include "bot/tf_bot.h"
#include "bot/behavior/tf_bot_retreat_to_cover.h"

extern ConVar tf_bot_path_lookahead_range;
ConVar tf_bot_retreat_to_cover_range( "tf_bot_retreat_to_cover_range", "1000", FCVAR_CHEAT );
ConVar tf_bot_debug_retreat_to_cover( "tf_bot_debug_retreat_to_cover", "0", FCVAR_CHEAT );
ConVar tf_bot_wait_in_cover_min_time( "tf_bot_wait_in_cover_min_time", "1", FCVAR_CHEAT );
ConVar tf_bot_wait_in_cover_max_time( "tf_bot_wait_in_cover_max_time", "2", FCVAR_CHEAT );


//---------------------------------------------------------------------------------------------
CTFBotRetreatToCover::CTFBotRetreatToCover( float hideDuration )
{
	m_hideDuration = hideDuration;
	m_actionToChangeToOnceCoverReached = NULL;
}


//---------------------------------------------------------------------------------------------
CTFBotRetreatToCover::CTFBotRetreatToCover( Action< CTFBot > *actionToChangeToOnceCoverReached )
{
	m_hideDuration = -1.0f;
	m_actionToChangeToOnceCoverReached = actionToChangeToOnceCoverReached;
}


//---------------------------------------------------------------------------------------------
// for testing a given area's exposure to known threats
class CTestAreaAgainstThreats :  public IVision::IForEachKnownEntity
{
public:
	CTestAreaAgainstThreats( CTFBot *me, CTFNavArea *area )
	{
		m_me = me;
		m_area = area;
		m_exposedThreatCount = 0;
	}

	virtual bool Inspect( const CKnownEntity &known )
	{
		VPROF_BUDGET( "CTestAreaAgainstThreats::Inspect", "NextBot" );

		if ( m_me->IsEnemy( known.GetEntity() ) )
		{
			const CNavArea *threatArea = known.GetLastKnownArea();

			if ( threatArea )
			{
				// is area visible by known threat
				if ( m_area->IsPotentiallyVisible( threatArea ) )
					++m_exposedThreatCount;
			}
		}

		return true;
	}

	CTFBot *m_me;
	CTFNavArea *m_area;
	int m_exposedThreatCount;
};


// collect nearby areas that provide cover from our known threats
class CSearchForCover : public ISearchSurroundingAreasFunctor
{
public:
	CSearchForCover( CTFBot *me )
	{
		m_me = me;
		m_minExposureCount = 9999;

		if ( tf_bot_debug_retreat_to_cover.GetBool() )
			TheNavMesh->ClearSelectedSet();
	}

	virtual bool operator() ( CNavArea *baseArea, CNavArea *priorArea, float travelDistanceSoFar )
	{
		VPROF_BUDGET( "CSearchForCover::operator()", "NextBot" );

		CTFNavArea *area = (CTFNavArea *)baseArea;

		CTestAreaAgainstThreats test( m_me, area );
		m_me->GetVisionInterface()->ForEachKnownEntity( test );

		if ( test.m_exposedThreatCount <= m_minExposureCount )
		{
			// this area is at least as good as already found cover
			if ( test.m_exposedThreatCount < m_minExposureCount )
			{
				// this area is better than already found cover - throw out list and start over
				m_coverAreaVector.RemoveAll();
				m_minExposureCount = test.m_exposedThreatCount;
			}

			m_coverAreaVector.AddToTail( area );
		}

		return true;				
	}

	// return true if 'adjArea' should be included in the ongoing search
	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar ) 
	{
		if ( travelDistanceSoFar > tf_bot_retreat_to_cover_range.GetFloat() )
			return false;

		// allow falling off ledges, but don't jump up - too slow
		return ( currentArea->ComputeAdjacentConnectionHeightChange( adjArea ) < m_me->GetLocomotionInterface()->GetStepHeight() );
	}

	virtual void PostSearch( void )
	{
		if ( tf_bot_debug_retreat_to_cover.GetBool() )
		{
			for( int i=0; i<m_coverAreaVector.Count(); ++i )
				TheNavMesh->AddToSelectedSet( m_coverAreaVector[i] );
		}
	}

	CTFBot *m_me;
	CUtlVector< CTFNavArea * > m_coverAreaVector;
	int m_minExposureCount;
};


//---------------------------------------------------------------------------------------------
CTFNavArea *CTFBotRetreatToCover::FindCoverArea( CTFBot *me )
{
	VPROF_BUDGET( "CTFBotRetreatToCover::FindCoverArea", "NextBot" );

	CSearchForCover search( me );
	SearchSurroundingAreas( me->GetLastKnownArea(), search );

	if ( search.m_coverAreaVector.Count() == 0 )
	{
		return NULL;
	}

	// first in vector should be closest via travel distance
	// pick from the closest 10 areas to avoid the whole team bunching up in one spot
	int last = MIN( 10, search.m_coverAreaVector.Count() );
	int which = RandomInt( 0, last-1 );
	return search.m_coverAreaVector[ which ];
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotRetreatToCover::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_coverArea = FindCoverArea( me );

	if ( m_coverArea == NULL )
		return Done( "No cover available!" );

	if ( m_hideDuration < 0.0f )
	{
		m_hideDuration = RandomFloat( tf_bot_wait_in_cover_min_time.GetFloat(), tf_bot_wait_in_cover_max_time.GetFloat() );
	}

	m_waitInCoverTimer.Start( m_hideDuration );

	// if I'm a spy, cloak and disguise while I retreat
	if ( me->IsPlayerClass( TF_CLASS_SPY ) )
	{
		if ( !me->m_Shared.IsStealthed() )
		{
			me->PressAltFireButton();
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotRetreatToCover::Update( CTFBot *me, float interval )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat( true );

	if ( me->m_Shared.InCond( TF_COND_INVULNERABLE ) )
		return Done( "I'm invulnerable - no need to retreat!" );

	if ( ShouldRetreat( me ) == ANSWER_NO )
		return Done( "No longer need to retreat" );

	// attack while retreating
	me->EquipBestWeaponForThreat( threat );

	// reload while moving to cover
	bool isDoingAFullReload = false;
	CTFWeaponBase *myPrimary = (CTFWeaponBase *)me->Weapon_GetSlot( TF_WPN_TYPE_PRIMARY );
	if ( myPrimary && me->GetAmmoCount( TF_AMMO_PRIMARY ) > 0 && me->IsBarrageAndReloadWeapon( myPrimary ) )
	{
		if ( myPrimary->Clip1() < myPrimary->GetMaxClip1() )
		{
			me->PressReloadButton();
			isDoingAFullReload = true;
		}
	}


	// move to cover, or stop if we've found opportunistic cover (no visible threats right now)
	if ( me->GetLastKnownArea() == m_coverArea || !threat )
	{
		// we are now in cover

		if ( threat )
		{
			// threats are still visible - find new cover
			m_coverArea = FindCoverArea( me );

			if ( m_coverArea == NULL )
			{
				return Done( "My cover is exposed, and there is no other cover available!" );
			}
		}

		if ( me->IsPlayerClass( TF_CLASS_SPY ) && !me->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			// don't leave cover until my disguise kicks in
			return Continue();
		}

		// uncloak so we can attack when we leave cover
		if ( me->m_Shared.IsStealthed() )
		{
			me->PressAltFireButton();
		}

		if ( m_actionToChangeToOnceCoverReached )
		{
			return ChangeTo( m_actionToChangeToOnceCoverReached, "Doing given action now that I'm in cover" );
		}

		// if I'm being healed by a medic who nearly has his charge built up, wait in cover until his charge is ready
		int numHealers = me->m_Shared.GetNumHealers();
		for ( int i=0; i<numHealers; ++i )
		{
			CTFPlayer *medic = ToTFPlayer( me->m_Shared.GetHealerByIndex( i ) );

			if ( medic && medic->MedicGetChargeLevel() > 0.9f )
			{
				// wait for uber to finish
				return ( medic->MedicGetChargeLevel() < 1.f ) ? Continue() : Done();
			}
		}

		// stay in cover while we fully reload
		if ( isDoingAFullReload )
		{
			return Continue();
		}

		if ( m_waitInCoverTimer.IsElapsed() )
		{
			return Done( "Been in cover long enough" );
		}
	}
	else
	{
		// not in cover yet
		m_waitInCoverTimer.Reset();

		if ( m_repathTimer.IsElapsed() )
		{
			m_repathTimer.Start( RandomFloat( 0.3f, 0.5f ) );

			CTFBotPathCost cost( me, RETREAT_ROUTE );
			m_path.Compute( me, m_coverArea->GetCenter(), cost );
		}

		m_path.Update( me );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotRetreatToCover::OnStuck( CTFBot *me )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotRetreatToCover::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotRetreatToCover::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
// Hustle yer butt to safety!
QueryResultType CTFBotRetreatToCover::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}


