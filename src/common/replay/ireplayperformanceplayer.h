//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IREPLAYPERFORMANCEPLAYER_H
#define IREPLAYPERFORMANCEPLAYER_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "interface.h"

//----------------------------------------------------------------------------------------

class CReplay;
class CReplayPerformance;

//----------------------------------------------------------------------------------------

class IReplayPerformancePlayer : public IBaseInterface
{
public:
	virtual void		BeginPerformancePlay( CReplayPerformance *pPerformance ) = 0;
	virtual void		EndPerformancePlay() = 0;

	virtual bool		IsPlaying() const = 0;
};

//----------------------------------------------------------------------------------------

#endif // IREPLAYPERFORMANCEPLAYER_H
