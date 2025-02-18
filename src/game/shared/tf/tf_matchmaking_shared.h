//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Matchmaking stuff shared between GC and gameserver / client
//
//=============================================================================

#ifndef TF_MATCHMAKING_SHARED_H
#define TF_MATCHMAKING_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_gcmessages.pb.h"

class IMatchGroupDescription;

#define NEXT_MAP_VOTE_OPTIONS 3

// Replace this hard-coded value concept in order to support all bracket types (i.e. anything higher than 6v6, etc)
// This increases a number of hard-coded data structures and so on, so beware
#define MATCH_SIZE_MAX 24

// Similarly, some hot path MM structures use this for fixed arrays. It should be the maximum number of players that
// will ever be in a party and no larger.
#define MAX_PARTY_SIZE 6

// How many *bytes* a party chat string can be, including null terminus
#define MAX_PARTY_CHAT_MSG 256

// Range clients are allowed to pass up for custom ping tolerance
// Currently matches CS:GO
#define CUSTOM_PING_TOLERANCE_MIN 25
#define CUSTOM_PING_TOLERANCE_MAX 350

// You must reach this casual level to gain competitive access
const int k_nMinCasualLevelForCompetitive = 3;

// Sticky rank constants
const int k_nLadder_MinGamesBetweenRankChanges = 10;
const int k_nLadder_MinGamesInThresholdToRank = 5;

// XXX(JohnS): Before we can actually use other rating backends for matchmaking or display purposes, there are remaining
//             hard coded assumptions about where the primary rating is, and issues with e.g. Match_Result assuming the
//             backend in use *now* is what the match was created for, etc.  I've sprinkled this around at all the
//             landmines I found while implementing the new rating backend, so... comment this out and fix everything
//             that breaks.
static inline void FixmeMMRatingBackendSwapping() {}

// Backend-agnostic storage type for rating data, so we're not manually passing pairs around to every rating-specific
// interface when we inevitably decide to add a third.
//
// Keep in mind that it is much easier to query well structured data for reports & otherwise, so we should prefer
// e.g. adding another value to packing two 16-bit ints in RatingSecondary for a new backend that needs more 16-bit
// values.  (The calculus may change if we end up with a backend that has eight 8-bit values, however)
//
// Schema objects that embed rating data: RatingHistory, RatingData
// Proto objects that embed rating data:  CSOTFRatingData
struct MMRatingData_t {
	uint32_t unRatingPrimary;
	uint32_t unRatingSecondary;
	uint32_t unRatingTertiary;

	inline bool operator==(const MMRatingData_t &b) const
		{ return this->unRatingPrimary   == b.unRatingPrimary &&
		         this->unRatingSecondary == b.unRatingSecondary &&
		         this->unRatingTertiary  == b.unRatingTertiary; }
};

// Stored value, don't re-order
//
// If you add a new backend, see ITFMMRatingBackend::GetRatingBackend -- you need to at least provide a GetDefault()
//
// GDPR Warning - The GDPR exporter only shows whitelisted rating types to the user.  Any new rating types added
//                here that are not redundant with other data may need to be exported.
enum EMMRating
{
	k_nMMRating_LowestValue = -1,
	k_nMMRating_Invalid								= -1,

	k_nMMRating_First								= 0,
	k_nMMRating_6v6_DRILLO							= 0,
	k_nMMRating_6v6_DRILLO_PlayerAcknowledged		= 1,
	k_nMMRating_6v6_GLICKO							= 2,
	k_nMMRating_12v12_DRILLO						= 3,
	k_nMMRating_Casual_12v12_GLICKO					= 4,
	k_nMMRating_Casual_XP							= 5,
	k_nMMRating_Casual_XP_PlayerAcknowledged		= 6,
	k_nMMRating_6v6_Rank							= 7,
	k_nMMRating_6v6_Rank_PlayerAcknowledged			= 8,
	k_nMMRating_Casual_12v12_Rank					= 9,
	k_nMMRating_Casual_12v12_Rank_PlayerAcknowledged= 10,
	k_nMMRating_6v6_GLICKO_PlayerAcknowledged		= 11,
	k_nMMRating_Comp_12v12_Rank						= 12,
	k_nMMRating_Comp_12v12_Rank_PlayerAcknowledged	= 13,
	k_nMMRating_Comp_12v12_GLICKO					= 14,
	k_nMMRating_Comp_12v12_GLICKO_PlayerAcknowledged= 15,
	k_nMMRating_Last								= 15,
};

