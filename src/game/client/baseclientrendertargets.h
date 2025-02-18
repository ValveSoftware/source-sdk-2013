//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Has init functions for all the standard render targets used by most games.
//			Mods who wish to make their own render targets can inherit from this class
//			and in the 'InitClientRenderTargets' interface called by the engine, set up
//			their own render targets as well as calling the init functions for various
//			common render targets provided by this class.
//
//			Note: Unless the client defines a singleton interface by inheriting from
//			this class and exposing the singleton instance, these init and shutdown
//			functions WILL NOT be called by the engine.
//
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#ifndef CLIENTRENDERTARTETS_H_
#define CLIENTRENDERTARTETS_H_
#ifdef _WIN32
#pragma once
#endif

#include "game/client/iclientrendertargets.h"		// base class with interfaces called by the engine
#include "materialsystem/imaterialsystem.h"		// for material system classes and interfaces


// Externs
class IMaterialSystem;
class IMaterialSystemHardwareConfig;

class CBaseClientRenderTargets : public IClientRenderTargets
{
	// no networked vars
	DECLARE_CLASS_GAMEROOT( CBaseClientRenderTargets, IClientRenderTargets );
public:
	// Interface called by engine during material system startup.
	virtual void InitClientRenderTargets ( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig, int iWaterTextureSize = 1024, int iCameraTextureSize = 256 );
	// Shutdown all custom render targets here.
	virtual void ShutdownClientRenderTargets ( void );

protected:
	
	// Standard render textures used by most mods-- Classes inheriting from
	// this can choose to init these or not depending on their needs.

	// For reflective and refracting water
	CTextureReference		m_WaterReflectionTexture;
	CTextureReference		m_WaterRefractionTexture;

	// Used for monitors
	CTextureReference		m_CameraTexture;

	// Used for the HUD in stereo and head tracking mode
	CTextureReference		m_UITexture;

	// Init functions for the common render targets
	ITexture* CreateWaterReflectionTexture( IMaterialSystem* pMaterialSystem, int iSize = 1024 );
	ITexture* CreateWaterRefractionTexture( IMaterialSystem* pMaterialSystem, int iSize = 1024 );
	ITexture* CreateCameraTexture( IMaterialSystem* pMaterialSystem, int iSize = 256 );

};

#endif // CLIENTRENDERTARTETS_H_
