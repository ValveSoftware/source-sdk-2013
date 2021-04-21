//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Displays easy, flexible VGui text. Mapbase equivalent of point_worldtext.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "vguiscreen.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_TESTDISPLAY_START_DISABLED (1 << 0)

//-----------------------------------------------------------------------------
// vgui_text_display
//-----------------------------------------------------------------------------
class CVGuiTextDisplay : public CBaseEntity
{
public:

	DECLARE_CLASS( CVGuiTextDisplay, CBaseEntity );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CVGuiTextDisplay();
	virtual ~CVGuiTextDisplay();

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
	void	InputToggle( inputdata_t &inputdata );

	void	InputSetMessage( inputdata_t &inputdata );
	void	InputSetTextAlignment( inputdata_t &inputdata );
	void	InputSetFont( inputdata_t &inputdata );
	void	InputSetResolution( inputdata_t &inputdata );
	void	InputSetTextSize( inputdata_t &inputdata );

private:

	// Control panel
	void GetControlPanelInfo( int nPanelIndex, const char *&pPanelName );
	void GetControlPanelClassName( int nPanelIndex, const char *&pPanelName );
	void SpawnControlPanels( void );
	void RestoreControlPanels( void );

private:
	CNetworkVar( bool, m_bEnabled );

	CNetworkString( m_szDisplayText, 256 );
	CNetworkVar( int, m_iContentAlignment );
	CNetworkString( m_szFont, 64 );
	CNetworkVar( int, m_iResolution );
	float m_flTextSize;

	//CNetworkColor32( m_DisplayColor ); // Use render color

	bool		m_bDoFullTransmit;

	CHandle<CVGuiScreen>	m_hScreen;
};

LINK_ENTITY_TO_CLASS( vgui_text_display, CVGuiTextDisplay );

//-----------------------------------------------------------------------------
// Save/load 
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CVGuiTextDisplay )

	DEFINE_FIELD( m_bEnabled, FIELD_BOOLEAN ),

	DEFINE_AUTO_ARRAY_KEYFIELD( m_szDisplayText, FIELD_CHARACTER, "message" ),
	DEFINE_KEYFIELD( m_iContentAlignment, FIELD_INTEGER, "alignment" ),
	DEFINE_AUTO_ARRAY_KEYFIELD( m_szFont, FIELD_CHARACTER, "font" ),
	DEFINE_KEYFIELD( m_iResolution, FIELD_INTEGER, "resolution" ),
	DEFINE_KEYFIELD( m_flTextSize, FIELD_FLOAT, "textsize" ),

	DEFINE_FIELD( m_bDoFullTransmit, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_hScreen, FIELD_EHANDLE ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetMessage", InputSetMessage ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetTextAlignment", InputSetTextAlignment ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetFont", InputSetFont ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetResolution", InputSetResolution ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetPanelSize", InputSetTextSize ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CVGuiTextDisplay, DT_VGuiTextDisplay )
	SendPropBool( SENDINFO( m_bEnabled ) ),
	SendPropString( SENDINFO( m_szDisplayText ) ),
	SendPropInt( SENDINFO( m_iContentAlignment ) ),
	SendPropString( SENDINFO( m_szFont ) ),
	SendPropInt( SENDINFO( m_iResolution ) ),
END_SEND_TABLE()

CVGuiTextDisplay::CVGuiTextDisplay()
{
	m_flTextSize = 100.0f;
	m_iResolution = 200;
	m_iContentAlignment = 7; // a_south
}

CVGuiTextDisplay::~CVGuiTextDisplay()
{
	DestroyVGuiScreen( m_hScreen.Get() );
}

//-----------------------------------------------------------------------------
// Read in Hammer data
//-----------------------------------------------------------------------------
bool CVGuiTextDisplay::KeyValue( const char *szKeyName, const char *szValue )
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
	}
	else if( FStrEq( szKeyName, "message" ) )
	{
		Q_strcpy( m_szDisplayText.GetForModify(), szValue );
	}
	else if( FStrEq( szKeyName, "font" ) )
	{
		Q_strcpy( m_szFont.GetForModify(), szValue );
	}
	else if( FStrEq( szKeyName, "color" ) )
	{
		// Use render color
		return BaseClass::KeyValue( "rendercolor", szValue );
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int CVGuiTextDisplay::UpdateTransmitState()
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
void CVGuiTextDisplay::SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
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
void CVGuiTextDisplay::Spawn( void )
{
	Precache();

	BaseClass::Spawn();

	m_bEnabled = !HasSpawnFlags( SF_TESTDISPLAY_START_DISABLED );

	SpawnControlPanels();

	ScreenVisible( m_bEnabled );

	m_bDoFullTransmit = true;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CVGuiTextDisplay::Precache( void )
{
	BaseClass::Precache();

	PrecacheVGuiScreen( "text_display_panel" );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CVGuiTextDisplay::OnRestore( void )
{
	BaseClass::OnRestore();

	RestoreControlPanels();

	ScreenVisible( m_bEnabled );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CVGuiTextDisplay::ScreenVisible( bool bVisible )
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
void CVGuiTextDisplay::Disable( void )
{
	if ( !m_bEnabled )
		return;

	m_bEnabled = false;

	ScreenVisible( false );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CVGuiTextDisplay::Enable( void )
{
	if ( m_bEnabled )
		return;

	m_bEnabled = true;

	ScreenVisible( true );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CVGuiTextDisplay::InputDisable( inputdata_t &inputdata )
{
	Disable();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CVGuiTextDisplay::InputEnable( inputdata_t &inputdata )
{
	Enable();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CVGuiTextDisplay::InputToggle( inputdata_t &inputdata )
{
	m_bEnabled ? Disable() : Enable();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CVGuiTextDisplay::InputSetMessage( inputdata_t &inputdata )
{
	Q_strcpy( m_szDisplayText.GetForModify(), inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CVGuiTextDisplay::InputSetTextAlignment( inputdata_t &inputdata )
{
	m_iContentAlignment = inputdata.value.Int();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CVGuiTextDisplay::InputSetFont( inputdata_t &inputdata )
{
	Q_strcpy( m_szFont.GetForModify(), inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CVGuiTextDisplay::InputSetResolution( inputdata_t &inputdata )
{
	m_iResolution = inputdata.value.Int();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CVGuiTextDisplay::InputSetTextSize( inputdata_t &inputdata )
{
	m_flTextSize = inputdata.value.Float();

	if (m_hScreen)
	{
		m_hScreen->SetActualSize( m_flTextSize, m_flTextSize );
		m_hScreen->SetLocalOrigin( m_hScreen->CollisionProp()->OBBCenter() * -1.0f );
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CVGuiTextDisplay::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
{
	pPanelName = "text_display_panel";
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CVGuiTextDisplay::GetControlPanelClassName( int nPanelIndex, const char *&pPanelName )
{
	pPanelName = "vgui_screen";
}

//-----------------------------------------------------------------------------
// This is called by the base object when it's time to spawn the control panels
//-----------------------------------------------------------------------------
void CVGuiTextDisplay::SpawnControlPanels()
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

		float flWidth = m_flTextSize;
		float flHeight = m_flTextSize;

		CVGuiScreen *pScreen = CreateVGuiScreen( pScreenClassname, pScreenName, this, this, 0 );
		pScreen->ChangeTeam( GetTeamNumber() );
		pScreen->SetActualSize( flWidth, flHeight );
		pScreen->SetLocalOrigin( pScreen->CollisionProp()->OBBCenter() * -1.0f );
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
void CVGuiTextDisplay::RestoreControlPanels( void )
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
