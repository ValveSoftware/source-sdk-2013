//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "vguitextwindow.h"
#include <networkstringtabledefs.h>
#include <cdll_client_int.h>
#include <clientmode_shared.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <filesystem.h>
#include <KeyValues.h>
#include <convar.h>
#include <vgui_controls/ImageList.h>

#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Button.h>

#include <game/client/iviewport.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;
extern INetworkStringTable *g_pStringTableInfoPanel;

#define TEMP_HTML_FILE	"textwindow_temp.html"

ConVar cl_disablehtmlmotd( "cl_disablehtmlmotd", "0", FCVAR_ARCHIVE, "Disable HTML motds." );

//=============================================================================
// HPE_BEGIN:
// [Forrest] Replaced text window command string with TEXTWINDOW_CMD enumeration
// of options.  Passing a command string is dangerous and allowed a server network
// message to run arbitrary commands on the client.
//=============================================================================
CON_COMMAND( showinfo, "Shows a info panel: <type> <title> <message> [<command number>]" )
{
	if ( !gViewPortInterface )
		return;
	
	if ( args.ArgC() < 4 )
		return;
		
	IViewPortPanel * panel = gViewPortInterface->FindPanelByName( PANEL_INFO );

	 if ( panel )
	 {
		 KeyValues *kv = new KeyValues("data");
		 kv->SetInt( "type", Q_atoi(args[ 1 ]) );
		 kv->SetString( "title", args[ 2 ] );
		 kv->SetString( "message", args[ 3 ] );

		 if ( args.ArgC() == 5 )
			 kv->SetString( "command", args[ 4 ] );

		 panel->SetData( kv );

		 gViewPortInterface->ShowPanel( panel, true );

		 kv->deleteThis();
	 }
	 else
	 {
		 Msg("Couldn't find info panel.\n" );
	 }
}
//=============================================================================
// HPE_END
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTextWindow::CTextWindow(IViewPort *pViewPort) : Frame(NULL, PANEL_INFO	)
{
	// initialize dialog
	m_pViewPort = pViewPort;

//	SetTitle("", true);

	m_szTitle[0] = '\0';
	m_szMessage[0] = '\0';
	m_szMessageFallback[0] = '\0';
	m_nExitCommand = TEXTWINDOW_CMD_NONE;
	m_bShownURL = false;
	m_bUnloadOnDismissal = false;
	
	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);
	SetProportional(true);

	// hide the system buttons
	SetTitleBarVisible( false );

	m_pTextMessage = new TextEntry( this, "TextMessage" );
#if defined( ENABLE_CHROMEHTMLWINDOW )
	m_pHTMLMessage = new CMOTDHTML( this,"HTMLMessage" );
#else
	m_pHTMLMessage = NULL;
#endif
	m_pTitleLabel  = new Label( this, "MessageTitle", "Message Title" );
	m_pOK		   = new Button(this, "ok", "#PropertyDialog_OK");

	m_pOK->SetCommand("okay");
	m_pTextMessage->SetMultiline( true );
	m_nContentType = TYPE_TEXT;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTextWindow::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings("Resource/UI/TextWindow.res");

	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTextWindow::~CTextWindow()
{
	// remove temp file again
	g_pFullFileSystem->RemoveFile( TEMP_HTML_FILE, "DEFAULT_WRITE_PATH" );
}

void CTextWindow::Reset( void )
{
	//=============================================================================
	// HPE_BEGIN:
	// [Forrest] Replace strange hard-coded default message with hard-coded error message.
	//=============================================================================
	Q_strcpy( m_szTitle, "Error loading info message." );
	Q_strcpy( m_szMessage, "" );
	Q_strcpy( m_szMessageFallback, "" );
	//=============================================================================
	// HPE_END
	//=============================================================================

	m_nExitCommand = TEXTWINDOW_CMD_NONE;
	m_nContentType = TYPE_TEXT;
	m_bShownURL = false;
	m_bUnloadOnDismissal = false;
	Update();
}

void CTextWindow::ShowText( const char *text )
{
	m_pTextMessage->SetVisible( true );
	m_pTextMessage->SetText( text );
	m_pTextMessage->GotoTextStart();
}

