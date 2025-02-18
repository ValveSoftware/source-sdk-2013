// STATIC: "BASETEXTURE"			"0..1"
// STATIC: "ENVMAP"					"0..1"
// STATIC: "ENVMAPMASK"				"0..1"
// STATIC: "SELFILLUM"				"0..1"
// STATIC: "BASEALPHAENVMAPMASK"	"0..1"

// SKIP: !$ENVMAP && ( $BASEALPHAENVMAPMASK || $ENVMAPMASK )
// SKIP: !$BASETEXTURE && $BASEALPHAENVMAPMASK
// SKIP: $BASEALPHAENVMAPMASK && $ENVMAPMASK
// SKIP: !$BASETEXTURE && $BASEALPHAENVMAPMASK
// SKIP: $SELFILLUM && $BASEALPHAENVMAPMASK
// SKIP: !$BASETEXTURE && $SELFILLUM

const float3 g_OverbrightFactor		: register( c0 );
const float3 g_SelfIllumTint		: register( c1 );
const float3 g_EnvmapTint			: register( c2 );

sampler BaseTextureSampler	: register( s0 );
sampler LightmapSampler		: register( s1 );
sampler EnvmapSampler		: register( s2 );
sampler EnvmapMaskSampler	: register( s3 );

//sampler DetailSampler		: register( s3 );

struct PS_INPUT
{
	float2 baseTexCoord			: TEXCOORD0;
	float2 lightmapTexCoord		: TEXCOORD1;
	float3 envmapTexCoord		: TEXCOORD2;
	float2 envmapMaskTexCoord	: TEXCOORD3;
	float4 vertexColor			: COLOR0;
};

float4 main( PS_INPUT i ) : COLOR
{
	bool bBaseTexture = BASETEXTURE ? true : false;
	bool bEnvmap = ENVMAP ? true : false;
	bool bEnvmapMask = ENVMAPMASK ? true : false;
	bool bSelfIllum = SELFILLUM ? true : false;
	bool bBaseAlphaEnvmapMask = BASEALPHAENVMAPMASK ? true : false;

#if 1
	float4 baseColor = float4( 1.0f, 1.0f, 1.0f, 1.0f );
	if( bBaseTexture )
	{
		baseColor = tex2D( BaseTextureSampler, i.baseTexCoord );
	}

	float3 specularFactor = 1.0f;
	
	if( bEnvmapMask )
	{
		specularFactor *= tex2D( EnvmapMaskSampler, i.envmapMaskTexCoord ).xyz;	
	}
	if( bBaseAlphaEnvmapMask )
	{
		specularFactor *= 1.0 - baseColor.a; // this blows!
	}

	float3 diffuseLighting = tex2D( LightmapSampler, i.lightmapTexCoord );

	float3 albedo = float3( 1.0f, 1.0f, 1.0f );
	float alpha = 1.0f;
	if( bBaseTexture )
	{
		albedo *= baseColor;
		if( !bBaseAlphaEnvmapMask && !bSelfIllum )
		{
			alpha *= baseColor.a;
		}
	}

	// The vertex color contains the modulation color + vertex color combined
	albedo *= i.vertexColor;
	alpha *= i.vertexColor.a; // not sure about this one

	float3 diffuseComponent = ( albedo * diffuseLighting * 2.0f ) * g_OverbrightFactor;

	if( bSelfIllum )
	{
		float3 selfIllumComponent = g_SelfIllumTint * albedo;
		diffuseComponent = lerp( diffuseComponent, selfIllumComponent, baseColor.a );
	}

	float3 specularLighting = float3( 0.0f, 0.0f, 0.0f );

	if( bEnvmap )
	{
		specularLighting = tex2D( EnvmapSampler, i.envmapTexCoord );
		specularLighting *= specularFactor;
		specularLighting *= g_EnvmapTint;
	}

	float3 result = diffuseComponent + specularLighting;
	return float4( result, alpha );
#endif

#if 0
	float4 baseColor = float4( 1.0f, 1.0f, 1.0f, 1.0f );
	float3 diffuseLighting = tex2D( LightmapSampler, i.lightmapTexCoord );

	float3 albedo = float3( 1.0f, 1.0f, 1.0f );
	float alpha = 1.0f;
	albedo *= i.vertexColor;
	alpha *= i.vertexColor.a; // not sure about this one

	float3 diffuseComponent = ( albedo * diffuseLighting * 2.0f ) * g_OverbrightFactor;
	float3 result = diffuseComponent;
	return float4( result, alpha );
#endif

#if 0
	float4 result;

	result.rgb = tex2D( LightmapSampler, i.lightmapTexCoord ).rgb * i.vertexColor.rgb;
	result.a = i.vertexColor.a;
	result.rgb = ( result.rgb * g_OverbrightFactor ) * 2.0f;
	return result;
#endif
}


