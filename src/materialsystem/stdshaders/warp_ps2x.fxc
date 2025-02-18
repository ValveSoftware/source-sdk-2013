// DYNAMIC: "DISTORT_TYPE"					"0..2"

#include "shader_constant_register_map.h"
#include "common_ps_fxc.h"

sampler BaseTextureSampler		: register( s0 );


// Faster, but less readable maths.
#define OPTIMISED 1



const float4 g_vWarpParms0		: register( c0 );
#if OPTIMISED
#define stereo_distortion_ScaleX		g_vWarpParms0.x
#define stereo_distortion_OffsetX		g_vWarpParms0.y
#define stereo_distortion_ScaleY		g_vWarpParms0.z
#define stereo_distortion_OffsetY		g_vWarpParms0.w
#else
#define stereo_distortion_grow_outside	g_vWarpParms0.x
#define stereo_distortion_grow_inside	g_vWarpParms0.y
#define stereo_distortion_grow_above	g_vWarpParms0.z
#define stereo_distortion_grow_below	g_vWarpParms0.w
#endif

const float4 g_vWarpParms1		: register( c1 );
#define distortion_l_centre			g_vWarpParms1.xy
#define distortion_r_centre			g_vWarpParms1.zw

const float4 g_vWarpParms2		: register( c2 );
#define distortion_l_coeff0			g_vWarpParms2.x
#define distortion_l_coeff1			g_vWarpParms2.y
#define distortion_l_coeff2			g_vWarpParms2.z
#define distortion_red_coeff0delta	g_vWarpParms2.w

const float4 g_vWarpParms3		: register( c3 );
#define distortion_r_coeff0			g_vWarpParms3.x
#define distortion_r_coeff1			g_vWarpParms3.y
#define distortion_r_coeff2			g_vWarpParms3.z
#define distortion_blue_coeff0delta	g_vWarpParms3.w

const float4 g_vWarpParms4		: register( c4 );
#define aspect_height_over_width_t2	g_vWarpParms4.x


struct PS_INPUT
{
	float2 vBaseTexCoord		: TEXCOORD0;
};


