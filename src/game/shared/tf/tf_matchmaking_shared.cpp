//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Matchmaking stuff shared between GC and gameserver / client
//
//=============================================================================

#include "cbase.h"
#include "tf_matchmaking_shared.h"
#include "tf_match_description.h"
#include "tf_ladder_data.h"


#ifdef CLIENT_DLL
#include "tf_gc_client.h"
#include "tf_gamerules.h"
#endif

#ifdef GAME_DLL
#include "tf_gc_server.h"
#include "tf_gamerules.h"
#include "tf_party.h"
#endif

const char *s_pszMatchGroups[] =
{
	"MatchGroup_MvM_Practice",
	"MatchGroup_MvM_MannUp",

	"MatchGroup_Ladder_6v6",
	"MatchGroup_Ladder_9v9",
	"MatchGroup_Ladder_12v12",

	"MatchGroup_Casual_6v6",
	"MatchGroup_Casual_9v9",
	"MatchGroup_Casual_12v12",

	"MatchGroup_Competitive_Event",
};

COMPILE_TIME_ASSERT( ARRAYSIZE( s_pszMatchGroups ) == ETFMatchGroup_ARRAYSIZE );

const char *GetMatchGroupLocalizationName( ETFMatchGroup eMatchGroup )
{
	if ( eMatchGroup < 0 || (unsigned int)eMatchGroup >= ARRAYSIZE( s_pszMatchGroups ) )
	{
		return "MatchGroup_UNKNOWN";
	}
	return s_pszMatchGroups[ eMatchGroup ];
}

// These are the official names of the leaderboards in the steam API.  Changing/adding these requires publishing the
// same changes on the partner site.
//
// --> Update EMatchGroupLeaderboard if you change this
const char *g_szMatchGroupLeaderboardNames[] =
{
	"tf2_ladder_6v6",
	"tf2_casual_12v12",
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_szMatchGroupLeaderboardNames ) == k_eMatchGroupLeaderboard_Count );

const char *GetMatchGroupLeaderboardName( EMatchGroupLeaderboard eMatchGroupLeaderboard )
{
	if ( eMatchGroupLeaderboard < 0 || eMatchGroupLeaderboard >= ARRAYSIZE( g_szMatchGroupLeaderboardNames ) )
	{
		AssertMsg( false, "Bogus value passed to GetMatchGroupLeaderboardName" );
		return nullptr;
	}
	return g_szMatchGroupLeaderboardNames[ eMatchGroupLeaderboard ];
}

	#define GCConVar ConVar
	#define FCVAR_MATCHSIZE_THING ( FCVAR_REPLICATED )

GCConVar tf_mm_match_size_mvm( "tf_mm_match_size_mvm", "6", FCVAR_MATCHSIZE_THING,
                               "How many players in an MvM matchmade group?" );
GCConVar tf_mm_match_size_ladder_6v6( "tf_mm_match_size_ladder_6v6", "12", FCVAR_MATCHSIZE_THING,
                                      "Number of players required to play a 6v6 ladder game.", true, 1, true, 12 );

GCConVar tf_mm_match_size_ladder_9v9( "tf_mm_match_size_ladder_9v9", "18", FCVAR_MATCHSIZE_THING,
                                      "Number of players required to play a 9v9 ladder game." );
GCConVar tf_mm_match_size_ladder_12v12( "tf_mm_match_size_ladder_12v12", "24", FCVAR_MATCHSIZE_THING,
                                        "Number of players required to play a 12v12 ladder game." );
GCConVar tf_mm_match_size_ladder_12v12_minimum( "tf_mm_match_size_ladder_12v12_minimum", "12", FCVAR_MATCHSIZE_THING,
                                                "Specifies the minimum number of players needed to launch a 12v12 match. Set to -1 to disable." );


//-----------------------------------------------------------------------------
// Purpose: Init internal bitvec with ints from the protobuf message
//-----------------------------------------------------------------------------
CCasualCriteriaHelper::CCasualCriteriaHelper( const CTFCasualMatchCriteria& criteria )
{
	m_mapsBits.Resize( GetItemSchema()->GetMasterMapsList().Count(), true );
	Assert( m_mapsBits.GetNumDWords() >= criteria.selected_maps_bits_size() );

	for( int i=0; i < criteria.selected_maps_bits_size() && i < m_mapsBits.GetNumDWords(); ++i )
	{
		m_mapsBits.SetDWord( i , criteria.selected_maps_bits( i ) );
	}

	// validate all of the bits to make sure the maps are in valid categories
	int nNumBits = m_mapsBits.GetNumBits();
	for( int i=0; i < nNumBits; ++i )
	{
		if ( m_mapsBits.IsBitSet( i ) == false )
			continue;

		if ( !IsMapInValidCategory( i ) )
		{
			const MapDef_t* pMap = GetItemSchema()->GetMasterMapDefByIndex( i );
			if ( pMap )
			{
				DevMsg( "CCasualCriteriaHelper: Map %s is selected, but not in any valid game modes!\n", pMap->pszMapName );
			}
			SetMapSelected( i, false );
		}
	}
}
	
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CCasualCriteriaHelper::IsMapSelected( const MapDef_t* pMapDef ) const
{
	if ( !pMapDef )
		return false;

	return IsMapSelected( pMapDef->m_nDefIndex );
}

