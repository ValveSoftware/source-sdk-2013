//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"

#if !defined( NO_ENTITY_PREDICTION )

#if defined( CLIENT_DLL )

#include "igamesystem.h"

#endif
#include <memory.h>
#include <stdarg.h>
#include "tier0/dbg.h"
#include "tier1/strtools.h"
#include "predictioncopy.h"
#include "engine/ivmodelinfo.h"
#include "tier1/fmtstr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// --------------------------------------------------------------
//
// CSave
//
// --------------------------------------------------------------
static const char *g_FieldTypes[ FIELD_TYPECOUNT ] = 
{
	"FIELD_VOID",			// FIELD_VOID
	"FIELD_FLOAT",			// FIELD_FLOAT
	"FIELD_STRING",			// FIELD_STRING
	"FIELD_VECTOR",			// FIELD_VECTOR
	"FIELD_QUATERNION",		// FIELD_QUATERNION
	"FIELD_INTEGER",		// FIELD_INTEGER
	"FIELD_BOOLEAN",		// FIELD_BOOLEAN
	"FIELD_SHORT",			// FIELD_SHORT
	"FIELD_CHARACTER",		// FIELD_CHARACTER
	"FIELD_COLOR32",		// FIELD_COLOR32
	"FIELD_EMBEDDED",		// FIELD_EMBEDDED	(handled specially)
	"FIELD_CUSTOM",			// FIELD_CUSTOM		(handled specially)
	"FIELD_CLASSPTR",		// FIELD_CLASSPTR
	"FIELD_EHANDLE",		// FIELD_EHANDLE
	"FIELD_EDICT",			// FIELD_EDICT
	"FIELD_POSITION_VECTOR",// FIELD_POSITION_VECTOR
	"FIELD_TIME",			// FIELD_TIME
	"FIELD_TICK",			// FIELD_TICK
	"FIELD_MODELNAME",		// FIELD_MODELNAME
	"FIELD_SOUNDNAME",		// FIELD_SOUNDNAME
	"FIELD_INPUT",			// FIELD_INPUT		(uses custom type)
	"FIELD_FUNCTION",		// FIELD_FUNCTION
	"FIELD_VMATRIX",			
	"FIELD_VMATRIX_WORLDSPACE",
	"FIELD_MATRIX3X4_WORLDSPACE",
	"FIELD_INTERVAL"		// FIELD_INTERVAL
	"FIELD_MODELINDEX"		// FIELD_MODELINDEX
};

