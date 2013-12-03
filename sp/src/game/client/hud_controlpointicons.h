//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef HUD_CONTROLPOINTICONS_H
#define HUD_CONTROLPOINTICONS_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/AnimationController.h"
#include "vgui_controls/CircularProgressBar.h"
#include <vgui/ISurface.h>
#include "tf_controls.h"
#include "IconPanel.h"

#define PULSE_TIME_PER_ICON		1.5f

#define PULSE_RAMP_TIME			0.5
#define PULSE_REMAP_SIZE		6.0
#define FAKE_CAPTURE_TIME		5.0
#define FAKE_CAPTURE_POST_PAUSE	2.0

extern ConVar mp_capstyle;
extern ConVar mp_blockstyle;

#define STARTCAPANIM_SWOOP_LENGTH		0.4
#define STARTCAPANIM_ICON_SWITCH		0.15
#define FINISHCAPANIM_SWOOP_LENGTH		0.2

#define CAP_BOX_INDENT_X				XRES(2)
#define CAP_BOX_INDENT_Y				YRES(2)

#define CP_TEXTURE_COUNT	8

class CControlPointIcon;

// Options for how the cap progress teardrop positions itself around the cap point icon
enum 
{
	CP_DIR_N,
	CP_DIR_NW,
	CP_DIR_NE,
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CControlPointCountdown : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CControlPointCountdown, vgui::EditablePanel );

public:
	CControlPointCountdown(Panel *parent, const char *name);

	virtual void ApplySchemeSettings( IScheme *scheme );
	virtual void PerformLayout();
	virtual void OnTick( void );

	void SetUnlockTime( float flTime );
	float GetUnlockTime( void ){ return m_flUnlockTime; }

private:

	bool	m_bFire5SecRemain;
	bool	m_bFire4SecRemain;
	bool	m_bFire3SecRemain;
	bool	m_bFire2SecRemain;
	bool	m_bFire1SecRemain;
	bool	m_bFire0SecRemain;

	int		m_flUnlockTime;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CControlPointProgressBar : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CControlPointProgressBar, vgui::EditablePanel );
public:
	CControlPointProgressBar(Panel *parent);

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout( void );
	virtual void Paint( void );
	virtual bool IsVisible( void );
	virtual void Reset( void );

	void SetupForPoint( CControlPointIcon *pIcon );
	void UpdateBarText( void );

private:
	CControlPointIcon			*m_pAttachedToIcon;
	vgui::CircularProgressBar	*m_pBar;
	vgui::Label					*m_pBarText;
	CIconPanel					*m_pTeardrop;
	CIconPanel					*m_pTeardropSide;
	CIconPanel					*m_pBlocked;
	int							m_iOrgHeight;
	int							m_iMidGroupIndex;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CControlPointIconSwoop : public vgui::ImagePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CControlPointIconSwoop, vgui::ImagePanel );
public:
	CControlPointIconSwoop(Panel *parent, const char *name) : vgui::ImagePanel( parent, name )
	{
		SetImage( "../sprites/obj_icons/capture_highlight" );
		SetShouldScaleImage( true );
		ListenForGameEvent( "localplayer_changeteam" );
	}

	virtual void PaintBackground( void )
	{
		float flElapsedTime = (gpGlobals->curtime - m_flStartCapAnimStart);

		if (GetImage())
		{
			surface()->DrawSetColor(255, 255, 255, 255);
			int iYPos = RemapValClamped( flElapsedTime, 0, STARTCAPANIM_SWOOP_LENGTH, 0, GetTall() );
			GetImage()->SetPos( 0, iYPos );
			GetImage()->Paint();
		}

		// Once we've finished the swoop, go away
		if ( flElapsedTime >= STARTCAPANIM_SWOOP_LENGTH )
		{
			SetVisible( false );
		}
	}

	virtual bool IsVisible( void )
	{
		if ( IsInFreezeCam() == true )
			return false;

		return BaseClass::IsVisible();
	}

	void	StartSwoop( void )
	{
		m_flStartCapAnimStart = gpGlobals->curtime;
	}

	void	FireGameEvent( IGameEvent * event )
	{
		if ( FStrEq( "localplayer_changeteam", event->GetName() ) )
		{
			SetVisible( false );
		}
	}

private:
	float				m_flStartCapAnimStart;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CControlPointIconCapArrow : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CControlPointIconCapArrow, vgui::Panel );
public:
	CControlPointIconCapArrow( CControlPointIcon *pIcon, Panel *parent, const char *name);

	virtual void Paint( void );
	virtual bool IsVisible( void );

	void	SetImage( const char *pszImage )
	{
		m_pArrowMaterial = materials->FindMaterial( pszImage, TEXTURE_GROUP_VGUI );
	}

