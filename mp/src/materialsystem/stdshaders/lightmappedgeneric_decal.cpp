//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Lightmap only shader
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

#include "lightmappedgeneric_decal.inc"
#include "mathlib/bumpvects.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


BEGIN_VS_SHADER( LightmappedGeneric_Decal,
			  "Help for LightmappedGeneric_Decal" )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	// Set up anything that is necessary to make decisions in SHADER_FALLBACK.
	SHADER_INIT_PARAMS()
	{
		if ( g_pHardwareConfig->SupportsBorderColor() )
		{
			params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight_border" );
		}
		else
		{
			params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight001" );
		}

		// No texture means no self-illum or env mask in base alpha
		if ( !params[BASETEXTURE]->IsDefined() )
		{
			CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
			CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		}

		SET_FLAGS( MATERIAL_VAR_DECAL );
		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT
	{
		LoadTexture( FLASHLIGHTTEXTURE, TEXTUREFLAGS_SRGB );
		
		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture( BASETEXTURE );

			if ( !params[BASETEXTURE]->GetTextureValue()->IsTranslucent() )
			{
				CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
				CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
			}
		}

		// Don't alpha test if the alpha channel is used for other purposes
		if (IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
		{
			CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );
		}
	}

	void DrawDecal( IMaterialVar **params, IShaderDynamicAPI *pShaderAPI, IShaderShadow *pShaderShadow )
	{
		if( IsSnapshotting() )
		{
			// Be sure not to write to dest alpha
			pShaderShadow->EnableAlphaWrites( false );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );

			SetNormalBlendingShadowState( BASETEXTURE, true ); 

			int pTexCoords[3] = { 2, 2, 1 };
			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION | VERTEX_COLOR, 3, pTexCoords, 0 );

			lightmappedgeneric_decal_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "LightmappedGeneric_Decal", vshIndex.GetIndex() );
			pShaderShadow->SetPixelShader( "LightmappedGeneric_Decal" );
			FogToFogColor();
		}
		else
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );

			// Load the z^2 components of the lightmap coordinate axes only
			// This is (N dot basis)^2
			Vector vecZValues( g_localBumpBasis[0].z, g_localBumpBasis[1].z, g_localBumpBasis[2].z );
			vecZValues *= vecZValues;

			Vector4D basis[3];
			basis[0].Init( vecZValues.x, vecZValues.x, vecZValues.x, 0.0f );
			basis[1].Init( vecZValues.y, vecZValues.y, vecZValues.y, 0.0f );
			basis[2].Init( vecZValues.z, vecZValues.z, vecZValues.z, 0.0f );
			pShaderAPI->SetPixelShaderConstant( 0, (float*)basis, 3 );

			pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_LIGHTMAP_BUMPED );
			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM );
			SetModulationPixelShaderDynamicState( 3 );

			lightmappedgeneric_decal_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();
	}

	SHADER_DRAW
	{
		if( UsingFlashlight( params ) )
		{
			DrawFlashlight_dx80( params, pShaderAPI, pShaderShadow, false, -1, -1, -1, 
				FLASHLIGHTTEXTURE, FLASHLIGHTTEXTUREFRAME, true, false, 0, -1, -1 );
		}
		else
		{
			DrawDecal( params, pShaderAPI, pShaderShadow );
		}
	}
END_SHADER
