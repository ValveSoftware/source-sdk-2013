//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
				   
#include "lightmappedgeneric_vs11.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


DEFINE_FALLBACK_SHADER( WorldTwoTextureBlend, WorldTwoTextureBlend_DX8 )

BEGIN_VS_SHADER( WorldTwoTextureBlend_DX8, 
			  "Help for WorldTwoTextureBlend_DX8" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM_OVERRIDE( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/WorldTwoTextureBlend", "iris texture", 0 )
		SHADER_PARAM( SELFILLUMTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/WorldTwoTextureBlend_detail", "detail texture" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "1.0", "scale of the detail texture" )
		SHADER_PARAM( DETAIL_ALPHA_MASK_BASE_TEXTURE, SHADER_PARAM_TYPE_BOOL, "0", 
			"If this is 1, then when detail alpha=0, no base texture is blended and when "
			"detail alpha=1, you get detail*base*lightmap" )
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		if ( IsPC() && g_pHardwareConfig->GetDXSupportLevel() < 80 )
			return "WorldTwoTextureBlend_DX6";

		return 0;
	}
	SHADER_INIT
	{
		LoadTexture( FLASHLIGHTTEXTURE );
		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture( BASETEXTURE );
		}

		if (params[DETAIL]->IsDefined())
		{
			LoadTexture( DETAIL );
		}
	}

	SHADER_INIT_PARAMS()
	{
		// FLASHLIGHTFIXME
		params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight001" );

		if( !params[SELFILLUMTINT]->IsDefined() )
		{
			params[SELFILLUMTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		}

		if( !params[DETAIL_ALPHA_MASK_BASE_TEXTURE]->IsDefined() )
		{
			params[DETAIL_ALPHA_MASK_BASE_TEXTURE]->SetIntValue( 0 );
		}
	
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
	}

	const char *GetPixelShaderName( IMaterialVar** params, bool bHasBaseTexture, bool bHasDetailTexture )
	{
		bool bSelfIllum = IS_FLAG_SET(MATERIAL_VAR_SELFILLUM);
		if ( !bHasBaseTexture )
		{
			if ( !bHasDetailTexture )
				return "LightmappedGeneric_NoTexture";

			return "LightmappedGeneric_DetailNoTexture";
		}
		
		if ( !bHasDetailTexture )
		{
			if ( bSelfIllum )
				return "LightmappedGeneric_SelfIlluminated";
			
			return "LightmappedGeneric";
		}
		
		if ( !params[DETAIL_ALPHA_MASK_BASE_TEXTURE]->GetIntValue() )
		{
			if ( bSelfIllum )
				return "WorldTwoTextureBlend_SelfIlluminated";

			return "WorldTwoTextureBlend";
		}

		return "WorldTwoTextureBlend_DetailAlpha";
	}


	SHADER_DRAW
	{
		bool hasFlashlight = UsingFlashlight( params );
		if( hasFlashlight )
		{
			DrawFlashlight_dx80( params, pShaderAPI, pShaderShadow, false, -1, -1, -1, 
				FLASHLIGHTTEXTURE, FLASHLIGHTTEXTUREFRAME, true, false, 0, -1, -1 );
			return;
		}

		bool bHasBaseTexture = params[BASETEXTURE]->IsTexture();
		bool bHasDetailTexture = params[DETAIL]->IsTexture();
		bool bHasVertexColor = IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR );

		SHADOW_STATE
		{
			if ( bHasBaseTexture )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			}
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			if ( bHasDetailTexture )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
			}
			pShaderShadow->EnableBlending( false );

			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 2, 0, 0 );

			// Let the shaders do the fun stuff.
			lightmappedgeneric_vs11_Static_Index vshIndex;
			vshIndex.SetDETAIL( bHasDetailTexture );
			vshIndex.SetENVMAP( false );
			vshIndex.SetENVMAPCAMERASPACE( false );
			vshIndex.SetENVMAPSPHERE( false );
			vshIndex.SetVERTEXCOLOR( bHasVertexColor );
			pShaderShadow->SetVertexShader( "LightmappedGeneric_vs11", vshIndex.GetIndex() );

			const char *pPixelShaderName = GetPixelShaderName( params, bHasBaseTexture, bHasDetailTexture );
			pShaderShadow->SetPixelShader( pPixelShaderName );
			
			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			if ( bHasBaseTexture )
			{
				BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
				SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM );
			}

			pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_LIGHTMAP );

			if ( bHasDetailTexture )
			{
				BindTexture( SHADER_SAMPLER2, DETAIL );
				SetVertexShaderTextureScaledTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, BASETEXTURETRANSFORM, DETAILSCALE );
			}

			SetModulationVertexShaderDynamicState();

			// Dynamic vertex shader index.
			lightmappedgeneric_vs11_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );

			EnablePixelShaderOverbright( 0, true, true );
			SetPixelShaderConstant( 1, SELFILLUMTINT );
		}
		Draw();
	}

END_SHADER

