//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include <stdarg.h>
#include "imessagechars.h"
#include <vgui/IVGui.h>
#include "VGuiMatSurface/IMatSystemSurface.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/Controls.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Simultaneous message limit
#define MAX_MESSAGECHARS_MESSAGES 1024
#define MAX_MESSAGECHARSPANEL_LEN 1024

//-----------------------------------------------------------------------------
// Purpose: Panel for displaying console characters at specified locations
//-----------------------------------------------------------------------------
class CMessageCharsPanel : public vgui::Panel
{
	typedef vgui::Panel BaseClass;
public:
	// Internal pool of such messages
	typedef struct message_s
	{
		struct		message_s *next;
		int			x, y;
		byte		r, g, b, a;
		char		*text;
		vgui::HFont	hCustomFont;
		float		fTTL;
		int			messageID;
	} message_t;

	// Construct/destruct
						CMessageCharsPanel( vgui::VPANEL parent );
	virtual				~CMessageCharsPanel( void );

	// Add block of text to list
	virtual int			AddText( 
		float flTime, 
		vgui::HFont hCustomFont, 
		int x, 
		int y, 
		int r, 
		int g, 
		int b, 
		int a, 
		char *fmt, 
		int messageID,
		... );

	// Determine text side and height
	virtual void		GetTextExtents( vgui::HFont hCustomFont, int *wide, int *tall, const char *string );

	virtual void		ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void		Paint();
	virtual void		OnTick( void );

	virtual bool		ShouldDraw( void );

	void				RemoveStringsByID( int messageID );

	void				Clear( void );

private:
	// Allocate a new message
	message_t			*AllocMessage( void );
	// Clear out all messages
	void				Reset( void );

	vgui::HFont			m_hFont;

	// Pool of messages
	message_t			m_Messages[ MAX_MESSAGECHARS_MESSAGES ];
	message_t			*m_pActive;
	message_t			*m_pFree;
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *parent - 
// Output : 
//-----------------------------------------------------------------------------
CMessageCharsPanel::CMessageCharsPanel( vgui::VPANEL parent ) : 
	BaseClass( NULL, "CMessageCharsPanel" )
{
	SetParent( parent );
	SetSize( ScreenWidth(), ScreenHeight() );
	SetPos( 0, 0 );
	SetVisible( true );
	SetCursor( null );
	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );
	
	m_hFont = vgui::INVALID_FONT; 

	SetFgColor( Color( 0, 0, 0, 255 ) );
	SetPaintBackgroundEnabled( false );

	Q_memset( m_Messages, 0, sizeof( m_Messages ) );

	Reset();

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
CMessageCharsPanel::~CMessageCharsPanel( void )
{
}

void CMessageCharsPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont( "Default" );
	Assert( m_hFont != vgui::INVALID_FONT );

