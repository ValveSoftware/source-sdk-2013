//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#ifndef MODELIMAGEPANEL_H
#define MODELIMAGEPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "basemodel_panel.h"

class CIconRenderReceiver;

class CModelImagePanel : public CBaseModelPanel
{
DECLARE_CLASS_SIMPLE( CModelImagePanel, CBaseModelPanel );

public:

	// Constructor, Destructor.
	CModelImagePanel( vgui::Panel *pParent, const char *pName );
	virtual ~CModelImagePanel();

	virtual void PerformLayout() OVERRIDE;
	virtual void Paint() OVERRIDE;
	virtual void OnSizeChanged( int wide, int tall ) OVERRIDE;

	virtual void SetMDL( MDLHandle_t handle, void *pProxyData = NULL ) OVERRIDE;
	virtual void SetMDL( const char *pMDLName, void *pProxyData = NULL ) OVERRIDE;
	void SetMDLBody( unsigned int nBody );
	void SetMDLSkin( int nSkin );

	void InvalidateImage();

private:
	CIconRenderReceiver *m_pCachedIcon;
	IMaterial			*m_pCachedMaterial;
	int					m_iCachedTextureID;
};

#endif // MODELIMAGEPANEL_H
