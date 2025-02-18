//======= Copyright © 1996-2007, Valve Corporation, All rights reserved. ======

#include "common_vs_fxc.h"

// This could be packed more efficiently if needed.
const float4x2 cXformTexCoord0			: register( c2  );
const float4x2 cXformTexCoord1			: register( c4  );
const float4x2 cXformTexCoord2			: register( c6  );
const float4x2 cXformTexCoord3			: register( c8  );

struct VS_INPUT
{
	float3 vPos						: POSITION;
	float2 vBaseTexCoord			: TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 projPos					: POSITION;	
	float4 texCoord01				: TEXCOORD0;
	float4 texCoord23				: TEXCOORD1;
};

#define texCoord0	texCoord01.xy
#define texCoord1	texCoord01.zw
#define texCoord2	texCoord23.xy
#define texCoord3	texCoord23.zw

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	o.projPos		= float4( v.vPos, 1.0f );

	o.texCoord0		= mul( float3( v.vBaseTexCoord, 1 ), (float3x2)cXformTexCoord0 );
	o.texCoord1		= mul( float3( v.vBaseTexCoord, 1 ), (float3x2)cXformTexCoord1 );
	o.texCoord2		= mul( float3( v.vBaseTexCoord, 1 ), (float3x2)cXformTexCoord2 );
	o.texCoord3		= mul( float3( v.vBaseTexCoord, 1 ), (float3x2)cXformTexCoord3 );

	return o;
}
