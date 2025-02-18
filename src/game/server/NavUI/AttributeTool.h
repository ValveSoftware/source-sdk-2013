//--------------------------------------------------------------------------------------------------------
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef ATTRIBUTETOOL_H
#define ATTRIBUTETOOL_H

#include "NavUI.h"
#include "nav.h"

#ifdef SERVER_USES_VGUI

//--------------------------------------------------------------------------------------------------------
class AttributeToolPanel : public CNavUIToolPanel
{
	DECLARE_CLASS_SIMPLE( AttributeToolPanel, CNavUIToolPanel );

public:
	AttributeToolPanel( vgui::Panel *parent, const char *toolName );
};

#endif // SERVER_USES_VGUI

#endif // ATTRIBUTETOOL_H
