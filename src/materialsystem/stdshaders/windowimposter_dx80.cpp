//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "BaseVSShader.h"

#include "windowimposter_vs11.inc"
#include "windowimposter_ps11.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( WindowImposter, WindowImposter_DX80 )

BEGIN_VS_SHADER( WindowImposter_DX80,
			  "Help for WindowImposter_DX80" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
	END_SHADER_PARAMS

	// Set up anything that is necessary to make decisions in SHADER_FALLBACK.
	SHADER_INIT_PARAMS()
	{
	}

	SHADER_FALLBACK
	{
		if ( IsPC() && g_pHardwareConfig->GetDXSupportLevel() < 80)
			return "WindowImposter_DX60";

		return NULL;
	}

	SHADER_INIT
	{
		LoadCubeMap( ENVMAP );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

			windowimposter_vs11_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "windowimposter_vs11", vshIndex.GetIndex() );

			pShaderShadow->SetPixelShader("windowimposter_ps11");
			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, 0, 0 );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			pShaderShadow->EnableDepthWrites( false );
			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			windowimposter_vs11_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );

			BindTexture( SHADER_SAMPLER0, ENVMAP, -1 );
			SetModulationVertexShaderDynamicState();
		}
		Draw();
	}

END_SHADER
