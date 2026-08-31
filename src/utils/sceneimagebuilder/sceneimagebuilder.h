//========= -------------------------------------------------------------- ============//
//
// Purpose: 
//
//=====================================================================================//
#ifndef SCENEIMAGEBUILDER_H
#define SCENEIMAGEBUILDER_H

#ifdef _WIN32
#pragma once
#endif // _WIN32

#include "appframework/tier3app.h"
#include "sceneimage.h"
#include "choreoevent.h"
#include "sceneimage.cpp"


#define SCENESRC_DIR	"scenes"
#define SCENEIMAGE_FILE "scenes.image"


class CSceneImageBuilderApp : public CTier3SteamApp, public ISceneImage
{
private:
	typedef CTier3SteamApp BaseClass;

private:
	CSceneImage m_SceneImage;
	bool m_bPause = false;
	bool m_bQuiet = false;
	bool m_bLog = false;

private:
	void ParseCommandline();
	void HitKeyToContinue();
	void PrintUsage();
	void PrintHeader();
	virtual bool SetupSearchPaths();

public:
	virtual bool CreateSceneImageFile(CUtlBuffer& targetBuffer, char const* pchModPath, bool bLittleEndian, bool bQuiet, ISceneCompileStatus* Status) override;
	virtual bool PreInit();
	virtual void PostShutdown();
	virtual bool Create();
	virtual void Destroy();
	virtual int	 Main();
    virtual void SceneBuild();
};


#endif // SCENEIMAGEBUILDER_H