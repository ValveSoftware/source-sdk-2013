//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "shaderlib/cshader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

SOFTWARE_VERTEX_SHADER( myHappyLittleSoftwareVertexShader )
{
	float t = pShaderAPI->CurrentTime();
	for( int i = 0; i < meshBuilder.NumVertices(); i++ )
	{
		const float *pPos = meshBuilder.Position();
		meshBuilder.Position3f( pPos[0], pPos[1], pPos[2] + 10.0f * sin( t + pPos[0] ) );
		meshBuilder.AdvanceVertex();
	}
}

FORWARD_DECLARE_SOFTWARE_VERTEX_SHADER( myHappyLittleSoftwareVertexShader );

BEGIN_SHADER_FLAGS( DebugSoftwareVertexShader, "blah", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM_OVERRIDE( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/basetexture", "unused", SHADER_PARAM_NOT_EDITABLE )
		SHADER_PARAM_OVERRIDE( FRAME, SHADER_PARAM_TYPE_INTEGER, "0", "unused", SHADER_PARAM_NOT_EDITABLE )
		SHADER_PARAM_OVERRIDE( BASETEXTURETRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "unused", SHADER_PARAM_NOT_EDITABLE )
		SHADER_PARAM_OVERRIDE( COLOR, SHADER_PARAM_TYPE_COLOR, "{255 255 255}", "unused", SHADER_PARAM_NOT_EDITABLE )
		SHADER_PARAM_OVERRIDE( ALPHA, SHADER_PARAM_TYPE_FLOAT, "1.0", "unused", SHADER_PARAM_NOT_EDITABLE )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		if( g_pHardwareConfig->SupportsVertexAndPixelShaders() )
		{
			USE_SOFTWARE_VERTEX_SHADER( myHappyLittleSoftwareVertexShader );
		}
		else
		{
			USE_SOFTWARE_VERTEX_SHADER( myHappyLittleSoftwareVertexShader );
		}
	}
	SHADER_DRAW
	{
		SHADOW_STATE
		{
		}
		DYNAMIC_STATE
		{
		}
		Draw();
	}
END_SHADER
