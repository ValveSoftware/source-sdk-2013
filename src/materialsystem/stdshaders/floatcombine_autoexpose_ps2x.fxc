// STATIC: "CONVERT_TO_SRGB" "0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB" "0..0"	[= 0] [XBOX]

#define HDRTYPE HDR_TYPE_FLOAT
#define HDRENABLED 1
#include "common_ps_fxc.h"

sampler FBSampler	: register( s0 );
sampler BloomSampler	: register( s1 );
sampler Exposure_Sampler	: register( s2 );

const HALF4 settings : register( c0 );
						// x=sharpness,y=woodcut,z=bloom_amt,w=alpha sharpen factor
const HALF4 settings2 : register( c1 );						// x=bloom exp,y=vignette min scale z=vignette_power
const HALF4 settings3 : register( c2 );						// x=autoexpose min y=autoexpose_max

struct PS_INPUT
{
	float2 texCoord				: TEXCOORD0;
	float2 ZeroTexCoord				: TEXCOORD1;
};


float4 main( PS_INPUT i ) : COLOR
{
	float4 fbSample = tex2D( FBSampler, i.texCoord );
	float4 bloom=tex2D(BloomSampler,i.texCoord);
	float4 exposure_data=tex2D(Exposure_Sampler,i.ZeroTexCoord);
	float avg_lum=exposure_data.r;
	float tmscale=max(0.18/max(avg_lum,0.0001),settings3.x);
	tmscale=min(tmscale,settings3.y);

	
	float2 xofs=2*(i.texCoord-float2(0.5,0.5));
	float dist=(1.0/2.0)*(xofs.x*xofs.x+xofs.y*xofs.y);
	float vig=pow(1-dist,settings2.z);

	fbSample=lerp(fbSample,bloom,(1-vig)*settings2.w);
	fbSample=lerp(bloom,fbSample,settings.x+settings.w*fbSample.a*settings2.w);

	bloom.xyz=min(bloom.xyz,1.0);
	float lum=.3*bloom.x+.59*bloom.y+.11*bloom.z;
	lum=min(1.0,lum);

	float4 c_out=(fbSample)+settings.z*pow(lum,settings2.x)*bloom;
	c_out.xyz*=tmscale;
	return FinalOutput( c_out, 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
}
