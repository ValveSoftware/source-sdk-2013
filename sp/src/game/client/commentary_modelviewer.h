//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef COMMENTARY_MODELVIEWER_H
#define COMMENTARY_MODELVIEWER_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <game/client/iviewport.h>
#include "basemodelpanel.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCommentaryModelPanel : public CModelPanel
{
public:
	DECLARE_CLASS_SIMPLE( CCommentaryModelPanel, CModelPanel );

	CCommentaryModelPanel( vgui::Panel *parent, const char *name );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCommentaryModelViewer : public vgui::Frame, public IViewPortPanel
{
	DECLARE_CLASS_SIMPLE( CCommentaryModelViewer, vgui::Frame );
public:
	CCommentaryModelViewer(IViewPort *pViewPort);
	virtual ~CCommentaryModelViewer();

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	PerformLayout( void );
	virtual void	OnCommand( const char *command );
	virtual void	OnKeyCodePressed( vgui::KeyCode code );
	virtual void	OnThink( void );

	void			SetModel( const char *pszName, const char *pszAttached );

	void			HandleMovementInput( void );

	// IViewPortPanel
public:
	virtual const char *GetName( void ) { return PANEL_COMMENTARY_MODELVIEWER; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset() {};
	virtual void Update() {};
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }

private:
	IViewPort				*m_pViewPort;
	CCommentaryModelPanel	*m_pModelPanel;

	Vector					m_vecResetPos;
	Vector					m_vecResetAngles;
	bool					m_bTranslating;
	float					m_flYawSpeed;
	float					m_flZoomSpeed;
};

#endif // COMMENTARY_MODELVIEWER_H
