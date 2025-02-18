//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef TF_GC_SERVER_H
#define TF_GC_SERVER_H
#ifdef _WIN32
#pragma once
#endif

#if !defined( _X360 ) && !defined( NO_STEAM )
#include "steam/steam_api.h"
#include "steam/steam_gameserver.h"
#endif

//#include "tf_gc_common.h"
#include "gcsdk/gcclientsdk.h"
#include "playergroup.h"
//#include "dota_gamerules.h"
#include "gc_clientsystem.h"
#include "tf_gcmessages.h"
#include "GameEventListener.h"
#include "rtime.h"
#include "tf_shareddefs.h"
#include "tf_gc_shared.h"
#include "tf_lobby_shared.h"
#include "vote_controller.h"

class CTFGSLobby;
class CTFParty;
class CReliableMessageQueue;

//enum EDOTA_Uploading_Match_Stats
//{
//	EDOTA_MATCH_STATS_IDLE,
//	EDOTA_MATCH_STATS_UPLOADING,
//	EDOTA_MATCH_STATS_UPLOAD_COMPLETE
//};

#ifdef ENABLE_GC_MATCHMAKING

class CMvMVictoryInfo
{
public:
	int m_nLobbyId;
	CUtlString m_sChallengeName;
#ifdef USE_MVM_TOUR
	CUtlString m_sMannUpTourOfDuty;
#endif // USE_MVM_TOUR
	CUtlVector<uint64> m_vPlayerIds;
	CUtlVector<bool> m_vSquadSurplus;
	RTime32 m_tEventTime;

	void Init ( CTFGSLobby *pLobby );
};

class CMatchInfo
{
	friend class CTFGCServerSystem;
public:
	CMatchInfo( const CTFGSLobby *pLobby );
	~CMatchInfo();

	uint64 m_nMatchID;
	uint64 m_nLobbyID;
	ETFMatchGroup m_eMatchGroup;
	uint32 m_uLobbyFlags;
	uint32 m_uInitialAverageMMRating;
	RTime32 m_rtMatchCreated;
	uint32 m_unEventTeamStatus;
	bool m_bFirstPersonActive;
	int m_nBotsAdded;
	// If we created this match speculatively while waiting for the GC to officially grant it a match ID
	bool m_bAwaitingMatchID;
	// If we created a new match for an lobby that had previously completed a match (NewMatchForExistingLobby), this was
	// the originating match.
	uint64 m_nPreviousMatchID;
	// If we have requested a new match for this lobby, and the assignment arrived, but we haven't rotated to it yet.
	uint64 m_nNextMatchID;

	struct DailyStatsRankBucket_t
	{
		uint32 nRank;
		uint32 nRecords;
		uint32 nAvgScore;
		uint32 nStDevScore;
		uint32 nAvgKills;
		uint32 nStDevKills;
		uint32 nAvgDamage;
		uint32 nStDevDamage;
		uint32 nAvgHealing;
		uint32 nStDevHealing;
		uint32 nAvgSupport;
		uint32 nStDevSupport;
	};

	struct PlayerMatchData_t
	{
		friend class CTFGCServerSystem;
		friend class CMatchInfo;
		PlayerMatchData_t( CSteamID steamID, ConstTFLobbyPlayer memberData )
			: steamID( steamID )
			, strOriginalName( memberData.GetName() )
			, uOriginalPartyID( memberData.GetOriginalPartyID() )
			, eGCTeam( memberData.GetTeam() )
			, bDropped( false )
			, bConnected( false )
			, rtJoinedMatch( CRTime::RTime32TimeCur() )
			, bPendingVoteKickRequest( false )
			, bCannotCallVoteKicks( false )
			, bCannotBeTargetedByVoteKicks( false )
			, nVoteKickAttempts( 0 )
			, nDisconnectedSeconds( 0 )
			, nScoreMedal( 0 )
			, nKillsMedal( 0 )
			, nDamageMedal( 0 )
			, nHealingMedal( 0 )
			, nSupportMedal( 0 )
			, bLateJoin( false )
			, nScore( 0 )
			, bPlayed( false )
			, bEverConnected( false )
			, bEverActive( false )
			, bEverDisconnected( false )
			, flNormalizedMMSkillRating( memberData.GetNormalizedRating() )
			, nRank( memberData.GetRank() )
			, unClassesPlayed( 0u )
			, bChatSuspension( memberData.GetChatSuspension() )
			, bVoteKickPending( false )
			, rtLastActiveEvent( CRTime::RTime32TimeCur() )
			, bAlwaysSafeToLeave( false )
			, bDropWasAbandon( false )
			, eDropReason( TFMatchLeaveReason_UNSPECIFIED )
			, nConnectingButNotActiveIndex( 0 )
			, m_mapXPAccumulation( DefLessFunc( CMsgTFXPSource::XPSourceType ) )
			{ Assert( memberData.BMatchPlayer() ); }

