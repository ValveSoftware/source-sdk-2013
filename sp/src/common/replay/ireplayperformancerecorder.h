//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IREPLAYPERFORMANCERECORDER_H
#define IREPLAYPERFORMANCERECORDER_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "interface.h"

//----------------------------------------------------------------------------------------

class CReplay;
class Vector;
class QAngle;
class CReplayPerformance;

//----------------------------------------------------------------------------------------

class IReplayPerformanceRecorder : public IBaseInterface
{
public:
	virtual void		BeginPerformanceRecord( CReplay *pReplay ) = 0;
	virtual void		EndPerformanceRecord() = 0;

	virtual void		NotifyPauseState( bool bPaused ) = 0;

	virtual CReplayPerformance	*GetPerformance() = 0;
	virtual bool		IsRecording() const = 0;

	virtual void		SnipAtTime( float flTime ) = 0;
	virtual void		NotifySkipping() = 0;
	virtual void		ClearSkipping() = 0;

	virtual void		AddEvent_Camera_Change_FirstPerson( float flTime, int nEntityIndex ) = 0;
	virtual void		AddEvent_Camera_Change_ThirdPerson( float flTime, int nEntityIndex ) = 0;
	virtual void		AddEvent_Camera_Change_Free( float flTime ) = 0;
	virtual void		AddEvent_Camera_ChangePlayer( float flTime, int nEntIndex ) = 0;
	virtual void		AddEvent_Camera_SetView( float flTime, const Vector& origin, const QAngle &angles, float fov ) = 0;
	virtual void		AddEvent_Slowmo( float flTime, float flScale ) = 0;
};

//----------------------------------------------------------------------------------------

#endif // IREPLAYPERFORMANCERECORDER_H
