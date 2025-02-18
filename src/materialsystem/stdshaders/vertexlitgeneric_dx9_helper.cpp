//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
#include "BaseVSShader.h"
#include "vertexlitgeneric_dx9_helper.h"
#include "skin_dx9_helper.h"

#include "VertexLit_and_unlit_Generic_vs20.inc"
#include "VertexLit_and_unlit_Generic_bump_vs20.inc"

#include "vertexlit_and_unlit_generic_ps20.inc"
#include "vertexlit_and_unlit_generic_ps20b.inc"
#include "vertexlit_and_unlit_generic_bump_ps20.inc"
#include "vertexlit_and_unlit_generic_bump_ps20b.inc"

#ifndef _X360
#include "vertexlit_and_unlit_generic_vs30.inc"
#include "vertexlit_and_unlit_generic_ps30.inc"
#include "vertexlit_and_unlit_generic_bump_vs30.inc"
#include "vertexlit_and_unlit_generic_bump_ps30.inc"
#endif

#include "commandbuilder.h"
#include "convar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar mat_fullbright( "mat_fullbright","0", FCVAR_CHEAT );
static ConVar r_lightwarpidentity( "r_lightwarpidentity","0", FCVAR_CHEAT );


static inline bool WantsSkinShader( IMaterialVar** params, const VertexLitGeneric_DX9_Vars_t &info )
{
	if ( info.m_nPhong == -1)								// Don't use skin without Phong
		return false;
	
	if ( params[info.m_nPhong]->GetIntValue() == 0 )		// Don't use skin without Phong turned on
		return false;

	if ( ( info.m_nDiffuseWarpTexture != -1 ) && params[info.m_nDiffuseWarpTexture]->IsTexture() ) // If there's Phong and diffuse warp do skin
		return true;

	if ( ( info.m_nBaseMapAlphaPhongMask != -1 ) && params[info.m_nBaseMapAlphaPhongMask]->GetIntValue() != 1 )
	{
		if ( info.m_nBumpmap == -1 )						// Don't use without a bump map
			return false;

		if ( !params[info.m_nBumpmap]->IsTexture() )		// Don't use if the texture isn't specified
			return false;
	}

	return true;
}

int g_nSnapShots;

//-----------------------------------------------------------------------------
// Initialize shader parameters
//-----------------------------------------------------------------------------
void InitParamsVertexLitGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, bool bVertexLitGeneric, VertexLitGeneric_DX9_Vars_t &info )
{	
	InitIntParam( info.m_nPhong, params, 0 );

	InitFloatParam( info.m_nAlphaTestReference, params, 0.0f );
	InitIntParam( info.m_nVertexAlphaTest, params, 0 );

	InitIntParam( info.m_nFlashlightNoLambert, params, 0 );

	if ( info.m_nDetailTint != -1 && !params[info.m_nDetailTint]->IsDefined() )
	{
		params[info.m_nDetailTint]->SetVecValue( 1.0f, 1.0f, 1.0f );
	}

	if ( info.m_nEnvmapTint != -1 && !params[info.m_nEnvmapTint]->IsDefined() )
	{
		params[info.m_nEnvmapTint]->SetVecValue( 1.0f, 1.0f, 1.0f );
	}

	InitIntParam( info.m_nEnvmapFrame, params, 0 );
	InitIntParam( info.m_nBumpFrame, params, 0 );
	InitFloatParam( info.m_nDetailTextureBlendFactor, params, 1.0 );
	InitIntParam( info.m_nReceiveFlashlight, params, 0 );

	InitFloatParam( info.m_nDetailScale, params, 4.0f );

	if ( (info.m_nBlendTintByBaseAlpha != -1) && (!params[info.m_nBlendTintByBaseAlpha]->IsDefined()) )
	{
		params[info.m_nBlendTintByBaseAlpha]->SetIntValue( 0 );
	}

	InitFloatParam( info.m_nTintReplacesBaseColor, params, 0 );

	if ( (info.m_nSelfIllumTint != -1) && (!params[info.m_nSelfIllumTint]->IsDefined()) )
	{
		params[info.m_nSelfIllumTint]->SetVecValue( 1.0f, 1.0f, 1.0f );
	}


	if ( WantsSkinShader( params, info ) )
	{
		if ( !g_pHardwareConfig->SupportsPixelShaders_2_b() || !g_pConfig->UsePhong() )
		{
			params[info.m_nPhong]->SetIntValue( 0 );
		}
		else
		{
			InitParamsSkin_DX9( pShader, params, pMaterialName, info );
			return;
		}
	}
	
	// FLASHLIGHTFIXME: Do ShaderAPI::BindFlashlightTexture
	if ( info.m_nFlashlightTexture != -1 )
	{
		if ( g_pHardwareConfig->SupportsBorderColor() )
		{
			params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight_border" );
		}
		else
		{
			params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight001" );
		}
	}
	
	// Write over $basetexture with $info.m_nBumpmap if we are going to be using diffuse normal mapping.
	if ( info.m_nAlbedo != -1 && g_pConfig->UseBumpmapping() && info.m_nBumpmap != -1 && params[info.m_nBumpmap]->IsDefined() && params[info.m_nAlbedo]->IsDefined() &&
		params[info.m_nBaseTexture]->IsDefined() )
	{
		params[info.m_nBaseTexture]->SetStringValue( params[info.m_nAlbedo]->GetStringValue() );
	}

	// This shader can be used with hw skinning
	SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );

	if ( bVertexLitGeneric )
	{
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );
	}
	else
	{
		CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
	}

	InitIntParam( info.m_nEnvmapMaskFrame, params, 0 );
	InitFloatParam( info.m_nEnvmapContrast, params, 0.0 );
	InitFloatParam( info.m_nEnvmapSaturation, params, 1.0f );
	InitFloatParam( info.m_nSeamlessScale, params, 0.0 );

	// handle line art parms
	InitFloatParam( info.m_nEdgeSoftnessStart, params, 0.5 );
	InitFloatParam( info.m_nEdgeSoftnessEnd, params, 0.5 );
	InitFloatParam( info.m_nGlowAlpha, params, 1.0 );
	InitFloatParam( info.m_nOutlineAlpha, params, 1.0 );

	// No texture means no self-illum or env mask in base alpha
	if ( info.m_nBaseTexture != -1 && !params[info.m_nBaseTexture]->IsDefined() )
	{
		CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
		CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
	}

	// If in decal mode, no debug override...
	if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
	{
		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
	}

	if( ( (info.m_nBumpmap != -1) && g_pConfig->UseBumpmapping() && params[info.m_nBumpmap]->IsDefined() ) 
		// we don't need a tangent space if we have envmap without bumpmap
		//		|| ( info.m_nEnvmap != -1 && params[info.m_nEnvmap]->IsDefined() ) 
		)
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
	}
	else if ( (info.m_nDiffuseWarpTexture != -1) && params[info.m_nDiffuseWarpTexture]->IsDefined() ) // diffuse warp goes down bump path...
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
	}
	else // no tangent space needed
	{
		CLEAR_FLAGS( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
	}

	bool hasNormalMapAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
	if ( hasNormalMapAlphaEnvmapMask )
	{
		params[info.m_nEnvmapMask]->SetUndefined();
		CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
	}

	if ( IS_FLAG_SET( MATERIAL_VAR_BASEALPHAENVMAPMASK ) && info.m_nBumpmap != -1 && 
		params[info.m_nBumpmap]->IsDefined() && !hasNormalMapAlphaEnvmapMask )
	{
		Warning( "material %s has a normal map and $basealphaenvmapmask.  Must use $normalmapalphaenvmapmask to get specular.\n\n", pMaterialName );
		params[info.m_nEnvmap]->SetUndefined();
	}
	
	if ( info.m_nEnvmapMask != -1 && params[info.m_nEnvmapMask]->IsDefined() && info.m_nBumpmap != -1 && params[info.m_nBumpmap]->IsDefined() )
	{
		params[info.m_nEnvmapMask]->SetUndefined();
		if ( !hasNormalMapAlphaEnvmapMask )
		{
			Warning( "material %s has a normal map and an envmapmask.  Must use $normalmapalphaenvmapmask.\n\n", pMaterialName );
			params[info.m_nEnvmap]->SetUndefined();
		}
	}

	// If mat_specular 0, then get rid of envmap
	if ( !g_pConfig->UseSpecular() && info.m_nEnvmap != -1 && params[info.m_nEnvmap]->IsDefined() && params[info.m_nBaseTexture]->IsDefined() )
	{
		params[info.m_nEnvmap]->SetUndefined();
	}

	InitFloatParam( info.m_nHDRColorScale, params, 1.0f );

	InitIntParam( info.m_nLinearWrite, params, 0 );
	InitIntParam( info.m_nGammaColorRead, params, 0 );

	InitIntParam( info.m_nDepthBlend, params, 0 );
	InitFloatParam( info.m_nDepthBlendScale, params, 50.0f );
}


//-----------------------------------------------------------------------------
// Initialize shader
//-----------------------------------------------------------------------------

void InitVertexLitGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, bool bVertexLitGeneric, VertexLitGeneric_DX9_Vars_t &info )
{
	// both detailed and bumped = needs skin shader (for now)
	bool bNeedsSkinBecauseOfDetail = false;
	
	//bool bHasBump = ( info.m_nBumpmap != -1 ) && params[info.m_nBumpmap]->IsTexture();
	//if ( bHasBump )
	//{
	//	if (  ( info.m_nDetail != -1 ) && params[info.m_nDetail]->IsDefined() )
	//		bNeedsSkinBecauseOfDetail = true;
	//}

	if ( bNeedsSkinBecauseOfDetail || 
		 ( info.m_nPhong != -1 && 
		   params[info.m_nPhong]->GetIntValue() && 
		 g_pHardwareConfig->SupportsPixelShaders_2_b() ) )
	{
		InitSkin_DX9( pShader, params, info );
		return;
	}

	if ( info.m_nFlashlightTexture != -1 )
	{
		pShader->LoadTexture( info.m_nFlashlightTexture, TEXTUREFLAGS_SRGB );
	}
	
	bool bIsBaseTextureTranslucent = false;
	if ( info.m_nBaseTexture != -1 && params[info.m_nBaseTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nBaseTexture, ( info.m_nGammaColorRead != -1 ) && ( params[info.m_nGammaColorRead]->GetIntValue() == 1 ) ? 0 : TEXTUREFLAGS_SRGB );
		
		if ( params[info.m_nBaseTexture]->GetTextureValue()->IsTranslucent() )
		{
			bIsBaseTextureTranslucent = true;
		}
	}

	bool bHasSelfIllumMask = IS_FLAG_SET( MATERIAL_VAR_SELFILLUM ) && (info.m_nSelfIllumMask != -1) && params[info.m_nSelfIllumMask]->IsDefined();

	// No alpha channel in any of the textures? No self illum or envmapmask
	if ( !bIsBaseTextureTranslucent )
	{
		bool bHasSelfIllumFresnel = IS_FLAG_SET( MATERIAL_VAR_SELFILLUM ) && ( info.m_nSelfIllumFresnel != -1 ) && ( params[info.m_nSelfIllumFresnel]->GetIntValue() != 0 );

		// Can still be self illum with no base alpha if using one of these alternate modes
		if ( !bHasSelfIllumFresnel && !bHasSelfIllumMask )
		{
			CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
		}

		CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
	}

	if ( info.m_nDetail != -1 && params[info.m_nDetail]->IsDefined() )
	{
		int nDetailBlendMode = ( info.m_nDetailTextureCombineMode == -1 ) ? 0 : params[info.m_nDetailTextureCombineMode]->GetIntValue();
		if ( nDetailBlendMode == 0 ) //Mod2X
			pShader->LoadTexture( info.m_nDetail );
		else
			pShader->LoadTexture( info.m_nDetail, TEXTUREFLAGS_SRGB );
	}

	if ( g_pConfig->UseBumpmapping() )
	{
		if ( (info.m_nBumpmap != -1) && params[info.m_nBumpmap]->IsDefined() )
		{
			pShader->LoadBumpMap( info.m_nBumpmap );
			SET_FLAGS2( MATERIAL_VAR2_DIFFUSE_BUMPMAPPED_MODEL );
		}
		else if ( (info.m_nDiffuseWarpTexture != -1) && params[info.m_nDiffuseWarpTexture]->IsDefined() )
		{
			SET_FLAGS2( MATERIAL_VAR2_DIFFUSE_BUMPMAPPED_MODEL );
		}
	}

	// Don't alpha test if the alpha channel is used for other purposes
	if ( IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
	{
		CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );
	}
	
	if ( info.m_nEnvmap != -1 && params[info.m_nEnvmap]->IsDefined() )
	{
		if ( !IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE) )
		{
			pShader->LoadCubeMap( info.m_nEnvmap, g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE ? TEXTUREFLAGS_SRGB : 0 );
		}
		else
		{
			pShader->LoadTexture( info.m_nEnvmap, g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE ? TEXTUREFLAGS_SRGB : 0 );
		}
		
		if ( !g_pHardwareConfig->SupportsCubeMaps() )
		{
			SET_FLAGS( MATERIAL_VAR_ENVMAPSPHERE );
		}
	}
	if ( info.m_nEnvmapMask != -1 && params[info.m_nEnvmapMask]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nEnvmapMask );
	}

	if ( (info.m_nDiffuseWarpTexture != -1) && params[info.m_nDiffuseWarpTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nDiffuseWarpTexture );
	}

	if ( bHasSelfIllumMask )
	{
		pShader->LoadTexture( info.m_nSelfIllumMask );
	}
}

class CVertexLitGeneric_DX9_Context : public CBasePerMaterialContextData
{
public:
	CCommandBufferBuilder< CFixedCommandStorageBuffer< 800 > > m_SemiStaticCmdsOut;

};


//-----------------------------------------------------------------------------
// Draws the shader
//-----------------------------------------------------------------------------
static void DrawVertexLitGeneric_DX9_Internal( CBaseVSShader *pShader, IMaterialVar** params,
											   IShaderDynamicAPI *pShaderAPI,
											   IShaderShadow* pShaderShadow,
											   bool bVertexLitGeneric, bool bHasFlashlight, 
											   VertexLitGeneric_DX9_Vars_t &info,
											   VertexCompressionType_t vertexCompression,
											   CBasePerMaterialContextData **pContextDataPtr ) 

{
	CVertexLitGeneric_DX9_Context *pContextData = reinterpret_cast< CVertexLitGeneric_DX9_Context *> ( *pContextDataPtr );

/*^*/ // 	printf("\t\t>DrawVertexLitGeneric_DX9_Internal\n");

	bool bHasBump = IsTextureSet( info.m_nBumpmap, params );
#if !defined( _X360 )
	bool bIsDecal = IS_FLAG_SET( MATERIAL_VAR_DECAL );
#endif

	bool hasDiffuseLighting = bVertexLitGeneric;
/*^*/ // 	printf("\t\t[%d] bVertexLitGeneric\n",(int)bVertexLitGeneric);

	if ( IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE) )
	{
		bHasFlashlight = false;
	}

	bool bIsAlphaTested = IS_FLAG_SET( MATERIAL_VAR_ALPHATEST ) != 0;
	bool bHasDiffuseWarp = (!bHasFlashlight || IsX360() ) && hasDiffuseLighting && (info.m_nDiffuseWarpTexture != -1) && params[info.m_nDiffuseWarpTexture]->IsTexture();


	//bool bNoCull = IS_FLAG_SET( MATERIAL_VAR_NOCULL );
	bool bFlashlightNoLambert = false;
	if ( ( info.m_nFlashlightNoLambert != -1 ) && params[info.m_nFlashlightNoLambert]->GetIntValue() )
	{
		bFlashlightNoLambert = true;
	}

	bool bAmbientOnly = IsBoolSet( info.m_nAmbientOnly, params );

	float fBlendFactor = GetFloatParam( info.m_nDetailTextureBlendFactor, params, 1.0 );
	bool bHasDetailTexture = IsTextureSet( info.m_nDetail, params );
	int nDetailBlendMode = bHasDetailTexture ? GetIntParam( info.m_nDetailTextureCombineMode, params ) : 0;
	int nDetailTranslucencyTexture = -1;

	if ( bHasDetailTexture )
	{
		if ( ( nDetailBlendMode == 6 ) && ( ! (g_pHardwareConfig->SupportsPixelShaders_2_b() ) ) )
		{
			nDetailBlendMode = 5;								// skip fancy threshold blending if ps2.0
		}
		if ( ( nDetailBlendMode == 3 ) || ( nDetailBlendMode == 8 ) || ( nDetailBlendMode == 9 ) )
			nDetailTranslucencyTexture = info.m_nDetail;
	}

	bool bBlendTintByBaseAlpha = IsBoolSet( info.m_nBlendTintByBaseAlpha, params );
	float fTintReplaceFactor = GetFloatParam( info.m_nTintReplacesBaseColor, params, 0.0 );

	BlendType_t nBlendType;
	bool bHasBaseTexture = IsTextureSet( info.m_nBaseTexture, params );
	if ( bHasBaseTexture )
	{
		// if base alpha is used for tinting, ignore the base texture for computing translucency
		nBlendType = pShader->EvaluateBlendRequirements( bBlendTintByBaseAlpha ? -1 : info.m_nBaseTexture, true, nDetailTranslucencyTexture );
	}
	else
	{
		nBlendType = pShader->EvaluateBlendRequirements( info.m_nEnvmapMask, false );
	}
	bool bFullyOpaque = (nBlendType != BT_BLENDADD) && (nBlendType != BT_BLEND) && !bIsAlphaTested && (!bHasFlashlight || IsX360() ); //dest alpha is free for special use

	bool bHasEnvmap = (!bHasFlashlight || IsX360() ) && info.m_nEnvmap != -1 && params[info.m_nEnvmap]->IsTexture();

	
	bool bHasVertexColor = bVertexLitGeneric ? false : IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR );
	bool bHasVertexAlpha = bVertexLitGeneric ? false : IS_FLAG_SET( MATERIAL_VAR_VERTEXALPHA );
