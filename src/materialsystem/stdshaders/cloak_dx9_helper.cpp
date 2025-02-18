//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"
#include "cloak_dx9_helper.h"
#include "../shaderapidx9/locald3dtypes.h"												   
#include "convar.h"
#include "cpp_shader_constant_register_map.h"
#include "cloak_vs20.inc"
#include "cloak_ps20.inc"
#include "cloak_ps20b.inc"

#ifndef _X360
#include "cloak_vs30.inc"
#include "cloak_ps30.inc"
#endif

static ConVar r_lightwarpidentity( "r_lightwarpidentity", "0", FCVAR_CHEAT );

// FIXME: doesn't support fresnel!
void InitParamsCloak_DX9( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, Cloak_DX9_Vars_t &info )
{
	SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
	SET_FLAGS( MATERIAL_VAR_TRANSLUCENT );

	SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );

	if( !params[info.m_nFresnelReflection]->IsDefined() )
	{
		params[info.m_nFresnelReflection]->SetFloatValue( 1.0f );
	}
	if( !params[info.m_nMasked]->IsDefined() )
	{
		params[info.m_nMasked]->SetIntValue( 0 );
	}
	SET_FLAGS2( MATERIAL_VAR2_NEEDS_POWER_OF_TWO_FRAME_BUFFER_TEXTURE );
}

void InitCloak_DX9( CBaseVSShader *pShader, IMaterialVar** params, Cloak_DX9_Vars_t &info )
{
	if (params[info.m_nBaseTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nBaseTexture, TEXTUREFLAGS_SRGB );
	}

	if (params[info.m_nNormalMap]->IsDefined() )
	{
		pShader->LoadBumpMap( info.m_nNormalMap );
	}

	if ( (info.m_nDiffuseWarpTexture != -1) && params[info.m_nDiffuseWarpTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nDiffuseWarpTexture );
	}

	SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );
}

void DrawCloak_DX9( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
				    IShaderShadow* pShaderShadow, Cloak_DX9_Vars_t &info, VertexCompressionType_t vertexCompression )
{
	bool bIsModel = IS_FLAG_SET( MATERIAL_VAR_MODEL );
	bool bMasked = (params[info.m_nMasked]->GetIntValue() != 0);
	bool hasDiffuseWarp = (info.m_nDiffuseWarpTexture != -1) && params[info.m_nDiffuseWarpTexture]->IsTexture();
	bool hasPhongExponentTexture = (info.m_nPhongExponentTexture != -1) && params[info.m_nPhongExponentTexture]->IsTexture();
	bool hasPhongTintMap = hasPhongExponentTexture && (info.m_nPhongAlbedoTint != -1) && ( params[info.m_nPhongAlbedoTint]->GetIntValue() != 0 );
	bool bHasRimLight = (info.m_nRimLight != -1) && ( params[info.m_nRimLight]->GetIntValue() != 0 );
	bool bHasRimMaskMap = hasPhongExponentTexture && bHasRimLight && (info.m_nRimMask != -1) && ( params[info.m_nRimMask]->GetIntValue() != 0 );

	SHADOW_STATE
	{
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );

		pShader->SetInitialShadowState( );

		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );	// Always SRGB read on base map
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER2, true );	// Refraction map sampler...

		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER5, true );

		pShaderShadow->EnableSRGBWrite( true );

		unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL;
		int nTexCoordCount = 1;
		int userDataSize = 0;
		if( bIsModel )
		{
			userDataSize = 4;
		}
		else
		{
			flags |= VERTEX_TANGENT_S | VERTEX_TANGENT_T;
		}

		// This shader supports compressed vertices, so OR in that flag:
		flags |= VERTEX_FORMAT_COMPRESSED;

		pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );

#ifndef _X360
		if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
		{
			bool bUseStaticControlFlow = g_pHardwareConfig->SupportsStaticControlFlow();

			DECLARE_STATIC_VERTEX_SHADER( cloak_vs20 );
			SET_STATIC_VERTEX_SHADER_COMBO( MODEL,  bIsModel );
			SET_STATIC_VERTEX_SHADER_COMBO( USE_STATIC_CONTROL_FLOW, bUseStaticControlFlow );
			SET_STATIC_VERTEX_SHADER( cloak_vs20 );

			// Bind ps_2_b shader so we can get Phong terms
			if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( cloak_ps20b );
				SET_STATIC_PIXEL_SHADER_COMBO( LIGHTWARPTEXTURE, hasDiffuseWarp );
				SET_STATIC_PIXEL_SHADER( cloak_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( cloak_ps20 );
				SET_STATIC_PIXEL_SHADER_COMBO( LIGHTWARPTEXTURE, hasDiffuseWarp );
				SET_STATIC_PIXEL_SHADER( cloak_ps20 );
			}
		}
