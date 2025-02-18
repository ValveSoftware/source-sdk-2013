//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef HUD_BASECHAT_H
#define HUD_BASECHAT_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include "vgui_basepanel.h"
#include "vgui_controls/Frame.h"
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>

class CBaseHudChatInputLine;
class CBaseHudChatEntry;
class CHudChatFilterPanel;

namespace vgui
{
	class IScheme;
};

#define CHATLINE_NUM_FLASHES 8.0f
#define CHATLINE_FLASH_TIME 5.0f
#define CHATLINE_FADE_TIME 1.0f

#define CHAT_HISTORY_FADE_TIME 0.25f
#define CHAT_HISTORY_IDLE_TIME 15.0f
#define CHAT_HISTORY_IDLE_FADE_TIME 2.5f
#define CHAT_HISTORY_ALPHA 127

extern Color g_ColorBlue;
extern Color g_ColorRed;
extern Color g_ColorGreen;
extern Color g_ColorDarkGreen;
extern Color g_ColorYellow;
extern Color g_ColorGrey;

extern ConVar cl_showtextmsg;

enum ChatFilters
{
	CHAT_FILTER_NONE		= 0,
	CHAT_FILTER_JOINLEAVE	= 0x000001,
	CHAT_FILTER_NAMECHANGE	= 0x000002,
	CHAT_FILTER_PUBLICCHAT	= 0x000004,
	CHAT_FILTER_SERVERMSG	= 0x000008,
	CHAT_FILTER_TEAMCHANGE	= 0x000010,
    //=============================================================================
    // HPE_BEGIN:
    // [tj]Added a filter for achievement announce
    //=============================================================================
     
    CHAT_FILTER_ACHIEVEMENT	= 0x000020,
     
    //=============================================================================
    // HPE_END
    //=============================================================================
};


//-----------------------------------------------------------------------------
enum TextColor
{
	COLOR_NORMAL = 1,
	COLOR_USEOLDCOLORS = 2,
	COLOR_PLAYERNAME = 3,
	COLOR_LOCATION = 4,
	COLOR_ACHIEVEMENT = 5,
	COLOR_CUSTOM = 6,		// Will use the most recently SetCustomColor()
	COLOR_HEXCODE = 7,		// Reads the color from the next six characters
	COLOR_HEXCODE_ALPHA = 8,// Reads the color and alpha from the next eight characters
	COLOR_MAX
};

//--------------------------------------------------------------------------------------------------------------
struct TextRange
{
	TextRange() { preserveAlpha = false; }
	int start;
	int end;
	Color color;
	bool preserveAlpha;
};

void StripEndNewlineFromString( char *str );
void StripEndNewlineFromString( wchar_t *str );

char* ConvertCRtoNL( char *str );
wchar_t* ConvertCRtoNL( wchar_t *str );
wchar_t* ReadLocalizedString( bf_read &msg, OUT_Z_BYTECAP(outSizeInBytes) wchar_t *pOut, int outSizeInBytes, bool bStripNewline, OUT_Z_CAP(originalSize) char *originalString = NULL, int originalSize = 0 );
wchar_t* ReadChatTextString( bf_read &msg, OUT_Z_BYTECAP(outSizeInBytes) wchar_t *pOut, int outSizeInBytes );
char* RemoveColorMarkup( char *str );

//--------------------------------------------------------------------------------------------------------
/**
 * Simple utility function to allocate memory and duplicate a wide string
 */
inline wchar_t *CloneWString( const wchar_t *str )
{
	const int nLen = V_wcslen(str)+1;
	wchar_t *cloneStr = new wchar_t [ nLen ];
	const int nSize = nLen * sizeof( wchar_t );
	V_wcsncpy( cloneStr, str, nSize );
	return cloneStr;
}

//-----------------------------------------------------------------------------
// Purpose: An output/display line of the chat interface
//-----------------------------------------------------------------------------
class CBaseHudChatLine : public vgui::RichText
{
	typedef vgui::RichText BaseClass;

public:
	CBaseHudChatLine( vgui::Panel *parent, const char *panelName );
	~CBaseHudChatLine();

	void			SetExpireTime( void );

	bool			IsReadyToExpire( void );

	void			Expire( void );

	float			GetStartTime( void );

