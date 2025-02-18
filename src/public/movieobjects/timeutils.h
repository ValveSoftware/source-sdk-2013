//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TIMEUTILS_H
#define TIMEUTILS_H
#ifdef _WIN32
#pragma once
#endif

#include <limits.h>
#include <math.h>
#include "platform.h"
#include "tier0/dbg.h"


class DmeTime_t;
#define DMETIME_TO_SECONDS( t ) ((t) * 0.0001f)

class DmeFramerate_t
{
public:
	DmeFramerate_t( float fps );
	DmeFramerate_t( int fps = 0 );
	DmeFramerate_t( const DmeFramerate_t& src ) : m_num( src.m_num ), m_den( src.m_den ) {}

	void SetFramerate( float flFrameRate );
	void SetFramerate( int fps );
	// other (uncommon) options besides 30(29.97 - ntsc video) are 24 (23.976 - ntsc film) and 60 (59.94 - ntsc progressive)
	void SetFramerateNTSC( int multiplier = 30 );

	float GetFramesPerSecond() const;

	bool operator==( DmeFramerate_t f ) const { return m_num == f.m_num && m_den == f.m_den; }
	bool operator!=( DmeFramerate_t f ) const { return m_num != f.m_num && m_den != f.m_den; }
	bool operator< ( DmeFramerate_t f ) const { return m_num * ( int )f.m_den <  f.m_num * ( int )m_den; }
	bool operator> ( DmeFramerate_t f ) const { return m_num * ( int )f.m_den >  f.m_num * ( int )m_den; }
	bool operator<=( DmeFramerate_t f ) const { return m_num * ( int )f.m_den <= f.m_num * ( int )m_den; }
	bool operator>=( DmeFramerate_t f ) const { return m_num * ( int )f.m_den >= f.m_num * ( int )m_den; }

	DmeFramerate_t operator*( int i ) const { return DmeFramerate_t( m_num * i, m_den ); }
	DmeFramerate_t operator/( int i ) const { return DmeFramerate_t( m_num, m_den * i ); }

	unsigned short abs( unsigned short i ) { return i >= 0 ? i : -i; }

	DmeFramerate_t operator*=( int i ) { Assert( abs( m_num * i ) <= USHRT_MAX ); m_num *= ( unsigned short )i; return *this; }
	DmeFramerate_t operator/=( int i ) { Assert( abs( m_den * i ) <= USHRT_MAX ); m_den *= ( unsigned short )i; return *this; }

private:
	DmeFramerate_t( int nNumerator, int nDenominator );

	unsigned short m_num;
	unsigned short m_den;

	friend class DmeTime_t;
};

#define DMETIME_ZERO		DmeTime_t(0)
#define DMETIME_MINDELTA	DmeTime_t::MinTimeDelta()
#define DMETIME_MINTIME		DmeTime_t::MinTime()
#define DMETIME_MAXTIME		DmeTime_t::MaxTime()
#define DMETIME_INVALID		DmeTime_t::InvalidTime()

class DmeTime_t
{
public:
	DmeTime_t() : m_tms( INT_MIN ) {} // invalid time
	explicit DmeTime_t( int tms ) : m_tms( tms ) {}
	explicit DmeTime_t( float  sec ) : m_tms( RoundSecondsToTMS( sec ) ) {}
	explicit DmeTime_t( double sec ) : m_tms( RoundSecondsToTMS( sec ) ) {}
	DmeTime_t( int frame, DmeFramerate_t framerate );


	// time operators

	bool operator==( DmeTime_t t ) const { return m_tms == t.m_tms; }
	bool operator!=( DmeTime_t t ) const { return m_tms != t.m_tms; }
	bool operator< ( DmeTime_t t ) const { return m_tms <  t.m_tms; }
	bool operator> ( DmeTime_t t ) const { return m_tms >  t.m_tms; }
	bool operator<=( DmeTime_t t ) const { return m_tms <= t.m_tms; }
	bool operator>=( DmeTime_t t ) const { return m_tms >= t.m_tms; }

	DmeTime_t operator%( DmeTime_t t ) const { return DmeTime_t( m_tms % t.m_tms ); }
	DmeTime_t operator+( DmeTime_t t ) const { return DmeTime_t( m_tms + t.m_tms ); }
	DmeTime_t operator-( DmeTime_t t ) const { return DmeTime_t( m_tms - t.m_tms ); }
	DmeTime_t operator-() const { return DmeTime_t( -m_tms ); }

	DmeTime_t operator+=( DmeTime_t t ) { m_tms += t.m_tms; return *this; }
	DmeTime_t operator-=( DmeTime_t t ) { m_tms -= t.m_tms; return *this; }

	// float operators - comment these out to find potentially incorrect uses of DmeTime_t

	friend DmeTime_t operator*( DmeTime_t t, float f ) { t *= f; return t; }
	friend DmeTime_t operator*( float f, DmeTime_t t ) { t *= f; return t; }
	friend DmeTime_t operator/( DmeTime_t t, float f ) { t /= f; return t; }
	friend float     operator/( DmeTime_t n, DmeTime_t d ) { return float( n.m_tms / double( d.m_tms ) ); }

	DmeTime_t operator*=( float f );
	DmeTime_t operator/=( float f );


	// limits, special values and validity

	bool IsValid() const { return m_tms != INT_MIN; }

	static DmeTime_t InvalidTime() { return DmeTime_t( INT_MIN ); }
	static DmeTime_t MinTime() { return DmeTime_t( INT_MIN + 1 ); }
	static DmeTime_t MaxTime() { return DmeTime_t( INT_MAX ); }
	static DmeTime_t MinTimeDelta() { return DmeTime_t( 1 ); }


	// conversions between other time representations

	int GetTenthsOfMS() const { return m_tms; }
	float GetSeconds() const { return DMETIME_TO_SECONDS( m_tms ); }
	void SetTenthsOfMS( int tms ) { m_tms = tms; }
	void SetSeconds( float sec ) { m_tms = RoundSecondsToTMS( sec ); }


	// helper methods

	void Clamp( DmeTime_t lo, DmeTime_t hi );
	bool IsInRange( DmeTime_t lo, DmeTime_t hi ) const;


	// helper functions
	friend float GetFractionOfTime( DmeTime_t t, DmeTime_t duration, bool bClamp );
	friend int FrameForTime( DmeTime_t t, DmeFramerate_t framerate );


	// standard arithmetic methods

	friend DmeTime_t abs( DmeTime_t t ) { return t.m_tms >= 0 ? t : -t; }


	// framerate-dependent conversions to/from frames

	int CurrentFrame( DmeFramerate_t framerate, bool bRoundDown = true ) const;
	DmeTime_t TimeAtCurrentFrame( DmeFramerate_t framerate, bool bRoundDown = true ) const;
	DmeTime_t TimeAtNextFrame( DmeFramerate_t framerate ) const;
	DmeTime_t TimeAtPrevFrame( DmeFramerate_t framerate ) const;

private:
	DmeTime_t( int64 tms ) : m_tms( int( tms ) ) {}

	// conversion helper methods - implementation
	static int RoundSecondsToTMS( float sec );
	static int RoundSecondsToTMS( double sec );

	int m_tms;
};

float GetFractionOfTimeBetween( DmeTime_t t, DmeTime_t start, DmeTime_t end, bool bClamp = false );


#endif // TIMEUTILS_H
