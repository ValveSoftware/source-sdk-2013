//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Analogous to l2cache.h, this class represents information gleaned
//			from the 360's Performance Monitor Counters. In particular we 
//			are interested in l2 cache misses and load-hit-stores.
//
//=============================================================================//
#ifndef CPMCDATA_H
#define CPMCDATA_H
#ifdef _WIN32
#pragma once
#endif

#ifndef _X360
#error This file must only be compiled for XBOX360! 
#endif


// Warning: 
// As written, this class only supports profiling thread 0, processor 0.

class CPMCData
{
public:

	CPMCData();
	~CPMCData() {};

	void Start( void );
	void End( void );

	/// This function should be called exactly once during the lifespan of the program;
	/// it will set up the counters to record the information we are interested in. 
	/// This will stomp on whoever else might have set the performance counters elsewhere
	/// in the game.
	static void InitializeOnceProgramWide( void );
	static bool IsInitialized();

	//-------------------------------------------------------------------------
	// GetL2CacheMisses
	//-------------------------------------------------------------------------
	uint64 GetL2CacheMisses( void ) const
	{
		return m_Delta.L2CacheMiss;
	}

	uint64 GetLHS( void ) const
	{
		return m_Delta.LHS;
	}

/*
#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, tchar *pchName );		// Validate our internal structures
#endif // DBGFLAG_VALIDATE
*/

private:
	/// represents saved numbers from the counters we are interested in
	struct PMCounters
	{
		uint64 L2CacheMiss;
		uint64 LHS; ///< load hit store

		PMCounters(int64 _l2cm, int64 _lhs ) : L2CacheMiss(_l2cm), LHS(_lhs) {};
		PMCounters() : L2CacheMiss(0), LHS(0) {};
	};
	
	PMCounters m_OnStart; ///< values when we began the timer
	PMCounters m_Delta ; ///< computed total delta between start/stop
};

#endif   // CPMCDATA_H