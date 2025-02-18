#ifndef TF_ASYNCPANEL
#define TF_ASYNCPANEL

#include "vgui_controls/EditablePanel.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBaseASyncPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CBaseASyncPanel, EditablePanel );
public:
	CBaseASyncPanel( Panel *pParent, const char *pszPanelName );
	virtual ~CBaseASyncPanel() {}

	bool IsInitialized() const;
	void CheckForData();
	virtual void OnTick() OVERRIDE;
	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual void LoadControlSettings(const char *dialogResourceName, const char *pathID = NULL, KeyValues *pPreloadedKeyValues = NULL, KeyValues *pConditions = NULL) OVERRIDE;
protected:
	virtual void OnChildSettingsApplied( KeyValues *pInResourceData, Panel *pChild ) OVERRIDE;

private:
	void PresentDataIfReady();
	virtual bool CheckForData_Internal() = 0;

	bool m_bDataInitialized;
	bool m_bSettingsApplied;
	float m_flLastRequestTime;
	float m_flLastUpdatedTime;
	CUtlVector< PHandle > m_vecLoadingPanels;
	CUtlVector< PHandle > m_vecPanelsToShow;
	CPanelAnimationVar( float, m_flRefreshDelay, "refresh_delay", "-1.f" );
};

#endif //TF_ASYNCPANEL
