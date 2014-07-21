//========= Copyright Valve Corporation, All rights reserved. ============//
//
//===========================================================================//


#include "cbase.h"
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
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_vote_ui_active_after_voting( "cl_vote_ui_active_after_voting", "0" );
ConVar cl_vote_ui_show_notification( "cl_vote_ui_show_notification", "0" );

#ifdef TF_CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFVoteNotification : public CEconNotification
{
public:
	CTFVoteNotification( const char *pPlayerName ) : CEconNotification()
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( pPlayerName, m_wszPlayerName, sizeof(m_wszPlayerName) );
		SetLifetime( 7 );
		SetText( "#Vote_notification_text" );
		AddStringToken( "initiator", m_wszPlayerName );
	}
	virtual bool CanBeTriggered()
	{
		return true;
	}
	virtual void Trigger()
	{
		CTFGenericConfirmDialog *pDialog = ShowConfirmDialog( "#Vote_notification_title", "#Vote_notification_text", "#Vote_notification_view", "#cancel", &ConfirmShowVoteSetup );
		pDialog->SetContext( this );
		pDialog->AddStringToken( "initiator", m_wszPlayerName );
		// so we aren't deleted
		SetIsInUse( true );
	}
	virtual bool CanBeAcceptedOrDeclined()
	{
		return true;
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
				pHudVote->ShowVoteUI();
			}
		}
		pNotification->SetIsInUse( false );
		pNotification->MarkForDeletion();
	}

public:
	wchar_t m_wszPlayerName[MAX_PLAYER_NAME_LENGTH];
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

	const char *pszColor = inResourceData->GetString( "issue_fgcolor", "Label.TextColor" );
	m_IssueFGColor = pScheme->GetColor( pszColor, Color( 255, 255, 255, 255 ) );

	pszColor = inResourceData->GetString( "issue_fgcolor_disabled", "Label.TextColor" );
	m_IssueFGColorDisabled = pScheme->GetColor( pszColor, Color( 255, 255, 255, 255 ) );

	pszColor = inResourceData->GetString( "header_fgcolor", "Label.TextColor" );
	m_HeaderFGColor = pScheme->GetColor( pszColor, Color( 255, 255, 255, 255 ) );
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
void CVoteSetupDialog::AddVoteIssues( CUtlStringList &m_VoteSetupIssues )
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
	m_pVoteParameterList->AddColumnToSection( 0, "Avatar", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, 55 );
	m_pVoteParameterList->AddColumnToSection( 0, "", "", 0, 10 );	// Spacer
	m_pVoteParameterList->AddColumnToSection( 0, "Name", "#TF_Vote_Column_Name", 0, m_iParameterWidth * 0.6 );
	m_pVoteParameterList->AddColumnToSection( 0, "Properties", "#TF_Vote_Column_Properties", SectionedListPanel::COLUMN_CENTER, m_iParameterWidth * 0.3 );

	if ( m_hHeaderFont != INVALID_FONT )
	{
		m_pVoteParameterList->SetFontSection( 0, m_hHeaderFont );
		m_pVoteParameterList->SetSectionFgColor( 0, m_HeaderFGColor );
		m_pVoteParameterList->SetFontSection( 1, m_hHeaderFont );
		m_pVoteParameterList->SetSectionFgColor( 1, m_HeaderFGColor );
	}

	// Populate the Issue list
	for ( int index = 0; index < m_VoteIssues.Count(); index++ )
	{
		const char *pszIssue = m_VoteIssues[index];
		if ( !pszIssue || !pszIssue[0] )
			continue;

		KeyValues *pKeyValues = new KeyValues( "Issue" );
		pKeyValues->SetString( "Issue", pszIssue );
		int iId = m_pVoteSetupList->AddItem( 0, pKeyValues );
		pKeyValues->deleteThis();

		// Setup the list entry style
		if ( m_hIssueFont != INVALID_FONT )
		{
			m_pVoteSetupList->SetItemFont( iId, m_hIssueFont );

			bool bDisabled = V_stristr( pszIssue, "(Disabled on Server)" );		// driller: need to localize
			Color colFG = bDisabled ? m_IssueFGColorDisabled : m_IssueFGColor;
			m_pVoteSetupList->SetItemFgColor( iId, colFG );
		}
	}

	// Select the first item by default
	if ( m_pVoteSetupList->GetItemCount() > 0 )
	{
		m_pVoteSetupList->SetSelectedItem( 0 );
	}

	UpdateCurrentMap();
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
	if ( V_stricmp( command, "CallVote" ) == 0 )
	{
		int iSelectedItem = m_pVoteSetupList->GetSelectedItem();
		if ( iSelectedItem >= 0 )
		{
			char szVoteCommand[128];
			KeyValues *pIssueKeyValues = m_pVoteSetupList->GetItemData( iSelectedItem );
			const char *szIssue = pIssueKeyValues->GetString( "Issue" );
			if ( V_stricmp( "changelevel", szIssue ) == 0 || V_stricmp( "nextlevel", szIssue ) == 0 )
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
							Q_snprintf( szVoteCommand, sizeof( szVoteCommand ), "callvote %s %s\n;", szIssue, szMapName );
							engine->ClientCmd( szVoteCommand );
						}
					}
				}
			}
			else if ( V_stricmp( "kick", szIssue ) == 0 )
			{
				// Get selected Player
				int iSelectedParam = m_pVoteParameterList->GetSelectedItem();
				if ( iSelectedParam >= 0 )
				{
					KeyValues *pKeyValues = m_pVoteParameterList->GetItemData( iSelectedParam );
					if ( pKeyValues )
					{
						// Is Player valid?
						int playerIndex = pKeyValues->GetInt( "index" );
						const char *pReasonString = m_pComboBox->GetActiveItemUserData() ? m_pComboBox->GetActiveItemUserData()->GetName() : "other";
						player_info_t playerInfo;
						if ( engine->GetPlayerInfo( playerIndex, &playerInfo ) )
						{
							CBasePlayer *pPlayer = UTIL_PlayerByIndex( playerIndex );
							Q_snprintf( szVoteCommand, sizeof( szVoteCommand ), "callvote %s \"%d %s\"\n;", szIssue, pPlayer->GetUserID(), pReasonString );
							engine->ClientCmd( szVoteCommand );
#ifdef TF_CLIENT_DLL
							CSteamID steamID;
							CTFPlayer* pSubject = ToTFPlayer( pPlayer );
							if ( pSubject && pSubject->GetSteamID( &steamID ) && steamID.GetAccountID() != 0 )
							{
								GCSDK::CProtoBufMsg<CMsgTFVoteKickBanPlayer> msg( k_EMsgGCVoteKickBanPlayer );
								uint32 reason = GetKickBanPlayerReason( pReasonString );
								msg.Body().set_account_id_subject( steamID.GetAccountID() );
								msg.Body().set_kick_reason( reason );
								GCClientSystem()->BSendMessage( msg );
							}
#endif
						}
					}
				}
			}
