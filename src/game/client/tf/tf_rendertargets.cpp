//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: L4D mod render targets are specified by and accessable through this singleton
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "tf_rendertargets.h"
#include "materialsystem/imaterialsystem.h"
#include "rendertexture.h"
#if defined( REPLAY_ENABLED )
#include "replay/replay_screenshot.h"
#endif

ConVar tf_water_resolution( "tf_water_resolution", "1024", FCVAR_NONE, "Needs to be set at game launch time to override." );
ConVar tf_monitor_resolution( "tf_monitor_resolution", "1024", FCVAR_NONE, "Needs to be set at game launch time to override." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ITexture *CTFRenderTargets::CreateItemModelPanelTexture( const char *pszName, IMaterialSystem* pMaterialSystem, int iSize )
{
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		pszName,
		iSize, iSize, RT_SIZE_DEFAULT,
		pMaterialSystem->GetBackBufferFormat(),
		MATERIAL_RT_DEPTH_SHARED, 
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		0 );
}

//-----------------------------------------------------------------------------
// Purpose: InitClientRenderTargets, interface called by the engine at material system init in the engine
// Input  : pMaterialSystem - the interface to the material system from the engine (our singleton hasn't been set up yet)
//			pHardwareConfig - the user's hardware config, useful for conditional render targets setup
//-----------------------------------------------------------------------------
extern const char *g_ItemModelPanelRenderTargetNames[];
extern const char *g_pszModelImagePanelRTName;
void CTFRenderTargets::InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig )
{
	BaseClass::InitClientRenderTargets( pMaterialSystem, pHardwareConfig, tf_water_resolution.GetInt(), tf_monitor_resolution.GetInt() );

	// rt for item model panels
	for ( int i = 0; i < ITEM_MODEL_IMAGE_CACHE_SIZE; i++ )
	{
		int index = m_tfRenderTargets.AddToTail();
		m_tfRenderTargets[index].Init( CreateItemModelPanelTexture( g_ItemModelPanelRenderTargetNames[i], pMaterialSystem, 256 ) );
	}

	// rt for CModelImagePanel
	int index = m_tfRenderTargets.AddToTail();
	m_tfRenderTargets[index].Init( CreateItemModelPanelTexture( g_pszModelImagePanelRTName, pMaterialSystem, 256 ) );

	CReplayScreenshotTaker::CreateRenderTarget( pMaterialSystem );
}

//-----------------------------------------------------------------------------
// Purpose: Shutdown client render targets. This gets called during shutdown in the engine
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFRenderTargets::ShutdownClientRenderTargets()
{
	BaseClass::ShutdownClientRenderTargets();

	for ( int i = 0; i < m_tfRenderTargets.Count(); i++ )
	{
		m_tfRenderTargets[i].Shutdown();
	}
	m_tfRenderTargets.Purge();
}


static CTFRenderTargets g_TFRenderTargets;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CTFRenderTargets, IClientRenderTargets, 
	CLIENTRENDERTARGETS_INTERFACE_VERSION, g_TFRenderTargets );
CTFRenderTargets* g_pTFRenderTargets = &g_TFRenderTargets;