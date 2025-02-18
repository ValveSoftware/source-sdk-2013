//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "shaderlib/cshader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( DecalModulate, DecalModulate_DX6 )

BEGIN_SHADER( DecalModulate_dx6, 
			  "Help for DecalModulate_dx6" )
			  
	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS
	
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
			pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GREATER, 0.0f );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnablePolyOffset( SHADER_POLYOFFSET_DECAL );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_DST_COLOR, SHADER_BLEND_SRC_COLOR );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 );
			FogToGrey();
		}
		DYNAMIC_STATE
		{
			// This is kinda gross.  We really don't want to render anything here for the flashlight
			// pass since we are multiplying by what is already flashlight lit in the framebuffer.
			// There is no easy way to draw nothing conditionally, so I'll bind grey and multiply
			// which shouldn't change the contents of the framebuffer much.
			if( pShaderAPI->InFlashlightMode() )
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_GREY );
			}
			else
			{
				BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			}
		}
		Draw( );
	}
END_SHADER
