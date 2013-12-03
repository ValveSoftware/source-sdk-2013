//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SDKVIEWPORT_H
#define SDKVIEWPORT_H


#include "sdk_shareddefs.h"
#include "baseviewport.h"


using namespace vgui;

namespace vgui 
{
	class Panel;
}

class SDKViewport : public CBaseViewport
{

private:
	DECLARE_CLASS_SIMPLE( SDKViewport, CBaseViewport );

public:

	IViewPortPanel* CreatePanelByName(const char *szPanelName);
	void CreateDefaultPanels( void );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
		
	int GetDeathMessageStartHeight( void );
};


#endif // SDKViewport_H