		CSteamID steamID;
		// A snapshot of the name the GC saw at lobby-join time
		CUtlString strOriginalName;
		// This is the original as-matched party they joined the server with, they're free to re-arrange their party
		// while playing, but for purposes of premades/etc this is the party that joined.
		uint64 uOriginalPartyID;
		TF_GC_TEAM eGCTeam;

		// If true, this player was dropped from the match and is not part of the active lobby.  This is important for
		// cases where the GC connection is lost and the lobby state is stale.
		bool bDropped;
		bool bConnected;
		// Timestamp player joined the match at. Not guaranteed to be the same instant the match was created, depending
		// on how the GC does things.
		RTime32 rtJoinedMatch;

		uint32 nVoteKickAttempts;
		// A message is in queue on this player's behalf to start a kick
		bool bPendingVoteKickRequest;
		// Not allowed to start further kicks
		bool bCannotCallVoteKicks;
		// Not allowed to be the target of kicks
		bool bCannotBeTargetedByVoteKicks;

		// Number of cumulative seconds the player has been absent, *not* including the initial connect timeout.  Used
		// to determine when to award an abandon.  We may do odd things like "comp" you some seconds on a second, later,
		// disconnect, so this shouldn't be used for stats purposes.
		int nDisconnectedSeconds;

		int nScoreMedal;
		int nKillsMedal;
		int nDamageMedal;
		int nHealingMedal;
		int nSupportMedal;

		bool bLateJoin;
		int nScore;
		bool bPlayed;
		bool bEverConnected;
		bool bEverActive;
		// Did the player ever disconnect after having entered the connected state (not active state!)
		bool bEverDisconnected;
		// This is a single-value skill rating given to each player by the GC
		double flNormalizedMMSkillRating;
		// This is the user's rank.  The rating might be glicko-2 or some other complex rating system that requires
		// more than just the normalized skill rating value to compute your rank, so we pass is separately.
		// This is only used for medals.
		int nRank;
		uint32 unClassesPlayed;

		// Chat suspension on player
		bool bChatSuspension;
		// Whether they have a vote kick
		// determined by the GC pending.
		bool bVoteKickPending;

		const CMsgTFXPSourceBreakdown& GetXPSources() const { return m_XPBreakdown; }

		// Override releasing this player from obligation to this match beyond the normal abandon rules.  Used by MvM mode
		// for marking everyone who completes a wave as allowed to drop without penalty, for instance.
		void MarkAlwaysSafeToLeave() { bAlwaysSafeToLeave = true; }

		bool BDropWasAbandon() { return bDropped && bDropWasAbandon; }
		TFMatchLeaveReason GetDropReason() { return bDropped ? eDropReason : TFMatchLeaveReason_UNSPECIFIED; }
		RTime32 GetLastActiveEventTime( void ) { return rtLastActiveEvent; }

		MM_PlayerConnectionState_t GetConnectionState() const;
		void UpdateClassesPlayed( int nClass );

		struct XPBonusPool_t
		{
			XPBonusPool_t()
				: m_nBonusPoolRemaining( 0 )
				, m_flMultiplier( 1.f )
			{}

			CMsgTFXPSource_XPSourceType m_eType;

			// Only give up to this amount
			int m_nBonusPoolRemaining;
			// Give at this rate
			float m_flMultiplier;
		};

	private:

		void OnConnected( int nEntindex );
		void OnActive();

		// Last time the player changed between active (fully loaded in) and not-active. A player is active if
		// ( bConnected && !nConnectingButNotActiveIndex )
		RTime32 rtLastActiveEvent;
		bool bAlwaysSafeToLeave;
		// If dropped - was it an abandon and what was the reason.
		bool bDropWasAbandon;
		TFMatchLeaveReason eDropReason;

