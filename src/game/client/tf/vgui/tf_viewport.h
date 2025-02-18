//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_VIEWPORT_H
#define TF_VIEWPORT_H


#include "tf_shareddefs.h"
#include "baseviewport.h"


using namespace vgui;

namespace vgui 
{
	class Panel;
	class Label;
	class CBitmapImagePanel;
}

//==============================================================================
class TFViewport : public CBaseViewport
{

private:
	DECLARE_CLASS_SIMPLE( TFViewport, CBaseViewport );

public:
	TFViewport();
	~TFViewport();

	IViewPortPanel* CreatePanelByName(const char *szPanelName);
	void CreateDefaultPanels( void );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Start( IGameUIFuncs *pGameUIFuncs, IGameEventManager2 * pGameEventManager );
		
	int GetDeathMessageStartHeight( void );

	virtual void OnScreenSizeChanged( int iOldWide, int iOldTall );

	virtual void OnTick() OVERRIDE;

private:
	void CenterWindow( vgui::Frame *win );

};


#endif // TF_Viewport_H
