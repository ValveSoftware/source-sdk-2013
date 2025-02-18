//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "mathlib/vmatrix.h"

#include "water_ps14.inc"
#include "watercheap_vs14.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( Water, Water_DX81 )

BEGIN_VS_SHADER( Water_DX81, 
			  "Help for Water_DX81" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( REFRACTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( REFLECTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_WaterReflection", "" )
		SHADER_PARAM( REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0", "" )
		SHADER_PARAM( REFRACTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "refraction tint" )
		SHADER_PARAM( REFLECTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0", "" )
		SHADER_PARAM( REFLECTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "reflection tint" )
		SHADER_PARAM( NORMALMAP, SHADER_PARAM_TYPE_TEXTURE, "", "normal map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
		SHADER_PARAM( SCALE, SHADER_PARAM_TYPE_VEC2, "[1 1]", "" )
		SHADER_PARAM( TIME, SHADER_PARAM_TYPE_FLOAT, "", "" )
		SHADER_PARAM( WATERDEPTH, SHADER_PARAM_TYPE_FLOAT, "", "" )
		SHADER_PARAM( CHEAPWATERSTARTDISTANCE, SHADER_PARAM_TYPE_FLOAT, "", "This is the distance from the eye in inches that the shader should start transitioning to a cheaper water shader." )
		SHADER_PARAM( CHEAPWATERENDDISTANCE, SHADER_PARAM_TYPE_FLOAT, "", "This is the distance from the eye in inches that the shader should finish transitioning to a cheaper water shader." )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "env_cubemap", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( FOGCOLOR, SHADER_PARAM_TYPE_COLOR, "", "" )
		SHADER_PARAM( FORCECHEAP, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( FORCEEXPENSIVE, SHADER_PARAM_TYPE_BOOL, "", "" )
		SHADER_PARAM( REFLECTENTITIES, SHADER_PARAM_TYPE_BOOL, "", "" )
		SHADER_PARAM( REFLECTBLENDFACTOR, SHADER_PARAM_TYPE_FLOAT, "1.0", "" )
		SHADER_PARAM( NOFRESNEL, SHADER_PARAM_TYPE_BOOL, "0", "" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
		if( !params[CHEAPWATERSTARTDISTANCE]->IsDefined() )
		{
			params[CHEAPWATERSTARTDISTANCE]->SetFloatValue( 500.0f );
		}
		if( !params[CHEAPWATERENDDISTANCE]->IsDefined() )
		{
			params[CHEAPWATERENDDISTANCE]->SetFloatValue( 1000.0f );
		}
		if( !params[SCALE]->IsDefined() )
		{
			params[SCALE]->SetVecValue( 1.0f, 1.0f );
		}
		if( !params[FOGCOLOR]->IsDefined() )
		{
			params[FOGCOLOR]->SetVecValue( 1.0f, 0.0f, 0.0f );
			Warning( "material %s needs to have a $fogcolor.\n", pMaterialName );
		}
		if( !params[REFLECTENTITIES]->IsDefined() )
		{
			params[REFLECTENTITIES]->SetIntValue( 0 );
		}
		if( !params[FORCEEXPENSIVE]->IsDefined() )
		{
			params[FORCEEXPENSIVE]->SetIntValue( 0 );
		}
		if( params[FORCEEXPENSIVE]->GetIntValue() && params[FORCECHEAP]->GetIntValue() )
		{
			params[FORCEEXPENSIVE]->SetIntValue( 0 );
		}
		if( !params[REFLECTBLENDFACTOR]->IsDefined() )
		{
			params[REFLECTBLENDFACTOR]->SetFloatValue( 1.0f );
		}
		if( !params[FORCEEXPENSIVE]->GetIntValue() && !params[ENVMAP]->IsDefined() )
		{
			params[ENVMAP]->SetStringValue( "engine/defaultcubemap" );
		}
	}

	SHADER_FALLBACK
	{
		if( g_pHardwareConfig->GetDXSupportLevel() < 81 )
		{
			return "Water_DX80";
		}
		return 0;
	}

	SHADER_INIT
	{
		Assert( params[WATERDEPTH]->IsDefined() );
		if( params[REFRACTTEXTURE]->IsDefined() )
		{
			LoadTexture( REFRACTTEXTURE );
		}
		if( params[REFLECTTEXTURE]->IsDefined() )
		{
			LoadTexture( REFLECTTEXTURE );
		}
		if (params[ENVMAP]->IsDefined() )
		{
			LoadTexture( ENVMAP );
		}
		if (params[NORMALMAP]->IsDefined() )
		{
			LoadBumpMap( NORMALMAP );
		}
	}

	inline int GetReflectionRefractionPixelShaderIndex( bool bReflection, bool bRefraction )
	{
		// "REFLECT" "0..1"
		// "REFRACT" "0..1"
		int pshIndex = ( bReflection ? 1 : 0 ) | ( bRefraction ? 2 : 0 );				
		return pshIndex;
	}

	inline void DrawReflectionRefraction( IMaterialVar **params, IShaderShadow* pShaderShadow,
		IShaderDynamicAPI* pShaderAPI, bool bReflection, bool bRefraction ) 
	{
		SHADOW_STATE
		{
			SetInitialShadowState( );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			if( bRefraction )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
			}
			pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
			if( bReflection )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );
			}
			int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

			water_ps14_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "Water_ps14", vshIndex.GetIndex() );

			int pshIndex = GetReflectionRefractionPixelShaderIndex( bReflection, bRefraction );
			pShaderShadow->SetPixelShader ( "Water_ps14", pshIndex );
			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_NORMALIZATION_CUBEMAP );
			if( bRefraction )
			{
				BindTexture( SHADER_SAMPLER2, REFRACTTEXTURE, -1 );
			}
			BindTexture( SHADER_SAMPLER3, NORMALMAP, BUMPFRAME );
			if( bReflection )
			{
				BindTexture( SHADER_SAMPLER4, REFLECTTEXTURE, -1 );
			}

			SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, REFLECTAMOUNT );
			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, BUMPTRANSFORM );
			SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_3, REFRACTAMOUNT );
			
			float c0[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			pShaderAPI->SetPixelShaderConstant( 0, c0, 1 );
			
			SetPixelShaderConstant( 1, REFRACTTINT );
			SetPixelShaderConstant( 4, REFLECTTINT );
			
			float c2[4] = { 0.5f, 0.5f, 0.5f, 0.5f };
			pShaderAPI->SetPixelShaderConstant( 2, c2, 1 );
			
			// ERASE ME!
			float c3[4] = { 5.0f, 0.0f, 0.0f, 0.0f };
			pShaderAPI->SetPixelShaderConstant( 3, c3, 1 );

			// reflection/refraction scale
			float reflectionRefractionScale[4] = { params[REFLECTAMOUNT]->GetFloatValue(), 
				params[REFRACTAMOUNT]->GetFloatValue(), 0.0f, 0.0f };
			pShaderAPI->SetPixelShaderConstant( 5, reflectionRefractionScale, 1 );
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, reflectionRefractionScale, 1 );

			water_ps14_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();
	}
	
	enum DrawCheapType_t
	{
		DRAW_CHEAP_OPAQUE = 0,
		DRAW_CHEAP_FRESNEL_OPAQUE,
		DRAW_CHEAP_LOD_ONLY,
		DRAW_CHEAP_FRESNEL_AND_LOD,
	};

	inline void DrawCheapWater( IMaterialVar **params, IShaderShadow* pShaderShadow, 
		                        IShaderDynamicAPI* pShaderAPI, DrawCheapType_t type )
	{
		SHADOW_STATE
		{
			SetInitialShadowState( );

			// In edit mode, use nocull
			if ( UsingEditor( params ) )
			{
				s_pShaderShadow->EnableCulling( false );
			}

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );
			if ( (type != DRAW_CHEAP_OPAQUE) && (type != DRAW_CHEAP_FRESNEL_OPAQUE) )
			{
				EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}
			pShaderShadow->VertexShaderVertexFormat( 
				VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S |
				VERTEX_TANGENT_T, 1, 0, 0 );

			watercheap_vs14_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "WaterCheap_vs14", vshIndex.GetIndex() );

			static const char *s_pPixelShader[] =
			{
				"WaterCheapOpaque_ps14",
				"WaterCheapFresnelOpaque_ps14",
				"WaterCheap_ps14",
				"WaterCheapFresnel_ps14",
			};

			pShaderShadow->SetPixelShader( s_pPixelShader[type] );
			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();
			BindTexture( SHADER_SAMPLER0, NORMALMAP, BUMPFRAME );
			BindTexture( SHADER_SAMPLER3, ENVMAP, ENVMAPFRAME );
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER4, TEXTURE_NORMALIZATION_CUBEMAP );
			
			float pCheapWaterConstants[4] = { 0, 0, 0, 0 };
			if ( (type != DRAW_CHEAP_OPAQUE) && (type != DRAW_CHEAP_FRESNEL_OPAQUE) )
			{
				float flCheapWaterStartDistance = params[CHEAPWATERSTARTDISTANCE]->GetFloatValue();
				float flCheapWaterEndDistance = params[CHEAPWATERENDDISTANCE]->GetFloatValue();
				pCheapWaterConstants[0] = flCheapWaterStartDistance;
				pCheapWaterConstants[1] = 1.0f / ( flCheapWaterEndDistance - flCheapWaterStartDistance );
			}
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, pCheapWaterConstants );

			SetPixelShaderConstant( 0, FOGCOLOR );
			SetPixelShaderConstant( 1, REFLECTTINT, REFLECTBLENDFACTOR );
				
			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BUMPTRANSFORM );

			watercheap_vs14_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();
	}

	SHADER_DRAW
	{
		// NOTE: Here's what all this means.
		// 1) ForceCheap means use env_cubemap only
		// 2) ForceExpensive means do real reflection instead of env_cubemap.
		// By default, it will do refraction and use env_cubemap for the reflection.

		// Also, it will fade to cheap water at a particular distance, 
		// based on CheapWaterStartDistance and CheapWaterEndDistance
		// * In the ForceCheap case, no fading is required
		// * In the default case, it will fade based on these parameters in a single pass
		// * In the expensive case, it will have to perform the fade in a separate pass.

		bool bForceCheap = params[FORCECHEAP]->GetIntValue() != 0 || UsingEditor( params );
		bool bForceExpensive = params[FORCEEXPENSIVE]->GetIntValue() != 0;
		bool bRefraction = params[REFRACTTEXTURE]->IsTexture();
		bool bReflection = bForceExpensive && params[REFLECTTEXTURE]->IsTexture();
		DrawCheapType_t type = params[NOFRESNEL]->GetIntValue() ? DRAW_CHEAP_OPAQUE : DRAW_CHEAP_FRESNEL_OPAQUE;
		if( !bForceCheap && (bRefraction || bReflection) )
		{
			DrawReflectionRefraction( params, pShaderShadow, pShaderAPI, bReflection, bRefraction );
			if ( !bReflection )
			{
				type = params[NOFRESNEL]->GetIntValue() ? DRAW_CHEAP_LOD_ONLY : DRAW_CHEAP_FRESNEL_AND_LOD;
			}
			else
			{
				type = DRAW_CHEAP_LOD_ONLY;
			}
		}

		// Use $decal to see if we are a decal or not. . if we are, then don't bother
		// drawing the cheap version for now since we don't have access to env_cubemap
		if( params[ENVMAP]->IsTexture() && !IS_FLAG_SET( MATERIAL_VAR_DECAL ) && !bReflection )
		{
			DrawCheapWater( params, pShaderShadow, pShaderAPI, type );
		}
	}
END_SHADER

