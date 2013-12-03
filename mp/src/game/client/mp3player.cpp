//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A simple .mp3 player example
//
//=============================================================================

#include "cbase.h"

#if 0
#include "mp3player.h"
#include "KeyValues.h"
#include "filesystem.h"

#include "vgui_controls/MenuButton.h"
#include "vgui_controls/Menu.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/Slider.h"
#include "vgui_controls/ListPanel.h"
#include "vgui/IPanel.h"
#include "vgui/IVGui.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "vgui/ILocalize.h"
#include "vgui_controls/PHandle.h"

#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/PropertyPage.h"
#include "vgui_controls/TreeView.h"
#include "vgui_controls/FileOpenDialog.h"
#include "vgui_controls/DirectorySelectDialog.h"
#include "checksum_crc.h"

#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

// Singleton
static CMP3Player *g_pPlayer = NULL;

vgui::Panel *GetSDKRootPanel();

// Time between songs
#define END_GAP_TIME	1.0f

#define SOUND_ROOT "sound"

#define MUTED_VOLUME		0.02f

#define TREE_TEXT_COLOR		Color( 200, 255, 200, 255 )
#define LIST_TEXT_COLOR		TREE_TEXT_COLOR

#define DB_FILENAME			"resource/mp3player_db.txt"
#define MP3_SETTINGS_FILE	"resource/mp3settings.txt"

#define MP3_DEFAULT_MP3DIR "c:\\my music"

CMP3Player *GetMP3Player()
{
	Assert( g_pPlayer );
	return g_pPlayer;
}

static void mp3_f()
{
	CMP3Player *player = GetMP3Player();
	if ( player )
	{
		player->SetVisible( !player->IsVisible() );
	}
}
void MP3Player_Create( vgui::VPANEL parent )
{
	Assert( !g_pPlayer );

	new CMP3Player( parent, "MP3Player" );

#if 0
	mp3_f();
#endif
}

void MP3Player_Destroy()
{
	if ( g_pPlayer )
	{
		g_pPlayer->MarkForDeletion();
		g_pPlayer = NULL;
	}
}

static ConCommand mp3( "mp3", mp3_f, "Show/hide mp3 player UI." );

