//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_STEAMSTATS_H
#define TF_STEAMSTATS_H
#ifdef _WIN32
#pragma once
#endif

#include "steam/steam_api.h"
#include "GameEventListener.h"

class CTFSteamStats : public CAutoGameSystem, public CGameEventListener
{
public:
	CTFSteamStats();
	virtual void PostInit();
	virtual void LevelShutdownPreEntity();
	virtual void UploadStats();

private:
	void FireGameEvent( IGameEvent *event );
	void SetNextForceUploadTime();
	void ReportLiveStats();	// Xbox 360
	float m_flTimeNextForceUpload;
};

extern CTFSteamStats g_TFSteamStats;

#endif //TF_STEAMSTATS_H
