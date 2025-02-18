#include "common_vs_fxc.h"

struct VS_INPUT
{
	float3 vPos						: POSITION;
	float2 vInputImageCoord			: TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 projPos					: POSITION;	
	float2 InputImageCoord			: TEXCOORD0;
	float2 FilmGrainCoord			: TEXCOORD1;
};

// 
const float4 cFilmGrainOffset	:  register( SHADER_SPECIFIC_CONST_0 );  // should be SHADER_SPECIFIC_CONST_0


VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	// Just copy these through to the output...
	o.projPos = float4( v.vPos, 1.0f );
	o.InputImageCoord  = v.vInputImageCoord;

	// Scale and bias the input coordinates to get grain coordinates
	o.FilmGrainCoord.xy = v.vInputImageCoord.xy * cFilmGrainOffset.xy + cFilmGrainOffset.zw;

	return o;
}
