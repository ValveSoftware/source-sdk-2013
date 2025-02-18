//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: vmpi_distribute_work sends events to this module, and this module
//          can track the events or display them graphically for debugging.
//
//=============================================================================//

#ifndef VMPI_DISTRIBUTE_TRACKER_H
#define VMPI_DISTRIBUTE_TRACKER_H
#ifdef _WIN32
#pragma once
#endif

// If bDebugMode is set, then it will remember all the VMPI events
// in case you call VMPITracker_WriteDebugFile.
void VMPITracker_Start( int nWorkUnits );

void VMPITracker_WorkUnitSentToWorker( int iWorkUnit, int iWorker );
void VMPITracker_WorkUnitStarted( int iWorkUnit, int iWorker );
void VMPITracker_WorkUnitCompleted( int iWorkUnit, int iWorker );
void VMPITracker_End();

// This will bring up a little menu they can use to 
// write a debug file.
void VMPITracker_HandleDebugKeypresses();

bool VMPITracker_WriteDebugFile( const char *pFilename );


#endif // VMPI_DISTRIBUTE_TRACKER_H


