//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "shaderlib/cshader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( VertexLitGeneric, VertexLitGeneric_DX7 )
DEFINE_FALLBACK_SHADER( Skin_DX9, VertexLitGeneric_DX7 )

BEGIN_SHADER( VertexLitGeneric_DX7, 
			  "Help for VertexLitGeneric_DX7" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture" )
		SHADER_PARAM( SELFILLUMTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_envmask", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( ENVMAPMASKSCALE, SHADER_PARAM_TYPE_FLOAT, "1", "envmap mask scale" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		// FLASHLIGHTFIXME
		params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight001" );

		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );

		if( !params[ENVMAPMASKSCALE]->IsDefined() )
			params[ENVMAPMASKSCALE]->SetFloatValue( 1.0f );

		if( !params[ENVMAPTINT]->IsDefined() )
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );

		if( !params[SELFILLUMTINT]->IsDefined() )
			params[SELFILLUMTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );

		if( !params[DETAILSCALE]->IsDefined() )
			params[DETAILSCALE]->SetFloatValue( 4.0f );

		// No texture means no self-illum or env mask in base alpha
		if ( !params[BASETEXTURE]->IsDefined() )
		{
			CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
			CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		}

		// If in decal mode, no debug override...
		if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
		{
			SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		}

		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_BAKED_LIGHTING_SNAPSHOTS );

		// If mat_specular 0, then get rid of envmap
		if( !g_pConfig->UseSpecular() && params[ENVMAP]->IsDefined() && params[BASETEXTURE]->IsDefined() )
		{
			params[ENVMAP]->SetUndefined();
		}
	}

	SHADER_FALLBACK
	{
		if (g_pHardwareConfig->GetDXSupportLevel() < 70)
			return "VertexLitGeneric_DX6";

		return 0;
	}

	SHADER_INIT
	{
		LoadTexture( FLASHLIGHTTEXTURE );
		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture( BASETEXTURE );

			if (!params[BASETEXTURE]->GetTextureValue()->IsTranslucent())
			{
				CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
				CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
			}
		}

		if (params[DETAIL]->IsDefined())
		{
			LoadTexture( DETAIL );
		}

		// Don't alpha test if the alpha channel is used for other purposes
		if (IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
			CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );
			
		if (params[ENVMAP]->IsDefined())
		{
			if( !IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE) )
				LoadCubeMap( ENVMAP );
			else
				LoadTexture( ENVMAP );

			if( !g_pHardwareConfig->SupportsCubeMaps() )
			{
				SET_FLAGS( MATERIAL_VAR_ENVMAPSPHERE );
			}

			if (params[ENVMAPMASK]->IsDefined())
				LoadTexture( ENVMAPMASK );
		}
	}

	void DrawBaseTimesVertexColor( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			// alpha test
 			pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

			pShaderShadow->EnableCustomPixelPipe( true );
			
			pShaderShadow->CustomTextureStages( 1 );

			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_MODULATE2X, 
				SHADER_TEXARG_TEXTURE, SHADER_TEXARG_VERTEXCOLOR );
		
			// Get alpha from the texture so that alpha blend and alpha test work properly.
			bool bTextureIsTranslucent = TextureIsTranslucent( BASETEXTURE, true );
			if ( bTextureIsTranslucent )
			{
				pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
					SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG1, 
					SHADER_TEXARG_TEXTURE, SHADER_TEXARG_NONE );
			}
 			else
			{
				if ( IsAlphaModulating() )
				{
					pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
						SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG1, 
						SHADER_TEXARG_CONSTANTCOLOR, SHADER_TEXARG_NONE );
				}
				else
				{
					pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
						SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG1, 
						SHADER_TEXARG_ONE, SHADER_TEXARG_NONE );
				}
			}

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			int flags = SHADER_DRAW_POSITION | SHADER_DRAW_NORMAL | SHADER_DRAW_TEXCOORD0;
			pShaderShadow->DrawFlags( flags );
			DefaultFog();

			if ( IsAlphaModulating() || IsColorModulating() )
			{
				pShaderShadow->CustomTextureStages( 2 );

				if ( IsColorModulating() )
				{
					pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
						SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_MODULATE, 
						SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_CONSTANTCOLOR );
				}
				else
				{
					pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
						SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_SELECTARG1,
						SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_NONE );
				}

				// Get alpha from the texture so that alpha blend and alpha test work properly.
				if ( IsAlphaModulating() && bTextureIsTranslucent )
				{
					pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
						SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_MODULATE, 
						SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_CONSTANTCOLOR );
				}
				else
				{
					pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
						SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG1,
						SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_NONE );
				}
			}

			SetDefaultBlendingShadowState( BASETEXTURE, true );
		}
		DYNAMIC_STATE
		{
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE0, BASETEXTURETRANSFORM );
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			SetModulationDynamicState();
		}
		Draw();
	}

	void DrawVertexColorNoBase( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableCustomPixelPipe( true );
			
			pShaderShadow->CustomTextureStages( 1 );

			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_MODULATE2X, 
				SHADER_TEXARG_ONE, SHADER_TEXARG_VERTEXCOLOR );
		
			int flags = SHADER_DRAW_POSITION;
			pShaderShadow->DrawFlags( flags );
			DefaultFog();
		}
		DYNAMIC_STATE
		{
//			SetModulationDynamicState();
		}
		Draw();
	}

	void DrawBaseTimesBakedVertexLighting( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			// alpha test
 			pShaderShadow->EnableAlphaTest( IS_FLAG_SET( MATERIAL_VAR_ALPHATEST ) );

			// base
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			int flags = SHADER_DRAW_POSITION;
			if (params[BASETEXTURE]->IsTexture())
			{
				flags |= SHADER_DRAW_TEXCOORD1;
			}
			pShaderShadow->DrawFlags( flags );
			DefaultFog();

			pShaderShadow->EnableCustomPixelPipe( true );
			pShaderShadow->CustomTextureStages( 1 );

			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_COLOR, 
				SHADER_TEXOP_MODULATE2X,
				SHADER_TEXARG_SPECULARCOLOR, SHADER_TEXARG_TEXTURE );

			// Get alpha from the texture so that alpha blend and alpha test work properly.
			bool bTextureIsTranslucent = TextureIsTranslucent( BASETEXTURE, true );
			if ( bTextureIsTranslucent )
			{
				pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
					SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG1, 
					SHADER_TEXARG_TEXTURE, SHADER_TEXARG_NONE );
			}
			else
			{
				if ( IsAlphaModulating() )
				{
					pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
						SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG1, 
						SHADER_TEXARG_CONSTANTCOLOR, SHADER_TEXARG_NONE );
				}
				else
				{
					pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
						SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG1, 
						SHADER_TEXARG_ONE, SHADER_TEXARG_NONE );
				}
			}

			if ( IsAlphaModulating() || IsColorModulating() )
			{
				pShaderShadow->CustomTextureStages( 2 );

				if ( IsColorModulating())
				{
					pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
						SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_MODULATE, 
						SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_CONSTANTCOLOR );
				}
				else
				{
					pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
						SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_SELECTARG1,
						SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_NONE );
				}

				// Get alpha from the texture so that alpha blend and alpha test work properly.
				if ( IsAlphaModulating() && bTextureIsTranslucent )
				{
					pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
						SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_MODULATE, 
						SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_CONSTANTCOLOR );
				}
				else
				{
					pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
						SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG1,
						SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_NONE );
				}
			}

			SetDefaultBlendingShadowState( BASETEXTURE, true );
		}
		DYNAMIC_STATE
		{
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE1, BASETEXTURETRANSFORM );
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			SetModulationDynamicState();
		}
		Draw();
	}

	void DrawBakedVertexLightingNoBase( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			int flags = SHADER_DRAW_POSITION;
			pShaderShadow->DrawFlags( flags );
			DefaultFog();

			pShaderShadow->EnableCustomPixelPipe( true );
			pShaderShadow->CustomTextureStages( 1 );

			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_COLOR, 
				SHADER_TEXOP_MODULATE2X,
				SHADER_TEXARG_SPECULARCOLOR, SHADER_TEXARG_NONE );

			// Alpha isn't used, it doesn't matter what we set it to.
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG1, 
				SHADER_TEXARG_NONE, SHADER_TEXARG_NONE );
		}
		DYNAMIC_STATE
		{
		}
		Draw();
	}

	SHADER_DRAW
	{
		bool bBakedLighting = IS_FLAG2_SET( MATERIAL_VAR2_USE_FIXED_FUNCTION_BAKED_LIGHTING );
		bool hasFlashlight = UsingFlashlight( params );

		if( hasFlashlight )
		{
			DrawFlashlight_dx70( params, pShaderAPI, pShaderShadow, FLASHLIGHTTEXTURE, FLASHLIGHTTEXTUREFRAME );
			return;
		}
		// Pass 1 : Base * lightmap or just lightmap
		if ( params[BASETEXTURE]->IsTexture() )
		{
			// Draw base times lighting.
			// Lighting is either sent down per vertex from the app, or it's in the second
			// stream as color values.
			if( bBakedLighting )
			{
				DrawBaseTimesBakedVertexLighting( params, pShaderAPI, pShaderShadow );
			}
			else
			{
				DrawBaseTimesVertexColor( params, pShaderAPI, pShaderShadow );
			}

			// Detail map
			FixedFunctionMultiplyByDetailPass(
				BASETEXTURE, FRAME, BASETEXTURETRANSFORM, DETAIL, DETAILSCALE );

			// Draw the selfillum pass
			if ( IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) )
			{
				FixedFunctionSelfIlluminationPass( 
					SHADER_SAMPLER0, BASETEXTURE, FRAME, BASETEXTURETRANSFORM, SELFILLUMTINT );
			}
		}
		else
		{
			if( bBakedLighting )
			{
				DrawBakedVertexLightingNoBase( params, pShaderAPI, pShaderShadow );
			}
			else
			{
				DrawVertexColorNoBase( params, pShaderAPI, pShaderShadow );
			}

			FixedFunctionMultiplyByDetailPass(
				BASETEXTURE, FRAME, BASETEXTURETRANSFORM, DETAIL, DETAILSCALE );
		}
		 

		// Pass 2 : Masked environment map
		if ( params[ENVMAP]->IsTexture() && (IS_FLAG_SET(MATERIAL_VAR_MULTIPASS)) )
		{
			FixedFunctionAdditiveMaskedEnvmapPass( 
				ENVMAP, ENVMAPMASK, BASETEXTURE,
				ENVMAPFRAME, ENVMAPMASKFRAME, FRAME, 
				BASETEXTURETRANSFORM, ENVMAPMASKSCALE, ENVMAPTINT );
		}
		
	}
END_SHADER
