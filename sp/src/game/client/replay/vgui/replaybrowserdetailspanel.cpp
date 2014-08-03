//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replaybrowserdetailspanel.h"
#include "replaybrowsermainpanel.h"
#include "replaybrowseritemmanager.h"
#include "replaybrowsermovieplayerpanel.h"
#include "replaybrowserrenderdialog.h"
#include "vgui/IVGui.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "vgui/ILocalize.h"
#include "vgui_controls/FileOpenDialog.h"
#include "vgui_controls/PanelListPanel.h"
#include "vgui_controls/ScrollBar.h"
#include "vgui_controls/ScrollBarSlider.h"
#include "vgui_controls/TextEntry.h"
#include "vgui_controls/TextImage.h"
#include "vgui_avatarimage.h"
#include "gamestringpool.h"
#include "replay/genericclassbased_replay.h"
#include "replaybrowserlistitempanel.h"
#include "confirm_dialog.h"
#include "replay/ireplaymoviemanager.h"
#include "replay/ireplaymanager.h"
#include "replay/ireplayrenderqueue.h"
#include "replay/screenshot.h"
#include "replay/ireplayperformancemanager.h"
#include "replay/performance.h"
#include "vgui/ISystem.h"
#include "youtubeapi.h"
#include "replay/replayyoutubeapi.h"
#include "ienginevgui.h"
#include <filesystem.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------

extern IClientReplayContext *g_pClientReplayContext;
extern IReplayMovieManager *g_pReplayMovieManager;
extern IReplayPerformanceManager *g_pReplayPerformanceManager;

//-----------------------------------------------------------------------------

ConVar replay_movie_reveal_warning( "replay_movie_reveal_warning", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL | FCVAR_HIDDEN | FCVAR_DONTRECORD );
ConVar replay_movie_export_last_dir( "replay_movie_export_last_dir", "", FCVAR_ARCHIVE | FCVAR_CLIENTDLL | FCVAR_HIDDEN | FCVAR_DONTRECORD );
ConVar replay_renderqueue_first_add( "replay_renderqueue_first_add", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL | FCVAR_HIDDEN | FCVAR_DONTRECORD );

//-----------------------------------------------------------------------------

using namespace vgui;

//-----------------------------------------------------------------------------

class CConfirmDisconnectFromServerDialog : public CConfirmDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmDisconnectFromServerDialog, CConfirmDialog );
public:
	CConfirmDisconnectFromServerDialog( Panel *pParent )
	:	BaseClass( pParent )
	{
		surface()->PlaySound( "replay\\replaydialog_warn.wav" );
	}

	const wchar_t *GetText() { return g_pVGuiLocalize->Find( "#Replay_ConfirmDisconnectFromServer" ); }

	void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		SetTall( YRES( 170 ) );
	}
};

//-----------------------------------------------------------------------------

CKeyValueLabelPanel::CKeyValueLabelPanel( Panel *pParent, const char *pKey, const char *pValue )
:	EditablePanel( pParent, "KeyValueLabelPanel" )
{
	SetScheme( "ClientScheme" );

	m_pLabels[ 0 ] = new CExLabel( this, "KeyLabel", pKey );
	m_pLabels[ 1 ] = new CExLabel( this, "ValueLabel", pValue );
}

CKeyValueLabelPanel::CKeyValueLabelPanel( Panel *pParent, const char *pKey, const wchar_t *pValue )
:	EditablePanel( pParent, "KeyValueLabelPanel" )
{
	SetScheme( "ClientScheme" );

	m_pLabels[ 0 ] = new CExLabel( this, "KeyLabel", pKey );
	m_pLabels[ 1 ] = new CExLabel( this, "ValueLabel", pValue );
}

void CKeyValueLabelPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	HFont hFont = scheme()->GetIScheme( GetScheme() )->GetFont( "ReplayBrowserSmallest", true );

	m_pLabels[ 0 ]->SetFont( hFont );
	m_pLabels[ 1 ]->SetFont( hFont );

	m_pLabels[ 0 ]->SetFgColor( Color( 119, 107, 95, 255 ) );
	m_pLabels[ 1 ]->SetFgColor( Color( 255, 255, 255, 255 ) );
}

void CKeyValueLabelPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	int nContentWidth, nContentHeight;
	m_pLabels[0]->GetContentSize( nContentWidth, nContentHeight );

	int iMidX = GetParent()->GetWide() * 0.55f;
	m_pLabels[0]->SetBounds( 0, 0, iMidX, nContentHeight );

	m_pLabels[1]->GetContentSize( nContentWidth, nContentHeight );
	m_pLabels[1]->SetBounds( iMidX, 0, iMidX, nContentHeight );
}

int CKeyValueLabelPanel::GetHeight() const
{
	int nWidth, nHeight;
	m_pLabels[ 0 ]->GetContentSize( nWidth, nHeight );
	return nHeight;
}

int CKeyValueLabelPanel::GetValueHeight() const
{
	int nWidth, nHeight;
	m_pLabels[ 1 ]->GetContentSize( nWidth, nHeight );
	return nHeight;
}

void CKeyValueLabelPanel::SetValue( const wchar_t *pValue )
{
	m_pLabels[ 1 ]->SetText( pValue );
	InvalidateLayout();
}

//-----------------------------------------------------------------------------

CBaseDetailsPanel::CBaseDetailsPanel( Panel *pParent, const char *pName, ReplayHandle_t hReplay )
:	EditablePanel( pParent, pName ),
	m_hReplay( hReplay ),
	m_bShouldShow( true )
{
	SetScheme( "ClientScheme" );

	m_pInsetPanel = new EditablePanel( this, "InsetPanel" );
}

void CBaseDetailsPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetBorder( pScheme->GetBorder( "ReplayStatsBorder" ) );
	SetBgColor( Color( 0,0,0, 255 ) );
}

void CBaseDetailsPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	// Setup inset panel bounds
	const int n = GetMarginSize();
	m_pInsetPanel->SetBounds( n, n, GetWide() - 2*n, GetTall() - 2*n );
}

//-----------------------------------------------------------------------------

CRecordsPanel::CRecordsPanel( Panel *pParent, ReplayHandle_t hReplay )
:	CBaseDetailsPanel( pParent, "RecordsPanel", hReplay )
{
	m_pClassImage = new ImagePanel( this, "ClassImage" );
}

void CRecordsPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetBorder( pScheme->GetBorder( "ReplayDefaultBorder" ) );
	SetBgColor( Color( 0,0,0,0 ) );
}

void CRecordsPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	// Figure out the class image name
	char szImage[MAX_OSPATH];
	const CGenericClassBasedReplay *pReplay = GetGenericClassBasedReplay( m_hReplay );
	V_snprintf( szImage, sizeof( szImage ), "class_sel_sm_%s_%s", pReplay->GetMaterialFriendlyPlayerClass(), pReplay->GetPlayerTeam() );	// Cause default image to display

	int nHeight = 0;

	// Get the image
	IImage *pImage = scheme()->GetImage( szImage, true );
	if ( pImage )
	{
		// Get image dimensions
		int nImageWidth, nImageHeight;
		pImage->GetSize( nImageWidth, nImageHeight );

		// Compute height of the records panel as a little smaller than the image itself
		nHeight = nImageHeight * 11 / 16;

		// Setup the image panel - parent to records panel parent so it goes out of the records panel's bounds a bit
		const int nMargin = 7;
		const float flScale = 1.2f;
		m_pClassImage->SetImage( pImage );
		m_pClassImage->SetParent( GetParent() );
		m_pClassImage->SetShouldScaleImage( true );
		m_pClassImage->SetScaleAmount( flScale );
		int nX, nY;
		GetPos( nX, nY );
		m_pClassImage->SetBounds( nX + nMargin, nY - flScale * nImageHeight + GetTall() - nMargin, nImageWidth * flScale, nImageHeight * flScale );

#if !defined( TF_CLIENT_DLL )
		m_pClassImage->SetVisible( false );
#endif
	}

	SetTall( nHeight );
}

//-----------------------------------------------------------------------------

CStatsPanel::CStatsPanel( Panel *pParent, ReplayHandle_t hReplay )
:	CBaseDetailsPanel( pParent, "StatsPanel", hReplay )
{
	CGenericClassBasedReplay *pReplay = GetGenericClassBasedReplay( hReplay );

	// Don't show the panel unless there are stats to display
	m_bShouldShow = false;

	// Create all stat labels
	RoundStats_t const &stats = pReplay->GetStats();
	for ( int i = 0; i < REPLAY_MAX_DISPLAY_GAMESTATS; ++i )
	{
		const int nCurStat = stats.Get( g_pReplayDisplayGameStats[i].m_nStat );
		if ( !nCurStat )
		{
			m_paStatLabels[ i ] = NULL;
			continue;
		}

		// Setup value
		char szValue[256];
		V_snprintf( szValue, sizeof( szValue ), "%i", nCurStat );

		// Create labels for this stat
		m_paStatLabels[ i ] = new CKeyValueLabelPanel( GetInset(), g_pReplayDisplayGameStats[i].m_pStatLocalizationToken, szValue );

		// At least one stat to display
		m_bShouldShow = true;
	}
}

void CStatsPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

void CStatsPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	int nY = 0;
	for ( int i = 0; i < REPLAY_MAX_DISPLAY_GAMESTATS; ++i )
	{
		CKeyValueLabelPanel *pCurStatLabels = m_paStatLabels[ i ];
		if ( pCurStatLabels )
		{
			pCurStatLabels->SetBounds( 0, nY, GetInset()->GetWide(), YRES(13) );
			nY += YRES(13);
		}
	}

	SetTall( nY + GetMarginSize() * 2 );
}

