//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DMETIMEFRAME_H
#define DMETIMEFRAME_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"
#include "movieobjects/timeutils.h"


//-----------------------------------------------------------------------------
// Time classes used to help disambiguate between local + parent time
//-----------------------------------------------------------------------------
typedef int LocalTime_t;
typedef int ParentTime_t;

class CDmeTimeFrame : public CDmElement
{
	DEFINE_ELEMENT( CDmeTimeFrame, CDmElement );

public:
	// Methods of IDmElement
	virtual void	OnAttributeChanged( CDmAttribute *pAttribute );

	DmeTime_t ToChildMediaTime( DmeTime_t t, bool bClamp = true ) const;
	DmeTime_t FromChildMediaTime( DmeTime_t t, bool bClamp = true ) const;
	DmeTime_t ToChildMediaDuration( DmeTime_t dt ) const;
	DmeTime_t FromChildMediaDuration( DmeTime_t dt ) const;

	DmeTime_t GetStartTime() const;
	DmeTime_t GetDuration() const;
	DmeTime_t GetTimeOffset() const;
	float GetTimeScale() const;

	DmeTime_t GetStartInChildMediaTime() const;
	DmeTime_t GetEndInChildMediaTime() const;

	void SetStartTime( DmeTime_t startTime );
	void SetDuration( DmeTime_t duration );
	void SetTimeOffset( DmeTime_t offset );
	void SetTimeScale( float flScale );

	void SetEndTime( DmeTime_t endTime, bool bChangeDuration );
	void SetTimeScale( float flScale, DmeTime_t scaleCenter, bool bChangeDuration );

private:
	CDmaVar< int > m_Start;
	CDmaVar< int > m_Duration;
	CDmaVar< int > m_Offset;
	CDmaVar< float > m_Scale;
};


//-----------------------------------------------------------------------------
// inline methods
//-----------------------------------------------------------------------------
inline DmeTime_t CDmeTimeFrame::GetStartTime() const
{
	return DmeTime_t( m_Start );
}

inline DmeTime_t CDmeTimeFrame::GetDuration() const
{
	return DmeTime_t( m_Duration );
}

inline DmeTime_t CDmeTimeFrame::GetTimeOffset() const
{
	return DmeTime_t( m_Offset );
}

inline float CDmeTimeFrame::GetTimeScale() const
{
	return m_Scale;
}

inline void CDmeTimeFrame::SetStartTime( DmeTime_t flStartTime )
{
	m_Start = flStartTime.GetTenthsOfMS();
}

inline void CDmeTimeFrame::SetDuration( DmeTime_t flDuration )
{
	Assert( m_Duration >= 0 ); // if you want a reversed clip, set the timescale negative
	m_Duration = flDuration.GetTenthsOfMS();
}

inline void CDmeTimeFrame::SetTimeOffset( DmeTime_t flOffset )
{
	m_Offset = flOffset.GetTenthsOfMS();
}

inline void CDmeTimeFrame::SetTimeScale( float flScale )
{
	m_Scale = flScale;
}


//-----------------------------------------------------------------------------
// Convert back + forth between my media time and child media time
//-----------------------------------------------------------------------------
inline DmeTime_t CDmeTimeFrame::ToChildMediaTime( DmeTime_t t, bool bClamp ) const
{
	t -= DmeTime_t( m_Start );
	if ( bClamp )
	{
		t.Clamp( DMETIME_ZERO, DmeTime_t( m_Duration ) );
	}
	return ( t + DmeTime_t( m_Offset ) ) * m_Scale;
}

inline DmeTime_t CDmeTimeFrame::FromChildMediaTime( DmeTime_t t, bool bClamp ) const
{
	t = DmeTime_t( t ) / m_Scale - DmeTime_t( m_Offset );
	if ( bClamp )
	{
		t.Clamp( DMETIME_ZERO, DmeTime_t( m_Duration ) );
	}
	return t + DmeTime_t( m_Start );
}

inline DmeTime_t CDmeTimeFrame::ToChildMediaDuration( DmeTime_t dt ) const
{
	return dt * m_Scale;
}

inline DmeTime_t CDmeTimeFrame::FromChildMediaDuration( DmeTime_t dt ) const
{
	return dt / m_Scale;
}

inline DmeTime_t CDmeTimeFrame::GetStartInChildMediaTime() const
{
	DmeTime_t time = DmeTime_t( m_Offset ) * m_Scale;
	Assert( time == ToChildMediaTime( GetStartTime() ) );
	return time;
}

inline DmeTime_t CDmeTimeFrame::GetEndInChildMediaTime() const
{
	DmeTime_t time = DmeTime_t( m_Offset + m_Duration ) * m_Scale;
	Assert( time == ToChildMediaTime( GetStartTime() + GetDuration() ) );
	return time;
}


#endif // DMETIMEFRAME_H
