// based upon http://kaioa.com/node/91

// DYNAMIC: "NEED_BLINDMK"		"0..1"
// DYNAMIC: "NEED_ANOMYLIZE"	"0..1"
// DYNAMIC: "NEED_MONOCHROME"	"0..1"

#include "shader_constant_register_map.h"
#include "common_ps_fxc.h"


struct PS_INPUT
{
	float2 vScreenUV				: TEXCOORD0;
};

sampler FrameSampler						: register( s4 );

const float4 g_vColorParms					: register( c1 );
	#define cpu									g_vColorParms.x
	#define cpv									g_vColorParms.y
	#define am									g_vColorParms.z
	#define ayi									g_vColorParms.w


float3 rgb_from_xyz( float3 vNum )
{
	float3 vResult;
	
	vResult.r = dot( vNum, float3( 3.063218f, -1.393325f, -0.475802f ) );
    vResult.g = dot( vNum, float3( -0.969243f, 1.875966f, 0.041555f ) );
    vResult.b = dot( vNum, float3( 0.067871f, -0.228834f, 1.069251f ) );
        
    return vResult;
}

float3 xyz_from_rgb( float3 vNum )
{
	float3 vResult;

	vResult.x = dot( vNum, float3( 0.430574f, 0.341550f, 0.178325f ) );
    vResult.y = dot( vNum, float3( 0.222015f, 0.706655f, 0.071330f ) );
    vResult.z = dot( vNum, float3( 0.020183f, 0.129553f, 0.939180f ) );
	
	return vResult;
}

float3 anomylize( float3 a, float3 b )
{
	return ( ( 1.75f * b ) + a ) / 2.75f;
}

float3 monochrome( float3 r )
{
	return dot( r, float3( 0.299f, 0.587f, 0.114f ) );
}

const static float3 w_xyz = float3( 0.312713f, 0.329016f, 0.358271f );

float3 blindMK( float3 vColor )
{
	float3	c_xyz = xyz_from_rgb( vColor );

    float	sum_xyz = c_xyz.x + c_xyz.y + c_xyz.z;
    
    float2	c_uv = 0.0f;

	if ( sum_xyz != 0.0f )
	{
		c_uv = c_xyz.xy / sum_xyz;
	}

	float2	n_xz = w_xyz.xz * c_xyz.y / w_xyz.y;
	
	float	clm;
	if ( c_uv.x < cpu )
	{
		clm = ( cpv - c_uv.y ) / ( cpu - c_uv.x );
	}
	else
	{
       clm = ( c_uv.y - cpv ) / ( c_uv.x - cpu );
    }

	float	clyi = c_uv.y - c_uv.x * clm;
	float2	d_uv;
	d_uv.x = ( ayi - clyi ) / ( clm - am );
	d_uv.y = ( clm * d_uv.x ) + clyi;

	float3	s_xyz;
	s_xyz.x = d_uv.x * c_xyz.y / d_uv.y;
    s_xyz.y = c_xyz.y;
    s_xyz.z = ( 1.0f - ( d_uv.x + d_uv.y ) ) * c_xyz.y / d_uv.y;
    
    float3	s_rgb = rgb_from_xyz( s_xyz );

	float3	d_xyz = 0.0f;
	d_xyz.xz = n_xz - s_xyz.xz;
	
	float3	d_rgb = rgb_from_xyz( d_xyz );

	float3	adj_rgb = ( d_rgb != 0.0f ? ( ( s_rgb < 0.0f ? 0.0f : 1.0f ) - s_rgb / d_rgb ) : 0.0f );

	adj_rgb = ( adj_rgb < 0.0f ? 0.0f : adj_rgb > 1.0f ? 0.0f : adj_rgb );
	float	adjust = max( max( adj_rgb.r, adj_rgb.g ), adj_rgb.b );

	s_rgb = s_rgb + ( adjust * d_rgb );
	
	return s_rgb;
}


float4 main( PS_INPUT i ) : COLOR
{
	float4 vDiffuse = tex2D( FrameSampler, i.vScreenUV );

//	vDiffuse = float4( 1, 0, 0, 1 );
	
	float4 vResult = vDiffuse;

#if ( NEED_BLINDMK == 1 )
	vResult.rgb = blindMK( vResult.rgb );
#endif

#if ( NEED_MONOCHROME == 1 )
	vResult.rgb = monochrome( vResult.rgb );
#endif

#if ( NEED_ANOMYLIZE == 1 )
	vResult.rgb = anomylize( vDiffuse.rgb, vResult.rgb );
#endif

	return vResult;
}
