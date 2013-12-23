//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//


#include "BaseVSShader.h"
#include "mathlib/vmatrix.h"

#include "water_vs11.inc"
#include "watercheappervertexfresnel_vs11.inc"
#include "watercheap_vs11.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER( Water_DX80, 
			  "Help for Water_DX80" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( REFRACTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( REFLECTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_WaterReflection", "" )
		SHADER_PARAM( REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0", "" )
		SHADER_PARAM( REFRACTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "refraction tint" )
		SHADER_PARAM( REFLECTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0", "" )
		SHADER_PARAM( REFLECTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "reflection tint" )
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "", "dudv bump map" )
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
		if( !params[REFLECTBLENDFACTOR]->IsDefined() )
		{
			params[REFLECTBLENDFACTOR]->SetFloatValue( 1.0f );
		}
		if( params[FORCEEXPENSIVE]->GetIntValue() && params[FORCECHEAP]->GetIntValue() )
		{
			params[FORCEEXPENSIVE]->SetIntValue( 0 );
		}
	}

	SHADER_FALLBACK
	{
		if ( IsPC() && ( g_pHardwareConfig->GetDXSupportLevel() < 80 ||	!g_pHardwareConfig->HasProjectedBumpEnv() ) )
		{
			return "Water_DX60";
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
		if (params[BUMPMAP]->IsDefined() )
		{
			LoadTexture( BUMPMAP );
		}
		if (params[ENVMAP]->IsDefined() )
		{
			LoadCubeMap( ENVMAP );
		}
		if (params[NORMALMAP]->IsDefined() )
		{
			LoadBumpMap( NORMALMAP );
		}
	}

	inline void SetCheapWaterFactors( IMaterialVar **params, IShaderDynamicAPI* pShaderAPI, int nConstantReg )
	{
		float flCheapWaterStartDistance = params[CHEAPWATERSTARTDISTANCE]->GetFloatValue();
		float flCheapWaterEndDistance = params[CHEAPWATERENDDISTANCE]->GetFloatValue();
		float flCheapWaterConstants[4] = 
		{ 
			flCheapWaterStartDistance, 
			1.0f / ( flCheapWaterEndDistance - flCheapWaterStartDistance ), 
			0.0f, 
			0.0f 
		};
		pShaderAPI->SetVertexShaderConstant( nConstantReg, flCheapWaterConstants );
	}

	inline void DrawReflection( IMaterialVar **params, IShaderShadow* pShaderShadow, 
		                        IShaderDynamicAPI* pShaderAPI, bool bBlendReflection )
	{
		SHADOW_STATE
		{
			SetInitialShadowState( );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );

			int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );
			if( bBlendReflection )
			{
				EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}

			water_vs11_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "Water_vs11", vshIndex.GetIndex() );

			pShaderShadow->SetPixelShader( "WaterReflect_ps11", 0 );

			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();

			// The dx9.0c runtime says that we shouldn't have a non-zero dimension when using vertex and pixel shaders.
			pShaderAPI->SetTextureTransformDimension( SHADER_TEXTURE_STAGE1, 0, true );

			float fReflectionAmount = params[REFLECTAMOUNT]->GetFloatValue();
			pShaderAPI->SetBumpEnvMatrix( SHADER_TEXTURE_STAGE1, fReflectionAmount, 0.0f, 0.0f, fReflectionAmount );

			BindTexture( SHADER_SAMPLER0, BUMPMAP, BUMPFRAME );
			BindTexture( SHADER_SAMPLER1, REFLECTTEXTURE, -1 );
			BindTexture( SHADER_SAMPLER2, NORMALMAP, BUMPFRAME );
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER3, TEXTURE_NORMALIZATION_CUBEMAP );
			pShaderAPI->SetVertexShaderIndex( 0 );

			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, BUMPTRANSFORM );

			// used to invert y
			float c[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, c, 1 );

			SetPixelShaderConstant( 0, REFLECTTINT );

			water_vs11_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();
	}
	
	inline void DrawRefraction( IMaterialVar **params, IShaderShadow* pShaderShadow, 
		                        IShaderDynamicAPI* pShaderAPI )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

			water_vs11_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "Water_vs11", vshIndex.GetIndex() );

			pShaderShadow->SetPixelShader( "WaterRefract_ps11", 0 );
			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			// The dx9.0c runtime says that we shouldn't have a non-zero dimension when using vertex and pixel shaders.
			pShaderAPI->SetTextureTransformDimension( SHADER_TEXTURE_STAGE1, 0, true );
			float fRefractionAmount = params[REFRACTAMOUNT]->GetFloatValue();
			pShaderAPI->SetBumpEnvMatrix( SHADER_TEXTURE_STAGE1, fRefractionAmount, 0.0f, 0.0f, fRefractionAmount );
			BindTexture( SHADER_SAMPLER0, BUMPMAP, BUMPFRAME );
			BindTexture( SHADER_SAMPLER1, REFRACTTEXTURE );

			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, BUMPTRANSFORM );

			// used to invert y
			float c[4] = { 0.0f, 0.0f, 0.0f, -1.0f };
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, c, 1 );

			SetPixelShaderConstant( 0, REFRACTTINT );

			water_vs11_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();
	}

	inline void DrawRefractionForFresnel( IMaterialVar **params, IShaderShadow* pShaderShadow, 
		                        IShaderDynamicAPI* pShaderAPI )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableAlphaWrites( true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );

			int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

			water_vs11_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "Water_vs11", vshIndex.GetIndex() );

			pShaderShadow->SetPixelShader( "WaterRefractFresnel_ps11", 0 );
			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			// The dx9.0c runtime says that we shouldn't have a non-zero dimension when using vertex and pixel shaders.
			pShaderAPI->SetTextureTransformDimension( SHADER_TEXTURE_STAGE1, 0, true );
			float fRefractionAmount = params[REFRACTAMOUNT]->GetFloatValue();
			pShaderAPI->SetBumpEnvMatrix( SHADER_TEXTURE_STAGE1, fRefractionAmount, 0.0f, 0.0f, fRefractionAmount );
			BindTexture( SHADER_SAMPLER0, BUMPMAP, BUMPFRAME );
			BindTexture( SHADER_SAMPLER1, REFRACTTEXTURE );
			BindTexture( SHADER_SAMPLER2, NORMALMAP, BUMPFRAME );
			s_pShaderAPI->BindStandardTexture( SHADER_SAMPLER3, TEXTURE_NORMALIZATION_CUBEMAP );

			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, BUMPTRANSFORM );
			SetCheapWaterFactors( params, pShaderAPI, VERTEX_SHADER_SHADER_SPECIFIC_CONST_3 );

			// used to invert y
			float c[4] = { 0.0f, 0.0f, 0.0f, -1.0f };
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, c, 1 );

			SetPixelShaderConstant( 0, REFRACTTINT );

			water_vs11_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();
	}

	inline void DrawCheapWater( IMaterialVar **params, IShaderShadow* pShaderShadow, 
		                        IShaderDynamicAPI* pShaderAPI, bool bBlend, bool bBlendFresnel, bool bNoPerVertexFresnel )
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
			if( bBlend )
			{
				if ( bBlendFresnel )
				{
					EnableAlphaBlending( SHADER_BLEND_DST_ALPHA, SHADER_BLEND_ONE_MINUS_DST_ALPHA );
				}
				else
				{
					EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				}
			}

			pShaderShadow->VertexShaderVertexFormat( 
				VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S |
				VERTEX_TANGENT_T, 1, 0, 0 );

			if( bNoPerVertexFresnel )
			{
				watercheap_vs11_Static_Index vshIndex;
				pShaderShadow->SetVertexShader( "WaterCheap_vs11", vshIndex.GetIndex() );
			}
			else
			{
				watercheappervertexfresnel_vs11_Static_Index vshIndex;
				pShaderShadow->SetVertexShader( "WaterCheapPerVertexFresnel_vs11", vshIndex.GetIndex() );
			}

			static const char *s_pPixelShaderName[] = 
			{
				"WaterCheapOpaque_ps11",
				"WaterCheap_ps11",
				"WaterCheapNoFresnelOpaque_ps11",
				"WaterCheapNoFresnel_ps11",
			};

			int nPshIndex = 0;
			if ( bBlend ) nPshIndex |= 0x1;
			if ( bNoPerVertexFresnel ) nPshIndex |= 0x2;
			pShaderShadow->SetPixelShader( s_pPixelShaderName[nPshIndex] );

			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();
			BindTexture( SHADER_SAMPLER0, NORMALMAP, BUMPFRAME );
			BindTexture( SHADER_SAMPLER3, ENVMAP, ENVMAPFRAME );

			if( bBlend && !bBlendFresnel )
			{
				SetCheapWaterFactors( params, pShaderAPI, VERTEX_SHADER_SHADER_SPECIFIC_CONST_2 );
			}
			else
			{
				float flCheapWaterConstants[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
				pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, flCheapWaterConstants );
			}

			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BUMPTRANSFORM );

			SetPixelShaderConstant( 0, FOGCOLOR );
			SetPixelShaderConstant( 1, REFLECTTINT, REFLECTBLENDFACTOR );

			if( bNoPerVertexFresnel )
			{
				watercheap_vs11_Dynamic_Index vshIndex;
				vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
				pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
			}
			else
			{
				watercheappervertexfresnel_vs11_Dynamic_Index vshIndex;
				vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
				pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
			}
		}
		Draw();
	}

	SHADER_DRAW
	{
		// NOTE: Here's what all this means.
		// 1) ForceCheap means use env_cubemap only
		// 2) ForceExpensive means do real reflection instead of env_cubemap.
		// By default, it will do refraction and use env_cubemap for the reflection.
		// If dest alpha is available, it will also use dest alpha for a fresnel term.
		// otherwise there will be no fresnel term as it looks bizzare.

		bool bBlendReflection = false;
		bool bForceCheap = params[FORCECHEAP]->GetIntValue() != 0 || UsingEditor( params );
		bool bForceExpensive = !bForceCheap && (params[FORCEEXPENSIVE]->GetIntValue() != 0);
		bool bRefraction = params[REFRACTTEXTURE]->IsTexture();
		bool bReflection = bForceExpensive && params[REFLECTTEXTURE]->IsTexture();
		bool bReflectionUseFresnel = false;

		// Can't do fresnel when forcing cheap or if there's no refraction
		if( !bForceCheap )
		{
			if( bRefraction )
			{
				// NOTE: Expensive reflection does the fresnel correctly per-pixel
				if ( g_pHardwareConfig->HasDestAlphaBuffer() && !bReflection && !params[NOFRESNEL]->GetIntValue() )
				{
					DrawRefractionForFresnel( params, pShaderShadow, pShaderAPI );
					bReflectionUseFresnel = true;
				}
				else
				{
					DrawRefraction( params, pShaderShadow, pShaderAPI );
				}
				bBlendReflection = true;
			}
			if( bReflection )
			{
				DrawReflection( params, pShaderShadow, pShaderAPI, bBlendReflection );
			}
		}

		// Use $decal to see if we are a decal or not. . if we are, then don't bother
		// drawing the cheap version for now since we don't have access to env_cubemap
		if( !bReflection && params[ENVMAP]->IsTexture() && !IS_FLAG_SET( MATERIAL_VAR_DECAL ) )
		{
			bool bNoPerVertexFresnel = ( params[NOFRESNEL]->GetIntValue() || bReflectionUseFresnel || bForceCheap || !bRefraction );
			DrawCheapWater( params, pShaderShadow, pShaderAPI, bBlendReflection, bReflectionUseFresnel, bNoPerVertexFresnel );
		}
	}
END_SHADER