//-----------------------------------------------------------------------------

CDominationsPanel::CDominationsPanel( Panel *pParent, ReplayHandle_t hReplay )
:	CBaseDetailsPanel( pParent, "DominationsPanel", hReplay )
{
	CGenericClassBasedReplay *pReplay = GetGenericClassBasedReplay( hReplay );

	m_pNumDominationsImage = new ImagePanel( GetInset(), "NumDominations" );

	char szImage[256];
	int nNumDominations = pReplay->GetDominationCount();

	// Setup the # of dominations image
	V_snprintf( szImage, sizeof( szImage ), "../hud/leaderboard_dom%i", nNumDominations );
	m_pNumDominationsImage->SetImage( szImage );
	
	// Add avatars for each person dominated
	if ( steamapicontext && steamapicontext->SteamUtils() && steamapicontext->SteamUtils()->GetConnectedUniverse() )
	{
		for ( int i = 0; i < nNumDominations; ++i )
		{
			CAvatarImage *pAvatar = new CAvatarImage();
			CSteamID id( pReplay->GetDomination( i )->m_nVictimFriendId, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual );
			pAvatar->SetAvatarSteamID( id );
			pAvatar->SetAvatarSize( 32, 32 );
			pAvatar->UpdateFriendStatus();

			ImagePanel *pImagePanel = new ImagePanel( GetInset(), "DominationImage" );
			pImagePanel->SetImage( pAvatar );
			pImagePanel->SetShouldScaleImage( false );

			m_vecDominationImages.AddToTail( pImagePanel );
		}
	}
}

void CDominationsPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

void CDominationsPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	const int nBuffer = 7;

	int nImageWidth, nImageHeight;
	m_pNumDominationsImage->GetImage()->GetSize( nImageWidth, nImageHeight );
	m_pNumDominationsImage->SetBounds( 0, 0, nImageWidth, nImageHeight );
	int nX = nImageWidth + 2*nBuffer;
	int nY = 0;
	
	for ( int i = 0; i < m_vecDominationImages.Count(); ++i )
	{
		ImagePanel *pImagePanel = m_vecDominationImages[ i ];
		pImagePanel->GetImage()->GetSize( nImageWidth, nImageHeight );
		m_vecDominationImages[ i ]->SetBounds( nX, nY, nImageWidth, nImageHeight );

		nX += nImageWidth + nBuffer;
		if ( nX + nImageWidth > GetInset()->GetWide() )
		{
			nX = 0;
			nY += nImageHeight + nBuffer;
		}
	}

	SetTall( nY + nImageHeight + GetMarginSize() * 2 );
}

//-----------------------------------------------------------------------------

CKillsPanel::CKillsPanel( Panel *pParent, ReplayHandle_t hReplay )
:	CBaseDetailsPanel( pParent, "KillsPanel", hReplay )
{
	// Get the replay from the handle and add all kills
	CGenericClassBasedReplay *pReplay = GetGenericClassBasedReplay( hReplay );
	char szKillCount[64] = "0";
	if ( pReplay )
	{
		for ( int i = 0; i < pReplay->GetKillCount(); ++i )
		{
			// Construct path for image
			char szImgPath[MAX_OSPATH] = "";

#if defined( TF_CLIENT_DLL )
			// Get the kill info
			const CGenericClassBasedReplay::KillData_t *pKill = pReplay->GetKill( i );

			char const *pClass = pKill->m_nPlayerClass == TF_CLASS_DEMOMAN
				? "demo"
				: g_aPlayerClassNames_NonLocalized[ pKill->m_nPlayerClass ];

			V_snprintf( szImgPath, sizeof( szImgPath ), "../hud/leaderboard_class_%s", pClass );
#elif defined( CSTRIKE_DLL )
			V_strcpy( szImgPath, "../hud/scoreboard_dead" );
#endif

			// Get the image
			IImage *pImage = scheme()->GetImage( szImgPath, true );

			// Create new image panel
			ImagePanel *pImgPanel = new ImagePanel( GetInset(), "img" );
			pImgPanel->SetImage( pImage );

			// Cache for later
			m_vecKillImages.AddToTail( pImgPanel );
		}

		// Copy kill count
		V_snprintf( szKillCount, sizeof( szKillCount ), "%i", pReplay->GetKillCount() );
	}

	// Create labels
	m_pKillLabels = new CKeyValueLabelPanel( GetInset(), "#Replay_Kills", szKillCount );
}

void CKillsPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pKillLabels->SetBounds( 0, 0, GetWide(), m_pKillLabels->GetHeight() );

	// Setup image positions
	int nBuffer = 5;
	int nY = m_pKillLabels->GetHeight() + nBuffer * 2;
	int nImageY = nY;
	int nImageX = 0;
	for ( int i = 0; i < m_vecKillImages.Count(); ++i )
	{
		IImage *pCurImage = m_vecKillImages[ i ]->GetImage();
		if ( !pCurImage )
			continue;

		int nImageWidth, nImageHeight;
		pCurImage->GetSize( nImageWidth, nImageHeight );
		m_vecKillImages[ i ]->SetBounds( nImageX, nImageY, nImageWidth, nImageHeight );

		nImageX += nImageWidth + nBuffer;

		if ( i == 0 )
		{
			nY += nImageHeight;
		}

		if ( nImageX + nImageWidth > GetInset()->GetWide() )
		{
			nImageX = 0;
			nImageY += nImageHeight + nBuffer;
			nY += nImageHeight + nBuffer;
		}
	}

	// Set the height
	SetTall( nY + GetMarginSize() * 2 );
}

void CKillsPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------

extern const char *GetMapDisplayName( const char *mapName );

CBasicLifeInfoPanel::CBasicLifeInfoPanel( Panel *pParent, ReplayHandle_t hReplay )
:	CBaseDetailsPanel( pParent, "BasicLifeInfo", hReplay )
{
	// Create labels
	CGenericClassBasedReplay *pReplay = GetGenericClassBasedReplay( hReplay );
	m_pKilledByLabels = new CKeyValueLabelPanel( GetInset(), "#Replay_StatKilledBy", pReplay->WasKilled() ? pReplay->GetKillerName() : "#Replay_None" );
	m_pMapLabels = new CKeyValueLabelPanel( GetInset(), "#Replay_OnMap", GetMapDisplayName( pReplay->m_szMapName ) );
	m_pLifeLabels = new CKeyValueLabelPanel( GetInset(), "#Replay_Life", CReplayTime::FormatTimeString( (int)pReplay->m_flLength ) );
}

void CBasicLifeInfoPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

void CBasicLifeInfoPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	int nBuffer = 5;
	m_pKilledByLabels->SetBounds( 0, 0, GetWide(), m_pKilledByLabels->GetHeight() );
	m_pMapLabels->SetBounds( 0, m_pKilledByLabels->GetTall() + nBuffer, GetWide(), m_pMapLabels->GetHeight() );

	int nLifeLabelsY = ( m_pMapLabels->GetTall() + nBuffer ) * 2;
	m_pLifeLabels->SetBounds( 0, nLifeLabelsY, GetWide(), m_pLifeLabels->GetHeight() );

	SetTall( nLifeLabelsY + m_pLifeLabels->GetTall() + GetMarginSize() * 2 );
}

//-----------------------------------------------------------------------------

CYouTubeInfoPanel::CYouTubeInfoPanel( Panel *pParent )
								 :	CBaseDetailsPanel( pParent, "YouTubeInfo", NULL ),
								 m_pLabels( NULL )
{
	m_pLabels = new CKeyValueLabelPanel( GetInset(), "#Replay_YouTube", g_pVGuiLocalize->Find( "YouTube_NoStats" ) );
}

void CYouTubeInfoPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pLabels->SetBounds( 0, 0, GetWide(), m_pLabels->GetValueHeight() );

	SetTall( m_pLabels->GetTall() + GetMarginSize() * 2 );
}

void CYouTubeInfoPanel::SetInfo( const wchar_t *pInfo )
{
	m_pLabels->SetValue( pInfo );
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
	
CTitleEditPanel::CTitleEditPanel( Panel *pParent, QueryableReplayItemHandle_t hReplayItem, IReplayItemManager *pItemManager )
:	EditablePanel( pParent, "TitleEditPanel" ),
	m_hReplayItem( hReplayItem ),
	m_pItemManager( pItemManager ),
	m_bMouseOver( false ),
	m_pTitleEntry( NULL ),
	m_pHeaderLine( NULL ),
	m_pClickToEditLabel( NULL ),
	m_pCaratLabel( NULL )
{
	ivgui()->AddTickSignal( GetVPanel(), 10 );
}

CTitleEditPanel::~CTitleEditPanel()
{
	ivgui()->RemoveTickSignal( GetVPanel() );
}

void CTitleEditPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/replaybrowser/titleeditpanel.res", "GAME" );
	
	// Get ptr to carat label
	m_pCaratLabel = dynamic_cast< CExLabel * >( FindChildByName( "CaratLabel" ) );

	// Get ptr to "click to edit" label
	m_pClickToEditLabel = dynamic_cast< CExLabel * >( FindChildByName( "ClickToEditLabel" ) );

	// Setup title entry
	m_pTitleEntry = dynamic_cast< TextEntry * >( FindChildByName( "TitleInput" ) );
	m_pTitleEntry->SelectAllOnFocusAlways( true );
	
#if !defined( TF_CLIENT_DLL )
	m_pTitleEntry->SetPaintBorderEnabled( false );
#endif

	// Setup title entry text
	IQueryableReplayItem *pReplayItem = m_pItemManager->GetItem( m_hReplayItem );
	const wchar_t *pTitle = pReplayItem->GetItemTitle();
	m_pTitleEntry->SetText( pTitle[0] ? pTitle : L"#Replay_DefaultDetailsTitle" );

	// Cache pointer to the image
	m_pHeaderLine = dynamic_cast< ImagePanel * >( FindChildByName( "HeaderLine" ) );
}

void CTitleEditPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	int nCaratW, nCaratH;
	m_pCaratLabel->GetContentSize( nCaratW, nCaratH );
	m_pCaratLabel->SetWide( nCaratW );
	m_pCaratLabel->SetTall( nCaratH );

	// Get title entry pos
	int nTitleEntryX, nTitleEntryY;
	m_pTitleEntry->GetPos( nTitleEntryX, nTitleEntryY );

	// Set width of title entry to be width of parent, which has margins
	m_pTitleEntry->SetToFullHeight();
	m_pTitleEntry->SetWide( GetParent()->GetWide() - nTitleEntryX - 1 );

	// Get content size for label
	int nClickToEditW, nClickToEditH;
	m_pClickToEditLabel->GetContentSize( nClickToEditW, nClickToEditH );

	// Set click-to-edit bounds
	int nTitleEntryTall = m_pTitleEntry->GetTall();
	m_pClickToEditLabel->SetBounds(
		nTitleEntryX + GetParent()->GetWide() - nClickToEditW * 1.4f,
		nTitleEntryY + ( nTitleEntryTall - nClickToEditH ) / 2,
		nClickToEditW,
		nClickToEditH
	);

	// Setup header line position
	m_pHeaderLine->SetPos( 0, nTitleEntryY + m_pTitleEntry->GetTall() * 1.2f );
}

void CTitleEditPanel::OnTick()
{
	int nMouseX, nMouseY;
	input()->GetCursorPos( nMouseX, nMouseY );
	m_bMouseOver = m_pTitleEntry->IsWithin( nMouseX, nMouseY );
}

void CTitleEditPanel::PaintBackground()
{
	bool bEditing = m_pTitleEntry->HasFocus();
	bool bDrawExtraStuff = !vgui::input()->GetAppModalSurface() && ( m_bMouseOver || bEditing );	// Don't draw extra stuff when render dialog is up

	// If mouse is over and we're not editing, show the "click to edit" label
	m_pClickToEditLabel->SetVisible( m_bMouseOver && !bEditing );

	// Draw border if necessary
	if ( bDrawExtraStuff )
	{
		// Use the game UI panel here, since using this panel's vpanel (PushMakeCurrent() is set in
		// Panel::PaintTraverse(), the function that calls PaintBackground()) causes dimmed top and
		// left lines.  Using the game UI panel allows painting outside of the text entry itself.
		vgui::VPANEL vGameUI = enginevgui->GetPanel( PANEL_GAMEUIDLL );

		// Calculate title entry rect (x0,y0,x1,y1) - move the absolute upper-left corner 1 pixel left & up
		int aTitleRect[4];
		ipanel()->GetAbsPos( m_pTitleEntry->GetVPanel(), aTitleRect[0], aTitleRect[1] );

		--aTitleRect[0];
		--aTitleRect[1];
		aTitleRect[2] = aTitleRect[0] + m_pTitleEntry->GetWide() + 2;
		aTitleRect[3] = aTitleRect[1] + m_pTitleEntry->GetTall() + 2;

		surface()->PushMakeCurrent( vGameUI, false );

			// Draw background
			surface()->DrawSetColor( Color( 29, 28, 26, 255 ) );
			surface()->DrawFilledRect( aTitleRect[0], aTitleRect[1], aTitleRect[2], aTitleRect[3] );

			// Draw stroke
			surface()->DrawSetColor( Color( 202, 190, 164, 255 ) );
			surface()->DrawLine( aTitleRect[0], aTitleRect[1], aTitleRect[2], aTitleRect[1] );	// Top
			surface()->DrawLine( aTitleRect[0], aTitleRect[1], aTitleRect[0], aTitleRect[3] );	// Left
			surface()->DrawLine( aTitleRect[0], aTitleRect[3], aTitleRect[2], aTitleRect[3] );	// Bottom
			surface()->DrawLine( aTitleRect[2], aTitleRect[1], aTitleRect[2], aTitleRect[3] );	// Right

		surface()->PopMakeCurrent( vGameUI );
	}
}

void CTitleEditPanel::OnKeyCodeTyped( KeyCode code )
{
	IQueryableReplayItem *pReplayItem = m_pItemManager->GetItem( m_hReplayItem );

	const wchar_t *pTitle = pReplayItem->GetItemTitle();

	if ( m_pTitleEntry->HasFocus() && pReplayItem )
	{
		if ( code == KEY_ESCAPE )
		{
			// Get replay text and reset it
			m_pTitleEntry->SetText( pTitle );

			// Remove focus
			GetParent()->GetParent()->RequestFocus();
		}
		else if ( code == KEY_ENTER )
		{
			// If text is empty, reset to old title
			if ( m_pTitleEntry->GetTextLength() == 0 )
			{
				m_pTitleEntry->SetText( pTitle );
			}
			else	// Save title...
			{
				// Copy text into the replay
				// NOTE: SetItemTitle() will mark replay as dirty
				wchar_t wszNewTitle[MAX_REPLAY_TITLE_LENGTH];
				m_pTitleEntry->GetText( wszNewTitle, sizeof( wszNewTitle ) );
				pReplayItem->SetItemTitle( wszNewTitle );

				// Save!
				g_pReplayManager->FlagReplayForFlush( pReplayItem->GetItemReplay(), true );

				// Notify the thumbnail
				void *pUserData = pReplayItem->GetUserData();
				if ( pUserData )
				{
					CReplayBrowserThumbnail *pThumbnail = (CReplayBrowserThumbnail*)pUserData;
					pThumbnail->UpdateTitleText();
				}
			}

			GetParent()->GetParent()->RequestFocus();
		}

		return;
	}

	BaseClass::OnKeyCodeTyped( code );
}

//-----------------------------------------------------------------------------

CPlaybackPanel::CPlaybackPanel( Panel *pParent )
:	EditablePanel( pParent, "PlaybackPanel" )
{
}

CPlaybackPanel::~CPlaybackPanel()
{
	ivgui()->RemoveTickSignal( GetVPanel() );
}

void CPlaybackPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/replaybrowser/playbackpanel.res", "GAME" );
}

void CPlaybackPanel::PerformLayout()
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------

CPlaybackPanelSlideshow::CPlaybackPanelSlideshow( Panel *pParent, ReplayHandle_t hReplay )
:	CPlaybackPanel( pParent ),
	m_hReplay( hReplay )
{
	m_pScreenshotImage = new CReplayScreenshotSlideshowPanel( this, "Screenshot", hReplay );
}

void CPlaybackPanelSlideshow::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/replaybrowser/playbackpanelslideshow.res", "GAME" );

	m_pNoScreenshotLabel = dynamic_cast< CExLabel * >( FindChildByName( "NoScreenshotLabel" ) );

	// Check to see if there's a screenshot
	CGenericClassBasedReplay *pReplay = GetGenericClassBasedReplay( m_hReplay );
	if ( !pReplay )
		return;
	
	if ( !pReplay->GetScreenshotCount() && m_pNoScreenshotLabel )	// Show no-screenshot label
	{
		m_pNoScreenshotLabel->SetVisible( true );
	}
}

void CPlaybackPanelSlideshow::PerformLayout()
{
	BaseClass::PerformLayout();

	CGenericClassBasedReplay *pReplay = GetGenericClassBasedReplay( m_hReplay );

	int nMarginWidth = GetMarginSize();
	int nScreenshotWidth = GetViewWidth();
	if ( m_pScreenshotImage )
	{
		m_pScreenshotImage->SetBounds( nMarginWidth, nMarginWidth, nScreenshotWidth, GetTall() - 2*nMarginWidth );

		// Setup screenshot scale based on width of first screenshot (if there are any screenshots at all) - otherwise don't scale
		float flScale = pReplay->GetScreenshotCount() == 0 ? 1.0f : ( (float)nScreenshotWidth / ( .95f * pReplay->GetScreenshot( 0 )->m_nWidth ) );
		m_pScreenshotImage->GetImagePanel()->SetScaleAmount( flScale );
		m_pScreenshotImage->GetImagePanel()->SetShouldScaleImage( true );
	}

	// Setup the label
	int nLabelW, nLabelH;
	m_pNoScreenshotLabel->GetContentSize( nLabelW, nLabelH );
	m_pNoScreenshotLabel->SetBounds( 0, ( GetTall() - nLabelH ) / 2, GetWide(), nLabelH );
}

//-----------------------------------------------------------------------------

CPlaybackPanelMovie::CPlaybackPanelMovie( Panel *pParent, ReplayHandle_t hMovie )
:	CPlaybackPanel( pParent )
{
	IReplayMovie *pMovie = g_pReplayMovieManager->GetMovie( hMovie );
	m_pMoviePlayerPanel = new CMoviePlayerPanel( this, "MoviePlayer", pMovie->GetMovieFilename() );

	m_pMoviePlayerPanel->SetLooping( true );

	// TODO: show controls and don't play right away
	m_pMoviePlayerPanel->Play();
}

void CPlaybackPanelMovie::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

