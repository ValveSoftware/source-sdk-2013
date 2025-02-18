//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef TF_LOBBY_CONTAINER_FRAME_MVM_H
#define TF_LOBBY_CONTAINER_FRAME_MVM_H


#include "cbase.h"
//#include "tf_pvelobbypanel.h"
#include "tf_lobby_container_frame.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


class CBaseLobbyPanel;

// This is a big fat kludge so I can use the PropertyPage
class CLobbyContainerFrame_MvM : public CBaseLobbyContainerFrame
{
	DECLARE_CLASS_SIMPLE( CLobbyContainerFrame_MvM, CBaseLobbyContainerFrame );
public:
	CLobbyContainerFrame_MvM();
	~CLobbyContainerFrame_MvM();

	//
	// PropertyDialog overrides
	//
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void OnKeyCodePressed(vgui::KeyCode code) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;

	static bool TypeCanHandleMatchGroup( ETFMatchGroup eMatchGroup );
	virtual bool CanHandleMatchGroup( ETFMatchGroup eMatchGroup ) const OVERRIDE
	{
		return TypeCanHandleMatchGroup( eMatchGroup );
	}

private:

	virtual const char* GetResFile() const OVERRIDE { return "Resource/UI/LobbyContainerFrame_MvM.res"; }
	virtual bool VerifyPartyAuthorization() const OVERRIDE;
	virtual void WriteControls() OVERRIDE;
	virtual void HandleBackPressed() OVERRIDE;

	vgui::Button *m_pStartPartyButton;
	vgui::Button *m_pPlayNowButton;
	vgui::Button *m_pPracticeButton;
};

#endif //TF_LOBBY_CONTAINER_FRAME_MVM_H
