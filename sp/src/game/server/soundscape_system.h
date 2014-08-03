//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SOUNDSCAPE_SYSTEM_H
#define SOUNDSCAPE_SYSTEM_H
#ifdef _WIN32
#pragma once
#endif


#include "stringregistry.h"
#include "tier1/utlstring.h"
class CEnvSoundscape;

struct clusterSoundscapeList_t
{
	unsigned short	soundscapeCount;
	unsigned short	firstSoundscape;
};



class CSoundscapeSystem : public CAutoGameSystemPerFrame
{
public:
	CSoundscapeSystem( char const *name ) : CAutoGameSystemPerFrame( name )
	{
	}

	// game system
	virtual bool Init( void );
	virtual void Shutdown( void );
	virtual void FrameUpdatePostEntityThink( void );
	virtual void LevelInitPreEntity( void );
	virtual void LevelInitPostEntity();

	virtual void AddSoundscapeFile( const char *filename );
	int	GetSoundscapeIndex( const char *pName );
	bool IsValidIndex( int index );

	void FlushSoundscapes( void );
	void AddSoundscapeEntity( CEnvSoundscape *pSoundscape );
	void RemoveSoundscapeEntity( CEnvSoundscape *pSoundscape );
	void PrintDebugInfo( void );

	void AddSoundscapeSounds( KeyValues *pSoundscape, int soundscapeIndex );
	void PrecacheSounds( int soundscapeIndex );

private:
	CStringRegistry							m_soundscapes;
	int										m_soundscapeCount;
	CUtlVector< CEnvSoundscape * >			m_soundscapeEntities;
	CUtlVector<clusterSoundscapeList_t>		m_soundscapesInCluster;
	CUtlVector<unsigned short>				m_soundscapeIndexList;
	int										m_activeIndex;
	CUtlVector< CUtlVector< CUtlString > >	m_soundscapeSounds;
};

extern CSoundscapeSystem g_SoundscapeSystem;


#endif // SOUNDSCAPE_SYSTEM_H