void CTextWindow::ShowURL( const char *URL, bool bAllowUserToDisable )
{
#if defined( ENABLE_CHROMEHTMLWINDOW )
	#ifdef _DEBUG
		Msg( "CTextWindow::ShowURL( %s )\n", URL );
	#endif

	ClientModeShared *mode = ( ClientModeShared * )GetClientModeNormal();
	if ( ( bAllowUserToDisable && cl_disablehtmlmotd.GetBool() ) || !mode->IsHTMLInfoPanelAllowed() )
	{
		Warning( "Blocking HTML info panel '%s'; Using plaintext instead.\n", URL );

		// User has disabled HTML TextWindows. Show the fallback as text only.
		if ( g_pStringTableInfoPanel )
		{
			int index = g_pStringTableInfoPanel->FindStringIndex( "motd_text" );
			if ( index != ::INVALID_STRING_INDEX )
			{
				int length = 0;
				const char *data = (const char *)g_pStringTableInfoPanel->GetStringUserData( index, &length );
				if ( data && data[0] )
				{
					m_pHTMLMessage->SetVisible( false );
					ShowText( data );
				}
			}
		}
		return;
	} 

	m_pHTMLMessage->SetVisible( true );
	m_pHTMLMessage->OpenURL( URL, NULL );
	m_bShownURL = true;

#endif
}

void CTextWindow::ShowIndex( const char *entry )
{
	const char *data = NULL;
	int length = 0;

	if ( NULL == g_pStringTableInfoPanel )
		return;

	int index = g_pStringTableInfoPanel->FindStringIndex( m_szMessage );
		
	if ( index != ::INVALID_STRING_INDEX )
		data = (const char *)g_pStringTableInfoPanel->GetStringUserData( index, &length );

	if ( !data || !data[0] )
		return; // nothing to show

	// is this a web URL ?
	if ( !Q_strncmp( data, "http://", 7 ) || !Q_strncmp( data, "https://", 8 ) )
	{
		ShowURL( data );
		return;
	}

	// try to figure out if this is HTML or not
	if ( data[0] != '<' )
	{
		ShowText( data );
		return;
	}

	// data is a HTML, we have to write to a file and then load the file
	FileHandle_t hFile = g_pFullFileSystem->Open( TEMP_HTML_FILE, "wb", "DEFAULT_WRITE_PATH" );

	if ( hFile == FILESYSTEM_INVALID_HANDLE )
		return;

	g_pFullFileSystem->Write( data, length, hFile );
	g_pFullFileSystem->Close( hFile );

	if ( g_pFullFileSystem->Size( TEMP_HTML_FILE ) != (unsigned int)length )
		return; // something went wrong while writing

	ShowFile( TEMP_HTML_FILE );
}

void CTextWindow::ShowFile( const char *filename )
{
	if  ( Q_stristr( filename, ".htm" ) || Q_stristr( filename, ".html" ) )
	{
		// it's a local HTML file
		char localURL[ _MAX_PATH + 7 ];
		Q_strncpy( localURL, "file://", sizeof( localURL ) );
		
		char pPathData[ _MAX_PATH ];
		g_pFullFileSystem->GetLocalPath( filename, pPathData, sizeof(pPathData) );
		Q_strncat( localURL, pPathData, sizeof( localURL ), COPY_ALL_CHARACTERS );

		ShowURL( localURL );
	}
	else
	{
		// read from local text from file
		FileHandle_t f = g_pFullFileSystem->Open( m_szMessage, "rb", "GAME" );

		if ( !f )
			return;

		char buffer[2048];
			
		int size = MIN( g_pFullFileSystem->Size( f ), sizeof(buffer)-1 ); // just allow 2KB

		g_pFullFileSystem->Read( buffer, size, f );
		g_pFullFileSystem->Close( f );

		buffer[size]=0; //terminate string

		ShowText( buffer );
	}
}