CPredictionCopy::CPredictionCopy( int type, void *dest, bool dest_packed, void const *src, bool src_packed, 
	bool counterrors /*= false*/, bool reporterrors /*= false*/, bool performcopy /*= true*/,
	bool describefields /*= false*/, FN_FIELD_COMPARE func /*= NULL*/ )
{
	m_nType				= type;
	m_pDest				= dest;
	m_pSrc				= src;
	m_nDestOffsetIndex	= dest_packed ? TD_OFFSET_PACKED : TD_OFFSET_NORMAL;
	m_nSrcOffsetIndex	= src_packed ? TD_OFFSET_PACKED : TD_OFFSET_NORMAL;
	m_bErrorCheck		= counterrors;
	m_bReportErrors		= reporterrors;
	m_bPerformCopy		= performcopy;
	m_bDescribeFields	= describefields;

	m_pCurrentField		= NULL;
	m_pCurrentMap		= NULL;
	m_pCurrentClassName = NULL;
	m_bShouldReport		= false;
	m_bShouldDescribe	= false;
	m_nErrorCount		= 0;

	m_FieldCompareFunc	= func;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fmt - 
//			... - 
//-----------------------------------------------------------------------------
void CPredictionCopy::ReportFieldsDiffer( const char *fmt, ... )
{
	++m_nErrorCount;

	if ( !m_bShouldReport )
		return;

	if ( m_bDescribeFields && m_FieldCompareFunc )
		return;

	Assert( m_pCurrentMap );
	Assert( m_pCurrentClassName );

	const char *fieldname = "empty";
	int flags = 0;

	if ( m_pCurrentField )
	{
		flags		= m_pCurrentField->flags;
		fieldname	= m_pCurrentField->fieldName ? m_pCurrentField->fieldName : "NULL";
	}

	va_list argptr;
	char data[ 4096 ];
	int len;
	va_start(argptr, fmt);
	len = Q_vsnprintf(data, sizeof( data ), fmt, argptr);
	va_end(argptr);

	if ( m_nErrorCount == 1 )
	{
		Msg( "\n" );
	}

	Msg( "[Tick %d] %03i %s::%s - %s",
		gpGlobals->tickcount,
		m_nErrorCount,
		m_pCurrentClassName,
		fieldname,
		data );

	m_bShouldReport = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fmt - 
//			... - 
//-----------------------------------------------------------------------------
void CPredictionCopy::DescribeFields( difftype_t dt, const char *fmt, ... )
{
	if ( !m_bShouldDescribe )
		return;

	if ( !m_FieldCompareFunc )
		return;

	Assert( m_pCurrentMap );
	Assert( m_pCurrentClassName );

	const char *fieldname = "empty";
	int flags = 0;

	if ( m_pCurrentField )
	{
		flags		= m_pCurrentField->flags;
		fieldname	= m_pCurrentField->fieldName ? m_pCurrentField->fieldName : "NULL";
	}

	va_list argptr;
	char data[ 4096 ];
	int len;
	va_start(argptr, fmt);
	len = Q_vsnprintf(data, sizeof( data ), fmt, argptr);
	va_end(argptr);

	bool isnetworked = ( flags & FTYPEDESC_INSENDTABLE ) ? true : false;
	bool isnoterrorchecked = ( flags & FTYPEDESC_NOERRORCHECK ) ? true : false;

	( *m_FieldCompareFunc )( 
		m_pCurrentClassName,
		fieldname,
		g_FieldTypes[ m_pCurrentField->fieldType ],
		isnetworked,
		isnoterrorchecked,
		dt != IDENTICAL ? true : false,
		dt == WITHINTOLERANCE ? true : false,
		data 
	);

	m_bShouldDescribe = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPredictionCopy::CanCheck( void )
{
	Assert( m_pCurrentField );

	if ( m_pCurrentField->flags & FTYPEDESC_NOERRORCHECK )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : size - 
//			*outdata - 
//			*indata - 
//-----------------------------------------------------------------------------
/*
void CPredictionCopy::CopyData( difftype_t dt, int size, char *outdata, const char *indata )
{
	if ( !m_bPerformCopy )
		return;

	if ( dt == IDENTICAL )
		return;

	memcpy( outdata, indata, size );
}
*/

CPredictionCopy::difftype_t CPredictionCopy::CompareData( int size, char *outdata, const char *indata )
{
	if ( !m_bErrorCheck )
		return DIFFERS;

	if ( CanCheck() )
	{
		if ( memcmp( outdata, indata, size ) )
		{
			return DIFFERS;
		}
		else
		{
			// No difference, so no need to copy
			return IDENTICAL;
		}
	}

	// Fields differ
	return IDENTICAL;
}

void CPredictionCopy::DescribeData( difftype_t dt, int size, char *outdata, const char *indata )
{
	if ( !m_bErrorCheck )
		return;

	if ( dt == DIFFERS )
	{
		ReportFieldsDiffer( "binary data differs (%i bytes)\n", size );
	}

	DescribeFields( dt, "binary (%i bytes)\n", size );
}

void CPredictionCopy::WatchData( difftype_t dt, int size, char *outdata, const char *indata )
{
	if ( m_pWatchField != m_pCurrentField )
		return;

	WatchMsg( "binary (%i bytes)", size );
}

void CPredictionCopy::DescribeShort( difftype_t dt, short *outvalue, const short *invalue, int count )
{
	if ( !m_bErrorCheck )
		return;

	if ( dt == DIFFERS )
	{
		int i = 0;
		ReportFieldsDiffer( "short differs (net %i pred %i) diff(%i)\n", (int)(invalue[i]), (int)(outvalue[i]), (int)(outvalue[i] - invalue[i]) );
	}

	DescribeFields( dt, "short (%i)\n", (int)(outvalue[0]) );
}

void CPredictionCopy::WatchShort( difftype_t dt, short *outvalue, const short *invalue, int count )
{
	if ( m_pWatchField != m_pCurrentField )
		return;

	WatchMsg( "short (%i)", (int)(outvalue[0]) );
}

#if defined( CLIENT_DLL )
#include "cdll_int.h"

#endif

void CPredictionCopy::DescribeInt( difftype_t dt, int *outvalue, const int *invalue, int count )
{
	if ( !m_bErrorCheck )
		return;

	if ( dt == DIFFERS )
	{
		int i = 0;
		ReportFieldsDiffer( "int differs (net %i pred %i) diff(%i)\n", invalue[i], outvalue[i], outvalue[i] - invalue[i] );
	}

#if defined( CLIENT_DLL )
	bool described = false;
	if ( m_pCurrentField->flags & FTYPEDESC_MODELINDEX )
	{
		int modelindex = outvalue[0];
		model_t const *m = modelinfo->GetModel( modelindex );
		if ( m )
		{
			described = true;
			char shortfile[ 512 ];
			shortfile[ 0 ] = 0;
			Q_FileBase( modelinfo->GetModelName( m ), shortfile, sizeof( shortfile ) );

			DescribeFields( dt, "integer (%i->%s)\n", outvalue[0], shortfile );
		}
	}

	if ( !described )
	{
		DescribeFields( dt, "integer (%i)\n", outvalue[0] );
	}
#else
	DescribeFields( dt, "integer (%i)\n", outvalue[0] );
#endif
}

void CPredictionCopy::WatchInt( difftype_t dt, int *outvalue, const int *invalue, int count )
{
	if ( m_pWatchField != m_pCurrentField )
		return;

#if defined( CLIENT_DLL )
	bool described = false;
	if ( m_pCurrentField->flags & FTYPEDESC_MODELINDEX )
	{
		int modelindex = outvalue[0];
		model_t const *m = modelinfo->GetModel( modelindex );
		if ( m )
		{
			described = true;
			char shortfile[ 512 ];
			shortfile[ 0 ] = 0;
			Q_FileBase( modelinfo->GetModelName( m ), shortfile, sizeof( shortfile ) );

			WatchMsg( "integer (%i->%s)", outvalue[0], shortfile );
		}
	}

	if ( !described )
	{
		WatchMsg( "integer (%i)", outvalue[0] );
	}
#else
	WatchMsg( "integer (%i)", outvalue[0] );
#endif
}

void CPredictionCopy::DescribeBool( difftype_t dt, bool *outvalue, const bool *invalue, int count )
{
	if ( !m_bErrorCheck )
		return;

	if ( dt == DIFFERS )
	{
		int i = 0;
		ReportFieldsDiffer( "bool differs (net %s pred %s)\n", (invalue[i]) ? "true" : "false", (outvalue[i]) ? "true" : "false" );
	}

	DescribeFields( dt, "bool (%s)\n", (outvalue[0]) ? "true" : "false" );
}


void CPredictionCopy::WatchBool( difftype_t dt, bool *outvalue, const bool *invalue, int count )
{
	if ( m_pWatchField != m_pCurrentField )
		return;

	WatchMsg( "bool (%s)", (outvalue[0]) ? "true" : "false" );
}

void CPredictionCopy::DescribeFloat( difftype_t dt, float *outvalue, const float *invalue, int count )
{
	if ( !m_bErrorCheck )
		return;

	if ( dt == DIFFERS )
	{
		int i = 0;
		ReportFieldsDiffer( "float differs (net %f pred %f) diff(%f)\n", invalue[ i ], outvalue[ i ], outvalue[ i ] - invalue[ i ] );
	}

	DescribeFields( dt, "float (%f)\n", outvalue[ 0 ] );
}

void CPredictionCopy::WatchFloat( difftype_t dt, float *outvalue, const float *invalue, int count )
{
	if ( m_pWatchField != m_pCurrentField )
		return;

	WatchMsg( "float (%f)", outvalue[ 0 ] );
}

void CPredictionCopy::DescribeString( difftype_t dt, char *outstring, const char *instring )
{
	if ( !m_bErrorCheck )
		return;

	if ( dt == DIFFERS )
	{
		ReportFieldsDiffer( "string differs (net %s pred %s)\n", instring, outstring );
	}

	DescribeFields( dt, "string (%s)\n", outstring );
}

void CPredictionCopy::WatchString( difftype_t dt, char *outstring, const char *instring )
{
	if ( m_pWatchField != m_pCurrentField )
		return;

	WatchMsg( "string (%s)", outstring );
}

void CPredictionCopy::DescribeVector( difftype_t dt, Vector& outValue, const Vector &inValue )
{
	if ( !m_bErrorCheck )
		return;

	if ( dt == DIFFERS )
	{
		Vector delta = outValue - inValue;

		ReportFieldsDiffer( "vec differs (net %f %f %f - pred %f %f %f) delta(%f %f %f)\n", 
			inValue.x, inValue.y, inValue.z,
			outValue.x, outValue.y, outValue.z,
			delta.x, delta.y, delta.z );
	}

	DescribeFields( dt, "vector (%f %f %f)\n", 
				outValue.x, outValue.y, outValue.z );
}

void CPredictionCopy::DescribeVector( difftype_t dt, Vector* outValue, const Vector *inValue, int count )
{
	if ( !m_bErrorCheck )
		return;

	if ( dt == DIFFERS )
	{
		int i = 0;
		Vector delta = outValue[ i ] - inValue[ i ];

		ReportFieldsDiffer( "vec[] differs (1st diff) (net %f %f %f - pred %f %f %f) delta(%f %f %f)\n", 
			inValue[i].x, inValue[i].y, inValue[i].z,
			outValue[i].x, outValue[i].y, outValue[i].z,
			delta.x, delta.y, delta.z );
	}

	DescribeFields( dt, "vector (%f %f %f)\n", 
					outValue[0].x, outValue[0].y, outValue[0].z );
}

void CPredictionCopy::WatchVector( difftype_t dt, Vector& outValue, const Vector &inValue )
{
	if ( m_pWatchField != m_pCurrentField )
		return;

	WatchMsg( "vector (%f %f %f)", outValue.x, outValue.y, outValue.z );
}

void CPredictionCopy::WatchVector( difftype_t dt, Vector* outValue, const Vector *inValue, int count )
{
	if ( m_pWatchField != m_pCurrentField )
		return;

	WatchMsg( "vector (%f %f %f)", outValue[0].x, outValue[0].y, outValue[0].z );
}



void CPredictionCopy::DescribeQuaternion( difftype_t dt, Quaternion& outValue, const Quaternion &inValue )
{
	if ( !m_bErrorCheck )
		return;

	if ( dt == DIFFERS )
	{
		Quaternion delta;
		
		for ( int i = 0; i < 4; i++ )
		{
			delta[i] = outValue[i] - inValue[i];
		}

		ReportFieldsDiffer( "quaternion differs (net %f %f %f %f - pred %f %f %f %f) delta(%f %f %f %f)\n", 
			inValue[0], inValue[1], inValue[2], inValue[3],
			outValue[0], outValue[1], outValue[2], outValue[3],
			delta[0], delta[1], delta[2], delta[3] );
	}

	DescribeFields( dt, "quaternion (%f %f %f %f)\n", 
				outValue[0], outValue[1], outValue[2], outValue[3] );
}

void CPredictionCopy::DescribeQuaternion( difftype_t dt, Quaternion* outValue, const Quaternion *inValue, int count )
{
	if ( !m_bErrorCheck )
		return;

	if ( dt == DIFFERS )
	{
		int i = 0;
		Quaternion delta;

		for ( int j = 0; j < 4; j++ )
		{
			delta[j] = outValue[i][j] - inValue[i][j];
		}

		ReportFieldsDiffer( "quaternion[] differs (1st diff) (net %f %f %f %f - pred %f %f %f %f) delta(%f %f %f %f)\n", 
			(float)inValue[i][0], (float)inValue[i][1], (float)inValue[i][2], (float)inValue[i][3],
			(float)outValue[i][0], (float)outValue[i][1], (float)outValue[i][2], (float)outValue[i][3],
			delta[0], delta[1], delta[2], delta[3] );
	}

	DescribeFields( dt, "quaternion (%f %f %f %f)\n", 
					(float)outValue[0][0], (float)outValue[0][1], (float)outValue[0][2], (float)outValue[0][3] );
}

void CPredictionCopy::WatchQuaternion( difftype_t dt, Quaternion& outValue, const Quaternion &inValue )
{
	if ( m_pWatchField != m_pCurrentField )
		return;

	WatchMsg( "quaternion (%f %f %f %f)", (float)outValue[0], (float)outValue[1], (float)outValue[2], (float)outValue[3] );
}

void CPredictionCopy::WatchQuaternion( difftype_t dt, Quaternion* outValue, const Quaternion *inValue, int count )
{
	if ( m_pWatchField != m_pCurrentField )
		return;

	WatchMsg( "quaternion (%f %f %f %f)", outValue[0][0], outValue[0][1], outValue[0][2], outValue[0][3] );
}



void CPredictionCopy::DescribeEHandle( difftype_t dt, EHANDLE *outvalue, EHANDLE const *invalue, int count )
{
	if ( !m_bErrorCheck )
		return;

	if ( dt == DIFFERS )
	{
		int i = 0;
		ReportFieldsDiffer( "EHandles differ (net) 0x%p (pred) 0x%p\n", (void const *)invalue[ i ].Get(), (void *)outvalue[ i ].Get() );
	}

#if defined( CLIENT_DLL )
	C_BaseEntity *ent = outvalue[0].Get();
	if ( ent )
	{
		const char *classname = ent->GetClassname();
		if ( !classname[0] )
		{
			classname = typeid( *ent ).name();
		}

		DescribeFields( dt, "EHandle (0x%p->%s)", (void *)outvalue[ 0 ], classname );
	}
	else
	{
		DescribeFields( dt, "EHandle (NULL)" );
	}

#else
	DescribeFields( dt, "EHandle (0x%p)", (void *)outvalue[ 0 ] );
#endif

}

void CPredictionCopy::WatchEHandle( difftype_t dt, EHANDLE *outvalue, EHANDLE const *invalue, int count )
{
	if ( m_pWatchField != m_pCurrentField )
		return;

#if defined( CLIENT_DLL )
	C_BaseEntity *ent = outvalue[0].Get();
	if ( ent )
	{
		const char *classname = ent->GetClassname();
		if ( !classname[0] )
		{
			classname = typeid( *ent ).name();
		}

		WatchMsg( "EHandle (0x%p->%s)", (void *)outvalue[ 0 ], classname );
	}
	else
	{
		WatchMsg( "EHandle (NULL)" );
	}

#else
	WatchMsg( "EHandle (0x%p)", (void *)outvalue[ 0 ] );
#endif

}

void CPredictionCopy::CopyShort( difftype_t dt, short *outvalue, const short *invalue, int count )
{
	if ( !m_bPerformCopy )
		return;

	if ( dt == IDENTICAL )
		return;

	CopyData( dt, sizeof(short) * count, (char *)outvalue, (const char *)invalue ); 
}

CPredictionCopy::difftype_t CPredictionCopy::CompareShort( short *outvalue, const short *invalue, int count )
{
	if ( !m_bErrorCheck )
		return DIFFERS;

	if ( CanCheck() )
	{
		for ( int i = 0; i < count; i++ )
		{
			if ( outvalue[ i ] == invalue[ i ] )
				continue;

			return DIFFERS;
		}
	}

	return IDENTICAL;
}


void CPredictionCopy::CopyInt( difftype_t dt, int *outvalue, const int *invalue, int count )
{
	if ( !m_bPerformCopy )
		return;

	if ( dt == IDENTICAL )
		return;

	CopyData( dt, sizeof(int) * count, (char *)outvalue, (const char *)invalue );
}

CPredictionCopy::difftype_t CPredictionCopy::CompareInt( int *outvalue, const int *invalue, int count )
{
	if ( !m_bErrorCheck )
		return DIFFERS;

	difftype_t retval = IDENTICAL;

	if ( CanCheck() )
	{
		for ( int i = 0; i < count; i++ )
		{
			if ( outvalue[ i ] == invalue[ i ] )
				continue;

			if ( m_pCurrentField->flags & FTYPEDESC_ONLY_ERROR_IF_ABOVE_ZERO_TO_ZERO_OR_BELOW_ETC )
			{
				if ( ( outvalue[i] > 0 ) == ( invalue[i] > 0 ) )
				{
					retval = WITHINTOLERANCE;
					continue;
				}
			}

			ReportFieldsDiffer( "int differs (net %i pred %i) diff(%i)\n", invalue[i], outvalue[i], outvalue[i] - invalue[i] );
			return DIFFERS;
		}
	}

	return retval;
}

void CPredictionCopy::CopyBool( difftype_t dt, bool *outvalue, const bool *invalue, int count )
{
	if ( !m_bPerformCopy )
		return;

	if ( dt == IDENTICAL )
		return;

	CopyData( dt, sizeof( bool ) * count, (char *)outvalue, (const char *)invalue );
}

CPredictionCopy::difftype_t CPredictionCopy::CompareBool( bool *outvalue, const bool *invalue, int count )
{
	if ( !m_bErrorCheck )
		return DIFFERS;

	if ( CanCheck() )
	{
		for ( int i = 0; i < count; i++ )
		{
			if ( outvalue[ i ] == invalue[ i ] )
				continue;

			return DIFFERS;
		}
	}

	return IDENTICAL;
}

void CPredictionCopy::CopyFloat( difftype_t dt, float *outvalue, const float *invalue, int count )
{
	if ( !m_bPerformCopy )
		return;

	if ( dt == IDENTICAL )
		return;

	CopyData( dt, sizeof( float ) * count, (char *)outvalue, (const char *)invalue );
}

CPredictionCopy::difftype_t CPredictionCopy::CompareFloat( float *outvalue, const float *invalue, int count )
{
	if ( !m_bErrorCheck )
		return DIFFERS;

	difftype_t retval = IDENTICAL;

	if ( CanCheck() )
	{
		float tolerance = m_pCurrentField->fieldTolerance;
		Assert( tolerance >= 0.0f );
		bool usetolerance = tolerance > 0.0f;

		for ( int i = 0; i < count; i++ )
		{
			if ( outvalue[ i ] == invalue[ i ] )
				continue;

			if ( m_pCurrentField->flags & FTYPEDESC_ONLY_ERROR_IF_ABOVE_ZERO_TO_ZERO_OR_BELOW_ETC )
			{
				if ( ( outvalue[i] > 0.0f ) == ( invalue[i] > 0.0f ) )
				{
					retval = WITHINTOLERANCE;
					continue;
				}
			}

			if ( usetolerance &&
				( fabs( outvalue[ i ] - invalue[ i ] ) <= tolerance ) )
			{
				retval = WITHINTOLERANCE;
				continue;
			}

			return DIFFERS;
		}
	}
	
	return retval;
}

void CPredictionCopy::CopyString( difftype_t dt, char *outstring, const char *instring )
{
	if ( !m_bPerformCopy )
		return;

	if ( dt == IDENTICAL )
		return;

	CopyData( dt, Q_strlen( instring ) + 1, (char *)outstring, (const char *)instring );
}

CPredictionCopy::difftype_t CPredictionCopy::CompareString( char *outstring, const char *instring )
{
	if ( !m_bErrorCheck )
		return DIFFERS;

	if ( CanCheck() )
	{
		if ( Q_strcmp( outstring, instring ) )
		{
			return DIFFERS;
		}
	}

	return IDENTICAL;
}

void CPredictionCopy::CopyVector( difftype_t dt, Vector& outValue, const Vector &inValue )
{
	CopyVector( dt, &outValue, &inValue, 1 );
}

void CPredictionCopy::CopyQuaternion( difftype_t dt, Quaternion& outValue, const Quaternion &inValue )
{
	CopyQuaternion( dt, &outValue, &inValue, 1 );
}

CPredictionCopy::difftype_t CPredictionCopy::CompareVector( Vector& outValue, const Vector &inValue )
{
	if ( !m_bErrorCheck )
		return DIFFERS;

	if ( CanCheck() )
	{
		float tolerance = m_pCurrentField->fieldTolerance;
		Assert( tolerance >= 0.0f );

		if ( outValue != inValue && ( tolerance > 0.0f ) )
		{
			Vector delta = outValue - inValue;

			if ( fabs( delta.x ) <= tolerance &&
				 fabs( delta.y ) <= tolerance &&
				 fabs( delta.z ) <= tolerance )
			{
				return WITHINTOLERANCE;
			}
		}

		return DIFFERS;
	}

	return IDENTICAL;
}

static int QuaternionCompare (const Quaternion& q1, const Quaternion& q2 )
{
	for ( int i = 0; i < 4; i++ )
	{
		if ( q1[i] != q2[i] )
			return 0;
	}

	return 1;
}

CPredictionCopy::difftype_t CPredictionCopy::CompareQuaternion( Quaternion& outValue, const Quaternion &inValue )
{
	if ( !m_bErrorCheck )
		return DIFFERS;

	if ( CanCheck() )
	{
		float tolerance = m_pCurrentField->fieldTolerance;
		Assert( tolerance >= 0.0f );

		if ( QuaternionCompare( outValue, inValue ) == 0
			&& ( tolerance > 0.0f ) )
		{
			Quaternion delta;

			for ( int j = 0; j < 4; j++ )
			{
				delta[j] = outValue[j] - inValue[j];
			}

			if ( fabs( delta[0] ) <= tolerance &&
				 fabs( delta[1] ) <= tolerance &&
				 fabs( delta[2] ) <= tolerance &&
				 fabs( delta[3] ) <= tolerance )
			{
				return WITHINTOLERANCE;
			}
		}

		return DIFFERS;
	}

	return IDENTICAL;
}

void CPredictionCopy::CopyVector( difftype_t dt, Vector* outValue, const Vector *inValue, int count )
{
	if ( !m_bPerformCopy )
		return;

	if ( dt == IDENTICAL )
		return;

	CopyData( dt, sizeof( Vector ) * count, (char *)outValue, (const char *)inValue );
}

void CPredictionCopy::CopyQuaternion( difftype_t dt, Quaternion* outValue, const Quaternion *inValue, int count )
{
	if ( !m_bPerformCopy )
		return;

	if ( dt == IDENTICAL )
		return;

	CopyData( dt, sizeof( Quaternion ) * count, (char *)outValue, (const char *)inValue );
}


CPredictionCopy::difftype_t CPredictionCopy::CompareVector( Vector* outValue, const Vector *inValue, int count )
{
	if ( !m_bErrorCheck )
		return DIFFERS;

	difftype_t retval = IDENTICAL;

	if ( CanCheck() )
	{
		float tolerance = m_pCurrentField->fieldTolerance;
		Assert( tolerance >= 0.0f );

		for ( int i = 0; i < count; i++ )
		{
			if ( outValue[ i ] == inValue[ i ] )
				continue;

			Vector delta = outValue[ i ] - inValue[ i ];

			if ( tolerance > 0.0f )
			{
				if ( fabs( delta.x ) <= tolerance &&
					 fabs( delta.y ) <= tolerance &&
					 fabs( delta.z ) <= tolerance )
				{
					retval = WITHINTOLERANCE;
					continue;
				}
			}
			return DIFFERS;
		}
	}

	return retval;
}

CPredictionCopy::difftype_t CPredictionCopy::CompareQuaternion( Quaternion* outValue, const Quaternion *inValue, int count )
{
	if ( !m_bErrorCheck )
		return DIFFERS;

	difftype_t retval = IDENTICAL;

	if ( CanCheck() )
	{
		float tolerance = m_pCurrentField->fieldTolerance;
		Assert( tolerance >= 0.0f );

		for ( int i = 0; i < count; i++ )
		{
			if ( QuaternionCompare( outValue[ i ], inValue[ i ] ) )
				continue;

			Quaternion delta;

			for ( int j = 0; j < 4; j++ )
			{
				delta[j] = outValue[i][j] - inValue[i][j];
			}

			if ( tolerance > 0.0f )
			{
				if ( fabs( delta[0] ) <= tolerance &&
					 fabs( delta[1] ) <= tolerance &&
					 fabs( delta[2] ) <= tolerance &&
					 fabs( delta[3] ) <= tolerance )
				{
					retval = WITHINTOLERANCE;
					continue;
				}
			}
			return DIFFERS;
		}
	}

	return retval;
}

void CPredictionCopy::CopyEHandle( difftype_t dt, EHANDLE *outvalue, EHANDLE const *invalue, int count )
{
	if ( !m_bPerformCopy )
		return;

	if ( dt == IDENTICAL )
		return;

	for ( int i = 0; i < count; i++ )
	{
		outvalue[ i ] = invalue[ i ];
	}
}

CPredictionCopy::difftype_t CPredictionCopy::CompareEHandle( EHANDLE *outvalue, EHANDLE const *invalue, int count )
{
	if ( !m_bErrorCheck )
		return DIFFERS;

	int i;
	if ( CanCheck() )
	{
		for ( i = 0; i < count; i++ )
		{
			if ( outvalue[ i ].Get() == invalue[ i ].Get() )
				continue;

			return DIFFERS;
		}
	}

	return IDENTICAL;
}

void CPredictionCopy::CopyFields( int chain_count, datamap_t *pRootMap, typedescription_t *pFields, int fieldCount )
{
	int				i;
	int				flags;
	int				fieldOffsetSrc;
	int				fieldOffsetDest;
	int				fieldSize;

	m_pCurrentMap = pRootMap;
	if ( !m_pCurrentClassName )
	{
		m_pCurrentClassName = pRootMap->dataClassName;
	}

	for ( i = 0; i < fieldCount; i++ )
	{
		m_pCurrentField = &pFields[ i ];
		flags = m_pCurrentField->flags;

		// Mark any subchains first
		if ( m_pCurrentField->override_field != NULL )
		{
			m_pCurrentField->override_field->override_count = chain_count;
		}

		// Skip this field?
		if ( m_pCurrentField->override_count == chain_count )
		{
			continue;
		}

		// Always recurse into embeddeds
		if ( m_pCurrentField->fieldType != FIELD_EMBEDDED )
		{
			// Don't copy fields that are private to server or client
			if ( flags & FTYPEDESC_PRIVATE )
				continue;

			// For PC_NON_NETWORKED_ONLYs skip any fields that are present in the network send tables
			if ( m_nType == PC_NON_NETWORKED_ONLY && ( flags & FTYPEDESC_INSENDTABLE ) )
				continue;

			// For PC_NETWORKED_ONLYs skip any fields that are not present in the network send tables
			if ( m_nType == PC_NETWORKED_ONLY && !( flags & FTYPEDESC_INSENDTABLE ) )
				continue;
		}

		void *pOutputData;
		void const *pInputData;

		fieldOffsetDest = m_pCurrentField->fieldOffset[ m_nDestOffsetIndex ];
		fieldOffsetSrc	= m_pCurrentField->fieldOffset[ m_nSrcOffsetIndex ];
		fieldSize = m_pCurrentField->fieldSize;

		pOutputData = (void *)((char *)m_pDest + fieldOffsetDest );
		pInputData = (void const *)((char *)m_pSrc + fieldOffsetSrc );

		// Assume we can report
		m_bShouldReport = m_bReportErrors;
		m_bShouldDescribe = true;

		bool bShouldWatch = m_pWatchField == m_pCurrentField;

		difftype_t difftype;

		switch( m_pCurrentField->fieldType )
		{
		case FIELD_EMBEDDED:
			{
				typedescription_t *save = m_pCurrentField;
				void *saveDest = m_pDest;
				void const *saveSrc = m_pSrc;
				const char *saveName = m_pCurrentClassName;

				m_pCurrentClassName = m_pCurrentField->td->dataClassName;

				// FIXME: Should this be done outside the FIELD_EMBEDDED case??
				// Don't follow the pointer if we're reading from a compressed packet
				m_pSrc = pInputData;
				if ( ( flags & FTYPEDESC_PTR ) && (m_nSrcOffsetIndex == PC_DATA_NORMAL) )
				{
					m_pSrc = *((void**)m_pSrc);
				}

				m_pDest = pOutputData;
				if ( ( flags & FTYPEDESC_PTR ) && (m_nDestOffsetIndex == PC_DATA_NORMAL) )
				{
					m_pDest = *((void**)m_pDest);
				}

				CopyFields( chain_count, pRootMap, m_pCurrentField->td->dataDesc, m_pCurrentField->td->dataNumFields );

				m_pCurrentClassName = saveName;
				m_pCurrentField = save;
				m_pDest = saveDest;
				m_pSrc = saveSrc;
			}
			break;
		case FIELD_FLOAT:
			{
				difftype = CompareFloat( (float *)pOutputData, (float const *)pInputData, fieldSize );
				CopyFloat( difftype, (float *)pOutputData, (float const *)pInputData, fieldSize );
				if ( m_bErrorCheck && m_bShouldDescribe ) DescribeFloat( difftype, (float *)pOutputData, (float const *)pInputData, fieldSize );
				if ( bShouldWatch ) WatchFloat( difftype, (float *)pOutputData, (float const *)pInputData, fieldSize );
			}
			break;

		case FIELD_TIME:
		case FIELD_TICK:
			Assert( 0 );
			break;

		case FIELD_STRING:
			{
				difftype = CompareString( (char *)pOutputData, (char const*)pInputData );
				CopyString( difftype, (char *)pOutputData, (char const*)pInputData );
				if ( m_bErrorCheck && m_bShouldDescribe ) DescribeString( difftype,(char *)pOutputData, (char const*)pInputData );
				if ( bShouldWatch ) WatchString( difftype,(char *)pOutputData, (char const*)pInputData );
			}
			break;

		case FIELD_MODELINDEX:
			Assert( 0 );
			break;

		case FIELD_MODELNAME:
		case FIELD_SOUNDNAME:
			Assert( 0 );
			break;

		case FIELD_CUSTOM:
			Assert( 0 );
			break;

		case FIELD_CLASSPTR:
		case FIELD_EDICT:
			Assert( 0 );
			break;

		case FIELD_POSITION_VECTOR:
			Assert( 0 );
			break;

		case FIELD_VECTOR:
			{
				difftype = CompareVector( (Vector *)pOutputData, (Vector const *)pInputData, fieldSize );
				CopyVector( difftype, (Vector *)pOutputData, (Vector const *)pInputData, fieldSize );
				if ( m_bErrorCheck && m_bShouldDescribe ) DescribeVector( difftype, (Vector *)pOutputData, (Vector const *)pInputData, fieldSize );
				if ( bShouldWatch ) WatchVector( difftype, (Vector *)pOutputData, (Vector const *)pInputData, fieldSize );
			}
			break;

		case FIELD_QUATERNION:
			{
				difftype = CompareQuaternion( (Quaternion *)pOutputData, (Quaternion const *)pInputData, fieldSize );
				CopyQuaternion( difftype, (Quaternion *)pOutputData, (Quaternion const *)pInputData, fieldSize );
				if ( m_bErrorCheck && m_bShouldDescribe ) DescribeQuaternion( difftype, (Quaternion *)pOutputData, (Quaternion const *)pInputData, fieldSize );
				if ( bShouldWatch ) WatchQuaternion( difftype, (Quaternion *)pOutputData, (Quaternion const *)pInputData, fieldSize );
			}
			break;

		case FIELD_COLOR32:
			{
				difftype = CompareData( 4*fieldSize, (char *)pOutputData, (const char *)pInputData );
				CopyData( difftype, 4*fieldSize, (char *)pOutputData, (const char *)pInputData );
				if ( m_bErrorCheck && m_bShouldDescribe ) DescribeData( difftype, 4*fieldSize, (char *)pOutputData, (const char *)pInputData );
				if ( bShouldWatch ) WatchData( difftype, 4*fieldSize, (char *)pOutputData, (const char *)pInputData );
			}
			break;

		case FIELD_BOOLEAN:
			{
				difftype = CompareBool( (bool *)pOutputData, (bool const *)pInputData, fieldSize );
				CopyBool( difftype, (bool *)pOutputData, (bool const *)pInputData, fieldSize );
				if ( m_bErrorCheck && m_bShouldDescribe ) DescribeBool( difftype, (bool *)pOutputData, (bool const *)pInputData, fieldSize );
				if ( bShouldWatch ) WatchBool( difftype, (bool *)pOutputData, (bool const *)pInputData, fieldSize );
			}
			break;

		case FIELD_INTEGER:
			{
				difftype = CompareInt( (int *)pOutputData, (int const *)pInputData, fieldSize );
				CopyInt( difftype, (int *)pOutputData, (int const *)pInputData, fieldSize );
				if ( m_bErrorCheck && m_bShouldDescribe ) DescribeInt( difftype, (int *)pOutputData, (int const *)pInputData, fieldSize );
				if ( bShouldWatch ) WatchInt( difftype, (int *)pOutputData, (int const *)pInputData, fieldSize );
			}
			break;

		case FIELD_SHORT:
			{
				difftype = CompareShort( (short *)pOutputData, (short const *)pInputData, fieldSize );
				CopyShort( difftype, (short *)pOutputData, (short const *)pInputData, fieldSize );
				if ( m_bErrorCheck && m_bShouldDescribe ) DescribeShort( difftype, (short *)pOutputData, (short const *)pInputData, fieldSize );
				if ( bShouldWatch ) WatchShort( difftype, (short *)pOutputData, (short const *)pInputData, fieldSize );
			}
			break;

		case FIELD_CHARACTER:
			{
				difftype = CompareData( fieldSize, ((char *)pOutputData), (const char *)pInputData );
				CopyData( difftype, fieldSize, ((char *)pOutputData), (const char *)pInputData );
				
				int valOut = *((char *)pOutputData);
				int valIn  = *((const char *)pInputData);
				
				if ( m_bErrorCheck && m_bShouldDescribe ) DescribeInt( difftype, &valOut, &valIn, fieldSize );
				if ( bShouldWatch ) WatchData( difftype, fieldSize, ((char *)pOutputData), (const char *)pInputData );
			}
			break;
		case FIELD_EHANDLE:
			{
				difftype = CompareEHandle( (EHANDLE *)pOutputData, (EHANDLE const *)pInputData, fieldSize );
				CopyEHandle( difftype, (EHANDLE *)pOutputData, (EHANDLE const *)pInputData, fieldSize );
				if ( m_bErrorCheck && m_bShouldDescribe ) DescribeEHandle( difftype, (EHANDLE *)pOutputData, (EHANDLE const *)pInputData, fieldSize );
				if ( bShouldWatch ) WatchEHandle( difftype, (EHANDLE *)pOutputData, (EHANDLE const *)pInputData, fieldSize );
			}
			break;
		case FIELD_FUNCTION:
			{
			Assert( 0 );
			}
			break;
		case FIELD_VOID:
			{
				// Don't do anything, it's an empty data description
			}
			break;
		default:
			{
				Warning( "Bad field type\n" );
				Assert(0);
			}
			break;
		}
	}

	m_pCurrentClassName = NULL;
}

void CPredictionCopy::TransferData_R( int chaincount, datamap_t *dmap )
{
	// Copy from here first, then baseclasses
	CopyFields( chaincount, dmap, dmap->dataDesc, dmap->dataNumFields );

	if ( dmap->baseMap )
	{
		TransferData_R( chaincount, dmap->baseMap );
	}
}

static int g_nChainCount = 1;

static typedescription_t *FindFieldByName_R( const char *fieldname, datamap_t *dmap )
{
	int c = dmap->dataNumFields;
	for ( int i = 0; i < c; i++ )
	{
		typedescription_t *td = &dmap->dataDesc[ i ];
		if ( td->fieldType == FIELD_VOID )
			continue;

		if ( td->fieldType == FIELD_EMBEDDED )
		{
			// TODO:  this will only find the first subclass with the variable of the specified name
			//  At some point we might want to support multiple levels of overriding automatically
			typedescription_t *ret = FindFieldByName_R( fieldname, td->td );
			if ( ret )
			{
				return ret;
			}
		}

		if ( !V_stricmp( td->fieldName, fieldname ) )
		{
			return td;
		}
	}

	if ( dmap->baseMap )
	{
		return FindFieldByName_R( fieldname, dmap->baseMap );
	}
	return NULL;
}

void ValidateChains_R( datamap_t *dmap )
{
	dmap->chains_validated = true;

	int c = dmap->dataNumFields;
	for ( int i = 0; i < c; i++ )
	{
		typedescription_t *td = &dmap->dataDesc[ i ];
		if ( td->fieldType == FIELD_VOID )
			continue;

		if ( td->fieldType == FIELD_EMBEDDED )
		{
			ValidateChains_R( td->td );
			continue;
		}

		if ( !( td->flags & FTYPEDESC_OVERRIDE ) )
			continue;

		if ( dmap->baseMap )
		{
			typedescription_t *basefield = FindFieldByName_R( td->fieldName, dmap->baseMap );
			if ( basefield )
			{
				td->override_field = basefield;
			}
		}
	}

	if ( dmap->baseMap )
	{
		if ( !dmap->baseMap->chains_validated )
		{
			ValidateChains_R( dmap->baseMap );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fieldname - 
//			*dmap - 
// Output : typedescription_t
//-----------------------------------------------------------------------------
typedescription_t *FindFieldByName( const char *fieldname, datamap_t *dmap )
{
	return FindFieldByName_R( fieldname, dmap );
}

static ConVar pwatchent( "pwatchent", "-1", FCVAR_CHEAT, "Entity to watch for prediction system changes." );
static ConVar pwatchvar( "pwatchvar", "", FCVAR_CHEAT, "Entity variable to watch in prediction system for changes." );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fmt - 
//			... - 
//-----------------------------------------------------------------------------
void CPredictionCopy::WatchMsg( const char *fmt, ... )
{
	Assert( m_pCurrentField && (m_pCurrentField == m_pWatchField) );
	Assert( m_pOperation );

	va_list argptr;
	char data[ 4096 ];
	int len;
	va_start(argptr, fmt);
	len = Q_vsnprintf(data, sizeof( data ), fmt, argptr);
	va_end(argptr);

	Msg( "%i %s %s : %s\n", gpGlobals->tickcount, m_pOperation, m_pCurrentField->fieldName, data );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *operation - 
//			entindex - 
//			*dmap - 
//-----------------------------------------------------------------------------
void CPredictionCopy::DetermineWatchField( const char *operation, int entindex,  datamap_t *dmap )
{
	m_pWatchField = NULL;
	m_pOperation = operation;
	if ( !m_pOperation || !m_pOperation[0] )
		return;

	int enttowatch = pwatchent.GetInt();
	if ( enttowatch < 0 )
		return;

	if ( entindex != enttowatch )
		return;

	// See if they specified a field
	if ( pwatchvar.GetString()[0] == 0 )
		return;

	m_pWatchField = FindFieldByName( pwatchvar.GetString(), dmap );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *operation - 
//			entindex - 
//			*dmap - 
// Output : int
//-----------------------------------------------------------------------------
int CPredictionCopy::TransferData( const char *operation, int entindex, datamap_t *dmap )
{
	++g_nChainCount;

	if ( !dmap->chains_validated )
	{
		ValidateChains_R( dmap );
	}
	
	DetermineWatchField( operation, entindex, dmap );

	TransferData_R( g_nChainCount, dmap );

	return m_nErrorCount;
}

/*
//-----------------------------------------------------------------------------
// Purpose: Simply dumps all data fields in object
//-----------------------------------------------------------------------------
class CPredictionDescribeData
{
public:
	CPredictionDescribeData( void const *src );

	void	DescribeShort( const short *invalue, int count );
	void	DescribeInt( const int *invalue, int count );		
	void	DescribeBool( const bool *invalue, int count );	
	void	DescribeFloat( const float *invalue, int count );	
	void	DescribeData( int size, const char *indata );		
	void	DescribeString( const char *instring );			
	void	DescribeVector( const Vector &inValue );
	void	DescribeVector( const Vector *inValue, int count );
	void	DescribeEHandle( EHANDLE const *invalue, int count );

	void	DescribeFields( datamap_t *pMap, typedescription_t *pFields, int fieldCount );

private:

	void const		*m_pSrc;
	void			Describe( const char *fmt, ... );

	typedescription_t *m_pCurrentField;
	char const		*m_pCurrentClassName;
	datamap_t		*m_pCurrentMap;
};
*/

CPredictionDescribeData::CPredictionDescribeData( void const *src, bool src_packed, FN_FIELD_DESCRIPTION func /*= 0*/ )
{
	m_pSrc				= src;
	m_nSrcOffsetIndex	= src_packed ? TD_OFFSET_PACKED : TD_OFFSET_NORMAL;

	m_pCurrentField		= NULL;
	m_pCurrentMap		= NULL;
	m_pCurrentClassName = NULL;
	m_bShouldReport		= false;

	m_FieldDescFunc		= func;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fmt - 
//			... - 
//-----------------------------------------------------------------------------
void CPredictionDescribeData::Describe( const char *fmt, ... )
{
//	if ( !m_bShouldReport )
//		return;

	Assert( m_pCurrentMap );
	Assert( m_pCurrentClassName );

	const char *fieldname = "empty";
	int flags = 0;

	if ( m_pCurrentField )
	{
		flags		= m_pCurrentField->flags;
		fieldname	= m_pCurrentField->fieldName ? m_pCurrentField->fieldName : "NULL";
	}

	va_list argptr;
	char data[ 4096 ];
	int len;
	va_start(argptr, fmt);
	len = Q_vsnprintf(data, sizeof( data ), fmt, argptr);
	va_end(argptr);

	bool isprivate = ( flags & FTYPEDESC_PRIVATE ) ? true : false;
	bool isnetworked = ( flags & FTYPEDESC_INSENDTABLE ) ? true : false;

	if ( m_FieldDescFunc )
	{
		(*m_FieldDescFunc)( 
			m_pCurrentClassName, 
			m_pCurrentField->fieldName, 
			g_FieldTypes[ m_pCurrentField->fieldType ],
			isnetworked,
			data );
	}
	else
	{
		char suffix[ 128 ];

		suffix[ 0 ] = 0;
		if ( isprivate )
		{
			Q_strncat( suffix, "private", sizeof( suffix ), COPY_ALL_CHARACTERS );
		}
		if ( isnetworked )
		{
			if ( suffix[ 0 ] )
			{
				Q_strncat( suffix, " - ", sizeof( suffix ), COPY_ALL_CHARACTERS );
			}
			Q_strncat( suffix, "net", sizeof( suffix ), COPY_ALL_CHARACTERS );
		}

		if ( suffix[ 0 ] )
		{
			Msg( "%s::%s(%s) - %s",
				m_pCurrentClassName,
				fieldname,
				suffix,
				data );
		}
		else
		{
			Msg( "%s::%s - %s",
				m_pCurrentClassName,
				fieldname,
				data );
		}
	}

	m_bShouldReport = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : size - 
//			*outdata - 
//			*indata - 
//-----------------------------------------------------------------------------
void CPredictionDescribeData::DescribeData( int size, const char *indata )
{
	if ( !indata )
		return;

	Describe( "binary (%i bytes)\n", size );
}


void CPredictionDescribeData::DescribeShort( const short *invalue, int count )
{
	Describe( "short (%i)\n", (int)(invalue[0]) );
}


void CPredictionDescribeData::DescribeInt( const int *invalue, int count )
{
	for ( int i = 0; i < count; ++i )
	{
		Describe( "[%i] integer (%i)\n", i, invalue[i] );
	}
}

void CPredictionDescribeData::DescribeBool( const bool *invalue, int count )
{
	Describe( "bool (%s)\n", (invalue[0]) ? "true" : "false" );
}

void CPredictionDescribeData::DescribeFloat( const float *invalue, int count )
{
	Describe( "float (%f)\n", invalue[ 0 ] );
}

void CPredictionDescribeData::DescribeString( const char *instring )
{
	Describe( "string (%s)\n", instring );
}

void CPredictionDescribeData::DescribeVector( const Vector &inValue )
{
	Describe( "vector (%f %f %f)\n", 
				inValue.x, inValue.y, inValue.z );
}


void CPredictionDescribeData::DescribeVector( const Vector *inValue, int count )
{
	Describe( "vector (%f %f %f)\n", 
					inValue[0].x, inValue[0].y, inValue[0].z );
}

void CPredictionDescribeData::DescribeQuaternion( const Quaternion &inValue )
{
	Describe( "quaternion (%f %f %f %f)\n", 
				inValue[0], inValue[1], inValue[2], inValue[3] );
}


void CPredictionDescribeData::DescribeQuaternion( const Quaternion *inValue, int count )
{
	Describe( "quaternion (%f %f %f %f)\n", 
				inValue[0][0], inValue[0][1], inValue[0][2], inValue[0][3] );
}

void CPredictionDescribeData::DescribeEHandle( EHANDLE const *invalue, int count )
{
	Describe( "EHandle (%p)\n", (void *)invalue[ 0 ] );
}

void CPredictionDescribeData::DescribeFields_R( int chain_count, datamap_t *pRootMap, typedescription_t *pFields, int fieldCount )
{
	int				i;
	int				flags;
	int				fieldOffsetSrc;
	int				fieldSize;

	m_pCurrentMap = pRootMap;
	if ( !m_pCurrentClassName )
	{
		m_pCurrentClassName = pRootMap->dataClassName;
	}

	for ( i = 0; i < fieldCount; i++ )
	{
		m_pCurrentField = &pFields[ i ];
		flags = m_pCurrentField->flags;
		
		// Mark any subchains first
		if ( m_pCurrentField->override_field != NULL )
		{
			m_pCurrentField->override_field->override_count = chain_count;
		}

		// Skip this field?
		if ( m_pCurrentField->override_count == chain_count )
		{
			continue;
		}

		void const *pInputData;
		
		fieldOffsetSrc = m_pCurrentField->fieldOffset[ m_nSrcOffsetIndex ];
		fieldSize = m_pCurrentField->fieldSize;
		
		pInputData = (void const *)((char *)m_pSrc + fieldOffsetSrc );
		
		// Assume we can report
		m_bShouldReport = true;
		
		switch( m_pCurrentField->fieldType )
		{
		case FIELD_EMBEDDED:
			{
				typedescription_t *save = m_pCurrentField;
				void const *saveSrc = m_pSrc;
				const char *saveName = m_pCurrentClassName;
				
				m_pCurrentClassName = m_pCurrentField->td->dataClassName;
				
				m_pSrc = pInputData;
				if ( ( flags & FTYPEDESC_PTR ) && (m_nSrcOffsetIndex == PC_DATA_NORMAL) )
				{
					m_pSrc = *((void**)m_pSrc);
				}

				DescribeFields_R( chain_count, pRootMap, m_pCurrentField->td->dataDesc, m_pCurrentField->td->dataNumFields );
				
				m_pCurrentClassName = saveName;
				m_pCurrentField = save;
				m_pSrc = saveSrc;
			}
			break;
		case FIELD_FLOAT:
			DescribeFloat( (float const *)pInputData, fieldSize );
			break;
		case FIELD_TIME:
		case FIELD_TICK:
			Assert( 0 );
			break;
		case FIELD_STRING:
			DescribeString( (char const*)pInputData );
			break;
			
		case FIELD_MODELNAME:
		case FIELD_SOUNDNAME:
			Assert( 0 );
			break;
			
		case FIELD_MODELINDEX:
			Assert( 0 );
			break;

		case FIELD_CUSTOM:
			Assert( 0 );
			break;
			
		case FIELD_CLASSPTR:
		case FIELD_EDICT:
			break;
		case FIELD_POSITION_VECTOR:
			Assert( 0 );
			break;
		case FIELD_VECTOR:
			DescribeVector( (const Vector *)pInputData, fieldSize );
			break;
		case FIELD_QUATERNION:
			DescribeQuaternion( ( const Quaternion * )pInputData, fieldSize );
			break;
			
		case FIELD_COLOR32:
			DescribeData( 4*fieldSize, (const char *)pInputData );
			break;
			
		case FIELD_BOOLEAN:
			DescribeBool( (bool const *)pInputData, fieldSize );
			break;
		case FIELD_INTEGER:
			DescribeInt( (int const *)pInputData, fieldSize );
			break;
			
		case FIELD_SHORT:
			DescribeShort( (short const *)pInputData, fieldSize );
			break;
			
		case FIELD_CHARACTER:
			DescribeData( fieldSize, (const char *)pInputData );
			break;
			
		case FIELD_EHANDLE:
			DescribeEHandle( (EHANDLE const *)pInputData, fieldSize );
			break;
		case FIELD_FUNCTION:
			Assert( 0 );
			break;
		case FIELD_VOID:
			Describe( "FIELD_VOID: empty field\n" );
			break;
		default:
			Warning( "Bad field type\n" );
			Assert(0);
			break;
		}
	}

	m_pCurrentClassName = NULL;
}

void CPredictionDescribeData::DumpDescription( datamap_t *pMap )
{
	++g_nChainCount;

	if ( !pMap->chains_validated )
	{
		ValidateChains_R( pMap );
	}

	while ( pMap )
	{
        DescribeFields_R( g_nChainCount, pMap, pMap->dataDesc, pMap->dataNumFields );
		pMap = pMap->baseMap;
	}
}

#if defined( CLIENT_DLL )
CValueChangeTracker::CValueChangeTracker() :
	m_bActive( false ),
	m_bTracking( false )
{
	Q_memset( m_OrigValueBuf, 0, sizeof( m_OrigValueBuf ) );
}

C_BaseEntity *CValueChangeTracker::GetEntity()
{
	return m_hEntityToTrack.Get();
}

void CValueChangeTracker::GetValue( char *buf, size_t bufsize )
{
	buf[ 0 ] = 0;

	Assert( IsActive() );

	if ( !m_hEntityToTrack.Get() )
		return;

	void const *pInputData = ( const void * )m_hEntityToTrack.Get();
	typedescription_t *td = NULL;
	for ( int i = 0; i < m_FieldStack.Count(); ++i )
	{
		td = m_FieldStack[ i ];
		Assert( ( i == ( m_FieldStack.Count() -1 ) ) || 
			( td->fieldType & FIELD_EMBEDDED ) );
		int fieldOffsetSrc = td->fieldOffset[ TD_OFFSET_NORMAL ];
		const void *pSaveSrc = (const void *)( (char *)pInputData + fieldOffsetSrc );
		if ( ( td->flags & FTYPEDESC_PTR ) && 
			 ( td->fieldType & FIELD_EMBEDDED ) )
		{
			pInputData = *(const void **)pSaveSrc;
		}
		else
		{
			pInputData = (void const *)((char *)pSaveSrc );
		}
	}

	if ( !td || !pInputData )
		return;

	int fieldType = td->fieldType;

	switch( fieldType )
	{
	default:
	case FIELD_EMBEDDED:
	case FIELD_MODELNAME:
	case FIELD_SOUNDNAME:
	case FIELD_CUSTOM:
	case FIELD_CLASSPTR:
	case FIELD_EDICT:
	case FIELD_POSITION_VECTOR:
	case FIELD_VOID:
	case FIELD_FUNCTION:
		{
			Assert( 0 );
		}
		break;
	case FIELD_FLOAT:
	case FIELD_TIME:
		Q_snprintf( buf, bufsize, "%f", *(float const *)pInputData );
		break;
	case FIELD_STRING:
		Q_snprintf( buf, bufsize, "%s", (char const*)pInputData );
		break;
	case FIELD_VECTOR:
		{
			const Vector *pVec = (const Vector *)pInputData;
			Q_snprintf( buf, bufsize, "%f %f %f", pVec->x, pVec->y, pVec->z );
		}
		break;
	case FIELD_QUATERNION:
		{
			const Quaternion *p = ( const Quaternion * )pInputData;
			Q_snprintf( buf, bufsize, "%f %f %f %f", p->x, p->y, p->z, p->w );
		}
		break;

	case FIELD_COLOR32:
		{
			const Color *color = ( const Color * )pInputData;
			Q_snprintf( buf, bufsize, "%d %d %d %d", color->r(), color->g(), color->b(), color->a() );
		}
		break;

	case FIELD_BOOLEAN:
		Q_snprintf( buf, bufsize, "%s", (*(const bool *)pInputData) ? "true" : "false" );
		break;
	case FIELD_INTEGER:
	case FIELD_TICK:
	case FIELD_MODELINDEX:
		Q_snprintf( buf, bufsize, "%i", *(const int*)pInputData );
		break;

	case FIELD_SHORT:
		Q_snprintf( buf, bufsize, "%i", (int)*(const short*)pInputData );
		break;

	case FIELD_CHARACTER:
		Q_snprintf( buf, bufsize, "%c", *(const char *)pInputData );
		break;

	case FIELD_EHANDLE:
		Q_snprintf( buf, bufsize, "eh 0x%p", (void const *)((const EHANDLE *)pInputData)->Get() );
		break;
	}
}

void CValueChangeTracker::StartTrack( char const *pchContext )
{
	if ( !IsActive() )
		return;

	m_strContext = pchContext;

	// Grab current value into scratch buffer
	GetValue( m_OrigValueBuf, sizeof( m_OrigValueBuf ) );

	m_bTracking = true;
}

void CValueChangeTracker::EndTrack()
{
	if ( !IsActive() )
		return;

	if ( !m_bTracking )
		return;
	m_bTracking = false;

	char final[ eChangeTrackerBufSize ];
	GetValue( final, sizeof( final ) );

	CUtlString *history = &m_History[ m_History.AddToTail() ];
	if ( Q_stricmp( final, m_OrigValueBuf ) )
	{
		history->Set( CFmtStr( "+++ %-20.20s:  %s (was %s)", m_strContext.String(), final, m_OrigValueBuf ) );
	}
	else
	{
		history->Set( CFmtStr( "    %-20.20s:  %s", m_strContext.String(), final ) );
	}

	Msg( ":%s\n", history->String() );
}

void CValueChangeTracker::ClearTracking()
{
	m_bActive = false;
	m_bTracking = false;
	m_hEntityToTrack = NULL;
	m_strFieldName = "";
	m_History.RemoveAll();
	m_FieldStack.RemoveAll();
}

static bool FindFieldStackByName_R( const char *fieldname, datamap_t *dmap, CUtlVector< typedescription_t * >& stack )
{
	int c = dmap->dataNumFields;
	for ( int i = 0; i < c; i++ )
	{
		typedescription_t *td = &dmap->dataDesc[ i ];

		if ( td->fieldType == FIELD_VOID )
			continue;

		stack.AddToTail( td );

		if ( td->fieldType == FIELD_EMBEDDED )
		{
			// TODO:  this will only find the first subclass with the variable of the specified name
			//  At some point we might want to support multiple levels of overriding automatically
			bool ret = FindFieldStackByName_R( fieldname, td->td, stack );
			if ( ret )
			{
				return ret;
			}
		}

		if ( !Q_stricmp( td->fieldName, fieldname ) )
		{
			return true;
		}

		stack.FindAndRemove( td );
	}

	if ( dmap->baseMap )
	{
		return FindFieldStackByName_R( fieldname, dmap->baseMap, stack );
	}
	return false;
}

void CValueChangeTracker::SetupTracking( C_BaseEntity *ent, char const *pchFieldName )
{
	ClearTracking();

	// Find the field
	datamap_t *dmap = ent->GetPredDescMap();
	if ( !dmap )
	{
		Msg( "No prediction datamap_t for entity %d/%s\n", ent->index, ent->GetClassname() );
		return;
	}

	bool bFound = FindFieldStackByName_R( pchFieldName, dmap, m_FieldStack );
	if ( !bFound || !m_FieldStack.Count() )
	{
		Msg( "No field '%s' in datamap_t for entity %d/%s\n", pchFieldName, ent->index, ent->GetClassname() );
		return;
	}

	m_hEntityToTrack = ent;
	m_strFieldName = pchFieldName;
	m_bActive = true;
}

void CValueChangeTracker::Reset()
{
	m_History.RemoveAll();
}

bool CValueChangeTracker::IsActive() const
{
	return m_bActive;
}

void CValueChangeTracker::Spew()
{
	if ( IsActive() )
	{
		for ( int i = 0 ; i < m_History.Count(); ++i )
		{
			Msg( "%s\n", m_History[ i ].String() );
		}
	}

	Reset();
}

static CValueChangeTracker g_ChangeTracker;
CValueChangeTracker *g_pChangeTracker = &g_ChangeTracker;

CON_COMMAND_F( cl_pred_track, "<entindex> <fieldname>:  Track changes to entity index entindex, for field fieldname.", 0 )
{
	g_pChangeTracker->ClearTracking();

	if ( args.ArgC() != 3 )
	{
		Msg( "cl_pred_track <entindex> <fieldname>\n" );
		return;
	}

	int iEntIndex = Q_atoi( args[1] );

	C_BaseEntity *ent = cl_entitylist->GetBaseEntity( iEntIndex );
	if ( !ent )
	{
		Msg( "cl_pred_track:  Unknown ent index %d\n", iEntIndex );
		return;
   	}

	g_pChangeTracker->SetupTracking( ent, args[2] );
}

#endif

#if defined( CLIENT_DLL ) && defined( COPY_CHECK_STRESSTEST )

class CPredictionCopyTester : public IGameSystem
{
public:

	// Init, shutdown
	virtual void Init()
	{
		RunTests();
		Remove( this );
	}

	virtual void Shutdown() {}

	// Level init, shutdown
	virtual void LevelInit() {}
	// The level is shutdown in two parts
	virtual void LevelShutdownPreEntity() {}
	// Entities are deleted / released here...
	virtual void LevelShutdownPostEntity() {}
	// end of level shutdown

	// Called before rendering
	virtual void PreRender ( ) {}

	// Called after rendering
	virtual void PostRender() {}

	// Gets called each frame
	virtual void Update( float frametime ) {}

private:

	void RunTests( void );
};

IGameSystem* GetPredictionCopyTester( void )
{
	static CPredictionCopyTester s_PredictionCopyTesterSystem;
	return &s_PredictionCopyTesterSystem;
}

class CCopyTesterData
{
public:

	CCopyTesterData()
	{
		m_CharValue = 'a';
		m_ShortValue = (short)100;
		m_IntValue = (int)100;
		m_FloatValue = 1.0f;
		Q_strncpy( m_szValue, "primarydata", sizeof( m_szValue ) );
		m_Vector = Vector( 100, 100, 100 );
		m_Bool = false;
		m_Clr.r = m_Clr.g = m_Clr.b = m_Clr.a = 255;

		m_Ptr = (void *)0xfedcba98;
	//	m_hEHandle = NULL;
	}

	void MakeDifferent( void )
	{
		m_CharValue = 'd';
		m_ShortValue = (short)400;
		m_IntValue = (int)400;
		m_FloatValue = 4.0f;
		Q_strncpy( m_szValue, "secondarydata", sizeof( m_szValue ) );
		m_Vector = Vector( 400, 400, 400 );
		m_Bool = true;
		m_Clr.r = m_Clr.g = m_Clr.b = m_Clr.a = 1;
		m_Ptr = (void *)0x00000001;
	//	m_hEHandle = (C_BaseEntity *)0x00000001;
	}

	DECLARE_PREDICTABLE();

	char	m_CharValue;
	short	m_ShortValue;
	int		m_IntValue;
	float	m_FloatValue;
	char	m_szValue[ 128 ];
	Vector	m_Vector;
	bool	m_Bool;
	color32	m_Clr;
	void	*m_Ptr;
//	EHANDLE	m_hEHandle;

};

BEGIN_PREDICTION_DATA_NO_BASE( CCopyTesterData )

	DEFINE_FIELD( CCopyTesterData, m_CharValue, FIELD_CHARACTER ),
	DEFINE_FIELD( CCopyTesterData, m_ShortValue, FIELD_SHORT ),
	DEFINE_FIELD( CCopyTesterData, m_IntValue, FIELD_INTEGER ),
	DEFINE_FIELD( CCopyTesterData, m_FloatValue, FIELD_FLOAT ),
	DEFINE_FIELD( CCopyTesterData, m_szValue, FIELD_STRING ),
	DEFINE_FIELD( CCopyTesterData, m_Vector, FIELD_VECTOR ),
	DEFINE_FIELD( CCopyTesterData, m_Bool, FIELD_BOOLEAN ),
	DEFINE_FIELD( CCopyTesterData, m_Clr, FIELD_COLOR32 ),
//	DEFINE_FIELD( CCopyTesterData, m_hEHandle, FIELD_EHANDLE ),

END_PREDICTION_DATA()

class CCopyTesterData2 : public C_BaseEntity
{
	DECLARE_CLASS( CCopyTesterData2, C_BaseEntity );

public:
	CCopyTesterData2()
	{
		CONSTRUCT_PREDICTABLE( CCopyTesterData2 );

		m_CharValue = 'b';
		m_ShortValue = (short)200;
		m_IntValue = (int)200;
		m_FloatValue = 2.0f;
	}

	void MakeDifferent( void )
	{
		m_CharValue = 'e';
		m_ShortValue = (short)500;
		m_IntValue = (int)500;
		m_FloatValue = 5.0f;
		m_FooData.MakeDifferent();
	}

	DECLARE_PREDICTABLE();

	char	m_CharValue;
	short	m_ShortValue;
	int		m_IntValue;
	float	m_FloatValue;

	CCopyTesterData	m_FooData;
};

BEGIN_PREDICTION_DATA_NO_BASE( CCopyTesterData2 )

	DEFINE_FIELD( CCopyTesterData2, m_CharValue, FIELD_CHARACTER ),
	DEFINE_FIELD( CCopyTesterData2, m_ShortValue, FIELD_SHORT ),
	DEFINE_PRED_TYPEDESCRIPTION( CCopyTesterData2, m_FooData, CCopyTesterData ),
	DEFINE_FIELD( CCopyTesterData2, m_IntValue, FIELD_INTEGER ),
	DEFINE_FIELD( CCopyTesterData2, m_FloatValue, FIELD_FLOAT ),

END_PREDICTION_DATA()

void CPredictionCopyTester::RunTests( void )
{
	CCopyTesterData2 *foo1, *foo2, *foo3;

	foo1 = new CCopyTesterData2;
	foo2 = new CCopyTesterData2;
	foo3 = new CCopyTesterData2;

	foo2->MakeDifferent();


	{
		Msg( "Comparing and copying == objects, should have zero diffcount\n" );

		CPredictionCopy tester( PC_NON_NETWORKED_ONLY, foo1, false, foo3, false, true );
		int diff_count = 0;
		diff_count = tester.TransferData( foo3->GetPredDescMap(), foo1->GetPredDescMap()->dataDesc, foo1->GetPredDescMap()->dataNumFields );
		
		Msg( "diff_count == %i\n", diff_count );
		Assert( !diff_count );
	}

	{
		Msg( "Simple compare of != objects, should spew and have non-zero diffcount\n" );

		CPredictionCopy tester( PC_NON_NETWORKED_ONLY, foo1, false, foo2, false, true, false );
		int diff_count = 0;
		diff_count = tester.TransferData( foo2->GetPredDescMap(), foo1->GetPredDescMap()->dataDesc, foo1->GetPredDescMap()->dataNumFields );
		
		Msg( "diff_count == %i (should be 12)\n", diff_count );
		Assert( diff_count == 12 );
	}

	{
		Msg( "Comparing and coyping same != objects, should spew and have non-zero diffcount\n" );

		CPredictionCopy tester( PC_NON_NETWORKED_ONLY, foo1, false, foo2, false, true );
		int diff_count = 0;
		diff_count = tester.TransferData( foo2->GetPredDescMap(), foo1->GetPredDescMap()->dataDesc, foo1->GetPredDescMap()->dataNumFields );
		
		Msg( "diff_count == %i (should be 12)\n", diff_count );
		Assert( diff_count == 12 );
	}

	{
		Msg( "Comparing and copying objects which were just made to coincide, should have zero diffcount\n" );
	

		CPredictionCopy tester( PC_NON_NETWORKED_ONLY, foo1, false, foo2, false, true );
		int diff_count = 0;
		diff_count = tester.TransferData( foo2->GetPredDescMap(), foo1->GetPredDescMap()->dataDesc, foo1->GetPredDescMap()->dataNumFields );

		Msg( "diff_count == %i\n", diff_count );
		Assert( !diff_count );
	}

	{
		CPredictionDescribeData describe( foo1, false );
		describe.DumpDescription( foo1->GetPredDescMap(), foo1->GetPredDescMap()->dataDesc, foo1->GetPredDescMap()->dataNumFields );
	}
		
	delete foo3;
	delete foo2;
	delete foo1;

}

#endif // CLIENT_DLL
#endif // !NO_ENTITY_PREDICTION )