//-----------------------------------------------------------------------------
// Purpose: This assumes artist/album/file.mp3!!!
// Input  : *relative - 
//			*artist - 
//			artistlen - 
//			*album - 
//			albumlen - 
// Output : static bool
//-----------------------------------------------------------------------------
static bool SplitArtistAlbum( char const *relative, char *artist, size_t artistlen, char *album, size_t albumlen )
{
	artist[ 0 ] = 0;
	album[ 0 ] = 0;
	char str[ 512 ];
	Q_strncpy( str, relative, sizeof( str ) );

	char seps[] = "/\\";
	char *p = strtok( str, seps );
	int pos = 0;
	while ( p )
	{
		switch ( pos )
		{
		default:
			break;
		case 0:
            Q_strncpy( artist, p, artistlen );
			break;
		case 1:
			Q_strncpy( album, p, albumlen );
			break;
		case 2:
			if ( !Q_stristr( p, ".mp3" ) )
			{
				artist[ 0 ] = 0;
				album[ 0 ] = 0;
				return false;
			}
			return true;
			break;
		}

		++pos;
		p = strtok( NULL, seps );
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CMP3FileListPage : public PropertyPage
{
	DECLARE_CLASS_SIMPLE( CMP3FileListPage, PropertyPage );

public:

	CMP3FileListPage( Panel *parent, CMP3Player *player, char const *panelName ) : 
	  BaseClass( parent, panelName ),
	  m_pPlayer( player )
	{
		m_pList = new ListPanel( this, "FileList" );
		m_pList->AddColumnHeader( 0, "File", "File", 200, ListPanel::COLUMN_RESIZEWITHWINDOW );
		m_pList->AddColumnHeader( 1, "Artist", "Artist", 150, ListPanel::COLUMN_RESIZEWITHWINDOW );
		m_pList->AddColumnHeader( 2, "Album", "Album", 150, ListPanel::COLUMN_RESIZEWITHWINDOW );
	}

	void Reset()
	{
		m_pList->DeleteAllItems();
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		m_pList->SetFont( pScheme->GetFont( "DefaultVerySmall" ) );
		m_pList->SetFgColor( LIST_TEXT_COLOR );
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		int w, h;
		GetSize( w, h );
		m_pList->SetBounds( 0, 0, w, h );
	}

	void AddSong( int songIndex )
	{
		Assert( m_pPlayer );

		const MP3File_t* songInfo = m_pPlayer->GetSongInfo( songIndex );
		if ( !songInfo )
		{
			return;
		}

		KeyValues *kv = new KeyValues( "LI" );
		kv->SetString( "File", songInfo->shortname.String() );
		char fn[ 512 ];
		if ( g_pFullFileSystem->String( songInfo->filename, fn, sizeof( fn ) ) )
		{
			char artist[ 256 ];
			char album[ 256 ];
			if ( SplitArtistAlbum( fn, artist, sizeof( artist ), album, sizeof( album ) ) )
			{
				kv->SetString( "Artist", artist );
				kv->SetString( "Album", album );
			}
		}
		kv->SetInt( "SongIndex", songIndex );
		m_pList->AddItem( kv, 0, false, false );
		kv->deleteThis();
	}

	void GetSelectedSongs( CUtlVector< int >&list )
	{
		list.RemoveAll();

		int selCount = m_pList->GetSelectedItemsCount();
		if ( selCount <= 0 )
		{
			return;
		}
		for ( int i = 0; i < selCount; ++i )
		{
			int itemId = m_pList->GetSelectedItem( 0 );
			KeyValues *kv = m_pList->GetItem( itemId );
			if ( !kv )
			{
				continue;
			}
			int song = kv->GetInt( "SongIndex", -1 );
			if ( song == -1 )
			{
				continue;
			}
			list.AddToTail( song );
		}
	}

	MESSAGE_FUNC_INT( OnOpenContextMenu, "OpenContextMenu", itemID );

	virtual void OnCommand( char const *cmd );

	MESSAGE_FUNC( OnItemSelected, "ItemSelected" )
	{
		CUtlVector< int > songList;
		GetSelectedSongs( songList );
		m_pPlayer->SelectedSongs( CMP3Player::SONG_FROM_FILELIST, songList );
	}

private:

	CMP3Player		*m_pPlayer;
	ListPanel		*m_pList;

	DHANDLE< Menu >	m_hMenu;
};

void CMP3FileListPage::OnOpenContextMenu( int itemID )
{
	if ( m_hMenu.Get() != NULL )
	{
		delete m_hMenu.Get();
	}

	m_hMenu = new Menu( this, "FileListContext" );
	m_hMenu->AddMenuItem( "AddToPlaylist", "#PlaylistAdd", "addsong", this );

	int x, y;
	input()->GetCursorPos( x, y );

	m_hMenu->SetPos( x, y );
	m_hMenu->SetVisible( true );
}

void CMP3FileListPage::OnCommand( char const *cmd )
{
	if ( !Q_stricmp( cmd, "addsong" ) )
	{
		// Get selected item
		int c = m_pList->GetSelectedItemsCount();
		if ( c > 0 )
		{
			int itemId = m_pList->GetSelectedItem( 0 );
			KeyValues *kv = m_pList->GetItem( itemId );
			if ( kv )
			{
				int songIndex = kv->GetInt( "SongIndex" );
				m_pPlayer->AddToPlayList( songIndex, false );
			}
		}
	}
	else
	{
		BaseClass::OnCommand( cmd );
	}
}

class CMP3PlayListPage : public PropertyPage
{
	DECLARE_CLASS_SIMPLE( CMP3PlayListPage, PropertyPage );

public:

	CMP3PlayListPage( Panel *parent, CMP3Player *player, char const *panelName ) : 
	  BaseClass( parent, panelName ),
	  m_pPlayer( player )
	{
		m_pList = new ListPanel( this, "PlayList" );
		m_pList->AddColumnHeader( 0, "File", "File", 400, ListPanel::COLUMN_RESIZEWITHWINDOW );
		m_pList->AddColumnHeader( 1, "Artist", "Artist", 150, ListPanel::COLUMN_RESIZEWITHWINDOW );
		m_pList->AddColumnHeader( 2, "Album", "Album", 150, ListPanel::COLUMN_RESIZEWITHWINDOW );
	}

	void Reset()
	{
		m_pList->DeleteAllItems();
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		m_pList->SetFont( pScheme->GetFont( "DefaultVerySmall" ) );
		m_pList->SetFgColor( LIST_TEXT_COLOR );
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		int w, h;
		GetSize( w, h );
		m_pList->SetBounds( 0, 0, w, h );
	}

	void AddSong( int songIndex )
	{
		Assert( m_pPlayer );

		const MP3File_t* songInfo = m_pPlayer->GetSongInfo( songIndex );
		if ( !songInfo )
		{
			return;
		}

		KeyValues *kv = new KeyValues( "LI" );
		kv->SetString( "File", songInfo->shortname.String() );
		char fn[ 512 ];
		if ( g_pFullFileSystem->String( songInfo->filename, fn, sizeof( fn ) ) )
		{
			char artist[ 256 ];
			char album[ 256 ];
			if ( SplitArtistAlbum( fn, artist, sizeof( artist ), album, sizeof( album ) ) )
			{
				kv->SetString( "Artist", artist );
				kv->SetString( "Album", album );
			}
		}
		kv->SetInt( "SongIndex", songIndex );
		m_pList->AddItem( kv, 0, false, false );
		kv->deleteThis();
	}

	void RemoveSong( int songIndex )
	{
		// Get selected item
		int c = m_pList->GetSelectedItemsCount();
		if ( c > 0 )
		{
			int itemId = m_pList->GetSelectedItem( 0 );
			KeyValues *kv = m_pList->GetItem( itemId );
			if ( kv && ( kv->GetInt( "SongIndex", -1 ) == songIndex ) )
			{
				m_pList->RemoveItem( itemId );
			}
		}
	}

	void GetSelectedSongs( CUtlVector< int >&list )
	{
		list.RemoveAll();

		int selCount = m_pList->GetSelectedItemsCount();
		if ( selCount <= 0 )
		{
			return;
		}
		for ( int i = 0; i < selCount; ++i )
		{
			int itemId = m_pList->GetSelectedItem( 0 );
			KeyValues *kv = m_pList->GetItem( itemId );
			if ( !kv )
			{
				continue;
			}
			int song = kv->GetInt( "SongIndex", -1 );
			if ( song == -1 )
			{
				continue;
			}
			list.AddToTail( song );
		}
	}


	MESSAGE_FUNC_INT( OnOpenContextMenu, "OpenContextMenu", itemID );

	virtual void OnCommand( char const *cmd );

	MESSAGE_FUNC( OnItemSelected, "ItemSelected" )
	{
		CUtlVector< int > songList;
		GetSelectedSongs( songList );
		m_pPlayer->SelectedSongs( CMP3Player::SONG_FROM_PLAYLIST, songList );
	}

	void OnItemPlaying( int listIndex )
	{	
		int itemId = m_pList->GetItemIDFromRow( listIndex );
		m_pList->ClearSelectedItems();
		m_pList->SetSingleSelectedItem( itemId );
	}

private:

	CMP3Player		*m_pPlayer;
	ListPanel		*m_pList;

	DHANDLE< Menu >	m_hMenu;
};

void CMP3PlayListPage::OnOpenContextMenu( int itemID )
{
	if ( m_hMenu.Get() != NULL )
	{
		delete m_hMenu.Get();
	}

	m_hMenu = new Menu( this, "PlayListContext" );
	m_hMenu->AddMenuItem( "Remove", "#PlayListRemove", "removesong", this );
	m_hMenu->AddMenuItem( "Clear", "#PlayListClear", "clear", this );

	m_hMenu->AddMenuItem( "Load", "#PlayListLoad", "load", this );
	m_hMenu->AddMenuItem( "Save", "#PlayListSave", "save", this );

	int x, y;
	input()->GetCursorPos( x, y );

	m_hMenu->SetPos( x, y );
	m_hMenu->SetVisible( true );
}

void CMP3PlayListPage::OnCommand( char const *cmd )
{
	if ( !Q_stricmp( cmd, "removesong" ) )
	{
		// Get selected item
		int c = m_pList->GetSelectedItemsCount();
		if ( c > 0 )
		{
			int itemId = m_pList->GetSelectedItem( 0 );
			KeyValues *kv = m_pList->GetItem( itemId );
			if ( kv )
			{
				int songIndex = kv->GetInt( "SongIndex" );
				m_pPlayer->RemoveFromPlayList( songIndex );
			}
		}
	}
	else if ( !Q_stricmp( cmd, "clear" ) )
	{
		m_pPlayer->ClearPlayList();
	}
	else if ( !Q_stricmp( cmd, "load" ) )
	{
		m_pPlayer->OnLoadPlayList();
	}
	else if ( !Q_stricmp( cmd, "save" ) )
	{
		m_pPlayer->OnSavePlayList();
	}
	else if ( !Q_stricmp( cmd, "saveas" ) )
	{
		m_pPlayer->OnSavePlayListAs();
	}
	else
	{
		BaseClass::OnCommand( cmd );
	}
}

class CMP3FileSheet : public PropertySheet
{
	DECLARE_CLASS_SIMPLE( CMP3FileSheet, PropertySheet );

public:

	CMP3FileSheet( CMP3Player *player, char const *panelName );

	void		ResetFileList()
	{
		if ( m_pFileList )
		{
			m_pFileList->Reset();
		}
	}

	void		AddSongToFileList( int songIndex )
	{
		if ( m_pFileList )
		{
			m_pFileList->AddSong( songIndex );
		}
	}

	void		ResetPlayList()
	{
		if ( m_pPlayList )
		{
			m_pPlayList->Reset();
		}
	}
	void		AddSongToPlayList( int songIndex )
	{
		if ( m_pPlayList )
		{
			m_pPlayList->AddSong( songIndex );
		}
	}

	void		RemoveSongFromPlayList( int songIndex )
	{
		if ( m_pPlayList )
		{
			m_pPlayList->RemoveSong( songIndex );
		}
	}

	void OnPlayListItemPlaying( int listIndex )
	{
		if ( m_pPlayList )
		{
			m_pPlayList->OnItemPlaying( listIndex );
		}
	}

protected:

	CMP3Player			*m_pPlayer;

	CMP3PlayListPage	*m_pPlayList;
	CMP3FileListPage	*m_pFileList;
};

CMP3FileSheet::CMP3FileSheet( CMP3Player *player, char const *panelName ) :
	BaseClass( (Panel *)player, panelName ),
	m_pPlayer( player )
{
	m_pPlayList = new CMP3PlayListPage( this, player, "PlayList" );
	m_pFileList = new CMP3FileListPage( this, player, "FileList" );

	AddPage( m_pPlayList, "#PlayListTab" );
	AddPage( m_pFileList, "#FileListTab" );

	SetActivePage( m_pPlayList );
}	

class CMP3TreeControl : public TreeView
{
	DECLARE_CLASS_SIMPLE( CMP3TreeControl, TreeView );

public:

	CMP3TreeControl( CMP3Player *player, char const *panelName );

	int GetSelectedSongIndex();

	MESSAGE_FUNC( OnTreeViewItemSelected, "TreeViewItemSelected" )
	{
		CUtlVector< int > songList;
		int idx = GetSelectedSongIndex();
		if ( idx != -1 )
		{
			songList.AddToTail( idx );
		}

		m_pPlayer->SelectedSongs( CMP3Player::SONG_FROM_TREE, songList );

		if ( vgui::input()->IsMouseDown( MOUSE_RIGHT ) )
		{
			OpenContextMenu();
		}
	}

	virtual void OnCommand( char const *cmd );

private:
	void OpenContextMenu();


	CMP3Player *m_pPlayer;
	DHANDLE< Menu >	m_hMenu;
};

CMP3TreeControl::CMP3TreeControl( CMP3Player *player, char const *panelName ) :
	BaseClass( (Panel *)player, panelName ),
	m_pPlayer( player )
{
	AddActionSignalTarget( this );
}

int CMP3TreeControl::GetSelectedSongIndex()
{
	CUtlVector< KeyValues * > kv;
	GetSelectedItemData( kv );
	if ( !kv.Count() )
	{
		return -1;
	}
	return kv[ 0 ]->GetInt( "SongIndex", -1 );
}

void CMP3TreeControl::OpenContextMenu()
{
	if ( m_hMenu.Get() != NULL )
	{
		delete m_hMenu.Get();
	}

	m_hMenu = new Menu( this, "TreeContext" );
	m_hMenu->AddMenuItem( "AddToPlaylist", "#PlaylistAdd", "addsong", this );
	m_hMenu->AddMenuItem( "AddToPlaylist", "#PlaySong", "playsong", this );

	int x, y;
	input()->GetCursorPos( x, y );

	m_hMenu->SetPos( x, y );
	m_hMenu->SetVisible( true );
}

void CMP3TreeControl::OnCommand( char const *cmd )
{
	if ( !Q_stricmp( cmd, "addsong" ) )
	{
		// Get selected item
		int songIndex = GetSelectedSongIndex();
		if ( songIndex >= 0 )
		{
			m_pPlayer->AddToPlayList( songIndex, false );
		}
	}
	else if ( !Q_stricmp( cmd, "playsong" ) )
	{
		int songIndex = GetSelectedSongIndex();
		if ( songIndex >= 0 )
		{
			m_pPlayer->AddToPlayList( songIndex, true );
		}
	}
	else
	{
		BaseClass::OnCommand( cmd );
	}
}

class CMP3SongProgress : public Slider
{
	DECLARE_CLASS_SIMPLE( CMP3SongProgress, Slider );

public:

	CMP3SongProgress( Panel *parent, char const *panelName ) :
		BaseClass( parent, panelName )
	{
		SetPaintEnabled( false );
		SetRange( 0, 100 );
	}

	void	SetProgress( float frac )
	{
		SetValue( (int)( frac * 100.0f + 0.5f ), false );
	}

	virtual void PaintBackground()
	{
		//BaseClass::PaintBackground();
		
		int w, h;
		GetSize( w, h );
		
		float frac = (float)GetValue() * 0.01f;

		int barend = ( int )( (float)( w - 2 ) * frac + 0.5f );
		
		surface()->DrawSetColor( GetBgColor() );
		surface()->DrawFilledRect( 0, 0, w, h );
		surface()->DrawSetColor( GetFgColor() );
		surface()->DrawFilledRect( 1, 1, barend, h - 1 );
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		SetFgColor( TREE_TEXT_COLOR );
		SetBgColor( pScheme->GetColor( "BorderDark", Color( 0, 0, 0, 255 ) ) );
	}
};

CMP3Player::CMP3Player( VPANEL parent, char const *panelName ) :
	BaseClass( NULL, panelName ),
	m_SelectionFrom( SONG_FROM_UNKNOWN ),
	m_bDirty( false ),
	m_bSettingsDirty( false ),
	m_PlayListFileName( UTL_INVAL_SYMBOL ),
	m_bSavingFile( false ),
	m_bEnableAutoAdvance( true )
{
	g_pPlayer = this;

	// Get strings...
	g_pVGuiLocalize->AddFile( "resource/mp3player_%language%.txt" );
	SetParent( parent );

	SetMoveable( true );
	SetSizeable( true );
	SetMenuButtonVisible( false );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( true );

	m_pOptions = new MenuButton( this, "Menu", "#MP3Menu" );

	Menu *options = new Menu( m_pOptions, "Options" );
	options->AddMenuItem( "AddDir", "#AddDirectory", new KeyValues( "Command", "command", "adddirectory" ), this );
	options->AddMenuItem( "AddGame", "#AddGameSongs", new KeyValues( "Command", "command", "addgamesongs" ), this );
	options->AddMenuItem( "Refresh", "#RefreshDb", new KeyValues( "Command", "command", "refresh" ), this );

	m_pOptions->SetMenu( options );

	m_pTree = new CMP3TreeControl( this, "Tree" );
	m_pTree->MakeReadyForUse();

	// Make tree use small font
	IScheme *pscheme = scheme()->GetIScheme( GetScheme() );
	HFont treeFont = pscheme->GetFont( "DefaultVerySmall" );
	m_pTree->SetFont( treeFont );

	m_pFileSheet = new CMP3FileSheet( this, "FileSheet" );

	m_pPlay = new Button( this, "Play", "#Play", this, "play" );
	m_pStop = new Button( this, "Stop", "#Stop", this, "stop" );
	m_pNext = new Button( this, "NextTrack", "#Next", this, "nexttrack" );
	m_pPrev = new Button( this, "PrevTrack", "#Prev", this, "prevtrack" );
	m_pMute = new CheckButton( this, "Mute", "#Mute" );
	m_pShuffle = new CheckButton( this, "Shuffle", "#Shuffle" );

	m_pVolume = new Slider( this, "Volume" );
	m_pVolume->SetRange( (int)( MUTED_VOLUME * 100.0f ), 100 );
	m_pVolume->SetValue( 100 );

	m_pCurrentSong = new Label( this, "SongName", "#NoSong" );
	m_pDuration = new Label( this, "SongDuration", "" );
	
	m_pSongProgress = new CMP3SongProgress( this, "Progress" );
	m_pSongProgress->AddActionSignalTarget( this );
	
	SetSize( 400, 450 );

	SetMinimumSize( 350, 400 );

	SetTitle( "#MP3PlayerTitle", true );

	LoadControlSettings( "resource/MP3Player.res" );

	m_pCurrentSong->SetText( "#NoSong" );
	m_pDuration->SetText( "" );

	m_nCurrentFile = -1;
	m_bFirstTime = true;
	m_bPlaying = false;
	m_SongStart = -1.0f;
	m_nSongGuid = 0;
	m_nCurrentSong = 0;
	m_nCurrentPlaylistSong = 0;
	m_flCurrentVolume = 1.0f;
	m_bMuted = false;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
}

CMP3Player::~CMP3Player()
{
	if ( m_bDirty )
	{
		SaveDb( DB_FILENAME );
	}
	if ( m_bSettingsDirty )
	{
		SaveSettings();
	}
	DeleteSoundDirectories();
	RemoveTempSounds();
}

void CMP3Player::DeleteSoundDirectories()
{
	int c = m_SoundDirectories.Count();
	for ( int i = c - 1 ; i >= 0 ; --i )
	{
		delete m_SoundDirectories[ i ];
	}
	m_SoundDirectories.RemoveAll();
}

void CMP3Player::RemoveTempSounds()
{
	FileFindHandle_t fh;

	char path[ 512 ];
	Q_strncpy( path, "sound/_mp3/*.mp3", sizeof( path ) );

	char const *fn = g_pFullFileSystem->FindFirstEx( path, "MOD", &fh );
	if ( fn )
	{
		do
		{
			if ( fn[0] != '.'  )
			{
				char ext[ 10 ];
				Q_ExtractFileExtension( fn, ext, sizeof( ext ) );

				if ( !Q_stricmp( ext, "mp3" ) )
				{
					char killname[ 512 ];
					Q_snprintf( killname, sizeof( killname ), "sound/_mp3/%s", fn );
					g_pFullFileSystem->RemoveFile( killname, "MOD" );
				}
			}

			fn = g_pFullFileSystem->FindNext( fh );

		} while ( fn );

		g_pFullFileSystem->FindClose( fh );
	}
}

void CMP3Player::WipeSoundDirectories()
{
	int c = m_SoundDirectories.Count();
	for ( int i = c - 1 ; i >= 0 ; --i )
	{
		SoundDirectory_t *sd = m_SoundDirectories[ i ];
		sd->m_pTree->DeleteSubdirectories();
		sd->m_pTree->m_FilesInDirectory.RemoveAll();
	}
}

void CMP3Player::AddGameSounds( bool recurse )
{
	SoundDirectory_t *gamesounds = NULL;
	int idx = FindSoundDirectory( "" );
	if ( idx == m_SoundDirectories.InvalidIndex() )
	{
		gamesounds = new SoundDirectory_t( m_SoundDirectories.Count() );
		gamesounds->m_bGameSound = true;
		gamesounds->m_Root = "";
		gamesounds->m_pTree = new MP3Dir_t();
		gamesounds->m_pTree->m_DirName = "Game Sounds";
		gamesounds->m_pTree->m_FullDirPath = "";

		m_SoundDirectories.AddToTail( gamesounds );
	}
	else
	{
		gamesounds = m_SoundDirectories[ idx ];
		if ( recurse )
		{
			gamesounds->m_pTree->DeleteSubdirectories();
			gamesounds->m_pTree->m_FilesInDirectory.RemoveAll();
		}
	}
	
	if ( recurse && gamesounds )
	{
		m_nFilesAdded = 0;
		RecursiveFindMP3Files( gamesounds, SOUND_ROOT, "GAME" );
	}
}

void CMP3Player::OnRefresh()
{
	CUtlVector< CUtlSymbol > dirnames;
	int i, c;
	
	CUtlVector< FileNameHandle_t >	m_PlayListFiles;
	
	int pcount = m_PlayList.Count();
	for ( i = 0; i < pcount; ++i )
	{
		m_PlayListFiles.AddToTail( m_Files[ m_PlayList[ i ] ].filename );
	}

	m_Files.RemoveAll();
	WipeSoundDirectories();
	ClearPlayList();

	c = m_SoundDirectories.Count();
	for ( i = 0; i < c; ++i )
	{
		SoundDirectory_t *sd = m_SoundDirectories[ i ];

		// Now enumerate all .mp3 files in subfolders of this
		if ( sd->m_bGameSound )
		{
			m_nFilesAdded = 0;
			RecursiveFindMP3Files( sd, SOUND_ROOT, "GAME" );
		}
		else
		{
			// Add to search path
			g_pFullFileSystem->AddSearchPath( sd->m_Root.String(), "MP3" );
			// Don't pollute regular searches...
			g_pFullFileSystem->MarkPathIDByRequestOnly( "MP3", true );

			m_nFilesAdded = 0;
			RecursiveFindMP3Files( sd, "", "MP3" );
		}
	}

	for ( i = 0; i < pcount; ++i )
	{
		char fn[ 512 ];
		if ( g_pFullFileSystem->String( m_PlayListFiles[ i ], fn, sizeof( fn ) ) )
		{
			// Find index for song
			int songIndex = FindSong( fn );
			if ( songIndex >= 0 )
			{
				AddToPlayList( songIndex, false );
			}
		}
	}

	PopulateTree();

	m_bDirty = true;
}

void CMP3Player::SetVisible( bool state )
{
	BaseClass::SetVisible( state );
	if ( m_bFirstTime && state )
	{
		MoveToCenterOfScreen();

		m_bFirstTime = false;

		LoadSettings();
		if ( !RestoreDb( DB_FILENAME ) && m_Files.Count() == 0 )
		{
			// Load the "game" stuff
			OnRefresh();
		}
		
		PopulateTree();
	}
}

void CMP3Player::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	HFont treeFont = pScheme->GetFont( "DefaultVerySmall" );
	m_pTree->SetFont( treeFont );
}

void CMP3Player::OnCommand( char const *cmd )
{
	if ( !Q_stricmp( cmd, "OnClose" ) )
	{
		SetVisible( false );
	}
	else if ( !Q_stricmp( cmd, "play" ) )
	{
		OnPlay();
	}
	else if ( !Q_stricmp( cmd, "stop" ) )
	{
		OnStop();
	}
	else if ( !Q_stricmp( cmd, "nexttrack" ) )
	{
		OnNextTrack();
	}
	else if ( !Q_stricmp( cmd, "prevtrack" ) )
	{
		OnPrevTrack();
	}
	else if ( !Q_stricmp( cmd, "refresh" ) )
	{
		OnRefresh();
	}
	else if ( !Q_stricmp( cmd, "adddirectory" ) )
	{
		ShowDirectorySelectDialog();
	}
	else if ( !Q_stricmp( cmd, "addgamesongs" ) )
	{
		AddGameSounds( true );
		PopulateTree();
	}
	else
	{
		BaseClass::OnCommand( cmd );
	}
}

void CMP3Player::SplitFile( CUtlVector< CUtlSymbol >& splitList, char const *relative )
{
	char work[ 512 ];
	Q_strncpy( work, relative, sizeof( work ) );
	char const *separators = "/\\";

	char *token = strtok( work, separators );
	while ( token )
	{
		CUtlSymbol sym = token;
		splitList.AddToTail( sym );

		token = strtok( NULL, separators );
	}

}

MP3Dir_t *CMP3Player::FindOrAddSubdirectory( MP3Dir_t *parent, char const *dirname )
{
	Assert( parent );

	int c = parent->m_Subdirectories.Count();
	for ( int i = 0; i < c; ++i )
	{
		MP3Dir_t *sub = parent->m_Subdirectories[ i ];
		if ( !Q_stricmp( sub->m_DirName.String(), dirname ) )
		{
			return sub;
		}
	}

	// Add a new subdir
	MP3Dir_t *sub = new MP3Dir_t();
	sub->m_DirName = dirname;
	char fullpath[ 512 ];
	if ( !parent->m_FullDirPath.String()[0] )
	{
		Q_snprintf( fullpath, sizeof( fullpath ), "%s", dirname );
	}
	else
	{
		Q_snprintf( fullpath, sizeof( fullpath ), "%s\\%s", parent->m_FullDirPath.String(), dirname );
	}
	sub->m_FullDirPath = fullpath;
	parent->AddSubDirectory( sub );

	return sub;
}

int CMP3Player::AddSplitFileToDirectoryTree_R( int songIndex, MP3Dir_t *parent, CUtlVector< CUtlSymbol >& splitList, int level )
{
	char const *current = splitList[ level ].String();
	if ( !current )
	{
		return -1;
	}

	if ( level == splitList.Count() -1 )
	{
		// It's a filename, add if not already in list
		if ( songIndex != -1 &&
			 parent->m_FilesInDirectory.Find( songIndex ) == parent->m_FilesInDirectory.InvalidIndex() )
		{
			parent->m_FilesInDirectory.AddToTail( songIndex );
		}
		return songIndex;
	}

	// It's a directory
	MP3Dir_t *subdir = FindOrAddSubdirectory( parent, current );
	return AddSplitFileToDirectoryTree_R( songIndex, subdir, splitList, level + 1 );
}

int CMP3Player::AddFileToDirectoryTree( SoundDirectory_t *dir, char const *relative )
{
	// AddSong
	int songIndex = AddSong( relative, dir->GetIndex() );

	CUtlVector< CUtlSymbol > list;
	SplitFile( list, relative );

	return AddSplitFileToDirectoryTree_R( songIndex, dir->m_pTree, list, 0 );
}

void CMP3Player::RecursiveFindMP3Files( SoundDirectory_t *root, char const *current, char const *pathID )
{
	FileFindHandle_t fh;

#if 0
	if ( m_nFilesAdded >= 200 )
		return;
#endif

	char path[ 512 ];
	if ( current[ 0 ] )
	{
        Q_snprintf( path, sizeof( path ), "%s/*.*", current );
	}
	else
	{
		Q_snprintf( path, sizeof( path ), "*.*" );
	}

	Q_FixSlashes( path );

	char const *fn = g_pFullFileSystem->FindFirstEx( path, pathID, &fh );
	if ( fn )
	{
		do
		{
			if ( fn[0] != '.' && Q_strnicmp( fn, "_mp3", 4 ) )
			{
				if ( g_pFullFileSystem->FindIsDirectory( fh ) )
				{
					char nextdir[ 512 ];
					if ( current[ 0 ] )
					{
						Q_snprintf( nextdir, sizeof( nextdir ), "%s/%s", current, fn );
					}
					else
					{
						Q_snprintf( nextdir, sizeof( nextdir ), "%s", fn );
					}

					RecursiveFindMP3Files( root, nextdir, pathID );
				}
				else
				{
					char ext[ 10 ];
					Q_ExtractFileExtension( fn, ext, sizeof( ext ) );

					if ( !Q_stricmp( ext, "mp3" ) )
					{
						char relative[ 512 ];
						if ( root->m_bGameSound )
						{
							Q_snprintf( relative, sizeof( relative ), "%s/%s", current + Q_strlen( SOUND_ROOT"/" ), fn );
						}
						else
						{
							if ( current[ 0 ] )
							{
								Q_snprintf( relative, sizeof( relative ), "%s/%s", current, fn );
							}
							else
							{
								Q_snprintf( relative, sizeof( relative ), "%s", fn );
							}
						}
						Msg( "Found '%s/%s'\n", current, fn );

						Q_FixSlashes( relative );
						++m_nFilesAdded;
						AddFileToDirectoryTree( root, relative );
					}
				}
			}

			fn = g_pFullFileSystem->FindNext( fh );

		} while ( fn );

		g_pFullFileSystem->FindClose( fh );
	}
}

int CMP3Player::FindSong( char const *relative )
{
	Assert( !Q_stristr( relative, "/" ) );

	FileNameHandle_t handle = g_pFullFileSystem->FindOrAddFileName( relative );
	int c = m_Files.Count();
	for ( int i = 0 ; i < c ; ++i )
	{
		const MP3File_t& mp3 = m_Files[ i ];
		if ( mp3.filename == handle )
		{
			return i;
		}
	}
	return -1;
}

int CMP3Player::AddSong( char const *relative, int dirnum )
{
	int songIndex = FindSong( relative );
	
	if ( songIndex == -1 )
	{
#if 0
		if ( m_Files.Count() >= 200 )
			return -1;
#endif

		MP3File_t mp3;

		Assert( !Q_stristr( relative, "/" ) );

		mp3.filename = g_pFullFileSystem->FindOrAddFileName( relative );

		char shortname[ 256 ];
		Q_FileBase( relative, shortname, sizeof( shortname ) );
		Q_SetExtension( shortname, ".mp3", sizeof( shortname ) );
		mp3.shortname = shortname;
		mp3.flags = ( dirnum == 0 ) ? MP3File_t::FLAG_FROMGAME : MP3File_t::FLAG_FROMFS;
		mp3.dirnum = dirnum;
		songIndex = m_Files.AddToTail( mp3 );

		m_bDirty = true;
	}
	
	return songIndex;
}

void CMP3Player::RecursiveAddToTree( MP3Dir_t *current, int parentIndex )
{
	if ( !current )
	{
		return;
	}

	// Add all files at current level and then recurse through any directories
	int i, c;
	c = current->m_Subdirectories.Count();
	for ( i = 0 ; i < c; ++i )
	{
		MP3Dir_t *sub = current->m_Subdirectories[ i ];
		Assert( sub );
		
		KeyValues *kv = new KeyValues( "TVI" );
		kv->SetString( "Text", sub->m_DirName.String() );
		kv->SetPtr( "MP3Dir", sub );

		int index = m_pTree->AddItem( kv, parentIndex );
		m_pTree->SetItemFgColor( index, TREE_TEXT_COLOR );

		// Recurse...
		RecursiveAddToTree( sub, index );
	}

	// Add raw files
	c = current->m_FilesInDirectory.Count();
	for ( i = 0; i < c; ++i )
	{
		MP3File_t *song = &m_Files[ current->m_FilesInDirectory[ i ] ];

		KeyValues *kv = new KeyValues( "TVI" );

		kv->SetString( "Text", song->shortname.String() );
		kv->SetInt( "SongIndex", current->m_FilesInDirectory[ i ] );

		int index = m_pTree->AddItem( kv, parentIndex );
		m_pTree->SetItemFgColor( index, TREE_TEXT_COLOR );
	}
}

void CMP3Player::OnTreeViewItemSelected()
{
	PopulateLists();
}

void CMP3Player::PopulateTree()
{
	m_pTree->RemoveAll();

	// Now populate tree
	KeyValues *kv = new KeyValues( "TVI" );
	kv->SetString( "Text", "Songs" );

	int rootIndex = m_pTree->AddItem( kv, -1 );
	m_pTree->SetItemFgColor( rootIndex, TREE_TEXT_COLOR );

	int dircount = m_SoundDirectories.Count();
	for ( int dirnum = 0; dirnum < dircount; ++dirnum )
	{
		MP3Dir_t *tree = m_SoundDirectories[ dirnum ]->m_pTree;
		if ( !tree )
			continue;

		char const *dirname = tree->m_DirName.String();

		kv = new KeyValues( "TVI" );
		kv->SetString( "Text", dirname );

		int index = m_pTree->AddItem( kv, rootIndex );

		RecursiveAddToTree( tree, index );

		m_pTree->SetItemFgColor( index, TREE_TEXT_COLOR );
	}

	m_pTree->ExpandItem( rootIndex, true );

	PopulateLists();
}

// Instead of including windows.h
extern "C"
{
	extern int __stdcall CopyFileA( char *pszSource, char *pszDest, int bFailIfExists );
};

void CMP3Player::GetLocalCopyOfSong( const MP3File_t &mp3, char *outsong, size_t outlen )
{
	outsong[ 0 ] = 0;
	char fn[ 512 ];
	if ( !g_pFullFileSystem->String( mp3.filename, fn, sizeof( fn ) ) )
	{
		return;
	}

	if ( mp3.flags == MP3File_t::FLAG_FROMGAME )
	{
		Q_FixSlashes( fn );
		Q_strncpy( outsong, fn, outlen );
		return;
	}

	// Get temp filename from crc
	CRC32_t crc;
	CRC32_Init( &crc );
	CRC32_ProcessBuffer( &crc, fn, Q_strlen( fn ) );
	CRC32_Final( &crc );

	char hexname[ 16 ];
	Q_binarytohex( (const byte *)&crc, sizeof( crc ), hexname, sizeof( hexname ) );

	char hexfilename[ 512 ];
	Q_snprintf( hexfilename, sizeof( hexfilename ), "sound/_mp3/%s.mp3", hexname );

	Q_FixSlashes( hexfilename );

	if ( g_pFullFileSystem->FileExists( hexfilename, "MOD" ) )
	{
		Q_snprintf( outsong, outlen, "_mp3/%s.mp3", hexname );
	}
	else
	{
		// Make a local copy
		char mp3_temp_path[ 512 ];
		Q_snprintf( mp3_temp_path, sizeof( mp3_temp_path ), "sound/_mp3" );
		g_pFullFileSystem->CreateDirHierarchy( mp3_temp_path, "MOD" );

		char destpath[ 512 ];
		Q_snprintf( destpath, sizeof( destpath ), "%s/%s", engine->GetGameDirectory(), hexfilename );
		Q_FixSlashes( destpath );

		char sourcepath[ 512 ];

		Assert( mp3.dirnum >= 0 && mp3.dirnum < m_SoundDirectories.Count() );
		SoundDirectory_t *sdir = m_SoundDirectories[ mp3.dirnum ];
		Q_snprintf( sourcepath, sizeof( sourcepath ), "%s/%s", sdir->m_Root.String(), fn );
		Q_FixSlashes( sourcepath );

		// !!!HACK HACK:
		// Total hack right now, using windows OS calls to copy file to full destination
		int success = ::CopyFileA( sourcepath, destpath, TRUE );
		if ( success > 0 )
		{
			Q_snprintf( outsong, outlen, "_mp3/%s.mp3", hexname );
		}
	}

	Q_FixSlashes( outsong );
}

void CMP3Player::PlaySong( int songIndex, float skipTime /*= 0.0f */ ) 
{
	MP3File_t& song = m_Files[ songIndex ];

	float volume = 1.0f;

	char soundname[ 512 ];

	soundname[ 0 ] = 0;

	if ( song.playbackfilename == (FileNameHandle_t)0 )
	{
		GetLocalCopyOfSong( song, soundname, sizeof( soundname ) );
		if ( !soundname[ 0 ] )
		{
			return;
		}

		Assert( !Q_stristr( soundname, "/" ) );
		song.playbackfilename = g_pFullFileSystem->FindOrAddFileName( soundname );


	}
	else
	{
		if ( !g_pFullFileSystem->String( song.playbackfilename, soundname, sizeof( soundname ) ) )
		{
			return;
		}
	}

	// Msg( "Playing '%s'\n", soundname );

	if ( !soundname[ 0 ] )
	{
		return;
	}

	if ( m_bPlaying )
	{
		OnStop();
	}

	char drymix[ 512 ];
	Q_snprintf( drymix, sizeof( drymix ), "#%s", soundname );

	enginesound->EmitAmbientSound(
		drymix, 
		volume,
		PITCH_NORM,
		0,
		skipTime == 0.0f ? 0.0f : ( gpGlobals->curtime + skipTime  ) ); 

	m_nSongGuid = enginesound->GetGuidForLastSoundEmitted();

	m_nCurrentSong = songIndex;
	m_bPlaying = true;
	m_LastSong = song.playbackfilename;
	m_SongStart = gpGlobals->realtime;
	m_flSongDuration = GetMP3Duration( soundname );

	m_pCurrentSong->SetText( song.shortname.String() );
	
	m_nSongMinutes = (int)( m_flSongDuration / 60.0f );
	m_nSongSeconds = (int)( m_flSongDuration - (float)( m_nSongMinutes * 60 ) );

	char durationstr[ 256 ];
	Q_snprintf( durationstr, sizeof( durationstr ), "0:00 / %i:%02i", m_nSongMinutes, m_nSongSeconds );

	m_pDuration->SetText( durationstr );
	
	m_pSongProgress->SetProgress( 0.0f );
}

void CMP3Player::OnStop()
{
	if ( m_bPlaying )
	{
		m_bPlaying = false;

		if ( m_nSongGuid != 0 )
		{
			enginesound->StopSoundByGuid( m_nSongGuid );
			m_nSongGuid = 0;
		}

		m_LastSong = (FileNameHandle_t)0;
		m_pCurrentSong->SetText( "#NoSong" );
		m_pSongProgress->SetProgress( 0.0f );
		m_pDuration->SetText( "" );
	}
}

float CMP3Player::GetMP3Duration( char const *songname )
{
	return enginesound->GetSoundDuration( songname );
}

void CMP3Player::SelectedSongs( SongListSource_t from, CUtlVector< int >& songIndexList )
{
	m_SelectedSongs.RemoveAll();
	m_SelectionFrom = from;

	int i, c;
	c = songIndexList.Count();
	for ( i = 0; i < c; ++i )
	{
		m_SelectedSongs.AddToTail( songIndexList[ i ] );
	}
}

void CMP3Player::OnPlay()
{
	int c = m_SelectedSongs.Count();

	for ( int i = 0 ; i < c; ++i )
	{
		int songIndex = m_SelectedSongs[ i ];
		if ( songIndex < 0 || songIndex >= m_Files.Count() )
		{
			continue;
		}
		if ( m_SelectionFrom == SONG_FROM_PLAYLIST )
		{
			// Can only play one song at a time from playlist...
			PlaySong( songIndex );
			break;
		}
		else
		{
			AddToPlayList( songIndex, i == 0 );
		}
	}
}

void CMP3Player::OnTick()
{
	BaseClass::OnTick();

	if ( !m_bPlaying )
	{
		return;
	}

	float newVol = (float)m_pVolume->GetValue() / 100.0f;

	bool volumeChanged = ( newVol != m_flCurrentVolume );
	if ( volumeChanged )
	{
		m_flCurrentVolume = newVol;
	}
	bool muteChanged = m_bMuted != m_pMute->IsSelected();
	if ( muteChanged )
	{
		m_bMuted = m_pMute->IsSelected();
	}

	if ( m_nSongGuid == 0 )
	{
		return;
	}

	bool playing = enginesound->IsSoundStillPlaying( m_nSongGuid );
	if ( playing )
	{
		if ( muteChanged )
		{
			if ( m_bMuted )
			{
				OnChangeVolume( MUTED_VOLUME );
			}
			else
			{
				OnChangeVolume( m_flCurrentVolume );
			}
		}

		if ( volumeChanged )
		{
			// Msg( "set volume %f\n", m_flCurrentVolume );
			OnChangeVolume( m_flCurrentVolume );
		}

		if ( m_flSongDuration >= 0.001f )
		{
			float elapsed = gpGlobals->realtime - m_SongStart;

			float frac = elapsed / m_flSongDuration;
			frac = clamp( frac, 0.0f, 1.0f );
			m_pSongProgress->SetProgress( frac );

			int minutes = ( int ) ( elapsed / 60.0f );
			int seconds = (int)( elapsed - ( 60 * minutes ) );
			char durationstr[ 256 ];
			Q_snprintf( durationstr, sizeof( durationstr ), "%i:%02i / %i:%02i", minutes, seconds, m_nSongMinutes, m_nSongSeconds );

			m_pDuration->SetText( durationstr );
		}
		return;
	}

	if ( !m_bEnableAutoAdvance )
	{
		// If we got disconnected completely, reset the flag
		if ( !engine->IsConnected() )
		{
			m_bEnableAutoAdvance = true;
		}
		return;
	}

	// No song playing...
	m_nSongGuid = 0;
	OnNextTrack();
}

void CMP3Player::OnChangeVolume( float newVol )
{
	if ( !m_bPlaying )
	{
		return;
	}

	if ( !m_nSongGuid )
		return;

	enginesound->SetVolumeByGuid( m_nSongGuid, newVol );
}

void CMP3Player::GoToNextSong( int skip )
{
	bool shuffle = m_pShuffle->IsSelected();

	int nextSong = 0;

	if ( m_PlayList.Count() > 0 )
	{
		if ( shuffle )
		{
			m_nCurrentPlaylistSong = random->RandomInt( 0, m_PlayList.Count() - 1 );
		}
		else
		{
			m_nCurrentPlaylistSong = ( m_nCurrentPlaylistSong + skip ) % m_PlayList.Count();
			if ( m_nCurrentPlaylistSong < 0 )
			{
				m_nCurrentPlaylistSong = m_PlayList.Count() - 1;
			}
		}
		nextSong = m_PlayList[ m_nCurrentPlaylistSong ];

		m_pFileSheet->OnPlayListItemPlaying( m_nCurrentPlaylistSong );
	}
	else
	{
		if ( shuffle )
		{
			nextSong = random->RandomInt( 0, m_Files.Count() - 1 );
		}
		else
		{
			nextSong = ( m_nCurrentSong + skip ) % m_Files.Count();
			if ( nextSong < 0 )
			{
				nextSong = m_Files.Count() - 1;
			}
		}
	}

	PlaySong( nextSong );
}

void CMP3Player::OnNextTrack()
{
	if ( m_Files.Count() == 0 )
	{
		return;
	}

	GoToNextSong( +1 );
}

void CMP3Player::OnPrevTrack()
{
	if ( m_Files.Count() == 0 )
	{
		return;
	}

	GoToNextSong( -1 );
}

void CMP3Player::RemoveFSSongs()
{
	int c = m_Files.Count();
	for ( int i = c - 1; i >= 0; --i )
	{
		MP3File_t& mp3 = m_Files[ i ];
		if ( mp3.flags == MP3File_t::FLAG_FROMGAME )
			continue;

		m_Files.Remove( i );
	}
}

int CMP3Player::FindSoundDirectory( char const *fullpath )
{
	CUtlSymbol sym = fullpath;
	int c = m_SoundDirectories.Count();
	for ( int i = 0; i < c; ++i )
	{
		if ( sym == m_SoundDirectories[ i ]->m_Root )
			return i;
	}

	return m_SoundDirectories.InvalidIndex();
}

SoundDirectory_t *CMP3Player::AddSoundDirectory( char const *fullpath, bool recurse )
{
	// RemoveFSSongs();
	m_bDirty = true;
	m_bSettingsDirty = true;

	CUtlSymbol sym = fullpath;
	int sdi = FindSoundDirectory( fullpath );
	if ( sdi == m_SoundDirectories.InvalidIndex() )
	{
		SoundDirectory_t *sounddir = new SoundDirectory_t( m_SoundDirectories.Count() );
		sounddir->m_bGameSound = false;
		sounddir->m_Root = sym;
		sounddir->m_pTree = new MP3Dir_t();
		sounddir->m_pTree->m_DirName = fullpath;
		sounddir->m_pTree->m_FullDirPath = fullpath;
	
		sdi = m_SoundDirectories.AddToTail( sounddir );

		// Add to search path
		g_pFullFileSystem->AddSearchPath( fullpath, "MP3" );
		// Don't pollute regular searches...
		g_pFullFileSystem->MarkPathIDByRequestOnly( "MP3", true );

		// Now enumerate all .mp3 files in subfolders of this
		if ( recurse )
		{
			m_nFilesAdded = 0;
			RecursiveFindMP3Files( sounddir, "", "MP3" );
		}
	}

	return m_SoundDirectories[ sdi ];
}

void CMP3Player::PopulateLists()
{
	CUtlVector< KeyValues * > kv;
	m_pTree->GetSelectedItemData( kv );
	if ( !kv.Count() )
	{
		return;
	}

	MP3Dir_t *dir = static_cast< MP3Dir_t * >( kv[ 0 ]->GetPtr( "MP3Dir", 0 ) );
	if ( !dir )
	{
		return;
	}

	int i, c;
	c = dir->m_FilesInDirectory.Count();
	if ( !c )
	{
		return;
	}

	m_pFileSheet->ResetFileList();
	for ( i = 0; i < c ; ++i )
	{
		m_pFileSheet->AddSongToFileList( dir->m_FilesInDirectory[ i ] );
	}
}

MP3File_t *CMP3Player::GetSongInfo( int songIndex )
{
	if ( songIndex < 0 || songIndex >= m_Files.Count() )
	{
		return NULL;
	}
	return &m_Files[ songIndex ];
}

void CMP3Player::AddToPlayList( int songIndex, bool playNow )
{
	m_pFileSheet->AddSongToPlayList( songIndex );
	m_PlayList.AddToTail( songIndex );

	if ( playNow )
	{
		PlaySong( songIndex );
		SetPlayListSong( m_PlayList.Count() - 1 );
	}

	// refresh the playlist
}

void CMP3Player::RemoveFromPlayList( int songIndex )
{
	m_pFileSheet->RemoveSongFromPlayList( songIndex );
	m_PlayList.FindAndRemove( songIndex );

	SetPlayListSong( m_nCurrentPlaylistSong );
}

void CMP3Player::ClearPlayList()
{
	m_pFileSheet->ResetPlayList();
	m_PlayList.RemoveAll();
	m_nCurrentPlaylistSong = 0;
}

void CMP3Player::OnLoadPlayList()
{
	ShowFileOpenDialog( false );
}

void CMP3Player::OnSavePlayList()
{
	if ( UTL_INVAL_SYMBOL == m_PlayListFileName )
	{
		OnSavePlayListAs();
		return;
	}

	SavePlayList( m_PlayListFileName.String() );
}

void CMP3Player::OnSavePlayListAs()
{
	ShowFileOpenDialog( true );
}

void CMP3Player::RestoreSongs( KeyValues *songs )
{
	Assert( m_Files.Count() == 0 );

	for ( KeyValues *song = songs->GetFirstSubKey(); song != NULL; song = song->GetNextKey() )
	{
		int flags = 0;
		int game = song->GetInt( "fromgame", 0 );
		if ( game )
		{
			flags |= MP3File_t::FLAG_FROMGAME;
		}
		int fs = song->GetInt( "fromfs", 0 );
		if ( fs )
		{
			flags |= MP3File_t::FLAG_FROMFS;
		}

		int subdir = song->GetInt( "subdirindex", 0 );

		char shortname[ 512 ];
		char filename[ 512 ];

		Q_strncpy( shortname, song->GetString( "short", "" ), sizeof( shortname ) );
		Q_strncpy( filename, song->GetString( "filename", "" ), sizeof( filename ) );

		MP3File_t file;
		file.dirnum = subdir;
		file.flags = flags;
		file.shortname = shortname;
		file.filename = g_pFullFileSystem->FindOrAddFileName( filename );
		m_Files.AddToTail( file );
	}
}

void CMP3Player::RestoreDirectory( KeyValues *dir, SoundDirectory_t *sd )
{
	for ( KeyValues *kv = dir->GetFirstSubKey(); kv; kv = kv->GetNextKey() )
	{
		if ( !Q_stricmp( kv->GetName(), "name" ) )
		{
			sd->m_Root = kv->GetString();
		}
		else if ( !Q_stricmp( kv->GetName(), "gamesounds" ) )
		{
			sd->m_bGameSound = kv->GetInt() ? true : false;
		}
		else if ( !Q_stricmp( kv->GetName(), "dirname" ) )
		{
			sd->m_pTree->m_DirName = kv->GetString();
		}
		else if ( !Q_stricmp( kv->GetName(), "fullpath" ) )
		{
			sd->m_pTree->m_FullDirPath = kv->GetString();
		}
		else if ( !Q_stricmp( kv->GetName(), "files" ) )
		{
			for ( KeyValues *f = kv->GetFirstSubKey(); f != NULL; f = f->GetNextKey() )
			{
				if ( !Q_stricmp( f->GetName(), "file" ) )
				{
					int songIndex = f->GetInt();
					if ( songIndex >= 0 && songIndex < m_Files.Count() )
					{
						char fn[ 512 ];
						if ( g_pFullFileSystem->String( m_Files[ songIndex ].filename, fn, sizeof( fn ) ) )
						{
							AddFileToDirectoryTree( sd, fn );
						}
					}
				}
			}
		}
		else
		{
			Warning( "Unknown key '%s'\n", kv->GetName() );
		}
	}
}

void CMP3Player::RestoreDirectories( KeyValues *dirs )
{
	for ( KeyValues *dir = dirs->GetFirstSubKey(); dir != NULL; dir = dir->GetNextKey() )
	{
		char const *dirpath = dir->GetString( "fullpath", "" );
		if ( dirpath  )
		{
			int sdi = FindSoundDirectory( dirpath );
			if ( sdi == m_SoundDirectories.InvalidIndex() )
			{
				SoundDirectory_t *sd = AddSoundDirectory( dirpath, false );
				sdi = sd->GetIndex();
			}
			RestoreDirectory( dir, m_SoundDirectories[ sdi ] );
		}
	}
}

bool CMP3Player::RestoreDb( char const *filename )
{
	KeyValues *kv = new KeyValues( "db" );
	Assert( kv );
	if ( !kv->LoadFromFile( g_pFullFileSystem, filename, "MOD" ) )
	{
		Warning( "Unable to load '%s'\n", filename );
		return false;
	}

	KeyValues *songs = kv;

	Assert( !Q_stricmp( songs->GetName(), "songs" ) );
	RestoreSongs( songs );
	KeyValues *dirs = songs->GetNextKey();
	Assert( !Q_stricmp( dirs->GetName(), "directories" ) );
	RestoreDirectories( dirs );

	kv->deleteThis();

	return true;
}

void bpr( int level, CUtlBuffer& buf, char const *fmt, ... )
{
	char txt[ 4096 ];
	va_list argptr;
	va_start( argptr, fmt );
	_vsnprintf( txt, sizeof( txt ) - 1, fmt, argptr );
	va_end( argptr );

	int indent = 2;
	for ( int i = 0; i < ( indent * level ); ++i )
	{
		buf.Printf( " " );
	}
	buf.Printf( "%s", txt );
}

void CMP3Player::SaveDbFile( int level, CUtlBuffer& buf, MP3File_t *file, int filenumber )
{
	bpr( level, buf, "file\n" );
	bpr( level, buf, "{\n" );

	bpr( level + 1, buf, "filenumber %i\n", filenumber );

	if ( file->flags & MP3File_t::FLAG_FROMGAME )
	{
		bpr( level + 1, buf, "fromgame 1\n" );
	}
	if ( file->flags & MP3File_t::FLAG_FROMFS )
	{
		bpr( level + 1, buf, "fromfs 1\n" );
	}

	bpr( level + 1, buf, "subdirindex %i\n", file->dirnum );

	bpr( level + 1, buf, "short \"%s\"\n", file->shortname.String() );
	char fn[ 512 ];
	if ( g_pFullFileSystem->String( file->filename, fn, sizeof( fn ) ) )
	{
		bpr( level + 1, buf, "filename \"%s\"\n", fn );
	}
	else
	{
		Assert( 0 );
	}

	bpr( level, buf, "}\n" );
}

void CMP3Player::FlattenDirectoryFileList_R( MP3Dir_t *dir, CUtlVector< int >& list )
{
	int i, c;
	c = dir->m_FilesInDirectory.Count();
	for ( i = 0; i < c; ++i )
	{
		int songIndex = dir->m_FilesInDirectory[ i ];
		if ( list.Find( songIndex ) == list.InvalidIndex() )
		{
			list.AddToTail( songIndex );
		}
	}

	c = dir->m_Subdirectories.Count();
	for ( i = 0; i < c; ++i )
	{
		FlattenDirectoryFileList_R( dir->m_Subdirectories[ i ], list );
	}
}

void CMP3Player::SaveDbDirectory( int level, CUtlBuffer& buf, SoundDirectory_t *sd )
{
	bpr( level, buf, "directory\n" );
	bpr( level, buf, "{\n" );

	bpr( level + 1, buf, "gamesounds %i\n", sd->m_bGameSound ? 1 : 0 );
	bpr( level + 1, buf, "name \"%s\"\n", sd->m_Root.String() );
	bpr( level + 1, buf, "dirname \"%s\"\n", sd->m_pTree->m_DirName.String() );
	bpr( level + 1, buf, "fullpath \"%s\"\n", sd->m_pTree->m_FullDirPath.String() );

	CUtlVector< int > files;
	if ( sd->m_pTree )
	{
		FlattenDirectoryFileList_R( sd->m_pTree, files );
	}

	int i, c;
	
	c = files.Count();
	if ( c > 0 )
	{
		bpr( level + 1, buf, "files\n" );
		bpr( level + 1, buf, "{\n" );
		for ( i = 0; i < c; ++i )
		{
			bpr( level + 2, buf, "file %i\n", files[ i ] );
		}
		bpr( level + 1, buf, "}\n" );
	}

	bpr( level, buf, "}\n" );
}

void CMP3Player::SaveDb( char const *filename )
{
	int i, c;

	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );

	buf.Printf( "// mp3 database, automatically generated\n" );

	bpr( 0, buf, "songs\n{\n" );

	c = m_Files.Count();
	for ( i = 0; i < c; ++i )
	{
		MP3File_t* file = &m_Files[ i ];
		SaveDbFile( 1, buf, file, i );
	}

	bpr( 0, buf, "}\n" );

	bpr( 0, buf, "directories\n{\n" );

	c = m_SoundDirectories.Count();
	for ( i = 0; i < c; ++i )
	{
		SoundDirectory_t *sd = m_SoundDirectories[ i ];

		if ( sd->m_pTree )
		{
			SaveDbDirectory( 1, buf, sd );
		}
	}

	bpr( 0, buf, "}\n" );

	FileHandle_t fh = g_pFullFileSystem->Open( filename, "wb" );
	if ( FILESYSTEM_INVALID_HANDLE != fh )
	{
		g_pFullFileSystem->Write( buf.Base(), buf.TellPut(), fh );
		g_pFullFileSystem->Close( fh );
		m_bDirty = false;
	}
	else
	{
		Warning( "Unable to open '%s' for writing\n", filename );
	}
}

