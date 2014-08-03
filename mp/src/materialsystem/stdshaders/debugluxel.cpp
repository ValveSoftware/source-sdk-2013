//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "shaderlib/cshader.h"

#define USE_NEW_SHADER //Updating assembly shaders to fxc, this is for A/B testing.

#ifdef USE_NEW_SHADER

#include "unlitgeneric_vs20.inc"
#include "unlitgeneric_ps20.inc"
#include "unlitgeneric_ps20b.inc"

#endif



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_SHADER_FLAGS( DebugLuxels, "Help for DebugLuxels", SHADER_NOT_EDITABLE )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( NOSCALE, SHADER_PARAM_TYPE_BOOL, "0", "fixme" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );

#ifdef USE_NEW_SHADER
		if( g_pHardwareConfig->GetDXSupportLevel() >= 90 )
		{
			SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
		}
#endif
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

		if (IS_FLAG_SET(MATERIAL_VAR_TRANSLUCENT))
		{
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
		}

		if (IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR))
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_COLOR | SHADER_DRAW_LIGHTMAP_TEXCOORD0 );
		else
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_LIGHTMAP_TEXCOORD0 );
#ifdef USE_NEW_SHADER
		if( g_pHardwareConfig->GetDXSupportLevel() >= 90 )
		{
			bool bVertexColor = IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR);

			DECLARE_STATIC_VERTEX_SHADER( unlitgeneric_vs20 );
			SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR, bVertexColor ? 1 : 0  );
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
#endif
	}
	DYNAMIC_STATE
	{
		BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );

		int texCoordScaleX = 1, texCoordScaleY = 1;
		if (!params[NOSCALE]->GetIntValue())
		{
			pShaderAPI->GetLightmapDimensions( &texCoordScaleX, &texCoordScaleY );
		}

#ifdef USE_NEW_SHADER
		if( g_pHardwareConfig->GetDXSupportLevel() >= 90 )
		{
			float vVertexColor[4] = { IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR) ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f };
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

			//texture scale transform
			Vector4D transformation[2];
			transformation[0].Init( (float)texCoordScaleX, 0.0f, 0.0f, 0.0f );
			transformation[1].Init( 0.0f, (float)texCoordScaleY, 0.0f, 0.0f );
			s_pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, transformation[0].Base(), 2 ); 
		}
		else
#endif			
		{
			if (!params[NOSCALE]->GetIntValue())
			{
				pShaderAPI->MatrixMode( MATERIAL_TEXTURE0 );
				pShaderAPI->LoadIdentity( );
				pShaderAPI->ScaleXY( texCoordScaleX, texCoordScaleY );
			}
		}
	}
	Draw();
}
END_SHADER
