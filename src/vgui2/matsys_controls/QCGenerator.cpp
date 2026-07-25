//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#if defined(WIN32) && !defined( _X360 )
#include <windows.h>
#endif
#include "filesystem.h"
#include "filesystem_init.h"
#include "appframework/IAppSystemGroup.h"
#include "appframework/IAppSystem.h"
#include "appframework/AppFramework.h"
#include "filesystem_helpers.h"

#include "matsys_controls/QCGenerator.h"
#include "tier1/KeyValues.h"
#include "tier2/vconfig.h"
#include "vgui_controls/ListPanel.h"
#include "vgui_controls/TextEntry.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/FileOpenDialog.h"
#include "vgui_controls/DirectorySelectDialog.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/MessageBox.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "vgui/Cursor.h"
#include "vgui_controls/KeyBoardEditorDialog.h"

#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#endif

using namespace vgui;

#define MAX_KEYVALUE	1024


//-----------------------------------------------------------------------------
// Purpose: returns a pointer to the 'count' occurence of a character from the end of a string
//			returns 0 of there aren't 'count' number of the character in the string
//-----------------------------------------------------------------------------
char *strrchrcount(char *string, int character, int count )
{
	int j = count;
	int numChars = V_strlen( string );
	for( int i = numChars; i > 0; i-- )
	{
		if( string[i-1] == character )
		{
			j--;
		}
		if( j == 0 )
		{
			return string + i-1;
		}
	}
	return 0;
}



class CModalPreserveMessageBox : public vgui::MessageBox
{
public:
	CModalPreserveMessageBox(const char *title, const char *text, vgui::Panel *parent)
		: vgui::MessageBox( title, text, parent )
	{
		m_PrevAppFocusPanel = vgui::input()->GetAppModalSurface();
	}

	~CModalPreserveMessageBox()
	{
		vgui::input()->SetAppModalSurface( m_PrevAppFocusPanel );
	}


public:
	vgui::VPANEL	m_PrevAppFocusPanel;
};

void VGUIMessageBox( vgui::Panel *pParent, const char *pTitle, const char *pMsg, ... )
{
	char msg[4096];
	va_list marker;
	va_start( marker, pMsg );
	Q_vsnprintf( msg, sizeof( msg ), pMsg, marker );
	va_end( marker );

	vgui::MessageBox *dlg = new CModalPreserveMessageBox( pTitle, msg, pParent );
	dlg->DoModal();
	dlg->Activate();
	dlg->RequestFocus();
}