// All types must be accounted for here to ensure we're not accidentally overlooking something, rather than implicitly
// by not having an active backend/etc.
inline const char* EMMRating_DisplayedForGDPR( EMMRating eRatingType )
{
	switch ( eRatingType )
	{
		// Types that store at least in part a non-proprietary user-visible rating.  Must have a backend implementing
		// the GDPRExport features
		case k_nMMRating_6v6_GLICKO:                           return "Competitive_Glicko";
		case k_nMMRating_Casual_XP:                            return "Casual_XP";
		case k_nMMRating_Casual_XP_PlayerAcknowledged:         return "Casual_XP_Acknowledged";
		case k_nMMRating_6v6_Rank:                             return "Competitive_Rank";
		case k_nMMRating_6v6_Rank_PlayerAcknowledged:          return "Competitive_Rank_Acknowledged";
		case k_nMMRating_Casual_12v12_Rank:                    return "Casual_Rank";
		case k_nMMRating_Casual_12v12_Rank_PlayerAcknowledged: return "Casual_Rank_Acknowledged";
		case k_nMMRating_6v6_GLICKO_PlayerAcknowledged:        return "Competitive_Glicko_Acknowledged";
		case k_nMMRating_Comp_12v12_Rank:                      return "Competitive_Event_Rank";
		case k_nMMRating_Comp_12v12_Rank_PlayerAcknowledged:   return "Competitive_Event_Rank_Acknowledged";
		case k_nMMRating_Comp_12v12_GLICKO:                    return "Competitive_Event_Glicko";
		case k_nMMRating_Comp_12v12_GLICKO_PlayerAcknowledged: return "Competitive_Event_Glicko_Acknowledged";
		case k_nMMRating_6v6_DRILLO:                           return "Competitive_Drillo"; // Was displayed at one point, if they have it, show it
		case k_nMMRating_6v6_DRILLO_PlayerAcknowledged:        return "Competitive_Drillo_Acknowledged";
		// Internal / proprietary MM math that is not displayed to the user
		case k_nMMRating_12v12_DRILLO:
		case k_nMMRating_Casual_12v12_GLICKO:
			return nullptr; // Not displayed
		default:
			Assert( false ); // You have to explicitly decide
			break;
	}
	return nullptr;
}

// If a sticky rank's primary value is this, then it means it's still in placement
const int k_nPrimaryFieldPlacementValue = 0;

// This must be in the range of an int16 for database serialization
COMPILE_TIME_ASSERT( k_nMMRating_LowestValue >= INT16_MIN );
COMPILE_TIME_ASSERT( k_nMMRating_Last        <= INT16_MAX );

// Stored value, don't re-order
// XXX(JohnS): GDPR Warning - types 0 (MatchIDs) and 2 (Player Acknowledgement) are shown specially by the exporter.  New
//             sources here need to be checked for possible GDPR export
enum EMMRatingSource
{
	k_nMMRatingSource_LowestValue     = -1,
	k_nMMRatingSource_Invalid         = -1,

	k_nMMRatingSource_Match             = 0, // Match result. Source ID is match ID
	k_nMMRatingSource_Admin             = 1, // Admin command/manual adjustment. Source ID is probably 0 or something.
	k_nMMRatingSource_PlayerAcknowledge = 2, // For 'acknowledge' type ratings, the player acknowledged the value
	k_nMMRatingSource_ImportedOldSystem = 3, // For pre-history ratings, this source is used once for 'initial rating from old system'
	k_nMMRatingSource_Last              = 3,
};

// This must be in the range of an int16 for database serialization
COMPILE_TIME_ASSERT( k_nMMRatingSource_LowestValue >= INT16_MIN );
COMPILE_TIME_ASSERT( k_nMMRatingSource_Last        <= INT16_MAX );

// Also update these guys guy if you do the thing
//
// The debug name, like "12v12 Ladder Match"
const char *GetMatchGroupName( ETFMatchGroup eMatchGroup );
// A localization-suitable suffix, such as MatchGroup_Casual_12v12
//   See also the match description's localization methods.  This guy needs to work for even inactive match groups, for
//   e.g. GDPR export.
const char *GetMatchGroupLocalizationName( ETFMatchGroup eMatchGroup );

// Probably a better place for this...
// --> Update g_szLadderLeaderboardNames if you change this
enum EMatchGroupLeaderboard
{
	k_eMatchGroupLeaderboard_Invalid = -1,
	k_eMatchGroupLeaderboard_Ladder6v6 = 0,
	k_eMatchGroupLeaderboard_Casual12v12,
	k_eMatchGroupLeaderboard_Count
};

