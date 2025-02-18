//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CUSTOM_TEXTURE_CACHE_H
#define CUSTOM_TEXTURE_CACHE_H
#ifdef _WIN32
#pragma once
#endif

#ifndef GC_CLIENTSYSTEM_H
	#include "gc_clientsystem.h"
#endif

class IGameSystem;
class ITexture;
class CEconItemView;

/// Given a UGC cloud ID of a custom image, return a VGUI texture handle
/// that can be used for drawing.  Returns 0 on failure
int GetCustomTextureGuiHandle( uint64 hCloudId );

//-----------------------------------------------------------------------------
// Purpose: Job to do the async work of uploading the file to the CDN (if
// necessary) and sending the tool request message
//-----------------------------------------------------------------------------
class CApplyCustomTextureJob : public GCSDK::CGCClientJob
{
public:

	CApplyCustomTextureJob( itemid_t nToolItemID, itemid_t nSubjectItemID, const void *pPNGData, int nPNGDataBytes );

protected:
	char	m_chRemoteStorageName[ MAX_PATH ];

	virtual bool BYieldingRunGCJob();
	virtual EResult YieldingRunJob();
	virtual EResult YieldingFindFileIncacheOrUploadFileToCDN();
	virtual EResult YieldingApplyTool();

	/// The file data, in PNG format
	CUtlBuffer m_bufPNGData;

	/// Item that we are applying the texture onto
	itemid_t m_nSubjectItemID;

	/// Tool that is being applied and will be consumed
	itemid_t m_nToolItemID;

	/// Cloud file ID
	uint64 m_hCloudID;

private:
	void CleanUp();
};

/// get interface to the game system responsible for managing the custom texture cache
IGameSystem *CustomTextureToolCacheGameSystem();

/// A few internal things that really shouldn't be public
namespace CustomTextureSystem
{

/// If we're auditioning (while selecting a file to apply on a model),
/// what texture should we display?
extern ITexture *g_pPreviewCustomTexture;

/// What is the econ item that is being auditioned and so should
/// use the preview texture
extern CEconItemView *g_pPreviewEconItem;

/// Do we need to update the bits in the rendering system?
extern bool g_pPreviewCustomTextureDirty;

extern const char k_rchCustomTextureFilterPreviewImageName[];
extern const char k_rchCustomTextureFilterPreviewTextureName[];

}

#endif // CUSTOM_TEXTURE_CACHE_H
