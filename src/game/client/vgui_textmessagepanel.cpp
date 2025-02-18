//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "itextmessage.h"
#include <vgui_controls/Panel.h>
#include <vgui/IVGui.h>
#include <vgui/ILocalize.h>
#include "VGuiMatSurface/IMatSystemSurface.h"
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include "hud.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Simultaneous message limit
#define MAX_TEXTMESSAGE_CHARS 2048

//-----------------------------------------------------------------------------
// Purpose: For rendering the Titles.txt characters to the screen from the HUD
//-----------------------------------------------------------------------------
class CTextMessagePanel : public vgui::Panel
{
	typedef vgui::Panel BaseClass;
public:
	enum
	{
		TYPE_UNKNOWN = 0,
		TYPE_POSITION,
		TYPE_CHARACTER,
		TYPE_FONT,
	};

	struct message_t
	{
		vgui::HFont	font;
		short		x, y;
		wchar_t		ch;
		byte		type;
		byte		r, g, b, a;
	};

						CTextMessagePanel( vgui::VPANEL parent );
	virtual				~CTextMessagePanel( void );

	virtual void		SetPosition( int x, int y );

	virtual void		AddChar( int r, int g, int b, int a, wchar_t ch );
	virtual void		GetTextExtents( int *wide, int *tall, const char *string );

	virtual void		SetFont( vgui::HFont hCustomFont );
	virtual void		SetDefaultFont( void );

	virtual void		OnTick( void );

	virtual void		Paint();

	virtual bool		ShouldDraw( void );

	// Get character data for textmessage text
	virtual int			GetFontInfo( FONTABC *pABCs, vgui::HFont hFont );

	virtual void		ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );
		SetSize( ScreenWidth(), ScreenHeight() );
		SetPos( 0, 0 );
	}

private:
	message_t			*AllocMessage( void );
	void				Reset( void );

	vgui::HFont			m_hFont;
	vgui::HFont			m_hDefaultFont;
	CUtlVector< message_t > m_Messages;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : *parent - 
