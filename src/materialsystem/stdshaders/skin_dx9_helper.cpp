//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
#include "BaseVSShader.h"
#include "skin_dx9_helper.h"
#include "convar.h"
#include "cpp_shader_constant_register_map.h"
#include "skin_vs20.inc"
#include "skin_ps20b.inc"
#include "commandbuilder.h"

#ifndef _X360
#include "skin_vs30.inc"
#include "skin_ps30.inc"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar mat_fullbright( "mat_fullbright", "0", FCVAR_CHEAT );
static ConVar r_lightwarpidentity( "r_lightwarpidentity", "0", FCVAR_CHEAT );
static ConVar r_rimlight( "r_rimlight", "1", FCVAR_CHEAT );

// Textures may be bound to the following samplers:
//	SHADER_SAMPLER0	 Base (Albedo) / Gloss in alpha
//	SHADER_SAMPLER1	 Specular warp (including iridescence)
//	SHADER_SAMPLER2	 Diffuse Lighting warp texture
//	SHADER_SAMPLER3	 Normal Map
//	SHADER_SAMPLER4	 Flashlight Shadow Depth Map
//	SHADER_SAMPLER5	 Normalization cube map
//	SHADER_SAMPLER6	 Flashlight Cookie
//	SHADER_SAMPLER7	 Specular exponent
//	SHADER_SAMPLER8	 Cubic environment map
//  SHADER_SAMPLER9  Compressed wrinklemap
//  SHADER_SAMPLER10 Stretched wrinklemap
//  SHADER_SAMPLER11 Compressed wrinkle normal map
//  SHADER_SAMPLER12 Stretched wrinkle normal map
//  SHADER_SAMPLER13 Detail texture


//-----------------------------------------------------------------------------
// Initialize shader parameters
//-----------------------------------------------------------------------------
void InitParamsSkin_DX9( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, VertexLitGeneric_DX9_Vars_t &info )
{	
	// FLASHLIGHTFIXME: Do ShaderAPI::BindFlashlightTexture
	Assert( info.m_nFlashlightTexture >= 0 );

	if ( g_pHardwareConfig->SupportsBorderColor() )
	{
		params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight_border" );
	}
	else
	{
		params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight001" );
	}
	
	// Write over $basetexture with $info.m_nBumpmap if we are going to be using diffuse normal mapping.
	if( info.m_nAlbedo != -1 && g_pConfig->UseBumpmapping() && info.m_nBumpmap != -1 && params[info.m_nBumpmap]->IsDefined() && params[info.m_nAlbedo]->IsDefined() &&
		params[info.m_nBaseTexture]->IsDefined() )
	{
		params[info.m_nBaseTexture]->SetStringValue( params[info.m_nAlbedo]->GetStringValue() );
	}

	// This shader can be used with hw skinning
	SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
	SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );

	// No texture means no env mask in base alpha
	if ( !params[info.m_nBaseTexture]->IsDefined() )
	{
		CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
	}

	// If in decal mode, no debug override...
	if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
	{
		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
	}

	// Lots of reasons to want tangent space, since we bind a flat normal map in many cases where we don't have a bump map
	bool bBump = (info.m_nBumpmap != -1) && g_pConfig->UseBumpmapping() && params[info.m_nBumpmap]->IsDefined();
	bool bEnvMap = (info.m_nEnvmap != -1) && params[info.m_nEnvmap]->IsDefined();
	bool bDiffuseWarp = (info.m_nDiffuseWarpTexture != -1) && params[info.m_nDiffuseWarpTexture]->IsDefined();
	bool bPhong = (info.m_nPhong != -1) && params[info.m_nPhong]->IsDefined();
	if( bBump || bEnvMap || bDiffuseWarp || bPhong )
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
	}
	else
	{
		CLEAR_FLAGS( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
	}

	if ( ( info.m_nSelfIllumFresnel != -1 ) && ( !params[info.m_nSelfIllumFresnel]->IsDefined() ) )
	{
		params[info.m_nSelfIllumFresnel]->SetIntValue( 0 );
	}

	if ( ( info.m_nSelfIllumFresnelMinMaxExp != -1 ) && ( !params[info.m_nSelfIllumFresnelMinMaxExp]->IsDefined() ) )
	{
		params[info.m_nSelfIllumFresnelMinMaxExp]->SetVecValue( 0.0f, 1.0f, 1.0f );
	}

	if ( ( info.m_nBaseMapAlphaPhongMask != -1 ) && ( !params[info.m_nBaseMapAlphaPhongMask]->IsDefined() ) )
	{
		params[info.m_nBaseMapAlphaPhongMask]->SetIntValue( 0 );
	}

	if ( ( info.m_nEnvmapFresnel != -1 ) && ( !params[info.m_nEnvmapFresnel]->IsDefined() ) )
	{
		params[info.m_nEnvmapFresnel]->SetFloatValue( 0 );
	}
}