/*^*/ // 	printf("\t\t[%d] bHasVertexColor\n",(int)bHasVertexColor);
/*^*/ // 	printf("\t\t[%d] bHasVertexAlpha\n",(int)bHasVertexAlpha);

	if ( pShader->IsSnapshotting() || (! pContextData ) || ( pContextData->m_bMaterialVarsChanged ) )
	{
/*^*/ // 	printf("\t\t[1] snapshotting=%d  pContextData=%08x  pContextData->m_bMaterialVarsChanged=%d \n",(int)pShader->IsSnapshotting(), (int)pContextData, pContextData ? (int)pContextData->m_bMaterialVarsChanged : -1 );
		bool bSeamlessBase = IsBoolSet( info.m_nSeamlessBase, params );
		bool bSeamlessDetail = IsBoolSet( info.m_nSeamlessDetail, params );
		bool bDistanceAlpha = IsBoolSet( info.m_nDistanceAlpha, params );
		bool bHasSelfIllum = (!bHasFlashlight || IsX360() ) && IS_FLAG_SET( MATERIAL_VAR_SELFILLUM );
		bool bHasEnvmapMask = (!bHasFlashlight || IsX360() ) && info.m_nEnvmapMask != -1 && params[info.m_nEnvmapMask]->IsTexture();
		bool bHasSelfIllumFresnel = ( !IsTextureSet( info.m_nDetail, params ) ) && ( bHasSelfIllum ) && ( info.m_nSelfIllumFresnel != -1 ) && ( params[info.m_nSelfIllumFresnel]->GetIntValue() != 0 );

		bool bHasSelfIllumMask = bHasSelfIllum && IsTextureSet( info.m_nSelfIllumMask, params );
		bool hasSelfIllumInEnvMapMask =
			( info.m_nSelfIllumEnvMapMask_Alpha != -1 ) &&
			( params[info.m_nSelfIllumEnvMapMask_Alpha]->GetFloatValue() != 0.0 ) ;

		if  ( pShader->IsSnapshotting() )
		{
/*^*/ // 		printf("\t\t[2] snapshotting...\n");

			bool hasBaseAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_BASEALPHAENVMAPMASK );
			bool hasNormalMapAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
			
			
			if ( info.m_nVertexAlphaTest != -1 && params[info.m_nVertexAlphaTest]->GetIntValue() > 0 )
			{
				bHasVertexAlpha = true;
			}

			// look at color and alphamod stuff.
			// Unlit generic never uses the flashlight
			if ( bHasSelfIllumFresnel )
			{
				CLEAR_FLAGS( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
				hasNormalMapAlphaEnvmapMask = false;
			}

			bool bHasEnvmap = (!bHasFlashlight || IsX360() ) && ( info.m_nEnvmap != -1 ) && params[info.m_nEnvmap]->IsTexture();
			bool bHasLegacyEnvSphereMap = bHasEnvmap && IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE);
			bool bHasNormal = bVertexLitGeneric || bHasEnvmap || bHasFlashlight || bSeamlessBase || bSeamlessDetail;
			if ( IsPC() )
			{
				// On PC, LIGHTING_PREVIEW requires normals (they won't use much memory - unlitgeneric isn't used on many models)
				bHasNormal = true;
			}

			bool bHalfLambert = IS_FLAG_SET( MATERIAL_VAR_HALFLAMBERT );
			// Alpha test: FIXME: shouldn't this be handled in CBaseVSShader::SetInitialShadowState
			pShaderShadow->EnableAlphaTest( bIsAlphaTested );

			if ( info.m_nAlphaTestReference != -1 && params[info.m_nAlphaTestReference]->GetFloatValue() > 0.0f )
			{
				pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GEQUAL, params[info.m_nAlphaTestReference]->GetFloatValue() );
			}

			int nShadowFilterMode = 0;
			if ( bHasFlashlight )
			{
				if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					nShadowFilterMode = g_pHardwareConfig->GetShadowFilterMode();	// Based upon vendor and device dependent formats
				}

				if ( !IsX360() )
				{
					if (params[info.m_nBaseTexture]->IsTexture())
					{
						pShader->SetAdditiveBlendingShadowState( info.m_nBaseTexture, true );
					}
					else
					{
						pShader->SetAdditiveBlendingShadowState( info.m_nEnvmapMask, false );
					}

					if ( bIsAlphaTested )
					{
						// disable alpha test and use the zfunc zequals since alpha isn't guaranteed to 
						// be the same on both the regular pass and the flashlight pass.
						pShaderShadow->EnableAlphaTest( false );
						pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_EQUAL );
					}

					// Be sure not to write to dest alpha
					pShaderShadow->EnableAlphaWrites( false );

					pShaderShadow->EnableBlending( true );
					pShaderShadow->EnableDepthWrites( false );
				}
				else
				{
					pShader->SetBlendingShadowState( nBlendType );
				}
			}
			else
			{
				pShader->SetBlendingShadowState( nBlendType );
			}
		
			unsigned int flags = VERTEX_POSITION;
			if ( bHasNormal )
			{
				flags |= VERTEX_NORMAL;
			}
/*^*/ // 	printf("\t\t[%1d] VERTEX_NORMAL\n",(flags&VERTEX_NORMAL)!=0);

			int userDataSize = 0;
			bool bSRGBInputAdapter = false;

			// basetexture
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			if ( bHasBaseTexture )
			{
				if ( ( info.m_nGammaColorRead != -1 ) && ( params[info.m_nGammaColorRead]->GetIntValue() == 1 ) )
					pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, false );
				else
					pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );

				// If we're on OSX GL on a crappy OS which can't do sRGB from render targets, check to see if we're reading from one...
				if ( IsOSX() && !g_pHardwareConfig->CanDoSRGBReadFromRTs() )
				{
					ITexture *pBaseTexture = params[info.m_nBaseTexture]->GetTextureValue();
					if ( pBaseTexture && pBaseTexture->IsRenderTarget() )
					{
						bSRGBInputAdapter = true;
					}
				}
			}

			if ( bHasEnvmap )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
				if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE )
				{
					pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, true );
				}
			}
			if ( bHasFlashlight )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER8, true );	// Depth texture
				pShaderShadow->SetShadowDepthFiltering( SHADER_SAMPLER8 );
				pShaderShadow->EnableTexture( SHADER_SAMPLER6, true );	// Noise map
				pShaderShadow->EnableTexture( SHADER_SAMPLER7, true );	// Flashlight cookie
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER7, true );
				userDataSize = 4; // tangent S
			}

			if ( bHasDetailTexture )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
				if ( nDetailBlendMode != 0 ) //Not Mod2X
					pShaderShadow->EnableSRGBRead( SHADER_SAMPLER2, true );
			}

			if ( bHasBump || bHasDiffuseWarp )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
				userDataSize = 4; // tangent S
				// Normalizing cube map
				pShaderShadow->EnableTexture( SHADER_SAMPLER5, true );
			}
			if ( bHasEnvmapMask )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );
			}

			if ( bHasVertexColor || bHasVertexAlpha )
			{
				flags |= VERTEX_COLOR;
			}
/*^*/ // 	printf("\t\t[%1d] VERTEX_COLOR\n",(flags&VERTEX_COLOR)!=0);
/*^*/ // 	printf("\t\t[%1d] VERTEX_COLOR_STREAM_1\n",(flags&VERTEX_COLOR_STREAM_1)!=0);


			if( bHasDiffuseWarp && (!bHasFlashlight || IsX360() ) && !bHasSelfIllumFresnel )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER9, true );	// Diffuse warp texture
			}

			if ( (info.m_nDepthBlend != -1) && (params[info.m_nDepthBlend]->GetIntValue()) )
			{
				if( bHasBump )
					Warning( "DEPTHBLEND not supported by bump mapped variations of vertexlitgeneric to avoid shader bloat. Either remove the bump map or convince a graphics programmer that it's worth it.\n"  );
			
				pShaderShadow->EnableTexture( SHADER_SAMPLER10, true );
			}

			if( bHasSelfIllum )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER11, true );	// self illum mask
			}

			bool bSRGBWrite = true;
			if( (info.m_nLinearWrite != -1) && (params[info.m_nLinearWrite]->GetIntValue() == 1) )
			{
				bSRGBWrite = false;
			}

			pShaderShadow->EnableSRGBWrite( bSRGBWrite );
		
			// texcoord0 : base texcoord
			int pTexCoordDim[3] = { 2, 2, 3 };
			int nTexCoordCount = 1;
		
			if ( IsBoolSet( info.m_nSeparateDetailUVs, params ) )
			{
				++nTexCoordCount;
			}
			else
			{
				pTexCoordDim[1] = 0;
			}

#ifndef _X360
			// Special morphed decal information 
			if ( bIsDecal && g_pHardwareConfig->HasFastVertexTextures() )
			{
				nTexCoordCount = 3;
			}
#endif

			// This shader supports compressed vertices, so OR in that flag:
			flags |= VERTEX_FORMAT_COMPRESSED;
