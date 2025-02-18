#ifndef TF_WARINFOPANEL_H
#define TF_WARINFOPANEL_H


#include "vgui_controls/EditablePanel.h"
#include "tf_wardata.h"
#include "vgui_controls/ProgressBar.h"
#include "local_steam_shared_object_listener.h"

using namespace vgui;
using namespace GCSDK;

class CExLabel;

class CWarStandingPanel : public EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CWarStandingPanel, EditablePanel );
public:
	CWarStandingPanel( Panel* pParent, const char* pszPanelname );

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void OnThink() OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;
private:

	float GetPercentAnimated() const;

	struct TeamScore_t
	{
		TeamScore_t()
			: m_nLastScore( 0 )
			, m_nNewScore( 0 )
			, m_pTeamProgressBar( NULL )
			, m_pContainerPanel( NULL )
			, m_pTeamLabel( NULL )
			, m_pScoreLabel( NULL )
		{}
		int m_nLastScore;
		int m_nNewScore;
		ContinuousProgressBar	*m_pTeamProgressBar;
		EditablePanel* m_pContainerPanel;
		CExLabel* m_pTeamLabel;
		CExLabel* m_pScoreLabel;
	};

	bool m_bNeedsLerp;
	TeamScore_t m_Scores[2];
	float m_flLastUpdateTime;
	ContinuousProgressBar	*m_pTeam0ProgressBar;
	ContinuousProgressBar	*m_pTeam1ProgressBar;
	CUtlString				m_strWarName;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CWarLandingPanel : public EditablePanel, public CLocalSteamSharedObjectListener
{
	DECLARE_CLASS_SIMPLE( CWarLandingPanel, EditablePanel );
public:
	CWarLandingPanel( Panel *pParent, const char *pszPanelName );

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void OnCommand( const char *pCommand ) OVERRIDE;
	virtual void OnThink() OVERRIDE;
	virtual void PerformLayout() OVERRIDE;

	virtual void SOCreated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) { UpdateWarStatus( steamIDOwner, pObject, eEvent ); }
	virtual void SOUpdated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) { UpdateWarStatus( steamIDOwner, pObject, eEvent ); }
	virtual void SODestroyed( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) { UpdateWarStatus( steamIDOwner, pObject, eEvent ); }

	virtual void SetVisible( bool bVisible ) OVERRIDE;

private:

	enum EJoiningState_t
	{
		NO_ACTION = 0,
		CONFIRM_SIDE_SELECTION,
		ATTEMPTING_TO_JOIN_AND_WAITING_FOR_RESPONSE,
		SUCCESS_RESPONSE_RECIEVED_WAITING_FOR_USER_CONFIRMATION,
		FAILED_RESPONSE_RECIEVED_WAITING_FOR_USER_CONFIRMATION,
	};

	void UpdateWarStatus( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent );

	void UpdateUIState();

	float m_flChoseTeamTime;
	war_side_t m_nPendingSide;
	war_side_t m_nLastKnownSide;
	CUtlString m_strSceneAnimName;

	EJoiningState_t m_eJoiningState;
};

#endif //TF_WARINFOPANEL_H
