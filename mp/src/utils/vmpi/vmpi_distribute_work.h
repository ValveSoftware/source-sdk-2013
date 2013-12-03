//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef VMPI_DISTRIBUTE_WORK_H
#define VMPI_DISTRIBUTE_WORK_H
#ifdef _WIN32
#pragma once
#endif


#include "messbuf.h"
#include "utlvector.h"


class IWorkUnitDistributorCallbacks
{
public:
	// Called every 200ms or so as it does the work.
	// Return true to stop distributing work.
	virtual bool Update() { return false; }

	// Called when a subsequent number of work units is completed.
	// e.g. results received in the following order will trigger
	// the following calls to OnWorkUnitsCompleted:
	// Work unit numbers:                         wu2 wu4 wu5 wu1 wu0 wu6 wu3
	// Calling OnWorkUnitsCompleted with arg:      -   -   -   -   3   -   7
	// because when wu0 is received we already have { wu0, wu1, wu2 } so we signal
	// that 3 subsequent work units completed, like wise by the time when wu3 is
	// received we already have a full set { wu0, wu1, wu2, wu3, wu4, wu5, wu6 }
	// and signal that 7 work units completed.
	virtual void OnWorkUnitsCompleted( uint64 numWorkUnits ) { return; }
};


enum EWorkUnitDistributor
{
	k_eWorkUnitDistributor_Default,
	k_eWorkUnitDistributor_SDK
};

// Tells which work unit distributor is going to be used.
EWorkUnitDistributor VMPI_GetActiveWorkUnitDistributor();


// Before calling DistributeWork, you can set this and it'll call your virtual functions.
extern IWorkUnitDistributorCallbacks *g_pDistributeWorkCallbacks;


// You must append data to pBuf with the work unit results.
// Note: pBuf will be NULL if this is a local thread doing work on the master.
typedef void (*ProcessWorkUnitFn)( int iThread, uint64 iWorkUnit, MessageBuffer *pBuf );

// pBuf is ready to read the results written to the buffer in ProcessWorkUnitFn.
typedef void (*ReceiveWorkUnitFn)( uint64 iWorkUnit, MessageBuffer *pBuf, int iWorker );


// Use a CDispatchReg to register this function with whatever packet ID you give to DistributeWork.
bool DistributeWorkDispatch( MessageBuffer *pBuf, int iSource, int iPacketID );



// This is the function vrad and vvis use to divide the work units and send them out.
// It maintains a sliding window of work units so it can always keep the clients busy.
//
// The workers implement processFn to do the job work in a work unit.
// This function must send back a packet formatted with:
// cPacketID (char), cSubPacketID (char), iWorkUnit (int), (app-specific data for the results)
//
// The masters implement receiveFn to receive a work unit's results.
//
// Returns time it took to finish the work.
double DistributeWork( 
	uint64 nWorkUnits,				// how many work units to dole out
	char cPacketID,					// This packet ID must be reserved for DistributeWork and DistributeWorkDispatch
									// must be registered with it.
	ProcessWorkUnitFn processFn,	// workers implement this to process a work unit and send results back
	ReceiveWorkUnitFn receiveFn		// the master implements this to receive a work unit
	);


// VMPI calls this before shutting down because any threads that DistributeWork has running must stop,
// otherwise it can crash if a thread tries to send data in the middle of shutting down.
void DistributeWork_Cancel();


#endif // VMPI_DISTRIBUTE_WORK_H