void CPlaybackPanelMovie::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pMoviePlayerPanel->SetBounds( 9, 9, GetViewWidth(), GetViewHeight() );
	m_pMoviePlayerPanel->SetEnabled( true );
	m_pMoviePlayerPanel->SetVisible( true );
	m_pMoviePlayerPanel->SetZPos( 101 );
}

void CPlaybackPanelMovie::FreeMovieMaterial()
{
	m_pMoviePlayerPanel->FreeMaterial();
}

//-----------------------------------------------------------------------------

CCutImagePanel::CCutImagePanel( Panel *pParent, const char *pName )
:	BaseClass( pParent, pName, "" ),
	m_pSelectedBorder( NULL )
{
}

void CCutImagePanel::SetSelected( bool bState )
{
	BaseClass::SetSelected( bState );
}

IBorder *CCutImagePanel::GetBorder( bool bDepressed, bool bArmed, bool bSelected, bool bKeyFocus )
{
	if ( bSelected )
	{
		return m_pSelectedBorder;
	}

	return BaseClass::GetBorder( bDepressed, bArmed, bSelected, bKeyFocus );
}

void CCutImagePanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

}

//-----------------------------------------------------------------------------

#define FOR_EACH_BUTTON( _i )	for ( int _i = 0; _i < BUTTONS_PER_PAGE; ++_i )

CCutsPanel::CCutsPanel( Panel *pParent, ReplayHandle_t hReplay, int iSelectedPerformance )
:	BaseClass( pParent, "CutsPanel", hReplay ),
	m_iPage( 0 ),
	m_nVisibleButtons( 0 ),
	m_pVerticalLine( NULL ),
	m_pNoCutsLabel( NULL ),
	m_pOriginalLabel( NULL ),
	m_pCutsLabel( NULL )
{
	m_hDetailsPanel = dynamic_cast< CReplayDetailsPanel * >( pParent->GetParent() );

	FOR_EACH_BUTTON( i )
	{
		const int iButton = i;
		CFmtStr fmtName( "CutButton%i", iButton );

		CExImageButton *pNewButton = new CExImageButton( this, fmtName.Access(), "" );
		CFmtStr fmtCommand( "select_%i", iButton );
		pNewButton->SetCommand( fmtCommand.Access() );
		pNewButton->InvalidateLayout( true, true );
		pNewButton->AddActionSignalTarget( this );
		pNewButton->SetSelected( i == 0 );
#if !defined( TF_CLIENT_DLL )
		pNewButton->SetSelectedColor( Color( 0, 0, 0, 0 ), Color( 122, 25, 16, 255 ) );
#endif

		const int iPerformance = i - 1;
		m_aButtons[ i ].m_pButton = pNewButton;
		m_aButtons[ i ].m_iPerformance = iPerformance;

		CExButton *pAddToRenderQueueButton = new CExButton( pNewButton, "AddToRenderQueue", "+", this );
		m_aButtons[ i ].m_pAddToRenderQueueButton = pAddToRenderQueueButton;
	}

	// Layout right now
	InvalidateLayout( true, true );

	// Calculate page
	SetPage(
		( 1 + iSelectedPerformance ) / BUTTONS_PER_PAGE,
		( 1 + iSelectedPerformance ) % BUTTONS_PER_PAGE
	);

	ivgui()->AddTickSignal( GetVPanel(), 10 );
}

CCutsPanel::~CCutsPanel()
{
	ivgui()->RemoveTickSignal( GetVPanel() );
}

void CCutsPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/replaybrowser/cutspanel.res", "GAME" );

	m_pVerticalLine = dynamic_cast< EditablePanel * >( FindChildByName( "VerticalLine" ) );
	m_pNoCutsLabel = dynamic_cast< CExLabel * >( FindChildByName( "NoCutsLabel" ) );
	m_pOriginalLabel = dynamic_cast< CExLabel * >( FindChildByName( "OriginalLabel" ) );
	m_pCutsLabel = dynamic_cast< CExLabel * >( FindChildByName( "CutsLabel" ) );
	m_pNameLabel = dynamic_cast< CExLabel * >( FindChildByName( "NameLabel" ) );
	m_pPrevButton = dynamic_cast< CExButton * >( FindChildByName( "PrevButton" ) );
	m_pNextButton = dynamic_cast< CExButton * >( FindChildByName( "NextButton" ) );

	FOR_EACH_BUTTON( i )
	{
		CExImageButton *pCurButton = m_aButtons[ i ].m_pButton;
#if !defined( TF_CLIENT_DLL )
		pCurButton->SetPaintBorderEnabled( false );
#endif
		pCurButton->InvalidateLayout( true, true );
	}
}

void CCutsPanel::ApplySettings( KeyValues *pInResourceData )
{
	BaseClass::ApplySettings( pInResourceData );

	KeyValues *pButtonSettings = pInResourceData->FindKey( "button_settings" );
	if ( pButtonSettings )
	{
		KeyValues *pAddToRenderQueueButtonSettings = pButtonSettings->FindKey( "addtorenderqueuebutton_settings" );

		CReplay *pReplay = GetGenericClassBasedReplay( m_hReplay );
		FOR_EACH_BUTTON( i )
		{
			CExImageButton *pCurButton = m_aButtons[ i ].m_pButton;

			pCurButton->ApplySettings( pButtonSettings );

			// Set screenshot as image
			if ( pReplay && pReplay->GetScreenshotCount() )
			{
				const float flScale = (float)m_nCutButtonHeight / pReplay->GetScreenshot( 0 )->m_nHeight;
				int nImageWidth = m_nCutButtonWidth - 2 * m_nCutButtonBuffer;
				int nImageHeight = m_nCutButtonHeight - 2 * m_nCutButtonBuffer;

				CFmtStr fmtFile( "replay\\thumbnails\\%s", pReplay->GetScreenshot( 0 )->m_szBaseFilename );
				pCurButton->SetSubImage( fmtFile.Access() );
				pCurButton->GetImage()->SetScaleAmount( flScale );
				pCurButton->GetImage()->SetBounds( m_nCutButtonBuffer, m_nCutButtonBuffer, nImageWidth, nImageHeight );
			}

			if ( pAddToRenderQueueButtonSettings )
			{
				CExButton *pAddToQueueButton = m_aButtons[ i ].m_pAddToRenderQueueButton;
				pAddToQueueButton->ApplySettings( pAddToRenderQueueButtonSettings );
				pAddToQueueButton->AddActionSignalTarget( this );
			}
		}
	}
}

void CCutsPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	CReplay *pReplay = GetGenericClassBasedReplay( m_hReplay );
	const int nNumCuts = pReplay->GetPerformanceCount();

	int nX = m_iPage > 0 ? m_pPrevButton->GetWide() + m_nCutButtonSpace : 0;

	m_nVisibleButtons = 0;

	FOR_EACH_BUTTON( i )
	{
		CExImageButton *pCurButton = m_aButtons[ i ].m_pButton;
		const bool bVisible = ButtonToPerformance( i ) < nNumCuts;

		pCurButton->SetVisible( bVisible );

		if ( bVisible )
		{
			++m_nVisibleButtons;
		}

		pCurButton->SetBounds( nX, m_nButtonStartY, m_nCutButtonWidth, m_nCutButtonHeight );
		nX += m_nCutButtonWidth;

		if ( i == 0 && m_iPage == 0 )
		{
			nX += 2 * m_nCutButtonSpaceWide + m_pVerticalLine->GetWide();
		}
		else
		{
			nX += m_nCutButtonSpace;
		}
	}

	if ( m_pVerticalLine )
	{
		m_pVerticalLine->SetVisible( m_nVisibleButtons > 0 && m_iPage == 0 );
		m_pVerticalLine->SetPos( m_nCutButtonWidth + m_nCutButtonSpaceWide, 0 );
		m_pVerticalLine->SetTall( m_nTopMarginHeight + GetTall() );
	}

	const int nRightOfVerticalLineX = m_nCutButtonWidth + m_nCutButtonSpaceWide * 2 + m_pVerticalLine->GetWide();

	if ( m_pNoCutsLabel )
	{
		m_pNoCutsLabel->SetVisible( m_nVisibleButtons == 1 && m_iPage == 0 );

		int nY = ( GetTall() - m_pNoCutsLabel->GetTall() ) / 2;
		m_pNoCutsLabel->SetPos( nRightOfVerticalLineX, nY );
	}

	if ( m_pOriginalLabel )
	{
		m_pOriginalLabel->SetVisible( m_iPage == 0 );
	}

	if ( m_pCutsLabel )
	{
		m_pCutsLabel->SetVisible( m_nVisibleButtons > 1 && m_iPage == 0 );
		m_pCutsLabel->SetPos( m_nCutButtonWidth + 2 * m_nCutButtonSpaceWide + m_pVerticalLine->GetWide(), 0 );
	}

	bool bPrevCuts = m_iPage > 0;
	bool bMoreCuts = ( nNumCuts + 1 ) > ( m_iPage + 1 ) * BUTTONS_PER_PAGE;
	int nY = m_nTopMarginHeight + ( GetTall() - m_pNextButton->GetTall() ) / 2;
	m_pPrevButton->SetVisible( bPrevCuts );
	m_pPrevButton->SetPos( 0, nY );
	m_pNextButton->SetVisible( bMoreCuts );
	m_pNextButton->SetPos( nX, nY );
}

