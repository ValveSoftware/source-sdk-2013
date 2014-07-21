//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is an example of a material that modifies vertex data
// in the shader. NOTE: Every pass is given a clean set of vertex data.
// Modifications made in the first pass are *not* carried over to the next pass
// Modifications must take place during the DYNAMIC_STATE block.
// Use the function MeshBuilder() to build the mesh 
//
// Also note: Using thie feature is *really expensive*! It makes a copy of
// the vertex data *per pass!* If you wish to modify vertex data to be used
// with all passes, your best bet is to construct a dynamic mesh instead.
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "shaderlib/cshader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_SHADER_FLAGS( DebugModifyVertex, "Help for DebugModifyVertex", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( WAVE, SHADER_PARAM_TYPE_FLOAT, "1.0", "wave amplitude" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		if( g_pHardwareConfig->GetSamplerCount() >= 2 )
		{
			// lightmap
			SHADOW_STATE
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				pShaderShadow->EnableVertexDataPreprocess( true );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 );
				FogToFogColor();
			}

			DYNAMIC_STATE
			{
				BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );

				float amp = params[WAVE]->GetFloatValue();
				float currTime = pShaderAPI->CurrentTime();
				for (int i = 0; i < MeshBuilder()->NumVertices(); ++i)
				{
					float const* pPos = MeshBuilder()->Position();
					MeshBuilder()->Position3f( pPos[0] + amp * sin( currTime + pPos[2] / 4 ),
												pPos[1] + amp * sin( currTime + pPos[2] / 4 + 2 * 3.14 / 3 ),
												pPos[2] + amp * sin( currTime + pPos[2] / 4 + 4 * 3.14 / 3 ) );
					MeshBuilder()->AdvanceVertex();
				}
			}
			Draw();

			// base * vertex color
			SHADOW_STATE
			{
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_COLOR | 
					SHADER_DRAW_TEXCOORD0 );
				FogToFogColor();
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );

				// Notice here that since we didn't modify the position, and this is a second
				// pass, the position has been reset to it's initial, unmodified position
				float currTime = pShaderAPI->CurrentTime();
				for (int i = 0; i < MeshBuilder()->NumVertices(); ++i)
				{
					float const* pPos = MeshBuilder()->Position();
					MeshBuilder()->Color3f( ( sin( currTime + pPos[0] ) + 1.0F) * 0.5,
											( sin( currTime + pPos[1] ) + 1.0F) * 0.5,
											( sin( currTime + pPos[2] ) + 1.0F) * 0.5 );
					MeshBuilder()->AdvanceVertex();
				}
			}
			Draw();
		}
		else
		{
			ShaderWarning( "DebugModifyVertex: not "
				"implemented for single-texturing hardware\n" );
		}
	}
END_SHADER
