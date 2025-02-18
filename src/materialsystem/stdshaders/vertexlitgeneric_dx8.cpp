//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

#include "vertexlitgeneric_vs11.inc"
#include "vertexlitgeneric_selfillumonly.inc"
#include "emissive_scroll_blended_pass_helper.h"
#include "flesh_interior_blended_pass_helper.h"
#include "cloak_blended_pass_helper.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( VertexLitGeneric, VertexLitGeneric_DX8 )
DEFINE_FALLBACK_SHADER( Skin_DX9, VertexLitGeneric_DX8 )

BEGIN_VS_SHADER( VertexLitGeneric_DX8, 
				"Help for VertexLitGeneric" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( SELFILLUMTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture" )
		SHADER_PARAM( DETAILFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $detail" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "envmap frame number" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_envmask", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( ENVMAPMASKSCALE, SHADER_PARAM_TYPE_FLOAT, "1", "envmap mask scale" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "models/shadertest/shader1_normal", "bump map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
		SHADER_PARAM( ENVMAPCONTRAST, SHADER_PARAM_TYPE_FLOAT, "0.0", "contrast 0 == normal 1 == color*color" )
		SHADER_PARAM( ENVMAPSATURATION, SHADER_PARAM_TYPE_FLOAT, "1.0", "saturation 0 == greyscale 1 == normal" )
		SHADER_PARAM( ENVMAPOPTIONAL, SHADER_PARAM_TYPE_BOOL, "0", "Make the envmap only apply to dx9 and higher hardware" )
		SHADER_PARAM( FORCEBUMP, SHADER_PARAM_TYPE_BOOL, "0", "0 == Do bumpmapping if the card says it can handle it. 1 == Always do bumpmapping." )
		SHADER_PARAM( ALPHATESTREFERENCE, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )	

	    SHADER_PARAM( DETAILBLENDMODE, SHADER_PARAM_TYPE_INTEGER, "0", "mode for combining detail texture with base. 0=normal, 1= additive, 2=alpha blend detail over base, 3=crossfade" )
		SHADER_PARAM( DETAILBLENDFACTOR, SHADER_PARAM_TYPE_FLOAT, "1", "blend amount for detail texture." )

		// Emissive Scroll Pass
		SHADER_PARAM( EMISSIVEBLENDENABLED, SHADER_PARAM_TYPE_BOOL, "0", "Enable emissive blend pass" )
		SHADER_PARAM( EMISSIVEBLENDBASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "self-illumination map" )
		SHADER_PARAM( EMISSIVEBLENDSCROLLVECTOR, SHADER_PARAM_TYPE_VEC2, "[0.11 0.124]", "Emissive scroll vec" )
		SHADER_PARAM( EMISSIVEBLENDSTRENGTH, SHADER_PARAM_TYPE_FLOAT, "1.0", "Emissive blend strength" )
		SHADER_PARAM( EMISSIVEBLENDTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "self-illumination map" )
		SHADER_PARAM( EMISSIVEBLENDTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
		SHADER_PARAM( TIME, SHADER_PARAM_TYPE_FLOAT, "0.0", "Needs CurrentTime Proxy" )

		// Cloak Pass
		SHADER_PARAM( CLOAKPASSENABLED, SHADER_PARAM_TYPE_BOOL, "0", "Enables cloak render in a second pass" )
		SHADER_PARAM( CLOAKFACTOR, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )
		SHADER_PARAM( CLOAKCOLORTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Cloak color tint" )
		SHADER_PARAM( REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "2", "" )

		// Flesh Interior Pass
		SHADER_PARAM( FLESHINTERIORENABLED, SHADER_PARAM_TYPE_BOOL, "0", "Enable Flesh interior blend pass" )
		SHADER_PARAM( FLESHINTERIORTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Flesh color texture" )
		SHADER_PARAM( FLESHINTERIORNOISETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Flesh noise texture" )
		SHADER_PARAM( FLESHBORDERTEXTURE1D, SHADER_PARAM_TYPE_TEXTURE, "", "Flesh border 1D texture" )
		SHADER_PARAM( FLESHNORMALTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Flesh normal texture" )
		SHADER_PARAM( FLESHSUBSURFACETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Flesh subsurface texture" )
		SHADER_PARAM( FLESHCUBETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Flesh cubemap texture" )
		SHADER_PARAM( FLESHBORDERNOISESCALE, SHADER_PARAM_TYPE_FLOAT, "1.5", "Flesh Noise UV scalar for border" )
		SHADER_PARAM( FLESHDEBUGFORCEFLESHON, SHADER_PARAM_TYPE_BOOL, "0", "Flesh Debug full flesh" )
		SHADER_PARAM( FLESHEFFECTCENTERRADIUS1, SHADER_PARAM_TYPE_VEC4, "[0 0 0 0.001]", "Flesh effect center and radius" )
		SHADER_PARAM( FLESHEFFECTCENTERRADIUS2, SHADER_PARAM_TYPE_VEC4, "[0 0 0 0.001]", "Flesh effect center and radius" )
		SHADER_PARAM( FLESHEFFECTCENTERRADIUS3, SHADER_PARAM_TYPE_VEC4, "[0 0 0 0.001]", "Flesh effect center and radius" )
		SHADER_PARAM( FLESHEFFECTCENTERRADIUS4, SHADER_PARAM_TYPE_VEC4, "[0 0 0 0.001]", "Flesh effect center and radius" )
		SHADER_PARAM( FLESHSUBSURFACETINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Subsurface Color" )
		SHADER_PARAM( FLESHBORDERWIDTH, SHADER_PARAM_TYPE_FLOAT, "0.3", "Flesh border" )
		SHADER_PARAM( FLESHBORDERSOFTNESS, SHADER_PARAM_TYPE_FLOAT, "0.42", "Flesh border softness (> 0.0 && <= 0.5)" )
		SHADER_PARAM( FLESHBORDERTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Flesh border Color" )
		SHADER_PARAM( FLESHGLOBALOPACITY, SHADER_PARAM_TYPE_FLOAT, "1.0", "Flesh global opacity" )
		SHADER_PARAM( FLESHGLOSSBRIGHTNESS, SHADER_PARAM_TYPE_FLOAT, "0.66", "Flesh gloss brightness" )
		SHADER_PARAM( FLESHSCROLLSPEED, SHADER_PARAM_TYPE_FLOAT, "1.0", "Flesh scroll speed" )

		// Color Replacement Pass
		SHADER_PARAM( BLENDTINTBYBASEALPHA, SHADER_PARAM_TYPE_BOOL, "0", "Use the base alpha to blend in the $color modulation")
		SHADER_PARAM( BLENDTINTCOLOROVERBASE, SHADER_PARAM_TYPE_FLOAT, "0", "blend between tint acting as a multiplication versus a replace" )

	END_SHADER_PARAMS

	// Cloak Pass
	void SetupVarsCloakBlendedPass( CloakBlendedPassVars_t &info )
	{
		info.m_nCloakFactor = CLOAKFACTOR;
		info.m_nCloakColorTint = CLOAKCOLORTINT;
		info.m_nRefractAmount = REFRACTAMOUNT;

		// Delete these lines if not bump mapping!
		info.m_nBumpmap = BUMPMAP;
		info.m_nBumpFrame = BUMPFRAME;
		info.m_nBumpTransform = BUMPTRANSFORM;
	}

	bool NeedsPowerOfTwoFrameBufferTexture( IMaterialVar **params, bool bCheckSpecificToThisFrame ) const 
	{ 
		if ( params[CLOAKPASSENABLED]->GetIntValue() ) // If material supports cloaking
		{
			if ( bCheckSpecificToThisFrame == false ) // For setting model flag at load time
				return true;
			else if ( ( params[CLOAKFACTOR]->GetFloatValue() > 0.0f ) && ( params[CLOAKFACTOR]->GetFloatValue() < 1.0f ) ) // Per-frame check
				return true;
			// else, not cloaking this frame, so check flag2 in case the base material still needs it
		}

		// Check flag2 if not drawing cloak pass
		return IS_FLAG2_SET( MATERIAL_VAR2_NEEDS_POWER_OF_TWO_FRAME_BUFFER_TEXTURE ); 
	}

	bool IsTranslucent( IMaterialVar **params ) const
	{
		if ( params[CLOAKPASSENABLED]->GetIntValue() ) // If material supports cloaking
		{
			if ( ( params[CLOAKFACTOR]->GetFloatValue() > 0.0f ) && ( params[CLOAKFACTOR]->GetFloatValue() < 1.0f ) ) // Per-frame check
				return true;
			// else, not cloaking this frame, so check flag in case the base material still needs it
		}

		// Check flag if not drawing cloak pass
		return IS_FLAG_SET( MATERIAL_VAR_TRANSLUCENT ); 
	}

	// Emissive Scroll Pass
	void SetupVarsEmissiveScrollBlendedPass( EmissiveScrollBlendedPassVars_t &info )
	{
		info.m_nBlendStrength = EMISSIVEBLENDSTRENGTH;
		info.m_nBaseTexture = EMISSIVEBLENDBASETEXTURE;
		info.m_nFlowTexture = -1; // Not used in DX8
		info.m_nEmissiveTexture = EMISSIVEBLENDTEXTURE;
		info.m_nEmissiveTint = EMISSIVEBLENDTINT;
		info.m_nEmissiveScrollVector = EMISSIVEBLENDSCROLLVECTOR;
		info.m_nTime = TIME;
	}

	// Flesh Interior Pass
	void SetupVarsFleshInteriorBlendedPass( FleshInteriorBlendedPassVars_t &info )
	{
		info.m_nFleshTexture = FLESHINTERIORTEXTURE;
		info.m_nFleshNoiseTexture = FLESHINTERIORNOISETEXTURE;
		info.m_nFleshBorderTexture1D = FLESHBORDERTEXTURE1D;
		info.m_nFleshNormalTexture = FLESHNORMALTEXTURE;
		info.m_nFleshSubsurfaceTexture = FLESHSUBSURFACETEXTURE;
		info.m_nFleshCubeTexture = FLESHCUBETEXTURE;

		info.m_nflBorderNoiseScale = FLESHBORDERNOISESCALE;
		info.m_nflDebugForceFleshOn = FLESHDEBUGFORCEFLESHON;
		info.m_nvEffectCenterRadius1 = FLESHEFFECTCENTERRADIUS1;
		info.m_nvEffectCenterRadius2 = FLESHEFFECTCENTERRADIUS2;
		info.m_nvEffectCenterRadius3 = FLESHEFFECTCENTERRADIUS3;
		info.m_nvEffectCenterRadius4 = FLESHEFFECTCENTERRADIUS4;

		info.m_ncSubsurfaceTint = FLESHSUBSURFACETINT;
		info.m_nflBorderWidth = FLESHBORDERWIDTH;
		info.m_nflBorderSoftness = FLESHBORDERSOFTNESS;
		info.m_ncBorderTint = FLESHBORDERTINT;
		info.m_nflGlobalOpacity = FLESHGLOBALOPACITY;
		info.m_nflGlossBrightness = FLESHGLOSSBRIGHTNESS;
		info.m_nflScrollSpeed = FLESHSCROLLSPEED;

		info.m_nTime = TIME;
	}

	SHADER_INIT_PARAMS()
	{
		// FLASHLIGHTFIXME
		params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight001" );

		// We don't want no stinking bump mapping on models in dx8.
		// Wait a minute!  We want specular bump. .need to make that work by itself.
//		params[BUMPMAP]->SetUndefined();
//		if( IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK ) )
//		{
//			CLEAR_FLAGS( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
//			params[ENVMAP]->SetUndefined();
//		}
		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );

		if( !params[ENVMAPMASKSCALE]->IsDefined() )
			params[ENVMAPMASKSCALE]->SetFloatValue( 1.0f );

		if( !params[ENVMAPMASKFRAME]->IsDefined() )
			params[ENVMAPMASKFRAME]->SetIntValue( 0 );
		
		if( !params[ENVMAPTINT]->IsDefined() )
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );

		if( !params[SELFILLUMTINT]->IsDefined() )
			params[SELFILLUMTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );

		if( !params[DETAILSCALE]->IsDefined() )
			params[DETAILSCALE]->SetFloatValue( 4.0f );

		if( !params[DETAILBLENDFACTOR]->IsDefined() )
			params[DETAILBLENDFACTOR]->SetFloatValue( 1.0f );

		if( !params[DETAILBLENDMODE]->IsDefined() )
			params[DETAILBLENDMODE]->SetFloatValue( 0 );

		if( !params[ENVMAPCONTRAST]->IsDefined() )
			params[ENVMAPCONTRAST]->SetFloatValue( 0.0f );
		
		if( !params[ENVMAPSATURATION]->IsDefined() )
			params[ENVMAPSATURATION]->SetFloatValue( 1.0f );
		
		if( !params[ENVMAPFRAME]->IsDefined() )
			params[ENVMAPFRAME]->SetIntValue( 0 );

		if( !params[BUMPFRAME]->IsDefined() )
			params[BUMPFRAME]->SetIntValue( 0 );

		if( !params[ALPHATESTREFERENCE]->IsDefined() )
			params[ALPHATESTREFERENCE]->SetFloatValue( 0.0f );

		// No texture means no self-illum or env mask in base alpha
		if ( !params[BASETEXTURE]->IsDefined() )
		{
			CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
			CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		}

		// If in decal mode, no debug override...
		if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
		{
			SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		}

		if( g_pConfig->UseBumpmapping() && params[BUMPMAP]->IsDefined() )
		{
			SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
		}
		
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );

		// Get rid of the envmap if it's optional for this dx level.
		if( params[ENVMAPOPTIONAL]->IsDefined() && params[ENVMAPOPTIONAL]->GetIntValue() )
		{
			params[ENVMAP]->SetUndefined();
		}

		// If mat_specular 0, then get rid of envmap
		if( !g_pConfig->UseSpecular() && params[ENVMAP]->IsDefined() && params[BASETEXTURE]->IsDefined() )
		{
			params[ENVMAP]->SetUndefined();
		}

		// If a bumpmap is defined but an envmap isn't, then ignore the bumpmap.
		// It was meant to be used with diffuse
		if ( !params[ENVMAP]->IsDefined() )
		{
			params[BUMPMAP]->SetUndefined();
		}

		// Cloak Pass
		if ( !params[CLOAKPASSENABLED]->IsDefined() )
		{
			params[CLOAKPASSENABLED]->SetIntValue( 0 );
		}
		else if ( params[CLOAKPASSENABLED]->GetIntValue() )
		{
			CloakBlendedPassVars_t info;
			SetupVarsCloakBlendedPass( info );
			InitParamsCloakBlendedPass( this, params, pMaterialName, info );
		}

		// Emissive Scroll Pass
		if ( !params[EMISSIVEBLENDENABLED]->IsDefined() )
		{
			params[EMISSIVEBLENDENABLED]->SetIntValue( 0 );
		}
		else if ( params[EMISSIVEBLENDENABLED]->GetIntValue() )
		{
			EmissiveScrollBlendedPassVars_t info;
			SetupVarsEmissiveScrollBlendedPass( info );
			InitParamsEmissiveScrollBlendedPass( this, params, pMaterialName, info );
		}

		// Flesh Interior Pass
		if ( !params[FLESHINTERIORENABLED]->IsDefined() )
		{
			params[FLESHINTERIORENABLED]->SetIntValue( 0 );
		}
		else if ( params[FLESHINTERIORENABLED]->GetIntValue() )
		{
			FleshInteriorBlendedPassVars_t info;
			SetupVarsFleshInteriorBlendedPass( info );
			InitParamsFleshInteriorBlendedPass( this, params, pMaterialName, info );
		}

		// Color Replacement Pass
		if ( !params[BLENDTINTBYBASEALPHA]->IsDefined() )
		{
			params[BLENDTINTBYBASEALPHA]->SetIntValue(0);
		}

		if ( !params[BLENDTINTCOLOROVERBASE]->IsDefined() )
		{
			params[BLENDTINTCOLOROVERBASE]->SetFloatValue(0);
		}
	}

	SHADER_FALLBACK
	{	
		if ( IsPC() )
		{
			if ( g_pHardwareConfig->GetDXSupportLevel() < 70)
				return "VertexLitGeneric_DX6";

			if ( g_pHardwareConfig->GetDXSupportLevel() < 80)
				return "VertexLitGeneric_DX7";

			if ( g_pHardwareConfig->PreferReducedFillrate() )
				return "VertexLitGeneric_NoBump_DX8";
		}
		return 0;
	}

	SHADER_INIT
	{
		LoadTexture( FLASHLIGHTTEXTURE );

		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture( BASETEXTURE );

			if (!params[BASETEXTURE]->GetTextureValue()->IsTranslucent())
			{
				CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
				CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
			}
		}

		if (params[DETAIL]->IsDefined())
		{
			LoadTexture( DETAIL );
		}

		if (g_pConfig->UseBumpmapping() && params[BUMPMAP]->IsDefined())
		{
			LoadBumpMap( BUMPMAP );
		}

		// Don't alpha test if the alpha channel is used for other purposes
		if (IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
		{
			CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );
		}
			
		if (params[ENVMAP]->IsDefined())
		{
			if( !IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE) )
			{
				LoadCubeMap( ENVMAP );
			}
			else
			{
				LoadTexture( ENVMAP );
			}

			if( !g_pHardwareConfig->SupportsCubeMaps() )
			{
				SET_FLAGS( MATERIAL_VAR_ENVMAPSPHERE );
			}

			if (params[ENVMAPMASK]->IsDefined())
			{
				LoadTexture( ENVMAPMASK );
			}
		}

		// Cloak Pass
		if ( params[CLOAKPASSENABLED]->GetIntValue() )
		{
			CloakBlendedPassVars_t info;
			SetupVarsCloakBlendedPass( info );
			InitCloakBlendedPass( this, params, info );
		}

		// Emissive Scroll Pass
		if ( params[EMISSIVEBLENDENABLED]->GetIntValue() )
		{
			EmissiveScrollBlendedPassVars_t info;
			SetupVarsEmissiveScrollBlendedPass( info );
			InitEmissiveScrollBlendedPass( this, params, info );
		}

		// Flesh Interior Pass
		if ( params[FLESHINTERIORENABLED]->GetIntValue() )
		{
			FleshInteriorBlendedPassVars_t info;
			SetupVarsFleshInteriorBlendedPass( info );
			InitFleshInteriorBlendedPass( this, params, info );
		}
	}

	inline const char *GetUnbumpedPixelShaderName( IMaterialVar** params, bool bSkipEnvmap )
	{
		static char const* s_pPixelShaders[] = 
		{
			"VertexLitGeneric_EnvmapV2",
			"VertexLitGeneric_SelfIlluminatedEnvmapV2",

			"VertexLitGeneric_BaseAlphaMaskedEnvmapV2",
			"VertexLitGeneric_SelfIlluminatedEnvmapV2",

			// Env map mask
			"VertexLitGeneric_MaskedEnvmapV2",
			"VertexLitGeneric_SelfIlluminatedMaskedEnvmapV2",

			"VertexLitGeneric_MaskedEnvmapV2",
			"VertexLitGeneric_SelfIlluminatedMaskedEnvmapV2",

			// Detail
			"VertexLitGeneric_DetailEnvmapV2",
			"VertexLitGeneric_DetailSelfIlluminatedEnvmapV2",

			"VertexLitGeneric_DetailBaseAlphaMaskedEnvmapV2",
			"VertexLitGeneric_DetailSelfIlluminatedEnvmapV2",

			// Env map mask
			"VertexLitGeneric_DetailMaskedEnvmapV2",
			"VertexLitGeneric_DetailSelfIlluminatedMaskedEnvmapV2",

			"VertexLitGeneric_DetailMaskedEnvmapV2",
			"VertexLitGeneric_DetailSelfIlluminatedMaskedEnvmapV2",
		};

		if ( !params[BASETEXTURE]->IsTexture() )
		{
			if (params[ENVMAP]->IsTexture() && !bSkipEnvmap )
			{
				if (!params[ENVMAPMASK]->IsTexture())
				{
					return "VertexLitGeneric_EnvmapNoTexture";
				}
				else
				{
					return "VertexLitGeneric_MaskedEnvmapNoTexture";
				}
			}
			else
			{
				if ( params[DETAIL]->IsTexture() )
				{
					return "VertexLitGeneric_DetailNoTexture";
				}
				else
				{
					return "VertexLitGeneric_NoTexture";
				}
			}
		}
		else
		{
			if ( params[BLENDTINTBYBASEALPHA]->GetIntValue() )
			{
				return "VertexLitGeneric_BlendTint";
			}
			else if ( params[ENVMAP]->IsTexture() && !bSkipEnvmap )
			{
				int pshIndex = 0;
				if (IS_FLAG_SET(MATERIAL_VAR_SELFILLUM))
					pshIndex |= 0x1;
				if (IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK))
					pshIndex |= 0x2;
				if (params[ENVMAPMASK]->IsTexture())
					pshIndex |= 0x4;
				if (params[DETAIL]->IsTexture())
					pshIndex |= 0x8;
				return s_pPixelShaders[pshIndex];
			}
			else
			{
				if (IS_FLAG_SET(MATERIAL_VAR_SELFILLUM))
				{
					if ( params[DETAIL]->IsTexture() )
						return "VertexLitGeneric_DetailSelfIlluminated";
					else
						return "VertexLitGeneric_SelfIlluminated";
				}
				else if ( params[DETAIL]->IsTexture() )
				{
					switch( params[DETAILBLENDMODE]->GetIntValue() )
					{
						case 0:
							return "VertexLitGeneric_Detail";

						case 1:								// additive modes
							return "VertexLitGeneric_Detail_Additive";

						case 5:
						case 6:
							return "VertexLitGeneric_Detail_Additive_selfillum";
							break;
							
						default:
							return "VertexLitGeneric_Detail_LerpBase";
					}
				}
				else
				{
					return "VertexLitGeneric";
				}
			}
		}
	}

	void DrawUnbumpedUsingVertexShader( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, bool bSkipEnvmap )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

			if ( params[ALPHATESTREFERENCE]->GetFloatValue() > 0.0f )
			{
				pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GEQUAL, params[ALPHATESTREFERENCE]->GetFloatValue() );
			}

			int fmt = VERTEX_POSITION | VERTEX_NORMAL;

			// FIXME: We could enable this, but we'd never get it working on dx7 or lower
			// FIXME: This isn't going to work until we make more vertex shaders that
			// pass the vertex color and alpha values through.
#if 0
			if ( IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ) || IS_FLAG_SET( MATERIAL_VAR_VERTEXALPHA ) )
				fmt |= VERTEX_COLOR;
#endif

			if (params[ENVMAP]->IsTexture() && !bSkipEnvmap )
			{
				// envmap on stage 1
				pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

				// envmapmask on stage 2
				if (params[ENVMAPMASK]->IsTexture() || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK ) )
				{
					pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
				}
			}

			if (params[BASETEXTURE]->IsTexture())
			{
				SetDefaultBlendingShadowState( BASETEXTURE, true );
			}
			else
			{
				SetDefaultBlendingShadowState( ENVMAPMASK, false );
			}

 			if ( params[DETAIL]->IsTexture())
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
			}

			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

			// Set up the vertex shader index.
			vertexlitgeneric_vs11_Static_Index vshIndex;
			vshIndex.SetHALF_LAMBERT( IS_FLAG_SET( MATERIAL_VAR_HALFLAMBERT ) );
			//vshIndex.SetDETAIL( params[DETAIL]->IsTexture() );
			if( params[ENVMAP]->IsTexture() && !bSkipEnvmap )
			{
				vshIndex.SetENVMAP( true );
				vshIndex.SetENVMAPCAMERASPACE( IS_FLAG_SET(MATERIAL_VAR_ENVMAPCAMERASPACE) );
				if( IS_FLAG_SET(MATERIAL_VAR_ENVMAPCAMERASPACE) )
				{
					vshIndex.SetENVMAPSPHERE( false );
				}
				else
				{
					vshIndex.SetENVMAPSPHERE( IS_FLAG_SET( MATERIAL_VAR_ENVMAPSPHERE ) );
				}
			}
			else
			{
				vshIndex.SetENVMAP( false );
				vshIndex.SetENVMAPCAMERASPACE( false );
				vshIndex.SetENVMAPSPHERE( false );
			}
			pShaderShadow->SetVertexShader( "vertexlitgeneric_vs11", vshIndex.GetIndex() );

			const char *pshName = GetUnbumpedPixelShaderName( params, bSkipEnvmap );
			pShaderShadow->SetPixelShader( pshName );
			DefaultFog();
			pShaderShadow->EnableAlphaWrites( true );
		}
		DYNAMIC_STATE
		{
			if (params[BASETEXTURE]->IsTexture())
			{
				BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
				SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM );
			}

//			if (params[ENVMAP]->IsTexture())
			if (params[ENVMAP]->IsTexture() && !bSkipEnvmap )
			{
				BindTexture( SHADER_SAMPLER1, ENVMAP, ENVMAPFRAME );

				if (params[ENVMAPMASK]->IsTexture() || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
				{
					if (params[ENVMAPMASK]->IsTexture() )
						BindTexture( SHADER_SAMPLER2, ENVMAPMASK, ENVMAPMASKFRAME );
					else
						BindTexture( SHADER_SAMPLER2, BASETEXTURE, FRAME );
		
					SetVertexShaderTextureScaledTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, BASETEXTURETRANSFORM, ENVMAPMASKSCALE );
				}

				if (IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE) || 
					IS_FLAG_SET(MATERIAL_VAR_ENVMAPCAMERASPACE))
				{
					LoadViewMatrixIntoVertexShaderConstant( VERTEX_SHADER_VIEWMODEL );
				}
				SetEnvMapTintPixelShaderDynamicState( 2, ENVMAPTINT, -1 );
			}

			if ( params[DETAIL]->IsTexture())
			{
				BindTexture( SHADER_SAMPLER3, DETAIL, DETAILFRAME );
				SetVertexShaderTextureScaledTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, BASETEXTURETRANSFORM, DETAILSCALE );
			}

			SetAmbientCubeDynamicStateVertexShader();
			SetModulationPixelShaderDynamicState( 3 );
			EnablePixelShaderOverbright( 0, true, true );
			SetPixelShaderConstant( 1, SELFILLUMTINT );

			if ( params[DETAIL]->IsTexture() && ( ! IS_FLAG_SET( MATERIAL_VAR_SELFILLUM ) ) )
			{
				float c1[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
				c1[0] = c1[1] = c1[2] = c1[3] = params[DETAILBLENDFACTOR]->GetFloatValue();
				pShaderAPI->SetPixelShaderConstant( 1, c1, 1 );
			}

			if ( params[BLENDTINTBYBASEALPHA]->GetIntValue() )
			{
				float c1[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
				c1[0] = c1[1] = c1[2] = c1[3] = params[BLENDTINTCOLOROVERBASE]->GetFloatValue();
				pShaderAPI->SetPixelShaderConstant( 1, c1 );
			}

			vertexlitgeneric_vs11_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			vshIndex.SetSKINNING( pShaderAPI->GetCurrentNumBones() > 0 );
			vshIndex.SetLIGHT_COMBO( pShaderAPI->GetCurrentLightCombo() );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
			if( pShaderAPI->GetIntRenderingParameter( INT_RENDERPARM_WRITE_DEPTH_TO_DESTALPHA ) )
			{
				pShaderAPI->SetPixelShaderIndex( 0 );
			}
			else
			{
				// write 255 to alpha if we aren't told to write depth to dest alpha (We don't ever actually write depth to dest alpha in dx8)
				pShaderAPI->SetPixelShaderIndex( 1 );
				float c4[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
				pShaderAPI->SetPixelShaderConstant( 4, c4, 1 );
			}
		}
		Draw();
	}

	SHADER_DRAW
	{
		// Skip the standard rendering if cloak pass is fully opaque
		bool bDrawStandardPass = true;
		if ( false/*disabled! no effect*/ && params[CLOAKPASSENABLED]->GetIntValue() && ( pShaderShadow == NULL ) ) // && not snapshotting
		{
			CloakBlendedPassVars_t info;
			SetupVarsCloakBlendedPass( info );
			if ( CloakBlendedPassIsFullyOpaque( params, info ) )
			{
				// There is some strangeness in DX8 when trying to skip the main pass, so leave this alone for now
				//bDrawStandardPass = false;
			}
		}

		// Standard rendering pass
		if ( bDrawStandardPass )
		{
			// FLASHLIGHTFIXME: need to make these the same.
			bool hasFlashlight = UsingFlashlight( params );
			bool bBump = g_pConfig->UseBumpmapping() && params[BUMPMAP]->IsTexture();

			if( hasFlashlight )
			{
				DrawFlashlight_dx80( params, pShaderAPI, pShaderShadow, bBump, BUMPMAP, BUMPFRAME, BUMPTRANSFORM, 
					FLASHLIGHTTEXTURE, FLASHLIGHTTEXTUREFRAME, false, false, 0, -1, -1 );
			}
			else if( bBump )
			{
				bool bSkipEnvmap = true;
				DrawUnbumpedUsingVertexShader( params, pShaderAPI, pShaderShadow, bSkipEnvmap );

				// specular pass
				bool bBlendSpecular = true;
				if( params[ENVMAP]->IsTexture() )
				{
					DrawModelBumpedSpecularLighting( BUMPMAP, BUMPFRAME, ENVMAP, ENVMAPFRAME,
						ENVMAPTINT, ALPHA, ENVMAPCONTRAST, ENVMAPSATURATION, BUMPTRANSFORM, bBlendSpecular );
				}
			}
			else
			{
				bool bSkipEnvmap = false;
				DrawUnbumpedUsingVertexShader( params, pShaderAPI, pShaderShadow, bSkipEnvmap );
			}
		}
		else
		{
			// Skip this pass!
			Draw( false );
		}

		// Cloak Pass
		if ( params[CLOAKPASSENABLED]->GetIntValue() )
		{
			// If ( snapshotting ) or ( we need to draw this frame )
			if ( ( pShaderShadow != NULL ) || ( ( params[CLOAKFACTOR]->GetFloatValue() > 0.0f ) && ( params[CLOAKFACTOR]->GetFloatValue() < 1.0f ) ) )
			{
				CloakBlendedPassVars_t info;
				SetupVarsCloakBlendedPass( info );
				DrawCloakBlendedPass( this, params, pShaderAPI, pShaderShadow, info, vertexCompression );
			}
			else // We're not snapshotting and we don't need to draw this frame
			{
				// Skip this pass!
				Draw( false );
			}
		}

		// Emissive Scroll Pass
		if ( params[EMISSIVEBLENDENABLED]->GetIntValue() )
		{
			// If ( snapshotting ) or ( we need to draw this frame )
			if ( ( pShaderShadow != NULL ) || ( params[EMISSIVEBLENDSTRENGTH]->GetFloatValue() > 0.0f ) )
			{
				EmissiveScrollBlendedPassVars_t info;
				SetupVarsEmissiveScrollBlendedPass( info );
				DrawEmissiveScrollBlendedPass( this, params, pShaderAPI, pShaderShadow, info, vertexCompression );
			}
			else // We're not snapshotting and we don't need to draw this frame
			{
				// Skip this pass!
				Draw( false );
			}
		}

		// Flesh Interior Pass
		if ( params[FLESHINTERIORENABLED]->GetIntValue() )
		{
			// If ( snapshotting ) or ( we need to draw this frame )
			if ( ( pShaderShadow != NULL ) || ( true ) )
			{
				FleshInteriorBlendedPassVars_t info;
				SetupVarsFleshInteriorBlendedPass( info );
				DrawFleshInteriorBlendedPass( this, params, pShaderAPI, pShaderShadow, info, vertexCompression );
			}
			else // We're not snapshotting and we don't need to draw this frame
			{
				// Skip this pass!
				Draw( false );
			}
		}
	}
END_SHADER


//-----------------------------------------------------------------------------
// Version that doesn't do bumpmapping
//-----------------------------------------------------------------------------
BEGIN_INHERITED_SHADER( VertexLitGeneric_NoBump_DX8, VertexLitGeneric_DX8,
			  "Help for VertexLitGeneric_NoBump_DX8" )

	SHADER_FALLBACK
	{
		if (g_pConfig->bSoftwareLighting)
			return "VertexLitGeneric_DX6";

		if (!g_pHardwareConfig->SupportsVertexAndPixelShaders())
			return "VertexLitGeneric_DX7";

		return 0;
	}

	virtual bool ShouldUseBumpmapping( IMaterialVar **params ) 
	{
		if ( !g_pConfig->UseBumpmapping() )
			return false;

		if ( !params[BUMPMAP]->IsDefined() )
			return false;

		return ( params[FORCEBUMP]->GetIntValue() != 0 );
	}

END_INHERITED_SHADER

