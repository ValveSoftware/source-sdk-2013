//========= Copyright © 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: Allows movies to be played as a VGUI screen in the world
//
//=====================================================================================//

#include "cbase.h"
#include "EnvMessage.h"
#include "fmtstr.h"
#include "vguiscreen.h"
#include "filesystem.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

class CMovieDisplay : public CBaseEntity
{
public:

	DECLARE_CLASS( CMovieDisplay, CBaseEntity );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	virtual ~CMovieDisplay();

	virtual bool KeyValue( const char *szKeyName, const char *szValue );

	virtual int  UpdateTransmitState();
	virtual void SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways );

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void OnRestore( void );

	void	ScreenVisible( bool bVisible );

	void	Disable( void );
	void	Enable( void );

	void	InputDisable( inputdata_t &inputdata );
	void	InputEnable( inputdata_t &inputdata );

	void	InputSetDisplayText( inputdata_t &inputdata );

private:

	// Control panel
	void GetControlPanelInfo( int nPanelIndex, const char *&pPanelName );
	void GetControlPanelClassName( int nPanelIndex, const char *&pPanelName );
	void SpawnControlPanels( void );
	void RestoreControlPanels( void );

private:
	CNetworkVar( bool, m_bEnabled );
	CNetworkVar( bool, m_bLooping );

	CNetworkString( m_szDisplayText, 128 );

	// Filename of the movie to play
	CNetworkString( m_szMovieFilename, 128 );
	string_t	m_strMovieFilename;

	// "Group" name.  Screens of the same group name will play the same movie at the same time
	// Effectively this lets multiple screens tune to the same "channel" in the world
	CNetworkString( m_szGroupName, 128 );
	string_t	m_strGroupName;

	int			m_iScreenWidth;
	int			m_iScreenHeight;

	bool		m_bDoFullTransmit;

	CHandle<CVGuiScreen>	m_hScreen;
};

LINK_ENTITY_TO_CLASS( vgui_movie_display, CMovieDisplay );

