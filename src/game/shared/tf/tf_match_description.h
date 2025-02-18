//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef TF_MATCH_DESCRIPTION_H
#define TF_MATCH_DESCRIPTION_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_matchmaking_shared.h"
#ifdef CLIENT_DLL
#include "tf_progression_description.h"
#include "basemodel_panel.h"
#endif // CLIENT_DLL

#ifdef GAME_DLL
	// Can't foward declare CMatchInfo::PlayerMatchData_t because C++.  Bummer.
	#include "tf_gc_server.h"
#endif



class CSOTFLadderData;
class IProgressionDesc;
class IMatchGroupDescription;

const IMatchGroupDescription* GetMatchGroupDescription( const ETFMatchGroup& eGroup );
void RegisterMatchDesc( const ETFMatchGroup& eGroup, const IMatchGroupDescription* pDesc );

class CMatchDescRegister
{
public:
	CMatchDescRegister( ETFMatchGroup eGroup, const IMatchGroupDescription* pDesc ) { RegisterMatchDesc( eGroup, pDesc ); }
};
#define REGISTER_MATCH_GROUP_TYPE( eGroup, pDesc ) CMatchDescRegister g_##eGroup##ConvFunc( eGroup, pDesc );

enum EMatchType_t
{
	MATCH_TYPE_NONE = 0,
	MATCH_TYPE_MVM,
	MATCH_TYPE_COMPETITIVE,
	MATCH_TYPE_CASUAL
};

class IMatchGroupDescription
{
public:

	IMatchGroupDescription( ETFMatchGroup eMatchGroup );


#ifdef CLIENT_DLL
	virtual bool BGetRoundStartBannerParameters( int& nSkin, int& nBodyGroup ) const = 0;
	virtual bool BGetRoundDoorParameters( int& nSkin, int& nLogoBodyGroup ) const = 0;
	virtual const char *GetMapLoadBackgroundOverride( bool bWideScreen ) const = 0;
	virtual void SetupBadgePanel( CBaseModelPanel *pModelPanel, const LevelInfo_t& level, const CSteamID& steamID, bool bInPlacement ) const;
	bool BPlayerIsInPlacement( CSteamID steamID ) const;
	bool BLocalPlayerIsInPlacement() const;
	int GetNumPlacementMatchesToGo( CSteamID steamID ) const;
	struct PlayListData_t
	{
		const char* m_pszPlayListButtonToken;
		const char* m_pszPlayListDescToken;
		const char* m_pszPlayListBackground;
		const char* m_pszSidePanelTitle;
		const char* m_pszSidePanelDesc;
		const char* m_pszSidePanelImage;
	};
	virtual const PlayListData_t* GetPlayListEntryData() const { return NULL; };
#endif

#ifdef GAME_DLL
	// ! Check return, we might fail to setup
	virtual bool InitServerSettingsForMatch( const CTFGSLobby* pLobby ) const;
	virtual void InitGameRulesSettings() const = 0;
	virtual void InitGameRulesSettingsPostEntity() const = 0;
	virtual void PostMatchClearServerSettings() const = 0;
	virtual bool ShouldRequestLateJoin() const = 0;
	virtual bool BMatchIsSafeToLeaveForPlayer( const CMatchInfo* pMatchInfo, const CMatchInfo::PlayerMatchData_t *pMatchPlayer ) const = 0;
	virtual bool BPlayWinMusic( int nWinningTeam, bool bGameOver ) const = 0;
#endif

