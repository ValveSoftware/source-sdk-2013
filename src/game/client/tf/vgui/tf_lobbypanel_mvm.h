//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#ifndef TF_LOBBYPANEL_MVM_H
#define TF_LOBBYPANEL_MVM_H


#include "cbase.h"
#include "game/client/iviewport.h"
#include "vgui_bitmapimage.h"
#include "tf_lobbypanel.h"
#include "tf_mvm_criteria.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

class CMVMCriteriaPanel;

class CLobbyPanel_MvM : public CBaseLobbyPanel
{
	DECLARE_CLASS_SIMPLE( CLobbyPanel_MvM, CBaseLobbyPanel );
public:
	CLobbyPanel_MvM( vgui::Panel *pParent, CBaseLobbyContainerFrame* pLobbyContainer );
	virtual ~CLobbyPanel_MvM() {}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;

	virtual void ApplyChatUserSettings( const LobbyPlayerInfo& player, KeyValues* pSettings ) const OVERRIDE;
	virtual EMatchGroup GetMatchGroup( void ) const OVERRIDE;
	virtual const char* GetResFile() const OVERRIDE { return "Resource/UI/LobbyPanel_MvM.res"; } ;
	void WriteGameSettingsControls() OVERRIDE;
	virtual bool ShouldShowLateJoin() const OVERRIDE;


private:

	CPanelAnimationVarAliasType( int, m_iHasTicketWidth, "has_ticket_width", "12", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iSquadSurplusWidth, "squad_surplus_width", "12", "proportional_int" );

	int m_iImageNoTicket;
	int m_iImageHasTicket;
	int m_iImageNoSquadSurplus;
	int m_iImageSquadSurplus;
	CMVMCriteriaPanel* m_pCriteria;
};

#endif // TF_LOBBYPANEL_MVM_H