//-----------------------------------------------------------------------------
// Purpose: Check if bit is selected
//-----------------------------------------------------------------------------
bool CCasualCriteriaHelper::IsMapSelected( const uint32 nMapDefIndex ) const
{
	return m_mapsBits.IsBitSet( nMapDefIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CCasualCriteriaHelper::IsMapInValidCategory( uint32 nMapDefIndex ) const
{
	Assert( (int)nMapDefIndex < m_mapsBits.GetNumBits() );

	const MapDef_t* pMap = GetItemSchema()->GetMasterMapDefByIndex( nMapDefIndex );
	if ( !pMap || pMap->m_vecAssociatedGameCategories.Count() == 0 )
	{
		return false;
	}

	// Make sure this map is in at least one category that's in a casual matchmaking group
	FOR_EACH_VEC( pMap->m_vecAssociatedGameCategories, j )
	{
		const SchemaGameCategory_t* pCategory = GetItemSchema()->GetGameCategory( pMap->m_vecAssociatedGameCategories[j] );
		const SchemaMMGroup_t* pMMGroup = pCategory->m_pMMGroup;

		// Need to have active maps in a match making group
		if ( !pMMGroup || ( pCategory->m_vecEnabledMaps.Count() == 0 ) || ( !pCategory->PassesRestrictions() ) )
		{
			continue;
		}

		// and the matchmaking group needs to be Special Events, Core or Alternative (not Comp)
		if ( pMMGroup->m_eMMGroup == kMatchmakingType_SpecialEvents || pMMGroup->m_eMMGroup == kMatchmakingType_Core || pMMGroup->m_eMMGroup == kMatchmakingType_Alternative )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Check if this criteria is well formed
//-----------------------------------------------------------------------------
bool CCasualCriteriaHelper::IsValid() const
{
	bool bValidMapSeen = false;

	int nNumBits = m_mapsBits.GetNumBits();
	for( int i=0; i < nNumBits; ++i )
	{
		if ( m_mapsBits.IsBitSet( i ) == false )
			continue;

		if ( IsMapInValidCategory( i ) )
		{
			bValidMapSeen = true;
		}
	}

	return bValidMapSeen;
}

//-----------------------------------------------------------------------------
// Purpose: Turn helper back into protobuf message
//-----------------------------------------------------------------------------
CTFCasualMatchCriteria CCasualCriteriaHelper::GetCasualCriteria() const
{
	CTFCasualMatchCriteria outCriteria;
	for( int i=0; i < m_mapsBits.GetNumDWords(); ++i )
	{
		outCriteria.add_selected_maps_bits( m_mapsBits.GetDWord( i ) );
	}

	return outCriteria;
}

//-----------------------------------------------------------------------------
// Purpose: Intersection of this criteria and another
//-----------------------------------------------------------------------------
void CCasualCriteriaHelper::Intersect( const CTFCasualMatchCriteria& otherCriteria )
{
	CCasualCriteriaHelper otherHelper( otherCriteria );
	m_mapsBits.And( otherHelper.m_mapsBits, &m_mapsBits );
}

//-----------------------------------------------------------------------------
// Purpose: Flip a specific map bit
//-----------------------------------------------------------------------------
bool CCasualCriteriaHelper::SetMapSelected( uint32 nMapDefIndex, bool bSelected )
{
	Assert( (int)nMapDefIndex < m_mapsBits.GetNumBits() );

	if ( bSelected && !IsMapInValidCategory( nMapDefIndex ) )
	{
		const MapDef_t* pMap = GetItemSchema()->GetMasterMapDefByIndex( nMapDefIndex );
		if ( pMap )
		{
			DevMsg( "CCasualCriteriaHelper: Attempting to set map %s as selected, but not in any valid game modes!\n", pMap->pszMapName );
		}
		return false;
	}

	m_mapsBits.Set( (int)nMapDefIndex, bSelected );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Sets all bits to zero
//-----------------------------------------------------------------------------
void CCasualCriteriaHelper::Clear( void )
{
	m_mapsBits.ClearAll();
}

//
// MvM Missions
//

CMvMMissionSet::CMvMMissionSet() { Clear(); }
CMvMMissionSet::CMvMMissionSet( const CMvMMissionSet &x ) { m_bits = x.m_bits; }
CMvMMissionSet::~CMvMMissionSet() {}
void CMvMMissionSet::operator=( const CMvMMissionSet &x ) { m_bits = x.m_bits; }
void CMvMMissionSet::Clear() { m_bits = 0; }
bool CMvMMissionSet::operator==( const CMvMMissionSet &x ) const { return m_bits == x.m_bits; }

void CMvMMissionSet::SetMissionBySchemaIndex( int idxMission, bool flag )
{
	Assert( idxMission >= 0 && idxMission < GetItemSchema()->GetMvmMissions().Count() );
	uint64 mask = ( (uint64)1 << (unsigned)idxMission );
	if ( flag )
		m_bits |= mask;
	else
		m_bits &= ~mask;
}

bool CMvMMissionSet::GetMissionBySchemaIndex( int idxMission ) const
{
	// Bogus index?
	if ( idxMission == k_iMvmMissionIndex_NotInSchema )
		return false;
	if ( idxMission < 0 || idxMission >= GetItemSchema()->GetMvmMissions().Count() )
	{
		Assert( idxMission >= 0 );
		Assert( idxMission < GetItemSchema()->GetMvmMissions().Count() );
		return false;
	}

	// Check the bit
	uint64 mask = ( (uint64)1 << (unsigned)idxMission );
	return ( m_bits & mask ) != 0;
}

void CMvMMissionSet::Intersect( const CMvMMissionSet &x )
{
	m_bits &= x.m_bits;
}

bool CMvMMissionSet::HasIntersection( const CMvMMissionSet &x ) const
{
	return ( m_bits & x.m_bits ) != 0;
}

bool CMvMMissionSet::IsEmpty() const
{
	return ( m_bits == 0 );
}

ETFMatchGroup ETFMatchGroup_FuzzyParse( const char *pArg )
{
	// Numeric?
	int nMatchGroupID = -1;
	if ( sscanf( pArg, "%d", &nMatchGroupID ) == V_strlen( pArg ) )
	{
		if ( !ETFMatchGroup_IsValid( nMatchGroupID ) )
			{ return k_eTFMatchGroup_Invalid; }

		return (ETFMatchGroup)nMatchGroupID;
	}

	// Non-numeric, try as enum value (e.g. "k_eTFMatchGroup_Foo")
	ETFMatchGroup eMatchGroup;
	if ( ETFMatchGroup_Parse( pArg, &eMatchGroup ) )
		{ return eMatchGroup; }

	// Try prepending k_eTFMatchGroup_ so you could do e.g. "Casual_12v12"
	if ( ETFMatchGroup_Parse( CFmtStr( "k_eTFMatchGroup_%s", pArg ).Get(), &eMatchGroup ) )
		{ return eMatchGroup; }

	return k_eTFMatchGroup_Invalid;
}

const char *GetMatchGroupName( ETFMatchGroup eMatchGroup )
{
	switch ( eMatchGroup )
	{
		case k_eTFMatchGroup_Invalid: return "(Invalid)";
		case k_eTFMatchGroup_MvM_Practice: return "MvM Practice";
		case k_eTFMatchGroup_MvM_MannUp: return "MvM MannUp";
		case k_eTFMatchGroup_Ladder_6v6: return "6v6 Ladder Match";
		case k_eTFMatchGroup_Ladder_9v9: return "9v9 Ladder Match";
		case k_eTFMatchGroup_Ladder_12v12: return "12v12 Ladder Match";
		case k_eTFMatchGroup_Casual_6v6: return "6v6 Casual Match";
		case k_eTFMatchGroup_Casual_9v9: return "9v9 Casual Match";
		case k_eTFMatchGroup_Casual_12v12: return "12v12 Casual Match";
		case k_eTFMatchGroup_Event_Placeholder: return "12v12 Competitive Event Match";
	}
	// Update this list if you added more! ^
	COMPILE_TIME_ASSERT( k_eTFMatchGroup_Event_Placeholder == ETFMatchGroup_MAX );

	AssertMsg1( false, "Invalid match group %d", eMatchGroup );
	return "(Invalid match group)";
}

const char *GetServerPoolName( int iServerPool )
{
	switch ( iServerPool )
	{
		case k_nGameServerPool_MvM_Practice_Incomplete_Match: return "MvM Boot Camp Active";
		case k_nGameServerPool_MvM_MannUp_Incomplete_Match: return "MvM MannUp Active";
		case k_nGameServerPool_Casual_6v6_Incomplete_Match: return "Casual 6v6 Active";
		case k_nGameServerPool_Casual_9v9_Incomplete_Match: return "Casual 9v9 Active";
		case k_nGameServerPool_Casual_12v12_Incomplete_Match: return "Casual 12v12 Active";

		case k_nGameServerPool_MvM_Practice_Full: return "MvM Boot Camp Full";
		case k_nGameServerPool_MvM_MannUp_Full: return "MvM MannUp Full";
		case k_nGameServerPool_Casual_6v6_Full: return "Casual 6v6 Full";
		case k_nGameServerPool_Casual_9v9_Full: return "Casual 9v9 Full";
		case k_nGameServerPool_Casual_12v12_Full: return "Casual 12v12 Full";
	}

	AssertMsg1( false, "Invalid server pool %d", iServerPool );
	return "(Invalid pool index)";
}