void CMP3Player::OnSave()
{
	SaveDb( DB_FILENAME );
}

void CMP3Player::OnSliderMoved()
{
	if ( !m_bPlaying )
	{
		return;
	}

// The engine only allows 4 seconds of skip ahead right now and you have to be connected to get it to work
//  until this is relaxed we can't do this this way...
#if 0
	float frac = (float)m_pSongProgress->GetValue() / 100.0f;

	float offset = frac * m_flSongDuration;
	PlaySong( m_nCurrentSong, -offset );
#endif
}

void CMP3Player::LoadPlayList( char const *filename )
{
	KeyValues *kv = new KeyValues( "playlist" );
	Assert( kv );
	if ( !kv->LoadFromFile( g_pFullFileSystem, filename, "MOD" ) )
	{
		Warning( "Unable to load '%s'\n", MP3_SETTINGS_FILE );
		return;
	}

	m_PlayListFileName = filename;

	for ( KeyValues *song = kv->GetFirstSubKey(); song != NULL; song = song->GetNextKey() )
	{
		if ( !Q_stricmp( song->GetName(), "song" ) )
		{
			char const *songname = song->GetString( "relativepath" );
			if ( !songname || !songname[ 0 ] )
				continue;

			char const *dirname = song->GetString( "directory" );
			int flags = 0;
			int game = song->GetInt( "fromgame", 0 );
			if ( game )
			{
				flags |= MP3File_t::FLAG_FROMGAME;
			}
			int fs = song->GetInt( "fromfs", 0 );
			if ( fs )
			{
				flags |= MP3File_t::FLAG_FROMFS;
			}

			int songIndex = -1;

			// Find index
			int idx = FindSong( songname );
			if ( idx == -1 )
			{
				// See if directory exists...
				if ( flags & MP3File_t::FLAG_FROMGAME )
				{
					songIndex = AddSong( songname, 0 );
				}
				else if ( dirname )
				{
					SoundDirectory_t *sd = NULL;
					int dirnum = FindSoundDirectory( dirname );
					if ( dirnum == -1 )
					{
						sd = AddSoundDirectory( dirname, false );
					}
					else
					{
						sd = m_SoundDirectories[ dirnum ];
					}

					if ( sd )
					{
						songIndex = AddFileToDirectoryTree( sd, songname );
					}
				}
			}
			else
			{
				songIndex = idx;
			}

			if ( songIndex >= 0 )
			{
				m_PlayList.AddToTail( songIndex );
			}
		}
	}

	kv->deleteThis();

	PopulateTree();
	PopulateLists();
}

