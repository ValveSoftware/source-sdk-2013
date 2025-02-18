//	DYNAMIC: "DOWATERFOG"				"0..1"

//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "common_vs_fxc.h"

static const int g_FogType = DOWATERFOG;

struct VS_INPUT
{
	float3 vPos				: POSITION;
};

struct VS_OUTPUT
{
    float4 projPos			: POSITION;	
#if !defined( _X360 )
	float  fog				: FOG;
#endif
	float3 eyeToVertVector	: TEXCOORD0;
	float4 vertexColor		: COLOR;
	
	float4 worldPos_projPosZ		: TEXCOORD7;		// Necessary for pixel fog
};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	o.projPos = mul( float4( v.vPos, 1 ), cModelViewProj );

	float3 worldPos = mul( float4( v.vPos, 1 ), cModel[0] );
	o.worldPos_projPosZ = float4( worldPos.xyz, o.projPos.z );
	o.eyeToVertVector = worldPos - cEyePos;
#if !defined( _X360 )
	o.fog = CalcFog( worldPos, o.projPos, g_FogType );
#endif
	o.vertexColor = cModulationColor;
	return o;
}