//-----------------------------------------------------------------------------
CTextMessagePanel::CTextMessagePanel( vgui::VPANEL parent )
: BaseClass( NULL, "CTextMessagePanel" )
{
	SetParent( parent );
	SetSize( ScreenWidth(), ScreenHeight() );
	SetPos( 0, 0 );
	SetVisible( false );
	SetCursor( null );
	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );

	m_hFont = g_hFontTrebuchet24;
	m_hDefaultFont = m_hFont;

	SetFgColor( Color( 0, 0, 0, 255 ) );
	SetPaintBackgroundEnabled( false );

	// Clear memory out
	Reset();

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTextMessagePanel::~CTextMessagePanel( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get font sizes
// Input  : *pWidth - 
// Output : int
//-----------------------------------------------------------------------------
int CTextMessagePanel::GetFontInfo( FONTABC *pABCs, vgui::HFont hFont )
{
	int i;

	if ( !hFont )
	{
		hFont = m_hFont;
	}

	if ( !hFont )
		return 0;

	if ( pABCs )
	{
		for ( i =0; i < 256; i++ )
		{
			int a, b, c;
			vgui::surface()->GetCharABCwide( hFont, (char)i, a, b, c );
			pABCs[i].abcA = a;
			pABCs[i].abcB = b;
			pABCs[i].abcC = c;
			pABCs[i].total = a+b+c;
		}
	}

	return vgui::surface()->GetFontTall( hFont );
}

//-----------------------------------------------------------------------------
// Purpose: Clear all messages out of active list, etc.
//-----------------------------------------------------------------------------
void CTextMessagePanel::Reset( void )
{
	m_Messages.Purge();
	SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: Grab next free message, if any
// Output : CTextMessagePanel::message_t
//-----------------------------------------------------------------------------
CTextMessagePanel::message_t *CTextMessagePanel::AllocMessage( void )
{
	CTextMessagePanel::message_t *msg;

	if ( m_Messages.Count() >= MAX_TEXTMESSAGE_CHARS )
		return NULL;

	msg = &m_Messages[ m_Messages.AddToTail() ];

	msg->type = TYPE_UNKNOWN;
	msg->x = 0;
	msg->y = 0;
	msg->ch = 0;
	msg->r = 0;
	msg->g = 0;
	msg->b = 0;
	msg->a = 0;

	SetVisible( true );

	return msg;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : x - 
//			y - 
//-----------------------------------------------------------------------------
void CTextMessagePanel::SetPosition( int x, int y )
{
	CTextMessagePanel::message_t *msg = AllocMessage();
	if ( !msg )
		return;

	msg->type = TYPE_POSITION;

	// Used fields
	msg->x = x;
	msg->y = y;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a character to the active list, if possible
// Input  : x - 
//			y - 
//			r - 
//			g - 
//			b - 
//			a - 
//			ch - 
// Output : int
//-----------------------------------------------------------------------------
void CTextMessagePanel::AddChar( int r, int g, int b, int a, wchar_t ch )
{
	CTextMessagePanel::message_t *msg = AllocMessage();
	if ( !msg )
		return;

	msg->type = TYPE_CHARACTER;

	// Used fields
	msg->r = r;
	msg->g = g;
	msg->b = b;
	msg->a = a;
	msg->ch = ch;
}

//-----------------------------------------------------------------------------
// Purpose: Determine width and height of specified string
// Input  : *wide - 
//			*tall - 
//			*string - 
//-----------------------------------------------------------------------------
void CTextMessagePanel::GetTextExtents( int *wide, int *tall, const char *string )
{
	*wide = g_pMatSystemSurface->DrawTextLen( m_hFont, "%s", (char *)string );
	*tall = vgui::surface()->GetFontTall( m_hFont );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTextMessagePanel::SetFont( vgui::HFont hCustomFont )
{
	m_hFont = hCustomFont;

	CTextMessagePanel::message_t *msg = AllocMessage();
	if ( !msg )
		return;

	msg->type = TYPE_FONT;

	// Used fields
	msg->font = m_hFont;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTextMessagePanel::SetDefaultFont( void )
{
	SetFont( m_hDefaultFont );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTextMessagePanel::OnTick( void )
{
	SetVisible( ShouldDraw() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTextMessagePanel::ShouldDraw( void )
{
	if ( !m_Messages.Count() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Draw current text items
//-----------------------------------------------------------------------------
void CTextMessagePanel::Paint() 
{
	CTextMessagePanel::message_t *msg;

	int xpos = 0, ypos = 0;
	vgui::surface()->DrawSetTextFont( m_hFont );

	int messageCount = m_Messages.Count();
	for ( int i = 0 ; i < messageCount; ++i )
	{
		msg = &m_Messages[ i ];
	
		switch ( msg->type )
		{
		default:
		case TYPE_UNKNOWN:
			Assert( 0 );
			break;
		case TYPE_POSITION:
			xpos = msg->x;
			ypos = msg->y;
			break;
		case TYPE_FONT:
			m_hFont = msg->font;
			vgui::surface()->DrawSetTextFont( m_hFont );
			break;
		case TYPE_CHARACTER:
			if ( m_hFont )
			{
				int a, b, c;
				vgui::surface()->GetCharABCwide( m_hFont, msg->ch, a, b, c );

				if ( msg->ch > 32 )
				{
					vgui::surface()->DrawSetTextColor( msg->r,  msg->g,  msg->b,  msg->a );
					vgui::surface()->DrawSetTextPos( xpos, ypos );
					vgui::surface()->DrawUnicodeChar( msg->ch );
				}
				xpos += a + b + c;
			}
			break;
		}
	}

	Reset();
}

class CTextMessage : public ITextMessage 
{
private:
	CTextMessagePanel *textMessagePanel;
public:
	CTextMessage( void )
	{
		textMessagePanel = NULL;
	}

	void Create( vgui::VPANEL parent )
	{
		textMessagePanel = new CTextMessagePanel( parent );
	}

	void Destroy( void )
	{
		if ( textMessagePanel )
		{
			textMessagePanel->SetParent( (vgui::Panel *)NULL );
			delete textMessagePanel;
		}
	}

	void SetPosition( int x, int y )
	{
		if ( !textMessagePanel )
			return;

		textMessagePanel->SetPosition( x, y );
	}

	void AddChar( int r, int g, int b, int a, wchar_t ch )
	{
		if ( !textMessagePanel )
			return;

		textMessagePanel->AddChar( r, g, b, a, ch );
	}

	void GetLength( int *wide, int *tall, const char *string )
	{
		if ( !textMessagePanel )
		{
			*wide = *tall = 0;
			return;
		}

		textMessagePanel->GetTextExtents( wide, tall, string );
	}

	int GetFontInfo( FONTABC *pABCs, vgui::HFont hFont )
	{
		return textMessagePanel ? textMessagePanel->GetFontInfo( pABCs, hFont ) : 0;
	}

	void SetFont( vgui::HFont hCustomFont )
	{
		if ( !textMessagePanel )
			return;

		textMessagePanel->SetFont( hCustomFont );
	}

	void SetDefaultFont( void )
	{
		if ( !textMessagePanel )
			return;

		textMessagePanel->SetDefaultFont();
	}
};

static CTextMessage g_TextMessage;
ITextMessage *textmessage = &g_TextMessage;
