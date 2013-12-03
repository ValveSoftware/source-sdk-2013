//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IREPLAYFACTORY_H
#define IREPLAYFACTORY_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "interface.h"

//----------------------------------------------------------------------------------------

class CReplay;

//----------------------------------------------------------------------------------------

abstract_class IReplayFactory : public IBaseInterface
{
public:
	virtual CReplay		*Create() = 0;
};

#define INTERFACE_VERSION_REPLAY_FACTORY	"IReplayFactory001"

//----------------------------------------------------------------------------------------

#endif // IREPLAYFACTORY_H