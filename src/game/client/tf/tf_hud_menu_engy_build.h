//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_MENU_ENGY_BUILD_H
#define TF_HUD_MENU_ENGY_BUILD_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include "IconPanel.h"
#include "tf_controls.h"
#include "hudelement.h"
#include "tf_hud_base_build_menu.h"

using namespace vgui;

#define ALL_BUILDINGS	-1
#define NUM_ENGY_BUILDINGS 4

enum buildmenulayouts_t
{
	BUILDMENU_DEFAULT = 0,
	BUILDMENU_PIPBOY,
};

struct EngyConstructBuilding_t
{
	EngyConstructBuilding_t() {}

	EngyConstructBuilding_t( bool bEnabled,
							 ObjectType_t type,
							 int iMode, 
							 const char *pszConstructAvailableObjectRes,
							 const char *pszConstructAlreadyBuiltObjectRes,
							 const char *pszConstructCantAffordObjectRes,
							 const char *pszConstructUnavailableObjectRes,
							 const char *pszDestroyActiveObjectRes,
							 const char *pszDestroyInactiveObjectRes,
							 const char *pszDestroyUnavailableObjectRes )
							 : m_bEnabled( bEnabled )
							 , m_iObjectType ( type )
							 , m_iMode( iMode )
							 , m_pszConstructAvailableObjectRes( pszConstructAvailableObjectRes )
							 , m_pszConstructAlreadyBuiltObjectRes( pszConstructAlreadyBuiltObjectRes )
							 , m_pszConstructCantAffordObjectRes( pszConstructCantAffordObjectRes )
							 , m_pszConstructUnavailableObjectRes( pszConstructUnavailableObjectRes )
							 , m_pszDestroyActiveObjectRes( pszDestroyActiveObjectRes )
							 , m_pszDestroyInactiveObjectRes( pszDestroyInactiveObjectRes )
							 , m_pszDestroyUnavailableObjectRes( pszDestroyUnavailableObjectRes )
	{}

	bool m_bEnabled;
	ObjectType_t m_iObjectType;
	int m_iMode;
	// Construction panels
	const char *m_pszConstructAvailableObjectRes;
	const char *m_pszConstructAlreadyBuiltObjectRes;
	const char *m_pszConstructCantAffordObjectRes;
	const char *m_pszConstructUnavailableObjectRes;
	// Destruction panels
	const char *m_pszDestroyActiveObjectRes;
	const char *m_pszDestroyInactiveObjectRes;
	const char *m_pszDestroyUnavailableObjectRes;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct EngyBuildingReplacement_t
{
	EngyBuildingReplacement_t(	ObjectType_t type,
								int iMode,
								const char *strConstructionAvailable,
								const char *strConstructionAlreadyBuilt,
								const char *strConstructionCantAfford,
								const char *strConstructionUnavailable,
								const char *strDestructionActive,
								const char *strDestructionInactive,
								const char *strDestructionUnavailable,
								int iReplacementSlots,
								int iDisableSlots
								)
								: m_building( true,
											  type,
											  iMode,
											  strConstructionAvailable,
											  strConstructionAlreadyBuilt,
											  strConstructionCantAfford,
											  strConstructionUnavailable,
											  strDestructionActive,
											  strDestructionInactive,
											  strDestructionUnavailable )

	{
		m_iReplacementSlots = iReplacementSlots;
		m_iDisableSlots = iDisableSlots;
	}

	EngyConstructBuilding_t m_building;
	int m_iReplacementSlots;
	int m_iDisableSlots;
};

extern const EngyConstructBuilding_t g_kEngyBuildings[];
	 
class CHudMenuEngyBuild : public CHudBaseBuildMenu
{
	DECLARE_CLASS_SIMPLE( CHudMenuEngyBuild, EditablePanel );

public:
	CHudMenuEngyBuild( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );

	virtual void	SetVisible( bool state );

	virtual void	OnTick( void );

	int	HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	virtual int GetRenderGroupPriority() { return 50; }

	static buildmenulayouts_t CalcCustomBuildMenuLayout( void );

	static void ReplaceBuildings( EngyConstructBuilding_t (&targetBuildings)[ NUM_ENGY_BUILDINGS ] );
	static void GetBuildingIDAndModeFromSlot( int iSlot, int &iBuilding, int &iMode, const EngyConstructBuilding_t (&buildings)[ NUM_ENGY_BUILDINGS ] );

	virtual GameActionSet_t GetPreferredActionSet() { return IsActive() ? GAME_ACTION_SET_IN_GAME_HUD : GAME_ACTION_SET_NONE; }

private:

	void SendBuildMessage( int iSlot );
	bool SendDestroyMessage( int iSlot );

	void SetSelectedItem( int iSlot );

	void UpdateHintLabels( void );	// show/hide the bright and dim build, destroy hint labels

	void InitBuildings();

	bool CanBuild( int iSlot );

	EditablePanel *m_pAvailableObjects[NUM_ENGY_BUILDINGS];
	EditablePanel *m_pAlreadyBuiltObjects[NUM_ENGY_BUILDINGS];
	EditablePanel *m_pCantAffordObjects[NUM_ENGY_BUILDINGS];
	EditablePanel *m_pUnavailableObjects[NUM_ENGY_BUILDINGS];

	// 360 layout only
	CIconPanel *m_pActiveSelection;

	int m_iSelectedItem;

	CExLabel *m_pBuildLabelBright;
	CExLabel *m_pBuildLabelDim;

	CExLabel *m_pDestroyLabelBright;
	CExLabel *m_pDestroyLabelDim;

	bool m_bInConsoleMode;

	buildmenulayouts_t		m_eCurrentBuildMenuLayout;

	EngyConstructBuilding_t m_Buildings[NUM_ENGY_BUILDINGS];
};


#endif	// TF_HUD_MENU_ENGY_BUILD_H
