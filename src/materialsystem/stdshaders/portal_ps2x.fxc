// STATIC: "CONVERT_TO_SRGB"	"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"	"0..0"	[= 0] [XBOX]
// STATIC: "HASALPHAMASK"		"0..1"
// STATIC: "HASSTATICTEXTURE"	"0..1"

// DYNAMIC: "ADDSTATIC"			"0..1"
// DYNAMIC: "HDRENABLED"		"0..1"
// DYNAMIC: "PIXELFOGTYPE"		"0..1"

#define USESTATICTEXTURE (((ADDSTATIC == 1) && (HASSTATICTEXTURE == 1))?(1):(0))

#include "common_ps_fxc.h"
#include "shader_constant_register_map.h"



const float4 g_StaticAmount				: register( c0 ); //x is static, y is 1.0 - static
const float4 g_FogParams				: register( PSREG_FOG_PARAMS );
const float4 g_EyePos_SpecExponent		: register( PSREG_EYEPOS_SPEC_EXPONENT );

sampler PortalSampler					: register( s0 );

#if( (HASALPHAMASK == 1) || (USESTATICTEXTURE == 1) )
	sampler SecondarySampler	: register( s1 );
#	if( (HASALPHAMASK == 1) && (USESTATICTEXTURE == 1) )
		sampler TertiarySampler		: register( s2 );
#	endif
#endif


struct PS_INPUT
{
	float3 vPortalTexCoord			: TEXCOORD0;	

#	if( (HASALPHAMASK == 1) || (USESTATICTEXTURE == 1) )
		float2 vSecondaryTexCoord		: TEXCOORD1;
#		if( (HASALPHAMASK == 1) && (USESTATICTEXTURE == 1) )
			float2 vTertiaryTexCoord		: TEXCOORD2;
#		endif
#	endif

	float4 worldPos_projPosZ		: TEXCOORD7;		// Necessary for pixel fog
};




float4 main( PS_INPUT i ) : COLOR
{
	HALF4 result;	
	result.rgb = tex2D(PortalSampler, i.vPortalTexCoord.xy / i.vPortalTexCoord.z ).rgb;
		
	//mix in static	
#	if( ADDSTATIC == 1 )
	{
		result.rgb *= g_StaticAmount.y; //inverse static on original colors
		
#		if( HASSTATICTEXTURE == 1 )
		{
#			if( HASALPHAMASK == 1 )
				result.rgb += tex2D(TertiarySampler, i.vTertiaryTexCoord ).rgb * g_StaticAmount.x; //static
#			else
				result.rgb += tex2D(SecondarySampler, i.vSecondaryTexCoord ).rgb * g_StaticAmount.x; //static	
#			endif
		}
#		else
		{
			result.rgb += g_StaticAmount.x * 0.25; //mix in gray	
		}
#		endif
	}	
#	endif
	
#	if( HASALPHAMASK == 1 )
	{
		//alpha mask
		result.a = tex2D(SecondarySampler, i.vSecondaryTexCoord ).a;
	}
#	else
	{
		result.a = 1;
	}
#	endif


	float fogFactor = CalcPixelFogFactor( PIXELFOGTYPE, g_FogParams, g_EyePos_SpecExponent.z, i.worldPos_projPosZ.z, i.worldPos_projPosZ.w );
	return FinalOutput( result, fogFactor, PIXELFOGTYPE, TONEMAP_SCALE_LINEAR );
}