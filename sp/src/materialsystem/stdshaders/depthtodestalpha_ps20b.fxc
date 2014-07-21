#include "common_ps_fxc.h"

struct PS_INPUT
{
	float projPosZ		: TEXCOORD0;
};

float4 main( PS_INPUT i ) : COLOR
{
	return FinalOutput( float4( 0, 0, 0, 1.0f ), 0.0f, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE, true, i.projPosZ );
}
