//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

#include "color_projection_ps20.inc"
#include "color_projection_vs20.inc"

#include "../materialsystem_global.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar *mat_color_projection = NULL;

typedef struct SProjectionInfo
{
	bool	m_bNeedBlindMK;
	bool	m_bNeedMonochrome;
	bool	m_bNeedAnomylize;
	float	m_flCPU;
	float	m_flCPV;
	float	m_flAM;
	float	m_flAYI;
} TProjectionInfo;

#define MAX_PROJECTIONS	8

TProjectionInfo ProjectionInfo[ MAX_PROJECTIONS ] =
{
	{ true,		false,	false,	0.735f,		0.265f,		1.273463f,	-0.073894f },		// protanopia red-green blindness (no red cones)
	{ true,		false,	false,	1.14f,		-0.14f,		0.968437f,	0.003331f },		// deutanopia red-green blindness (no green cones)
	{ true,		false,	false,	0.171f,		-0.003f,	0.062921f,	0.292119f },		// tritanopia blue-yellow blindness (no blue cones)
	{ false,	true,	false,	0.0f,		0.0f,		0.0f,		0.0f },				// typical achromatopsia (no cones; rod monochromat)
	{ true,		false,	true,	0.735f,		0.265f,		1.273463f,	-0.073894f },		// protanomaly (anomalous red cones)
	{ true,		false,	true,	1.14f,		-0.14f,		0.968437f,	0.003331f },		// deutanomaly (anomalous green cones)
	{ true,		false,	true,	0.171f,		-0.003f,	0.062921f,	0.292119f },		// tritanomaly (anomalous blue cones)
	{ false,	true,	true,	0.0f,		0.0f,		0.0f,		0.0f }				// atypical achromatopsia (low cones; cone monochromat)
};

#if 0

#define cpu									ProjectionInfo[ 2 ].m_flCPU
#define cpv									ProjectionInfo[ 2 ].m_flCPV
#define am									ProjectionInfo[ 2 ].m_flAM
#define ayi									ProjectionInfo[ 2 ].m_flAYI


Vector rgb_from_xyz( Vector vNum )
{
	Vector vResult;

	vResult.x=( 3.063218*vNum.x-1.393325*vNum.y-0.475802*vNum.z);
	vResult.y=(-0.969243*vNum.x+1.875966*vNum.y+0.041555*vNum.z);
	vResult.z=( 0.067871*vNum.x-0.228834*vNum.y+1.069251*vNum.z);

	return vResult;
}

Vector xyz_from_rgb( Vector vNum )
{
	Vector vResult;

	vResult.x=(0.430574*vNum.x+0.341550*vNum.y+0.178325*vNum.z);
	vResult.y=(0.222015*vNum.x+0.706655*vNum.y+0.071330*vNum.z);
	vResult.z=(0.020183*vNum.x+0.129553*vNum.y+0.939180*vNum.z);

	return vResult;
}

Vector anomylize( Vector a, Vector b )
{
	return ( ( 1.75f * b ) + a ) / 2.75f;
}

Vector monochrome( Vector r)
{
	float z = (r.x*0.299+r.y*0.587+r.z*0.114);

	return Vector( z, z, z );;
}


Vector blindMK( Vector vColor )
{
	const float wx=0.312713;
	const float wy=0.329016;
	const float wz=0.358271;

	Vector c_xyz = xyz_from_rgb( vColor );

	float sum_xyz=c_xyz.x+c_xyz.y+c_xyz.z;

	Vector2D c_uv;
	c_uv.x=0;
	c_uv.y=0;

	if ( sum_xyz!=0 )
	{
		c_uv.x=c_xyz.x/sum_xyz;
		c_uv.y=c_xyz.y/sum_xyz;
	}

	float nx=wx*c_xyz.y/wy;
	float nz=wz*c_xyz.y/wy;

	Vector d_xyz;
	d_xyz.y=0;

	float clm;
	if ( c_uv.x< cpu )
	{
		clm=(cpv-c_uv.y)/(cpu-c_uv.x);
	}
	else
	{
		clm=(c_uv.y-cpv)/(c_uv.x-cpu);
	}

	float clyi=c_uv.y-c_uv.x*clm;
	Vector2D d_uv;
	d_uv.x=(ayi-clyi)/(clm-am);
	d_uv.y=(clm*d_uv.x)+clyi;

	Vector s_xyz;
	s_xyz.x=d_uv.x*c_xyz.y/d_uv.y;
	s_xyz.y=c_xyz.y;
	s_xyz.z=(1-(d_uv.x+d_uv.y))*c_xyz.y/d_uv.y;

	Vector s_rgb = rgb_from_xyz( s_xyz );

	d_xyz.x=nx-s_xyz.x;
	d_xyz.z=nz-s_xyz.z;

	Vector d_rgb = rgb_from_xyz( d_xyz );

	Vector adj_rgb;
	
	adj_rgb.Init();

	if ( d_rgb.x!=0 )
	{
		adj_rgb.x=( s_rgb.x<0 ? 0 : 1)-s_rgb.x/d_rgb.x;
	}
	if ( d_rgb.y!=0 )
	{
		adj_rgb.y=( s_rgb.y<0 ? 0 : 1)-s_rgb.y/d_rgb.y;
	}
	if ( d_rgb.z!=0 )
	{
		adj_rgb.z=( s_rgb.z<0 ? 0 : 1)-s_rgb.z/d_rgb.z;
	}

	float adjust = 0;
	if ( adj_rgb.x >= 0 && adj_rgb.x <= 1 )
	{
		adjust = adj_rgb.x;
	}
	if ( adj_rgb.y >= 0 && adj_rgb.y <= 1 && adj_rgb.y > adjust )
	{
		adjust = adj_rgb.y;
	}
	if ( adj_rgb.z >= 0 && adj_rgb.z <= 1 && adj_rgb.z > adjust )
	{
		adjust = adj_rgb.z;
	}

	s_rgb.x=s_rgb.x+(adjust*d_rgb.x);
	s_rgb.y=s_rgb.y+(adjust*d_rgb.y);
	s_rgb.z=s_rgb.z+(adjust*d_rgb.z);

	return s_rgb;
}

