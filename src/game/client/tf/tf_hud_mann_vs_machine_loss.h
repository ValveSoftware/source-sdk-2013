//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_MANN_VS_MACHINE_LOSS_H
#define TF_HUD_MANN_VS_MACHINE_LOSS_H
#ifdef _WIN32
#pragma once
#endif


#include "hudelement.h"
#include "tf_controls.h"
#include "hud.h"
#include <vgui/IScheme.h>
#include "tf_mann_vs_machine_stats.h"
#include "c_tf_objective_resource.h"
#include "tf_gamerules.h"
#include "tf_tips.h"
//=========================================================
class CMvMWaveLossPanel :  public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CMvMWaveLossPanel, vgui::EditablePanel );

public:
	CMvMWaveLossPanel( Panel *parent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void FireGameEvent( IGameEvent *event );
	virtual void OnTick( void );
	virtual void OnCommand( const char *command );

	void ShowPanel();

private:

	void SetCaptainCanteenImage( vgui::ImagePanel *panel, const char *pchImage, int nNewX );
	void SetHintImage( vgui::ImagePanel *panel, int iClassUsed, bool bAllowCaptainCanteen );

	void ClearContents();

	CExImageButton * m_pVoteButton;
	CExImageButton * m_pCloseButton;

	vgui::EditablePanel *m_pHintContainer;
	vgui::EditablePanel *m_pCollectionContainer;
	vgui::EditablePanel *m_pUsageContainer;

	vgui::ImagePanel *m_pCaptainCanteenBody;
	vgui::ImagePanel *m_pCaptainCanteenMisc;
	vgui::ImagePanel *m_pCaptainCanteenHat;

	vgui::ImagePanel *m_pHintImage1;
	vgui::ImagePanel *m_pHintImage2;
};
#endif // TF_HUD_MANN_VS_MACHINE_LOSS_H

