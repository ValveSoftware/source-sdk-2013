//==================================================================================================
//
// Physically Based Rendering Header for brushes and models
//
//==================================================================================================

// Universal Constants
static const float PI = 3.141592;
static const float ONE_OVER_PI = 0.318309;
static const float EPSILON = 0.00001;

// Shlick's approximation of the Fresnel factor
float3 fresnelSchlick(float3 F0, float cosTheta)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// Shlick's approximation of the Fresnel factor with account for roughness
float3 fresnelSchlickRoughness(float3 F0, float cosTheta, float roughness)
{
    return F0 + max(0.0, (1.0 - roughness) - F0) * pow(1.0 - cosTheta, 5.0);
}

// GGX/Towbridge-Reitz normal distribution function
// Uses Disney's reparametrization of alpha = roughness^2
float ndfGGX(float cosLh, float roughness)
{
    float alpha   = roughness * roughness;
    float alphaSq = alpha * alpha;

    float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
    return alphaSq / (PI * denom * denom);
}

// Single term for separable Schlick-GGX below
float gaSchlickG1(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method
float gaSchlickGGX(float cosLi, float cosLo, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights
    return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}

// Monte Carlo integration, approximate analytic version based on Dimitar Lazarov's work
// https://www.unrealengine.com/en-US/blog/physically-based-shading-on-mobile
float3 EnvBRDFApprox(float3 SpecularColor, float Roughness, float NoV)
{
    const float4 c0 = { -1, -0.0275, -0.572, 0.022 };
    const float4 c1 = { 1, 0.0425, 1.04, -0.04 };
    float4 r = Roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
    float2 AB = float2(-1.04, 1.04) * a004 + r.zw;
    return SpecularColor * AB.x + AB.y;
}

// Compute the matrix used to transform tangent space normals to world space
// This expects DirectX normal maps in Mikk Tangent Space http://www.mikktspace.com
float3x3 compute_tangent_frame(float3 N, float3 P, float2 uv, out float3 T, out float3 B, out float sign_det)
{
    float3 dp1 = ddx(P);
    float3 dp2 = ddy(P);
    float2 duv1 = ddx(uv);
    float2 duv2 = ddy(uv);

    sign_det = dot(dp2, cross(N, dp1)) > 0.0 ? -1 : 1;

    float3x3 M = float3x3(dp1, dp2, cross(dp1, dp2));
    float2x3 inverseM = float2x3(cross(M[1], M[2]), cross(M[2], M[0]));
    T = normalize(mul(float2(duv1.x, duv2.x), inverseM));
    B = normalize(mul(float2(duv1.y, duv2.y), inverseM));
    return float3x3(T, B, N);
}

float GetAttenForLight(float4 lightAtten, int lightNum)
{
#if (NUM_LIGHTS > 1)
    if (lightNum == 1) return lightAtten.y;
#endif

#if (NUM_LIGHTS > 2)
    if (lightNum == 2) return lightAtten.z;
#endif

#if (NUM_LIGHTS > 3)
    if (lightNum == 3) return lightAtten.w;
#endif

    return lightAtten.x;
}

// Calculate direct light for one source
float3 calculateLight(float3 lightIn, float3 lightIntensity, float3 lightOut, float3 normal, float3 fresnelReflectance, float roughness, float metalness, float lightDirectionAngle, float3 albedo)
{
    // Lh
    float3 HalfAngle = normalize(lightIn + lightOut); 
    float cosLightIn = max(0.0, dot(normal, lightIn));
    float cosHalfAngle = max(0.0, dot(normal, HalfAngle));

    // F - Calculate Fresnel term for direct lighting
    float3 F = fresnelSchlick(fresnelReflectance, max(0.0, dot(HalfAngle, lightOut)));

    // D - Calculate normal distribution for specular BRDF
    float D = ndfGGX(cosHalfAngle, roughness);

    // Calculate geometric attenuation for specular BRDF
    float G = gaSchlickGGX(cosLightIn, lightDirectionAngle, roughness);

    // Diffuse scattering happens due to light being refracted multiple times by a dielectric medium
    // Metals on the other hand either reflect or absorb energso diffuse contribution is always, zero
    // To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness
#if SPECULAR
    // Metalness is not used if F0 map is available
    float3 kd = float3(1, 1, 1) - F;
#else
    float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metalness);
#endif

    float3 diffuseBRDF = kd * albedo;

    // Cook-Torrance specular microfacet BRDF
    float3 specularBRDF = (F * D * G) / max(EPSILON, 4.0 * cosLightIn * lightDirectionAngle);
#if LIGHTMAPPED && !FLASHLIGHT
    // Ambient light from static lights is already precomputed in the lightmap. Don't add it again
    return specularBRDF * lightIntensity * cosLightIn;
#else
    return (diffuseBRDF + specularBRDF) * lightIntensity * cosLightIn;
#endif
}

// Get diffuse ambient light
float3 ambientLookupLightmap(float3 normal, float3 EnvAmbientCube[6], float3 textureNormal, float4 lightmapTexCoord1And2, float4 lightmapTexCoord3, sampler LightmapSampler, float4 g_DiffuseModulation)
{
    float2 bumpCoord1;
    float2 bumpCoord2;
    float2 bumpCoord3;

    ComputeBumpedLightmapCoordinates(
            lightmapTexCoord1And2, lightmapTexCoord3.xy,
            bumpCoord1, bumpCoord2, bumpCoord3);

    float3 lightmapColor1 = LightMapSample(LightmapSampler, bumpCoord1);
    float3 lightmapColor2 = LightMapSample(LightmapSampler, bumpCoord2);
    float3 lightmapColor3 = LightMapSample(LightmapSampler, bumpCoord3);

    float3 dp;
    dp.x = saturate(dot(textureNormal, bumpBasis[0]));
    dp.y = saturate(dot(textureNormal, bumpBasis[1]));
    dp.z = saturate(dot(textureNormal, bumpBasis[2]));
    dp *= dp;

    float3 diffuseLighting =    dp.x * lightmapColor1 +
                                dp.y * lightmapColor2 +
                                dp.z * lightmapColor3;

    float sum = dot(dp, float3(1, 1, 1));
    diffuseLighting *= g_DiffuseModulation.xyz / sum;
    return diffuseLighting;
}