#endif


BEGIN_VS_SHADER( color_projection, "Help for deferred color correction" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( FRAME_TEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB1", "" )

		SHADER_PARAM( HSV_CORRECTION, SHADER_PARAM_TYPE_VEC3, "[ 0.0 0.0 0.0 ]", "" )
		SHADER_PARAM( CONTRAST_CORRECTION, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )

	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_FULL_FRAME_BUFFER_TEXTURE );

#if 0
		Vector vResult;
		vResult = blindMK( Vector( 1, 0, 0 ) );
		vResult = blindMK( Vector( 1, 0, 0 ) );
		vResult = blindMK( Vector( 1, 0, 0 ) );
		vResult = blindMK( Vector( 1, 0, 0 ) );
		vResult = blindMK( Vector( 1, 0, 0 ) );
		vResult = blindMK( Vector( 1, 0, 0 ) );

		Msg( "%g %g %g", vResult.x, vResult.y, vResult.z );

#endif
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT
	{
		if ( mat_color_projection == NULL )
		{
			mat_color_projection = cvar->FindVar( "mat_color_projection" );
		}

		if ( params[ FRAME_TEXTURE ]->IsDefined() == false )
		{
			params[ FRAME_TEXTURE ]->SetStringValue( "_rt_FullFrameFB1" );
		}
//		params[ FRAME_TEXTURE ]->SetStringValue( "rj/colors" );
		LoadTexture( FRAME_TEXTURE );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			SetInitialShadowState( );

			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableDepthTest( false );
//			pShaderShadow->EnableBlending( true );
//			pShaderShadow->BlendOp( SHADER_BLEND_OP_REVSUBTRACT );
//			EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );

			pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );

			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER4, false );

			pShaderShadow->EnableSRGBWrite( false );
			pShaderShadow->EnableAlphaWrites( true ); // writing water fog alpha always.

			int fmt = VERTEX_POSITION;
			int nTexCoordDims[ 2 ] = { 2, 3 };
			pShaderShadow->VertexShaderVertexFormat( fmt, 2, nTexCoordDims, 0 );

			DECLARE_STATIC_VERTEX_SHADER( color_projection_vs20 );
			SET_STATIC_VERTEX_SHADER( color_projection_vs20 );

			DECLARE_STATIC_PIXEL_SHADER( color_projection_ps20 );
			SET_STATIC_PIXEL_SHADER( color_projection_ps20 );
		}

		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();

			BindTexture( SHADER_SAMPLER4, FRAME_TEXTURE, -1 );

			int nIndex = mat_color_projection->GetInt() - 1;
			if ( nIndex < 0 || nIndex >= MAX_PROJECTIONS )
			{
				nIndex = 0;
			}

			Vector4D vCorrectionParms;

			vCorrectionParms.x = ProjectionInfo[ nIndex ].m_flCPU;
			vCorrectionParms.y = ProjectionInfo[ nIndex ].m_flCPV;
			vCorrectionParms.z = ProjectionInfo[ nIndex ].m_flAM;
			vCorrectionParms.w = ProjectionInfo[ nIndex ].m_flAYI;
			pShaderAPI->SetPixelShaderConstant( 1, vCorrectionParms.Base() );

			DECLARE_DYNAMIC_VERTEX_SHADER( color_projection_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( color_projection_vs20 );

			DECLARE_DYNAMIC_PIXEL_SHADER( color_projection_ps20 );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( NEED_BLINDMK, ProjectionInfo[ nIndex ].m_bNeedBlindMK );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( NEED_MONOCHROME, ProjectionInfo[ nIndex ].m_bNeedMonochrome );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( NEED_ANOMYLIZE, ProjectionInfo[ nIndex ].m_bNeedAnomylize );
			SET_DYNAMIC_PIXEL_SHADER( color_projection_ps20 );
		}
		Draw();
	}
END_SHADER
