//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"


#include "morphaccumulate_vs30.inc"
#include "morphaccumulate_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( MorphAccumulate, MorphAccumulate_DX9 )

BEGIN_VS_SHADER_FLAGS( MorphAccumulate_DX9, "Help for MorphAccumulate", SHADER_NOT_EDITABLE )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( DELTA,		SHADER_PARAM_TYPE_TEXTURE, "", "position/normal deltas" )
		SHADER_PARAM( SIDESPEED,	SHADER_PARAM_TYPE_TEXTURE, "", "side/speed map" )
		SHADER_PARAM( DIMENSIONS,	SHADER_PARAM_TYPE_VEC3, "", "delta dimensions" )
		SHADER_PARAM( DELTASCALE,	SHADER_PARAM_TYPE_FLOAT, "", "delta scale" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT
	{
		LoadTexture( DELTA );
		LoadTexture( SIDESPEED );
	}

	SHADER_DRAW
	{
		bool bUseConstantBasedAccum = ( g_pHardwareConfig->NumVertexShaderConstants() >= VERTEX_SHADER_FLEX_WEIGHTS + VERTEX_SHADER_MAX_FLEX_WEIGHT_COUNT );

		SHADOW_STATE
		{
			EnableAlphaBlending( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
			pShaderShadow->EnableBlendingSeparateAlpha( true );
			pShaderShadow->BlendFuncSeparateAlpha( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
			pShaderShadow->EnableDepthTest( false );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableAlphaWrites( true );
			pShaderShadow->EnableCulling( false );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
			pShaderShadow->FogMode( SHADER_FOGMODE_DISABLED );

 			DECLARE_STATIC_VERTEX_SHADER( morphaccumulate_vs30 );
			SET_STATIC_VERTEX_SHADER_COMBO( CONSTANTBASEDMORPH, bUseConstantBasedAccum );
 			SET_STATIC_VERTEX_SHADER( morphaccumulate_vs30 );
 
 			DECLARE_STATIC_PIXEL_SHADER( morphaccumulate_ps30 );
			SET_STATIC_PIXEL_SHADER_COMBO( CONSTANTBASEDMORPH, bUseConstantBasedAccum );
			SET_STATIC_PIXEL_SHADER( morphaccumulate_ps30 );
 
 			// NOTE: Color indicates where in the morph accumulator to render into
			// Texcoord0 is the texcoord to read from in the source morph texture
 			// Texcoord1 indicates the strength of the morph target
			int pTexCoord[2] = { 4, 1 };
 			pShaderShadow->VertexShaderVertexFormat( VERTEX_FORMAT_USE_EXACT_FORMAT, 2, pTexCoord, 0 );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, DELTA );
			BindTexture( SHADER_SAMPLER1, SIDESPEED );

			if ( !bUseConstantBasedAccum )
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER2, TEXTURE_MORPH_WEIGHTS );

				int nXOffset = pShaderAPI->GetIntRenderingParameter( INT_RENDERPARM_MORPH_WEIGHT_X_OFFSET );
				int nYOffset = pShaderAPI->GetIntRenderingParameter( INT_RENDERPARM_MORPH_WEIGHT_Y_OFFSET );
				int nSubrectWidth = pShaderAPI->GetIntRenderingParameter( INT_RENDERPARM_MORPH_WEIGHT_SUBRECT_WIDTH );
				int nSubrectHeight = pShaderAPI->GetIntRenderingParameter( INT_RENDERPARM_MORPH_WEIGHT_SUBRECT_HEIGHT );
				float pMorphWeightSubrect[4] = { nXOffset, nYOffset, nSubrectWidth, nSubrectHeight };
				pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, pMorphWeightSubrect );

				int nWidth, nHeight;
				pShaderAPI->GetStandardTextureDimensions( &nWidth, &nHeight, TEXTURE_MORPH_WEIGHTS );
				float pMorphWeightDim[4] = { nWidth, nHeight, 0, 0 };
				pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, pMorphWeightDim );
			}

			SetPixelShaderConstant( 0, DELTASCALE );

 			DECLARE_DYNAMIC_VERTEX_SHADER( morphaccumulate_vs30 );
 			SET_DYNAMIC_VERTEX_SHADER( morphaccumulate_vs30 );
 
 			DECLARE_DYNAMIC_PIXEL_SHADER( morphaccumulate_ps30 );
 			SET_DYNAMIC_PIXEL_SHADER( morphaccumulate_ps30 );
		}
		Draw();
	}
END_SHADER