/*^*/ // 		printf("\t\t[%1d] VERTEX_FORMAT_COMPRESSED\n",(flags&VERTEX_FORMAT_COMPRESSED)!=0);

/*^*/ // 		printf("\t\t      -> CShaderShadowDX8::VertexShaderVertexFormat( flags=%08x, texcount=%d )\n",flags,nTexCoordCount);


			pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, pTexCoordDim, userDataSize );

			if ( bHasBump || bHasDiffuseWarp )
			{
#ifndef _X360
				if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
				{
					bool bUseStaticControlFlow = g_pHardwareConfig->SupportsStaticControlFlow();

					DECLARE_STATIC_VERTEX_SHADER( vertexlit_and_unlit_generic_bump_vs20 );
					SET_STATIC_VERTEX_SHADER_COMBO( HALFLAMBERT,  bHalfLambert);
					SET_STATIC_VERTEX_SHADER_COMBO( USE_WITH_2B,  g_pHardwareConfig->SupportsPixelShaders_2_b() );
#ifdef _X360
					SET_STATIC_VERTEX_SHADER_COMBO( FLASHLIGHT,  bHasFlashlight );
#endif
					SET_STATIC_VERTEX_SHADER_COMBO( USE_STATIC_CONTROL_FLOW, bUseStaticControlFlow );
					SET_STATIC_VERTEX_SHADER( vertexlit_and_unlit_generic_bump_vs20 );
				
					if ( g_pHardwareConfig->SupportsPixelShaders_2_b() || g_pHardwareConfig->ShouldAlwaysUseShaderModel2bShaders() ) // Always send GL this way
					{
						DECLARE_STATIC_PIXEL_SHADER( vertexlit_and_unlit_generic_bump_ps20b );
						SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP,  bHasEnvmap );
						SET_STATIC_PIXEL_SHADER_COMBO( DIFFUSELIGHTING,  hasDiffuseLighting );
						SET_STATIC_PIXEL_SHADER_COMBO( LIGHTWARPTEXTURE, bHasDiffuseWarp && !bHasSelfIllumFresnel );
						SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM,  bHasSelfIllum );
						SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUMFRESNEL, bHasSelfIllumFresnel );
						SET_STATIC_PIXEL_SHADER_COMBO( NORMALMAPALPHAENVMAPMASK,  hasNormalMapAlphaEnvmapMask && bHasEnvmap );
						SET_STATIC_PIXEL_SHADER_COMBO( HALFLAMBERT,  bHalfLambert);
						SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT,  bHasFlashlight );
						SET_STATIC_PIXEL_SHADER_COMBO( DETAILTEXTURE,  bHasDetailTexture );
						SET_STATIC_PIXEL_SHADER_COMBO( DETAIL_BLEND_MODE, nDetailBlendMode );
						SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHTDEPTHFILTERMODE, nShadowFilterMode );
						SET_STATIC_PIXEL_SHADER_COMBO( BLENDTINTBYBASEALPHA, bBlendTintByBaseAlpha );
						SET_STATIC_PIXEL_SHADER( vertexlit_and_unlit_generic_bump_ps20b );
					}
					else // ps_2_0
					{
						DECLARE_STATIC_PIXEL_SHADER( vertexlit_and_unlit_generic_bump_ps20 );
						SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP,  bHasEnvmap );
						SET_STATIC_PIXEL_SHADER_COMBO( DIFFUSELIGHTING,  hasDiffuseLighting );
						SET_STATIC_PIXEL_SHADER_COMBO( LIGHTWARPTEXTURE, bHasDiffuseWarp && !bHasSelfIllumFresnel );
						SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM,  bHasSelfIllum );
						SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUMFRESNEL, bHasSelfIllumFresnel );
						SET_STATIC_PIXEL_SHADER_COMBO( NORMALMAPALPHAENVMAPMASK,  hasNormalMapAlphaEnvmapMask && bHasEnvmap );
						SET_STATIC_PIXEL_SHADER_COMBO( HALFLAMBERT,  bHalfLambert);
						SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT,  bHasFlashlight );
						SET_STATIC_PIXEL_SHADER_COMBO( DETAILTEXTURE,  bHasDetailTexture );
						SET_STATIC_PIXEL_SHADER_COMBO( DETAIL_BLEND_MODE, nDetailBlendMode );
						SET_STATIC_PIXEL_SHADER_COMBO( BLENDTINTBYBASEALPHA, bBlendTintByBaseAlpha );
						SET_STATIC_PIXEL_SHADER( vertexlit_and_unlit_generic_bump_ps20 );
					}
				}
#ifndef _X360
				else
				{
					// The vertex shader uses the vertex id stream
					SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );

					DECLARE_STATIC_VERTEX_SHADER( vertexlit_and_unlit_generic_bump_vs30 );
					SET_STATIC_VERTEX_SHADER_COMBO( HALFLAMBERT,  bHalfLambert);
					SET_STATIC_VERTEX_SHADER_COMBO( USE_WITH_2B,  true );
					SET_STATIC_VERTEX_SHADER_COMBO( DECAL, bIsDecal );
					SET_STATIC_VERTEX_SHADER( vertexlit_and_unlit_generic_bump_vs30 );

					DECLARE_STATIC_PIXEL_SHADER( vertexlit_and_unlit_generic_bump_ps30 );
					SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP,  bHasEnvmap );
					SET_STATIC_PIXEL_SHADER_COMBO( DIFFUSELIGHTING,  hasDiffuseLighting );
					SET_STATIC_PIXEL_SHADER_COMBO( LIGHTWARPTEXTURE, bHasDiffuseWarp && !bHasSelfIllumFresnel );
					SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM,  bHasSelfIllum );
					SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUMFRESNEL, bHasSelfIllumFresnel );
					SET_STATIC_PIXEL_SHADER_COMBO( NORMALMAPALPHAENVMAPMASK,  hasNormalMapAlphaEnvmapMask && bHasEnvmap );
					SET_STATIC_PIXEL_SHADER_COMBO( HALFLAMBERT,  bHalfLambert);
					SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT,  bHasFlashlight );
					SET_STATIC_PIXEL_SHADER_COMBO( DETAILTEXTURE,  bHasDetailTexture );
					SET_STATIC_PIXEL_SHADER_COMBO( DETAIL_BLEND_MODE, nDetailBlendMode );
					SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHTDEPTHFILTERMODE, nShadowFilterMode );
					SET_STATIC_PIXEL_SHADER_COMBO( BLENDTINTBYBASEALPHA, bBlendTintByBaseAlpha );
					SET_STATIC_PIXEL_SHADER( vertexlit_and_unlit_generic_bump_ps30 );
				}
