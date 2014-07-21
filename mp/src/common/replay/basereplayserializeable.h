//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef REPLAYSERIALIIZEABLE_H
#define REPLAYSERIALIIZEABLE_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "replay/ireplayserializeable.h"
#include "replay/replayhandle.h"

//----------------------------------------------------------------------------------------

class CBaseReplaySerializeable : public IReplaySerializeable
{
public:
	CBaseReplaySerializeable();

	virtual void			SetHandle( ReplayHandle_t h );
	virtual ReplayHandle_t	GetHandle() const;
	virtual bool			Read( KeyValues *pIn );
	virtual void			Write( KeyValues *pOut );
	virtual const char		*GetFilename() const;
	virtual const char		*GetFullFilename() const;
	virtual const char		*GetDebugName() const;
	virtual void			SetLocked( bool bLocked );
	virtual bool			IsLocked() const;
	virtual void			OnDelete();
	virtual void			OnUnload();
	virtual void			OnAddedToDirtyList();

private:
	ReplayHandle_t			m_hThis;
	bool					m_bLocked;
};

//----------------------------------------------------------------------------------------

#endif // REPLAYSERIALIIZEABLE_H