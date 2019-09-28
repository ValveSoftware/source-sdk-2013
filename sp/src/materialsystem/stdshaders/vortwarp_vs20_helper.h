#include "common_vs_fxc.h"

float Sine( float min, float max, float t )
{
	return ( sin( t ) * 0.5f + 0.5f ) * ( max - min ) + min;
}

float3 QuadraticBezier( float3 A, float3 B, float3 C, float t )
{
	return lerp( lerp( A, B, t ), lerp( B, C, t ), t );
}

float3 CubicBezier( float3 A, float3 B, float3 C, float3 D, float t )
{
	return QuadraticBezier( lerp( A, B, t ), lerp( B, C, t ), lerp( C, D, t ), t );
}

void WorldSpaceVertexProcess( in float time, in float3 modelOrigin, inout float3 worldPos, inout float3 worldNormal, inout float3 worldTangentS, inout float3 worldTangentT )
{
	float myTime = time;
	myTime = saturate( 1.0f - myTime );
	myTime *= myTime;
	myTime *= myTime;
	myTime *= myTime;
//	worldPos.z += 72.0f * myTime;

	// end
	float3 A = float3( 0.0f, 0.0f, 1.0f );
	float3 B = float3( 1.0f, 1.0f, 1.0f );
	float3 C = float3( 0.0f, 0.0f, 1.0f );
	float3 D = float3( 0.0f, 0.0f, 1.0f );
	// start
	
//	float3 modelOrigin = float3( 70.0f, -14.0f, 0.0f );
	
	float t = worldPos.z * ( 1.0f / ( 72.0f ) ); // about 72 inches tall
	t = saturate( t );
	float3 worldPosDelta = ( worldPos - modelOrigin ) * CubicBezier( A, B, C, D, t );
	worldPosDelta.z += Sine( 0.0f, 10.0, worldPos.z );
	worldPos = lerp( worldPos, worldPosDelta + modelOrigin, myTime );
}
