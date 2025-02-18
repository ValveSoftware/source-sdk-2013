#include "common_vs_fxc.h"

const float4 g_vTextureSizeInfo : register( SHADER_SPECIFIC_CONST_0 );
const float4 g_mBaseTexCoordTransform[2] : register( SHADER_SPECIFIC_CONST_1 );
												   //SHADER_SPECIFIC_CONST_2

#define TEXEL_XINCR (g_vTextureSizeInfo.x)
#define TEXEL_YINCR (g_vTextureSizeInfo.y)
#define U_TO_PIXEL_COORD_SCALE (g_vTextureSizeInfo.z)
#define V_TO_PIXEL_COORD_SCALE (g_vTextureSizeInfo.w)

struct VS_INPUT
{
	float4 vPos			: POSITION;
	float2 vTexCoord0	: TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 projPos					: POSITION;
    
//#if defined( _X360 )
//	float2 baseTexCoord		: TEXCOORD0;
//#else
	float2 baseTexCoord00 : TEXCOORD0;
	float2 baseTexCoord01 : TEXCOORD1;
	float2 baseTexCoord10 : TEXCOORD2;
	float2 baseTexCoord11 : TEXCOORD3;
	float2 baseTexCoord_In_Pixels: TEXCOORD4;
//#endif
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	o.projPos = mul( v.vPos, cModelViewProj );

	float4 vTexCoordInput = { v.vTexCoord0.x, v.vTexCoord0.y, 0.0f, 1.0f };
	float2 vTexCoord;
	vTexCoord.x = dot( vTexCoordInput.xyzw, g_mBaseTexCoordTransform[0] );
	vTexCoord.y = dot( vTexCoordInput.xyzw, g_mBaseTexCoordTransform[1] );

//#if defined( _X360 )
//	o.baseTexCoord.xy = vTexCoord.xy;
//#else
	// Compute quantities needed for pixel shader texture lerping
	o.baseTexCoord00.x = vTexCoord.x - TEXEL_XINCR;
	o.baseTexCoord00.y = vTexCoord.y - TEXEL_YINCR;
	o.baseTexCoord10.x = vTexCoord.x + TEXEL_XINCR;
	o.baseTexCoord10.y = vTexCoord.y - TEXEL_YINCR;

	o.baseTexCoord01.x = vTexCoord.x - TEXEL_XINCR;
	o.baseTexCoord01.y = vTexCoord.y + TEXEL_YINCR;
	o.baseTexCoord11.x = vTexCoord.x + TEXEL_XINCR;
	o.baseTexCoord11.y = vTexCoord.y + TEXEL_YINCR;

	o.baseTexCoord_In_Pixels.xy = o.baseTexCoord00.xy;
	o.baseTexCoord_In_Pixels.x *= U_TO_PIXEL_COORD_SCALE;
	o.baseTexCoord_In_Pixels.y *= V_TO_PIXEL_COORD_SCALE;
//#endif

	return o;
}
