//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_matchmaking_dashboard.h"
#include "tf_gamerules.h"
#include "ienginevgui.h"
#include "clientmode_tf.h"
#include "tf_hud_disconnect_prompt.h"
#include "tf_gc_client.h"
#include "tf_party.h"
#include <vgui/ISurface.h>
#include "vgui_avatarimage.h"
#include "tf_matchmaking_dashboard_chat.h"
#include "tf_matchmaking_dashboard_parent_manager.h"
#include "tf_partyclient.h"
#include "vgui_controls/AnimationController.h"
#include "../../vgui2/src/VPanel.h"

using namespace vgui;
using namespace GCSDK;

ConVar tf_chat_popup_hold_time( "tf_chat_popup_hold_time", "5", FCVAR_ARCHIVE );

Panel* GetGlobalPartyChatPanel()
{
	Panel* pPanel = new CPartyChatPanel( NULL, "partychat" );
	pPanel->MakeReadyForUse();
	pPanel->AddActionSignalTarget( GetMMDashboard() );
	return pPanel;
}
REGISTER_FUNC_FOR_DASHBOARD_PANEL_TYPE( GetGlobalPartyChatPanel, k_eChat );


ChatTextEntry::ChatTextEntry( vgui::Panel *parent, const char *name ) 
	: vgui::TextEntry( parent, name )
{
	SetCatchEnterKey( true );
	SetAllowNonAsciiCharacters( true );
	SetDrawLanguageIDAtLeft( true );
}

void ChatTextEntry::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
	// We send messages via keyvalue strings, which are capped at 256.
	// Save 1 for the null
	SetMaximumCharCount( 255 );
}

void ChatTextEntry::OnKeyCodeTyped(vgui::KeyCode code)
{
	if ( code == KEY_ENTER || code == KEY_PAD_ENTER )
	{
		if ( GetTextLength() > 0 )
		{
			int nBufSizeBytes = ( GetTextLength() + 4 ) * sizeof( wchar_t );
			wchar_t *wText = (wchar_t *)stackalloc( nBufSizeBytes );
			GetText( wText, nBufSizeBytes );
			TextEntry::SetText("");

			// Convert to UTF8, which is really what we should
			// use for everything
			int nUtf8BufferSizeBytes = MAX_PARTY_CHAT_MSG;
			char *szText = (char *)stackalloc( nUtf8BufferSizeBytes );
			V_UnicodeToUTF8( wText, szText, nUtf8BufferSizeBytes );

			GTFPartyClient()->SendPartyChat( szText );
		}
	}
	else if ( code == KEY_TAB )
	{
		// Ignore tab, otherwise vgui will screw up the focus.
		return;
	}
	else
	{
		vgui::TextEntry::OnKeyCodeTyped( code );
	}
}

void ChatTextEntry::OnKillFocus()
{
	BaseClass::OnKillFocus();

	if ( IsVisible() && GetAlpha() > 0 )
	{
		MoveToFront();
	}
}


CON_COMMAND( say_party, "Send a message to the user's party, if they have one" )
{
	if ( args.ArgC() < 2 )
		return;

	if ( !engine->IsInGame() )
		return;

	if ( !GTFPartyClient()->BHaveActiveParty() )
		return;

	char pszMsg[ 256 ];
	V_sprintf_safe( pszMsg, "%s", (char *)args.ArgS() );
	// Comes in with quotes.  Let's get rid of those.
	V_StripSurroundingQuotes( pszMsg );

	GTFPartyClient()->SendPartyChat( pszMsg );
}

CUtlVector< ChatMessage_t > CPartyChatPanel::sm_vecChatMessages;

