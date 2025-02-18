//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_HUD_INSPECTPANEL_H
#define TF_HUD_INSPECTPANEL_H
#ifdef _WIN32
#pragma once
#endif

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudInspectPanel : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudInspectPanel, EditablePanel );

public:
	CHudInspectPanel( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );

	virtual int		GetRenderGroupPriority( void ) { return 35; }	// less than statpanel
	void		UserCmd_InspectTarget( void );
	C_TFPlayer		*GetInspectTarget( C_TFPlayer *pLocalTFPlayer );

	int	HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

private:
	void LockInspectRenderGroup( bool bLock );
	void SetPanelVisible( bool bVisible );

	CItemModelPanel		*m_pItemPanel;
	CHandle<C_TFPlayer>	m_hTarget;
	int					m_iTargetItemIterator;
};

#endif // TF_HUD_INSPECTPANEL_H
