//========== Copyright (c) Valve Corporation, All rights reserved. ==========//

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