void CCutsPanel::OnTick()
{
	if ( !TFModalStack()->IsEmpty() )
		return;

	int nMouseX, nMouseY;
	input()->GetCursorPos( nMouseX, nMouseY );

	// Early-out if not within the cuts panel at all.
	if ( !IsWithin( nMouseX, nMouseY ) )
		return;

	int iHoverPerformance = -2;
	bool bFoundHoverButton = false;
	FOR_EACH_BUTTON( i )
	{
		CExImageButton *pCurButton = m_aButtons[ i ].m_pButton;
		bool bIsHoverButton = false;
		if ( !bFoundHoverButton && pCurButton->IsWithin( nMouseX, nMouseY ) && pCurButton->IsVisible() )
		{
			iHoverPerformance = ButtonToPerformance( i );
			bFoundHoverButton = true;
			bIsHoverButton = true;
		}

		CExButton *pAddToRenderQueueButton = m_aButtons[ i ].m_pAddToRenderQueueButton;
		if ( pAddToRenderQueueButton )
		{
			pAddToRenderQueueButton->SetVisible( bIsHoverButton );
			
			if ( iHoverPerformance >= -1 )
			{
				// Set the text and command based on whether or not the take's already been queued
				const bool bInQueue = g_pClientReplayContext->GetRenderQueue()->IsInQueue( m_hReplay, iHoverPerformance );
				CFmtStr fmtCmd( "%srenderqueue_%i", bInQueue ? "removefrom" : "addto", iHoverPerformance );
				pAddToRenderQueueButton->SetCommand( fmtCmd.Access() );
				pAddToRenderQueueButton->SetText( bInQueue ? "-" : "+" );
			}
		}
	}

	// If the mouse is over a performance button, use that, otherwise use the selected
	// performance.
	if ( m_hDetailsPanel.Get() )
	{
		int iSelectedPerformance = m_hDetailsPanel->m_iSelectedPerformance;
		UpdateNameLabel( iHoverPerformance >= 0 ? iHoverPerformance : iSelectedPerformance >= 0 ? iSelectedPerformance : -1 );
	}
}

int CCutsPanel::ButtonToPerformance( int iButton ) const
{
	return -1 + m_iPage * BUTTONS_PER_PAGE + iButton;
}

void CCutsPanel::OnCommand( const char *pCommand )
{
	if ( !V_strnicmp( pCommand, "select_", 7 ) )
	{
		const int iButton = atoi( pCommand + 7 );
		SelectButtonFromPerformance( ButtonToPerformance( iButton ) );
	}
	else if ( !V_stricmp( pCommand, "prevpage" ) )
	{
		SetPage( m_iPage - 1 );
	}
	else if ( !V_stricmp( pCommand, "nextpage" ) )
	{
		SetPage( m_iPage + 1 );
	}
	else if ( !V_strnicmp( pCommand, "addtorenderqueue_", 17 ) )
	{
		if ( !replay_renderqueue_first_add.GetInt() )
		{
			ShowMessageBox( "#Replay_FirstRenderQueueAddTitle", "#Replay_FirstRenderQueueAddMsg", "#GameUI_OK" );
			replay_renderqueue_first_add.SetValue( 1 );
		}

		const int iPerformance = atoi( pCommand + 17 );
		if ( iPerformance >= -1 )
		{
			g_pClientReplayContext->GetRenderQueue()->Add( m_hReplay, iPerformance );
		}
	}
	else if ( !V_strnicmp( pCommand, "removefromrenderqueue_", 22 ) )
	{
		const int iPerformance = atoi( pCommand + 22 );
		if ( iPerformance >= -1 )
		{
			g_pClientReplayContext->GetRenderQueue()->Remove( m_hReplay, iPerformance );
		}
	}
}

void CCutsPanel::SetPage( int iPage, int iButtonToSelect )
{
	m_iPage = iPage;

	FOR_EACH_BUTTON( i )
	{
		ButtonInfo_t *pCurButtonInfo = &m_aButtons[ i ];
		const int iPerformance = ButtonToPerformance( i );
		pCurButtonInfo->m_iPerformance = iPerformance;
	}

	InvalidateLayout( true, false );
	SelectButtonFromPerformance( ButtonToPerformance( iButtonToSelect ) );
}

const CReplayPerformance *CCutsPanel::GetPerformance( int iPerformance ) const
{
	const CReplay *pReplay = GetGenericClassBasedReplay( m_hReplay );
	if ( !pReplay )
		return NULL;

	return iPerformance >= 0 ? pReplay->GetPerformance( iPerformance ) : NULL;
}

void CCutsPanel::SelectButtonFromPerformance( int iPerformance )
{
	FOR_EACH_BUTTON( i )
	{
		const ButtonInfo_t *pCurButtonInfo = &m_aButtons[ i ];
		CExImageButton *pCurButton = pCurButtonInfo->m_pButton;
		pCurButton->SetSelected( pCurButtonInfo->m_iPerformance == iPerformance );
		pCurButton->InvalidateLayout( true, true );
	}

	// Cache which performance to use in the details panel
	if ( m_hDetailsPanel.Get() )
	{
		m_hDetailsPanel->m_iSelectedPerformance = iPerformance;
	}

	UpdateNameLabel( iPerformance );
}

int CCutsPanel::PerformanceToButton( int iPerformance ) const
{
	FOR_EACH_BUTTON( i )
	{
		if ( m_aButtons[ i ].m_iPerformance == iPerformance )
		{
			return i;
		}
	}

	return -1;
}

void CCutsPanel::UpdateNameLabel( int iPerformance )
{
	const CReplayPerformance *pPerformance = GetPerformance( iPerformance );
	m_pNameLabel->SetText( pPerformance ? pPerformance->m_wszTitle : L"" );	

	// Get the button (in the range [0,BUTTONS_PER_PAGE]).
	const int iPerformanceButton = PerformanceToButton( iPerformance );	// Not necessarily the selected button - can be hover button

	// Get position of the button so we can use it's x position.
	int aSelectedButtonPos[2];
	m_aButtons[ iPerformanceButton ].m_pButton->GetPos( aSelectedButtonPos[0], aSelectedButtonPos[1] );

	if ( m_pNameLabel )
	{
		const int nNameLabelX = aSelectedButtonPos[0];
		const int nNameLabelY = m_nButtonStartY + m_nCutButtonHeight + m_nNameLabelTopMargin;
		m_pNameLabel->SetBounds(
			nNameLabelX,
			nNameLabelY,
			GetWide() - nNameLabelX,
			GetTall() - nNameLabelY
		);
	}
}

void CCutsPanel::OnPerformanceDeleted( int iPerformance )
{
	int iButton = PerformanceToButton( iPerformance );
	if ( iButton < 0 )
		return;

	// Deleted last performance on page?
	CReplay *pReplay = GetGenericClassBasedReplay( m_hReplay );
	const int nNumCuts = pReplay->GetPerformanceCount();
	if ( iPerformance == m_aButtons[ 0 ].m_iPerformance && iPerformance == nNumCuts )
	{
		SetPage( m_iPage - 1, BUTTONS_PER_PAGE - 1 );
	}
	else
	{
		SelectButtonFromPerformance( ButtonToPerformance( MIN( m_nVisibleButtons - 1, MAX( 0, iButton ) ) ) );
	}

	// Select the cut prior to the one we just deleted
	Assert( iPerformance >= 0 );

	InvalidateLayout( true, false );
}

//-----------------------------------------------------------------------------

static void ConfirmUploadMovie( bool bConfirmed, void *pContext )
{
	if ( bConfirmed )
	{
		CReplayDetailsPanel *pPanel = (CReplayDetailsPanel*)pContext;
		IQueryableReplayItem *pReplayItem = pPanel->m_pItemManager->GetItem( pPanel->m_hReplayItem );
		if ( pReplayItem && pReplayItem->IsItemAMovie() )
		{
			IReplayMovie *pMovie = static_cast< IReplayMovie * >( pReplayItem );
			if ( YouTube_GetLoginStatus() != kYouTubeLogin_LoggedIn )
			{
				YouTube_ShowLoginDialog( pMovie, pPanel );
			}
			else
			{	
				YouTube_ShowUploadDialog( pMovie, pPanel );
			}
		}
	}
}

class CYouTubeGetStatsHandler : public CYouTubeResponseHandler
{
public:
	CYouTubeGetStatsHandler( CReplayDetailsPanel *pPanel )
		: m_pPanel( pPanel )
		, m_handle( NULL )
		{
		}

	virtual ~CYouTubeGetStatsHandler()
	{
		if ( m_handle != NULL )
		{
			YouTube_CancelGetVideoInfo( m_handle );
		}
	}

	static bool GetEmptyElementTagContents( const char *pXML, const char *pTag, CUtlString &strTagContents )
	{
		CFmtStr1024 kLinkTagStart( "<%s ", pTag );
		const char *kLinkTagEnd = "/>";
		const char *pStart = strstr( pXML, kLinkTagStart.Access() );
		if ( pStart != NULL )
		{
			pStart += kLinkTagStart.Length();
			const char *pEnd = strstr( pStart, kLinkTagEnd );
			if ( pEnd != NULL )
			{
				strTagContents.SetDirect( pStart, pEnd - pStart );
				return true;
			}
		}
		return false;
	}

	static bool GetEmptyTagValue( const char *pTagContents, const char *pKeyName, CUtlString &value )
	{
		CFmtStr1024 kStart( "%s='", pKeyName );
		const char *kEnd = "'";
		const char *pStart = strstr( pTagContents, kStart.Access() );
		if ( pStart != NULL )
		{
			pStart += kStart.Length();
			const char *pEnd = strstr( pStart, kEnd );
			if ( pEnd != NULL )
			{
				value.SetDirect( pStart, pEnd - pStart );
				return true;
			}
		}
		return false;
	}

