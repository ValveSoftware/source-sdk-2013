//--------------------------------------------------------------------------------------------------------
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef NAV_MENU_H
#define NAV_MENU_H

#ifdef SERVER_USES_VGUI

#include <vgui_controls/Menu.h>
#include <game/client/iviewport.h>
#include <filesystem.h>
#include "utlstack.h"
#include "utlvector.h"
#include <KeyValues.h>

class NavMenu : public vgui::Menu
{
private:
	DECLARE_CLASS_SIMPLE( NavMenu, vgui::Menu );

public:
	NavMenu( vgui::Panel *parent, const char *panelName );
	~NavMenu();

	bool LoadFromFile( const char * fileName );	// load menu from file (via KeyValues)
};

#endif // SERVER_USES_VGUI

#endif // NAV_MENU_H