#ifndef _X360
		else
		{
			// The vertex shader uses the vertex id stream
			SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );

			DECLARE_STATIC_VERTEX_SHADER( cloak_vs30 );
			SET_STATIC_VERTEX_SHADER_COMBO( MODEL,  bIsModel );
			SET_STATIC_VERTEX_SHADER( cloak_vs30 );

			// Bind ps_2_b shader so we can get Phong terms
			DECLARE_STATIC_PIXEL_SHADER( cloak_ps30 );
			SET_STATIC_PIXEL_SHADER_COMBO( LIGHTWARPTEXTURE, hasDiffuseWarp );
			SET_STATIC_PIXEL_SHADER( cloak_ps30 );
		}
#endif

		pShader->DefaultFog();

		if( bMasked )
		{
			pShader->EnableAlphaBlending( SHADER_BLEND_ONE_MINUS_SRC_ALPHA, SHADER_BLEND_SRC_ALPHA );
		}
	}
	DYNAMIC_STATE
	{
		pShaderAPI->SetDefaultState();

		// Bind textures
		pShader->BindTexture( SHADER_SAMPLER0, info.m_nBaseTexture, 0 );							// Base Map
		pShaderAPI->BindStandardTexture( SHADER_SAMPLER2, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0 );	// Refraction Map
		pShader->BindTexture( SHADER_SAMPLER3, info.m_nNormalMap, info.m_nBumpFrame );				// Normal Map
		pShaderAPI->BindStandardTexture( SHADER_SAMPLER5, TEXTURE_NORMALIZATION_CUBEMAP_SIGNED );	// Normalization cube map

		if ( hasDiffuseWarp )
		{
			if ( r_lightwarpidentity.GetBool() )
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_IDENTITY_LIGHTWARP );
			}
			else
			{
				pShader->BindTexture( SHADER_SAMPLER1, info.m_nDiffuseWarpTexture );					// Light warp texture
			}
		}

		MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();
		int fogIndex = ( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z ) ? 1 : 0;

		LightState_t lightState;
		pShaderAPI->GetDX9LightState( &lightState );

#ifndef _X360
		if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
		{
			bool bUseStaticControlFlow = g_pHardwareConfig->SupportsStaticControlFlow();

			DECLARE_DYNAMIC_VERTEX_SHADER( cloak_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG,    fogIndex );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING,      pShaderAPI->GetCurrentNumBones() > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( NUM_LIGHTS, bUseStaticControlFlow ? 0 : lightState.m_nNumLights );
			SET_DYNAMIC_VERTEX_SHADER( cloak_vs20 );

			// Bind ps_2_b shader so we can get Phong, rim and a cloudier refraction
			if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( cloak_ps20b );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LIGHTS, lightState.m_nNumLights );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITEWATERFOGTODESTALPHA,  fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER( cloak_ps20b );
			}
			else
			{
				// JasonM Hack
				//
				// In general, cloaking on ps_2_0 needs re-working for multipass...yuck...
				//
				int nPS20NumLights = max( lightState.m_nNumLights, 1 );
				DECLARE_DYNAMIC_PIXEL_SHADER( cloak_ps20 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LIGHTS, nPS20NumLights );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITEWATERFOGTODESTALPHA,  fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER( cloak_ps20 );
			}
		}
#ifndef _X360
		else
		{
			pShader->SetHWMorphVertexShaderState( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, VERTEX_SHADER_SHADER_SPECIFIC_CONST_7, SHADER_VERTEXTEXTURE_SAMPLER0 );

			DECLARE_DYNAMIC_VERTEX_SHADER( cloak_vs30 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG,    fogIndex );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING,      pShaderAPI->GetCurrentNumBones() > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( MORPHING,		pShaderAPI->IsHWMorphingEnabled() );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER( cloak_vs30 );

			DECLARE_DYNAMIC_PIXEL_SHADER( cloak_ps30 );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LIGHTS, lightState.m_nNumLights );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITEWATERFOGTODESTALPHA,  fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
			SET_DYNAMIC_PIXEL_SHADER( cloak_ps30 );
		}