		// Track the janky source-engine state between ClientConnect (when we allow them in) and ClientActive
		int nConnectingButNotActiveIndex;

		// XP accumulation for a player
		CMsgTFXPSourceBreakdown m_XPBreakdown;
		// The breakdown stores ints, but we need float precision or else we're going to round off a
		// significant amount of XP as the match plays on.
		CUtlMap< CMsgTFXPSource::XPSourceType, float > m_mapXPAccumulation;

		CUtlVector< XPBonusPool_t > m_vecXPBonusPools;
	};

	enum RankStatType_t
	{
		RankStat_Invalid = -1,
		RankStat_Score = 0,
		RankStat_Kills,
		RankStat_Damage,
		RankStat_Healing,
		RankStat_Support,
	};

	void SetDailyRankData( DailyStatsRankBucket_t vecRankData );
	bool RequestGCRankData( void );
	bool CalculatePlayerMatchRankData( void );
	bool CalculateMatchSkillRatingAdjustments( int iWinningTeam );
	const CMatchInfo::PlayerMatchData_t* GetMatchDataForPlayer( CSteamID steamID ) const;
	CMatchInfo::PlayerMatchData_t* GetMatchDataForPlayer( CSteamID steamID );

	// For iterating over all players. Index is relative to GetNumTotalMatchPlayers
	CMatchInfo::PlayerMatchData_t* GetMatchDataForPlayer( int nPlayer );

	// This is the total number of players we have match data for -- it may include dropped players not part of the
	// match any longer.
	int GetNumTotalMatchPlayers() const;

	// Number of players still active in the match.
	int GetNumActiveMatchPlayers() const;

	// Number of players still active in the match for a specific team
	int GetNumActiveMatchPlayersForTeam( int nTeam ) const;

	// Total skill rating for a team
	double GetTotalSkillRatingForTeam( int nTeam ) const;

	// Subset of active match players who are currently connected
	int GetNumConnectedMatchPlayers() const;

	// Indicates that this is a stale match that is ending.  In cases such as rolling matches, we never "end" a match,
	// just roll into the next one, since "ended" matches indicate that we've terminated our relationship with the
	// players/GC.
	bool BMatchTerminated() const { return m_bMatchEnded; }

	// Indicates we've sent a result for this match. The match may still be active if we're intending to use it to start
	// a rolling match or other post-game activities.
	bool BSentResult() const { return m_bSentResult; }

	// The canonical size of this type of match.  Can be passed from the GC and override the match description size.
	uint32 GetCanonicalMatchSize() const;

	const char *GetMatchMap() const { return m_strMapName.Length() ? m_strMapName.Get() : NULL; }

	// Rewards the player with XP based on the count scaled by the XP per unit of that type.
	// nCount here is the raw occurances of the action (ie. Points scored, Gold Medals Scored)
	void GiveXPRewardToPlayerForAction( CSteamID steamID, CMsgTFXPSource::XPSourceType eType, int nCount );
	// Directly assign the value
	void GiveXPDirectly( CSteamID steamID, CMsgTFXPSource::XPSourceType eType, int nAmount, bool bCanAwardBonusXP = true );
	// Give an XP bonus that increases 
	void GiveXPBonus( CSteamID steamID,
					  CMsgTFXPSource_XPSourceType eType,
					  float flMultipler,
					  int nBonusPool );

	// Is this player allowed to leave the match without incurring an abandon right now

	// TODO(JohnS): This should not go from true to false due to race conditions (players clicks DC, sees no warning,
	//              races with server deciding its unsafe again), but does for MvM late join.  The GS-initiated late
	//              join rework would make it possible to fix that (once it enters too-low-to-latejoin state it stays
	//              there)
	bool BPlayerSafeToLeaveMatch( CSteamID steamID );

protected:
	int GetRankForStat( RankStatType_t statType, int nRankIndex, uint32 nValue );
	float NormalDistributionCDF( float flValue, float flMu, float flSigma );

private:
	CMatchInfo();
	CMatchInfo( const CMatchInfo &otherinfo );

	// Track a new player participating in our match
	void AddPlayer( CSteamID steamID, ConstTFLobbyPlayer memberData, bool bIsLateJoin, int nEntindex, bool bActive );