	virtual void HandleResponse( long responseCode, const char *pResponse )
	{
		// @note tom bui: wish I had an XML parser

		if ( strstr( pResponse, "<internalReason>Private video</internalReason>" ) != NULL )
		{
			m_pPanel->m_pYouTubeInfoPanel->SetInfo( g_pVGuiLocalize->Find( "#YouTube_PrivateVideo" ) );
			m_pPanel->SetYouTubeStatus( CReplayDetailsPanel::kYouTubeStatus_Private );
			return;
		}

		int iNumFavorited = 0;
		int iNumViews = 0;
		int iNumLikes = 0;

		wchar_t wszFavorited[256] = L"0";
		wchar_t wszViews[256] = L"0";

		CUtlString strTagStatistics;
		if ( GetEmptyElementTagContents( pResponse, "yt:statistics", strTagStatistics ) )
		{
			CUtlString favoriteCount;
			CUtlString viewCount;
			GetEmptyTagValue( strTagStatistics, "favoriteCount", favoriteCount );
			GetEmptyTagValue( strTagStatistics, "viewCount", viewCount );

			iNumFavorited = Q_atoi( favoriteCount.Get() );
			iNumViews = Q_atoi( viewCount.Get() );
			
			g_pVGuiLocalize->ConvertANSIToUnicode( favoriteCount.Get(), wszFavorited, sizeof( wszFavorited ) );
			g_pVGuiLocalize->ConvertANSIToUnicode( viewCount.Get(), wszViews, sizeof( wszViews ) );
		}

		wchar_t wszLikes[256] = L"0";
		CUtlString strTagRating;
		if ( GetEmptyElementTagContents( pResponse, "yt:rating", strTagRating ) )
		{
			CUtlString likes;
			GetEmptyTagValue( strTagRating, "numLikes", likes );
			iNumLikes = Q_atoi( likes.Get() );
			g_pVGuiLocalize->ConvertANSIToUnicode( likes.Get(), wszLikes, sizeof( wszLikes ) );
		}

		//const char *kLinkStartTag = "<link rel='alternate' type='text/html' href='";
		CUtlString strTagLink;
		if ( GetEmptyElementTagContents( pResponse, "link rel='alternate'", strTagLink ) )
		{
			GetEmptyTagValue( strTagLink, "href", m_strVideoURL );
		}

		wchar_t wszStats[256] = L"";
		g_pVGuiLocalize->ConstructString( wszStats,sizeof( wszStats ), g_pVGuiLocalize->Find( "#YouTube_Stats" ), 3, 
			wszFavorited,
			wszViews,
			wszLikes );

		if ( m_strVideoURL.IsEmpty() == false )
		{
			m_pPanel->m_pYouTubeInfoPanel->SetInfo( wszStats );
			m_pPanel->SetYouTubeStatus( CReplayDetailsPanel::kYouTubeStatus_RetrievedInfo );
			m_pPanel->InvalidateLayout();

			IGameEvent *event = gameeventmanager->CreateEvent( "replay_youtube_stats" );
			if ( event )
			{
				event->SetInt( "views", iNumViews );
				event->SetInt( "likes", iNumLikes );
				event->SetInt( "favorited", iNumFavorited );
				gameeventmanager->FireEventClientSide( event );
			}
		}
		else
		{
			m_pPanel->m_pYouTubeInfoPanel->SetInfo( g_pVGuiLocalize->Find( "#YouTube_CouldNotRetrieveStats" ) );
			m_pPanel->SetYouTubeStatus( CReplayDetailsPanel::kYouTubeStatus_CouldNotRetrieveInfo );
		}
	}

	CReplayDetailsPanel *m_pPanel;
	YouTubeInfoHandle_t m_handle;
	CUtlString			m_strVideoURL;
};

CReplayDetailsPanel::CReplayDetailsPanel( Panel *pParent, QueryableReplayItemHandle_t hReplayItem,
										  int iPerformance, IReplayItemManager *pItemManager )
:	EditablePanel( pParent, "DetailsPanel" ),
	m_hReplayItem( hReplayItem ),
	m_pItemManager( pItemManager ),
	m_pCutsPanel( NULL ),
	m_iSelectedPerformance( iPerformance ),
	m_pYouTubeResponseHandler( NULL ),
	m_hExportMovieDialog( NULL )
{
	m_hReplay = pItemManager->GetItem( hReplayItem )->GetItemReplayHandle();

	CGenericClassBasedReplay *pReplay = GetGenericClassBasedReplay( m_hReplay );

	m_pInsetPanel = new EditablePanel( this, "InsetPanel" );
	m_pTitleEditPanel = new CTitleEditPanel( GetInset(), m_hReplayItem, m_pItemManager );
	m_pPlaybackPanel = new CPlaybackPanelSlideshow( GetInset(), m_hReplay );
	m_pRecordsPanel = new CRecordsPanel( GetInset(), m_hReplay );

	m_pInfoPanel = new EditablePanel( this, "InfoContainerPanel" );
	m_pScrollPanel = new vgui::ScrollableEditablePanel( GetInset(), m_pInfoPanel, "StatsScroller" );
	m_pScrollPanel->GetScrollbar()->SetAutohideButtons( true );
#if !defined( TF_CLIENT_DLL )
	for ( int i = 0; i < 2; ++i )
	{
		m_pScrollPanel->GetScrollbar()->GetButton( i )->SetPaintBorderEnabled( false );
	}
#endif
	
	m_pBasicInfoPanel = new CBasicLifeInfoPanel( m_pInfoPanel, m_hReplay );
	m_pStatsPanel = new CStatsPanel( m_pInfoPanel, m_hReplay );
	m_pKillsPanel = new CKillsPanel( m_pInfoPanel, m_hReplay );

	const bool bIsMoviePanel = pItemManager->AreItemsMovies();
	if ( bIsMoviePanel )
	{
		m_pYouTubeInfoPanel = new CYouTubeInfoPanel( m_pInfoPanel );
	}
	else
	{
		m_pCutsPanel = new CCutsPanel( GetInset(), m_hReplay, m_iSelectedPerformance );
	}	

	// Add info panels to a list

	if ( pReplay->GetDominationCount() )
	{
		m_pDominationsPanel = new CDominationsPanel( m_pInfoPanel, m_hReplay );
		m_vecInfoPanels.AddToTail( m_pDominationsPanel );
	}

	m_vecInfoPanels.AddToTail( m_pBasicInfoPanel );
	m_vecInfoPanels.AddToTail( m_pStatsPanel );
	m_vecInfoPanels.AddToTail( m_pKillsPanel );

	if ( bIsMoviePanel )
	{
		m_vecInfoPanels.AddToTail( m_pYouTubeInfoPanel );
	}

	m_pYouTubeResponseHandler = new CYouTubeGetStatsHandler( this );

	RequestFocus();
}

CReplayDetailsPanel::~CReplayDetailsPanel()
{
	m_pDeleteButton->MarkForDeletion();
	m_pRenderButton->MarkForDeletion();
	m_pPlayButton->MarkForDeletion();
	delete m_pYouTubeResponseHandler;
}

void CReplayDetailsPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/replaybrowser/detailspanel.res", "GAME" );

	m_pExportMovie = dynamic_cast< CExButton * >( FindChildByName( "ExportMovieButton" ) );
	m_pDeleteButton = dynamic_cast< CExButton * >( FindChildByName( "DeleteButton" ) );
	m_pRenderButton = dynamic_cast< CExButton * >( FindChildByName( "RenderButton" ) );
	m_pPlayButton = dynamic_cast< CExButton * >( FindChildByName( "PlayButton" ) );
	m_pYouTubeUpload = dynamic_cast< CExButton * >( FindChildByName( "YouTubeUploadButton" ) );
	m_pYouTubeView = dynamic_cast< CExButton * >( FindChildByName( "ViewYouTubeButton" ) );
	m_pYouTubeShareURL = dynamic_cast< CExButton * >( FindChildByName( "ShareYouTubeURLButton" ) );
	m_pShowRenderInfoButton = dynamic_cast< CExImageButton * >( FindChildByName( "ShowRenderInfoButton") );

	if ( m_pDeleteButton )
	{
		SetXToRed( m_pDeleteButton );
	}

	m_pExportMovie->SetParent( GetInset() );
	m_pYouTubeUpload->SetParent( GetInset() );
	m_pYouTubeView->SetParent( GetInset() );
	m_pYouTubeShareURL->SetParent( GetInset() );
	m_pShowRenderInfoButton->SetParent( GetInset() );

	m_pDeleteButton->SetParent( GetParent()->GetParent()->GetParent() );
	m_pPlayButton->SetParent( GetParent()->GetParent()->GetParent() );
	m_pRenderButton->SetParent( GetParent()->GetParent()->GetParent() );

	m_pDeleteButton->AddActionSignalTarget( this );
	m_pPlayButton->AddActionSignalTarget( this );
	m_pRenderButton->AddActionSignalTarget( this );
}

void CReplayDetailsPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	SetTall( GetParent()->GetTall() );

	int nInsetWidth = GetInset()->GetWide();
	int nScreenshotWidth = nInsetWidth * .55f;

	// Setup info panels along the right-hand side
	const int nBuffer = 7;
	const int nLeftRightBuffer = 19;
	int aPlaybackPos[2];
	m_pPlaybackPanel->GetPos( aPlaybackPos[0], aPlaybackPos[1] );
	int nInfoPanelsStartY = aPlaybackPos[1];
	int nInfoPanelsCurrentY = nInfoPanelsStartY;
	int nRightColumnWidth = nInsetWidth - nScreenshotWidth - nLeftRightBuffer - XRES(20);

