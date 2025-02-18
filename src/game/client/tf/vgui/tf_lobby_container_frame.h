//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef TF_LOBBY_CONTAINER_FRAME_H
#define TF_LOBBY_CONTAINER_FRAME_H


#include "cbase.h"
#include "vgui_controls/PropertyDialog.h"
#include "GameEventListener.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

bool BIsPartyInUIState();

class CBaseLobbyPanel;

// This is a big fat kludge so I can use the PropertyPage
class CBaseLobbyContainerFrame : public vgui::PropertyDialog, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CBaseLobbyContainerFrame, PropertyDialog );
public:
	CBaseLobbyContainerFrame( const char *pszPanelName );
	virtual ~CBaseLobbyContainerFrame();

	//
	// PropertyDialog overrides
	//
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout( void ) OVERRIDE;
	virtual void OnKeyCodeTyped(vgui::KeyCode code) OVERRIDE;
	virtual void OnKeyCodePressed(vgui::KeyCode code) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;

	//
	// CGameEventListener overrides
	//
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

	void StartSearch( void );
	virtual void ShowPanel(bool bShow);
	void SetNextButtonEnabled( bool bValue );

	virtual void OnThink() OVERRIDE;

	static void LeaveLobbyPanel( bool bConfirmed, void *pContext )
	{
		CBaseLobbyContainerFrame* pCaller = (CBaseLobbyContainerFrame*)pContext;
		if ( bConfirmed && pCaller )
		{
			pCaller->OnCommand( "back" );
		}
	}

	MESSAGE_FUNC( OpenPingOptions, "Context_Ping" );

	// What is this panel good for?
	virtual bool CanHandleMatchGroup( ETFMatchGroup eMatchGroup ) const = 0;
	// Helper that checks against the currently selected party matchgroup
	bool CanHandleCurrentMatchGroup() const;

protected:
	bool ShouldShowPartyButton() const;
	virtual void WriteControls();
	virtual void HandleBackPressed();
	void OpenOptionsContextMenu();

	CBaseLobbyPanel *m_pContents;
	vgui::Button *m_pNextButton;
	vgui::Button *m_pBackButton;

private:
	virtual const char* GetResFile() const = 0;
	virtual bool VerifyPartyAuthorization() const = 0;


	vgui::Button *m_pStartPartyButton;
	vgui::Menu *m_pContextMenu;

	bool m_bNextButtonEnabled;

	vgui::DHANDLE< vgui::Panel > m_hPingPanel;
};

#endif //TF_LOBBY_CONTAINER_FRAME_H