private:
	IMaterial			*m_pArrowMaterial;
	CControlPointIcon	*m_pAttachedToIcon;

};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CControlPointIconCapturePulse : public vgui::ImagePanel
{
	DECLARE_CLASS_SIMPLE( CControlPointIconCapturePulse, vgui::ImagePanel );
public:
	CControlPointIconCapturePulse(Panel *parent, const char *name) : vgui::ImagePanel( parent, name )
	{
		SetImage( "../sprites/obj_icons/icon_obj_white" );
		SetShouldScaleImage( true );
	}

	virtual void PaintBackground( void )
	{
		if ( m_flFinishCapAnimStart && gpGlobals->curtime > m_flFinishCapAnimStart )
		{
			float flElapsedTime = MAX( 0, (gpGlobals->curtime - m_flFinishCapAnimStart) );
			if (GetImage())
			{
				surface()->DrawSetColor(255, 255, 255, 255);
				int iSize = RemapValClamped( flElapsedTime, 0, FINISHCAPANIM_SWOOP_LENGTH, GetWide(), m_iShrinkSize );
				GetImage()->SetPos( (GetWide() - iSize)*0.5, (GetTall() - iSize)*0.5 );
				GetImage()->SetSize( iSize, iSize );
				GetImage()->Paint();
			}

			// Once we've finished the swoop, go away
			if ( flElapsedTime >= FINISHCAPANIM_SWOOP_LENGTH )
			{
				SetVisible( false );
			}
		}
	}

	void	StartPulse( float flTime, int iShrinkSize )
	{
		m_flFinishCapAnimStart = flTime;
		m_iShrinkSize = iShrinkSize;

		if ( GetWide() < m_iShrinkSize )
		{
			SetWide( m_iShrinkSize );
		}
	}

	virtual bool IsVisible( void )
	{
		if ( IsInFreezeCam() == true )
			return false;

		return BaseClass::IsVisible();
	}

private:
	float				m_flFinishCapAnimStart;
	int					m_iShrinkSize;
};

//-----------------------------------------------------------------------------
// Purpose: The base image in the cap point icons that pulses.
//-----------------------------------------------------------------------------
class CControlPointIconPulseable : public vgui::ImagePanel
{
	DECLARE_CLASS_SIMPLE( CControlPointIconPulseable, vgui::ImagePanel );
public:
	CControlPointIconPulseable(Panel *parent, const char *name, int iIndex) : vgui::ImagePanel( parent, name )
	{
		SetShouldScaleImage( true );
		m_pPulseImage = NULL;
		m_iCPIndex = iIndex;
	}

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void OnSizeChanged(int newWide, int newTall);
	virtual void PaintBackground( void );

	void	StartPulsing( float flDelay, float flPulseTime, bool bAccelerate );
	void	StopPulsing( void );

	virtual bool IsVisible( void )
	{
		if ( IsInFreezeCam() == true )
			return false;

		return BaseClass::IsVisible();
	}

private:
	int					m_iCPIndex;
	float				m_flStartCapAnimStart;
	float				m_flPulseTime;
	bool				m_bAccelerateOverCapture;
	IImage				*m_pPulseImage;
};

//-----------------------------------------------------------------------------
// Purpose: A single icon that shows the state of one control point
//-----------------------------------------------------------------------------
class CControlPointIcon : public vgui::EditablePanel, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CControlPointIcon, vgui::EditablePanel );
public:	
	CControlPointIcon( Panel *parent, const char *pName, int iIndex );
	~CControlPointIcon( void );
	
	virtual void ApplySchemeSettings( IScheme *scheme );
	virtual void PerformLayout( void );

	void		 UpdateImage( void );
	void		 UpdateCapImage( void );
	bool		 IsPointLocked( void );
	int			 GetCapIndex( void ) { return m_iCPIndex; }
	void		 SetSwipeUp( bool bUp ) { m_bSwipeUp = bUp; }
	bool		 ShouldSwipeUp( void ) { return m_bSwipeUp; }
	int			 GetCapProgressDir( void ) { return m_iCapProgressDir; }
	void		 SetCapProgressDir( int iDir ) { m_iCapProgressDir = iDir; }
	void		 FakePulse( float flTime );
	bool		 IsVisible( void );
	virtual void Paint( void );
	bool		 IsPointUnlockCountdownRunning( void );

	virtual void FireGameEvent( IGameEvent *event );

	void SetUnlockTime( float flTime )
	{
		if ( m_pCountdown )
		{
			m_pCountdown->SetUnlockTime( flTime );
		}
	}

	void SetTimerTime( float flTime ); // used to display CCPTimerLogic countdowns

