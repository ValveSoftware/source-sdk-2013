//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Weapon selection handling
//
// $NoKeywords: $
//=============================================================================//
#if !defined( WEAPON_SELECTION_H )
#define WEAPON_SELECTION_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"

class C_BaseCombatWeapon;
class C_BasePlayer;

extern ConVar hud_fastswitch;

// weapon switch types for Convar hud_fastswitch
#define HUDTYPE_BUCKETS					0	// PC buckets
#define HUDTYPE_FASTSWITCH				1	// PC fastswitch
#define	HUDTYPE_PLUS					2	// console buckets
#define HUDTYPE_CAROUSEL				3	// console carousel scroll

//-----------------------------------------------------------------------------
// Purpose: Base class for tf2 & hl2 weapon selection hud elements
//-----------------------------------------------------------------------------
abstract_class CBaseHudWeaponSelection : public CHudElement
{
	DECLARE_CLASS( CBaseHudWeaponSelection, CHudElement );

public:
	CBaseHudWeaponSelection( const char *pElementName );
	virtual void Init( void );
	virtual void VidInit( void );
	virtual void ProcessInput();
	virtual void Reset(void);
	virtual void OnThink(void);

	virtual void OpenSelection( void );
	virtual void HideSelection( void );

	virtual void				CancelWeaponSelection( void );

	// Game specific overrides
	virtual void CycleToNextWeapon( void ) = 0;
	virtual void CycleToPrevWeapon( void ) = 0;
	virtual void SwitchToLastWeapon( void );
	virtual C_BaseCombatWeapon *GetWeaponInSlot( int iSlot, int iSlotPos ) = 0;
	virtual void SelectWeaponSlot( int iSlot ) = 0;
	virtual C_BaseCombatWeapon	*GetFirstPos( int iSlot );
	virtual C_BaseCombatWeapon	*GetNextActivePos( int iSlot, int iSlotPos );
	virtual void				SetWeaponSelected( void );
	virtual void				SelectWeapon( void );

	virtual C_BaseCombatWeapon	*GetSelectedWeapon( void ) = 0;

	virtual void OnWeaponPickup( C_BaseCombatWeapon *pWeapon );
	virtual bool IsInSelectionMode();

	void UserCmd_Slot1( void );
	void UserCmd_Slot2( void );
	void UserCmd_Slot3( void );
	void UserCmd_Slot4( void );
	void UserCmd_Slot5( void );
	void UserCmd_Slot6( void );
	void UserCmd_Slot7( void );
	void UserCmd_Slot8( void );
	void UserCmd_Slot9( void );
	void UserCmd_Slot0( void );
	void UserCmd_Slot10( void );
	void UserCmd_Close( void );
	void UserCmd_NextWeapon( void );
	void UserCmd_PrevWeapon( void );
	void UserCmd_LastWeapon( void );
	void UserCmd_DropPrimary( void );

	virtual void		SelectSlot( int iSlot );

	virtual bool IsHudMenuTakingInput();
	virtual bool IsHudMenuPreventingWeaponSelection();

	bool HandleHudMenuInput( int iSlot );

	static CBaseHudWeaponSelection *GetInstance();

	// these functions are exposed as virtual so that the tf_hints system can redraw the weapon selection
	virtual void DrawWList( C_BasePlayer *pPlayer, C_BaseCombatWeapon *pSelectedWeapon, bool drawOutline = false, int ora = 0, int og = 0, int ob = 0, int oa = 0 ) {}
	virtual bool ComputeRect( C_BasePlayer *pPlayer, C_BaseCombatWeapon *pSelectedWeapon, wrect_t *outrect ) { return false; }
	
	virtual int	KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

protected:
	// returns true if there is a weapon currently visible to select
	virtual bool IsWeaponSelectable()	{ return IsInSelectionMode(); }

	bool	CanBeSelectedInHUD( C_BaseCombatWeapon *pWeapon );

	void	UpdateSelectionTime( void );

	float	m_flSelectionTime;	// most recent time at which weapon selection had input

	static CBaseHudWeaponSelection *s_pInstance;

	bool	m_bSelectionVisible;

	CHandle< C_BaseCombatWeapon >	m_hSelectedWeapon;
};

// accessor
CBaseHudWeaponSelection *GetHudWeaponSelection();

#endif // WEAPON_SELECTION_H