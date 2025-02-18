//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef C_TF_ITEMEFFECTMETER_H
#define C_TF_ITEMEFFECTMETER_H

#include "cbase.h"

#include "c_tf_player.h"
#include "c_tf_playerclass.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "tf_weapon_invis.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/Label.h>
#include "tf_imagepanel.h"

using namespace vgui;

class CHudItemEffectMeter;
class CItemEffectMeterLogic;
class CItemEffectMeterManager;

extern CItemEffectMeterManager g_ItemEffectMeterManager;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CItemEffectMeterManager : public CGameEventListener
{
public:
	~CItemEffectMeterManager();

	void			ClearExistingMeters();
	void			SetPlayer( C_TFPlayer* pPlayer );
	void			Update( C_TFPlayer* pPlayer );
	virtual void	FireGameEvent( IGameEvent *event );
	int				GetNumEnabled( void );

private:
	CUtlVector< vgui::DHANDLE< CHudItemEffectMeter > >	m_Meters;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
DECLARE_AUTO_LIST( IHudItemEffectMeterAutoList );
class CHudItemEffectMeter : public CHudElement, public EditablePanel, public IHudItemEffectMeterAutoList
{
	DECLARE_CLASS_SIMPLE( CHudItemEffectMeter, EditablePanel );

public:
	CHudItemEffectMeter( const char *pszElementName, C_TFPlayer* pPlayer );
	~CHudItemEffectMeter();

	static void		CreateHudElementsForClass( C_TFPlayer* pPlayer, CUtlVector< vgui::DHANDLE< CHudItemEffectMeter > >& outMeters );

	// Hud Element
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual void	PerformLayout() OVERRIDE;
	virtual bool	ShouldDraw( void );
	virtual void	Update( C_TFPlayer* pPlayer );

	// Effect Meter Logic
	virtual bool		IsEnabled( void )		{ return m_bEnabled; }
	virtual const char*	GetLabelText( void );
	virtual const char*	GetIconName( void )		{ return "../hud/ico_stickybomb_red"; }
	virtual float		GetProgress( void );
	virtual bool		ShouldBeep( void )
	{ 
		if ( m_pPlayer && m_pPlayer->IsPlayerClass( TF_CLASS_SPY ) )
		{
			CTFWeaponInvis *pWpn = (CTFWeaponInvis *) m_pPlayer->Weapon_OwnsThisID( TF_WEAPON_INVIS );
			if ( pWpn && pWpn->HasFeignDeath() )
				return true;
		}
		
		return false;
	}
	virtual const char*	GetBeepSound( void )		{ return "TFPlayer.ReCharged"; }
	virtual const char *GetResFile( void )			{ return "resource/UI/HudItemEffectMeter.res"; }
	virtual int			GetCount( void )			{ return -1; }
	virtual bool		ShouldFlash( void )			{ return false; }
	virtual bool		ShowPercentSymbol( void )	{ return false; }

	virtual int			GetNumProgressBar( void ) const { return 1; }
	virtual Color		GetProgressBarColor( void )	{ return Color( 255, 255, 255, 255 ); }
	virtual Color		GetLabelTextColor( void )	{ return Color( 255, 255, 255, 255 ); }

	// Override this to update some field on the panel when state changes
	virtual int			GetState( void )			{ return -1; }

	virtual bool		IsKillstreakMeter( void ) { return false; }

	virtual void		SetLabelText( const char *pszText = NULL );

	virtual bool		ShouldAutoAdjustPosition() const { return true; }

protected:
	vgui::Label *m_pLabel;
	CUtlVector< vgui::ContinuousProgressBar* > m_vecProgressBars;
	float				m_flOldProgress;

	CHandle<C_TFPlayer>	m_pPlayer;
	bool				m_bEnabled;

	CTFImagePanel		*m_pItemEffectIcon;

	int					m_nState;

	CPanelAnimationVarAliasType( float, m_iXOffset, "x_offset", "0", "proportional_float" );
};

//-----------------------------------------------------------------------------
// Purpose: Template variation for weapon based meters.
//-----------------------------------------------------------------------------
template <class T>
class CHudItemEffectMeter_Weapon : public CHudItemEffectMeter
{
public:
	CHudItemEffectMeter_Weapon( const char *pszElementName, C_TFPlayer *pPlayer, int iWeaponID, bool bBeeps=true, const char* pszResFile=NULL );

	T*					GetWeapon( void );

	virtual void		PerformLayout() OVERRIDE { CHudItemEffectMeter::PerformLayout(); }

	virtual void		Update( C_TFPlayer *pPlayer ) OVERRIDE;

	// Effect Meter Logic
	virtual bool		IsEnabled( void );
	virtual const char*	GetLabelText( void ) { return m_hWeapon ? m_hWeapon->GetEffectLabelText() : ""; }
	virtual const char*	GetIconName( void ) { return "../hud/ico_stickybomb_red"; }
	virtual float		GetProgress( void );
	virtual bool		ShouldBeep( void ) { return m_bBeeps; }
	virtual const char*	GetBeepSound( void ) OVERRIDE { return CHudItemEffectMeter::GetBeepSound(); }
	virtual const char *GetResFile( void );
	virtual int			GetCount( void ) { return -1; }
	virtual bool		ShouldFlash( void ) { return false; }

	virtual int			GetNumProgressBar( void ) const OVERRIDE { return 1; }
	virtual Color		GetProgressBarColor( void ) OVERRIDE { return Color( 255, 255, 255, 255 ); }
	virtual Color		GetLabelTextColor( void ) OVERRIDE { return Color( 255, 255, 255, 255 ); }
	virtual int			GetState( void ) OVERRIDE { return -1; }

	virtual bool		ShouldDraw( void );
	virtual bool		ShowPercentSymbol( void )	{ return false; }
	virtual bool		IsKillstreakMeter( void )	{ return false; }

	virtual bool		ShouldAutoAdjustPosition() const OVERRIDE { return true; }

private:
	CHandle<T>			m_hWeapon;
	int					m_iWeaponID;
	bool				m_bBeeps;
	const char*			m_pszResFile;
};

class CHudItemEffectMeter_Rune : public CHudItemEffectMeter
{
public:

	CHudItemEffectMeter_Rune( const char *pszElementName, C_TFPlayer *pPlayer );

	// Effect Meter Logic
	virtual bool		IsEnabled( void );
	virtual float		GetProgress( void );
	virtual bool		ShouldDraw( void );

	virtual const char*	GetLabelText( void ) { return "Powerup"; }
	virtual const char *GetResFile( void )		{ return "resource/UI/HudPowerupEffectMeter.res"; }

	virtual bool		ShouldFlash( void );
};


class CHudItemEffectMeter_ItemAttribute : public CHudItemEffectMeter
{
public:
	CHudItemEffectMeter_ItemAttribute( const char *pszElementName, C_TFPlayer *pPlayer, loadout_positions_t iLoadoutSlot, const char *pszLabelText = NULL, bool bBeeps = true );

	const IHasGenericMeter	*GetItem();

	// Effect Meter Logic
	virtual const char*	GetLabelText(void) OVERRIDE { return m_strLabelText.Get() ? m_strLabelText.Get() : ""; }
	virtual float		GetProgress( void ) OVERRIDE;
	virtual bool		ShouldBeep( void ) OVERRIDE { return m_bBeeps; }
	virtual bool		ShouldDraw(void) OVERRIDE;
	virtual void		OnTick( void ) OVERRIDE;

private:
	CHandle< CBaseEntity >	m_hEntity;
	const IHasGenericMeter* m_pMeterEntity;
	loadout_positions_t		m_iLoadoutSlot;
	CUtlString				m_strLabelText;
	bool					m_bBeeps;
};

#endif
