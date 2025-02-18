//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_HUD_ANNOTATIONSPANEL_H
#define TF_HUD_ANNOTATIONSPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include <vgui/IScheme.h>
#include "hud.h"
#include "hudelement.h"
#include "tf_imagepanel.h"
#include "tf_hud_playerstatus.h"
#include "vgui_avatarimage.h"
#include "item_model_panel.h"
using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFAnnotationsPanelCallout : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFAnnotationsPanelCallout, EditablePanel );
public:
	CTFAnnotationsPanelCallout( Panel *parent, const char *name, int id, Vector &location, const char *text );
	~CTFAnnotationsPanelCallout();

	virtual void ApplySettings( KeyValues *pInResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );

	bool	UpdateCallout();
	inline int GetAnnotationID(){ return m_ID; }
	inline void Touch();
	inline void	SetLocation( const Vector &location ) { m_Location = location; }
	inline void SetFollowEntity( CBaseEntity *pFollowEntity ) { m_FollowEntity = pFollowEntity; }
	inline void	SetVisibilityBitfield( int iVisibilityBitfield ) { m_iVisibilityBitfield = iVisibilityBitfield; }
	void	SetShowDistance( bool bShowDistance );
	void    SetLifetime( float flLifetime );
	void	SetText( const char *text );	
	void	FadeAndRemove();

private:
	int m_ID;
	Vector m_Location;
	CHandle< CBaseEntity > m_FollowEntity;
	Label *m_pAnnotationLabel;
	Label *m_pDistanceLabel;
	ImagePanel *m_pArrow;
	const char *m_Text;
	float m_DeathTime;
	float m_flLerpPercentage;
	int m_iVisibilityBitfield;
	bool m_bWasOffscreen;
	bool m_bShowDistance;
	Panel *m_pBackground;
	KeyValues *m_pArrowImages;
	float m_flAlpha[2];
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFAnnotationsPanel : public EditablePanel, public CHudElement
{
private:
	DECLARE_CLASS_SIMPLE( CTFAnnotationsPanel, EditablePanel );

public:
	CTFAnnotationsPanel( const char *pElementName );
	virtual ~CTFAnnotationsPanel();

	virtual void Reset();
	virtual void Init();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void FireGameEvent( IGameEvent * event );

	void UpdateAnnotations( void );
	void AddAnnotation( IGameEvent * event );
	void HideAnnotation( int id );
	void RemoveAll();
	virtual bool ShouldDraw( void );
	void OnThink( void );


protected:
	CTFAnnotationsPanelCallout *TestAndAddCallout( int id, Vector &origin, const char *text );
		
private:
	bool					m_bShouldBeVisible;
	CUtlVector<CTFAnnotationsPanelCallout*>	m_pCalloutPanels;
	vgui::Panel				*m_pFreezePanelBG;

};

#endif // TF_HUD_ANNOTATIONSPANEL_H
