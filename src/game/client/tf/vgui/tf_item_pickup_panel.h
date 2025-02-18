//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_ITEM_PICKUP_PANEL_H
#define TF_ITEM_PICKUP_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "item_pickup_panel.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFItemPickupPanel : public CItemPickupPanel
{
	DECLARE_CLASS_SIMPLE( CTFItemPickupPanel, CItemPickupPanel );
public:
	CTFItemPickupPanel( Panel *parent );
	virtual ~CTFItemPickupPanel();

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	OnCommand( const char *command );

protected:
	virtual void	UpdateModelPanels( void );

private:
	vgui::ImagePanel				*m_pClassImage;
	vgui::Panel						*m_pClassImageBG;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFItemDiscardPanel : public CItemDiscardPanel
{
	DECLARE_CLASS_SIMPLE( CTFItemDiscardPanel, CItemDiscardPanel );
public:
	CTFItemDiscardPanel( Panel *parent );
	virtual ~CTFItemDiscardPanel();

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	PerformLayout( void );
	virtual void	ShowPanel( bool bShow );
	virtual void	OnTick( void );
	virtual void	OnCommand( const char *command );

private:
	vgui::Label		*m_pExplanationALabel;
	vgui::Label		*m_pExplanationBLabel;
	vgui::Label		*m_pExplanationCaratLabel;
	float			m_flStartExplanationsAt;
};

#endif // TF_ITEM_PICKUP_PANEL_H
