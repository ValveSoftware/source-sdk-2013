//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the big scary boom-boom machine Antlions fear.
//
//=============================================================================//

#include "cbase.h"
#include "EnvMessage.h"
#include "fmtstr.h"
#include "vguiscreen.h"
#include "filesystem.h"


#define SLIDESHOW_LIST_BUFFER_MAX 8192


struct SlideKeywordList_t
{
	char	szSlideKeyword[64];
};


class CSlideshowDisplay : public CBaseEntity
{
public:

	DECLARE_CLASS( CSlideshowDisplay, CBaseEntity );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	virtual ~CSlideshowDisplay();

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
	void	InputRemoveAllSlides( inputdata_t &inputdata );
	void	InputAddSlides( inputdata_t &inputdata );

	void	InputSetMinSlideTime( inputdata_t &inputdata );
	void	InputSetMaxSlideTime( inputdata_t &inputdata );

	void	InputSetCycleType( inputdata_t &inputdata );
	void	InputSetNoListRepeats( inputdata_t &inputdata );

private:

	// Control panel
	void GetControlPanelInfo( int nPanelIndex, const char *&pPanelName );
	void GetControlPanelClassName( int nPanelIndex, const char *&pPanelName );
	void SpawnControlPanels( void );
	void RestoreControlPanels( void );
	void BuildSlideShowImagesList( void );

private:

	CNetworkVar( bool, m_bEnabled );

	CNetworkString( m_szDisplayText, 128 );

	CNetworkString( m_szSlideshowDirectory, 128 );
	string_t	m_String_tSlideshowDirectory;

	CUtlVector<SlideKeywordList_t*>		m_SlideKeywordList;
	CNetworkArray( unsigned char, m_chCurrentSlideLists, 16 );

	CNetworkVar( float, m_fMinSlideTime );
	CNetworkVar( float, m_fMaxSlideTime );

	CNetworkVar( int, m_iCycleType );
	CNetworkVar( bool, m_bNoListRepeats );

	int		m_iScreenWidth;
	int		m_iScreenHeight;

	bool	m_bDoFullTransmit;

	typedef CHandle<CVGuiScreen>	ScreenHandle_t;
	CUtlVector<ScreenHandle_t>	m_hScreens;
};


LINK_ENTITY_TO_CLASS( vgui_slideshow_display, CSlideshowDisplay );

//-----------------------------------------------------------------------------
// Save/load 
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CSlideshowDisplay )
	DEFINE_FIELD( m_bEnabled, FIELD_BOOLEAN ),

	DEFINE_AUTO_ARRAY_KEYFIELD( m_szDisplayText, FIELD_CHARACTER, "displaytext" ),

	DEFINE_AUTO_ARRAY( m_szSlideshowDirectory, FIELD_CHARACTER ),
	DEFINE_KEYFIELD( m_String_tSlideshowDirectory, FIELD_STRING, "directory" ),

	// DEFINE_FIELD( m_SlideKeywordList, CUtlVector < SlideKeywordList_t* > ),
	DEFINE_AUTO_ARRAY( m_chCurrentSlideLists, FIELD_CHARACTER ),

	DEFINE_KEYFIELD( m_fMinSlideTime, FIELD_FLOAT, "minslidetime" ),
	DEFINE_KEYFIELD( m_fMaxSlideTime, FIELD_FLOAT, "maxslidetime" ),

	DEFINE_KEYFIELD( m_iCycleType, FIELD_INTEGER, "cycletype" ),
	DEFINE_KEYFIELD( m_bNoListRepeats, FIELD_BOOLEAN, "nolistrepeats" ),

	DEFINE_KEYFIELD( m_iScreenWidth, FIELD_INTEGER, "width" ),
	DEFINE_KEYFIELD( m_iScreenHeight, FIELD_INTEGER, "height" ),

	//DEFINE_FIELD( m_bDoFullTransmit, FIELD_BOOLEAN ),

	//DEFINE_UTLVECTOR( m_hScreens, FIELD_EHANDLE ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetDisplayText", InputSetDisplayText ),

	DEFINE_INPUTFUNC( FIELD_VOID, "RemoveAllSlides", InputRemoveAllSlides ),
	DEFINE_INPUTFUNC( FIELD_STRING, "AddSlides", InputAddSlides ),

	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetMinSlideTime", InputSetMinSlideTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetMaxSlideTime", InputSetMaxSlideTime ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetCycleType", InputSetCycleType ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetNoListRepeats", InputSetNoListRepeats ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CSlideshowDisplay, DT_SlideshowDisplay )
	SendPropBool( SENDINFO(m_bEnabled) ),
	SendPropString( SENDINFO( m_szDisplayText ) ),
	SendPropString( SENDINFO( m_szSlideshowDirectory ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_chCurrentSlideLists), SendPropInt( SENDINFO_ARRAY(m_chCurrentSlideLists), 8, SPROP_UNSIGNED ) ),
	SendPropFloat( SENDINFO(m_fMinSlideTime), 11, 0, 0.0f, 20.0f ),
	SendPropFloat( SENDINFO(m_fMaxSlideTime), 11, 0, 0.0f, 20.0f ),
	SendPropInt( SENDINFO(m_iCycleType), 2, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO(m_bNoListRepeats) ),
