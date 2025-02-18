//========= Copyright Valve Corporation, All rights reserved. ============//
//
//===========================================================================//


#include "cbase.h"
#include "inputsystem/iinputsystem.h"
#include "input.h"
#include "iinput.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"
#include "clientmode_shared.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterialvar.h"
#include "con_nprint.h"
#include "hud_vote.h"
#include "menu.h"
#include "c_playerresource.h"

#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ImageList.h>
#include "vgui_avatarimage.h"

#ifdef TF_CLIENT_DLL
#include "ienginevgui.h"
#include "tf_gcmessages.h"
#include "c_tf_player.h"
#include "econ_notifications.h"
#include "confirm_dialog.h"
#include "gc_clientsystem.h"
#include "tf_gamerules.h"
#include "c_playerresource.h"
#include "c_tf_objective_resource.h"
#include "c_tf_playerresource.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Josh:
// Enabled this now by default because of people reporting to me
// that his behaviour feels really broken!
//
// Especially when the vote caster, etc can't track the result.
// Given how much we are reliant on kicking bots, keeping this open is important
// for people to know what's going on!
//
// This is essentially replaced by the alpha when you have voted
// on an issue.
ConVar cl_vote_ui_active_after_voting( "cl_vote_ui_active_after_voting", "1" );
ConVar cl_vote_ui_show_notification( "cl_vote_ui_show_notification", "0" );

#ifdef TF_CLIENT_DLL

extern const char *FormatSeconds( int seconds );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFVoteNotification : public CEconNotification
{
public:
	CTFVoteNotification( const char *pPlayerName, int nVoteIdx ) : CEconNotification(), m_nVoteIdx( nVoteIdx )
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( pPlayerName, m_wszPlayerName, sizeof(m_wszPlayerName) );
		SetLifetime( 7 );
		SetText( "#GameUI_Vote_Notification_Text" );
		AddStringToken( "initiator", m_wszPlayerName );
	}

	virtual EType NotificationType() { return eType_AcceptDecline; }

	virtual void Trigger()
	{
		CTFGenericConfirmDialog *pDialog = ShowConfirmDialog( "#GameUI_Vote_Notification_Title",
															  "#GameUI_Vote_Notification_Text",
															  "#GameUI_Vote_Notification_View",
															  "#cancel", &ConfirmShowVoteSetup );
		pDialog->SetContext( this );
		pDialog->AddStringToken( "initiator", m_wszPlayerName );
		// so we aren't deleted
		SetIsInUse( true );
	}

	virtual void Accept()
	{
		ConfirmShowVoteSetup( true, this );
	}
	virtual void Decline()
	{
		ConfirmShowVoteSetup( false, this );
	}
	static void ConfirmShowVoteSetup( bool bConfirmed, void *pContext )
	{
		CTFVoteNotification *pNotification = (CTFVoteNotification*)pContext;
		if ( bConfirmed )
		{
			// Show vote
			CHudVote *pHudVote = GET_HUDELEMENT( CHudVote );
			if ( pHudVote )
			{
				pHudVote->ShowVoteUI( pNotification->m_nVoteIdx, true );
			}
		}
		pNotification->SetIsInUse( false );
		pNotification->MarkForDeletion();
	}

public:
	wchar_t m_wszPlayerName[MAX_PLAYER_NAME_LENGTH];
private:
	int		m_nVoteIdx;
};
#endif	// TF_CLIENT_DLL


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
VoteBarPanel::VoteBarPanel( vgui::Panel *parent, const char *panelName ) : vgui::Panel( parent, panelName )
{
	for( int index = 0; index < MAX_VOTE_OPTIONS; index++ )
	{
		m_nVoteOptionCount[index] = 0;
	}
	m_nPotentialVotes = 0;

	ListenForGameEvent( "vote_changed" );
}

void VoteBarPanel::Paint( void )
{
	int wide, tall;
	GetSize( wide, tall );

	int x = 0;

	// driller:  this shouldn't ship - temp UI solution for playtesting
	for ( int i = 0; i < 2; i++ )
	{
		// Draw an outlined box
		vgui::surface()->DrawSetColor( 128, 128, 128, 128 );
		vgui::surface()->DrawFilledRect( x, 0, x + m_iBoxSize, m_iBoxSize );

		vgui::surface()->DrawSetColor( 0, 0, 0, 255 );
		vgui::surface()->DrawFilledRect( x + m_iBoxInset, m_iBoxInset, x + m_iBoxSize - m_iBoxInset, m_iBoxSize - m_iBoxInset );

		vgui::surface()->DrawSetColor( Color(255, 255, 255, 255) );

		x += ( m_iBoxSize + 64 );
	}

	x = 0;

	int iImageInset = 2 * m_iBoxInset;

	// Yes image
	vgui::surface()->DrawSetTexture( m_nYesTextureId );
	vgui::surface()->DrawTexturedRect( x + iImageInset, iImageInset, x + m_iBoxSize - iImageInset, m_iBoxSize - iImageInset );

	x += ( m_iBoxSize + 64 );

	// No image
	vgui::surface()->DrawSetTexture( m_nNoTextureId );
	vgui::surface()->DrawTexturedRect( x + iImageInset, iImageInset, x + m_iBoxSize - iImageInset, m_iBoxSize - iImageInset );

}

void VoteBarPanel::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();
	if ( !eventName )
	return;

	if( FStrEq( eventName, "vote_changed" ) )
	{
		for ( int index = 0; index < MAX_VOTE_OPTIONS; index++ )
		{
			char szOption[2];
			Q_snprintf( szOption, sizeof( szOption ), "%i", index + 1 );

			char szVoteOption[13] = "vote_option";
			Q_strncat( szVoteOption, szOption, sizeof( szVoteOption ), COPY_ALL_CHARACTERS );

			m_nVoteOptionCount[index] = event->GetInt( szVoteOption );
		}
		m_nPotentialVotes = event->GetInt( "potentialVotes" );
	}
}

