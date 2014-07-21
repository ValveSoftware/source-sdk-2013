//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef EXPRESSIONSAMPLE_H
#define EXPRESSIONSAMPLE_H
#ifdef _WIN32
#pragma once
#endif

#include "interpolatortypes.h"

class CUtlBuffer;
class ISceneTokenProcessor;
class IChoreoStringPool;

#pragma pack(1)
struct EdgeInfo_t
{
	EdgeInfo_t() : 
		m_bActive( false ),
		m_CurveType( CURVE_DEFAULT ),
		m_flZeroPos( 0.0f )
	{
	}

	bool			m_bActive;
	unsigned short 	m_CurveType;
	float			m_flZeroPos;
};

struct CExpressionSample
{
	CExpressionSample() :
		value( 0.0f ),
		time( 0.0f )
	{
		selected = 0;
		m_curvetype = CURVE_DEFAULT;
	}

	void SetCurveType( int curveType )
	{
		m_curvetype = curveType;
	}

	int	GetCurveType() const
	{
		return m_curvetype;
	}

	// Height
	float			value;
	// time from start of event
	float			time;

	unsigned short	selected	: 1;
private:
	unsigned short	m_curvetype	: 15;
};
#pragma pack()

//-----------------------------------------------------------------------------
// Purpose: Provides generic access to scene or event ramp data
//-----------------------------------------------------------------------------
class ICurveDataAccessor
{
public:
	virtual float	GetDuration() = 0;
	virtual bool	CurveHasEndTime() = 0; // only matters for events
	virtual int		GetDefaultCurveType() = 0;
};

//-----------------------------------------------------------------------------
// Purpose: The generic curve data
//-----------------------------------------------------------------------------

class CCurveData
{
public:
	int				GetCount( void );
	CExpressionSample *Get( int index );
	CExpressionSample *Add( float time, float value, bool selected );
	void			Delete( int index );
	void			Clear( void );
	void			Resort( ICurveDataAccessor *data );

	EdgeInfo_t			*GetEdgeInfo( int idx );

	void				SetEdgeInfo( bool leftEdge, int curveType, float zero );
	void				GetEdgeInfo( bool leftEdge, int& curveType, float& zero ) const;
	void				SetEdgeActive( bool leftEdge, bool state );
	bool				IsEdgeActive( bool leftEdge ) const;
	int					GetEdgeCurveType( bool leftEdge ) const;
	float				GetEdgeZeroValue( bool leftEdge ) const;
	void				RemoveOutOfRangeSamples( ICurveDataAccessor *data );

	void				SaveToBuffer( CUtlBuffer& buf, IChoreoStringPool *pStringPool );
	bool				RestoreFromBuffer( CUtlBuffer& buf, IChoreoStringPool *pStringPool );

	void				Parse( ISceneTokenProcessor *tokenizer, ICurveDataAccessor *data );
	void				FileSave( CUtlBuffer& buf, int level, const char *name );

	float	GetIntensity( ICurveDataAccessor *data, float time );
	CExpressionSample *GetBoundedSample( ICurveDataAccessor *data, int number, bool& bClamped );

	CCurveData & operator = (const CCurveData &src) 
	{
		// Copy ramp over
		m_Ramp.RemoveAll();
		int i;
		for ( i = 0; i < src.m_Ramp.Count(); i++ )
		{
			CExpressionSample sample = src.m_Ramp[ i ];
			CExpressionSample *newSample = Add( sample.time, sample.value, sample.selected );
			newSample->SetCurveType( sample.GetCurveType() );
		}
		m_RampEdgeInfo[ 0 ] = src.m_RampEdgeInfo[ 0 ];
		m_RampEdgeInfo[ 1 ] = src.m_RampEdgeInfo[ 1 ];

		return *this;

	};

private:
	CUtlVector< CExpressionSample > m_Ramp;
	EdgeInfo_t		m_RampEdgeInfo[ 2 ];

public:
	float	GetIntensityArea( ICurveDataAccessor *data, float time );

private:
	void	UpdateIntensityArea( ICurveDataAccessor *data );
	CUtlVector< float > m_RampAccumulator;
};


#endif // EXPRESSIONSAMPLE_H
