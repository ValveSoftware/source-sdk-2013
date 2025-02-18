//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_ESCORT_H
#define TF_HUD_ESCORT_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include "tf_controls.h"
#include "GameEventListener.h"
#include "c_tf_objective_resource.h"
#include "IconPanel.h"
#include "hud_controlpointicons.h"
#include "tf_gamerules.h"
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"

class CEscortHillPanel : public vgui::Panel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CEscortHillPanel, vgui::Panel );
public:
	CEscortHillPanel( vgui::Panel *parent, const char *name );

	virtual void Paint( void );
	virtual void PerformLayout( void );

	void SetHillNumber( int nHill ){ m_nHill = nHill; }
	void SetTeamNumber( int nTeam ){ m_nTeam = nTeam; }

	virtual void OnTick( void );

public: // CGameEventListener:
	virtual void FireGameEvent( IGameEvent *event );

private:
	void UpdateHillAnimations( bool bOnHill );

private:
	int		m_iTexture;
	int		m_nHill; // hill number this panel represents
	int		m_nTeam;

	bool	m_bOnHill;
	bool	m_bFadingOut;

	float	m_flUVScroll;
	float	m_flUVW;
	int		m_nPanelWide;
	int		m_nPanelTall;
	int		m_bDownhill;
};

class CEscortStatusTeardrop : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CEscortStatusTeardrop, vgui::EditablePanel );
public:
	CEscortStatusTeardrop(Panel *parent);

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool IsVisible( void );

	void SetupForPoint( int iCPIndex );
	void UpdateBarText( int iCPIndex );

private:
	vgui::Label					*m_pBarText;
	CIconPanel					*m_pTeardrop;
	CIconPanel					*m_pBlocked;
	vgui::ImagePanel			*m_pCapping;
	int							m_iOrgHeight;
	int							m_iMidGroupIndex;
};

class CTFHudEscortProgressBar : public vgui::ImagePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFHudEscortProgressBar, vgui::ImagePanel );

	CTFHudEscortProgressBar( vgui::Panel *parent, const char *name );

	virtual void Paint();
	void SetPercentage( float flPercentage ){ m_flPercent = flPercentage; }

	void SetTeam( int nTeam );

private:

	float	m_flPercent;
	int		m_iTexture;
	int		m_nTeam;
};

class CSimpleControlPoint : public vgui::ImagePanel
{
	DECLARE_CLASS_SIMPLE( CSimpleControlPoint, vgui::ImagePanel );
public:

	CSimpleControlPoint(Panel *parent, const char *name, int index) : vgui::ImagePanel( parent, name )
	{
		SetShouldScaleImage( true );
		m_iCPIndex = index;
		m_bForceOpaque = false;
	}

	virtual bool IsVisible( void )
	{
		if ( IsInFreezeCam() == true )
			return false;

		return BaseClass::IsVisible();
	}

	void SetCPIndex( int index )
	{
		m_iCPIndex = index;
	}

	void UpdateImage( void )
	{
		int iOwner = ObjectiveResource()->GetOwningTeam( m_iCPIndex );
		bool bMultipleTrains = ( TFGameRules() && TFGameRules()->HasMultipleTrains() ) ? true : false;

		const char *szMatName = "";
		switch ( iOwner )
		{
		case TF_TEAM_BLUE:
			if ( !bMultipleTrains && !m_bForceOpaque )
			{
				szMatName = "hud/cart_point_blue";
			}
			else
			{
				szMatName = "hud/cart_point_blue_opaque";
			}
			break;
		case TF_TEAM_RED:
			if ( !bMultipleTrains && !m_bForceOpaque )
			{
				szMatName = "hud/cart_point_red";
			}
			else
			{
				szMatName = "hud/cart_point_red_opaque";
			}
			break;
		default:
			if ( !bMultipleTrains && !m_bForceOpaque )
			{
				szMatName = "hud/cart_point_neutral";
			}
			else
			{
				szMatName = "hud/cart_point_neutral_opaque";
			}
			break;
		}

		SetImage( VarArgs("..\\%s", szMatName ) );
	}

	int	GetCapIndex( void ) { return m_iCPIndex; }
	void SetForceOpaqueImages( bool state )
	{ 
		m_bForceOpaque = state; 
		UpdateImage();
	}

private:
	int					m_iCPIndex;
	bool				m_bForceOpaque;
};

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTFHudEscort : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTFHudEscort, vgui::EditablePanel );

public:

	CTFHudEscort( vgui::Panel *parent, const char *name );
	~CTFHudEscort();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();
	virtual bool IsVisible( void );
	virtual void Reset(){ m_iCurrentCP = -1; }
	void OnTick();

	void UpdateCPImages( void );
	void UpdateStatusTeardropFor( int iIndex );

	void SetTeam( int nTeam )
	{
		m_nTeam = nTeam; 
		if ( m_pProgressBar )
		{
			m_pProgressBar->SetTeam( m_nTeam );
		}
	}

	void SetTopPanel( bool bTopPanel )
	{
		m_bTopPanel = bTopPanel;
	}

public: // CGameEventListener:
	virtual void FireGameEvent( IGameEvent *event );

private:
	void UpdateAlarmAnimations( void );

private:
	int m_iSpeedLevel;	// how fast is the escorted object moving
	float m_flProgress;	// left to right progress percent
	CInterpolatedVar<float> m_flProgressHistory;

	vgui::EditablePanel *m_pEscortItemPanel;

	CUtlVector<CSimpleControlPoint *>		m_Icons;
	bool m_bHaveValidPointPositions;

	vgui::ImagePanel *m_pSpeedBackwards;

	vgui::ImagePanel *m_pCapPlayerImage;
	CExLabel	     *m_pCapNumPlayers;
	vgui::ImagePanel *m_pBlocked;

	vgui::ImagePanel *m_pCPTemplate;

	vgui::ImagePanel *m_pLevelBar;

	CEscortStatusTeardrop *m_pStatus;
	int m_iCurrentCP;

	CControlPointIconSwoop *m_pHilightSwoop;	
	bool m_bShowSwoop;

	float m_flRecedeTime;

	vgui::ImagePanel *m_pEscortItemImage;
	vgui::ImagePanel *m_pEscortItemImageBottom;
	vgui::ImagePanel *m_pHomeCPIcon;

	int m_nTeam;

	bool m_bMultipleTrains; // stored to compare against what TFGamerules says...

	int		m_iBlueMaterialIndex; // blue material for multiple tracks colored overlay
	int		m_iRedMaterialIndex; // red material for multiple tracks colored overlay

	CTFHudEscortProgressBar *m_pProgressBar;
	CEscortHillPanel *m_pHillPanels[TEAM_TRAIN_MAX_HILLS];

	vgui::ImagePanel *m_pEscortItemImageAlert;
	bool m_bAlarm;
	bool m_bTopPanel;

	int m_nNumHills;
};

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTFHudMultipleEscort : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTFHudMultipleEscort, vgui::EditablePanel );

public:
 	CTFHudMultipleEscort( vgui::Panel *parent, const char *name );
	~CTFHudMultipleEscort();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void SetVisible( bool state );
	virtual void Reset();

public: // IGameEventListener Interface
	virtual void FireGameEvent( IGameEvent * event );

private:
	CTFHudEscort	*m_pBluePanel;
	CTFHudEscort	*m_pRedPanel;
};

#endif	// TF_HUD_ESCORT_H
