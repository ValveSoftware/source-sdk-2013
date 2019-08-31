//========== Copyright (c) Valve Corporation, All rights reserved. ==========//

#ifdef PIXELSHADER
	#define VS_OUTPUT PS_INPUT
#endif

struct VS_OUTPUT
{
#ifndef PIXELSHADER
	float4 projPos				: POSITION;	
#endif

	float2 texCoord				: TEXCOORD0;
	float4 worldPos_projPosZ	: TEXCOORD1;	
	float4 argbcolor			: COLOR;

#ifndef PIXELSHADER
	#if !defined( _X360 ) && !defined( SHADER_MODEL_VS_3_0 )
		float fog : FOG;
	#endif
#endif
};

#ifdef PIXELSHADER
	#undef VS_OUTPUT
#endif
