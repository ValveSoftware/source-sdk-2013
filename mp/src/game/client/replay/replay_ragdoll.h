//========= Copyright Valve Corporation, All rights reserved. ============//
//
//----------------------------------------------------------------------------------------

#ifndef REPLAY_RAGDOLL_H
#define REPLAY_RAGDOLL_H
#ifdef _WIN32
#pragma once
#endif

//--------------------------------------------------------------------------------

class C_BaseAnimating;

//--------------------------------------------------------------------------------

struct RagdollSimulationFrame_t
{
	RagdollSimulationFrame_t() : pPositions(NULL), pAngles(NULL), nTick(-1) {}

	static RagdollSimulationFrame_t* Alloc( int nNumBones );

	int			nTick;
	Vector*		pPositions;
	QAngle*		pAngles;
	Vector		vRootPosition;
	QAngle		angRootAngles;
};

//--------------------------------------------------------------------------------

struct RagdollSimulationData_t
{
	RagdollSimulationData_t( C_BaseAnimating* pEntity = NULL, int nStartTick = 0, int nNumBones = 0 );

	void Record();

	int	m_nEntityIndex;
	int	m_nStartTick;
	int	m_nDuration;
	int m_nNumBones;

	typedef unsigned short Iterator_t;

	CUtlLinkedList< RagdollSimulationFrame_t*, Iterator_t > m_lstFrames;
	C_BaseAnimating* m_pEntity;
};

//--------------------------------------------------------------------------------

// TODO: Refactor this into an interface and hide implementation in cpp file

class CReplayRagdollRecorder
{
private:
	CReplayRagdollRecorder();
	~CReplayRagdollRecorder();

public:
	static CReplayRagdollRecorder& Instance();

	void Init();
	void Shutdown();

	void Think();

	void AddEntry( C_BaseAnimating* pEntity, int nStartTick, int nNumBones );
	void StopRecordingRagdoll( C_BaseAnimating* pEntity );

	void CleanupStartupTicksAndDurations( int nStartTick );
	bool DumpRagdollsToDisk( char const* pszFilename ) const;

	bool IsRecording() const		{ return m_bIsRecording; }

private:
	typedef unsigned short Iterator_t;

	void StopRecordingRagdollAtIndex( Iterator_t nIndex );

	void StopRecordingSleepingRagdolls();
	void Record();

	bool FindEntryInRecordingList( C_BaseAnimating* pEntity, Iterator_t& nOutIndex );

	void PrintDebug();

	CUtlLinkedList< RagdollSimulationData_t*, Iterator_t > m_lstRagdolls;
	CUtlLinkedList< RagdollSimulationData_t*, Iterator_t > m_lstRagdollsToRecord;	// Contains some of the elements from m_lstRagdolls -
																					// the ones which are still recording
	bool m_bIsRecording;
};

//--------------------------------------------------------------------------------

class CReplayRagdollCache
{
private:
	CReplayRagdollCache();

public:
	static CReplayRagdollCache& Instance();

	bool Init( char const* pszFilename );
	void Shutdown();

	void Think();

	bool IsInitialized() const				{ return m_bInit; }

	//
	// Returns false is no frame exists for the given entity at the given tick.
	// Otherwise, returns a 
	//
	bool GetFrame( C_BaseAnimating* pEntity, int nTick, bool* pBoneSimulated, CBoneAccessor* pBoneAccessor ) const;

private:
	RagdollSimulationData_t* FindRagdollEntry( C_BaseAnimating* pEntity, int nTick );
	const RagdollSimulationData_t* FindRagdollEntry( C_BaseAnimating* pEntity, int nTick ) const;

	bool FindFrame( RagdollSimulationFrame_t*& pFrameOut, RagdollSimulationFrame_t*& pNextFrameOut,
		const RagdollSimulationData_t* pRagdollEntry, int nTick );
	bool FindFrame( RagdollSimulationFrame_t*& pFrameOut, RagdollSimulationFrame_t*& pNextFrameOut,
		const RagdollSimulationData_t* pRagdollEntry, int nTick ) const;

	typedef unsigned short Iterator_t;
	bool m_bInit;

	CUtlLinkedList< RagdollSimulationData_t*, Iterator_t > m_lstRagdolls;
};

//--------------------------------------------------------------------------------

bool Replay_CacheRagdolls( const char* pFilename, int nStartTick );

//--------------------------------------------------------------------------------

inline const RagdollSimulationData_t* CReplayRagdollCache::FindRagdollEntry( C_BaseAnimating* pEntity, int nTick ) const
{
	return const_cast< CReplayRagdollCache* >( this )->FindRagdollEntry( pEntity, nTick );
}

inline bool CReplayRagdollCache::FindFrame( RagdollSimulationFrame_t*& pFrameOut, RagdollSimulationFrame_t*& pNextFrameOut,
											const RagdollSimulationData_t* pRagdollEntry, int nTick ) const
{
	return const_cast< CReplayRagdollCache* >( this )->FindFrame( pFrameOut, pNextFrameOut, pRagdollEntry, nTick );
}

//--------------------------------------------------------------------------------

#endif // REPLAY_RAGDOLL_H
