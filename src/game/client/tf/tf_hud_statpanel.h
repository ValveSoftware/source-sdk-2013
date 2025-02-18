//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TFSTATPANEL_H
#define TFSTATPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include <vgui/IScheme.h>
#include "hud.h"
#include "hudelement.h"
#include "../game/shared/tf/tf_shareddefs.h"
#include "../game/shared/tf/tf_gamestats_shared.h"
#include "tf_hud_playerstatus.h"
#include "tf_hud_arena_class_layout.h"

using namespace vgui;

enum PlayerStatsVersions_t
{
	PLAYERSTATS_FILE_VERSION
};

enum RecordBreakType_t
{
	RECORDBREAK_NONE,
	RECORDBREAK_CLOSE,
	RECORDBREAK_TIE,
	RECORDBREAK_BEST,

	RECORDBREAK_MAX
};

class CTFStatPanel : public EditablePanel, public CHudElement
{
private:
	DECLARE_CLASS_SIMPLE( CTFStatPanel, EditablePanel );

public:
	CTFStatPanel( const char *pElementName );
	virtual ~CTFStatPanel();

	virtual void Reset();
	virtual void Init();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void FireGameEvent( IGameEvent * event );
	virtual void OnTick();
	virtual void LevelShutdown();
	void Show();
	void Hide();
	virtual bool ShouldDraw( void );

	void		ShowStatPanel( int iClass, int iTeam, int iCurStatValue, TFStatType_t statType, RecordBreakType_t recordBreakType,
								bool bAlive );
	void		TestStatPanel( TFStatType_t statType, RecordBreakType_t recordType );

	void		WriteStats( void );
	bool		ReadStats( void );
	int			CalcCRC( int iSteamID );
	void		ClearStatsInMemory( void );
	void		ResetStats( void );
	static ClassStats_t &GetClassStats( int iClass );
	static MapStats_t &GetMapStats( map_identifier_t iMapID );
	static bool	IsValidMapID( map_identifier_t iMapID );
	static const char* GetMapNameFromID( map_identifier_t iMapID );
	static float GetTotalHoursPlayed( void );		// Return the total time this player has played the game, in hours
	void		UpdateStatSummaryPanel();
	bool		IsLocalFileTrusted() { return m_bLocalFileTrusted; }
	void		SetStatsChanged( bool bChanged ) { m_bStatsChanged = bChanged; }

	void MsgFunc_PlayerStatsUpdate( bf_read &msg );
	void MsgFunc_MapStatsUpdate( bf_read &msg );

	virtual int GetRenderGroupPriority() { return 40; }	// less than winpanel, build menu

private:
	void		GetStatValueAsString( int iValue, TFStatType_t statType, char *value, int valuelen );
	void		UpdateStats( int iClass, const RoundStats_t &stats, bool bAlive );
	void		UpdateMapStats( map_identifier_t iMapID, const RoundMapStats_t &stats );
	void		ResetDisplayedStat();

	int							m_iCurStatValue;			// the value of the currently displayed stat
	int							m_iCurStatClass;			// the player class for current stat
	int							m_iCurStatTeam;				// the team of current stat
	TFStatType_t				m_statType;					// type of current stat
	RecordBreakType_t			m_recordBreakType;			// was record broken, tied, or just close
	bool						m_bDisplayAfterSpawn;		// should we display after player respawns
	float						m_flTimeLastSpawn;
	float						m_flTimeHide;				// time at which to hide the panel

	CUtlVector<ClassStats_t>	m_aClassStats;
	CUtlVector<MapStats_t>		m_aMapStats;
	bool						m_bStatsChanged;
	bool						m_bLocalFileTrusted;		// do we believe our local stats data file has not been tampered with
	CTFClassImage				*m_pClassImage;

	bool						m_bShouldBeVisible;
};

CTFStatPanel *GetStatPanel();

#endif //TFSTATPANEL_H
