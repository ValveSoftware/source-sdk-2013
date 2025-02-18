//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: LCD support
//
//=====================================================================================//

#if defined( WIN32 ) && !defined( _X360 )
#include <windows.h>
#endif

#include "cbase.h"

#ifdef POSIX
#define HICON int
const int DT_LEFT = 1;
const int DT_CENTER = 2;
const int DT_RIGHT = 3;
#endif

#include "hud_lcd.h"

#include "vgui_controls/Controls.h"
#include "vgui_controls/SectionedListPanel.h"
#include "vgui/ISurface.h"
#include "vgui/IScheme.h"
#include "vgui/IVGui.h"
#include "vgui/ILocalize.h"
#include "vgui/IPanel.h"

#include "c_team.h"
#include "c_playerresource.h"
#include "filesystem.h"
#include "g15/ig15.h"

#include "tier0/icommandline.h"

#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define G15_RESOURCE_FILE "resource/g15.res"
#define G15_MODULE_NAME "bin/g15.dll"

#define SMALL_ITEM_HEIGHT	10
#define G15_DEFAULT_MAX_CHAT_HISTORY 4

CLCD gLCD;
IHudLCD *hudlcd = &gLCD;

CON_COMMAND( g15_reload, "Reloads the Logitech G-15 Keyboard configs." )
{
	if ( !CommandLine()->FindParm( "-g15" ) )
	{
		Msg( "Must run with -g15 to enable support for the LCD Keyboard\n" );
		return;
	}

	gLCD.Reload();
}

CON_COMMAND( g15_dumpplayer, "Spew player data." )
{
	if ( !CommandLine()->FindParm( "-g15" ) )
	{
		Msg( "Must run with -g15 to enable support for the LCD Keyboard\n" );
		return;
	}

	gLCD.DumpPlayer();
}

static ConVar g15_update_msec( "g15_update_msec", "250", FCVAR_ARCHIVE, "Logitech G-15 Keyboard update interval." );

void CLCDItem::Wipe( IG15 *lcd )
{
	for ( int i = 0; i < m_Children.Count(); ++i )
	{
		if ( m_Children[ i ]->m_Handle )
		{
			lcd->RemoveAndDestroyObject( m_Children[ i ]->m_Handle );
		}

		m_Children[ i ]->Wipe( lcd );

		delete m_Children[ i ];
	}

	m_Children.Purge();

}
void CLCDItemAggregate::Create( IG15 *lcd )
{
	// Nothing
}

void CLCDItemAggregate::Wipe( IG15 *lcd )
{
	BaseClass::Wipe( lcd );

	for ( int i = 0; i < m_Definition.Count(); ++i )
	{
		m_Definition[ i ]->Wipe( lcd );

		delete m_Definition[ i ];
	}

	m_Definition.Purge();
}

void CLCDItemAggregate::WipeChildrenOnly( IG15 *lcd )
{
	BaseClass::Wipe( lcd );
}

void CLCDItemIcon::Create( IG15 *lcd )
{	
#ifdef WIN32
	m_Handle = lcd->AddIcon( (HICON)m_icon, w, h );
#else
	m_Handle = lcd->AddIcon( (void *)m_icon, w, h );
#endif
	lcd->SetOrigin( m_Handle, x, y );
	lcd->SetVisible( m_Handle, false );
}

void CLCDItemText::Create( IG15 *lcd )
{	
	m_Handle = lcd->AddText( G15_STATIC_TEXT, (G15TextSize)m_iSize, m_iAlign, w );
	lcd->SetOrigin( m_Handle, x, y );
	lcd->SetText( m_Handle, m_OriginalText );
	lcd->SetVisible( m_Handle, false );
}


///-----------------------------------------------------------------------------
/// Constructor
///-----------------------------------------------------------------------------
CLCD::CLCD( void )
	:	m_lcd( NULL ),
	m_nCurrentPage( 0 ),
	m_nSubPage( 0 ),
	m_bHadPlayer( false ),
	m_dwNextUpdateTime( 0u ),
	m_nMaxChatHistory( G15_DEFAULT_MAX_CHAT_HISTORY ),
	m_pG15Module( 0 ),
	m_G15Factory( 0 )
{
	m_Size[ 0 ] = m_Size[ 1 ] = 0;
}

///-----------------------------------------------------------------------------
/// Destructor
///-----------------------------------------------------------------------------
CLCD::~CLCD( void )
{
}

void CLCD::Reload()
{
	Shutdown();

	Msg( "Reloading G15 config\n" );

	Init();
}

