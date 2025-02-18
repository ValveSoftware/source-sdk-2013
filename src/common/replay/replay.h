//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef REPLAY_H
#define REPLAY_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "replay/iqueryablereplayitem.h"
#include "replay/replaytime.h"
#include "replay/basereplayserializeable.h"
#include "qlimits.h"
#include "utlstring.h"
#include "utlvector.h"
#include "replay/shared_defs.h"

//----------------------------------------------------------------------------------------

class IReplayDownloadEventHandler;
class CReplayScreenshot;
class CReplayPerformance;

//----------------------------------------------------------------------------------------

class CReplay : public CBaseReplaySerializeable,
				public IQueryableReplayItem
{
	typedef CBaseReplaySerializeable BaseClass;

public:
	enum ReplayStatus_t
	{
		REPLAYSTATUS_INVALID,
		REPLAYSTATUS_ERROR,
		REPLAYSTATUS_DOWNLOADPHASE,		// Multiple sub-states during download state
		REPLAYSTATUS_READYTOCONVERT,	// Download is complete, ready to convert
		REPLAYSTATUS_RENDERING,			// Currently rendering the file
		REPLAYSTATUS_RENDERED,
		REPLAYSTATUS_MAX
	};

	CReplay();
	virtual ~CReplay() {}

	//
	// IReplaySerializeable
	//
	virtual const char	*GetSubKeyTitle() const;
	virtual const char	*GetPath() const;
	virtual void		OnDelete();
	virtual bool		Read( KeyValues *pIn );
	virtual void		Write( KeyValues *pOut );

	virtual void DumpGameSpecificData() const {}

	virtual void Update() {}

	// Hooks to allow replays to setup event listeners, etc.
	virtual void OnBeginRecording() {}
	virtual void OnEndRecording() {}

	// Called when a replay is "completed"
	virtual void OnComplete();

	// Should we allow this replay to be deleted?
	virtual bool ShouldAllowDelete() const { return true; }

	void AutoNameTitleIfEmpty();

	void AddScreenshot( int nWidth, int nHeight, const char *pBaseFilename );

	bool HasReconstructedReplay() const;
	bool IsSignificantBlock( int iBlockReconstruction ) const;	// Does this replay care about the given block?

	CReplayPerformance *AddNewPerformance( bool bGenTitle = true, bool bGenFilename = true );
	void AddPerformance( KeyValues *pIn );
	void AddPerformance( CReplayPerformance *pPerformance );

	// Accessors:
	inline int GetScreenshotCount() const			{ return m_vecScreenshots.Count(); }
	inline const CReplayScreenshot *GetScreenshot( int i ) const { return m_vecScreenshots[ i ]; }
	bool IsDownloaded() const;
	inline int GetPerformanceCount() const			{ return m_vecPerformances.Count(); }
	CReplayPerformance *GetPerformance( int i );
	const CReplayPerformance *GetPerformance( int i ) const;
	inline bool HasPerformance( CReplayPerformance *pPerformance )	{ return m_vecPerformances.Find( pPerformance ) != m_vecPerformances.InvalidIndex(); }
	bool FindPerformance( CReplayPerformance *pPerformance, int &iResult );
	CReplayPerformance *GetPerformanceWithTitle( const wchar_t *pTitle );
	inline const char *GetMapName() const { return m_szMapName; }
	inline int GetSpawnTick() const { return m_nSpawnTick; }
	inline int GetDeathTick() const { return m_nDeathTick; }

	// IQueryableReplayItem implementation:
	virtual const CReplayTime &GetItemDate() const;
	virtual bool IsItemRendered() const;
	virtual CReplay *GetItemReplay();
	virtual ReplayHandle_t	GetItemReplayHandle() const;
	virtual QueryableReplayItemHandle_t	GetItemHandle() const;
	virtual const wchar_t *GetItemTitle() const;
	virtual void SetItemTitle( const wchar_t *pTitle );
	virtual float GetItemLength() const;
	virtual void *GetUserData();
	virtual void SetUserData( void* pUserData );
	virtual bool IsItemAMovie() const;

	// Non-persistent data
	mutable IReplayDownloadEventHandler *m_pDownloadEventHandler;	// Implemented by replay browser - the reason we've got one per replay rather than
																	// one per download group is because the browser needs to receive events per-thumbnail
	bool				m_bSaved;				// True as soon as the replay is saved to disk for the first time
	bool				m_bRequestedByUser;		// Did the user request to save this replay?
	bool				m_bComplete;			// Indicates whether the replay is done recording - this should be false
												// until the player dies, the round ends, or the level changes
	void				*m_pUserData;
	float				m_flNextUpdateTime;
	bool				m_bDirty;

	// Persistent data
	ReplayHandle_t		m_hSession;			// The recording session in which this replay was recorded
	char				m_szMapName[MAX_OSPATH];
	ReplayStatus_t		m_nStatus;
	const char*			m_pFileURL;	// In the form <protocol>://<server address>:<port number>/path/file - points to the string in the download group
	wchar_t				m_wszTitle[MAX_REPLAY_TITLE_LENGTH];
	CUtlString			m_strKilledBy;	// Name of player who killed, if any
	int					m_nDeathTime;
	int					m_nSpawnTick;
	int					m_nDeathTick;
	float				m_flLength;	// The length of the entire replay, including the post-death time, in seconds
	bool				m_bRendered;	// Has the replay been rendered yet?
	int					m_nPlayerSlot;	// Player slot (+1), used to determine which player recorded the demo during playback
	int					m_nPostDeathRecordTime;	// replay_postdeathrecordtime at the time of record
	CUtlVector< CReplayScreenshot * >	m_vecScreenshots;
	CUtlVector< CReplayPerformance * >	m_vecPerformances;
	int					m_iMaxSessionBlockRequired;	// The maximum session block required to reconstruct a viewable .dem file from spawn tick until length
	CReplayTime			m_RecordTime;	// Contains time/date when spawn tick was recorded
	float				m_flStartTime;	// Start time (uses engine's host_time)
	CUtlString			m_strReconstructedFilename;
	bool				m_bSavedDuringThisSession;
};

//----------------------------------------------------------------------------------------

#endif // REPLAY_H
