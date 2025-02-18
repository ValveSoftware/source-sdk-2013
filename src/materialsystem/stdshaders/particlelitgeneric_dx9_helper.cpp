//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#if 0

#include "particlelitgeneric_dx9_helper.h"
#include "BaseVSShader.h"
#include "particlelit_generic_vs30.inc"
#include "particlelit_generic_ps30.inc"
#include "convar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Initialize shader parameters
//-----------------------------------------------------------------------------
void InitParamsParticleLitGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, ParticleLitGeneric_DX9_Vars_t &info )
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


	SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );
	CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
	CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
	
	if( (info.m_nBumpFrame != -1) && !params[info.m_nBumpFrame]->IsDefined() )
	{
		params[info.m_nBumpFrame]->SetIntValue( 0 );
	}

	if( (info.m_nBumpmap != -1) && g_pConfig->UseBumpmapping() && params[info.m_nBumpmap]->IsDefined() )
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
	}
	else
	{
		CLEAR_FLAGS( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
	}
}


//-----------------------------------------------------------------------------
// Initialize shader
//-----------------------------------------------------------------------------
void InitParticleLitGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, ParticleLitGeneric_DX9_Vars_t &info )
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
	}
	// No alpha channel in any of the textures? No self illum or envmapmask
	if ( !bIsBaseTextureTranslucent )
	{
		CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
		CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
	}

	if ( g_pConfig->UseBumpmapping() )
	{
		if ( (info.m_nBumpmap != -1) && params[info.m_nBumpmap]->IsDefined() )
		{
			pShader->LoadBumpMap( info.m_nBumpmap );
			SET_FLAGS2( MATERIAL_VAR2_DIFFUSE_BUMPMAPPED_MODEL );
		}
	}

	// Don't alpha test if the alpha channel is used for other purposes
	if ( IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
	{
		CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );
	}
}


//-----------------------------------------------------------------------------
// Draws the shader
//-----------------------------------------------------------------------------
void DrawParticleLitGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, 
	IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, ParticleLitGeneric_DX9_Vars_t &info )
{
	bool hasBaseTexture = params[info.m_nBaseTexture]->IsTexture();
	bool hasBump = (info.m_nBumpmap != -1) && params[info.m_nBumpmap]->IsTexture();
	bool hasVertexColor = IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR );
	bool hasVertexAlpha = IS_FLAG_SET( MATERIAL_VAR_VERTEXALPHA );
	bool bIsAlphaTested = IS_FLAG_SET( MATERIAL_VAR_ALPHATEST ) != 0;
	bool bNoFog = IS_FLAG_SET( MATERIAL_VAR_NOFOG );

	HDRType_t hdrType = g_pHardwareConfig->GetHDRType();

	BlendType_t blendType = pShader->EvaluateBlendRequirements( info.m_nBaseTexture, true );	
	if( pShader->IsSnapshotting() )
	{
		// look at color and alphamod stuff.
		// Unlit generic never uses the flashlight
		bool hasFlashlight = CShader_IsFlag2Set( params, MATERIAL_VAR2_USE_FLASHLIGHT );
		bool bHalfLambert = IS_FLAG_SET( MATERIAL_VAR_HALFLAMBERT );

		// Alpha test: FIXME: shouldn't this be handled in CBaseVSShader::SetInitialShadowState
		pShaderShadow->EnableAlphaTest( bIsAlphaTested );

		if( info.m_nAlphaTestReference != -1 && params[info.m_nAlphaTestReference]->GetFloatValue() > 0.0f )
		{
			pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GEQUAL, params[info.m_nAlphaTestReference]->GetFloatValue() );
		}

		if( hasFlashlight )
		{
			pShader->SetAdditiveBlendingShadowState( info.m_nBaseTexture, true );
			if( bIsAlphaTested )
			{
				// disable alpha test and use the zfunc zequals since alpha isn't guaranteed to 
				// be the same on both the regular pass and the flashlight pass.
				pShaderShadow->EnableAlphaTest( false );
				pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_EQUAL );
			}
			pShaderShadow->EnableBlending( true );
			pShaderShadow->EnableDepthWrites( false );
		}
		else
		{
			pShader->SetDefaultBlendingShadowState( info.m_nBaseTexture, true );
		}
		
		unsigned int flags = VERTEX_POSITION;

		int userDataSize = 0;
		if( hasBaseTexture )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );
		}
		if( hasFlashlight )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER7, true );
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER7, true );
			userDataSize = 4; // tangent S
		}
		if( hasBump )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
			userDataSize = 4; // tangent S
			// Normalizing cube map
			pShaderShadow->EnableTexture( SHADER_SAMPLER5, true );
		}
		if( hasVertexColor || hasVertexAlpha )
		{
			flags |= VERTEX_COLOR;
		}

		pShaderShadow->EnableSRGBWrite( true );
		
		// texcoord0 : base texcoord
		int pTexCoordCount[2] = { 2, 3 };
		pShaderShadow->VertexShaderVertexFormat( 
			flags, 2, pTexCoordCount, 0, userDataSize );

		DECLARE_STATIC_VERTEX_SHADER( particlelit_generic_vs30 );
		SET_STATIC_VERTEX_SHADER_COMBO( HALFLAMBERT,  bHalfLambert);
		SET_STATIC_VERTEX_SHADER( particlelit_generic_vs30 );
		
		DECLARE_STATIC_PIXEL_SHADER( particlelit_generic_ps30 );
		SET_STATIC_PIXEL_SHADER_COMBO( HALFLAMBERT,  bHalfLambert);
