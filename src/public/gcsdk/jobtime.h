//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Encapsultes the job system's version of time (which is != to wall clock time)
//
//=============================================================================

#ifndef JOBTIME_H
#define JOBTIME_H
#ifdef _WIN32
#pragma once
#endif

namespace GCSDK
{

// CJobTime
// This is our primary job time structure.  It's similar to the Windows FILETIME structure, but
// with 1/10th the nominal resolution.
// It offers 1 microsecond resolution beginning on January 1, 1601.
// This is NOT wall clock time, it is time based proxy for a frame counter.
// This time PAUSES when you are in the debugger. Most timeout checks (things that don't need to be 
// exact time wise should use this );
class CJobTime
{
public:
	CJobTime();

	void SetToJobTime();
	void SetFromJobTime( int64 dMicroSecOffset );

	// Amount of time that's passed between this CJobTime and the current time.
	int64 CServerMicroSecsPassed() const;

	// Time accessors
	uint64 LTime() const { return m_lTime; }
	void SetLTime( uint64 lTime ) { m_lTime = lTime; }

	// Access our cached current time value
	static void UpdateJobTime( int cMicroSecPerShellFrame );
	static void SetCurrentJobTime( uint64 lCurrentTime );
	static uint64 LJobTimeCur() { return sm_lTimeCur; }

	bool operator==( const CJobTime &val ) const { return val.m_lTime == m_lTime; }
	bool operator!=( const CJobTime &val ) const { return val.m_lTime != m_lTime; }
	bool operator<( const CJobTime &val ) const { return m_lTime < val.m_lTime; }
	bool operator>( const CJobTime &val ) const { return m_lTime > val.m_lTime; }
	const CJobTime& operator+( const int64 &val )  { m_lTime += val; return *this; }
	const CJobTime& operator+=( const int64 &val )  { m_lTime += val; return *this; }

private:

	uint64 m_lTime;							// Our time value (microseconds since 1/1/1601)
	static uint64 sm_lTimeCur;				// Cached value of the current time (updated each frame)
};

const uint64 k_lJobTimeMaxFuture = (uint64)-1;


}
#endif // JOBTIME_H