void CMP3Player::SavePlayList( char const *filename )
{
	FileHandle_t fh = g_pFullFileSystem->Open( filename, "wb" );
	if ( FILESYSTEM_INVALID_HANDLE != fh )
	{
		m_PlayListFileName = filename;

		CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );

		buf.Printf( "// mp3 playlist\n" );

		bpr( 0, buf, "playlist\n{\n" );

		int c = m_PlayList.Count();
		for ( int i = 0; i < c; ++i )
		{
			MP3File_t& song = m_Files[ m_PlayList[ i ] ];
			char fn[ 512 ];
			if ( g_pFullFileSystem->String( song.filename, fn, sizeof( fn ) ) )
			{
				char dirname[ 512 ];
				dirname[0]=0;
				if ( song.dirnum >= 0 )
				{
					SoundDirectory_t *sd = m_SoundDirectories[ song.dirnum ];

					Q_strncpy( dirname, sd->m_Root.String(), sizeof( dirname ) );
				}

				bpr( 1, buf, "song\n" );
				{
					bpr( 2, buf, "%s 1\n", song.flags == MP3File_t::FLAG_FROMFS ? "fromfs" : "fromgame" );

					if ( dirname[0])
					{
						bpr( 2, buf, "directory \"%s\"\n", dirname );
					}
					bpr( 2, buf, "relativepath \"%s\"\n", fn );
				}
				bpr( 1, buf, "\n" );
			}
		}

		bpr( 0, buf, "}\n" );

		g_pFullFileSystem->Close( fh );

		SetMostRecentPlayList( filename );
	}
}

