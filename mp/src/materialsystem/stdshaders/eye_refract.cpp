//========= Copyright Valve Corporation, All rights reserved. ============//

#include "BaseVSShader.h"
#include "eye_refract_helper.h"
#include "cloak_blended_pass_helper.h"
#include "emissive_scroll_blended_pass_helper.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( EyeRefract, EyeRefract_dx9 )
BEGIN_VS_SHADER( EyeRefract_dx9, "Help for Eyes" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( IRIS, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "iris texture" )
		SHADER_PARAM( IRISFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame for the iris texture" )
		SHADER_PARAM( CORNEATEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "cornea texture" )
		SHADER_PARAM( AMBIENTOCCLTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "reflection texture" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )

		SHADER_PARAM( EYEORIGIN, SHADER_PARAM_TYPE_VEC3, "[0 0 0]", "origin for the eyes" )

		SHADER_PARAM( IRISU, SHADER_PARAM_TYPE_VEC4, "[0 1 0 0 ]", "U projection vector for the iris" )
		SHADER_PARAM( IRISV, SHADER_PARAM_TYPE_VEC4, "[0 0 1 0]", "V projection vector for the iris" )

		SHADER_PARAM( DILATION, SHADER_PARAM_TYPE_FLOAT, "0", "Pupil dilation (0 is none, 1 is maximal)" )
		SHADER_PARAM( GLOSSINESS, SHADER_PARAM_TYPE_FLOAT, "1", "Glossiness of eye (1 is default, 0 is not glossy at all)" )
		SHADER_PARAM( SPHERETEXKILLCOMBO, SHADER_PARAM_TYPE_BOOL, "1", "texkill pixels not on sphere" )
		SHADER_PARAM( RAYTRACESPHERE, SHADER_PARAM_TYPE_BOOL, "1", "Raytrace sphere" )
		SHADER_PARAM( PARALLAXSTRENGTH, SHADER_PARAM_TYPE_FLOAT, "1", "Parallax strength" )
		SHADER_PARAM( CORNEABUMPSTRENGTH, SHADER_PARAM_TYPE_FLOAT, "1", "Cornea strength" )

		SHADER_PARAM( AMBIENTOCCLCOLOR, SHADER_PARAM_TYPE_VEC3, "[1 1 1]", "Ambient occlusion color" )
		SHADER_PARAM( EYEBALLRADIUS, SHADER_PARAM_TYPE_FLOAT, "0", "Eyeball radius for ray casting" )

		SHADER_PARAM( INTRO, SHADER_PARAM_TYPE_BOOL, "0", "is eyes in the ep1 intro" )
 	    SHADER_PARAM( ENTITYORIGIN, SHADER_PARAM_TYPE_VEC3,"0.0","center if the model in world space" )
 	    SHADER_PARAM( WARPPARAM, SHADER_PARAM_TYPE_FLOAT,"0.0","animation param between 0 and 1" )

		SHADER_PARAM( LIGHTWARPTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "1D ramp texture for tinting scalar diffuse term" )

		// Cloak Pass
		SHADER_PARAM( CLOAKPASSENABLED, SHADER_PARAM_TYPE_BOOL, "0", "Enables cloak render in a second pass" )
		SHADER_PARAM( CLOAKFACTOR, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )
		SHADER_PARAM( CLOAKCOLORTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Cloak color tint" )
		SHADER_PARAM( REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "2", "" )

		// Emissive Scroll Pass
		SHADER_PARAM( EMISSIVEBLENDENABLED, SHADER_PARAM_TYPE_BOOL, "0", "Enable emissive blend pass" )
		SHADER_PARAM( EMISSIVEBLENDSCROLLVECTOR, SHADER_PARAM_TYPE_VEC2, "[0.11 0.124]", "Emissive scroll vec" )
		SHADER_PARAM( EMISSIVEBLENDSTRENGTH, SHADER_PARAM_TYPE_FLOAT, "1.0", "Emissive blend strength" )
		SHADER_PARAM( EMISSIVEBLENDTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "self-illumination map" )
		SHADER_PARAM( EMISSIVEBLENDTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
		SHADER_PARAM( EMISSIVEBLENDFLOWTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "flow map" )
	END_SHADER_PARAMS

	void SetupVarsEyeRefract( Eye_Refract_Vars_t &info )
	{
		info.m_nFrame = FRAME;
		info.m_nIris = IRIS;
		info.m_nIrisFrame = IRISFRAME;
		info.m_nEyeOrigin = EYEORIGIN;
		info.m_nIrisU = IRISU;
		info.m_nIrisV = IRISV;
		info.m_nDilation = DILATION;
		info.m_nGlossiness = GLOSSINESS;
		info.m_nIntro = INTRO;
		info.m_nEntityOrigin = ENTITYORIGIN;
		info.m_nWarpParam = WARPPARAM;
		info.m_nCorneaTexture = CORNEATEXTURE;
		info.m_nAmbientOcclTexture = AMBIENTOCCLTEXTURE;
		info.m_nEnvmap = ENVMAP;
		info.m_nSphereTexKillCombo = SPHERETEXKILLCOMBO;
		info.m_nRaytraceSphere = RAYTRACESPHERE;
		info.m_nParallaxStrength = PARALLAXSTRENGTH;
		info.m_nCorneaBumpStrength = CORNEABUMPSTRENGTH;
		info.m_nAmbientOcclColor = AMBIENTOCCLCOLOR;
		info.m_nEyeballRadius = EYEBALLRADIUS;
		info.m_nDiffuseWarpTexture = LIGHTWARPTEXTURE;
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

	// Emissive Scroll Pass
	void SetupVarsEmissiveScrollBlendedPass( EmissiveScrollBlendedPassVars_t &info )
	{
		info.m_nBlendStrength = EMISSIVEBLENDSTRENGTH;
		info.m_nBaseTexture = IRIS;
		info.m_nFlowTexture = EMISSIVEBLENDFLOWTEXTURE;
		info.m_nEmissiveTexture = EMISSIVEBLENDTEXTURE;
		info.m_nEmissiveTint = EMISSIVEBLENDTINT;
		info.m_nEmissiveScrollVector = EMISSIVEBLENDSCROLLVECTOR;
	}

	SHADER_INIT_PARAMS()
	{
		Eye_Refract_Vars_t info;
		SetupVarsEyeRefract( info );
		InitParams_Eyes_Refract( this, params, pMaterialName, info );

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
	}

	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			return "Eyes_dx8";
		}

		return 0;
	}

	SHADER_INIT
	{
		Eye_Refract_Vars_t info;
		SetupVarsEyeRefract( info );
		Init_Eyes_Refract( this, params, info );

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
	}

	SHADER_DRAW
	{
		// Skip the standard rendering if cloak pass is fully opaque
		bool bDrawStandardEye = true;
		if ( params[CLOAKPASSENABLED]->GetIntValue() && ( pShaderShadow == NULL ) ) // && not snapshotting
		{
			CloakBlendedPassVars_t info;
			SetupVarsCloakBlendedPass( info );
			if ( CloakBlendedPassIsFullyOpaque( params, info ) )
			{
				bDrawStandardEye = false;
			}
		}

		// Standard rendering pass
		if ( bDrawStandardEye )
		{
			Eye_Refract_Vars_t info;
			SetupVarsEyeRefract( info );
			Draw_Eyes_Refract( this, params, pShaderAPI, pShaderShadow, info, vertexCompression );
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
	}
END_SHADER
