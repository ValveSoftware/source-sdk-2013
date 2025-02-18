//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_ROUNDINFO_H
#define TF_ROUNDINFO_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: displays the RoundInfo menu
//-----------------------------------------------------------------------------
class RoundInfoOverlay;

class CTFRoundInfo : public vgui::Frame, public IViewPortPanel, public CGameEventListener
{
private:
	DECLARE_CLASS_SIMPLE( CTFRoundInfo, vgui::Frame );

public:
	CTFRoundInfo( IViewPort *pViewPort );

	virtual const char *GetName( void ){ return PANEL_ROUNDINFO; }
	virtual void SetData( KeyValues *data );
	virtual void Reset(){ Update(); }
	virtual void Update();
	virtual bool NeedsUpdate( void ){ return false; }
	virtual bool HasInputElements( void ){ return true; }
	virtual void ShowPanel( bool bShow );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ){ return BaseClass::GetVPanel(); }
	virtual bool IsVisible(){ return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ){ BaseClass::SetParent( parent ); }

	virtual void FireGameEvent( IGameEvent *event );

	virtual GameActionSet_t GetPreferredActionSet() { return GAME_ACTION_SET_IN_GAME_HUD; }

protected:
	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void PerformLayout();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnCommand( const char *command );

	void UpdateImage( vgui::ImagePanel *pImagePanel, const char *pszImageName );

protected:
	IViewPort			*m_pViewPort;

	CExLabel			*m_pTitle;
	vgui::ImagePanel	*m_pMapImage;
	
#ifdef _X360
	CTFFooter			*m_pFooter;
#else
	CExButton			*m_pContinue;
#endif

	char				m_szMapImage[MAX_ROUND_IMAGE_NAME];

	RoundInfoOverlay	*m_pOverlay;

	int m_iFoundPoints;
	int m_iNextRoundPoints[2];
};


#endif // TF_ROUNDINFO_H