#endif

		pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, info.m_nBumpTransform );

		if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE )
		{
			pShader->SetPixelShaderConstant( 27, info.m_nRefractTint );
		}
		else
		{
			pShader->SetPixelShaderConstantGammaToLinear( 27, info.m_nRefractTint );
		}

		pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );

		// Pack phong exponent in with the eye position
		float vEyePos_SpecExponent[4], vFresnelRanges_SpecBoost[4] = {1, 0.5, 1, 1};
		float vSpecularTint[4] = {1, 1, 1, 1}, vRimBoost[4] = {1, 1, 1, 1};
		pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );

		if ( (info.m_nPhongExponent != -1) && params[info.m_nPhongExponent]->IsDefined() )
			vEyePos_SpecExponent[3] = params[info.m_nPhongExponent]->GetFloatValue();		// This overrides the channel in the map
		else
			vEyePos_SpecExponent[3] = 0;													// Use the alpha channel of the normal map for the exponent

		if ( (info.m_nPhongTint != -1 ) && params[info.m_nPhongTint]->IsDefined() )			// Get the tint parameter
			params[info.m_nPhongTint]->GetVecValue(vSpecularTint, 4);

		// Get the rim light power (goes in w of Phong tint)
		if ( bHasRimLight && (info.m_nRimLightPower != -1) && params[info.m_nRimLightPower]->IsDefined() )
		{
			vSpecularTint[3] = params[info.m_nRimLightPower]->GetFloatValue();
			vSpecularTint[3] = max(vSpecularTint[3], 1.0f);	// Make sure this is at least 1
		}

		// Get the rim boost power (goes in w of flashlight position)
		if ( bHasRimLight && (info.m_nRimLightBoost != -1) && params[info.m_nRimLightBoost]->IsDefined() )
		{
			vRimBoost[3] = params[info.m_nRimLightBoost]->GetFloatValue();
		}

		// Rim mask...if this is true, use alpha channel of spec exponent texture to mask the rim term
		if ( bHasRimMaskMap )
		{
			float vRimMaskControl[4] = {0, 0, 0, 0}; // Only x is relevant in shader code
			vRimMaskControl[0] = params[info.m_nRimMask]->GetFloatValue();
			pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_ATTENUATION, vRimMaskControl, 1 );
		}

		// If it's all zeros, there was no constant tint in the vmt
		if ( (vSpecularTint[0] == 0.0f) && (vSpecularTint[1] == 0.0f) && (vSpecularTint[2] == 0.0f) )
		{
			if ( hasPhongTintMap )				// If we have a map to use, tell the shader
			{
				vSpecularTint[0] = -1;
			}
			else								// Otherwise, just tint with white
			{
				vSpecularTint[0] = 1.0f;
				vSpecularTint[1] = 1.0f;
				vSpecularTint[2] = 1.0f;
			}
		}

		if ( (info.m_nPhongFresnelRanges != -1 ) && params[info.m_nPhongFresnelRanges]->IsDefined() )
		{
			params[info.m_nPhongFresnelRanges]->GetVecValue( vFresnelRanges_SpecBoost, 3 );	// Grab optional fresnel range parameters
			// Change fresnel range encoding from (min, mid, max) to ((mid-min)*2, mid, (max-mid)*2)
			vFresnelRanges_SpecBoost[0] = (vFresnelRanges_SpecBoost[1] - vFresnelRanges_SpecBoost[0]) * 2;
			vFresnelRanges_SpecBoost[2] = (vFresnelRanges_SpecBoost[2] - vFresnelRanges_SpecBoost[1]) * 2;
		}

		if ( ( info.m_nPhongBoost != -1 ) &&params[info.m_nPhongBoost]->IsDefined() )		// Grab optional phong boost param
			vFresnelRanges_SpecBoost[3] = params[info.m_nPhongBoost]->GetFloatValue();
		else
			vFresnelRanges_SpecBoost[3] = 1.0f;

		pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1 );
		pShaderAPI->SetPixelShaderConstant( PSREG_FRESNEL_SPEC_PARAMS, vFresnelRanges_SpecBoost, 1 );

		pShaderAPI->SetPixelShaderConstant( PSREG_SPEC_RIM_PARAMS, vSpecularTint, 1 );
		pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_POSITION_RIM_BOOST, vRimBoost, 1 );	// Rim boost in w on non-flashlight pass

		pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );

		// Lighting constants
		
		pShaderAPI->SetPixelShaderStateAmbientLightCube( PSREG_AMBIENT_CUBE, !lightState.m_bAmbientLight );
		pShaderAPI->CommitPixelShaderLighting( PSREG_LIGHT_INFO_ARRAY );

		// Set c0 and c1 to contain first two rows of ViewProj matrix
		VMatrix matView, matProj, matViewProj;
		pShaderAPI->GetMatrix( MATERIAL_VIEW, matView.m[0] );
		pShaderAPI->GetMatrix( MATERIAL_PROJECTION, matProj.m[0] );
		matViewProj = matView * matProj;
		pShaderAPI->SetPixelShaderConstant( 0, matViewProj.m[0], 2 );

		// Cloaking control constants
		float vCloakControls[4] = { params[info.m_nRefractAmount]->GetFloatValue(), params[info.m_nCloakFactor]->GetFloatValue(), 0.0f, 0.0f };
		pShaderAPI->SetPixelShaderConstant( 3, vCloakControls, 1 );
	}
	pShader->Draw();
}
