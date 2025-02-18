//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//
#ifndef TF_RENDERTARGETS_H
#define TF_RENDERTARGETS_H
#ifdef _WIN32
#pragma once
#endif

#include "baseclientrendertargets.h" // Base class, with interfaces called by engine and inherited members to init common render targets
#include "item_model_panel.h"

// externs
class IMaterialSystem;
class IMaterialSystemHardwareConfig;

class CTFRenderTargets : public CBaseClientRenderTargets
{
	// no networked vars
	DECLARE_CLASS_GAMEROOT( CTFRenderTargets, CBaseClientRenderTargets );
public:
	virtual void InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig );
	virtual void ShutdownClientRenderTargets();

private:
	ITexture *CreateItemModelPanelTexture( const char *pszName, IMaterialSystem* pMaterialSystem, int iSize );

private:
	// Used for rendering item model panels.
	CUtlVector< CTextureReference >		m_tfRenderTargets;
};

extern CTFRenderTargets* g_pTFRenderTargets;


#endif // TF_RENDERTARGETS_H