//-----------------------------------------------------------------------------
// Initialize shader
//-----------------------------------------------------------------------------
void InitSkin_DX9( CBaseVSShader *pShader, IMaterialVar** params, VertexLitGeneric_DX9_Vars_t &info )
{
	Assert( info.m_nFlashlightTexture >= 0 );
	pShader->LoadTexture( info.m_nFlashlightTexture, TEXTUREFLAGS_SRGB );
	
	bool bIsBaseTextureTranslucent = false;
	if ( params[info.m_nBaseTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nBaseTexture, TEXTUREFLAGS_SRGB );
		
		if ( params[info.m_nBaseTexture]->GetTextureValue()->IsTranslucent() )
		{
			bIsBaseTextureTranslucent = true;
		}

		if ( ( info.m_nWrinkle != -1 ) && ( info.m_nStretch != -1 ) &&
			params[info.m_nWrinkle]->IsDefined() && params[info.m_nStretch]->IsDefined() )
		{
			pShader->LoadTexture( info.m_nWrinkle, TEXTUREFLAGS_SRGB );
			pShader->LoadTexture( info.m_nStretch, TEXTUREFLAGS_SRGB );
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

	if ( (info.m_nPhongExponentTexture != -1) && params[info.m_nPhongExponentTexture]->IsDefined() &&
		 (info.m_nPhong != -1) && params[info.m_nPhong]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nPhongExponentTexture );
	}

	if ( (info.m_nDiffuseWarpTexture != -1) && params[info.m_nDiffuseWarpTexture]->IsDefined() &&
		 (info.m_nPhong != -1) && params[info.m_nPhong]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nDiffuseWarpTexture );
	}

	if ( (info.m_nPhongWarpTexture != -1) && params[info.m_nPhongWarpTexture]->IsDefined() &&
		 (info.m_nPhong != -1) && params[info.m_nPhong]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nPhongWarpTexture );
	}

	if ( info.m_nDetail != -1 && params[info.m_nDetail]->IsDefined() )
	{
		int nDetailBlendMode = ( info.m_nDetailTextureCombineMode == -1 ) ? 0 : params[info.m_nDetailTextureCombineMode]->GetIntValue();
		if ( nDetailBlendMode == 0 ) // Mod2X
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

			if ( ( info.m_nNormalWrinkle != -1 ) && ( info.m_nNormalStretch != -1 ) &&
				params[info.m_nNormalWrinkle]->IsDefined() && params[info.m_nNormalStretch]->IsDefined() )
			{
				pShader->LoadTexture( info.m_nNormalWrinkle );
				pShader->LoadTexture( info.m_nNormalStretch );
			}
		}
	}
	
	if ( params[info.m_nEnvmap]->IsDefined() )
	{
		pShader->LoadCubeMap( info.m_nEnvmap, g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE ? TEXTUREFLAGS_SRGB : 0  );
	}

	if ( bHasSelfIllumMask )
	{
		pShader->LoadTexture( info.m_nSelfIllumMask );
	}
}

class CSkin_DX9_Context : public CBasePerMaterialContextData
{
public:
	CCommandBufferBuilder< CFixedCommandStorageBuffer< 800 > > m_SemiStaticCmdsOut;
	bool m_bFastPath;

};

//-----------------------------------------------------------------------------
// Draws the shader
//-----------------------------------------------------------------------------
void DrawSkin_DX9_Internal( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow,
	bool bHasFlashlight, VertexLitGeneric_DX9_Vars_t &info, VertexCompressionType_t vertexCompression,
							CBasePerMaterialContextData **pContextDataPtr )
{
	bool bHasBaseTexture = (info.m_nBaseTexture != -1) && params[info.m_nBaseTexture]->IsTexture();
	bool bHasBump = (info.m_nBumpmap != -1) && params[info.m_nBumpmap]->IsTexture();

	bool bHasBaseTextureWrinkle = bHasBaseTexture && 
		(info.m_nWrinkle != -1) && params[info.m_nWrinkle]->IsTexture() &&
		(info.m_nStretch != -1) && params[info.m_nStretch]->IsTexture();

	bool bHasBumpWrinkle = bHasBump && 
		(info.m_nNormalWrinkle != -1) && params[info.m_nNormalWrinkle]->IsTexture() &&
		(info.m_nNormalStretch != -1) && params[info.m_nNormalStretch]->IsTexture();

	bool bHasVertexColor = IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR );
	bool bHasVertexAlpha = IS_FLAG_SET( MATERIAL_VAR_VERTEXALPHA );
	bool bIsAlphaTested = IS_FLAG_SET( MATERIAL_VAR_ALPHATEST ) != 0;
	bool bHasSelfIllum = IS_FLAG_SET( MATERIAL_VAR_SELFILLUM ) != 0;
	bool bHasSelfIllumFresnel = ( bHasSelfIllum ) && ( info.m_nSelfIllumFresnel != -1 ) && ( params[info.m_nSelfIllumFresnel]->GetIntValue() != 0 );
	bool bHasSelfIllumMask = ( bHasSelfIllum ) && (info.m_nSelfIllumMask != -1) && params[info.m_nSelfIllumMask]->IsTexture();

	// Tie these to specular
	bool bHasPhong = (info.m_nPhong != -1) && ( params[info.m_nPhong]->GetIntValue() != 0 );
	bool bHasSpecularExponentTexture = (info.m_nPhongExponentTexture != -1) && params[info.m_nPhongExponentTexture]->IsTexture();
	bool bHasPhongTintMap = bHasSpecularExponentTexture && (info.m_nPhongAlbedoTint != -1) && ( params[info.m_nPhongAlbedoTint]->GetIntValue() != 0 );
	bool bHasDiffuseWarp = (info.m_nDiffuseWarpTexture != -1) && params[info.m_nDiffuseWarpTexture]->IsTexture();
	bool bHasPhongWarp = (info.m_nPhongWarpTexture != -1) && params[info.m_nPhongWarpTexture]->IsTexture();
	bool bHasNormalMapAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );

#if !defined( _X360 )
	bool bIsDecal = IS_FLAG_SET( MATERIAL_VAR_DECAL );
