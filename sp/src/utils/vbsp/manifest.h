//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef __MANIFEST_H
#define __MANIFEST_H

#ifdef _WIN32
#pragma once
#endif

#include "boundbox.h"

//
// Each cordon is a named collection of bounding boxes.
//
struct Cordon_t
{
	inline Cordon_t()
	{
		m_bActive = false;
	}

	CUtlString m_szName;
	bool m_bActive;					// True means cull using this cordon when cordoning is enabled.
	CUtlVector<BoundBox> m_Boxes;
};

class CManifestMap
{
public:
	CManifestMap( void );
	char		m_RelativeMapFileName[ MAX_PATH ];
	bool		m_bTopLevelMap;
};

class CManifest
{
public:
	CManifest( void );

	static ChunkFileResult_t LoadManifestMapKeyCallback( const char *szKey, const char *szValue, CManifestMap *pManifestMap );
	static ChunkFileResult_t LoadManifestVMFCallback( CChunkFile *pFile, CManifest *pManifest );
	static ChunkFileResult_t LoadManifestMapsCallback( CChunkFile *pFile, CManifest *pManifest );
	static ChunkFileResult_t LoadCordonBoxCallback( CChunkFile *pFile, Cordon_t *pCordon );
	static ChunkFileResult_t LoadCordonBoxKeyCallback( const char *szKey, const char *szValue, BoundBox *pBox );
	static ChunkFileResult_t LoadCordonKeyCallback( const char *szKey, const char *szValue, Cordon_t *pCordon );
	static ChunkFileResult_t LoadCordonCallback( CChunkFile *pFile, CManifest *pManifest );
	static ChunkFileResult_t LoadCordonsKeyCallback( const char *pszKey, const char *pszValue, CManifest *pManifest );
	static ChunkFileResult_t LoadCordonsCallback( CChunkFile *pFile, CManifest *pManifest );
	static ChunkFileResult_t LoadManifestCordoningPrefsCallback( CChunkFile *pFile, CManifest *pManifest );

	bool			LoadSubMaps( CMapFile *pMapFile, const char *pszFileName );
	epair_t			*CreateEPair( char *pKey, char *pValue );
	bool			LoadVMFManifest( const char *pszFileName );
	const char		*GetInstancePath( ) { return m_InstancePath; }

	void			CordonWorld( );

private:
	bool			LoadVMFManifestUserPrefs( const char *pszFileName );


	CUtlVector< CManifestMap * >	m_Maps;
	char							m_InstancePath[ MAX_PATH ];
	bool							m_bIsCordoning;
	CUtlVector< Cordon_t >			m_Cordons;
	entity_t						*m_CordoningMapEnt;
};

#endif // #ifndef __MANIFEST_H
