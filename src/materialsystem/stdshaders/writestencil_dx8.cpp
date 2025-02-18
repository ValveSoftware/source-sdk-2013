//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

#include "writez.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( WriteStencil, WriteStencil_DX8 )

BEGIN_VS_SHADER_FLAGS( WriteStencil_DX8, "Help for WriteStencil", SHADER_NOT_EDITABLE )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_FALLBACK
	{
		if ( IsPC() && g_pHardwareConfig->GetDXSupportLevel() < 80 )
			return "WriteStencil_DX6";

		return 0;
	}

	SHADER_INIT
	{
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableColorWrites( false );	//
			pShaderShadow->EnableAlphaWrites( false );	//	Write ONLY to stencil
			pShaderShadow->EnableDepthWrites( false );	//

			writez_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "writez", vshIndex.GetIndex() );

			pShaderShadow->SetPixelShader( "white" );
			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, 0, 0 );
		}
		DYNAMIC_STATE
		{
			writez_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();
	}
END_SHADER