#ifdef TF_CLIENT_DLL
static const char* s_pszBotIcons[SCOREBOARD_PING_ICONS] =
{
	"../hud/scoreboard_ping_bot_red",
	"../hud/scoreboard_ping_bot_blue",
};
#endif // TF_CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CVoteSetupDialog::CVoteSetupDialog( vgui::Panel *parent ) : BaseClass( parent, "VoteSetupDialog" )
{
	SetMoveable( false );
	SetSizeable( false );

	m_pVoteSetupList = new SectionedListPanel( this, "VoteSetupList" );
	m_pVoteParameterList = new SectionedListPanel( this, "VoteParameterList" );
	m_pCallVoteButton = new Button( this, "CallVoteButton", "CallVote", this, "CallVote" );
	m_pComboBox = new ComboBox( this, "ComboBox", 5, false );
	m_pImageList = NULL;

#ifdef TF_CLIENT_DLL
	Q_memset( m_iImageClass, 0, sizeof( m_iImageClass ) );
	Q_memset( m_iImageTeamBot, 0, sizeof( m_iImageTeamBot ) );
#endif // TF_CLIENT_DLL

#ifdef TF_CLIENT_DLL
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
#else
	SetScheme( "ClientScheme" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CVoteSetupDialog::~CVoteSetupDialog()
{
	if ( m_pImageList )
	{
		delete m_pImageList;
		m_pImageList = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVoteSetupDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetProportional( true );

	LoadControlSettings( "Resource/UI/VoteHud.res" );

	m_pComboBox->GetComboButton()->SetFgColor( Color( 117,107,94,255 ) ); 
	m_pComboBox->GetComboButton()->SetDefaultColor( Color( 117,107,94,255), Color( 0,0,0,0) );
	m_pComboBox->GetComboButton()->SetArmedColor( Color( 117,107,94,255), Color( 0,0,0,0) );
	m_pComboBox->GetComboButton()->SetDepressedColor( Color( 117,107,94,255), Color( 0,0,0,0) );

	if ( m_pImageList )
	{
		delete m_pImageList;
	}

	m_pImageList = new ImageList( false );

#ifdef TF_CLIENT_DLL
	for ( int i = 1 ; i < SCOREBOARD_CLASS_ICONS ; i++ )
	{
		m_iImageClass[i] = m_pImageList->AddImage( scheme()->GetImage( g_pszClassIcons[i], true ) );
	}

	for ( int i = 0; i < 2; i++ )
	{
		m_iImageTeamBot[i] = m_pImageList->AddImage( scheme()->GetImage( s_pszBotIcons[i], true ) );
	}
#endif // TF_CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Does dialog-specific customization after applying scheme settings.
//-----------------------------------------------------------------------------
void CVoteSetupDialog::PostApplySchemeSettings( vgui::IScheme *pScheme )
{
	// resize the images to our resolution
	for ( int i = 0; i < m_pImageList->GetImageCount(); i++ )
	{
		int wide, tall;
		m_pImageList->GetImage( i )->GetSize( wide, tall );
		m_pImageList->GetImage( i )->SetSize(scheme()->GetProportionalScaledValueEx( GetScheme(), wide ), scheme()->GetProportionalScaledValueEx( GetScheme(), tall ) );
	}

	m_pVoteParameterList->SetImageList( m_pImageList, false );
	m_pVoteParameterList->SetVisible( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVoteSetupDialog::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings( inResourceData );

	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );

	m_hIssueFont = INVALID_FONT;
	const char *pszFont = inResourceData->GetString( "issue_font", NULL );
	if ( pszFont && pszFont[0] )
	{
		m_hIssueFont = pScheme->GetFont( pszFont, true );
	}

	m_hHeaderFont = INVALID_FONT;
	pszFont = inResourceData->GetString( "header_font", NULL );
	if ( pszFont && pszFont[0] )
	{
		m_hHeaderFont = pScheme->GetFont( pszFont, true );
	}

#ifdef TF_CLIENT_DLL
	m_hPlayerNameFont = INVALID_FONT;
	pszFont = inResourceData->GetString( "player_font", NULL );
	if ( pszFont && pszFont[0] )
	{
		m_hPlayerNameFont = pScheme->GetFont( pszFont, true );
	}
#endif // TF_CLIENT_DLL

	const char *pszColor = inResourceData->GetString( "issue_fgcolor", "Label.TextColor" );
	m_IssueFGColor = pScheme->GetColor( pszColor, Color( 255, 255, 255, 255 ) );

	pszColor = inResourceData->GetString( "issue_fgcolor_disabled", "Label.TextColor" );
	m_IssueFGColorDisabled = pScheme->GetColor( pszColor, Color( 255, 255, 255, 255 ) );

	pszColor = inResourceData->GetString( "header_fgcolor", "Label.TextColor" );
	m_HeaderFGColor = pScheme->GetColor( pszColor, Color( 255, 255, 255, 255 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVoteSetupDialog::InitializeIssueList( void )
{
	m_pComboBox->RemoveAll();
	m_pComboBox->SetVisible( false );
	SetDialogVariable( "combo_label", "" );

	for ( int index = 0; index < m_VoteIssues.Count(); index++ )
	{
		if ( !m_VoteIssues[index].szName[0] )
			continue;

		bool bActive = m_VoteIssues[index].bIsActive;

		char szIssueLocalized[k_MAX_VOTE_NAME_LENGTH] = { 0 };
		g_pVGuiLocalize->ConvertUnicodeToANSI( g_pVGuiLocalize->Find( m_VoteIssues[index].szNameString ), szIssueLocalized, sizeof( szIssueLocalized ) );

		if ( !bActive )
		{
			char szDisabled[k_MAX_VOTE_NAME_LENGTH] = { 0 };
			g_pVGuiLocalize->ConvertUnicodeToANSI( g_pVGuiLocalize->Find( "#GameUI_Vote_Disabled" ), szDisabled, sizeof( szDisabled ) );
			V_strcat_safe( szIssueLocalized, szDisabled );
		}

		KeyValues *pKeyValues = new KeyValues( "Issue" );
		pKeyValues->SetString( "Issue", szIssueLocalized );
		pKeyValues->SetString( "IssueRaw", m_VoteIssues[index].szName );
		pKeyValues->SetBool( "Active", m_VoteIssues[index].bIsActive );
		int iId = m_pVoteSetupList->AddItem( 0, pKeyValues );
		pKeyValues->deleteThis();

		// Setup the list entry style
		if ( m_hIssueFont != INVALID_FONT )
		{
			m_pVoteSetupList->SetItemFont( iId, m_hIssueFont );
			Color colFG = bActive ? m_IssueFGColor : m_IssueFGColorDisabled;
			m_pVoteSetupList->SetItemFgColor( iId, colFG );
		}
	}

	// Select the first item by default
	if ( m_pVoteSetupList->GetItemCount() > 0 )
	{
		m_pVoteSetupList->SetSelectedItem( 0 );
	}
	else
	{
		// No active issues
		char szIssueLocalized[k_MAX_VOTE_NAME_LENGTH] = { 0 };
		g_pVGuiLocalize->ConvertUnicodeToANSI( g_pVGuiLocalize->Find( "#GameUI_Vote_System_Disabled" ), szIssueLocalized, sizeof( szIssueLocalized ) );

		KeyValues *pKeyValues = new KeyValues( "Issue" );
		pKeyValues->SetString( "Issue", szIssueLocalized );
		pKeyValues->SetString( "IssueRaw", "Disabled" );
		pKeyValues->SetBool( "Active", false );
		int iId = m_pVoteSetupList->AddItem( 0, pKeyValues );
		pKeyValues->deleteThis();

		if ( m_hIssueFont != INVALID_FONT )
		{
			m_pVoteSetupList->SetItemFont( iId, m_hIssueFont );
			m_pVoteSetupList->SetItemFgColor( iId, m_IssueFGColor );
		}
	}

	UpdateCurrentMap();
}

//-----------------------------------------------------------------------------
// Purpose: Keep track of the current map
//-----------------------------------------------------------------------------
void CVoteSetupDialog::UpdateCurrentMap( void )
{
	Q_FileBase( engine->GetLevelName(), m_szCurrentMap, sizeof(m_szCurrentMap) );
	Q_strlower( m_szCurrentMap );
}

//-----------------------------------------------------------------------------
// Purpose: Feeds Issues from the server to this Dialog
//-----------------------------------------------------------------------------
void CVoteSetupDialog::AddVoteIssues( CUtlVector< VoteIssue_t > &m_VoteSetupIssues )
{
	m_VoteIssues.RemoveAll();
	for ( int index = 0; index < m_VoteSetupIssues.Count(); index++ )
	{
		m_VoteIssues.AddToTail( m_VoteSetupIssues[index] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Feeds the server's MapCycle to the parameters dialog
//-----------------------------------------------------------------------------
void CVoteSetupDialog::AddVoteIssueParams_MapCycle( CUtlStringList &m_VoteSetupMapCycle )
{
	m_VoteIssuesMapCycle.RemoveAll();
	for ( int index = 0; index < m_VoteSetupMapCycle.Count(); index++ )
	{
		m_VoteIssuesMapCycle.AddToTail( m_VoteSetupMapCycle[index] );
	}
}

#ifdef TF_CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Feeds the server's PopFiles to the parameters dialog
//-----------------------------------------------------------------------------
void CVoteSetupDialog::AddVoteIssueParams_PopFiles( CUtlStringList &m_VoteSetupPopFiles )
{
	m_VoteIssuesPopFiles.RemoveAll();
	for ( int index = 0; index < m_VoteSetupPopFiles.Count(); index++ )
	{
		m_VoteIssuesPopFiles.AddToTail( m_VoteSetupPopFiles[index] );
	}
}
#endif // TF_CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVoteSetupDialog::Activate()
{
	InvalidateLayout( true, true );
	
	BaseClass::Activate();

	ResetData();

	m_pVoteSetupList->SetVerticalScrollbar( true );
	m_pVoteSetupList->RemoveAll();
	m_pVoteSetupList->RemoveAllSections();
	m_pVoteSetupList->AddSection( 0, "Issue" );
	m_pVoteSetupList->SetSectionAlwaysVisible( 0, true );
	m_pVoteSetupList->SetSectionFgColor( 0, Color( 255, 255, 255, 255 ) );
	m_pVoteSetupList->SetBgColor( Color( 0, 0, 0, 0 ) );
	m_pVoteSetupList->SetBorder( NULL );
	m_pVoteSetupList->AddColumnToSection( 0, "Issue", "#TF_Vote_Column_Issue", SectionedListPanel::COLUMN_CENTER, m_iIssueWidth );

	if ( m_hHeaderFont != INVALID_FONT )
	{
		m_pVoteSetupList->SetFontSection( 0, m_hHeaderFont );
		m_pVoteSetupList->SetSectionFgColor( 0, m_HeaderFGColor );
	}

	m_pVoteParameterList->SetVerticalScrollbar( true );
	m_pVoteParameterList->RemoveAll();
	m_pVoteParameterList->RemoveAllSections();
	m_pVoteParameterList->AddSection( 0, "Name" );
	m_pVoteParameterList->SetSectionAlwaysVisible( 0, true );
	m_pVoteParameterList->SetSectionFgColor( 0, Color( 255, 255, 255, 255 ) );
	m_pVoteParameterList->SetBgColor( Color( 0, 0, 0, 0 ) );
	m_pVoteParameterList->SetBorder( NULL );
#ifdef TF_CLIENT_DLL
	int nAvatarSize = QuickPropScale( 16 );
	int nSpacerSize = QuickPropScale( 5 );
	m_pVoteParameterList->AddColumnToSection( 0, "Avatar", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, nAvatarSize );
	m_pVoteParameterList->AddColumnToSection( 0, "", "", 0, nSpacerSize );	// Spacer
	int iRealWidth = m_iParameterWidth - ( nAvatarSize + nSpacerSize + nAvatarSize );
	m_pVoteParameterList->AddColumnToSection( 0, "Name", "#TF_Vote_Column_Name", 0, iRealWidth * 0.75 );
	m_pVoteParameterList->AddColumnToSection( 0, "Properties", "#TF_Vote_Column_Properties", SectionedListPanel::COLUMN_CENTER, iRealWidth * 0.2 );
	m_pVoteParameterList->AddColumnToSection( 0, "Score", "", 0, iRealWidth * 0.05 );
	m_pVoteParameterList->AddColumnToSection( 0, "Class", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, nAvatarSize );
#else // !TF_CLIENT_DLL
	m_pVoteParameterList->AddColumnToSection( 0, "Avatar", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, 55 );
	m_pVoteParameterList->AddColumnToSection( 0, "", "", 0, 10 );	// Spacer
	m_pVoteParameterList->AddColumnToSection( 0, "Name", "#TF_Vote_Column_Name", 0, m_iParameterWidth * 0.6 );
	m_pVoteParameterList->AddColumnToSection( 0, "Properties", "#TF_Vote_Column_Properties", SectionedListPanel::COLUMN_CENTER, m_iParameterWidth * 0.3 );
#endif // !TF_CLIENT_DLL

	if ( m_hHeaderFont != INVALID_FONT )
	{
		m_pVoteParameterList->SetFontSection( 0, m_hHeaderFont );
		m_pVoteParameterList->SetSectionFgColor( 0, m_HeaderFGColor );
		m_pVoteParameterList->SetFontSection( 1, m_hHeaderFont );
		m_pVoteParameterList->SetSectionFgColor( 1, m_HeaderFGColor );
	}

#ifdef TF_CLIENT_DLL
	m_hRowFont = m_pVoteSetupList->GetRowFont();
#endif // TF_CLIENT_DLL

	InitializeIssueList();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CVoteSetupDialog::OnClose()
{
	ResetData();
	BaseClass::OnClose();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CVoteSetupDialog::OnCommand(const char *command)
{
	// We should have enough data to issue a CallVote command
	if ( !V_stricmp( command, "CallVote" ) )
	{
		int iSelectedItem = m_pVoteSetupList->GetSelectedItem();
		if ( iSelectedItem >= 0 )
		{
			char szVoteCommand[k_MAX_VOTE_NAME_LENGTH];
			KeyValues *pIssueKeyValues = m_pVoteSetupList->GetItemData( iSelectedItem );
			const char *szIssueRaw = pIssueKeyValues->GetString( "IssueRaw" );
			if ( !V_stricmp( "ChangeLevel", szIssueRaw ) || !V_stricmp( "NextLevel", szIssueRaw ) )
			{
				int nSelectedParam = m_pVoteParameterList->GetSelectedItem();
				if ( nSelectedParam >= 0 )
				{
					// Get selected Map
					int iSelectedParam = m_pVoteParameterList->GetSelectedItem();
					if ( iSelectedParam >= 0 )
					{
						KeyValues *pParameterKeyValues = m_pVoteParameterList->GetItemData( iSelectedParam );
						if ( pParameterKeyValues )
						{
							// Which Map?
							const char *szMapName = pParameterKeyValues->GetString( "Name" );
							Q_snprintf( szVoteCommand, sizeof( szVoteCommand ), "callvote %s %s\n;", szIssueRaw, szMapName );
							engine->ClientCmd( szVoteCommand );
						}
					}
				}
			}
			else if ( !V_stricmp( "Kick", szIssueRaw ) )
			{
				// Get selected Player
				int iSelectedParam = m_pVoteParameterList->GetSelectedItem();
				if ( iSelectedParam >= 0 )
				{
					KeyValues *pKeyValues = m_pVoteParameterList->GetItemData( iSelectedParam );
					if ( pKeyValues )
					{
						// Is Player valid?
						int iPlayerIndex = pKeyValues->GetInt( "index" );
						if ( ( iPlayerIndex > 0 ) && ( iPlayerIndex <= MAX_PLAYERS ) )
						{
							if ( g_PR->IsConnected( iPlayerIndex ) )
							{
								const char *pReasonString = m_pComboBox->GetActiveItemUserData() ? m_pComboBox->GetActiveItemUserData()->GetName() : "other";
								Q_snprintf( szVoteCommand, sizeof( szVoteCommand ), "callvote %s \"%d %s\"\n;", szIssueRaw, g_PR->GetUserID( iPlayerIndex ), pReasonString );
								engine->ClientCmd( szVoteCommand );
#ifdef TF_CLIENT_DLL
								#if 0 // No longer being collected, see GC job comment
								uint32 unSteamID = g_TF_PR->GetAccountID( iPlayerIndex );
								if ( unSteamID != 0 )
								{
									GCSDK::CProtoBufMsg<CMsgTFVoteKickBanPlayer> msg( k_EMsgGCVoteKickBanPlayer );
									uint32 reason = GetKickBanPlayerReason( pReasonString );
									msg.Body().set_account_id_subject( unSteamID );
									msg.Body().set_kick_reason( reason );
									GCClientSystem()->BSendMessage( msg );
								}
								#endif // 0
#endif
							}
						}
					}
				}
			}
#ifdef TF_CLIENT_DLL
			else if ( !V_stricmp( "ChangeMission", szIssueRaw ) )
			{
				int nSelectedParam = m_pVoteParameterList->GetSelectedItem();
				if ( nSelectedParam >= 0 )
				{
					// Get selected Challenge
					int iSelectedParam = m_pVoteParameterList->GetSelectedItem();
					if ( iSelectedParam >= 0 )
					{
						KeyValues *pParameterKeyValues = m_pVoteParameterList->GetItemData( iSelectedParam );
						if ( pParameterKeyValues )
						{
							// Which Pop File?
							const char *szPopFile = pParameterKeyValues->GetString( "Name" );
							Q_snprintf( szVoteCommand, sizeof( szVoteCommand ), "callvote %s %s\n;", szIssueRaw, szPopFile );
							engine->ClientCmd( szVoteCommand );
						}
					}
				}
			}
#endif	// TF_CLIENT_DLL
			else
			{
				// Non-parameter vote.  i.e.  callvote scrambleteams
				Q_snprintf( szVoteCommand, sizeof(szVoteCommand), "callvote %s\n;", szIssueRaw );
				engine->ClientCmd( szVoteCommand );
			}

			Close();
		}
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVoteSetupDialog::OnItemSelected( vgui::Panel *panel )
{
	if ( panel == m_pVoteSetupList )
	{
		m_pComboBox->RemoveAll();
		m_pComboBox->SetVisible( false );
		SetDialogVariable( "combo_label", "" );

		// Which Issue did we select?
		int iSelectedItem = m_pVoteSetupList->GetSelectedItem();
		if ( iSelectedItem >= 0 )
		{
			KeyValues *pIssueKeyValues = m_pVoteSetupList->GetItemData( iSelectedItem );
			if ( !pIssueKeyValues )
				return;

			CHudVote *pHudVote = GET_HUDELEMENT( CHudVote );
			if ( !pHudVote )
				return;

			// We're rebuilding, so clear state
			m_bVoteButtonEnabled = false;
			m_pVoteParameterList->ClearSelection();
			m_pVoteParameterList->RemoveAll();

			const char *pszIssueRaw = pIssueKeyValues->GetString( "IssueRaw" );
			bool bActive = pIssueKeyValues->GetBool( "Active" );
			if ( !pHudVote->IsVoteSystemActive() || !bActive )
			{
				m_bVoteButtonEnabled = false;
			}
			// CHANGELEVEL / NEXTLEVEL
			else if ( !V_stricmp( "ChangeLevel", pszIssueRaw ) || !V_stricmp( "NextLevel", pszIssueRaw ) )
			{
				// Feed the mapcycle to the parameters list
				for ( int index = 0; index < m_VoteIssuesMapCycle.Count(); index++ )
				{
					// Don't show the current map
					if ( V_strncmp( m_VoteIssuesMapCycle[index], m_szCurrentMap, ( V_strlen( m_VoteIssuesMapCycle[index] ) - 1 ) ) == 0 )
						continue;

					KeyValues *pKeyValues = new KeyValues( "Name" );
					pKeyValues->SetString( "Name", m_VoteIssuesMapCycle[index] );
					pKeyValues->SetInt( "index", index );
					int iId = m_pVoteParameterList->AddItem( 0, pKeyValues );
					pKeyValues->deleteThis();

					if ( m_hIssueFont != INVALID_FONT )
					{
						m_pVoteParameterList->SetItemFont( iId, m_hIssueFont );
						m_pVoteParameterList->SetItemFgColor( iId, m_IssueFGColor );
					}
				}

				if ( m_pVoteParameterList->GetItemCount() == 0 )
				{
					KeyValues *pKeyValues = new KeyValues( "Name" );
					pKeyValues->SetString( "Name", "#TF_vote_no_maps" );
					pKeyValues->SetInt( "index", 1 );
					m_pVoteParameterList->AddItem( 0, pKeyValues );
					pKeyValues->deleteThis();
				}
			}
			// KICK
			else if ( !V_stricmp( "Kick", pszIssueRaw ) )
			{
				int iLocalPlayerIndex = GetLocalPlayerIndex();
				int nLocalPlayerTeam = GetLocalPlayerTeam();

				if ( !iLocalPlayerIndex || ( nLocalPlayerTeam == TEAM_UNASSIGNED ) )
					return;

				// Feed the player list to the parameters list
				int nMaxClients = engine->GetMaxClients();
				for ( int iPlayerIndex = 1; iPlayerIndex <= nMaxClients; iPlayerIndex++ )
				{
					if ( !g_PR->IsConnected( iPlayerIndex ) )
						continue;
#ifdef TF_CLIENT_DLL
					MM_PlayerConnectionState_t eConnectionState = g_TF_PR->GetPlayerConnectionState( iPlayerIndex );
					if ( eConnectionState != MM_CONNECTED )
						continue;
#endif // TF_CLIENT_DLL
					if ( iPlayerIndex == iLocalPlayerIndex )
						continue;

					int nTeam = g_PR->GetTeam( iPlayerIndex );

					bool bAllowKickUnassigned = false;
#ifdef TF_CLIENT_DLL
					// Allow kicking team unassigned or spectator in MvM
					if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && g_PR->IsConnected( iPlayerIndex ) && ( nTeam == TEAM_UNASSIGNED || nTeam == TEAM_SPECTATOR ) )
					{
						if ( !g_PR->IsFakePlayer( iPlayerIndex ) )
							bAllowKickUnassigned = true;
					}
#endif // TF_CLIENT_DLL
					
					// Can't kick people on the other team, so don't list them
					if ( ( nTeam != nLocalPlayerTeam ) && !bAllowKickUnassigned )
							continue;

					char szPlayerIndex[32];
					Q_snprintf( szPlayerIndex, sizeof( szPlayerIndex ), "%d", iPlayerIndex );

					KeyValues *pKeyValues = new KeyValues( szPlayerIndex );
					pKeyValues->SetString( "Name", g_PR->GetPlayerName( iPlayerIndex ) );
					pKeyValues->SetInt( "index", iPlayerIndex );
					int iId = m_pVoteParameterList->AddItem( 0, pKeyValues );
					pKeyValues->deleteThis();

					if ( m_hIssueFont != INVALID_FONT )
					{
						m_pVoteParameterList->SetItemFont( iId, m_hIssueFont );
						m_pVoteParameterList->SetItemFgColor( iId, m_IssueFGColor );
					}
				}

#ifdef TF_CLIENT_DLL
				SetDialogVariable( "combo_label", g_pVGuiLocalize->Find( "#TF_VoteKickReason" ) );
				m_pComboBox->AddItem( g_pVGuiLocalize->Find( "TF_VoteKickReason_Other" ), new KeyValues( "other" ) );
				m_pComboBox->AddItem( g_pVGuiLocalize->Find( "TF_VoteKickReason_Cheating" ), new KeyValues( "cheating" ) );
				m_pComboBox->AddItem( g_pVGuiLocalize->Find( "TF_VoteKickReason_Idle" ), new KeyValues( "idle" ) );
				m_pComboBox->AddItem( g_pVGuiLocalize->Find( "TF_VoteKickReason_Scamming" ), new KeyValues( "scamming" ) );
				m_pComboBox->SilentActivateItemByRow( 0 );
				m_pComboBox->SetVisible( true );
#endif
			}
#ifdef TF_CLIENT_DLL
			// CHANGE POP FILE
			else if ( !V_stricmp( "ChangeMission", pszIssueRaw ) )
			{
				// Feed the popfiles to the parameters list
				for ( int index = 0; index < m_VoteIssuesPopFiles.Count(); index++ )
				{
					// Don't show the current pop file
					const char *pszPopFileName = TFObjectiveResource()->GetMvMPopFileName();
					if ( !pszPopFileName || !pszPopFileName[0] )
					{
						// Use the map name
						char szShortMapName[ MAX_MAP_NAME ];
						V_strncpy( szShortMapName, engine->GetLevelName(), sizeof( szShortMapName ) );
						V_StripExtension( szShortMapName, szShortMapName, sizeof( szShortMapName ) );					

						if ( V_strncmp( m_VoteIssuesPopFiles[index], V_GetFileName( szShortMapName ), ( V_strlen( m_VoteIssuesPopFiles[index] ) - 1 ) ) == 0 )
							continue;
					}
					else
					{
						// Use the specified pop file
						if ( V_strncmp( m_VoteIssuesPopFiles[index], TFObjectiveResource()->GetMvMPopFileName(), ( V_strlen( m_VoteIssuesPopFiles[index] ) - 1 ) ) == 0 )
							continue;
					}

					KeyValues *pKeyValues = new KeyValues( "Name" );
					pKeyValues->SetString( "Name", m_VoteIssuesPopFiles[index] );
					pKeyValues->SetInt( "index", index );
					int iId = m_pVoteParameterList->AddItem( 0, pKeyValues );
					pKeyValues->deleteThis();

					if ( m_hIssueFont != INVALID_FONT )
					{
						m_pVoteParameterList->SetItemFont( iId, m_hIssueFont );
						m_pVoteParameterList->SetItemFgColor( iId, m_IssueFGColor );
					}
				}

				if ( m_pVoteParameterList->GetItemCount() == 0 )
				{
					KeyValues *pKeyValues = new KeyValues( "Name" );
					pKeyValues->SetString( "Name", "#TF_vote_no_challenges" );
					pKeyValues->SetInt( "index", 1 );
					m_pVoteParameterList->AddItem( 0, pKeyValues );
					pKeyValues->deleteThis();
				}
			}
#endif	// TF_CLIENT_DLL
			else
			{
				// User selected an issue that doesn't require a parameter - Scrambleteams, Restartgame, etc
				m_bVoteButtonEnabled = true;
			}
		}
	}
	else if ( panel == m_pVoteParameterList )
	{
		// If this issue requires a parameter, make sure we have one selected before enabling the CallVote button
		int iSelectedParam = m_pVoteParameterList->GetSelectedItem();
		if ( iSelectedParam >= 0 )
		{
			KeyValues *pParameterKeyValues = m_pVoteParameterList->GetItemData( iSelectedParam );
			if ( pParameterKeyValues )
			{
				const char *szParameterName = pParameterKeyValues->GetString( "Name" );
				if ( szParameterName )
				{
					m_bVoteButtonEnabled = true;
				}
			}
		}
	}

	m_pCallVoteButton->SetEnabled( m_bVoteButtonEnabled );
	RefreshIssueParameters();
}

//-----------------------------------------------------------------------------
// Purpose:  Updates any additional data/info on Vote issue parameters
//-----------------------------------------------------------------------------
void CVoteSetupDialog::RefreshIssueParameters()
{
	// In the case of the KICK issue, we list players and show additional properties (Bot, Disconnected)
	int iSelectedItem = m_pVoteSetupList->GetSelectedItem();
	if ( iSelectedItem >= 0 )
	{
		KeyValues *pIssueKeyValues = m_pVoteSetupList->GetItemData( iSelectedItem );
		const char *pszIssueRaw = pIssueKeyValues->GetString( "IssueRaw" );
		if ( !V_stricmp( "Kick", pszIssueRaw ) )
		{
			if ( m_pVoteParameterList->GetItemCount() > 0 )
			{
				for ( int index = 0; index < m_pVoteParameterList->GetItemCount(); index++ )
				{
#ifdef TF_CLIENT_DLL
					if ( m_hPlayerNameFont != INVALID_FONT )
						m_pVoteParameterList->SetItemFont( index, m_hPlayerNameFont );
#endif // TF_CLIENT_DLL

					KeyValues *pKeyValues = m_pVoteParameterList->GetItemData( index );
					if ( !pKeyValues )
						continue;

					int playerIndex = pKeyValues->GetInt( "index" );
					player_info_t playerInfo;

					if ( !engine->GetPlayerInfo( playerIndex, &playerInfo ) )
					{
						pKeyValues->SetString( "Properties", "Offline" );
						continue;
					}

					pKeyValues->SetString( "Name", UTIL_GetFilteredPlayerName( playerIndex, playerInfo.name ) );

#ifdef TF_CLIENT_DLL
					{
						const char *pszString = "";
						pszString = FormatSeconds( gpGlobals->curtime - g_TF_PR->GetConnectTime( playerIndex ) );
						pKeyValues->SetString( "Properties", pszString );
					}

					if ( g_PR->IsValid( playerIndex ) && g_PR->IsConnected( playerIndex ) )
					{
						char szScore[64];
						int nScore = g_PR->GetPlayerScore( playerIndex );
						V_snprintf( szScore, sizeof( szScore ), "%d", nScore );
						pKeyValues->SetString( "Score", szScore );
					}
					else
					{
						pKeyValues->SetString( "Score", "" );
					}

					CSteamID steamID;
					steamID = GetSteamIDForPlayerIndex( playerIndex );
					if ( playerInfo.fakeplayer )
					{
						C_BasePlayer* pPlayer = UTIL_PlayerByIndex( playerIndex );

						// misyl:
						// Just default to blue for spectators or whatever for bots.
						// We don't have art for that, not a big deal.

						bool bIsRed = pPlayer && pPlayer->GetTeamNumber() == TF_TEAM_RED;
						pKeyValues->SetInt( "Avatar", bIsRed ? m_iImageTeamBot[0] : m_iImageTeamBot[1] );
					}
					else if ( steamID.IsValid() )
					{
						CAvatarImage *pAvatar = new CAvatarImage();
						pAvatar->SetAvatarSteamID( steamID );
						pAvatar->SetAvatarSize( 32, 32 );
						int iImageIndex = m_pImageList->AddImage( pAvatar );
						pKeyValues->SetInt( "Avatar", iImageIndex );
					}
					else
					{
						// Never show the wrong image.
						pKeyValues->SetInt( "Avatar", -1 );
					}

					int iClass = g_TF_PR->GetPlayerClass( playerIndex );
					if ( iClass >= TF_FIRST_NORMAL_CLASS && iClass <= TF_LAST_NORMAL_CLASS )
						pKeyValues->SetInt( "Class", m_iImageClass[ iClass ] );
					else
						pKeyValues->SetInt( "Class", -1 );
#else // !TF_CLIENT_DLL
					if ( playerInfo.fakeplayer )
					{
						const char *pszString = "Bot";
						pKeyValues->SetString( "Properties", pszString );
					}
					else
					{
						const char *pszString = "";
						pKeyValues->SetString( "Properties", pszString );
					}

					CSteamID steamID;
					C_BasePlayer* pPlayer = UTIL_PlayerByIndex( playerIndex );
					if ( pPlayer && pPlayer->GetSteamID( &steamID ) && steamID.GetAccountID() != 0 )
					{
						CAvatarImage *pAvatar = new CAvatarImage();
						pAvatar->SetAvatarSteamID( steamID );
						pAvatar->SetAvatarSize( 32, 32 );
						int iImageIndex = m_pImageList->AddImage( pAvatar );
						pKeyValues->SetInt( "Avatar", iImageIndex );
					}
#endif // !TF_CLIENT_DLL

					m_pVoteParameterList->InvalidateItem( index );
				}
			}
		}

		m_pVoteParameterList->SetImageList( m_pImageList, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVoteSetupDialog::ResetData()
{
	m_bVoteButtonEnabled = false;
 	m_pVoteSetupList->RemoveAll();
	m_pVoteParameterList->RemoveAll();
	m_pComboBox->RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
DECLARE_HUDELEMENT( CHudVote );
DECLARE_HUD_MESSAGE( CHudVote, CallVoteFailed );
DECLARE_HUD_MESSAGE( CHudVote, VoteStart );
DECLARE_HUD_MESSAGE( CHudVote, VotePass );
DECLARE_HUD_MESSAGE( CHudVote, VoteFailed );
DECLARE_HUD_MESSAGE( CHudVote, VoteSetup );

CHudVote::CHudVote( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "CHudVote" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

#ifdef TF_CLIENT_DLL
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
#endif

	SetProportional( true );

	SetHiddenBits( 0 );
	m_pVoteSetupDialog = new CVoteSetupDialog( pParent );
	RegisterForRenderGroup( "mid" );

	for ( int i = 0; i < ARRAYSIZE( m_pVotePanels ); i++ )
		m_pVotePanels[ i ] = new CHudVotePanel( this, i );
}

void CHudVote::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetProportional( true );

	for ( int i = 0; i < ARRAYSIZE( m_pVotePanels ); i++ )
		m_pVotePanels[ i ]->SetSize( GetWide(), GetTall() );
}

void CHudVote::Init( void )
{
	m_bVoteSystemActive = false;

	HOOK_HUD_MESSAGE( CHudVote, CallVoteFailed );
	HOOK_HUD_MESSAGE( CHudVote, VoteStart );
	HOOK_HUD_MESSAGE( CHudVote, VotePass );
	HOOK_HUD_MESSAGE( CHudVote, VoteFailed );
	HOOK_HUD_MESSAGE( CHudVote, VoteSetup );

	for ( int i = 0; i < ARRAYSIZE( m_pVotePanels ); i++ )
		m_pVotePanels[ i ]->Init();
}

void CHudVote::LevelInit( void )
{
	for ( int i = 0; i < ARRAYSIZE( m_pVotePanels ); i++ )
		m_pVotePanels[ i ]->LevelInit();
}

bool CHudVote::ShouldDraw( void )
{
	for ( int i = 0; i < ARRAYSIZE( m_pVotePanels ); i++ )
	{
		if ( m_pVotePanels[ i ]->ShouldDraw() )
			return true;
	}

	return false;
}

bool CHudVote::IsActive()
{
	for ( int i = 0; i < ARRAYSIZE( m_pVotePanels ); i++ )
	{
		if ( m_pVotePanels[ i ]->IsVoteUIActive() )
			return true;
	}

	return m_bActive;
}

CHudVotePanel *CHudVote::GetInputVotePanel()
{
	CHudVotePanel *pOrderedVotePanels[ 2 ] =
	{
		NULL, NULL,
	};

	if ( m_pVotePanels[ 0 ]->IsFirst() )
	{
		pOrderedVotePanels[ 0 ] = m_pVotePanels[ 0 ];
		pOrderedVotePanels[ 1 ] = m_pVotePanels[ 1 ];
	}
	else
	{ 
		pOrderedVotePanels[ 0 ] = m_pVotePanels[ 1 ];
		pOrderedVotePanels[ 1 ] = m_pVotePanels[ 0 ];
	}

	CHudVotePanel *pVotePanel = NULL;
	for ( int i = 0; i < ARRAYSIZE( pOrderedVotePanels ); i++ )
	{
		if ( pOrderedVotePanels[ i ] && pOrderedVotePanels[ i ]->m_bVotingActive && pOrderedVotePanels[ i ]->m_bShowVoteActivePanel && !pOrderedVotePanels[ i ]->m_bPlayerVoted )
		{
			pVotePanel = pOrderedVotePanels[ i ];
			break;
		}
	}

	return pVotePanel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CHudVote::KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( !IsVisible() )
		return 1;

	if ( !down )
		return 1;

	CHudVotePanel *pVotePanel = GetInputVotePanel();

 	if ( !pVotePanel )
 		return 1;

	int nSlot = 999;

	if ( down && keynum == KEY_F1 )
	{
		nSlot = 1;
	}
	else if ( down && keynum == KEY_F2 )
	{
		nSlot = 2;
	}
	else if ( down && keynum == KEY_F3 )
	{
		nSlot = 3;
	}
	else if ( down && keynum == KEY_F4 )
	{
		nSlot = 4;
	}
	else if ( down && keynum == KEY_F5 )
	{
		nSlot = 5;
	}
	else
	{
		return 1;
	}

	// Limit key checking to the number of options
	if ( nSlot > pVotePanel->m_nVoteChoicesCount )
		return 1;

	char szVoteCommand[64];
	Q_snprintf( szVoteCommand, sizeof( szVoteCommand ), "vote %d option%d", pVotePanel->m_nVoteIdx, nSlot );

	engine->ClientCmd( szVoteCommand );

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Propagate vote option parameters to the Issue Parameters list
//-----------------------------------------------------------------------------
void CHudVote::PropagateOptionParameters( void )
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return;

	m_pVoteSetupDialog->AddVoteIssueParams_MapCycle( m_VoteSetupMapCycle );

#ifdef TF_CLIENT_DLL
	m_pVoteSetupDialog->AddVoteIssueParams_PopFiles( m_VoteSetupPopFiles );
#endif // TF_CLIENT_DLL

	// Insert future issue param data containers here
}

// Josh:
// These MsgFunc_s should be cleaned up
// at some point to not be friends with CHudVotePanel and set
// stuff directly.
// It is only like this for now to avoid any potential
// refactory breakage in the transition to multi-team voting and CHudVotePanel.

//-----------------------------------------------------------------------------
// Purpose:  Sent only to the caller
//-----------------------------------------------------------------------------
void CHudVote::MsgFunc_CallVoteFailed( bf_read &msg )
{
	if ( IsPlayingDemo() )
		return;

	vote_create_failed_t nReason = (vote_create_failed_t)msg.ReadByte();
	int nTime = msg.ReadShort();

	// if we're already drawing a vote, do nothing
	if ( ShouldDraw() )
		return;

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return;

	CHudVotePanel *pFreeVotePanel = NULL;
	for ( int i = 0; i < ARRAYSIZE( m_pVotePanels ); i++ )
	{
		if ( !m_pVotePanels[ i ]->m_bVotingActive )
		{
			pFreeVotePanel = m_pVotePanels[ i ];
			break;
		}
	}

	if ( !pFreeVotePanel )
		return;

	pLocalPlayer->EmitSound("Vote.Failed");

	pFreeVotePanel->m_pVoteActive->SetVisible( false );
	pFreeVotePanel->m_pVoteFailed->SetVisible( false );
	pFreeVotePanel->m_pVotePassed->SetVisible( false );
	pFreeVotePanel->m_pCallVoteFailed->SetVisible( true );
	m_pVoteSetupDialog->SetVisible( false );

	pFreeVotePanel->m_flHideTime = gpGlobals->curtime + 4.f;

	char szTime[k_MAX_VOTE_NAME_LENGTH];
	wchar_t wszTime[k_MAX_VOTE_NAME_LENGTH];
	bool bMinutes = ( nTime > 65 );
	if ( bMinutes )
	{
		nTime /= 60;
	}
	Q_snprintf( szTime, sizeof ( szTime), "%i", nTime );
	g_pVGuiLocalize->ConvertANSIToUnicode( szTime, wszTime, sizeof( wszTime ) );

	wchar_t wszHeaderString[k_MAX_VOTE_NAME_LENGTH];

	switch( nReason )
	{
		case VOTE_FAILED_GENERIC:
			pFreeVotePanel->m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed" );
			break;

		case VOTE_FAILED_TRANSITIONING_PLAYERS:
			pFreeVotePanel->m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_transition_vote" );
			break;

		case VOTE_FAILED_RATE_EXCEEDED:
		{
			const char *pszTimeString = ( bMinutes ) ? ( ( nTime < 2 ) ? "#GameUI_vote_failed_vote_spam_min" : "#GameUI_vote_failed_vote_spam_mins" ) : "#GameUI_vote_failed_vote_spam";
			g_pVGuiLocalize->ConstructString_safe( wszHeaderString, g_pVGuiLocalize->Find( pszTimeString ), 1, wszTime );
			pFreeVotePanel->m_pCallVoteFailed->SetDialogVariable( "FailedReason", wszHeaderString );
			break;
		}

		case VOTE_FAILED_ISSUE_DISABLED:
			pFreeVotePanel->m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_disabled_issue" );
			break;

		case VOTE_FAILED_MAP_NOT_FOUND:
			pFreeVotePanel->m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_map_not_found" );
			break;

		case VOTE_FAILED_MAP_NOT_VALID:
			pFreeVotePanel->m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_map_not_valid" );
			break;

		case VOTE_FAILED_MAP_NAME_REQUIRED:
			pFreeVotePanel->m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_map_name_required" );
			break;

		case VOTE_FAILED_ON_COOLDOWN:
		{
			const char *pszTimeString = ( bMinutes ) ? ( ( nTime < 2 ) ? "#GameUI_vote_failed_recently_min" : "#GameUI_vote_failed_recently_mins" ) : "#GameUI_vote_failed_recently";
			g_pVGuiLocalize->ConstructString_safe( wszHeaderString, g_pVGuiLocalize->Find( pszTimeString ), 1, wszTime );
			pFreeVotePanel->m_pCallVoteFailed->SetDialogVariable( "FailedReason", wszHeaderString );
			break;
		}

		case VOTE_FAILED_TEAM_CANT_CALL:
			pFreeVotePanel->m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_team_cant_call" );
			break;

		case VOTE_FAILED_WAITINGFORPLAYERS:
			pFreeVotePanel->m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_waitingforplayers" );
			break;

		case VOTE_FAILED_CANNOT_KICK_ADMIN:
			pFreeVotePanel->m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_cannot_kick_admin" );
			break;

		case VOTE_FAILED_SCRAMBLE_IN_PROGRESS:
			pFreeVotePanel->m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_scramble_in_prog" );
			break;

		case VOTE_FAILED_SPECTATOR:
			pFreeVotePanel->m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_spectator" );
			break;

		case VOTE_FAILED_NEXTLEVEL_SET:
			pFreeVotePanel->m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_nextlevel_set" );
			break;

		case VOTE_FAILED_CANNOT_KICK_FOR_TIME:
		{
			const char *pszTimeString = ( bMinutes ) ? ( ( nTime < 2 ) ? "#GameUI_vote_failed_cannot_kick_min" : "#GameUI_vote_failed_cannot_kick_mins" ) : "#GameUI_vote_failed_cannot_kick";
			g_pVGuiLocalize->ConstructString_safe( wszHeaderString, g_pVGuiLocalize->Find( pszTimeString ), 1, wszTime );
			pFreeVotePanel->m_pCallVoteFailed->SetDialogVariable( "FailedReason", wszHeaderString );
			break;
		}

		case VOTE_FAILED_CANNOT_KICK_DURING_ROUND:
			pFreeVotePanel->m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_round_active" );
			break;

		case VOTE_FAILED_MODIFICATION_ALREADY_ACTIVE:
			pFreeVotePanel->m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_event_already_active" );
			break;

		case VOTE_FAILED_VOTE_IN_PROGRESS:
			pFreeVotePanel->m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_vote_in_progress" );
			break;

		case VOTE_FAILED_KICK_LIMIT_REACHED:
			pFreeVotePanel->m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_kick_limit" );
			break;

		case VOTE_FAILED_KICK_DENIED_BY_GC:
			pFreeVotePanel->m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_kick_its_you" );
			break;

		case VOTE_FAILED_PLAYER_TRANSITIONING:
			pFreeVotePanel->m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_transition_vote_player" );
			break;

		case VOTE_FAILED_INVALID_ARGUMENT:
			pFreeVotePanel->m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_invalid_argument" );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:  Sent to everyone
//-----------------------------------------------------------------------------
void CHudVote::MsgFunc_VoteFailed( bf_read &msg )
{
	if ( IsPlayingDemo() )
		return;

	int nVoteTeamIndex = msg.ReadByte();

	int nVoteIdx = msg.ReadLong();

	CHudVotePanel *pVotePanel = NULL;
	for ( int i = 0; i < ARRAYSIZE( m_pVotePanels ); i++ )
	{
		if ( m_pVotePanels[ i ]->m_nVoteIdx == nVoteIdx )
		{
			pVotePanel = m_pVotePanels[ i ];
			break;
		}
	}

	if ( !pVotePanel )
		return;

	pVotePanel->m_nVoteTeamIndex = nVoteTeamIndex;

	vote_create_failed_t nReason = (vote_create_failed_t)msg.ReadByte();

	// Visibility of this error is handled by OnThink()
	pVotePanel->m_bVotingActive = false;
	pVotePanel->m_bVotePassed = false;
	pVotePanel->m_flVoteResultCycleTime = gpGlobals->curtime;
	pVotePanel->m_flHideTime = gpGlobals->curtime + 5.f;
	pVotePanel->m_nVoteIdx = -1;

	switch ( nReason )
	{
	case VOTE_FAILED_GENERIC:
		pVotePanel->m_pVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed" );
		break;

	case VOTE_FAILED_YES_MUST_EXCEED_NO:
		pVotePanel->m_pVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_yesno" );
		break;

	case VOTE_FAILED_QUORUM_FAILURE:
		pVotePanel->m_pVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_quorum" );
		break;
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "vote_failed" );
	if ( event )
	{
		event->SetInt( "voteidx", pVotePanel->m_nVoteIdx );
		event->SetInt( "team", pVotePanel->m_nVoteTeamIndex );
		gameeventmanager->FireEventClientSide( event );
	}

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return;

	bool bShowToPlayer = ( !pVotePanel->m_nVoteTeamIndex || pLocalPlayer->GetTeamNumber() == pVotePanel->m_nVoteTeamIndex );
	if ( bShowToPlayer )
	{
		pLocalPlayer->EmitSound("Vote.Failed");
	}
}

extern ConVar sv_vote_holder_may_vote_no;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVote::MsgFunc_VoteStart( bf_read &msg )
{
	if ( IsPlayingDemo() )
		return;

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return;

	// Is this a team-only vote?
	int nVoteTeamIndex = msg.ReadByte();
	if ( nVoteTeamIndex >= FIRST_GAME_TEAM && nVoteTeamIndex != pLocalPlayer->GetTeamNumber() )
		return;

	CHudVotePanel *pVotePanel = NULL;
	for ( int i = 0; i < ARRAYSIZE( m_pVotePanels ); i++ )
	{
		if ( !m_pVotePanels[ i ]->m_bVotingActive )
		{
			pVotePanel = m_pVotePanels[ i ];
			break;
		}
	}

	if ( !pVotePanel )
		return;

	pVotePanel->m_nVoteTeamIndex = nVoteTeamIndex;
	pVotePanel->m_nVoteIdx = msg.ReadLong();

	// Entity calling the vote
	bool bShowNotif = cl_vote_ui_show_notification.GetBool();
	const char *pszCallerName = "Server";
	pVotePanel->m_iVoteCallerIdx = msg.ReadByte();
	if ( pVotePanel->m_iVoteCallerIdx != DEDICATED_SERVER )
	{
		C_BasePlayer *pVoteCaller = UTIL_PlayerByIndex( pVotePanel->m_iVoteCallerIdx );
		if ( pVoteCaller )
		{
			pszCallerName = pVoteCaller->GetPlayerName();

			// Don't show a notification to the caller
			if ( pVoteCaller == pLocalPlayer )
			{
				bShowNotif = false;
			}
		}
		else
		{
			// Caller invalid for some reason
			pszCallerName = "Player";
		}
	}

	// DisplayString
	char szIssue[k_MAX_VOTE_NAME_LENGTH] = { 0 };
	msg.ReadString( szIssue, sizeof(szIssue) );

	// DetailString
	char szParam1[k_MAX_VOTE_NAME_LENGTH] = { 0 };
	msg.ReadString( szParam1, sizeof(szParam1) );

	pVotePanel->m_bIsYesNoVote = msg.ReadByte();
	int iTargetEntIndex = msg.ReadByte();

	pVotePanel->m_flHideTime = -1.f;
	pVotePanel->m_flVoteResultCycleTime = -1.f;
	pVotePanel->m_bPlayerVoted = pVotePanel->m_iVoteCallerIdx == GetLocalPlayerIndex() && !sv_vote_holder_may_vote_no.GetBool();
	pVotePanel->m_bVotingActive = true;
	pVotePanel->m_pVoteFailed->SetVisible( false );
	pVotePanel->m_pVotePassed->SetVisible( false );
	pVotePanel->m_pCallVoteFailed->SetVisible( false );
	m_pVoteSetupDialog->SetVisible( false );
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( pVotePanel->m_pVoteActive, "HideVoteBackgrounds" );

	pVotePanel->m_voteBar->SetVisible( pVotePanel->m_bIsYesNoVote );

	// There will always be at least two choices...
	pVotePanel->m_pVoteActive->SetControlVisible( "LabelOption1", true );
	pVotePanel->m_pVoteActive->SetControlVisible( "LabelOption2", true );

	// ...sometimes more
	pVotePanel->m_pVoteActive->SetControlVisible( "LabelOption3", pVotePanel->m_VoteSetupChoices.Count() > 2 ? true : false );
	pVotePanel->m_pVoteActive->SetControlVisible( "Option3Background_Selected", pVotePanel->m_VoteSetupChoices.Count() > 2 ? true : false );
	pVotePanel->m_pVoteActive->SetControlVisible( "LabelOption4", pVotePanel->m_VoteSetupChoices.Count() > 3 ? true : false );
	pVotePanel->m_pVoteActive->SetControlVisible( "Option4Background_Selected", pVotePanel->m_VoteSetupChoices.Count() > 3 ? true : false );
	pVotePanel->m_pVoteActive->SetControlVisible( "LabelOption5", pVotePanel->m_VoteSetupChoices.Count() > 4 ? true : false );
	pVotePanel->m_pVoteActive->SetControlVisible( "Option5Background_Selected", pVotePanel->m_VoteSetupChoices.Count() > 4 ? true : false );

	pVotePanel->m_pVoteActive->SetControlVisible( "VoteCountLabel", pVotePanel->m_bIsYesNoVote );
	pVotePanel->m_pVoteActive->SetControlVisible( "Option1CountLabel", pVotePanel->m_bIsYesNoVote );
	pVotePanel->m_pVoteActive->SetControlVisible( "Option2CountLabel", pVotePanel->m_bIsYesNoVote );
	pVotePanel->m_pVoteActive->SetControlVisible( "Divider1", pVotePanel->m_bIsYesNoVote );
	pVotePanel->m_pVoteActive->SetControlVisible( "Divider2", pVotePanel->m_bIsYesNoVote );

	// Display vote caller's name
	wchar_t wszCallerName[MAX_PLAYER_NAME_LENGTH];
	
	wchar_t wszHeaderString[k_MAX_VOTE_NAME_LENGTH];

	// Player
	g_pVGuiLocalize->ConvertANSIToUnicode( pszCallerName, wszCallerName, sizeof( wszCallerName ) );

	// String
	g_pVGuiLocalize->ConstructString_safe( wszHeaderString, g_pVGuiLocalize->Find( "#GameUI_vote_header" ), 1, wszCallerName );

	// Final
	pVotePanel->m_pVoteActive->SetDialogVariable( "header", wszHeaderString );

	// Display the Issue
	wchar_t *pwcParam;
	wchar_t wcParam[k_MAX_VOTE_NAME_LENGTH];

	wchar_t *pwcIssue;
	wchar_t wcIssue[k_MAX_VOTE_NAME_LENGTH];

	if ( Q_strlen( szParam1 ) > 0 )
	{
		if ( szParam1[0] == '#' )
		{
			// localize it
			pwcParam = g_pVGuiLocalize->Find( szParam1 );
		}
		else
		{
			// convert to wchar
			g_pVGuiLocalize->ConvertANSIToUnicode( szParam1, wcParam, sizeof( wcParam ) );
			pwcParam = wcParam;
		}

		g_pVGuiLocalize->ConstructString_safe( wcIssue, g_pVGuiLocalize->Find( szIssue ), 1, pwcParam );
		pwcIssue = wcIssue;
	}
	else
	{
		// no param, just localize the issue
		pwcIssue = g_pVGuiLocalize->Find( szIssue );
	}
	pVotePanel->m_pVoteActive->SetDialogVariable( "voteissue", pwcIssue );

	// Figure out which UI
	if ( pVotePanel->m_bIsYesNoVote )
	{
		// YES / NO UI
		wchar_t wzFinal[k_MAX_VOTE_NAME_LENGTH] = L"";
		wchar_t *pszText = g_pVGuiLocalize->Find( ::input->IsSteamControllerActive() ? "#GameUI_vote_yes_sc_instruction" : "#GameUI_vote_yes_pc_instruction" );
		if ( pszText )
		{
			UTIL_ReplaceKeyBindings( pszText, 0, wzFinal, sizeof( wzFinal ), ::input->IsSteamControllerActive() ? GAME_ACTION_SET_FPSCONTROLS : GAME_ACTION_SET_NONE );
			if ( pVotePanel->m_pVoteActive )
				pVotePanel->m_pVoteActive->SetControlString( "LabelOption1", wzFinal );
		}

		pszText = g_pVGuiLocalize->Find( ::input->IsSteamControllerActive() ? "#GameUI_vote_no_sc_instruction" : "#GameUI_vote_no_pc_instruction" );
		if ( pszText )
		{
			UTIL_ReplaceKeyBindings( pszText, 0, wzFinal, sizeof( wzFinal ), ::input->IsSteamControllerActive() ? GAME_ACTION_SET_FPSCONTROLS : GAME_ACTION_SET_NONE );
			if ( pVotePanel->m_pVoteActive )
				pVotePanel->m_pVoteActive->SetControlString( "LabelOption2", wzFinal );
		}
	}
	else
	{
		// GENERAL UI
		if ( pVotePanel->m_VoteSetupChoices.Count() )
		{
			// Clear the labels to prevent previous options from being displayed,
			// such as when there are fewer options this vote than the previous
			for ( int iIndex = 0; iIndex < MAX_VOTE_OPTIONS; iIndex++ )
			{
				// Construct Label name
				char szOptionNum[2];
				Q_snprintf( szOptionNum, sizeof( szOptionNum ), "%i", iIndex + 1 );

				char szVoteOptionCount[13] = "LabelOption";
				Q_strncat( szVoteOptionCount, szOptionNum, sizeof( szVoteOptionCount ), COPY_ALL_CHARACTERS );

				pVotePanel->m_pVoteActive->SetControlString( szVoteOptionCount, "" );
			}

			// Set us up the vote
			for ( int iIndex = 0; iIndex < pVotePanel->m_nVoteChoicesCount; iIndex++ )
			{
				// Construct Option name
				const char *pszChoiceName = pVotePanel->m_VoteSetupChoices[iIndex];

				char szOptionName[k_MAX_VOTE_NAME_LENGTH];
				Q_snprintf( szOptionName, sizeof( szOptionName ), "F%i. ", iIndex + 1 );

				Q_strncat( szOptionName, pszChoiceName, sizeof( szOptionName ), COPY_ALL_CHARACTERS );

				// Construct Label name
				char szOptionNum[2];
				Q_snprintf( szOptionNum, sizeof( szOptionNum ), "%i", iIndex + 1 );

				char szVoteOptionCount[13] = "LabelOption";
				Q_strncat( szVoteOptionCount, szOptionNum, sizeof( szVoteOptionCount ), COPY_ALL_CHARACTERS );

				// Set Label string
				if ( pVotePanel->m_pVoteActive )
				{
					pVotePanel->m_pVoteActive->SetControlString( szVoteOptionCount, szOptionName );
				}
			}
		}
	}

	// Is the target a player?
	int nTargetLabelX = pVotePanel->m_nVoteActiveIssueLabelX;
	C_BasePlayer *pTargetPlayer = NULL;
	if ( iTargetEntIndex )
	{
		pTargetPlayer = UTIL_PlayerByIndex( iTargetEntIndex );
		if ( pTargetPlayer )
		{
			pVotePanel->m_pVoteActiveTargetAvatar->SetPlayer( pTargetPlayer );
			pVotePanel->m_pVoteActiveTargetAvatar->SetShouldDrawFriendIcon( false );
			nTargetLabelX += ( pVotePanel->m_pVoteActiveTargetAvatar->GetWide() + XRES( 3 ) );
		}
	}
	pVotePanel->m_pVoteActiveIssueLabel->SetPos( nTargetLabelX, pVotePanel->m_nVoteActiveIssueLabelY );
	pVotePanel->m_pVoteActiveTargetAvatar->SetVisible( pTargetPlayer ?  true : false );

	IGameEvent *event = gameeventmanager->CreateEvent( "vote_started" );
	if ( event )
	{
		event->SetString( "issue", szIssue );
		event->SetString( "param1", szParam1 );
		event->SetInt( "team", pVotePanel->m_nVoteTeamIndex );
		event->SetInt( "initiator", pVotePanel->m_iVoteCallerIdx );
		event->SetInt( "voteidx", pVotePanel->m_nVoteIdx );
		gameeventmanager->FireEventClientSide( event );
	}

#ifdef TF_CLIENT_DLL
	if ( bShowNotif )
	{
		NotificationQueue_Add( new CTFVoteNotification( pszCallerName, pVotePanel->m_nVoteIdx ) );
	}
	else
	{
		pVotePanel->m_bShowVoteActivePanel = true;
	}
#else
	pVotePanel->m_bShowVoteActivePanel = true;
#endif	// TF_CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVote::MsgFunc_VotePass( bf_read &msg )
{
	if ( IsPlayingDemo() )
		return;

	int nVoteTeamIndex = msg.ReadByte();

	int nVoteIdx = msg.ReadLong();

	CHudVotePanel *pVotePanel = NULL;
	for ( int i = 0; i < ARRAYSIZE( m_pVotePanels ); i++ )
	{
		if ( m_pVotePanels[ i ]->m_nVoteIdx == nVoteIdx )
		{
			pVotePanel = m_pVotePanels[ i ];
			break;
		}
	}

	if ( !pVotePanel )
		return;

	pVotePanel->m_nVoteTeamIndex = nVoteTeamIndex;

	// Passed string
	char szResult[k_MAX_VOTE_NAME_LENGTH];
	szResult[0] = 0;
	msg.ReadString( szResult, sizeof(szResult) );

	// Detail string
	char szParam1[k_MAX_VOTE_NAME_LENGTH];
	szParam1[0] = 0;
	msg.ReadString( szParam1, sizeof(szParam1) );

	// Localize
	wchar_t *pwcParam;
	wchar_t wcParam[k_MAX_VOTE_NAME_LENGTH];

	wchar_t *pwcIssue;
	wchar_t wcIssue[k_MAX_VOTE_NAME_LENGTH];

	if ( Q_strlen( szParam1 ) > 0 )
	{
		if ( szParam1[0] == '#' )
		{
			pwcParam = g_pVGuiLocalize->Find( szParam1 );
		}
		else
		{
			// Convert to wchar
			g_pVGuiLocalize->ConvertANSIToUnicode( szParam1, wcParam, sizeof( wcParam ) );
			pwcParam = wcParam;
		}

		g_pVGuiLocalize->ConstructString_safe( wcIssue, g_pVGuiLocalize->Find( szResult ), 1, pwcParam );
		pwcIssue = wcIssue;
	}
	else
	{
		// No param, just localize the result
		pwcIssue = g_pVGuiLocalize->Find( szResult );
	}

	pVotePanel->m_pVotePassed->SetDialogVariable( "passedresult", pwcIssue );

	pVotePanel->m_bVotingActive = false;
	pVotePanel->m_bVotePassed = true;
	pVotePanel->m_flVoteResultCycleTime = gpGlobals->curtime;
	pVotePanel->m_flHideTime = gpGlobals->curtime + 5.f;
	pVotePanel->m_nVoteIdx = -1;

	// driller:  this event has no listeners - will eventually hook into stats
	IGameEvent *event = gameeventmanager->CreateEvent( "vote_passed" );
	if ( event )
	{
		event->SetString( "details", szResult );
		event->SetString( "param1", szParam1 );
		event->SetInt( "team", pVotePanel->m_nVoteTeamIndex );
		event->SetInt( "voteidx", pVotePanel->m_nVoteIdx );
		gameeventmanager->FireEventClientSide( event );
	}

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return;

	pLocalPlayer->EmitSound( "Vote.Passed" );
}

//-----------------------------------------------------------------------------
// Purpose: Creates a UI for Vote Issue selection
//-----------------------------------------------------------------------------
void CHudVote::MsgFunc_VoteSetup( bf_read &msg )
{
	if ( IsPlayingDemo() )
		return;


	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return;

	// Load up the list of Vote Issues
	m_VoteSetupIssues.RemoveAll();
	int nIssueCount = msg.ReadByte();
	if ( nIssueCount )
	{
		for ( int i = 0; i < nIssueCount; i++ )
		{
			char szIssue[k_MAX_VOTE_NAME_LENGTH];
			char szIssueString[k_MAX_VOTE_NAME_LENGTH];
			msg.ReadString( szIssue, sizeof( szIssue ) );
			msg.ReadString( szIssueString, sizeof( szIssueString ) );
			bool bIsActive = (bool)msg.ReadByte();
			
			m_bVoteSystemActive |= bIsActive;
			
			bool bAdd = true;
			FOR_EACH_VEC( m_VoteSetupIssues, j )
			{
				if ( !V_strcmp( szIssue, m_VoteSetupIssues[j].szName ) )
				{
					bAdd = false;
					break;
				}
			}

			if ( bAdd )
			{
				// When empty, assume that we just pre-pend #Vote_ to szIssue (reduces msg size)
				if ( !szIssueString[0] )
				{
					V_sprintf_safe( szIssueString, "#Vote_%s", szIssue );
				}

				VoteIssue_t issue;
				V_strcpy_safe( issue.szName, szIssue );
				V_strcpy_safe( issue.szNameString, szIssueString );
				issue.bIsActive = bIsActive;
				
				// Send it over to the listpanel
				m_VoteSetupIssues.AddToTail( issue );
			}
		}
	}

	m_pVoteSetupDialog->AddVoteIssues( m_VoteSetupIssues );

	// Load up the list of Vote Issue Parameters
	m_VoteSetupMapCycle.RemoveAll();

	// Use the appropriate stringtable for maps based on gamemode
	bool bMvM = false;
	INetworkStringTable *pStringTable = g_pStringTableServerMapCycle;

#ifdef TF_CLIENT_DLL
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		bMvM = true;
		pStringTable = g_pStringTableServerMapCycleMvM;
	}
#endif // TF_CLIENT_DLL

	if ( pStringTable )
	{
		int index = bMvM ? pStringTable->FindStringIndex( "ServerMapCycleMvM" ) : pStringTable->FindStringIndex( "ServerMapCycle" );
		if ( index != ::INVALID_STRING_INDEX )
		{
			int nLength = 0;
			const char *pszMapCycle = (const char *)pStringTable->GetStringUserData( index, &nLength );
			if ( pszMapCycle && pszMapCycle[0] )
			{
				if ( pszMapCycle && nLength )
				{
					V_SplitString( pszMapCycle, "\n", m_VoteSetupMapCycle );
				}

				// Alphabetize
				if ( m_VoteSetupMapCycle.Count() )
				{
					m_VoteSetupMapCycle.Sort( m_VoteSetupMapCycle.SortFunc );
				}
			}
		}
	}

#ifdef TF_CLIENT_DLL
	m_VoteSetupPopFiles.RemoveAll();
	if ( g_pStringTableServerPopFiles )
	{
		int index = g_pStringTableServerPopFiles->FindStringIndex( "ServerPopFiles" );
		if ( index != ::INVALID_STRING_INDEX )
		{
			int nLength = 0;
			const char *pszPopFiles = (const char *)g_pStringTableServerPopFiles->GetStringUserData( index, &nLength );
			if ( pszPopFiles && pszPopFiles[0] )
			{
				if ( pszPopFiles && nLength )
				{
					V_SplitString( pszPopFiles, "\n", m_VoteSetupPopFiles );
				}

				// Alphabetize
				if ( m_VoteSetupPopFiles.Count() )
				{
					m_VoteSetupPopFiles.Sort( m_VoteSetupPopFiles.SortFunc );
				}
			}
		}
	}
#endif // TF_CLIENT_DLL

	// Now send any data we gathered over to the listpanel
	PropagateOptionParameters();

	m_pVoteSetupDialog->Activate();
	m_pVoteSetupDialog->SetVisible( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudVote::IsPlayingDemo() const
{
	return engine->IsPlayingDemo();
}

bool CHudVote::IsShowingVoteSetupDialog()
{
	return m_pVoteSetupDialog && m_pVoteSetupDialog->IsEnabled() && m_pVoteSetupDialog->IsVisible();
}

void CHudVote::ShowVoteUI( int nVoteIdx, bool bShow )
{
	for ( int i = 0; i < ARRAYSIZE( m_pVotePanels ); i++ )
	{
		if ( m_pVotePanels[ i ]->m_nVoteIdx == nVoteIdx )
			m_pVotePanels[ i ]->ShowVoteUI( bShow );
	}
}

bool CHudVote::IsVoteUIActive()
{
	for ( int i = 0; i < ARRAYSIZE( m_pVotePanels ); i++ )
	{
		if ( m_pVotePanels[ i ]->IsVoteUIActive() )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:  Handles all UI for Voting
//-----------------------------------------------------------------------------
CHudVotePanel::CHudVotePanel( vgui::Panel *pParent, int nIdx ) : BaseClass( NULL, "CHudVotePanel" )
{
	m_nVotePanelIdx = nIdx;
	SetParent( pParent );

#ifdef TF_CLIENT_DLL
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
#endif

	SetProportional( true );

	for( int index = 0; index < MAX_VOTE_OPTIONS; index++ )
	{
		m_nVoteOptionCount[index] = 0;
	}
	m_pVoteActive = new EditablePanel( this, "VoteActive" );
	m_pVoteActiveIssueLabel = new vgui::Label( m_pVoteActive, "Issue", "" );
	m_pVoteActiveTargetAvatar = new CAvatarImagePanel( m_pVoteActive, "TargetAvatarImage" );
	m_voteBar = new VoteBarPanel( m_pVoteActive, "VoteBar" );
	m_pVoteFailed = new EditablePanel( this, "VoteFailed" );
	m_pVotePassed = new EditablePanel( this, "VotePassed" );
	m_pCallVoteFailed = new EditablePanel( this, "CallVoteFailed" );
}

bool CHudVotePanel::IsFirst()
{
	if ( !IsVisible() )
		return false;

	bool bFirst = true;

	CHudVote *pHudVote = GET_HUDELEMENT( CHudVote );
	if ( pHudVote )
	{
		CHudVotePanel *pOtherVotePanel = pHudVote->GetVotePanel( m_nVotePanelIdx == 1 ? 0 : 1 );

		if ( pOtherVotePanel && pOtherVotePanel->IsVisible() && pOtherVotePanel->m_nVoteIdx < m_nVoteIdx )
			bFirst = false;
	}

	return bFirst;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVotePanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetProportional( true );

	LoadControlSettings( "Resource/UI/VoteHud.res" );

	m_pVoteActiveIssueLabel->GetPos( m_nVoteActiveIssueLabelX, m_nVoteActiveIssueLabelY );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVotePanel::Init( void )
{
	m_bVotingActive = false;
	m_flVoteResultCycleTime = -1;
	m_flHideTime = -1;
	m_bIsYesNoVote = true;
	m_bPlayerVoted = false;
	m_nVoteChoicesCount = 2;  // Yes/No is the default
	m_bShowVoteActivePanel = false;
	m_iVoteCallerIdx = -1;
	m_nVoteTeamIndex = 0;
	m_nVoteIdx = -1;

	ListenForGameEvent( "vote_changed" );
	ListenForGameEvent( "vote_options" );
	ListenForGameEvent( "vote_cast" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVotePanel::LevelInit( void )
{
	m_bVotingActive = false;
	m_flVoteResultCycleTime = -1;
	m_flHideTime = -1;
	m_flPostVotedHideTime = -1;
	m_bPlayerVoted = false;
	m_bShowVoteActivePanel = false;
	m_nVoteIdx = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVotePanel::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();
	if ( !eventName )
		return;

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return;

	if( FStrEq( eventName, "vote_changed" ) )
	{
		if ( m_nVoteIdx != event->GetInt( "voteidx" ) )
			return;

		for ( int index = 0; index < MAX_VOTE_OPTIONS; index++ )
		{
			char szOption[2];
			Q_snprintf( szOption, sizeof( szOption ), "%i", index + 1 );

			char szVoteOptionCount[13] = "vote_option";
			Q_strncat( szVoteOptionCount, szOption, sizeof( szVoteOptionCount ), COPY_ALL_CHARACTERS );

			m_nVoteOptionCount[index] = event->GetInt( szVoteOptionCount );
		}
		m_nPotentialVotes = event->GetInt( "potentialVotes" );
	}
	else if ( FStrEq( eventName, "vote_options" ) )
	{
		if ( m_nVoteIdx != event->GetInt( "voteidx" ) )
			return;

		m_VoteSetupChoices.RemoveAll();
	
		m_nVoteChoicesCount = event->GetInt( "count" );
		for ( int iIndex = 0; iIndex < m_nVoteChoicesCount; iIndex++ )
		{
			char szNumber[2];
			Q_snprintf( szNumber, sizeof( szNumber ), "%i", iIndex + 1 );

			char szOptionName[8] = "option";
			Q_strncat( szOptionName, szNumber, sizeof( szOptionName ), COPY_ALL_CHARACTERS );

			const char *pszOptionName = event->GetString( szOptionName );
			m_VoteSetupChoices.CopyAndAddToTail( pszOptionName );
		}
	}
	else if ( FStrEq( eventName, "vote_cast" ) )
	{
		int iPlayer = event->GetInt( "entityid" );
		C_BasePlayer *pPlayer = UTIL_PlayerByIndex( iPlayer );
		if ( pPlayer != pLocalPlayer )
			return;

		if ( m_nVoteIdx != event->GetInt( "voteidx" ) )
			return;

		int vote_option = event->GetInt( "vote_option", TEAM_UNASSIGNED );
		if( vote_option == VOTE_OPTION1 )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pVoteActive, "PulseOption1" );
		}
		else if( vote_option == VOTE_OPTION2 )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pVoteActive, "PulseOption2" );
		}
		else if( vote_option == VOTE_OPTION3 )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pVoteActive, "PulseOption3" );
		}
		else if( vote_option == VOTE_OPTION4 )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pVoteActive, "PulseOption4" );
		}
		else if( vote_option == VOTE_OPTION5 )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pVoteActive, "PulseOption5" );
		}

		m_bPlayerVoted = true;

		bool bForceActive = false;
#ifdef TF_CLIENT_DLL
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			if ( m_iVoteCallerIdx == GetLocalPlayerIndex() )
			{
				bForceActive = true;
			}
		}
#endif // TF_CLIENT_DLL

		if ( !cl_vote_ui_active_after_voting.GetBool() && !bForceActive )
		{
			m_flPostVotedHideTime = gpGlobals->curtime + 1.5f;
		}
	}
}

ConVar cl_vote_non_input_alpha( "cl_vote_non_input_alpha", "150", FCVAR_NONE );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVotePanel::OnThink()
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer )
	{
		bool bShowToPlayer = ( !m_nVoteTeamIndex || pLocalPlayer->GetTeamNumber() == m_nVoteTeamIndex );

		// We delay hiding the menu after we cast a vote
		if ( m_bPlayerVoted && m_flPostVotedHideTime > 0 && gpGlobals->curtime > m_flPostVotedHideTime )
		{
			m_pVoteActive->SetVisible( false );
			m_bShowVoteActivePanel = false;
			m_flPostVotedHideTime = -1;
		}

		if ( m_flVoteResultCycleTime > 0 && gpGlobals->curtime > m_flVoteResultCycleTime )
		{
			m_pVoteActive->SetVisible( false );
			m_pVoteFailed->SetVisible( !m_bVotePassed && bShowToPlayer );
			m_pVotePassed->SetVisible( m_bVotePassed && bShowToPlayer );
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pVoteActive, "HideVoteBackgrounds" );

			m_flVoteResultCycleTime = -1;
			m_bPlayerVoted = false;
			m_bVotingActive = false;
			m_bShowVoteActivePanel = false;
			m_iVoteCallerIdx = -1;
		}

		if ( m_bVotingActive && m_bShowVoteActivePanel )
		{
			// driller:  Need to rewrite this to handle all vote types (Yes/No and General)
			if ( m_bIsYesNoVote && m_pVoteActive )
			{
				char szYesCount[k_MAX_VOTE_NAME_LENGTH] = "";
				Q_snprintf( szYesCount, sizeof( szYesCount ), "%d", m_nVoteOptionCount[0] );

				char szNoCount[k_MAX_VOTE_NAME_LENGTH] = "";
				Q_snprintf( szNoCount, sizeof( szNoCount ), "%d", m_nVoteOptionCount[1] );

				m_pVoteActive->SetControlString( "Option1CountLabel", szYesCount );
				m_pVoteActive->SetControlString( "Option2CountLabel", szNoCount );
			}

			if ( !m_pVoteActive->IsVisible() && bShowToPlayer )
			{
				m_pVoteActive->SetVisible( true );
				pLocalPlayer->EmitSound("Vote.Created");
			}
		}
	}

	bool bFirst = IsFirst();
	SetPos( ( bFirst ? 0 : 1 ) * m_pVoteActive->GetWide() + vgui::scheme()->GetProportionalScaledValueEx( GetScheme(), 4 ), 0 );

	// Josh: Set alpha to indicate which is our
	// active input panel.
	// Also helps greatly with visibility after voting!
	CHudVote *pHudVote = GET_HUDELEMENT( CHudVote );
	if ( pHudVote )
	{
		CHudVotePanel *pInputPanel = pHudVote->GetInputVotePanel();
		SetAlpha( pInputPanel == this ? 255 : cl_vote_non_input_alpha.GetInt() );
	}

	BaseClass::OnThink();
}

bool CHudVotePanel::IsVisible()
{
	if ( !ShouldDraw() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudVotePanel::ShouldDraw( void )
{
	return ( m_bVotingActive || gpGlobals->curtime < m_flHideTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudVotePanel::IsVoteUIActive( void )
{
	return m_bShowVoteActivePanel;
}

bool CHudVotePanel::IsShowingVotingUI()
{
	return m_pVoteActive && m_pVoteActive->IsEnabled() && m_pVoteActive->IsVisible();
}

