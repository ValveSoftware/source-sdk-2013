//========= Copyright Valve Corporation, All rights reserved. ============//
//=======================================================================================//

#if defined( REPLAY_ENABLED )

#ifndef GENERICCLASSBASED_REPLAY_H
#define GENERICCLASSBASED_REPLAY_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "replay/replay.h"
#include "replay/iclientreplaycontext.h"
#include "GameEventListener.h"

// For RoundStats_t
#include "replay/gamedefs.h"

//----------------------------------------------------------------------------------------

extern IClientReplayContext *g_pClientReplayContext;

//----------------------------------------------------------------------------------------

class CGenericClassBasedReplay : public CReplay,
								 public CGameEventListener
{
	typedef CReplay BaseClass;
public:
	CGenericClassBasedReplay();
	~CGenericClassBasedReplay();

	virtual void OnBeginRecording();
	virtual void OnEndRecording();
	virtual void OnComplete();
	virtual bool ShouldAllowDelete() const;
	virtual void OnDelete();

	virtual void FireGameEvent( IGameEvent *pEvent );

	virtual bool Read( KeyValues *pIn );
	virtual void Write( KeyValues *pOut );

	virtual void DumpGameSpecificData() const;

	void SetPlayerClass( int nPlayerClass );
	void SetPlayerTeam( int nPlayerTeam );

	void RecordPlayerDeath( const char *pKillerName, int nKillerClass );

	// Add a new kill to the list
	void AddKill( const char *pPlayerName, int nPlayerClass );

	// Get the player class as a string
	virtual const char *GetPlayerClass() const;

	// Get the player team as a string
	virtual const char *GetPlayerTeam() const = 0;

	// Utility to get the material-friendly player class (demoman->demo, heavyweapons->heavy)
	virtual const char *GetMaterialFriendlyPlayerClass() const;

	// Was there a killer?
	inline bool WasKilled() const { return m_szKillerName[0] != 0; }

	// Get killer name
	const char *GetKillerName() const;

	// Get the killer class, if there was a killer
	const char *GetKillerClass() const;

	int			GetDownloadStatus() const;

	// Kill info
	struct KillData_t
	{
		char	m_szPlayerName[MAX_OSPATH];
		int		m_nPlayerClass;
	};

	inline int GetKillCount() const		{ return m_vecKills.Count(); }
	inline const KillData_t *GetKill( int nKillIndex )	{ return m_vecKills[ nKillIndex ]; }

	// A generic info struct used for dominations, assisted dominations, revenges, assisted revenged...
	// Not all data members are necessarily used
	struct GenericStatInfo_t
	{
		GenericStatInfo_t() : m_nVictimFriendId( 0 ), m_nAssisterFriendId( 0 ) {}
		uint32	m_nVictimFriendId;
		uint32	m_nAssisterFriendId;
	};

	inline int GetDominationCount() const	{ return m_vecDominations.Count(); }
	inline const GenericStatInfo_t *GetDomination( int nIndex ) const	{ return m_vecDominations[ nIndex ]; }

	inline int GetAssisterDominationCount() const	{ return m_vecAssisterDominations.Count(); }
	inline const GenericStatInfo_t *GetAssisterDomination( int nIndex ) const	{ return m_vecAssisterDominations[ nIndex ]; }

	inline int GetRevengeCount() const		{ return m_vecRevenges.Count(); }
	inline const GenericStatInfo_t *GetRevenge( int nIndex ) const	{ return m_vecRevenges[ nIndex ]; }

	inline int GetAssisterRevengeCount() const		{ return m_vecAssisterRevenges.Count(); }
	inline const GenericStatInfo_t *GetAssisterRevenge( int nIndex ) const	{ return m_vecAssisterRevenges[ nIndex ]; }

	RoundStats_t const &GetStats() const	{ return m_lifeStats; }

protected:
	int		m_nPlayerClass;
	int		m_nPlayerTeam;
	int		m_nStatUndefined;

	char	m_szKillerName[ MAX_OSPATH ];
	int		m_nKillerClass;

	virtual bool IsValidClass( int nClass ) const = 0;
	virtual bool IsValidTeam( int iTeam ) const = 0;
	virtual bool GetCurrentStats( RoundStats_t &out ) = 0;
	virtual const char *GetStatString( int iStat ) const = 0;
	virtual const char *GetPlayerClass( int iClass ) const = 0;

	virtual void Update();

	// Domination
	void AddDomination( int nVictimID );
	void AddAssisterDomination( int nVictimID, int nAssiterID );

	void AddRevenge( int nVictimID );
	void AddAssisterRevenge( int nVictimID, int nAssiterID );

	float GetKillScreenshotDelay();

	RoundStats_t	m_refStats;		// Reference stats, used to compute current stats
	RoundStats_t	m_lifeStats;	// Stats for this life, based on reference stats (m_refStats)

private:
	void MedicUpdate();

	bool GetFriendIdFromUserId( int nPlayerIndex, uint32 &nFriendIdOut ) const;	// Get a friend ID based on player index.  Returns true on success
	void AddKillStatFromUserIds( CUtlVector< GenericStatInfo_t * > &vec, int nVictimId, int nAssisterId = 0 );
	void AddKillStatFromFriendIds( CUtlVector< GenericStatInfo_t * > &vec, uint32 nVictimFriendId, uint32 nAssisterFriendId = 0 );
	void WriteKillStatVector( CUtlVector< GenericStatInfo_t * > const &vec, const char *pSubKeyName, const char *pElementKeyName,
							  KeyValues *pRootKey, int nNumMembersToWrite ) const;
	void AddKillStats( CUtlVector< GenericStatInfo_t * > &vecKillStats, KeyValues *pIn, const char *pSubKeyName, int iStatIndex );
	void RecordUpdatedStats();

	CUtlVector< KillData_t * >			m_vecKills;
	CUtlVector< GenericStatInfo_t * >	m_vecDominations;
	CUtlVector< GenericStatInfo_t * >	m_vecAssisterDominations;
	CUtlVector< GenericStatInfo_t * >	m_vecRevenges;
	CUtlVector< GenericStatInfo_t * >	m_vecAssisterRevenges;

	// TODO... dominations, achievements, etc.
};

//----------------------------------------------------------------------------------------

inline CGenericClassBasedReplay *ToGenericClassBasedReplay( CReplay *pClientReplay )
{
	return static_cast< CGenericClassBasedReplay * >( pClientReplay );
}

inline const CGenericClassBasedReplay *ToGenericClassBasedReplay( const CReplay *pClientReplay )
{
	return static_cast< const CGenericClassBasedReplay * >( pClientReplay );
}

inline CGenericClassBasedReplay *GetGenericClassBasedReplay( ReplayHandle_t hReplay )
{
	return ToGenericClassBasedReplay( g_pClientReplayContext->GetReplay( hReplay ) );
}

//----------------------------------------------------------------------------------------

#endif	// GENERICCLASSBASED_REPLAY_H

#endif
