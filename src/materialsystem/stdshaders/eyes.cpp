//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: eye renderer
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "eyes_dx8_dx9_helper.h"
#include "cloak_blended_pass_helper.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( eyes, Eyes_dx8 )

BEGIN_VS_SHADER( Eyes_dx8, 
			  "Help for Eyes" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM_OVERRIDE( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "models/alyx/eyeball_l", "iris texture", 0 )
		SHADER_PARAM( IRIS, SHADER_PARAM_TYPE_TEXTURE, "models/alyx/pupil_l", "iris texture" )
		SHADER_PARAM( IRISFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame for the iris texture" )
		SHADER_PARAM( GLINT, SHADER_PARAM_TYPE_TEXTURE, "models/humans/male/glint", "glint texture" )
		SHADER_PARAM( EYEORIGIN, SHADER_PARAM_TYPE_VEC3, "[0 0 0]", "origin for the eyes" )
		SHADER_PARAM( EYEUP, SHADER_PARAM_TYPE_VEC3, "[0 0 1]", "up vector for the eyes" )
		SHADER_PARAM( IRISU, SHADER_PARAM_TYPE_VEC4, "[0 1 0 0]", "U projection vector for the iris" )
		SHADER_PARAM( IRISV, SHADER_PARAM_TYPE_VEC4, "[0 0 1 0]", "V projection vector for the iris" )
		SHADER_PARAM( GLINTU, SHADER_PARAM_TYPE_VEC4, "[0 1 0 0]", "U projection vector for the glint" )
		SHADER_PARAM( GLINTV, SHADER_PARAM_TYPE_VEC4, "[0 0 1 0]", "V projection vector for the glint" )
		SHADER_PARAM( DILATION, SHADER_PARAM_TYPE_FLOAT, "0", "Iris dilation" )
		SHADER_PARAM( INTRO, SHADER_PARAM_TYPE_BOOL, "0", "is eyes in the ep1 intro" )
 	    SHADER_PARAM( ENTITYORIGIN, SHADER_PARAM_TYPE_VEC3,"0.0","center if the model in world space" )
 	    SHADER_PARAM( WARPPARAM, SHADER_PARAM_TYPE_FLOAT,"0.0","animation param between 0 and 1" )

		// Cloak Pass
		SHADER_PARAM( CLOAKPASSENABLED, SHADER_PARAM_TYPE_BOOL, "0", "Enables cloak render in a second pass" )
		SHADER_PARAM( CLOAKFACTOR, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )
		SHADER_PARAM( CLOAKCOLORTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Cloak color tint" )
		SHADER_PARAM( REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "2", "" )
	END_SHADER_PARAMS

	void SetupVars( Eyes_DX8_DX9_Vars_t &info )
	{
		info.m_nBaseTexture = BASETEXTURE;
		info.m_nFrame = FRAME;
		info.m_nIris = IRIS;
		info.m_nIrisFrame = IRISFRAME;
		info.m_nGlint = GLINT;
		info.m_nEyeOrigin = EYEORIGIN;
		info.m_nEyeUp = EYEUP;
		info.m_nIrisU = IRISU;
		info.m_nIrisV = IRISV;
		info.m_nGlintU = GLINTU;
		info.m_nGlintV = GLINTV;
		info.m_nDilation = DILATION;
		info.m_nIntro = INTRO;
		info.m_nEntityOrigin = ENTITYORIGIN;
		info.m_nWarpParam = WARPPARAM;
	}

	// Cloak Pass
	void SetupVarsCloakBlendedPass( CloakBlendedPassVars_t &info )
	{
		info.m_nCloakFactor = CLOAKFACTOR;
		info.m_nCloakColorTint = CLOAKCOLORTINT;
		info.m_nRefractAmount = REFRACTAMOUNT;
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

	SHADER_INIT_PARAMS()
	{
		Eyes_DX8_DX9_Vars_t info;
		SetupVars( info );
		InitParamsEyes_DX8_DX9( this, params, pMaterialName, info );

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
	}

	SHADER_FALLBACK
	{
		if ( IsPC() && g_pHardwareConfig->GetDXSupportLevel() < 80 )
			return "Eyes_dx6";

		return 0;
	}

	SHADER_INIT
	{
		Eyes_DX8_DX9_Vars_t info;
		SetupVars( info );
		InitEyes_DX8_DX9( this, params, info );

		// Cloak Pass
		if ( params[CLOAKPASSENABLED]->GetIntValue() )
		{
			CloakBlendedPassVars_t info;
			SetupVarsCloakBlendedPass( info );
			InitCloakBlendedPass( this, params, info );
		}
	}

	SHADER_DRAW
	{
		// Skip the standard rendering if cloak pass is fully opaque
		bool bDrawStandardPass = true;
		if ( params[CLOAKPASSENABLED]->GetIntValue() && ( pShaderShadow == NULL ) ) // && not snapshotting
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
			Eyes_DX8_DX9_Vars_t info;
			SetupVars( info );
			DrawEyes_DX8_DX9( false, this, params, pShaderAPI, pShaderShadow, info, vertexCompression );
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
	}
END_SHADER
