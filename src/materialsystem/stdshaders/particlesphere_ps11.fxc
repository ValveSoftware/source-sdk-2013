
const float3 g_OverbrightFactor		: register( c0 );
const float3 g_SelfIllumTint		: register( c1 );
const float3 g_EnvmapTint			: register( c2 );

sampler BumpmapSampler		: register( s0 );

struct PS_INPUT
{
	float2 vBumpTexCoord			: TEXCOORD0;
	float3 vTangentSpaceLightDir	: TEXCOORD1;
	float3 vAmbientColor			: TEXCOORD2;
	
	float4 vDirLightScale	: COLOR0;
};

float4 main( PS_INPUT i ) : COLOR
{
	float4 baseColor = tex2D( BumpmapSampler, i.vBumpTexCoord );
	
	// Dot the bump normal and the light vector.
	float4 vBumpMapNormal = (baseColor - 0.5);				// The format of the sphere map is 0 to 1, 
															// so this is now -0.5 to 0.5.

	float3 vTangentSpaceLightDir = (i.vTangentSpaceLightDir - 0.5) * 2;	// This is  -1  to 1
	float4 vOutput = dot( vBumpMapNormal, vTangentSpaceLightDir ) + 0.5;
	
	// Scale by the light color outputted by the vertex shader (ie: based on distance).
	vOutput *= i.vDirLightScale;

	// Add ambient.
	vOutput += float4( i.vAmbientColor.x, i.vAmbientColor.y, i.vAmbientColor.z, 0 );

	// Alpha = normal map alpha * vertex alpha
	vOutput.a = baseColor.a * i.vDirLightScale.a;

	return vOutput;
}


