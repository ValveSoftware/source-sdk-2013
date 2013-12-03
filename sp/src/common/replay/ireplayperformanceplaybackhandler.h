//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IREPLAYPERFORMANCEPLAYBACKHANDLER_H
#define IREPLAYPERFORMANCEPLAYBACKHANDLER_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "interface.h"
#include "replay/ireplayperformancecontroller.h"

//----------------------------------------------------------------------------------------

class Vector;
class QAngle;

//----------------------------------------------------------------------------------------

class IReplayPerformancePlaybackHandler : public IBaseInterface
{
public:
	virtual void		OnEvent_Camera_Change_FirstPerson( float flTime, int nEntityIndex ) = 0;
	virtual void		OnEvent_Camera_Change_ThirdPerson( float flTime, int nEntityIndex ) = 0;
	virtual void		OnEvent_Camera_Change_Free( float flTime ) = 0;
	virtual void		OnEvent_Camera_ChangePlayer( float flTime, int nEntIndex ) = 0;
	virtual void		OnEvent_Camera_SetView( const SetViewParams_t &params ) = 0;
	virtual void		OnEvent_TimeScale( float flTime, float flScale ) = 0;
};

//----------------------------------------------------------------------------------------

#endif // IREPLAYPERFORMANCEPLAYBACKHANDLER_H
