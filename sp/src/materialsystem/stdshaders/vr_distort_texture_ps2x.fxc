// DYNAMIC: "CMBO_USERENDERTARGET" "0..1"

#include "shader_constant_register_map.h"
#include "common_ps_fxc.h"

sampler BaseTextureSampler		: register( s0 );
sampler DistortMapTextureSampler		: register( s1 );


struct PS_INPUT
{
	float2 vBaseTexCoord		: TEXCOORD0;
};


float4 main( PS_INPUT i ) : COLOR
{
	float2 vOriginal = i.vBaseTexCoord.xy;

	float4 vRead = tex2D( DistortMapTextureSampler, vOriginal );

	float2 vGreen;
	vGreen.r = ( vRead.x + vRead.z ) / 2.0;
	vGreen.g = ( vRead.y + vRead.w ) / 2.0;

	float4 vFinal;
	vFinal.r = tex2D( BaseTextureSampler, vRead.xy ).r;
	vFinal.ga = tex2D( BaseTextureSampler, vGreen ).ga;
	vFinal.b = tex2D( BaseTextureSampler, vRead.zw ).b;
	
	float fBoundsCheck;
	#if ( CMBO_USERENDERTARGET )
	{
		fBoundsCheck = saturate( dot( (vGreen.xy < float2(0.01,0.01)), float2(1,1)) + dot( (vGreen.xy > float2(0.99,0.99)), float2(1,1)) );
	}
	#else
	{
		fBoundsCheck = saturate( dot( (vGreen.xy < float2(0.005,0.005)), float2(1,1)) + dot( (vGreen.xy > float2(0.995,0.995)), float2(1,1))
			+ (vGreen.x > 0.495 && vGreen.x < 0.505 ) );
	}
	#endif

	vFinal.xyz = lerp( vFinal.xyz, float3(0,0,0), fBoundsCheck );

	return vFinal;
}



