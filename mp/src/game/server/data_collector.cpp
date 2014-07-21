//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// data_collector.cpp
// Data collection system
// Author: Michael S. Booth, June 2004

#include "cbase.h"
#include "data_collector.h"

static CDataCollector *collector = NULL;

//----------------------------------------------------------------------------------------------------------------------
void StartDataCollection( void )
{
	if (collector)
	{
		// already collecting
		return;
	}

	collector = new CDataCollector;
	Msg( "Data colletion started.\n" );
}
ConCommand data_collection_start( "data_collection_start", StartDataCollection, "Start collecting game event data." );


//----------------------------------------------------------------------------------------------------------------------
void StopDataCollection( void )
{
	if (collector)
	{
		delete collector;
		collector = NULL;

		Msg( "Data collection stopped.\n" );
	}
}
ConCommand data_collection_stop( "data_collection_stop", StopDataCollection, "Stop collecting game event data." );


//----------------------------------------------------------------------------------------------------------------------
CDataCollector::CDataCollector( void )
{
	// register for all events
	gameeventmanager->AddListener( this, true );
}

//----------------------------------------------------------------------------------------------------------------------
CDataCollector::~CDataCollector()
{
	gameeventmanager->RemoveListener( this );
}

//----------------------------------------------------------------------------------------------------------------------
/**
 * This is invoked for each event that occurs in the game
 */
void CDataCollector::FireGameEvent( KeyValues *event )
{
	DevMsg( "Collected event '%s'\n", event->GetName() );
}
