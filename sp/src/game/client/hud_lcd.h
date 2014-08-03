//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CLCD Manages the Logitech G-Series Gaming Keyboard LCD
//
// $NoKeywords: $
//=============================================================================//
#ifndef HUD_LCD_H
#define HUD_LCD_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlvector.h"
#include "tier1/utlstring.h"
#include "tier1/utldict.h"

#include "ihudlcd.h"

class KeyValues;
class IG15;
class C_BasePlayer;

enum
{
	LCDITEM_UNKNOWN = 0,
	LCDITEM_PAGE,
	LCDITEM_TEXT,
	LCDITEM_ICON,
	LCDITEM_AGGREGATE, // Made up of subitems
};

// Aggregate item types
enum
{
	AGGTYPE_UNKNOWN = 0,
	AGGTYPE_PERPLAYER,
	AGGTYPE_PERTEAM,
};

class CLCDItem
{
public:
	CLCDItem() : 
		m_bActive( true ),
		m_nSubPage( 0 ),
		m_Type( LCDITEM_UNKNOWN ),
		m_Handle( 0 ),
		x( 0 ),
		y( 0 ),
		w( 0 ),
		h( 0 )
	{
	}

	virtual ~CLCDItem() {}

	virtual void Create( IG15 *lcd ) = 0;
	virtual void Wipe( IG15 *lcd );

	bool		m_bActive;
	int			m_Type;
	void		*m_Handle;
	int			x, y, w, h;

	int			m_nSubPage;

	CUtlVector< CLCDItem * >	m_Children;
};

class CLCDItemText : public CLCDItem
{
	typedef CLCDItem BaseClass;
public:
	CLCDItemText() :
		m_bHasWildcard( false ),
		m_iSize( 0 ),
		m_iAlign( 0 )
	{
		m_Type = LCDITEM_TEXT;
	}

	virtual void Create( IG15 *lcd );

	CUtlString	m_OriginalText;
	bool		m_bHasWildcard;
	int			m_iSize;
	int			m_iAlign;
};

class CLCDItemIcon : public CLCDItem
{
	typedef CLCDItem BaseClass;

public:
	CLCDItemIcon() : 
	  m_icon( NULL )
	{
		m_Type = LCDITEM_ICON;
	}

	virtual void Create( IG15 *lcd );

	CUtlString	m_IconName;
	void		*m_icon;
};

class CLCDItemAggregate : public CLCDItem
{
	typedef CLCDItem BaseClass;

public:
	CLCDItemAggregate() : 
		m_AggType( AGGTYPE_UNKNOWN ),
		m_dwNextUpdateTime( 0 ),
		m_yincrement( 0 )
	{
		m_Type = LCDITEM_AGGREGATE;
	}

	virtual void Create( IG15 *lcd );
	virtual void Wipe( IG15 *lcd );

	void WipeChildrenOnly( IG15 *lcd );

	unsigned int				m_dwNextUpdateTime;
	int							m_AggType;
	
	int							m_yincrement;

	// Representative row
	CUtlVector< CLCDItem * >	m_Definition;
};

class CLCDPage : public CLCDItem
{
public:
	CLCDPage() :
	  m_bSubItem( false ),
	  m_bTitlePage( false ),
	  m_bRequiresPlayer( false ),
	  m_nSubPageCount( 1 )
	{
		m_Type = LCDITEM_PAGE;
	}

	~CLCDPage()
	{
	}

	virtual void Create( IG15 *lcd )
	{
	}

	CLCDItem *Alloc( int type )
	{
		CLCDItem *item = NULL;

		switch ( type )
		{
		default:
			break;
		case LCDITEM_PAGE:
			// This shouldn't occur
			break;
		case LCDITEM_TEXT:
			item = new CLCDItemText();
			break;
		case LCDITEM_ICON:
			item = new CLCDItemIcon();
			break;
		case LCDITEM_AGGREGATE:
			item = new CLCDItemAggregate();
			break;
		}

		if ( item )
		{
			return item;
		}

		Assert( 0 );
		return NULL;
	}

	void InitFromKeyValues( KeyValues *kv );

	bool						m_bSubItem;
	bool						m_bTitlePage;
	bool						m_bRequiresPlayer;
	int							m_nSubPageCount;
};

//-----------------------------------------------------------------------------
// Purpose: Manages the Logitech G-Series Gaming Keyboard LCD
//-----------------------------------------------------------------------------
class CLCD : public IHudLCD
{
public:
						CLCD();
						~CLCD();

	// Implement IHudLCD
	virtual void	SetGlobalStat( char const *name, char const *value );
	virtual void	AddChatLine( char const *txt );

	// Exposed as a ConCommand
	void				Reload();
	void				DumpPlayer();

public:

	// Init's called when the HUD's created at DLL load
	void				Init( void );	
	void				Shutdown();
	void				Update( void );
	bool				IsConnected() const;

private:

	CLCDItemIcon		*ParseItemIcon( CLCDPage *page, bool bCreateHandles, KeyValues *sub );
	CLCDItemText		*ParseItemText( CLCDPage *page, bool bCreateHandles, KeyValues *sub );
	void				ParseItems_R( CLCDPage *page, bool bCreateHandles, KeyValues *kv, CUtlVector< CLCDItem * >& list );

	void				ParsePage( KeyValues *kv );
	void				ParseIconMappings( KeyValues *kv );
	void				ParseReplacements( KeyValues *kv );
	void				DisplayCurrentPage( unsigned int dwCurTime );

	void				ShowItems_R( CLCDPage *page, unsigned int dwCurTime, CUtlVector< CLCDItem * >& list, bool show );

	int					FindTitlePage();
	void				BuildUpdatedText( char const *in, CUtlString& out );
	void				LookupToken( char const *token, CUtlString& value );
	bool				ExtractArrayIndex( char *str, size_t bufsize, int *index );

	bool				Replace( CUtlString& str, char const *search, char const *replace );
	void				DoGlobalReplacements( CUtlString& str );
	void				ReduceParentheses( CUtlString& str );

	bool				IsPageValid( int currentPage, C_BasePlayer *player );
	void				UpdateChat();

	IG15					*m_lcd ;

	CUtlString				m_Title;
	int						m_Size[ 2 ];
	CUtlVector< CLCDPage * >	m_Pages;
	int						m_nCurrentPage;
	int						m_nSubPage;
	int						m_nMaxChatHistory;

	CUtlDict< int, int >	m_TextSizes;
	CUtlDict< int, int >	m_TextAlignments;
	
	struct IconInfo_t
	{
		void	*m_handle;
	};

	CUtlDict< IconInfo_t, int >	m_Icons;
	bool					m_bHadPlayer;

	CUtlDict< CUtlString, int >		m_GlobalStats;
	CUtlVector< CUtlString >		m_ChatHistory;

	unsigned int			m_dwNextUpdateTime;
	CSysModule				*m_pG15Module;
	CreateInterfaceFn		m_G15Factory;
};

extern CLCD gLCD;

#endif // HUD_LCD_H