#endif
			}
			else // !(bHasBump || bHasDiffuseWarp)
			{
				bool bDistanceAlphaFromDetail = false;
				bool bSoftMask = false;
				bool bGlow = false;
				bool bOutline = false;

				static ConVarRef mat_reduceparticles( "mat_reduceparticles" );
				bool bDoDepthBlend = IsBoolSet( info.m_nDepthBlend, params ) && !mat_reduceparticles.GetBool();

				if ( bDistanceAlpha )
				{
					bDistanceAlphaFromDetail = IsBoolSet( info.m_nDistanceAlphaFromDetail, params );
					bSoftMask = IsBoolSet( info.m_nSoftEdges, params );
					bGlow = IsBoolSet( info.m_nGlow, params );
					bOutline = IsBoolSet( info.m_nOutline, params );
				}

#ifndef _X360
				if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
				{
					bool bUseStaticControlFlow = g_pHardwareConfig->SupportsStaticControlFlow();

					DECLARE_STATIC_VERTEX_SHADER( vertexlit_and_unlit_generic_vs20 );
					SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR,  bHasVertexColor || bHasVertexAlpha );
					SET_STATIC_VERTEX_SHADER_COMBO( CUBEMAP,  bHasEnvmap );
					SET_STATIC_VERTEX_SHADER_COMBO( HALFLAMBERT,  bHalfLambert );
					SET_STATIC_VERTEX_SHADER_COMBO( FLASHLIGHT,  bHasFlashlight );
					SET_STATIC_VERTEX_SHADER_COMBO( SEAMLESS_BASE, bSeamlessBase );
					SET_STATIC_VERTEX_SHADER_COMBO( SEAMLESS_DETAIL, bSeamlessDetail );
					SET_STATIC_VERTEX_SHADER_COMBO( SEPARATE_DETAIL_UVS, IsBoolSet( info.m_nSeparateDetailUVs, params ) );
					SET_STATIC_VERTEX_SHADER_COMBO( USE_STATIC_CONTROL_FLOW, bUseStaticControlFlow );
					SET_STATIC_VERTEX_SHADER_COMBO( DONT_GAMMA_CONVERT_VERTEX_COLOR, (! bSRGBWrite ) && bHasVertexColor );
					SET_STATIC_VERTEX_SHADER( vertexlit_and_unlit_generic_vs20 );

					if ( g_pHardwareConfig->SupportsPixelShaders_2_b() || g_pHardwareConfig->ShouldAlwaysUseShaderModel2bShaders() ) // Always send Gl this way
					{
						DECLARE_STATIC_PIXEL_SHADER( vertexlit_and_unlit_generic_ps20b );
						SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM_ENVMAPMASK_ALPHA, ( hasSelfIllumInEnvMapMask && ( bHasEnvmapMask ) ) );
						SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP,  bHasEnvmap );
						SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP_SPHERE_LEGACY,  bHasLegacyEnvSphereMap );
						SET_STATIC_PIXEL_SHADER_COMBO( DIFFUSELIGHTING,  hasDiffuseLighting );
						SET_STATIC_PIXEL_SHADER_COMBO( ENVMAPMASK,  bHasEnvmapMask );
						SET_STATIC_PIXEL_SHADER_COMBO( BASEALPHAENVMAPMASK,  hasBaseAlphaEnvmapMask );
						SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM,  bHasSelfIllum );
						SET_STATIC_PIXEL_SHADER_COMBO( VERTEXCOLOR,  bHasVertexColor );
						SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT,  bHasFlashlight );
						SET_STATIC_PIXEL_SHADER_COMBO( DETAILTEXTURE,  bHasDetailTexture );
						SET_STATIC_PIXEL_SHADER_COMBO( DETAIL_BLEND_MODE, nDetailBlendMode );
						SET_STATIC_PIXEL_SHADER_COMBO( SEAMLESS_BASE, bSeamlessBase );
						SET_STATIC_PIXEL_SHADER_COMBO( SEAMLESS_DETAIL, bSeamlessDetail );
						SET_STATIC_PIXEL_SHADER_COMBO( DISTANCEALPHA, bDistanceAlpha );
						SET_STATIC_PIXEL_SHADER_COMBO( DISTANCEALPHAFROMDETAIL, bDistanceAlphaFromDetail );
						SET_STATIC_PIXEL_SHADER_COMBO( SOFT_MASK, bSoftMask );
						SET_STATIC_PIXEL_SHADER_COMBO( OUTLINE, bOutline );
						SET_STATIC_PIXEL_SHADER_COMBO( OUTER_GLOW, bGlow );
						SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHTDEPTHFILTERMODE, nShadowFilterMode );
						SET_STATIC_PIXEL_SHADER_COMBO( DEPTHBLEND, bDoDepthBlend );
						SET_STATIC_PIXEL_SHADER_COMBO( SRGB_INPUT_ADAPTER, bSRGBInputAdapter ? 1 : 0 );
						SET_STATIC_PIXEL_SHADER_COMBO( BLENDTINTBYBASEALPHA, bBlendTintByBaseAlpha );
						SET_STATIC_PIXEL_SHADER( vertexlit_and_unlit_generic_ps20b );
					}
					else // ps_2_0
					{
						DECLARE_STATIC_PIXEL_SHADER( vertexlit_and_unlit_generic_ps20 );
						SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM_ENVMAPMASK_ALPHA, ( hasSelfIllumInEnvMapMask && ( bHasEnvmapMask ) ) );
						SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP,  bHasEnvmap );
						SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP_SPHERE_LEGACY,  bHasLegacyEnvSphereMap );
						SET_STATIC_PIXEL_SHADER_COMBO( DIFFUSELIGHTING,  hasDiffuseLighting );
						SET_STATIC_PIXEL_SHADER_COMBO( ENVMAPMASK,  bHasEnvmapMask );
						SET_STATIC_PIXEL_SHADER_COMBO( BASEALPHAENVMAPMASK,  hasBaseAlphaEnvmapMask );
						SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM,  bHasSelfIllum );
						SET_STATIC_PIXEL_SHADER_COMBO( VERTEXCOLOR,  bHasVertexColor );
						SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT,  bHasFlashlight );
						SET_STATIC_PIXEL_SHADER_COMBO( DETAILTEXTURE,  bHasDetailTexture );
						SET_STATIC_PIXEL_SHADER_COMBO( DETAIL_BLEND_MODE, nDetailBlendMode );
						SET_STATIC_PIXEL_SHADER_COMBO( SEAMLESS_BASE, bSeamlessBase );
						SET_STATIC_PIXEL_SHADER_COMBO( SEAMLESS_DETAIL, bSeamlessDetail );
						SET_STATIC_PIXEL_SHADER_COMBO( DISTANCEALPHA, bDistanceAlpha );
						SET_STATIC_PIXEL_SHADER_COMBO( DISTANCEALPHAFROMDETAIL, bDistanceAlphaFromDetail );
						SET_STATIC_PIXEL_SHADER_COMBO( SOFT_MASK, bSoftMask );
						SET_STATIC_PIXEL_SHADER_COMBO( OUTLINE, bOutline );
						SET_STATIC_PIXEL_SHADER_COMBO( OUTER_GLOW, bGlow );
						SET_STATIC_PIXEL_SHADER_COMBO( BLENDTINTBYBASEALPHA, bBlendTintByBaseAlpha );
						SET_STATIC_PIXEL_SHADER( vertexlit_and_unlit_generic_ps20 );
					}
				}
#ifndef _X360
				else
				{
					// The vertex shader uses the vertex id stream
					SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );

					DECLARE_STATIC_VERTEX_SHADER( vertexlit_and_unlit_generic_vs30 );
					SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR,  bHasVertexColor || bHasVertexAlpha );
					SET_STATIC_VERTEX_SHADER_COMBO( CUBEMAP,  bHasEnvmap );
					SET_STATIC_VERTEX_SHADER_COMBO( HALFLAMBERT,  bHalfLambert );
					SET_STATIC_VERTEX_SHADER_COMBO( FLASHLIGHT,  bHasFlashlight );
					SET_STATIC_VERTEX_SHADER_COMBO( SEAMLESS_BASE, bSeamlessBase );
					SET_STATIC_VERTEX_SHADER_COMBO( SEAMLESS_DETAIL, bSeamlessDetail );
					SET_STATIC_VERTEX_SHADER_COMBO( SEPARATE_DETAIL_UVS, IsBoolSet( info.m_nSeparateDetailUVs, params ) );
					SET_STATIC_VERTEX_SHADER_COMBO( DECAL, bIsDecal );
					SET_STATIC_VERTEX_SHADER_COMBO( DONT_GAMMA_CONVERT_VERTEX_COLOR, bSRGBWrite ? 0 : 1 );
					SET_STATIC_VERTEX_SHADER( vertexlit_and_unlit_generic_vs30 );

					DECLARE_STATIC_PIXEL_SHADER( vertexlit_and_unlit_generic_ps30 );
					SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM_ENVMAPMASK_ALPHA, ( hasSelfIllumInEnvMapMask && ( bHasEnvmapMask ) ) );
					SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP,  bHasEnvmap );
					SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP_SPHERE_LEGACY,  bHasLegacyEnvSphereMap );
					SET_STATIC_PIXEL_SHADER_COMBO( DIFFUSELIGHTING,  hasDiffuseLighting );
					SET_STATIC_PIXEL_SHADER_COMBO( ENVMAPMASK,  bHasEnvmapMask );
					SET_STATIC_PIXEL_SHADER_COMBO( BASEALPHAENVMAPMASK,  hasBaseAlphaEnvmapMask );
					SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM,  bHasSelfIllum );
					SET_STATIC_PIXEL_SHADER_COMBO( VERTEXCOLOR,  bHasVertexColor );
					SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT,  bHasFlashlight );
					SET_STATIC_PIXEL_SHADER_COMBO( DETAILTEXTURE,  bHasDetailTexture );
					SET_STATIC_PIXEL_SHADER_COMBO( DETAIL_BLEND_MODE, nDetailBlendMode );
					SET_STATIC_PIXEL_SHADER_COMBO( SEAMLESS_BASE, bSeamlessBase );
					SET_STATIC_PIXEL_SHADER_COMBO( SEAMLESS_DETAIL, bSeamlessDetail );
					SET_STATIC_PIXEL_SHADER_COMBO( DISTANCEALPHA, bDistanceAlpha );
					SET_STATIC_PIXEL_SHADER_COMBO( DISTANCEALPHAFROMDETAIL, bDistanceAlphaFromDetail );
					SET_STATIC_PIXEL_SHADER_COMBO( SOFT_MASK, bSoftMask );
					SET_STATIC_PIXEL_SHADER_COMBO( OUTLINE, bOutline );
					SET_STATIC_PIXEL_SHADER_COMBO( OUTER_GLOW, bGlow );
					SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHTDEPTHFILTERMODE, nShadowFilterMode );
					SET_STATIC_PIXEL_SHADER_COMBO( DEPTHBLEND, bDoDepthBlend );
					SET_STATIC_PIXEL_SHADER_COMBO( BLENDTINTBYBASEALPHA, bBlendTintByBaseAlpha );
					SET_STATIC_PIXEL_SHADER( vertexlit_and_unlit_generic_ps30 );
				}
