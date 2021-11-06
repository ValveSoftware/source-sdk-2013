//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Note that this header exists in the Alien Swarm SDK, but not in stock Source SDK 2013.
// Although technically a new Mapbase file, it only serves to move otherwise identical code,
// so most code and repo conventions will pretend it was always there.
//
// --------------------------------------------------------------------
//
// Purpose: Color correction entity with simple radial falloff
//
//=============================================================================//

#ifndef C_COLORCORRECTION_H
#define C_COLORCORRECTION_H
#ifdef _WIN32
#pragma once
#endif

#include "colorcorrectionmgr.h"

//------------------------------------------------------------------------------
// Purpose : Color correction entity with radial falloff
//------------------------------------------------------------------------------
class C_ColorCorrection : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_ColorCorrection, C_BaseEntity );

	DECLARE_CLIENTCLASS();

	C_ColorCorrection();
	virtual ~C_ColorCorrection();

	void OnDataChanged(DataUpdateType_t updateType);
	bool ShouldDraw();

#ifdef MAPBASE // From Alien Swarm SDK
	virtual void Update(C_BasePlayer *pPlayer, float ccScale);
	
	bool IsMaster() const { return m_bMaster; }
	bool IsClientSide() const;
	bool IsExclusive() const { return m_bExclusive; }

	void EnableOnClient( bool bEnable, bool bSkipFade = false );

	Vector GetOrigin();
	float  GetMinFalloff();
	float  GetMaxFalloff();

	void   SetWeight( float fWeight );

protected:
	void StartFade( float flDuration );
	float GetFadeRatio() const;
	bool IsFadeTimeElapsed() const;
#else
	void ClientThink();

private:
#endif
	Vector	m_vecOrigin;

	float	m_minFalloff;
	float	m_maxFalloff;
	float	m_flCurWeight;
	char	m_netLookupFilename[MAX_PATH];

	bool	m_bEnabled;

#ifdef MAPBASE // From Alien Swarm SDK
	float	m_flFadeInDuration;
	float	m_flFadeOutDuration;
	float	m_flMaxWeight;

	bool	m_bMaster;
	bool	m_bClientSide;
	bool	m_bExclusive;

	bool	m_bFadingIn;
	float	m_flFadeStartWeight;
	float	m_flFadeStartTime;
	float	m_flFadeDuration;
#endif

	ClientCCHandle_t m_CCHandle;
};

#endif
