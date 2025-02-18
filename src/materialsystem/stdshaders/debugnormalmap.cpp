//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"

#define USE_NEW_SHADER //Updating assembly shaders to fxc, this is for A/B testing.



#ifdef USE_NEW_SHADER

#include "unlitgeneric_vs20.inc"
#include "unlitgeneric_ps20.inc"
#include "unlitgeneric_ps20b.inc"

#endif

#include "unlitgeneric_vs11.inc"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER_FLAGS( DebugNormalMap, "Help for DebugNormalMap", SHADER_NOT_EDITABLE )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/WorldDiffuseBumpMap_bump", "bump map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
	}

	SHADER_INIT
	{
	}

	SHADER_FALLBACK
	{
		if( g_pHardwareConfig->GetDXSupportLevel() < 80 )
		{
//			Assert( 0 );
			return "Wireframe";
		}
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

			// Set stream format (note that this shader supports compression)
			unsigned int flags = VERTEX_POSITION | VERTEX_FORMAT_COMPRESSED;
			int nTexCoordCount = 1;
			int userDataSize = 0;
			pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );

#ifdef USE_NEW_SHADER
			if( g_pHardwareConfig->GetDXSupportLevel() >= 90 )
			{
				DECLARE_STATIC_VERTEX_SHADER( unlitgeneric_vs20 );
				SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR, 0  );
				SET_STATIC_VERTEX_SHADER( unlitgeneric_vs20 );

				if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					DECLARE_STATIC_PIXEL_SHADER( unlitgeneric_ps20b );
					SET_STATIC_PIXEL_SHADER( unlitgeneric_ps20b );
				}
				else
				{
					DECLARE_STATIC_PIXEL_SHADER( unlitgeneric_ps20 );
					SET_STATIC_PIXEL_SHADER( unlitgeneric_ps20 );
				}
			}
			else
#endif
			{
				unlitgeneric_vs11_Static_Index vshIndex;
				vshIndex.SetDETAIL( false );
				vshIndex.SetENVMAP( false );
				vshIndex.SetENVMAPCAMERASPACE( false );
				vshIndex.SetENVMAPSPHERE( false );
				vshIndex.SetVERTEXCOLOR( false );
				vshIndex.SetSEPARATEDETAILUVS( false );
				pShaderShadow->SetVertexShader( "unlitgeneric_vs11", vshIndex.GetIndex() );

				pShaderShadow->SetPixelShader( "unlitgeneric" );
			}
		}
		DYNAMIC_STATE
		{
			if ( params[BUMPMAP]->IsTexture() )
			{
				BindTexture( SHADER_SAMPLER0, BUMPMAP, BUMPFRAME );
			}
			else
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_NORMALMAP_FLAT );
			}
			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BUMPTRANSFORM );
#ifdef USE_NEW_SHADER
			if( g_pHardwareConfig->GetDXSupportLevel() >= 90 )
			{
				float vVertexColor[4] = { 0, 0, 0, 0 };
				pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, vVertexColor, 1 );

				DECLARE_DYNAMIC_VERTEX_SHADER( unlitgeneric_vs20 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
				SET_DYNAMIC_VERTEX_SHADER( unlitgeneric_vs20 );

				if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( unlitgeneric_ps20b );
					SET_DYNAMIC_PIXEL_SHADER( unlitgeneric_ps20b );
				}
				else
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( unlitgeneric_ps20 );
					SET_DYNAMIC_PIXEL_SHADER( unlitgeneric_ps20 );
				}
			}
			else
#endif
			{
				unlitgeneric_vs11_Dynamic_Index vshIndex;
				vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
				vshIndex.SetSKINNING( pShaderAPI->GetCurrentNumBones() > 0 );
				pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
			}
		}
		Draw();
	}
END_SHADER

