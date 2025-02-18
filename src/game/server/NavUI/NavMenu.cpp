//--------------------------------------------------------------------------------------------------------
//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#ifdef SERVER_USES_VGUI
#include <filesystem.h>
#include "NavMenu.h"
#include "vgui_controls/MenuItem.h"
#endif // SERVER_USES_VGUI

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef SERVER_USES_VGUI

using namespace vgui;


//--------------------------------------------------------------------------------------------------------
NavMenu::NavMenu( Panel *parent, const char *panelName ) : Menu( parent, panelName )
{ 
}


//--------------------------------------------------------------------------------------------------------
bool NavMenu::LoadFromFile( const char * fileName)	// load menu from KeyValues
{
	KeyValues * kv = new KeyValues(fileName);

	if  ( !kv->LoadFromFile( filesystem, fileName, "GAME" ) )
		return false;

	bool ret = false;//LoadFromKeyValues( kv );

	kv->deleteThis();
	return ret;
}


//--------------------------------------------------------------------------------------------------------
NavMenu::~NavMenu()
{
}

#endif // SERVER_USES_VGUI

//--------------------------------------------------------------------------------------------------------