///------------------------------------------------------------------------------
/// Initializes the LCD device, and sets up the text handles
///------------------------------------------------------------------------------
void CLCD::Init( void )
{
	if ( !CommandLine()->FindParm( "-g15" ) )
		return;

	if ( m_lcd ) 
		return;

	m_pG15Module = Sys_LoadModule( G15_MODULE_NAME );
	if ( !m_pG15Module )
	{
		return;
	}

    m_G15Factory = Sys_GetFactory( m_pG15Module );
	if ( !m_G15Factory )
	{
		Shutdown();
		return;
	}

	m_lcd = reinterpret_cast< IG15 * >( m_G15Factory( G15_INTERFACE_VERSION, NULL ) );
	if ( !m_lcd )
	{
		Shutdown();
		return;
	}
	
	m_lcd->GetLCDSize( m_Size[ 0 ], m_Size[ 1 ] );
	
	m_nCurrentPage = 0;
	m_nSubPage = 0;

	m_TextSizes.Insert( "small", G15_SMALL );
	m_TextSizes.Insert( "medium", G15_MEDIUM );
	m_TextSizes.Insert( "big", G15_BIG );

	m_TextAlignments.Insert( "left", DT_LEFT );
	m_TextAlignments.Insert( "center", DT_CENTER );
	m_TextAlignments.Insert( "right", DT_RIGHT );

	KeyValues *kv = new KeyValues( "G15" );
	if ( kv->LoadFromFile( filesystem, G15_RESOURCE_FILE, "MOD" ) )
	{
		char const *title = kv->GetString( "game", "Source Engine" );
		m_nMaxChatHistory = clamp( 1, kv->GetInt( "chatlines", m_nMaxChatHistory ), 64 );
		Assert( title );
		m_Title = title;
		m_lcd->Init( m_Title.String() );

		for ( KeyValues *sub = kv->GetFirstSubKey(); sub; sub = sub->GetNextKey() )
		{
			char const *keyName = sub->GetName();
			if ( !Q_stricmp( keyName, "game" ) )
			{
				// Handled above!!!
			}
			else if ( !Q_stricmp( keyName, "icons" ) )
			{
				ParseIconMappings( sub );
			}
			else if ( !Q_stricmp( keyName, "replace" ) )
			{
				ParseReplacements( sub );
			}
			else if ( !Q_stricmp( keyName, "page" ) )
			{
				ParsePage( sub );
			}
		}
	}
	kv->deleteThis();

	UpdateChat();

	Msg( "Logitech LCD Keyboard initialized\n" );
}

///--------------------------------------------------------------------------
/// Destroys the local EZ LCD Object
///--------------------------------------------------------------------------
void CLCD::Shutdown( void )
{
	for ( int i = 0; i < m_Pages.Count(); ++i )
	{
		CLCDPage *page = m_Pages[ i ];
		page->Wipe( m_lcd );
		delete page;
	}

	m_Pages.Purge();

	if ( m_lcd )
	{
		m_lcd->Shutdown();
		m_lcd = NULL;
	}

	m_TextSizes.Purge();
	m_TextAlignments.Purge();
	m_GlobalStats.Purge();

	m_G15Factory = 0;
	
	if ( m_pG15Module )
	{
		Sys_UnloadModule( m_pG15Module );
		m_pG15Module = 0;
	}
}

int CLCD::FindTitlePage()
{
	for ( int i = 0; i < m_Pages.Count(); ++i )
	{
		if ( m_Pages[ i ]->m_bTitlePage )
			return i;
	}

	return -1;
}

bool CLCD::IsPageValid( int currentPage, C_BasePlayer *player )
{
	if ( m_Pages[ currentPage ]->m_bTitlePage && player )
		return false;

	if ( m_Pages[ currentPage ]->m_bRequiresPlayer && !player )
		return false;

	return true;
}

///---------------------------------------------------------------------
/// Update routine
///---------------------------------------------------------------------
void CLCD::Update( void )
{
	if ( !m_lcd ) 
		return ;

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	bool hasplayer = player ? true : false;

	bool changed = hasplayer != m_bHadPlayer;
	m_bHadPlayer = hasplayer;

	int pageCount = m_Pages.Count();

	int prevPage = m_nCurrentPage;

	if ( pageCount > 0 )
	{
		bool force = false;
		if ( changed && hasplayer )
		{
			force = true;
			m_nCurrentPage = 0;
			m_nSubPage = 0;
		}

		if ( !IsPageValid( m_nCurrentPage, player ) )
		{
			force = true;
		}
		
		if ( m_lcd->ButtonTriggered( G15_BUTTON_1 ) || force )
		{
			m_nSubPage = 0;

			for ( int i = 0; i < pageCount; ++i )
			{
                m_nCurrentPage = ( m_nCurrentPage + 1 ) % pageCount;
				if ( !IsPageValid( m_nCurrentPage, player ) )
					continue;



				break;
			}
		}

		if ( m_lcd->ButtonTriggered( G15_BUTTON_2 ) )
		{
			int pc = m_Pages[ m_nCurrentPage ]->m_nSubPageCount;

			m_nSubPage = ( m_nSubPage + 1 ) % pc;
		}
	}
	else
	{
		m_nCurrentPage = -1;
		m_nSubPage = 0;
	}

	bool pageChanged = prevPage != m_nCurrentPage;

	unsigned int dwCurTime = (unsigned int)( 1000.0 * gpGlobals->realtime );

	if ( m_lcd->IsConnected() )
	{
		if ( dwCurTime >= m_dwNextUpdateTime || pageChanged )
		{
			m_dwNextUpdateTime = dwCurTime + g15_update_msec.GetInt();
            DisplayCurrentPage( dwCurTime );
		}
	}

	m_lcd->UpdateLCD( dwCurTime );
}

///--------------------------------------------------------------------------
///
///--------------------------------------------------------------------------
bool CLCD::IsConnected( void ) const
{
	return m_lcd ? m_lcd->IsConnected() : false;
}