	// Accessors for param values
	inline int GetMatchSize() const								{ return m_pmm_match_group_size->GetInt(); }
	inline bool BShouldAutomaticallyRequeueOnMatchEnd() const	{ return m_bAutomaticallyRequeueAfterMatchEnds; }
	inline bool BUsesMapVoteAfterMatchEnds() const				{ return m_bUsesMapVoteOnRoundEnd; }
	inline bool BScramblesTeamsOnRollingMatch() const			{ return m_bScramblesTeamsOnRollingMatch; }
	inline bool BUsesXP() const									{ return m_bUsesXP; }
	inline bool BUsesDashboard() const							{ return m_bUsesDashboardOnRoundEnd; }
	inline bool BUsesStrictMatchmakerScoring() const			{ return m_bStrictMatchmakerScoring; }
	inline bool BRequiresCompleteMatches() const				{ return m_bRequireCompleteMatch; }
	inline const char* GetNameLocToken() const					{ return m_pszModeNameLocToken; }
	inline const char* GetRichPresenceLocToken() const			{ return m_pszRichPresenceLocToken; }
	inline bool BAllowsPartyJoins() const						{ return m_bAllowPartyJoins; }
	inline bool BAllowsPartySpectate() const					{ return m_bAllowPartySpectate; }
	inline bool BAllowsQuestProgress() const					{ return m_bContractProgressAllowed; }
	inline EMMPenaltyPool GetPenaltyPool() const				{ return m_ePenaltyPool; }
	inline float GetRequiredMatchmakingScore() const			{ return m_pmm_required_score->GetFloat(); } // TODO: What if no m_pmm_required_score?
	inline bool BSupportLowPriorityQueue() const				{ return m_bSupportsLowPriorityQueue; }
	inline EMatchMode GetLateJoinMode() const					{ return m_eLateJoinMode; }
	inline EMatchType_t GetMatchType() const					{ return m_eMatchType; }
	inline bool BUsesFixedWeaponSpread() const					{ return m_bFixedWeaponSpread; }
	inline bool BUsesAutoBalance() const						{ return m_bUseAutoBalance; }
	inline bool BUsesAutoReady() const							{ return m_bAutoReady; }
	inline bool BUsesPreRoundDoors() const						{ return m_bShowPreRoundDoors; }
	inline bool BUsesPostRoundDoors() const						{ return m_bShowPostRoundDoors; }
	inline bool BDistributePerformanceMedals() const			{ return m_bDistributePerformanceMedals; }
	inline bool BUsesMatchHUD() const							{ return m_bUseMatchHud; }
	inline bool BAllowDrawingAtMatchHistory() const				{ return m_bAllowDrawingAtMatchSummary; }
	inline bool BUseMatchSummaryStage() const					{ return m_bUseMatchSummaryStage; }
	inline const char* GetMatchEndKickWarning() const			{ return m_pszMatchEndKickWarning; }
	inline bool BAllowTeamChange() const						{ return m_bAllowTeamChange; }
	inline bool BAllowSpectatorModeChange() const				{ return m_bAllowSpecModeChange; }
	inline bool BUsesRandomCrits() const						{ return m_bRandomWeaponCrits; }
	inline bool BForceClientSettings() const					{ return m_bForceClientSettings; }
	inline const char* GetMatchStartSound() const				{ return m_pszMatchStartSound; }
	inline int	GetNumWinsToExitPlacement() const				{ return m_nNumWinsToExitPlacement; }
	inline int	GetNumPlacementMatchesPerDay() const			{ return m_nNumPlacementMatchesPerDay; }
	inline bool BUsesPlacementMatches() const					{ return m_nNumWinsToExitPlacement > 0; }
	inline EMMRating GetCurrentDisplayRating() const			{ return m_eCurrentDisplayRating; }
	inline EMMRating GetLastAckdDisplayRating() const			{ return m_eLastAckdDisplayRating; }
	inline EMMRating GetCurrentDisplayRank() const				{ return m_eCurrentDisplayRank; }
	inline EMMRating GetLastAckdDisplayRank() const				{ return m_eLastAckdDisplayRank; }
	inline bool BUsesStickyRanks() const						{ return m_bUsesStickyRanks; }
	inline bool BUsesStrictAbandons() const						{ return m_bUsesStrictAbandons; }
	inline bool BRequiresCompetitiveAccess() const				{ return m_bRequiresCompetitiveAccess; }
	inline EMatchGroupLeaderboard GetLeaderboard() const		{ return m_eLeaderboard; }
	inline bool BUsesStrictSpectatorRules() const				{ return m_bUsesStrictSpectatorRules; }

	// Meta-permissions that are based on other set flags
	//
	// Only match-vote modes need this ability right now
	inline bool BCanServerRequestNewMatchForLobby() const		{ return BUsesMapVoteAfterMatchEnds(); }
	// Auto-balance and anything that is allowed to roll new match lobbies needs to have this ability (for speculative
	// matches if the GC is unavailable).  It should be possible to add a mode where we do rolling matches, but only
	// when the GC is responding, which would not need the unilateral-team-assignment ability
	inline bool BCanServerChangeMatchPlayerTeams() const		{ return BCanServerRequestNewMatchForLobby() || m_bUseAutoBalance; }


