// DYNAMIC: "MODE"				"0..9"

// STATIC: "CONVERT_TO_SRGB"	"0..1"	[ps20b][= g_pHardwareConfig->NeedsShaderSRGBConversion()] [PC]
// STATIC: "CONVERT_TO_SRGB"	"0..0"	[= 0] [XBOX]
// STATIC: "LINEAR_TO_SRGB"		"0..1"	[ps20b]

#define HDRTYPE HDR_TYPE_NONE
#include "common_ps_fxc.h"

const float g_Alpha : register( c0 );

sampler BaseTextureSampler	: register( s0 );
sampler BaseTextureSampler2	: register( s1 );

struct PS_INPUT
{
	float2 baseTexCoord				: TEXCOORD0;
};
	   
float3 RGBtoHSV( in float3 rgb )
{
	float3 hsv;
	float fmin, fmax, delta;
	fmin = min( min( rgb.r, rgb.g ), rgb.b );
	fmax = max( max( rgb.r, rgb.g) , rgb.b );
	hsv.b = fmax;				// v
	delta = fmax - fmin;
	if( delta != 0 )
	{
		hsv.g = delta / fmax;		// s
		if( rgb.r == fmax )
			hsv.r = ( rgb.g - rgb.b ) / delta;		// between yellow & magenta
		else if( rgb.g == fmax )
			hsv.r = 2 + ( rgb.b - rgb.r ) / delta;	// between cyan & yellow
		else
			hsv.r = 4 + ( rgb.r - rgb.g ) / delta;	// between magenta & cyan
		hsv.r *= 60;				// degrees
		if( hsv.r < 0 )
			hsv.r += 360;
	}
	else 
	{
		// r = g = b = 0		// s = 0, v is undefined
		hsv.g = 0;
		hsv.r = -1;
	}
	return hsv;
}

float3 HSVtoRGB( in float3 hsv )
{
	int i;
	float3 rgb;
	float h = hsv.r;
	float s = hsv.g;
	float v = hsv.b;
	float f, p, q, t;
	if( s == 0 ) 
	{
		// achromatic (grey)
		rgb.rgb = v;
	}
	else
	{
		h /= 60;			// sector 0 to 5
		i = floor( h );
		f = h - i;			// factorial part of h
		p = v * ( 1 - s );
		q = v * ( 1 - s * f );
		t = v * ( 1 - s * ( 1 - f ) );
		if( h < 1)
		{
			rgb.r = v;
			rgb.g = t;
			rgb.b = p;
		}
		else if( h >= 1 && h < 2 )
		{
			rgb.r = q;
			rgb.g = v;
			rgb.b = p;
		}
		else if( h >= 2 && h < 3 )
		{
			rgb.r = p;
			rgb.g = v;
			rgb.b = t;
		}
		else if( h >= 3 && h < 4 )
		{
			rgb.r = p;
			rgb.g = q;
			rgb.b = v;
		}
		else if( h >= 4 && h < 5 )
		{
			rgb.r = t;
			rgb.g = p;
			rgb.b = v;
		}
		else // if ( h >= 5 )
		{
			rgb.r = v;
			rgb.g = p;
			rgb.b = q;
		}
	}
	return rgb;
}

// We have to run through this input converter on OpenGL if the
// rest of the shader code is expecting sRGB values
float3 SampleTexture( sampler texSampler, float2 tc )
{
	float3 c = tex2D( texSampler, tc ).xyz;

	#if ( LINEAR_TO_SRGB )
	{
		c = LinearToGamma( c );
	}
	#endif

	return c;
}

// We have to run through this output converter on OpenGL if we
// expect to be writing out sRGB values (since sRGB will be forced on)
float3 OutputColor( float3 result )
{
	#if ( LINEAR_TO_SRGB )
	{
		return GammaToLinear( result );
	}
	#endif

	return result;
}