//-----------------------------------------------------------------------------
// Save/load 
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CMovieDisplay )

	DEFINE_FIELD( m_bEnabled, FIELD_BOOLEAN ),

	DEFINE_AUTO_ARRAY_KEYFIELD( m_szDisplayText, FIELD_CHARACTER, "displaytext" ),

	DEFINE_AUTO_ARRAY( m_szMovieFilename, FIELD_CHARACTER ),
	DEFINE_KEYFIELD( m_strMovieFilename, FIELD_STRING, "moviefilename" ),

	DEFINE_AUTO_ARRAY( m_szGroupName, FIELD_CHARACTER ),
	DEFINE_KEYFIELD( m_strGroupName, FIELD_STRING, "groupname" ),

	DEFINE_KEYFIELD( m_iScreenWidth, FIELD_INTEGER, "width" ),
	DEFINE_KEYFIELD( m_iScreenHeight, FIELD_INTEGER, "height" ),
	DEFINE_KEYFIELD( m_bLooping, FIELD_BOOLEAN, "looping" ),

	DEFINE_FIELD( m_bDoFullTransmit, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_hScreen, FIELD_EHANDLE ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetDisplayText", InputSetDisplayText ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CMovieDisplay, DT_MovieDisplay )
	SendPropBool( SENDINFO( m_bEnabled ) ),
	SendPropBool( SENDINFO( m_bLooping ) ),
	SendPropString( SENDINFO( m_szMovieFilename ) ),
	SendPropString( SENDINFO( m_szGroupName ) ),
END_SEND_TABLE()

CMovieDisplay::~CMovieDisplay()
{
	DestroyVGuiScreen( m_hScreen.Get() );
}

//-----------------------------------------------------------------------------
// Read in Hammer data
//-----------------------------------------------------------------------------
bool CMovieDisplay::KeyValue( const char *szKeyName, const char *szValue ) 
{
	// NOTE: Have to do these separate because they set two values instead of one
	if( FStrEq( szKeyName, "angles" ) )
	{
		Assert( GetMoveParent() == NULL );
		QAngle angles;
		UTIL_StringToVector( angles.Base(), szValue );

		// Because the vgui screen basis is strange (z is front, y is up, x is right)
		// we need to rotate the typical basis before applying it
		VMatrix mat, rotation, tmp;
		MatrixFromAngles( angles, mat );
		MatrixBuildRotationAboutAxis( rotation, Vector( 0, 1, 0 ), 90 );
		MatrixMultiply( mat, rotation, tmp );
		MatrixBuildRotateZ( rotation, 90 );
		MatrixMultiply( tmp, rotation, mat );
		MatrixToAngles( mat, angles );
		SetAbsAngles( angles );

		return true;
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int CMovieDisplay::UpdateTransmitState()
{
	if ( m_bDoFullTransmit )
	{
		m_bDoFullTransmit = false;
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	return SetTransmitState( FL_EDICT_FULLCHECK );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CMovieDisplay::SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
{
	// Are we already marked for transmission?
	if ( pInfo->m_pTransmitEdict->Get( entindex() ) )
		return;

	BaseClass::SetTransmit( pInfo, bAlways );

	// Force our screen to be sent too.
	m_hScreen->SetTransmit( pInfo, bAlways );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CMovieDisplay::Spawn( void )
{
	// Move the strings into a networkable form
	Q_strcpy( m_szMovieFilename.GetForModify(), m_strMovieFilename.ToCStr() );
	Q_strcpy( m_szGroupName.GetForModify(), m_strGroupName.ToCStr() );

	Precache();

	BaseClass::Spawn();

	m_bEnabled = false;

	SpawnControlPanels();

	ScreenVisible( m_bEnabled );

	m_bDoFullTransmit = true;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CMovieDisplay::Precache( void )
{
	BaseClass::Precache();

	PrecacheVGuiScreen( "video_display_screen" );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CMovieDisplay::OnRestore( void )
{
	BaseClass::OnRestore();

	RestoreControlPanels();

	ScreenVisible( m_bEnabled );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CMovieDisplay::ScreenVisible( bool bVisible )
{
	// Set its active state
	m_hScreen->SetActive( bVisible );

	if ( bVisible )
	{
		m_hScreen->RemoveEffects( EF_NODRAW );
	}
	else
	{
		m_hScreen->AddEffects( EF_NODRAW );
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CMovieDisplay::Disable( void )
{
	if ( !m_bEnabled )
		return;

	m_bEnabled = false;

	ScreenVisible( false );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CMovieDisplay::Enable( void )
{
	if ( m_bEnabled )
		return;

	m_bEnabled = true;

	ScreenVisible( true );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CMovieDisplay::InputDisable( inputdata_t &inputdata )
{
	Disable();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CMovieDisplay::InputEnable( inputdata_t &inputdata )
{
	Enable();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CMovieDisplay::InputSetDisplayText( inputdata_t &inputdata )
{
	Q_strcpy( m_szDisplayText.GetForModify(), inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CMovieDisplay::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
{
	pPanelName = "movie_display_screen";
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CMovieDisplay::GetControlPanelClassName( int nPanelIndex, const char *&pPanelName )
{
	pPanelName = "vgui_screen";
}

//-----------------------------------------------------------------------------
// This is called by the base object when it's time to spawn the control panels
//-----------------------------------------------------------------------------
void CMovieDisplay::SpawnControlPanels()
{
	int nPanel;
	for ( nPanel = 0; true; ++nPanel )
	{
		const char *pScreenName;
		GetControlPanelInfo( nPanel, pScreenName );
		if (!pScreenName)
			continue;

		const char *pScreenClassname;
		GetControlPanelClassName( nPanel, pScreenClassname );
		if ( !pScreenClassname )
			continue;

		float flWidth = m_iScreenWidth;
		float flHeight = m_iScreenHeight;

		CVGuiScreen *pScreen = CreateVGuiScreen( pScreenClassname, pScreenName, this, this, 0 );
		pScreen->ChangeTeam( GetTeamNumber() );
		pScreen->SetActualSize( flWidth, flHeight );
		pScreen->SetActive( true );
		pScreen->MakeVisibleOnlyToTeammates( false );
		pScreen->SetTransparency( true );
		m_hScreen = pScreen;

		return;
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CMovieDisplay::RestoreControlPanels( void )
{
	int nPanel;
	for ( nPanel = 0; true; ++nPanel )
	{
		const char *pScreenName;
		GetControlPanelInfo( nPanel, pScreenName );
		if (!pScreenName)
			continue;

		const char *pScreenClassname;
		GetControlPanelClassName( nPanel, pScreenClassname );
		if ( !pScreenClassname )
			continue;

		CVGuiScreen *pScreen = (CVGuiScreen *)gEntList.FindEntityByClassname( NULL, pScreenClassname );

		while ( ( pScreen && pScreen->GetOwnerEntity() != this ) || Q_strcmp( pScreen->GetPanelName(), pScreenName ) != 0 )
		{
			pScreen = (CVGuiScreen *)gEntList.FindEntityByClassname( pScreen, pScreenClassname );
		}

		if ( pScreen )
		{
			m_hScreen = pScreen;
			m_hScreen->SetActive( true );
		}

		return;
	}
}
