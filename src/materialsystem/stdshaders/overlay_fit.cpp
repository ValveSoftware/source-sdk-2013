//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "BaseVSShader.h"
#include "convar.h"

#include "overlay_fit_vs11.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// FIXME: Need to make a dx9 version so that "CENTROID" works.

BEGIN_VS_SHADER_FLAGS( Overlay_Fit, "Help for TerrainTest2", SHADER_NOT_EDITABLE )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	// Set up anything that is necessary to make decisions in SHADER_FALLBACK.
	SHADER_INIT_PARAMS()
	{
		// No texture means no self-illum or env mask in base alpha
		CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		
		// If in decal mode, no debug override...
		if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
		{
			SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		}

		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
	}

	SHADER_FALLBACK
	{
		if ( IsPC() && !g_pHardwareConfig->SupportsVertexAndPixelShaders() )
			return "WorldTwoTextureBlend";

		return 0;
	}

	SHADER_INIT
	{
		CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );	// BASETEXTURE
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );	// LIGHTMAP
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );	// BASETEXTURE

			// texcoord0 : base texcoord
			// texcoord1 : alpha texcoord (mapped to fill the overlay)
			unsigned int flags = VERTEX_POSITION | VERTEX_COLOR;
			int numTexCoords = 3;
			pShaderShadow->VertexShaderVertexFormat( flags, numTexCoords, 0, 0 );

			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );

			pShaderShadow->EnablePolyOffset( SHADER_POLYOFFSET_DECAL );

			overlay_fit_vs11_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "Overlay_Fit_vs11", vshIndex.GetIndex() );

			pShaderShadow->SetPixelShader ( "Overlay_Fit_ps11", 0 );

			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_LIGHTMAP );
			BindTexture( SHADER_SAMPLER2, BASETEXTURE, FRAME );

			overlay_fit_vs11_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();
	}
END_SHADER

