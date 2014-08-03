//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "BaseVSShader.h"
#include "debugdrawenvmapmask_vs20.inc"
#include "debugdrawenvmapmask_ps20.inc"
#include "debugdrawenvmapmask_ps20b.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER_FLAGS( DebugDrawEnvmapMask, "Help for DebugDrawEnvmapMask", SHADER_NOT_EDITABLE )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( SHOWALPHA, SHADER_PARAM_TYPE_INTEGER, "0", "" )
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
		if( g_pHardwareConfig->GetDXSupportLevel() < 90 )
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

			DECLARE_STATIC_VERTEX_SHADER( debugdrawenvmapmask_vs20 );
			SET_STATIC_VERTEX_SHADER( debugdrawenvmapmask_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( debugdrawenvmapmask_ps20b );
				SET_STATIC_PIXEL_SHADER( debugdrawenvmapmask_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( debugdrawenvmapmask_ps20 );
				SET_STATIC_PIXEL_SHADER( debugdrawenvmapmask_ps20 );
			}
		}
		DYNAMIC_STATE
		{
			int numBones	= s_pShaderAPI->GetCurrentNumBones();

			DECLARE_DYNAMIC_VERTEX_SHADER( debugdrawenvmapmask_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING,  numBones > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER( debugdrawenvmapmask_vs20 );

			bool bShowAlpha = params[SHOWALPHA]->GetIntValue() ? true : false;
			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( debugdrawenvmapmask_ps20b );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( SHOWALPHA,  bShowAlpha );
				SET_DYNAMIC_PIXEL_SHADER( debugdrawenvmapmask_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( debugdrawenvmapmask_ps20 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( SHOWALPHA,  bShowAlpha );
				SET_DYNAMIC_PIXEL_SHADER( debugdrawenvmapmask_ps20 );
			}

			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM );
		}
		Draw();
	}
END_SHADER
