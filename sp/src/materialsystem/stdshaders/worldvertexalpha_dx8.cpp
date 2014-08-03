//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

#include "worldvertexalpha.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( WorldVertexAlpha, WorldVertexAlpha_DX8 )

BEGIN_VS_SHADER( WorldVertexAlpha_DX8, 
			  "Help for WorldVertexAlpha_DX8" )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
	}
	SHADER_INIT
	{
		// Load the base texture here!
		LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		if( g_pHardwareConfig->SupportsVertexAndPixelShaders() && !UsingEditor( params ) )
		{
			// NOTE: This is the DX8, Non-Hammer version.

			SHADOW_STATE
			{
				// Base time lightmap (Need two texture stages)
				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

				int fmt = VERTEX_POSITION;
				pShaderShadow->VertexShaderVertexFormat( fmt, 2, 0, 0 );

				pShaderShadow->EnableBlending( true );

				// Looks backwards, but this is done so that lightmap alpha = 1 when only
				// using 1 texture (needed for translucent displacements).
				pShaderShadow->BlendFunc( SHADER_BLEND_ONE_MINUS_SRC_ALPHA, SHADER_BLEND_SRC_ALPHA );

				worldvertexalpha_Static_Index vshIndex;
				pShaderShadow->SetVertexShader( "WorldVertexAlpha", vshIndex.GetIndex() );

				pShaderShadow->SetPixelShader( "WorldVertexAlpha" );
				FogToFogColor();
			}

			DYNAMIC_STATE
			{
				// Bind the base texture (Stage0) and lightmap (Stage1)
				BindTexture( SHADER_SAMPLER0, BASETEXTURE );
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_LIGHTMAP );
				EnablePixelShaderOverbright( 0, true, true );

				worldvertexalpha_Dynamic_Index vshIndex;
				vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
				pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
			}

			Draw();
		}
		else
		{
			// NOTE: This is the DX7, Hammer version.
			// FIXME: Gary - you need to write a proper non-fixed function shader for this.
			SHADOW_STATE
			{
				SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
				pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, OVERBRIGHT );

				pShaderShadow->EnableBlending( true );

				// Looks backwards, but this is done so that lightmap alpha = 1 when only
				// using 1 texture (needed for translucent displacements).
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
//				pShaderShadow->BlendFunc( SHADER_BLEND_ONE_MINUS_SRC_ALPHA, SHADER_BLEND_SRC_ALPHA );

				// Use vertex color for Hammer because it puts the blending alpha in the vertices.
				unsigned int colorFlag = 0;
				if( UsingEditor( params ) )
				{
					colorFlag |= SHADER_DRAW_COLOR;
				}

				pShaderShadow->DrawFlags( colorFlag | SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD1 | 
					                      SHADER_DRAW_LIGHTMAP_TEXCOORD0 );
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_SAMPLER1, BASETEXTURE );
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_LIGHTMAP );
			}

			Draw();
		}
	}
END_SHADER