#endif
			}

			if ( bHasFlashlight && !IsX360() )
			{
				pShader->FogToBlack();
			}
			else
			{
				pShader->DefaultFog();
			}

			// HACK HACK HACK - enable alpha writes all the time so that we have them for
			// underwater stuff and the loadout and character select screens.
			pShaderShadow->EnableAlphaWrites( bFullyOpaque );
		}

		if ( pShaderAPI && ( (! pContextData ) || ( pContextData->m_bMaterialVarsChanged ) ) )
		{
/*^*/ // 		printf("\t\t[3] pShaderAPI && ( (! pContextData ) || ( pContextData->m_bMaterialVarsChanged ) )  TRUE \n");
			if ( ! pContextData )								// make sure allocated
			{
				++g_nSnapShots;
				pContextData = new CVertexLitGeneric_DX9_Context;
				*pContextDataPtr = pContextData;
			}
			pContextData->m_SemiStaticCmdsOut.Reset();
			pContextData->m_SemiStaticCmdsOut.SetPixelShaderFogParams( 21 );
			if ( bHasBaseTexture )
			{
				pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER0, info.m_nBaseTexture, info.m_nBaseTextureFrame );
			}
			else
			{
				if( bHasEnvmap )
				{
					// if we only have an envmap (no basetexture), then we want the albedo to be black.
					pContextData->m_SemiStaticCmdsOut.BindStandardTexture( SHADER_SAMPLER0, TEXTURE_BLACK );
				}
				else
				{
					pContextData->m_SemiStaticCmdsOut.BindStandardTexture( SHADER_SAMPLER0, TEXTURE_WHITE );
				}
			}
			if ( bHasDetailTexture )
			{
				pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER2, info.m_nDetail, info.m_nDetailFrame );
			}
			if ( bHasSelfIllum )
			{
				if ( bHasSelfIllumMask )												// Separate texture for self illum?
				{
					pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER11, info.m_nSelfIllumMask, -1 );	// Bind it
				}
				else																	// else
				{
					pContextData->m_SemiStaticCmdsOut.BindStandardTexture( SHADER_SAMPLER11, TEXTURE_BLACK );	// Bind dummy
				}
			}

			if ( (info.m_nDepthBlend != -1) && (params[info.m_nDepthBlend]->GetIntValue()) )
			{
				pContextData->m_SemiStaticCmdsOut.BindStandardTexture( SHADER_SAMPLER10, TEXTURE_FRAME_BUFFER_FULL_DEPTH );
			}
			if ( bSeamlessDetail || bSeamlessBase )
			{
				float flSeamlessData[4]={ params[info.m_nSeamlessScale]->GetFloatValue(),
										  0,0,0};
				pContextData->m_SemiStaticCmdsOut.SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, flSeamlessData );
			}

			if ( info.m_nBaseTextureTransform != -1 )
			{
				pContextData->m_SemiStaticCmdsOut.SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, info.m_nBaseTextureTransform );
			}
			
			
			if ( bHasDetailTexture )
			{
				if ( IS_PARAM_DEFINED( info.m_nDetailTextureTransform ) )
					pContextData->m_SemiStaticCmdsOut.SetVertexShaderTextureScaledTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, info.m_nDetailTextureTransform, info.m_nDetailScale );
				else
					pContextData->m_SemiStaticCmdsOut.SetVertexShaderTextureScaledTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, info.m_nBaseTextureTransform, info.m_nDetailScale );
				//Assert( !bHasBump );
				if ( info.m_nDetailTint  != -1 )
					pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstantGammaToLinear( 10, info.m_nDetailTint );
				else
				{
					pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant4( 10, 1, 1, 1, 1 );
				}
			}
			if ( bDistanceAlpha )
			{
				float flSoftStart = GetFloatParam( info.m_nEdgeSoftnessStart, params );
				float flSoftEnd = GetFloatParam( info.m_nEdgeSoftnessEnd, params );
				// set all line art shader parms
				bool bScaleEdges = IsBoolSet( info.m_nScaleEdgeSoftnessBasedOnScreenRes, params );
				bool bScaleOutline = IsBoolSet( info.m_nScaleOutlineSoftnessBasedOnScreenRes, params );

				float flResScale = 1.0;

				float flOutlineStart0 = GetFloatParam( info.m_nOutlineStart0, params );
				float flOutlineStart1 = GetFloatParam( info.m_nOutlineStart1, params );
				float flOutlineEnd0 = GetFloatParam( info.m_nOutlineEnd0, params );
				float flOutlineEnd1 = GetFloatParam( info.m_nOutlineEnd1, params );

				if ( bScaleEdges || bScaleOutline )
				{
					int nWidth, nHeight;
					pShaderAPI->GetBackBufferDimensions( nWidth, nHeight );
					flResScale=max( 0.5, max( 1024.0/nWidth, 768/nHeight ) );
				
					if ( bScaleEdges )
					{
						float flMid = 0.5 * ( flSoftStart + flSoftEnd );
						flSoftStart = clamp( flMid + flResScale * ( flSoftStart - flMid ), 0.05, 0.99 );
						flSoftEnd = clamp( flMid + flResScale * ( flSoftEnd - flMid ), 0.05, 0.99 );
					}
				

					if ( bScaleOutline )
					{
						// shrink the soft part of the outline, enlarging hard part
						float flMidS = 0.5 * ( flOutlineStart1 + flOutlineStart0 );
						flOutlineStart1 = clamp( flMidS + flResScale * ( flOutlineStart1 - flMidS ), 0.05, 0.99 );
						float flMidE = 0.5 * ( flOutlineEnd1 + flOutlineEnd0 );
						flOutlineEnd1 = clamp( flMidE + flResScale * ( flOutlineEnd1 - flMidE ), 0.05, 0.99 );
					}
					
				}

				float flConsts[]={
					// c5 - glow values
					GetFloatParam( info.m_nGlowX, params ),
					GetFloatParam( info.m_nGlowY, params ),
					GetFloatParam( info.m_nGlowStart, params ),
					GetFloatParam( info.m_nGlowEnd, params ),
					// c6 - glow color
					0,0,0,										// will be filled in
					GetFloatParam( info.m_nGlowAlpha, params ),
					// c7 - mask range parms
					flSoftStart,
					flSoftEnd,
					0,0,
					// c8 - outline color
					0,0,0,
					GetFloatParam( info.m_nOutlineAlpha, params ),
					// c9 - outline parms. ordered for optimal ps20 .wzyx swizzling
					flOutlineStart0,
					flOutlineEnd1,
					flOutlineEnd0,
					flOutlineStart1,
				};

				if ( info.m_nGlowColor != -1 )
				{
					params[info.m_nGlowColor]->GetVecValue( flConsts+4, 3 );
				}
				if ( info.m_nOutlineColor != -1 )
				{
					params[info.m_nOutlineColor]->GetVecValue( flConsts+12, 3 );
				}
				pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 5, flConsts, 5 );

			}
			if ( !g_pConfig->m_bFastNoBump )
			{
				if ( bHasBump )
				{
					pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER3, info.m_nBumpmap, info.m_nBumpFrame );
				}
				else if ( bHasDiffuseWarp )
				{
					pContextData->m_SemiStaticCmdsOut.BindStandardTexture( SHADER_SAMPLER3, TEXTURE_NORMALMAP_FLAT );
				}
			}
			else
			{
				if ( bHasBump )
				{
					pContextData->m_SemiStaticCmdsOut.BindStandardTexture( SHADER_SAMPLER3, TEXTURE_NORMALMAP_FLAT );
				}
			}
			// Setting w to 1 means use separate selfillummask
			float vEnvMapSaturation_SelfIllumMask[4] = {1.0f, 1.0f, 1.0f, 0.0f};
			if ( info.m_nEnvmapSaturation != -1 )
				params[info.m_nEnvmapSaturation]->GetVecValue( vEnvMapSaturation_SelfIllumMask, 3 );
			
			vEnvMapSaturation_SelfIllumMask[3] = bHasSelfIllumMask ? 1.0f : 0.0f;
			pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 3, vEnvMapSaturation_SelfIllumMask, 1 );
			if ( bHasEnvmap )
			{
				pContextData->m_SemiStaticCmdsOut.SetEnvMapTintPixelShaderDynamicStateGammaToLinear( 0, info.m_nEnvmapTint, fTintReplaceFactor );
			}
			else
			{
				pContextData->m_SemiStaticCmdsOut.SetEnvMapTintPixelShaderDynamicStateGammaToLinear( 0, -1, fTintReplaceFactor);
			}

			if ( bHasEnvmapMask )
			{
				pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER4, info.m_nEnvmapMask, info.m_nEnvmapMaskFrame );
			}

			if ( bHasSelfIllumFresnel && (!bHasFlashlight || IsX360() ) )
			{
				float vConstScaleBiasExp[4] = { 1.0f, 0.0f, 1.0f, 0.0f };
				float flMin = IS_PARAM_DEFINED( info.m_nSelfIllumFresnelMinMaxExp ) ? params[info.m_nSelfIllumFresnelMinMaxExp]->GetVecValue()[0] : 0.0f;
				float flMax = IS_PARAM_DEFINED( info.m_nSelfIllumFresnelMinMaxExp ) ? params[info.m_nSelfIllumFresnelMinMaxExp]->GetVecValue()[1] : 1.0f;
				float flExp = IS_PARAM_DEFINED( info.m_nSelfIllumFresnelMinMaxExp ) ? params[info.m_nSelfIllumFresnelMinMaxExp]->GetVecValue()[2] : 1.0f;

				vConstScaleBiasExp[1] = ( flMax != 0.0f ) ? ( flMin / flMax ) : 0.0f; // Bias
				vConstScaleBiasExp[0] = 1.0f - vConstScaleBiasExp[1]; // Scale
				vConstScaleBiasExp[2] = flExp; // Exp
				vConstScaleBiasExp[3] = flMax; // Brightness

				pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 11, vConstScaleBiasExp );
			}

			if( bHasDiffuseWarp && (!bHasFlashlight || IsX360() ) && !bHasSelfIllumFresnel )
			{
				if ( r_lightwarpidentity.GetBool() )
				{
					pContextData->m_SemiStaticCmdsOut.BindStandardTexture( SHADER_SAMPLER9, TEXTURE_IDENTITY_LIGHTWARP );
				}
				else
				{
					pContextData->m_SemiStaticCmdsOut.BindTexture( pShader, SHADER_SAMPLER9, info.m_nDiffuseWarpTexture, -1 );
				}
			}

			if ( bHasFlashlight )
			{
				// Tweaks associated with a given flashlight
				VMatrix worldToTexture;
				const FlashlightState_t &flashlightState = pShaderAPI->GetFlashlightState( worldToTexture );
				float tweaks[4];
				tweaks[0] = flashlightState.m_flShadowFilterSize / flashlightState.m_flShadowMapResolution;
				tweaks[1] = ShadowAttenFromState( flashlightState );
				pShader->HashShadow2DJitter( flashlightState.m_flShadowJitterSeed, &tweaks[2], &tweaks[3] );
				pShaderAPI->SetPixelShaderConstant( 2, tweaks, 1 );

				// Dimensions of screen, used for screen-space noise map sampling
				float vScreenScale[4] = {1280.0f / 32.0f, 720.0f / 32.0f, 0, 0};
				int nWidth, nHeight;
				pShaderAPI->GetBackBufferDimensions( nWidth, nHeight );
				vScreenScale[0] = (float) nWidth  / 32.0f;
				vScreenScale[1] = (float) nHeight / 32.0f;
				pShaderAPI->SetPixelShaderConstant( 31, vScreenScale, 1 );
			}

			if ( ( !bHasFlashlight || IsX360() ) && ( info.m_nEnvmapContrast != -1 ) )
				pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 2, info.m_nEnvmapContrast );

			// mat_fullbright 2 handling
			bool bLightingOnly = bVertexLitGeneric && mat_fullbright.GetInt() == 2 && !IS_FLAG_SET( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
			if( bLightingOnly )
			{
				if ( bHasBaseTexture )
				{
					if( ( bHasSelfIllum && !hasSelfIllumInEnvMapMask )  )
					{
						pContextData->m_SemiStaticCmdsOut.BindStandardTexture( SHADER_SAMPLER0, TEXTURE_GREY_ALPHA_ZERO );
					}
					else
					{
						pContextData->m_SemiStaticCmdsOut.BindStandardTexture( SHADER_SAMPLER0, TEXTURE_GREY );
					}
				}
				if ( bHasDetailTexture )
				{
					pContextData->m_SemiStaticCmdsOut.BindStandardTexture( SHADER_SAMPLER2, TEXTURE_GREY );
				}
			}
			
			if ( bHasBump || bHasDiffuseWarp )
			{
				pContextData->m_SemiStaticCmdsOut.BindStandardTexture( SHADER_SAMPLER5, TEXTURE_NORMALIZATION_CUBEMAP_SIGNED );
				pContextData->m_SemiStaticCmdsOut.SetPixelShaderStateAmbientLightCube( 5 );
				pContextData->m_SemiStaticCmdsOut.CommitPixelShaderLighting( 13 );
			}
			pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant_W( 4, info.m_nSelfIllumTint, fBlendFactor );
			pContextData->m_SemiStaticCmdsOut.SetAmbientCubeDynamicStateVertexShader();
			pContextData->m_SemiStaticCmdsOut.End();
		}
	}
	if ( pShaderAPI )
	{
		CCommandBufferBuilder< CFixedCommandStorageBuffer< 1000 > > DynamicCmdsOut;
		DynamicCmdsOut.Call( pContextData->m_SemiStaticCmdsOut.Base() );

		if ( bHasEnvmap )
		{
			DynamicCmdsOut.BindTexture( pShader, SHADER_SAMPLER1, info.m_nEnvmap, info.m_nEnvmapFrame );
		}

		bool bFlashlightShadows = false;
		if ( bHasFlashlight )
		{
			VMatrix worldToTexture;
			ITexture *pFlashlightDepthTexture;
			FlashlightState_t state = pShaderAPI->GetFlashlightStateEx( worldToTexture, &pFlashlightDepthTexture );
			bFlashlightShadows = state.m_bEnableShadows && ( pFlashlightDepthTexture != NULL );

			if( pFlashlightDepthTexture && g_pConfig->ShadowDepthTexture() && state.m_bEnableShadows )
			{
				pShader->BindTexture( SHADER_SAMPLER8, pFlashlightDepthTexture, 0 );
				DynamicCmdsOut.BindStandardTexture( SHADER_SAMPLER6, TEXTURE_SHADOW_NOISE_2D );
			}

			SetFlashLightColorFromState( state, pShaderAPI, 28, bFlashlightNoLambert );

			Assert( info.m_nFlashlightTexture >= 0 && info.m_nFlashlightTextureFrame >= 0 );
			pShader->BindTexture( SHADER_SAMPLER7, state.m_pSpotlightTexture, state.m_nSpotlightTextureFrame );
		}


		// Set up light combo state
		LightState_t lightState = {0, false, false};
		if ( bVertexLitGeneric && (!bHasFlashlight || IsX360() ) )
		{
			pShaderAPI->GetDX9LightState( &lightState );
		}

		MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();
		int fogIndex = ( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z ) ? 1 : 0;
		int numBones = pShaderAPI->GetCurrentNumBones();

		bool bWriteDepthToAlpha;
		bool bWriteWaterFogToAlpha;
		if( bFullyOpaque ) 
		{
			bWriteDepthToAlpha = pShaderAPI->ShouldWriteDepthToDestAlpha();
			bWriteWaterFogToAlpha = (fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z);
			AssertMsg( !(bWriteDepthToAlpha && bWriteWaterFogToAlpha), "Can't write two values to alpha at the same time." );
		}
		else
		{
			//can't write a special value to dest alpha if we're actually using as-intended alpha
			bWriteDepthToAlpha = false;
			bWriteWaterFogToAlpha = false;
		}

		if ( bHasBump || bHasDiffuseWarp )
		{
#ifndef _X360
			if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
			{
				bool bUseStaticControlFlow = g_pHardwareConfig->SupportsStaticControlFlow();

				DECLARE_DYNAMIC_VERTEX_SHADER( vertexlit_and_unlit_generic_bump_vs20 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG,  fogIndex );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING,  numBones > 0 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( NUM_LIGHTS, bUseStaticControlFlow ? 0 : lightState.m_nNumLights );
				SET_DYNAMIC_VERTEX_SHADER_CMD( DynamicCmdsOut, vertexlit_and_unlit_generic_bump_vs20 );

				// Bind ps_2_b shader so we can get shadow mapping...
				if ( g_pHardwareConfig->SupportsPixelShaders_2_b() || g_pHardwareConfig->ShouldAlwaysUseShaderModel2bShaders() ) // Always send GL this way
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( vertexlit_and_unlit_generic_bump_ps20b );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LIGHTS, lightState.m_nNumLights );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( AMBIENT_LIGHT, lightState.m_bAmbientLight ? 1 : 0 );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTSHADOWS, bFlashlightShadows );
//					SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
					SET_DYNAMIC_PIXEL_SHADER_CMD( DynamicCmdsOut, vertexlit_and_unlit_generic_bump_ps20b );
				}
				else
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( vertexlit_and_unlit_generic_bump_ps20 );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LIGHTS, lightState.m_nNumLights );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( AMBIENT_LIGHT, lightState.m_bAmbientLight ? 1 : 0 );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITEWATERFOGTODESTALPHA, bWriteWaterFogToAlpha );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
					SET_DYNAMIC_PIXEL_SHADER_CMD( DynamicCmdsOut, vertexlit_and_unlit_generic_bump_ps20 );
				}
			}
