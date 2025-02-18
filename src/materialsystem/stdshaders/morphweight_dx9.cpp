//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"


#include "morphweight_vs30.inc"
#include "morphweight_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//#define V1 1

DEFINE_FALLBACK_SHADER( MorphWeight, MorphWeight_DX9 )

BEGIN_VS_SHADER_FLAGS( MorphWeight_DX9, "Help for morphweight", SHADER_NOT_EDITABLE )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT
	{
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableDepthTest( false );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableAlphaWrites( true );
			pShaderShadow->EnableCulling( false );
			pShaderShadow->FogMode( SHADER_FOGMODE_DISABLED );

 			DECLARE_STATIC_VERTEX_SHADER( morphweight_vs30 );
 			SET_STATIC_VERTEX_SHADER( morphweight_vs30 );
 
 			DECLARE_STATIC_PIXEL_SHADER( morphweight_ps30 );
 			SET_STATIC_PIXEL_SHADER( morphweight_ps30 );
 
			// Texcoord0 is the texcoord to write the weights into
 			// Texcoord1 contains the morph weights
			int pTexCoord[2] = { 2, 4 };

 			pShaderShadow->VertexShaderVertexFormat( VERTEX_FORMAT_USE_EXACT_FORMAT, 2, pTexCoord, 0 );
		}
		DYNAMIC_STATE
		{
 			DECLARE_DYNAMIC_VERTEX_SHADER( morphweight_vs30 );
 			SET_DYNAMIC_VERTEX_SHADER( morphweight_vs30 );
 
 			DECLARE_DYNAMIC_PIXEL_SHADER( morphweight_ps30 );
 			SET_DYNAMIC_PIXEL_SHADER( morphweight_ps30 );
		}
		Draw();
	}
END_SHADER