void CMP3Player::SetMostRecentPlayList( char const *filename )
{
	m_PlayListFileName = filename;
	m_bSettingsDirty = true;
}

void CMP3Player::LoadSettings()
{
	KeyValues *kv = new KeyValues( "settings" );
	Assert( kv );
	if ( !kv->LoadFromFile( g_pFullFileSystem, MP3_SETTINGS_FILE, "MOD" ) )
	{
		Warning( "Unable to load '%s'\n", MP3_SETTINGS_FILE );
		return;
	}

	char const *filename = kv->GetString( "mostrecentplaylist", "" );
	if ( filename && filename[ 0 ] )
	{
		LoadPlayList( filename );
	}

	KeyValues *dirs = kv->FindKey( "directories", false );
	if ( dirs )
	{
		for ( KeyValues *sub = dirs; sub ; sub = sub->GetNextKey() )
		{
			char const *dirname = sub->GetString( "dirname", "" );
			if ( dirname && dirname[ 0 ] )
			{
				AddSoundDirectory( dirname, false ); 
			}
			else if ( dirname )
			{
				AddGameSounds( false );
			}
		}
	}

	kv->deleteThis();

	m_bSettingsDirty = false;
}

void CMP3Player::ShowFileOpenDialog( bool saving )
{
	m_bSavingFile = saving;
	if ( m_hSaveLoadPlaylist.Get() )
	{
		m_hSaveLoadPlaylist.Get()->MarkForDeletion();
	}

	m_hSaveLoadPlaylist = new FileOpenDialog( this, "Choose Playlist", !saving );
	if ( m_hSaveLoadPlaylist.Get() )
	{
		m_hSaveLoadPlaylist->SetStartDirectory( "resource/" );
		m_hSaveLoadPlaylist->AddFilter( "*.txt", "Playlists", true );
		m_hSaveLoadPlaylist->DoModal( m_bSavingFile );
	}
}

