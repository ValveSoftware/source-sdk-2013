//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

#include "decalmodulate_vs11.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( DecalModulate, DecalModulate_DX8 )

BEGIN_VS_SHADER( DecalModulate_DX8, "" )
			  
	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		if ( IsPC() && g_pHardwareConfig->GetDXSupportLevel() < 80 )
			return "DecalModulate_DX6";
		return 0;
	}

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableAlphaTest( true );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnablePolyOffset( SHADER_POLYOFFSET_DECAL );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_DST_COLOR, SHADER_BLEND_SRC_COLOR );
			FogToGrey();

			decalmodulate_vs11_Static_Index vshIndex;
			s_pShaderShadow->SetVertexShader( "decalmodulate_vs11", vshIndex.GetIndex() );

			s_pShaderShadow->SetPixelShader( "decalmodulate_ps11", 0 );

			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, 0, 0, 0 );
		}
		DYNAMIC_STATE
		{
			if ( pShaderAPI->InFlashlightMode() )
			{
				// Don't draw anything for the flashlight pass
				Draw( false );
				return;
			}

			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );

			// Set an identity base texture transformation
			Vector4D transformation[2];
			transformation[0].Init( 1.0f, 0.0f, 0.0f, 0.0f );
			transformation[1].Init( 0.0f, 1.0f, 0.0f, 0.0f );
		 	pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, transformation[0].Base(), 2 ); 

			MaterialFogMode_t fogType = s_pShaderAPI->GetSceneFogMode();
			int fogIndex = ( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z ) ? 1 : 0;
			decalmodulate_vs11_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( fogIndex );
			vshIndex.SetSKINNING( 0 );
			s_pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();

	}
END_SHADER
