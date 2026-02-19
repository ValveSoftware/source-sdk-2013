//========= Contributed by 4v4 PASS Time developers. ==========================//
//
// Purpose: HUD element to show the build date and time.
//
//=============================================================================//

#ifndef PF_HUD_BUILDINFO_H
#define PF_HUD_BUILDINFO_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include "vgui_controls/EditablePanel.h"
#include <econ_controls.h>

class CHudBuildInfo : public CHudElement, public vgui::EditablePanel 
{
    DECLARE_CLASS_SIMPLE(CHudBuildInfo, vgui::EditablePanel);
public:
	CHudBuildInfo( const char *pElementName );


	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) override;

	virtual const char *GetResFilename( void ) { return "resource/UI/HudBuildInfo.res"; }

private:
    CExLabel    *m_pBuildTextLabel;
};


#endif // PF_HUD_BUILDINFO_H