void CMP3Player::OnFileSelected( char const *fullpath )
{
	if ( m_bSavingFile )
	{
		SavePlayList( fullpath );
		m_PlayListFileName = fullpath;
	}
	else
	{
		m_PlayListFileName = fullpath;
		LoadPlayList( fullpath );
		m_nCurrentPlaylistSong = 0;
	}

	if ( m_hSaveLoadPlaylist.Get() )
	{
		m_hSaveLoadPlaylist.Get()->MarkForDeletion();
	}
}


void CMP3Player::ShowDirectorySelectDialog()
{
	if ( m_hDirectorySelect.Get() )
	{
		m_hDirectorySelect.Get()->MarkForDeletion();
	}

	m_hDirectorySelect = new DirectorySelectDialog( this, "Choose Directory" );
	if ( m_hDirectorySelect.Get() )
	{
		m_hDirectorySelect->MakeReadyForUse();
		m_hDirectorySelect->SetStartDirectory( MP3_DEFAULT_MP3DIR );
		m_hDirectorySelect->SetFgColor( TREE_TEXT_COLOR );
		m_hDirectorySelect->DoModal();
	}
}

void CMP3Player::OnDirectorySelected( KeyValues *params )
{
	if ( m_hDirectorySelect.Get() )
	{
		m_hDirectorySelect.Get()->MarkForDeletion();
	}

	char const *fullpath = params->GetString( "dir", "" );
	if ( fullpath && fullpath[ 0 ] )
	{
		char dir[ 512 ];
		Q_strncpy( dir, fullpath, sizeof( dir ) );
		Q_StripTrailingSlash( dir );
		AddSoundDirectory( dir, true );
		PopulateTree();
		m_bDirty = true;
	}
}

