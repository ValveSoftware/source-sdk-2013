//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#ifndef TF_BADGE_PANEL_H
#define TF_BADGE_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/EditablePanel.h"
#include "tf_progression_description.h"
#include "local_steam_shared_object_listener.h"

class IProgressionDesc;

class CTFBadgePanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFBadgePanel, vgui::EditablePanel );
public:
	CTFBadgePanel( vgui::Panel *pParent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;

	void SetupDummyBadge( uint32 nLevel, bool bInPlacement );

	void SetupBadge( const IMatchGroupDescription* pMatchDesc, const LevelInfo_t& levelInfo, const CSteamID& steamID, bool bInPlacement );
	void SetupBadge( const IMatchGroupDescription* pMatchDesc, const LevelInfo_t& levelInfo, const CSteamID& steamID );
	void SetupBadge( const IMatchGroupDescription* pMatchDesc, const CSteamID& steamID );

private:
	class CModelImagePanel *m_pBadgePanel;
	uint32 m_nPrevLevel;
	bool m_bPreviouslyInPlacement = false;
};

class CTFLocalPlayerBadgePanel : public CTFBadgePanel
							   , public CLocalSteamSharedObjectListener
{
public:
	DECLARE_CLASS_SIMPLE( CTFLocalPlayerBadgePanel, CTFBadgePanel );
	CTFLocalPlayerBadgePanel( vgui::Panel *pParent, const char *pName );

	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;

	virtual void SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;

	void SetMatchGroup( ETFMatchGroup eMatchGroup );

private:
	void UpdateBadge();
	ETFMatchGroup m_eMatchGroup = k_eTFMatchGroup_Invalid;
};

#endif // TF_BADGE_PANEL_H