float3 ambientLookup(float3 normal, float3 EnvAmbientCube[6], float3 textureNormal, float4 lightmapTexCoord1And2, float4 lightmapTexCoord3, sampler LightmapSampler, float4 g_DiffuseModulation)
{
#if LIGHTMAPPED
    return ambientLookupLightmap(normal, EnvAmbientCube, textureNormal, lightmapTexCoord1And2, lightmapTexCoord3, LightmapSampler, g_DiffuseModulation);
#else
    return PixelShaderAmbientLight(normal, EnvAmbientCube);
#endif
}

// Create an ambient cube from the envmap
void setupEnvMapAmbientCube(out float3 EnvAmbientCube[6], sampler EnvmapSampler)
{
    float4 directionPosX = { 1, 0, 0, 12 }; float4 directionNegX = {-1, 0, 0, 12 };
    float4 directionPosY = { 0, 1, 0, 12 }; float4 directionNegY = { 0,-1, 0, 12 };
    float4 directionPosZ = { 0, 0, 1, 12 }; float4 directionNegZ = { 0, 0,-1, 12 };
    EnvAmbientCube[0] = ENV_MAP_SCALE * texCUBElod(EnvmapSampler, directionPosX).rgb;
    EnvAmbientCube[1] = ENV_MAP_SCALE * texCUBElod(EnvmapSampler, directionNegX).rgb;
    EnvAmbientCube[2] = ENV_MAP_SCALE * texCUBElod(EnvmapSampler, directionPosY).rgb;
    EnvAmbientCube[3] = ENV_MAP_SCALE * texCUBElod(EnvmapSampler, directionNegY).rgb;
    EnvAmbientCube[4] = ENV_MAP_SCALE * texCUBElod(EnvmapSampler, directionPosZ).rgb;
    EnvAmbientCube[5] = ENV_MAP_SCALE * texCUBElod(EnvmapSampler, directionNegZ).rgb;
}

#if PARALLAXOCCLUSION
float2 parallaxCorrect(float2 texCoord, float3 viewRelativeDir, float3 worldSpaceWorldToEye, float3 worldSpaceNormal, sampler depthMap, float parallaxDepth, float parallaxCenter)
{
    float fLength = length( viewRelativeDir );
    float fParallaxLength = sqrt( fLength * fLength - viewRelativeDir.z * viewRelativeDir.z ) / viewRelativeDir.z; 
    float2 vParallaxDirection = normalize(  viewRelativeDir.xy );
    float2 vParallaxOffsetTS = vParallaxDirection * fParallaxLength;
    float fViewDotHorizonFactor = min(saturate(dot(normalize(worldSpaceNormal), normalize(worldSpaceWorldToEye))), 0.5) * 2;
    vParallaxOffsetTS *= saturate(parallaxDepth * fViewDotHorizonFactor);

     // Compute all the derivatives:
    float2 dx = ddx( texCoord );
    float2 dy = ddy( texCoord );

    int nNumSteps = 20;

    float fCurrHeight = 0.0;
    float fStepSize   = 1.0 / (float) nNumSteps;
    float fPrevHeight = 1.0;
    float fNextHeight = 0.0;

    int    nStepIndex = 0;
    bool   bCondition = true;

    float2 vTexOffsetPerStep = fStepSize * vParallaxOffsetTS;
    float2 vTexCurrentOffset = texCoord;
    float  fCurrentBound     = 1.0;
    float  fParallaxAmount   = 0.0;

    float2 pt1 = 0;
    float2 pt2 = 0;

    float2 texOffset2 = 0;

    while ( nStepIndex < nNumSteps ) 
    {
        vTexCurrentOffset -= vTexOffsetPerStep;

        // Sample height map which in this case is stored in the alpha channel of the normal map:
        fCurrHeight = parallaxCenter + tex2Dgrad( depthMap, vTexCurrentOffset, dx, dy ).a;

        fCurrentBound -= fStepSize;

        if ( fCurrHeight > fCurrentBound ) 
        {     
            pt1 = float2( fCurrentBound, fCurrHeight );
            pt2 = float2( fCurrentBound + fStepSize, fPrevHeight );

            texOffset2 = vTexCurrentOffset - vTexOffsetPerStep;

            nStepIndex = nNumSteps + 1;
        }
        else
        {
            nStepIndex++;
            fPrevHeight = fCurrHeight;
        }
    }   // End of while ( nStepIndex < nNumSteps )

    float fDelta2 = pt2.x - pt2.y;
    float fDelta1 = pt1.x - pt1.y;
    fParallaxAmount = (pt1.x * fDelta2 - pt2.x * fDelta1 ) / ( fDelta2 - fDelta1 );
    float2 vParallaxOffset = vParallaxOffsetTS * (1 - fParallaxAmount);
    // The computed texture offset for the displaced point on the pseudo-extruded surface:
    float2 texSample = texCoord - vParallaxOffset;
    return texSample;
}
#endif

float3 worldToRelative(float3 worldVector, float3 surfTangent, float3 surfBasis, float3 surfNormal)
{
   return float3(
       dot(worldVector, surfTangent),
       dot(worldVector, surfBasis),
       dot(worldVector, surfNormal)
   );
}
