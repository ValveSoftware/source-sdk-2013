//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef FASTTIMER_H
#define FASTTIMER_H
#ifdef _WIN32
#pragma once
#endif

#ifdef _WIN32
#include <intrin.h>
#endif

#include <assert.h>
#include "tier0/platform.h"

PLATFORM_INTERFACE uint64 g_ClockSpeed;
#if defined( _X360 ) && defined( _CERT )
PLATFORM_INTERFACE unsigned long g_dwFakeFastCounter;
#endif

PLATFORM_INTERFACE double g_ClockSpeedMicrosecondsMultiplier;
PLATFORM_INTERFACE double g_ClockSpeedMillisecondsMultiplier;
PLATFORM_INTERFACE double g_ClockSpeedSecondsMultiplier;

class CCycleCount
{
friend class CFastTimer;

public:
					CCycleCount();
					CCycleCount( uint64 cycles );

	void			Sample();	// Sample the clock. This takes about 34 clocks to execute (or 26,000 calls per millisecond on a P900).

	void			Init();		// Set to zero.
	void			Init( float initTimeMsec );
	void			Init( double initTimeMsec )		{ Init( (float)initTimeMsec ); }
	void			Init( uint64 cycles );
	bool			IsLessThan( CCycleCount const &other ) const;					// Compare two counts.

	// Convert to other time representations. These functions are slow, so it's preferable to call them
	// during display rather than inside a timing block.
	unsigned long	GetCycles()  const;
	uint64			GetLongCycles() const;

	unsigned long	GetMicroseconds() const;
	uint64			GetUlMicroseconds() const;
	double			GetMicrosecondsF() const; 	
	void			SetMicroseconds( unsigned long nMicroseconds );

	unsigned long	GetMilliseconds() const;
	double			GetMillisecondsF() const;

	double			GetSeconds() const;

	CCycleCount&	operator+=( CCycleCount const &other );

	// dest = rSrc1 + rSrc2
	static void		Add( CCycleCount const &rSrc1, CCycleCount const &rSrc2, CCycleCount &dest );	// Add two samples together.
	
	// dest = rSrc1 - rSrc2
	static void		Sub( CCycleCount const &rSrc1, CCycleCount const &rSrc2, CCycleCount &dest );	// Add two samples together.

	static uint64	GetTimestamp();

	uint64			m_Int64;
};

class PLATFORM_CLASS CClockSpeedInit
{
public:
	CClockSpeedInit()
	{
		Init();
	}

	static void Init();
};

class CFastTimer
{
public:
	// These functions are fast to call and should be called from your sampling code.
	void				Start();
	void				End();

	const CCycleCount &	GetDuration() const;	// Get the elapsed time between Start and End calls.
	CCycleCount 		GetDurationInProgress() const; // Call without ending. Not that cheap.

	// Return number of cycles per second on this processor.
	static inline int64	GetClockSpeed();

private:
	CCycleCount	m_Duration;
#ifdef DEBUG_FASTTIMER
	bool m_bRunning;		// Are we currently running?
#endif
};


// This is a helper class that times whatever block of code it's in
class CTimeScope
{
public:
				CTimeScope( CFastTimer *pTimer );
				~CTimeScope();

private:	
	CFastTimer	*m_pTimer;
};

inline CTimeScope::CTimeScope( CFastTimer *pTotal )
{
	m_pTimer = pTotal;
	m_pTimer->Start();
}

inline CTimeScope::~CTimeScope()
{
	m_pTimer->End();
}

// This is a helper class that times whatever block of code it's in and
// adds the total (int microseconds) to a global counter.
class CTimeAdder
{
public:
				CTimeAdder( CCycleCount *pTotal );
				~CTimeAdder();

	void		End();

private:	
	CCycleCount	*m_pTotal;
	CFastTimer	m_Timer;
};

inline CTimeAdder::CTimeAdder( CCycleCount *pTotal )
{
	m_pTotal = pTotal;
	m_Timer.Start();
}