void CTextWindow::Update( void )
{
	SetTitle( m_szTitle, false );

	m_pTitleLabel->SetText( m_szTitle );

#if defined( ENABLE_CHROMEHTMLWINDOW )
	m_pHTMLMessage->SetVisible( false );
#endif
	m_pTextMessage->SetVisible( false );

	if ( m_nContentType == TYPE_INDEX )
	{
		ShowIndex( m_szMessage );
	}
	else if ( m_nContentType == TYPE_URL )
	{
		if ( !Q_strncmp( m_szMessage, "http://", 7 ) || !Q_strncmp( m_szMessage, "https://", 8 ) || !Q_stricmp( m_szMessage, "about:blank" ) )
		{
			ShowURL( m_szMessage );
		}
		else
		{
			// We should have trapped this at a higher level
			Assert( !"URL protocol is missing or blocked" );
		}
	}
	else if ( m_nContentType == TYPE_FILE )
	{
		ShowFile( m_szMessage );
	}
	else if ( m_nContentType == TYPE_TEXT )
	{
		ShowText( m_szMessage );
	}
	else
	{
		DevMsg("CTextWindow::Update: unknown content type %i\n", m_nContentType );
	}
}

void CTextWindow::OnCommand( const char *command )
{
	if (!Q_strcmp(command, "okay"))
	{
		//=============================================================================
		// HPE_BEGIN:
		// [Forrest] Replaced text window command string with TEXTWINDOW_CMD enumeration
		// of options.  Passing a command string is dangerous and allowed a server network
		// message to run arbitrary commands on the client.
		//=============================================================================
		const char *pszCommand = NULL;
		switch ( m_nExitCommand )
		{
			case TEXTWINDOW_CMD_NONE:
				break;

			case TEXTWINDOW_CMD_JOINGAME:
				pszCommand = "joingame";
				break;

			case TEXTWINDOW_CMD_CHANGETEAM:
				pszCommand = "changeteam";
				break;

			case TEXTWINDOW_CMD_IMPULSE101:
				pszCommand = "impulse 101";
				break;

			case TEXTWINDOW_CMD_MAPINFO:
				pszCommand = "mapinfo";
				break;

			case TEXTWINDOW_CMD_CLOSED_HTMLPAGE:
				pszCommand = "closed_htmlpage";
				break;

			case TEXTWINDOW_CMD_CHOOSETEAM:
				pszCommand = "chooseteam";
				break;

			default:
				DevMsg("CTextWindow::OnCommand: unknown exit command value %i\n", m_nExitCommand );
				break;
		}

		if ( pszCommand != NULL )
		{
			engine->ClientCmd_Unrestricted( pszCommand );
		}
		//=============================================================================
		// HPE_END
		//=============================================================================
		
		m_pViewPort->ShowPanel( this, false );
	}

	BaseClass::OnCommand(command);
}

void CTextWindow::OnKeyCodePressed( vgui::KeyCode code )
{
	if ( code == KEY_XBUTTON_A || code == KEY_XBUTTON_B )
	{
		OnCommand( "okay" );
		return;
	}

	BaseClass::OnKeyCodePressed(code);
}

void CTextWindow::SetData(KeyValues *data)
{
	SetData( data->GetInt( "type" ), data->GetString( "title" ), data->GetString( "msg" ), data->GetString( "msg_fallback" ), data->GetInt( "cmd" ), data->GetBool( "unload" ) );
}

void CTextWindow::SetData( int type, const char *title, const char *message, const char *message_fallback, int command, bool bUnload )
{
	Q_strncpy(  m_szTitle, title, sizeof( m_szTitle ) );
	Q_strncpy(  m_szMessage, message, sizeof( m_szMessage ) );
	Q_strncpy(  m_szMessageFallback, message_fallback, sizeof( m_szMessageFallback ) );

	m_nExitCommand = command;

	m_nContentType = type;
	m_bUnloadOnDismissal = bUnload;

	Update();
}

void CTextWindow::ShowPanel( bool bShow )
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	m_pViewPort->ShowBackGround( bShow );

	if ( bShow )
	{
		Activate();
		SetMouseInputEnabled( true );
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );

#if defined( ENABLE_CHROMEHTMLWINDOW )
		if ( m_bUnloadOnDismissal && m_bShownURL )
		{
			m_pHTMLMessage->OpenURL( "about:blank", NULL );
			m_bShownURL = false;
		}
#endif
	}
}

bool CTextWindow::CMOTDHTML::OnStartRequest( const char *url, const char *target, const char *pchPostData, bool bIsRedirect )
{
	if ( Q_strstr( url, "steam://" ) )
		return false;

	return BaseClass::OnStartRequest( url, target, pchPostData, bIsRedirect );
}
