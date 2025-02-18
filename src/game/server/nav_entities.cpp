//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// nav_entities.cpp
// AI Navigation entities
// Author: Michael S. Booth (mike@turtlerockstudios.com), January 2003

#include "cbase.h"

#include "nav_mesh.h"
#include "nav_node.h"
#include "nav_pathfind.h"
#include "nav_colors.h"
#include "fmtstr.h"
#include "props_shared.h"
#include "func_breakablesurf.h"

#ifdef TERROR
#include "func_elevator.h"
#include "AmbientLight.h"
#endif

#ifdef TF_DLL
#include "tf_player.h"
#include "bot/tf_bot.h"
#endif

#include "Color.h"
#include "collisionutils.h"
#include "functorutils.h"
#include "team.h"
#include "nav_entities.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------
BEGIN_DATADESC( CFuncNavCost )

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_KEYFIELD( m_iszTags, FIELD_STRING, "tags" ),
	DEFINE_KEYFIELD( m_team, FIELD_INTEGER, "team" ),
	DEFINE_KEYFIELD( m_isDisabled, FIELD_BOOLEAN, "start_disabled" ),

	DEFINE_THINKFUNC( CostThink ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( func_nav_avoid, CFuncNavAvoid );
LINK_ENTITY_TO_CLASS( func_nav_prefer, CFuncNavPrefer );

CUtlVector< CHandle< CFuncNavCost > > CFuncNavCost::gm_masterCostVector;
CountdownTimer CFuncNavCost::gm_dirtyTimer;

#define UPDATE_DIRTY_TIME 0.2f


//--------------------------------------------------------------------------------------------------------
void CFuncNavCost::Spawn( void )
{
	BaseClass::Spawn();

	gm_masterCostVector.AddToTail( this );
	gm_dirtyTimer.Start( UPDATE_DIRTY_TIME );

	SetSolid( SOLID_BSP );	
	AddSolidFlags( FSOLID_NOT_SOLID );

	SetMoveType( MOVETYPE_NONE );
	SetModel( STRING( GetModelName() ) );
	AddEffects( EF_NODRAW );
	SetCollisionGroup( COLLISION_GROUP_NONE );

	VPhysicsInitShadow( false, false );

	SetThink( &CFuncNavCost::CostThink );
	SetNextThink( gpGlobals->curtime + UPDATE_DIRTY_TIME );

	m_tags.RemoveAll();

	const char *tags = STRING( m_iszTags );

	// chop space-delimited string into individual tokens
	if ( tags )
	{
		char *buffer = V_strdup ( tags );

		for( char *token = strtok( buffer, " " ); token; token = strtok( NULL, " " ) )
		{
			m_tags.AddToTail( CFmtStr( "%s", token ) );
		}

		delete [] buffer;
	}
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavCost::UpdateOnRemove( void )
{
	gm_masterCostVector.FindAndFastRemove( this );
	BaseClass::UpdateOnRemove();

	gm_dirtyTimer.Start( UPDATE_DIRTY_TIME );
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavCost::InputEnable( inputdata_t &inputdata )
{
	m_isDisabled = false;
	gm_dirtyTimer.Start( UPDATE_DIRTY_TIME );
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavCost::InputDisable( inputdata_t &inputdata )
{
	m_isDisabled = true;
	gm_dirtyTimer.Start( UPDATE_DIRTY_TIME );
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavCost::InputToggle( inputdata_t &inputdata )
{
	m_isDisabled = !m_isDisabled;
	gm_dirtyTimer.Start( UPDATE_DIRTY_TIME );
}

//--------------------------------------------------------------------------------------------------------
void CFuncNavCost::CostThink( void )
{
	SetNextThink( gpGlobals->curtime + UPDATE_DIRTY_TIME );

	if ( gm_dirtyTimer.HasStarted() && gm_dirtyTimer.IsElapsed() )
	{
		// one or more avoid entities have changed - update nav decoration
		gm_dirtyTimer.Invalidate();

		UpdateAllNavCostDecoration();
	}
}


//--------------------------------------------------------------------------------------------------------
bool CFuncNavCost::HasTag( const char *groupname ) const
{
	for( int i=0; i<m_tags.Count(); ++i )
	{
		if ( FStrEq( m_tags[i], groupname ) )
		{
			return true;
		}
	}

	return false;
}


//--------------------------------------------------------------------------------------------------------
// Return true if this cost applies to the given actor
bool CFuncNavCost::IsApplicableTo( CBaseCombatCharacter *who ) const
{
	if ( !who )
	{
		return false;
	}

	if ( m_team > 0 )
	{
		if ( who->GetTeamNumber() != m_team )
		{
			return false;
		}
	}

#ifdef TF_DLL
	// TODO: Make group comparison efficient and move to base combat character
	CTFBot *bot = ToTFBot( who );
	if ( bot )
	{
		if ( bot->HasTheFlag() )
		{
			if ( HasTag( "bomb_carrier" ) )
			{
				return true;
			}

			// check custom bomb_carrier tags for this bot
			for( int i=0; i<m_tags.Count(); ++i )
			{
				const char* pszTag = m_tags[i];
				if ( V_stristr( pszTag, "bomb_carrier" ) )
				{
					if ( bot->HasTag( pszTag ) )
					{
						return true;
					}
				}
			}

			// the bomb carrier only pays attention to bomb_carrier costs
			return false;
		}

		if ( bot->HasMission( CTFBot::MISSION_DESTROY_SENTRIES ) )
		{
			if ( HasTag( "mission_sentry_buster" ) )
			{
				return true;
			}
		}
		
		if ( bot->HasMission( CTFBot::MISSION_SNIPER ) )
		{
			if ( HasTag( "mission_sniper" ) )
			{
				return true;
			}
		}
		
		if ( bot->HasMission( CTFBot::MISSION_SPY ) )
		{
			if ( HasTag( "mission_spy" ) )
			{
				return true;
			}
		}

		if ( bot->HasMission( CTFBot::MISSION_REPROGRAMMED ) )
		{
			return false;
		}

		if ( !bot->IsOnAnyMission() )
		{
			if ( HasTag( "common" ) )
			{
				return true;
			}
		}

		if ( HasTag( bot->GetPlayerClass()->GetName() ) )
		{
			return true;
		}

		// check custom tags for this bot
		for( int i=0; i<m_tags.Count(); ++i )
		{
			if ( bot->HasTag( m_tags[i] ) )
			{
				return true;
			}
		}

		// this cost doesn't apply to me
		return false;
	}
#endif

	return false;
}


//--------------------------------------------------------------------------------------------------------
// Reevaluate all func_nav_cost entities and update the nav decoration accordingly.
// This is required to handle overlapping func_nav_cost entities.
void CFuncNavCost::UpdateAllNavCostDecoration( void )
{
	int i, j;

	// first, clear all avoid decoration from the mesh
	for( i=0; i<TheNavAreas.Count(); ++i )
	{
		TheNavAreas[i]->ClearAllNavCostEntities();
	}

	// now, mark all areas with active cost entities overlapping them
	for( i=0; i<gm_masterCostVector.Count(); ++i )
	{
		CFuncNavCost *cost = gm_masterCostVector[i];

		if ( !cost || !cost->IsEnabled() )
		{
			continue;
		}

		Extent extent;
		extent.Init( cost );

		CUtlVector< CNavArea * > overlapVector;
		TheNavMesh->CollectAreasOverlappingExtent( extent, &overlapVector );

		Ray_t ray;
		trace_t tr;
		ICollideable *pCollide = cost->CollisionProp();

		for( j=0; j<overlapVector.Count(); ++j )
		{
			ray.Init( overlapVector[j]->GetCenter(), overlapVector[j]->GetCenter() );

			enginetrace->ClipRayToCollideable( ray, MASK_ALL, pCollide, &tr );
			
			if ( tr.startsolid )
			{
				overlapVector[j]->AddFuncNavCostEntity( cost );
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------
// Return pathfind cost multiplier for the given actor
float CFuncNavAvoid::GetCostMultiplier( CBaseCombatCharacter *who ) const
{
	if ( IsApplicableTo( who ) )
	{
		return 25.0f;
	}

	return 1.0f;
}


//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------
// Return pathfind cost multiplier for the given actor
float CFuncNavPrefer::GetCostMultiplier( CBaseCombatCharacter *who ) const
{
	if ( IsApplicableTo( who ) )
	{
		return 0.04f;	// 1/25th
	}

	return 1.0f;
}



//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------
BEGIN_DATADESC( CFuncNavBlocker )

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "BlockNav", InputBlockNav ),
	DEFINE_INPUTFUNC( FIELD_VOID, "UnblockNav", InputUnblockNav ),
	DEFINE_KEYFIELD( m_blockedTeamNumber, FIELD_INTEGER, "teamToBlock" ),
	DEFINE_KEYFIELD( m_bDisabled,	FIELD_BOOLEAN,	"StartDisabled" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( func_nav_blocker, CFuncNavBlocker );


CUtlLinkedList<CFuncNavBlocker *> CFuncNavBlocker::gm_NavBlockers;

//-----------------------------------------------------------------------------------------------------
int CFuncNavBlocker::DrawDebugTextOverlays( void )
{
	int offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		CFmtStr str;

		// FIRST_GAME_TEAM skips TEAM_SPECTATOR and TEAM_UNASSIGNED, so we can print
		// useful team names in a non-game-specific fashion.
		for ( int i=FIRST_GAME_TEAM; i<FIRST_GAME_TEAM + MAX_NAV_TEAMS; ++i )
		{
			if ( IsBlockingNav( i ) )
			{
				CTeam *team = GetGlobalTeam( i );
				if ( team )
				{
					EntityText( offset++, str.sprintf( "blocking team %s", team->GetName() ), 0 );
				}
				else
				{
					EntityText( offset++, str.sprintf( "blocking team %d", i ), 0 );
				}
			}
		}

		NavAreaCollector collector( true );
		Extent extent;
		extent.Init( this );
		TheNavMesh->ForAllAreasOverlappingExtent( collector, extent );

		for ( int i=0; i<collector.m_area.Count(); ++i )
		{
			CNavArea *area = collector.m_area[i];
			Extent areaExtent;
			area->GetExtent( &areaExtent );
			if ( debugoverlay )
			{
				debugoverlay->AddBoxOverlay( vec3_origin, areaExtent.lo, areaExtent.hi, vec3_angle, 0, 255, 0, 10, NDEBUG_PERSIST_TILL_NEXT_SERVER );
			}
		}
	}

	return offset;
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavBlocker::UpdateBlocked()
{
	NavAreaCollector collector( true );
	Extent extent;
	extent.Init( this );
	TheNavMesh->ForAllAreasOverlappingExtent( collector, extent );

	for ( int i=0; i<collector.m_area.Count(); ++i )
	{
		CNavArea *area = collector.m_area[i];
		area->UpdateBlocked( true );
	}

}


//--------------------------------------------------------------------------------------------------------
// Forces nav areas to unblock when the nav blocker is deleted (round restart) so flow can compute properly
void CFuncNavBlocker::UpdateOnRemove( void )
{
	UnblockNav();

	gm_NavBlockers.FindAndRemove( this );

	BaseClass::UpdateOnRemove();
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavBlocker::Spawn( void )
{
	gm_NavBlockers.AddToTail( this );

	if ( !m_blockedTeamNumber )
		m_blockedTeamNumber = TEAM_ANY;

	SetMoveType( MOVETYPE_NONE );
	SetModel( STRING( GetModelName() ) );
	AddEffects( EF_NODRAW );
	SetCollisionGroup( COLLISION_GROUP_NONE );
	SetSolid( SOLID_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );
	CollisionProp()->WorldSpaceAABB( &m_CachedMins, &m_CachedMaxs );

	if ( m_bDisabled )
	{
		UnblockNav();
	}
	else
	{
		BlockNav();
	}
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavBlocker::InputBlockNav( inputdata_t &inputdata )
{
	BlockNav();
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavBlocker::InputUnblockNav( inputdata_t &inputdata )
{
	UnblockNav();
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavBlocker::BlockNav( void )
{
	if ( m_blockedTeamNumber == TEAM_ANY )
	{
		for ( int i=0; i<MAX_NAV_TEAMS; ++i )
		{
			m_isBlockingNav[ i ] = true;
		}
	}
	else
	{
		int teamNumber = m_blockedTeamNumber % MAX_NAV_TEAMS;
		m_isBlockingNav[ teamNumber ] = true;
	}

	Extent extent;
	extent.Init( this );
	TheNavMesh->ForAllAreasOverlappingExtent( *this, extent );
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavBlocker::UnblockNav( void )
{
	if ( m_blockedTeamNumber == TEAM_ANY )
	{
		for ( int i=0; i<MAX_NAV_TEAMS; ++i )
		{
			m_isBlockingNav[ i ] = false;
		}
	}
	else
	{
		int teamNumber = m_blockedTeamNumber % MAX_NAV_TEAMS;
		m_isBlockingNav[ teamNumber ] = false;
	}

	UpdateBlocked();
}


//--------------------------------------------------------------------------------------------------------
// functor that blocks areas in our extent
bool CFuncNavBlocker::operator()( CNavArea *area )
{
	area->MarkAsBlocked( m_blockedTeamNumber, this );
	return true;
}


//--------------------------------------------------------------------------------------------------------
bool CFuncNavBlocker::CalculateBlocked( bool *pResultByTeam, const Vector &vecMins, const Vector &vecMaxs )
{
	int nTeamsBlocked = 0;
	int i;
	bool bBlocked = false;
	for ( i=0; i<MAX_NAV_TEAMS; ++i )
	{
		pResultByTeam[i] = false;
	}

	FOR_EACH_LL( gm_NavBlockers, iBlocker )
	{
		CFuncNavBlocker *pBlocker = gm_NavBlockers[iBlocker];
		bool bIsIntersecting = false;

		for ( i=0; i<MAX_NAV_TEAMS; ++i )
		{
			if ( pBlocker->m_isBlockingNav[i] )
			{
				if ( !pResultByTeam[i] )
				{
					if ( bIsIntersecting || ( bIsIntersecting = IsBoxIntersectingBox( pBlocker->m_CachedMins, pBlocker->m_CachedMaxs, vecMins, vecMaxs ) ) != false )
					{
						bBlocked = true;
						pResultByTeam[i] = true;
						nTeamsBlocked++;
					}
					else
					{
						continue;
					}
				}
			}
		}

		if ( nTeamsBlocked == MAX_NAV_TEAMS )
		{
			break;
		}
 	}
	return bBlocked;
}


//-----------------------------------------------------------------------------------------------------
/**
  * An entity that can obstruct nav areas.  This is meant for semi-transient areas that obstruct
  * pathfinding but can be ignored for longer-term queries like computing L4D flow distances and
  * escape routes.
  */
class CFuncNavObstruction : public CBaseEntity, public INavAvoidanceObstacle
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CFuncNavObstruction, CBaseEntity );

public:
	void Spawn();
	virtual void UpdateOnRemove( void );

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	virtual bool IsPotentiallyAbleToObstructNavAreas( void ) const { return true; }		// could we at some future time obstruct nav?
	virtual float GetNavObstructionHeight( void ) const { return JumpCrouchHeight; }	// height at which to obstruct nav areas
	virtual bool CanObstructNavAreas( void ) const { return !m_bDisabled; }				// can we obstruct nav right this instant?
	virtual CBaseEntity *GetObstructingEntity( void ) { return this; }
	virtual void OnNavMeshLoaded( void )
	{
		if ( !m_bDisabled )
		{
			ObstructNavAreas();
		}
	}

	int DrawDebugTextOverlays( void );

	bool operator()( CNavArea *area );	// functor that obstructs areas in our extent

private:

	void ObstructNavAreas( void );
	bool m_bDisabled;
};



//--------------------------------------------------------------------------------------------------------
BEGIN_DATADESC( CFuncNavObstruction )
	DEFINE_KEYFIELD( m_bDisabled,	FIELD_BOOLEAN,	"StartDisabled" ),
END_DATADESC()


LINK_ENTITY_TO_CLASS( func_nav_avoidance_obstacle, CFuncNavObstruction );


//-----------------------------------------------------------------------------------------------------
int CFuncNavObstruction::DrawDebugTextOverlays( void )
{
	int offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		if ( CanObstructNavAreas() )
		{
			EntityText( offset++, "Obstructing nav", NDEBUG_PERSIST_TILL_NEXT_SERVER );
		}
		else
		{
			EntityText( offset++, "Not obstructing nav", NDEBUG_PERSIST_TILL_NEXT_SERVER );
		}
	}

	return offset;
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavObstruction::UpdateOnRemove( void )
{
	TheNavMesh->UnregisterAvoidanceObstacle( this );

	BaseClass::UpdateOnRemove();
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavObstruction::Spawn( void )
{
	SetMoveType( MOVETYPE_NONE );
	SetModel( STRING( GetModelName() ) );
	AddEffects( EF_NODRAW );
	SetCollisionGroup( COLLISION_GROUP_NONE );
	SetSolid( SOLID_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );

	if ( !m_bDisabled )
	{
		ObstructNavAreas();
		TheNavMesh->RegisterAvoidanceObstacle( this );
	}
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavObstruction::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
	ObstructNavAreas();
	TheNavMesh->RegisterAvoidanceObstacle( this );
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavObstruction::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
	TheNavMesh->UnregisterAvoidanceObstacle( this );
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavObstruction::ObstructNavAreas( void )
{
	Extent extent;
	extent.Init( this );
	TheNavMesh->ForAllAreasOverlappingExtent( *this, extent );
}


//--------------------------------------------------------------------------------------------------------
// functor that blocks areas in our extent
bool CFuncNavObstruction::operator()( CNavArea *area )
{
	area->MarkObstacleToAvoid( GetNavObstructionHeight() );
	return true;
}


//--------------------------------------------------------------------------------------------------------------