inline CTimeAdder::~CTimeAdder()
{
	End();
}

inline void CTimeAdder::End()
{
	if( m_pTotal )
	{
		m_Timer.End();
		*m_pTotal += m_Timer.GetDuration();
		m_pTotal = 0;
	}
}



// -------------------------------------------------------------------------- // 
// Simple tool to support timing a block of code, and reporting the results on
// program exit or at each iteration
//
//	Macros used because dbg.h uses this header, thus Msg() is unavailable
// -------------------------------------------------------------------------- // 

#define PROFILE_SCOPE(name) \
	class C##name##ACC : public CAverageCycleCounter \
	{ \
	public: \
		~C##name##ACC() \
		{ \
			Msg("%-48s: %6.3f avg (%8.1f total, %7.3f peak, %5d iters)\n",  \
				#name, \
				GetAverageMilliseconds(), \
				GetTotalMilliseconds(), \
				GetPeakMilliseconds(), \
				GetIters() ); \
		} \
	}; \
	static C##name##ACC name##_ACC; \
	CAverageTimeMarker name##_ATM( &name##_ACC )

#define TIME_SCOPE(name) \
	class CTimeScopeMsg_##name \
	{ \
	public: \
		CTimeScopeMsg_##name() { m_Timer.Start(); } \
		~CTimeScopeMsg_##name() \
		{ \
			m_Timer.End(); \
			Msg( #name "time: %.4fms\n", m_Timer.GetDuration().GetMillisecondsF() ); \
		} \
	private:	\
		CFastTimer	m_Timer; \
	} name##_TSM;


// -------------------------------------------------------------------------- // 

class CAverageCycleCounter
{
public:
	CAverageCycleCounter();
	
	void Init();
	void MarkIter( const CCycleCount &duration );
	
	unsigned GetIters() const;
	
	double GetAverageMilliseconds() const;
	double GetTotalMilliseconds() const;
	double GetPeakMilliseconds() const;

private:
	unsigned	m_nIters;
	CCycleCount m_Total;
	CCycleCount	m_Peak;
	bool		m_fReport;
	const tchar *m_pszName;
};

// -------------------------------------------------------------------------- // 

class CAverageTimeMarker
{
public:
	CAverageTimeMarker( CAverageCycleCounter *pCounter );
	~CAverageTimeMarker();
	
private:
	CAverageCycleCounter *m_pCounter;
	CFastTimer	m_Timer;
};


// -------------------------------------------------------------------------- // 
// CCycleCount inlines.
// -------------------------------------------------------------------------- // 

inline CCycleCount::CCycleCount()
{
	Init( (uint64)0 );
}

inline CCycleCount::CCycleCount( uint64 cycles )
{
	Init( cycles );
}

inline void CCycleCount::Init()
{
	Init( (uint64)0 );
}

inline void CCycleCount::Init( float initTimeMsec )
{
	if ( g_ClockSpeedMillisecondsMultiplier > 0 )
		Init( (uint64)(initTimeMsec / g_ClockSpeedMillisecondsMultiplier) );
	else
		Init( (uint64)0 );
}

inline void CCycleCount::Init( uint64 cycles )
{
	m_Int64 = cycles;
}

inline void CCycleCount::Sample()
{
	m_Int64 = Plat_Rdtsc();
}

inline CCycleCount& CCycleCount::operator+=( CCycleCount const &other )
{
	m_Int64 += other.m_Int64;
	return *this;
}


inline void CCycleCount::Add( CCycleCount const &rSrc1, CCycleCount const &rSrc2, CCycleCount &dest )
{
	dest.m_Int64 = rSrc1.m_Int64 + rSrc2.m_Int64;
}

inline void CCycleCount::Sub( CCycleCount const &rSrc1, CCycleCount const &rSrc2, CCycleCount &dest )
{
	dest.m_Int64 = rSrc1.m_Int64 - rSrc2.m_Int64;
}

inline uint64 CCycleCount::GetTimestamp()
{
	CCycleCount c;
	c.Sample();
	return c.GetLongCycles();
}

inline bool CCycleCount::IsLessThan(CCycleCount const &other) const
{
	return m_Int64 < other.m_Int64;
}


inline unsigned long CCycleCount::GetCycles() const
{
	return (unsigned long)m_Int64;
}

inline uint64 CCycleCount::GetLongCycles() const
{
	return m_Int64;
}

inline unsigned long CCycleCount::GetMicroseconds() const
{
	return (unsigned long)((m_Int64 * 1000000) / g_ClockSpeed);
}

inline uint64 CCycleCount::GetUlMicroseconds() const
{
	return ((m_Int64 * 1000000) / g_ClockSpeed);
}


inline double CCycleCount::GetMicrosecondsF() const
{
	return (double)( m_Int64 * g_ClockSpeedMicrosecondsMultiplier );
}


inline void	CCycleCount::SetMicroseconds( unsigned long nMicroseconds )
{
	m_Int64 = ((uint64)nMicroseconds * g_ClockSpeed) / 1000000;
}


inline unsigned long CCycleCount::GetMilliseconds() const
{
	return (unsigned long)((m_Int64 * 1000) / g_ClockSpeed);
}


inline double CCycleCount::GetMillisecondsF() const
{
	return (double)( m_Int64 * g_ClockSpeedMillisecondsMultiplier );
}


inline double CCycleCount::GetSeconds() const
{
	return (double)( m_Int64 * g_ClockSpeedSecondsMultiplier );
}


// -------------------------------------------------------------------------- // 
// CFastTimer inlines.
// -------------------------------------------------------------------------- // 
inline void CFastTimer::Start()
{
	m_Duration.Sample();
#ifdef DEBUG_FASTTIMER
	m_bRunning = true;
#endif
}


inline void CFastTimer::End()
{
	CCycleCount cnt;
	cnt.Sample();
	if ( IsX360() )
	{
		// have to handle rollover, hires timer is only accurate to 32 bits
		// more than one overflow should not have occurred, otherwise caller should use a slower timer
		if ( (uint64)cnt.m_Int64 <= (uint64)m_Duration.m_Int64 )
		{
			// rollover occurred	
			cnt.m_Int64 += 0x100000000LL;	
		}
	}

	m_Duration.m_Int64 = cnt.m_Int64 - m_Duration.m_Int64;

#ifdef DEBUG_FASTTIMER
	m_bRunning = false;
#endif
}

inline CCycleCount CFastTimer::GetDurationInProgress() const
{
	CCycleCount cnt;
	cnt.Sample();
	if ( IsX360() )
	{
		// have to handle rollover, hires timer is only accurate to 32 bits
		// more than one overflow should not have occurred, otherwise caller should use a slower timer
		if ( (uint64)cnt.m_Int64 <= (uint64)m_Duration.m_Int64 )
		{
			// rollover occurred	
			cnt.m_Int64 += 0x100000000LL;	
		}
	}

	CCycleCount result;
	result.m_Int64 = cnt.m_Int64 - m_Duration.m_Int64;
	
	return result;
}


inline int64 CFastTimer::GetClockSpeed()
{
	return g_ClockSpeed;
}


inline CCycleCount const& CFastTimer::GetDuration() const
{
#ifdef DEBUG_FASTTIMER
	assert( !m_bRunning );
#endif
	return m_Duration;
}


// -------------------------------------------------------------------------- // 
// CAverageCycleCounter inlines

inline CAverageCycleCounter::CAverageCycleCounter()
 :	m_nIters( 0 )
{
}

inline void CAverageCycleCounter::Init()
{
	m_Total.Init();
	m_Peak.Init();
	m_nIters = 0;
}

inline void CAverageCycleCounter::MarkIter( const CCycleCount &duration )
{
	++m_nIters;
	m_Total += duration;
	if ( m_Peak.IsLessThan( duration ) )
		m_Peak = duration;
}

inline unsigned CAverageCycleCounter::GetIters() const
{
	return m_nIters;
}

inline double CAverageCycleCounter::GetAverageMilliseconds() const
{
	if ( m_nIters )
		return (m_Total.GetMillisecondsF() / (double)m_nIters);
	else
		return 0;
}

inline double CAverageCycleCounter::GetTotalMilliseconds() const
{
	return m_Total.GetMillisecondsF();
}

inline double CAverageCycleCounter::GetPeakMilliseconds() const
{
	return m_Peak.GetMillisecondsF();
}

// -------------------------------------------------------------------------- // 

inline CAverageTimeMarker::CAverageTimeMarker( CAverageCycleCounter *pCounter )
{
	m_pCounter = pCounter;
	m_Timer.Start();
}

inline CAverageTimeMarker::~CAverageTimeMarker()
{
	m_Timer.End();
	m_pCounter->MarkIter( m_Timer.GetDuration() );
}


// CLimitTimer
// Use this to time whether a desired interval of time has passed.  It's extremely fast
// to check while running.  NOTE: CMicroSecOverage() and CMicroSecLeft() are not as fast to check.
class CLimitTimer
{
public:
	CLimitTimer() {}
	CLimitTimer( uint64 cMicroSecDuration ) { SetLimit( cMicroSecDuration ); }
	void SetLimit( uint64 m_cMicroSecDuration );
	bool BLimitReached() const;

	int CMicroSecOverage() const;
	uint64 CMicroSecLeft() const; 

private:
	uint64 m_lCycleLimit;
};


//-----------------------------------------------------------------------------
// Purpose: Initializes the limit timer with a period of time to measure.
// Input  : cMicroSecDuration -		How long a time period to measure
//-----------------------------------------------------------------------------
inline void CLimitTimer::SetLimit( uint64 cMicroSecDuration )
{
	uint64 dlCycles = ( ( uint64 ) cMicroSecDuration * g_ClockSpeed ) / ( uint64 ) 1000000L;
	CCycleCount cycleCount;
	cycleCount.Sample( );
	m_lCycleLimit = cycleCount.GetLongCycles( ) + dlCycles;
}


//-----------------------------------------------------------------------------
// Purpose: Determines whether our specified time period has passed
// Output:	true if at least the specified time period has passed
//-----------------------------------------------------------------------------
inline bool CLimitTimer::BLimitReached() const
{
	CCycleCount cycleCount;
	cycleCount.Sample( );
	return ( cycleCount.GetLongCycles( ) >= m_lCycleLimit );
}


//-----------------------------------------------------------------------------
// Purpose: If we're over our specified time period, return the amount of the overage.
// Output:	# of microseconds since we reached our specified time period.
//-----------------------------------------------------------------------------
inline int CLimitTimer::CMicroSecOverage() const
{
	CCycleCount cycleCount;
	cycleCount.Sample();
	uint64 lcCycles = cycleCount.GetLongCycles();

	if ( lcCycles < m_lCycleLimit )
		return 0;

	return( ( int ) ( ( lcCycles - m_lCycleLimit ) * ( uint64 ) 1000000L / g_ClockSpeed ) );
}


//-----------------------------------------------------------------------------
// Purpose: If we're under our specified time period, return the amount under.
// Output:	# of microseconds until we reached our specified time period, 0 if we've passed it
//-----------------------------------------------------------------------------
inline uint64 CLimitTimer::CMicroSecLeft() const
{
	CCycleCount cycleCount;
	cycleCount.Sample();
	uint64 lcCycles = cycleCount.GetLongCycles();

	if ( lcCycles >= m_lCycleLimit )
		return 0;

	return( ( uint64 ) ( ( m_lCycleLimit - lcCycles ) * ( uint64 ) 1000000L / g_ClockSpeed ) );
}


#endif // FASTTIMER_H