#endif

	// Rimlight must be set to non-zero to trigger rim light combo (also requires Phong)
	bool bHasRimLight = r_rimlight.GetBool() && bHasPhong && (info.m_nRimLight != -1) && ( params[info.m_nRimLight]->GetIntValue() != 0 );
	bool bHasRimMaskMap = bHasSpecularExponentTexture && bHasRimLight && (info.m_nRimMask != -1) && ( params[info.m_nRimMask]->GetIntValue() != 0 );

	float fBlendFactor=( info.m_nDetailTextureBlendFactor == -1 )? 1 : params[info.m_nDetailTextureBlendFactor]->GetFloatValue();
	bool hasDetailTexture = ( info.m_nDetail != -1 ) && params[info.m_nDetail]->IsTexture();
	int nDetailBlendMode = ( hasDetailTexture && info.m_nDetailTextureCombineMode != -1 ) ? params[info.m_nDetailTextureCombineMode]->GetIntValue() : 0;

	bool bBlendTintByBaseAlpha = IsBoolSet( info.m_nBlendTintByBaseAlpha, params ) && !bHasSelfIllum;	// Pixel shader can't do both BLENDTINTBYBASEALPHA and SELFILLUM, so let selfillum win

	float flTintReplacementAmount = GetFloatParam( info.m_nTintReplacesBaseColor, params );

	BlendType_t nBlendType= pShader->EvaluateBlendRequirements( bBlendTintByBaseAlpha ? -1 : info.m_nBaseTexture, true );

	bool bFullyOpaque = (nBlendType != BT_BLENDADD) && (nBlendType != BT_BLEND) && !bIsAlphaTested && !bHasFlashlight; //dest alpha is free for special use

	CSkin_DX9_Context *pContextData = reinterpret_cast< CSkin_DX9_Context *> ( *pContextDataPtr );
	if ( ! pContextData )
	{
		pContextData = new CSkin_DX9_Context;
		*pContextDataPtr = pContextData;
	}

	if( pShader->IsSnapshotting() )
	{
		// look at color and alphamod stuff.
		// Unlit generic never uses the flashlight
		bool bHasEnvmap = !bHasFlashlight && params[info.m_nEnvmap]->IsTexture();
		bool bHasNormal = params[info.m_nBumpmap]->IsTexture();
		bool bCanUseBaseAlphaPhongMaskFastPath = (info.m_nBaseMapAlphaPhongMask != -1) && ( params[info.m_nBaseMapAlphaPhongMask]->GetIntValue() != 0 );

		if ( ! ( params[info.m_nBaseTexture]->GetTextureValue()->IsTranslucent() ) )
			bCanUseBaseAlphaPhongMaskFastPath = true;
		
		pContextData->m_bFastPath =
			(! bHasBump ) && 
			(! bHasSpecularExponentTexture ) &&
			(! bHasPhongTintMap ) &&
			(! bHasPhongWarp ) && 
			(! bHasRimLight ) && 
			(! hasDetailTexture ) &&
			bCanUseBaseAlphaPhongMaskFastPath &&
			(! bHasSelfIllum ) &&
			(! bBlendTintByBaseAlpha );
		
		// Alpha test: FIXME: shouldn't this be handled in CBaseVSShader::SetInitialShadowState
		pShaderShadow->EnableAlphaTest( bIsAlphaTested );

		if( info.m_nAlphaTestReference != -1 && params[info.m_nAlphaTestReference]->GetFloatValue() > 0.0f )
		{
			pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GEQUAL, params[info.m_nAlphaTestReference]->GetFloatValue() );
		}

		int nShadowFilterMode = 0;
		if( bHasFlashlight )
		{
			if (params[info.m_nBaseTexture]->IsTexture())
			{
				pShader->SetAdditiveBlendingShadowState( info.m_nBaseTexture, true );
			}

			if( bIsAlphaTested )
			{
				// disable alpha test and use the zfunc zequals since alpha isn't guaranteed to 
				// be the same on both the regular pass and the flashlight pass.
				pShaderShadow->EnableAlphaTest( false );
				pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_EQUAL );
			}
			pShaderShadow->EnableBlending( true );
			pShaderShadow->EnableDepthWrites( false );

			// Be sure not to write to dest alpha
			pShaderShadow->EnableAlphaWrites( false );

			nShadowFilterMode = g_pHardwareConfig->GetShadowFilterMode();	// Based upon vendor and device dependent formats
		}
		else // not flashlight pass
		{
			if (params[info.m_nBaseTexture]->IsTexture())
			{
				pShader->SetDefaultBlendingShadowState( info.m_nBaseTexture, true );
			}

			if ( bHasEnvmap )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER8, true );	// Cubic environment map
				if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE )
				{
					pShaderShadow->EnableSRGBRead( SHADER_SAMPLER8, true );
				}
			}
		}
		
		unsigned int flags = VERTEX_POSITION;
		if( bHasNormal )
		{
			flags |= VERTEX_NORMAL;
		}

		int userDataSize = 0;

		// Always enable...will bind white if nothing specified...
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );		// Base (albedo) map
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );

		if ( bHasBaseTextureWrinkle || bHasBumpWrinkle )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER9, true );	// Base (albedo) compression map
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER9, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER10, true );	// Base (albedo) expansion map
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER10, true );
		}

		if( bHasDiffuseWarp )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );	// Diffuse warp texture
		}

		if( bHasPhongWarp )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );	// Specular warp texture
		}

		// Specular exponent map or dummy
		pShaderShadow->EnableTexture( SHADER_SAMPLER7, true );	// Specular exponent map

		if( bHasFlashlight )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );	// Shadow depth map
			pShaderShadow->SetShadowDepthFiltering( SHADER_SAMPLER4 );
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER4, false );
			pShaderShadow->EnableTexture( SHADER_SAMPLER5, true );	// Noise map
			pShaderShadow->EnableTexture( SHADER_SAMPLER6, true );	// Flashlight cookie
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER6, true );
			userDataSize = 4; // tangent S
		}

		// Always enable, since flat normal will be bound
		pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );		// Normal map
		userDataSize = 4; // tangent S
		pShaderShadow->EnableTexture( SHADER_SAMPLER5, true );		// Normalizing cube map

		if ( bHasBaseTextureWrinkle || bHasBumpWrinkle )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER11, true );	// Normal compression map
			pShaderShadow->EnableTexture( SHADER_SAMPLER12, true );	// Normal expansion map
		}

		if ( hasDetailTexture )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER13, true );
			if ( nDetailBlendMode != 0 ) //Not Mod2X
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER13, true );
		}

		if ( bHasSelfIllum )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER14, true );
		}

		if( bHasVertexColor || bHasVertexAlpha )
		{
			flags |= VERTEX_COLOR;
		}

		pShaderShadow->EnableSRGBWrite( true );
		
		// texcoord0 : base texcoord, texcoord2 : decal hw morph delta
		int pTexCoordDim[3] = { 2, 0, 3 };
		int nTexCoordCount = 1;