void CLCD::ShowItems_R( CLCDPage *page, unsigned int dwCurTime, CUtlVector< CLCDItem * >& list, bool bShowItems )
{
	int itemCount = list.Count();
	for ( int j = 0; j < itemCount; ++j )
	{
		CLCDItem *item = list[ j ];
		if ( !item->m_bActive )
			continue;

		if ( bShowItems )
		{
			switch ( item->m_Type )
			{
			default:
				break;
			case LCDITEM_TEXT:
				{
					CLCDItemText *txt = static_cast< CLCDItemText * >( item );
					if ( txt )
					{
						// Need to build updated text
						CUtlString updated;
						CUtlString str = txt->m_OriginalText;
						BuildUpdatedText( str.String(), updated );
						DoGlobalReplacements( updated );
						ReduceParentheses( updated );

						m_lcd->SetText( item->m_Handle, updated.String() );
					}
				}
				break;
			case LCDITEM_ICON:
				{
					CLCDItemIcon *icon = static_cast< CLCDItemIcon * >( item );
					if ( icon )
					{
						// Need to build updated text
						CUtlString updated;
						CUtlString str = icon->m_IconName;
						BuildUpdatedText( str.String(), updated );
						DoGlobalReplacements( updated );
						ReduceParentheses( updated );

						int idx = m_Icons.Find( updated.String() );
						if ( idx != m_Icons.InvalidIndex() )
						{
							icon->m_icon = (void *)m_Icons[ idx ].m_handle;
						}

						// Recreate
						if ( icon->m_Handle )
						{
							m_lcd->RemoveAndDestroyObject( icon->m_Handle );
							icon->m_Handle = 0;
						}
						icon->Create( m_lcd );
					}
				}
				break;
			case LCDITEM_AGGREGATE:
				{
					CLCDItemAggregate *ag = static_cast< CLCDItemAggregate * >( item );
					if ( ag->m_dwNextUpdateTime > dwCurTime )
						break;

					// FIXME: encode update interval in text file
					ag->m_dwNextUpdateTime = dwCurTime + 1000;

					// Blow away current data
					ag->WipeChildrenOnly( m_lcd );
									
					CUtlVector< int >	validIndices;
					char prefix[ 256 ];
					char altprefix[ 256 ];
					prefix[ 0 ] = 0;
					altprefix[ 0 ] = 0;

					int curx = ag->x;
					int cury = ag->y;

					switch ( ag->m_AggType )
					{
					default:
						Assert( 0 );
						break;
					case AGGTYPE_PERPLAYER:
						// Add all players into list
						{
							for ( int pl = 1; pl <= gpGlobals->maxClients; ++pl )
							{
								if ( g_PR && g_PR->IsConnected( pl ) )
								{
									validIndices.AddToTail( pl );
								}
							}

							Q_strncpy( prefix, "(playerindex)", sizeof( prefix ) );
							Q_strncpy( altprefix, "(playerindexplusone)", sizeof( altprefix ) );
						}
						break;
					case AGGTYPE_PERTEAM:
						{
							C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();

							if ( local )
							{
								for ( int pl = 1; pl <= gpGlobals->maxClients; ++pl )
								{
									if ( g_PR && g_PR->IsConnected( pl ) && local->GetTeamNumber() == g_PR->GetTeam( pl ) )
									{
										validIndices.AddToTail( pl );
									}
								}
							}

							Q_strncpy( prefix, "(playerindex)", sizeof( prefix ) );
							Q_strncpy( altprefix, "(playerindexplusone)", sizeof( altprefix ) );
						}
						break;
					}

					int subPage = 0;
					int spItems = 0;

					int ecount = validIndices.Count();
					for ( int e = 0; e < ecount; ++e )
					{
						// Now fixup any strings
						int index = validIndices[ e ];

						char s1[ 512 ], s2[ 512 ];
						Q_snprintf( s1, sizeof( s1 ), "%d", index );
						Q_snprintf( s2, sizeof( s2 ), "%d", index + 1 );
						
						// Now replace "playerindex" with the index as needed
						for( int r = 0; r < ag->m_Definition.Count(); ++r )
						{
							CLCDItem *newItem = NULL;

							CLCDItem *itemDefn = ag->m_Definition[ r ];
							switch ( itemDefn->m_Type )
							{
							default:
								break;

							case LCDITEM_TEXT:
								{
									CLCDItemText *text = static_cast< CLCDItemText * >(itemDefn);
									CUtlString s;
									s = text->m_OriginalText;
									Replace( s, prefix, s1 );
									Replace( s, altprefix, s2 );
									char itemNumber[ 32 ];
									Q_snprintf( itemNumber, sizeof( itemNumber ), "%d", e +1 );

									Replace( s, "(itemnumber)", itemNumber );

									DoGlobalReplacements( s );
									// ReduceParentheses( s );

									// text->m_OriginalText = s;

									CLCDItemText *copy = static_cast< CLCDItemText * >( page->Alloc( itemDefn->m_Type ) );
									*copy = *text;
									copy->m_bActive = true;
									copy->m_OriginalText = s;
									copy->Create( m_lcd );

									m_lcd->SetOrigin( copy->m_Handle, curx + copy->x, cury + copy->y );

									newItem = copy;
								}
								break;
							case LCDITEM_ICON:
								{
									CLCDItemIcon *icon = static_cast< CLCDItemIcon * >(itemDefn);
									CLCDItemIcon *copy = static_cast< CLCDItemIcon * >( page->Alloc( itemDefn->m_Type ) );
									*copy = *icon;
									copy->m_bActive = true;
									copy->Create( m_lcd );

									m_lcd->SetOrigin( copy->m_Handle, curx + copy->x, cury + copy->y );

									newItem = copy;
								}
								break;
							}

							if ( newItem )
							{
								++spItems;
								newItem->m_nSubPage = subPage;
								ag->m_Children.AddToTail( newItem );
							}
						}

						cury += ag->m_yincrement;

						if ( cury + SMALL_ITEM_HEIGHT > m_Size[ 1 ] )
						{
							spItems = 0;
							++subPage;
							cury = ag->y;
						}
					}

					if ( spItems > 0 )
					{
						page->m_nSubPageCount = subPage + 1;
					}
					else
					{
						// We thought we needed a new page, but didn't actually use it
						page->m_nSubPageCount = subPage;
					}
				}
			}
		}

		m_lcd->SetVisible( item->m_Handle, bShowItems && ( ( item->m_nSubPage == -1 ) || item->m_nSubPage == m_nSubPage ) );
		
		ShowItems_R( page, dwCurTime, item->m_Children, bShowItems );
	}
}

