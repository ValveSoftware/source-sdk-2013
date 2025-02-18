//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#ifndef TF_PVP_RANK_PANEL_H
#define TF_PVP_RANK_PANEL_H

#include "cbase.h"
#include "vgui_controls/EditablePanel.h"
#include "tf_match_description.h"
#include "GameEventListener.h"
#include "local_steam_shared_object_listener.h"
#include "tf_progression_description.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

class CBaseModelPanel;
namespace vgui
{
	class ContinuousProgressBar;
	class Button;
};

namespace GCSDK
{
	class CSharedObject;
};

using namespace GCSDK;

class CPvPRankPanel : public vgui::EditablePanel, public CLocalSteamSharedObjectListener, public CGameEventListener
{
public:
	DECLARE_CLASS_SIMPLE( CPvPRankPanel, vgui::EditablePanel );

	CPvPRankPanel( Panel *parent, const char *panelName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual void OnThink() OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;
	virtual void FireGameEvent( IGameEvent *pEvent ) OVERRIDE;
	virtual void SetVisible( bool bVisible ) OVERRIDE;

	void SetMatchGroup( ETFMatchGroup eMatchGroup );
	void SetMatchStats( void );

	virtual void SOCreated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE;
	virtual void SOUpdated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE;

	MESSAGE_FUNC_PARAMS( OnAnimEvent, "AnimEvent", pParams );

protected:

	virtual void PlayLevelUpEffects( const LevelInfo_t& level ) const;
	virtual void PlayLevelDownEffects( const LevelInfo_t& level ) const;

private:

	struct RatingState_t : public CGameEventListener, public CLocalSteamSharedObjectListener
	{
		RatingState_t( ETFMatchGroup eMatchGroup );

		virtual void FireGameEvent( IGameEvent *pEvent ) OVERRIDE;
		// Get the current rating value
		uint32 GetCurrentRating() const;
		// Get the previous rating value before any changes occurred
		uint32 GetStartRating() const { return m_nStartRating; }
		// Get the target rating that the delta lerp is headed to
		uint32 GetTargetRating() const { return m_nTargetRating; }
		uint32 GetActualRating() const { return m_nActualRating; }
		// Start the timer that causes GetCurrentXP to lerp from m_nStartXP to m_nTargetXP
		bool BeginRatingDeltaLerp();

		virtual void SOCreated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE;
		virtual void SOUpdated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE;

		bool IsInitialized() const { return m_bInitialized; }

	private:

		void UpdateRating( bool bInitial );

		const ETFMatchGroup m_eMatchGroup;
		uint32 m_nStartRating;	// The rating we show to the user that was the last state they saw
		uint32 m_nTargetRating;	// The rating the user believes is their current value
		uint32 m_nActualRating;	// The actual latest rating value
		RealTimeCountdownTimer m_progressTimer;
		bool m_bCurrentDeltaViewed;
		bool m_bInitialized;
	};

	RatingState_t& GetRatingState() const;
	void UpdateRankControls( const LevelInfo_t& levelCurrent );
	void UpdateRatingControls( uint32 nPreviousRating, uint32 nCurrentRating, const LevelInfo_t& levelCurrent );
	void UpdateBaseState();

	virtual const char* GetResFile() const;
	virtual KeyValues* GetConditions() const;
	void BeginRatingLerp();
	const LevelInfo_t& GetLevel( bool bCurrent ) const;
	bool BIsInPlacement() const;

	ETFMatchGroup m_eMatchGroup;
	EditablePanel* m_pModelContainer;
	vgui::ContinuousProgressBar* m_pContinuousProgressBar;
	vgui::EditablePanel* m_pXPBar;
	CBaseModelPanel* m_pModelPanel;
	const IMatchGroupDescription* m_pMatchDesc = NULL;
	const IProgressionDesc* m_pProgressionDesc;
	EditablePanel* m_pBGPanel;
	vgui::Button* m_pModelButton;
	bool m_bShowRating = false;
	bool m_bRevealingRank = false;

	uint32 m_nLastLerpRating;
	uint32 m_nLastSeenLevel;
	bool m_bInitializedBaseState;

	CPanelAnimationVar( bool, m_bShowModel, "show_model", "1" );
	CPanelAnimationVar( bool, m_bShowName, "show_name", "1" );
	CPanelAnimationVar( bool, m_bShowType, "show_type", "0" );
	CPanelAnimationVar( bool, m_bShowProgress, "show_progress", "1" );
	CPanelAnimationVar( bool, m_bShowSourcesWhenHidden, "show_sources_when_hidden", "0" );
	CPanelAnimationVar( bool, m_bInstantlyUpdate, "instantly_update", "1" );
};

#endif //TF_PVP_RANK_PANEL_H
