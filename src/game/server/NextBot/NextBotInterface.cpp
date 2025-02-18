// NextBotInterface.cpp
// Implentation of system methods for NextBot interface
// Author: Michael Booth, May 2006
//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "props.h"
#include "fmtstr.h"
#include "team.h"

#include "NextBotInterface.h"
#include "NextBotBodyInterface.h"
#include "NextBotManager.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// development only, off by default for 360
ConVar NextBotDebugHistory( "nb_debug_history", IsX360() ? "0" : "1", FCVAR_CHEAT, "If true, each bot keeps a history of debug output in memory" );

//----------------------------------------------------------------------------------------------------------------
INextBot::INextBot( void ) : m_debugHistory( MAX_NEXTBOT_DEBUG_HISTORY, 0 )	// CUtlVector: grow to max length, alloc 0 initially
{
	m_tickLastUpdate = -999;
	m_id = -1;
	m_componentList = NULL;
	m_debugDisplayLine = 0;

	m_immobileTimer.Invalidate();
	m_immobileCheckTimer.Invalidate();
	m_immobileAnchor = vec3_origin;

	m_currentPath = NULL;

	// register with the manager
	m_id = TheNextBots().Register( this );
}


//----------------------------------------------------------------------------------------------------------------
INextBot::~INextBot()
{
	ResetDebugHistory();

	// tell the manager we're gone
	TheNextBots().UnRegister( this );

	// delete Intention first, since destruction of Actions may access other components
	if ( m_baseIntention )
		delete m_baseIntention;

	if ( m_baseLocomotion )
		delete m_baseLocomotion;

	if ( m_baseBody )
		delete m_baseBody;

	if ( m_baseVision )
		delete m_baseVision;
}


//----------------------------------------------------------------------------------------------------------------
void INextBot::Reset( void )
{
	m_tickLastUpdate = -999;
	m_debugType = 0;
	m_debugDisplayLine = 0;

	m_immobileTimer.Invalidate();
	m_immobileCheckTimer.Invalidate();
	m_immobileAnchor = vec3_origin;

	for( INextBotComponent *comp = m_componentList; comp; comp = comp->m_nextComponent )
	{
		comp->Reset();
	}
}


//----------------------------------------------------------------------------------------------------------------
void INextBot::ResetDebugHistory( void )
{
	for ( int i=0; i<m_debugHistory.Count(); ++i )
	{
		delete m_debugHistory[i];
	}

	m_debugHistory.RemoveAll();
}


