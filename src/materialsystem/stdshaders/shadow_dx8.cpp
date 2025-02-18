//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//


#include "BaseVSShader.h"

#include "shadow_vs14.inc"
#include "unlitgeneric_vs11.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( Shadow, Shadow_DX8 )

BEGIN_VS_SHADER_FLAGS( Shadow_DX8, "Help for Shadow_DX8", SHADER_NOT_EDITABLE )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		// FIXME: Need fallback for dx5, don't fade out shadows, just pop them out
		/*
		The alpha blending state either must be:
		Src Color * Dst Color + Dst Color * 0	
		(src color = C*A + 1-A)

		or

		// Can't be this, doesn't work with fog
		Src Color * Dst Color + Dst Color * (1-Src Alpha)	
		(src color = C * A, Src Alpha = A)
		*/
	}

	SHADER_FALLBACK
	{
		if ( IsPC() && g_pHardwareConfig->GetDXSupportLevel() < 80 )
		{
			return "Shadow_DX6";
		}
		return 0;
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			if ( g_pHardwareConfig->SupportsPixelShaders_1_4() )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
				pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
				pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
				pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );
			}

			EnableAlphaBlending( SHADER_BLEND_DST_COLOR, SHADER_BLEND_ZERO );
			unsigned int flags = VERTEX_POSITION | VERTEX_COLOR;
			int numTexCoords = 1;
			pShaderShadow->VertexShaderVertexFormat( flags, numTexCoords, 0, 0 );
			if( g_pHardwareConfig->GetDXSupportLevel() >= 81 )
			{
				shadow_vs14_Static_Index vshIndex;
				pShaderShadow->SetVertexShader( "Shadow_vs14", vshIndex.GetIndex() );
				pShaderShadow->SetPixelShader( "Shadow_ps14" );
			}
			else
			{
				unlitgeneric_vs11_Static_Index vshIndex;
				vshIndex.SetDETAIL( false );
				vshIndex.SetENVMAP( false );
				vshIndex.SetENVMAPCAMERASPACE( false );
				vshIndex.SetENVMAPSPHERE( false );
				vshIndex.SetVERTEXCOLOR( true );
				vshIndex.SetSEPARATEDETAILUVS( false );
				pShaderShadow->SetVertexShader( "UnlitGeneric_vs11", vshIndex.GetIndex() );
				pShaderShadow->SetPixelShader( "Shadow" );
			}
			// We need to fog to *white* regardless of overbrighting...
			FogToWhite();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );

			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM );
			SetPixelShaderConstant( 1, COLOR );
			if ( g_pHardwareConfig->GetDXSupportLevel() >= 81 )
			{
				BindTexture( SHADER_SAMPLER1, BASETEXTURE, FRAME );
				BindTexture( SHADER_SAMPLER2, BASETEXTURE, FRAME );
				BindTexture( SHADER_SAMPLER3, BASETEXTURE, FRAME );
				BindTexture( SHADER_SAMPLER4, BASETEXTURE, FRAME );

				// Get texture dimensions...
				int nWidth = 16;
				int nHeight = 16;
				ITexture *pTexture = params[BASETEXTURE]->GetTextureValue();
				if (pTexture)
				{
					nWidth = pTexture->GetActualWidth();
					nHeight = pTexture->GetActualHeight();
				}
				Vector4D vecJitter( 1.0 / nWidth, 1.0 / nHeight, 0.0, 0.0 );
				pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, vecJitter.Base() );

				vecJitter.y *= -1.0f;
				pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_3, vecJitter.Base() );

				shadow_vs14_Dynamic_Index vshIndex;
				vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
				pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
			}
			else
			{
				unlitgeneric_vs11_Dynamic_Index vshIndex;
				vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
				vshIndex.SetSKINNING( pShaderAPI->GetCurrentNumBones() > 0 );
				pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
			}
		}
		Draw( );
	}
END_SHADER
