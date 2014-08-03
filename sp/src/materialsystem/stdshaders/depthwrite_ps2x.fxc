//  STATIC: "COLOR_DEPTH"					"0..1"

//	DYNAMIC: "ALPHACLIP"					"0..1"

const float g_AlphaThreshold : register( c0 );

const float2 g_vNearFarPlanes	: register( c1 );
	#define g_flNearPlane			g_vNearFarPlanes.x
	#define g_flFarPlane			g_vNearFarPlanes.y

struct PS_INPUT
{
#if ALPHACLIP
	float2 texCoord0	:	TEXCOORD0;
#endif

#if COLOR_DEPTH
	float4 vWorldPos_projPosZ	: TEXCOORD1;
#endif
};

sampler BaseTextureSampler	: register( s0 );

float4 main( PS_INPUT i ) : COLOR
{
	float4 color = float4( 1, 0, 0, 1 );	// opaque alpha....the color doesn't matter for this shader

#if ALPHACLIP
	color = tex2D( BaseTextureSampler, i.texCoord0 );

	clip( color.a - g_AlphaThreshold );

#endif

#if ( COLOR_DEPTH == 1 )

	return float4( i.vWorldPos_projPosZ.w / g_flFarPlane, 0.0, 0.0, 1.0 );

#else

	return color;

#endif
}