	inline bool BIsTrustedServersOnly() const { return m_bTrustedServersOnly; }

	const ETFMatchGroup m_eMatchGroup;
	const IProgressionDesc* m_pProgressionDesc;

protected:
	bool                    m_bActive                             = false;
	EMatchMode              m_eLateJoinMode                       = eMatchMode_Invalid;
	EMMPenaltyPool          m_ePenaltyPool                        = eMMPenaltyPool_Invalid;
	bool                    m_bUsesSkillRatings                   = false;
	bool                    m_bSupportsLowPriorityQueue           = false;
	const ConVar*           m_pmm_required_score                  = nullptr;
	bool                    m_bUseMatchHud                        = false;
	const char*             m_pszExecFileName                     = nullptr;
	const ConVar*           m_pmm_match_group_size                = nullptr;
	const ConVar*           m_pmm_match_group_size_minimum        = nullptr; // Optional
	bool                    m_bShowPreRoundDoors                  = true;
	bool                    m_bShowPostRoundDoors                 = true;
	const char*             m_pszMatchEndKickWarning              = nullptr;
	const char*             m_pszMatchStartSound                  = nullptr;
	bool                    m_bAutoReady                          = false;
	bool                    m_bUseMatchSummaryStage               = false;
	bool                    m_bDistributePerformanceMedals        = false;
	bool                    m_bUseFirstBlood                      = false;
	bool                    m_bUseReducedBonusTime                = false;
	bool                    m_bUseAutoBalance                     = false;
	bool                    m_bAllowTeamChange                    = false;
	bool                    m_bRandomWeaponCrits                  = true;
	bool                    m_bFixedWeaponSpread                  = false;
	// If we should not allow match to complete without a complete set of players.
	bool                    m_bRequireCompleteMatch               = true;
	bool                    m_bTrustedServersOnly                 = true;
	bool                    m_bForceClientSettings                = false;
	bool                    m_bAllowDrawingAtMatchSummary         = false;
	bool                    m_bAllowSpecModeChange                = false;
	bool                    m_bAutomaticallyRequeueAfterMatchEnds = false;
	bool                    m_bUsesMapVoteOnRoundEnd              = false;
	bool                    m_bScramblesTeamsOnRollingMatch       = false;
	bool                    m_bUsesXP                             = false;
	bool                    m_bUsesDashboardOnRoundEnd            = true;
	bool                    m_bUsesSurveys                        = false;
	// Be strict about finding quality matches, for more-competitive matchgroups that want to prioritize match quality
	// over speed.
	bool                    m_bStrictMatchmakerScoring            = false;
	const char*             m_pszModeNameLocToken                 = nullptr;
	// This has to be one of the TF_RichPresence_ values, because they must be propagated to steam
	const char*             m_pszRichPresenceLocToken             = nullptr;
	bool                    m_bAllowPartyJoins                    = false;
	bool                    m_bAllowPartySpectate                 = false;
	bool                    m_bContractProgressAllowed            = false;
	EMatchType_t            m_eMatchType                          = MATCH_TYPE_NONE;
	int                     m_nNumWinsToExitPlacement             = 0;
	int                     m_nNumPlacementMatchesPerDay          = 0;
	EMMRating               m_eCurrentDisplayRating               = k_nMMRating_Invalid;
	EMMRating               m_eLastAckdDisplayRating              = k_nMMRating_Invalid;
	EMMRating               m_eCurrentDisplayRank                 = k_nMMRating_Invalid;
	EMMRating               m_eLastAckdDisplayRank                = k_nMMRating_Invalid;
	bool                    m_bUsesStickyRanks                    = false;
	// Whether or not abandoning a match will incur a penalty.  Also used by the client to choose when to show certain
	// warnings about abandons.
	bool                    m_bUsesStrictAbandons                 = false;
	bool                    m_bRequiresCompetitiveAccess          = false;
	// If set, update this leaderboard on match results.  Must provide BuildLeaderboardValue above.
	EMatchGroupLeaderboard  m_eLeaderboard                        = k_eMatchGroupLeaderboard_Invalid;
	bool                    m_bUsesStrictSpectatorRules           = false;
};

#endif //TF_MATCH_DESCRIPTION_H
