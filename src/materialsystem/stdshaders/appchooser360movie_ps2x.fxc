//======= Copyright © 1996-2006, Valve Corporation, All rights reserved. ======
#define CONVERT_TO_SRGB 0
#include "common_ps_fxc.h"

sampler TexSampler	: register( s0 );

struct PS_INPUT
{
	float2 baseTexCoord				: TEXCOORD0;		// Base texture coordinate

#if defined( _X360 ) //matching pixel shader inputs to vertex shader outputs to avoid shader patches	
	float2 ZeroTexCoord				: TEXCOORD1;
    float2 bloomTexCoord            : TEXCOORD2;
#endif

	float4 vColor					: TEXCOORD3;
};

float4 main( PS_INPUT i ) : COLOR
{
	float4 vTextureColor = tex2D( TexSampler, i.baseTexCoord );
	vTextureColor.r = SrgbGammaToLinear( vTextureColor.r );
	vTextureColor.g = SrgbGammaToLinear( vTextureColor.g );
	vTextureColor.b = SrgbGammaToLinear( vTextureColor.b );

	float4 result;
	result.rgb = vTextureColor.rgb * i.vColor.rgb;
	result.a = i.vColor.a;

	return result;
}
