//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#include "basetypes.h"
#include "tier1/strtools.h"
#include "interpolatortypes.h"
#include "tier0/dbg.h"
#include "mathlib/mathlib.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

struct InterpolatorNameMap_t
{
	int						type;
	char const				*name;
	char const				*printname;
};

static InterpolatorNameMap_t g_InterpolatorNameMap[] =
{
	{ INTERPOLATE_DEFAULT,						"default",					"Default" },
	{ INTERPOLATE_CATMULL_ROM_NORMALIZEX,		"catmullrom_normalize_x",	"Catmull-Rom (Norm X)" },
	{ INTERPOLATE_EASE_IN,						"easein",					"Ease In" },			
	{ INTERPOLATE_EASE_OUT,						"easeout",					"Ease Out" },
	{ INTERPOLATE_EASE_INOUT,					"easeinout",				"Ease In/Out" },			
	{ INTERPOLATE_BSPLINE,						"bspline",					"B-Spline" },							
	{ INTERPOLATE_LINEAR_INTERP,				"linear_interp",			"Linear Interp." },			
	{ INTERPOLATE_KOCHANEK_BARTELS,				"kochanek",					"Kochanek-Bartels" },		
	{ INTERPOLATE_KOCHANEK_BARTELS_EARLY,		"kochanek_early",			"Kochanek-Bartels Early" },	
	{ INTERPOLATE_KOCHANEK_BARTELS_LATE,		"kochanek_late",			"Kochanek-Bartels Late" },
	{ INTERPOLATE_SIMPLE_CUBIC,					"simple_cubic",				"Simple Cubic" },
	{ INTERPOLATE_CATMULL_ROM,					"catmullrom",				"Catmull-Rom" },
	{ INTERPOLATE_CATMULL_ROM_NORMALIZE,		"catmullrom_normalize",		"Catmull-Rom (Norm)" },
	{ INTERPOLATE_CATMULL_ROM_TANGENT,			"catmullrom_tangent",		"Catmull-Rom (Tangent)" },
	{ INTERPOLATE_EXPONENTIAL_DECAY,			"exponential_decay",		"Exponential Decay" },
	{ INTERPOLATE_HOLD,							"hold",						"Hold" },
};

int Interpolator_InterpolatorForName( char const *name )
{
	for ( int i = 0; i < NUM_INTERPOLATE_TYPES; ++i )
	{
		InterpolatorNameMap_t *slot = &g_InterpolatorNameMap[ i ];
		if ( !Q_stricmp( name, slot->name ) )
			return slot->type;
	}
	
	Assert( !"Interpolator_InterpolatorForName failed!!!" );
	return INTERPOLATE_DEFAULT;
}

