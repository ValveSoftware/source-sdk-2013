//========== Copyright (c) Valve Corporation, All rights reserved. ==========//
//
// Purpose: 
//
//===========================================================================
// STATIC: "PARALLAXCORRECT"			"0..1"

//	DYNAMIC: "DOWATERFOG"				"0..1"

#include "common_vs_fxc.h"

static const int g_FogType = DOWATERFOG;

struct VS_INPUT
{
	float3 vPos				: POSITION;
#if PARALLAXCORRECT
	float4 vNormal			: NORMAL;
#endif
};

struct VS_OUTPUT
{
    float4 projPos			: POSITION;	
#if !defined( _X360 ) && !defined( SHADER_MODEL_VS_3_0 )
	float  fog				: FOG;
#endif
	float3 eyeToVertVector	: TEXCOORD0;
	float4 vertexColor		: COLOR;
	
	float4 worldPos_projPosZ		: TEXCOORD1;		// Necessary for pixel fog
	
#if PARALLAXCORRECT
	float3 worldNormal				: TEXCOORD2;
#endif
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	o.projPos = mul( float4( v.vPos, 1 ), cModelViewProj );

	float3 worldPos = mul( float4( v.vPos, 1 ), cModel[0] );
	o.worldPos_projPosZ = float4( worldPos.xyz, o.projPos.z );
	o.eyeToVertVector = worldPos - cEyePos;
	
#if !defined( _X360 ) && !defined( SHADER_MODEL_VS_3_0 )
	o.fog = CalcFixedFunctionFog( worldPos, g_FogType );
#endif

#if PARALLAXCORRECT
	float3 vObjNormal;
	DecompressVertex_Normal( v.vNormal, vObjNormal );
	o.worldNormal = mul( vObjNormal, ( float3x3 )cModel[0] );
#endif

	o.vertexColor = cModulationColor;
	return o;
}