#if defined( TF_CLIENT_DLL )
	if ( m_pRecordsPanel->ShouldShow() )
	{
		m_pRecordsPanel->SetPos( nScreenshotWidth + nLeftRightBuffer, nInfoPanelsStartY );
		m_pRecordsPanel->SetWide( nRightColumnWidth );
		m_pRecordsPanel->InvalidateLayout( true, true );
		m_pRecordsPanel->SetVisible( true );
		nInfoPanelsCurrentY += m_pRecordsPanel->GetTall() + nBuffer;
	}
	else
#endif
	{
		m_pRecordsPanel->SetVisible( false );
	}

	int insetX, insetY;
	GetInset()->GetPos( insetX, insetY );
	m_pScrollPanel->SetPos( nScreenshotWidth + nLeftRightBuffer, nInfoPanelsCurrentY );
	m_pScrollPanel->SetWide( nRightColumnWidth + XRES(20) );
	m_pScrollPanel->SetTall( GetTall() - insetY - nInfoPanelsCurrentY );
	m_pInfoPanel->SetWide( nRightColumnWidth );	

	int nCurrentY = 0;
	for ( int i = 0; i < m_vecInfoPanels.Count(); ++i )
	{
		CBaseDetailsPanel *pPanel = m_vecInfoPanels[ i ];

		if ( pPanel->ShouldShow() )
		{
			// Set the width since these panel's PerformLayout()'s depend on it
			pPanel->SetWide( nRightColumnWidth );

			// Call panel's PerformLayout() now
			pPanel->InvalidateLayout( true, true );

			pPanel->SetPos( 0, nCurrentY );

			// Show it
			pPanel->SetVisible( true );

			// Update the current y position based on the panel's height (set in its PerformLayout())
			nCurrentY += pPanel->GetTall() + nBuffer;
		}
		else
		{
			pPanel->SetVisible( false );
		}
	}
	m_pInfoPanel->SetTall( nCurrentY );
	m_pInfoPanel->InvalidateLayout( true );
	m_pScrollPanel->InvalidateLayout( true );
	m_pScrollPanel->GetScrollbar()->SetAutohideButtons( true );
	m_pScrollPanel->GetScrollbar()->InvalidateLayout( true );

	// @note Tom Bui: set the positions AGAIN now that we've invalidated, cause VGUI hates me
	nCurrentY = 0;
	for ( int i = 0; i < m_vecInfoPanels.Count(); ++i )
	{
		CBaseDetailsPanel *pPanel = m_vecInfoPanels[ i ];
		if ( pPanel->ShouldShow() )
		{
			pPanel->SetPos( 0, nCurrentY );
			nCurrentY += pPanel->GetTall() + nBuffer;
		}
	}

	// Setup playback panel based on dimensions of first screenshot
	CGenericClassBasedReplay *pReplay = GetGenericClassBasedReplay( m_hReplay );
	float flAspectRatio;
	if ( pReplay->GetScreenshotCount() )
	{
		const CReplayScreenshot *pScreenshot = pReplay->GetScreenshot( 0 );
		flAspectRatio = (float)pScreenshot->m_nWidth / pScreenshot->m_nHeight;
	}
	else
	{
		// Default to 4:3 if there are no screenshots
		flAspectRatio = 4.0f/3;
	}

	if ( m_pItemManager->AreItemsMovies() )
	{
		m_pRenderButton->SetVisible( false );
		m_pPlayButton->SetVisible( false );
		m_pExportMovie->SetVisible( true );
		m_pShowRenderInfoButton->SetVisible( true );

		int nButtonY = nInfoPanelsStartY + m_pPlaybackPanel->GetTall() + YRES( 5 );
		int nButtonX = 0;
		m_pYouTubeUpload->SetPos( nButtonX, nButtonY );
		m_pYouTubeView->SetPos( nButtonX, nButtonY );
		nButtonX += m_pYouTubeUpload->GetWide() + XRES( 5 );
		
		m_pYouTubeShareURL->SetPos( nButtonX, nButtonY );
		nButtonX += m_pYouTubeShareURL->GetWide() + XRES( 5 );

		m_pExportMovie->SetPos( nButtonX, nButtonY );

		int aDeletePos[2];
		m_pDeleteButton->GetPos( aDeletePos[0], aDeletePos[1] );
		m_pDeleteButton->SetPos( ScreenWidth() / 2 + XRES( 195 ), aDeletePos[1] );

		int aScreenshotPos[2];
		m_pPlaybackPanel->GetPos( aScreenshotPos[0], aScreenshotPos[1] );
		m_pShowRenderInfoButton->SetPos(
			aScreenshotPos[0] + m_pPlaybackPanel->GetWide() - m_pShowRenderInfoButton->GetWide() - XRES( 8 ),
			aScreenshotPos[1] + m_pPlaybackPanel->GetTall() - m_pShowRenderInfoButton->GetTall() - YRES( 8 )
		);

		// retrieve stats
		if ( m_pYouTubeResponseHandler->m_handle == NULL )
		{
			IQueryableReplayItem *pReplayItem = m_pItemManager->GetItem( m_hReplayItem );
			if ( pReplayItem && pReplayItem->IsItemAMovie() )
			{
				IReplayMovie *pMovie = static_cast< IReplayMovie * >( pReplayItem );
				if ( pMovie->IsUploaded() )
				{
					m_pYouTubeResponseHandler->m_handle = YouTube_GetVideoInfo( pMovie->GetUploadURL(), *m_pYouTubeResponseHandler );
					SetYouTubeStatus( kYouTubeStatus_RetrievingInfo );
				}
				else
				{
					SetYouTubeStatus( kYouTubeStatus_NotUploaded );
				}
			}
		}
	}
	else
	{
		m_pYouTubeUpload->SetVisible( false );
		m_pYouTubeView->SetVisible( false );
		m_pYouTubeShareURL->SetVisible( false );
		m_pShowRenderInfoButton->SetVisible( false );

		// Without this, the name label won't show when we automatically select the recently watched/saved
		// performance, because the cuts panel width/height isn't set when UpdateNameLabel() gets called
		// from within CCutsPanel::CCutsPanel().
		m_pCutsPanel->UpdateNameLabel( m_iSelectedPerformance );
	}
}

/*static*/ void CReplayDetailsPanel::OnPlayerWarningDlgConfirm( bool bConfirmed, void *pContext )
{
	CReplayDetailsPanel *pPanel = (CReplayDetailsPanel*)pContext;
	pPanel->ShowExportDialog();
}

void CReplayDetailsPanel::ShowExportDialog()
{
	IQueryableReplayItem *pReplayItem = m_pItemManager->GetItem( m_hReplayItem );
	if ( pReplayItem && pReplayItem->IsItemAMovie() )
	{
		IReplayMovie *pMovie = static_cast< IReplayMovie * >( pReplayItem );
		CFmtStr srcMovieFullFilename( "%s%s", g_pReplayMovieManager->GetRenderDir(), pMovie->GetMovieFilename() );
		if ( !g_pFullFileSystem->FileExists( srcMovieFullFilename.Access() ) )
		{
			ShowMessageBox( "#Replay_ExportMovieError_Title", "#Replay_ExportMovieNoFile_Text", "#GameUI_OK" );
			return;
		}
	}

	if ( m_hExportMovieDialog == NULL )
	{
		m_hExportMovieDialog = new FileOpenDialog(NULL, "#Replay_FindExportMovieLocation", FOD_SAVE );
#ifdef USE_WEBM_FOR_REPLAY
		m_hExportMovieDialog->AddFilter("*.webm", "#Replay_WebMMovieFiles", true );
#else
		m_hExportMovieDialog->AddFilter("*.mov", "#Replay_MovieFiles", true );
#endif
		m_hExportMovieDialog->AddActionSignalTarget( this );
		if ( !FStrEq( replay_movie_export_last_dir.GetString(), "" ) )
		{
			m_hExportMovieDialog->SetStartDirectory( replay_movie_export_last_dir.GetString() );
		}
	}
	m_hExportMovieDialog->DoModal(false);
	m_hExportMovieDialog->Activate();
}

void CReplayDetailsPanel::OnFileSelected( const char *fullpath )
{
	// this can take a while, put up a waiting cursor
	surface()->SetCursor(dc_hourglass);

	IQueryableReplayItem *pReplayItem = m_pItemManager->GetItem( m_hReplayItem );
	if ( pReplayItem && pReplayItem->IsItemAMovie() )
	{
		IReplayMovie *pMovie = static_cast< IReplayMovie * >( pReplayItem );
		CFmtStr srcMovieFullFilename( "%s%s", g_pReplayMovieManager->GetRenderDir(), pMovie->GetMovieFilename() );
		if ( !engine->CopyLocalFile( srcMovieFullFilename.Access(), fullpath ) )
		{
			ShowMessageBox( "#Replay_ExportMovieError_Title", "#Replay_ExportMovieError_Text", "#GameUI_OK" );
		}
		else
		{
			ShowMessageBox( "#Replay_ExportMovieSuccess_Title", "#Replay_ExportMovieSuccess_Text", "#GameUI_OK" );
		}
		char basepath[ MAX_PATH ];
		Q_ExtractFilePath( fullpath, basepath, sizeof( basepath ) );
		replay_movie_export_last_dir.SetValue( basepath );
	}

	// change the cursor back to normal
	surface()->SetCursor(dc_user);
}

