//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef ISCENEFILECACHE_H
#define ISCENEFILECACHE_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "appframework/IAppSystem.h"

// the file cache can support persisting some calcs
struct SceneCachedData_t
{
	unsigned int	msecs;
	int				numSounds;
	int				sceneId;
};

class ISceneFileCache : public IAppSystem
{
public:

	// async implemenation
	virtual size_t		GetSceneBufferSize( char const *filename ) = 0;
	virtual bool		GetSceneData( char const *filename, byte *buf, size_t bufsize ) = 0;

	// persisted scene data, returns true if valid, false otherwise
	virtual bool		GetSceneCachedData( char const *pFilename, SceneCachedData_t *pData ) = 0;
	virtual short		GetSceneCachedSound( int iScene, int iSound ) = 0;
	virtual const char	*GetSceneString( short stringId ) = 0;

	// Physically reloads image from disk
	virtual void		Reload() = 0;
};

#define SCENE_FILE_CACHE_INTERFACE_VERSION "SceneFileCache002"

#endif // ISCENEFILECACHE_H
