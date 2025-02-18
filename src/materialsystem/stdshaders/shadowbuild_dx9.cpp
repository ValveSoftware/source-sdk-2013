//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A shader that builds the shadow using render-to-texture
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "mathlib/vmatrix.h"

#include "unlitgeneric_vs20.inc"
#include "shadowbuildtexture_ps20.inc"
#include "shadowbuildtexture_ps20b.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( ShadowBuild, ShadowBuild_DX9 )

BEGIN_VS_SHADER_FLAGS( ShadowBuild_DX9, "Help for ShadowBuild", SHADER_NOT_EDITABLE )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( TRANSLUCENT_MATERIAL, SHADER_PARAM_TYPE_MATERIAL, "", "Points to a material to grab translucency from" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
	}

	SHADER_FALLBACK
	{
		if ( !g_pHardwareConfig->SupportsVertexAndPixelShaders() )
			return "ShadowBuild_DX6";

		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
			return "ShadowBuild_DX8";

		return 0;
	}

	SHADER_INIT
	{
		if ( params[BASETEXTURE]->IsDefined() )
		{
			LoadTexture( BASETEXTURE, TEXTUREFLAGS_SRGB );
		}
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			// Add the alphas into the frame buffer
			EnableAlphaBlending( SHADER_BLEND_ONE, SHADER_BLEND_ONE );

			// base texture.  We just use this for alpha, but enable SRGB read to make everything consistent.
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );

			pShaderShadow->EnableSRGBWrite( true );

			pShaderShadow->EnableAlphaWrites( true );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_ALWAYS );

			// Specify vertex format (note that this shader supports compression)
			unsigned int flags = VERTEX_POSITION | VERTEX_FORMAT_COMPRESSED;
			unsigned int nTexCoordCount = 1;
			unsigned int userDataSize = 0;
			pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );

			DECLARE_STATIC_VERTEX_SHADER( unlitgeneric_vs20 );
			SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR, 0  );
			SET_STATIC_VERTEX_SHADER( unlitgeneric_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( shadowbuildtexture_ps20b );
				SET_STATIC_PIXEL_SHADER( shadowbuildtexture_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( shadowbuildtexture_ps20 );
				SET_STATIC_PIXEL_SHADER( shadowbuildtexture_ps20 );
			}
		}
		DYNAMIC_STATE
		{
			SetModulationVertexShaderDynamicState();

			// Snack important parameters from the original material
			// FIXME: What about alpha modulation? Need a solution for that
			ITexture *pTexture = NULL;
			IMaterialVar **ppTranslucentParams = NULL;
			if (params[TRANSLUCENT_MATERIAL]->IsDefined())
			{
				IMaterial *pMaterial = params[TRANSLUCENT_MATERIAL]->GetMaterialValue();
				if (pMaterial)
				{
					ppTranslucentParams = pMaterial->GetShaderParams();
					if ( ppTranslucentParams[BASETEXTURE]->IsTexture() )
					{
						pTexture = ppTranslucentParams[BASETEXTURE]->GetTextureValue();
					}
				}
			}

			if (pTexture)
			{
				BindTexture( SHADER_SAMPLER0, pTexture, ppTranslucentParams[FRAME]->GetIntValue() );

				Vector4D transformation[2];
				const VMatrix &mat = ppTranslucentParams[BASETEXTURETRANSFORM]->GetMatrixValue();
				transformation[0].Init( mat[0][0], mat[0][1], mat[0][2], mat[0][3] );
				transformation[1].Init( mat[1][0], mat[1][1], mat[1][2], mat[1][3] );
				pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, transformation[0].Base(), 2 ); 
			}
			else
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_LIGHTMAP_FULLBRIGHT );
			}

			float vVertexColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, vVertexColor, 1 );

			// Compute the vertex shader index.
			DECLARE_DYNAMIC_VERTEX_SHADER( unlitgeneric_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER( unlitgeneric_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( shadowbuildtexture_ps20b );
				SET_DYNAMIC_PIXEL_SHADER( shadowbuildtexture_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( shadowbuildtexture_ps20 );
				SET_DYNAMIC_PIXEL_SHADER( shadowbuildtexture_ps20 );
			}
		}
		Draw( );
	}
END_SHADER
