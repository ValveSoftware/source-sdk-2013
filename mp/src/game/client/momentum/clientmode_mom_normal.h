//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#if !defined( CLIENTMODE_MOM_NORM_H )
#define CLIENTMODE_MOM_NORM_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui/Cursor.h>

class CHudViewport;

namespace vgui
{
    typedef unsigned long HScheme;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class ClientModeMOMNormal : public ClientModeShared
{
public:
    DECLARE_CLASS(ClientModeMOMNormal, ClientModeShared);

    ClientModeMOMNormal();
    ~ClientModeMOMNormal();

    virtual void	Init();
    virtual bool	ShouldDrawCrosshair(void);
};

extern IClientMode *GetClientModeNormal();
extern vgui::HScheme g_hVGuiCombineScheme;

#endif // CLIENTMODE_MOM_NORMAL