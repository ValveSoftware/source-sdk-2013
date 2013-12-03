//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IREPLAYPERFORMANCECONTROLLER_H
#define IREPLAYPERFORMANCECONTROLLER_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "interface.h"
#include "tier1/strtools.h"

//----------------------------------------------------------------------------------------

class IReplayPerformanceEditor;
class CReplay;
class Vector;
class QAngle;
class CReplayPerformance;

//----------------------------------------------------------------------------------------

// These values are what we use to represent 

struct SetViewParams_t 
{
	SetViewParams_t() { V_memset( this, 0, sizeof( SetViewParams_t ) ); }
	SetViewParams_t( float flTime, Vector *pOrigin, QAngle *pAngles, float flFov, float flAccel,
					 float flSpeed, float flRotFilter )
	:	m_flTime( flTime ),
		m_pOrigin( pOrigin ),
		m_pAngles( pAngles ),
		m_flFov( flFov ),
		m_flAccel( flAccel ),
		m_flSpeed( flSpeed ),
		m_flRotationFilter( flRotFilter )
	{
	}

	float	m_flTime;
	Vector	*m_pOrigin;
	QAngle	*m_pAngles;
	float	m_flFov;

	// Right now only used for updating UI during playback:
	float	m_flAccel;
	float	m_flSpeed;
	float	m_flRotationFilter;
};

//----------------------------------------------------------------------------------------

class IReplayPerformanceController : public IBaseInterface
{
public:
	virtual void		SetEditor( IReplayPerformanceEditor *pEditor ) = 0;

	virtual bool		IsPlaybackDataLeft() = 0;

	virtual void		StartRecording( CReplay *pReplay, bool bSnip ) = 0;
	virtual void		NotifyRewinding() = 0;

	virtual void		Stop() = 0;
	virtual bool		SaveAsync() = 0;
	virtual bool		SaveAsAsync( const wchar *pTitle ) = 0;

	virtual bool		IsSaving() const = 0;

	virtual void		SaveThink() = 0;

	virtual bool		GetLastSaveStatus() const = 0;

	virtual bool		IsRecording() const = 0;
	virtual bool		IsPlaying() const = 0;

	virtual bool		IsDirty() const = 0;
	virtual void		NotifyDirty() = 0;

	virtual CReplayPerformance	*GetPerformance() = 0;
	virtual CReplayPerformance	*GetSavedPerformance() = 0;
	virtual bool		HasSavedPerformance() = 0;

	virtual void		NotifyPauseState( bool bPaused ) = 0;

	virtual void		ClearRewinding() = 0;

	virtual void		OnSignonStateFull() = 0;

	virtual float		GetPlaybackTimeScale() const = 0;

	//
	// Recorder-specific:
	//
	virtual void		AddEvent_Camera_Change_FirstPerson( float flTime, int nEntityIndex ) = 0;
	virtual void		AddEvent_Camera_Change_ThirdPerson( float flTime, int nEntityIndex ) = 0;
	virtual void		AddEvent_Camera_Change_Free( float flTime ) = 0;
	virtual void		AddEvent_Camera_ChangePlayer( float flTime, int nEntIndex ) = 0;
	virtual void		AddEvent_Camera_SetView( const SetViewParams_t &params ) = 0;
	virtual void		AddEvent_TimeScale( float flTime, float flScale ) = 0;
};

//----------------------------------------------------------------------------------------

#endif // IREPLAYPERFORMANCECONTROLLER_H
