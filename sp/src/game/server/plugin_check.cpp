//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// This class allows the game dll to control what functions 3rd party plugins can 
//  call on clients.
//
//=============================================================================//

#include "cbase.h"
#include "eiface.h"

//-----------------------------------------------------------------------------
// Purpose: An implementation 
//-----------------------------------------------------------------------------
class CPluginHelpersCheck : public IPluginHelpersCheck
{
public:
	virtual bool CreateMessage( const char *plugin, edict_t *pEntity, DIALOG_TYPE type, KeyValues *data );
};

CPluginHelpersCheck s_PluginCheck;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CPluginHelpersCheck, IPluginHelpersCheck, INTERFACEVERSION_PLUGINHELPERSCHECK, s_PluginCheck);

bool CPluginHelpersCheck::CreateMessage( const char *plugin, edict_t *pEntity, DIALOG_TYPE type, KeyValues *data )
{
	// return false here to disallow a plugin from running this command on this client
	return true;
}

