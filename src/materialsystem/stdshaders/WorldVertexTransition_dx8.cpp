//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"
#include "convar.h"

#include "worldvertextransition.inc"
#include "worldvertextransition_vs14.inc"
#include "worldvertextransition_seamless.inc"
#include "lightmappedgeneric_vs11.inc"
#include "writevertexalphatodestalpha_vs11.inc"
#include "worldvertextransition_dx8_helper.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( WorldVertexTransition, WorldVertexTransition_DX8 )

ConVar mat_fullbright( "mat_fullbright","0", FCVAR_CHEAT );

BEGIN_VS_SHADER( WorldVertexTransition_DX8,
			  "Help for WorldVertexTransition_DX8" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BASETEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture2", "base texture2 help" )
		SHADER_PARAM( FRAME2, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $baseTexture" )
		SHADER_PARAM( BASETEXTURETRANSFORM2, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$baseTexture texcoord transform" )
		SHADER_PARAM( SELFILLUMTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $detail" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_envmask", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASKSCALE, SHADER_PARAM_TYPE_FLOAT, "1", "envmap mask scale" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "models/shadertest/shader1_normal", "bump map for BASETEXTURE" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
		SHADER_PARAM( ENVMAPCONTRAST, SHADER_PARAM_TYPE_FLOAT, "0.0", "contrast 0 == normal 1 == color*color" )
		SHADER_PARAM( ENVMAPSATURATION, SHADER_PARAM_TYPE_FLOAT, "1.0", "saturation 0 == greyscale 1 == normal" )
		SHADER_PARAM( BUMPBASETEXTURE2WITHBUMPMAP, SHADER_PARAM_TYPE_BOOL, "0", "" )
		SHADER_PARAM( FRESNELREFLECTION, SHADER_PARAM_TYPE_FLOAT, "0.0", "1.0 == mirror, 0.0 == water" )
		SHADER_PARAM( SSBUMP, SHADER_PARAM_TYPE_INTEGER, "0", "whether or not to use alternate bumpmap format with height" )
		SHADER_PARAM( SEAMLESS_SCALE, SHADER_PARAM_TYPE_FLOAT, "0", "Scale factor for 'seamless' texture mapping. 0 means to use ordinary mapping" )
		SHADER_PARAM( BLENDMODULATETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "texture to use r/g channels for blend range for" )
		SHADER_PARAM( BLENDMASKTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$blendmodulatetexture texcoord transform" )
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		if ( IsPC() && g_pHardwareConfig->GetDXSupportLevel() < 80 )
			return "WorldVertexTransition_DX6";
		return 0;
	}

	void SetupVars( WorldVertexTransitionEditor_DX8_Vars_t& info )
	{
		info.m_nBaseTextureVar = BASETEXTURE;
		info.m_nBaseTextureFrameVar = FRAME;
		info.m_nBaseTextureTransformVar = BASETEXTURETRANSFORM;
		info.m_nBaseTexture2Var = BASETEXTURE2;
		info.m_nBaseTexture2FrameVar = FRAME2;
		info.m_nBaseTexture2TransformVar = BASETEXTURETRANSFORM2;
	}

	SHADER_INIT_PARAMS()
	{
		// Initializes FLASHLIGHTTEXTURE + MATERIAL_VAR2_LIGHTING_LIGHTMAP
		WorldVertexTransitionEditor_DX8_Vars_t info;
		SetupVars( info );
		InitParamsWorldVertexTransitionEditor_DX8( params, info );

		// FLASHLIGHTFIXME
		params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight001" );

		if( IsUsingGraphics() && params[ENVMAP]->IsDefined() && !CanUseEditorMaterials() )
		{
			if( stricmp( params[ENVMAP]->GetStringValue(), "env_cubemap" ) == 0 )
			{
				Warning( "env_cubemap used on world geometry without rebuilding map. . ignoring: %s\n", pMaterialName );
				params[ENVMAP]->SetUndefined();
			}
		}
		
		if( !params[ENVMAPMASKSCALE]->IsDefined() )
		{
			params[ENVMAPMASKSCALE]->SetFloatValue( 1.0f );
		}

		if( !params[ENVMAPTINT]->IsDefined() )
		{
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		}

		if( !params[SELFILLUMTINT]->IsDefined() )
		{
			params[SELFILLUMTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		}

		if( !params[DETAILSCALE]->IsDefined() )
		{
			params[DETAILSCALE]->SetFloatValue( 4.0f );
		}

		if( !params[FRESNELREFLECTION]->IsDefined() )
		{
			params[FRESNELREFLECTION]->SetFloatValue( 1.0f );
		}

		if( !params[ENVMAPMASKFRAME]->IsDefined() )
		{
			params[ENVMAPMASKFRAME]->SetIntValue( 0 );
		}
		
		if( !params[ENVMAPFRAME]->IsDefined() )
		{
			params[ENVMAPFRAME]->SetIntValue( 0 );
		}

		if( !params[BUMPFRAME]->IsDefined() )
		{
			params[BUMPFRAME]->SetIntValue( 0 );
		}

		if( !params[ENVMAPCONTRAST]->IsDefined() )
		{
			params[ENVMAPCONTRAST]->SetFloatValue( 0.0f );
		}
		
		if( !params[ENVMAPSATURATION]->IsDefined() )
		{
			params[ENVMAPSATURATION]->SetFloatValue( 1.0f );
		}
		
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

		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
		if( g_pConfig->UseBumpmapping() && params[BUMPMAP]->IsDefined() )
		{
			SET_FLAGS2( MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP );
		}

		if( !params[BUMPBASETEXTURE2WITHBUMPMAP]->IsDefined() )
		{
			params[BUMPBASETEXTURE2WITHBUMPMAP]->SetIntValue( 0 );
		}

		if( !params[DETAILSCALE]->IsDefined() )
		{
			params[DETAILSCALE]->SetFloatValue( 4.0f );
		}

		if( !params[DETAILFRAME]->IsDefined() )
		{
			params[DETAILFRAME]->SetIntValue( 0 );
		}

		if( params[SEAMLESS_SCALE]->IsDefined() && params[SEAMLESS_SCALE]->GetFloatValue() != 0.0f )
		{
			// seamless scale is going to be used, so kill some other features. . might implement with these features later.
			params[DETAIL]->SetUndefined();
			params[BUMPMAP]->SetUndefined();
			params[ENVMAP]->SetUndefined();
		}

		if ( !params[SEAMLESS_SCALE]->IsDefined() )
		{
			// zero means don't do seamless mapping.
			params[SEAMLESS_SCALE]->SetFloatValue( 0.0f );
		}

		if( params[SSBUMP]->IsDefined() && params[SSBUMP]->GetIntValue() != 0 )
		{
			// turn of normal mapping since we have ssbump defined, which 
			// means that we didn't make a dx8 fallback for this material.
			params[BUMPMAP]->SetUndefined();
		}
	}
	SHADER_INIT
	{
		// Loads BASETEXTURE, BASETEXTURE2
		WorldVertexTransitionEditor_DX8_Vars_t info;
		SetupVars( info );
		InitWorldVertexTransitionEditor_DX8( this, params, info );

		// FLASHLIGHTFIXME
		if ( params[FLASHLIGHTTEXTURE]->IsDefined() )
		{
			LoadTexture( FLASHLIGHTTEXTURE );
		}

		if (params[DETAIL]->IsDefined())
		{
			LoadTexture( DETAIL );
		}
		
		if ( g_pHardwareConfig->SupportsPixelShaders_1_4() && params[BLENDMODULATETEXTURE]->IsDefined() )
		{
			LoadTexture( BLENDMODULATETEXTURE );
		}

		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
		if( params[ENVMAP]->IsDefined() && !params[BUMPMAP]->IsDefined() )
		{
			Warning( "must have $bumpmap if you have $envmap for worldvertextransition\n" );
			params[ENVMAP]->SetUndefined();
			params[BUMPMAP]->SetUndefined();
		}
		if( g_pConfig->UseBumpmapping() && params[BUMPMAP]->IsDefined() )
		{
			SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
			SET_FLAGS2( MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP );
			LoadBumpMap( BUMPMAP );
		}
		if( params[ENVMAP]->IsDefined() )
		{
			if( !IS_FLAG_SET( MATERIAL_VAR_ENVMAPSPHERE ) )
			{
				LoadCubeMap( ENVMAP );
			}
			else
			{
				Warning( "$envmapsphere not supported by worldvertextransition\n" );
				params[ENVMAP]->SetUndefined();
			}
		}
	}

	void WriteVertexAlphaToDestAlpha( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		if( pShaderShadow )
		{
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableAlphaWrites( true );
			pShaderShadow->EnableColorWrites( false );

			writevertexalphatodestalpha_vs11_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "writevertexalphatodestalpha_vs11", vshIndex.GetIndex() );
		
			pShaderShadow->SetPixelShader( "writevertexalphatodestalpha_ps11" );
			pShaderShadow->VertexShaderVertexFormat( 
				VERTEX_POSITION | VERTEX_COLOR, 2, 0, 0 );
		}
		else
		{
			writevertexalphatodestalpha_vs11_Dynamic_Index vshIndex;
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();
	}

	void DrawFlashlightPass( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, int passID )
	{
		bool bBump = ( passID == 0 ) && ShouldUseBumpmapping( params ) && params[BUMPMAP]->IsTexture();
		DrawFlashlight_dx80( params, pShaderAPI, pShaderShadow, bBump, BUMPMAP, BUMPFRAME, BUMPTRANSFORM, 
			FLASHLIGHTTEXTURE, FLASHLIGHTTEXTUREFRAME, true, true, passID, BASETEXTURE2, FRAME2 );
	}

	bool ShouldUseBumpmapping( IMaterialVar **params ) 
	{ 
		return g_pConfig->UseBumpmapping() && params[BUMPMAP]->IsDefined(); 
	}

	void DrawFlashlight( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		WriteVertexAlphaToDestAlpha( params, pShaderAPI, pShaderShadow );
		DrawFlashlightPass( params, pShaderAPI, pShaderShadow, 0 );
		DrawFlashlightPass( params, pShaderAPI, pShaderShadow, 1 );
	}
	
	SHADER_DRAW
	{
		bool bLightingOnly = mat_fullbright.GetInt() == 2 && !IS_FLAG_SET( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		bool bSupports14 = g_pHardwareConfig->SupportsPixelShaders_1_4();

		// FLASHLIGHTFIXME: need to make these the same.
		bool hasFlashlight = UsingFlashlight( params );
		if( hasFlashlight )
		{
			DrawFlashlight( params, pShaderAPI, pShaderShadow );
		}
		else if ( params[SEAMLESS_SCALE]->GetFloatValue() != 0.0f )
		{
			// This is the seamless_scale version, which doesn't use $detail or $bumpmap
			SHADOW_STATE
			{
				// three copies of the base texture for seamless blending
				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
				pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
				
				// lightmap
				pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );

				int fmt = VERTEX_POSITION;
				pShaderShadow->VertexShaderVertexFormat( fmt, 2, 0, 0 );

				worldvertextransition_seamless_Static_Index vshIndex;
				pShaderShadow->SetVertexShader( "WorldVertexTransition_Seamless", vshIndex.GetIndex() );

				int pshIndex = 0;
				pShaderShadow->SetPixelShader( "WorldVertexTransition_Seamless", pshIndex );

				FogToFogColor();
			}
			DYNAMIC_STATE
			{
				// Texture 0..2
				if( bLightingOnly )
				{
					pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_GREY );
					pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_GREY );
					pShaderAPI->BindStandardTexture( SHADER_SAMPLER2, TEXTURE_GREY );
				}
				else
				{
					BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
					BindTexture( SHADER_SAMPLER1, BASETEXTURE, FRAME );
					BindTexture( SHADER_SAMPLER2, BASETEXTURE, FRAME );
				}

				// Texture 3 = lightmap
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER3, TEXTURE_LIGHTMAP );

				EnablePixelShaderOverbright( 0, true, true );

				float fSeamlessScale = params[SEAMLESS_SCALE]->GetFloatValue();
				float map_scale[4]= { fSeamlessScale, fSeamlessScale, fSeamlessScale, fSeamlessScale };
				pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, map_scale );

				worldvertextransition_seamless_Dynamic_Index vshIndex;
				vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
				pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
			}
			Draw();
			SHADOW_STATE
			{
				// inherit state from previous pass

				EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}
			DYNAMIC_STATE
			{
				if( !bLightingOnly )
				{
					BindTexture( SHADER_SAMPLER0, BASETEXTURE2, FRAME2 );
					BindTexture( SHADER_SAMPLER1, BASETEXTURE2, FRAME2 );
					BindTexture( SHADER_SAMPLER2, BASETEXTURE2, FRAME2 );
				}
			}
			Draw();
		}
		else if( !params[BUMPMAP]->IsTexture() || !g_pConfig->UseBumpmapping() )
		{
			bool bDetail = params[DETAIL]->IsTexture();
			bool bBlendModulate = params[BLENDMODULATETEXTURE]->IsTexture();
			SHADOW_STATE
			{
				// This is the dx8, non-worldcraft version, non-bumped version
				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
				pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
				if( bDetail )
				{
					pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
				}
				if ( bSupports14 && bBlendModulate )
				{
					pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );
				}

				int fmt = VERTEX_POSITION | VERTEX_COLOR;
				pShaderShadow->VertexShaderVertexFormat( fmt, 2, 0, 0 );

				if ( !bSupports14 )
				{
					worldvertextransition_Static_Index vshIndex;
					pShaderShadow->SetVertexShader( "WorldVertexTransition", vshIndex.GetIndex() );

					int pshIndex = bDetail ? 1 : 0;
					pShaderShadow->SetPixelShader( "WorldVertexTransition", pshIndex );
				}
				else
				{
					worldvertextransition_vs14_Static_Index vshIndex;
					pShaderShadow->SetVertexShader( "WorldVertexTransition_vs14", vshIndex.GetIndex() );

					int pshIndex = bDetail ? 1 : 0;
					pshIndex += bBlendModulate ? 2 : 0;
					pShaderShadow->SetPixelShader( "WorldVertexTransition_ps14", pshIndex );
				}
			
				FogToFogColor();
			}

			DYNAMIC_STATE
			{
				// Texture 1
				if( bLightingOnly )
				{
					pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_GREY );
					pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_GREY );
				}
				else
				{
					BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
					BindTexture( SHADER_SAMPLER1, BASETEXTURE2, FRAME2 );
				}
				if( bDetail )
				{
					BindTexture( SHADER_SAMPLER3, DETAIL, DETAILFRAME );
				}
				if ( bSupports14 && bBlendModulate )
				{
					BindTexture( SHADER_SAMPLER4, BLENDMODULATETEXTURE );
				}

				// always set the transform for detail textures since I'm assuming that you'll
				// always have a detailscale.
				// go ahead and set this even if we don't have a detail texture so the vertex shader doesn't 
				// barf chunks with unitialized data.
				SetVertexShaderTextureScaledTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, BASETEXTURETRANSFORM, DETAILSCALE );

				if ( bSupports14 )
				{
					SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, BLENDMASKTRANSFORM );
				}

				// Texture 3 = lightmap
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER2, TEXTURE_LIGHTMAP );
				
				EnablePixelShaderOverbright( 0, true, true );
				
				SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM );
				SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, BASETEXTURETRANSFORM2 );
				if ( !bSupports14 )
				{
					worldvertextransition_Dynamic_Index vshIndex;
					vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
					pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
				}
				else
				{
					worldvertextransition_vs14_Dynamic_Index vshIndex;
					vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
					pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
				}
			}
			Draw();
		}
		else
		{
			if( params[BUMPBASETEXTURE2WITHBUMPMAP]->GetIntValue() )
			{
				DrawWorldBumpedUsingVertexShader( BASETEXTURE, BASETEXTURETRANSFORM,
					BUMPMAP, BUMPFRAME, BUMPTRANSFORM, ENVMAPMASK, ENVMAPMASKFRAME, ENVMAP, 
					ENVMAPFRAME, ENVMAPTINT, COLOR, ALPHA, ENVMAPCONTRAST, ENVMAPSATURATION, FRAME,
					FRESNELREFLECTION, true, BASETEXTURE2, BASETEXTURETRANSFORM2, FRAME2, false );
			}
			else
			{
				// draw the base texture with everything else you normally would for
				// bumped world materials
				DrawWorldBumpedUsingVertexShader(
					BASETEXTURE, BASETEXTURETRANSFORM,
					BUMPMAP, BUMPFRAME, BUMPTRANSFORM,
					ENVMAPMASK, ENVMAPMASKFRAME, ENVMAP, ENVMAPFRAME, ENVMAPTINT,
					COLOR, ALPHA, ENVMAPCONTRAST, ENVMAPSATURATION, FRAME,
					FRESNELREFLECTION,
					false, -1, -1, -1, false );

				// blend basetexture 2 on top of everything using lightmap alpha.
				SHADOW_STATE
				{
					pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
					pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
					pShaderShadow->VertexShaderVertexFormat( 
						VERTEX_POSITION | VERTEX_COLOR, 2, 0, 0 );
					pShaderShadow->EnableBlending( true );
					pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );

					lightmappedgeneric_vs11_Static_Index vshIndex;
					vshIndex.SetDETAIL( false );
					vshIndex.SetENVMAP( false );
					vshIndex.SetENVMAPCAMERASPACE( false );
					vshIndex.SetENVMAPSPHERE( false );
					vshIndex.SetVERTEXCOLOR( true );
					pShaderShadow->SetVertexShader( "LightmappedGeneric_vs11", vshIndex.GetIndex() );

					pShaderShadow->SetPixelShader( "WorldVertexTransition_BlendBase2" );
					FogToFogColor();
				}
				DYNAMIC_STATE
				{
					if( bLightingOnly )
					{
						pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_GREY );
					}
					else
					{
						BindTexture( SHADER_SAMPLER0, BASETEXTURE2, FRAME2 );
					}
					pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_LIGHTMAP );
					
					float half[4] = { 0.5f, 0.5f, 0.5f, 0.5f };
					pShaderAPI->SetPixelShaderConstant( 4, half );
					EnablePixelShaderOverbright( 0, true, true );
					SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM2 );

					lightmappedgeneric_vs11_Dynamic_Index vshIndex;
					vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
					pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
				}
				Draw();
			}
		}
	}
END_SHADER