//----------------------------------------------------------------------------------------------------------------
bool INextBot::BeginUpdate()
{
	if ( TheNextBots().ShouldUpdate( this ) )
	{
		TheNextBots().NotifyBeginUpdate( this );
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------------------------------------------
void INextBot::EndUpdate( void )
{
	TheNextBots().NotifyEndUpdate( this );
}

//----------------------------------------------------------------------------------------------------------------
void INextBot::Update( void )
{
	VPROF_BUDGET( "INextBot::Update", "NextBot" );

	m_debugDisplayLine = 0;

	if ( IsDebugging( NEXTBOT_DEBUG_ALL ) )
	{
		CFmtStr msg;
		DisplayDebugText( msg.sprintf( "#%d", GetEntity()->entindex() ) );
	}

	UpdateImmobileStatus();

	// update all components
	for( INextBotComponent *comp = m_componentList; comp; comp = comp->m_nextComponent )
	{
		if ( comp->ComputeUpdateInterval() )
		{
			comp->Update();
		}
	}
}


//----------------------------------------------------------------------------------------------------------------
void INextBot::Upkeep( void )
{
	VPROF_BUDGET( "INextBot::Upkeep", "NextBot" );

	// do upkeep for all components
	for( INextBotComponent *comp = m_componentList; comp; comp = comp->m_nextComponent )
	{
		comp->Upkeep();
	}
}


//----------------------------------------------------------------------------------------------------------------
bool INextBot::SetPosition( const Vector &pos )
{
	IBody *body = GetBodyInterface();
	if (body)
	{
		return body->SetPosition( pos );
	}
	
	// fall back to setting raw entity position
	GetEntity()->SetAbsOrigin( pos );
	return true;
}


//----------------------------------------------------------------------------------------------------------------
const Vector &INextBot::GetPosition( void ) const
{
	return const_cast< INextBot * >( this )->GetEntity()->GetAbsOrigin();
}


//----------------------------------------------------------------------------------------------------------------
/**
 * Return true if given actor is our enemy
 */
bool INextBot::IsEnemy( const CBaseEntity *them ) const
{
	if ( them == NULL )
		return false;
		
	// this is not strictly correct, as spectators are not enemies
	return const_cast< INextBot * >( this )->GetEntity()->GetTeamNumber() != them->GetTeamNumber();
}


//----------------------------------------------------------------------------------------------------------------
/**
 * Return true if given actor is our friend
 */
bool INextBot::IsFriend( const CBaseEntity  *them ) const
{
	if ( them == NULL )
		return false;
		
	return const_cast< INextBot * >( this )->GetEntity()->GetTeamNumber() == them->GetTeamNumber();
}


//----------------------------------------------------------------------------------------------------------------
/**
 * Return true if 'them' is actually me
 */
bool INextBot::IsSelf( const CBaseEntity *them ) const
{
	if ( them == NULL )
		return false;

	return const_cast< INextBot * >( this )->GetEntity()->entindex() == them->entindex();
}


//----------------------------------------------------------------------------------------------------------------
/**
 * Components call this to register themselves with the bot that contains them
 */
void INextBot::RegisterComponent( INextBotComponent *comp )
{
	// add to head of singly linked list
	comp->m_nextComponent = m_componentList;
	m_componentList = comp;
}


//----------------------------------------------------------------------------------------------------------------
bool INextBot::IsRangeLessThan( CBaseEntity *subject, float range ) const
{
	Vector botPos;
	CBaseEntity *bot = const_cast< INextBot * >( this )->GetEntity();
	if ( !bot || !subject )
		return 0.0f;

	bot->CollisionProp()->CalcNearestPoint( subject->WorldSpaceCenter(), &botPos );
	float computedRange = subject->CollisionProp()->CalcDistanceFromPoint( botPos );
	return computedRange < range;
}


//----------------------------------------------------------------------------------------------------------------
bool INextBot::IsRangeLessThan( const Vector &pos, float range ) const
{
	Vector to = pos - GetPosition();
	return to.IsLengthLessThan( range );
}


//----------------------------------------------------------------------------------------------------------------
bool INextBot::IsRangeGreaterThan( CBaseEntity *subject, float range ) const
{
	Vector botPos;
	CBaseEntity *bot = const_cast< INextBot * >( this )->GetEntity();
	if ( !bot || !subject )
		return true;

	bot->CollisionProp()->CalcNearestPoint( subject->WorldSpaceCenter(), &botPos );
	float computedRange = subject->CollisionProp()->CalcDistanceFromPoint( botPos );
	return computedRange > range;
}


//----------------------------------------------------------------------------------------------------------------
bool INextBot::IsRangeGreaterThan( const Vector &pos, float range ) const
{
	Vector to = pos - GetPosition();
	return to.IsLengthGreaterThan( range );
}


//----------------------------------------------------------------------------------------------------------------
float INextBot::GetRangeTo( CBaseEntity *subject ) const
{
	Vector botPos;
	CBaseEntity *bot = const_cast< INextBot * >( this )->GetEntity();
	if ( !bot || !subject )
		return 0.0f;

	bot->CollisionProp()->CalcNearestPoint( subject->WorldSpaceCenter(), &botPos );
	float computedRange = subject->CollisionProp()->CalcDistanceFromPoint( botPos );
	return computedRange;
}


//----------------------------------------------------------------------------------------------------------------
float INextBot::GetRangeTo( const Vector &pos ) const
{
	Vector to = pos - GetPosition();
	return to.Length();
}


//----------------------------------------------------------------------------------------------------------------
float INextBot::GetRangeSquaredTo( CBaseEntity *subject ) const
{
	Vector botPos;
	CBaseEntity *bot = const_cast< INextBot * >( this )->GetEntity();
	if ( !bot || !subject )
		return 0.0f;

	bot->CollisionProp()->CalcNearestPoint( subject->WorldSpaceCenter(), &botPos );
	float computedRange = subject->CollisionProp()->CalcDistanceFromPoint( botPos );
	return computedRange * computedRange;
}


//----------------------------------------------------------------------------------------------------------------
float INextBot::GetRangeSquaredTo( const Vector &pos ) const
{
	Vector to = pos - GetPosition();
	return to.LengthSqr();	
}


//----------------------------------------------------------------------------------------------------------------
bool INextBot::IsDebugging( unsigned int type ) const
{
	if ( TheNextBots().IsDebugging( type ) )
	{
		return TheNextBots().IsDebugFilterMatch( this );
	}

	return false;
}


//----------------------------------------------------------------------------------------------------------------
/**
 * Return the name of this bot for debugging purposes
 */
const char *INextBot::GetDebugIdentifier( void ) const
{
	const int nameSize = 256;
	static char name[ nameSize ];
	
	Q_snprintf( name, nameSize, "%s(#%d)", const_cast< INextBot * >( this )->GetEntity()->GetClassname(), const_cast< INextBot * >( this )->GetEntity()->entindex() );

	return name;
}


//----------------------------------------------------------------------------------------------------------------
/**
 * Return true if we match the given debug symbol
 */
bool INextBot::IsDebugFilterMatch( const char *name ) const
{
	// compare debug identifier
	if ( !Q_strnicmp( name, GetDebugIdentifier(), Q_strlen( name ) ) )
	{
		return true;
	}

	// compare team name
	CTeam *team = GetEntity()->GetTeam();
	if ( team && !Q_strnicmp( name, team->GetName(), Q_strlen( name ) ) )
	{
		return true;
	}


	return false;
}


//----------------------------------------------------------------------------------------------------------------
/**
 * There are some things we never want to climb on
 */
bool INextBot::IsAbleToClimbOnto( const CBaseEntity *object ) const
{
	if ( object == NULL || !const_cast<CBaseEntity *>(object)->IsAIWalkable() )
	{
		return false;
	}

	// never climb onto doors
	if ( FClassnameIs( const_cast< CBaseEntity * >( object ), "prop_door*" ) || FClassnameIs( const_cast< CBaseEntity * >( object ), "func_door*" ) )
	{
		return false;
	}

	// ok to climb on this object
	return true;
}


//----------------------------------------------------------------------------------------------------------------
/**
 * Can we break this object
 */
bool INextBot::IsAbleToBreak( const CBaseEntity *object ) const
{
	if ( object && object->m_takedamage == DAMAGE_YES )
	{
		if ( FClassnameIs( const_cast< CBaseEntity * >( object ), "func_breakable" ) && 
			 object->GetHealth() )
		{
			return true;
		}

		if ( FClassnameIs( const_cast< CBaseEntity * >( object ), "func_breakable_surf" ) )
		{
			return true;
		}

		if ( dynamic_cast< const CBreakableProp * >( object ) != NULL )
		{
			return true;
		}
	}

	return false;
}


//----------------------------------------------------------------------------------------------------------
void INextBot::DisplayDebugText( const char *text ) const
{
	const_cast< INextBot * >( this )->GetEntity()->EntityText( m_debugDisplayLine++, text, 0.1 );
}


//--------------------------------------------------------------------------------------------------------
void INextBot::DebugConColorMsg( NextBotDebugType debugType, const Color &color, const char *fmt, ... )
{
	bool isDataFormatted = false;

	va_list argptr;
	char data[ MAX_NEXTBOT_DEBUG_LINE_LENGTH ];

	if ( developer.GetBool() && IsDebugging( debugType ) )
	{
		va_start(argptr, fmt);
		Q_vsnprintf(data, sizeof( data ), fmt, argptr);
		va_end(argptr);
		isDataFormatted = true;

		ConColorMsg( color, "%s", data );
	}

	if ( !NextBotDebugHistory.GetBool() )
	{
		if ( m_debugHistory.Count() )
		{
			ResetDebugHistory();
		}
		return;
	}

	// Don't bother with event data - it's spammy enough to overshadow everything else.
	if ( debugType == NEXTBOT_EVENTS )
		return;

	if ( !isDataFormatted )
	{
		va_start(argptr, fmt);
		Q_vsnprintf(data, sizeof( data ), fmt, argptr);
		va_end(argptr);
		isDataFormatted = true;
	}

	int lastLine = m_debugHistory.Count() - 1;
	if ( lastLine >= 0 )
	{
		NextBotDebugLineType *line = m_debugHistory[lastLine];
		if ( line->debugType == debugType && V_strstr( line->data, "\n" ) == NULL )
		{
			// append onto previous line
			V_strncat( line->data, data, MAX_NEXTBOT_DEBUG_LINE_LENGTH );
			return;
		}
	}

	// Prune out an old line if needed, keeping a pointer to re-use the memory
	NextBotDebugLineType *line = NULL;
	if ( m_debugHistory.Count() == MAX_NEXTBOT_DEBUG_HISTORY )
	{
		line = m_debugHistory[0];
		m_debugHistory.Remove( 0 );
	}

	// Add to debug history
	if ( !line )
	{
		line = new NextBotDebugLineType;
	}
	line->debugType = debugType;
	V_strncpy( line->data, data, MAX_NEXTBOT_DEBUG_LINE_LENGTH );
	m_debugHistory.AddToTail( line );
}


//--------------------------------------------------------------------------------------------------------
// build a vector of debug history of the given types
void INextBot::GetDebugHistory( unsigned int type, CUtlVector< const NextBotDebugLineType * > *lines ) const
{
	if ( !lines )
		return;

	lines->RemoveAll();

	for ( int i=0; i<m_debugHistory.Count(); ++i )
	{
		NextBotDebugLineType *line = m_debugHistory[i];
		if ( line->debugType & type )
		{
			lines->AddToTail( line );
		}
	}
}


//--------------------------------------------------------------------------------------------------------
void INextBot::UpdateImmobileStatus( void )
{
	if ( m_immobileCheckTimer.IsElapsed() )
	{
		m_immobileCheckTimer.Start( 1.0f );

		// if we haven't moved farther than this in 1 second, we're immobile
		if ( ( GetEntity()->GetAbsOrigin() - m_immobileAnchor ).IsLengthGreaterThan( GetImmobileSpeedThreshold() ) )
		{
			// moved far enough, not immobile
			m_immobileAnchor = GetEntity()->GetAbsOrigin();
			m_immobileTimer.Invalidate();
		}
		else
		{
			// haven't escaped our anchor - we are immobile
			if ( !m_immobileTimer.HasStarted() )
			{
				m_immobileTimer.Start();
			}
		}
	}
}