void CReplayDetailsPanel::OnCommand( const char *pCommand )
{
	if ( FStrEq( pCommand, "delete_replayitem" ) )
	{
		ReplayUI_GetBrowserPanel()->AttemptToDeleteReplayItem( this, m_hReplayItem, m_pItemManager, m_iSelectedPerformance );
		return;
	}

	else if ( FStrEq( pCommand, "render_replay_dlg" ) )
	{
		ShowRenderDialog();
		return;
	}

	else if ( FStrEq( pCommand, "play" ) )
	{
		if ( engine->IsInGame() )
		{
			ShowPlayConfirmationDialog();
		}
		else
		{
			g_pClientReplayContext->PlayReplay( m_hReplay, m_iSelectedPerformance, true );
		}
		return;
	}

	else if ( FStrEq( pCommand, "exportmovie" ) )
	{
		IQueryableReplayItem *pReplayItem = m_pItemManager->GetItem( m_hReplayItem );
		if ( !pReplayItem || !pReplayItem->IsItemAMovie() )
			return;

		IReplayMovie *pMovie = static_cast< IReplayMovie * >( pReplayItem );
		if ( !pMovie )
			return;

		if ( replay_movie_reveal_warning.GetBool() )
		{
#ifdef USE_WEBM_FOR_REPLAY
			CTFMessageBoxDialog *pDialog = ShowMessageBox( "#Replay_Tip", "#Replay_UseVLCPlayer", "#Replay_ThanksIWill",  OnPlayerWarningDlgConfirm );			
#else
			CTFMessageBoxDialog *pDialog = ShowMessageBox( "#Replay_Tip", "#Replay_UseQuickTimePlayer", "#Replay_ThanksIWill",  OnPlayerWarningDlgConfirm );
#endif
			pDialog->SetContext( this );
			replay_movie_reveal_warning.SetValue( 0 );
		}
		else if ( pMovie->GetRenderSettings().m_bRaw )
		{
			ShowMessageBox( "#Replay_CantExport", "#YouTube_Upload_MovieIsRaw", "#GameUI_OK" );
		}
		else
		{
			ShowExportDialog();
		}
	}

	else if ( FStrEq( pCommand, "youtubeupload" ) )
	{
		IQueryableReplayItem *pReplayItem = m_pItemManager->GetItem( m_hReplayItem );
		if ( pReplayItem && pReplayItem->IsItemAMovie() )
		{
			IReplayMovie *pMovie = static_cast< IReplayMovie * >( pReplayItem );
			if ( !pMovie )
				return;

			if ( pMovie->GetRenderSettings().m_bRaw )
			{
				ShowMessageBox( "#Replay_CantUpload", "#YouTube_Upload_MovieIsRaw", "#GameUI_OK" );
				return;
			}

			// Movie already exists?
			CFmtStr srcMovieFullFilename( "%s%s", g_pReplayMovieManager->GetRenderDir(), pMovie->GetMovieFilename() );
			if ( !g_pFullFileSystem->FileExists( srcMovieFullFilename.Access() ) )
			{
				ShowMessageBox( "#YouTube_Upload_Title", "#YouTube_Upload_MissingFile", "#GameUI_OK" );
				return;
			}
			else if ( pMovie->IsUploaded() )
			{
				CTFGenericConfirmDialog *pDialog = ShowConfirmDialog( "#YouTube_Upload_Title", "#YouTube_FileAlreadyUploaded", "#GameUI_OK", "#GameuI_CancelBold", &ConfirmUploadMovie, this );
				pDialog->SetContext( this );
			}
			else
			{
				ConfirmUploadMovie( true, this );
			}
		}
	}

	else if ( FStrEq( pCommand, "viewyoutube" ) )
	{
		if ( steamapicontext && steamapicontext->SteamFriends() && m_pYouTubeResponseHandler->m_strVideoURL.IsEmpty() == false )
		{
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( m_pYouTubeResponseHandler->m_strVideoURL.Get() );
		}
	}

	else if ( FStrEq( pCommand, "shareyoutubeurl" ) )
	{
		system()->SetClipboardText( m_pYouTubeResponseHandler->m_strVideoURL.Get(), m_pYouTubeResponseHandler->m_strVideoURL.Length() );
		ShowMessageBox( "#Replay_CopyURL_Title", "#Replay_CopyURL_Text", "#GameUI_OK" );
	}	

	else if ( FStrEq( pCommand, "showrenderinfo" ) )
	{
		ShowRenderInfo();
	}

	else
	{
		BaseClass::OnCommand( pCommand );
	}
}

void CReplayDetailsPanel::ShowRenderInfo()
{
	IQueryableReplayItem *pReplayItem = m_pItemManager->GetItem( m_hReplayItem );
	if ( !pReplayItem || !pReplayItem->IsItemAMovie() )
		return;

	IReplayMovie *pMovie = static_cast< IReplayMovie * >( pReplayItem );
	const ReplayRenderSettings_t &Settings = pMovie->GetRenderSettings();
	const wchar_t *pCodecName = g_pVideo ? g_pVideo->GetCodecName( Settings.m_Codec ) : L"?";
	wchar_t *pAAEnabled = g_pVGuiLocalize->Find( Settings.m_bAAEnabled ? "#Replay_Enabled" : "#Replay_Disabled" );
	wchar_t *pRaw = g_pVGuiLocalize->Find( Settings.m_bRaw ? "#Replay_Yes" : "#Replay_No" );
	CFmtStr fmtRes( "%ix%i", Settings.m_nWidth, Settings.m_nHeight );
	CFmtStr fmtFramerate( "%.3f", Settings.m_FPS.GetFPS() );

	KeyValuesAD kvParams( "params" );
	kvParams->SetString( "res", fmtRes.Access() );
	kvParams->SetString( "framerate", fmtFramerate.Access() );
	kvParams->SetInt( "motionblurquality", Settings.m_nMotionBlurQuality );
	kvParams->SetInt( "encodingquality", Settings.m_nEncodingQuality );
	kvParams->SetWString( "codec", pCodecName );
	kvParams->SetWString( "antialiasing", pAAEnabled );
	kvParams->SetString( "rendertime", CReplayTime::FormatTimeString( pMovie->GetRenderTime() ) );
	kvParams->SetWString( "raw", pRaw );

	wchar_t wszStr[1024];
	g_pVGuiLocalize->ConstructString(
		wszStr,
		sizeof( wszStr ),
		"#Replay_MovieRenderInfo",
		kvParams
	);

	ShowMessageBox( "#Replay_RenderInfo", wszStr, "#GameUI_OK" );
}

void CReplayDetailsPanel::GoBack()
{
	// Send to parent
	GetParent()->OnCommand( "back" );
}

void CReplayDetailsPanel::ShowPlayConfirmationDialog()
{
	CConfirmDisconnectFromServerDialog *pConfirm = SETUP_PANEL( new CConfirmDisconnectFromServerDialog( this ) );
	if ( pConfirm )
	{
		pConfirm->Show();
	}
}

void CReplayDetailsPanel::OnConfirmDisconnect( KeyValues *pParams )
{
	if ( pParams->GetBool( "confirmed" ) )
	{
		g_pClientReplayContext->PlayReplay( m_hReplay, m_iSelectedPerformance, true );
	}
}

void CReplayDetailsPanel::OnMessage( const KeyValues* pParams, VPANEL hFromPanel )
{
	if ( FStrEq( pParams->GetName(), "ReplayItemDeleted" ) )
	{
		const int iPerformance = const_cast< KeyValues * >( pParams )->GetInt( "perf", -1 );
		if ( iPerformance >= 0 )
		{
			CReplayPerformance *pPerformance = GetGenericClassBasedReplay( m_hReplay )->GetPerformance( m_iSelectedPerformance );
			g_pReplayPerformanceManager->DeletePerformance( pPerformance );
			m_pCutsPanel->InvalidateLayout( true, false );	// Without this, m_nVisibleButtons will be wrong.
			m_pCutsPanel->OnPerformanceDeleted( m_iSelectedPerformance );
		}
		else
		{
			GoBack();
		}
		return;
	}

	BaseClass::OnMessage( pParams, hFromPanel );
}

void CReplayDetailsPanel::ShowRenderDialog()
{
	::ReplayUI_ShowRenderDialog( this, m_hReplay, false, m_iSelectedPerformance );
}

void CReplayDetailsPanel::FreeMovieFileLock()
{
	m_pPlaybackPanel->FreeMovieMaterial();
}

void CReplayDetailsPanel::SetYouTubeStatus( eYouTubeStatus status )
{
	m_pYouTubeUpload->SetVisible( status == kYouTubeStatus_CouldNotRetrieveInfo || status == kYouTubeStatus_NotUploaded );
	m_pYouTubeUpload->SetEnabled( status == kYouTubeStatus_CouldNotRetrieveInfo || status == kYouTubeStatus_NotUploaded );
	m_pYouTubeView->SetVisible( !m_pYouTubeUpload->IsVisible() );
	m_pYouTubeView->SetEnabled( status == kYouTubeStatus_RetrievedInfo );
	m_pYouTubeShareURL->SetEnabled( status == kYouTubeStatus_RetrievedInfo );
}

void CReplayDetailsPanel::OnMousePressed( MouseCode code )
{
	if ( code == MOUSE_LEFT )
	{
		RequestFocus();
	}
}

void CReplayDetailsPanel::OnKeyCodeTyped( KeyCode code )
{
	if ( code == KEY_DELETE )
	{
		OnCommand( "delete_replayitem" );
	}

	BaseClass::OnKeyCodeTyped( code );
}

#endif
