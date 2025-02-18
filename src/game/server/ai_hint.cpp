//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Hint node utilities and functions
//
// $NoKeywords: $
//=============================================================================//

// @TODO (toml 03-04-03): there is far too much duplicate code in here

#include "cbase.h"
#include "ai_hint.h"
#include "ai_network.h"
#include "ai_node.h"
#include "ai_basenpc.h"
#include "ai_networkmanager.h"
#include "ndebugoverlay.h"
#include "animation.h"
#include "tier1/strtools.h"
#include "mapentities_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define REPORTFAILURE(text) if ( hintCriteria.HasFlag( bits_HINT_NODE_REPORT_FAILURES ) ) \
								NDebugOverlay::Text( GetAbsOrigin(), text, false, 60 )

//==================================================
// CHintCriteria
//==================================================

CHintCriteria::CHintCriteria( void )
{
	m_iFirstHintType = HINT_NONE;
	m_iLastHintType = HINT_NONE;
	m_strGroup		= NULL_STRING;
	m_iFlags		= 0;
	m_HintTypes.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CHintCriteria::~CHintCriteria( void )
{
	m_zoneInclude.Purge();
	m_zoneExclude.Purge();
	m_HintTypes.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the hint type for this search criteria
// Input  : nHintType - the hint type for this search criteria
//-----------------------------------------------------------------------------
void CHintCriteria::SetHintType( int nHintType )
{
	m_iFirstHintType = nHintType;
	m_iLastHintType = HINT_NONE;
	m_HintTypes.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Add another type of hint that matches the search criteria
//-----------------------------------------------------------------------------
void CHintCriteria::AddHintType( int hintType )
{
	m_HintTypes.AddToTail( hintType );
}

int CHintCriteria::NumHintTypes() const
{
	return m_HintTypes.Count();
}

int	CHintCriteria::GetHintType( int idx ) const
{
	return m_HintTypes[ idx ];
}

bool CHintCriteria::MatchesSingleHintType() const
{ 
	if ( m_HintTypes.Count() != 0 )
	{
		return false;
	}

	if ( m_iFirstHintType != HINT_ANY && 
		    m_iLastHintType == HINT_NONE )
	{
		return true;
	}
	
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHintCriteria::MatchesHintType( int hintType ) const
{
	int c = m_HintTypes.Count();
 	for ( int i = 0; i < c; ++i )
	{
		if ( m_HintTypes[i] == hintType )
			return true;
	}

	// See if we're trying to filter the nodes
	if ( GetFirstHintType() != HINT_ANY )
	{
		if( GetLastHintType() == HINT_NONE )
		{
			// Searching for a single type of hint.
			if( GetFirstHintType() != hintType )
				return false;
		}
		else
		{
			// This search is for a range of hint types.
			if( hintType < GetFirstHintType() || hintType > GetLastHintType() )
				return false;
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Allows us to search for nodes within a range of consecutive types.
//-----------------------------------------------------------------------------
void CHintCriteria::SetHintTypeRange( int firstType, int lastType )
{
	if( lastType < firstType )
	{
		DevMsg( 2, "Hint Type Range is backwards - Fixing up.\n" );

		int temp;

		temp = firstType;
		firstType = lastType;
		lastType = temp;
	}

	m_iFirstHintType = firstType;
	m_iLastHintType = lastType;
	m_HintTypes.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bitmask - 
//-----------------------------------------------------------------------------
void CHintCriteria::SetFlag( int bitmask )
{
	m_iFlags |= bitmask;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bitmask - 
//-----------------------------------------------------------------------------
void CHintCriteria::ClearFlag( int bitmask )
{
	m_iFlags &= ~bitmask;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : group - 
//-----------------------------------------------------------------------------
void CHintCriteria::SetGroup( string_t group )
{
	m_strGroup = group;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a zone to a zone list
// Input  : list - the list of zones to add the new zone to
//			&position - the origin point of the zone
//			radius - the radius of the zone
//-----------------------------------------------------------------------------
void CHintCriteria::AddZone( zoneList_t &list, const Vector &position, float radius )
{
	int id = list.AddToTail();
	list[id].position	= position;
	list[id].radiussqr	= radius*radius;
}

//-----------------------------------------------------------------------------
// Purpose: Adds an include zone to the search criteria
// Input  : &position - the origin point of the zone
//			radius - the radius of the zone
//-----------------------------------------------------------------------------
void CHintCriteria::AddIncludePosition( const Vector &position, float radius )
{
	AddZone( m_zoneInclude, position, radius );
}

//-----------------------------------------------------------------------------
// Purpose: Adds an exclude zone to the search criteria
// Input  : &position - the origin point of the zone
//			radius - the radius of the zone
//-----------------------------------------------------------------------------
void CHintCriteria::AddExcludePosition( const Vector &position, float radius )
{
	AddZone( m_zoneExclude, position, radius );
}

//-----------------------------------------------------------------------------
// Purpose: Test to see if this position falls within any of the zones in the list
// Input  : *zone - list of zones to test against
//			&testPosition - position to test with
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
inline bool CHintCriteria::InZone( const zoneList_t &zone, const Vector &testPosition ) const
{
	int	numZones = zone.Count();

	//Iterate through all zones in the list
	for ( int i = 0; i < numZones; i++ )
	{
		if ( ((zone[i].position) - testPosition).LengthSqr() < (zone[i].radiussqr) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Determine if a point within our include list
// Input  : &testPosition - position to test with
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHintCriteria::InIncludedZone( const Vector &testPosition ) const
{
	return InZone( m_zoneInclude, testPosition );
}

//-----------------------------------------------------------------------------
// Purpose: Determine if a point within our exclude list
// Input  : &testPosition - position to test with
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHintCriteria::InExcludedZone( const Vector &testPosition ) const
{
	return InZone( m_zoneExclude, testPosition );
}

//-----------------------------------------------------------------------------
// Init static variables
//-----------------------------------------------------------------------------
CAIHintVector CAI_HintManager::gm_AllHints;
CUtlMap< int,  CAIHintVector >	CAI_HintManager::gm_TypedHints( 0, 0, DefLessFunc( int ) );
CAI_Hint*	CAI_HintManager::gm_pLastFoundHints[ CAI_HintManager::HINT_HISTORY ];
int			CAI_HintManager::gm_nFoundHintIndex = 0;

CAI_Hint *CAI_HintManager::AddFoundHint( CAI_Hint *hint )
{
	if ( hint )
	{
		CAI_HintManager::gm_nFoundHintIndex = ( CAI_HintManager::gm_nFoundHintIndex + 1 ) & CAI_HintManager::HINT_HISTORY_MASK;
		gm_pLastFoundHints[ CAI_HintManager::gm_nFoundHintIndex ] = hint;
	}
	return hint;
	
}

int CAI_HintManager::GetFoundHintCount()
{
	return CAI_HintManager::HINT_HISTORY;
}

CAI_Hint *CAI_HintManager::GetFoundHint( int index )
{
	return gm_pLastFoundHints[ ( CAI_HintManager::gm_nFoundHintIndex + index ) & CAI_HintManager::HINT_HISTORY_MASK ];
}

CAI_Hint *CAI_HintManager::GetLastFoundHint()
{
	for ( int i = 0; i < CAI_HintManager::HINT_HISTORY; ++i )
	{
		// Walk backward
		int slot = ( ( CAI_HintManager::gm_nFoundHintIndex - i ) & CAI_HintManager::HINT_HISTORY_MASK );
		if ( gm_pLastFoundHints[ slot ] )
			return gm_pLastFoundHints[ slot ];
	}
	return NULL;
}

void CAI_HintManager::ResetFoundHints()
{
	Q_memset( gm_pLastFoundHints, 0, sizeof( gm_pLastFoundHints ) );
	CAI_HintManager::gm_nFoundHintIndex = 0;
}

bool CAI_HintManager::IsInFoundHintList( CAI_Hint *hint )
{
	for ( int i = 0; i < CAI_HintManager::HINT_HISTORY; ++i )
	{
		if ( gm_pLastFoundHints[ i ] == hint )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
int CAI_HintManager::FindAllHints( CAI_BaseNPC *pNPC, const Vector &position, const CHintCriteria &hintCriteria, CUtlVector<CAI_Hint *> *pResult )
{
	//  If we have no hints, bail
	int c = CAI_HintManager::gm_AllHints.Count();
	if ( !c )
		return NULL;

	// Remove the nearest flag. It makes now sense with random.
	bool hadNearest = hintCriteria.HasFlag( bits_HINT_NODE_NEAREST );
	(const_cast<CHintCriteria &>(hintCriteria)).ClearFlag( bits_HINT_NODE_NEAREST );

	//  Now loop till we find a valid hint or return to the start
	CAI_Hint *pTestHint;
	for ( int i = 0; i < c; ++i )
	{
		pTestHint = CAI_HintManager::gm_AllHints[ i ];
		Assert( pTestHint );
		if ( pTestHint->HintMatchesCriteria( pNPC, hintCriteria, position, NULL ) )
			pResult->AddToTail( pTestHint );
	}

	if ( hadNearest )
		(const_cast<CHintCriteria &>(hintCriteria)).SetFlag( bits_HINT_NODE_NEAREST );

	return pResult->Count();
}

//-----------------------------------------------------------------------------
// Purpose: Finds a random hint within the requested radious of the npc
//  Builds a list of all suitable hints and chooses randomly from amongst them.
// Input  : *pNPC - 
//			nHintType - 
//			nFlags - 
//			flMaxDist - 
// Output : CAI_Hint
//-----------------------------------------------------------------------------
CAI_Hint *CAI_HintManager::FindHintRandom( CAI_BaseNPC *pNPC, const Vector &position, const CHintCriteria &hintCriteria )
{
	CUtlVector<CAI_Hint *> hintList;

	if ( FindAllHints( pNPC, position, hintCriteria, &hintList ) > 0 )
	{
		// Pick one randomly
		return ( CAI_HintManager::AddFoundHint( hintList[ random->RandomInt( 0, hintList.Size() - 1 ) ]  ) );
	}

	// start at the top of the list for the next search
	CAI_HintManager::ResetFoundHints();
	return NULL;
}

// #define HINT_PROFILING 1
#if defined( HINT_PROFILING )
static void AppendTimer( int idx, char *buf, size_t bufsize, CFastTimer& timer )
{
	char s[ 32 ];
	Q_snprintf( s, sizeof( s ), "%d %6.3f ms", idx, timer.GetDuration().GetMillisecondsF() );

	Q_strncat( buf, s, bufsize );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *hintCriteria - 
// Output : CAI_Hint
//-----------------------------------------------------------------------------
CAI_Hint *CAI_HintManager::FindHint( CAI_BaseNPC *pNPC, const Vector &position, const CHintCriteria &hintCriteria )
{
#if defined( HINT_PROFILING )
	CFastTimer timer;
	timer.Start();
#endif
	bool singleType = hintCriteria.MatchesSingleHintType();
	bool lookingForNearest = hintCriteria.HasFlag( bits_HINT_NODE_NEAREST );
	bool bIgnoreHintType = true;

	CUtlVector< CAIHintVector * > lists;
	if ( singleType )
	{
		int slot = CAI_HintManager::gm_TypedHints.Find( hintCriteria.GetFirstHintType() );
		if ( slot != CAI_HintManager::gm_TypedHints.InvalidIndex() )
		{
			lists.AddToTail( &CAI_HintManager::gm_TypedHints[ slot ] );
		}
	}
	else
	{
		int typeCount = hintCriteria.NumHintTypes();
		if ( typeCount > 0 )
		{
			for ( int listType = 0; listType < typeCount; ++listType )
			{
				int slot = CAI_HintManager::gm_TypedHints.Find( hintCriteria.GetHintType( listType ) );
				if ( slot != CAI_HintManager::gm_TypedHints.InvalidIndex() )
				{
					lists.AddToTail( &CAI_HintManager::gm_TypedHints[ slot ] );
				}
			}
		}
		else
		{
			// Still need to check hint type in this case
			lists.AddToTail( &CAI_HintManager::gm_AllHints );
			bIgnoreHintType = false;
		}
	}

	CAI_Hint *pBestHint	= NULL;

	int visited = 0;

	int listCount = lists.Count();

	if ( listCount == 0 )
		return NULL;
	
	// Try the fast match path
	int i, count;
	// Start with hint after the last one used
	CAI_Hint *pTestHint = NULL;

	float flBestDistance = MAX_TRACE_LENGTH;

	if ( !lookingForNearest )
	{
		// Fast check of previous results
		count = CAI_HintManager::GetFoundHintCount();
		for ( i = 0; i < count; ++i )
		{
			pTestHint = CAI_HintManager::GetFoundHint( i );
			if ( pTestHint )
			{
				Assert( dynamic_cast<CAI_Hint *>(pTestHint) != NULL );
				++visited;
				if ( pTestHint->HintMatchesCriteria( pNPC, hintCriteria, position, &flBestDistance ) )
				{
#if defined( HINT_PROFILING )
					Msg( "fast result visited %d\n", visited );
#endif
					return pTestHint;
				}
			}
		}
	}

	// Longer search, reset best distance
	flBestDistance = MAX_TRACE_LENGTH;

	for ( int listNum = 0; listNum < listCount; ++listNum )
	{
		CAIHintVector *list = lists[ listNum ];
		count = list->Count();
		// -------------------------------------------
		//  If we have no hints, bail
		// -------------------------------------------
		if ( !count )
			continue;

		//  Now loop till we find a valid hint or return to the start
		for ( i = 0 ; i < count; ++i )
		{
			pTestHint = list->Element( i );
			Assert( pTestHint );

			++visited;

			Assert( dynamic_cast<CAI_Hint *>(pTestHint) != NULL );
			if ( pTestHint->HintMatchesCriteria( pNPC, hintCriteria, position, &flBestDistance, false, bIgnoreHintType ) )
			{
				// If we were searching for the nearest, just note that this is now the nearest node
				if ( lookingForNearest )
				{
					pBestHint = pTestHint;
				}
				else 
				{
					// If we're not looking for the nearest, we're done
					CAI_HintManager::AddFoundHint( pTestHint );
#if defined( HINT_PROFILING )
					Msg( "visited %d\n", visited );
#endif
					return pTestHint;
				}
			}
		} 
	}
	// Return the nearest node that we found
	if ( pBestHint )
	{
		CAI_HintManager::AddFoundHint( pBestHint );
	}
	
#if defined( HINT_PROFILING )
	timer.End();

	Msg( "visited %d\n", visited );
	if ( !pBestHint )
	{
		Msg( "%i search failed for [%d] at pos %.3f %.3f %.3f [%.4f msec ~ %.4f msec per node]\n",
			gpGlobals->tickcount,
			pNPC ? pNPC->entindex() : -1,
			position.x, position.y, position.z,
			timer.GetDuration().GetMillisecondsF(),
			timer.GetDuration().GetMillisecondsF()/MAX( (float)visited, 1.0f ) );
	}
#endif
	return pBestHint;
}

//-----------------------------------------------------------------------------
// Purpose: Searches for a hint node that this NPC cares about. If one is
//			claims that hint node for this NPC so that no other NPCs
//			try to use it.
//
// Input  : nFlags - Search criterea. Can currently be one or more of the following:
//				bits_HINT_NODE_VISIBLE - searches for visible hint nodes.
//				bits_HINT_NODE_RANDOM - calls through the FindHintRandom and builds list of all matching
//				nodes and picks randomly from among them.  Note:  Depending on number of hint nodes, this
//				could be slower, so use with care.
//
// Output : Returns pointer to hint node if available hint node was found that matches the
//			given criterea that this NPC also cares about. Otherwise, returns NULL
//-----------------------------------------------------------------------------
CAI_Hint* CAI_HintManager::FindHint( CAI_BaseNPC *pNPC, Hint_e nHintType, int nFlags, float flMaxDist, const Vector *pMaxDistFrom )
{
	assert( pNPC != NULL );
	if ( pNPC == NULL )
		return NULL;

	CHintCriteria	hintCriteria;
	hintCriteria.SetHintType( nHintType );
	hintCriteria.SetFlag( nFlags );

	// Using the NPC's hint group?
	if ( nFlags & bits_HINT_NODE_USE_GROUP )
	{
		hintCriteria.SetGroup( pNPC->GetHintGroup() );
	}

	// Add the search position
	Vector vecPosition = ( pMaxDistFrom != NULL ) ? (*pMaxDistFrom) : pNPC->GetAbsOrigin();
	hintCriteria.AddIncludePosition( vecPosition, flMaxDist );

	// If asking for a random node, use random logic instead
	if ( nFlags & bits_HINT_NODE_RANDOM )
		return FindHintRandom( pNPC, vecPosition, hintCriteria );

	return FindHint( pNPC, vecPosition, hintCriteria );
}

//-----------------------------------------------------------------------------
// Purpose: Position only search
// Output : CAI_Hint
//-----------------------------------------------------------------------------
CAI_Hint *CAI_HintManager::FindHint( const Vector &position, const CHintCriteria &hintCriteria )
{
	return FindHint( NULL, position, hintCriteria );
}

//-----------------------------------------------------------------------------
// Purpose: NPC only search
// Output : CAI_Hint
//-----------------------------------------------------------------------------
CAI_Hint *CAI_HintManager::FindHint( CAI_BaseNPC *pNPC, const CHintCriteria &hintCriteria )
{
	assert( pNPC != NULL );
	if ( pNPC == NULL )
		return NULL;

	return FindHint( pNPC, pNPC->GetAbsOrigin(), hintCriteria );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CAI_Hint* CAI_HintManager::CreateHint( HintNodeData *pNodeData, const char *pMapData )
{
	// Reset last found hint if new node is added
	CAI_HintManager::ResetFoundHints();

	CAI_Hint *pHint = (CAI_Hint*)CreateEntityByName("ai_hint");
	if ( pHint )
	{	
		// First, parse the mapdata chunk we were passed
		if ( pMapData )
		{
			CEntityMapData entData( (char*)pMapData );
			pHint->ParseMapData( &entData );

			// Restore the desired classname (parsing the mapdata stomps it)
			pHint->SetClassname( "ai_hint" );
		}

		pHint->SetName( pNodeData->strEntityName );
		pHint->SetAbsOrigin( pNodeData->vecPosition );
		memcpy( &(pHint->m_NodeData), pNodeData, sizeof(HintNodeData) );
		DispatchSpawn( pHint );

		return pHint;
	}

	return NULL;
}

//------------------------------------------------------------------------------
void CAI_HintManager::AddHint( CAI_Hint *pHint )
{
	// ---------------------------------
	//  Add to linked list of hints
	// ---------------------------------
	CAI_HintManager::gm_AllHints.AddToTail( pHint );
	CAI_HintManager::AddHintByType( pHint );
}

void CAI_Hint::SetHintType( int hintType, bool force /*= false*/ )
{
	if ( !force && hintType == m_NodeData.nHintType )
		return;

	CAI_HintManager::RemoveHintByType( this );
	m_NodeData.nHintType = hintType;
	CAI_HintManager::AddHintByType( this );
}

void CAI_HintManager::AddHintByType( CAI_Hint *pHint )
{
	Hint_e type = pHint->HintType();

	int slot = CAI_HintManager::gm_TypedHints.Find( type );
	if ( slot == CAI_HintManager::gm_TypedHints.InvalidIndex() )
	{
		slot = CAI_HintManager::gm_TypedHints.Insert( type);
	}
	CAI_HintManager::gm_TypedHints[ slot ].AddToTail( pHint );
}

void CAI_HintManager::RemoveHintByType( CAI_Hint *pHintToRemove )
{
	int slot = CAI_HintManager::gm_TypedHints.Find( pHintToRemove->HintType() );
	if ( slot != CAI_HintManager::gm_TypedHints.InvalidIndex() )
	{
		CAI_HintManager::gm_TypedHints[ slot ].FindAndRemove( pHintToRemove );
	}
}

//------------------------------------------------------------------------------
void CAI_HintManager::RemoveHint( CAI_Hint *pHintToRemove )
{
	// --------------------------------------
	//  Remove from linked list of hints
	// --------------------------------------
	gm_AllHints.FindAndRemove( pHintToRemove );
	RemoveHintByType( pHintToRemove );

	if ( CAI_HintManager::IsInFoundHintList( pHintToRemove ) )
	{
		CAI_HintManager::ResetFoundHints();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *token - 
// Output : int
//-----------------------------------------------------------------------------
int CAI_HintManager::GetFlags( const char *token )
{
	int len = strlen( token );
	if ( len <= 0 )
	{
		return bits_HINT_NODE_NONE;
	}

	char *lowercase = (char *)_alloca( len + 1 );
	Q_strncpy( lowercase, token, len+1 );
	strlwr( lowercase );

	if ( strstr( "none", lowercase ) )
	{
		return bits_HINT_NODE_NONE;
	}

	int bits = 0;

	if ( strstr( "visible", lowercase ) )
	{
		bits |= bits_HINT_NODE_VISIBLE;
	}

	if ( strstr( "nearest", lowercase ) )
	{
		bits |= bits_HINT_NODE_NEAREST;
	}

	if ( strstr( "random", lowercase ) )
	{
		bits |= bits_HINT_NODE_RANDOM;
	}

	// Can't be nearest and random, defer to nearest
	if ( ( bits & bits_HINT_NODE_NEAREST ) &&
		 ( bits & bits_HINT_NODE_RANDOM ) )
	{
		// Remove random
		bits &= ~bits_HINT_NODE_RANDOM;

		DevMsg( "HINTFLAGS:%s, inconsistent, the nearest node is never a random hint node, treating as nearest request!\n",
			token );
	}

	return bits;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAI_Hint *CAI_HintManager::GetFirstHint( AIHintIter_t *pIter )					
{ 
	if ( !gm_AllHints.Count() )
	{
		*pIter = (AIHintIter_t)(intp)gm_AllHints.InvalidIndex();
		return NULL;
	}
	*pIter = (AIHintIter_t)0; 
	return gm_AllHints[0]; 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAI_Hint *CAI_HintManager::GetNextHint(  AIHintIter_t *pIter )
{
	int iterAsInt = size_cast< int >( (intp )*pIter );

	if ( iterAsInt != gm_AllHints.InvalidIndex() )
	{
		intp i = iterAsInt + 1;
		if ( gm_AllHints.Count() <= i )
		{
			*pIter = (AIHintIter_t)(intp)gm_AllHints.InvalidIndex();
			return NULL;
		}
		*pIter = (AIHintIter_t)i; 
		return gm_AllHints[i]; 
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_HintManager::DumpHints()
{
	AIHintIter_t iter;
	CAI_Hint *pCurHint = GetFirstHint( &iter );
	while (pCurHint)
	{
		const Vector &v = pCurHint->GetAbsOrigin();
		Msg( "(%.1f, %.1f, %.1f) -- Node ID: %d; WC id %d; type %d\n",
				v.x, v.y, v.z,
				pCurHint->GetNodeId(),
				pCurHint->GetWCId(),
				pCurHint->HintType() );
		pCurHint = GetNextHint( &iter );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_HintManager::ValidateHints()
{
#ifdef _DEBUG
	int nTyped = 0;
	FOR_EACH_VEC( gm_AllHints, i )
	{
		Assert( dynamic_cast<CAI_Hint *>(gm_AllHints[i]) != NULL );
	}

	for ( int i = gm_TypedHints.FirstInorder(); i != gm_TypedHints.InvalidIndex(); i = gm_TypedHints.NextInorder( i ) )
	{
		FOR_EACH_VEC( gm_TypedHints[i], j )
		{
			nTyped++;
			Assert( dynamic_cast<CAI_Hint *>(gm_TypedHints[i][j]) != NULL );
		}
	}

	Assert( gm_AllHints.Count() == nTyped );
#endif
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_HintManager::DrawHintOverlays(float flDrawDuration)
{
	int c = gm_AllHints.Count();
	for ( int i = 0; i < c; ++i )
	{
        CAI_Hint *pHint = gm_AllHints[ i ];
		int		r		= 0;
		int		g		= 0;
		int		b		= 255;
		Vector	vHintPos;

		if (pHint->m_NodeData.nNodeID != NO_NODE)
		{
			vHintPos = g_pBigAINet->GetNode(pHint->m_NodeData.nNodeID)->GetPosition(g_pAINetworkManager->GetEditOps()->m_iHullDrawNum);
		}
		else
		{
			vHintPos = pHint->GetAbsOrigin();
		}

		if ( pHint->GetNodeId() != NO_NODE )
			NDebugOverlay::Text( vHintPos + Vector(0,6,8), CFmtStr("(%d), (%d)", pHint->HintType(), pHint->GetNodeId()), true, flDrawDuration );
		else
			NDebugOverlay::Text( vHintPos + Vector(0,6,8), CFmtStr("(%d)", pHint->HintType()), true, flDrawDuration );

		// If node is currently locked
		if (pHint->m_NodeData.iDisabled)
		{
			r = 100;
			g = 100;
			b = 100;
		}
		else if (pHint->m_hHintOwner != NULL)
		{
			r = 255;
			g = 0;
			b = 0;

			CBaseEntity* pOwner = pHint->User();
			if (pOwner)
			{
				char owner[255];
				Q_strncpy(owner,pOwner->GetDebugName(),sizeof(owner));
				Vector loc = vHintPos;
				loc.x+=6;
				loc.y+=6;
				loc.z+=6;
				NDebugOverlay::Text( loc, owner, true, flDrawDuration );
				NDebugOverlay::Line( vHintPos, pOwner->WorldSpaceCenter(), 128, 128, 128, false, 0);
			}
		}
		else if (pHint->IsLocked())
		{
			r = 200;
			g = 150;
			b = 10;
		}

		NDebugOverlay::Box(vHintPos, Vector(-3,-3,-3), Vector(3,3,3), r,g,b,0,flDrawDuration);

		// Draw line in facing direction
		Vector offsetDir	= 12.0 * Vector(cos(DEG2RAD(pHint->Yaw())),sin(DEG2RAD(pHint->Yaw())),0);
		NDebugOverlay::Line(vHintPos, vHintPos+offsetDir, r,g,b,false,flDrawDuration);
	}
}

//##################################################################
// > CAI_Hint
//##################################################################
LINK_ENTITY_TO_CLASS( ai_hint, CAI_Hint );

BEGIN_DATADESC( CAI_Hint )

	DEFINE_EMBEDDED( m_NodeData ),
	//				m_nTargetNodeID (reset on load)

	DEFINE_FIELD(	 m_hHintOwner,		FIELD_EHANDLE),
	DEFINE_FIELD(	 m_flNextUseTime,	FIELD_TIME),
	DEFINE_FIELD(	 m_vecForward,		FIELD_VECTOR),
	DEFINE_KEYFIELD( m_nodeFOV,			FIELD_FLOAT,	"nodeFOV" ),

	DEFINE_THINKFUNC( EnableThink ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID,		"EnableHint",		InputEnableHint ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"DisableHint",		InputDisableHint ),

	// Outputs
	DEFINE_OUTPUT( m_OnNPCStartedUsing,	"OnNPCStartedUsing" ),
	DEFINE_OUTPUT( m_OnNPCStoppedUsing,	"OnNPCStoppedUsing" ),

END_DATADESC( );

//------------------------------------------------------------------------------
// Purpose : 
//------------------------------------------------------------------------------
void CAI_Hint::InputEnableHint( inputdata_t &inputdata )
{
	m_NodeData.iDisabled		= false;
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CAI_Hint::InputDisableHint( inputdata_t &inputdata )
{
	m_NodeData.iDisabled		= true;
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_Hint::Spawn( void )
{
	// Cache off the forward vector
	GetVectors( &m_vecForward, NULL, NULL );

	if( m_nodeFOV != 360 )
	{
		// As a micro-optimization, leave the FOV at 360 to save us
		// a dot product later when checking node FOV.
		m_nodeFOV = cos( DEG2RAD(m_nodeFOV/2) );
	}

	SetSolid( SOLID_NONE );
}

void CAI_Hint::Activate()
{
	BaseClass::Activate();
	CAI_HintManager::AddHint( this );
}

void CAI_Hint::UpdateOnRemove( void )
{
	CAI_HintManager::RemoveHint( this );
	BaseClass::UpdateOnRemove();
}

//------------------------------------------------------------------------------
// Purpose :  If connected to a node returns node position, otherwise
//			  returns local hint position
//
//			  NOTE: Assumes not using multiple AI networks  
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_Hint::GetPosition(CBaseCombatCharacter *pBCC, Vector *vPosition)
{
	if ( m_NodeData.nNodeID != NO_NODE )
	{
		*vPosition = g_pBigAINet->GetNodePosition( pBCC, m_NodeData.nNodeID );
	}
	else
	{
		*vPosition = GetAbsOrigin();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : hull - 
//			*vPosition - 
//-----------------------------------------------------------------------------
void CAI_Hint::GetPosition( Hull_t hull, Vector *vPosition )
{
	if ( m_NodeData.nNodeID != NO_NODE )
	{
		*vPosition = g_pBigAINet->GetNodePosition( hull, m_NodeData.nNodeID );
	}
	else
	{
		*vPosition = GetAbsOrigin();
	}
}

//------------------------------------------------------------------------------
// Purpose :  If connected to a node returns node direction, otherwise
//			  returns local hint direction
//
//			  NOTE: Assumes not using multiple AI networks  
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_Hint::GetDirection( )
{
	return UTIL_YawToVector( Yaw() );
}

//------------------------------------------------------------------------------
// Purpose :  If connected to a node returns node yaw, otherwise
//			  returns local hint yaw
//
//			  NOTE: Assumes not using multiple AI networks  
// Input   :
// Output  :
//------------------------------------------------------------------------------
float CAI_Hint::Yaw(void)
{
	if (m_NodeData.nNodeID != NO_NODE)
	{
		return g_pBigAINet->GetNodeYaw(m_NodeData.nNodeID );
	}
	else
	{
		return GetLocalAngles().y;
	}
}



//------------------------------------------------------------------------------
// Purpose :  Returns if this is something that's interesting to look at
//
//			  NOTE: Assumes not using multiple AI networks  
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CAI_Hint::IsViewable(void)
{
	if (m_NodeData.iDisabled)
	{
		return false;
	}

	switch( HintType() )
	{
	case HINT_WORLD_VISUALLY_INTERESTING:
	case HINT_WORLD_VISUALLY_INTERESTING_DONT_AIM:
	case HINT_WORLD_VISUALLY_INTERESTING_STEALTH:
		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CAI_Hint::IsInNodeFOV( CBaseEntity *pOther )
{
	if( m_nodeFOV == 360 )
	{
		return true;
	}

#if 0 
	NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + m_vecForward * 16, 255, 255, 0, false, 1 );
#endif

	Vector vecToNPC = pOther->GetAbsOrigin() - GetAbsOrigin();
	VectorNormalize( vecToNPC );
	float flDot = DotProduct( vecToNPC, m_vecForward );

	if( flDot > m_nodeFOV )
	{
#if 0 
		NDebugOverlay::Line( GetAbsOrigin(), pOther->GetAbsOrigin(), 0, 255, 0, false, 1 );
#endif
		return true;
	}

#if 0 
	NDebugOverlay::Line( GetAbsOrigin(), pOther->GetAbsOrigin(), 255, 0, 0, false, 1 );
#endif

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Locks the node for use by an AI for hints
// Output : Returns true if the node was available for locking, false on failure.
//-----------------------------------------------------------------------------
bool CAI_Hint::Lock( CBaseEntity* pNPC )
{
	if ( m_hHintOwner != pNPC && m_hHintOwner != NULL )
		return false;
	m_hHintOwner = pNPC;
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Unlocks the node, making it available for hint use by other AIs.
//			after the given delay time
//-----------------------------------------------------------------------------
void CAI_Hint::Unlock( float delay )
{
	m_hHintOwner	= NULL;
	m_flNextUseTime = gpGlobals->curtime + delay;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true is hint node is open for use
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_Hint::IsLockedBy(  CBaseEntity *pNPC )
{
	return (m_hHintOwner == pNPC);
};

//-----------------------------------------------------------------------------
// Purpose: Returns true is hint node is open for use
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_Hint::IsLocked( void )
{
	if (m_NodeData.iDisabled)
	{
		return true;
	}

	if (gpGlobals->curtime < m_flNextUseTime)
	{
		return true;
	}
	
	if (m_hHintOwner != NULL)
	{
		return true;
	}
	return false;
};

//-----------------------------------------------------------------------------
// Purpose: Return true if pTestHint passes the criteria specified in hintCriteria
//-----------------------------------------------------------------------------
bool CAI_Hint::HintMatchesCriteria( CAI_BaseNPC *pNPC, const CHintCriteria &hintCriteria, const Vector &position, float *flNearestDistance, bool bIgnoreLock, bool bIgnoreHintType )
{
	// Cannot be locked
	if ( !bIgnoreLock && IsLocked() )
	{
		REPORTFAILURE( "Node is locked." );
		return false;
	}

	if ( !bIgnoreHintType && !hintCriteria.MatchesHintType( HintType() ) )
	{
		return false;
	}

	if ( GetMinState() > NPC_STATE_IDLE || GetMaxState() < NPC_STATE_COMBAT )
	{
		if ( pNPC && ( pNPC->GetState() < GetMinState() || pNPC->GetState() > GetMaxState() ) )
		{
			REPORTFAILURE( "NPC not in correct state." );
			return false;
		}
	}

	// See if we're filtering by group name
	if ( hintCriteria.GetGroup() != NULL_STRING )
	{
		AssertIsValidString( GetGroup() );
		AssertIsValidString( hintCriteria.GetGroup() );
		if ( GetGroup() == NULL_STRING || GetGroup() != hintCriteria.GetGroup() )
		{
			Assert(GetGroup() == NULL_STRING || strcmp( STRING(GetGroup()), STRING(hintCriteria.GetGroup())) != 0 );
			REPORTFAILURE( "Doesn't match NPC hint group." );
			return false;
		}
	}

	// If we're watching for include zones, test it
	if ( ( hintCriteria.HasIncludeZones() ) && ( hintCriteria.InIncludedZone( GetAbsOrigin() ) == false ) )
	{
		REPORTFAILURE( "Not inside include zones." );
		return false;
	}

	// If we're watching for exclude zones, test it
	if ( ( hintCriteria.HasExcludeZones() ) && ( hintCriteria.InExcludedZone( GetAbsOrigin() ) ) )
	{
		REPORTFAILURE( "Inside exclude zones." );
		return false;
	}

	// See if the class handles this hint type
	if ( ( pNPC != NULL ) && ( pNPC->FValidateHintType( this ) == false ) )
	{
		REPORTFAILURE( "NPC doesn't know how to handle that type." );
		return false;
	}

	if ( hintCriteria.HasFlag(bits_HINT_NPC_IN_NODE_FOV) )
	{
		if ( pNPC == NULL )
		{
			AssertMsg(0,"Hint node attempted to verify NPC in node FOV without NPC!\n");
		}
		else
		{
			if( !IsInNodeFOV(pNPC) )
			{
				REPORTFAILURE( "NPC Not in hint's FOV" );
				return false;
			}
		}
	}

	if ( hintCriteria.HasFlag( bits_HINT_NODE_IN_AIMCONE ) )
	{
		if ( pNPC == NULL )
		{
			AssertMsg( 0, "Hint node attempted to find node in aimcone without specifying NPC!\n" );
		}
		else
		{
			if( !pNPC->FInAimCone( GetAbsOrigin() ) )
			{
				REPORTFAILURE( "Hint isn't in NPC's aimcone" );
				return false;
			}
		}
	}

	if ( hintCriteria.HasFlag( bits_HINT_NODE_IN_VIEWCONE ) )
	{
		if ( pNPC == NULL )
		{
			AssertMsg( 0, "Hint node attempted to find node in viewcone without specifying NPC!\n" );
		}
		else
		{
			if( !pNPC->FInViewCone( this ) )
			{
				REPORTFAILURE( "Hint isn't in NPC's viewcone" );
				return false;
			}
		}
	}

	if ( hintCriteria.HasFlag( bits_HINT_NOT_CLOSE_TO_ENEMY ) )
	{
		if ( pNPC == NULL )
		{
			AssertMsg( 0, "Hint node attempted to find node not close to enemy without specifying NPC!\n" );
		}
		else
		{
			if( pNPC->GetEnemy() )
			{
				float flDistHintToEnemySqr = GetAbsOrigin().DistToSqr( pNPC->GetEnemy()->GetAbsOrigin() ) ;

				if( flDistHintToEnemySqr < Square( 30.0f * 12.0f ) )
				{
					REPORTFAILURE( "Hint takes NPC close to Enemy" );
					return false;
				}
			}
		}
	}

	{
		AI_PROFILE_SCOPE( HINT_FVisible );
		// See if we're requesting a visible node
		if ( hintCriteria.HasFlag( bits_HINT_NODE_VISIBLE ) )
		{
			if ( pNPC == NULL )
			{
				//NOTENOTE: If you're hitting this, you've asked for a visible node without specifing an NPC!
				AssertMsg( 0, "Hint node attempted to find visible node without specifying NPC!\n" );
			}
			else
			{
				if( m_NodeData.nNodeID == NO_NODE )
				{
					// This is just an info_hint, not a node.
					if( !pNPC->FVisible( this ) )
					{
						REPORTFAILURE( "Hint isn't visible to NPC." );
						return false;
					}
				}
				else
				{
					// This hint associated with a node.
					trace_t tr;
					Vector vHintPos;
					GetPosition(pNPC,&vHintPos);
					AI_TraceLine ( pNPC->EyePosition(), vHintPos + pNPC->GetViewOffset(), MASK_NPCSOLID_BRUSHONLY, pNPC, COLLISION_GROUP_NONE, &tr );
					if ( tr.fraction != 1.0f )
					{
						REPORTFAILURE( "Node isn't visible to NPC." );
						return false;
					}
				}
			}
		}
	}

	// Check for clear if requested
	if ( hintCriteria.HasFlag( bits_HINT_NODE_CLEAR ) )
	{
		if ( pNPC == NULL )
		{
			//NOTENOTE: If you're hitting this, you've asked for a clear node without specifing an NPC!
			AssertMsg( 0, "Hint node attempted to find clear node without specifying NPC!\n" );
		}
		else
		{
			trace_t tr;
			// Can my bounding box fit there?
			AI_TraceHull ( GetAbsOrigin(), GetAbsOrigin(), pNPC->WorldAlignMins(), pNPC->WorldAlignMaxs(), 
				MASK_SOLID, pNPC, COLLISION_GROUP_NONE, &tr );

			if ( tr.fraction != 1.0 )
			{
				REPORTFAILURE( "Node isn't clear." );
				return false;
			}
		}
	}

	// See if this is our next, closest node
	if ( hintCriteria.HasFlag( bits_HINT_NODE_NEAREST ) )
	{
		Assert( flNearestDistance );

		// Calculate our distance
		float distance = (GetAbsOrigin() - position).Length();

		// Must be closer than the current best
		if ( distance > *flNearestDistance )
		{
			REPORTFAILURE( "Not the nearest node." );
			return false;
		}

		// Remember the distance
		*flNearestDistance = distance;
	}

	if ( hintCriteria.HasFlag(bits_HINT_HAS_LOS_TO_PLAYER|bits_HAS_EYEPOSITION_LOS_TO_PLAYER) )
	{
		CBasePlayer *pPlayer = AI_GetSinglePlayer();

		if( pPlayer != NULL )
		{
			Vector vecDest = GetAbsOrigin(); 

			if( hintCriteria.HasFlag(bits_HAS_EYEPOSITION_LOS_TO_PLAYER) )
			{
				vecDest += pNPC->GetDefaultEyeOffset();
			}

			if( !pPlayer->FVisible(vecDest) )
			{
				REPORTFAILURE( "Do not have LOS to player" );
				return false;
			}
		}
	}

	// Must either be visible or not if requested
	if ( hintCriteria.HasFlag( bits_HINT_NODE_NOT_VISIBLE_TO_PLAYER|bits_HINT_NODE_VISIBLE_TO_PLAYER ) )
	{
		bool bWasSeen = false;
		// Test all potential seers
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);
			
			if ( pPlayer )
			{
				// Only spawn if the player's looking away from me
				Vector vLookDir = pPlayer->EyeDirection3D();
				Vector vTargetDir = GetAbsOrigin() - pPlayer->EyePosition();
				VectorNormalize(vTargetDir);

				float fDotPr = DotProduct(vLookDir,vTargetDir);
				if ( fDotPr > 0 )
				{
					trace_t tr;
					UTIL_TraceLine( pPlayer->EyePosition(), GetAbsOrigin(), MASK_SOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, &tr);
					
					if ( tr.fraction == 1.0 )
					{
						if ( hintCriteria.HasFlag( bits_HINT_NODE_NOT_VISIBLE_TO_PLAYER ) )
						{
							REPORTFAILURE( "Node is visible to player." );
							return false;
						}
						bWasSeen = true;
					}
				}
			}
		}

		if ( !bWasSeen && hintCriteria.HasFlag( bits_HINT_NODE_VISIBLE_TO_PLAYER ) )
		{
			REPORTFAILURE( "Node isn't visible to player." );
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CAI_Hint::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf(tempstr,sizeof(tempstr),"%s (%i)", GetHintTypeDescription( HintType() ), HintType());
		EntityText(text_offset,tempstr,0);
		text_offset++;
		Q_snprintf(tempstr,sizeof(tempstr),"delay %f", MAX( 0.0f, m_flNextUseTime - gpGlobals->curtime ) ) ;
		EntityText(text_offset,tempstr,0);
		text_offset++;

		if ( m_NodeData.iDisabled )
		{
			Q_snprintf(tempstr,sizeof(tempstr),"DISABLED" );
			EntityText(text_offset,tempstr,0);
			text_offset++;
		}

	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_Hint::CAI_Hint(void)
{
	m_flNextUseTime	= 0;
	m_nTargetNodeID = NO_NODE;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_Hint::~CAI_Hint(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: Sometimes FValidateHint, etc. will want to examine the underlying node to 
//  see if it's truly suitable ( e.g., in the same air/ground network of nodes? )
// Output : C_AINode *
//-----------------------------------------------------------------------------
CAI_Node *CAI_Hint::GetNode( void )
{
	if ( m_NodeData.nNodeID != NO_NODE )
	{
		return g_pBigAINet->GetNode( m_NodeData.nNodeID, false );
	}
	return NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_Hint::DisableForSeconds( float flSeconds )
{
	Unlock( flSeconds );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_Hint::EnableThink()
{
	SetDisabled( false );
	SetThink( NULL );
}

void CAI_Hint::FixupTargetNode()
{
	if ( m_NodeData.nTargetWCNodeID != -1 )
		m_nTargetNodeID = g_pAINetworkManager->GetEditOps()->GetNodeIdFromWCId( m_NodeData.nTargetWCNodeID );
	else
		m_nTargetNodeID = NO_NODE;
}

void CAI_Hint::OnRestore()
{
	BaseClass::OnRestore();

	m_NodeData.nNodeID = g_pAINetworkManager->GetEditOps()->GetNodeIdFromWCId( m_NodeData.nWCNodeID );
	FixupTargetNode();

	CAI_Node *pNode = GetNode();
	
	if ( !pNode )
	{
		if ( m_NodeData.nWCNodeID > 0 )
			DevMsg("Warning: AI hint has incorrect or no AI node\n");
	}
	else
	{
		m_NodeData.vecPosition = pNode->GetOrigin();
		Teleport( &m_NodeData.vecPosition, NULL, NULL );
		pNode->SetHint( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_Hint::NPCStartedUsing( CAI_BaseNPC *pNPC )
{
	m_OnNPCStartedUsing.Set( pNPC, pNPC, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_Hint::NPCStoppedUsing( CAI_BaseNPC *pNPC )
{
	m_OnNPCStoppedUsing.Set( pNPC, pNPC, this );
}


CON_COMMAND(ai_dump_hints, "")
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CAI_HintManager::ValidateHints();
	CAI_HintManager::DumpHints();
}


//-----------------------------------------------------------------------------
//
// hints - these MUST coincide with the HINTS listed under Hint_e
//
//-----------------------------------------------------------------------------
struct hinttypedescs_t 
{
	Hint_e iType;
	const char *pszDesc;
};
hinttypedescs_t g_pszHintDescriptions[] =
{
	{	HINT_NONE, "None"	},
	//{	HINT_NOT_USED_WORLD_DOOR, "Obsolete / Unused"	},
	{	HINT_WORLD_WINDOW, "World: Window"	},
	//{	HINT_NOT_USED_WORLD_BUTTON, "Obsolete / Unused"	},
	//{	HINT_NOT_USED_WORLD_MACHINERY, "Obsolete / Unused"	},
	//{	HINT_NOT_USED_WORLD_LEDGE, "Obsolete / Unused"	},
	//{	HINT_NOT_USED_WORLD_LIGHT_SOURCE, "Obsolete / Unused"	},
	//{	HINT_NOT_USED_WORLD_HEAT_SOURCE, "Obsolete / Unused"	},
	//{	HINT_NOT_USED_WORLD_BLINKING_LIGHT, "Obsolete / Unused"	},
	//{	HINT_NOT_USED_WORLD_BRIGHT_COLORS, "Obsolete / Unused"	},
	//{	HINT_NOT_USED_WORLD_HUMAN_BLOOD, "Obsolete / Unused"	},
	//{	HINT_NOT_USED_WORLD_ALIEN_BLOOD, "Obsolete / Unused"	},

	{	HINT_WORLD_WORK_POSITION, "Act Busy"	},
	{	HINT_WORLD_VISUALLY_INTERESTING, "World: Visually Interesting"	},
	{	HINT_WORLD_VISUALLY_INTERESTING_DONT_AIM, "World: Visually Interesting (Don't Aim)"	},
	{	HINT_WORLD_INHIBIT_COMBINE_MINES, "World: Inhibit Combine Mines"	},
	{	HINT_WORLD_VISUALLY_INTERESTING_STEALTH, "World: Visually Interesting (Stealth)"	},

	{	HINT_TACTICAL_COVER_MED, "Tactical: Cover Medium"	},
	{	HINT_TACTICAL_COVER_LOW, "Tactical: Cover Low"	},
	{	HINT_TACTICAL_SPAWN, "Tactical: Spawn"	},
	{	HINT_TACTICAL_PINCH, "Tactical: Pinch"	},
	//{	HINT_NOT_USED_TACTICAL_GUARD, "Obsolete / Unused"	},
	{	HINT_TACTICAL_ENEMY_DISADVANTAGED, "Tactical: Enemy Disadvantage"	},
	//{	HINT_NOT_USED_HEALTH_KIT, "Obsolete / Unused"	},

	//{	HINT_NOT_USED_URBAN_STREETCORNER, "Obsolete / Unused"	},
	//{	HINT_NOT_USED_URBAN_STREETLAMP, "Obsolete / Unused"	},
	//{	HINT_NOT_USED_URBAN_DARK_SPOT, "Obsolete / Unused"	},
	//{	HINT_NOT_USED_URBAN_POSTER, "Obsolete / Unused"	},
	//{	HINT_NOT_USED_URBAN_SHELTER, "Obsolete / Unused"	},

	//{	HINT_NOT_USED_ASSASSIN_SECLUDED, "Obsolete / Unused"	},
	//{	HINT_NOT_USED_ASSASSIN_RAFTERS, "Obsolete / Unused"	},
	//{	HINT_NOT_USED_ASSASSIN_GROUND, "Obsolete / Unused"	},
	//{	HINT_NOT_USED_ASSASSIN_MONKEYBARS, "Obsolete / Unused"	},

	{	HINT_ANTLION_BURROW_POINT, "Antlion: Burrow Point"	},
	{	HINT_ANTLION_THUMPER_FLEE_POINT, "Antlion: Thumper Flee Point"	},

	//{	HINT_HEADCRAB_BURROW_POINT, "Obsolete / Unused"	},

	//{	HINT_NOT_USED_ROLLER_PATROL_POINT, "Obsolete / Unused"	},
	//{	HINT_NOT_USED_ROLLER_CLEANUP_POINT, "Obsolete / Unused"	},
	
	//{	HINT_NOT_USED_PSTORM_ROCK_SPAWN, "Obsolete / Unused"	},

	{	HINT_CROW_FLYTO_POINT, "Crow: Flyto Point"	},

	//{	HINT_BUG_PATROL_POINT, "Obsolete / Unused"	},

	{	HINT_FOLLOW_WAIT_POINT, "Follow: Wait Point"	},
	{	HINT_JUMP_OVERRIDE, "Jump Override"	},
	{	HINT_PLAYER_SQUAD_TRANSITON_POINT, "Squad Transition Point"	},
	{	HINT_NPC_EXIT_POINT, "Act Busy: Exit Point"	},
	{	HINT_STRIDER_NODE, "Strider"	},

	{	HINT_PLAYER_ALLY_MOVE_AWAY_DEST, "Ally MoveAway Point"	},

	{	HINT_HL1_WORLD_MACHINERY, "HL1: World: Machinery"	},
	{	HINT_HL1_WORLD_BLINKING_LIGHT, "HL1: World: Blinking Light"	},
	{	HINT_HL1_WORLD_HUMAN_BLOOD, "HL1: World: Human Blood"	},
	{	HINT_HL1_WORLD_ALIEN_BLOOD, "HL1: World: Alien Blood"	},

	{	HINT_CSTRIKE_HOSTAGE_ESCAPE, "CS Port: Hostage Escape"	},
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *GetHintTypeDescription( Hint_e iHintType )
{
	for ( int i = 0; i < ARRAYSIZE(g_pszHintDescriptions); i++ )
	{
		if ( g_pszHintDescriptions[i].iType == iHintType )
			return g_pszHintDescriptions[i].pszDesc;
	}

	return "Obsolete / Unused";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *GetHintTypeDescription( CAI_Hint *pHint )
{
	return GetHintTypeDescription( pHint->HintType() );
}

//-----------------------------------------------------------------------------
// Purpose: Debug command to drop hints into the world
//-----------------------------------------------------------------------------
void CC_ai_drop_hint( const CCommand &args )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if ( !pPlayer )
		return;

	if ( args.ArgC() < 2 )
	{
		Msg("Invalid hint type specified. Format: ai_drop_hint <hint type>\nValid hint types:\n");

		for ( int i = 0; i < ARRAYSIZE(g_pszHintDescriptions); i++ )
		{
			Msg("%d : %s\n", g_pszHintDescriptions[i].iType, g_pszHintDescriptions[i].pszDesc );
		}
		return;
	}

	HintNodeData nodeData;
	nodeData.strEntityName = MAKE_STRING("ai_drop_hint");
	nodeData.vecPosition = pPlayer->EyePosition();
	nodeData.nHintType = atoi( args[1] );
	nodeData.nNodeID = NO_NODE;
	nodeData.strGroup = NULL_STRING;
	nodeData.iDisabled = false;
	nodeData.iszActivityName = NULL_STRING;
	nodeData.fIgnoreFacing = HIF_DEFAULT;
	nodeData.minState = NPC_STATE_IDLE;
	nodeData.maxState = NPC_STATE_COMBAT;
	CAI_Hint *pHint = CAI_HintManager::CreateHint( &nodeData, NULL );
	if ( pHint )
	{
		((CBaseEntity *)pHint)->Activate();
		pHint->KeyValue( "nodeFOV", "360" );
		pHint->m_debugOverlays |= (OVERLAY_TEXT_BIT | OVERLAY_BBOX_BIT); 
	}
}
ConCommand ai_drop_hint( "ai_drop_hint", CC_ai_drop_hint, "Drop an ai_hint at the player's current eye position.", FCVAR_CHEAT );
