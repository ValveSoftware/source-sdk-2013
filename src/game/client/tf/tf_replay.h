//========= Copyright Valve Corporation, All rights reserved. ============//
//=======================================================================================//

#if defined( REPLAY_ENABLED )

#ifndef TF_REPLAY_H
#define TF_REPLAY_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "replay/genericclassbased_replay.h"

//----------------------------------------------------------------------------------------

class CTFReplay : public CGenericClassBasedReplay
{
	typedef CGenericClassBasedReplay BaseClass;
public:
	CTFReplay();
	~CTFReplay();

	virtual void OnBeginRecording();
	virtual void OnEndRecording();
	virtual void OnComplete();
	virtual void FireGameEvent( IGameEvent *pEvent );

	virtual bool Read( KeyValues *pIn );
	virtual void Write( KeyValues *pOut );

	virtual void DumpGameSpecificData() const;

	virtual const char *GetPlayerClass() const { return g_aPlayerClassNames_NonLocalized[ m_nPlayerClass ]; }
	virtual const char *GetPlayerTeam() const { return m_nPlayerTeam == TF_TEAM_RED ? "red" : "blu"; }
	virtual const char *GetMaterialFriendlyPlayerClass() const;

private:
	virtual void Update();
	void MedicUpdate();
	float GetSentryKillScreenshotDelay();

	virtual bool IsValidClass( int nClass ) const;
	virtual bool IsValidTeam( int iTeam ) const;
	virtual bool GetCurrentStats( RoundStats_t &out );
	virtual const char *GetStatString( int iStat ) const;
	virtual const char *GetPlayerClass( int iClass ) const;

	float			m_flNextMedicUpdateTime;
};

//----------------------------------------------------------------------------------------

inline CTFReplay *ToTFReplay( CReplay *pClientReplay )
{
	return static_cast< CTFReplay * >( pClientReplay );
}

inline const CTFReplay *ToTFReplay( const CReplay *pClientReplay )
{
	return static_cast< const CTFReplay * >( pClientReplay );
}

inline CTFReplay *GetTFReplay( ReplayHandle_t hReplay )
{
	return ToTFReplay( g_pClientReplayContext->GetReplay( hReplay ) );
}

//----------------------------------------------------------------------------------------

#endif	// TF_REPLAY_H

#endif
