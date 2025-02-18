//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

#include "vortwarp_vs11.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( VortWarp, VortWarp_dx8 )

float QuadraticBezier( float t, float A, float B, float C )
{
	return Lerp( t, Lerp( t, A, B ), Lerp( t, B, C ) );
}

float CubicBezier( float t, float A, float B, float C, float D )
{
	return QuadraticBezier( t, Lerp( t, A, B ), Lerp( t, B, C ), Lerp( t, C, D ) );
}

BEGIN_VS_SHADER( VortWarp_dx8, 
				"Help for VortWarp_dx8" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( SELFILLUMTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
 	    SHADER_PARAM( ENTITYORIGIN, SHADER_PARAM_TYPE_VEC3,"0.0","center if the model in world space" )
 	    SHADER_PARAM( WARPPARAM, SHADER_PARAM_TYPE_FLOAT,"0.0","animation param between 0 and 1" )

		SHADER_PARAM( SELFILLUMMAP, SHADER_PARAM_TYPE_TEXTURE, "", "self-illumination map" )
		SHADER_PARAM( UNLIT, SHADER_PARAM_TYPE_BOOL, "", "" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );

		if( !params[SELFILLUMTINT]->IsDefined() )
			params[SELFILLUMTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );

		if( !params[UNLIT]->IsDefined() )
		{
			params[UNLIT]->SetIntValue( 0 );
		}
		if( !params[SELFILLUMTINT]->IsDefined() )
		{
			params[SELFILLUMTINT]->SetVecValue( 0.0f, 0.0f, 0.0f, 0.0f );
		}
		
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );
	}

	SHADER_FALLBACK
	{	
		if ( g_pHardwareConfig->GetDXSupportLevel() < 80)
			return "Vortwarp_dx7";
		return 0;
	}

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture( BASETEXTURE );
		}
		if( params[SELFILLUMMAP]->IsDefined() )
		{
			LoadTexture( SELFILLUMMAP );
		}
	}

	SHADER_DRAW
	{
		BlendType_t blendType = BT_BLEND;
		if ( params[BASETEXTURE]->IsTexture() )
		{
			blendType = EvaluateBlendRequirements( BASETEXTURE, true );
		}
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

			int fmt = VERTEX_POSITION | VERTEX_NORMAL;

			if (params[BASETEXTURE]->IsTexture())
			{
				SetDefaultBlendingShadowState( BASETEXTURE, true );
			}

			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

			// Set up the vertex shader index.
			vortwarp_vs11_Static_Index vshIndex;
			vshIndex.SetUNLIT( params[UNLIT]->GetIntValue() != 0 );
			vshIndex.SetHALF_LAMBERT( IS_FLAG_SET( MATERIAL_VAR_HALFLAMBERT ) );

			pShaderShadow->SetVertexShader( "vortwarp_vs11", vshIndex.GetIndex() );

			int pshIndex;
			if( blendType == BT_BLEND )
			{
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				pShaderShadow->EnableAlphaWrites( false );
				pshIndex = 1;
			}
			else
			{
				pShaderShadow->EnableAlphaWrites( true );
				pshIndex = 0;
			}

			if( params[UNLIT]->GetIntValue() != 0 )
			{
				pshIndex |= 0x2;
			}

			pShaderShadow->SetPixelShader( "vortwarp_ps11", pshIndex );


			DefaultFog();
		}
		DYNAMIC_STATE
		{
			if (params[BASETEXTURE]->IsTexture())
			{
				BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
				SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM );
			}

			if( params[SELFILLUMMAP]->IsTexture() )
			{
				BindTexture( SHADER_SAMPLER1, SELFILLUMMAP, -1 );
			}

			float warp = params[WARPPARAM]->GetFloatValue();
			float t = warp;
			warp = CubicBezier( warp, 0.0f, 1.0f, 0.0f, 0.0f );
			warp = Lerp( t, warp, 1.0f );
			float warpVec[4] = { warp, warp, warp, warp };
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, warpVec, 1 );
			SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_3, ENTITYORIGIN );
			SetAmbientCubeDynamicStateVertexShader();
			SetModulationPixelShaderDynamicState( 3 );
			EnablePixelShaderOverbright( 0, true, true );
			SetPixelShaderConstant( 1, SELFILLUMTINT );
			float c4[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			if( warp > 0.0f && warp < 1.0f )
			{
				c4[3] = 0.0f;
			}
			pShaderAPI->SetPixelShaderConstant( 4, c4, 1 );

			float c5[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			pShaderAPI->SetPixelShaderConstant( 5, c5, 1 );

			float curTime = pShaderAPI->CurrentTime();
			float selfIllumScroll[4] = { .11f * curTime, .124 * curTime, 0.0f, 0.0f };
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, selfIllumScroll, 1 );

			vortwarp_vs11_Dynamic_Index vshIndex;
			vshIndex.SetSKINNING( pShaderAPI->GetCurrentNumBones() > 0 );
			vshIndex.SetLIGHT_COMBO( 0 );
			vshIndex.SetLIGHT_COMBO( pShaderAPI->GetCurrentLightCombo() );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();
	}
END_SHADER
