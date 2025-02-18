//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "shaderlib/cshader.h"
#include "convar.h"
#include "debugdrawdepth_vs20.inc"
#include "debugdrawdepth_ps20.inc"
#include "debugdrawdepth_ps20b.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar mat_debugdepthmode( "mat_debugdepthmode", "0" );
static ConVar mat_debugdepthval( "mat_debugdepthval", "128.0f" );
static ConVar mat_debugdepthvalmax( "mat_debugdepthvalmax", "256.0f" );

BEGIN_SHADER_FLAGS( DebugDepth, "Help for DebugDepth", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
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
			return "WireFrame";
		}
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			// Set stream format (note that this shader supports compression)
			unsigned int flags = VERTEX_POSITION | VERTEX_FORMAT_COMPRESSED;
			int nTexCoordCount = 1;
			int userDataSize = 0;
			pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );

			DECLARE_STATIC_VERTEX_SHADER( debugdrawdepth_vs20 );
			SET_STATIC_VERTEX_SHADER( debugdrawdepth_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( debugdrawdepth_ps20b );
				SET_STATIC_PIXEL_SHADER( debugdrawdepth_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( debugdrawdepth_ps20 );
				SET_STATIC_PIXEL_SHADER( debugdrawdepth_ps20 );
			}
		}
		DYNAMIC_STATE
		{
			DECLARE_DYNAMIC_VERTEX_SHADER( debugdrawdepth_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING,  s_pShaderAPI->GetCurrentNumBones() > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER( debugdrawdepth_vs20 );

			Vector4D vecZFilter( 0, 0, 0, 1 );
			int nDepthMode = mat_debugdepthmode.GetInt();
			if ( nDepthMode > 1 )
			{
				nDepthMode = 0;
			}

			vecZFilter[nDepthMode] = 1;
			s_pShaderAPI->SetPixelShaderConstant( 1, vecZFilter.Base() );

			Vector4D vecModulationColor( 0, 0, 0, 1 );
			if ( IS_FLAG_SET( MATERIAL_VAR_DECAL ) )
			{
				vecModulationColor[0] = 0;
				vecModulationColor[1] = 1;
				vecModulationColor[2] = 1;
			}
			else
			{
				vecModulationColor[0] = 1;
				vecModulationColor[1] = 1;
				vecModulationColor[2] = 1;
			}
			s_pShaderAPI->SetPixelShaderConstant( 2, vecModulationColor.Base() );

			float flDepthFactor = mat_debugdepthval.GetFloat();
			float flDepthFactorMax = mat_debugdepthvalmax.GetFloat();
			if ( flDepthFactor == 0 )
			{
				flDepthFactor = 1.0f;
			}
			Vector4D vecZFactor( (flDepthFactorMax - flDepthFactor), flDepthFactor, 1, 1 );
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, vecZFactor.Base() );
		}
		Draw();
	}
END_SHADER