#ifndef _X360
		// Special morphed decal information 
		if ( bIsDecal && g_pHardwareConfig->HasFastVertexTextures() )
		{
			nTexCoordCount = 3;
		}
#endif

		// This shader supports compressed vertices, so OR in that flag:
		flags |= VERTEX_FORMAT_COMPRESSED;

		pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, pTexCoordDim, userDataSize );


#ifndef _X360
		if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
		{
			bool bUseStaticControlFlow = g_pHardwareConfig->SupportsStaticControlFlow();

			DECLARE_STATIC_VERTEX_SHADER( skin_vs20 );
			SET_STATIC_VERTEX_SHADER_COMBO( USE_STATIC_CONTROL_FLOW, bUseStaticControlFlow );
			SET_STATIC_VERTEX_SHADER( skin_vs20 );

			// Assume we're only going to get in here if we support 2b
			DECLARE_STATIC_PIXEL_SHADER( skin_ps20b );
			SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT, bHasFlashlight );
			SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM,  bHasSelfIllum && !bHasFlashlight );
			SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUMFRESNEL,  bHasSelfIllumFresnel && !bHasFlashlight );
			SET_STATIC_PIXEL_SHADER_COMBO( LIGHTWARPTEXTURE, bHasDiffuseWarp && bHasPhong );
			SET_STATIC_PIXEL_SHADER_COMBO( PHONGWARPTEXTURE, bHasPhongWarp && bHasPhong );
			SET_STATIC_PIXEL_SHADER_COMBO( WRINKLEMAP, bHasBaseTextureWrinkle || bHasBumpWrinkle );
			SET_STATIC_PIXEL_SHADER_COMBO( DETAILTEXTURE,  hasDetailTexture );
			SET_STATIC_PIXEL_SHADER_COMBO( DETAIL_BLEND_MODE, nDetailBlendMode );
			SET_STATIC_PIXEL_SHADER_COMBO( RIMLIGHT, bHasRimLight );
			SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP, bHasEnvmap );
			SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHTDEPTHFILTERMODE, nShadowFilterMode );
			SET_STATIC_PIXEL_SHADER_COMBO( CONVERT_TO_SRGB, 0 );
			SET_STATIC_PIXEL_SHADER_COMBO( FASTPATH_NOBUMP, pContextData->m_bFastPath );
			SET_STATIC_PIXEL_SHADER_COMBO( BLENDTINTBYBASEALPHA, bBlendTintByBaseAlpha );
			SET_STATIC_PIXEL_SHADER( skin_ps20b );
		}
#ifndef _X360
		else
		{
			// The vertex shader uses the vertex id stream
			SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );

			DECLARE_STATIC_VERTEX_SHADER( skin_vs30 );
			SET_STATIC_VERTEX_SHADER_COMBO( DECAL, bIsDecal );
			SET_STATIC_VERTEX_SHADER( skin_vs30 );

			DECLARE_STATIC_PIXEL_SHADER( skin_ps30 );
			SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT, bHasFlashlight );
			SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM,  bHasSelfIllum && !bHasFlashlight );
			SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUMFRESNEL,  bHasSelfIllumFresnel && !bHasFlashlight );
			SET_STATIC_PIXEL_SHADER_COMBO( LIGHTWARPTEXTURE, bHasDiffuseWarp && bHasPhong );
			SET_STATIC_PIXEL_SHADER_COMBO( PHONGWARPTEXTURE, bHasPhongWarp && bHasPhong );
			SET_STATIC_PIXEL_SHADER_COMBO( WRINKLEMAP, bHasBaseTextureWrinkle || bHasBumpWrinkle );
			SET_STATIC_PIXEL_SHADER_COMBO( DETAILTEXTURE,  hasDetailTexture );
			SET_STATIC_PIXEL_SHADER_COMBO( DETAIL_BLEND_MODE, nDetailBlendMode );
			SET_STATIC_PIXEL_SHADER_COMBO( RIMLIGHT, bHasRimLight );
			SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP, bHasEnvmap );
			SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHTDEPTHFILTERMODE, nShadowFilterMode );
			SET_STATIC_PIXEL_SHADER_COMBO( CONVERT_TO_SRGB, 0 );
			SET_STATIC_PIXEL_SHADER_COMBO( FASTPATH_NOBUMP, pContextData->m_bFastPath );
			SET_STATIC_PIXEL_SHADER_COMBO( BLENDTINTBYBASEALPHA, bBlendTintByBaseAlpha );
			SET_STATIC_PIXEL_SHADER( skin_ps30 );
		}
