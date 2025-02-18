//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_STATSSUMMARY_H
#define TF_STATSSUMMARY_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_hud_statpanel.h"
#include "GameEventListener.h"

struct ClassDetails_t
{
	TFStatType_t statType;			// type of stat
	uint			 iFlagsClass;		// bit mask of classes to show this stat for
	const char * szResourceName;	// name of label resource (format "Most damage:"
	const char * szAltResourceName;	// name of alternative label resource "damage"
};
extern ClassDetails_t g_PerClassStatDetails[15];

#define MAKESTATFLAG(x)	( 1 << x )
#define ALL_CLASSES		0xFFFFFFFF

class CTFStatsSummaryPanel : public vgui::EditablePanel, public CGameEventListener
{
private:
	DECLARE_CLASS_SIMPLE( CTFStatsSummaryPanel, vgui::EditablePanel );

public:
	CTFStatsSummaryPanel();	 
	CTFStatsSummaryPanel( vgui::Panel *parent );
	~CTFStatsSummaryPanel();	 

	void Init( void );

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnCommand( const char *command );
	virtual void OnKeyCodePressed( KeyCode code );
	virtual void PerformLayout();
	virtual void OnThink();
	void SetStats( CUtlVector<ClassStats_t> &vecClassStats );
	void ShowModal();

	void SetupForEmbedded( void );

	void OnMapLoad( const char *pMapName );

	virtual void FireGameEvent( IGameEvent *event );
private:
	MESSAGE_FUNC( OnActivate, "activate" );
	MESSAGE_FUNC( OnDeactivate, "deactivate" );

	enum StatDisplay_t
	{
		SHOW_MAX = 1,
		SHOW_TOTAL,
		SHOW_AVG
	};

	void Reset();
	void SetDefaultSelections();
	void UpdateDialog();
	void UpdateBarCharts();
	void UpdateClassDetails( bool bIsMVM = false );
	void UpdateTip();
	void UpdateControls();
	void ClearMapLabel();
	void ShowMapInfo( bool bShowMapInfo, bool bIsMVM = false, bool bBackgroundOverride = false );
	void UpdateLeaderboard();
	void InitBarChartComboBox( vgui::ComboBox *pComboBox );
	void SetValueAsClass( const char *pDialogVariable, int iValue, int iPlayerClass );
	void DisplayBarValue( int iChart, int iClass, ClassStats_t &stats, TFStatType_t statType, StatDisplay_t flags, float flMaxValue );	
	static float GetDisplayValue( ClassStats_t &stats, TFStatType_t statType, StatDisplay_t statDisplay );
	const char *RenderValue( float flValue, TFStatType_t statType, StatDisplay_t statDisplay );
	static float SafeCalcFraction( float flNumerator, float flDemoninator );
	static int __cdecl CompareClassStats( const ClassStats_t *pStats0, const ClassStats_t *pStats1 );
	MESSAGE_FUNC( DoResetStats, "DoResetStats" );
	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", data );

	vgui::EditablePanel *m_pPlayerData;

	vgui::EditablePanel *m_pInteractiveHeaders;
	vgui::EditablePanel *m_pNonInteractiveHeaders;
	vgui::ComboBox *m_pBarChartComboBoxA;
	vgui::ComboBox *m_pBarChartComboBoxB;
	vgui::ComboBox *m_pClassComboBox;
	CTFImagePanel	*m_pTipImage;
	vgui::Label		*m_pTipText;
	vgui::EditablePanel *m_pMapInfoPanel;
	vgui::Panel		*m_pLeaderboardTitle;

	vgui::ImagePanel *m_pMainBackground;
	void UpdateMainBackground( void ); 

	vgui::EditablePanel	*m_pContributedPanel;

#ifdef _X360
	CTFFooter		*m_pFooter;
#else
	vgui::Button *m_pNextTipButton;
	vgui::Button *m_pCloseButton;
	vgui::Button *m_pResetStatsButton;
#endif

	bool m_bInteractive;							// are we in interactive mode
	bool m_bEmbedded;								// are we embedded in a property sheet?
	bool m_bControlsLoaded;							// have we loaded controls yet
	CUtlVector<ClassStats_t> m_aClassStats;			// stats data
	int m_xStartLHBar;								// x min of bars in left hand bar chart
	int m_xStartRHBar;								// x min of bars in right hand bar chart
	int m_iBarMaxWidth;								// width of bars in bar charts
	int m_iBarHeight;								// height of bars in bar charts

	int m_iSelectedClass;							// what class is selected, if any
	int m_iTotalSpawns;								// how many spawns of all classes does this player have
	TFStatType_t m_statBarGraph[2];					// what stat is displayed in the left hand and right hand bar graphs
	StatDisplay_t m_displayBarGraph[2];				// the display type for the left hand and right hand bar graphs

	bool m_bShowingLeaderboard;
	bool m_bLoadingCommunityMap;
	int m_xStartLeaderboard;
	int m_yStartLeaderboard;
	CUtlVector< vgui::EditablePanel* > m_vecLeaderboardEntries;

#ifdef _X360
	bool m_bShowBackButton;
#endif
};


CTFStatsSummaryPanel *GStatsSummaryPanel();
void DestroyStatsSummaryPanel();
const char *FormatSeconds( int seconds );

void UpdateStatSummaryPanels( CUtlVector<ClassStats_t> &vecClassStats );

#endif // TF_STATSSUMMARY_H
