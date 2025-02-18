//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IREPLAYSERIALIIZEABLE_H
#define IREPLAYSERIALIIZEABLE_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "interface.h"
#include "replay/replayhandle.h"

//----------------------------------------------------------------------------------------

class KeyValues;

//----------------------------------------------------------------------------------------

class IReplaySerializeable : public IBaseInterface
{
public:
	virtual void			SetHandle( ReplayHandle_t h ) = 0;
	virtual ReplayHandle_t	GetHandle() const = 0;

	virtual bool			Read( KeyValues *pIn ) = 0;
	virtual void			Write( KeyValues *pOut ) = 0;

	virtual const char		*GetSubKeyTitle() const = 0;
	virtual const char		*GetFilename() const = 0;
	virtual const char		*GetPath() const = 0;

	virtual const char		*GetFullFilename() const = 0;

	virtual void			SetLocked( bool bLocked ) = 0;
	virtual bool			IsLocked() const = 0;

	virtual void			OnDelete() = 0;
	virtual void			OnUnload() = 0;

	virtual const char		*GetDebugName() const = 0;
};

//----------------------------------------------------------------------------------------

#endif // IREPLAYSERIALIIZEABLE_H
