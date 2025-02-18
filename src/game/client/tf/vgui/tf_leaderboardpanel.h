#ifndef TF_LEADERBOARDPANEL_H
#define TF_LEADERBOARDPANEL_H

#include "vgui_controls/EditablePanel.h"
#include "tf_wardata.h"
#include "vgui_controls/ProgressBar.h"
#include "quest_log_panel.h"
#include "tf_asyncpanel.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFLeaderboardPanel : public CBaseASyncPanel
{
	DECLARE_CLASS_SIMPLE( CTFLeaderboardPanel, CBaseASyncPanel );
public:
	CTFLeaderboardPanel( Panel *pParent, const char *pszPanelName );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData );

protected:
	virtual bool GetLeaderboardData( CUtlVector< LeaderboardEntry_t* >& scores ) = 0;
	virtual bool UpdateLeaderboards();
	virtual bool CheckForData_Internal() OVERRIDE;

	CUtlVector< EditablePanel* > m_vecLeaderboardEntries;

	CPanelAnimationVarAliasType( int, m_yEntryStep, "entry_step", "5", "proportional_int");
	Color m_EvenTextColor;
	Color m_OddTextColor;
	Color m_LocalPlayerTextColor;
};

#endif //TF_LEADERBOARDPANEL_H
