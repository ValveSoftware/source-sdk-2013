//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "tier0/dbg.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "choreoevent.h"
#include "choreoactor.h"
#include "choreochannel.h"
#include "minmax.h"
#include "mathlib/mathlib.h"
#include "tier1/strtools.h"
#include "choreoscene.h"
#include "ichoreoeventcallback.h"
#include "tier1/utlbuffer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int CChoreoEvent::s_nGlobalID = 1;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *owner - 
//			*name - 
//			percentage - 
//-----------------------------------------------------------------------------
CEventRelativeTag::CEventRelativeTag( CChoreoEvent *owner, const char *name, float percentage )
{
	Assert( owner );
	Assert( name );
	Assert( percentage >= 0.0f );
	Assert( percentage <= 1.0f );

	m_Name = name;
	m_flPercentage = percentage;
	m_pOwner = owner;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : src - 
//-----------------------------------------------------------------------------
CEventRelativeTag::CEventRelativeTag( const CEventRelativeTag& src )
{
	m_Name			= src.m_Name;
	m_flPercentage	= src.m_flPercentage;
	m_pOwner		= src.m_pOwner;
}
	
//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CEventRelativeTag::GetName( void )
{
	return m_Name.Get();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CEventRelativeTag::GetPercentage( void )
{
	return m_flPercentage;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : percentage - 
//-----------------------------------------------------------------------------
void CEventRelativeTag::SetPercentage( float percentage )
{
	m_flPercentage = percentage;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoEvent
//-----------------------------------------------------------------------------
CChoreoEvent *CEventRelativeTag::GetOwner( void )
{
	return m_pOwner;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CEventRelativeTag::SetOwner( CChoreoEvent *event )
{
	m_pOwner = event;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the corrected time based on the owner's length and start time
// Output : float
//-----------------------------------------------------------------------------
float CEventRelativeTag::GetStartTime( void )
{
	Assert( m_pOwner );
	if ( !m_pOwner )
	{
		return 0.0f;
	}

	float ownerstart		= m_pOwner->GetStartTime();
	float ownerduration		= m_pOwner->GetDuration();

	return ( ownerstart + ownerduration * m_flPercentage );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *owner - 
//			*name - 
//			percentage - 
//-----------------------------------------------------------------------------
CFlexTimingTag::CFlexTimingTag( CChoreoEvent *owner, const char *name, float percentage, bool locked )
: BaseClass( owner, name, percentage )
{
	m_bLocked = locked;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : src - 
//-----------------------------------------------------------------------------
CFlexTimingTag::CFlexTimingTag( const CFlexTimingTag& src )
: BaseClass( src )
{
	m_bLocked = src.m_bLocked;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFlexTimingTag::GetLocked( void )
{
	return m_bLocked;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : locked - 
//-----------------------------------------------------------------------------
void CFlexTimingTag::SetLocked( bool locked )
{
	m_bLocked = locked;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *owner - 
//			*name - 
//			percentage - 
//-----------------------------------------------------------------------------
CEventAbsoluteTag::CEventAbsoluteTag( CChoreoEvent *owner, const char *name, float t )
{
	Assert( owner );
	Assert( name );
	Assert( t >= 0.0f );

	m_Name = name;
	m_flPercentage = t;
	m_pOwner = owner;
	m_bLocked = false;
	m_bLinear = false;
	m_bEntry = false;
	m_bExit = false;

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : src - 
//-----------------------------------------------------------------------------
CEventAbsoluteTag::CEventAbsoluteTag( const CEventAbsoluteTag& src )
{
	m_Name			= src.m_Name;
	m_flPercentage	= src.m_flPercentage;
	m_pOwner		= src.m_pOwner;
	m_bLocked		= src.m_bLocked;
	m_bLinear		= src.m_bLinear;
	m_bEntry		= src.m_bEntry;
	m_bExit			= src.m_bExit;
}
	
//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CEventAbsoluteTag::GetName( void )
{
	return m_Name.Get();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CEventAbsoluteTag::GetPercentage( void )
{
	return m_flPercentage;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : percentage - 
//-----------------------------------------------------------------------------
void CEventAbsoluteTag::SetPercentage( float percentage )
{
	m_flPercentage = percentage;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CEventAbsoluteTag::GetEventTime( void )
{
	Assert( m_pOwner );
	if ( !m_pOwner )
	{
		return 0.0f;
	}

	float ownerduration		= m_pOwner->GetDuration();

	return (m_flPercentage * ownerduration);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : percentage - 
//-----------------------------------------------------------------------------
void CEventAbsoluteTag::SetEventTime( float t )
{
	Assert( m_pOwner );
	if ( !m_pOwner )
	{
		return;
	}

	float ownerduration		= m_pOwner->GetDuration();

	m_flPercentage = (t / ownerduration);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CEventAbsoluteTag::GetAbsoluteTime( void )
{
	Assert( m_pOwner );
	if ( !m_pOwner )
	{
		return 0.0f;
	}

	float ownerstart		= m_pOwner->GetStartTime();
	float ownerduration		= m_pOwner->GetDuration();

	return (ownerstart + m_flPercentage * ownerduration);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : percentage - 
//-----------------------------------------------------------------------------
void CEventAbsoluteTag::SetAbsoluteTime( float t )
{
	Assert( m_pOwner );
	if ( !m_pOwner )
	{
		return;
	}

	float ownerstart		= m_pOwner->GetStartTime();
	float ownerduration		= m_pOwner->GetDuration();

	m_flPercentage = (t - ownerstart) / ownerduration;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoEvent
//-----------------------------------------------------------------------------
CChoreoEvent *CEventAbsoluteTag::GetOwner( void )
{
	return m_pOwner;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CEventAbsoluteTag::SetOwner( CChoreoEvent *event )
{
	m_pOwner = event;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CEventAbsoluteTag::SetLocked( bool bLocked  )
{
	m_bLocked = bLocked;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoEvent
//-----------------------------------------------------------------------------
bool CEventAbsoluteTag::GetLocked( void )
{
	return m_bLocked;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CEventAbsoluteTag::SetLinear( bool bLinear  )
{
	m_bLinear = bLinear;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoEvent
//-----------------------------------------------------------------------------
bool CEventAbsoluteTag::GetLinear( void )
{
	return m_bLinear;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CEventAbsoluteTag::SetEntry( bool bEntry  )
{
	m_bEntry = bEntry;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoEvent
//-----------------------------------------------------------------------------
bool CEventAbsoluteTag::GetEntry( void )
{
	return m_bEntry;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CEventAbsoluteTag::SetExit( bool bExit  )
{
	m_bExit = bExit;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoEvent
//-----------------------------------------------------------------------------
bool CEventAbsoluteTag::GetExit( void )
{
	return m_bExit;
}




// FLEX ANIMATIONS
//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : *event - 
//-----------------------------------------------------------------------------
CFlexAnimationTrack::CFlexAnimationTrack( CChoreoEvent *event )
{
	m_pEvent			= event;
	m_pControllerName	=	NULL;
	m_bActive			= false;
	m_bCombo			= false;
	m_bServerSide		= false;
	m_nFlexControllerIndex[ 0 ] = m_nFlexControllerIndex[ 1 ] = -1;
	m_nFlexControllerIndexRaw[ 0 ] = m_nFlexControllerIndexRaw[ 1 ] = LocalFlexController_t(-1);

	// base track has range, combo is always 0..1
	m_flMin = 0.0f;
	m_flMax = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : src - 
//-----------------------------------------------------------------------------
CFlexAnimationTrack::CFlexAnimationTrack( const CFlexAnimationTrack* src )
{
	m_pControllerName = NULL;
	SetFlexControllerName( src->m_pControllerName ? src->m_pControllerName : "" );

	m_bActive	= src->m_bActive;
	m_bCombo	= src->m_bCombo;
	m_bServerSide = src->m_bServerSide;

	for ( int t = 0; t < 2; t++ )
	{
		m_Samples[ t ].Purge();
		for ( int i = 0 ;i < src->m_Samples[ t ].Size(); i++ )
		{
			CExpressionSample s = src->m_Samples[ t ][ i ];
			m_Samples[ t ].AddToTail( s );
		}
	}

	for ( int side = 0; side < 2; side++ )
	{
		m_nFlexControllerIndex[ side ] = src->m_nFlexControllerIndex[ side ];
		m_nFlexControllerIndexRaw[ side ] = src->m_nFlexControllerIndexRaw[ side ];
	}

	m_flMin = src->m_flMin;
	m_flMax = src->m_flMax;

	m_EdgeInfo[ 0 ] = src->m_EdgeInfo[ 0 ];
	m_EdgeInfo[ 1 ] = src->m_EdgeInfo[ 1 ];

	m_pEvent = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFlexAnimationTrack::~CFlexAnimationTrack( void )
{
	delete[] m_pControllerName;

	for ( int t = 0; t < 2; t++ )
	{
		m_Samples[ t ].Purge();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::SetEvent( CChoreoEvent *event )
{
	m_pEvent = event;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::Clear( void )
{
	for ( int t = 0; t < 2; t++ )
	{
		m_Samples[ t ].RemoveAll();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::RemoveSample( int index, int type /*=0*/ )
{
	Assert( type == 0 || type == 1 );

	m_Samples[ type ].Remove( index );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::SetFlexControllerName( const char *name )
{
	delete[] m_pControllerName;
	int len = Q_strlen( name ) + 1;
	m_pControllerName = new char[ len ];
	Q_strncpy( m_pControllerName, name, len );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *CFlexAnimationTrack::GetFlexControllerName( void )
{
	return m_pControllerName ? m_pControllerName : "";
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CFlexAnimationTrack::GetNumSamples( int type /*=0*/ )
{
	Assert( type == 0 || type == 1 );

	return m_Samples[ type ].Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : CExpressionSample
//-----------------------------------------------------------------------------
CExpressionSample *CFlexAnimationTrack::GetSample( int index, int type /*=0*/ )
{
	Assert( type == 0 || type == 1 );

	if ( index < 0 || index >= GetNumSamples( type ) )
		return NULL;
	return &m_Samples[ type ][ index ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFlexAnimationTrack::IsTrackActive( void )
{
	return m_bActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : active - 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::SetTrackActive( bool active )
{
	m_bActive = active;
}

void CFlexAnimationTrack::SetEdgeInfo( bool leftEdge, int curveType, float zero )
{
	int idx = leftEdge ? 0 : 1;
	m_EdgeInfo[ idx ].m_CurveType = curveType;
	m_EdgeInfo[ idx ].m_flZeroPos = zero;
}

void CFlexAnimationTrack::GetEdgeInfo( bool leftEdge, int& curveType, float& zero ) const
{
	int idx = leftEdge ? 0 : 1;
	curveType = m_EdgeInfo[ idx ].m_CurveType;
	zero = m_EdgeInfo[ idx ].m_flZeroPos;
}

void CFlexAnimationTrack::SetEdgeActive( bool leftEdge, bool state )
{
	int idx = leftEdge ? 0 : 1;
	m_EdgeInfo[ idx ].m_bActive = state;
}

bool CFlexAnimationTrack::IsEdgeActive( bool leftEdge ) const
{
	int idx = leftEdge ? 0 : 1;
	return m_EdgeInfo[ idx ].m_bActive;
}

int CFlexAnimationTrack::GetEdgeCurveType( bool leftEdge ) const
{
	if ( !IsEdgeActive( leftEdge ) )
	{
		return CURVE_DEFAULT;
	}

	int idx = leftEdge ? 0 : 1;
	return m_EdgeInfo[ idx ].m_CurveType;
}

float CFlexAnimationTrack::GetEdgeZeroValue( bool leftEdge ) const
{
	if ( !IsEdgeActive( leftEdge ) )
	{
		return 0.0f;
	}

	int idx = leftEdge ? 0 : 1;
	return m_EdgeInfo[ idx ].m_flZeroPos;
}

float CFlexAnimationTrack::GetDefaultEdgeZeroPos() const
{
	float zero = 0.0f;
	if ( m_flMin != m_flMax )
	{
		zero = ( 0.0f - m_flMin ) / ( m_flMax - m_flMin );
	}
	return zero;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CFlexAnimationTrack::GetZeroValue( int type, bool leftSide )
{
	// Stereo track is always clamped to 0.5 and doesn't care about l/r settings
	if ( type == 1 )
	{
		return 0.5f;
	}

	if ( IsEdgeActive( leftSide ) )
	{
		return GetEdgeZeroValue( leftSide );
	}

	return GetDefaultEdgeZeroPos();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : number - 
// Output : CExpressionSample
//-----------------------------------------------------------------------------
CExpressionSample *CFlexAnimationTrack::GetBoundedSample( int number, bool& bClamped, int type /*=0*/ )
{
	Assert( type == 0 || type == 1 );

	if ( number < 0 )
	{
		// Search for two samples which span time f
		static CExpressionSample nullstart;
		nullstart.time = 0.0f;
		nullstart.value = GetZeroValue( type, true );
		if ( type == 0 )
		{
			nullstart.SetCurveType( GetEdgeCurveType( true ) );
		}
		else
		{
			nullstart.SetCurveType( CURVE_DEFAULT );
		}
		bClamped = true;
		return &nullstart;
	}
	else if ( number >= GetNumSamples( type ) )
	{
		static CExpressionSample nullend;
		nullend.time = m_pEvent->GetDuration();
		nullend.value = GetZeroValue( type, false );
		if ( type == 0 )
		{
			nullend.SetCurveType( GetEdgeCurveType( false ) );
		}
		else
		{
			nullend.SetCurveType( CURVE_DEFAULT );
		}
		bClamped = true;
		return &nullend;
	}
	
	bClamped = false;
	return GetSample( number, type );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//			type - 
// Output : float
//-----------------------------------------------------------------------------
float CFlexAnimationTrack::GetIntensityInternal( float time, int type )
{
	Assert( type == 0 || type == 1 );

	float retval = 0.0f;

	// find samples that span the time
	if ( !m_pEvent || !m_pEvent->HasEndTime() || time < m_pEvent->GetStartTime() )
	{
		retval = GetZeroValue( type, true );;
	}
	else if ( time > m_pEvent->GetEndTime() )
	{
		retval = GetZeroValue( type, false );;
	}
	else
	{
		float elapsed = time - m_pEvent->GetStartTime();
		retval = GetFracIntensity( elapsed, type );
	}

	// scale
	if (type == 0 && m_flMin != m_flMax)
	{
		retval = retval * (m_flMax - m_flMin) + m_flMin;
	}
	return retval;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//			type - 
// Output : float
//-----------------------------------------------------------------------------
float CFlexAnimationTrack::GetFracIntensity( float time, int type )
{
	float zeroValueLeft = GetZeroValue( type, true );

	Assert( type == 0 || type == 1 );

	// find samples that span the time
	if ( !m_pEvent || !m_pEvent->HasEndTime() )
		return zeroValueLeft;

	int rampCount = GetNumSamples( type );
	if ( rampCount < 1 )
	{
		return zeroValueLeft;
	}

	CExpressionSample *esStart = NULL;
	CExpressionSample *esEnd = NULL;

	// do binary search for sample in time period
	int j = MAX( rampCount / 2, 1 );
	int i = j;
	while ( i > -2 && i < rampCount + 1 )
	{
		bool dummy;
		esStart = GetBoundedSample( i, dummy, type );
		esEnd = GetBoundedSample( i + 1, dummy, type );

		j = MAX( j / 2, 1 );
		if ( time < esStart->time)
		{
			i -= j;
		}
		else if ( time > esEnd->time)
		{
			i += j;
		}
		else
		{
			if ( time == esEnd->time )
			{	
				++i;
				esStart = GetBoundedSample( i, dummy, type );
				esEnd = GetBoundedSample( i + 1, dummy, type );
			}
			break;
		}
	}

	if (!esStart)
	{
		return zeroValueLeft;
	}

	int prev = i - 1;
	int next = i + 2;

	prev = MAX( -1, prev );
	next = MIN( next, rampCount );

	bool bclamp[ 2 ];
	CExpressionSample *esPre = GetBoundedSample( prev, bclamp[ 0 ], type );
	CExpressionSample *esNext = GetBoundedSample( next, bclamp[ 1 ], type );

	float dt = esEnd->time - esStart->time;

	Vector vPre( esPre->time, esPre->value, 0 );
	Vector vStart( esStart->time, esStart->value, 0 );
	Vector vEnd( esEnd->time, esEnd->value, 0 );
	Vector vNext( esNext->time, esNext->value, 0 );

	float f2 = 0.0f;
	if ( dt > 0.0f )
	{
		f2 = ( time - esStart->time ) / ( dt );
	}
	f2 = clamp( f2, 0.0f, 1.0f );

	Vector vOut;
	int dummy;
	int earlypart, laterpart;

	// Not holding out value of previous curve...
	Interpolator_CurveInterpolatorsForType( esStart->GetCurveType(), dummy, earlypart );
	Interpolator_CurveInterpolatorsForType( esEnd->GetCurveType(), laterpart, dummy );

	if ( earlypart == INTERPOLATE_HOLD )
	{
		// Hold "out" of previous sample (can cause a discontinuity)
		VectorLerp( vStart, vEnd, f2, vOut );
		vOut.y = vStart.y;
	}
	else if ( laterpart == INTERPOLATE_HOLD )
	{
		// Hold "out" of previous sample (can cause a discontinuity)
		VectorLerp( vStart, vEnd, f2, vOut );
		vOut.y = vEnd.y;
	}
	else
	{
		bool sameCurveType = earlypart == laterpart ? true : false;
		if ( sameCurveType )
		{
			Interpolator_CurveInterpolate( laterpart, vPre, vStart, vEnd, vNext, f2, vOut );
		}
		else // curves differ, sigh
		{
			Vector vOut1, vOut2;

			Interpolator_CurveInterpolate( earlypart, vPre, vStart, vEnd, vNext, f2, vOut1 );
			Interpolator_CurveInterpolate( laterpart, vPre, vStart, vEnd, vNext, f2, vOut2 );

			VectorLerp( vOut1, vOut2, f2, vOut );
		}
	}

	float retval = clamp( vOut.y, 0.0f, 1.0f );
	return retval;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
// Output : float
//-----------------------------------------------------------------------------
float CFlexAnimationTrack::GetSampleIntensity( float time )
{
	return GetIntensityInternal( time, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
// Output : float
//-----------------------------------------------------------------------------
float CFlexAnimationTrack::GetBalanceIntensity( float time )
{
	if ( IsComboType() )
	{
		return GetIntensityInternal( time, 1 );
	}

	return 1.0f;
}

// For a given time, computes 0->1 intensity value for the slider
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
// Output : float
//-----------------------------------------------------------------------------
float CFlexAnimationTrack::GetIntensity( float time, int side )
{
	float mag	= GetSampleIntensity( time );

	float scale = 1.0f;

	if ( IsComboType() )
	{
		float balance = GetBalanceIntensity( time );

		// Asking for left but balance is to right, then fall off as we go
		//  further right
		if ( side == 0 && balance > 0.5f )
		{
			scale = (1.0f - balance ) / 0.5f;
		}
		// Asking for right, but balance is left, fall off as we go left.
		else if ( side == 1 && balance < 0.5f )
		{
			scale = ( balance / 0.5f );
		}
	}

	return mag * scale;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//			value - 
//-----------------------------------------------------------------------------
CExpressionSample *CFlexAnimationTrack::AddSample( float time, float value, int type /*=0*/ )
{
	Assert( type == 0 || type == 1 );

	CExpressionSample sample;
	sample.time = time;
	sample.value = value;
	sample.selected = false;

	int idx = m_Samples[ type ].AddToTail( sample );
	
	// Resort( type );
	return &m_Samples[ type ][ idx ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::Resort( int type /*=0*/ )
{
	Assert( type == 0 || type == 1 );

	for ( int i = 0; i < m_Samples[ type ].Size(); i++ )
	{
		for ( int j = i + 1; j < m_Samples[ type ].Size(); j++ )
		{
			CExpressionSample src = m_Samples[ type ][ i ];
			CExpressionSample dest = m_Samples[ type ][ j ];

			if ( src.time > dest.time )
			{
				m_Samples[ type ][ i ] = dest;
				m_Samples[ type ][ j ] = src;
			}
		}
	}

	// Make sure nothing is out of range
	RemoveOutOfRangeSamples( 0 );
	RemoveOutOfRangeSamples( 1 );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoEvent
//-----------------------------------------------------------------------------
CChoreoEvent *CFlexAnimationTrack::GetEvent( void )
{
	return m_pEvent;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : side - 
// Output : int
//-----------------------------------------------------------------------------
int CFlexAnimationTrack::GetFlexControllerIndex( int side /*= 0*/ )
{
	Assert( side == 0 || side == 1 );

	if ( IsComboType() )
	{
		return m_nFlexControllerIndex[ side ];
	}
	
	return m_nFlexControllerIndex[ 0 ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : side - 
// Output : int
//-----------------------------------------------------------------------------
LocalFlexController_t CFlexAnimationTrack::GetRawFlexControllerIndex( int side /*= 0*/ )
{
	Assert( side == 0 || side == 1 );

	if ( IsComboType() )
	{
		return m_nFlexControllerIndexRaw[ side ];
	}
	
	return m_nFlexControllerIndexRaw[ 0 ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//			side - 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::SetFlexControllerIndex( LocalFlexController_t raw, int index, int side /*= 0*/ )
{
	Assert( side == 0 || side == 1 );

	m_nFlexControllerIndex[ side ] = index;
	// Model specific
	m_nFlexControllerIndexRaw[ side ] = raw;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : combo - 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::SetComboType( bool combo )
{
	m_bCombo = combo;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFlexAnimationTrack::IsComboType( void )
{
	return m_bCombo;
}

//-----------------------------------------------------------------------------
// Purpose: True if this should be simulated on the server side always
// Input  : state - 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::SetServerSide( bool state )
{
	m_bServerSide = state;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFlexAnimationTrack::IsServerSide() const
{
	return m_bServerSide;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::SetMin( float value )
{
	m_flMin = value;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::SetMax( float value )
{
	m_flMax = value;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CFlexAnimationTrack::GetMin( int type )
{
	if (type == 0)
		return m_flMin;
	else
		return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CFlexAnimationTrack::GetMax( int type )
{
	if (type == 0)
		return m_flMax;
	else
		return 1.0f;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFlexAnimationTrack::IsInverted( void )
{
	if (m_bInverted)
		return true;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::SetInverted( bool isInverted )
{
	m_bInverted = isInverted;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
void CFlexAnimationTrack::RemoveOutOfRangeSamples( int type )
{
	Assert( m_pEvent );
	if ( !m_pEvent )
		return;

	Assert( m_pEvent->HasEndTime() );
	float duration = m_pEvent->GetDuration();

	int c = m_Samples[ type ].Size();
	for ( int i = c-1; i >= 0; i-- )
	{
		CExpressionSample src = m_Samples[ type ][ i ];
		if ( src.time < 0 ||
			 src.time > duration )
		{
			m_Samples[ type ].Remove( i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CChoreoEvent::CChoreoEvent( CChoreoScene *scene )
{
	Init( scene );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//			*name - 
//-----------------------------------------------------------------------------
CChoreoEvent::CChoreoEvent( CChoreoScene *scene, EVENTTYPE type, const char *name )
{
	Init( scene );
	SetType( type );
	SetName( name );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//			*name - 
//			*param - 
//-----------------------------------------------------------------------------
CChoreoEvent::CChoreoEvent( CChoreoScene *scene, EVENTTYPE type, const char *name, const char *param )
{
	Init( scene );
	SetType( type );
	SetName( name );
	SetParameters( param );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CChoreoEvent::~CChoreoEvent( void )
{
	RemoveAllTracks();
	ClearEventDependencies();
	delete m_pSubScene;
}

//-----------------------------------------------------------------------------
// Purpose: Assignment
// Input  : src - 
// Output : CChoreoEvent&
//-----------------------------------------------------------------------------
CChoreoEvent& CChoreoEvent::operator=( const CChoreoEvent& src )
{
	MEM_ALLOC_CREDIT();

	// Copy global id when copying entity
	m_nGlobalID = src.m_nGlobalID;

	m_pActor = NULL;
	m_pChannel = NULL;

	m_nDefaultCurveType = src.m_nDefaultCurveType;
	m_fType = src.m_fType;
	m_Name = src.m_Name;
	m_Parameters = src.m_Parameters;
	m_Parameters2= src.m_Parameters2;
	m_Parameters3= src.m_Parameters3;
	m_flStartTime = src.m_flStartTime;
	m_flEndTime = src.m_flEndTime;

	m_bFixedLength = src.m_bFixedLength;
	m_flGestureSequenceDuration = src.m_flGestureSequenceDuration;
	m_bResumeCondition = src.m_bResumeCondition;
	m_bLockBodyFacing = src.m_bLockBodyFacing;
	m_flDistanceToTarget = src.m_flDistanceToTarget;
	m_bForceShortMovement = src.m_bForceShortMovement;
	m_bSyncToFollowingGesture = src.m_bSyncToFollowingGesture;
	m_bPlayOverScript = src.m_bPlayOverScript;
	m_bUsesTag = src.m_bUsesTag;
	m_TagName = src.m_TagName;
	m_TagWavName = src.m_TagWavName;

	ClearAllRelativeTags();
	ClearAllTimingTags();
	int t;
	for ( t = 0; t < NUM_ABS_TAG_TYPES; t++ )
	{
		ClearAllAbsoluteTags( (AbsTagType)t );
	}

	int i;
	for ( i = 0; i < src.m_RelativeTags.Size(); i++ )
	{	
		CEventRelativeTag newtag( src.m_RelativeTags[ i ] );
		newtag.SetOwner( this );
		m_RelativeTags.AddToTail( newtag );
	}

	for ( i = 0; i < src.m_TimingTags.Size(); i++ )
	{	
		CFlexTimingTag newtag( src.m_TimingTags[ i ] );
		newtag.SetOwner( this );
		m_TimingTags.AddToTail( newtag );
	}
	for ( t = 0; t < NUM_ABS_TAG_TYPES; t++ )
	{
		for ( i = 0; i < src.m_AbsoluteTags[ t ].Size(); i++ )
		{
			CEventAbsoluteTag newtag( src.m_AbsoluteTags[ t ][ i ] );
			newtag.SetOwner( this );
			m_AbsoluteTags[ t ].AddToTail( newtag );
		}
	}

	RemoveAllTracks();

	for ( i = 0 ; i < src.m_FlexAnimationTracks.Size(); i++ )
	{
		CFlexAnimationTrack *newtrack = new CFlexAnimationTrack( src.m_FlexAnimationTracks[ i ] );
		newtrack->SetEvent( this );
		m_FlexAnimationTracks.AddToTail( newtrack );
	}

	m_bTrackLookupSet = src.m_bTrackLookupSet;

	// FIXME:  Use a safe handle?
	//m_pSubScene = src.m_pSubScene;

	m_bProcessing = src.m_bProcessing;
	m_pMixer = src.m_pMixer;

	m_pScene = src.m_pScene;

	m_nPitch = src.m_nPitch;
	m_nYaw = src.m_nYaw;

	m_nNumLoops = src.m_nNumLoops;
	m_nLoopsRemaining = src.m_nLoopsRemaining;

	// Copy ramp over
	m_Ramp = src.m_Ramp;

	m_ccType = src.m_ccType;
	m_CCToken = src.m_CCToken;
	m_bUsingCombinedSoundFile = src.m_bUsingCombinedSoundFile;
	m_uRequiredCombinedChecksum = src.m_uRequiredCombinedChecksum; 
	m_nNumSlaves = src.m_nNumSlaves;
	m_flLastSlaveEndTime = src.m_flLastSlaveEndTime;	
	m_bCCTokenValid = src.m_bCCTokenValid;   
	m_bCombinedUsingGenderToken = src.m_bCombinedUsingGenderToken;

	m_bSuppressCaptionAttenuation = src.m_bSuppressCaptionAttenuation;

	m_bActive = src.m_bActive;

	return *this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEvent::Init( CChoreoScene *scene )
{
	m_nGlobalID			= s_nGlobalID++;
	m_nDefaultCurveType	= CURVE_CATMULL_ROM_TO_CATMULL_ROM;
	m_fType				= UNSPECIFIED;
	m_Name.Set("");
	m_Parameters.Set("");
	m_Parameters2.Set("");
	m_Parameters3.Set("");

	m_flStartTime		= 0.0f;
	m_flEndTime			= -1.0f;

	m_pActor			= NULL;
	m_pChannel			= NULL;
	m_pScene			= scene;

	m_bFixedLength		= false;
	m_bResumeCondition	= false;
	SetUsingRelativeTag( false, 0, 0 );

	m_bTrackLookupSet	= false;

	m_bLockBodyFacing	= false;
	m_flDistanceToTarget = 0.0f;
	m_bForceShortMovement = false;
	m_bSyncToFollowingGesture = false;
	m_bPlayOverScript = false;

	m_pSubScene			= NULL;
	m_bProcessing		= false;
	m_pMixer  			= NULL;
	m_flGestureSequenceDuration = 0.0f;

	m_nPitch = m_nYaw = 0;

	m_nNumLoops = -1;
	m_nLoopsRemaining = 0;

	// Close captioning/localization support
	m_CCToken.Set("");
	m_ccType					= CC_MASTER;
	m_bUsingCombinedSoundFile	= false;
	m_uRequiredCombinedChecksum = 0; 
	m_nNumSlaves				= 0;
	m_flLastSlaveEndTime		= 0.0f;	
	m_bCCTokenValid				= false;  
	m_bCombinedUsingGenderToken = false;
	m_bSuppressCaptionAttenuation = false;
	m_bActive					= true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
CChoreoEvent::EVENTTYPE CChoreoEvent::GetType( void )
{
	return (EVENTTYPE)m_fType;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetType( EVENTTYPE type )
{
	m_fType = type;

	if ( m_fType == SPEAK ||
		m_fType == SUBSCENE )
	{
		m_bFixedLength = true;
	}
	else
	{
		m_bFixedLength = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetName( const char *name )
{
	m_Name = name;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoEvent::GetName( void )
{
	return m_Name.Get();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *param - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetParameters( const char *param )
{
	m_Parameters = param;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoEvent::GetParameters( void )
{
	return m_Parameters.Get();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *param - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetParameters2( const char *param )
{
	int iLength = Q_strlen( param );
	m_Parameters2 = param;

	// HACK: Remove trailing " " until faceposer is fixed
	if ( iLength > 0 )
	{
		if ( param[iLength-1] == ' ' )
		{
			char tmp[1024];
			Q_strncpy( tmp, param, sizeof(tmp) );
			tmp[iLength-1] = 0;
			m_Parameters2.Set(tmp);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoEvent::GetParameters2( void )
{
	return m_Parameters2.Get();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *param - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetParameters3( const char *param )
{
	int iLength = Q_strlen( param );
	m_Parameters3 = param;

	// HACK: Remove trailing " " until faceposer is fixed
	if ( iLength > 0 )
	{
		if ( param[iLength-1] == ' ' )
		{
			char tmp[1024];
			Q_strncpy( tmp, param, sizeof(tmp) );
			tmp[iLength-1] = 0;
			m_Parameters3.Set(tmp);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoEvent::GetParameters3( void )
{
	return m_Parameters3.Get();
}

//-----------------------------------------------------------------------------
// Purpose: debugging description
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoEvent::GetDescription( void )
{
	static char description[ 256 ];

	description[ 0 ] = 0;

	if ( !GetActor() )
	{
		Q_snprintf( description,sizeof(description), "global %s", m_Name.Get() );
	}
	else
	{
		Assert( m_pChannel );
		Q_snprintf( description,sizeof(description), "%s : %s : %s -- %s \"%s\"", m_pActor->GetName(), m_pChannel->GetName(), GetName(), NameForType( GetType() ), GetParameters() );
		if ( GetType() == EXPRESSION )
		{
			char sz[ 256 ];

			Q_snprintf( sz,sizeof(sz), " \"%s\"", GetParameters2() );
			Q_strncat( description, sz, sizeof(description), COPY_ALL_CHARACTERS );
		}
	}

	return description;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : starttime - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetStartTime( float starttime )
{
	m_flStartTime = starttime;
	if ( m_flEndTime != -1.0f )
	{
		if ( m_flEndTime < m_flStartTime )
		{
			m_flEndTime = m_flStartTime;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoEvent::GetStartTime( )
{
	return m_flStartTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : endtime - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetEndTime( float endtime  )
{
	bool changed = m_flEndTime != endtime;

	m_flEndTime = endtime;

	if ( endtime != -1.0f )
	{
		if ( m_flEndTime < m_flStartTime )
		{
			m_flEndTime = m_flStartTime;
		}

		if ( changed )
		{
			OnEndTimeChanged();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoEvent::GetEndTime( )
{
	return m_flEndTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoEvent::HasEndTime( void )
{
	return m_flEndTime != -1.0f ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoEvent::GetCompletion( float time )
{
	float t = (time - GetStartTime()) / (GetEndTime() - GetStartTime());

	if (t < 0.0f)
		return 0.0f;
	else if (t > 1.0f)
		return 1.0f;
	
	return t;
}

// ICurveDataAccessor method
bool CChoreoEvent::CurveHasEndTime()
{
	return HasEndTime();
}

//-----------------------------------------------------------------------------
// Default curve type
//-----------------------------------------------------------------------------
void CChoreoEvent::SetDefaultCurveType( int nCurveType )
{
	m_nDefaultCurveType = nCurveType;
}

int CChoreoEvent::GetDefaultCurveType()
{
	return m_nDefaultCurveType;
}

float CCurveData::GetIntensity( ICurveDataAccessor *data, float time )
{
	float zeroValue = 0.0f;

	// find samples that span the time
	if ( !data->CurveHasEndTime() )
	{
		return zeroValue;
	}

	int rampCount = GetCount();
	if ( rampCount < 1 )
	{
		// Full intensity
		return 1.0f;
	}

	CExpressionSample *esStart = NULL;
	CExpressionSample *esEnd = NULL;

	// do binary search for sample in time period
	int j = MAX( rampCount / 2, 1 );
	int i = j;
	while ( i > -2 && i < rampCount + 1 )
	{
		bool dummy;
		esStart = GetBoundedSample( data, i, dummy );
		esEnd = GetBoundedSample( data, i + 1, dummy  );

		j = MAX( j / 2, 1 );
		if ( time < esStart->time)
		{
			i -= j;
		}
		else if ( time > esEnd->time)
		{
			i += j;
		}
		else
		{
			break;
		}
	}

	if (!esStart)
	{
		return 1.0f;
	}

	int prev = i - 1;
	int next = i + 2;

	prev = MAX( -1, prev );
	next = MIN( next, rampCount );

	bool bclamp[ 2 ];
	CExpressionSample *esPre = GetBoundedSample( data, prev, bclamp[ 0 ] );
	CExpressionSample *esNext = GetBoundedSample( data, next, bclamp[ 1 ] );

	float dt = esEnd->time - esStart->time;

	Vector vPre( esPre->time, esPre->value, 0 );
	Vector vStart( esStart->time, esStart->value, 0 );
	Vector vEnd( esEnd->time, esEnd->value, 0 );
	Vector vNext( esNext->time, esNext->value, 0 );

	if ( bclamp[ 0 ] )
	{
		vPre.x = vStart.x;
	}

	if ( bclamp[ 1 ] )
	{
		vNext.x = vEnd.x;
	}

	float f2 = 0.0f;
	if ( dt > 0.0f )
	{
		f2 = ( time - esStart->time ) / ( dt );
	}
	f2 = clamp( f2, 0.0f, 1.0f );

	Vector vOut;
	int dummy;
	int earlypart, laterpart;

	int startCurve	= esStart->GetCurveType();
	int endCurve	= esEnd->GetCurveType();

	if ( startCurve == CURVE_DEFAULT )
	{
		startCurve = data->GetDefaultCurveType();
	}
	if ( endCurve == CURVE_DEFAULT )
	{
		endCurve = data->GetDefaultCurveType();
	}

	// Not holding out value of previous curve...
	Interpolator_CurveInterpolatorsForType( startCurve, dummy, earlypart );
	Interpolator_CurveInterpolatorsForType( endCurve, laterpart, dummy );

	if ( earlypart == INTERPOLATE_HOLD )
	{
		// Hold "out" of previous sample (can cause a discontinuity)
		VectorLerp( vStart, vEnd, f2, vOut );
		vOut.y = vStart.y;
	}
	else if ( laterpart == INTERPOLATE_HOLD )
	{
		// Hold "out" of previous sample (can cause a discontinuity)
		VectorLerp( vStart, vEnd, f2, vOut );
		vOut.y = vEnd.y;
	}
	else
	{
		bool sameCurveType = earlypart == laterpart ? true : false;
		if ( sameCurveType )
		{
			Interpolator_CurveInterpolate( laterpart, vPre, vStart, vEnd, vNext, f2, vOut );
		}
		else // curves differ, sigh
		{
			Vector vOut1, vOut2;

			Interpolator_CurveInterpolate( earlypart, vPre, vStart, vEnd, vNext, f2, vOut1 );
			Interpolator_CurveInterpolate( laterpart, vPre, vStart, vEnd, vNext, f2, vOut2 );

			VectorLerp( vOut1, vOut2, f2, vOut );
		}
	}

	float retval = clamp( vOut.y, 0.0f, 1.0f );
	return retval;
}

//-----------------------------------------------------------------------------
// Purpose: Get intensity for event, bounded by scene global intensity
// Output : float
//-----------------------------------------------------------------------------
float CChoreoEvent::GetIntensity( float scenetime )
{
	float global_intensity = 1.0f;
	if ( m_pScene )
	{
		global_intensity = m_pScene->GetSceneRampIntensity( scenetime );
	}
	else
	{
		Assert( 0 );
	}

	float event_intensity = _GetIntensity( scenetime );

	return global_intensity * event_intensity;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoEvent::_GetIntensity( float scenetime )
{
	// Convert to event local time
	float time = scenetime - GetStartTime();
	return m_Ramp.GetIntensity( this, time );
}


float CChoreoEvent::GetIntensityArea( float scenetime )
{
	// Convert to event local time
	float time = scenetime - GetStartTime();
	return m_Ramp.GetIntensityArea( this, time );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CCurveData::GetIntensityArea( ICurveDataAccessor *data, float time )
{
	float zeroValue = 0.0f;

	// find samples that span the time
	if ( !data->CurveHasEndTime() )
	{
		return zeroValue;
	}

	int rampCount = GetCount();
	if ( rampCount < 1 )
	{
		// Full intensity
		return 1.0f;
	}

	CExpressionSample *esStart = NULL;
	CExpressionSample *esEnd = NULL;

	// do binary search for sample in time period
	int j = MAX( rampCount / 2, 1 );
	int i = j;
	while ( i > -2 && i < rampCount + 1 )
	{
		bool dummy;
		esStart = GetBoundedSample( data, i, dummy );
		esEnd = GetBoundedSample( data, i + 1, dummy  );

		j = MAX( j / 2, 1 );
		if ( time < esStart->time)
		{
			i -= j;
		}
		else if ( time > esEnd->time)
		{
			i += j;
		}
		else
		{
			break;
		}
	}

	UpdateIntensityArea( data );

	float flTotal = 0.0f;
	flTotal = m_RampAccumulator[i+1];

	int prev = i - 1;
	int next = i + 2;

	prev = MAX( -1, prev );
	next = MIN( next, rampCount );

	bool bclamp[ 2 ];
	CExpressionSample *esPre = GetBoundedSample( data, prev, bclamp[ 0 ] );
	CExpressionSample *esNext = GetBoundedSample( data, next, bclamp[ 1 ] );

	float dt = esEnd->time - esStart->time;

	Vector vPre( esPre->time, esPre->value, 0 );
	Vector vStart( esStart->time, esStart->value, 0 );
	Vector vEnd( esEnd->time, esEnd->value, 0 );
	Vector vNext( esNext->time, esNext->value, 0 );

	if ( bclamp[ 0 ] )
	{
		vPre.x = vStart.x;
	}

	if ( bclamp[ 1 ] )
	{
		vNext.x = vEnd.x;
	}

	float f2 = 0.0f;
	if ( dt > 0.0f )
	{
		f2 = ( time - esStart->time ) / ( dt );
	}
	f2 = clamp( f2, 0.0f, 1.0f );

	Vector vOut;
	int dummy;
	int earlypart, laterpart;

	int startCurve	= esStart->GetCurveType();
	int endCurve	= esEnd->GetCurveType();

	if ( startCurve == CURVE_DEFAULT )
	{
		startCurve = data->GetDefaultCurveType();
	}
	if ( endCurve == CURVE_DEFAULT )
	{
		endCurve = data->GetDefaultCurveType();
	}

	// Not holding out value of previous curve...
	Interpolator_CurveInterpolatorsForType( startCurve, dummy, earlypart );
	Interpolator_CurveInterpolatorsForType( endCurve, laterpart, dummy );

	// FIXME: needs other curve types
	Catmull_Rom_Spline_Integral_Normalize( 
		vPre,
		vStart,
		vEnd,
		vNext,
		f2, 
		vOut );

	// Con_Printf( "Accum %f : Partial %f\n", flTotal, vOut.y * (vEnd.x - vStart.x) * f2 );
	flTotal = flTotal + clamp( vOut.y, 0.0f, 1.0f ) * (vEnd.x - vStart.x);
	return flTotal;
}


void CCurveData::UpdateIntensityArea( ICurveDataAccessor *data )
{
	int rampCount = GetCount();;
	if ( rampCount < 1 )
	{
		return;
	}

	if (m_RampAccumulator.Count() == rampCount + 2)
	{
		return;
	}

	m_RampAccumulator.SetCount( rampCount + 2 );

	int i = -1;

	bool dummy;
	CExpressionSample *esPre = GetBoundedSample( data, i - 1, dummy );
	CExpressionSample *esStart = GetBoundedSample( data, i, dummy );
	CExpressionSample *esEnd = GetBoundedSample( data, MIN( i + 1, rampCount ), dummy );

	Vector vPre( esPre->time, esPre->value, 0 );
	Vector vStart( esStart->time, esStart->value, 0 );
	Vector vEnd( esEnd->time, esEnd->value, 0 );

	Vector vOut;
	for (i = -1; i < rampCount; i++)
	{
		CExpressionSample *esNext = GetBoundedSample( data, MIN( i + 2, rampCount ), dummy );
		Vector vNext( esNext->time, esNext->value, 0 );

		Catmull_Rom_Spline_Integral_Normalize( 
			vPre,
			vStart,
			vEnd,
			vNext,
			1.0f, 
			vOut );

		m_RampAccumulator[i+1] = clamp( vOut.y, 0.0f, 1.0f ) * (vEnd.x - vStart.x);

		vPre = vStart;
		vStart = vEnd;
		vEnd = vNext;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dt - 
//-----------------------------------------------------------------------------
void CChoreoEvent::OffsetStartTime( float dt )
{
	SetStartTime( GetStartTime() + dt );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dt - 
//-----------------------------------------------------------------------------
void CChoreoEvent::OffsetEndTime( float dt )
{
	if ( HasEndTime() )
	{
		SetEndTime( GetEndTime() + dt );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dt - 
//-----------------------------------------------------------------------------
void CChoreoEvent::OffsetTime( float dt )
{
	if ( HasEndTime() )
	{
		m_flEndTime += dt;
	}
	m_flStartTime += dt;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetActor( CChoreoActor *actor )
{
	m_pActor = actor;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoActor
//-----------------------------------------------------------------------------
CChoreoActor *CChoreoEvent::GetActor( void )
{
	return m_pActor;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *channel - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetChannel( CChoreoChannel *channel )
{
	m_pChannel = channel;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoChannel
//-----------------------------------------------------------------------------
CChoreoChannel *CChoreoEvent::GetChannel( void )
{
	return m_pChannel;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetSubScene( CChoreoScene *scene )
{
	m_pSubScene = scene;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoScene
//-----------------------------------------------------------------------------
CChoreoScene *CChoreoEvent::GetSubScene( void )
{
	return m_pSubScene;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct EventNameMap_t
{
	CChoreoEvent::EVENTTYPE type;
	char const				*name;
};

static EventNameMap_t g_NameMap[] =
{
	{ CChoreoEvent::UNSPECIFIED,		"unspecified" },  // error condition!!!
	{ CChoreoEvent::SECTION,			"section" },
	{ CChoreoEvent::EXPRESSION,			"expression" },
	{ CChoreoEvent::LOOKAT,				"lookat" },
	{ CChoreoEvent::MOVETO,				"moveto" },
	{ CChoreoEvent::SPEAK,				"speak" },
	{ CChoreoEvent::GESTURE,			"gesture" },
	{ CChoreoEvent::SEQUENCE,			"sequence" },
	{ CChoreoEvent::FACE,				"face" },
	{ CChoreoEvent::FIRETRIGGER,		"firetrigger" },
	{ CChoreoEvent::FLEXANIMATION,		"flexanimation" },
	{ CChoreoEvent::SUBSCENE,			"subscene" },
	{ CChoreoEvent::LOOP,				"loop" },
	{ CChoreoEvent::INTERRUPT,			"interrupt" },
	{ CChoreoEvent::STOPPOINT,			"stoppoint" },
	{ CChoreoEvent::PERMIT_RESPONSES,	"permitresponses" },
	{ CChoreoEvent::GENERIC,			"generic" },
};

//-----------------------------------------------------------------------------
// Purpose: A simple class to verify the names data above at runtime
//-----------------------------------------------------------------------------
class CCheckEventNames
{
public:
	CCheckEventNames()
	{
		if ( ARRAYSIZE( g_NameMap ) != CChoreoEvent::NUM_TYPES )
		{
			Error( "g_NameMap contains %llu entries, CChoreoEvent::NUM_TYPES == %i!",
				(uint64)(ARRAYSIZE( g_NameMap )), CChoreoEvent::NUM_TYPES );
		}
		for ( int i = 0; i < CChoreoEvent::NUM_TYPES; ++i )
		{
			if ( !g_NameMap[ i ].name )
			{
				Error( "g_NameMap:  Event type at %i has NULL name string!", i ); 
			}

			if ( (CChoreoEvent::EVENTTYPE)(i) == g_NameMap[ i ].type )
				continue;

			Error( "g_NameMap:  Event type at %i has wrong value (%i)!",
				i, (int)g_NameMap[ i ].type ); 
		}
	}
};
static CCheckEventNames g_CheckNamesSingleton;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
CChoreoEvent::EVENTTYPE CChoreoEvent::TypeForName( const char *name )
{
	for ( int i = 0; i < NUM_TYPES; ++i )
	{
		EventNameMap_t *slot = &g_NameMap[ i ];
		if ( !Q_stricmp( name, slot->name ) )
			return slot->type;
	}
	
	Assert( !"CChoreoEvent::TypeForName failed!!!" );
	return UNSPECIFIED;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoEvent::NameForType( EVENTTYPE type )
{
	int i = (int)type;
	if ( i < 0 || i >= NUM_TYPES )
	{
		Assert( "!CChoreoEvent::NameForType:  bogus type!" );
		// returns "unspecified!!!";
		return g_NameMap[ 0 ].name;
	}

	return g_NameMap[ i ].name;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct CCNameMap_t
{
	CChoreoEvent::CLOSECAPTION type;
	char const				*name;
};

static CCNameMap_t g_CCNameMap[] =
{
	{ CChoreoEvent::CC_MASTER,			"cc_master" },  // error condition!!!
	{ CChoreoEvent::CC_SLAVE,			"cc_slave" },
	{ CChoreoEvent::CC_DISABLED,		"cc_disabled" },
};

//-----------------------------------------------------------------------------
// Purpose: A simple class to verify the names data above at runtime
//-----------------------------------------------------------------------------
class CCheckCCNames
{
public:
	CCheckCCNames()
	{
		if ( ARRAYSIZE( g_CCNameMap ) != CChoreoEvent::NUM_CC_TYPES )
		{
			Error( "g_CCNameMap contains %llu entries, CChoreoEvent::NUM_CC_TYPES == %i!",
				(uint64)(ARRAYSIZE( g_CCNameMap )), CChoreoEvent::NUM_CC_TYPES );
		}
		for ( int i = 0; i < CChoreoEvent::NUM_CC_TYPES; ++i )
		{
			if ( !g_CCNameMap[ i ].name )
			{
				Error( "g_NameMap:  CC type at %i has NULL name string!", i ); 
			}

			if ( (CChoreoEvent::CLOSECAPTION)(i) == g_CCNameMap[ i ].type )
				continue;

			Error( "g_CCNameMap:  Event type at %i has wrong value (%i)!",
				i, (int)g_CCNameMap[ i ].type ); 
		}
	}
};
static CCheckCCNames g_CheckCCNamesSingleton;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : CLOSECAPTION
//-----------------------------------------------------------------------------
CChoreoEvent::CLOSECAPTION CChoreoEvent::CCTypeForName( const char *name )
{
	for ( int i = 0; i < NUM_CC_TYPES; ++i )
	{
		CCNameMap_t *slot = &g_CCNameMap[ i ];
		if ( !Q_stricmp( name, slot->name ) )
			return slot->type;
	}
	
	Assert( !"CChoreoEvent::TypeForName failed!!!" );
	return CC_MASTER;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoEvent::NameForCCType( CLOSECAPTION type )
{
	int i = (int)type;
	if ( i < 0 || i >= NUM_CC_TYPES )
	{
		Assert( "!CChoreoEvent::NameForType:  bogus type!" );
		// returns "unspecified!!!";
		return g_CCNameMap[ 0 ].name;
	}

	return g_CCNameMap[ i ].name;
}

//-----------------------------------------------------------------------------
// Purpose: Is the event something that can be sized ( a wave file, e.g. )
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoEvent::IsFixedLength( void )
{
	return m_bFixedLength;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : isfixedlength - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetFixedLength( bool isfixedlength )
{
	m_bFixedLength = isfixedlength;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : resumecondition - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetResumeCondition( bool resumecondition )
{
	m_bResumeCondition = resumecondition;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoEvent::IsResumeCondition( void )
{
	return m_bResumeCondition;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : lockbodyfacing - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetLockBodyFacing( bool lockbodyfacing )
{
	m_bLockBodyFacing = lockbodyfacing;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoEvent::IsLockBodyFacing( void )
{
	return m_bLockBodyFacing;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : distancetotarget - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetDistanceToTarget( float distancetotarget )
{
	m_flDistanceToTarget = distancetotarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns ideal distance to target
//-----------------------------------------------------------------------------
float CChoreoEvent::GetDistanceToTarget( void )
{
	return m_flDistanceToTarget;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : set if small (sub-1/2 bbox) movements are forced
//-----------------------------------------------------------------------------

void CChoreoEvent::SetForceShortMovement( bool bForceShortMovement )
{
	m_bForceShortMovement = bForceShortMovement;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output  : get if small (sub-1/2 bbox) movements are forced
//-----------------------------------------------------------------------------

bool CChoreoEvent::GetForceShortMovement( void )
{
	return m_bForceShortMovement;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : set if the gesture should sync its exit tag with the following gestures entry tag
//-----------------------------------------------------------------------------

void CChoreoEvent::SetSyncToFollowingGesture( bool bSyncToFollowingGesture )
{
	m_bSyncToFollowingGesture = bSyncToFollowingGesture;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output  : get if the gesture should sync its exit tag with the following gestures entry tag
//-----------------------------------------------------------------------------

bool CChoreoEvent::GetSyncToFollowingGesture( void )
{
	return m_bSyncToFollowingGesture;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : set if the sequence should player overtop of an underlying SS
//-----------------------------------------------------------------------------

void CChoreoEvent::SetPlayOverScript( bool bPlayOverScript )
{
	m_bPlayOverScript = bPlayOverScript;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output  : get if the sequence should player overtop of an underlying SS
//-----------------------------------------------------------------------------

bool CChoreoEvent::GetPlayOverScript( void )
{
	return m_bPlayOverScript;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoEvent::GetDuration( void )
{
	if ( HasEndTime() )
	{
		return GetEndTime() - GetStartTime();
	}
	
	return 0.0f;
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEvent::ClearAllRelativeTags( void )
{
	m_RelativeTags.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoEvent::GetNumRelativeTags( void )
{
	return m_RelativeTags.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : tagnum - 
// Output : CEventRelativeTag
//-----------------------------------------------------------------------------
CEventRelativeTag *CChoreoEvent::GetRelativeTag( int tagnum )
{
	Assert( tagnum >= 0 && tagnum < m_RelativeTags.Size() );
	return &m_RelativeTags[ tagnum ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tagname - 
//			percentage - 
//-----------------------------------------------------------------------------
void CChoreoEvent::AddRelativeTag( const char *tagname, float percentage )
{
	CEventRelativeTag rt( this, tagname, percentage );
	m_RelativeTags.AddToTail( rt );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tagname - 
//-----------------------------------------------------------------------------
void CChoreoEvent::RemoveRelativeTag( const char *tagname )
{
	for ( int i = 0; i < m_RelativeTags.Size(); i++ )
	{
		CEventRelativeTag *prt = &m_RelativeTags[ i ];
		if ( !prt )
			continue;

		if ( !stricmp( prt->GetName(), tagname ) )
		{
			m_RelativeTags.Remove( i );
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tagname - 
// Output : CEventRelativeTag *
//-----------------------------------------------------------------------------
CEventRelativeTag * CChoreoEvent::FindRelativeTag( const char *tagname )
{
	for ( int i = 0; i < m_RelativeTags.Size(); i++ )
	{
		CEventRelativeTag *prt = &m_RelativeTags[ i ];
		if ( !prt )
			continue;

		if ( !stricmp( prt->GetName(), tagname ) )
		{
			return prt;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoEvent::IsUsingRelativeTag( void )
{
	return m_bUsesTag;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : usetag - 
//			0 - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetUsingRelativeTag( bool usetag, const char *tagname /*= 0*/, 
	const char *wavname /* = 0 */ )
{
	m_bUsesTag = usetag;
	if ( tagname )
	{
		m_TagName = tagname;
	}
	else
	{
		m_TagName.Set("");
	}
	if ( wavname )
	{
		m_TagWavName = wavname;
	}
	else
	{
		m_TagWavName.Set("");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoEvent::GetRelativeTagName( void )
{
	return m_TagName.Get();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoEvent::GetRelativeWavName( void )
{
	return m_TagWavName.Get();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEvent::ClearAllTimingTags( void )
{
	m_TimingTags.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoEvent::GetNumTimingTags( void )
{
	return m_TimingTags.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : tagnum - 
// Output : CEventRelativeTag
//-----------------------------------------------------------------------------
CFlexTimingTag *CChoreoEvent::GetTimingTag( int tagnum )
{
	Assert( tagnum >= 0 && tagnum < m_TimingTags.Size() );
	return &m_TimingTags[ tagnum ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tagname - 
//			percentage - 
//-----------------------------------------------------------------------------
void CChoreoEvent::AddTimingTag( const char *tagname, float percentage, bool locked )
{
	CFlexTimingTag tt( this, tagname, percentage, locked );
	m_TimingTags.AddToTail( tt );

	// Sort tags
	CFlexTimingTag temp( (CChoreoEvent *)0x1, "", 0.0f, false );

	// ugly bubble sort
	for ( int i = 0; i < m_TimingTags.Size(); i++ )
	{
		for ( int j = i + 1; j < m_TimingTags.Size(); j++ )
		{
			CFlexTimingTag *t1 = &m_TimingTags[ i ];
			CFlexTimingTag *t2 = &m_TimingTags[ j ];

			if ( t1->GetPercentage() > t2->GetPercentage() )
			{
				temp = *t1;
				*t1 = *t2;
				*t2 = temp;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tagname - 
//-----------------------------------------------------------------------------
void CChoreoEvent::RemoveTimingTag( const char *tagname )
{
	for ( int i = 0; i < m_TimingTags.Size(); i++ )
	{
		CFlexTimingTag *ptt = &m_TimingTags[ i ];
		if ( !ptt )
			continue;

		if ( !stricmp( ptt->GetName(), tagname ) )
		{
			m_TimingTags.Remove( i );
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tagname - 
// Output : CEventRelativeTag *
//-----------------------------------------------------------------------------
CFlexTimingTag * CChoreoEvent::FindTimingTag( const char *tagname )
{
	for ( int i = 0; i < m_TimingTags.Size(); i++ )
	{
		CFlexTimingTag *ptt = &m_TimingTags[ i ];
		if ( !ptt )
			continue;

		if ( !stricmp( ptt->GetName(), tagname ) )
		{
			return ptt;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEvent::OnEndTimeChanged( void )
{
	int c = GetNumFlexAnimationTracks();
	for ( int i = 0; i < c; i++ )
	{
		CFlexAnimationTrack *track = GetFlexAnimationTrack( i );
		Assert( track );
		if ( !track )
			continue;

		track->Resort( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoEvent::GetNumFlexAnimationTracks( void )
{
	return m_FlexAnimationTracks.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : CFlexAnimationTrack
//-----------------------------------------------------------------------------
CFlexAnimationTrack *CChoreoEvent::GetFlexAnimationTrack( int index )
{
	if ( index < 0 || index >= GetNumFlexAnimationTracks() )
		return NULL;
	return m_FlexAnimationTracks[ index ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *controllername - 
// Output : CFlexAnimationTrack
//-----------------------------------------------------------------------------
CFlexAnimationTrack *CChoreoEvent::AddTrack( const char *controllername )
{
	CFlexAnimationTrack *newTrack = new CFlexAnimationTrack( this );
	newTrack->SetFlexControllerName( controllername );

	m_FlexAnimationTracks.AddToTail( newTrack );

	return newTrack;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//-----------------------------------------------------------------------------
void CChoreoEvent::RemoveTrack( int index )
{	
	CFlexAnimationTrack *track = GetFlexAnimationTrack( index );
	if ( !track )
		return;

	m_FlexAnimationTracks.Remove( index );
	delete track;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEvent::RemoveAllTracks( void )
{
	while ( GetNumFlexAnimationTracks() > 0 )
	{
		RemoveTrack( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *controllername - 
// Output : CFlexAnimationTrack
//-----------------------------------------------------------------------------
CFlexAnimationTrack *CChoreoEvent::FindTrack( const char *controllername )
{
	for ( int i = 0; i < GetNumFlexAnimationTracks(); i++ )
	{
		CFlexAnimationTrack *t = GetFlexAnimationTrack( i );
		if ( t && !stricmp( t->GetFlexControllerName(), controllername ) )
		{
			return t;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoEvent::GetTrackLookupSet( void )
{
	return m_bTrackLookupSet;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : set - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetTrackLookupSet( bool set )
{
	m_bTrackLookupSet = set;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoEvent::IsProcessing( void ) const
{
	return m_bProcessing;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cb - 
//			t - 
//-----------------------------------------------------------------------------
void CChoreoEvent::StartProcessing( IChoreoEventCallback *cb, CChoreoScene *scene, float t )
{
	Assert( !m_bProcessing );
	m_bProcessing = true;
	if ( cb )
	{
		cb->StartEvent( t, scene, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cb - 
//			t - 
//-----------------------------------------------------------------------------
void CChoreoEvent::ContinueProcessing( IChoreoEventCallback *cb, CChoreoScene *scene, float t )
{
	Assert( m_bProcessing );
	if ( cb )
	{
		cb->ProcessEvent( t, scene, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cb - 
//			t - 
//-----------------------------------------------------------------------------
void CChoreoEvent::StopProcessing( IChoreoEventCallback *cb, CChoreoScene *scene, float t )
{
	Assert( m_bProcessing );
	if ( cb )
	{
		cb->EndEvent( t, scene, this );
	}
	m_bProcessing = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cb - 
//			t - 
//-----------------------------------------------------------------------------
bool CChoreoEvent::CheckProcessing( IChoreoEventCallback *cb, CChoreoScene *scene, float t )
{
	//Assert( !m_bProcessing );
	if ( cb )
	{
		return cb->CheckEvent( t, scene, this );
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEvent::ResetProcessing( void )
{
	if ( GetType() == LOOP )
	{
		m_nLoopsRemaining = m_nNumLoops;
	}

	m_bProcessing = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *mixer - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetMixer( CAudioMixer *mixer )
{
	m_pMixer = mixer;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CAudioMixer
//-----------------------------------------------------------------------------
CAudioMixer *CChoreoEvent::GetMixer( void ) const
{
	return m_pMixer;
}

// Snap to scene framerate
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEvent::SnapTimes()
{
	if ( HasEndTime() && !IsFixedLength() )
	{
		m_flEndTime = SnapTime( m_flEndTime );
	}
	float oldstart = m_flStartTime;
	m_flStartTime = SnapTime( m_flStartTime );

	// Don't snap end time for fixed length events, just set based on new start time
	if ( IsFixedLength() )
	{
		float dt = m_flStartTime - oldstart;
		m_flEndTime += dt;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoEvent::SnapTime( float t )
{
	CChoreoScene *scene = GetScene();
	if ( !scene)
	{
		Assert( 0 );
		return t;
	}

	return scene->SnapTime( t );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoScene
//-----------------------------------------------------------------------------
CChoreoScene *CChoreoEvent::GetScene( void )
{
	return m_pScene;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetScene( CChoreoScene *scene )
{
	m_pScene = scene;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
// Output : char const
//-----------------------------------------------------------------------------
const char *CChoreoEvent::NameForAbsoluteTagType( AbsTagType t )
{
	switch ( t )
	{
	case PLAYBACK:
		return "playback_time";
	case ORIGINAL:
		return "shifted_time";
	default:
		break;
	}

	return "AbsTagType(unknown)";
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : AbsTagType
//-----------------------------------------------------------------------------
CChoreoEvent::AbsTagType CChoreoEvent::TypeForAbsoluteTagName( const char *name )
{
	if ( !Q_strcasecmp( name, "playback_time" ) )
	{
		return PLAYBACK;
	}
	else if ( !Q_strcasecmp( name, "shifted_time" ) )
	{
		return ORIGINAL;
	}

	return (AbsTagType)-1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//-----------------------------------------------------------------------------
void CChoreoEvent::ClearAllAbsoluteTags( AbsTagType type )
{
	m_AbsoluteTags[ type ].Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoEvent::GetNumAbsoluteTags( AbsTagType type )
{
	return m_AbsoluteTags[ type ].Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//			tagnum - 
// Output : CEventAbsoluteTag
//-----------------------------------------------------------------------------
CEventAbsoluteTag *CChoreoEvent::GetAbsoluteTag( AbsTagType type, int tagnum )
{
	Assert( tagnum >= 0 && tagnum < m_AbsoluteTags[ type ].Size() );
	return &m_AbsoluteTags[ type ][ tagnum ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//			*tagname - 
// Output : CEventAbsoluteTag
//-----------------------------------------------------------------------------
CEventAbsoluteTag *CChoreoEvent::FindAbsoluteTag( AbsTagType type, const char *tagname )
{
	for ( int i = 0; i < m_AbsoluteTags[ type ].Size(); i++ )
	{
		CEventAbsoluteTag *ptag = &m_AbsoluteTags[ type ][ i ];
		if ( !ptag )
			continue;

		if ( !stricmp( ptag->GetName(), tagname ) )
		{
			return ptag;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//			*tagname - 
//			t - 
//-----------------------------------------------------------------------------
void CChoreoEvent::AddAbsoluteTag( AbsTagType type, const char *tagname, float t )
{
	CEventAbsoluteTag at( this, tagname, t );
	m_AbsoluteTags[ type ].AddToTail( at );

	// Sort tags
	CEventAbsoluteTag temp( (CChoreoEvent *)0x1, "", 0.0f );

	// ugly bubble sort
	for ( int i = 0; i < m_AbsoluteTags[ type ].Size(); i++ )
	{
		for ( int j = i + 1; j < m_AbsoluteTags[ type ].Size(); j++ )
		{
			CEventAbsoluteTag *t1 = &m_AbsoluteTags[ type ][ i ];
			CEventAbsoluteTag *t2 = &m_AbsoluteTags[ type ][ j ];

			if ( t1->GetPercentage() > t2->GetPercentage() )
			{
				temp = *t1;
				*t1 = *t2;
				*t2 = temp;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//			*tagname - 
//-----------------------------------------------------------------------------
void CChoreoEvent::RemoveAbsoluteTag( AbsTagType type, const char *tagname )
{
	for ( int i = 0; i < m_AbsoluteTags[ type ].Size(); i++ )
	{
		CEventAbsoluteTag *ptag = &m_AbsoluteTags[ type ][ i ];
		if ( !ptag )
			continue;

		if ( !stricmp( ptag->GetName(), tagname ) )
		{
			m_AbsoluteTags[ type ].Remove( i );
			return;
		}
	}
}



//-----------------------------------------------------------------------------
// Purpose: makes sure tags in PLAYBACK are in the same order as ORIGINAL
// Input  : 
// Output : true if they were in order, false if it has to reorder them
//-----------------------------------------------------------------------------
bool CChoreoEvent::VerifyTagOrder( )
{
	bool bInOrder = true;

	// Sort tags
	CEventAbsoluteTag temp( (CChoreoEvent *)0x1, "", 0.0f );

	for ( int i = 0; i < m_AbsoluteTags[ CChoreoEvent::ORIGINAL ].Size(); i++ )
	{
		CEventAbsoluteTag *ptag = &m_AbsoluteTags[ CChoreoEvent::ORIGINAL ][ i ];
		if ( !ptag )
			continue;

		CEventAbsoluteTag *t1 = &m_AbsoluteTags[ CChoreoEvent::PLAYBACK ][ i ];

		if ( stricmp( ptag->GetName(), t1->GetName() ) == 0)
			continue;

		bInOrder = false;
		for ( int j = i + 1; j < m_AbsoluteTags[ CChoreoEvent::PLAYBACK ].Size(); j++ )
		{
			CEventAbsoluteTag *t2 = &m_AbsoluteTags[ CChoreoEvent::PLAYBACK ][ j ];

			if ( stricmp( ptag->GetName(), t2->GetName() ) == 0 )
			{
				temp = *t1;
				*t1 = *t2;
				*t2 = temp;
				break;
			}
		}
	}
	return bInOrder;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//			*tagname - 
//-----------------------------------------------------------------------------

float CChoreoEvent::GetBoundedAbsoluteTagPercentage( AbsTagType type, int tagnum )
{
	if ( tagnum <= -2 )
	{
		/*
		if (GetNumAbsoluteTags( type ) >= 1)
		{
			CEventAbsoluteTag *tag = GetAbsoluteTag( type, 0 );
			Assert( tag );
			return -tag->GetTime();
		}
		*/
		return 0.0f; // -0.5f;
	}
	else if ( tagnum == -1 )
	{
		return 0.0f;
	}
	else if ( tagnum == GetNumAbsoluteTags( type ) )
	{
		return 1.0;
	}
	else if ( tagnum > GetNumAbsoluteTags( type ) )
	{
		/*
		if (GetNumAbsoluteTags( type ) >= 1)
		{
			CEventAbsoluteTag *tag = GetAbsoluteTag( type, tagnum - 2 );
			Assert( tag );
			return 2.0 - tag->GetTime();
		}
		*/
		return 1.0; // 1.5;
	}

	/*
	{
		float duration = GetDuration();
		
		if ( type == SHIFTED )
		{
			float seqduration;
			GetGestureSequenceDuration( seqduration );
			return seqduration;
		}
		return duration;
	}
	*/
	
	CEventAbsoluteTag *tag = GetAbsoluteTag( type, tagnum );
	Assert( tag );
	return tag->GetPercentage();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoEvent::GetOriginalPercentageFromPlaybackPercentage( float t )
{
	Assert( GetType() == GESTURE );
	if ( GetType() != GESTURE )
		return t;

	int count = GetNumAbsoluteTags( PLAYBACK );

	if ( count != GetNumAbsoluteTags( ORIGINAL ) )
	{
		return t;
	}

	if ( count <= 0 )
	{
		return t;
	}

	if ( t <= 0.0f )
		return 0.0f;

	float s = 0.0f, n = 0.0f;

	// find what tags this is between
	int i;
	for ( i = -1 ; i < count; i++ )
	{
		s = GetBoundedAbsoluteTagPercentage( PLAYBACK, i );
		n = GetBoundedAbsoluteTagPercentage( PLAYBACK, i + 1 );

		if ( t >= s && t <= n )
		{
			break;
		}
	}

	int prev = i - 1;
	int start = i;
	int end = i + 1;
	int next = i + 2;

	prev = MAX( -2, prev );
	start = MAX( -1, start );
	end = MIN( end, count );
	next = MIN( next, count + 1 );

	CEventAbsoluteTag *pStartTag = NULL;
	CEventAbsoluteTag *pEndTag = NULL;

	// check for linear portion of lookup
	if (start >= 0 && start < count)
	{
		pStartTag = GetAbsoluteTag( PLAYBACK, start );
	}
	if (end >= 0 && end < count)
	{
		pEndTag = GetAbsoluteTag( PLAYBACK, end );
	}

	if (pStartTag && pEndTag)
	{
		if (pStartTag->GetLinear() && pEndTag->GetLinear())
		{
			CEventAbsoluteTag *pOrigStartTag = GetAbsoluteTag( ORIGINAL, start );
			CEventAbsoluteTag *pOrigEndTag = GetAbsoluteTag( ORIGINAL, end );

			if (pOrigStartTag && pOrigEndTag)
			{
				s = ( t - pStartTag->GetPercentage() ) / (pEndTag->GetPercentage() - pStartTag->GetPercentage());
				return (1 - s) * pOrigStartTag->GetPercentage() + s * pOrigEndTag->GetPercentage();
			}
		}
	}

	float dt = n - s;

	Vector vPre( GetBoundedAbsoluteTagPercentage( PLAYBACK, prev ), GetBoundedAbsoluteTagPercentage( ORIGINAL, prev ), 0 );
	Vector vStart( GetBoundedAbsoluteTagPercentage( PLAYBACK, start ), GetBoundedAbsoluteTagPercentage( ORIGINAL, start ), 0 );
	Vector vEnd( GetBoundedAbsoluteTagPercentage( PLAYBACK, end ), GetBoundedAbsoluteTagPercentage( ORIGINAL, end ), 0 );
	Vector vNext( GetBoundedAbsoluteTagPercentage( PLAYBACK, next ), GetBoundedAbsoluteTagPercentage( ORIGINAL, next ), 0 );

	// simulate sections of either side of "linear" portion of ramp as linear slope
	if (pStartTag && pStartTag->GetLinear())
	{
		vPre.Init( vStart.x - (vEnd.x - vStart.x), vStart.y - (vEnd.y - vStart.y), 0 );
	}

	if (pEndTag && pEndTag->GetLinear())
	{
		vNext.Init( vEnd.x + (vEnd.x - vStart.x), vEnd.y + (vEnd.y - vStart.y), 0 );
	}


	float f2 = 0.0f;
	if ( dt > 0.0f )
	{
		f2 = ( t - s ) / ( dt );
	}
	f2 = clamp( f2, 0.0f, 1.0f );

	Vector vOut;
	Catmull_Rom_Spline_NormalizeX( 
		vPre,
		vStart,
		vEnd,
		vNext,
		f2, 
		vOut );

	return vOut.y;

	/*
	float duration;
	GetGestureSequenceDuration( duration );

	float retval = clamp( vOut.y, 0.0f, duration );
	return retval;
	*/
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoEvent::GetPlaybackPercentageFromOriginalPercentage( float t )
{
	Assert( GetType() == GESTURE );
	if ( GetType() != GESTURE )
		return t;

	int count = GetNumAbsoluteTags( PLAYBACK );

	if ( count != GetNumAbsoluteTags( ORIGINAL ) )
	{
		return t;
	}

	if ( count <= 0 )
	{
		return t;
	}

	if ( t <= 0.0f )
		return 0.0f;

	float s = 0.0f, n = 0.0f;

	// find what tags this is between
	int i;
	for ( i = -1 ; i < count; i++ )
	{
		s = GetBoundedAbsoluteTagPercentage( PLAYBACK, i );
		n = GetBoundedAbsoluteTagPercentage( PLAYBACK, i + 1 );

		if ( t >= s && t <= n )
		{
			break;
		}
	}

	int prev = i - 1;
	int start = i;
	int end = i + 1;
	int next = i + 2;

	prev = MAX( -2, prev );
	start = MAX( -1, start );
	end = MIN( end, count );
	next = MIN( next, count + 1 );

	CEventAbsoluteTag *pStartTag = NULL;
	CEventAbsoluteTag *pEndTag = NULL;

	// check for linear portion of lookup
	if (start >= 0 && start < count)
	{
		pStartTag = GetAbsoluteTag( ORIGINAL, start );
	}
	if (end >= 0 && end < count)
	{
		pEndTag = GetAbsoluteTag( ORIGINAL, end );
	}

	// check for linear portion of lookup
	if (pStartTag && pEndTag)
	{
		if (pStartTag->GetLinear() && pEndTag->GetLinear())
		{
			CEventAbsoluteTag *pPlaybackStartTag = GetAbsoluteTag( PLAYBACK, start );
			CEventAbsoluteTag *pPlaybackEndTag = GetAbsoluteTag( PLAYBACK, end );

			if (pPlaybackStartTag && pPlaybackEndTag)
			{
				s = ( t - pStartTag->GetPercentage() ) / (pEndTag->GetPercentage() - pStartTag->GetPercentage());
				return (1 - s) * pPlaybackStartTag->GetPercentage() + s * pPlaybackEndTag->GetPercentage();
			}
		}
	}

	float dt = n - s;

	Vector vPre( GetBoundedAbsoluteTagPercentage( ORIGINAL, prev ), GetBoundedAbsoluteTagPercentage( PLAYBACK, prev ), 0 );
	Vector vStart( GetBoundedAbsoluteTagPercentage( ORIGINAL, start ), GetBoundedAbsoluteTagPercentage( PLAYBACK, start ), 0 );
	Vector vEnd( GetBoundedAbsoluteTagPercentage( ORIGINAL, end ), GetBoundedAbsoluteTagPercentage( PLAYBACK, end ), 0 );
	Vector vNext( GetBoundedAbsoluteTagPercentage( ORIGINAL, next ), GetBoundedAbsoluteTagPercentage( PLAYBACK, next ), 0 );

	// simulate sections of either side of "linear" portion of ramp as linear slope
	if (pStartTag && pStartTag->GetLinear())
	{
		vPre.Init( vStart.x - (vEnd.x - vStart.x), vStart.y - (vEnd.y - vStart.y), 0 );
	}

	if (pEndTag && pEndTag->GetLinear())
	{
		vNext.Init( vEnd.x + (vEnd.x - vStart.x), vEnd.y + (vEnd.y - vStart.y), 0 );
	}

	float f2 = 0.0f;
	if ( dt > 0.0f )
	{
		f2 = ( t - s ) / ( dt );
	}
	f2 = clamp( f2, 0.0f, 1.0f );

	Vector vOut;
	Catmull_Rom_Spline_NormalizeX( 
		vPre,
		vStart,
		vEnd,
		vNext,
		f2, 
		vOut );

	return vOut.y;

	/*
	float duration;
	GetGestureSequenceDuration( duration );

	float retval = clamp( vOut.y, 0.0f, duration );
	return retval;
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : duration - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetGestureSequenceDuration( float duration )
{
	m_flGestureSequenceDuration = duration;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : duration - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoEvent::GetGestureSequenceDuration( float& duration )
{
	bool valid = m_flGestureSequenceDuration != 0.0f;

	if ( !valid )
	{
		duration = GetDuration();
	}
	else
	{
		duration = m_flGestureSequenceDuration;
	}

	return valid;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pitch - 
//-----------------------------------------------------------------------------
int CChoreoEvent::GetPitch( void ) const
{
	return m_nPitch;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pitch - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetPitch( int pitch )
{
	m_nPitch	= pitch;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : yaw - 
//-----------------------------------------------------------------------------
int CChoreoEvent::GetYaw( void ) const
{
	return m_nYaw;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : yaw - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetYaw( int yaw )
{
	m_nYaw		= yaw;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
//			-1 - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetLoopCount( int numloops )
{
	Assert( GetType() == LOOP );
	// Never below -1
	m_nNumLoops = MAX( numloops, -1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoEvent::GetNumLoopsRemaining( void )
{
	Assert( GetType() == LOOP );

	return m_nLoopsRemaining;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : loops - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetNumLoopsRemaining( int loops )
{
	Assert( GetType() == LOOP );

	m_nLoopsRemaining = loops;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int	
//-----------------------------------------------------------------------------
int CChoreoEvent::GetLoopCount( void )
{
	Assert( GetType() == LOOP );
	return m_nNumLoops;
}

EdgeInfo_t *CCurveData::GetEdgeInfo( int idx )
{
	return &m_RampEdgeInfo[ idx ];
}

int	 CCurveData::GetCount( void )
{
	return m_Ramp.Count();
}

CExpressionSample *CCurveData::Get( int index )
{
	if ( index < 0 || index >= GetCount() )
		return NULL;

	return &m_Ramp[ index ];
}

CExpressionSample *CCurveData::Add( float time, float value, bool selected )
{
	CExpressionSample sample;

	sample.time = time;
	sample.value = value;
	sample.selected = selected;

	int idx = m_Ramp.AddToTail( sample );
	return &m_Ramp[ idx ];
}

void CCurveData::Delete( int index )
{
	if ( index < 0 || index >= GetCount() )
		return;

	m_Ramp.Remove( index );
}

void CCurveData::Clear( void )
{
	m_Ramp.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCurveData::Resort( ICurveDataAccessor *data )
{
	for ( int i = 0; i < m_Ramp.Size(); i++ )
	{
		for ( int j = i + 1; j < m_Ramp.Size(); j++ )
		{
			CExpressionSample src = m_Ramp[ i ];
			CExpressionSample dest = m_Ramp[ j ];

			if ( src.time > dest.time )
			{
				m_Ramp[ i ] = dest;
				m_Ramp[ j ] = src;
			}
		}
	}

	RemoveOutOfRangeSamples( data );

	// m_RampAccumulator.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : number - 
// Output : CExpressionSample
//-----------------------------------------------------------------------------
CExpressionSample *CCurveData::GetBoundedSample( ICurveDataAccessor *data, int number, bool& bClamped )
{
	// Search for two samples which span time f
	if ( number < 0 )
	{
		static CExpressionSample nullstart;
		nullstart.time = 0.0f;
		nullstart.value = GetEdgeZeroValue( true );
		nullstart.SetCurveType( GetEdgeCurveType( true ) );
		bClamped = true;
		return &nullstart;
	}
	else if ( number >= GetCount() )
	{
		static CExpressionSample nullend;
		nullend.time = data->GetDuration();
		nullend.value = GetEdgeZeroValue( false );
		nullend.SetCurveType( GetEdgeCurveType( false ) );
		bClamped = true;
		return &nullend;
	}
	
	bClamped = false;
	return Get( number );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCurveData::RemoveOutOfRangeSamples( ICurveDataAccessor *data )
{
	float duration = data->GetDuration();

	int c = GetCount();
	for ( int i = c-1; i >= 0; i-- )
	{
		CExpressionSample src = m_Ramp[ i ];
		if ( src.time < 0 ||
			 src.time > duration + 0.01 )
		{
			m_Ramp.Remove( i );
		}
	}
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEvent::RescaleGestureTimes( float newstart, float newend, bool bMaintainAbsoluteTagPositions )
{
	if ( GetType() != CChoreoEvent::GESTURE )
		return;

	// Did it actually change
	if ( newstart == GetStartTime() &&
		 newend == GetEndTime() )
	{
		 return;
	}

	float newduration = newend - newstart;

	float dt = 0.0f;
	//If the end is moving, leave tags stay where they are (dt == 0.0f)
	if ( newstart != GetStartTime() )
	{
		// Otherwise, if the new start is later, then tags need to be shifted backwards
		dt -= ( newstart - GetStartTime() );
	}

	if ( bMaintainAbsoluteTagPositions )
	{
		int i;
		int count = GetNumAbsoluteTags( CChoreoEvent::PLAYBACK );
		for ( i = 0; i < count; i++ )
		{
			CEventAbsoluteTag *tag = GetAbsoluteTag( CChoreoEvent::PLAYBACK, i );
			float tagtime = tag->GetPercentage() * GetDuration();
	
			tagtime += dt;
	
			tagtime = clamp( tagtime / newduration, 0.0f, 1.0f );
	
			tag->SetPercentage( tagtime );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Make sure tags aren't co-located or out of order
//-----------------------------------------------------------------------------
bool CChoreoEvent::PreventTagOverlap( void )
{
	bool bHadOverlap  = false;

	// FIXME: limit to single frame?
	float minDp = 0.01;

	float minP = 1.00;

	int count = GetNumAbsoluteTags( CChoreoEvent::PLAYBACK );
	for ( int i = count - 1; i >= 0; i-- )
	{
		CEventAbsoluteTag *tag = GetAbsoluteTag( CChoreoEvent::PLAYBACK, i );
		
		if (tag->GetPercentage() > minP)
		{
			tag->SetPercentage( minP );

			minDp = MIN( 0.01, minP / (i + 1) );
			bHadOverlap = true;
		}
		else
		{
			minP = tag->GetPercentage();
		}
		minP = MAX( minP - minDp, 0 );
	}

	return bHadOverlap;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
// Output : CEventAbsoluteTag
//-----------------------------------------------------------------------------
CEventAbsoluteTag *CChoreoEvent::FindEntryTag( AbsTagType type )
{
	for ( int i = 0; i < m_AbsoluteTags[ type ].Size(); i++ )
	{
		CEventAbsoluteTag *ptag = &m_AbsoluteTags[ type ][ i ];
		if ( !ptag )
			continue;

		if ( ptag->GetEntry() )
		{
			return ptag;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
// Output : CEventAbsoluteTag
//-----------------------------------------------------------------------------
CEventAbsoluteTag *CChoreoEvent::FindExitTag( AbsTagType type )
{
	for ( int i = 0; i < m_AbsoluteTags[ type ].Size(); i++ )
	{
		CEventAbsoluteTag *ptag = &m_AbsoluteTags[ type ][ i ];
		if ( !ptag )
			continue;

		if ( ptag->GetExit() )
		{
			return ptag;
		}
	}
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *style - 
//			maxlen - 
//-----------------------------------------------------------------------------
void CChoreoEvent::GetMovementStyle( char *style, int maxlen )
{
	Assert( GetType() == MOVETO );

	style[0] = 0;

	const char *in = m_Parameters2.Get();
	char *out = style;

	while ( *in && *in != '\0' && *in != ' ' )
	{
		if ( out - style >= maxlen - 1 )
			break;
		*out++ = *in++;
	}

	*out = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *style - 
//			maxlen - 
//-----------------------------------------------------------------------------
void CChoreoEvent::GetDistanceStyle( char *style, int maxlen )
{
	Assert( GetType() == MOVETO );

	style[0]= 0;

	const char *in = Q_strstr( m_Parameters2.Get(), " " );
	if ( !in )
		return;

	in++;
	char *out = style;

	while ( *in && *in != '\0' )
	{
		if ( out - style >= maxlen - 1 )
			break;
		*out++ = *in++;
	}

	*out = 0;
}

void CChoreoEvent::SetCloseCaptionType( CLOSECAPTION type )
{
	Assert( m_fType == SPEAK );
	m_ccType = type;
}

CChoreoEvent::CLOSECAPTION CChoreoEvent::GetCloseCaptionType() const
{
	Assert( m_fType == SPEAK );
	return (CLOSECAPTION)m_ccType;
}

void CChoreoEvent::SetCloseCaptionToken( char const *token )
{
	Assert( m_fType == SPEAK );
	Assert( token );
	m_CCToken = token;
}

char const *CChoreoEvent::GetCloseCaptionToken() const
{
	Assert( m_fType == SPEAK );
	return m_CCToken.Get();
}

bool CChoreoEvent::GetPlaybackCloseCaptionToken( char *dest, int destlen )
{
	dest[0] = 0;

	Assert( m_fType == SPEAK );

	switch ( m_ccType )
	{
	default:
	case CC_DISABLED:
		{
			return false;
		}
	case CC_SLAVE:
		{
			// If it's a slave, then only disable if we're not using the combined wave
			if ( IsUsingCombinedFile() )
			{
				return false;
			}

			if ( m_CCToken[ 0 ] != 0 )
			{
				Q_strncpy( dest, m_CCToken.Get(), destlen );
			}
			else
			{
				Q_strncpy( dest, m_Parameters.Get(), destlen );
			}
			return true;
		}
	case CC_MASTER:
		{
			// Always use the override if we're the master, otherwise always use the default
			//  parameter
			if ( m_CCToken[ 0 ] != 0 )
			{
				Q_strncpy( dest, m_CCToken.Get(), destlen );
			}
			else
			{
				Q_strncpy( dest, m_Parameters.Get(), destlen );
			}
			return true;
		}
	}

	return false;
}


void CChoreoEvent::SetUsingCombinedFile( bool isusing )
{
	Assert( m_fType == SPEAK );
	m_bUsingCombinedSoundFile = isusing;
}

bool CChoreoEvent::IsUsingCombinedFile() const
{
	Assert( m_fType == SPEAK );
	return m_bUsingCombinedSoundFile;
}

void CChoreoEvent::SetRequiredCombinedChecksum( unsigned int checksum )
{
	Assert( m_fType == SPEAK );
	m_uRequiredCombinedChecksum = checksum;
}

unsigned int CChoreoEvent::GetRequiredCombinedChecksum()
{
	Assert( m_fType == SPEAK );
	return m_uRequiredCombinedChecksum;
}

void CChoreoEvent::SetNumSlaves( int num )
{
	Assert( m_fType == SPEAK );
	Assert( num >= 0 );
	m_nNumSlaves = num;
}

int CChoreoEvent::GetNumSlaves() const
{
	Assert( m_fType == SPEAK );
	return m_nNumSlaves;
}

void CChoreoEvent::SetLastSlaveEndTime( float t )
{
	Assert( m_fType == SPEAK );
	m_flLastSlaveEndTime = t;
}

float CChoreoEvent::GetLastSlaveEndTime() const
{
	Assert( m_fType == SPEAK );
	return m_flLastSlaveEndTime;
}

void CChoreoEvent::SetCloseCaptionTokenValid( bool valid )
{
	Assert( m_fType == SPEAK );
	m_bCCTokenValid = valid;
}

bool CChoreoEvent::GetCloseCaptionTokenValid() const
{
	Assert( m_fType == SPEAK );
	return m_bCCTokenValid;
}


//-----------------------------------------------------------------------------
// Purpose: Removes characters which can't appear in windows filenames
// Input  : *in - 
//			*dest - 
//			destlen - 
// Output : static void
//-----------------------------------------------------------------------------
static void CleanupTokenName( char const *in, char *dest, int destlen )
{
	char *out = dest;
	while ( *in && ( out - dest ) < destlen )
	{
		if ( V_isalnum( *in ) ||   // lowercase, uppercase, digits and underscore are valid
			*in == '_' )
		{
			*out++ = *in;
		}
		else
		{
			*out++ = '_';  // Put underscores in for bogus characters
		}
		in++;
	}
	*out = 0;
}

bool CChoreoEvent::ComputeCombinedBaseFileName( char *dest, int destlen, bool creategenderwildcard )
{
	if ( m_fType  != SPEAK )
		return false;

	if ( m_ccType != CC_MASTER )
		return false;

	if ( GetNumSlaves() == 0 )
		return false;

	if ( !m_pScene )
		return false;

	char vcdpath[ 512 ];
	char cleanedtoken[ MAX_CCTOKEN_STRING ];
	CleanupTokenName( m_CCToken.Get(), cleanedtoken, sizeof( cleanedtoken ) );

	if ( Q_strlen( cleanedtoken ) <= 0 )
		return false;

	Q_strncpy( vcdpath, m_pScene->GetFilename(), sizeof( vcdpath ) );
	Q_StripFilename( vcdpath );
	Q_FixSlashes( vcdpath, '/' );

	char *pvcd = vcdpath;

	char *offset = Q_strstr( vcdpath, "scenes" );
	if ( offset )
	{
		pvcd = offset + 6;
		if ( *pvcd == '/' )
		{
			++pvcd;
		}
	}

	int len = Q_strlen( pvcd );

	if ( len > 0 && ( len + 1 ) < ( sizeof( vcdpath ) - 1 ) )
	{
		pvcd[ len ] = '/';
		pvcd[ len + 1 ] = 0;
	}

	Assert( !Q_strstr( pvcd, ":" ) );

	if ( creategenderwildcard )
	{
		Q_snprintf( dest, destlen, "sound/combined/%s%s_$gender.wav", pvcd, cleanedtoken );
	}
	else
	{
		Q_snprintf( dest, destlen, "sound/combined/%s%s.wav", pvcd, cleanedtoken );
	}
	return true;
}

bool CChoreoEvent::IsCombinedUsingGenderToken() const
{
	return m_bCombinedUsingGenderToken;
}

void CChoreoEvent::SetCombinedUsingGenderToken( bool using_gender )
{
	m_bCombinedUsingGenderToken = using_gender;
}


int CChoreoEvent::ValidateCombinedFile()
{

	return 0;
}

bool CChoreoEvent::IsSuppressingCaptionAttenuation() const
{
	return m_bSuppressCaptionAttenuation;
}

void CChoreoEvent::SetSuppressingCaptionAttenuation( bool suppress )
{
	m_bSuppressCaptionAttenuation = suppress;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoEvent::ClearEventDependencies()
{
	m_Dependencies.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *other - 
//-----------------------------------------------------------------------------
void CChoreoEvent::AddEventDependency( CChoreoEvent *other )
{
	if ( m_Dependencies.Find( other ) == m_Dependencies.InvalidIndex() )
	{
		m_Dependencies.AddToTail( other );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : list - 
//-----------------------------------------------------------------------------
void CChoreoEvent::GetEventDependencies( CUtlVector< CChoreoEvent * >& list )
{
	int c = m_Dependencies.Count();
	for ( int i = 0; i < c; ++i )
	{
		list.AddToTail( m_Dependencies[ i ] );
	}
}

void CCurveData::SetEdgeInfo( bool leftEdge, int curveType, float zero )
{
	int idx = leftEdge ? 0 : 1;
	m_RampEdgeInfo[ idx ].m_CurveType = curveType;
	m_RampEdgeInfo[ idx ].m_flZeroPos = zero;
}

void CCurveData::GetEdgeInfo( bool leftEdge, int& curveType, float& zero ) const
{
	int idx = leftEdge ? 0 : 1;
	curveType = m_RampEdgeInfo[ idx ].m_CurveType;
	zero = m_RampEdgeInfo[ idx ].m_flZeroPos;
}

void CCurveData::SetEdgeActive( bool leftEdge, bool state )
{
	int idx = leftEdge ? 0 : 1;
	m_RampEdgeInfo[ idx ].m_bActive = state;
}

bool CCurveData::IsEdgeActive( bool leftEdge ) const
{
	int idx = leftEdge ? 0 : 1;
	return m_RampEdgeInfo[ idx ].m_bActive;
}

int CCurveData::GetEdgeCurveType( bool leftEdge ) const
{
	if ( !IsEdgeActive( leftEdge ) )
	{
		return CURVE_DEFAULT;
	}

	int idx = leftEdge ? 0 : 1;
	return m_RampEdgeInfo[ idx ].m_CurveType;
}

float CCurveData::GetEdgeZeroValue( bool leftEdge ) const
{
	if ( !IsEdgeActive( leftEdge ) )
	{
		return 0.0f;
	}

	int idx = leftEdge ? 0 : 1;
	return m_RampEdgeInfo[ idx ].m_flZeroPos;
}


void CChoreoEvent::SaveToBuffer( CUtlBuffer& buf, CChoreoScene *pScene, IChoreoStringPool *pStringPool )
{
	buf.PutChar( GetType() );
	buf.PutShort( pStringPool->FindOrAddString( GetName() ) );

	float st = GetStartTime();
	buf.PutFloat( st );

	float et = GetEndTime();
	buf.PutFloat( et );

	buf.PutShort( pStringPool->FindOrAddString( GetParameters() ) );
	buf.PutShort( pStringPool->FindOrAddString( GetParameters2() ) );
	buf.PutShort( pStringPool->FindOrAddString( GetParameters3() ) );

	m_Ramp.SaveToBuffer( buf, pStringPool );  

	int flags = 0;
	flags |= IsResumeCondition() ? 1<<0 : 0;
	flags |= IsLockBodyFacing() ? 1<<1 : 0;
	flags |= IsFixedLength() ? 1<<2 : 0;
	flags |= GetActive() ? 1<<3 : 0;
	flags |= GetForceShortMovement() ? 1<<4 : 0;
	flags |= GetPlayOverScript() ? 1<<5 : 0;

	buf.PutUnsignedChar( flags );

	buf.PutFloat( GetDistanceToTarget() );

	int numRelativeTags = GetNumRelativeTags();
	Assert( numRelativeTags <= 255 );
	buf.PutUnsignedChar( numRelativeTags );

	for ( int t = 0; t < numRelativeTags; t++ )
	{
		CEventRelativeTag *rt = GetRelativeTag( t );
		Assert( rt );
		buf.PutShort( pStringPool->FindOrAddString( rt->GetName() ) );

		Assert( rt->GetPercentage() >= 0.0f && rt->GetPercentage() <= 1.0f );
		unsigned char p = rt->GetPercentage() * 255.0f;
		buf.PutUnsignedChar( p );
	}

	int numTimingTags = GetNumTimingTags(); 
	Assert( numTimingTags <= 255 );
	buf.PutUnsignedChar( numTimingTags );

	for ( int t = 0; t < numTimingTags; t++ )
	{
		CFlexTimingTag *tt = GetTimingTag( t );
		Assert( tt );
		buf.PutShort( pStringPool->FindOrAddString( tt->GetName() ) );

		// save as u0.8
		Assert( tt->GetPercentage() >= 0.0f && tt->GetPercentage() <= 1.0f );
		unsigned char p = tt->GetPercentage() * 255.0f;
		buf.PutUnsignedChar( p );

		// Don't save locked state, it's only used by the editor tt->GetLocked()
	}

	int tagtype;
	for ( tagtype = 0; tagtype < CChoreoEvent::NUM_ABS_TAG_TYPES; tagtype++ )
	{
		int num = GetNumAbsoluteTags( (CChoreoEvent::AbsTagType)tagtype );
		Assert( num <= 255 );
		buf.PutUnsignedChar( num );

		for ( int i = 0; i < num ; ++i )
		{
			CEventAbsoluteTag *abstag = GetAbsoluteTag( (CChoreoEvent::AbsTagType)tagtype, i );
			Assert( abstag );
			buf.PutShort( pStringPool->FindOrAddString( abstag->GetName() ) );

			// save as u4.12
			Assert( abstag->GetPercentage() >= 0.0f && abstag->GetPercentage() <= 15.0f );
			unsigned short p = abstag->GetPercentage() * 4096.0f;
			buf.PutUnsignedShort( p );
		}
	}

	if ( GetType() == CChoreoEvent::GESTURE )
	{
		float duration;
		if ( GetGestureSequenceDuration( duration ) )
		{
			buf.PutFloat( duration );
		}
		else
		{
			buf.PutFloat( -1.0f );
		}
	}

	buf.PutChar( IsUsingRelativeTag() ? 1 : 0 );
	if ( IsUsingRelativeTag() )
	{
		buf.PutShort( pStringPool->FindOrAddString( GetRelativeTagName() ) );
		buf.PutShort( pStringPool->FindOrAddString( GetRelativeWavName() ) );
	}
	
	SaveFlexAnimationsToBuffer( buf, pStringPool );

	if ( GetType() == LOOP )
	{
		buf.PutChar( GetLoopCount() );
	}

	if ( GetType() == CChoreoEvent::SPEAK )
	{
		buf.PutChar( GetCloseCaptionType() );
		buf.PutShort( pStringPool->FindOrAddString( GetCloseCaptionToken() ) );
		flags = 0;

		if ( GetCloseCaptionType() != CChoreoEvent::CC_DISABLED &&
			 IsUsingCombinedFile() )
		{
			flags |= ( 1<<0 );
		}
		if ( IsCombinedUsingGenderToken() )
		{
			flags |= ( 1<<1 );
		}
		if ( IsSuppressingCaptionAttenuation() )
		{
			flags |= ( 1<<2 );
		}

		buf.PutChar( flags );
	}
}

bool CChoreoEvent::RestoreFromBuffer( CUtlBuffer& buf, CChoreoScene *pScene, IChoreoStringPool *pStringPool )
{
	MEM_ALLOC_CREDIT();

	SetType( (EVENTTYPE)buf.GetChar() );
	char sz[ 256 ];
	pStringPool->GetString( buf.GetShort(), sz, sizeof( sz ) );
	SetName( sz );

	SetStartTime( buf.GetFloat() );
	SetEndTime( buf.GetFloat() );

	char params[ 2048 ];
	pStringPool->GetString( buf.GetShort(), params, sizeof( params ) );
	SetParameters( params );
	pStringPool->GetString( buf.GetShort(), params, sizeof( params ) );
	SetParameters2( params );
	pStringPool->GetString( buf.GetShort(), params, sizeof( params ) );
	SetParameters3( params );

	if ( !m_Ramp.RestoreFromBuffer( buf, pStringPool ) )
		return false;

	int flags = buf.GetUnsignedChar();
	SetResumeCondition( ( flags & ( 1<<0 ) ) ? true : false );
	SetLockBodyFacing( ( flags & ( 1<<1 ) ) ? true : false );
	SetFixedLength( ( flags & ( 1<<2 ) ) ? true : false );
	SetActive( ( flags & ( 1<<3 ) ) ? true : false );
	SetForceShortMovement( ( flags & ( 1<<4 ) ) ? true : false );
	SetPlayOverScript( ( flags & ( 1<<5 ) ) ? true : false );

	SetDistanceToTarget( buf.GetFloat() );

	int numRelTags = buf.GetUnsignedChar();
	for ( int i = 0; i < numRelTags; ++i )
	{
		char tagName[ 256 ];
		pStringPool->GetString( buf.GetShort(), tagName, sizeof( tagName ) );
		float percentage = (float)buf.GetUnsignedChar() * 1.0f/255.0f;
		AddRelativeTag( tagName, percentage );
	}

	int numTimingTags = buf.GetUnsignedChar();
	for ( int i = 0; i < numTimingTags; ++i )
	{
		char tagName[ 256 ];
		pStringPool->GetString( buf.GetShort(), tagName, sizeof( tagName ) );
		float percentage = (float)buf.GetUnsignedChar() * 1.0f/255.0f;
		// Don't parse locked state, only used by editors
		AddTimingTag( tagName, percentage, false );
	}

	int tagtype;
	for ( tagtype = 0; tagtype < CChoreoEvent::NUM_ABS_TAG_TYPES; tagtype++ )
	{
		int num = buf.GetUnsignedChar();
		for ( int i = 0; i < num; ++i )
		{
			char tagName[ 256 ];
			pStringPool->GetString( buf.GetShort(), tagName, sizeof( tagName ) );
			float percentage = (float)buf.GetUnsignedShort() * 1.0f/4096.0f;

			// Don't parse locked state, only used by editors
			AddAbsoluteTag( (CChoreoEvent::AbsTagType)tagtype, tagName, percentage );
		}
	}

	if ( GetType() == CChoreoEvent::GESTURE )
	{
		float duration = buf.GetFloat();
		if ( duration != -1 )
		{
			SetGestureSequenceDuration( duration );
		}
	}
	
	if ( buf.GetChar() == 1 )
	{
		char tagname[ 256 ];
		char wavname[ 256 ];
		pStringPool->GetString( buf.GetShort(), tagname, sizeof( tagname ) );
		pStringPool->GetString( buf.GetShort(), wavname, sizeof( wavname ) );

		SetUsingRelativeTag( true, tagname, wavname );
	}

	if ( !RestoreFlexAnimationsFromBuffer( buf, pStringPool ) )
		return false;

	if ( GetType() == LOOP )
	{
		SetLoopCount( buf.GetChar() );
	}

	if ( GetType() == CChoreoEvent::SPEAK )
	{
		SetCloseCaptionType( (CLOSECAPTION)buf.GetChar() );
		char cctoken[ 256 ];
		pStringPool->GetString( buf.GetShort(), cctoken, sizeof( cctoken ) );
		SetCloseCaptionToken( cctoken );
		int flags = buf.GetChar();
		if ( flags & ( 1<<0 ) )
		{
			SetUsingCombinedFile( true );
		}
		if ( flags & ( 1<<1 ) )
		{
			SetCombinedUsingGenderToken( true );
		}
		if ( flags & ( 1<<2 ) )
		{
			SetSuppressingCaptionAttenuation( true );
		}
	}

	return true;
}

void CCurveData::SaveToBuffer( CUtlBuffer& buf, IChoreoStringPool *pStringPool )
{
	int c = GetCount();
	Assert( c <= 255 );
	buf.PutUnsignedChar( c );

	for ( int i = 0; i < c; i++ )
	{
		CExpressionSample *sample = Get( i );
		buf.PutFloat( sample->time );

		Assert( sample->value >= 0.0f && sample->value <= 1.0f );
		unsigned char v = sample->value * 255.0f;
		buf.PutUnsignedChar( v );
	}	
}

bool CCurveData::RestoreFromBuffer( CUtlBuffer& buf, IChoreoStringPool *pStringPool )
{
	int c = buf.GetUnsignedChar();
	for ( int i = 0; i < c; i++ )
	{
		float t, v;
		t = buf.GetFloat();
		v = (float)buf.GetUnsignedChar() * 1.0f/255.0f;

		Add( t, v, false );
	}

	return true;
}

void CChoreoEvent::SaveFlexAnimationsToBuffer( CUtlBuffer& buf, IChoreoStringPool *pStringPool )
{
	int numFlexAnimationTracks = GetNumFlexAnimationTracks();
	Assert( numFlexAnimationTracks <= 255 );
	buf.PutUnsignedChar( numFlexAnimationTracks );

	for ( int i = 0; i < numFlexAnimationTracks; i++ )
	{
		CFlexAnimationTrack *track = GetFlexAnimationTrack( i );

		buf.PutShort( pStringPool->FindOrAddString( track->GetFlexControllerName() ) );

		int flags = 0;
		flags |= track->IsTrackActive() ? 1<<0 : 0;
		flags |= track->IsComboType() ? 1<<1 : 0;
		buf.PutUnsignedChar( flags );

		buf.PutFloat( track->GetMin() );
		buf.PutFloat( track->GetMax() );

		buf.PutShort( track->GetNumSamples( 0 ) );
        for ( int j = 0 ; j < track->GetNumSamples( 0 ) ; j++ )
		{
			CExpressionSample *s = track->GetSample( j, 0 );
			if ( !s )
				continue;

			buf.PutFloat( s->time );

			Assert( s->value >= 0.0f && s->value <= 1.0f );
			unsigned char v = s->value * 255.0f;
			buf.PutUnsignedChar( v );

			buf.PutUnsignedShort( s->GetCurveType() );
		}

		// Write out combo samples
		if ( track->IsComboType() )
		{
			int numSamples = track->GetNumSamples( 1 );
			Assert( numSamples <= 32767 );
			buf.PutUnsignedShort( numSamples );

			for ( int j = 0; j < numSamples; j++ )
			{
				CExpressionSample *s = track->GetSample( j, 1 );
				if ( !s )
					continue;

				buf.PutFloat( s->time );

				Assert( s->value >= 0.0f && s->value <= 1.0f );
				unsigned char v = s->value * 255.0f;
				buf.PutUnsignedChar( v );

				buf.PutUnsignedShort( s->GetCurveType() );
			}
		}
	}
}

bool CChoreoEvent::RestoreFlexAnimationsFromBuffer( CUtlBuffer& buf, IChoreoStringPool *pStringPool )
{
	int numTracks = buf.GetUnsignedChar();

	for ( int i = 0; i < numTracks; i++ )
	{
		char name[ 256 ];
		pStringPool->GetString( buf.GetShort(), name, sizeof( name ) );

		CFlexAnimationTrack *track = AddTrack( name );

		int flags = buf.GetUnsignedChar();
		track->SetTrackActive( ( flags & ( 1<<0 ) ) ? true : false );
		track->SetComboType( ( flags & ( 1<<1 ) ) ? true : false );

		track->SetMin( buf.GetFloat() );
		track->SetMax( buf.GetFloat() );

		int s = buf.GetShort();
		for ( int j = 0; j < s; ++j )
		{
			float t, v;
			t = buf.GetFloat();
			v = (float)buf.GetUnsignedChar() * 1.0f/255.0f;

			CExpressionSample *pSample = track->AddSample( t, v, 0 );
			pSample->SetCurveType( buf.GetUnsignedShort() );
		}

		if ( track->IsComboType() )
		{
			int s = buf.GetUnsignedShort();
			for ( int j = 0; j < s; ++j )
			{
				float t, v;
				t = buf.GetFloat();
				v = (float)buf.GetUnsignedChar() * 1.0f/255.0f;

				CExpressionSample *pSample = track->AddSample( t, v, 1 );
				pSample->SetCurveType( buf.GetUnsignedShort() );
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Marks the event as enabled/disabled
// Input  : state - 
//-----------------------------------------------------------------------------
void CChoreoEvent::SetActive( bool state )
{
	m_bActive = state;
}

bool CChoreoEvent::GetActive() const
{
	return m_bActive;
}
