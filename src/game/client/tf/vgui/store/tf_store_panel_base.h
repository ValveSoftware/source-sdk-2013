//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_STORE_PANEL_BASE_H
#define TF_STORE_PANEL_BASE_H
#ifdef _WIN32
#pragma once
#endif

#include "store/store_panel.h"

class CArmoryPanel;
class CStorePage;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFBaseStorePanel : public CStorePanel
{
	DECLARE_CLASS_SIMPLE( CTFBaseStorePanel, CStorePanel );
protected:
	// CTFBaseStorePanel should not be instantiated directly
	CTFBaseStorePanel( Panel *parent );

public:
	// UI Layout
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnThink();

	// GC Management
	virtual void	PostTransactionCompleted( void );

	// Cart Management
	CStoreCart	 *GetCart( void ) { return &m_Cart; }
	void		ShowStorePanel( void );
	void		InitiateCheckout( void );
	void		CheckoutCancel( void );
	
	virtual void SetTransactionID( uint64 inID ) OVERRIDE;

	// Armory management
	MESSAGE_FUNC_PARAMS( OnArmoryOpened, "ArmoryOpened", data );
	MESSAGE_FUNC( OnArmoryClosed, "ArmoryClosed" );

private:
	CArmoryPanel	*m_pArmoryPanel;			
	vgui::Panel		*m_pNotificationsPresentPanel;
};

#endif // TF_STORE_PANEL_BASE_H
