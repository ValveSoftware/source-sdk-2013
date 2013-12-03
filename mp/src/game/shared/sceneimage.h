//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SCENEIMAGE_H
#define SCENEIMAGE_H
#ifdef _WIN32
#pragma once
#endif

class ISceneTokenProcessor;

class ISceneCompileStatus
{
public:
	virtual void UpdateStatus( char const *pchSceneName, bool bQuiet, int nIndex, int nCount ) = 0;
};

class CUtlBuffer;

class ISceneImage
{
public:
	virtual bool CreateSceneImageFile( CUtlBuffer &targetBuffer, char const *pchModPath, bool bLittleEndian, bool bQuiet, ISceneCompileStatus *Status ) = 0;
};

extern ISceneImage *g_pSceneImage;
extern ISceneTokenProcessor *tokenprocessor;

#endif // SCENEIMAGE_H