float4 main( PS_INPUT i ) : COLOR
{
	float2 vOriginal = i.vBaseTexCoord.xy;
	float BaseX;

	float2 centre;
	float dcoeff0;
	float dcoeff1;
	float dcoeff2;

#if OPTIMISED
	float stereo_distortion_OffsetX_Corrected;
#else
	float stereo_distortion_grow_left;
	float stereo_distortion_grow_right;
#endif

	if ( vOriginal.x < 0.5 )
	{
		BaseX = 0.25;
#if OPTIMISED
		stereo_distortion_OffsetX_Corrected = stereo_distortion_OffsetX;
#else
		stereo_distortion_grow_left = stereo_distortion_grow_outside;
		stereo_distortion_grow_right = stereo_distortion_grow_inside;
#endif

		centre = distortion_l_centre;
		dcoeff0 = distortion_l_coeff0;
		dcoeff1 = distortion_l_coeff1;
		dcoeff2 = distortion_l_coeff2;
	}
	else
	{
		BaseX = 0.75;
#if OPTIMISED
		stereo_distortion_OffsetX_Corrected = -stereo_distortion_OffsetX;
#else
		stereo_distortion_grow_left = stereo_distortion_grow_inside;
		stereo_distortion_grow_right = stereo_distortion_grow_outside;
#endif

		centre = distortion_r_centre;
		dcoeff0 = distortion_r_coeff0;
		dcoeff1 = distortion_r_coeff1;
		dcoeff2 = distortion_r_coeff2;
	}

	float fLocalAspectHeightOverWidthTimes2 = aspect_height_over_width_t2;

	float2 Delta;
	Delta.x = ( vOriginal.x - BaseX ) * 4.0;
	Delta.y = ( vOriginal.y - 0.5 ) * fLocalAspectHeightOverWidthTimes2;

	// Delta now runs from -1.0 to +1.0, i.e. NDC, of the right/left destination (i.e. left/right half of the rendertarget).
	// Vertically, it has the same scale in pixels as horizontally, but because the aspect ratio is usually not 1:1, the range may be larger or smaller than [-1,+1]

	// Offset by the calibration center.
	Delta -= centre;

	float r2 = Delta.x * Delta.x + Delta.y * Delta.y;

	// case DISTORT_DPOLY3:
    //        {
    //            T rdistortion = T(1.0) / (T(1.0) + r2 * (T(dcoeff[0]) + r2 * (T(dcoeff[1]) + r2 * T(dcoeff[2]))));
    //            xp = xp*rdistortion;
    //            yp = yp*rdistortion;
    //        }
    //        break;

	float rdistortion_r = 1.0;
	float rdistortion_g = 1.0;
	float rdistortion_b = 1.0;
	#if ( DISTORT_TYPE == 1 )
	{
		rdistortion_r = 1.0 / (1.0 + r2 * ((dcoeff0+distortion_red_coeff0delta)  + r2 * (dcoeff1 + r2 * dcoeff2)));
		rdistortion_g = 1.0 / (1.0 + r2 * ( dcoeff0                              + r2 * (dcoeff1 + r2 * dcoeff2)));
		rdistortion_b = 1.0 / (1.0 + r2 * ((dcoeff0+distortion_blue_coeff0delta) + r2 * (dcoeff1 + r2 * dcoeff2)));
	}
	#elif ( DISTORT_TYPE == 2 )
	{
		rdistortion_r = (1.0 + r2 * ((dcoeff0+distortion_red_coeff0delta)  + r2 * (dcoeff1 + r2 * dcoeff2)));
		rdistortion_g = (1.0 + r2 * ( dcoeff0                              + r2 * (dcoeff1 + r2 * dcoeff2)));
		rdistortion_b = (1.0 + r2 * ((dcoeff0+distortion_blue_coeff0delta) + r2 * (dcoeff1 + r2 * dcoeff2)));
	}
	#endif

	float2 DeltaR = Delta * rdistortion_r;
	float2 DeltaG = Delta * rdistortion_g;
	float2 DeltaB = Delta * rdistortion_b;

	// Offset back from the calibration center.
	DeltaR += centre;
	DeltaG += centre;
	DeltaB += centre;

	float inv_aspect = 2.0 / fLocalAspectHeightOverWidthTimes2;

#if OPTIMISED

	float stereo_distortion_ScaleY_corrected = stereo_distortion_ScaleY * inv_aspect;

	DeltaR.x = DeltaR.x * stereo_distortion_ScaleX           + stereo_distortion_OffsetX_Corrected;
	DeltaR.y = DeltaR.y * stereo_distortion_ScaleY_corrected + stereo_distortion_OffsetY;
	DeltaG.x = DeltaG.x * stereo_distortion_ScaleX           + stereo_distortion_OffsetX_Corrected;
	DeltaG.y = DeltaG.y * stereo_distortion_ScaleY_corrected + stereo_distortion_OffsetY;
	DeltaB.x = DeltaB.x * stereo_distortion_ScaleX           + stereo_distortion_OffsetX_Corrected;
	DeltaB.y = DeltaB.y * stereo_distortion_ScaleY_corrected + stereo_distortion_OffsetY;

#else

	// This is now in NDC of the left/right half of the source, if there was no distortion growth.
	// So now include the growth.
	float ScaleX = 2.0f / ( stereo_distortion_grow_left + stereo_distortion_grow_right + 2.0f );
	float ScaleY = 2.0f / ( stereo_distortion_grow_above + stereo_distortion_grow_below + 2.0f );
	float OffsetX = ( stereo_distortion_grow_left - stereo_distortion_grow_right ) * ScaleX * 0.5;
	float OffsetY = ( stereo_distortion_grow_above - stereo_distortion_grow_below ) * ScaleY * 0.5;

	ScaleY *= inv_aspect;

	DeltaR.x = DeltaR.x * ScaleX + OffsetX;
	DeltaR.y = DeltaR.y * ScaleY + OffsetY;
	DeltaG.x = DeltaG.x * ScaleX + OffsetX;
	DeltaG.y = DeltaG.y * ScaleY + OffsetY;
	DeltaB.x = DeltaB.x * ScaleX + OffsetX;
	DeltaB.y = DeltaB.y * ScaleY + OffsetY;

#endif
	
	// Now convert back to useful texcoords.
	float2 PixPosR, PixPosG, PixPosB;
	PixPosR.x = DeltaR.x * 0.25 + BaseX;
	PixPosR.y = DeltaR.y * 0.5 + 0.5;
	PixPosG.x = DeltaG.x * 0.25 + BaseX;
	PixPosG.y = DeltaG.y * 0.5 + 0.5;
	PixPosB.x = DeltaB.x * 0.25 + BaseX;
	PixPosB.y = DeltaB.y * 0.5 + 0.5;

	float4 vFinal = 0.0;
	if ( ( DeltaG.x > -1.0 ) && ( DeltaG.y > -1.0 ) && ( DeltaG.x < 1.0 ) && ( DeltaG.y < 1.0 ) )
	{
		vFinal.r = tex2D( BaseTextureSampler, PixPosR ).r;
		vFinal.g = tex2D( BaseTextureSampler, PixPosG ).g;
		vFinal.b = tex2D( BaseTextureSampler, PixPosB ).b;
	}

#if 0		// useful for checking where the center of distortion is.
	if ( ( r2 > 0.45*0.45 ) && ( r2 < 0.55*0.55 ) )
	{
		vFinal.g = 0.0;
	}
#endif

	return vFinal;
}



