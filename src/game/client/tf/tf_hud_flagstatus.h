//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_FLAGSTATUS_H
#define TF_HUD_FLAGSTATUS_H
#ifdef _WIN32
#pragma once
#endif

#include "entity_capture_flag.h"
#include "tf_controls.h"
#include "tf_imagepanel.h"
#include "GameEventListener.h"
#include "hudelement.h"

class CCaptureFlag;
class CTFFlagCalloutPanel;

//-----------------------------------------------------------------------------
// Purpose:  Draws the rotated arrow panels
//-----------------------------------------------------------------------------
class CTFArrowPanel : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CTFArrowPanel, vgui::Panel );

	CTFArrowPanel( vgui::Panel *parent, const char *name );
	virtual void Paint();
	virtual bool IsVisible( void );
	void SetEntity( EHANDLE hEntity ){ m_hEntity = hEntity; }
	float GetAngleRotation( void );
	void OnTick( void );

private:

	EHANDLE				m_hEntity;	

	CMaterialReference	m_RedMaterial;
	CMaterialReference	m_BlueMaterial;
	CMaterialReference	m_NeutralMaterial;
	CMaterialReference	m_NeutralRedMaterial;

	CMaterialReference	m_RedMaterialNoArrow;
	CMaterialReference	m_BlueMaterialNoArrow;

	bool				m_bUseRed;
	float				m_flNextColorSwitch;
	IMaterial			*m_pMaterial;
};

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTFFlagStatus : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFFlagStatus, vgui::EditablePanel );

	CTFFlagStatus( vgui::Panel *parent, const char *name );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool IsVisible( void );
	void UpdateStatus( void );

	void SetEntity( EHANDLE hEntity )
	{ 
		m_hEntity = hEntity;

		if ( m_pArrow )
		{
			m_pArrow->SetEntity( hEntity );
		}

		UpdateStatus();
	}

	CBaseEntity *GetEntity( void ){ return m_hEntity.Get(); }

private:

	EHANDLE			m_hEntity;

	CTFArrowPanel	*m_pArrow;
	CTFImagePanel	*m_pStatusIcon;
	CTFImagePanel	*m_pBriefcase;
};


//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTFHudFlagObjectives : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTFHudFlagObjectives, vgui::EditablePanel );

public:

	CTFHudFlagObjectives( vgui::Panel *parent, const char *name );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool IsVisible( void );
	virtual void Reset();
	void OnTick();

public: // IGameEventListener:
	virtual void FireGameEvent( IGameEvent *event );

private:
	
	void UpdateStatus( C_BasePlayer *pNewOwner = NULL, C_BaseEntity *pFlagEntity = NULL );
	void SetPlayingToLabelVisible( bool bVisible );
	void SetCarriedImage( const char *pchIcon );

private:

	vgui::ImagePanel		*m_pCarriedImage;

	CExLabel				*m_pPlayingTo;
	vgui::Panel				*m_pPlayingToBG;

	CTFFlagStatus			*m_pRedFlag;
	CTFFlagStatus			*m_pBlueFlag;
	CTFArrowPanel			*m_pCapturePoint;

	bool					m_bFlagAnimationPlayed;
	bool					m_bCarryingFlag;

	vgui::ImagePanel		*m_pSpecCarriedImage;

	vgui::ImagePanel		*m_pPoisonImage;
	CExLabel				*m_pPoisonTimeLabel;

	bool					m_bPlayingHybrid_CTF_CP;
	bool					m_bPlayingSpecialDeliveryMode;

	int						m_nNumValidFlags;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFFlagCalloutPanel : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFFlagCalloutPanel, vgui::EditablePanel );
public:
	CTFFlagCalloutPanel( const char *pElementName );
	~CTFFlagCalloutPanel( void );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );
	virtual void OnTick( void );
	virtual void PaintBackground( void );
	virtual void Paint( void );
	
	void	GetCalloutPosition( const Vector &vecDelta, float flRadius, float *xpos, float *ypos, float *flRotation );
	void	SetFlag( CCaptureFlag *pFlag, float flDuration, Vector &vecOffset );
	static CTFFlagCalloutPanel *AddFlagCalloutIfNotFound( CCaptureFlag *pFlag, float flDuration, Vector &vecLocation );
	bool	ShouldShowFlagIconToLocalPlayer( void );
	void	ScaleAndPositionCallout( float flScale = 1.f );
	
	CHandle< CCaptureFlag > m_hFlag;

private:
	IMaterial		*m_pArrowMaterial;
	CTFImagePanel	*m_pFlagCalloutPanel;
	vgui::Label		*m_pFlagValueLabel;
	CTFImagePanel	*m_pFlagStatusIcon;

	float			m_flRemoveTime;
	float			m_flFirstDisplayTime;
	Vector			m_vecOffset;
	int				m_iDrawArrow;
	bool			m_bFlagVisible;		// LOS

	float			m_flPrevScale;
	int				m_nPanelWideOrig;
	int				m_nPanelTallOrig;
	int				m_nLabelWideOrig;
	int				m_nLabelTallOrig;
	int				m_nIconWideOrig;
	int				m_nIconTallOrig;

	static CUtlVector< CTFFlagCalloutPanel* > m_FlagCalloutPanels;
};

#endif	// TF_HUD_FLAGSTATUS_H
