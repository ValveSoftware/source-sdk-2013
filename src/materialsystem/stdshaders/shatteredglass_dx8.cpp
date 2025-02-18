//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Lightmap only shader
//
// $Header: $
// $NoKeywords: $
//=============================================================================//


#include "BaseVSShader.h"
#include "shatteredglass.inc"
#include "shatteredglass_envmap.inc"
#include "shatteredglass_envmapsphere.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER( ShatteredGlass_DX8,
			  "Help for ShatteredGlass_DX8" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/Detail", "detail" )
		SHADER_PARAM( DETAILTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "detail scale" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( UNLITFACTOR, SHADER_PARAM_TYPE_FLOAT, "0.7", "0.0 == multiply by lightmap, 1.0 == multiply by 1" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		if( !params[ENVMAPTINT]->IsDefined() )
		{
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		}

		if( !params[UNLITFACTOR]->IsDefined() )
		{
			params[UNLITFACTOR]->SetFloatValue( 0.3f );
		}

		// No texture means no self-illum or env mask in base alpha
		if ( !params[BASETEXTURE]->IsDefined() )
		{
			CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		}

		// If in decal mode, no debug override...
		if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
		{
			SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		}
	}

	SHADER_FALLBACK
	{
		if ( IsPC() && g_pHardwareConfig->GetDXSupportLevel() < 80 )
		{
			return "ShatteredGlass_DX7";
		}
		return 0;
	}

	SHADER_INIT
	{
		if ( params[BASETEXTURE]->IsDefined() )
		{
			LoadTexture( BASETEXTURE );

			if ( !params[BASETEXTURE]->GetTextureValue()->IsTranslucent() )
			{
				if ( IS_FLAG_SET( MATERIAL_VAR_BASEALPHAENVMAPMASK ) )
				{
					CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
				}
			}
		}

		if ( params[DETAIL]->IsDefined() )
		{					 
			LoadTexture( DETAIL );
		}

		if ( params[ENVMAP]->IsDefined())
		{
			LoadCubeMap( ENVMAP );
		}
	}

	SHADER_DRAW
	{
		bool bHasEnvmap = params[ENVMAP]->IsTexture();
		bool bHasVertexColor = IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR);
	
		// Base
		SHADOW_STATE
		{
			// alpha test
 			pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

			// Alpha blending, enable alpha blending if the detail texture is translucent
			bool detailIsTranslucent = TextureIsTranslucent( DETAIL, false );
			if ( detailIsTranslucent )
			{
				if ( IS_FLAG_SET( MATERIAL_VAR_ADDITIVE ) )
					EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
				else
					EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}
			else
			{
				SetDefaultBlendingShadowState( BASETEXTURE, true );
			}

			// Base texture and envmapmask
			unsigned int flags = VERTEX_POSITION;
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

			// Lightmap
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

			// Detail texture
			pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );

			// Envmap
			if ( bHasEnvmap )
			{
				flags |= VERTEX_NORMAL;
				pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
			}

			if ( bHasVertexColor )
			{
				flags |= VERTEX_COLOR;
			}

			pShaderShadow->VertexShaderVertexFormat( flags, 3, 0, 0 );

			Assert( !bHasVertexColor );
			if ( !bHasEnvmap )
			{
				shatteredglass_Static_Index vshIndex;
				pShaderShadow->SetVertexShader( "shatteredglass", vshIndex.GetIndex() );

				pShaderShadow->SetPixelShader( "shatteredglass" );
			}
			else
			{
				if ( !IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE) )
				{
					shatteredglass_envmap_Static_Index vshIndex;
					pShaderShadow->SetVertexShader( "shatteredglass_envmap", vshIndex.GetIndex() );
				}
				else
				{
					shatteredglass_envmapsphere_Static_Index vshIndex;
					pShaderShadow->SetVertexShader( "shatteredglass_envmapsphere", vshIndex.GetIndex() );
				}
				pShaderShadow->SetPixelShader( "shatteredglass_envmap" );
			}
			DefaultFog();
		}
		DYNAMIC_STATE
		{
			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM );
			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, DETAILTRANSFORM );

			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_LIGHTMAP );
			BindTexture( SHADER_SAMPLER3, DETAIL );

			if( !bHasEnvmap )
			{
				shatteredglass_Dynamic_Index vshIndex;
				vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
				pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
			}
			else
			{
				BindTexture( SHADER_SAMPLER2, ENVMAP, ENVMAPFRAME );
				if ( !IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE) )
				{
					shatteredglass_envmap_Dynamic_Index vshIndex;
					vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
					pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
				}
				else
				{
					shatteredglass_envmapsphere_Dynamic_Index vshIndex;
					vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
					pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
				}
			}

			if (IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE) || IS_FLAG_SET(MATERIAL_VAR_ENVMAPCAMERASPACE))
			{
				LoadViewMatrixIntoVertexShaderConstant( VERTEX_SHADER_VIEWMODEL );
			}

			SetModulationVertexShaderDynamicState();
			EnablePixelShaderOverbright( 0, true, true );
			SetPixelShaderConstant( 2, ENVMAPTINT );

			// Blend between lightmap + unlit based on the unlit factor to make it look better
			float flUnlitFactor[4];
			flUnlitFactor[0] = flUnlitFactor[1] = flUnlitFactor[2] = flUnlitFactor[3] = params[UNLITFACTOR]->GetFloatValue();
			pShaderAPI->SetPixelShaderConstant( 3, flUnlitFactor, 1 );
		}
		Draw();
	}
END_SHADER