const char *GetMatchGroupLeaderboardName( EMatchGroupLeaderboard );

// Late join modes
enum EMatchMode
{
	// Uninitialized/unknown
	eMatchMode_Invalid,
	// Not late join / don't use late join
	eMatchMode_MatchMaker_CompleteFromQueue,
	// The add-one-player-at-a-time mode that doesn't work with the new scoring system, but still used for MvM and other
	// old-scoring-system stuff.
	eMatchMode_MatchMaker_LateJoinDropIn,
	// The new late join mode that re-evaulates complete matches with the missing spot(s) filled.
	eMatchMode_MatchMaker_LateJoinMatchBased,
	// A match that is being manually crafted.  Caller must provide additional match party pool, as it cannot use the
	// matchmaker's existing party pools.
	eMatchMode_Manual_NewMatch,
	// Same as above, except based on an existing lobby
	eMatchMode_Manual_ExistingMatchBased,
};

inline bool EMatchMode_IsManual( EMatchMode eMatchMode )
{
	switch ( eMatchMode )
	{
		case eMatchMode_Invalid:
		case eMatchMode_MatchMaker_CompleteFromQueue:
		case eMatchMode_MatchMaker_LateJoinDropIn:
		case eMatchMode_MatchMaker_LateJoinMatchBased:
			return false;
		case eMatchMode_Manual_NewMatch:
		case eMatchMode_Manual_ExistingMatchBased:
			return true;
		default: Assert( !"Unhandled enum value" );
	}
	return false;
}

// Uses existing lobby and "incomplete match" parties that are already joined.  Different from InProgress, which
// includes DropIn (in drop-in mode the match description only has new people that are going to be merged into some
// to-be-determined match.)
inline bool EMatchMode_UsesExistingLobbyAndParties( EMatchMode eMatchMode )
{
	switch ( eMatchMode )
	{
		case eMatchMode_Invalid:
		case eMatchMode_MatchMaker_CompleteFromQueue:
		case eMatchMode_Manual_NewMatch:
		case eMatchMode_MatchMaker_LateJoinDropIn:
			return false;
		case eMatchMode_MatchMaker_LateJoinMatchBased:
		case eMatchMode_Manual_ExistingMatchBased:
			return true;
		default: Assert( !"Unhandled enum value" );
	}
	return false;
}

inline bool EMatchMode_InProgressMatch( EMatchMode eMatchMode )
{
	switch ( eMatchMode )
	{
		case eMatchMode_Invalid:
		case eMatchMode_MatchMaker_CompleteFromQueue:
		case eMatchMode_Manual_NewMatch:
			return false;
		case eMatchMode_MatchMaker_LateJoinDropIn:
		case eMatchMode_MatchMaker_LateJoinMatchBased:
		case eMatchMode_Manual_ExistingMatchBased:
			return true;
		default: Assert( !"Unhandled enum value" );
	}
	return false;
}

// Matchgroup stuff

// Try to parse some input as a matchgroup
ETFMatchGroup ETFMatchGroup_FuzzyParse( const char *pArg );

inline bool IsMvMMatchGroup( ETFMatchGroup eMatchGroup )
{
	return ( eMatchGroup == k_eTFMatchGroup_MvM_Practice ) || ( eMatchGroup == k_eTFMatchGroup_MvM_MannUp );
}

inline bool IsLadderGroup( ETFMatchGroup eMatchGroup )
{
	return ( eMatchGroup >= k_eTFMatchGroup_Ladder_First && eMatchGroup <= k_eTFMatchGroup_Ladder_Last )
		|| ( eMatchGroup >= k_eTFMatchGroup_Casual_First && eMatchGroup <= k_eTFMatchGroup_Casual_Last )
		|| eMatchGroup == k_eTFMatchGroup_Event_Placeholder;
}

inline bool IsCasualGroup( ETFMatchGroup eMatchGroup )
{
	return ( eMatchGroup >= k_eTFMatchGroup_Casual_First ) && ( eMatchGroup <= k_eTFMatchGroup_Casual_Last );
}

inline bool IsMannUpGroup( ETFMatchGroup eMatchGroup )
{
	switch ( eMatchGroup )
	{
		case k_eTFMatchGroup_MvM_Practice:
			return false;
		case k_eTFMatchGroup_MvM_MannUp:
			return true;
		case k_eTFMatchGroup_Ladder_6v6:
		case k_eTFMatchGroup_Ladder_9v9:
		case k_eTFMatchGroup_Ladder_12v12:
		case k_eTFMatchGroup_Invalid:
		case k_eTFMatchGroup_Casual_12v12:
		case k_eTFMatchGroup_Casual_6v6:
		case k_eTFMatchGroup_Casual_9v9:
		case k_eTFMatchGroup_Event_Placeholder:
			return false;
		default:
			Assert( !"IsMannUpGroup called with invalid match group" );
			return false;
	}

	return false;
}

