#include "common_vs_fxc.h"


const float4x3 cModelView			: register(SHADER_SPECIFIC_CONST_0);
const float4x4 cProj				: register(SHADER_SPECIFIC_CONST_3);

// derivative of catmull rom spline courtesy of calc
float4 DCatmullRomSpline ( float4 a, float4 b, float4 c, float4 d, float t )
{
	return 0.5 *( c - a + t * ( 2 * a - 5 * b + 4 * c - d + t * (3 * b - a - 3 * c + d ) ) 
				 + t * ( 2 * a - 5 * b + 4 * c - d + 2 * ( t * ( 3 * b - a - 3 * c + d ) ) ) );
}

float3 DCatmullRomSpline3 ( float3 a, float3 b, float3 c, float3 d, float t )
{
	return 0.5 *( c - a + t * ( 2 * a - 5 * b + 4 * c - d + t * (3 * b - a - 3 * c + d ) ) 
				 + t * ( 2 * a - 5 * b + 4 * c - d + 2 * ( t * ( 3 * b - a - 3 * c + d ) ) ) );
}

    
float4 CatmullRomSpline( float4 a, float4 b, float4 c, float4 d, float t )
{
	return b + 0.5 * t * ( c - a + t * ( 2 * a - 5 * b + 4 * c - d + t * ( -a + 3 * b -3 * c + d ) ) );
}

struct VS_INPUT
{
	// This is all of the stuff that we ever use.
	float4 vTint            : COLOR;
	float4 vParms           : POSITION;						// T V side_id
	float4 vSplinePt0		: TEXCOORD0;					// x y z rad
	float4 vSplinePt1		: TEXCOORD1;					// x y z rad
	float4 vSplinePt2		: TEXCOORD2;					// x y z rad
	float4 vSplinePt3		: TEXCOORD3;					// x y z rad
};

struct VS_OUTPUT
{
    float4 projPos			: POSITION;	

	float2 texCoord0		: TEXCOORD0;
	float2 texCoord1		: TEXCOORD1;
	float4 argbcolor		: COLOR;
	float4 blendfactor0		: TEXCOORD2;
	float4 vScreenPos		: TEXCOORD7;
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o;

	float4 posrad =
		CatmullRomSpline( v.vSplinePt0, v.vSplinePt1, v.vSplinePt2, v.vSplinePt3, v.vParms.x );
	float3 v2p = ( posrad.xyz - cEyePos );
	float3 tangent=DCatmullRomSpline3( v.vSplinePt0, v.vSplinePt1, v.vSplinePt2, v.vSplinePt3, v.vParms.x );
	float3 ofs = normalize( cross(v2p, normalize( tangent) ) );
	posrad.xyz += ofs * ( posrad.w * ( v.vParms.z - .5 ) );
	o.projPos  = mul( float4(posrad.xyz, 1.0f), cViewProj );
	o.texCoord0 = o.texCoord1 = float2( (1-v.vParms.z), v.vParms.y);
	o.argbcolor = v.vTint;
	o.blendfactor0 = float4(0,0,0,0);
	o.vScreenPos = float4(0,0,0,0);
	return o;

}