	int				GetCount( void );

	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);

	vgui::HFont		GetFont() { return m_hFont; }

	Color			GetTextColor( void ) { return m_clrText; }
	void			SetNameLength( int iLength ) { m_iNameLength = iLength;	}
	void			SetNameColor( Color cColor ){ m_clrNameColor = cColor; 	}
		
	virtual void	PerformFadeout( void );
	virtual void	InsertAndColorizeText( wchar_t *buf, int clientIndex );
	virtual			void Colorize( int alpha = 255 );								///< Re-inserts the text in the appropriate colors at the given alpha


	void			SetNameStart( int iStart ) { m_iNameStart = iStart;	}

protected:
	int				m_iNameLength;
	vgui::HFont		m_hFont;

	Color			m_clrText;
	Color			m_clrNameColor;

	float			m_flExpireTime;

	CUtlVector< TextRange > m_textRanges;
	wchar_t					*m_text;

	int				m_iNameStart;
	
private:
	float			m_flStartTime;
	int				m_nCount;

	vgui::HFont		m_hFontMarlett;


private:
	CBaseHudChatLine( const CBaseHudChatLine & ); // not defined, not accessible
};


class CHudChatHistory : public vgui::RichText
{
	DECLARE_CLASS_SIMPLE( CHudChatHistory, vgui::RichText );
public:

	CHudChatHistory( vgui::Panel *pParent, const char *panelName );

	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);
};

class CHudChatFilterButton : public vgui::Button
{
	DECLARE_CLASS_SIMPLE( CHudChatFilterButton, vgui::Button );

public:

	CHudChatFilterButton(  vgui::Panel *pParent, const char *pName, const char *pText );

	virtual void DoClick( void );
};

class CHudChatFilterCheckButton : public vgui::CheckButton
{
	DECLARE_CLASS_SIMPLE( CHudChatFilterCheckButton, vgui::CheckButton );

public:

	CHudChatFilterCheckButton( vgui::Panel *pParent, const char *pName, const char *pText, int iFlag );

	int		GetFilterFlag( void ) { return m_iFlag; }

private:

	int m_iFlag;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBaseHudChat : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CBaseHudChat, vgui::EditablePanel );
public:
	DECLARE_MULTIPLY_INHERITED();

	enum
	{
		CHAT_INTERFACE_LINES = 6,
		MAX_CHARS_PER_LINE = 128
	};

	CBaseHudChat( const char *pElementName );

	virtual void	CreateChatInputLine( void );
	virtual void	CreateChatLines( void );
	
	virtual void	Init( void );

	void			LevelInit( const char *newmap );
	void			LevelShutdown( void );

	void			MsgFunc_TextMsg(const char *pszName, int iSize, void *pbuf);
	
	virtual void	Printf( int iFilter, PRINTF_FORMAT_STRING const char *fmt, ... );
	virtual void	ChatPrintf( int iPlayerIndex, int iFilter, PRINTF_FORMAT_STRING const char *fmt, ... ) FMTFUNCTION( 4, 5 );
	
	virtual void	StartMessageMode( int iMessageModeType );
	virtual void	StopMessageMode( void );
	void			Send( void );

	MESSAGE_FUNC( OnChatEntrySend, "ChatEntrySend" );
	MESSAGE_FUNC( OnChatEntryStopMessageMode, "ChatEntryStopMessageMode" );

	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void	Paint( void );
	virtual void	OnTick( void );
	virtual void	Reset();
#ifdef _XBOX
	virtual bool	ShouldDraw();
#endif
	vgui::Panel		*GetInputPanel( void );

	static int		m_nLineCounter;

	virtual int		GetChatInputOffset( void );

	// IGameEventListener interface:
	virtual void FireGameEvent( IGameEvent *event);

	CHudChatHistory			*GetChatHistory();

	void					FadeChatHistory();
	float					m_flHistoryFadeTime;
	float					m_flHistoryIdleTime;

	virtual void			MsgFunc_SayText( bf_read &msg );
	virtual void			MsgFunc_SayText2( bf_read &msg );
	virtual void			MsgFunc_TextMsg( bf_read &msg );
	virtual void			MsgFunc_VoiceSubtitle( bf_read &msg );

	
	CBaseHudChatInputLine	*GetChatInput( void ) { return m_pChatInput; }
	CHudChatFilterPanel		*GetChatFilterPanel( void );

	virtual int				GetFilterFlags( void ) { return m_iFilterFlags; }
	void					SetFilterFlag( int iFilter );

	//-----------------------------------------------------------------------------
	virtual Color	GetDefaultTextColor( void );
	virtual Color	GetTextColorForClient( TextColor colorNum, int clientIndex );
	virtual Color	GetClientColor( int clientIndex );

	virtual int		GetFilterForString( const char *pString );

	virtual const char *GetDisplayedSubtitlePlayerName( int clientIndex );

	bool			IsVoiceSubtitle( void ) { return m_bEnteringVoice; }
	void			SetVoiceSubtitleState( bool bState ) { m_bEnteringVoice = bState; }
	int				GetMessageMode( void ) { return m_nMessageMode; }

	void			SetCustomColor( Color colNew ) { m_ColorCustom = colNew; }
	void			SetCustomColor( const char *pszColorName );