	// Mark a player as dropped from the match
	void DropPlayer( CSteamID steamID, TFMatchLeaveReason eReason, bool bWasAbandon );
	void SetEnded() { m_bMatchEnded = true; }

	CUtlVector < DailyStatsRankBucket_t > m_vDailyStatsRankData;
	CUtlVector < PlayerMatchData_t* > m_vMatchRankData;

	CUtlString m_strMapName;
	bool m_bMatchEnded;
	bool m_bSentResult;
	// Canonical size for this match type, override passed from GC.
	uint32 m_nGCMatchSize;

	float m_flBronzePercentile;
	float m_flSilverPercentile;
	float m_flGoldPercentile;
};

class CTFGCServerSystem : public CGCClientSystem, public GCSDK::ISharedObjectListener, public IServerGCLobby, public CGameEventListener
{
	DECLARE_CLASS_GAMEROOT( CTFGCServerSystem, CGCClientSystem );

	// Messages that need to do callbacks
	friend class ReliableMsgNewMatchForLobby;
	friend class ReliableMsgChangeMatchPlayerTeams;
	friend class ReliableMsgVoteKickPlayerRequest;
	friend class ReliableMsgProcessMatchVoteKick;
public:
	CTFGCServerSystem( void );
	~CTFGCServerSystem( void );

	// CAutoGameSystemPerFrame
	virtual bool Init() OVERRIDE;
	virtual void LevelInitPreEntity() OVERRIDE;
	virtual void LevelShutdownPostEntity() OVERRIDE;
	virtual void Shutdown() OVERRIDE;
	virtual void PreClientUpdate() OVERRIDE;

	void SetHibernation( bool bHibernating );
	bool ShouldHideServer();

