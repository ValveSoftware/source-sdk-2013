// DYNAMIC: "FOGTYPE" "0..1"

#include "common_vs_fxc.h"

static const int g_FogType = FOGTYPE;
const float4 cCustomConstants[6]	:  register( SHADER_SPECIFIC_CONST_0 );


const float4 g_vLightPosition : register( SHADER_SPECIFIC_CONST_0 );
const float4 g_vLightColor : register( SHADER_SPECIFIC_CONST_1 );		// range 0-1
const float  g_flLightIntensity : register( SHADER_SPECIFIC_CONST_2 );	// scales g_vLightColor


struct VS_INPUT
{
	// If this is float4, and the input is float3, the w component default to one.
	float4 vPos				: POSITION; 
	float2 vBumpTexCoord	: TEXCOORD0;
	float4 vAmbientColor	: COLOR0;
};

struct VS_OUTPUT
{
    float4 projPos					: POSITION;	
#if !defined( _X360 )
	float  fog						: FOG;
#endif
	float2 vBumpTexCoord			: TEXCOORD0;
	float3 vTangentSpaceLightDir	: TEXCOORD1;
	float3 vAmbientColor			: TEXCOORD2;
	
#if defined( _X360 )
	float4 vScreenPos_ReverseZ		: TEXCOORD3;
#else
	float4 vScreenPos				: TEXCOORD3;
#endif

	
	float4 vDirLightScale			: COLOR0;
	
	float4 worldPos_projPosZ		: TEXCOORD7;		// Necessary for pixel fog
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o;

	// Transform the input position.
	float4 projPos = mul( v.vPos, cModelViewProj );
	o.projPos = projPos;
	projPos.z = dot( v.vPos, cModelViewProjZ );
	
#if defined( _X360 )
	o.vScreenPos_ReverseZ.x = projPos.x;
	o.vScreenPos_ReverseZ.y = -projPos.y; // invert Y
	o.vScreenPos_ReverseZ.xy = (o.vScreenPos_ReverseZ.xy + projPos.w) * 0.5f;
	o.vScreenPos_ReverseZ.z = projPos.w - projPos.z;
	o.vScreenPos_ReverseZ.w = projPos.w;
#else	
	o.vScreenPos.x = projPos.x;
	o.vScreenPos.y = -projPos.y; // invert Y
	o.vScreenPos.xy = (o.vScreenPos.xy + projPos.w) * 0.5f;
	o.vScreenPos.z = projPos.z;
	o.vScreenPos.w = projPos.w;
#endif
	
	o.worldPos_projPosZ = float4( v.vPos.xyz, projPos.z );

#if !defined( _X360 )
	// Setup fog.
	o.fog = CalcFog( mul( v.vPos, cModel[0] ), projPos, g_FogType );
#endif

	// Copy texcoords over.
	o.vBumpTexCoord = v.vBumpTexCoord;

	// Copy the vertex color over.
	o.vAmbientColor = v.vAmbientColor;

	// ------------------------------------------------------------------------------
	//  Generate a tangent space and rotate L.
	//  This can be thought of as rotating the normal map to face the viewer.
	// 
	//  This is useful when a particle is way off to the side of the screen.
	//  You should be looking at the half-sphere with a normal pointing from the
	//  particle to the viewer. Instead, you're looking at the half-sphere with
	//  a normal along Z. This tangent space builder code fixes the problem.
	// 
	//  Note that since the model and view matrices are identity, the coordinate
	//  system has X=right, Y=up, and Z=behind you  (negative Z goes into the screen).
	// ------------------------------------------------------------------------------
	
	// This basis wants Z positive going into the screen so flip it here.
	float4 vForward = normalize( float4( v.vPos.x, v.vPos.y, -v.vPos.z, 1 ) );

	// This is the same as CrossProduct( vForward, Vector( 1, 0, 0 ) )
	float4 vUp = normalize( float4( 0, vForward.z, -vForward.y, vForward.w ) );
	
	// vRight = CrossProduct( vUp, vForward )
	float4 vRight =  vUp.yzxw * vForward.zxyw;
	vRight       += -vUp.zxyw * vForward.yzxw;

	
	// Put the light in tangent space.
	float4 vToLight = g_vLightPosition - v.vPos;
	float4 vTangentSpaceLight = vRight*vToLight.x + vUp*vToLight.y + vForward*vToLight.z;

	// Output texcoord 1 holds the normalized transformed light direction.
	o.vTangentSpaceLightDir = normalize( vTangentSpaceLight ) * 0.5 + 0.5; // make it 0-1 for the pixel shader
	
	
	// Handle oversaturation here. The shader code already scaled the light color so its max value is 1,
	// so if our intensity/distance scale is > 1, then all we need to do is use the light color.
	float flTransposedLenSqr = dot( vTangentSpaceLight, vTangentSpaceLight );
	float flScaledIntensity = g_flLightIntensity / flTransposedLenSqr;
	if ( flScaledIntensity > 1 )
	{
		o.vDirLightScale.xyz = g_vLightColor;
	}
	else
	{
		o.vDirLightScale.xyz = g_vLightColor * flScaledIntensity;
	}
	
	// Alpha comes right from the vertex color.
	o.vDirLightScale.a = v.vAmbientColor.a;
	return o;
}