#endif

		if( bHasFlashlight )
		{
			pShader->FogToBlack();
		}
		else
		{
			pShader->DefaultFog();
		}

		// HACK HACK HACK - enable alpha writes all the time so that we have them for underwater stuff
		pShaderShadow->EnableAlphaWrites( bFullyOpaque );
	}
	else // not snapshotting -- begin dynamic state
	{
		bool bLightingOnly = mat_fullbright.GetInt() == 2 && !IS_FLAG_SET( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		bool bHasEnvmap = !bHasFlashlight && params[info.m_nEnvmap]->IsTexture();

		if( bHasBaseTexture )
		{
			pShader->BindTexture( SHADER_SAMPLER0, info.m_nBaseTexture, info.m_nBaseTextureFrame );
		}
		else
		{
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_WHITE );
		}

		if ( bHasBaseTextureWrinkle )
		{
			pShader->BindTexture( SHADER_SAMPLER9, info.m_nWrinkle, info.m_nBaseTextureFrame );
			pShader->BindTexture( SHADER_SAMPLER10, info.m_nStretch, info.m_nBaseTextureFrame );
		}
		else if ( bHasBumpWrinkle )
		{
			pShader->BindTexture( SHADER_SAMPLER9, info.m_nBaseTexture, info.m_nBaseTextureFrame );
			pShader->BindTexture( SHADER_SAMPLER10, info.m_nBaseTexture, info.m_nBaseTextureFrame );
		}

		if( bHasDiffuseWarp && bHasPhong )
		{
			if ( r_lightwarpidentity.GetBool() )
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER2, TEXTURE_IDENTITY_LIGHTWARP );
			}
			else
			{
				pShader->BindTexture( SHADER_SAMPLER2, info.m_nDiffuseWarpTexture );
			}
		}

		if( bHasPhongWarp )
		{
			pShader->BindTexture( SHADER_SAMPLER1, info.m_nPhongWarpTexture );
		}

		if( bHasSpecularExponentTexture && bHasPhong )
		{
			pShader->BindTexture( SHADER_SAMPLER7, info.m_nPhongExponentTexture );
		}
		else
		{
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER7, TEXTURE_WHITE );
		}

		if( !g_pConfig->m_bFastNoBump )
		{
			if( bHasBump )
				pShader->BindTexture( SHADER_SAMPLER3, info.m_nBumpmap, info.m_nBumpFrame );
			else
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER3, TEXTURE_NORMALMAP_FLAT );

			if ( bHasBumpWrinkle )
			{
				pShader->BindTexture( SHADER_SAMPLER11, info.m_nNormalWrinkle, info.m_nBumpFrame );
				pShader->BindTexture( SHADER_SAMPLER12, info.m_nNormalStretch, info.m_nBumpFrame );
			}
			else if ( bHasBaseTextureWrinkle )
			{
				pShader->BindTexture( SHADER_SAMPLER11, info.m_nBumpmap, info.m_nBumpFrame );
				pShader->BindTexture( SHADER_SAMPLER12, info.m_nBumpmap, info.m_nBumpFrame );
			}
		}
		else
		{
			if( bHasBump )
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER3, TEXTURE_NORMALMAP_FLAT );
			}
			if ( bHasBaseTextureWrinkle || bHasBumpWrinkle )
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER11, TEXTURE_NORMALMAP_FLAT );
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER12, TEXTURE_NORMALMAP_FLAT );
			}
		}

		if ( hasDetailTexture )
		{
			pShader->BindTexture( SHADER_SAMPLER13, info.m_nDetail, info.m_nDetailFrame );
		}

		if ( bHasSelfIllum )
		{
			if ( bHasSelfIllumMask )												// Separate texture for self illum?
			{
				pShader->BindTexture( SHADER_SAMPLER14, info.m_nSelfIllumMask );	// Bind it
			}
			else																	// else
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER14, TEXTURE_BLACK );	// Bind dummy
			}
		}

		LightState_t lightState = { 0, false, false };
		bool bFlashlightShadows = false;
		if( bHasFlashlight )
		{
			Assert( info.m_nFlashlightTexture >= 0 && info.m_nFlashlightTextureFrame >= 0 );
			pShader->BindTexture( SHADER_SAMPLER6, info.m_nFlashlightTexture, info.m_nFlashlightTextureFrame );
			VMatrix worldToTexture;
			ITexture *pFlashlightDepthTexture;
			FlashlightState_t state = pShaderAPI->GetFlashlightStateEx( worldToTexture, &pFlashlightDepthTexture );
			bFlashlightShadows = state.m_bEnableShadows && ( pFlashlightDepthTexture != NULL );

			SetFlashLightColorFromState( state, pShaderAPI, PSREG_FLASHLIGHT_COLOR );

			if( pFlashlightDepthTexture && g_pConfig->ShadowDepthTexture() && state.m_bEnableShadows )
			{
				pShader->BindTexture( SHADER_SAMPLER4, pFlashlightDepthTexture, 0 );
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER5, TEXTURE_SHADOW_NOISE_2D );
			}
		}
		else // no flashlight
		{
			if ( bHasEnvmap	)
			{
				pShader->BindTexture( SHADER_SAMPLER8, info.m_nEnvmap, info.m_nEnvmapFrame );
			}

			pShaderAPI->GetDX9LightState( &lightState );
		}

		MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();
		int fogIndex = ( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z ) ? 1 : 0;
		int numBones = pShaderAPI->GetCurrentNumBones();

		// don't have an easy way to get this through to GLM, so just print it old school
		//printf("\n-D- DrawSkin_DX9_Internal numBones is %d", numBones );
		
		bool bWriteDepthToAlpha = false;
		bool bWriteWaterFogToAlpha = false;
		if( bFullyOpaque ) 
		{
			bWriteDepthToAlpha = pShaderAPI->ShouldWriteDepthToDestAlpha();
			bWriteWaterFogToAlpha = (fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z);
			AssertMsg( !(bWriteDepthToAlpha && bWriteWaterFogToAlpha), "Can't write two values to alpha at the same time." );
		}

