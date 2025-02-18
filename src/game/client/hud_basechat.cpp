//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud_basechat.h"

#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include "iclientmode.h"
#include "hud_macros.h"
#include "engine/IEngineSound.h"
#include "text_message.h"
#include <vgui/ILocalize.h>
#include "vguicenterprint.h"
#include "vgui/KeyCode.h"
#include <KeyValues.h>
#include "ienginevgui.h"
#include "c_playerresource.h"
#include "ihudlcd.h"
#include "vgui/IInput.h"
#include "vgui/ILocalize.h"
#include "multiplay_gamerules.h"
#include "voice_status.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CHAT_WIDTH_PERCENTAGE 0.6f

#ifndef _XBOX
ConVar hud_saytext_time( "hud_saytext_time", "12", 0 );
ConVar cl_showtextmsg( "cl_showtextmsg", "1", 0, "Enable/disable text messages printing on the screen." );
ConVar cl_chatfilters( "cl_chatfilters", "63", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Stores the chat filter settings " );
ConVar cl_chatfilter_version( "cl_chatfilter_version", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_HIDDEN, "Stores the chat filter version" );
ConVar cl_mute_all_comms("cl_mute_all_comms", "1", FCVAR_ARCHIVE, "If 1, then all communications from a player will be blocked when that player is muted, including chat messages.");

const int kChatFilterVersion = 1;

Color g_ColorBlue( 153, 204, 255, 255 );
Color g_ColorRed( 255, 63, 63, 255 );
Color g_ColorGreen( 153, 255, 153, 255 );
Color g_ColorDarkGreen( 64, 255, 64, 255 );
Color g_ColorYellow( 255, 178, 0, 255 );
Color g_ColorGrey( 204, 204, 204, 255 );


// removes all color markup characters, so Msg can deal with the string properly
// returns a pointer to str
char* RemoveColorMarkup( char *str )
{
	char *out = str;
	for ( char *in = str; *in != 0; ++in )
	{
		if ( *in > 0 && *in < COLOR_MAX )
		{
			if ( *in == COLOR_HEXCODE || *in == COLOR_HEXCODE_ALPHA )
			{
				// skip the next six or eight characters
				const int nSkip = ( *in == COLOR_HEXCODE ? 6 : 8 );
				for ( int i = 0; i < nSkip && *in != 0; i++ )
				{
					++in;
				}

				// if we reached the end of the string first, then back up
				if ( *in == 0 )
				{
					--in;
				}
			}

			continue;
		}
		*out = *in;
		++out;
	}
	*out = 0;

	return str;
}

// converts all '\r' characters to '\n', so that the engine can deal with the properly
// returns a pointer to str
char* ConvertCRtoNL( char *str )
{
	for ( char *ch = str; *ch != 0; ch++ )
		if ( *ch == '\r' )
			*ch = '\n';
	return str;
}

// converts all '\r' characters to '\n', so that the engine can deal with the properly
// returns a pointer to str
wchar_t* ConvertCRtoNL( wchar_t *str )
{
	for ( wchar_t *ch = str; *ch != 0; ch++ )
		if ( *ch == L'\r' )
			*ch = L'\n';
	return str;
}

void StripEndNewlineFromString( char *str )
{
	int s = strlen( str ) - 1;
	if ( s >= 0 )
	{
		if ( str[s] == '\n' || str[s] == '\r' )
			str[s] = 0;
	}
}

void StripEndNewlineFromString( wchar_t *str )
{
	int s = wcslen( str ) - 1;
	if ( s >= 0 )
	{
		if ( str[s] == L'\n' || str[s] == L'\r' )
			str[s] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reads a string from the current message and checks if it is translatable
//-----------------------------------------------------------------------------
wchar_t* ReadLocalizedString( bf_read &msg, OUT_Z_BYTECAP(outSizeInBytes) wchar_t *pOut, int outSizeInBytes, bool bStripNewline, OUT_Z_CAP(originalSize) char *originalString, int originalSize )
{
	char szString[2048];
	szString[0] = 0;
	msg.ReadString( szString, sizeof(szString) );

	if ( originalString )
	{
		Q_strncpy( originalString, szString, originalSize );
	}

	const wchar_t *pBuf = g_pVGuiLocalize->Find( szString );
	if ( pBuf )
	{
		V_wcsncpy( pOut, pBuf, outSizeInBytes );
	}
	else
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( szString, pOut, outSizeInBytes );
	}

	if ( bStripNewline )
		StripEndNewlineFromString( pOut );

	return pOut;
}

//-----------------------------------------------------------------------------
// Purpose: Reads a string from the current message, converts it to unicode, and strips out color codes
//-----------------------------------------------------------------------------
wchar_t* ReadChatTextString( bf_read &msg, OUT_Z_BYTECAP(outSizeInBytes) wchar_t *pOut, int outSizeInBytes )
{
	char szString[2048];
	szString[0] = 0;
	msg.ReadString( szString, sizeof(szString) );

	g_pVGuiLocalize->ConvertANSIToUnicode( szString, pOut, outSizeInBytes );

	StripEndNewlineFromString( pOut );

	// converts color control characters into control characters for the normal color
	for ( wchar_t *test = pOut; test && *test; ++test )
	{
		if ( *test && (*test < COLOR_MAX ) )
		{
			if ( *test == COLOR_HEXCODE || *test == COLOR_HEXCODE_ALPHA )
			{
				// mark the next seven or nine characters. one for the control character and six or eight for the code itself.
				const int nSkip = ( *test == COLOR_HEXCODE ? 7 : 9 );
				for ( int i = 0; i < nSkip && *test != 0; i++, test++ )
				{
					*test = COLOR_NORMAL;
				}

				// if we reached the end of the string first, then back up
				if ( *test == 0 )
				{
					--test;
				}
			}
			else
			{
				*test = COLOR_NORMAL;
			}
		}
	}

	return pOut;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *parent - 
//			*panelName - 
//-----------------------------------------------------------------------------
CBaseHudChatLine::CBaseHudChatLine( vgui::Panel *parent, const char *panelName ) : 
	vgui::RichText( parent, panelName )
{
	m_hFont = m_hFontMarlett = 0;
	m_flExpireTime = 0.0f;
	m_flStartTime = 0.0f;
	m_iNameLength	= 0;
	m_text = NULL;

	SetPaintBackgroundEnabled( true );
	
	SetVerticalScrollbar( false );
}

CBaseHudChatLine::~CBaseHudChatLine()
{
	if ( m_text )
	{
		delete[] m_text;
		m_text = NULL;
	}
}
	

void CBaseHudChatLine::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont( "Default" );

#ifdef HL1_CLIENT_DLL
	SetBgColor( Color( 0, 0, 0, 0 ) );
	SetFgColor( Color( 0, 0, 0, 0 ) );

	SetBorder( NULL );
#else
	SetBgColor( Color( 0, 0, 0, 100 ) );
#endif


	m_hFontMarlett = pScheme->GetFont( "Marlett" );

	m_clrText = pScheme->GetColor( "FgColor", GetFgColor() );
	SetFont( m_hFont );
}


void CBaseHudChatLine::PerformFadeout( void )
{
	// Flash + Extra bright when new
	float curtime = gpGlobals->curtime;

	int lr = m_clrText[0];
	int lg = m_clrText[1];
	int lb = m_clrText[2];
	
	if ( curtime >= m_flStartTime && curtime < m_flStartTime + CHATLINE_FLASH_TIME )
	{
		float frac1 = ( curtime - m_flStartTime ) / CHATLINE_FLASH_TIME;
		float frac = frac1;

		frac *= CHATLINE_NUM_FLASHES;
		frac *= 2 * M_PI;

		frac = cos( frac );

		frac = clamp( frac, 0.0f, 1.0f );

		frac *= (1.0f-frac1);

		int r = lr, g = lg, b = lb;

		r = r + ( 255 - r ) * frac;
		g = g + ( 255 - g ) * frac;
		b = b + ( 255 - b ) * frac;
	
		// Draw a right facing triangle in red, faded out over time
		int alpha = 63 + 192 * (1.0f - frac1 );
		alpha = clamp( alpha, 0, 255 );

		wchar_t wbuf[4096];
		GetText(0, wbuf, sizeof(wbuf));

		SetText( "" );

		InsertColorChange( Color( r, g, b, 255 ) );
		InsertString( wbuf );
	}
	else if ( curtime <= m_flExpireTime && curtime > m_flExpireTime - CHATLINE_FADE_TIME )
	{
		float frac = ( m_flExpireTime - curtime ) / CHATLINE_FADE_TIME;

		int alpha = frac * 255;
		alpha = clamp( alpha, 0, 255 );

		wchar_t wbuf[4096];
		GetText(0, wbuf, sizeof(wbuf));

		SetText( "" );

		InsertColorChange( Color( lr * frac, lg * frac, lb * frac, alpha ) );
		InsertString( wbuf );
	}
	else
	{
		wchar_t wbuf[4096];
		GetText(0, wbuf, sizeof(wbuf));

		SetText( "" );

		InsertColorChange( Color( lr, lg, lb, 255 ) );
		InsertString( wbuf );
	}

	OnThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//-----------------------------------------------------------------------------
void CBaseHudChatLine::SetExpireTime( void )
{
	m_flStartTime = gpGlobals->curtime;
	m_flExpireTime = m_flStartTime + hud_saytext_time.GetFloat();
	m_nCount = CBaseHudChat::m_nLineCounter++;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseHudChatLine::GetCount( void )
{
	return m_nCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseHudChatLine::IsReadyToExpire( void )
{
	// Engine disconnected, expire right away
	if ( !engine->IsInGame() && !engine->IsConnected() )
		return true;

	if ( gpGlobals->curtime >= m_flExpireTime )
		return true;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CBaseHudChatLine::GetStartTime( void )
{
	return m_flStartTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudChatLine::Expire( void )
{
	SetVisible( false );

	// Spit out label text now
//	char text[ 256 ];
//	GetText( text, 256 );

//	Msg( "%s\n", text );
}
#endif // _XBOX

//-----------------------------------------------------------------------------
// Purpose: The prompt and text entry area for chat messages
//-----------------------------------------------------------------------------
#ifndef _XBOX
CBaseHudChatInputLine::CBaseHudChatInputLine( vgui::Panel *parent, char const *panelName ) : 
	vgui::Panel( parent, panelName )
{
	SetMouseInputEnabled( false );

	m_pPrompt = new vgui::Label( this, "ChatInputPrompt", L"Enter text:" );

	m_pInput = new CBaseHudChatEntry( this, "ChatInput", parent );	
	m_pInput->SetMaximumCharCount( 127 );
}

void CBaseHudChatInputLine::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	
	// FIXME:  Outline
	vgui::HFont hFont = pScheme->GetFont( "ChatFont" );

	m_pPrompt->SetFont( hFont );
	m_pInput->SetFont( hFont );

	m_pInput->SetFgColor( pScheme->GetColor( "Chat.TypingText", pScheme->GetColor( "Panel.FgColor", Color( 255, 255, 255, 255 ) ) ) );

	SetPaintBackgroundEnabled( true );
	m_pPrompt->SetPaintBackgroundEnabled( true );
	m_pPrompt->SetContentAlignment( vgui::Label::a_west );
	m_pPrompt->SetTextInset( 2, 0 );

	m_pInput->SetMouseInputEnabled( true );

#ifdef HL1_CLIENT_DLL
	m_pInput->SetBgColor( Color( 255, 255, 255, 0 ) );
#endif

	SetBgColor( Color( 0, 0, 0, 0) );

}

void CBaseHudChatInputLine::SetPrompt( const wchar_t *prompt )
{
	Assert( m_pPrompt );
	m_pPrompt->SetText( prompt );
	InvalidateLayout();
}

void CBaseHudChatInputLine::ClearEntry( void )
{
	Assert( m_pInput );
	SetEntry( L"" );
}

void CBaseHudChatInputLine::SetEntry( const wchar_t *entry )
{
	Assert( m_pInput );
	Assert( entry );

	m_pInput->SetText( entry );
}

void CBaseHudChatInputLine::GetMessageText( OUT_Z_BYTECAP(buffersizebytes) wchar_t *buffer, int buffersizebytes )
{
	m_pInput->GetText( buffer, buffersizebytes);
}

void CBaseHudChatInputLine::PerformLayout()
{
	BaseClass::PerformLayout();

	int wide, tall;
	GetSize( wide, tall );

	int w,h;
	m_pPrompt->GetContentSize( w, h); 
	m_pPrompt->SetBounds( 0, 0, w, tall );

	m_pInput->SetBounds( w + 2, 0, wide - w - 2 , tall );
}

vgui::Panel *CBaseHudChatInputLine::GetInputPanel( void )
{
	return m_pInput;
}
#endif //_XBOX


CHudChatFilterButton::CHudChatFilterButton( vgui::Panel *pParent, const char *pName, const char *pText ) : 
BaseClass( pParent, pName, pText )
{
}

CHudChatFilterCheckButton::CHudChatFilterCheckButton( vgui::Panel *pParent, const char *pName, const char *pText, int iFlag ) : 
BaseClass( pParent, pName, pText )
{
	m_iFlag = iFlag;
}


CHudChatFilterPanel::CHudChatFilterPanel( vgui::Panel *pParent, const char *pName ) : BaseClass ( pParent, pName )
{
	pParent->SetSize( 10, 10 ); // Quiet "parent not sized yet" spew
	SetParent( pParent );

	new CHudChatFilterCheckButton( this, "joinleave_button", "Sky is blue?", CHAT_FILTER_JOINLEAVE );
	new CHudChatFilterCheckButton( this, "namechange_button", "Sky is blue?", CHAT_FILTER_NAMECHANGE );
	new CHudChatFilterCheckButton( this, "publicchat_button", "Sky is blue?", CHAT_FILTER_PUBLICCHAT );
	new CHudChatFilterCheckButton( this, "servermsg_button", "Sky is blue?", CHAT_FILTER_SERVERMSG );
	new CHudChatFilterCheckButton( this, "teamchange_button", "Sky is blue?", CHAT_FILTER_TEAMCHANGE );
    //=============================================================================
    // HPE_BEGIN:
    // [tj]Added a new filter checkbox for achievement announces.
    //     Also. Yes. Sky is blue.
    //=============================================================================
     
    new CHudChatFilterCheckButton( this, "achivement_button", "Sky is blue?", CHAT_FILTER_ACHIEVEMENT);
     
    //=============================================================================
    // HPE_END
    //=============================================================================
    
}

void CHudChatFilterPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	LoadControlSettings( "resource/UI/ChatFilters.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	Color cColor = pScheme->GetColor( "DullWhite", GetBgColor() );
	SetBgColor( Color ( cColor.r(), cColor.g(), cColor.b(), CHAT_HISTORY_ALPHA ) );

	SetFgColor( pScheme->GetColor( "Blank", GetFgColor() ) );
}

void CHudChatFilterPanel::OnFilterButtonChecked( vgui::Panel *panel )
{
	CHudChatFilterCheckButton *pButton = dynamic_cast < CHudChatFilterCheckButton * > ( panel );

	if ( pButton && GetChatParent() && IsVisible() )
	{
		if ( pButton->IsSelected() )
		{
			GetChatParent()->SetFilterFlag( GetChatParent()->GetFilterFlags() | pButton->GetFilterFlag() );
		}
		else
		{
			GetChatParent()->SetFilterFlag( GetChatParent()->GetFilterFlags() & ~ pButton->GetFilterFlag() );
		}
	}
}

void CHudChatFilterPanel::SetVisible(bool state)
{
	if ( state == true )
	{
		for (int i = 0; i < GetChildCount(); i++)
		{
			CHudChatFilterCheckButton *pButton = dynamic_cast < CHudChatFilterCheckButton * > ( GetChild(i) );

			if ( pButton )
			{
				if ( cl_chatfilters.GetInt() & pButton->GetFilterFlag() )
				{
					pButton->SetSelected( true );
				}
				else
				{
					pButton->SetSelected( false );
				}
			}
		}
	}

	BaseClass::SetVisible( state );
}

void CHudChatFilterButton::DoClick( void )
{
	BaseClass::DoClick();

	CBaseHudChat *pChat = dynamic_cast < CBaseHudChat * > (GetParent() );

	if ( pChat )
	{
		pChat->GetChatInput()->RequestFocus();

		if ( pChat->GetChatFilterPanel() )
		{
			if ( pChat->GetChatFilterPanel()->IsVisible() )
			{
				pChat->GetChatFilterPanel()->SetVisible( false );
			}
			else
			{
				pChat->GetChatFilterPanel()->SetVisible( true );
				pChat->GetChatFilterPanel()->MakePopup();
				pChat->GetChatFilterPanel()->SetMouseInputEnabled( true );
			}
		}
	}
}

CHudChatHistory::CHudChatHistory( vgui::Panel *pParent, const char *panelName ) : BaseClass( pParent, "HudChatHistory" )
{
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ChatScheme.res", "ChatScheme");
	SetScheme(scheme);

	InsertFade( -1, -1 );
}

void CHudChatHistory::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetFont( pScheme->GetFont( "ChatFont" ) );
	SetAlpha( 255 );
}

int CBaseHudChat::m_nLineCounter = 1;
//-----------------------------------------------------------------------------
// Purpose: Text chat input/output hud element
//-----------------------------------------------------------------------------
CBaseHudChat::CBaseHudChat( const char *pElementName )
: CHudElement( pElementName ), BaseClass( NULL, "HudChat" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ChatScheme.res", "ChatScheme" );
	SetScheme(scheme);

	g_pVGuiLocalize->AddFile( "resource/chat_%language%.txt" );

	m_nMessageMode = 0;

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	// (We don't actually want input until they bring up the chat line).
	MakePopup();
	SetZPos( -30 );

	SetHiddenBits( HIDEHUD_CHAT );

	m_pFiltersButton = new CHudChatFilterButton( this, "ChatFiltersButton", "Filters" );

	if ( m_pFiltersButton )
	{
		m_pFiltersButton->SetScheme( scheme );
		m_pFiltersButton->SetVisible( true );
		m_pFiltersButton->SetEnabled( true );
		m_pFiltersButton->SetMouseInputEnabled( true );
		m_pFiltersButton->SetKeyBoardInputEnabled( false );
	}

	m_pChatHistory = new CHudChatHistory( this, "HudChatHistory" );

	CreateChatLines();
	CreateChatInputLine();
	GetChatFilterPanel();

	m_iFilterFlags = cl_chatfilters.GetInt();
}

void CBaseHudChat::CreateChatInputLine( void )
{
#ifndef _XBOX
	m_pChatInput = new CBaseHudChatInputLine( this, "ChatInputLine" );
	m_pChatInput->SetVisible( false );

	if ( GetChatHistory() )
	{
		GetChatHistory()->SetMaximumCharCount( 127 * 100 );
		GetChatHistory()->SetVisible( true );
	}
#endif
}

void CBaseHudChat::CreateChatLines( void )
{
#ifndef _XBOX
	m_ChatLine = new CBaseHudChatLine( this, "ChatLine1" );
	m_ChatLine->SetVisible( false );		

#endif
}


#define BACKGROUND_BORDER_WIDTH 20

CHudChatFilterPanel *CBaseHudChat::GetChatFilterPanel( void )
{
	if ( m_pFilterPanel == NULL )
	{
		m_pFilterPanel = new CHudChatFilterPanel( this, "HudChatFilterPanel"  );

		if ( m_pFilterPanel )
		{
			vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ChatScheme.res", "ChatScheme");

			m_pFilterPanel->SetScheme( scheme );
			m_pFilterPanel->InvalidateLayout( true, true );
			m_pFilterPanel->SetMouseInputEnabled( true );
			m_pFilterPanel->SetPaintBackgroundType( 2 );
			m_pFilterPanel->SetPaintBorderEnabled( true );
			m_pFilterPanel->SetVisible( false );
		}
	}

	return m_pFilterPanel;
}

void CBaseHudChat::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	LoadControlSettings( "resource/UI/BaseChat.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	SetPaintBackgroundType( 2 );
	SetPaintBorderEnabled( true );
	SetPaintBackgroundEnabled( true );

	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );
	m_nVisibleHeight = 0;

#ifdef HL1_CLIENT_DLL
	SetBgColor( Color( 0, 0, 0, 0 ) );
	SetFgColor( Color( 0, 0, 0, 0 ) );
#endif

	Color cColor = pScheme->GetColor( "DullWhite", GetBgColor() );
	SetBgColor( Color ( cColor.r(), cColor.g(), cColor.b(), CHAT_HISTORY_ALPHA ) );

	GetChatHistory()->SetVerticalScrollbar( false );
}

void CBaseHudChat::Reset( void )
{
#ifndef HL1_CLIENT_DLL
	m_nVisibleHeight = 0;
	Clear();
#endif
}

#ifdef _XBOX
bool CBaseHudChat::ShouldDraw()
{
	// never think, never draw
	return false;
}
#endif

void CBaseHudChat::Paint( void )
{
#ifndef _XBOX
	if ( m_nVisibleHeight == 0 )
		return;
#endif
}

CHudChatHistory *CBaseHudChat::GetChatHistory( void )
{
	return m_pChatHistory;
}



void CBaseHudChat::Init( void )
{
	if ( IsXbox() )
		return;

	ListenForGameEvent( "hltv_chat" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszName - 
//			iSize - 
//			*pbuf - 
//-----------------------------------------------------------------------------
void CBaseHudChat::MsgFunc_SayText( bf_read &msg )
{
	char szString[256];

	int client = msg.ReadByte();
	msg.ReadString( szString, sizeof(szString) );
	bool bWantsToChat = msg.ReadByte();

	if ( bWantsToChat )
	{
		// print raw chat text
		ChatPrintf( client, CHAT_FILTER_NONE, "%s", szString );
	}
	else
	{
		// try to lookup translated string
		Printf( CHAT_FILTER_NONE, "%s", hudtextmessage->LookupString( szString ) );
	}

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "HudChat.Message" );

	Msg( "%s", szString );
}

int CBaseHudChat::GetFilterForString( const char *pString )
{
	if ( !Q_stricmp( pString, "#HL_Name_Change" ) ) 
	{
		return CHAT_FILTER_NAMECHANGE;
	}

	return CHAT_FILTER_NONE;
}


//-----------------------------------------------------------------------------
// Purpose: Reads in a player's Chat text from the server
//-----------------------------------------------------------------------------
void CBaseHudChat::MsgFunc_SayText2( bf_read &msg )
{
	// Got message during connection
	if ( !g_PR )
		return;

	int client = msg.ReadByte();
	bool bWantsToChat = msg.ReadByte();

	wchar_t szBuf[6][256];
	char untranslated_msg_text[256];
	wchar_t *msg_text = ReadLocalizedString( msg, szBuf[0], sizeof( szBuf[0] ), false, untranslated_msg_text, sizeof( untranslated_msg_text ) );

	// keep reading strings and using C format strings for subsituting the strings into the localised text string
	ReadChatTextString ( msg, szBuf[1], sizeof( szBuf[1] ) );		// player name
	ReadChatTextString ( msg, szBuf[2], sizeof( szBuf[2] ) );		// chat text
	ReadLocalizedString( msg, szBuf[3], sizeof( szBuf[3] ), true );
	ReadLocalizedString( msg, szBuf[4], sizeof( szBuf[4] ), true );

	g_pVGuiLocalize->ConstructString( szBuf[5], sizeof( szBuf[5] ), msg_text, 4, szBuf[1], szBuf[2], szBuf[3], szBuf[4] );

	char ansiString[512];
	g_pVGuiLocalize->ConvertUnicodeToANSI( ConvertCRtoNL( szBuf[5] ), ansiString, sizeof( ansiString ) );

	if ( bWantsToChat )
	{
		int iFilter = CHAT_FILTER_NONE;

		if ( client > 0 && (g_PR->GetTeam( client ) != g_PR->GetTeam( GetLocalPlayerIndex() )) )
		{
			iFilter = CHAT_FILTER_PUBLICCHAT;
		}

		// print raw chat text
		ChatPrintf( client, iFilter, "%s", ansiString );

		Msg( "%s\n", RemoveColorMarkup(ansiString) );

		CLocalPlayerFilter filter;
		C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "HudChat.Message" );
	}
	else
	{
		// print raw chat text
		ChatPrintf( client, GetFilterForString( untranslated_msg_text), "%s", ansiString );
	}
}

//-----------------------------------------------------------------------------
// Message handler for text messages
// displays a string, looking them up from the titles.txt file, which can be localised
// parameters:
//   byte:   message direction  ( HUD_PRINTCONSOLE, HUD_PRINTNOTIFY, HUD_PRINTCENTER, HUD_PRINTTALK )
//   string: message
// optional parameters:
//   string: message parameter 1
//   string: message parameter 2
//   string: message parameter 3
//   string: message parameter 4
// any string that starts with the character '#' is a message name, and is used to look up the real message in titles.txt
// the next (optional) one to four strings are parameters for that string (which can also be message names if they begin with '#')
//-----------------------------------------------------------------------------
void CBaseHudChat::MsgFunc_TextMsg( bf_read &msg )
{
	char szString[2048];
	int msg_dest = msg.ReadByte();

	wchar_t szBuf[5][256];
	wchar_t outputBuf[256];

	for ( int i=0; i<5; ++i )
	{
		msg.ReadString( szString, sizeof(szString) );
		char *tmpStr = hudtextmessage->LookupString( szString, &msg_dest );
		const wchar_t *pBuf = g_pVGuiLocalize->Find( tmpStr );
		if ( pBuf )
		{
			// Copy pBuf into szBuf[i].
			int nMaxChars = sizeof( szBuf[i] ) / sizeof( wchar_t );
			wcsncpy( szBuf[i], pBuf, nMaxChars );
			szBuf[i][nMaxChars-1] = 0;
		}
		else
		{
			if ( i )
			{
				StripEndNewlineFromString( tmpStr );  // these strings are meant for subsitution into the main strings, so cull the automatic end newlines
			}
			g_pVGuiLocalize->ConvertANSIToUnicode( tmpStr, szBuf[i], sizeof(szBuf[i]) );
		}
	}

	if ( !cl_showtextmsg.GetInt() )
		return;

	int len;
	switch ( msg_dest )
	{
	case HUD_PRINTCENTER:
		g_pVGuiLocalize->ConstructString( outputBuf, sizeof(outputBuf), szBuf[0], 4, szBuf[1], szBuf[2], szBuf[3], szBuf[4] );
		internalCenterPrint->Print( ConvertCRtoNL( outputBuf ) );
		break;

	case HUD_PRINTNOTIFY:
		g_pVGuiLocalize->ConstructString( outputBuf, sizeof(outputBuf), szBuf[0], 4, szBuf[1], szBuf[2], szBuf[3], szBuf[4] );
		g_pVGuiLocalize->ConvertUnicodeToANSI( outputBuf, szString, sizeof(szString) );
		len = strlen( szString );
		if ( len && szString[len-1] != '\n' && szString[len-1] != '\r' )
		{
			Q_strncat( szString, "\n", sizeof(szString), 1 );
		}
		Msg( "%s", ConvertCRtoNL( szString ) );
		break;

	case HUD_PRINTTALK:
		g_pVGuiLocalize->ConstructString( outputBuf, sizeof(outputBuf), szBuf[0], 4, szBuf[1], szBuf[2], szBuf[3], szBuf[4] );
		g_pVGuiLocalize->ConvertUnicodeToANSI( outputBuf, szString, sizeof(szString) );
		len = strlen( szString );
		if ( len && szString[len-1] != '\n' && szString[len-1] != '\r' )
		{
			Q_strncat( szString, "\n", sizeof(szString), 1 );
		}
		Printf( CHAT_FILTER_NONE, "%s", ConvertCRtoNL( szString ) );
		Msg( "%s", ConvertCRtoNL( szString ) );
		break;

	case HUD_PRINTCONSOLE:
		g_pVGuiLocalize->ConstructString( outputBuf, sizeof(outputBuf), szBuf[0], 4, szBuf[1], szBuf[2], szBuf[3], szBuf[4] );
		g_pVGuiLocalize->ConvertUnicodeToANSI( outputBuf, szString, sizeof(szString) );
		len = strlen( szString );
		if ( len && szString[len-1] != '\n' && szString[len-1] != '\r' )
		{
			Q_strncat( szString, "\n", sizeof(szString), 1 );
		}
		Msg( "%s", ConvertCRtoNL( szString ) );
		break;
	}
}

void CBaseHudChat::MsgFunc_VoiceSubtitle( bf_read &msg )
{
	// Got message during connection
	if ( !g_PR )
		return;

	if ( !cl_showtextmsg.GetInt() )
		return;

	char szString[2048];
	char szPrefix[64];	//(Voice)
	wchar_t szBuf[128];

	int client = msg.ReadByte();
	int iMenu = msg.ReadByte();
	int iItem = msg.ReadByte();

	const char *pszSubtitle = "";

	CGameRules *pGameRules = GameRules();

	CMultiplayRules *pMultiRules = dynamic_cast< CMultiplayRules * >( pGameRules );

	Assert( pMultiRules );

	if ( pMultiRules )
	{
		pszSubtitle = pMultiRules->GetVoiceCommandSubtitle( iMenu, iItem );
	}

	SetVoiceSubtitleState( true );

	const wchar_t *pBuf = g_pVGuiLocalize->Find( pszSubtitle );
	if ( pBuf )
	{
		// Copy pBuf into szBuf[i].
		int nMaxChars = sizeof( szBuf ) / sizeof( wchar_t );
		wcsncpy( szBuf, pBuf, nMaxChars );
		szBuf[nMaxChars-1] = 0;
	}
	else
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( pszSubtitle, szBuf, sizeof(szBuf) );
	}

	int len;
	g_pVGuiLocalize->ConvertUnicodeToANSI( szBuf, szString, sizeof(szString) );
	len = strlen( szString );
	if ( len && szString[len-1] != '\n' && szString[len-1] != '\r' )
	{
		Q_strncat( szString, "\n", sizeof(szString), 1 );
	}

	const wchar_t *pVoicePrefix = g_pVGuiLocalize->Find( "#Voice" );
	g_pVGuiLocalize->ConvertUnicodeToANSI( pVoicePrefix, szPrefix, sizeof(szPrefix) );
	
	ChatPrintf( client, CHAT_FILTER_NONE, "%c(%s) %s%c: %s", COLOR_PLAYERNAME, szPrefix, GetDisplayedSubtitlePlayerName( client ), COLOR_NORMAL, ConvertCRtoNL( szString ) );

	SetVoiceSubtitleState( false );
}

const char *CBaseHudChat::GetDisplayedSubtitlePlayerName( int clientIndex )
{
	return g_PR->GetPlayerName( clientIndex );
}

#ifndef _XBOX
static int __cdecl SortLines( void const *line1, void const *line2 )
{
	CBaseHudChatLine *l1 = *( CBaseHudChatLine ** )line1;
	CBaseHudChatLine *l2 = *( CBaseHudChatLine ** )line2;

	// Invisible at bottom
	if ( l1->IsVisible() && !l2->IsVisible() )
		return -1;
	else if ( !l1->IsVisible() && l2->IsVisible() )
		return 1;

	// Oldest start time at top
	if ( l1->GetStartTime() < l2->GetStartTime() )
		return -1;
	else if ( l1->GetStartTime() > l2->GetStartTime() )
		return 1;

	// Otherwise, compare counter
	if ( l1->GetCount() < l2->GetCount() )
		return -1;
	else if ( l1->GetCount() > l2->GetCount() )
		return 1;

	return 0;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Allow inheriting classes to change this spacing behavior
//-----------------------------------------------------------------------------
int CBaseHudChat::GetChatInputOffset( void )
{
	return m_iFontHeight;
}

//-----------------------------------------------------------------------------
// Purpose: Do respositioning here to avoid latency due to repositioning of vgui
//  voice manager icon panel
//-----------------------------------------------------------------------------
void CBaseHudChat::OnTick( void )
{
#ifndef _XBOX
	m_nVisibleHeight = 0;

	CBaseHudChatLine *line = m_ChatLine;

	if ( line )
	{
		vgui::HFont font = line->GetFont();
		m_iFontHeight = vgui::surface()->GetFontTall( font ) + 2;

		// Put input area at bottom

		int iChatX, iChatY, iChatW, iChatH;
		int iInputX, iInputY, iInputW, iInputH;
		
		m_pChatInput->GetBounds( iInputX, iInputY, iInputW, iInputH );
		GetBounds( iChatX, iChatY, iChatW, iChatH );

		m_pChatInput->SetBounds( iInputX, iChatH - (m_iFontHeight * 1.75), iInputW, m_iFontHeight );

		//Resize the History Panel so it fits more lines depending on the screen resolution.
		int iChatHistoryX, iChatHistoryY, iChatHistoryW, iChatHistoryH;

		GetChatHistory()->GetBounds( iChatHistoryX, iChatHistoryY, iChatHistoryW, iChatHistoryH );

		iChatHistoryH = (iChatH - (m_iFontHeight * 2.25)) - iChatHistoryY;

		GetChatHistory()->SetBounds( iChatHistoryX, iChatHistoryY, iChatHistoryW, iChatHistoryH );
	}

	FadeChatHistory();

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : width - 
//			*text - 
//			textlen - 
// Output : int
//-----------------------------------------------------------------------------
int CBaseHudChat::ComputeBreakChar( int width, const char *text, int textlen )
{
#ifndef _XBOX
	CBaseHudChatLine *line = m_ChatLine;
	vgui::HFont font = line->GetFont();

	int currentlen = 0;
	int lastbreak = textlen;
	for (int i = 0; i < textlen ; i++)
	{
		char ch = text[i];

		if ( ch <= 32 )
		{
			lastbreak = i;
		}

		wchar_t wch[2];

		g_pVGuiLocalize->ConvertANSIToUnicode( &ch, wch, sizeof( wch ) );

		int a,b,c;

		vgui::surface()->GetCharABCwide(font, wch[0], a, b, c);
		currentlen += a + b + c;

		if ( currentlen >= width )
		{
			// If we haven't found a whitespace char to break on before getting
			//  to the end, but it's still too long, break on the character just before
			//  this one
			if ( lastbreak == textlen )
			{
				lastbreak = MAX( 0, i - 1 );
			}
			break;
		}
	}

	if ( currentlen >= width )
	{
		return lastbreak;
	}
	return textlen;
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fmt - 
//			... - 
//-----------------------------------------------------------------------------
void CBaseHudChat::Printf( int iFilter, const char *fmt, ... )
{
	va_list marker;
	char msg[4096];

	va_start(marker, fmt);
	Q_vsnprintf(msg, sizeof( msg), fmt, marker);
	va_end(marker);

	ChatPrintf( 0, iFilter, "%s", msg );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudChat::StartMessageMode( int iMessageModeType )
{
#ifndef _XBOX
	m_nMessageMode = iMessageModeType;

	m_pChatInput->ClearEntry();

	const wchar_t *pszPrompt = ( m_nMessageMode == MM_SAY ) ? g_pVGuiLocalize->Find( "#chat_say" ) : g_pVGuiLocalize->Find( "#chat_say_team" ); 
	if ( pszPrompt )
	{
		m_pChatInput->SetPrompt( pszPrompt );
	}
	else
	{
		if ( m_nMessageMode == MM_SAY )
		{
			m_pChatInput->SetPrompt( L"Say :" );
		}
		else
		{
			m_pChatInput->SetPrompt( L"Say (TEAM) :" );
		}
	}
	
	if ( GetChatHistory() )
	{
		GetChatHistory()->SetMouseInputEnabled( true );
		GetChatHistory()->SetKeyBoardInputEnabled( false );
		GetChatHistory()->SetVerticalScrollbar( true );
		GetChatHistory()->ResetAllFades( true );
		GetChatHistory()->SetPaintBorderEnabled( true );
		GetChatHistory()->SetVisible( true );
	}

	vgui::SETUP_PANEL( this );
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );
	m_pChatInput->SetVisible( true );
	vgui::surface()->CalculateMouseVisible();
	m_pChatInput->RequestFocus();
	m_pChatInput->SetPaintBorderEnabled( true );
	m_pChatInput->SetMouseInputEnabled( true );

	//Place the mouse cursor near the text so people notice it.
	int x, y, w, h;
	GetChatHistory()->GetBounds( x, y, w, h );
	vgui::input()->SetCursorPos( x + ( w/2), y + (h/2) );

	m_flHistoryFadeTime = gpGlobals->curtime + CHAT_HISTORY_FADE_TIME;

	m_pFilterPanel->SetVisible( false );

	engine->ClientCmd_Unrestricted( "gameui_preventescapetoshow\n" );
		
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudChat::StopMessageMode( void )
{
#ifndef _XBOX

	engine->ClientCmd_Unrestricted( "gameui_allowescapetoshow\n" );

	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );

	if ( GetChatHistory() )
	{
		GetChatHistory()->SetPaintBorderEnabled( false );
		GetChatHistory()->GotoTextEnd();
		GetChatHistory()->SetMouseInputEnabled( false );
		GetChatHistory()->SetVerticalScrollbar( false );
		GetChatHistory()->ResetAllFades( false, true, CHAT_HISTORY_FADE_TIME );
		GetChatHistory()->SelectNoText();
	}

	//Clear the entry since we wont need it anymore.
	m_pChatInput->ClearEntry();

	//hide filter panel
	m_pFilterPanel->SetVisible( false );

	m_flHistoryFadeTime = gpGlobals->curtime + CHAT_HISTORY_FADE_TIME;

	m_nMessageMode = MM_NONE;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudChat::OnChatEntrySend( void )
{
	Send();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudChat::OnChatEntryStopMessageMode( void )
{
	StopMessageMode();
}

void CBaseHudChat::FadeChatHistory( void )
{
	float frac = ( m_flHistoryFadeTime -  gpGlobals->curtime ) / CHAT_HISTORY_FADE_TIME;

	int alpha = frac * CHAT_HISTORY_ALPHA;
	alpha = clamp( alpha, 0, CHAT_HISTORY_ALPHA );

	if ( alpha >= 0 )
	{
		if ( GetChatHistory() )
		{
			if ( IsMouseInputEnabled() )
			{
				SetAlpha( 255 );
				GetChatHistory()->SetBgColor( Color( 0, 0, 0, CHAT_HISTORY_ALPHA - alpha ) );
				m_pChatInput->GetPrompt()->SetAlpha( (CHAT_HISTORY_ALPHA*2) - alpha );
				m_pChatInput->GetInputPanel()->SetAlpha( (CHAT_HISTORY_ALPHA*2) - alpha );
				SetBgColor( Color( GetBgColor().r(), GetBgColor().g(), GetBgColor().b(), CHAT_HISTORY_ALPHA - alpha ) );
				m_pFiltersButton->SetAlpha( (CHAT_HISTORY_ALPHA*2) - alpha );
			}
			else
			{
				GetChatHistory()->SetBgColor( Color( 0, 0, 0, alpha ) );
				SetBgColor( Color( GetBgColor().r(), GetBgColor().g(), GetBgColor().b(), alpha ) );
			
				m_pChatInput->GetPrompt()->SetAlpha( alpha );
				m_pChatInput->GetInputPanel()->SetAlpha( alpha );
				m_pFiltersButton->SetAlpha( alpha );
			}
		}
	}
}

void CBaseHudChat::SetFilterFlag( int iFilter )
{
	m_iFilterFlags = iFilter;

	cl_chatfilters.SetValue( m_iFilterFlags );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color CBaseHudChat::GetTextColorForClient( TextColor colorNum, int clientIndex )
{
	Color c;
	switch ( colorNum )
	{
	case COLOR_CUSTOM:
		c = m_ColorCustom;
		break;

	case COLOR_PLAYERNAME:
		c = GetClientColor( clientIndex );
		break;

	case COLOR_LOCATION:
		c = g_ColorDarkGreen;
		break;

	case COLOR_ACHIEVEMENT:
		{
			vgui::IScheme *pSourceScheme = vgui::scheme()->GetIScheme( vgui::scheme()->GetScheme( "SourceScheme" ) ); 
			if ( pSourceScheme )
			{
				c = pSourceScheme->GetColor( "SteamLightGreen", GetBgColor() );
			}
			else
			{
				c = GetDefaultTextColor();
			}
		}
		break;

	default:
		c = GetDefaultTextColor();
	}

	return Color( c[0], c[1], c[2], 255 );
}

//-----------------------------------------------------------------------------
void CBaseHudChat::SetCustomColor( const char *pszColorName )
{
	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( vgui::scheme()->GetScheme( "ClientScheme" ) );
	SetCustomColor( pScheme->GetColor( pszColorName, Color(255,255,255,255) ) );
}

//-----------------------------------------------------------------------------
Color CBaseHudChat::GetDefaultTextColor( void )
{
	return g_ColorYellow;
}

//-----------------------------------------------------------------------------
Color CBaseHudChat::GetClientColor( int clientIndex )
{
	if ( clientIndex == 0 ) // console msg
	{
		return g_ColorGreen;
	}
	else if( g_PR )
	{
		return g_ColorGrey;
	}

	return g_ColorYellow;
}

//-----------------------------------------------------------------------------
// Purpose: Parses a line of text for color markup and inserts it via Colorize()
//-----------------------------------------------------------------------------
void CBaseHudChatLine::InsertAndColorizeText( wchar_t *buf, int clientIndex )
{
	if ( m_text )
	{
		delete[] m_text;
		m_text = NULL;
	}
	m_textRanges.RemoveAll();

	m_text = CloneWString( buf );

	CBaseHudChat *pChat = dynamic_cast<CBaseHudChat*>(GetParent() );

	if ( pChat == NULL )
		return;

	wchar_t *txt = m_text;
	int lineLen = wcslen( m_text );
	Color colCustom;
	if ( m_text[0] == COLOR_PLAYERNAME || m_text[0] == COLOR_LOCATION || m_text[0] == COLOR_NORMAL || m_text[0] == COLOR_ACHIEVEMENT || m_text[0] == COLOR_CUSTOM || m_text[0] == COLOR_HEXCODE || m_text[0] == COLOR_HEXCODE_ALPHA )
	{
		while ( txt && *txt )
		{
			TextRange range;
			bool bFoundColorCode = false;
			bool bDone = false;
			int nBytesIn = txt - m_text;

			switch ( *txt )
			{
			case COLOR_CUSTOM:
			case COLOR_PLAYERNAME:
			case COLOR_LOCATION:
			case COLOR_ACHIEVEMENT:
			case COLOR_NORMAL:
				{
					// save this start
					range.start = nBytesIn + 1;
					range.color = pChat->GetTextColorForClient( (TextColor)(*txt), clientIndex );
					range.end = lineLen;
					bFoundColorCode = true;
				}
				++txt;
				break;
			case COLOR_HEXCODE:
			case COLOR_HEXCODE_ALPHA:
				{
					bool bReadAlpha = ( *txt == COLOR_HEXCODE_ALPHA );
					const int nCodeBytes = ( bReadAlpha ? 8 : 6 );
					range.start = nBytesIn + nCodeBytes + 1;
					range.end = lineLen;
					range.preserveAlpha = bReadAlpha;
					++txt;

					if ( range.end > range.start )
					{
						int r = V_nibble( txt[0] ) << 4 | V_nibble( txt[1] );
						int g = V_nibble( txt[2] ) << 4 | V_nibble( txt[3] );
						int b = V_nibble( txt[4] ) << 4 | V_nibble( txt[5] );
						int a = 255;

						if ( bReadAlpha )
						{
							a = V_nibble( txt[6] ) << 4 | V_nibble( txt[7] );
						}

						range.color = Color( r, g, b, a );
						bFoundColorCode = true;

						txt += nCodeBytes;
					}
					else
					{
						// Not enough characters remaining for a hex code. Skip the rest of the string.
						bDone = true;
					}
				}
				break;
			default:
				++txt;
			}

			if ( bDone )
			{
				break;
			}

			if ( bFoundColorCode )
			{
				int count = m_textRanges.Count();
				if ( count )
				{
					m_textRanges[count-1].end = nBytesIn;
				}

				m_textRanges.AddToTail( range );
			}
		}
	}

	if ( !m_textRanges.Count() && m_iNameLength > 0 && m_text[0] == COLOR_USEOLDCOLORS )
	{
		TextRange range;
		range.start = 0;
		range.end = m_iNameStart;
		range.color = pChat->GetTextColorForClient( COLOR_NORMAL, clientIndex );
		m_textRanges.AddToTail( range );

		range.start = m_iNameStart;
		range.end = m_iNameStart + m_iNameLength;
		range.color = pChat->GetTextColorForClient( COLOR_PLAYERNAME, clientIndex );
		m_textRanges.AddToTail( range );

		range.start = range.end;
		range.end = wcslen( m_text );
		range.color = pChat->GetTextColorForClient( COLOR_NORMAL, clientIndex );
		m_textRanges.AddToTail( range );
	}

	if ( !m_textRanges.Count() )
	{
		TextRange range;
		range.start = 0;
		range.end = wcslen( m_text );
		range.color = pChat->GetTextColorForClient( COLOR_NORMAL, clientIndex );
		m_textRanges.AddToTail( range );
	}

	for ( int i=0; i<m_textRanges.Count(); ++i )
	{
		wchar_t * start = m_text + m_textRanges[i].start;
		if ( *start > 0 && *start < COLOR_MAX )
		{
			Assert( *start != COLOR_HEXCODE && *start != COLOR_HEXCODE_ALPHA );
			m_textRanges[i].start += 1;
		}
	}

	Colorize();
}

//-----------------------------------------------------------------------------
// Purpose: Inserts colored text into the RichText control at the given alpha
//-----------------------------------------------------------------------------
void CBaseHudChatLine::Colorize( int alpha )
{
	// clear out text
	SetText( "" );

	CBaseHudChat *pChat = dynamic_cast<CBaseHudChat*>(GetParent() );

	if ( pChat && pChat->GetChatHistory() )
	{	
		pChat->GetChatHistory()->InsertString( "\n" );
	}

	wchar_t wText[4096];
	Color color;
	for ( int i=0; i<m_textRanges.Count(); ++i )
	{
		wchar_t * start = m_text + m_textRanges[i].start;
		int len = m_textRanges[i].end - m_textRanges[i].start + 1;
		if ( len > 1 && len <= ARRAYSIZE( wText ) )
		{
			wcsncpy( wText, start, len );
			wText[len-1] = 0;
			color = m_textRanges[i].color;
			if ( !m_textRanges[i].preserveAlpha )
			{
				color[3] = alpha;
			}

			InsertColorChange( color );
			InsertString( wText );

			CBaseHudChat *pChat = dynamic_cast<CBaseHudChat*>(GetParent() );

			if ( pChat && pChat->GetChatHistory() )
			{	
				pChat->GetChatHistory()->InsertColorChange( color );
				pChat->GetChatHistory()->InsertString( wText );
				pChat->GetChatHistory()->InsertFade( hud_saytext_time.GetFloat(), CHAT_HISTORY_IDLE_FADE_TIME );

				if ( i == m_textRanges.Count()-1 )
				{
					pChat->GetChatHistory()->InsertFade( -1, -1 );
				}
			}

		}
	}

	InvalidateLayout( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseHudChatLine
//-----------------------------------------------------------------------------
CBaseHudChatLine *CBaseHudChat::FindUnusedChatLine( void )
{
#ifndef _XBOX
	return m_ChatLine;
#else
	return NULL;
#endif
}

void CBaseHudChat::Send( void )
{
#ifndef _XBOX
	wchar_t szTextbuf[128];

	m_pChatInput->GetMessageText( szTextbuf, sizeof( szTextbuf ) );
	
	char ansi[128];
	g_pVGuiLocalize->ConvertUnicodeToANSI( szTextbuf, ansi, sizeof( ansi ) );

	int len = Q_strlen(ansi);

	/*
This is a very long string that I am going to attempt to paste into the cs hud chat entry and we will see if it gets cropped or not.
	*/

	// remove the \n
	if ( len > 0 &&
		ansi[ len - 1 ] == '\n' )
	{
		ansi[ len - 1 ] = '\0';
	}

	if( len > 0 )
	{
		// Let the game rules at it
		if ( GameRules() )
		{
			GameRules()->ModifySentChat( ansi, ARRAYSIZE(ansi) );
		}

		char szbuf[144];	// more than 128
		Q_snprintf( szbuf, sizeof(szbuf), "%s \"%s\"", m_nMessageMode == MM_SAY ? "say" : "say_team", ansi );

		engine->ClientCmd_Unrestricted(szbuf);
	}
	
	m_pChatInput->ClearEntry();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : vgui::Panel
//-----------------------------------------------------------------------------
vgui::Panel *CBaseHudChat::GetInputPanel( void )
{
#ifndef _XBOX
	return m_pChatInput->GetInputPanel();
#else
	return NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudChat::Clear( void )
{
#ifndef _XBOX
	// Kill input prompt
	StopMessageMode();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *newmap - 
//-----------------------------------------------------------------------------
void CBaseHudChat::LevelInit( const char *newmap )
{
	Clear();

	//=============================================================================
	// HPE_BEGIN:
	// [pfreese] initialize new chat filters to defaults. We do this because
	// unused filter bits are zero, and we might want them on for new filters that
	// are added.
	//
	// Also, we have to do this here instead of somewhere more sensible like the 
	// c'tor or Init() method, because cvars are currently loaded twice: once
	// during initialization from the local file, and later (after HUD elements
	// have been construction and initialized) from Steam Cloud remote storage.
	//=============================================================================

	switch ( cl_chatfilter_version.GetInt() )
	{
	case 0:
		m_iFilterFlags |= CHAT_FILTER_ACHIEVEMENT;
		// fall through
	case kChatFilterVersion:
		break;
	}

	if ( cl_chatfilter_version.GetInt() != kChatFilterVersion )
	{
		cl_chatfilters.SetValue( m_iFilterFlags );
		cl_chatfilter_version.SetValue( kChatFilterVersion );
	}

	//=============================================================================
	// HPE_END
	//=============================================================================
}

void CBaseHudChat::LevelShutdown( void )
{
	Clear();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fmt - 
//			... - 
//-----------------------------------------------------------------------------
void CBaseHudChat::ChatPrintf( int iPlayerIndex, int iFilter, const char *fmt, ... )
{
	va_list marker;
	char msg[4096];

	va_start(marker, fmt);
	Q_vsnprintf(msg, sizeof( msg), fmt, marker);
	va_end(marker);

	// Strip any trailing '\n'
	if ( strlen( msg ) > 0 && msg[ strlen( msg )-1 ] == '\n' )
	{
		msg[ strlen( msg ) - 1 ] = 0;
	}

	// Strip leading \n characters ( or notify/color signifiers ) for empty string check
	char *pmsg = msg;
	while ( *pmsg && ( *pmsg == '\n' || ( *pmsg > 0 && *pmsg < COLOR_MAX ) ) )
	{
		pmsg++;
	}

	if ( !*pmsg )
		return;

	// Now strip just newlines, since we want the color info for printing
	pmsg = msg;
	while ( *pmsg && ( *pmsg == '\n' ) )
	{
		pmsg++;
	}

	if ( !*pmsg )
		return;

	CBaseHudChatLine *line = (CBaseHudChatLine *)FindUnusedChatLine();
	if ( !line )
	{
		line = (CBaseHudChatLine *)FindUnusedChatLine();
	}

	if ( !line )
	{
		return;
	}

	if ( iFilter != CHAT_FILTER_NONE )
	{
		if ( !(iFilter & GetFilterFlags() ) )
			return;
	}

	// If a player is muted for voice, also mute them for text because jerks gonna jerk.
	if ( cl_mute_all_comms.GetBool() && iPlayerIndex != 0 )
	{
		if ( GetClientVoiceMgr() && GetClientVoiceMgr()->IsPlayerBlocked( iPlayerIndex ) )	
			return;
	}

	if ( *pmsg < 32 )
	{
		hudlcd->AddChatLine( pmsg + 1 );
	}
	else
	{
		hudlcd->AddChatLine( pmsg );
	}

	line->SetText( "" );

	int iNameStart = 0;
	int iNameLength = 0;

	player_info_t sPlayerInfo;
	if ( iPlayerIndex == 0 )
	{
		Q_memset( &sPlayerInfo, 0, sizeof(player_info_t) );
		Q_strncpy( sPlayerInfo.name, "Console", sizeof(sPlayerInfo.name)  );	
	}
	else
	{
		engine->GetPlayerInfo( iPlayerIndex, &sPlayerInfo );
	}	

	int bufSize = (strlen( pmsg ) + 1 ) * sizeof(wchar_t);
	wchar_t *wbuf = static_cast<wchar_t *>( _alloca( bufSize ) );
	if ( wbuf )
	{
		Color clrNameColor = GetClientColor( iPlayerIndex );

		line->SetExpireTime();

		g_pVGuiLocalize->ConvertANSIToUnicode( pmsg, wbuf, bufSize);

		// find the player's name in the unicode string, in case there is no color markup
		const char *pName = sPlayerInfo.name;

		if ( pName )
		{
			wchar_t wideName[MAX_PLAYER_NAME_LENGTH];
			g_pVGuiLocalize->ConvertANSIToUnicode( pName, wideName, sizeof( wideName ) );

			const wchar_t *nameInString = wcsstr( wbuf, wideName );

			if ( nameInString )
			{
				iNameStart = (nameInString - wbuf);
				iNameLength = wcslen( wideName );
			}
		}

		line->SetVisible( false );
		line->SetNameStart( iNameStart );
		line->SetNameLength( iNameLength );
		line->SetNameColor( clrNameColor );

		line->InsertAndColorizeText( wbuf, iPlayerIndex );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudChat::FireGameEvent( IGameEvent *event )
{
#ifndef _XBOX
	const char *eventname = event->GetName();

	if ( Q_strcmp( "hltv_chat", eventname ) == 0 )
	{
		C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();

		if ( !player )
			return;
		
		ChatPrintf( player->entindex(), CHAT_FILTER_NONE, "(SourceTV) %s", event->GetString( "text" ) );
	}
#endif
}