float4 main( PS_INPUT i ) : COLOR
{
	float3 result;
#if MODE == 0
	// negative greyscale of scene * gman
	float3 scene = SampleTexture( BaseTextureSampler, i.baseTexCoord );
	float3 gman = SampleTexture( BaseTextureSampler2, i.baseTexCoord );

	float scale = 1.0f / 3.0f;
	scene.xyz = dot( float3( scale, scale, scale), scene.xyz );
	scene = float3( 1, 1, 1 ) - scene;

	return FinalOutput( float4( OutputColor( scene * gman ), g_Alpha ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
#endif

#if MODE == 1
	float3 scene = SampleTexture( BaseTextureSampler, i.baseTexCoord );
	float3 gman = SampleTexture( BaseTextureSampler2, i.baseTexCoord );
	float scale = 1.0f / 3.0f;
	scene.xyz = dot( float3( scale, scale, scale ), scene.xyz );

	float gmanLum = dot( float3( scale, scale, scale ), gman );
	if( gmanLum < 0.3 )
	{
		result = OutputColor( float3( 1, 1, 1 ) - gman );
		return FinalOutput( float4( result, g_Alpha ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
	}
	else
	{
		result = OutputColor( ( float3( 1, 1, 1 ) - gman ) * scene );
		return FinalOutput( float4( result, g_Alpha ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
	}
#endif

#if MODE == 2
	float3 scene = SampleTexture( BaseTextureSampler, i.baseTexCoord );
	float3 gman = SampleTexture( BaseTextureSampler2, i.baseTexCoord );

	float startRamp = .2;
	float endRamp = .5;
	
	float scale = 1.0f / 3.0f;
	float gmanLum = dot( float3( scale, scale, scale ), gman );
	float sceneLum = dot( float3( scale, scale, scale ), scene );

	float blend = ( gmanLum - startRamp ) * ( 1.0f / ( endRamp - startRamp ) );
	blend = saturate( blend );
	
//	return gmanLum * ( 1.0f - blend ) + scene * blend;

	result = OutputColor( min( gmanLum.xxx, scene ) );
	return FinalOutput( float4( result, g_Alpha ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
#endif

#if MODE == 3
	float3 scene = SampleTexture( BaseTextureSampler, i.baseTexCoord );
	float3 gman = SampleTexture( BaseTextureSampler2, i.baseTexCoord );
	float scale = 1.0f / 3.0f;
	float gmanLum = dot( float3( scale, scale, scale ), gman );
	float sceneLum = dot( float3( scale, scale, scale ), scene );

	float a = 0.0f;
	float b = 0.4f;
	float c = 0.7f;
	float d = 1.0f;

	float blend;
	if( gmanLum < b )
	{
		blend = ( gmanLum - a ) / ( b - a );
	}
	else if( gmanLum > c )
	{
		blend = 1.0f - ( ( gmanLum - c) / ( d - c ) );
	}
	else
	{
		blend = 1.0f;
	}

	blend = saturate( blend );
	
	result = OutputColor( gmanLum.xxx * ( float3( 1, 1, 1 ) - blend.xxx ) + scene * blend.xxx );
	return FinalOutput( float4( result, g_Alpha ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
#endif

#if MODE == 4
	float3 scene = SampleTexture( BaseTextureSampler, i.baseTexCoord );
	float3 gman = SampleTexture( BaseTextureSampler2, i.baseTexCoord );
	float scale = 1.0f / 3.0f;
	float gmanLum = dot( float3( scale, scale, scale ), gman );
	float sceneLum = dot( float3( scale, scale, scale ), scene );

	float a = 0.0f;
	float b = 0.4f;
	float c = 0.7f;
	float d = 1.0f;

	float blend;
	if( gmanLum < b )
	{
		blend = ( gmanLum - a ) / ( b - a );
	}
	else if( gmanLum > c )
	{
		blend = 1.0f - ( ( gmanLum - c) / ( d - c ) );
	}
	else
	{
		blend = 1.0f;
	}

	blend = saturate( blend );

	result = OutputColor( gman * ( float3( 1, 1, 1 ) - blend.xxx ) + scene * blend.xxx );
	return FinalOutput( float4( result, g_Alpha ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
#endif

#if MODE == 5
	float3 scene = SampleTexture( BaseTextureSampler, i.baseTexCoord );
	float3 gman = SampleTexture( BaseTextureSampler2, i.baseTexCoord );
	float scale = 1.0f / 3.0f;
//	float sceneLum = dot( float3( scale, scale, scale ), scene );
	float sceneLum = scene.r;

	if( sceneLum > 0.0f )
	{
		return FinalOutput( float4( OutputColor( scene ), g_Alpha ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
	}
	else
	{
		float3 hsv = RGBtoHSV( gman );

//		float blend = saturate( hsv.b - .5 );
		float blend = hsv.b - .5;
		
		hsv.b *= 1.0f + blend;
		hsv.g *= 1.0f - blend;
		return FinalOutput( float4( OutputColor( HSVtoRGB( hsv ) ), g_Alpha ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
	}
#endif

#if MODE == 6
	float3 scene = SampleTexture( BaseTextureSampler, i.baseTexCoord );
	float3 gman = SampleTexture( BaseTextureSampler2, i.baseTexCoord );
	return FinalOutput( float4( OutputColor( scene + gman ), g_Alpha ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
#endif

#if MODE == 7
	float3 scene = SampleTexture( BaseTextureSampler, i.baseTexCoord );
	return FinalOutput( float4( OutputColor( scene ), g_Alpha ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
#endif

#if MODE == 8
	float3 gman = SampleTexture( BaseTextureSampler2, i.baseTexCoord );
	return FinalOutput( float4( OutputColor( gman ), g_Alpha ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
#endif

#if MODE == 9
	// Fetch textures
	float3 cLayer1 = SampleTexture( BaseTextureSampler, i.baseTexCoord.xy );
	float3 cLayer2 = SampleTexture( BaseTextureSampler2, i.baseTexCoord.xy );

	/*
	// Put colors roughly back into gamma space
	float3 cGammaLayer1 = pow( cLayer1, 0.454545f );
	float3 cGammaLayer2 = pow( cLayer2, 0.454545f );

	// Brightness
	//float flLayer1Brightness = saturate( dot( cGammaLayer1.rgb, float3( 0.3f, 0.59f, 0.11f ) ) );
	//float flLayer2Brightness = saturate( dot( cGammaLayer2.rgb, float3( 0.3f, 0.59f, 0.11f ) ) );
	float flLayer1Brightness = saturate( dot( cGammaLayer1.rgb, float3( 0.333f, 0.334f, 0.333f ) ) );
	float flLayer2Brightness = saturate( dot( cGammaLayer2.rgb, float3( 0.333f, 0.334f, 0.333f ) ) );

	// Blend layers in rough gamma space
	float3 cGammaOverlayResult;
	if ( flLayer1Brightness < 0.5f )
	{
		cGammaOverlayResult.rgb = ( 2.0f * cGammaLayer1.rgb * cGammaLayer2.rgb );
	}
	else
	{
		cGammaOverlayResult.rgb = ( 1.0f - ( 2.0f * ( 1.0f - cGammaLayer1.rgb ) * ( 1.0f - cGammaLayer2.rgb ) ) );
	}

	// Convert back to linear space
	float3 cLinearOverlayResult = pow( cGammaOverlayResult.rgb, 2.2f );
	//*/

	float flLayer1Brightness = saturate( dot( cLayer1.rgb, float3( 0.333f, 0.334f, 0.333f ) ) );
	float flLayer2Brightness = saturate( dot( cLayer2.rgb, float3( 0.333f, 0.334f, 0.333f ) ) );

	// Modify layer 1 to be more contrasty
	cLayer1.rgb = saturate( cLayer1.rgb * cLayer1.rgb * 2.0f );
	float3 cLinearOverlayResult = cLayer1.rgb + cLayer2.rgb * saturate( 1.0f - flLayer1Brightness * 2.0f );

	// Tonemap, fog, etc.
	return FinalOutput( float4( OutputColor( cLinearOverlayResult.rgb ), g_Alpha ), 0, PIXEL_FOG_TYPE_NONE, TONEMAP_SCALE_NONE );
#endif
}
