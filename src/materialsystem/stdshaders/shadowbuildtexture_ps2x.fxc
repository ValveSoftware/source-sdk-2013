// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#include "common_ps_fxc.h"

sampler BaseTextureSampler	: register( s0 );

struct PS_INPUT
{
	float2 vTexCoord0	: TEXCOORD0;
	float4 vColor		: COLOR0;
		
#if defined( _X360 ) //matching pixel shader inputs to vertex shader outputs to avoid shader patches	
	float2 vTexCoord1	: TEXCOORD1;
	float2 vTexCoord2	: TEXCOORD2;
	float2 vTexCoord3	: TEXCOORD3;
	float4 fogFactorW	: COLOR1;
	float4 worldPos_projPosZ		: TEXCOORD7;		// Necessary for pixel fog
#endif
};

HALF4 main( PS_INPUT i ) : COLOR
{
	//relevant data is in the alpha channel, modulate vertex alpha by texture alpha
	float returnAlpha = tex2D( BaseTextureSampler, i.vTexCoord0 ).a * i.vColor.a;
	
	return FinalOutput( float4( 1.0f, 1.0f, 1.0f, returnAlpha ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}
