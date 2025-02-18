// STATIC: "CONVERT_TO_SRGB"	"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"	"0..0"	[= 0] [XBOX]
// STATIC: "SHOWALPHA"			"0..1"
// DYNAMIC: "ISCUBEMAP"			"0..1"

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

sampler g_tSampler : register( s0 );

struct PS_INPUT
{
	float2 texCoord				: TEXCOORD0;
};

const float3 g_vConst0 : register( c0 );
#define g_flIsHdrCube g_vConst0.x
#define g_flIsHdr2D g_vConst0.y

float4 main( PS_INPUT i ) : COLOR
{
	float4 sample = tex2D( g_tSampler, i.texCoord );
	float4 result = { 0.0f, 0.0f, 0.0f, 1.0f };

	result.rgb = sample.rgb;
	#if SHOWALPHA
		result.rgb = sample.a;
	#endif

	if ( g_flIsHdr2D )
		result.rgb *= MAX_HDR_OVERBRIGHT;

	#if ISCUBEMAP
		bool bNoDataForThisPixel = false;
		float3 vec = float3( 0, 0, 0 );
		float x = i.texCoord.x;
		float y = i.texCoord.y;
		float x2 = frac( ( i.texCoord.x ) * 3.0f ) * 2.0f - 1.0f;
		float y2 = frac( ( i.texCoord.y ) * 4.0f ) * 2.0f - 1.0f;
		if ( ( x >= 0.3333f ) && ( x <= 0.6666f ) ) //Center row
		{
			if ( y >= 0.75f )
				vec = float3( x2, 1.0, y2 );
			else if ( y >= 0.5f )
				vec = float3( x2, y2, -1.0 );
			else if ( y >= 0.25f )
				vec = float3( x2, -1.0, -y2 );
			else if ( y >= 0.0f )
				vec = float3( x2, -y2, 1.0 );
		}
		else if ( ( y >= 0.25f ) && ( y <= 0.5f ) )
		{
			if ( x <= 0.3333f )
				vec = float3( -1.0f, -x2, -y2 );
			else if (x >= 0.6666f)
				vec = float3( 1.0f, x2, -y2 );
			else
				bNoDataForThisPixel = true;
		}
		else
		{
			bNoDataForThisPixel = true;
		}

		float4 cBase = texCUBE( g_tSampler, vec );
		#if SHOWALPHA
			cBase.rgb = cBase.a;
		#endif

		if ( g_flIsHdrCube )
			cBase.rgb *= ENV_MAP_SCALE;

		if ( bNoDataForThisPixel == true )
			cBase.rgb = float3( 0.9f, 0.4f, 0.15f );

		result.rgb = cBase.rgb;
		result.a = 1.0f; // - bNoDataForThisPixel;
	#endif

	return FinalOutput( result, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}