void CMP3Player::SaveSettings()
{
	if ( !m_bSettingsDirty )
	{
		return;
	}

	m_bSettingsDirty = false;

	FileHandle_t fh = g_pFullFileSystem->Open( MP3_SETTINGS_FILE, "wb" );
	if ( FILESYSTEM_INVALID_HANDLE != fh )
	{
		CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );

		buf.Printf( "// mp3 settings, automatically generated\n" );

		bpr( 0, buf, "settings\n{\n" );

		// FIXME:  Move to function if there are more settings to save...
		if ( UTL_INVAL_SYMBOL != m_PlayListFileName )
		{
			bpr( 1, buf, "mostrecentplaylist \"%s\"\n", m_PlayListFileName.String() );
		}

		int c;
		c = m_SoundDirectories.Count();
		if ( c > 0 )
		{
			bpr( 1, buf, "directories\n" );
			bpr( 1, buf, "{\n" );

			for ( int i = 0; i < c; ++i )
			{
				SoundDirectory_t *sd = m_SoundDirectories[ i ];
				Assert( sd );

				bpr( 2, buf, "dirname \"%s\"\n", sd->m_Root.String() );
			}

			bpr( 1, buf, "}\n" );
		}

		bpr( 0, buf, "}\n" );

		g_pFullFileSystem->Write( buf.Base(), buf.TellPut(), fh );
		g_pFullFileSystem->Close( fh );
	}
}