#ifndef _X360
			else
			{
				pShader->SetHWMorphVertexShaderState( VERTEX_SHADER_SHADER_SPECIFIC_CONST_10, VERTEX_SHADER_SHADER_SPECIFIC_CONST_11, SHADER_VERTEXTEXTURE_SAMPLER0 );

				DECLARE_DYNAMIC_VERTEX_SHADER( vertexlit_and_unlit_generic_bump_vs30 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG,  fogIndex );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING,  numBones > 0 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( MORPHING, pShaderAPI->IsHWMorphingEnabled() );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
				SET_DYNAMIC_VERTEX_SHADER( vertexlit_and_unlit_generic_bump_vs30 );

				DECLARE_DYNAMIC_PIXEL_SHADER( vertexlit_and_unlit_generic_bump_ps30 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LIGHTS, lightState.m_nNumLights );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( AMBIENT_LIGHT, lightState.m_bAmbientLight ? 1 : 0 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTSHADOWS, bFlashlightShadows );
//				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER_CMD( DynamicCmdsOut, vertexlit_and_unlit_generic_bump_ps30 );

				bool bUnusedTexCoords[3] = { false, false, !pShaderAPI->IsHWMorphingEnabled() || !bIsDecal };
				pShaderAPI->MarkUnusedVertexFields( 0, 3, bUnusedTexCoords );
			}
