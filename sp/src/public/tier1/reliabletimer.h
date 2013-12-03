//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef RELIABLETIMER_H
#define RELIABLETIMER_H

#include "tier0/dbg.h"
//#include "constants.h"
#include "tier0/fasttimer.h"
#include "tier1/tier1.h"
#include "tier1/strtools.h"

#define DbgAssert Assert
#define kMILLION (1000000)
#define kTHOUSAND (1000)

// Timer class that uses QueryPerformanceCounter.  This is heavier-weight than CFastTimer which uses rdtsc,
// but this is reliable on multi-core systems whereas CFastTimer is not.

class CReliableTimer
{
public:
	CReliableTimer();
	void Start();
	void End();
	int64 GetMicroseconds();
	int64 GetMilliseconds();
	void SetLimit( uint64 m_cMicroSecDuration );
	bool BLimitReached();
	int64 CMicroSecOverage();
	int64 CMicroSecLeft(); 
	int64 CMilliSecLeft();
private:
	int64 GetPerformanceCountNow();

	int64 m_nPerformanceCounterStart;
	int64 m_nPerformanceCounterEnd;
	int64 m_nPerformanceCounterLimit;

	static int64 sm_nPerformanceFrequency;
	static bool sm_bUseQPC;
};


//-----------------------------------------------------------------------------
// Purpose: Records timer start time
//-----------------------------------------------------------------------------
inline void CReliableTimer::Start()
{
	m_nPerformanceCounterStart = GetPerformanceCountNow();
}

