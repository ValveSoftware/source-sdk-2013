//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// data_collector.h
// Data collection system
// Author: Michael S. Booth, June 2004

#ifndef _DATA_COLLECTOR_H_
#define _DATA_COLLECTOR_H_

#include <igameevents.h>
#include <KeyValues.h>

/**
 * This class is used to monitor the event stream and
 * store interesting events to disk for later analysis.
 */
class CDataCollector : public IGameEventListener
{
public:
	CDataCollector( void );
	~CDataCollector();

	// IGameEventListener 
	virtual void FireGameEvent( KeyValues *event );
};


extern void StartDataCollection( void );
extern void StopDataCollection( void );


#endif // _DATA_COLLECTOR_H_