END_SEND_TABLE()


CSlideshowDisplay::~CSlideshowDisplay()
{
	int i;
	// Kill the control panels
	for ( i = m_hScreens.Count(); --i >= 0; )
	{
		DestroyVGuiScreen( m_hScreens[i].Get() );
	}
	m_hScreens.RemoveAll();
}

//-----------------------------------------------------------------------------
// Read in worldcraft data...
//-----------------------------------------------------------------------------
bool CSlideshowDisplay::KeyValue( const char *szKeyName, const char *szValue ) 
{
	//!! temp hack, until worldcraft is fixed
	// strip the # tokens from (duplicate) key names
	char *s = (char *)strchr( szKeyName, '#' );
	if ( s )
	{
		*s = '\0';
	}

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

int CSlideshowDisplay::UpdateTransmitState()
{
	if ( m_bDoFullTransmit )
	{
		m_bDoFullTransmit = false;
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	return SetTransmitState( FL_EDICT_FULLCHECK );
}

void CSlideshowDisplay::SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
{
	// Are we already marked for transmission?
	if ( pInfo->m_pTransmitEdict->Get( entindex() ) )
		return;

	BaseClass::SetTransmit( pInfo, bAlways );

	// Force our screens to be sent too.
	for ( int i=0; i < m_hScreens.Count(); i++ )
	{
		CVGuiScreen *pScreen = m_hScreens[i].Get();
		pScreen->SetTransmit( pInfo, bAlways );
	}
}

void CSlideshowDisplay::Spawn( void )
{
	Q_strcpy( m_szSlideshowDirectory.GetForModify(), m_String_tSlideshowDirectory.ToCStr() );
	Precache();

	BaseClass::Spawn();

	m_bEnabled = false;
	
	// Clear out selected list
	m_chCurrentSlideLists.GetForModify( 0 ) = 0;	// Select all slides to begin with
	for ( int i = 1; i < 16; ++i )
		m_chCurrentSlideLists.GetForModify( i ) = (unsigned char)-1;

	SpawnControlPanels();

	ScreenVisible( m_bEnabled );

	m_bDoFullTransmit = true;
}

void CSlideshowDisplay::Precache( void )
{
	BaseClass::Precache();

	BuildSlideShowImagesList();

	PrecacheVGuiScreen( "slideshow_display_screen" );
}

void CSlideshowDisplay::OnRestore( void )
{
	BaseClass::OnRestore();

	BuildSlideShowImagesList();

	RestoreControlPanels();

	ScreenVisible( m_bEnabled );
}

void CSlideshowDisplay::ScreenVisible( bool bVisible )
{
	for ( int iScreen = 0; iScreen < m_hScreens.Count(); ++iScreen )
	{
		CVGuiScreen *pScreen = m_hScreens[ iScreen ].Get();
		if ( bVisible )
			pScreen->RemoveEffects( EF_NODRAW );
		else
			pScreen->AddEffects( EF_NODRAW );
	}
}

void CSlideshowDisplay::Disable( void )
{
	if ( !m_bEnabled )
		return;

	m_bEnabled = false;

	ScreenVisible( false );
}

void CSlideshowDisplay::Enable( void )
{
	if ( m_bEnabled )
		return;

	m_bEnabled = true;

	ScreenVisible( true );
}


void CSlideshowDisplay::InputDisable( inputdata_t &inputdata )
{
	Disable();
}

void CSlideshowDisplay::InputEnable( inputdata_t &inputdata )
{
	Enable();
}


void CSlideshowDisplay::InputSetDisplayText( inputdata_t &inputdata )
{
	Q_strcpy( m_szDisplayText.GetForModify(), inputdata.value.String() );
}

void CSlideshowDisplay::InputRemoveAllSlides( inputdata_t &inputdata )
{
	// Clear out selected list
	for ( int i = 0; i < 16; ++i )
		m_chCurrentSlideLists.GetForModify( i ) = (unsigned char)-1;
}

void CSlideshowDisplay::InputAddSlides( inputdata_t &inputdata )
{
	// Find the list with the current keyword
	int iList;
	for ( iList = 0; iList < m_SlideKeywordList.Count(); ++iList )
	{
		if ( Q_strcmp( m_SlideKeywordList[ iList ]->szSlideKeyword, inputdata.value.String() ) == 0 )
			break;
	}

	if ( iList < m_SlideKeywordList.Count() )
	{
		// Found the keyword list, so add this index to the selected lists
		int iNumCurrentSlideLists;
		for ( iNumCurrentSlideLists = 0; iNumCurrentSlideLists < 16; ++iNumCurrentSlideLists )
		{
			if ( m_chCurrentSlideLists[ iNumCurrentSlideLists ] == (unsigned char)-1 )
				break;
		}

		if ( iNumCurrentSlideLists >= 16 )
			return;

		m_chCurrentSlideLists.GetForModify( iNumCurrentSlideLists ) = iList;
	}
}


void CSlideshowDisplay::InputSetMinSlideTime( inputdata_t &inputdata )
{
	m_fMinSlideTime = inputdata.value.Float();
}

void CSlideshowDisplay::InputSetMaxSlideTime( inputdata_t &inputdata )
{
	m_fMaxSlideTime = inputdata.value.Float();
}


void CSlideshowDisplay::InputSetCycleType( inputdata_t &inputdata )
{
	m_iCycleType = inputdata.value.Int();
}

void CSlideshowDisplay::InputSetNoListRepeats( inputdata_t &inputdata )
{
	m_bNoListRepeats = inputdata.value.Bool();
}


void CSlideshowDisplay::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
{
	pPanelName = "slideshow_display_screen";
}

void CSlideshowDisplay::GetControlPanelClassName( int nPanelIndex, const char *&pPanelName )
{
	pPanelName = "vgui_screen";
}

//-----------------------------------------------------------------------------
// This is called by the base object when it's time to spawn the control panels
//-----------------------------------------------------------------------------
void CSlideshowDisplay::SpawnControlPanels()
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

		CVGuiScreen *pScreen = CreateVGuiScreen( pScreenClassname, pScreenName, this, this, -1 );
		pScreen->ChangeTeam( GetTeamNumber() );
		pScreen->SetActualSize( flWidth, flHeight );
		pScreen->SetActive( true );
		pScreen->MakeVisibleOnlyToTeammates( false );
		pScreen->SetTransparency( true );
		int nScreen = m_hScreens.AddToTail( );
		m_hScreens[nScreen].Set( pScreen );

		return;
	}
}