#ifndef _X360
		if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
		{
			bool bUseStaticControlFlow = g_pHardwareConfig->SupportsStaticControlFlow();

			DECLARE_DYNAMIC_VERTEX_SHADER( skin_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, fogIndex );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, numBones > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( LIGHTING_PREVIEW, pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_ENABLE_FIXED_LIGHTING)!=0);
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( NUM_LIGHTS, bUseStaticControlFlow ? 0 : lightState.m_nNumLights );
			SET_DYNAMIC_VERTEX_SHADER( skin_vs20 );

			DECLARE_DYNAMIC_PIXEL_SHADER( skin_ps20b );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LIGHTS, lightState.m_nNumLights );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITEWATERFOGTODESTALPHA, bWriteWaterFogToAlpha );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITE_DEPTH_TO_DESTALPHA, bWriteDepthToAlpha );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTSHADOWS, bFlashlightShadows );
			SET_DYNAMIC_PIXEL_SHADER( skin_ps20b );
		}
#ifndef _X360
		else
		{
			pShader->SetHWMorphVertexShaderState( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, VERTEX_SHADER_SHADER_SPECIFIC_CONST_7, SHADER_VERTEXTEXTURE_SAMPLER0 );

			DECLARE_DYNAMIC_VERTEX_SHADER( skin_vs30 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, fogIndex );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, numBones > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( LIGHTING_PREVIEW, pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_ENABLE_FIXED_LIGHTING)!=0);
			SET_DYNAMIC_VERTEX_SHADER_COMBO( MORPHING, pShaderAPI->IsHWMorphingEnabled() );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER( skin_vs30 );

			DECLARE_DYNAMIC_PIXEL_SHADER( skin_ps30 );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LIGHTS,  lightState.m_nNumLights );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITEWATERFOGTODESTALPHA, bWriteWaterFogToAlpha );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITE_DEPTH_TO_DESTALPHA, bWriteDepthToAlpha );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTSHADOWS, bFlashlightShadows );
			SET_DYNAMIC_PIXEL_SHADER( skin_ps30 );

			bool bUnusedTexCoords[3] = { false, false, !pShaderAPI->IsHWMorphingEnabled() || !bIsDecal };
			pShaderAPI->MarkUnusedVertexFields( 0, 3, bUnusedTexCoords );
		}
