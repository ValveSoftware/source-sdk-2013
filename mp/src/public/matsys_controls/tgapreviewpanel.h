//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TGAPREVIEWPANEL_H
#define TGAPREVIEWPANEL_H

#ifdef _WIN32
#pragma once
#endif


#include "matsys_controls/proceduraltexturepanel.h"
#include "tier1/utlstring.h"


//-----------------------------------------------------------------------------
//
// TGA Preview panel
//
//-----------------------------------------------------------------------------
class CTGAPreviewPanel : public CProceduralTexturePanel
{
	DECLARE_CLASS_SIMPLE( CTGAPreviewPanel, CProceduralTexturePanel );

public:
	// constructor
	CTGAPreviewPanel( vgui::Panel *pParent, const char *pName );
	void SetTGA( const char *pFullPath );
	const char *GetTGA() const;

private:
	CUtlString m_TGAName;
};


#endif // TGAPREVIEWPANEL_H