CPartyChatPanel::CPartyChatPanel( Panel *parent, const char *panelName )
	: BaseClass( parent, panelName )
{
	SetProportional( true );

	m_pChatEntry = new ChatTextEntry( this, "chatentry" );
	m_pChatLog = new RichText( this, "chatlog" );
	m_pChatLog->SetUnusedScrollbarInvisible( true );
	m_pChatLog->SetDrawOffsets( QuickPropScale( 3 ), QuickPropScale( 1 ) );

	REGISTER_COLOR_AS_OVERRIDABLE( m_colorChatDefault, "chat_color_default" );
	REGISTER_COLOR_AS_OVERRIDABLE( m_colorChatPlayerName, "chat_color_player_name" );
	REGISTER_COLOR_AS_OVERRIDABLE( m_colorChatPlayerChatText, "chat_color_chat_text" );
	REGISTER_COLOR_AS_OVERRIDABLE( m_colorChatPartyEvent, "chat_color_party_event" );

	ListenForGameEvent( "party_chat" );

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
}

void CPartyChatPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/GlobalChat.res" );

	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		m_localSteamID = steamapicontext->SteamUser()->GetSteamID();
	}

	m_fonts[ SMALL ] = pScheme->GetFont( m_strChatLogFont[ SMALL ], true );
	m_fonts[ MEDIUM ] = pScheme->GetFont( m_strChatLogFont[ MEDIUM ], true );
	m_fonts[ LARGE ] = pScheme->GetFont( m_strChatLogFont[ LARGE ], true );
	m_pChatLog->SetFont( m_fonts[ m_eFontSize ] );
}

void CPartyChatPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_strChatLogFont[ SMALL ] = inResourceData->GetString( "log_font_small", NULL );
	m_strChatLogFont[ MEDIUM ] = inResourceData->GetString( "log_font_medium", NULL );
	m_strChatLogFont[ LARGE ] = inResourceData->GetString( "log_font_large", NULL );
}

void CPartyChatPanel::PerformLayout()
{
	BaseClass::PerformLayout();
}

void CPartyChatPanel::OnSizeChanged( int wide, int tall )
{
	BaseClass::OnSizeChanged( wide, tall );

	int nBGAlpha = RemapValClamped( GetPercentExpanded(), 0.f, 1.f, 150, 250 );
	Color bgColor( 0, 0, 0, nBGAlpha );
	m_pChatLog->SetBgColor( bgColor );

	m_pChatLog->SetTall( GetTall() - m_pChatLog->GetYPos() - YRES( 15 ) );
}

void CPartyChatPanel::OnShowChatEntry( KeyValues* pParams )
{
	bool bShow = pParams->GetBool( "visible" ) ;
	m_pChatEntry->SetMouseInputEnabled( bShow );
	m_pChatEntry->SetEditable( bShow );

	if ( bShow )
	{
		m_pChatEntry->RequestFocus();
		m_pChatEntry->MakePopup();
	}
}

void CPartyChatPanel::OnToggleCollapse( bool bIsExpanded )
{
	auto pAnim = g_pClientMode->GetViewportAnimationController();

	if ( bIsExpanded )
	{
		pAnim->RunAnimationCommand( m_pChatEntry, "alpha", 255, 0.1f, 0.2f, AnimationController::INTERPOLATOR_LINEAR, 0, true, false );
	
		PostMessage( this, new KeyValues( "ShowChatEntry", "visible", "1" ), 0.3f );
	}
	else
	{
		pAnim->RunAnimationCommand( m_pChatEntry, "alpha", 0, 0.0f, 0.2f, AnimationController::INTERPOLATOR_LINEAR, 0, true, false );
		PostMessage( this, new KeyValues( "ShowChatEntry", "visible", "0" ), 0.3f );
		surface()->ReleasePanel( m_pChatEntry->GetVPanel() );
		((VPanel*)m_pChatEntry->GetVPanel())->SetPopup( false );
	}
}

