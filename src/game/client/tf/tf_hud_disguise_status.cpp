//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_tf_player.h"
#include "c_playerresource.h"
#include "c_tf_playerresource.h"
#include "tf_classdata.h"
#include "hud.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "tf_imagepanel.h"
#include "item_model_panel.h"
#include "tf_hud_playerstatus.h"
#include "tf_spectatorgui.h"
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include <vgui/IVGui.h>
#include <vgui/IScheme.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/Frame.h>
#include "vgui_controls/TextImage.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include "ienginevgui.h"
#include "hud_chat.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CDisguiseStatus : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CDisguiseStatus, EditablePanel );

public:
	CDisguiseStatus( const char *pElementName );

	void			Init( void );
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual void	PerformLayout( void );
	virtual void	Paint( void );

	void			ShowAndUpdateStatus( void );
	void			HideStatus( void );
	virtual bool	ShouldDraw( void );

	void			CheckName( void );
	void			CheckWeapon( void );
	void			CheckHealth( void );

private:
	CPanelAnimationVar( HFont, m_hFont, "TextFont", "TargetID" );

	CTFImagePanel			*m_pBGPanel;
	CEmbeddedItemModelPanel	*m_pModelPanel;

	CTFSpectatorGUIHealth	*m_pDisguiseHealth;

	Label					*m_pDisguiseNameLabel;
	Label					*m_pWeaponNameLabel;

	bool					m_bDisguised;
	CTFWeaponBase			*m_pDisguiseWeapon;
	int						m_iDisguiseTeam;
};

DECLARE_HUDELEMENT( CDisguiseStatus );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDisguiseStatus::CDisguiseStatus( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "DisguiseStatus" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_pModelPanel = NULL;
	m_pBGPanel = NULL;
	m_pDisguiseNameLabel = NULL;
	m_pWeaponNameLabel = NULL;

	m_pDisguiseHealth = new CTFSpectatorGUIHealth( this, "SpectatorGUIHealth" );

	m_bDisguised = true;
	m_pDisguiseWeapon = NULL;
	m_iDisguiseTeam = TEAM_UNASSIGNED;
}