#endif

		pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, info.m_nBaseTextureTransform );

		if( bHasBump )
		{
			pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, info.m_nBumpTransform );
		}

		if ( hasDetailTexture )
		{
			if ( IS_PARAM_DEFINED( info.m_nDetailTextureTransform ) )
				pShader->SetVertexShaderTextureScaledTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4,
																info.m_nDetailTextureTransform, 
																info.m_nDetailScale );
			else
				pShader->SetVertexShaderTextureScaledTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4,
																info.m_nBaseTextureTransform, 
																info.m_nDetailScale );
		}

		pShader->SetModulationPixelShaderDynamicState_LinearColorSpace( 1 );
		pShader->SetPixelShaderConstant_W( PSREG_SELFILLUMTINT, info.m_nSelfIllumTint, fBlendFactor );
		bool bInvertPhongMask = ( info.m_nInvertPhongMask != -1 ) && ( params[info.m_nInvertPhongMask]->GetIntValue() != 0 );
		float fInvertPhongMask = bInvertPhongMask ? 1 : 0;

		bool bHasBaseAlphaPhongMask = (info.m_nBaseMapAlphaPhongMask != -1) && ( params[info.m_nBaseMapAlphaPhongMask]->GetIntValue() != 0 );
		float fHasBaseAlphaPhongMask = bHasBaseAlphaPhongMask ? 1 : 0;
		// Controls for lerp-style paths through shader code
		float vShaderControls[4] = { fHasBaseAlphaPhongMask, 0.0f/*unused*/, flTintReplacementAmount, fInvertPhongMask };
		pShaderAPI->SetPixelShaderConstant( PSREG_CONSTANT_27, vShaderControls, 1 );

		if ( hasDetailTexture )
		{
#if 0														// needs constant change
			if ( info.m_nDetailTint  != -1 )
				pShader->SetPixelShaderConstantGammaToLinear( 10, info.m_nDetailTint );
			else
			{
				float boring_tint[4]={1,1,1,1};
				pShaderAPI->SetPixelShaderConstant( 10, boring_tint, 1 );
			}
#endif
		}

		if ( bHasSelfIllumFresnel && !bHasFlashlight )
		{
			float vConstScaleBiasExp[4] = { 1.0f, 0.0f, 1.0f, 0.0f };
			float flMin = IS_PARAM_DEFINED( info.m_nSelfIllumFresnelMinMaxExp ) ? params[info.m_nSelfIllumFresnelMinMaxExp]->GetVecValue()[0] : 0.0f;
			float flMax = IS_PARAM_DEFINED( info.m_nSelfIllumFresnelMinMaxExp ) ? params[info.m_nSelfIllumFresnelMinMaxExp]->GetVecValue()[1] : 1.0f;
			float flExp = IS_PARAM_DEFINED( info.m_nSelfIllumFresnelMinMaxExp ) ? params[info.m_nSelfIllumFresnelMinMaxExp]->GetVecValue()[2] : 1.0f;

			vConstScaleBiasExp[1] = ( flMax != 0.0f ) ? ( flMin / flMax ) : 0.0f; // Bias
			vConstScaleBiasExp[0] = 1.0f - vConstScaleBiasExp[1]; // Scale
			vConstScaleBiasExp[2] = flExp; // Exp
			vConstScaleBiasExp[3] = flMax; // Brightness

			pShaderAPI->SetPixelShaderConstant( PSREG_SELFILLUM_SCALE_BIAS_EXP, vConstScaleBiasExp, 1 );
		}

		pShader->SetAmbientCubeDynamicStateVertexShader();

		if( !bHasFlashlight )
		{
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER5, TEXTURE_NORMALIZATION_CUBEMAP_SIGNED );

			// Setting .x to 1 means to apply Fresnel to env map.  Setting w to 1 means use separate selfillummask
			float vEnvMapFresnel_SelfIllumMask[4] = {0.0f, 0.0f, 0.0f, 0.0f};
			vEnvMapFresnel_SelfIllumMask[3] = bHasSelfIllumMask ? 1.0f : 0.0f;

			if( bHasEnvmap )
			{
				float vEnvMapTint_MaskControl[4] = {1.0f, 1.0f, 1.0f, 0.0f};

				// If we have a tint, grab it
				if ( (info.m_nEnvmapTint != -1) && params[info.m_nEnvmapTint]->IsDefined() )
					params[info.m_nEnvmapTint]->GetVecValue(vEnvMapTint_MaskControl, 3);

				// Set control for source of env map mask (normal alpha or base alpha)
				vEnvMapTint_MaskControl[3] = bHasNormalMapAlphaEnvmapMask ? 1.0f : 0.0f;

				if ( (info.m_nEnvmapFresnel != -1) && params[info.m_nEnvmapFresnel]->IsDefined() )
					vEnvMapFresnel_SelfIllumMask[0] = params[info.m_nEnvmapFresnel]->GetFloatValue();

				// Handle mat_fullbright 2 (diffuse lighting only with 50% gamma space basetexture)
				if( bLightingOnly )
				{
					vEnvMapTint_MaskControl[0] = vEnvMapTint_MaskControl[1] = vEnvMapTint_MaskControl[2] = 0.0f;
				}

				pShaderAPI->SetPixelShaderConstant( PSREG_ENVMAP_TINT__SHADOW_TWEAKS, vEnvMapTint_MaskControl, 1 );
			}

			pShaderAPI->SetPixelShaderConstant( PSREG_ENVMAP_FRESNEL__SELFILLUMMASK, vEnvMapFresnel_SelfIllumMask, 1 );
		}

		pShaderAPI->SetPixelShaderStateAmbientLightCube( PSREG_AMBIENT_CUBE, !lightState.m_bAmbientLight );	// Force to black if not bAmbientLight
		pShaderAPI->CommitPixelShaderLighting( PSREG_LIGHT_INFO_ARRAY );

		// Pack Phong exponent in with the eye position
		float vEyePos_SpecExponent[4], vFresnelRanges_SpecBoost[4] = {1, 0.5, 1, 1}, vRimBoost[4] = {1, 1, 1, 1};
		float vSpecularTint[4] = {1, 1, 1, 4};
		pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );

		// Use the alpha channel of the normal map for the exponent by default
		vEyePos_SpecExponent[3] = -1.f;
		if ( (info.m_nPhongExponent != -1) && params[info.m_nPhongExponent]->IsDefined() )
		{
			float fValue = params[info.m_nPhongExponent]->GetFloatValue();
			if ( fValue > 0.f )
			{
				// Nonzero value in material overrides map channel
				vEyePos_SpecExponent[3] = fValue;
			}
		}

		// Get the tint parameter
		if ( (info.m_nPhongTint != -1) && params[info.m_nPhongTint]->IsDefined() )
		{
			params[info.m_nPhongTint]->GetVecValue(vSpecularTint, 3);
		}

		// Get the rim light power (goes in w of Phong tint)
		if ( bHasRimLight && (info.m_nRimLightPower != -1) && params[info.m_nRimLightPower]->IsDefined() )
		{
			vSpecularTint[3] = params[info.m_nRimLightPower]->GetFloatValue();
			vSpecularTint[3] = max(vSpecularTint[3], 1.0f);	// Make sure this is at least 1
		}

		// Get the rim boost (goes in w of flashlight position)
		if ( bHasRimLight && (info.m_nRimLightBoost != -1) && params[info.m_nRimLightBoost]->IsDefined() )
		{
			vRimBoost[3] = params[info.m_nRimLightBoost]->GetFloatValue();
		}

		if ( !bHasFlashlight )
		{
			float vRimMaskControl[4] = {0, 0, 0, 0}; // Only x is relevant in shader code
			vRimMaskControl[0] = bHasRimMaskMap ? params[info.m_nRimMask]->GetFloatValue() : 0.0f;

			// Rim mask...if this is true, use alpha channel of spec exponent texture to mask the rim term
			pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_ATTENUATION, vRimMaskControl, 1 );
		}

		// If it's all zeros, there was no constant tint in the vmt
		if ( (vSpecularTint[0] == 0.0f) && (vSpecularTint[1] == 0.0f) && (vSpecularTint[2] == 0.0f) )
		{
			if ( bHasPhongTintMap )				// If we have a map to use, tell the shader
			{
				vSpecularTint[0] = -1;
			}
			else								// Otherwise, just tint with white
			{
				vSpecularTint[0] = 1.0f;
				vSpecularTint[1] = 1.0f;
				vSpecularTint[2] = 1.0f;
			}
		}

		// handle mat_fullbright 2 (diffuse lighting only)
		if( bLightingOnly )
		{
			// BASETEXTURE
			if( bHasSelfIllum && !bHasFlashlight )
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_GREY_ALPHA_ZERO );
			}
			else
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_GREY );
			}

			// DETAILTEXTURE
			if ( hasDetailTexture )
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER13, TEXTURE_GREY );
			}

			// turn off specularity
			vSpecularTint[0] = vSpecularTint[1] = vSpecularTint[2] = 0.0f;
		}

		if ( (info.m_nPhongFresnelRanges != -1) && params[info.m_nPhongFresnelRanges]->IsDefined() )
		{
			params[info.m_nPhongFresnelRanges]->GetVecValue( vFresnelRanges_SpecBoost, 3 );	// Grab optional Fresnel range parameters
			// Change fresnel range encoding from (min, mid, max) to ((mid-min)*2, mid, (max-mid)*2)
			vFresnelRanges_SpecBoost[0] = (vFresnelRanges_SpecBoost[1] - vFresnelRanges_SpecBoost[0]) * 2;
			vFresnelRanges_SpecBoost[2] = (vFresnelRanges_SpecBoost[2] - vFresnelRanges_SpecBoost[1]) * 2;
		}

		if ( (info.m_nPhongBoost != -1 ) && params[info.m_nPhongBoost]->IsDefined())		// Grab optional Phong boost param
			vFresnelRanges_SpecBoost[3] = params[info.m_nPhongBoost]->GetFloatValue();
		else
			vFresnelRanges_SpecBoost[3] = 1.0f;

		pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1 );
		pShaderAPI->SetPixelShaderConstant( PSREG_FRESNEL_SPEC_PARAMS, vFresnelRanges_SpecBoost, 1 );
		
		pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_POSITION_RIM_BOOST, vRimBoost, 1 );	// Rim boost in w on non-flashlight pass

		pShaderAPI->SetPixelShaderConstant( PSREG_SPEC_RIM_PARAMS, vSpecularTint, 1 );
		pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );

		// flashlightfixme: put this in common code.
		if( bHasFlashlight )
		{
			VMatrix worldToTexture;
			float atten[4], pos[4], tweaks[4];

			const FlashlightState_t &flashlightState = pShaderAPI->GetFlashlightState( worldToTexture );
			SetFlashLightColorFromState( flashlightState, pShaderAPI, PSREG_FLASHLIGHT_COLOR );

			pShader->BindTexture( SHADER_SAMPLER6, flashlightState.m_pSpotlightTexture, flashlightState.m_nSpotlightTextureFrame );

			atten[0] = flashlightState.m_fConstantAtten;		// Set the flashlight attenuation factors
			atten[1] = flashlightState.m_fLinearAtten;
			atten[2] = flashlightState.m_fQuadraticAtten;
			atten[3] = flashlightState.m_FarZ;
			pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_ATTENUATION, atten, 1 );

			pos[0] = flashlightState.m_vecLightOrigin[0];		// Set the flashlight origin
			pos[1] = flashlightState.m_vecLightOrigin[1];
			pos[2] = flashlightState.m_vecLightOrigin[2];
			pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_POSITION_RIM_BOOST, pos, 1 );	// steps on rim boost

			pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_TO_WORLD_TEXTURE, worldToTexture.Base(), 4 );

			// Tweaks associated with a given flashlight
			tweaks[0] = ShadowFilterFromState( flashlightState );
			tweaks[1] = ShadowAttenFromState( flashlightState );
			pShader->HashShadow2DJitter( flashlightState.m_flShadowJitterSeed, &tweaks[2], &tweaks[3] );
			pShaderAPI->SetPixelShaderConstant( PSREG_ENVMAP_TINT__SHADOW_TWEAKS, tweaks, 1 );

			// Dimensions of screen, used for screen-space noise map sampling
			float vScreenScale[4] = {1280.0f / 32.0f, 720.0f / 32.0f, 0, 0};
			int nWidth, nHeight;
			pShaderAPI->GetBackBufferDimensions( nWidth, nHeight );
			vScreenScale[0] = (float) nWidth  / 32.0f;
			vScreenScale[1] = (float) nHeight / 32.0f;
			pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_SCREEN_SCALE, vScreenScale, 1 );

			if ( IsX360() )
			{
				pShaderAPI->SetBooleanPixelShaderConstant( 0, &flashlightState.m_nShadowQuality, 1 );
			}
		}
	}
	pShader->Draw();
}


//-----------------------------------------------------------------------------
// Draws the shader
//-----------------------------------------------------------------------------
extern ConVar r_flashlight_version2;
void DrawSkin_DX9( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow,
				   VertexLitGeneric_DX9_Vars_t &info, VertexCompressionType_t vertexCompression,
				   CBasePerMaterialContextData **pContextDataPtr )

{
	bool bHasFlashlight = pShader->UsingFlashlight( params );
	if ( bHasFlashlight && ( IsX360() || r_flashlight_version2.GetInt() ) )
	{
		DrawSkin_DX9_Internal( pShader, params, pShaderAPI,
			pShaderShadow, false, info, vertexCompression, pContextDataPtr++ );
		if ( pShaderShadow )
		{
			pShader->SetInitialShadowState( );
		}
	}
	DrawSkin_DX9_Internal( pShader, params, pShaderAPI,
		pShaderShadow, bHasFlashlight, info, vertexCompression, pContextDataPtr );
}