	SetSize( ScreenWidth(), ScreenHeight() );
	SetPos( 0, 0 );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMessageCharsPanel::Clear( void )
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: Reset all messages
//-----------------------------------------------------------------------------
void CMessageCharsPanel::Reset( void )
{
	m_pActive = NULL;
	int i;
	for( i = 0; i < MAX_MESSAGECHARS_MESSAGES-1; i++ )
	{
		if ( m_Messages[ i ].text )
		{
			delete[] m_Messages[ i ].text;
			m_Messages[ i ].text = NULL;
		}
		m_Messages[ i ].next = &m_Messages[ i + 1 ];
	}
	m_Messages[ i ].next = NULL;
	m_pFree = &m_Messages[ 0 ];
	SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: Allocate a message if possible
// Output : CMessageCharsPanel::message_t
//-----------------------------------------------------------------------------
CMessageCharsPanel::message_t *CMessageCharsPanel::AllocMessage( void )
{
	CMessageCharsPanel::message_t *msg;

	if ( !m_pFree )
		return NULL;

	msg			= m_pFree;
	m_pFree		= m_pFree->next;

	msg->next	= m_pActive;
	m_pActive	= msg;

	msg->x		= 0;
	msg->y		= 0;
	msg->text	= NULL;

	msg->hCustomFont = NULL;

	return msg;
}

//-----------------------------------------------------------------------------
// Purpose: Allocate message and fill in data
// Input  : x - 
//			y - 
//			*fmt - 
//			... - 
// Output : int
//-----------------------------------------------------------------------------
int CMessageCharsPanel::AddText( 
	float flTime, 
	vgui::HFont hCustomFont, 
	int x, 
	int y, 
	int r, 
	int g, 
	int b, 
	int a, 
	char *fmt, 
	int messageID,
	... )
{
	va_list argptr;
	char data[ MAX_MESSAGECHARSPANEL_LEN ];
	int len;

	va_start(argptr, messageID);
	len = Q_vsnprintf(data, sizeof( data ), fmt, argptr);
	va_end(argptr);

	data[ MAX_MESSAGECHARSPANEL_LEN - 1 ] = 0;

	CMessageCharsPanel::message_t *msg = AllocMessage();
	if ( !msg )
		return x;

	msg->x = x;
	msg->y = y;
	msg->r = r;
	msg->g = g;
	msg->b = b;
	msg->a = a;
	msg->messageID = messageID;

	Assert( !msg->text );

	int textLength = Q_strlen( data ) + 1;
	msg->text = new char[ textLength ];
	Assert( msg->text );
	Q_strncpy( msg->text, data, textLength );

	if ( flTime )
		msg->fTTL = gpGlobals->curtime + flTime;
	else
		msg->fTTL = 0;
	SetVisible( true );

	if ( hCustomFont )
		msg->hCustomFont = hCustomFont;
	else
		msg->hCustomFont = m_hFont;

	// Return new cursor position
	return x + g_pMatSystemSurface->DrawTextLen( msg->hCustomFont, "%s", data );
}

//-----------------------------------------------------------------------------
// Purpose: Determine text size ahead of time
// Input  : *wide - 
//			*tall - 
//			*string - 
//-----------------------------------------------------------------------------
void CMessageCharsPanel::GetTextExtents( vgui::HFont hCustomFont, int *wide, int *tall, const char *string )
{
	if ( !hCustomFont )
	{
		// Make sure we actually have the font...
		vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
		hCustomFont = pScheme->GetFont( "Default" );
	}

	Assert( hCustomFont );

	*wide = g_pMatSystemSurface->DrawTextLen( hCustomFont, "%s", (char *)string );
	*tall = vgui::surface()->GetFontTall( hCustomFont );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMessageCharsPanel::OnTick( void )
{
	bool bVisible = ShouldDraw();
	if ( IsVisible() != bVisible )
	{
		SetVisible( bVisible );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMessageCharsPanel::ShouldDraw( void )
{
	if ( !m_pActive )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
//-----------------------------------------------------------------------------
void CMessageCharsPanel::Paint() 
{
	CMessageCharsPanel::message_t *msg = m_pActive;
	while ( msg )
	{
		g_pMatSystemSurface->DrawColoredText( msg->hCustomFont, msg->x, msg->y, msg->r, msg->g, msg->b, msg->a, "%s", msg->text );
		msg = msg->next;
	}

	// Clear our dead messages
	message_t *pPrev = NULL;
	message_t *pCurrent = m_pActive;
	while ( pCurrent )
	{
		if ( pCurrent->fTTL <= gpGlobals->curtime )
		{
			// Move it to the free list
			if ( !pPrev )
			{
				m_pActive = pCurrent->next;
			}
			else
			{
				pPrev->next = pCurrent->next;
			}

			// Store off next one, because we're about to move the current
			message_t *pNext = pCurrent->next;
			delete[] pCurrent->text;
			pCurrent->text = NULL;
			pCurrent->next = m_pFree;
			m_pFree = pCurrent;

			// Don't advance pPrev
			pCurrent = pNext;
			continue;
		}

		pPrev = pCurrent;
		pCurrent = pCurrent->next;
	}
}


void CMessageCharsPanel::RemoveStringsByID( int messageID )
{
	for ( message_t *pCurrent = m_pActive; pCurrent; pCurrent = pCurrent->next )
	{
		if ( pCurrent->messageID == messageID )
			pCurrent->fTTL = gpGlobals->curtime - 1000;
	}
}

class CMessageChars : public IMessageChars
{
private:
	CMessageCharsPanel *messageCharsPanel;
public:
	CMessageChars( void )
	{
		messageCharsPanel = NULL;
	}

	void Create( vgui::VPANEL parent )
	{
		messageCharsPanel = new CMessageCharsPanel( parent );
	}

	void Destroy( void )
	{
		if ( messageCharsPanel )
		{
			messageCharsPanel->SetParent( (vgui::Panel *)NULL );
			messageCharsPanel->MarkForDeletion();
			messageCharsPanel = NULL;
		}
	}

	int DrawStringForTime( float flTime, vgui::HFont hCustomFont, int x, int y, int r, int g, int b, int a, const char *fmt, int messageID, ... )
	{
		va_list argptr;
		char data[ MAX_MESSAGECHARSPANEL_LEN ];
		int len;

		va_start(argptr, messageID);
		len = Q_vsnprintf(data, sizeof( data ), fmt, argptr);
		va_end(argptr);

		data[ MAX_MESSAGECHARSPANEL_LEN - 1 ] = 0;

		if ( !messageCharsPanel )
			return x;

		return messageCharsPanel->AddText( flTime, hCustomFont, x, y, r, g, b, a, data, messageID );
	}

	int DrawStringForTime( float flTime, vgui::HFont hCustomFont, int x, int y, const char *fmt, int messageID, ... )
	{
		int r = 192, g = 192, b = 192;

		va_list argptr;
		va_start(argptr, messageID);
		int result = DrawString( hCustomFont, x, y, r, g, b, 255, fmt, messageID, argptr );
		va_end( argptr );
		return result;
	}

	virtual void RemoveStringsByID( int messageID )
	{
		messageCharsPanel->RemoveStringsByID( messageID );
	}

	int DrawString( vgui::HFont hCustomFont, int x, int y, int r, int g, int b, int a, const char *fmt, int messageID, ... )
	{
		va_list argptr;
		va_start(argptr, messageID);
		int result = DrawStringForTime( 0, hCustomFont, x, y, r, g, b, a, fmt, messageID, argptr );
		va_end( argptr );
		return result;
	}

	int DrawString( vgui::HFont hCustomFont, int x, int y, const char *fmt, int messageID, ... )
	{
		va_list argptr;
		va_start(argptr, messageID);
		int result = DrawStringForTime( 0, hCustomFont, x, y, fmt, messageID, argptr );
		va_end( argptr );
		return result;
	}

	void GetStringLength( vgui::HFont hCustomFont, int *width, int *height, const char *fmt, ... )
	{
		if ( !messageCharsPanel )
		{
			return;
		}

		va_list argptr;
		char data[ MAX_MESSAGECHARSPANEL_LEN ];
		int len;

		va_start(argptr, fmt);
		len = Q_vsnprintf(data, sizeof( data ), fmt, argptr);
		va_end(argptr);

		data[ MAX_MESSAGECHARSPANEL_LEN - 1 ] = 0;

		messageCharsPanel->GetTextExtents( hCustomFont, width, height, data );
	}

	void Clear( void )
	{
		if ( !messageCharsPanel )
			return;
		messageCharsPanel->Clear();
	}
};

static CMessageChars g_MessageChars;
IMessageChars *messagechars = ( IMessageChars * )&g_MessageChars;
