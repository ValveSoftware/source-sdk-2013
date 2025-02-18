//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "BaseVSShader.h"
#include "lightmappedgeneric_vs11.inc"
#include "lightmappedgeneric_decal.inc"
#include "mathlib/bumpvects.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( DecalBaseTimesLightmapAlphaBlendSelfIllum, DecalBaseTimesLightmapAlphaBlendSelfIllum_DX8 )

BEGIN_VS_SHADER( DecalBaseTimesLightmapAlphaBlendSelfIllum_DX8, "" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM_OVERRIDE( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "decals/decalporthole001b", "decal base texture", 0 )
		SHADER_PARAM( SELFILLUMTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "decals/decalporthole001b_mask", "self-illum texture" )
		SHADER_PARAM( SELFILLUMTEXTUREFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "self-illum texture frame" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		// FLASHLIGHTFIXME
		params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight001" );
		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		SET_FLAGS( MATERIAL_VAR_TRANSLUCENT );
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
	}

	SHADER_FALLBACK
	{
		if ( IsPC() && ( g_pHardwareConfig->GetDXSupportLevel() < 80 ) )
		{
			return "DecalBaseTimesLightmapAlphaBlendSelfIllum_DX6";
		}
		return 0;
	}

	SHADER_INIT
	{
		LoadTexture( FLASHLIGHTTEXTURE );
		LoadTexture( BASETEXTURE );
		LoadTexture( SELFILLUMTEXTURE );
	}

	void DrawDecal( IMaterialVar **params, IShaderDynamicAPI *pShaderAPI, IShaderShadow *pShaderShadow )
	{
		if( IsSnapshotting() )
		{
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnablePolyOffset( SHADER_POLYOFFSET_DECAL );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );

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

		if( IsSnapshotting() )
		{
			SetInitialShadowState( );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnablePolyOffset( SHADER_POLYOFFSET_DECAL );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, 0, 0 );
			lightmappedgeneric_vs11_Static_Index vshIndex;
			vshIndex.SetDETAIL( false );
			vshIndex.SetENVMAP( false );
			vshIndex.SetENVMAPCAMERASPACE( false );
			vshIndex.SetENVMAPSPHERE( false );
			vshIndex.SetVERTEXCOLOR( false );
			pShaderShadow->SetVertexShader( "LightmappedGeneric_vs11", vshIndex.GetIndex() );
			pShaderShadow->SetPixelShader( "DecalBaseTimesLightmapAlphaBlendSelfIllum2_ps11" );

			FogToFogColor();
		}
		else
		{
			BindTexture( SHADER_SAMPLER0, SELFILLUMTEXTURE, SELFILLUMTEXTUREFRAME );

			lightmappedgeneric_vs11_Dynamic_Index vshIndex;
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
