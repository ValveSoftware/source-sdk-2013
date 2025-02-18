//========= Copyright Valve Corporation, All rights reserved. ============//

#include "BaseVSShader.h"
#include "portal_refract_dx8_helper.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( PortalRefract, PortalRefract_dx8 )
BEGIN_VS_SHADER( PortalRefract_dx8, "PortalRefract_dx8" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( STAGE, SHADER_PARAM_TYPE_INTEGER, "0", "Stage of portal rendering (0, 1, 2)" )
		SHADER_PARAM( PORTALOPENAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0.0", "Portal open amount 0.0-1.0" )
		SHADER_PARAM( PORTALSTATIC, SHADER_PARAM_TYPE_FLOAT, "0.0", "Portal static amount 0.0-1.0" )
		SHADER_PARAM( PORTALMASKTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Mask texture" )
		SHADER_PARAM( TEXTURETRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "Texcoord transform" )
		SHADER_PARAM( PORTALCOLORTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Color texture" )
		SHADER_PARAM( PORTALCOLORSCALE, SHADER_PARAM_TYPE_FLOAT, "0.0", "Portal color scale" )
	END_SHADER_PARAMS

	void SetupVarsPortalRefract_DX8( PortalRefractVarsDX8_t &info )
	{
		info.m_nStage = STAGE;
		info.m_nPortalOpenAmount = PORTALOPENAMOUNT;
		info.m_nPortalStatic = PORTALSTATIC;
		info.m_nPortalMaskTexture = PORTALMASKTEXTURE;
		info.m_nTextureTransform = TEXTURETRANSFORM;
		info.m_nPortalColorTexture = PORTALCOLORTEXTURE;
		info.m_nPortalColorScale = PORTALCOLORSCALE;
	}

	bool NeedsPowerOfTwoFrameBufferTexture( IMaterialVar **params, bool bCheckSpecificToThisFrame ) const 
	{
		return false;
	}

	SHADER_INIT_PARAMS()
	{
		PortalRefractVarsDX8_t info;
		SetupVarsPortalRefract_DX8( info );
		InitParamsPortalRefract_DX8( this, params, pMaterialName, info );
	}

	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 80 )
		{
			return "Wireframe";
		}

		return 0;
	}

	SHADER_INIT
	{
		PortalRefractVarsDX8_t info;
		SetupVarsPortalRefract_DX8( info );
		InitPortalRefract_DX8( this, params, info );
	}

	SHADER_DRAW
	{
		// Skip drawing stage 0 in DX8
		bool bDraw = true;
		if ( params[STAGE]->GetIntValue() == 0 )
		{
			bDraw = false;
		}

		// If ( snapshotting ) or ( we need to draw this frame )
		if ( ( pShaderShadow != NULL ) || ( bDraw == true ) )
		{
			PortalRefractVarsDX8_t info;
			SetupVarsPortalRefract_DX8( info );
			DrawPortalRefract_DX8( this, params, pShaderAPI, pShaderShadow, info );
		}
		else // We're not snapshotting and we don't need to draw this frame
		{
			// Skip this pass!
			Draw( false );
		}
	}
END_SHADER
