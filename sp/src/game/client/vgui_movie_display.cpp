//========= Copyright © 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "c_vguiscreen.h"
#include "vgui_controls/Label.h"
#include "vgui_bitmappanel.h"
#include <vgui/IVGui.h>
#include "c_slideshow_display.h"
#include "ienginevgui.h"
#include "fmtstr.h"
#include "vgui_controls/ImagePanel.h"
#include <vgui/ISurface.h>
#include "video/ivideoservices.h"
#include "engine/IEngineSound.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "c_movie_display.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

using namespace vgui;

struct VideoPlaybackInfo_t
{
	VideoPlaybackInfo_t( void ) : 
		m_pMaterial ( NULL ), 
		m_nSourceHeight(0), m_nSourceWidth(0),
		m_flU(0.0f),m_flV(0.0f) {}

	IMaterial		*m_pMaterial;
	int				m_nSourceHeight, m_nSourceWidth;		// Source movie's dimensions
	float			m_flU, m_flV;							// U,V ranges for video on its sheet
};

//-----------------------------------------------------------------------------
// Control screen 
//-----------------------------------------------------------------------------
class CMovieDisplayScreen : public CVGuiScreenPanel
{
	DECLARE_CLASS( CMovieDisplayScreen, CVGuiScreenPanel );

public:
	CMovieDisplayScreen( vgui::Panel *parent, const char *panelName );
	~CMovieDisplayScreen( void );

	virtual void ApplySchemeSettings( IScheme *pScheme );

	virtual bool Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData );
	virtual void OnTick( void );
	virtual void Paint( void );

private:
	bool	IsActive( void );

	void	SetupMovie( void );
	void	UpdateMovie( void );
	bool	BeginPlayback( const char *pFilename );
	void	CalculatePlaybackDimensions( int nSrcWidth, int nSrcHeight );

	inline void GetPanelPos( int &xpos, int &ypos )
	{
		xpos = ( (float) ( GetWide() - m_nPlaybackWidth ) / 2 );
		ypos = ( (float) ( GetTall() - m_nPlaybackHeight ) / 2 );
	}

private:

	// BINK playback info
	IVideoMaterial			*m_VideoMaterial;
	VideoPlaybackInfo_t		m_playbackInfo;
	CHandle<C_VGuiScreen>	m_hVGUIScreen;
	CHandle<C_MovieDisplay>	m_hScreenEntity;
	
	int				m_nTextureId;
	int				m_nPlaybackHeight;		// Playback dimensions (proper ration adjustments)
	int				m_nPlaybackWidth;
	bool			m_bBlackBackground;
	bool			m_bSlaved;
	bool			m_bInitialized;
	bool			m_bLastActiveState;		// HACK: I'd rather get a real callback...

	// VGUI specifics
	Label			*m_pDisplayTextLabel;

	Color			m_cDefault;
	Color			m_cInvisible;

	bool bIsAlreadyVisible;
};

DECLARE_VGUI_SCREEN_FACTORY( CMovieDisplayScreen, "movie_display_screen" );

CUtlVector <CMovieDisplayScreen *>	g_MovieDisplays;