	// ISharedObjectListener
	virtual void	SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void	PreSOUpdate( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { /* do nothing */ }
	virtual void	SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void	PostSOUpdate( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { /* do nothing */ }
	virtual void	SODestroyed( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void	SOCacheSubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { }
	virtual void	SOCacheUnsubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { }

	void DumpLobby();

	// IServerGCLobby methods
	virtual bool HasLobby() const;
	virtual bool SteamIDAllowedToConnect(const CSteamID &steamId) const;
	virtual void UpdateServerDetails(void);
	virtual bool ShouldHibernate();

	bool MatchAllowsNameChanges();
	bool GetPlayerGCMatchName( const CSteamID &steamId, char *pszOutGCMatchName, size_t nGCMatchNameLen );

	// IGameEventListener2
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

	CTFParty* GetPartyForPlayer( CSteamID steamID ) const;

	CMatchInfo *GetMatch() { return m_pMatchInfo; }
	const CMatchInfo *GetMatch() const { return m_pMatchInfo; }

	// Verbose accessor helpers
	//
	// Get match only if it is live
	CMatchInfo *GetLiveMatch() { return ( m_pMatchInfo && !m_pMatchInfo->m_bMatchEnded ) ? m_pMatchInfo : NULL; }
	const CMatchInfo *GetLiveMatch() const { return const_cast<CTFGCServerSystem*>(this)->GetLiveMatch(); }
	// Get a player only if there is a live match and they are still in the match (not dropped)
	CMatchInfo::PlayerMatchData_t *GetLiveMatchPlayer( CSteamID steamID );
	const CMatchInfo::PlayerMatchData_t *GetLiveMatchPlayer( CSteamID steamID ) const ;

	int GetTeamForLobbyMember( const CSteamID &steamId ) const;
//	bool IsLobbyMemberBroadcaster( const CSteamID &steamId ) const;
//	ELanguage GetBroadcasterLanguage( const CSteamID &steamId ) const;
	float GetFirstConnectTimeForLobbyMember( const CSteamID &steamId ) const;
	int GetVoteKickAttemptsByLobbyMember( const CSteamID &steamID ) const;
	void IncrementVoteKickAttemptsByLobbyMember( const CSteamID &steamID );

	// Called by the vote system when someone asks to perform a votekick.
	//
	// This function can return 'Handled' which means it will go talk to the GC and then submit the request back to the
	// vote system when it has more data.
	enum EVoteKickRequest {
		eVoteKick_Allow,  // -> Clear to perform a votekick
		eVoteKick_Deny,   // -> Tell them no
		eVoteKick_Handled // -> We'll take it from here (e.g. need to go talk to the GC, we'll start a vote or tell them no
		                  //    later when that resolves)
	};
	EVoteKickRequest PlayerRequestVoteKick( const CSteamID &steamID, const CSteamID &steamIDKickTarget, TFVoteKickReason eReason );

	// Submit votekick results from the vote system (which should have gotten permission via PlayerRequestVoteKick)
	void SubmitVoteKickResults( CSteamID steamIDInitiator, CSteamID steamIDTarget, TFVoteKickReason eReason,
	                            const CUtlMap<CSteamID, int> &mapVotesBySteamID, bool bDefaultPass );
	// If true, results of a vote kick were submitted and we're working on it.
	bool BVoteKickPending( CSteamID steamIDTarget ) const;
	// Was this player in the match, and then votekicked?  This is true even if they left and then were subsequently
	// upgraded to kicked.
	bool BPlayerWasVoteKicked( CSteamID steamID ) const;
//
//	EDOTA_Uploading_Match_Stats UploadingMatchStats() { return m_nUploadingMatchStats; }
//	void OnStatsSubmitted( uint32 unMatchID, int32 nReplaySalt );
//	uint32 GetLastMatchID() { return m_unLastMatchID; }
//	int32 GetLastReplaySalt() { return m_nLastReplaySalt; }

	void ClientActive( CSteamID steamIDClient );
	void ClientConnected( CSteamID steamIDPlayer, edict_t *pEntity );
	void ClientDisconnected( CSteamID steamIDClient );

	inline bool IsMMServerModeActive() const { return m_bMMServerMode; }

	void MMServerModeChanged();

//	void SetRelayedGameServerSteamID( const CSteamID &steamID ) { m_relayedGameServerSteamID = steamID; }
//	void SetParentRelayCount( int nParentRelayCount ) { m_nParentRelayCount = nParentRelayCount; }

	float GetTimeLastConnectedToGC( void ) { return m_timeLastConnectedToGC; }

	void EndManagedMatch();

	// Sends match results. Expects the managed match be ended.

	/// MvM game rules processing lets us the players have won
	void SendMvMVictoryResult();

	// Takes ownership of matchResultMsg
	void SendCompetitiveMatchResult( GCSDK::CProtoBufMsg< CMsgGC_Match_Result > *matchResultMsg );

	// If the GC has confirmed we are in the pool for late joins. GetTimeRequestedLateJoin() can be compared with
	// BLateJoinEligible() to reason about delays in the GC making us available for late join.
	bool BLateJoinEligible();

	double GetTimeRequestedLateJoin() { return m_flTimeRequestedLateJoin; }

	// Eject a player from the match, kicking them if they are still present, with given reason.
	bool EjectMatchPlayer( CSteamID steamID, TFMatchLeaveReason eReason );

	void MatchPlayerVoteKicked( CSteamID steamID );

	const MapDef_t* GetNextMapVoteByIndex( int nIndex ) const;

	// Changing Match Player Teams
	//
	// Is game logic allowed to perform team reassignments for this match mode?
	bool CanChangeMatchPlayerTeams();
	// When game logic changes the team of a match player, this should be called immediately. It is invalid to call this
	// if CanChangeMatchPlayerTeams is false.
	void ChangeMatchPlayerTeam( CSteamID steamID, TF_GC_TEAM eTeam );
	// Multi-player version of the above, less GC traffic when multiple reassignments occur at once.
	struct PlayerTeamPair_t { CSteamID steamID; TF_GC_TEAM eTeam; };
	template< typename ANY_ALLOCATOR >
		void ChangeMatchPlayerTeams( const CUtlVector< PlayerTeamPair_t, ANY_ALLOCATOR > &vecNewTeams );

	// Rolling Matches
	//
	// Some match types let us keep a lobby and roll into a new match.  This may not always be allowed depending on GC
	// state.
	//
	// Upon calling RequestNewMatchForLobby, a timer starts, after which the new match is launched in
	// LaunchNewMatchForLobby. During this period our match object is the old match, but the lobby may have been updated
	// to reflect the new match.
	//
	// !! Currently if the GC is out of contact, we will speculatively continue with a new match.  There are rare cases
	//    where the GC will return and decline this request, in which case the resulting match will be unofficial and
	//    not recorded.  It should be trivial to add a bool to prevent such speculative matches should it be necessary.
	bool CanRequestNewMatchForLobby();
	void RequestNewMatchForLobby( const MapDef_t* pNewMap );

	// If the match is in a bogus state and has no useful resolution, terminate it and submit a minidump. This usually
	// just means reboot the match server.
	void AbortInvalidMatchState();

	//
	// Reliable Messages
	//
	const CReliableMessageQueue &ReliableMsgQueue() const { return m_ReliableMsgQueue; }
	CReliableMessageQueue &ReliableMsgQueue() { return m_ReliableMsgQueue; }
	bool BPendingReliableMessages() const { return ReliableMsgQueue().NumPendingMessages() > 0; }
	bool BStalledReliableMessages() const { return ReliableMsgQueue().BStalled(); }

	void ProcessPlayerInventoryRequest( CSteamID steamID, KeyValues* pKVRequest );

protected:

	// CGCClientSystem
	virtual void PreInitGC() OVERRIDE;
	virtual void PostInitGC() OVERRIDE;

private:
	void SendPlayerLeftMatch( CSteamID steamID, TFMatchLeaveReason eReason, bool bAbandoned );

	// Send a kick-lobby message for a stale or unexpected lobby
	void SendRejectLobby();

	// Kick a player that is no longer present in the match and should not be here.
	// Returns true if they were present
	bool KickRemovedMatchPlayer( CSteamID steamIDClient );

	// The lobby object should only be looked at by this class and may be out of date when the GC reboots and similar --
	// most users should use the canonical match state in GetMatch()
	const CTFGSLobby *GetLobby() const;
	CTFGSLobby *GetLobby();

	// Accepts a reservation request from the GC, adding this player to our reserved list, and, for MM mode, to the
	// match.
	void AcceptGCReservation( CSteamID steamID, ConstTFLobbyPlayer memberData, bool bIsLateJoin, int nEntindex, bool bActive );

	// Rolling Matches (private)
	//
	// If we requested a new match for our existing lobby. We don't actually launch the new match for this timer, but
	// the GC may get back to us before (or after!) that period, so this tracks that we're not currently in sync with
	// our lobby object.  See RequestNewMatchForLobby / LaunchNewMatchForLobby and the "Team Assignments" section of the
	// big comment at the top of tf_gc_server.cpp
	bool BPendingNewMatch() const { return m_flWaitingForNewMatchTime != 0.f; }
	// Called at the end of the m_flWaitingForNewMatchTime period to actually create the new match. We could let the
	// caller finish the launch process by changing this timer to a bool and making this public.
	void LaunchNewMatchForLobby();

	// Callbacks from the GC
	void ChangeMatchPlayerTeamsResponse( bool bSuccess );
	bool CanKickPlayer( CTFPlayer *pVoterPlayer, CTFPlayer *pTargetPlayer );
	bool CanKickPlayerMvM( CTFPlayer *pVoterPlayer, CTFPlayer *pTargetPlayer );
	void VoteKickPlayerRequestResponse( CSteamID voterSteamID,
	                                    CSteamID targetSteamID,
	                                    TFVoteKickReason eReason,
	                                    bool bAllowed,         // Can this votekick proceed
	                                    bool bVoterInhibit,    // Should voter be inhibited from further attempts
	                                    bool bTargetInhibit ); // Should target be inhibited from further attempts
	                                                           // against
	void ProcessMatchVoteKickResponse( CSteamID voterSteamID, CSteamID targetSteamID, bool bSucceeded );
	void NewMatchForLobbyResponse( bool bSuccess );
	// Static callbacks that are just forwarding to us
	static void ChangeMatchPlayerTeamsResponseCallback( GCSDK::CProtoBufMsg<CMsgGCChangeMatchPlayerTeamsResponse>& msg );
	static void NewMatchForLobbyResponseCallback( GCSDK::CProtoBufMsg<CMsgGCNewMatchForLobbyResponse>& msg );

	bool m_bSetupSchema;

	RTime32 m_unGameStartTime;
	float m_timeLastSendGameServerInfoAndConnectedPlayers;
	ServerMatchmakingState m_eLastGameServerUpdateState;
	CUtlString m_sLastGameServerUpdateMap;
	CUtlString m_sLastGameServerUpdateTags;
	int m_nLastGameServerUpdateBotCount;
	int m_nLastGameServerUpdateMaxHumans;
	int m_nLastGameServerUpdateSlotsFree;
	uint32 m_nLastGameServerUpdateLobbyMMVersion;

//	EDOTA_Uploading_Match_Stats m_nUploadingMatchStats;
//	uint32 m_unLastMatchID;
//	int32 m_nLastReplaySalt;
	CSteamID m_ourSteamID;
	CSteamID m_relayedGameServerSteamID;
	int m_nParentRelayCount;
	bool m_bMMServerMode;
	double m_flTimeBecameEmptyWithLobby;
	double m_flTimeRequestedLateJoin;
	bool m_bLateJoinEligible;
	int m_iSavedVisibleMaxPlayers;
	bool m_bOverridingVisibleMaxPlayers;
	bool m_bWaitingForNewMatchID;
	float m_flWaitingForNewMatchTime;
	bool m_bCreatingVoteKick = false;

	CMvMVictoryInfo m_mvmVictoryInfo;

	// Check for match players who have been disconnected for long enough to warrant an abandon and do so.
	void MatchPlayerAbandonThink();

	void SetMatchPlayerDropped( CSteamID steamID, TFMatchLeaveReason eReason );

	void UpdateConnectedPlayersAndServerInfo( CMsgGameServerMatchmakingStatus_Event event, bool bForceSendServerInfo );

	CMatchInfo *m_pMatchInfo;
	float m_timeLastConnectedToGC;
//	DOTAGameVersion	m_GameVersion;

	enum EWebapiEquipmentState {
		kWebapiEquipmentState_Init,

		// Waiting for message from client listing equipped items
		kWebapiEquipmentState_WaitingForClientRequest,

		// Request inventory for the remote client
		kWebapiEquipmentState_RequestInventory,
		kWebapiEquipmentState_WaitingForInventory,
		kWebapiEquipmentState_InventoryReceived,

		// Failure that requires client intervention
		// (they need to request a new auth token and send us more info)
		kWebapiEquipmentState_NotifyClientOfFailure,
	};

	struct WebapiEquipmentState_t
	{
		CSteamID m_ownerID;

		EWebapiEquipmentState m_eState = kWebapiEquipmentState_Init;

		// Webapi equipment request
		HTTPRequestHandle m_hEquipmentRequest = INVALID_HTTPREQUEST_HANDLE;
		CCallResult<CTFGCServerSystem::WebapiEquipmentState_t, HTTPRequestCompleted_t> m_EquipmentRequestCompleted;

		// Current request that is in flight
		KeyValues* m_pKVCurrentRequest = nullptr;

		// If we get a request to change loadout while we are waiting to get the items in the user's
		// current inventory, we wait for the existing request to complete before asking for the new items
		//
		// Note that we will only request new items and assume that any existing items are up-to-date
		// and unchanged, since sdk games cannot mutate the user's actual inventory.
		KeyValues* m_pKVNextRequest = nullptr;

		// Backoff
		RTime32 m_rtNextRequest = 0;
		int m_nBackoffSec = 0;
		void Backoff();
		void RequestSucceeded();	// resets backoff timers
		bool IsBackingOff();

		// Steam callback
		void OnWebapiEquipmentReceived( HTTPRequestCompleted_t* pInfo, bool bIOFailure );
	};
	typedef CUtlMap< CSteamID, WebapiEquipmentState_t*, int, CDefLess<CSteamID> > TMapEquipmentRequests;
	TMapEquipmentRequests m_mapEquipmentRequests;
	WebapiEquipmentState_t& FindOrCreateWebapiEquipmentState( CSteamID steamID );

	void WebapiEquipmentThink();
	void WebapiEquipmentThinkRequest( CSteamID steamID, WebapiEquipmentState_t* pState );
	void OnWebapiEquipmentReceived( CSteamID steamID, HTTPRequestCompleted_t* pInfo, bool bIOFailure );

	void SDK_ApplyInventoryInfo( CGCClientSharedObjectCache* pCache, KeyValues* pKVRequest ); // mod expansion point
	void SDK_ApplyLocalLoadout(CGCClientSharedObjectCache* pCache, KeyValues* pKVRequest);


	//
	// Reliable Messages
	//

	CReliableMessageQueue m_ReliableMsgQueue;
};

CTFGCServerSystem *GTFGCClientSystem();

#endif // #ifdef ENABLE_GC_MATCHMAKING

#endif // TF_GC_SERVER_H
