//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#if !defined( CLIENTSTATS_H )
#define CLIENTSTATS_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include <limits.h>
#include "tier0/dbg.h"

#define INTERFACEVERSION_CLIENTSTATS "ClientStats004"

//-----------------------------------------------------------------------------
// An interface used to help the client stats implementation tell time
//-----------------------------------------------------------------------------

struct IClientStatsTime
{
	virtual float GetTime() = 0;
};

//-----------------------------------------------------------------------------
// Allows clients to draw their own stats text, will be passed by the
// engine into DisplayStats of the IClientStats interface.
//-----------------------------------------------------------------------------

struct IClientStatsTextDisplay
{
	// Draws the stats
	virtual void DrawStatsText( PRINTF_FORMAT_STRING const char *fmt, ... ) = 0;

	virtual void SetDrawColor( unsigned char r, unsigned char g, unsigned char b ) = 0;

	// Sets a color based on a value and its max acceptable limit
	virtual void SetDrawColorFromStatValues( float limit, float value ) = 0;
};


//-----------------------------------------------------------------------------
// This will exist as a singleton within the client DLL and will be hooked into
// the engine to allow clients to render their own stats.
//-----------------------------------------------------------------------------

abstract_class IClientStats
{
public:
	// This is called at startup to tell the stats about time
	virtual void Init( IClientStatsTime* pTime ) = 0;

	// These methods are called at the beginning and the end of each run
	virtual void BeginRun() = 0;
	virtual void EndRun() = 0;

	// These methods are called at the beginning and the end of each frame
	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;

	// ---------------------------------------------------------------
	// All this stuff is used to prop stats for gathering r_speeds data during timedemo.
	// ---------------------------------------------------------------
	virtual int GetNumTimesStats( void ) const = 0;

	// returns timed stats
	virtual double TimedStatInFrame( int statID ) const = 0;
	virtual double TotalTimedStat( int statID ) const = 0;
};


//-----------------------------------------------------------------------------
// This is a templatized implementation which can be instantiated anywhere
// Note that you still have to install it and display it though.
//-----------------------------------------------------------------------------

template <int timedStatCount, int countedStatCount>
abstract_class CBaseClientStats : public IClientStats
{
public:
	void Init( IClientStatsTime* pTime );
	void BeginRun();
	void EndRun();
	void BeginFrame();
	void EndFrame();

	// Timed stat gathering
	void BeginTimedStat( int stat );
	void EndTimedStat( int stat );

	// ---------------------------------------------------------------
	// All this stuff is used to prop stats for gathering r_speeds data during timedemo.
	// ---------------------------------------------------------------
	// returns timed stats
	double TimedStatInFrame( int statID ) const
	{
		Assert( statID >= 0 && statID < timedStatCount );
		Assert( m_StatFrameTime[statID] >= 0.0 );
		return m_StatFrameTime[statID];
	}

	double TotalTimedStat( int statID ) const
	{
		Assert( statID >= 0 && statID < timedStatCount );
		return m_TotalStatTime[statID];
	}
	virtual const char *GetCountedStatName( int statID ) const = 0;
	virtual const char *GetTimedStatName( int statID ) const = 0;

protected:

	// Timed statistics
	double m_StatFrameTime[timedStatCount];
	double m_StatStartTime[timedStatCount];
	double m_TotalStatTime[timedStatCount];

private:
	IClientStatsTime* m_pTime;
};


//-----------------------------------------------------------------------------
// Initializes client stats
//-----------------------------------------------------------------------------

template <int timedStatCount, int countedStatCount>
void CBaseClientStats<timedStatCount, countedStatCount>::Init( IClientStatsTime* pTime )
{
	Assert( pTime );
	m_pTime = pTime;
}

//-----------------------------------------------------------------------------
// These methods are called at the beginning and the end of each run
//-----------------------------------------------------------------------------

template <int timedStatCount, int countedStatCount>
void CBaseClientStats<timedStatCount, countedStatCount>::BeginRun()
{
	int i;

	for (i = 0; i < timedStatCount; ++i)
		m_TotalStatTime[i] = 0.0;

}

template <int timedStatCount, int countedStatCount>
void CBaseClientStats<timedStatCount, countedStatCount>::EndRun()
{
}


//-----------------------------------------------------------------------------
// These methods are called at the beginning and the end of each frame
//-----------------------------------------------------------------------------

template <int timedStatCount, int countedStatCount>
void CBaseClientStats<timedStatCount, countedStatCount>::BeginFrame()
{
	int i;
	for (i = 0; i < timedStatCount; ++i)
		m_StatFrameTime[i] = 0.0;
}

template <int timedStatCount, int countedStatCount>
void CBaseClientStats<timedStatCount, countedStatCount>::EndFrame()
{
	int i;
	for (i = 0; i < timedStatCount; ++i)
		m_TotalStatTime[i] += m_StatFrameTime[i];
}


//-----------------------------------------------------------------------------
// Inlined stat gathering methods
//-----------------------------------------------------------------------------

template <int timedStatCount, int countedStatCount>
void CBaseClientStats<timedStatCount, countedStatCount>::BeginTimedStat( int stat )
{
	if (m_pTime)
		m_StatStartTime[stat] = m_pTime->GetTime();
}

template <int timedStatCount, int countedStatCount>
void CBaseClientStats<timedStatCount, countedStatCount>::EndTimedStat( int stat )
{
	if (m_pTime)
		m_StatFrameTime[stat] += m_pTime->GetTime() - m_StatStartTime[stat];
}


#endif // CLIENTSTATS_H
