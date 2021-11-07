//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HLTVDIRECTOR_H
#define HLTVDIRECTOR_H

#ifdef _WIN32
#pragma once
#endif

#include "GameEventListener.h"
#include <igamesystem.h>
#include <ihltvdirector.h>
#include <ihltv.h>
#include <utlrbtree.h>

#define	HLTV_MIN_DIRECTOR_DELAY		10	// minimum delay if director is enabled
#define	HLTV_MAX_DELAY				120	// maximum delay


#define MAX_NUM_CAMERAS				64	// support up to 64 fixed cameras per level

#define MIN_SHOT_LENGTH				4.0f  // minimum time of a cut (seconds)
#define MAX_SHOT_LENGTH				8.0f  // maximum time of a cut (seconds)
#define DEF_SHOT_LENGTH				6.0f  // average time of a cut (seconds)

class CHLTVGameEvent
{
public:
		int			m_Tick;		// tick of this command
		int			m_Priority; // game event priority
		IGameEvent	*m_Event;	// IGameEvent
};

class CHLTVDirector : public CGameEventListener, public CBaseGameSystemPerFrame, public IHLTVDirector
{
public:
	DECLARE_CLASS_NOBASE( CHLTVDirector );

	virtual char const *Name() { return "CHLTVDirector"; }

	CHLTVDirector();
	virtual ~CHLTVDirector();

	virtual void SetHLTVServer( IHLTVServer *hltv ); // give the director an HLTV interface 
	IHLTVServer* GetHLTVServer( void ); 
	int		GetDirectorTick( void );	// get current broadcast tick from director
	int		GetPVSEntity( void ); // get current view entity (PVS)
	Vector	GetPVSOrigin( void ); // get current PVS origin, if PVS entity is 0
	float	GetDelay( void ); // returns current delay in seconds
	bool	IsActive( void );

	virtual const char** GetModEvents(); // returns list of event names forwarded to HLTV clients

	void	BuildCameraList( void );
		

public: // IGameEventListener Interface
	virtual void	FireGameEvent( IGameEvent * event );
	
public: // CBaseGameSystem overrides

	virtual bool	Init();
	virtual void	Shutdown();
	virtual void	FrameUpdatePostEntityThink();
	virtual void	LevelInitPostEntity();
	virtual const char	*GetFixedCameraEntityName( void ) { return "point_viewcontrol"; }

			bool	SetCameraMan( int iPlayerIndex );
			int		GetCameraMan() { return m_iCameraManIndex; }


protected:

	virtual void	StartNewShot();	
	virtual void	StartRandomShot();
	virtual void	StartDelayMessage();
	virtual void	StartBestFixedCameraShot(bool bForce);
	virtual void	StartBestPlayerCameraShot();
	virtual void	StartFixedCameraShot(int iCamera, int iTarget);
	virtual void	StartChaseCameraShot(int iTarget1, int iTarget2, int distance, int phi, int theta, bool bInEye);
	virtual void	UpdateSettings();
	virtual void	AnalyzePlayers();
	virtual void 	AnalyzeCameras();
	virtual bool	StartCameraManShot();
	virtual void	StartInstantBroadcastShot();
	virtual void	FinishCameraManShot();
	virtual void	BuildActivePlayerList();
	virtual CHLTVGameEvent *FindBestGameEvent();
	virtual void	CreateShotFromEvent( CHLTVGameEvent *ge );

	int		FindFirstEvent( int tick ); // finds first event >= tick
	void	CheckHistory();
	void	RemoveEventsFromHistory(int tick); // removes all commands < tick, or all if tick -1
	
	IHLTVServer		*m_pHLTVServer;	// interface to servers HLTV object
	float			m_fDelay;	// hltv delay in seconds
	int				m_nBroadcastTick; // world time that is currently "on the air"
	int				m_iPVSEntity;	// entity for PVS center
	Vector			m_vPVSOrigin;	// PVS origin if PVS entity is 0
	int				m_iCameraMan;	//  >0 if current view entity is a cameraman
	CBasePlayer		*m_pHLTVClient; // the HLTV fake client
	int				m_nNextShotTick;	// time for the next scene cut
	int				m_iLastPlayer;		// last player in random rotation

	int				m_nNextAnalyzeTick;	
		
	int				m_nNumFixedCameras;	//number of cameras in current map
	CBaseEntity		*m_pFixedCameras[MAX_NUM_CAMERAS]; // fixed cameras (point_viewcontrol)
	
	int				m_nNumActivePlayers;	//number of cameras in current map
	CBasePlayer		*m_pActivePlayers[MAX_PLAYERS]; // fixed cameras (point_viewcontrol)
	int				m_iCameraManIndex;		// entity index of current camera man or 0
	
	CUtlRBTree<CHLTVGameEvent>	m_EventHistory;
};

extern IGameSystem* HLTVDirectorSystem();
extern CHLTVDirector* HLTVDirector();

#endif // HLTVDIRECTOR_H
