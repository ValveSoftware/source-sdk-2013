// STATIC: "MAXTEXTURESTAGES"	"0..2"
// STATIC: "HASALPHAMASK"		"0..1"
// STATIC: "HASSTATICTEXTURE"	"0..1"

// DYNAMIC: "ADDSTATIC"			"0..1"
// DYNAMIC: "PIXELFOGTYPE"		"0..1"

// When max texture stages is less than 2, we go into multipass mode, but without a static texture, we consolidate back to a single pass by using gray as static
// SKIP: ($MAXTEXTURESTAGES < 2) && ($HASSTATICTEXTURE == 0) && ($ADDSTATIC == 1)

#define TEXTURESTAGES (MAXTEXTURESTAGES + 1)
#define USESTATICTEXTURE (((ADDSTATIC == 1) && (HASSTATICTEXTURE == 1))?(1):(0))

#include "common_ps_fxc.h"



const HALF3 g_StaticAmount	: register( c0 ); //x is static, y is 1.0 - static

sampler PrimarySampler		: register( s0 );
#if( TEXTURESTAGES == 3 )
#	if( (HASALPHAMASK == 1) || (USESTATICTEXTURE == 1) )
		sampler SecondarySampler	: register( s1 );
#		if( (HASALPHAMASK == 1) && (USESTATICTEXTURE == 1) )
			sampler TertiarySampler		: register( s2 );
#		endif
#	endif
#elif( (TEXTURESTAGES > 1) && (HASALPHAMASK == 1) )
	sampler SecondarySampler	: register( s1 );
#endif

struct PS_INPUT
{
	float3 vPrimaryTexCoord			: TEXCOORD0;	

#	if( TEXTURESTAGES == 3 )
#		if( (HASALPHAMASK == 1) || (USESTATICTEXTURE == 1) )
			float2 vSecondaryTexCoord		: TEXCOORD1;
#			if( (HASALPHAMASK == 1) && (USESTATICTEXTURE == 1) )
				float2 vTertiaryTexCoord		: TEXCOORD2;
#			endif
#		endif
#	elif( TEXTURESTAGES > 1 && HASALPHAMASK == 1 )
		float2 vSecondaryTexCoord		: TEXCOORD1;
#	endif
};




HALF4 main( PS_INPUT i ) : COLOR
{
	HALF4 result;
	
#	if( TEXTURESTAGES == 3 ) //we can do everything in one pass
	{
		result.rgb = tex2D(PrimarySampler, i.vPrimaryTexCoord.xy ).rgb;
		
		//mix in static	
#		if( ADDSTATIC == 1 )
		{
			result.rgb *= g_StaticAmount.y; //inverse static on original colors
			
#			if( HASSTATICTEXTURE == 1 )
			{
#				if( HASALPHAMASK == 1 )
					result.rgb += tex2D(TertiarySampler, i.vTertiaryTexCoord ).rgb * g_StaticAmount.x; //static
#				else
					result.rgb += tex2D(SecondarySampler, i.vSecondaryTexCoord ).rgb * g_StaticAmount.x; //static	
#				endif
			}
#			else
			{
				result.rgb += g_StaticAmount.x * 0.25; //mix in gray	
			}
#			endif
		}	
#		endif
		
#		if( HASALPHAMASK == 1 )
		{
			//alpha mask
			result.a = tex2D(SecondarySampler, i.vSecondaryTexCoord ).a;
		}
#		else
		{
			result.a = 1;
		}
#		endif	
	}
#	else //multiple pass configuration
	{
#		if( ADDSTATIC == 1 ) //in multipass configuration and adding static
		{
#			if( HASSTATICTEXTURE == 1 )
				result.rgb = tex2D(PrimarySampler, i.vPrimaryTexCoord.xy ).rgb;
#			endif
			result.a = g_StaticAmount.x; //in multipass, static is achieved by alpha blending the static onto an existing cutout pixel in the destination pixel
		}
#		else //in multipass config on the cutout pass
		{
			result.rgb = tex2D(PrimarySampler, i.vPrimaryTexCoord.xy ).rgb;
			
#			if( HASSTATICTEXTURE == 0 ) //there's no need to do another pass if there's no other texture to mix with, just mix in gray
			{
				result.rgb *= g_StaticAmount.y;
				result.rgb += g_StaticAmount.x * 0.25; //mix in gray
			}
#			endif
			
			result.a = 1;
		}
#		endif
		
#		if( (TEXTURESTAGES > 1) && (HASALPHAMASK == 1) )
			result.a *= tex2D(SecondarySampler, i.vSecondaryTexCoord ).a; //modulate in the alpha instead of setting it so we get blend as well as mask
#		endif
	}
#	endif

	return result;
}