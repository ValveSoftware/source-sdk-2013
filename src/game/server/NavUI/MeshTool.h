//--------------------------------------------------------------------------------------------------------
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef MESHTOOL_H
#define MESHTOOL_H

#ifdef SERVER_USES_VGUI

#include "NavUI.h"
#include "nav.h"


//--------------------------------------------------------------------------------------------------------
class MeshToolPanel : public CNavUIToolPanel
{
	DECLARE_CLASS_SIMPLE( MeshToolPanel, CNavUIToolPanel );

public:
	MeshToolPanel( vgui::Panel *parent, const char *toolName );
};

#endif // SERVER_USES_VGUI

#endif // MESHTOOL_H