enum EMMServerMode
{
	eMMServerMode_Idle,
	eMMServerMode_Incomplete_Match,
	eMMServerMode_Full,
	eMMServerMode_Count
};

// Separate penalty pools (and rules) for different classes of modes
enum EMMPenaltyPool
{
	eMMPenaltyPool_Invalid = -1,
	eMMPenaltyPool_Casual = 0, // Pool with lenient penalties for most casual/mainstream gamemodes
	eMMPenaltyPool_Ranked,  // Pool with strict and cumulative penalties for ranked gamemodes where abandons tank matches
	eMMPenaltyPool_Count
};

enum
{
	// !! This should match up with GetServerPoolIndex to map these between match groups and server pools
	// eMMServerMode_NotParticipating
	k_nGameServerPool_NotParticipating = -1,

	// eMMServerMode_Incomplete_Match
	k_nGameServerPool_MvM_Practice_Incomplete_Match = 0,
	k_nGameServerPool_MvM_MannUp_Incomplete_Match,
	k_nGameServerPool_Ladder_6v6_Incomplete_Match,
	k_nGameServerPool_Ladder_9v9_Incomplete_Match,
	k_nGameServerPool_Ladder_12v12_Incomplete_Match,
	k_nGameServerPool_Casual_6v6_Incomplete_Match,
	k_nGameServerPool_Casual_9v9_Incomplete_Match,
	k_nGameServerPool_Casual_12v12_Incomplete_Match,
	k_nGameServerPool_Event_Pool_Incomplete_Match,

	// eMMServerMode_Full
	k_nGameServerPool_MvM_Practice_Full,
	k_nGameServerPool_MvM_MannUp_Full,
	k_nGameServerPool_Ladder_6v6_Full,
	k_nGameServerPool_Ladder_9v9_Full,
	k_nGameServerPool_Ladder_12v12_Full,
	k_nGameServerPool_Casual_6v6_Full,
	k_nGameServerPool_Casual_9v9_Full,
	k_nGameServerPool_Casual_12v12_Full,
	k_nGameServerPool_Event_Pool_Full,

	// eMMServerMode_Idle
	k_nGameServerPool_Idle,
	// When adding a new matchgroup, add case handling to GetMatchGroupName(), GetServerPoolName(), YldWebAPIServersByDataCenter()

	k_nGameServerPoolCountTotal,
};

// Also update this guy if you touch pools
const char *GetServerPoolName( int iServerPool );

const int k_nGameServerPool_Incomplete_Match_First = k_nGameServerPool_MvM_Practice_Incomplete_Match;
const int k_nGameServerPool_Incomplete_Match_Last = k_nGameServerPool_Event_Pool_Incomplete_Match;
const int k_nGameServerPool_Full_First = k_nGameServerPool_MvM_Practice_Full;
const int k_nGameServerPool_Full_Last = k_nGameServerPool_Event_Pool_Full;

// Audit these constant and helpers if things are added
COMPILE_TIME_ASSERT( k_nGameServerPoolCountTotal == 19 );
COMPILE_TIME_ASSERT( k_nGameServerPool_Incomplete_Match_First + ETFMatchGroup_MAX == k_nGameServerPool_Incomplete_Match_Last );
COMPILE_TIME_ASSERT( k_nGameServerPool_Full_First + ETFMatchGroup_MAX == k_nGameServerPool_Full_Last );

inline bool IsIncompleteMatchPool( int nGameServerPool )
{
	return nGameServerPool >= k_nGameServerPool_Incomplete_Match_First && nGameServerPool <= k_nGameServerPool_Incomplete_Match_Last;
}

// Stuff is simpler if we can set a max number of challenges in the schema
#define MAX_MVM_CHALLENGES 64

// Store a set of MvM challenges (search criteria, etc)
class CMvMMissionSet
{
public:
	CMvMMissionSet();
	CMvMMissionSet( const CMvMMissionSet &x );
	~CMvMMissionSet();
	void operator=( const CMvMMissionSet &x );
	bool operator==( const CMvMMissionSet &x ) const;

	/// Set to the empty set
	void Clear();