#ifdef TF_CLIENT_DLL
			else if ( V_stricmp( "ChangeMission", szIssue ) == 0 )
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
							Q_snprintf( szVoteCommand, sizeof( szVoteCommand ), "callvote %s %s\n;", szIssue, szPopFile );
							engine->ClientCmd( szVoteCommand );
						}
					}
				}
			}
#endif	// TF_CLIENT_DLL
			else
			{
				// Non-parameter vote.  i.e.  callvote scrambleteams
				Q_snprintf( szVoteCommand, sizeof(szVoteCommand), "callvote %s\n;", szIssue );
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

			// We're rebuilding, so clear state
			m_bVoteButtonEnabled = false;
			m_pVoteParameterList->ClearSelection();
			m_pVoteParameterList->RemoveAll();

			const char *szName = pIssueKeyValues->GetString( "Issue" );
			if ( V_stricmp( "Voting disabled on this Server", szName ) == 0 )
			{
				m_bVoteButtonEnabled = false;
			}
			else if ( V_stristr( szName, "(Disabled on Server)" ) )		// driller: need to localize
			{
				m_bVoteButtonEnabled = false;
			}
			// CHANGELEVEL / NEXTLEVEL
			else if ( V_stricmp( "changelevel", szName ) == 0 || V_stricmp( "nextlevel", szName ) == 0 )
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
			else if ( V_stricmp( "kick", szName ) == 0 )
			{
				// Feed the player list to the parameters list
				int nMaxClients = engine->GetMaxClients();
				for ( int playerIndex = 1; playerIndex <= nMaxClients; playerIndex++ )
				{
					C_BasePlayer *pPlayer = UTIL_PlayerByIndex( playerIndex );
					if ( !pPlayer )
						continue;

					C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
					if ( !pLocalPlayer )
						continue;

					if ( pPlayer == pLocalPlayer )
						continue;

					bool bAllowKickUnassigned = false;
#ifdef TF_CLIENT_DLL
					// Allow kicking team unassigned in MvM
					if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && g_PR->IsConnected( playerIndex ) && pPlayer->GetTeamNumber() == TEAM_UNASSIGNED )
					{
						bAllowKickUnassigned = true;
					}
#endif // TF_CLIENT_DLL
					
					// Can't kick people on the other team, so don't list them
					if ( pPlayer->GetTeam() != pLocalPlayer->GetTeam() && !bAllowKickUnassigned )
						continue;

					char szPlayerIndex[32];
					Q_snprintf( szPlayerIndex, sizeof( szPlayerIndex ), "%d", playerIndex );

					KeyValues *pKeyValues = new KeyValues( szPlayerIndex );
					pKeyValues->SetString( "Name", pPlayer->GetPlayerName() );
					pKeyValues->SetInt( "index", playerIndex );
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
			else if ( V_stricmp( "ChangeMission", szName ) == 0 )
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
		const char *szName = pIssueKeyValues->GetString( "Issue" );
		if ( V_stricmp( "kick", szName ) == 0 )
		{
			if ( m_pVoteParameterList->GetItemCount() > 0 )
			{
				for ( int index = 0; index < m_pVoteParameterList->GetItemCount(); index++ )
				{
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

					pKeyValues->SetString( "Name", playerInfo.name );

					if ( playerInfo.fakeplayer )
					{
						pKeyValues->SetString( "Properties", "Bot" );
					}
					else
					{
						pKeyValues->SetString( "Properties", "" );
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

					m_pVoteParameterList->InvalidateItem( index );
				}

				m_pVoteParameterList->SetImageList( m_pImageList, false );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVoteSetupDialog::ResetData()
{
	m_bVoteButtonEnabled = false;
 	m_pVoteSetupList->DeleteAllItems();
	m_pVoteParameterList->DeleteAllItems();
	m_pComboBox->DeleteAllItems();
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

//-----------------------------------------------------------------------------
// Purpose:  Handles all UI for Voting
//-----------------------------------------------------------------------------
CHudVote::CHudVote( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "CHudVote" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

#ifdef TF_CLIENT_DLL
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
#endif

	SetHiddenBits( 0 );
	for( int index = 0; index < MAX_VOTE_OPTIONS; index++ )
	{
		m_nVoteOptionCount[index] = 0;
	}
	m_pVoteActive = new EditablePanel( this, "VoteActive" );
	m_voteBar = new VoteBarPanel( m_pVoteActive, "VoteBar" );
	m_pVoteFailed = new EditablePanel( this, "VoteFailed" );
	m_pVotePassed = new EditablePanel( this, "VotePassed" );
	m_pCallVoteFailed = new EditablePanel( this, "CallVoteFailed" );
	m_pVoteSetupDialog = new CVoteSetupDialog( pParent );

	RegisterForRenderGroup( "mid" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVote::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetProportional( true );

	LoadControlSettings( "Resource/UI/VoteHud.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVote::Init( void )
{
	ListenForGameEvent( "vote_changed" );
	ListenForGameEvent( "vote_options" );
	ListenForGameEvent( "vote_cast" );

	SetVoteActive( false );
	m_flVoteResultCycleTime = -1;
	m_flHideTime = -1;
	m_bIsYesNoVote = true;
	m_bPlayerVoted = false;
	m_nVoteChoicesCount = 2;  // Yes/No is the default
	m_bShowVoteActivePanel = false;
	m_iVoteCallerIdx = -1;

	HOOK_HUD_MESSAGE( CHudVote, CallVoteFailed );
	HOOK_HUD_MESSAGE( CHudVote, VoteStart );
	HOOK_HUD_MESSAGE( CHudVote, VotePass );
	HOOK_HUD_MESSAGE( CHudVote, VoteFailed );
	HOOK_HUD_MESSAGE( CHudVote, VoteSetup );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVote::LevelInit( void )
{
	SetVoteActive( false );
	m_flVoteResultCycleTime = -1;
	m_flHideTime = -1;
	m_flPostVotedHideTime = -1;
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

	if ( !m_bVoteActive )
		return 1;

 	if ( m_bPlayerVoted )
 		return 1;

	if ( !m_bShowVoteActivePanel )
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
	if ( nSlot > m_nVoteChoicesCount )
		return 1;

	char szNumber[2];
	Q_snprintf( szNumber, sizeof( szNumber ), "%i", nSlot );

	char szOptionName[13] = "vote option";
	Q_strncat( szOptionName, szNumber, sizeof( szOptionName ), COPY_ALL_CHARACTERS );

	engine->ClientCmd( szOptionName );

	return 0;
}

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

	pLocalPlayer->EmitSound("Vote.Failed");

	m_pVoteActive->SetVisible( false );
	m_pVoteFailed->SetVisible( false );
	m_pVotePassed->SetVisible( false );
	m_pCallVoteFailed->SetVisible( true );
	m_pVoteSetupDialog->SetVisible( false );

	m_flHideTime = gpGlobals->curtime + 4.0;

	char szTime[256];
	wchar_t wszTime[256];
	Q_snprintf( szTime, sizeof ( szTime), "%i", nTime );
	g_pVGuiLocalize->ConvertANSIToUnicode( szTime, wszTime, sizeof( wszTime ) );

	wchar_t wszHeaderString[512];
	wchar_t *pwszHeaderString;

	switch( nReason )
	{
		case VOTE_FAILED_GENERIC:
			m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed" );
			break;

		case VOTE_FAILED_TRANSITIONING_PLAYERS:
			m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_transition_vote" );
			break;

		case VOTE_FAILED_RATE_EXCEEDED:
			g_pVGuiLocalize->ConstructString( wszHeaderString, sizeof(wszHeaderString), g_pVGuiLocalize->Find( "#GameUI_vote_failed_vote_spam" ), 1, wszTime );
			pwszHeaderString = wszHeaderString;
			m_pCallVoteFailed->SetDialogVariable( "FailedReason", pwszHeaderString );
			break;

		case VOTE_FAILED_ISSUE_DISABLED:
			m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_disabled_issue" );
			break;

		case VOTE_FAILED_MAP_NOT_FOUND:
			m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_map_not_found" );
			break;

		case VOTE_FAILED_MAP_NOT_VALID:
			m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_map_not_valid" );
			break;

		case VOTE_FAILED_MAP_NAME_REQUIRED:
			m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_map_name_required" );
			break;

		case VOTE_FAILED_FAILED_RECENTLY:
			g_pVGuiLocalize->ConstructString( wszHeaderString, sizeof(wszHeaderString), g_pVGuiLocalize->Find( "#GameUI_vote_failed_recently" ), 1, wszTime );
			pwszHeaderString = wszHeaderString;
			m_pCallVoteFailed->SetDialogVariable( "FailedReason", pwszHeaderString );
			break;

		case VOTE_FAILED_TEAM_CANT_CALL:
			m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_team_cant_call" );
			break;

		case VOTE_FAILED_WAITINGFORPLAYERS:
			m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_waitingforplayers" );
			break;

		case VOTE_FAILED_CANNOT_KICK_ADMIN:
			m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_cannot_kick_admin" );
			break;

		case VOTE_FAILED_SCRAMBLE_IN_PROGRESS:
			m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_scramble_in_prog" );
			break;

		case VOTE_FAILED_SPECTATOR:
			m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_spectator" );
			break;

		case VOTE_FAILED_NEXTLEVEL_SET:
			m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_nextlevel_set" );
			break;

		case VOTE_FAILED_CANNOT_KICK_FOR_TIME:
			g_pVGuiLocalize->ConstructString( wszHeaderString, sizeof(wszHeaderString), g_pVGuiLocalize->Find( "#GameUI_vote_failed_cannot_kick_for_time" ), 1, wszTime );
			pwszHeaderString = wszHeaderString;
			m_pCallVoteFailed->SetDialogVariable( "FailedReason", pwszHeaderString );
			break;

		case VOTE_FAILED_CANNOT_KICK_DURING_ROUND:
			m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_round_active" );
			break;

		case VOTE_FAILED_MODIFICATION_ALREADY_ACTIVE:
			m_pCallVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_event_already_active" );
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

	int iTeam = msg.ReadByte();
	vote_create_failed_t nReason = (vote_create_failed_t)msg.ReadByte();

	// Visibility of this error is handled by OnThink()
	SetVoteActive( false );
	m_bVotePassed = false;
	m_flVoteResultCycleTime = gpGlobals->curtime + 2;
	m_flHideTime = gpGlobals->curtime + 5;

	switch ( nReason )
	{
	case VOTE_FAILED_GENERIC:
		m_pVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed" );
		break;

	case VOTE_FAILED_YES_MUST_EXCEED_NO:
		m_pVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_yesno" );
		break;

	case VOTE_FAILED_QUORUM_FAILURE:
		m_pVoteFailed->SetControlString( "FailedReason", "#GameUI_vote_failed_quorum" );
		break;
	}

	// driller:  this event has no listeners - will eventually hook into stats
	IGameEvent *event = gameeventmanager->CreateEvent( "vote_failed" );
	if ( event )
	{
		event->SetInt( "team", iTeam );
		gameeventmanager->FireEventClientSide( event );
	}

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return;

	pLocalPlayer->EmitSound("Vote.Failed");
}

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
	int iTeam = msg.ReadByte();
	uint8 invalidTeam = (uint8)TEAM_INVALID;
	if ( iTeam != invalidTeam && iTeam != pLocalPlayer->GetTeamNumber() )
		return;

	// Entity calling the vote
	bool bShowNotif = cl_vote_ui_show_notification.GetBool();
	const char *pszCallerName = "Server";
	m_iVoteCallerIdx = msg.ReadByte();
	if ( m_iVoteCallerIdx != DEDICATED_SERVER )
	{
		C_BasePlayer *pVoteCaller = UTIL_PlayerByIndex( m_iVoteCallerIdx );
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
	char szIssue[256];
	szIssue[0] = 0;
	msg.ReadString( szIssue, sizeof(szIssue) );

	// DetailString
	char szParam1[256];
	szParam1[0] = 0;
	msg.ReadString( szParam1, sizeof(szParam1) );

	m_bIsYesNoVote = msg.ReadByte();

	SetVoteActive( true );
	m_pVoteFailed->SetVisible( false );
	m_pVotePassed->SetVisible( false );
	m_pCallVoteFailed->SetVisible( false );
	m_pVoteSetupDialog->SetVisible( false );
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pVoteActive, "HideVoteBackgrounds" );

	m_voteBar->SetVisible( m_bIsYesNoVote );

	// There will always be at least two choices...
	m_pVoteActive->SetControlVisible( "LabelOption1", true );
	m_pVoteActive->SetControlVisible( "LabelOption2", true );

	// ...sometimes more
	m_pVoteActive->SetControlVisible( "LabelOption3", m_VoteSetupChoices.Count() > 2 ? true : false );
	m_pVoteActive->SetControlVisible( "Option3Background_Selected", m_VoteSetupChoices.Count() > 2 ? true : false );
	m_pVoteActive->SetControlVisible( "LabelOption4", m_VoteSetupChoices.Count() > 3 ? true : false );
	m_pVoteActive->SetControlVisible( "Option4Background_Selected", m_VoteSetupChoices.Count() > 3 ? true : false );
	m_pVoteActive->SetControlVisible( "LabelOption5", m_VoteSetupChoices.Count() > 4 ? true : false );
	m_pVoteActive->SetControlVisible( "Option5Background_Selected", m_VoteSetupChoices.Count() > 4 ? true : false );

	m_pVoteActive->SetControlVisible( "VoteCountLabel", m_bIsYesNoVote );
	m_pVoteActive->SetControlVisible( "Option1CountLabel", m_bIsYesNoVote );
	m_pVoteActive->SetControlVisible( "Option2CountLabel", m_bIsYesNoVote );
	m_pVoteActive->SetControlVisible( "Divider1", m_bIsYesNoVote );
	m_pVoteActive->SetControlVisible( "Divider2", m_bIsYesNoVote );

	// Display vote caller's name
	wchar_t wszCallerName[MAX_PLAYER_NAME_LENGTH];
	
	wchar_t wszHeaderString[512];
	wchar_t *pwszHeaderString;

	// Player
	g_pVGuiLocalize->ConvertANSIToUnicode( pszCallerName, wszCallerName, sizeof( wszCallerName ) );

	// String
	g_pVGuiLocalize->ConstructString( wszHeaderString, sizeof(wszHeaderString), g_pVGuiLocalize->Find( "#GameUI_vote_header" ), 1, wszCallerName );
	pwszHeaderString = wszHeaderString;

	// Final
	m_pVoteActive->SetDialogVariable( "header", pwszHeaderString );

	// Display the Issue
	wchar_t *pwcParam;
	wchar_t wcParam[128];

	wchar_t *pwcIssue;
	wchar_t wcIssue[512];

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

		g_pVGuiLocalize->ConstructString( wcIssue, sizeof(wcIssue), g_pVGuiLocalize->Find( szIssue ), 1, pwcParam );
		pwcIssue = wcIssue;
	}
	else
	{
		// no param, just localize the issue
		pwcIssue = g_pVGuiLocalize->Find( szIssue );
	}
	m_pVoteActive->SetDialogVariable( "voteissue", pwcIssue );

	// Figure out which UI
	if ( m_bIsYesNoVote )
	{
		// YES / NO UI
		wchar_t wzFinal[512] = L"";
		wchar_t *pszText = g_pVGuiLocalize->Find( "#GameUI_vote_yes_pc_instruction" );
		if ( pszText )
		{
			UTIL_ReplaceKeyBindings( pszText, 0, wzFinal, sizeof( wzFinal ) );
			if ( m_pVoteActive )
				m_pVoteActive->SetControlString( "LabelOption1", wzFinal );
		}

		pszText = g_pVGuiLocalize->Find( "#GameUI_vote_no_pc_instruction" );
		if ( pszText )
		{
			UTIL_ReplaceKeyBindings( pszText, 0, wzFinal, sizeof( wzFinal ) );
			if ( m_pVoteActive )
				m_pVoteActive->SetControlString( "LabelOption2", wzFinal );
		}
	}
	else
	{
		// GENERAL UI
		if ( m_VoteSetupChoices.Count() )
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

				m_pVoteActive->SetControlString( szVoteOptionCount, "" );
			}

			// Set us up the vote
			for ( int iIndex = 0; iIndex < m_nVoteChoicesCount; iIndex++ )
			{
				// Construct Option name
				const char *pszChoiceName = m_VoteSetupChoices[iIndex];

				char szOptionName[256];
				Q_snprintf( szOptionName, sizeof( szOptionName ), "F%i. ", iIndex + 1 );

				Q_strncat( szOptionName, pszChoiceName, sizeof( szOptionName ), COPY_ALL_CHARACTERS );

				// Construct Label name
				char szOptionNum[2];
				Q_snprintf( szOptionNum, sizeof( szOptionNum ), "%i", iIndex + 1 );

				char szVoteOptionCount[13] = "LabelOption";
				Q_strncat( szVoteOptionCount, szOptionNum, sizeof( szVoteOptionCount ), COPY_ALL_CHARACTERS );

				// Set Label string
				if ( m_pVoteActive )
				{
					m_pVoteActive->SetControlString( szVoteOptionCount, szOptionName );
				}
			}
		}
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "vote_started" );
	if ( event )
	{
		event->SetString( "issue", szIssue );
		event->SetString( "param1", szParam1 );
		event->SetInt( "team", iTeam );
		event->SetInt( "initiator", m_iVoteCallerIdx );
		gameeventmanager->FireEventClientSide( event );
	}

#ifdef TF_CLIENT_DLL
	if ( bShowNotif )
	{
		NotificationQueue_Add( new CTFVoteNotification( pszCallerName ) );
	}
	else
	{
		ShowVoteUI();
	}
#else
	ShowVoteUI();
#endif	// TF_CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVote::MsgFunc_VotePass( bf_read &msg )
{
	if ( IsPlayingDemo() )
		return;

	int iTeam = msg.ReadByte();

	// Passed string
	char szResult[256];
	szResult[0] = 0;
	msg.ReadString( szResult, sizeof(szResult) );

	// Detail string
	char szParam1[256];
	szParam1[0] = 0;
	msg.ReadString( szParam1, sizeof(szParam1) );

	// Localize
	wchar_t *pwcParam;
	wchar_t wcParam[128];

	wchar_t *pwcIssue;
	wchar_t wcIssue[512];

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

		g_pVGuiLocalize->ConstructString( wcIssue, sizeof(wcIssue), g_pVGuiLocalize->Find( szResult ), 1, pwcParam );
		pwcIssue = wcIssue;
	}
	else
	{
		// No param, just localize the result
		pwcIssue = g_pVGuiLocalize->Find( szResult );
	}

	m_pVotePassed->SetDialogVariable( "passedresult", pwcIssue );

	SetVoteActive( false );
	m_bVotePassed = true;
	m_flVoteResultCycleTime = gpGlobals->curtime + 2;
	m_flHideTime = gpGlobals->curtime + 5;

	// driller:  this event has no listeners - will eventually hook into stats
	IGameEvent *event = gameeventmanager->CreateEvent( "vote_passed" );
	if ( event )
	{
		event->SetString( "details", szResult );
		event->SetString( "param1", szParam1 );
		event->SetInt( "team", iTeam );
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

	m_pVoteActive->SetVisible( false );
	m_pVoteFailed->SetVisible( false );
	m_pVotePassed->SetVisible( false );
	m_pCallVoteFailed->SetVisible( false );

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return;

	// Load up the list of Vote Issues
	m_VoteSetupIssues.RemoveAll();
	int nIssueCount = msg.ReadByte();
	if ( nIssueCount )
	{
		for ( int index = 0; index < nIssueCount; index++ )
		{
			char szIssue[256];
			msg.ReadString( szIssue, sizeof(szIssue) );
			if ( !m_VoteSetupIssues.HasElement( szIssue ) )
			{
				// Send it over to the listpanel
				m_VoteSetupIssues.CopyAndAddToTail( szIssue );
			}
		}
	}
	else
	{
		m_VoteSetupIssues.CopyAndAddToTail( "Voting disabled on this Server" );
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVote::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();
	if ( !eventName )
		return;

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return;

	if( FStrEq( eventName, "vote_changed" ) )
	{
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVote::OnThink()
{
	// We delay hiding the menu after we cast a vote
	if ( m_bPlayerVoted && m_flPostVotedHideTime > 0 && m_flPostVotedHideTime < gpGlobals->curtime )
	{
		m_pVoteActive->SetVisible( false );
		m_bShowVoteActivePanel = false;
		m_flPostVotedHideTime = -1;
	}

	if ( m_flVoteResultCycleTime > 0 && m_flVoteResultCycleTime < gpGlobals->curtime )
	{
		m_pVoteActive->SetVisible( false );
		m_pVoteFailed->SetVisible( !m_bVotePassed );
		m_pVotePassed->SetVisible( m_bVotePassed );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pVoteActive, "HideVoteBackgrounds" );

		m_flVoteResultCycleTime = -1;
		m_bPlayerVoted = false;
		m_bVoteActive = false;
		m_bShowVoteActivePanel = false;
		m_iVoteCallerIdx = -1;
	}

	if ( m_bVoteActive )
	{
		// driller:  Need to rewrite this to handle all vote types (Yes/No and General)
		if ( m_bIsYesNoVote && m_pVoteActive )
		{
			char szYesCount[512] = "";
			Q_snprintf( szYesCount, 512, "%d", m_nVoteOptionCount[0] );

			char szNoCount[512] = "";
			Q_snprintf( szNoCount, 512, "%d", m_nVoteOptionCount[1] );

			m_pVoteActive->SetControlString( "Option1CountLabel", szYesCount );
			m_pVoteActive->SetControlString( "Option2CountLabel", szNoCount );
		}

		if ( !m_pVoteActive->IsVisible() && m_bShowVoteActivePanel )
		{
			m_pVoteActive->SetVisible( true );

			C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
			if ( pLocalPlayer )
			{
				pLocalPlayer->EmitSound("Vote.Created");
			}
		}
	}

	BaseClass::OnThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudVote::ShouldDraw( void )
{
	return ( m_bVoteActive || m_flHideTime > gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudVote::IsPlayingDemo() const
{
	return engine->IsPlayingDemo();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVote::SetVoteActive( bool bActive )
{
	m_bVoteActive = bActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVote::ShowVoteUI( void )
{
	m_bShowVoteActivePanel = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudVote::IsVoteUIActive( void )
{
	return m_bShowVoteActivePanel;
}