protected:
	CBaseHudChatLine		*FindUnusedChatLine( void );

	CBaseHudChatInputLine	*m_pChatInput;
	CBaseHudChatLine		*m_ChatLine;
	int					m_iFontHeight;

	CHudChatHistory			*m_pChatHistory;

	CHudChatFilterButton	*m_pFiltersButton;
	CHudChatFilterPanel		*m_pFilterPanel;

	Color			m_ColorCustom;

private:	
	void			Clear( void );

	int				ComputeBreakChar( int width, const char *text, int textlen );

	int				m_nMessageMode;

	int				m_nVisibleHeight;

	vgui::HFont		m_hChatFont;

	int				m_iFilterFlags;
	bool			m_bEnteringVoice;

};

class CBaseHudChatEntry : public vgui::TextEntry
{
	typedef vgui::TextEntry BaseClass;
public:
	CBaseHudChatEntry( vgui::Panel *parent, char const *panelName, vgui::Panel *pChat )
		: BaseClass( parent, panelName )
	{
		SetProportional( true );
		SetCatchEnterKey( true );
		SetAllowNonAsciiCharacters( true );
		SetDrawLanguageIDAtLeft( true );
		m_pHudChat = pChat;
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings(pScheme);

		SetPaintBorderEnabled( false );
	}

	virtual void OnKeyCodeTyped(vgui::KeyCode code)
	{
		if ( code == KEY_ENTER || code == KEY_PAD_ENTER || code == KEY_ESCAPE )
		{
			if ( code != KEY_ESCAPE )
			{
				if ( m_pHudChat )
				{
					PostMessage( m_pHudChat, new KeyValues("ChatEntrySend") );
				}
			}
		
			// End message mode.
			if ( m_pHudChat )
			{
				PostMessage( m_pHudChat, new KeyValues("ChatEntryStopMessageMode") );
			}
		}
		else if ( code == KEY_TAB )
		{
			// Ignore tab, otherwise vgui will screw up the focus.
			return;
		}
		else
		{
			BaseClass::OnKeyCodeTyped( code );
		}
	}

private:
	vgui::Panel *m_pHudChat;
};

//-----------------------------------------------------------------------------
// Purpose: The prompt and text entry area for chat messages
//-----------------------------------------------------------------------------
class CBaseHudChatInputLine : public vgui::Panel
{
	typedef vgui::Panel BaseClass;
	
public:
	CBaseHudChatInputLine( vgui::Panel *parent, char const *panelName );

	void			SetPrompt( const wchar_t *prompt );
	void			ClearEntry( void );
	void			SetEntry( const wchar_t *entry );
	void			GetMessageText( OUT_Z_BYTECAP(buffersizebytes) wchar_t *buffer, int buffersizebytes );

	virtual void	PerformLayout();
	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);

	vgui::Panel		*GetInputPanel( void );
	virtual vgui::VPANEL GetCurrentKeyFocus() { return m_pInput->GetVPanel(); } 

	virtual void Paint()
	{
		BaseClass::Paint();
	}

	vgui::Label		*GetPrompt( void ) { return m_pPrompt; }

protected:
	vgui::Label		*m_pPrompt;
	CBaseHudChatEntry	*m_pInput;
};


class CHudChatFilterPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudChatFilterPanel, vgui::EditablePanel );

public:

	CHudChatFilterPanel(  vgui::Panel *pParent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	MESSAGE_FUNC_PTR( OnFilterButtonChecked, "CheckButtonChecked", panel );

	CBaseHudChat *GetChatParent( void ) { return dynamic_cast < CBaseHudChat * > ( GetParent() ); }

	virtual void SetVisible(bool state);

private:

};

#endif // HUD_BASECHAT_H
