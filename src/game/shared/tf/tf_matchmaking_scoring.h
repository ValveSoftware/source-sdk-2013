//========= Copyright Valve Corporation, All rights reserved. ============//
#include "tf_quickplay_shared.h"

//
// NOTE: This actually declares global variables and is intended to
// only be included ONCE on the client, and ONCE on the GC.
//

	#define TF2SCORECONVAR(name, defaultval, desc) ConVar name(#name, defaultval, FCVAR_NONE, desc)

TF2SCORECONVAR(tf_matchmaking_numbers_serverfull_headroom, "1", "Scoring will consider the server 'full' when this many slots are available" );
TF2SCORECONVAR(tf_matchmaking_numbers_valve_bonus_hrs_a, "8.00", "Valve server scoring bonus: hours played A" );
TF2SCORECONVAR(tf_matchmaking_numbers_valve_bonus_pts_a, "0.30", "Valve server scoring bonus: bonus points A" );
TF2SCORECONVAR(tf_matchmaking_numbers_valve_bonus_hrs_b, "16.00", "Valve server scoring bonus: hours played B" );
TF2SCORECONVAR(tf_matchmaking_numbers_valve_bonus_pts_b, "0.00", "Valve server scoring bonus: bonus points B" );
TF2SCORECONVAR(tf_matchmaking_numbers_increase_maxplayers_penalty, "0.50", "Max scoring penalty to servers that have increased the max number of players" );
TF2SCORECONVAR(tf_matchmaking_retry_cooldown_seconds, "300", "Time to remember quickplay join attempt, and apply scoring penalty to rejoin the same server" );
TF2SCORECONVAR(tf_matchmaking_retry_max_penalty, "1.0", "Max scoring penalty to rejoin a server previously matched.  (Decays linearly over the cooldown period)" );
TF2SCORECONVAR(tf_matchmaking_noob_map_score_boost, "0.75", "Boost added for quick-plaay scoring purposes if you are a noob and the map is considered noob-friendly" );
TF2SCORECONVAR(tf_matchmaking_noob_hours_played, "8.0", "Number of hours played to determine 'noob' status for quickplay scoring purposes" );

TF2SCORECONVAR(tf_matchmaking_ping_a,        "50.0f", "Quickplay scoring ping time data point A" );
TF2SCORECONVAR(tf_matchmaking_ping_a_score,    "0.9", "Quickplay scoring ping score data point A" );
TF2SCORECONVAR(tf_matchmaking_ping_b,       "150.0f", "Quickplay scoring ping time data point B" );
TF2SCORECONVAR(tf_matchmaking_ping_b_score,    "0.0", "Quickplay scoring ping score data point B" );
TF2SCORECONVAR(tf_matchmaking_ping_c,       "300.0f", "Quickplay scoring ping time data point C" );
TF2SCORECONVAR(tf_matchmaking_ping_c_score,   "-1.0", "Quickplay scoring ping score data point C" );

TF2SCORECONVAR(tf_matchmaking_goodenough_score_start, "8.5", "Good enough score at start of search" );
TF2SCORECONVAR(tf_matchmaking_goodenough_count_start,  "20", "Good enough count at start of search" );
TF2SCORECONVAR(tf_matchmaking_goodenough_score_end,   "7.0", "Good enough score at end of search" );
TF2SCORECONVAR(tf_matchmaking_goodenough_count_end,     "5", "Good enough count at end of search" );

TF2SCORECONVAR( tf_mm_options_bonus, "0.5", "Scoring bonus when approaching tobor rating." );
TF2SCORECONVAR( tf_mm_options_penalty, "-0.25", "Scoring penalty when options score is too far outside an acceptable range." );

TF2SCORECONVAR( tf_matchmaking_server_player_count_score, "1.5", "Maximum score when server is at/near optimal player count." );

//ConVar tf_matchmaking_goodenough_hi_score_start( "tf_matchmaking_goodenough_hi_score_start", "6.0", FCVAR_NONE );
//ConVar tf_matchmaking_goodenough_hi_count_start( "tf_matchmaking_goodenough_hi_count_start", "5", FCVAR_NONE );
//ConVar tf_matchmaking_goodenough_hi_score_end( "tf_matchmaking_goodenough_hi_score_end", "3.0", FCVAR_NONE );
//ConVar tf_matchmaking_goodenough_hi_count_end( "tf_matchmaking_goodenough_hi_count_end", "2", FCVAR_NONE );

ConVar tf_matchmaking_max_search_time( "tf_matchmaking_max_search_time", "45", FCVAR_NONE );

#undef TF2SCORECONVAR

static inline float lerp( float inA, float outA, float inB, float outB, float x )
{
	Assert( inA != inB );
	return outA + ( outB - outA ) * ( x - inA ) / ( inB - inA );
}

struct TF2ScoringNumbers_t
{

	//
	// If we do further experiments, we should make distinct enum values, so we can easily
	// compare the stats on the backend
	//
	enum ExperimentGroup_t
	{
		k_ExperimentGroup_None, // no experiment active