#endif
		}
		else // !( bHasBump || bHasDiffuseWarp )
		{
			if ( bAmbientOnly )	// Override selected light combo to be ambient only
			{
				lightState.m_bAmbientLight = true;
				lightState.m_bStaticLight = false;
				lightState.m_nNumLights = 0;
			}

#ifndef _X360
			if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
			{
				bool bUseStaticControlFlow = g_pHardwareConfig->SupportsStaticControlFlow();

				DECLARE_DYNAMIC_VERTEX_SHADER( vertexlit_and_unlit_generic_vs20 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( DYNAMIC_LIGHT, lightState.HasDynamicLight() );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( STATIC_LIGHT,  lightState.m_bStaticLight  ? 1 : 0 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG,  fogIndex );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING,  numBones > 0 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO(
					LIGHTING_PREVIEW, 
					pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_ENABLE_FIXED_LIGHTING)!=0);
				SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( NUM_LIGHTS, bUseStaticControlFlow ? 0 : lightState.m_nNumLights );
				SET_DYNAMIC_VERTEX_SHADER_CMD( DynamicCmdsOut, vertexlit_and_unlit_generic_vs20 );

				// Bind ps_2_b shader so we can get shadow mapping
				if ( g_pHardwareConfig->SupportsPixelShaders_2_b() || g_pHardwareConfig->ShouldAlwaysUseShaderModel2bShaders() ) // Always send GL this way
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( vertexlit_and_unlit_generic_ps20b );

//					SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTSHADOWS, bFlashlightShadows );
					SET_DYNAMIC_PIXEL_SHADER_COMBO(
						LIGHTING_PREVIEW,
						pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_ENABLE_FIXED_LIGHTING) );
					SET_DYNAMIC_PIXEL_SHADER_CMD( DynamicCmdsOut, vertexlit_and_unlit_generic_ps20b );
				}
				else
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( vertexlit_and_unlit_generic_ps20 );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
					SET_DYNAMIC_PIXEL_SHADER_COMBO(
						LIGHTING_PREVIEW,
						pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_ENABLE_FIXED_LIGHTING) );
					SET_DYNAMIC_PIXEL_SHADER_CMD( DynamicCmdsOut, vertexlit_and_unlit_generic_ps20 );
				}
			}
#ifndef _X360
			else
			{
				pShader->SetHWMorphVertexShaderState( VERTEX_SHADER_SHADER_SPECIFIC_CONST_10, VERTEX_SHADER_SHADER_SPECIFIC_CONST_11, SHADER_VERTEXTEXTURE_SAMPLER0 );

				DECLARE_DYNAMIC_VERTEX_SHADER( vertexlit_and_unlit_generic_vs30 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( DYNAMIC_LIGHT, lightState.HasDynamicLight() );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( STATIC_LIGHT,  lightState.m_bStaticLight  ? 1 : 0 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG,  fogIndex );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING,  numBones > 0 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( LIGHTING_PREVIEW, 
					pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_ENABLE_FIXED_LIGHTING)!=0);
				SET_DYNAMIC_VERTEX_SHADER_COMBO( MORPHING, pShaderAPI->IsHWMorphingEnabled() );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
				SET_DYNAMIC_VERTEX_SHADER_CMD( DynamicCmdsOut, vertexlit_and_unlit_generic_vs30 );

				DECLARE_DYNAMIC_PIXEL_SHADER( vertexlit_and_unlit_generic_ps30 );
//				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTSHADOWS, bFlashlightShadows );
				SET_DYNAMIC_PIXEL_SHADER_COMBO(	LIGHTING_PREVIEW,
					pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_ENABLE_FIXED_LIGHTING) );
				SET_DYNAMIC_PIXEL_SHADER_CMD( DynamicCmdsOut, vertexlit_and_unlit_generic_ps30 );

				bool bUnusedTexCoords[3] = { false, false, !pShaderAPI->IsHWMorphingEnabled() || !bIsDecal };
				pShaderAPI->MarkUnusedVertexFields( 0, 3, bUnusedTexCoords );
			}
#endif
		}

		if ( ( info.m_nHDRColorScale != -1 ) && pShader->IsHDREnabled() )
		{
			pShader->SetModulationPixelShaderDynamicState_LinearColorSpace_LinearScale( 1, params[info.m_nHDRColorScale]->GetFloatValue() );
		}
		else
		{
			pShader->SetModulationPixelShaderDynamicState_LinearColorSpace( 1 );
		}

		float eyePos[4];
		pShaderAPI->GetWorldSpaceCameraPosition( eyePos );
		DynamicCmdsOut.SetPixelShaderConstant( 20, eyePos );
		
		// Non-bump case does its own depth feathering work
		if ( !bHasBump && !bHasDiffuseWarp )
		{
			DynamicCmdsOut.SetDepthFeatheringPixelShaderConstant( 13, GetFloatParam( info.m_nDepthBlendScale, params, 50.0f ) );
		}

		float fPixelFogType = pShaderAPI->GetPixelFogCombo() == 1 ? 1 : 0;
		float fWriteDepthToAlpha = bWriteDepthToAlpha && IsPC() ? 1 : 0;
		float fWriteWaterFogToDestAlpha = (pShaderAPI->GetPixelFogCombo() == 1 && bWriteWaterFogToAlpha) ? 1 : 0;
		float fVertexAlpha = bHasVertexAlpha ? 1 : 0;

		// Controls for lerp-style paths through shader code (bump and non-bump have use different register)
		float vShaderControls[4] = { fPixelFogType, fWriteDepthToAlpha, fWriteWaterFogToDestAlpha, fVertexAlpha	 };
		DynamicCmdsOut.SetPixelShaderConstant( 12, vShaderControls, 1 );

		// flashlightfixme: put this in common code.
		if ( bHasFlashlight )
		{
			VMatrix worldToTexture;
			const FlashlightState_t &flashlightState = pShaderAPI->GetFlashlightState( worldToTexture );
			SetFlashLightColorFromState( flashlightState, pShaderAPI, 28, bFlashlightNoLambert );

			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, worldToTexture.Base(), 4 );

			pShader->BindTexture( SHADER_SAMPLER7, flashlightState.m_pSpotlightTexture, flashlightState.m_nSpotlightTextureFrame );

			float atten_pos[8];
			atten_pos[0] = flashlightState.m_fConstantAtten;			// Set the flashlight attenuation factors
			atten_pos[1] = flashlightState.m_fLinearAtten;
			atten_pos[2] = flashlightState.m_fQuadraticAtten;
			atten_pos[3] = flashlightState.m_FarZ;
			atten_pos[4] = flashlightState.m_vecLightOrigin[0];			// Set the flashlight origin
			atten_pos[5] = flashlightState.m_vecLightOrigin[1];
			atten_pos[6] = flashlightState.m_vecLightOrigin[2];
			atten_pos[7] = 1.0f;
			DynamicCmdsOut.SetPixelShaderConstant( 22, atten_pos, 2 );

			DynamicCmdsOut.SetPixelShaderConstant( 24, worldToTexture.Base(), 4 );
		}
		DynamicCmdsOut.End();
		pShaderAPI->ExecuteCommandBuffer( DynamicCmdsOut.Base() );
	}
	pShader->Draw();

/*^*/ // 	printf("\t\t<DrawVertexLitGeneric_DX9_Internal\n");
}


void DrawVertexLitGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
	IShaderShadow* pShaderShadow, bool bVertexLitGeneric, VertexLitGeneric_DX9_Vars_t &info, VertexCompressionType_t vertexCompression,
								CBasePerMaterialContextData **pContextDataPtr )
{
	if ( WantsSkinShader( params, info ) && g_pHardwareConfig->SupportsPixelShaders_2_b() && g_pConfig->UseBumpmapping() && g_pConfig->UsePhong() )
	{
		DrawSkin_DX9( pShader, params, pShaderAPI, pShaderShadow, info, vertexCompression, pContextDataPtr );
		return;
	}
	
	bool bReceiveFlashlight = bVertexLitGeneric;
	bool bNewFlashlight = IsX360();
	if ( bNewFlashlight )
	{
		bReceiveFlashlight = bReceiveFlashlight || ( GetIntParam( info.m_nReceiveFlashlight, params ) != 0 );
	}
	bool bHasFlashlight = bReceiveFlashlight && pShader->UsingFlashlight( params );

	DrawVertexLitGeneric_DX9_Internal( pShader, params, pShaderAPI,
		pShaderShadow, bVertexLitGeneric, bHasFlashlight, info, vertexCompression, pContextDataPtr );
}
