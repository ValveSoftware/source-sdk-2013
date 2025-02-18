// DYNAMIC: "SKINNING"					"0..1"

#include "common_vs_fxc.h"

static const bool g_bSkinning		= SKINNING ? true : false;

const float4 cBumpTexCoordTransform[2]	: register( SHADER_SPECIFIC_CONST_1 );
const float g_CoreColorTexCoordOffset	: register( SHADER_SPECIFIC_CONST_3 );

struct VS_INPUT
{
	float4 vPos							: POSITION;
	float4 vBoneWeights					: BLENDWEIGHT;
	float4 vBoneIndices					: BLENDINDICES;
	float3 vNormal						: NORMAL;
	float4 vBaseTexCoord				: TEXCOORD0;
	float4 vUserData					: TANGENT;
};

struct VS_OUTPUT
{
    float4 vProjPos					: POSITION;	
	float  vFog						: FOG;

	float4 oT0						: TEXCOORD0;
	float4 oT1						: TEXCOORD1;
	float4 oT2						: TEXCOORD2;
//	float3 oT3						: TEXCOORD3;

	float4 fogFactorW				: COLOR1;
};

float LengthThroughSphere( float3 vecRayOrigin, float3 vecRayDelta, 
	float3 vecSphereCenter, float flRadius, out float alpha )
{
	// Solve using the ray equation + the sphere equation
	// P = o + dt
	// (x - xc)^2 + (y - yc)^2 + (z - zc)^2 = r^2
	// (ox + dx * t - xc)^2 + (oy + dy * t - yc)^2 + (oz + dz * t - zc)^2 = r^2
	// (ox - xc)^2 + 2 * (ox-xc) * dx * t + dx^2 * t^2 +
	//		(oy - yc)^2 + 2 * (oy-yc) * dy * t + dy^2 * t^2 +
	//		(oz - zc)^2 + 2 * (oz-zc) * dz * t + dz^2 * t^2 = r^2
	// (dx^2 + dy^2 + dz^2) * t^2 + 2 * ((ox-xc)dx + (oy-yc)dy + (oz-zc)dz) t +
	//		(ox-xc)^2 + (oy-yc)^2 + (oz-zc)^2 - r^2 = 0
	// or, t = (-b +/- sqrt( b^2 - 4ac)) / 2a
	// a = DotProduct( vecRayDelta, vecRayDelta );
	// b = 2 * DotProduct( vecRayOrigin - vecCenter, vecRayDelta )
	// c = DotProduct(vecRayOrigin - vecCenter, vecRayOrigin - vecCenter) - flRadius * flRadius;

	float3 vecSphereToRay;
	vecSphereToRay = vecRayOrigin - vecSphereCenter;

	float a = dot( vecRayDelta, vecRayDelta );

	// This would occur in the case of a zero-length ray
//	if ( a == 0.0f )
//	{
//		*pT1 = *pT2 = 0.0f;
//		return vecSphereToRay.LengthSqr() <= flRadius * flRadius;
//	}

	float b = 2 * dot( vecSphereToRay, vecRayDelta );
	float c = dot( vecSphereToRay, vecSphereToRay ) - flRadius * flRadius;
	float flDiscrim = b * b - 4 * a * c;
//	if ( flDiscrim < 0.0f )
//		return 0.0f;

	float hack = flDiscrim;
	flDiscrim = sqrt( flDiscrim );
	float oo2a = 0.5f / a;
	if( hack < 0.0f )
	{
		alpha = 0.0f;
		return 0.0f;
	}
	else
	{
		alpha = 1.0f;
		return abs( flDiscrim ) * 2 * oo2a;
	}
//	*pT1 = ( - b - flDiscrim ) * oo2a;
//	*pT2 = ( - b + flDiscrim ) * oo2a;
//	return true;
}

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float4 projPos;
	float3 worldNormal, worldPos, worldTangentS, worldTangentT;

	SkinPositionNormalAndTangentSpace( 
			g_bSkinning, 
			v.vPos, v.vNormal, v.vUserData,
			v.vBoneWeights, v.vBoneIndices,
			worldPos, worldNormal, worldTangentS, worldTangentT );

	// Projected position
	o.vProjPos = projPos = mul( float4( worldPos, 1 ), cViewProj );
	
	// calculate fog
	o.fogFactorW = o.vFog = CalcFog( worldPos, projPos, FOGTYPE_RANGE );
	
	// Eye vector
	float3 vWorldEyeVect = cEyePos - worldPos;

	// Transform to the tangent space
	o.oT1.x = dot( vWorldEyeVect, worldTangentS );
	o.oT1.y = dot( vWorldEyeVect, worldTangentT );
	o.oT1.z = dot( vWorldEyeVect, worldNormal );

	// Tranform bump coordinates
	float2 bumpTexCoord;
	bumpTexCoord.x = dot( v.vBaseTexCoord, cBumpTexCoordTransform[0] );
	bumpTexCoord.y = dot( v.vBaseTexCoord, cBumpTexCoordTransform[1] );
	
	// dudv map
	o.oT0.xy = bumpTexCoord;
	
	// flip Y by multiplying by -1
	projPos.y *= -1.0f;

	// transform from [-w,w] to [0,2*w]
	// The reason this is w is because we are in perspective space/homogenous clip space.
	projPos.xy += projPos.w;

	// transform from [0,2*w] to [0,w]
	// We'll end up dividing by w in the pixel shader to get to [0,1]
	projPos.xy *= 0.5f;

	o.oT1.xy = projPos.xy;

	// emit w to both z and w in case the driver screws up and divides by z
	o.oT1.z = o.oT1.w = projPos.w;

	// hack
	float3 g_SphereCenter = { 2688.0f, 12139.0f, 5170.0f };
//	float g_SphereDiameter = 430.0f;
	float g_SphereDiameter = 530.0f;
	float g_SphereRadius = g_SphereDiameter * 0.5f;

	float dummyAlpha;
	float lengthThroughSphere = LengthThroughSphere( cEyePos, normalize( vWorldEyeVect ), 
					g_SphereCenter, g_SphereRadius, dummyAlpha );

	float normalizedLengthThroughSphere = saturate( lengthThroughSphere / g_SphereDiameter );
	o.oT2.xy = saturate( float2( normalizedLengthThroughSphere, g_CoreColorTexCoordOffset ) );

	// hack texcoord shit
//	o.oT0.xy = 3.0f * ( worldPos.xy - g_SphereCenter.xy ) / g_SphereRadius;

	return o;
}


