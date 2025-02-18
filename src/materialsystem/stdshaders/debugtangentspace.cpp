//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "shaderlib/cshader.h"

#define USE_NEW_SHADER //Updating assembly shaders to fxc, this is for A/B testing.

#ifdef USE_NEW_SHADER
#include "debugtangentspace_vs11.inc"
#include "debugtangentspace_vs20.inc"
#include "unlitgeneric_notexture_ps11.inc"
#include "unlitgeneric_notexture_ps20.inc"
#include "unlitgeneric_notexture_ps20b.inc"

#else
#include "debugtangentspace.inc"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_SHADER( DebugTangentSpace, "Help for DebugTangentSpace" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM_OVERRIDE( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/basetexture", "unused", SHADER_PARAM_NOT_EDITABLE )
		SHADER_PARAM_OVERRIDE( FRAME, SHADER_PARAM_TYPE_INTEGER, "0", "unused", SHADER_PARAM_NOT_EDITABLE )
		SHADER_PARAM_OVERRIDE( BASETEXTURETRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "unused", SHADER_PARAM_NOT_EDITABLE )
		SHADER_PARAM_OVERRIDE( COLOR, SHADER_PARAM_TYPE_COLOR, "{255 255 255}", "unused", SHADER_PARAM_NOT_EDITABLE )
		SHADER_PARAM_OVERRIDE( ALPHA, SHADER_PARAM_TYPE_FLOAT, "1.0", "unused", SHADER_PARAM_NOT_EDITABLE )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
	}

	SHADER_INIT
	{
	}

	SHADER_DRAW
	{
		if (g_pHardwareConfig->SupportsVertexAndPixelShaders())
		{
			SHADOW_STATE
			{
				// Set stream format (note that this shader supports compression)
				unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;
				int nTexCoordCount = 0;
				int userDataSize = 4;
				pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );

#ifdef USE_NEW_SHADER
				if( g_pHardwareConfig->GetDXSupportLevel() >= 90 )
				{
					DECLARE_STATIC_VERTEX_SHADER( debugtangentspace_vs20 );
					SET_STATIC_VERTEX_SHADER( debugtangentspace_vs20 );

					if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
					{
						DECLARE_STATIC_PIXEL_SHADER( unlitgeneric_notexture_ps20b );
						SET_STATIC_PIXEL_SHADER( unlitgeneric_notexture_ps20b );
					}
					else
					{
						DECLARE_STATIC_PIXEL_SHADER( unlitgeneric_notexture_ps20 );
						SET_STATIC_PIXEL_SHADER( unlitgeneric_notexture_ps20 );
					}
				}
				else
				{
					DECLARE_STATIC_VERTEX_SHADER( debugtangentspace_vs11 );
					SET_STATIC_VERTEX_SHADER( debugtangentspace_vs11 );

					DECLARE_STATIC_PIXEL_SHADER( unlitgeneric_notexture_ps11 );
					SET_STATIC_PIXEL_SHADER( unlitgeneric_notexture_ps11 );
				}
#else
				debugtangentspace_Static_Index vshIndex;
				pShaderShadow->SetVertexShader( "DebugTangentSpace", vshIndex.GetIndex() );

				pShaderShadow->SetPixelShader( "UnlitGeneric_NoTexture" );
#endif
			}
			DYNAMIC_STATE
			{

#ifdef USE_NEW_SHADER
				if( g_pHardwareConfig->GetDXSupportLevel() >= 90 )
				{					
					DECLARE_DYNAMIC_VERTEX_SHADER( debugtangentspace_vs20 );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
					SET_DYNAMIC_VERTEX_SHADER( debugtangentspace_vs20 );

					if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
					{
						DECLARE_DYNAMIC_PIXEL_SHADER( unlitgeneric_notexture_ps20b );
						SET_DYNAMIC_PIXEL_SHADER( unlitgeneric_notexture_ps20b );
					}
					else
					{
						DECLARE_DYNAMIC_PIXEL_SHADER( unlitgeneric_notexture_ps20 );
						SET_DYNAMIC_PIXEL_SHADER( unlitgeneric_notexture_ps20 );
					}					
				}
				else // legacy hardware
				{
					DECLARE_DYNAMIC_VERTEX_SHADER( debugtangentspace_vs11 );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
					SET_DYNAMIC_VERTEX_SHADER( debugtangentspace_vs11 );

					DECLARE_DYNAMIC_PIXEL_SHADER( unlitgeneric_notexture_ps11 );
					SET_DYNAMIC_PIXEL_SHADER( unlitgeneric_notexture_ps11 );
				}
#else
				debugtangentspace_Dynamic_Index vshIndex;
				vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
				vshIndex.SetSKINNING( pShaderAPI->GetCurrentNumBones() > 0 );
				pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
#endif
			}
			Draw();
		}
	}
END_SHADER