	/// get/set individual bits, based on index of the challenge in the schema
	void SetMissionBySchemaIndex( int iChallengeSchemaIndex, bool flag );
	bool GetMissionBySchemaIndex( int iChallengeSchemaIndex ) const;

	/// Intersect this set with the other set.  Use IsEmpty()
	/// to see if this produced the empty set.
	void Intersect( const CMvMMissionSet &x );

	/// Return true if the two sets have a nonzero intersection.  (Neither object is modified)
	bool HasIntersection( const CMvMMissionSet &x ) const;

	/// Return true if any challenges are selected
	bool IsEmpty() const;
private:

	COMPILE_TIME_ASSERT( MAX_MVM_CHALLENGES <= 64 );

	// Just use a plain old uint64 for now.  We can make this into a proper bitfield class at some point
	uint64 m_bits;
};

// Player Skill Ratings
const int k_nDrilloRating_MinRatingAdjust              = 1;
const int k_nDrilloRating_MaxRatingAdjust              = 100;
const int k_nDrilloRating_Ladder_MaxRatingAdjust       = 500;
const int k_nDrilloRating_Ladder_MaxLossAdjust_LowRank = 100;
const uint32 k_unDrilloRating_MaxDifference            = 25000;
const uint32 k_unDrilloRating_Min                      = 1;
const uint32 k_unDrilloRating_Ladder_Min               = 10000;
const uint32 k_unDrilloRating_Max                      = 50000;
const uint32 k_unDrilloRating_Avg                      = 20000;
const uint32 k_unDrilloRating_Ladder_Start             = 10000;
const uint32 k_unDrilloRating_Ladder_LowSkill          = 19500; // First 6 ranks ceiling
const uint32 k_unDrilloRating_Ladder_HighSkill         = 33001; // Last 6 ranks floor

struct MapDef_t;

//-----------------------------------------------------------------------------
// Purpose: Wrapper class to make dealing with CTFCasualMatchCriteria
//			much easier.
//-----------------------------------------------------------------------------
class CCasualCriteriaHelper
{
public:
	CCasualCriteriaHelper( const CTFCasualMatchCriteria& criteria );

	bool IsMapSelected( const MapDef_t* pMapDef ) const;
	bool IsMapSelected( const uint32 nMapDefIndex ) const;
	bool IsValid() const;
	bool AnySelected() const { return !m_mapsBits.IsAllClear(); }
	CTFCasualMatchCriteria GetCasualCriteria() const;

	void Intersect( const CTFCasualMatchCriteria& otherCriteria );
	bool SetMapSelected( uint32 nMapDefIndex, bool bSelected );

	void Clear( void );

	bool operator==(const CCasualCriteriaHelper &other) const { return m_mapsBits == other.m_mapsBits; }
	bool operator!=(const CCasualCriteriaHelper &other) const { return m_mapsBits != other.m_mapsBits; }

private:
	bool IsMapInValidCategory( uint32 nMapDefIndex ) const;

private:
	CLargeVarBitVec m_mapsBits;
};

// CSOTFLobby flags
#define LOBBY_FLAG_LOWPRIORITY	( 1 << 0 )
#define LOBBY_FLAG_REMATCH		( 1 << 1 )

// CMsgGC_Match_Result match flags
#define MATCH_FLAG_LOWPRIORITY ( 1 << 0 )
#define MATCH_FLAG_REMATCH ( 1 << 1 )

// CMsgGC_Match_Result player flags
#define MATCH_FLAG_PLAYER_LEAVER    ( 1 << 0 )
#define MATCH_FLAG_PLAYER_LATEJOIN  ( 1 << 1 )
// Separate from LEAVER - was marked as an abandon and issued a penalty.  You can be a leaver without being an
// abandoner.
#define MATCH_FLAG_PLAYER_ABANDONER ( 1 << 2 )
#define MATCH_FLAG_PLAYER_PLAYED	( 1 << 3 ) // Did they stay long enough for the game to start?
// These added 4/11/18, inverted to minimize impact to interpreting older flags
#define MATCH_FLAG_PLAYER_NEVER_CONNECTED ( 1 << 4 ) // Player was never actually seen by the match server
#define MATCH_FLAG_PLAYER_NEVER_ACTIVE    ( 1 << 5 ) // Player never actually reached active state, even if they connected (failed to load in)
#define MATCH_FLAG_PLAYER_NEVER_DISCONNECTED ( 1 << 6 ) // Player never disconnected after their initial connect

#endif // #ifndef TF_MATCHMAKING_SHARED_H
