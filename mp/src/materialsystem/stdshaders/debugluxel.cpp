//===== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "shaderlib/CShader.h"

#include "debugluxel_ps20b.inc"
#include "debugluxel_ps20.inc"
#include "debugluxel_vs20.inc"


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

		if( g_pHardwareConfig->GetDXSupportLevel() >= 90 )
		{
			SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
		}
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

			DECLARE_STATIC_VERTEX_SHADER( debugluxel_vs20 );
			SET_STATIC_VERTEX_SHADER( debugluxel_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( debugluxel_ps20b );
				SET_STATIC_PIXEL_SHADER( debugluxel_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( debugluxel_ps20 );
				SET_STATIC_PIXEL_SHADER( debugluxel_ps20 );
			}

			SetDefaultBlendingShadowState( BASETEXTURE );
			DisableFog();
			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 2, NULL, 0 );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );

			int texCoordScaleX = 1, texCoordScaleY = 1;
			if (!params[NOSCALE]->GetIntValue())
			{
				pShaderAPI->GetLightmapDimensions( &texCoordScaleX, &texCoordScaleY );
			}

			DECLARE_DYNAMIC_VERTEX_SHADER( debugluxel_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( debugluxel_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( debugluxel_ps20b );
				SET_DYNAMIC_PIXEL_SHADER( debugluxel_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( debugluxel_ps20 );
				SET_DYNAMIC_PIXEL_SHADER( debugluxel_ps20 );
			}

			//texture scale transform
			Vector4D transformation[2];
			transformation[0].Init( texCoordScaleX, 0.0f, 0.0f, 0.0f );
			transformation[1].Init( 0.0f, texCoordScaleY, 0.0f, 0.0f );
			s_pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, transformation[0].Base(), 2 ); 
		}
		Draw();
	}
END_SHADER