void CSlideshowDisplay::RestoreControlPanels( void )
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
			int nScreen = m_hScreens.AddToTail( );
			m_hScreens[nScreen].Set( pScreen );	
			pScreen->SetActive( true );
		}

		return;
	}
}

void CSlideshowDisplay::BuildSlideShowImagesList( void )
{
	FileFindHandle_t matHandle;
	char szDirectory[_MAX_PATH];
	char szMatFileName[_MAX_PATH] = {'\0'};
	char szFileBuffer[ SLIDESHOW_LIST_BUFFER_MAX ];
	char *pchCurrentLine = NULL;

	if ( IsX360() )
	{
		Q_snprintf( szDirectory, sizeof( szDirectory ), "materials/vgui/%s/slides.txt", m_szSlideshowDirectory.Get() );

		FileHandle_t fh = g_pFullFileSystem->Open( szDirectory, "rt" );
		if ( !fh )
		{
			DevWarning( "Couldn't read slideshow image file %s!", szDirectory );
			return;
		}

		int iFileSize = MIN( g_pFullFileSystem->Size( fh ), SLIDESHOW_LIST_BUFFER_MAX );

		int iBytesRead = g_pFullFileSystem->Read( szFileBuffer, iFileSize, fh );
		g_pFullFileSystem->Close( fh );

		// Ensure we don't write outside of our buffer
		if ( iBytesRead > iFileSize )
			iBytesRead = iFileSize;
		szFileBuffer[ iBytesRead ] = '\0';

		pchCurrentLine = szFileBuffer;

		// Seek to end of first line
		char *pchNextLine = pchCurrentLine;
		while ( *pchNextLine != '\0' && *pchNextLine != '\n' && *pchNextLine != ' ' )
			++pchNextLine;

		if ( *pchNextLine != '\0' )
		{
			// Mark end of string
			*pchNextLine = '\0';

			// Seek to start of next string
			++pchNextLine;
			while ( *pchNextLine != '\0' && ( *pchNextLine == '\n' || *pchNextLine == ' ' ) )
				++pchNextLine;
		}

		Q_strncpy( szMatFileName, pchCurrentLine, sizeof(szMatFileName) );
		pchCurrentLine = pchNextLine;
	}
	else
	{
		Q_snprintf( szDirectory, sizeof( szDirectory ), "materials/vgui/%s/*.vmt", m_szSlideshowDirectory.Get() );
		const char *pMatFileName = g_pFullFileSystem->FindFirst( szDirectory, &matHandle );

		if ( pMatFileName )
			Q_strncpy( szMatFileName, pMatFileName, sizeof(szMatFileName) );
	}

	int iSlideIndex = 0;

	while ( szMatFileName[ 0 ] )
	{
		char szFileName[_MAX_PATH];
		Q_snprintf( szFileName, sizeof( szFileName ), "vgui/%s/%s", m_szSlideshowDirectory.Get(), szMatFileName );
		szFileName[ Q_strlen( szFileName ) - 4 ] = '\0';

		PrecacheMaterial( szFileName );	

		// Get material keywords
		char szFullFileName[_MAX_PATH];
		Q_snprintf( szFullFileName, sizeof( szFullFileName ), "materials/vgui/%s/%s", m_szSlideshowDirectory.Get(), szMatFileName );

		KeyValues *pMaterialKeys = new KeyValues( "material" );
		bool bLoaded = pMaterialKeys->LoadFromFile( g_pFullFileSystem, szFullFileName, NULL );

		if ( bLoaded )
		{
			char szKeywords[ 256 ];
			Q_strcpy( szKeywords, pMaterialKeys->GetString( "%keywords", "" ) );

			char *pchKeyword = szKeywords;

			while ( pchKeyword[ 0 ] != '\0' )
			{
				char *pNextKeyword = pchKeyword;

				// Skip commas and spaces
				while ( pNextKeyword[ 0 ] != '\0' && pNextKeyword[ 0 ] != ',' )
					++pNextKeyword;

				if ( pNextKeyword[ 0 ] != '\0' )
				{
					pNextKeyword[ 0 ] = '\0';
					++pNextKeyword;

					while ( pNextKeyword[ 0 ] != '\0' && ( pNextKeyword[ 0 ] == ',' || pNextKeyword[ 0 ] == ' ' ) )
						++pNextKeyword;
				}

				// Find the list with the current keyword
				int iList;
				for ( iList = 0; iList < m_SlideKeywordList.Count(); ++iList )
				{
					if ( Q_strcmp( m_SlideKeywordList[ iList ]->szSlideKeyword, pchKeyword ) == 0 )
						break;
				}

				if ( iList >= m_SlideKeywordList.Count() )
				{
					// Couldn't find the list, so create it
					iList = m_SlideKeywordList.AddToTail( new SlideKeywordList_t );
					Q_strcpy( m_SlideKeywordList[ iList ]->szSlideKeyword, pchKeyword );
				}

				pchKeyword = pNextKeyword;
			}
		}

		// Find the generic list
		int iList;
		for ( iList = 0; iList < m_SlideKeywordList.Count(); ++iList )
		{
			if ( Q_strcmp( m_SlideKeywordList[ iList ]->szSlideKeyword, "" ) == 0 )
				break;
		}

		if ( iList >= m_SlideKeywordList.Count() )
		{
			// Couldn't find the generic list, so create it
			iList = m_SlideKeywordList.AddToHead( new SlideKeywordList_t );
			Q_strcpy( m_SlideKeywordList[ iList ]->szSlideKeyword, "" );
		}

		if ( IsX360() )
		{
			// Seek to end of first line
			char *pchNextLine = pchCurrentLine;
			while ( *pchNextLine != '\0' && *pchNextLine != '\n' && *pchNextLine != ' ' )
				++pchNextLine;

			if ( *pchNextLine != '\0' )
			{
				// Mark end of string
				*pchNextLine = '\0';

				// Seek to start of next string
				++pchNextLine;
				while ( *pchNextLine != '\0' && ( *pchNextLine == '\n' || *pchNextLine == ' ' ) )
					++pchNextLine;
			}

			Q_strncpy( szMatFileName, pchCurrentLine, sizeof(szMatFileName) );
			pchCurrentLine = pchNextLine;
		}
		else
		{
			const char *pMatFileName = g_pFullFileSystem->FindNext( matHandle );

			if ( pMatFileName )
				Q_strncpy( szMatFileName, pMatFileName, sizeof(szMatFileName) );
			else
				szMatFileName[ 0 ] = '\0';
		}

		++iSlideIndex;
	}

	if ( !IsX360() )
	{
		g_pFullFileSystem->FindClose( matHandle );
	}
}