void CLCD::DisplayCurrentPage( unsigned int dwCurTime )
{
	int pageCount = m_Pages.Count();
	for ( int i = 0; i < pageCount; ++i )
	{
		bool bShowItems = ( i == m_nCurrentPage ) ? true : false;

		CLCDPage* page = m_Pages[ i ];
		ShowItems_R( page, dwCurTime, page->m_Children, bShowItems );
	}
}

CLCDItemIcon *CLCD::ParseItemIcon( CLCDPage *page, bool bCreateHandles, KeyValues *sub )
{
	CLCDItemIcon *item = static_cast< CLCDItemIcon * >( page->Alloc( LCDITEM_ICON ) );

	item->m_IconName = sub->GetString( "name", "" );

	item->m_nSubPage = sub->GetInt( "header", 0 ) ? -1 : page->m_nSubPageCount - 1;
	item->w = sub->GetInt( "w", 24 );
	item->h = sub->GetInt( "h", 24 );
	item->x = sub->GetInt( "x", 0 );
	item->y = sub->GetInt( "y", 0 );

	int idx = m_Icons.Find( item->m_IconName.String() );
	item->m_icon = 0;
	if ( idx != m_Icons.InvalidIndex() )
	{
		item->m_icon = (void *)m_Icons[ idx ].m_handle;
	}

	if ( bCreateHandles )
	{
		item->Create( m_lcd );
	}

	return item;
}

CLCDItemText *CLCD::ParseItemText( CLCDPage *page, bool bCreateHandles, KeyValues *sub )
{
	CLCDItemText *item = static_cast< CLCDItemText * >( page->Alloc( LCDITEM_TEXT ) );

	const char *initialText = sub->GetString( "text", "" );
	item->m_bHasWildcard = Q_strstr( initialText, "%" ) ? true : false;

	item->m_nSubPage = sub->GetInt( "header", 0 ) ? -1 : page->m_nSubPageCount - 1;
	item->w = sub->GetInt( "w", 150 );
	item->x = sub->GetInt( "x", 0 );
	item->y = sub->GetInt( "y", 0 );

	const char *sizeStr = sub->GetString( "size", "small" );
	item->m_iSize = G15_SMALL;
	int iFound = m_TextSizes.Find( sizeStr );
	if ( iFound != m_TextSizes.InvalidIndex() )
	{
		item->m_iSize = m_TextSizes[ iFound ];
	}

	const char *alignStr = sub->GetString( "align", "left" );
	item->m_iAlign = DT_LEFT;
	iFound = m_TextAlignments.Find( alignStr );
	if ( iFound != m_TextAlignments.InvalidIndex() )
	{
		item->m_iAlign = m_TextAlignments[ iFound ];
	}

	item->m_OriginalText = initialText;
	if ( bCreateHandles )
	{
		item->Create( m_lcd );
	}

	return item;
}

void CLCD::ParseItems_R( CLCDPage *page, bool bCreateHandles, KeyValues *kv, CUtlVector< CLCDItem * >& list )
{
	for ( KeyValues *sub = kv->GetFirstSubKey(); sub; sub = sub->GetNextKey() )
	{
		char const *keyName = sub->GetName();
		if ( !Q_stricmp( keyName, "iterate_players" ) ||
			      !Q_stricmp( keyName, "iterate_team" ) )
		{
			int aggType = AGGTYPE_UNKNOWN;
			if ( !Q_stricmp( keyName, "iterate_players" ) )
			{
				aggType = AGGTYPE_PERPLAYER;
			}
			else if ( !Q_stricmp( keyName, "iterate_team" ) )
			{
				aggType = AGGTYPE_PERTEAM;
			}

			// Now we parse the items out as we generally would
			CLCDItemAggregate *item = static_cast< CLCDItemAggregate * >( page->Alloc( LCDITEM_AGGREGATE ) );
			item->m_AggType = aggType;

			item->x = sub->GetInt( "x", 0 );
			item->y = sub->GetInt( "y", 0 );
			item->m_yincrement = sub->GetInt( "y_increment", 10 );

			// Parse the definition
			ParseItems_R( page, false, sub, item->m_Definition );

			// Add the definition items as "inactive" items to the scene (so they get destroyed)
			for ( int i = 0; i < item->m_Definition.Count(); ++i )
			{
				CLCDItem *pItem = item->m_Definition[ i ];
				pItem->m_bActive = false;
			}

			list.AddToTail( item );
		}
		else if ( !Q_stricmp( keyName, "static_icon" ) )
		{
			CLCDItemIcon *item = ParseItemIcon( page, true, sub );
			Assert( item );
			list.AddToTail(item );
		}
		else if ( !Q_stricmp( keyName, "static_text" ) )
		{
			CLCDItemText *item = ParseItemText( page, true, sub );
			Assert( item );
			list.AddToTail( item );
		}
		else if ( !Q_stricmp( keyName, "newsubpage" ) )
		{
			// Add to new subpage
			++page->m_nSubPageCount;
		}
		else
		{
			// Skip unknown stuff
			continue;
		}
	}
}