private:
	virtual void OnTick();

private:
	int								m_iCPIndex;
	vgui::ImagePanel				*m_pOverlayImage;
	CControlPointIconPulseable		*m_pBaseImage;
	CControlPointIconCapArrow		*m_pCapImage;
	DHANDLE< CControlPointIconSwoop	>	m_pCapHighlightImage;
	DHANDLE< CControlPointIconCapturePulse > m_pCapPulseImage;
	vgui::ImagePanel				*m_pCapPlayerImage;
	vgui::Label						*m_pCapNumPlayers;
	bool							m_bSwipeUp;
	float							m_flStartCapAnimStart;
	int								m_iCapProgressDir;
	int								m_iPrevCappers;
	bool							m_bCachedLockedState;

	bool							m_bCachedCountdownState;
	CControlPointCountdown			*m_pCountdown;

	DHANDLE< CExLabel >				m_pCPTimerLabel; // used to display CCPTimerLogic countdowns
	DHANDLE< vgui::ImagePanel >		m_pCPTimerBG; // used to display CCPTimerLogic countdowns
	float							m_flCPTimerTime;
	bool							m_bRedText;
	Color							m_cRegularColor;
	Color							m_cHighlightColor;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudControlPointIcons : public CHudElement, public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CHudControlPointIcons, vgui::Panel );

	CHudControlPointIcons( const char *pName );
	virtual ~CHudControlPointIcons( void );

	virtual void ApplySchemeSettings( IScheme *scheme );
	virtual void PerformLayout( void );
	virtual void Paint();
	virtual void Init();
	virtual void Reset();
	virtual bool IsVisible( void );
	virtual void LevelShutdown( void );

	virtual bool ShouldDraw( void )
	{
		return IsVisible();
	}

	virtual void FireGameEvent( IGameEvent *event );

	void UpdateProgressBarFor( int iIndex );
	void InitIcons();
	void ShutdownIcons();
	void DrawBackgroundBox( int xpos, int ypos, int nBoxWidth, int nBoxHeight, bool bCutCorner );
	bool PaintTeamBaseIcon( int index, float flXPos, float flYPos, float flIconSize );

	bool IsFakingCapture( int index = -1, bool *bMult = NULL, float *flFakeTime = NULL ) 
	{ 
		if ( m_bFakingCapture && m_flFakeCaptureTime < gpGlobals->curtime )
		{
			m_iCurrentCP = -1;
			m_bFakingCapture = false;
			m_bFakingCaptureMult = false;
		}

		if ( bMult) *bMult = m_bFakingCaptureMult; 
		if ( flFakeTime ) *flFakeTime = m_flFakeCaptureTime; 
		return (m_bFakingCapture && (index == -1 || index == m_iCurrentCP)); 
	}

private:
	int m_iCPTextures[CP_TEXTURE_COUNT];
	int m_iCPCappingTextures[CP_TEXTURE_COUNT];
	int m_iTeamBaseTextures[MAX_TEAMS];

	int m_iBackgroundTexture;
	Color m_clrBackground;
	Color m_clrBorder;

	int m_iCurrentCP;	// the index of the control point the local is currently in
	int m_iLastCP;		// the index of the control point the local player was last in

	// Capture faking for Intros
	float	m_flPulseTime;
	bool	m_bFakingCapture;
	bool	m_bFakingCaptureMult;
	float	m_flFakeCaptureTime;

	CPanelAnimationVar( vgui::HFont, m_hTextFont, "ChatFont", "Default" );

	CPanelAnimationVarAliasType( int, m_nCornerCutSize, "CornerCutSize", "5", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_nBackgroundOverlap, "BackgroundOverlap", "5", "proportional_int" );

	CPanelAnimationVarAliasType( int, m_iIconStartX, "icon_start_x", "10", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iIconStartY, "icon_start_y", "10", "proportional_int" );
	CPanelAnimationVarAliasType( float, m_flIconExpand, "icon_expand", "0", "proportional_float" );
	CPanelAnimationVarAliasType( int,	m_iIconSize, "iconsize", "24", "proportional_int" );


	CPanelAnimationVarAliasType( int,	m_iIconGapWidth, "separator_width", "7", "proportional_int" );
	CPanelAnimationVarAliasType( int,	m_iIconGapHeight, "separator_height", "9", "proportional_int" );
	CPanelAnimationVarAliasType( int,	m_nHeightOffset, "height_offset", "0", "proportional_int" );

	CUtlVector<CControlPointIcon *>		m_Icons;
};

#endif // HUD_CONTROLPOINTICONS_H