//-----------------------------------------------------------------------------
// Constructor: 
//-----------------------------------------------------------------------------
CMovieDisplayScreen::CMovieDisplayScreen( vgui::Panel *parent, const char *panelName )
: BaseClass( parent, "CMovieDisplayScreen", vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/MovieDisplayScreen.res", "MovieDisplayScreen" ) ) 
{
	m_pDisplayTextLabel = new vgui::Label( this, "NumberDisplay", "testing!");

	m_VideoMaterial = NULL;
	m_nTextureId = -1;
	m_bBlackBackground = true;
	m_bSlaved = false;
	m_bInitialized = false;
	// Add ourselves to the global list of movie displays
	g_MovieDisplays.AddToTail( this );
	//m_VideoMaterial->SetMuted(true);
	m_bLastActiveState = IsActive();
}

//-----------------------------------------------------------------------------
// Purpose: Clean up the movie
//-----------------------------------------------------------------------------
CMovieDisplayScreen::~CMovieDisplayScreen( void )
{
	if ( g_pVideo != NULL && m_VideoMaterial != NULL )
	{
		g_pVideo->DestroyVideoMaterial( m_VideoMaterial );
		m_VideoMaterial = NULL;
	}

	// Clean up our texture reference
	g_pMatSystemSurface->DestroyTextureID( m_nTextureId );

	// Remove ourselves from the global list of movie displays
	g_MovieDisplays.FindAndRemove( this );
}

//-----------------------------------------------------------------------------
// Purpose: Setup our scheme
//-----------------------------------------------------------------------------
void CMovieDisplayScreen::ApplySchemeSettings( IScheme *pScheme )
{
	assert( pScheme );

	m_cDefault = Color( 255, 255, 255, 255 );
	m_cInvisible = Color( 0, 0, 0, 0 );	

	m_pDisplayTextLabel->SetFgColor( m_cDefault );
	m_pDisplayTextLabel->SetText( "" );
	m_pDisplayTextLabel->SetVisible( false );
}

//-----------------------------------------------------------------------------
// Initialization 
//-----------------------------------------------------------------------------
bool CMovieDisplayScreen::Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData )
{
	// Make sure we get ticked...
	vgui::ivgui()->AddTickSignal( GetVPanel() );

	if ( !BaseClass::Init( pKeyValues, pInitData ) )
		return false;

	// Save this for simplicity later on
	m_hVGUIScreen = dynamic_cast<C_VGuiScreen *>( GetEntity() );
	if ( m_hVGUIScreen != NULL )
	{
		// Also get the associated entity
		m_hScreenEntity = dynamic_cast<C_MovieDisplay *>(m_hVGUIScreen->GetOwnerEntity());
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to check our active state
//-----------------------------------------------------------------------------
bool CMovieDisplayScreen::IsActive( void )
{
	bool bScreenActive = false;
	if ( m_hVGUIScreen != NULL )
	{
		bScreenActive = m_hVGUIScreen->IsActive();
	}

	return bScreenActive;
}

//-----------------------------------------------------------------------------
// Purpose: Either become the master of a group of screens, or become a slave to another
//-----------------------------------------------------------------------------
void CMovieDisplayScreen::SetupMovie( void )
{
	// Only bother if we haven't been setup yet
	if ( m_bInitialized )
		return;

	const char *szGroupName = m_hScreenEntity->GetGroupName();

	CMovieDisplayScreen *pMasterScreen = NULL;
	for ( int i = 0; i < g_MovieDisplays.Count(); i++ )
	{
		// Must be valid and not us
		if ( g_MovieDisplays[i] == NULL || g_MovieDisplays[i] == this )
			continue;

		// Must have an associated movie entity
		if ( g_MovieDisplays[i]->m_hScreenEntity == NULL )
			continue;

		// Must have a group name to care
		if ( szGroupName[0] == NULL )
			continue;

		// Group names must match!
		// FIXME: Use an ID instead?
		const char *szTestGroupName = g_MovieDisplays[i]->m_hScreenEntity->GetGroupName();
		if ( Q_strnicmp( szTestGroupName, szGroupName, 128 ) )
			continue;

		// See if we've found a master display
		if ( g_MovieDisplays[i]->m_bInitialized && g_MovieDisplays[i]->m_bSlaved == false )
		{
			m_bSlaved = true;

			// Share the info from the master
			m_playbackInfo = g_MovieDisplays[i]->m_playbackInfo;

			// We need to calculate our own playback dimensions as we may be a different size than our parent
			CalculatePlaybackDimensions( m_playbackInfo.m_nSourceWidth, m_playbackInfo.m_nSourceHeight );

			// Bind our texture
			m_nTextureId = surface()->CreateNewTextureID( true );
			g_pMatSystemSurface->DrawSetTextureMaterial( m_nTextureId, m_playbackInfo.m_pMaterial );

			// Hold this as the master screen
			pMasterScreen = g_MovieDisplays[i];
			break;
		}
	}

	// We need to try again, we have no screen entity!
	if ( m_hScreenEntity == NULL )
		return;

	// No master found, become one
	if ( pMasterScreen == NULL )
	{
		const char *szFilename = m_hScreenEntity->GetMovieFilename();
		BeginPlayback( szFilename );
		m_bSlaved = false;
	}

	// Done
	m_bInitialized = true;
}

//-----------------------------------------------------------------------------
// Purpose: Deal with the details of the video playback
//-----------------------------------------------------------------------------
void CMovieDisplayScreen::UpdateMovie( void )
{
	// Only the master in a group updates the bink file
	if ( m_bSlaved )
		return;

	if ( m_VideoMaterial == NULL )
		return;

	// Get the current activity state of the screen
	bool bScreenActive = IsActive();

	// Pause if the game has paused
	if ( engine->IsPaused() || engine->Con_IsVisible() )
	{
		bScreenActive = false;
	}

	// See if we've changed our activity state
	if ( bScreenActive != m_bLastActiveState )
	{
		m_VideoMaterial->SetPaused( !bScreenActive );
	}

	// Updated
	m_bLastActiveState = bScreenActive;

	// Update the frame if we're currently enabled
	if ( bScreenActive  )
	{
		// Update our frame
		if ( m_VideoMaterial->Update() == false )
		{
			// Issue a close command
			// OnVideoOver();
			// StopPlayback();
		}

		if (!m_hScreenEntity->IsMuted())
		{
			m_VideoMaterial->SetMuted(false);
		}
	}
}

//-----------------------------------------------------------------------------
// Update the display string
//-----------------------------------------------------------------------------
void CMovieDisplayScreen::OnTick()
{
	BaseClass::OnTick();

	// Create our playback or slave to another screen already playing
	SetupMovie();

	// Now update the movie
	UpdateMovie();
}

//-----------------------------------------------------------------------------
// Purpose: Adjust the playback dimensions to properly account for our screen dimensions
//-----------------------------------------------------------------------------
void CMovieDisplayScreen::CalculatePlaybackDimensions( int nSrcWidth, int nSrcHeight )
{
	float flFrameRatio = ( (float) GetWide() / (float) GetTall() );
	float flVideoRatio = ( (float) nSrcWidth / (float) nSrcHeight );

	if ( flVideoRatio > flFrameRatio )
	{
		m_nPlaybackWidth = GetWide();
		m_nPlaybackHeight = ( GetWide() / flVideoRatio );
	}
	else if ( flVideoRatio < flFrameRatio )
	{
		m_nPlaybackWidth = ( GetTall() * flVideoRatio );
		m_nPlaybackHeight = GetTall();
	}
	else
	{
		m_nPlaybackWidth = GetWide();
		m_nPlaybackHeight = GetTall();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Begins playback of a movie
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMovieDisplayScreen::BeginPlayback( const char *pFilename )
{
	// need working video services
	if ( g_pVideo == NULL )
		return false;

	// Create a new video material
	if ( m_VideoMaterial != NULL )
	{
		g_pVideo->DestroyVideoMaterial( m_VideoMaterial );
		m_VideoMaterial = NULL;
	}

	// Create a globally unique name for this material
	char szMaterialName[256];
	
	// Append our group name if we have one
	const char *szGroupName = m_hScreenEntity->GetGroupName();
	if ( szGroupName[0] != NULL )
	{
		Q_snprintf( szMaterialName, sizeof(szMaterialName), "%s_%s", pFilename, szGroupName );
	}
	else
	{
		Q_snprintf( szMaterialName, sizeof(szMaterialName), "%s_%s", pFilename, m_hScreenEntity->GetEntityName() );
	}

	m_VideoMaterial = g_pVideo->CreateVideoMaterial( szMaterialName, pFilename, "GAME",
													VideoPlaybackFlags::DEFAULT_MATERIAL_OPTIONS,
													VideoSystem::DETERMINE_FROM_FILE_EXTENSION/*, m_bAllowAlternateMedia*/ );

	if ( m_VideoMaterial == NULL )
		return false;

	
	
		m_VideoMaterial->SetMuted(true); // FIXME: Allow?
	

	if ( m_hScreenEntity->IsLooping() )
	{
		m_VideoMaterial->SetLooping( true );
	}

	if ( m_VideoMaterial->HasAudio())
	{
		// We want to be the sole audio source
		enginesound->NotifyBeginMoviePlayback();
	}

	// Get our basic info from the movie
	m_VideoMaterial->GetVideoImageSize( &m_playbackInfo.m_nSourceWidth, &m_playbackInfo.m_nSourceHeight );
	m_VideoMaterial->GetVideoTexCoordRange( &m_playbackInfo.m_flU, &m_playbackInfo.m_flV );
	m_playbackInfo.m_pMaterial = m_VideoMaterial->GetMaterial();

	// Get our playback dimensions
	CalculatePlaybackDimensions( m_playbackInfo.m_nSourceWidth, m_playbackInfo.m_nSourceHeight );
	
	// Bind our texture
	m_nTextureId = surface()->CreateNewTextureID( true );
	g_pMatSystemSurface->DrawSetTextureMaterial( m_nTextureId, m_playbackInfo.m_pMaterial );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Update and draw the frame
//-----------------------------------------------------------------------------
void CMovieDisplayScreen::Paint( void )
{
	// Masters must keep the video updated
	if ( m_bSlaved == false && m_VideoMaterial == NULL )
	{
		BaseClass::Paint();
		return;
	}

	// Sit in the "center"
	int xpos, ypos;
	GetPanelPos( xpos, ypos );

	// Black out the background (we could omit drawing under the video surface, but this is straight-forward)
	if ( m_bBlackBackground )
	{
		surface()->DrawSetColor(  0, 0, 0, 255 );
		surface()->DrawFilledRect( 0, 0, GetWide(), GetTall() );
	}

	// Draw it
	surface()->DrawSetTexture( m_nTextureId );
	surface()->DrawSetColor(  255, 255, 255, 255 );
	surface()->DrawTexturedSubRect( xpos, ypos, xpos+m_nPlaybackWidth, ypos+m_nPlaybackHeight, 0.0f, 0.0f, m_playbackInfo.m_flU, m_playbackInfo.m_flV );
	
	// Parent's turn
	BaseClass::Paint();
}
