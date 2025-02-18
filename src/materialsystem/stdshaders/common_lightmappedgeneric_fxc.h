//========= Copyright Valve Corporation, All rights reserved. ============//


#if defined( _X360 )

void GetBaseTextureAndNormal( sampler base, sampler base2, sampler bump, bool bBase2, bool bBump, float3 coords, float3 vWeights,
								out float4 vResultBase, out float4 vResultBase2, out float4 vResultBump )
{
	vResultBase = 0;
	vResultBase2 = 0;
	vResultBump = 0;

	if ( !bBump )
	{
		vResultBump = float4(0, 0, 1, 1);
	}

#if SEAMLESS

	vWeights = max( vWeights - 0.3, 0 );

	vWeights *= 1.0f / dot( vWeights, float3(1,1,1) );

	[branch]
	if (vWeights.x > 0)
	{
		vResultBase  += vWeights.x * tex2D( base,  coords.zy );

		if ( bBase2 )
		{
			vResultBase2 += vWeights.x * tex2D( base2, coords.zy );
		}

		if ( bBump )
		{
			vResultBump  += vWeights.x * tex2D( bump,  coords.zy );
		}
	}

	[branch]
	if (vWeights.y > 0)
	{
		vResultBase  += vWeights.y * tex2D( base,  coords.xz );

		if ( bBase2 )
		{
			vResultBase2 += vWeights.y * tex2D( base2, coords.xz );
		}
		if ( bBump )
		{
			vResultBump  += vWeights.y * tex2D( bump,  coords.xz );
		}
	}

	[branch]
	if (vWeights.z > 0)
	{
		vResultBase  += vWeights.z * tex2D( base,  coords.xy );
		if ( bBase2 )
		{
			vResultBase2 += vWeights.z * tex2D( base2, coords.xy );
		}

		if ( bBump )
		{
			vResultBump  += vWeights.z * tex2D( bump,  coords.xy );
		}
	}

#else  // not seamless

	vResultBase = tex2D( base, coords.xy );

	if ( bBase2 )
	{
		vResultBase2 = tex2D( base2, coords.xy );
	}

	if ( bBump )
	{
		vResultBump  = tex2D( bump, coords.xy );
	}

#endif


}

#else // PC

void GetBaseTextureAndNormal( sampler base, sampler base2, sampler bump, bool bBase2, bool bBump, float3 coords, float3 vWeights,
							 out float4 vResultBase, out float4 vResultBase2, out float4 vResultBump )
{
	vResultBase = 0;
	vResultBase2 = 0;
	vResultBump = 0;

	if ( !bBump )
	{
		vResultBump = float4(0, 0, 1, 1);
	}

#if SEAMLESS

	vResultBase  += vWeights.x * tex2D( base, coords.zy );
	if ( bBase2 )
	{
		vResultBase2 += vWeights.x * tex2D( base2, coords.zy );
	}
	if ( bBump )
	{
		vResultBump  += vWeights.x * tex2D( bump, coords.zy );
	}

	vResultBase  += vWeights.y * tex2D( base, coords.xz );
	if ( bBase2 )
	{
		vResultBase2 += vWeights.y * tex2D( base2, coords.xz );
	}
	if ( bBump )
	{
		vResultBump  += vWeights.y * tex2D( bump, coords.xz );
	}

	vResultBase  += vWeights.z * tex2D( base, coords.xy );
	if ( bBase2 )
	{
		vResultBase2 += vWeights.z * tex2D( base2, coords.xy );
	}
	if ( bBump )
	{
		vResultBump  += vWeights.z * tex2D( bump, coords.xy );
	}

#else  // not seamless

	vResultBase  = tex2D( base, coords.xy );
	if ( bBase2 )
	{
		vResultBase2 = tex2D( base2, coords.xy );
	}
	if ( bBump )
	{
		vResultBump  = tex2D( bump, coords.xy );
	}
#endif

}

#endif

// misyl:
// Bicubic lightmap code lovingly taken and adapted from Godot
// ( https://github.com/godotengine/godot/pull/89919 )
// Licensed under MIT.

float w0(float a) {
	return (1.0 / 6.0) * (a * (a * (-a + 3.0) - 3.0) + 1.0);
}

float w1(float a) {
	return (1.0 / 6.0) * (a * a * (3.0 * a - 6.0) + 4.0);
}

float w2(float a) {
	return (1.0 / 6.0) * (a * (a * (-3.0 * a + 3.0) + 3.0) + 1.0);
}

float w3(float a) {
	return (1.0 / 6.0) * (a * a * a);
}

