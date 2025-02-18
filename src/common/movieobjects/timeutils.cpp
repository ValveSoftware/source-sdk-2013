//========= Copyright Valve Corporation, All rights reserved. ============//

#include "movieobjects/timeutils.h"

#include "tier0/dbg.h"
#include "mathlib/mathlib.h"

#include <math.h>


////////////////////////////////////////////////////////////////////////////////////////
//
// DmeFramerate_t
//
// exact (rational) representation of common framerates - any integral or ntsc framerate
//
////////////////////////////////////////////////////////////////////////////////////////

DmeFramerate_t::DmeFramerate_t( float fps )
{
	SetFramerate( fps );
}

DmeFramerate_t::DmeFramerate_t( int fps /*= 0*/ ) :
	m_num( fps ), m_den( 10000 )
{
}

DmeFramerate_t::DmeFramerate_t( int nNumerator, int nDenominator ) : 
	m_num( nNumerator ), m_den( nDenominator * 10000 )
{
}

void DmeFramerate_t::SetFramerate( float flFrameRate )
{
	if ( IsIntegralValue( flFrameRate ) )
	{
		SetFramerate( RoundFloatToInt( flFrameRate ) );
	}
	else if ( IsIntegralValue( flFrameRate * 1001.0f / 1000.0f ) ) // 1001 is the ntsc divisor (30*1000/1001 = 29.97, etc)
	{
		SetFramerateNTSC( RoundFloatToInt( flFrameRate * 1001.0f / 1000.0f ) );
	}
	else
	{
		Assert( 0 );
		SetFramerate( RoundFloatToInt( flFrameRate ) );
	}
}

void DmeFramerate_t::SetFramerate( int fps )
{
	m_num = fps;
	m_den = 10000;
}

// other (uncommon) options besides 30(29.97 - ntsc video) are 24 (23.976 - ntsc film) and 60 (59.94 - ntsc progressive)
void DmeFramerate_t::SetFramerateNTSC( int multiplier /*= 30*/ )
{
	// ntsc = 30 fps * 1000 / 1001 
	//      = ( 30 / 10000 fptms ) * 1000 / 1001
	//      = 30 / 10010
	m_num = multiplier;
	m_den = 10010;
}

float DmeFramerate_t::GetFramesPerSecond() const
{
	return 10000.0f * m_num / float( m_den );
}


////////////////////////////////////////////////////////////////////////////////////////
//
// DmeTime_t
//
// representing time as integral tenths of a millisecond (tms)
//
////////////////////////////////////////////////////////////////////////////////////////

DmeTime_t::DmeTime_t( int frame, DmeFramerate_t framerate )
{
	int64 num = int64( framerate.m_num );
	int64 prod = frame * int64( framerate.m_den );
	// add signed offset to force integer truncation (towards 0) to give us truncation towards -inf
	if ( frame < 0 )
	{
		prod -= num - 1;
	}
	m_tms = int( prod / num ); // round tms towards 0
}


// float operators - comment these out to find potentially incorrect uses of DmeTime_t

DmeTime_t DmeTime_t::operator*=( float f )
{
	m_tms = int( floor( m_tms * f + 0.5f ) );
	return *this;
}

DmeTime_t DmeTime_t::operator/=( float f )
{
	m_tms = int( floor( m_tms / f + 0.5f ) );
	return *this;
}


// helper methods

void DmeTime_t::Clamp( DmeTime_t lo, DmeTime_t hi )
{
	m_tms = clamp( m_tms, lo.m_tms, hi.m_tms );
}

bool DmeTime_t::IsInRange( DmeTime_t lo, DmeTime_t hi ) const
{
	return m_tms >= lo.m_tms && m_tms < hi.m_tms;
}


// helper functions

float GetFractionOfTimeBetween( DmeTime_t t, DmeTime_t start, DmeTime_t end, bool bClamp /*= false*/ )
{
	return GetFractionOfTime( t - start, end - start, bClamp );
}

float GetFractionOfTime( DmeTime_t t, DmeTime_t duration, bool bClamp /*= false*/  )
{
	if ( duration == DMETIME_ZERO )
		return 0.0f;

	if ( bClamp )
	{
		t.Clamp( DMETIME_ZERO, duration );
	}
	return t.m_tms / float( duration.m_tms );
}

int FrameForTime( DmeTime_t t, DmeFramerate_t framerate )
{
	return t.CurrentFrame( framerate );
}


// framerate-dependent conversions to/from frames

int DmeTime_t::CurrentFrame( DmeFramerate_t framerate, bool bRoundDown ) const
{
	int64 den = int64( framerate.m_den );
	int64 num = int64( framerate.m_num );
	int64 prod = int64( m_tms ) * num;

	// times within this range are considered on a frame: (frame*den/num - 1, frame*den/num]
	// this follows from the truncation towards -inf behavior of the frame,framerate constructor above
	// the following logic is there to ensure the above rule,
	// while working around the truncation towards 0 behavior of integer divide
	if ( m_tms < 0 )
	{
		if ( bRoundDown )
			prod -= den - num;
	}
	else
	{
		if ( bRoundDown )
			prod += num - 1;
		else
			prod += den - 1;
	}
	return int( prod / den );
}

DmeTime_t DmeTime_t::TimeAtCurrentFrame( DmeFramerate_t framerate, bool bRoundDown ) const
{
	int frame = CurrentFrame( framerate, bRoundDown );
	return DmeTime_t( frame, framerate );
}
DmeTime_t DmeTime_t::TimeAtNextFrame( DmeFramerate_t framerate ) const
{
	// since we always round towards -inf, go to next frame whether we're on a frame or not
	int frame = CurrentFrame( framerate, true );
	return DmeTime_t( frame + 1, framerate );
}
DmeTime_t DmeTime_t::TimeAtPrevFrame( DmeFramerate_t framerate ) const
{
	int frame = CurrentFrame( framerate, false );
	return DmeTime_t( frame - 1, framerate ); // we're exactly on a frame
}


int DmeTime_t::RoundSecondsToTMS( float sec )
{
	return floor( 10000.0f * sec + 0.5f ); // round at half-tms boundary
}

int DmeTime_t::RoundSecondsToTMS( double sec )
{
	return floor( 10000.0 * sec + 0.5 ); // round at half-tms boundary
}