//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CDisguiseStatus::Init( void )
{
	HideStatus();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDisguiseStatus::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	LoadControlSettings( "Resource/UI/DisguiseStatusPanel.res" );
	m_pModelPanel	= dynamic_cast<CEmbeddedItemModelPanel*>( FindChildByName("itemmodelpanel") );
	m_pBGPanel		= dynamic_cast<CTFImagePanel *> ( FindChildByName("DisguiseStatusBG") );
	m_pDisguiseNameLabel = dynamic_cast<Label *>(FindChildByName("DisguiseNameLabel"));
	m_pWeaponNameLabel = dynamic_cast<Label *>(FindChildByName("WeaponNameLabel"));

	SetPaintBackgroundEnabled( false );

	HideStatus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDisguiseStatus::ShouldDraw( void )
{
	bool bShow = false;
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return false;

	if ( !pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		bShow = false;
	}
	else
	{
		bShow = true;
	}

	if ( bShow && (!m_bDisguised || (m_pDisguiseWeapon != pPlayer->m_Shared.GetDisguiseWeapon()) || (m_iDisguiseTeam != pPlayer->m_Shared.GetDisguiseTeam()) ) )
	{
		ShowAndUpdateStatus();
	}
	else if ( !bShow && m_bDisguised )
	{
		HideStatus();
	}

	return bShow;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDisguiseStatus::ShowAndUpdateStatus( void )
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	m_iDisguiseTeam = pPlayer->m_Shared.GetDisguiseTeam();

//	m_pModelPanel->SetVisible( false );

	if ( m_pBGPanel )
	{
		m_pBGPanel->SetVisible( true );
		m_pBGPanel->SetBGTeam( m_iDisguiseTeam );
		m_pBGPanel->UpdateBGImage();
	}

	if ( m_pDisguiseNameLabel )
	{
		m_pDisguiseNameLabel->SetVisible( true );

		CheckName();
	}

	if ( m_pWeaponNameLabel )
	{
		m_pWeaponNameLabel->SetVisible( true );

		CheckWeapon();
	}

	if ( m_pDisguiseHealth )
	{
		m_pDisguiseHealth->SetVisible( true );

		CheckHealth();
	}

	m_bDisguised = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDisguiseStatus::HideStatus( void )
{
	if ( m_pModelPanel )
	{
		m_pModelPanel->SetVisible( false );
	}

	if ( m_pBGPanel )
	{
		m_pBGPanel->SetVisible( false );
	}

	if ( m_pDisguiseNameLabel )
	{
		m_pDisguiseNameLabel->SetVisible( false );
	}

	if ( m_pWeaponNameLabel )
	{
		m_pWeaponNameLabel->SetVisible( false );
	}

	if ( m_pDisguiseHealth )
	{
		m_pDisguiseHealth->SetVisible( false );
	}

	m_bDisguised = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDisguiseStatus::CheckName( void )
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	C_TFPlayer *pTargetPlayer = pPlayer->m_Shared.GetDisguiseTarget();
	if ( pTargetPlayer )
	{
		if ( g_PR != NULL )
		{
			const char *pszName = pTargetPlayer->GetPlayerName();
			if ( pszName && pszName[0] )
			{
				SetDialogVariable( "disguisename", pszName );
				return;
			}
		}
	}

	SetDialogVariable( "disguisename", pPlayer->GetPlayerName() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDisguiseStatus::CheckWeapon( void )
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	m_pDisguiseWeapon = pPlayer->m_Shared.GetDisguiseWeapon();
	if ( m_pDisguiseWeapon )
	{
		const wchar_t *pszDisguiseWeapon = g_pVGuiLocalize->Find( m_pDisguiseWeapon->GetTFWpnData().szPrintName );

		CAttributeContainer *pContainer = m_pDisguiseWeapon->GetAttributeContainer();
		if ( pContainer )
		{
			CEconItemView *pItem = pContainer->GetItem();
			if ( pItem )
			{
				pszDisguiseWeapon = pItem->GetItemName();
			}
		}

		SetDialogVariable( "weaponname", pszDisguiseWeapon );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDisguiseStatus::CheckHealth( void )
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	if ( !m_pDisguiseHealth )
		return;

	m_pDisguiseHealth->SetHealth( pPlayer->m_Shared.GetDisguiseHealth(), pPlayer->m_Shared.GetDisguiseMaxHealth(), pPlayer->m_Shared.GetDisguiseMaxBuffedHealth() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDisguiseStatus::PerformLayout( void )
{
	/*
	int iXIndent = XRES(5);
	int iXPostdent = XRES(10);
	int iWidth = m_pTargetHealth->GetWide() + iXIndent + iXPostdent;

	int iTextW, iTextH;
	int iDataW, iDataH;

	if ( m_pTargetNameLabel && m_pTargetDataLabel )
	{
		m_pTargetNameLabel->GetContentSize( iTextW, iTextH );
		m_pTargetDataLabel->GetContentSize( iDataW, iDataH );
		iWidth += max(iTextW,iDataW);

		SetSize( iWidth, GetTall() );

		int nOffset = m_bArenaPanelVisible ? YRES (120) : 0; // HACK: move the targetID up a bit so it won't overlap the panel

		SetPos( (ScreenWidth() - iWidth) * 0.5, m_nOriginalY - nOffset );

		if ( m_pBGPanel )
		{
			m_pBGPanel->SetSize( iWidth, GetTall() );
		}
	}
	*/

	BaseClass::PerformLayout();
};

//-----------------------------------------------------------------------------
// Purpose: Draw function for the element
//-----------------------------------------------------------------------------
void CDisguiseStatus::Paint()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	if ( !pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
		return;

// 	ConVarRef cl_hud_minmode( "cl_hud_minmode", true );
// 	if ( cl_hud_minmode.IsValid() && cl_hud_minmode.GetBool() )
// 	{
// 		HideStatus();
// 		return;
// 	}

	CheckName();

	CheckWeapon();

	CheckHealth();
}