char const *Interpolator_NameForInterpolator( int type, bool printname )
{
	int i = (int)type;
	int c = ARRAYSIZE( g_InterpolatorNameMap );
	if ( i < 0 || i >= c )
	{
		Assert( "!Interpolator_NameForInterpolator:  bogus type!" );
		// returns "unspecified!!!";
		return printname ? g_InterpolatorNameMap[ 0 ].printname : g_InterpolatorNameMap[ 0 ].name;
	}

	return printname ? g_InterpolatorNameMap[ i ].printname : g_InterpolatorNameMap[ i ].name;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct CurveNameMap_t
{
	int						type;
	int						hotkey;
};

static CurveNameMap_t g_CurveNameMap[] =
{
	{ CURVE_CATMULL_ROM_TO_CATMULL_ROM,							'1' },
	{ CURVE_EASE_IN_TO_EASE_OUT,								'2' },
	{ CURVE_EASE_IN_TO_EASE_IN,									'3' },
	{ CURVE_EASE_OUT_TO_EASE_OUT,								'4' },
	{ CURVE_BSPLINE_TO_BSPLINE,									'5' },
	{ CURVE_LINEAR_INTERP_TO_LINEAR_INTERP,						'6' },
	{ CURVE_KOCHANEK_BARTELS_TO_KOCHANEK_BARTELS,				'7' },
	{ CURVE_KOCHANEK_BARTELS_EARLY_TO_KOCHANEK_BARTELS_EARLY,	'8' },
	{ CURVE_KOCHANEK_BARTELS_LATE_TO_KOCHANEK_BARTELS_LATE,		'9' },
	{ CURVE_SIMPLE_CUBIC_TO_SIMPLE_CUBIC,						'0' },
};

// Turn enum into string and vice versa
int Interpolator_CurveTypeForName( const char *name )
{
	char sz[ 128 ];
	Q_strncpy( sz, name, sizeof( sz ) );

	int leftcurve = 0;
	int rightcurve = 0;

	int skip = Q_strlen( "curve_" );

	if ( !Q_strnicmp( sz, "curve_", skip ) )
	{
		char *p = sz + skip;
		char *second = Q_stristr( p, "_to_curve_" );
		
		char save = *second;
		*second = 0;

		leftcurve = Interpolator_InterpolatorForName( p );

		*second = save;

		p = second + Q_strlen( "_to_curve_" );

		rightcurve = Interpolator_InterpolatorForName( p );
	}

	return MAKE_CURVE_TYPE( leftcurve, rightcurve );
}

const char *Interpolator_NameForCurveType( int type, bool printname )
{
	static char outname[ 256 ];

	int leftside = GET_LEFT_CURVE( type );
	int rightside = GET_RIGHT_CURVE( type );

	if ( !printname )
	{
		Q_snprintf( outname, sizeof( outname ), "curve_%s_to_curve_%s", 
			Interpolator_NameForInterpolator( leftside, printname ),
			Interpolator_NameForInterpolator( rightside, printname ) );
	}
	else
	{
		Q_snprintf( outname, sizeof( outname ), "%s <-> %s", 
			Interpolator_NameForInterpolator( leftside, printname ),
			Interpolator_NameForInterpolator( rightside, printname ) );
	}

	return outname;
}

void Interpolator_CurveInterpolatorsForType( int type, int& inbound, int& outbound )
{
	inbound = GET_LEFT_CURVE( type );
	outbound = GET_RIGHT_CURVE( type );
}

int Interpolator_CurveTypeForHotkey( int key )
{
	int c = ARRAYSIZE( g_CurveNameMap );
	for ( int i = 0; i < c; ++i )
	{
		CurveNameMap_t *slot = &g_CurveNameMap[ i ];
		if ( slot->hotkey == key )
			return slot->type;
	}

	return -1;
}

void Interpolator_GetKochanekBartelsParams( int interpolationType, float& tension, float& bias, float& continuity )
{
	switch ( interpolationType )
	{
	default:
		tension = 0.0f;
		bias = 0.0f;
		continuity = 0.0f;
		Assert( 0 );
		break;
	case INTERPOLATE_KOCHANEK_BARTELS:
        tension		= 0.77f;
		bias		= 0.0f;
		continuity	= 0.77f;
		break;
	case INTERPOLATE_KOCHANEK_BARTELS_EARLY:
        tension		= 0.77f;
		bias		= -1.0f;
		continuity	= 0.77f;
		break;
	case INTERPOLATE_KOCHANEK_BARTELS_LATE:
        tension		= 0.77f;
		bias		= 1.0f;
		continuity	= 0.77f;
		break;
	}
}

void Interpolator_CurveInterpolate( int interpolationType,
	const Vector &vPre,
	const Vector &vStart,
	const Vector &vEnd,
	const Vector &vNext,
	float f,
	Vector &vOut )
{
	vOut.Init();

	switch ( interpolationType )
	{
	default:
		Warning( "Unknown interpolation type %d\n",
			(int)interpolationType );
		// break; // Fall through and use catmull_rom as default
	case INTERPOLATE_DEFAULT:
	case INTERPOLATE_CATMULL_ROM_NORMALIZEX:
		Catmull_Rom_Spline_NormalizeX( 
			vPre,
			vStart,
			vEnd,
			vNext,
			f, 
			vOut );
		break;
	case INTERPOLATE_CATMULL_ROM:
		Catmull_Rom_Spline( 
			vPre,
			vStart,
			vEnd,
			vNext,
			f, 
			vOut );
		break;
	case INTERPOLATE_CATMULL_ROM_NORMALIZE:
		Catmull_Rom_Spline_Normalize( 
			vPre,
			vStart,
			vEnd,
			vNext,
			f, 
			vOut );
		break;
	case INTERPOLATE_CATMULL_ROM_TANGENT:
		Catmull_Rom_Spline_Tangent( 
			vPre,
			vStart,
			vEnd,
			vNext,
			f, 
			vOut );
		break;
	case INTERPOLATE_EASE_IN:
		{
			f = sin( M_PI * f * 0.5f );
			// Fixme, since this ignores vPre and vNext we could omit computing them aove
			VectorLerp( vStart, vEnd, f, vOut );
		}
		break;
	case INTERPOLATE_EASE_OUT:
		{
			f = 1.0f - sin( M_PI * f * 0.5f + 0.5f * M_PI );
			// Fixme, since this ignores vPre and vNext we could omit computing them aove
			VectorLerp( vStart, vEnd, f, vOut );
		}
		break;
	case INTERPOLATE_EASE_INOUT:
		{
			f = SimpleSpline( f );
			// Fixme, since this ignores vPre and vNext we could omit computing them aove
			VectorLerp( vStart, vEnd, f, vOut );
		}
		break;
	case INTERPOLATE_LINEAR_INTERP:
		// Fixme, since this ignores vPre and vNext we could omit computing them aove
		VectorLerp( vStart, vEnd, f, vOut );
		break;
	case INTERPOLATE_KOCHANEK_BARTELS:
	case INTERPOLATE_KOCHANEK_BARTELS_EARLY:
	case INTERPOLATE_KOCHANEK_BARTELS_LATE:
		{
			float t, b, c;
			Interpolator_GetKochanekBartelsParams( interpolationType, t, b, c );
			Kochanek_Bartels_Spline_NormalizeX
			( 
				t, b, c, 
				vPre,
				vStart,
				vEnd,
				vNext,
				f, 
				vOut 
			);
		}
		break;
	case INTERPOLATE_SIMPLE_CUBIC:
		Cubic_Spline_NormalizeX( 
			vPre,
			vStart,
			vEnd,
			vNext,
			f, 
			vOut );
		break;
	case INTERPOLATE_BSPLINE:
		BSpline( 
			vPre,
			vStart,
			vEnd,
			vNext,
			f, 
			vOut );
		break;
	case INTERPOLATE_EXPONENTIAL_DECAY:
		{
			float dt = vEnd.x - vStart.x;
			if ( dt > 0.0f )
			{
				float val = 1.0f - ExponentialDecay( 0.001, dt, f * dt ); 
				vOut.y = vStart.y + val * ( vEnd.y - vStart.y );
			}
			else
			{
				vOut.y = vStart.y;
			}
		}
		break;
	case INTERPOLATE_HOLD:
		{
			vOut.y = vStart.y;
		}
		break;
	}
}

void Interpolator_CurveInterpolate_NonNormalized( int interpolationType,
	const Vector &vPre,
	const Vector &vStart,
	const Vector &vEnd,
	const Vector &vNext,
	float f,
	Vector &vOut )
{
	vOut.Init();

	switch ( interpolationType )
	{
	default:
		Warning( "Unknown interpolation type %d\n",
			(int)interpolationType );
		// break; // Fall through and use catmull_rom as default
	case INTERPOLATE_CATMULL_ROM_NORMALIZEX:
	case INTERPOLATE_DEFAULT:
	case INTERPOLATE_CATMULL_ROM:
	case INTERPOLATE_CATMULL_ROM_NORMALIZE:
	case INTERPOLATE_CATMULL_ROM_TANGENT:
		Catmull_Rom_Spline( 
			vPre,
			vStart,
			vEnd,
			vNext,
			f, 
			vOut );
		break;
	case INTERPOLATE_EASE_IN:
		{
			f = sin( M_PI * f * 0.5f );
			// Fixme, since this ignores vPre and vNext we could omit computing them aove
			VectorLerp( vStart, vEnd, f, vOut );
		}
		break;
	case INTERPOLATE_EASE_OUT:
		{
			f = 1.0f - sin( M_PI * f * 0.5f + 0.5f * M_PI );
			// Fixme, since this ignores vPre and vNext we could omit computing them aove
			VectorLerp( vStart, vEnd, f, vOut );
		}
		break;
	case INTERPOLATE_EASE_INOUT:
		{
			f = SimpleSpline( f );
			// Fixme, since this ignores vPre and vNext we could omit computing them aove
			VectorLerp( vStart, vEnd, f, vOut );
		}
		break;
	case INTERPOLATE_LINEAR_INTERP:
		// Fixme, since this ignores vPre and vNext we could omit computing them aove
		VectorLerp( vStart, vEnd, f, vOut );
		break;
	case INTERPOLATE_KOCHANEK_BARTELS:
	case INTERPOLATE_KOCHANEK_BARTELS_EARLY:
	case INTERPOLATE_KOCHANEK_BARTELS_LATE:
		{
			float t, b, c;
			Interpolator_GetKochanekBartelsParams( interpolationType, t, b, c );
			Kochanek_Bartels_Spline
			( 
				t, b, c, 
				vPre,
				vStart,
				vEnd,
				vNext,
				f, 
				vOut 
			);
		}
		break;
	case INTERPOLATE_SIMPLE_CUBIC:
		Cubic_Spline( 
			vPre,
			vStart,
			vEnd,
			vNext,
			f, 
			vOut );
		break;
	case INTERPOLATE_BSPLINE:
		BSpline( 
			vPre,
			vStart,
			vEnd,
			vNext,
			f, 
			vOut );
		break;
	case INTERPOLATE_EXPONENTIAL_DECAY:
		{
			float dt = vEnd.x - vStart.x;
			if ( dt > 0.0f )
			{
				float val = 1.0f - ExponentialDecay( 0.001, dt, f * dt ); 
				vOut.y = vStart.y + val * ( vEnd.y - vStart.y );
			}
			else
			{
				vOut.y = vStart.y;
			}
		}
		break;
	case INTERPOLATE_HOLD:
		{
			vOut.y = vStart.y;
		}
		break;
	}
}


void Interpolator_CurveInterpolate_NonNormalized( int interpolationType,
												 const Quaternion &vPre,
												 const Quaternion &vStart,
												 const Quaternion &vEnd,
												 const Quaternion &vNext,
												 float f,
												 Quaternion &vOut )
{
	vOut.Init();

	switch ( interpolationType )
	{
	default:
		Warning( "Unknown interpolation type %d\n",
			(int)interpolationType );
		// break; // Fall through and use catmull_rom as default
	case INTERPOLATE_CATMULL_ROM_NORMALIZEX:
	case INTERPOLATE_DEFAULT:
	case INTERPOLATE_CATMULL_ROM:
	case INTERPOLATE_CATMULL_ROM_NORMALIZE:
	case INTERPOLATE_CATMULL_ROM_TANGENT:
	case INTERPOLATE_KOCHANEK_BARTELS:
	case INTERPOLATE_KOCHANEK_BARTELS_EARLY:
	case INTERPOLATE_KOCHANEK_BARTELS_LATE:
	case INTERPOLATE_SIMPLE_CUBIC:
	case INTERPOLATE_BSPLINE:
		// FIXME, since this ignores vPre and vNext we could omit computing them aove
		QuaternionSlerp( vStart, vEnd, f, vOut );
		break;
	case INTERPOLATE_EASE_IN:
		{
			f = sin( M_PI * f * 0.5f );
			// Fixme, since this ignores vPre and vNext we could omit computing them aove
			QuaternionSlerp( vStart, vEnd, f, vOut );
		}
		break;
	case INTERPOLATE_EASE_OUT:
		{
			f = 1.0f - sin( M_PI * f * 0.5f + 0.5f * M_PI );
			// Fixme, since this ignores vPre and vNext we could omit computing them aove
			QuaternionSlerp( vStart, vEnd, f, vOut );
		}
		break;
	case INTERPOLATE_EASE_INOUT:
		{
			f = SimpleSpline( f );
			// Fixme, since this ignores vPre and vNext we could omit computing them aove
			QuaternionSlerp( vStart, vEnd, f, vOut );
		}
		break;
	case INTERPOLATE_LINEAR_INTERP:
		// Fixme, since this ignores vPre and vNext we could omit computing them aove
		QuaternionSlerp( vStart, vEnd, f, vOut );
		break;
	case INTERPOLATE_EXPONENTIAL_DECAY:
		vOut.Init();
		break;
	case INTERPOLATE_HOLD:
		{
			vOut = vStart;
		}
		break;
	}
}