//-----------------------------------------------------------------------------
// Purpose: Records timer end time
//-----------------------------------------------------------------------------
inline void CReliableTimer::End()
{
	m_nPerformanceCounterEnd = GetPerformanceCountNow();

	// enforce that we've advanced at least one cycle
	if ( m_nPerformanceCounterEnd < m_nPerformanceCounterStart )
	{
#ifdef _SERVER
		if ( m_nPerformanceCounterEnd+10000 < m_nPerformanceCounterStart )
			AssertMsgOnce( false, CDbgFmtMsg( "CReliableTimer went backwards - start:%lld end:%lld", m_nPerformanceCounterStart, m_nPerformanceCounterEnd ).ToString() );
#endif
		m_nPerformanceCounterEnd = m_nPerformanceCounterStart + 1;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Gets microseconds elapsed between start and end
//-----------------------------------------------------------------------------
inline int64 CReliableTimer::GetMicroseconds()
{
	DbgAssert( m_nPerformanceCounterStart );		// timer must have been started
	DbgAssert( m_nPerformanceCounterEnd );			// timer must have been ended
	DbgAssert( 0 != sm_nPerformanceFrequency );	// must have calc'd performance counter frequency
	return ( ( m_nPerformanceCounterEnd - m_nPerformanceCounterStart ) * kMILLION / sm_nPerformanceFrequency );
}


//-----------------------------------------------------------------------------
// Purpose: Gets microseconds elapsed between start and end
//-----------------------------------------------------------------------------
inline int64 CReliableTimer::GetMilliseconds()
{
	DbgAssert( m_nPerformanceCounterStart );		// timer must have been started
	DbgAssert( m_nPerformanceCounterEnd );			// timer must have been ended
	DbgAssert( 0 != sm_nPerformanceFrequency );	// must have calc'd performance counter frequency
	return ( ( m_nPerformanceCounterEnd - m_nPerformanceCounterStart ) * kTHOUSAND / sm_nPerformanceFrequency );
}


//-----------------------------------------------------------------------------
// Purpose: Sets a limit on this timer that can subsequently be checked against
//-----------------------------------------------------------------------------
inline void CReliableTimer::SetLimit( uint64 cMicroSecDuration )
{
	DbgAssert( 0 != sm_nPerformanceFrequency );	// must have calc'd performance counter frequency
	m_nPerformanceCounterStart = GetPerformanceCountNow();
	m_nPerformanceCounterLimit = m_nPerformanceCounterStart + ( ( cMicroSecDuration * sm_nPerformanceFrequency  ) / kMILLION );
}


//-----------------------------------------------------------------------------
// Purpose: Returns if previously set limit has been reached
//-----------------------------------------------------------------------------
inline bool CReliableTimer::BLimitReached()
{
	DbgAssert( m_nPerformanceCounterStart );	// SetLimit must have been called
	DbgAssert( m_nPerformanceCounterLimit );	// SetLimit must have been called
	int64 nPerformanceCountNow = GetPerformanceCountNow();

	// make sure time advances
	if ( nPerformanceCountNow < m_nPerformanceCounterStart )
	{
#ifdef _SERVER
		if ( nPerformanceCountNow+10000 < m_nPerformanceCounterStart )
			AssertMsgOnce( false, CDbgFmtMsg( "CReliableTimer went backwards - start:%lld end:%lld", m_nPerformanceCounterStart, m_nPerformanceCounterEnd ).ToString() );
#endif
		// reset the limit to be lower, to match our new clock
		m_nPerformanceCounterLimit = nPerformanceCountNow + (m_nPerformanceCounterLimit - m_nPerformanceCounterStart);
	}

	return ( nPerformanceCountNow >= m_nPerformanceCounterLimit );
}


//-----------------------------------------------------------------------------
// Purpose: Returns microseconds current time is past limit, or 0 if not past limit
//-----------------------------------------------------------------------------
inline int64 CReliableTimer::CMicroSecOverage()
{
	DbgAssert( m_nPerformanceCounterStart );	// SetLimit must have been called
	DbgAssert( m_nPerformanceCounterLimit );	// SetLimit must have been called
	int64 nPerformanceCountNow = GetPerformanceCountNow();
#ifdef _SERVER
	if ( nPerformanceCountNow+10000 < m_nPerformanceCounterStart )
		AssertMsgOnce( nPerformanceCountNow >= m_nPerformanceCounterStart, CDbgFmtMsg( "CReliableTimer went backwards - start:%lld end:%lld", m_nPerformanceCounterStart, m_nPerformanceCounterEnd ).ToString() );
#endif
	int64 nPerformanceCountOver = ( nPerformanceCountNow > m_nPerformanceCounterLimit ? 
		nPerformanceCountNow - m_nPerformanceCounterLimit : 0 );

	Assert( 0 != sm_nPerformanceFrequency );	// must have calc'd performance counter frequency
	return ( nPerformanceCountOver * kMILLION / sm_nPerformanceFrequency );
}


//-----------------------------------------------------------------------------
// Purpose: Returns microseconds remaining until limit
//-----------------------------------------------------------------------------
inline int64 CReliableTimer::CMicroSecLeft()
{
	DbgAssert( m_nPerformanceCounterStart );	// SetLimit must have been called
	DbgAssert( m_nPerformanceCounterLimit );	// SetLimit must have been called
	int64 nPerformanceCountNow = GetPerformanceCountNow();
#ifdef _SERVER
	if ( nPerformanceCountNow+10000 < m_nPerformanceCounterStart )
		AssertMsgOnce( nPerformanceCountNow >= m_nPerformanceCounterStart, CDbgFmtMsg( "CReliableTimer went backwards - start:%lld end:%lld", m_nPerformanceCounterStart, m_nPerformanceCounterEnd ).ToString() );
#endif
	int64 nPerformanceCountLeft = ( nPerformanceCountNow < m_nPerformanceCounterLimit ? 
		m_nPerformanceCounterLimit - nPerformanceCountNow : 0 );

	DbgAssert( 0 != sm_nPerformanceFrequency );	// must have calc'd performance counter frequency
	return ( nPerformanceCountLeft * kMILLION / sm_nPerformanceFrequency );
}


//-----------------------------------------------------------------------------
// Purpose: Returns milliseconds remaining until limit
//-----------------------------------------------------------------------------
inline int64 CReliableTimer::CMilliSecLeft()
{
	return CMicroSecLeft() / 1000;
}


#endif // TICKLIMITTIMER_H