void CMP3Player::SetPlayListSong( int listIndex )
{
	if ( m_PlayList.Count() > 0 )
	{
        m_nCurrentPlaylistSong = listIndex % m_PlayList.Count();
	}
	else
	{
		m_nCurrentPlaylistSong = 0;
	}
}

void CMP3Player::EnableAutoAdvance( bool state )
{
	m_bEnableAutoAdvance = state;
}

//-----------------------------------------------------------------------------
// Purpose: The purpose of this is that when a changelevel occurs, the engine calls
//  StopAllSounds several times, and the OnTick handler thinks the song has finished playing 
//  and so it moves to the next song.  This causes the play list to skip ahead by > 1 song during a level
//  change.
//-----------------------------------------------------------------------------
class CMP3PlayerGameSystem : public CAutoGameSystem
{
public:
	CMP3PlayerGameSystem()
	{
	}

	virtual void LevelInitPreEntity()
	{
		g_pPlayer->EnableAutoAdvance( true );
	}

	virtual void LevelShutdownPostEntity()
	{
		// If we are still connected, disable auto advance until we get into the next level
		if ( engine->IsConnected() )
		{
			g_pPlayer->EnableAutoAdvance( false );
		}
	}
};

static CMP3PlayerGameSystem g_MP3Helper;

#else

void MP3Player_Create( vgui::VPANEL parent )
{
}

void MP3Player_Destroy()
{
}

#endif