void CPartyChatPanel::FireGameEvent( IGameEvent *event )
{
	const char *pszEventName = event->GetName();

	if ( !Q_stricmp( pszEventName, "party_chat" ) )
	{
		// Add a new message entry, but make free up some history if we need to
		if ( sm_vecChatMessages.Count() > 100 )
		{
			delete[] sm_vecChatMessages.Head().m_pwszText;
			sm_vecChatMessages.Remove( 0 );
		}

		auto& msg = sm_vecChatMessages[ sm_vecChatMessages.AddToTail() ];

		msg.m_pwszText = nullptr;
		msg.m_steamID = SteamIDFromDecimalString( event->GetString( "steamid", "0" ) );
		msg.m_eType = (ETFPartyChatType)event->GetInt( "type", k_eTFPartyChatType_Invalid );

		// Get the color for this guy based on their steamID
		Color colorName = m_colorChatPlayerName;
		auto pParty = GTFPartyClient()->GetActiveParty();
		if ( pParty && msg.m_eType == k_eTFPartyChatType_MemberChat )
		{
			int nSlot = pParty->GetClientCentricMemberIndexBySteamID( msg.m_steamID );
			colorName = GetMMDashboard()->GetPartyMemberColor( nSlot );
		}

		const char *pszText = event->GetString( "text", "" );
		int l = V_strlen( pszText );
		int nBufSize = ( l *sizeof(wchar_t) ) + 4;
		msg.m_pwszText = (wchar_t *)new wchar_t[ nBufSize ];
		V_UTF8ToUnicode( pszText, msg.m_pwszText, nBufSize );

		// Play a sound if someone says something when we're not looking
		if ( !BIsExpanded() )
		{
			PlaySoundEntry( "Chat.DisplayText" );

			class CChatPopup: public CTFDashboardNotification
			{
				DECLARE_CLASS_SIMPLE( CChatPopup, CTFDashboardNotification );
			public:
				CChatPopup( float flLifeTime ) : CTFDashboardNotification( CTFDashboardNotification::TYPE_CHAT,
																		   CTFDashboardNotification::LEFT,
																		   flLifeTime,
																		   "ChatPopup" )
				{
					m_pRichText = new RichText( this, "text" );
					LoadControlSettings( "resource/UI/ChatPopup.res" );
				
					// Inset a bit
					m_pRichText->SetDrawOffsets( QuickPropScale( 4 ), 0 );
					MakeReadyForUse();

					// Initial anims
					auto pAnim = g_pClientMode->GetViewportAnimationController();
					Color currentBG = GetBgColor();
					// BG flash
					pAnim->RunAnimationCommand( this, "bgcolor", GetColor( "TanDark" ), 0.f, 0.f, AnimationController::INTERPOLATOR_LINEAR, 0, true, false );
					pAnim->RunAnimationCommand( this, "bgcolor", currentBG, 0.15f, 0.3f, AnimationController::INTERPOLATOR_LINEAR, 0,  false, false );
				}

				void SetToMinSize()
				{
					m_pRichText->SetToFullHeight(); // Be as tight as possible
					SetTall( m_pRichText->GetTall() );
				}

				RichText* m_pRichText;
			};

			CChatPopup* pChatPopup = new CChatPopup( tf_chat_popup_hold_time.GetFloat() );
			pChatPopup->AddActionSignalTarget( this ); // So we get "PopupDeleted"

			pChatPopup->m_pRichText->SetFont( m_fonts[ LARGE ] ); // Better to read you with
			RenderPartyChatMessage( sm_vecChatMessages.Tail(), pChatPopup->m_pRichText, m_colorChatPartyEvent, colorName, m_colorChatPlayerChatText );
			pChatPopup->SetToMinSize();
		}

		if ( sm_vecChatMessages.Count() > 0 )
		{
			m_pChatLog->InsertString("\n");
		}
		RenderPartyChatMessage( sm_vecChatMessages.Tail(), m_pChatLog, m_colorChatPartyEvent, colorName, m_colorChatPlayerChatText );

		return;
	}

	Assert( false );
}