void CLCD::ParsePage( KeyValues *kv )
{
	CLCDPage *newPage = new CLCDPage();
	m_Pages.AddToTail( newPage );

	newPage->m_bTitlePage = kv->GetInt( "titlepage", 0 ) ? true : false;
	newPage->m_bRequiresPlayer = kv->GetInt( "requiresplayer", 0 ) ? true : false;

	ParseItems_R( newPage, true, kv, newPage->m_Children );
}

void CLCD::ParseIconMappings( KeyValues *kv )
{
	for ( KeyValues *icon = kv->GetFirstSubKey(); icon; icon = icon->GetNextKey() )
	{
		IconInfo_t info;
		HICON hIcon = 0;
		char const *name = icon->GetName();
		char fullpath[ 512 ];
		filesystem->RelativePathToFullPath( icon->GetString(), "GAME", fullpath, sizeof( fullpath ) );
#ifdef WIN32
		hIcon = (HICON)::LoadImageA( NULL, fullpath, IMAGE_ICON, 32, 32, LR_LOADFROMFILE );
#else
		hIcon = 0;
#endif
		info.m_handle = (void *)(intp)hIcon;
		m_Icons.Insert( name, info );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Simply dumps all data fields in object
//-----------------------------------------------------------------------------
class CDescribeData
{
public:
	CDescribeData( void const *src );

	void	DescribeShort( const short *invalue, int count );
	void	DescribeInt( const int *invalue, int count );		
	void	DescribeBool( const bool *invalue, int count );	
	void	DescribeFloat( const float *invalue, int count );	
	void	DescribeSimpleString( const char *indata, int length );		
	void	DescribeString( const string_t *instring, int count );			
	void	DescribeVector( const Vector *inValue, int count );
	void	DescribeColor( const Color *invalue, int count );	
	void	DumpDescription( datamap_t *pMap );

	void	GetValueForField( char const *fieldName, char *buf, size_t bufsize );

private:
	void	DescribeFields_R( int chain_count, datamap_t *pMap, typedescription_t *pFields, int fieldCount );
	bool	BuildFieldPath( CUtlString& path );
	
	void const		*m_pSrc;
	int				m_nSrcOffsetIndex;

	void			Describe( const char *fmt, ... );

	typedescription_t *m_pCurrentField;
	char const		*m_pCurrentClassName;
	datamap_t		*m_pCurrentMap;

	CUtlVector< CUtlString > m_FieldPath;
};

CDescribeData::CDescribeData( void const *src )
{
	m_pSrc				= src;
	m_nSrcOffsetIndex	= TD_OFFSET_NORMAL;

	m_pCurrentField		= NULL;
	m_pCurrentMap		= NULL;
	m_pCurrentClassName = NULL;
}

typedescription_t *FindFieldByName( datamap_t *pMap, char const *fn )
{
	while ( pMap )
	{
		for ( int i = 0; i < pMap->dataNumFields; i++ )
		{
			typedescription_t *current = &pMap->dataDesc[ i ];
			if ( !current->fieldName )
				continue;

			if ( !Q_stricmp( current->fieldName, fn ) )
				return current;
		}

		pMap = pMap->baseMap;
	}

	return NULL;
}

typedescription_t *FindField( datamap_t *pMap, char const *relativePath )
{
	if ( !Q_strstr( relativePath, "." ) )
	{
		// Simple case, just look up field name
		return FindFieldByName( pMap, relativePath );
	}

	// Complex case

	Assert( 0 );
	return NULL;
}

bool CDescribeData::BuildFieldPath( CUtlString& path )
{
	int c = m_FieldPath.Count();
	if ( c == 0 )
		return false;

	for ( int i = 0; i < c; ++i )
	{
		CUtlString& s = m_FieldPath[ i ];
		if ( i != 0 )
		{
			path += ".";
		}
		path += s;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fmt - 
//			... - 
//-----------------------------------------------------------------------------
void CDescribeData::Describe( const char *fmt, ... )
{
	Assert( m_pCurrentMap );
	Assert( m_pCurrentClassName );

	const char *fieldname = "empty";

	if ( m_pCurrentField )
	{
		fieldname	= m_pCurrentField->fieldName ? m_pCurrentField->fieldName : "NULL";
	}

	va_list argptr;
	char data[ 4096 ];
	int len;
	va_start(argptr, fmt);
	len = Q_vsnprintf(data, sizeof( data ), fmt, argptr);
	va_end(argptr);

	CUtlString fp;
	if ( BuildFieldPath( fp ) )
	{
		Msg( "%s.%s%s",
			fp.String(),
			fieldname,
			data );
	}
	else
	{
		Msg( "%s%s",
			fieldname,
			data );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : size - 
//			*outdata - 
//			*indata - 
//-----------------------------------------------------------------------------
void CDescribeData::DescribeSimpleString( char const *invalue, int count )
{
	Describe( "%s\n", invalue ? invalue : "" );
}


void CDescribeData::DescribeShort( const short *invalue, int count )
{
	for ( int i = 0; i < count; ++i )
	{
		if ( count == 1 )
		{
			Describe( " short (%i)\n", (int)(invalue[i]) );
		}
		else
		{
			Describe( "[%i] short (%i)\n", i, (int)(invalue[i]) );
		}
	}
}


void CDescribeData::DescribeInt( const int *invalue, int count )
{
	for ( int i = 0; i < count; ++i )
	{
		if ( count == 1 )
		{
			Describe( " integer (%i)\n", invalue[i] );
		}
		else
		{
			Describe( "[%i] integer (%i)\n", i, invalue[i] );
		}
	}
}

void CDescribeData::DescribeBool( const bool *invalue, int count )
{
	for ( int i = 0; i < count; ++i )
	{
		if ( count == 1 )
		{
			Describe( " bool (%s)\n", (invalue[i]) ? "true" : "false" );
		}
		else
		{
			Describe( "[%i] bool (%s)\n", i, (invalue[i]) ? "true" : "false" );
		}
	}
}

void CDescribeData::DescribeFloat( const float *invalue, int count )
{
	for ( int i = 0; i < count; ++i )
	{
		if ( count == 1 )
		{
			Describe( " float (%f)\n", invalue[ i ] );
		}
		else
		{
			Describe( "[%i] float (%f)\n", i, invalue[ i ] );
		}
	}
}

void CDescribeData::DescribeString( const string_t *instring, int count )
{
	for ( int i = 0; i < count; ++i )
	{
		if ( count == 1 )
		{
			Describe( " string (%s)\n", instring[ i ] ? instring[ i ]  : "" );
		}
		else
		{
			Describe( "[%i] string (%s)\n", i, instring[ i ] ? instring[ i ]  : "" );
		}
	}
}

void CDescribeData::DescribeColor( const Color *invalue, int count )
{
	for ( int i = 0; i < count; ++i )
	{
		if ( count == 1 )
		{
			Describe( " color (%i %i %i %i)\n", invalue[ i ].r(), invalue[ i ].g(), invalue[ i ].b(), invalue[ i ].a() );
		}
		else
		{
			Describe( "[%i] color (%i %i %i %i)\n", i, invalue[ i ].r(), invalue[ i ].g(), invalue[ i ].b(), invalue[ i ].a() );
		}
	}
}

void CDescribeData::DescribeVector( const Vector *inValue, int count )
{
	for ( int i = 0; i < count; ++i )
	{
		if ( count == 1 )
		{
			Describe( " vector (%f %f %f)\n", 
				inValue[i].x, inValue[i].y, inValue[i].z );
		}
		else
		{
			Describe( "[%i] vector (%f %f %f)\n", 
				i,
				inValue[i].x, inValue[i].y, inValue[i].z );
		}
	}
}

void CDescribeData::DescribeFields_R( int chain_count, datamap_t *pRootMap, typedescription_t *pFields, int fieldCount )
{
	int				i;
	int				flags;
	int				fieldOffsetSrc;
	int				fieldSize;

	m_pCurrentMap = pRootMap;
	if ( !m_pCurrentClassName )
	{
		m_pCurrentClassName = pRootMap->dataClassName;
	}

	for ( i = 0; i < fieldCount; i++ )
	{
		m_pCurrentField = &pFields[ i ];
		flags = m_pCurrentField->flags;

		// Skip this field
		if ( flags & FTYPEDESC_VIEW_NEVER )
			continue;
		
		// Mark any subchains first
		if ( m_pCurrentField->override_field != NULL )
		{
			m_pCurrentField->override_field->override_count = chain_count;
		}

		// Skip this field?
		if ( m_pCurrentField->override_count == chain_count )
		{
			continue;
		}

		void const *pInputData;
		
		fieldOffsetSrc = m_pCurrentField->fieldOffset[ m_nSrcOffsetIndex ];
		fieldSize = m_pCurrentField->fieldSize;
		
		pInputData = (void const *)((char *)m_pSrc + fieldOffsetSrc );
		
		switch( m_pCurrentField->fieldType )
		{
		default:
			break;
		case FIELD_EMBEDDED:
			{
				typedescription_t *save = m_pCurrentField;
				void const *saveSrc = m_pSrc;
				const char *saveName = m_pCurrentClassName;
				
				m_pCurrentClassName = m_pCurrentField->td->dataClassName;
				
				CUtlString str;
				str = m_pCurrentField->fieldName;

				m_FieldPath.AddToTail( str );

				m_pSrc = pInputData;
				if ( ( flags & FTYPEDESC_PTR ) && (m_nSrcOffsetIndex == PC_DATA_NORMAL) )
				{
					m_pSrc = *((void**)m_pSrc);
				}

				DescribeFields_R( chain_count, pRootMap, m_pCurrentField->td->dataDesc, m_pCurrentField->td->dataNumFields );
				
				m_FieldPath.Remove( m_FieldPath.Count() - 1 );

				m_pCurrentClassName = saveName;
				m_pCurrentField = save;
				m_pSrc = saveSrc;
			}
			break;
		case FIELD_FLOAT:
			DescribeFloat( (float const *)pInputData, fieldSize );
			break;
		case FIELD_STRING:
			DescribeString( (const string_t*)pInputData, fieldSize );
			break;
		case FIELD_VECTOR:
			DescribeVector( (const Vector *)pInputData, fieldSize );
			break;
		case FIELD_COLOR32:
			DescribeColor( (const Color *)pInputData, fieldSize );
			break;
			
		case FIELD_BOOLEAN:
			DescribeBool( (bool const *)pInputData, fieldSize );
			break;
		case FIELD_INTEGER:
			DescribeInt( (int const *)pInputData, fieldSize );
			break;
			
		case FIELD_SHORT:
			DescribeShort( (short const *)pInputData, fieldSize );
			break;
			
		case FIELD_CHARACTER:
			DescribeSimpleString( (const char *)pInputData, fieldSize );
			break;
		}
	}

	m_pCurrentClassName = NULL;
}

static int g_nChainCount = 1;
extern void ValidateChains_R( datamap_t *dmap );

void CDescribeData::DumpDescription( datamap_t *pMap )
{
	++g_nChainCount;

	if ( !pMap->chains_validated )
	{
		ValidateChains_R( pMap );
	}

	while ( pMap )
	{
        DescribeFields_R( g_nChainCount, pMap, pMap->dataDesc, pMap->dataNumFields );

		pMap = pMap->baseMap;
	}
}


void CLCD::DumpPlayer()
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return;

	Msg( "(localplayer)\n\n" );

	{
		CDescribeData helper( player );
		helper.DumpDescription( player->GetPredDescMap() );
	}

	Msg( "(localteam)\n\n" );

	C_Team *team = player->GetTeam();
	if ( team )
	{
		CDescribeData helper( team );
		helper.DumpDescription( team->GetPredDescMap() );
	}

	Msg( "(playerresource)\n\n" );

	if ( g_PR )
	{
		CDescribeData helper( g_PR );
		helper.DumpDescription( g_PR->GetPredDescMap() );
	}

	Msg( "(localplayerweapon)\n\n" );
	// Get the player's weapons, too
	C_BaseCombatWeapon *active = player->GetActiveWeapon();
	if ( active )
	{
		CDescribeData helper( active );
		helper.DumpDescription( active->GetPredDescMap() );
	}

	Msg( "Other replacements:\n\n" );

	// Global replacements
	for( int i = m_GlobalStats.First() ; i != m_GlobalStats.InvalidIndex(); i = m_GlobalStats.Next( i ) )
	{
		CUtlString& r = m_GlobalStats[ i ];

		char const *pReplace = r.String();
		char ansi[ 512 ];
		ansi[ 0 ] = 0;

		if ( pReplace[ 0 ] == '#' )
		{
			const wchar_t *pWString = g_pVGuiLocalize->Find( pReplace );
			if ( pWString )
			{
				g_pVGuiLocalize->ConvertUnicodeToANSI( pWString, ansi, sizeof( ansi ) );
				pReplace = ansi;
			}
		}

		Msg( "'%s' = '%s'\n", m_GlobalStats.GetElementName( i ), pReplace );
	}
}

bool CLCD::ExtractArrayIndex( char *str, size_t bufsize, int *index )
{
	Assert( index );
	*index = 0;

	char s[ 2048 ];
	Q_strncpy( s, str, sizeof( s ) );

	char *pos = Q_strstr( s, "[" );
	if ( !pos )
		return false;

	char *pos2 = Q_strstr( s, "]" );
	if ( !pos2 )
		return false;

	char num[ 32 ];
	Q_strncpy( num, pos + 1, pos2 - pos );
	*index = Q_atoi( num );

	int left = pos - s + 1;
	char o[ 2048 ];
	Q_strncpy( o, s, left );
	Q_strncat( o, pos2 + 1, sizeof( o ), COPY_ALL_CHARACTERS );

	Q_strncpy( str, o, bufsize );
	return true;
}

void CLCD::LookupToken( char const *in, CUtlString& value )
{
	value = "";

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
	{
		return;
	}

	C_BaseEntity *ref = NULL;

	char outbuf[ 1024 ];
	char *o = outbuf;
	char const *i = in;
	while ( *i )
	{
		if ( *i == '(' )
		{
			char token[ 512 ];
			char *to = token;
			// swallow everything until matching '%' character
			++i;
			while ( *i && *i != ')' )
			{
				*to++ = *i++;
			}

			if ( *i )
				++i;

			*to = 0;

			if ( !Q_stricmp( token, "localplayer" ) )
			{
				ref = player;
			}
			else if ( !Q_stricmp( token, "localteam" ) )
			{
				ref = player->GetTeam();
			}
			else if ( !Q_stricmp( token, "localplayerweapon" ) )
			{
				ref = player->GetActiveWeapon();
			}
			else if ( !Q_stricmp( token, "playerresource" ) )
			{
				ref = g_PR;
			}
		}
		else
		{
			*o++ = *i++;
		}
	}

	*o = '\0';

	// Fixme, also need to get array reference removed from end and do the . field searching stuff

	// Outbuf now has the actual field
	if ( !ref )
	{
		return;
	}

	int iIndex = 0;
	ExtractArrayIndex( outbuf, sizeof( outbuf ), &iIndex );
	
	typedescription_t *td = FindField( ref->GetPredDescMap(), outbuf );
	if ( !td )
	{
		return;
	}

	// Not allowed to see this one
	if ( td->flags & FTYPEDESC_VIEW_NEVER )
	{
		return;
	}

	int fieldOffsetSrc = td->fieldOffset[ TD_OFFSET_NORMAL ];
	// int fieldSize = td->fieldSize;
		
	void const *pInputData = (void const *)((char *)ref + fieldOffsetSrc );

	char sz[ 256 ];
	sz[ 0 ] = 0;
	// Found it, now get the value
	switch ( td->fieldType )
	{
	case FIELD_FLOAT:
		Q_snprintf( sz, sizeof( sz ), "%.2f", *((float *)pInputData + iIndex ) );
		break;
	case FIELD_STRING:
		{
			string_t *pString = (string_t *)((string_t *)pInputData + iIndex );
			Q_snprintf( sz, sizeof( sz ), "%s", pString ? STRING( *pString ) : "" );
		}
		break;
	case FIELD_VECTOR:
		{
			Vector v = *((Vector *)pInputData + iIndex );
            Q_snprintf( sz, sizeof( sz ), "%.2f %.2f %.2f", v.x, v.y, v.z );
		}
		break;
	case FIELD_COLOR32:
		{
			Color c = *(( Color * )pInputData + iIndex );
			Q_snprintf( sz, sizeof( sz ), "%d %d %d %d", c.r(), c.g(), c.b(), c.a() );
		}
		break;
		
	case FIELD_BOOLEAN:
		Q_snprintf( sz, sizeof( sz ), "%s", *( ( bool *)pInputData + iIndex ) ? "true" : "false" );
		break;
	case FIELD_INTEGER:
		Q_snprintf( sz, sizeof( sz ), "%i", *( (int *)pInputData + iIndex ));
		break;
		
	case FIELD_SHORT:
		Q_snprintf( sz, sizeof( sz ), "%i", *( (short *)pInputData + iIndex ) );
		break;
		
	case FIELD_CHARACTER:
		Q_snprintf( sz, sizeof( sz ), "%s", ((const char *)pInputData + iIndex ) );
		break;
	}

	value = sz;
}

void CLCD::BuildUpdatedText( char const *in, CUtlString& out )
{
	char outbuf[ 1024 ];
	char *o = outbuf;
	char const *i = in;
	while ( *i )
	{
		if ( *i == '%' )
		{
			char token[ 512 ];
			char *to = token;
			// swallow everything until matching '%' character
			++i;
			while ( *i && *i != '%' )
			{
				*to++ = *i++;
			}

			if ( *i )
				++i;

			*to = 0;

			// Now we have the token, do the lookup
			CUtlString value;
			LookupToken( token, value );

			to = (char *)value.String();
			while ( *to )
			{
				*o++ = *to++;
			}
		}
		else
		{
			*o++ = *i++;
		}
	}

	*o = '\0';

	out = outbuf;
}


bool CLCD::Replace( CUtlString& str, char const *search, char const *replace )
{
	// If search string is part of replacement, this is a bad thing!!!
	Assert( !*replace || !Q_strstr( replace, search ) );

	bool changed = false;
	if ( !Q_strstr( str.String(), search ) )
		return false;

	char s[ 2048 ];
	Q_strncpy( s, str.String(), sizeof( s ) );

	int searchlen = Q_strlen( search );
	while ( true )
	{
		char *pos = Q_strstr( s, search );
		if ( !pos )
			break;

		char temp[ 4096 ];
		// Found an instance
		int left = pos - s + 1;
		Assert( left < sizeof( temp ) );
		Q_strncpy( temp, s, left );
		Q_strncat( temp, replace, sizeof( temp ), COPY_ALL_CHARACTERS );
		int rightofs = left + searchlen - 1;
		Q_strncat( temp, &s[ rightofs ], sizeof( temp ), COPY_ALL_CHARACTERS );

		// Replace entire string
		Q_strncpy( s, temp, sizeof( s ) );
		changed = true;
	}

	str = s;

	return changed;
}

void CLCD::SetGlobalStat( char const *name, char const *value )
{
	if ( !m_lcd )
		return;

	int idx = m_GlobalStats.Find( name );
	if ( idx == m_GlobalStats.InvalidIndex() )
	{
		idx = m_GlobalStats.Insert( name );
	}

	m_GlobalStats[ idx ] = value;
}

void CLCD::AddChatLine( char const *txt )
{
	if ( !m_lcd )
		return;

	while ( m_ChatHistory.Count() >= m_nMaxChatHistory )
	{
		m_ChatHistory.Remove( 0 );
	}

	m_ChatHistory.AddToTail( CUtlString( txt ) );

	UpdateChat();
}

void CLCD::UpdateChat()
{
	for ( int i = 0; i < m_nMaxChatHistory; ++i )
	{
		char name[ 32 ];
		Q_snprintf( name, sizeof( name ), "chat_%d", i + 1 );

		SetGlobalStat( name, i < m_ChatHistory.Count() ? m_ChatHistory[ i ].String() : " " );
	}
}

void CLCD::DoGlobalReplacements( CUtlString& str )
{
	// Put some limit to avoid infinite recursion
	int maxChanges = 16;

	bool changed = false;
	do
	{
		changed = false;
		for ( int i = m_GlobalStats.First(); i != m_GlobalStats.InvalidIndex(); i = m_GlobalStats.Next( i ) )
		{
			CUtlString &r = m_GlobalStats[ i ];

			char const *pReplace = r.String();
			char ansi[ 512 ];
			ansi[ 0 ] = 0;

			if ( pReplace[ 0 ] == '#' )
			{
				const wchar_t *pWString = g_pVGuiLocalize->Find( pReplace );
				if ( pWString )
				{
					g_pVGuiLocalize->ConvertUnicodeToANSI( pWString, ansi, sizeof( ansi ) );
					pReplace = ansi;
				}
			}

			if ( Replace( str, m_GlobalStats.GetElementName( i ), pReplace ) )
			{
				changed = true;
			}
		}
	} while ( changed && --maxChanges >= 0 );
}

void CLCD::ReduceParentheses( CUtlString& str )
{
	char s[ 2048 ];
	Q_strncpy( s, str.String(), sizeof( s ) );

	while ( true )
	{
		char *pos = Q_strstr( s, "(" );
		if ( !pos )
			break;

		char *end = Q_strstr( pos, ")" );
		if ( !end )
			break;

		char temp[ 4096 ];
		// Found an instance
		int left = pos - s + 1;
		Assert( left < sizeof( temp ) );
		Q_strncpy( temp, s, left );
		int rightofs = end - s + 1;
		Q_strncat( temp, &s[ rightofs ], sizeof( temp ), COPY_ALL_CHARACTERS );

		// Replace entire string
		Q_strncpy( s, temp, sizeof( s ) );
	}

	str = s;
}

void CLCD::ParseReplacements( KeyValues *kv )
{
	for ( KeyValues *sub = kv->GetFirstSubKey(); sub; sub = sub->GetNextKey() )
	{
		char const *key = sub->GetName();
		char const *value = sub->GetString();

		SetGlobalStat( key, value );
	}
}
