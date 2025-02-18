//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef POSITIONWATCHER_H
#define POSITIONWATCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "ehandle.h"

// inherit from this interface to be able to call WatchPositionChanges
abstract_class IWatcherCallback
{
public:
	virtual ~IWatcherCallback() {}
};

abstract_class IPositionWatcher : public IWatcherCallback
{
public:
	virtual void NotifyPositionChanged( CBaseEntity *pEntity ) = 0;
};

// NOTE: The table of watchers is NOT saved/loaded!  Recreate these links on restore
void ReportPositionChanged( CBaseEntity *pMovedEntity );
void WatchPositionChanges( CBaseEntity *pWatcher, CBaseEntity *pMovingEntity );
void RemovePositionWatcher( CBaseEntity *pWatcher, CBaseEntity *pMovingEntity );


// inherit from this interface to be able to call WatchPositionChanges
abstract_class IVPhysicsWatcher : public IWatcherCallback
{
public:
	virtual void NotifyVPhysicsStateChanged( IPhysicsObject *pPhysics, CBaseEntity *pEntity, bool bAwake ) = 0;
};

// NOTE: The table of watchers is NOT saved/loaded!  Recreate these links on restore
void ReportVPhysicsStateChanged( IPhysicsObject *pPhysics, CBaseEntity *pEntity, bool bAwake );
void WatchVPhysicsStateChanges( CBaseEntity *pWatcher, CBaseEntity *pPhysicsEntity );
void RemoveVPhysicsStateWatcher( CBaseEntity *pWatcher, CBaseEntity *pPhysicsEntity );


#endif // POSITIONWATCHER_H