// g0 and g1 are the two amplitude functions
float g0(float a) {
	return w0(a) + w1(a);
}

float g1(float a) {
	return w2(a) + w3(a);
}

// h0 and h1 are the two offset functions
float h0(float a) {
	return -1.0 + w1(a) / (w0(a) + w1(a));
}

float h1(float a) {
	return 1.0 + w3(a) / (w2(a) + w3(a));
}

#ifndef BICUBIC_LIGHTMAP
#define BICUBIC_LIGHTMAP 0
#endif

float3 LightMapSample( sampler LightmapSampler, float2 vTexCoord )
{
#	if ( !defined( _X360 ) || !defined( USE_32BIT_LIGHTMAPS_ON_360 ) )
	{
#if BICUBIC_LIGHTMAP
		float flLightmapPageWidth = 1024;
		float flLightmapPageHeight = 512;

		const float2 vTextureSize = float2( flLightmapPageWidth, flLightmapPageHeight );
		const float2 vTexelSize = float2( 1.0f, 1.0f ) / vTextureSize;

		vTexCoord.xy = vTexCoord.xy * vTextureSize + float2( 0.5f, 0.5f );

		float2 iuv = floor( vTexCoord.xy );
		float2 fuv = frac( vTexCoord.xy );

		float g0x = g0( fuv.x );
		float g1x = g1( fuv.x );
		float h0x = h0( fuv.x );
		float h1x = h1( fuv.x );
		float h0y = h0( fuv.y );
		float h1y = h1( fuv.y );

		float2 p0 = ( float2( iuv.x + h0x, iuv.y + h0y ) - float2( 0.5f, 0.5f ) ) * vTexelSize;
		float2 p1 = ( float2( iuv.x + h1x, iuv.y + h0y ) - float2( 0.5f, 0.5f ) ) * vTexelSize;
		float2 p2 = ( float2( iuv.x + h0x, iuv.y + h1y ) - float2( 0.5f, 0.5f ) ) * vTexelSize;
		float2 p3 = ( float2( iuv.x + h1x, iuv.y + h1y ) - float2( 0.5f, 0.5f ) ) * vTexelSize;

		float3 sample = 
			( g0( fuv.y ) * ( g0x * tex2D( LightmapSampler, p0 ) + g1x * tex2D( LightmapSampler, p1 ) ) ) +
			( g1( fuv.y ) * ( g0x * tex2D( LightmapSampler, p2 ) + g1x * tex2D( LightmapSampler, p3 ) ) );
#else
		float3 sample = tex2D( LightmapSampler, vTexCoord );
#endif

		return sample;
	}
#	else
	{
#		if 0 //1 for cheap sampling, 0 for accurate scaling from the individual samples
		{
			float4 sample = tex2D( LightmapSampler, vTexCoord );

			return sample.rgb * sample.a;
		}
#		else
		{
			float4 Weights;
			float4 samples_0; //no arrays allowed in inline assembly
			float4 samples_1;
			float4 samples_2;
			float4 samples_3;
			
			asm {
				tfetch2D samples_0, vTexCoord.xy, LightmapSampler, OffsetX = -0.5, OffsetY = -0.5, MinFilter=point, MagFilter=point, MipFilter=keep, UseComputedLOD=false
				tfetch2D samples_1, vTexCoord.xy, LightmapSampler, OffsetX =  0.5, OffsetY = -0.5, MinFilter=point, MagFilter=point, MipFilter=keep, UseComputedLOD=false
				tfetch2D samples_2, vTexCoord.xy, LightmapSampler, OffsetX = -0.5, OffsetY =  0.5, MinFilter=point, MagFilter=point, MipFilter=keep, UseComputedLOD=false
				tfetch2D samples_3, vTexCoord.xy, LightmapSampler, OffsetX =  0.5, OffsetY =  0.5, MinFilter=point, MagFilter=point, MipFilter=keep, UseComputedLOD=false

				getWeights2D Weights, vTexCoord.xy, LightmapSampler
			};

			Weights = float4( (1-Weights.x)*(1-Weights.y), Weights.x*(1-Weights.y), (1-Weights.x)*Weights.y, Weights.x*Weights.y );

			float3 result;
			result.rgb  = samples_0.rgb * (samples_0.a * Weights.x);
			result.rgb += samples_1.rgb * (samples_1.a * Weights.y);
			result.rgb += samples_2.rgb * (samples_2.a * Weights.z);
			result.rgb += samples_3.rgb * (samples_3.a * Weights.w);
		
			return result;
		}
#		endif
	}
#	endif
}