		//
		// Experiment 1
		//
		k_ExperimentGroup_Experiment1_Control = 1,
		k_ExperimentGroup_Experiment1_ValveBias = 2,
		k_ExperimentGroup_Experiment1_ValveBiasInactive = 3,
		k_ExperimentGroup_Experiment1_CommunityBias = 4,
		k_ExperimentGroup_Experiment1_CommunityBiasInactive = 5,
	};

	ExperimentGroup_t m_eExperimentGroup;

	TF2ScoringNumbers_t( CSteamID whosAsking )
	{
		m_eExperimentGroup = k_ExperimentGroup_None;

		//
		// Assign experiment group for experiment 1
		//
		switch ( whosAsking.GetAccountID() & 3 )
		{
			case 0:
				// 80% chance to adjust behaviour
				if ( RandomFloat( 0.0f, 1.0f) < 0.8f )
				{
					m_eExperimentGroup = k_ExperimentGroup_Experiment1_ValveBias;
				}
				else
				{
					m_eExperimentGroup = k_ExperimentGroup_Experiment1_ValveBiasInactive;
				}
				break;

			case 2:
				// 80% chance to adjust behaviour
				if ( RandomFloat( 0.0f, 1.0f) < 0.8f )
				{
					m_eExperimentGroup = k_ExperimentGroup_Experiment1_CommunityBias;
				}
				else
				{
					m_eExperimentGroup = k_ExperimentGroup_Experiment1_CommunityBiasInactive;
				}
				break;

			default:
				m_eExperimentGroup = k_ExperimentGroup_Experiment1_Control;
				break;
		}
	}
};

float QuickplayCalculateServerScore( int numHumans, int numBots, int maxPlayers, int nNumInSearchParty )
{
	Assert( nNumInSearchParty > 0 );

	// Safety check against a degenerate case with invalid max number of players.
	// Protects against some bad math below
	if ( maxPlayers < kTFQuickPlayMinMaxNumberOfPlayers )
	{
		return -100.0f;
	}
	if ( maxPlayers > kTFQuickPlayMaxPlayers )
	{
		// Server should have been filtered, but in case we get here...
		maxPlayers = kTFQuickPlayMaxPlayers;
	}

	float score = 0.0;

	// Check for completely full server
	int newNumHumans = numHumans + nNumInSearchParty;
	int newNumTotalPlayers = newNumHumans + numBots;
	if ( newNumTotalPlayers + tf_matchmaking_numbers_serverfull_headroom.GetInt() > maxPlayers )
	{
		// Server full!  Huge penalty!
		score += -100.0f;
	}
	else
	{
		// Data points for piecewise linear interpolation.
		// First point is implied: empty server is a score of zero.
		//
		// Then we increase up to point A
		int playerCountA = maxPlayers / 3;
		float scoreA = 0.20f;

		// Next data point is when the score peaks at 100%
		int idealPlayerCount = maxPlayers * 5 / 6;

		// Finally, the last data point when the server is full.
		// This choice reflects a pretty steep dropoff.  This server
		// is already in a good state, we should begin to send players
		// to other servers so that they can start to fill up, and reduce
		// the race condition with players trying to join nearly full
		// servers and being too late and getting rejected.
		float scoreFull = scoreA;
		float flMaxScore = Max( tf_matchmaking_server_player_count_score.GetFloat(), 0.1f );

		// Do the piecewise linear interpolation
		if ( newNumHumans <= playerCountA )
		{
			score += lerp( 0, 0.0f, playerCountA, scoreA, float( newNumHumans ) );
		}
		else if ( newNumHumans <= idealPlayerCount )
		{
			// Interpolate from point A up to 100%
			score += lerp( float( playerCountA ), scoreA, idealPlayerCount, flMaxScore, float( newNumHumans ) );
		}
		else
		{
			// Greater than ideal.  Interpolate back down to the score when the server is full
			score += lerp( idealPlayerCount, flMaxScore, maxPlayers, scoreFull, float( newNumHumans ) );
		}
	}

// Don't apply a penalty anymore.  Instead, we just let players express their preference
//	// Give a penalty for servers that increase the max player
//	// number above the ideal.
//	if ( maxPlayers > kTFQuickPlayIdealMaxNumberOfPlayers )
//	{
//		// Max penalty, if they increased it up all the way to
//		// kTFQuickPlayMaxPlayers.  (Above this, we reject them completely)
//		int nExcessPlayers = maxPlayers - kTFQuickPlayIdealMaxNumberOfPlayers;
//		const int kMaxExcessPlayers = kTFQuickPlayMaxPlayers - kTFQuickPlayIdealMaxNumberOfPlayers;
//		float penalty = tf_matchmaking_numbers_increase_maxplayers_penalty.GetFloat() * (float)nExcessPlayers / (float)kMaxExcessPlayers;
//		score -= penalty;
//	}

	//// being tagged as quickplay is roughly the same weight as best ping and best ratio of player numbers
	//if ( bHasQuickplayTag )
	//{
	//	item.score += 2.0f;
	//}

	return score;
}