//		SET_STATIC_PIXEL_SHADER_COMBO( FLASHLIGHT,  hasFlashlight );
		SET_STATIC_PIXEL_SHADER_COMBO( HDRTYPE,  hdrType );
		SET_STATIC_PIXEL_SHADER( particlelit_generic_ps30 );

		if( hasFlashlight )
		{
			pShader->FogToBlack();
		}
		else
		{
			pShader->DefaultFog();
		}

		// HACK HACK HACK - enable alpha writes all the time so that we have them for
		// underwater stuff
		if( blendType != BT_BLENDADD && blendType != BT_BLEND && !bIsAlphaTested )
		{
			pShaderShadow->EnableAlphaWrites( true );
		}
	}
	else
	{
		bool hasFlashlight = pShaderAPI->InFlashlightMode();

		if( hasBaseTexture )
		{
			pShader->BindTexture( SHADER_SAMPLER0, info.m_nBaseTexture, info.m_nBaseTextureFrame );
		}
		if( !g_pConfig->m_bFastNoBump )
		{
			if( hasBump )
			{
				pShader->BindTexture( SHADER_SAMPLER3, info.m_nBumpmap, info.m_nBumpFrame );
			}
		}
		else
		{
			if( hasBump )
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER3, TEXTURE_NORMALMAP_FLAT );
			}
		}
		if( hasFlashlight )
		{
			Assert( info.m_nFlashlightTexture >= 0 && info.m_nFlashlightTextureFrame >= 0 );
			pShader->BindTexture( SHADER_SAMPLER7, info.m_nFlashlightTexture, info.m_nFlashlightTextureFrame );
		}

		LightState_t lightState = { 0, false, false };
		if( !hasFlashlight )
			pShaderAPI->GetDX9LightState( &lightState );

		MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();
//		int fogIndex = ( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z ) ? 1 : 0;

		DECLARE_DYNAMIC_VERTEX_SHADER( particlelit_generic_vs30 );
		SET_DYNAMIC_VERTEX_SHADER( particlelit_generic_vs30 );

		DECLARE_DYNAMIC_PIXEL_SHADER( particlelit_generic_ps30 );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LIGHTS, lightState.m_nNumLights );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( AMBIENT_LIGHT, lightState.m_bAmbientLight ? 1 : 0 );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( STATIC_LIGHT,  lightState.m_bStaticLight  ? 1 : 0 );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITEWATERFOGTODESTALPHA,  fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z &&
			blendType != BT_BLENDADD && blendType != BT_BLEND && !bIsAlphaTested );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( HDRENABLED,  pShader->IsHDREnabled() );
		SET_DYNAMIC_PIXEL_SHADER( particlelit_generic_ps30 );

		pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, info.m_nBaseTextureTransform );
		if( hasBump )
		{
			pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, info.m_nBumpTransform );
		}
		if( hasBump )
		{
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER5, TEXTURE_NORMALIZATION_CUBEMAP_SIGNED );
			pShaderAPI->SetPixelShaderStateAmbientLightCube( 5 );
			pShaderAPI->CommitPixelShaderLighting( 13 );
		}

		float eyePos[4];
		pShaderAPI->GetWorldSpaceCameraPosition( eyePos );
		pShaderAPI->SetPixelShaderConstant( 20, eyePos, 1 );
		pShaderAPI->SetPixelShaderFogParams( 21 );

		// flashlightfixme: put this in common code.
		if( hasFlashlight )
		{
			VMatrix worldToTexture;
			const FlashlightState_t &flashlightState = pShaderAPI->GetFlashlightState( worldToTexture );

			// Set the flashlight attenuation factors
			float atten[4];
			atten[0] = flashlightState.m_fConstantAtten;
			atten[1] = flashlightState.m_fLinearAtten;
			atten[2] = flashlightState.m_fQuadraticAtten;
			atten[3] = flashlightState.m_FarZ;
			pShaderAPI->SetPixelShaderConstant( 22, atten, 1 );

			// Set the flashlight origin
			float pos[4];
			pos[0] = flashlightState.m_vecLightOrigin[0];
			pos[1] = flashlightState.m_vecLightOrigin[1];
			pos[2] = flashlightState.m_vecLightOrigin[2];
			pos[3] = 1.0f;
			pShaderAPI->SetPixelShaderConstant( 23, pos, 1 );

			pShaderAPI->SetPixelShaderConstant( 24, worldToTexture.Base(), 4 );
		}		
	}
	pShader->Draw();
}

// Commenting out this entire file
#endif