//-----------------------------------------------------------------------------
// Purpose: Places all the info from the vgui controls into the QCInfo struct
//-----------------------------------------------------------------------------
void QCInfo::SyncFromControls()
{
	char tempText[MAX_PATH];
	
	vgui::Panel *pTargetField = pQCGenerator->FindChildByName( "staticPropCheck" );
	bStaticProp = ((CheckButton *)pTargetField)->IsSelected();
	 
	pTargetField = pQCGenerator->FindChildByName( "mostlyOpaqueCheck" );
	bMostlyOpaque = ((CheckButton *)pTargetField)->IsSelected();

	pTargetField = pQCGenerator->FindChildByName( "disableCollisionsCheck" );
	bDisableCollision = ((CheckButton *)pTargetField)->IsSelected();

	pTargetField = pQCGenerator->FindChildByName( "referencePhysicsCheck" );
	bReferenceAsPhys = ((CheckButton *)pTargetField)->IsSelected();

	pTargetField = pQCGenerator->FindChildByName( "concaveCheck" );
	bConcave = ((CheckButton *)pTargetField)->IsSelected();

	pTargetField = pQCGenerator->FindChildByName( "automassCheck" );
	bAutomass = ((CheckButton *)pTargetField)->IsSelected();

	pTargetField = pQCGenerator->FindChildByName( "massField" );
	((TextEntry *)pTargetField)->GetText(tempText, MAX_PATH);
	fMass = atof(tempText);

	pTargetField = pQCGenerator->FindChildByName( "scaleField" );
	((TextEntry *)pTargetField)->GetText(tempText, MAX_PATH);
	fScale = atof(tempText);

    pTargetField = pQCGenerator->FindChildByName( "collisionSMDField" );
	((TextEntry *)pTargetField)->GetText( tempText, MAX_PATH );	
	V_strcpy_safe( pszCollisionPath, tempText );

	pTargetField = pQCGenerator->FindChildByName( "surfacePropertyDropDown" );
	((ComboBox *)pTargetField)->GetText( tempText, MAX_PATH );
    V_strcpy_safe( pszSurfaceProperty, tempText );

	pTargetField = pQCGenerator->FindChildByName( "materialsField" );
	((TextEntry *)pTargetField)->GetText( tempText, MAX_PATH );
	V_strcpy_safe( pszMaterialPath, tempText );

	LODs.RemoveAll();
	pTargetField = pQCGenerator->FindChildByName( "LODList" );
	int numLOD = ((ListPanel *)pTargetField)->GetItemCount();
	for ( int i = 0; i < numLOD; i++ )
	{
		KeyValues *key = ((ListPanel *)pTargetField)->GetItem( i );
		LODInfo newLOD;

		V_strcpy_safe( newLOD.pszFilename, key->GetString( "SMD" ) );
		newLOD.iLOD = key->GetInt( "LOD" );		
        LODs.AddToTail( newLOD );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called during intialization to setup the initial state of the VGUI controls
//-----------------------------------------------------------------------------
void QCInfo::SyncToControls()
{
	char tempText[MAX_PATH];
	
	vgui::Panel *pTargetField = pQCGenerator->FindChildByName( "staticPropCheck" );
	((CheckButton *)pTargetField)->SetSelected( bStaticProp );
	 
	pTargetField = pQCGenerator->FindChildByName( "mostlyOpaqueCheck" );
	((CheckButton *)pTargetField)->SetSelected( bMostlyOpaque );

	pTargetField = pQCGenerator->FindChildByName( "disableCollisionsCheck" );
	((CheckButton *)pTargetField)->SetSelected( bDisableCollision );

	pTargetField = pQCGenerator->FindChildByName( "referencePhysicsCheck" );
	((CheckButton *)pTargetField)->SetSelected( bReferenceAsPhys );

	pTargetField = pQCGenerator->FindChildByName( "concaveCheck" );
	((CheckButton *)pTargetField)->SetSelected( bConcave );

	pTargetField = pQCGenerator->FindChildByName( "automassCheck" );
	((CheckButton *)pTargetField)->SetSelected( bAutomass );

	Q_snprintf( tempText, 10, "%d", (int)fMass );
	pTargetField = pQCGenerator->FindChildByName( "massField" );
	((TextEntry *)pTargetField)->SetText( tempText );

	Q_snprintf( tempText, 10, "%d", (int)fScale );
	pTargetField = pQCGenerator->FindChildByName( "scaleField" );
	((TextEntry *)pTargetField)->SetText( tempText );

	pTargetField = pQCGenerator->FindChildByName( "collisionSMDField" );
	((TextEntry *)pTargetField)->SetText( pszCollisionPath );

	pTargetField = pQCGenerator->FindChildByName( "materialsField" );
	((TextEntry *)pTargetField)->SetText( pszMaterialPath );

	pTargetField = pQCGenerator->FindChildByName( "surfacePropertyDropDown" );
	int numItems = ((ComboBox *)pTargetField)->GetItemCount();
	for( int i = 0; i < numItems; i++ )
	{
		((ComboBox *)pTargetField)->GetItemText( i, tempText, MAX_PATH );
		if ( !Q_strcmp( tempText, pszSurfaceProperty ) )
		{
			((ComboBox *)pTargetField)->SetItemEnabled( i, true );
			((ComboBox *)pTargetField)->SetText( tempText );
			break;
		}
	}   
}

CBrowseButton::CBrowseButton( vgui::Panel *pParent ) : BaseClass( pParent, "Browse Button", "...", pParent, "browse" )
{	
	SetParent( pParent );
	pszStartingDirectory = NULL;
	pszFileFilter = NULL;
	pszTargetField = NULL;
}

CBrowseButton::~CBrowseButton()
{
}

void CBrowseButton::SetCharVar( char **pVar, const char *pszNewText )
{
	if ( *pVar && pszNewText && !Q_strcmp( *pVar, pszNewText ) )
	{
		return;
	}

    if ( *pVar )
	{
		delete [] *pVar;
		*pVar = NULL;
	}

	if ( pszNewText )
	{
		int len = Q_strlen( pszNewText ) + 1;
		*pVar = new char[ len ];
		Q_strncpy( *pVar, pszNewText, len );
	}
}

void CBrowseButton::InitBrowseInfo( int x, int y, const char *pszName, const char *pszDir, const char *pszFilter, const char *pszField )
{
	SetSize( 24, 24 );
	SetPos( x, y );
	SetName( pszName );
	SetCharVar( GetStartingDirectory(), pszDir );
	SetCharVar( GetFileFilter(), pszFilter );
	SetCharVar( GetTargetField(), pszField );
	SetActionMessage();	
}

void CBrowseButton::SetActionMessage()
{
	KeyValues *newActionMessage = new KeyValues( "browse", "directory", pszStartingDirectory, "filter", pszFileFilter);
	newActionMessage->SetString( "targetField", pszTargetField );
	SetCommand( newActionMessage );
}

const char *ParseKeyvalue( const char *pBuffer, char *key, char *value )
{
	char com_token[1024];

	pBuffer = (const char *)ParseFile( pBuffer, com_token, NULL );
	if ( Q_strlen( com_token ) < MAX_KEYVALUE )
	{
		Q_strncpy( key, com_token, MAX_KEYVALUE );
		Q_strlower( key );
	}
	// no value on a close brace
	if ( !Q_strcmp( key, "}" ) )
	{
		value[0] = 0;
		return pBuffer;
	}

	pBuffer = (const char *)ParseFile( pBuffer, com_token, NULL );
	if ( Q_strlen( com_token ) < MAX_KEYVALUE )
	{
		Q_strncpy( value, com_token, MAX_KEYVALUE );
		Q_strlower( value );
	}

	return pBuffer;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CQCGenerator::CQCGenerator( vgui::Panel *pParent, const char *pszPath, const char *pszScene ) : BaseClass( pParent, "QCGenerator" )
{	
	m_szTargetField[0] = 0;
	m_nSelectedSequence = 0;
	m_nSelectedColumn = 0;

	m_QCInfo_t.Init( this );

	SetMinimumSize(846, 770);

	m_pLODPanel = new ListPanel(this, "LODList");
	m_pLODPanel->SetSelectIndividualCells( true );
	m_pLODPanel->AddColumnHeader(0, "SMD", "LOD SMD", 450, 0);	
	m_pLODPanel->AddColumnHeader(1, "LOD", "LOD Distance", 50, 0);		
	m_pLODPanel->AddActionSignalTarget( this );
	m_pLODPanel->SetMouseInputEnabled( true );

	LoadControlSettings( "QCGenerator.res" );	

	m_pCollisionBrowseButton = new CBrowseButton( this );
	m_pCollisionBrowseButton->InitBrowseInfo( 808, 158, "collisionBrowseButton", pszPath, "*.smd", "collisionSMDField" );

	char szTerminatedPath[1024] = "\0";
	sprintf( szTerminatedPath, "%s\\", pszPath );
	
	InitializeSMDPaths( szTerminatedPath, pszScene );

	char *pszMaterialsStart = strrchrcount( szTerminatedPath, '\\', 3 ) + 1;
	char *pszMaterialsEnd = strrchr( szTerminatedPath, '\\');
	Q_strncpy( m_QCInfo_t.pszMaterialPath, pszMaterialsStart, pszMaterialsEnd - pszMaterialsStart + 1 );

	SetParent( pParent );

	char szGamePath[1024] = "\0";
	char szSearchPath[1024] = "\0";

	// Get the currently set game configuration
	GetVConfigRegistrySetting( GAMEDIR_TOKEN, szGamePath, sizeof( szGamePath ) );	
	static const char *pSurfacePropFilename = "\\scripts\\surfaceproperties.txt";

	sprintf( szSearchPath, "%s%s", szGamePath, pSurfacePropFilename );

	FileHandle_t fp = g_pFullFileSystem->Open( szSearchPath, "rb" );	 

	if ( !fp )
	{
		//the set game configuration didn't have a surfaceproperties file; we are grabbing it from hl2
		//TODO:  This only works if they are in a subdirectory that is a peer to an hl2 directory 
		//		that contains the file.  It potentially needs to search the entire drive or prompt for the location
		char *pszEndGamePath = Q_strrchr( szGamePath, '\\' );
		pszEndGamePath[0] = 0;
		V_strcat_safe( szGamePath, "\\hl2" );
		sprintf( szSearchPath, "%s%s", szGamePath, pSurfacePropFilename );
		fp = g_pFullFileSystem->Open( szSearchPath, "rb" );	
	}

	int len = g_pFullFileSystem->Size( fp );

	const char *szSurfacePropContents = new char[len+1];
	g_pFullFileSystem->Read( (void *)szSurfacePropContents, len, fp );

	char key[MAX_KEYVALUE], value[MAX_KEYVALUE];

	vgui::Panel *pSurfacePropDropDown = FindChildByName( "surfacePropertyDropDown" );		

	//filling up the surface property dropdown
	while ( szSurfacePropContents )
	{
		szSurfacePropContents = ParseKeyvalue( szSurfacePropContents, key, value );
		((ComboBox *)pSurfacePropDropDown)->AddItem( key, NULL );							
		while ( szSurfacePropContents )
		{
			szSurfacePropContents = ParseKeyvalue( szSurfacePropContents, key, value );
			if (!stricmp( key, "}" ) )
			{
				break;
			}
		}
	}	
    m_QCInfo_t.SyncToControls();

	m_pLODEdit = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CQCGenerator::~CQCGenerator()
{
}

void CQCGenerator::OnCommand( const char *command )
{	
	if ( Q_stricmp( command, "createQC" ) == 0 )
	{
		m_QCInfo_t.SyncFromControls();
		GenerateQCFile();
	}
	if ( Q_stricmp( command, "deleteSeq" ) == 0 )
	{
		//delete it
		DeleteLOD();
	}
	if ( Q_stricmp( command, "editSeq" ) == 0 )
	{
		//edit
		EditLOD();
	}
	
	BaseClass::OnCommand( command );
}


void CQCGenerator::OnKeyCodeTyped( KeyCode code )
{
	switch ( code )
	{
	case KEY_ENTER:
			EditLOD();
	}
}
	

void CQCGenerator::OnBrowse( KeyValues *data )
{
    V_strcpy_safe( m_szTargetField, data->GetString( "targetField" ) );
	const char *filter = data->GetString( "filter" );
	
	if ( Q_strlen( filter ) == 0 )
	{
//		BrowseDirectory( data );
	}
	else
	{
		BrowseFile( data );	
	}
}

/*
//This function is no longer used in the current version of the program.
void CQCGenerator::BrowseDirectory( KeyValues *data )
{
	DirectorySelectDialog *pDialog = new DirectorySelectDialog( this, "Select Directory" );
	pDialog->AddActionSignalTarget( this );
	pDialog->DoModal();
	pDialog->SetStartDirectory( data->GetString( "directory" ) );	
}
*/

void CQCGenerator::BrowseFile( KeyValues *data )
{	    
	const char *filter = data->GetString( "filter" );

	FileOpenDialog *pDialog = new FileOpenDialog( this, "Select File", true );
	pDialog->AddFilter( filter, filter, true );
	pDialog->AddActionSignalTarget(this);
	pDialog->SetStartDirectory( data->GetString( "directory" ) );	
	pDialog->DoModal( true );
}

void CQCGenerator::OnFileSelected( KeyValues *data ) 
{
	if ( *m_szTargetField )
	{
		vgui::Panel *pTargetField = FindChildByName( m_szTargetField );
		((TextEntry *)pTargetField)->SetText( data->GetString( "fullpath" ) );
		Repaint();
	}
}

void CQCGenerator::OnDirectorySelected( KeyValues *data ) 
{
	if ( *m_szTargetField )
	{
		vgui::Panel *pTargetField = FindChildByName( m_szTargetField );
		((TextEntry *)pTargetField)->SetText( data->GetString( "dir" ) );
		Repaint();
	}
}

bool CQCGenerator::GenerateQCFile()
{
	//TODO: clean this up.  Consider creating a datatype that includes the string to write out when the QC file is created
	char *nameBegin = strrchr( m_QCInfo_t.pszSMDPath, '\\' );

	char szPath[MAX_PATH];
	char szName[MAX_PATH];
	Q_strncpy( szPath, m_QCInfo_t.pszSMDPath, nameBegin - m_QCInfo_t.pszSMDPath + 2 );
	V_strcpy_safe( szName, szPath);
	V_strcat_safe( szName, m_QCInfo_t.pszSceneName);
	V_strcat_safe( szName, ".qc" );
	FileHandle_t pSaveFile = g_pFullFileSystem->Open( szName, "wt" );
	if (!pSaveFile)
	{
		char szSaveError[1024] = "";
		Q_snprintf( szSaveError, 1024, "Save failed: invalid file name '%s'\n\nDirectory '%s' must exist.", szName, szPath );	
		VGUIMessageBox( this, "QC Generator error", szSaveError );
		return 0;
	}	
		
	//write qc header
	g_pFullFileSystem->FPrintf( pSaveFile, "//\n// .qc file version 1.0\n\n");
	//write out modelname info
	char szModelName[MAX_PATH];
	char *modelStart = strrchrcount( szName, '\\', 2) + 1;
	char *modelEnd = strrchr( szName, '.' );
	Q_strncpy( szModelName, modelStart, modelEnd - modelStart + 1 );
	V_strcat_safe( szModelName, ".mdl" );
	g_pFullFileSystem->FPrintf( pSaveFile, "$modelname %s\n\n", szModelName );
	//write out scale info
	g_pFullFileSystem->FPrintf( pSaveFile, "$scale %f\n", m_QCInfo_t.fScale );
	//write out body info
	g_pFullFileSystem->FPrintf( pSaveFile, "$body \"Body\" \"%s\"\n", strrchr( m_QCInfo_t.pszSMDPath, '\\' ) + 1 );

	if ( m_QCInfo_t.bStaticProp == true )
	{
		g_pFullFileSystem->FPrintf( pSaveFile, "$staticprop\n" );
	}

	if ( m_QCInfo_t.bMostlyOpaque == true )
	{
		g_pFullFileSystem->FPrintf( pSaveFile, "$mostlyopaque\n" );
	}

	//write out surfaceprop info
	g_pFullFileSystem->FPrintf( pSaveFile, "$surfaceprop \"%s\"\n\n", m_QCInfo_t.pszSurfaceProperty );
	//write materials

	g_pFullFileSystem->FPrintf( pSaveFile, "$cdmaterials %s\n\n", m_QCInfo_t.pszMaterialPath);

	if ( m_QCInfo_t.bStaticProp || m_QCInfo_t.bNoAnimation )
	{
		g_pFullFileSystem->FPrintf( pSaveFile, "// --------- Animation sequences -------\n");
		g_pFullFileSystem->FPrintf( pSaveFile, "$sequence  \"idle\" \"%s\" fps 30\n\n", strrchr(m_QCInfo_t.pszSMDPath, '\\')+1);
	}

	//write out lod info
	for( int i = 0; i < m_QCInfo_t.LODs.Count(); i++ )
	{
		LODInfo thisLOD = m_QCInfo_t.LODs.Element( i );
		g_pFullFileSystem->FPrintf( pSaveFile, "$lod %d\n{\n\treplacemodel \"%s\" \"%s\"\n}\n\n", thisLOD.iLOD, strrchr(m_QCInfo_t.pszSMDPath, '\\')+1, thisLOD.pszFilename );		
	}

	if ( m_QCInfo_t.bDisableCollision != true )
	{
		//write out collision header
		g_pFullFileSystem->FPrintf( pSaveFile, "\n" );
		//write out collision info
		if ( m_QCInfo_t.bReferenceAsPhys == true )
		{
			g_pFullFileSystem->FPrintf( pSaveFile, "$collisionmodel \"%s\"", strrchr( m_QCInfo_t.pszSMDPath, '\\' ) + 1 );
		}
		else 
		{
			if( Q_strcmp( m_QCInfo_t.pszCollisionPath, "" ) )
			{
				g_pFullFileSystem->FPrintf( pSaveFile, "$collisionmodel \"%s\"", strrchr( m_QCInfo_t.pszCollisionPath, '\\' ) + 1 );
			}
		}

		g_pFullFileSystem->FPrintf( pSaveFile, " {\n\t// Mass in kilograms\n ");

		if ( m_QCInfo_t.bAutomass == true )
		{
			g_pFullFileSystem->FPrintf( pSaveFile, "\t$automass\n" );
		}
		else
		{
			g_pFullFileSystem->FPrintf( pSaveFile, "\t$mass %f\n", m_QCInfo_t.fMass );
		}
		
		if ( m_QCInfo_t.bConcave == true )
		{
			g_pFullFileSystem->FPrintf( pSaveFile, "\t$concave\n" );
		}
		g_pFullFileSystem->FPrintf( pSaveFile, "}\n\n");
	}
	    	
	g_pFullFileSystem->Close( pSaveFile );

	char szCommand[MAX_PATH];
	char szGamePath[MAX_PATH];
	
	char studiomdlPath[512];
	g_pFullFileSystem->RelativePathToFullPath( "studiomdl.bat", NULL, studiomdlPath, sizeof( studiomdlPath ));

	GetVConfigRegistrySetting( GAMEDIR_TOKEN, szGamePath, sizeof( szGamePath ) );	

#ifdef WIN32
	STARTUPINFO startup; 
	PROCESS_INFORMATION process; 


	memset(&startup, 0, sizeof(startup)); 
	startup.cb = sizeof(startup); 

	
	sprintf( szCommand, "%s -game %s %s", studiomdlPath, szGamePath, szName);
	bool bReturn = CreateProcess( NULL, szCommand, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &startup, &process);
#else
	Assert( !"Implement me, why aren't we using a thread tool abstraction?" );
	bool bReturn = false;
#endif
	return bReturn;
}

void CQCGenerator::InitializeSMDPaths( const char *pszPath, const char *pszScene )
{
	V_strcpy_safe( m_QCInfo_t.pszSceneName, pszScene );

	FileFindHandle_t *pFileHandle = new FileFindHandle_t();

	g_pFullFileSystem->AddSearchPath( pszPath, "SMD_DIR" );
	
	const char *filename = g_pFullFileSystem->FindFirst( "*.smd", pFileHandle );	

	bool bFoundReference = false;
	bool bFoundCollision = false;
	bool bFoundLOD = false;

	//iterate through .smd files
  	const char *startName = pszScene;

	int nSearchLength = Q_strlen( pszScene );

	int currentLOD = 1;
	
	while( filename )
	{        
		if ( !strncmp( startName, filename, nSearchLength ) )
		{
			const char *filenameEnd = filename + nSearchLength;
			if ( !strncmp( filenameEnd, "_ref", 4 ) || !strncmp( filenameEnd, ".smd", 4 ) )
			{
				bFoundReference = true;
				//we have found the reference smd.
				V_strcpy_safe( m_QCInfo_t.pszSMDPath, pszPath );
				V_strcat_safe( m_QCInfo_t.pszSMDPath, filename );
			}
			if ( !strncmp( filenameEnd, "_phy", 4) || !strncmp( filenameEnd, "_col", 4 ) )
			{
				bFoundCollision = true;
				//we have found the collision smd.
				V_strcpy_safe( m_QCInfo_t.pszCollisionPath, pszPath );
				V_strcat_safe( m_QCInfo_t.pszCollisionPath, filename );
			}
			if ( !strncmp( filenameEnd, "_lod", 4) )
			{
				bFoundLOD = true;
				//we found an LOD smd.
				char lodName[255];
				Q_snprintf( lodName, Q_strlen( lodName ), "lod%d", currentLOD );
				//we found an LOD
				KeyValues *newKv = new KeyValues( lodName, "SMD", filename, "LOD", "10" );
				m_pLODPanel->AddItem( newKv, currentLOD, false, false );
				currentLOD++;
			}
		}
		filename = g_pFullFileSystem->FindNext( *pFileHandle );
	}
	char pszMessage[2048] = "";
	char pszRefMessage[1024] = "";
	char pszColMessage[1024] = "";
	if (!bFoundReference )
	{
		V_strcat_safe( m_QCInfo_t.pszSMDPath, pszPath );
		V_strcat_safe( m_QCInfo_t.pszSMDPath, pszScene );		
		V_strcat_safe( m_QCInfo_t.pszSMDPath, ".smd" );
		Q_snprintf( pszRefMessage, 1024, "Reference SMD not found.\n\nValid default reference SMDs are %s%s_ref*.smd and %s%s.smd\nUsing default of %s. Model will not compile.\n\n", pszPath, pszScene, pszPath, pszScene, m_QCInfo_t.pszSMDPath );		
	}
	if ( !bFoundCollision )
	{
		Q_snprintf( pszColMessage, 1024, "Collision SMD not found.\n\nThe valid default collision SMD is %s%s_phy*.smd.\nUsing reference SMD as default.\n", pszPath, pszScene );
		V_strcpy_safe( m_QCInfo_t.pszCollisionPath, m_QCInfo_t.pszSMDPath );
		m_QCInfo_t.bReferenceAsPhys = true;
	}
	if ( !bFoundReference || !bFoundCollision)
	{
		V_strcpy_safe( pszMessage, pszRefMessage );
		V_strcat_safe( pszMessage, pszColMessage );
		VGUIMessageBox( this, "Error Initializing Paths", pszMessage );
	}
}


void CQCGenerator::DeleteLOD()
{
	int numSelected = m_pLODPanel->GetSelectedItemsCount();
	int selected;
	for ( int i = numSelected-1; i >= 0; i-- )
	{
		selected = m_pLODPanel->GetSelectedItem( i );
		m_pLODPanel->RemoveItem( selected );
	}
}

void CQCGenerator::EditLOD()
{
	int numSelected = m_pLODPanel->GetSelectedItemsCount();
	if ( numSelected == 1 && !m_pLODPanel->IsInEditMode() )
	{
		if ( m_pLODEdit )
		{
			m_pLODEdit->MarkForDeletion();
			m_pLODEdit = NULL;
		}
		m_pLODEdit = new vgui::TextEntry( this, "Edit" );
		m_pLODEdit->SendNewLine( true );
		m_nSelectedSequence = m_pLODPanel->GetSelectedItem( 0 );
		m_nSelectedColumn = m_pLODPanel->GetSelectedColumn();
		m_pLODPanel->EnterEditMode( m_nSelectedSequence, m_nSelectedColumn, m_pLODEdit );
	}
}

void CQCGenerator::OnNewLODText()
{		
	KeyValues *pEditItem = m_pLODPanel->GetItem( m_nSelectedSequence );
	KeyValues *pListItem = pEditItem;
	wchar_t szEditText[MAX_PATH];

	pEditItem = pEditItem->GetFirstValue();
	const char *name = pEditItem->GetName();
	for( int i = 0; i < m_nSelectedColumn; i++ )
	{
		pEditItem = pEditItem->GetNextValue();
		name = pEditItem->GetName();
	}
	m_pLODEdit->GetText( szEditText, MAX_PATH );

	pListItem->SetWString( name, szEditText );

	m_pLODPanel->LeaveEditMode();
	m_pLODPanel->InvalidateLayout();
	return;
}