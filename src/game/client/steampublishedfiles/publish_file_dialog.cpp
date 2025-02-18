//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Dialog for selecting game configurations
//
//=====================================================================================//
#include "cbase.h"

#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/MessageBox.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/FileOpenDialog.h>
#include "vgui_bitmappanel.h"
#include <KeyValues.h>
#include "imageutils.h"
#include "bsp_utils.h"

#include "icommandline.h"
#include "publish_file_dialog.h"
#include "workshop/ugc_utils.h"

#ifdef TF_CLIENT_DLL
#include "../server/tf/workshop/maps_workshop.h"
#endif // TF_CLIENT_DLL

#include "steam/steam_api.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

static CUtlString g_MapFilename;
static CUtlString g_PreviewFilename;

ConVar publish_file_last_dir( "publish_file_last_dir", "", FCVAR_ARCHIVE | FCVAR_CLIENTDLL | FCVAR_HIDDEN | FCVAR_DONTRECORD );

CFilePublishDialog *g_pSteamFilePublishDialog = NULL;

#define WORKSHOP_TEMP_UPLOAD_DIR "workshop/upload"

class CPrepareFileThread : public CThread
{
public:
	CPrepareFileThread( const char *pszInputFile, const char *pszOutputFile )
		: m_strInput( pszInputFile )
		, m_strOutput( pszOutputFile )
		{}

	// Return 0 for success
	virtual int Run()
	{
		if ( V_strcasecmp( V_GetFileExtension( m_strInput.Get() ), "bsp" ) == 0 )
		{
			return BSP_SyncRepack( m_strInput.Get(), m_strOutput.Get() ) ? 0 : 1;
		}
		else
		{
			// Just copy file to prepared location
			return engine->CopyLocalFile( m_strInput.Get(), m_strOutput.Get() ) ? 0 : 1;
		}
	}

private:
	CUtlString m_strInput;
	CUtlString m_strOutput;
};


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CFilePublishDialog::CFilePublishDialog( Panel *parent, const char *name, PublishedFileDetails_t *pDetails ) : BaseClass( parent, name )
{
	m_pPrepareFileThread = NULL;

	g_pSteamFilePublishDialog = this;

	// These must be supplied
	m_bValidFile = false;
	m_bValidJpeg = false;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 0 );

	// Save this for later
	if ( pDetails != NULL )
	{
		m_FileDetails = *pDetails;
		m_bAddingNewFile = false;
		g_MapFilename = m_FileDetails.lpszFilename;
		m_nFileID = m_FileDetails.publishedFileDetails.m_nPublishedFileId;
	}
	else
	{
		// Clear it out
		m_FileDetails.lpszFilename = NULL;
		m_FileDetails.publishedFileDetails.m_eVisibility = k_ERemoteStoragePublishedFileVisibilityPublic;
		m_FileDetails.publishedFileDetails.m_hFile = k_UGCHandleInvalid;
		m_FileDetails.publishedFileDetails.m_hPreviewFile = k_UGCHandleInvalid;
		m_FileDetails.publishedFileDetails.m_nConsumerAppID = k_uAppIdInvalid;
		m_FileDetails.publishedFileDetails.m_nCreatorAppID = k_uAppIdInvalid;
		m_FileDetails.publishedFileDetails.m_nPublishedFileId = 0; // FIXME: Need a real "invalid" value
		m_FileDetails.publishedFileDetails.m_rtimeCreated = 0;
		m_FileDetails.publishedFileDetails.m_rtimeUpdated = 0;
		m_FileDetails.publishedFileDetails.m_ulSteamIDOwner = 0; // FIXME: Need a real "invalid" value
		memset( m_FileDetails.publishedFileDetails.m_rgchDescription, 0, k_cchPublishedDocumentDescriptionMax );
		memset( m_FileDetails.publishedFileDetails.m_rgchTitle, 0, k_cchPublishedDocumentTitleMax );

		m_bAddingNewFile = true;
		g_MapFilename = "";
		m_nFileID = k_PublishedFileIdInvalid;
	}

	m_nFileDetailsChanges = 0;

	m_fileOpenMode = FILEOPEN_NONE;

	// Setup our image panel
	m_pCroppedTextureImagePanel = new CBitmapPanel( this, "PreviewImage" );
	m_pCroppedTextureImagePanel->SetSize( DesiredPreviewWidth(), DesiredPreviewHeight() );
	m_pCroppedTextureImagePanel->SetVisible( true );

	m_pStatusBox = NULL;

	// Start downloading our preview image
	m_bPreviewDownloadPending = false;
	DownloadPreviewImage();
}

//-----------------------------------------------------------------------------
// Destructor 
//-----------------------------------------------------------------------------
CFilePublishDialog::~CFilePublishDialog()
{
	//delete m_pConfigCombo;
	g_pSteamFilePublishDialog = NULL;

	// We should be in a modal dialog when this is running, not closable
	Assert( !m_pPrepareFileThread );
	if ( m_pPrepareFileThread )
	{
		m_pPrepareFileThread->Stop();
		delete m_pPrepareFileThread;
		m_pPrepareFileThread = NULL;
	}
}

void CFilePublishDialog::ErrorMessage( ErrorCode_t errorCode, KeyValues *pkvTokens  )
{
	switch ( errorCode )
	{
	case kFailedToPublishFile:
		ErrorMessage( "Failed to publish file!" );
		break;
	case kFailedToUpdateFile:
		ErrorMessage( "Failed to update file!" );
		break;
	case kFailedToPrepareFile:
		ErrorMessage( "Failed to prepare file!" );
		break;
	case kSteamCloudNotAvailable:
		ErrorMessage( "Steam Cloud is not available." );
		break;
	case kSteamExceededCloudQuota:
		ErrorMessage( "Exceed Steam Cloud quota." );
		break;
	case kFailedToWriteToSteamCloud:
		ErrorMessage( "Failed to write to Steam cloud!" );
		break;
	case kFileNotFound:
		ErrorMessage( "File not found!" );
		break;
	case kNeedTitleAndDescription:
		ErrorMessage( "Need to have a title and description!" );
		break;
	case kFailedFileValidation:
		ErrorMessage( "File failed to validate!" );
		break;
	case kFailedUserModifiedFile:
		ErrorMessage( "File was manually modified after verifying process" );
		break;
	case kInvalidMapName:
		ErrorMessage( "Invalid name for map. Map names must be lowercase and of the form cp_foo.bsp." );
		break;
	default:
		Assert( false ); // Unhandled enum value
		break;
	}
	if ( pkvTokens )
	{
		pkvTokens->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
void CFilePublishDialog::ErrorMessage( const char *lpszText )
{
	vgui::MessageBox *pBox = new vgui::MessageBox( "", lpszText, this );
	pBox->SetPaintBorderEnabled( false );
	pBox->SetPaintBackgroundEnabled( true );
	pBox->SetBgColor( Color(0,0,0,255) ); 
	pBox->DoModal();
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
const char* CFilePublishDialog::GetStatusString( StatusCode_t statusCode )
{
	switch ( statusCode )
	{
	case kPublishing:
		return "Publishing, please wait...";
		break;
	case kUpdating:
		return "Publishing, please wait...";
		break;
	}
	return "";
}

//-----------------------------------------------------------------------------
// Purpose: Show our modal status window to cover asynchronous tasks
// TODO: Pull this out into a more generalized solution across dialogs
//-----------------------------------------------------------------------------
void CFilePublishDialog::ShowStatusWindow( StatusCode_t statusCode )
{
	// Throw up our status box
	if ( m_pStatusBox )
	{
		m_pStatusBox->CloseModal();
		m_pStatusBox = NULL; // FIXME: Does this clear up the memory?
	}

	const char *lpszText = GetStatusString( statusCode );

	// Pop a message to the user so they know to wait
	m_pStatusBox = new vgui::MessageBox( "", lpszText, this );
	m_pStatusBox->SetPaintBorderEnabled( false );
	m_pStatusBox->SetPaintBackgroundEnabled( true );
	m_pStatusBox->SetBgColor( Color(0,0,0,255) ); 
	m_pStatusBox->SetOKButtonVisible( false );
	m_pStatusBox->DoModal();
}

//-----------------------------------------------------------------------------
// Purpose: Hide our modal status window
// TODO: Pull this out into a more generalized solution across dialogs
//-----------------------------------------------------------------------------
void CFilePublishDialog::HideStatusWindow( void )
{
	m_pStatusBox->CloseModal();
	m_pStatusBox = NULL;
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
void CFilePublishDialog::DownloadPreviewImage( void )
{
	// TODO: We need a generic "no image" image
	if ( m_bAddingNewFile )
		return;

	// Start off our download
	char szTargetFilename[MAX_PATH];
	V_snprintf( szTargetFilename, sizeof(szTargetFilename), "%llu_thumb.jpg", m_FileDetails.publishedFileDetails.m_nPublishedFileId );
	m_UGCPreviewFileRequest.StartDownload( m_FileDetails.publishedFileDetails.m_hPreviewFile, "downloads", szTargetFilename );
	m_bPreviewDownloadPending = true;
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
void CFilePublishDialog::OnTick( void )
{
	BaseClass::OnTick();

	if ( m_pPrepareFileThread )
	{
		if ( !m_pPrepareFileThread->IsAlive() )
		{
			// Finished, trigger handler
			int result = m_pPrepareFileThread->GetResult();
			delete m_pPrepareFileThread;
			m_pPrepareFileThread = NULL;
			OnFilePrepared( result == 0 ? kNoError : kFailedToPrepareFile );
		}
	}

	if ( m_bPreviewDownloadPending )
	{
		UGCFileRequestStatus_t ugcStatus = m_UGCPreviewFileRequest.Update();
		switch ( ugcStatus )
		{
		case UGCFILEREQUEST_ERROR:
			Warning("An error occurred while attempting to download a file from the UGC server!\n");
			m_bPreviewDownloadPending = false;
			break;

		case UGCFILEREQUEST_FINISHED:
			// Update our image preview
			char szLocalFilename[MAX_PATH];
			m_UGCPreviewFileRequest.GetLocalFileName( szLocalFilename, sizeof(szLocalFilename) );
			char szLocalPath[ _MAX_PATH ];
			g_pFullFileSystem->GetLocalPath( szLocalFilename, szLocalPath, sizeof(szLocalPath) );
			SetPreviewImage( szLocalPath );
			m_bPreviewDownloadPending = false;
			break;

		default:
			// Working, continue to wait...
			return;
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
void CFilePublishDialog::SetPreviewImage( const char *lpszFilename )
{
	if ( lpszFilename == NULL )
		return;

	// Retain this
	g_PreviewFilename = lpszFilename;

	m_bValidJpeg = false;

	ConversionErrorType nErrorCode = ImgUtl_LoadBitmap( lpszFilename, m_imgSource );
	if ( nErrorCode != CE_SUCCESS )
	{
	}
	else
	{
		m_bValidJpeg = true;
		PerformSquarize();
		m_pCroppedTextureImagePanel->SetBitmap( GetPreviewBitmap() );
	}

	// Update the state of our publish button
	SetPublishButtonState();
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
void CFilePublishDialog::PerformSquarize()
{
	if ( !BForceSquarePreviewImage() )
		return;

	const Bitmap_t *pResizeSrc = &m_imgSource;
	if ( !IsSourceImageSquare() )
	{
		// Select the smaller dimension as the size
		int nSize = MIN( m_imgSource.Width(), m_imgSource.Height() );

		// Crop it.
		// Yeah, the crop and resize could be done all in one step.
		// And...I don't care.
		int x0 = ( m_imgSource.Width() - nSize ) / 2;
		int y0 = ( m_imgSource.Height() - nSize ) / 2;
		m_imgTemp.Crop( x0, y0, nSize, nSize, &m_imgSource );

		pResizeSrc = &m_imgTemp;
	}

	// resize
	ImgUtl_ResizeBitmap( m_imgSquare, DesiredPreviewWidth(), DesiredPreviewHeight(), pResizeSrc );
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
Bitmap_t &CFilePublishDialog::GetPreviewBitmap()
{
	return BForceSquarePreviewImage() ? m_imgSquare : m_imgSource;
}

//-----------------------------------------------------------------------------
// Purpose:	Setup our edit fields with the appropriate information
//-----------------------------------------------------------------------------
void CFilePublishDialog::PopulateEditFields( void )
{
	m_pFileTitle->SetText( m_FileDetails.publishedFileDetails.m_rgchTitle );
	m_pFileDescription->SetText( m_FileDetails.publishedFileDetails.m_rgchDescription );

	if ( m_FileDetails.lpszFilename && !FStrEq( m_FileDetails.lpszFilename, "" ) )
	{
		char szShortName[ MAX_PATH ];
		Q_FileBase( m_FileDetails.lpszFilename, szShortName, sizeof(szShortName) );
		const char *szExt = Q_GetFileExtension( m_FileDetails.lpszFilename );
		Q_SetExtension( szShortName, CFmtStr( ".%s", szExt ).Access(), sizeof(szShortName ) );
		m_pFilename->SetText( szShortName );
	}
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
void CFilePublishDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( GetResFile() );

	m_pFileTitle = dynamic_cast< vgui::TextEntry * >( FindChildByName( "FileTitle" ) );
	if ( m_pFileTitle )
	{
		m_pFileTitle->AddActionSignalTarget( this );
	}

	m_pFileDescription = dynamic_cast< vgui::TextEntry * >( FindChildByName( "FileDesc" ) );
	if ( m_pFileDescription )
	{
		m_pFileDescription->SetMultiline( true );
		m_pFileDescription->SetCatchEnterKey( true );
		m_pFileDescription->SetVerticalScrollbar( true );
	}

	m_pFilename = dynamic_cast< vgui::Label * >( FindChildByName( "SourceFile" ) );
	if ( !g_MapFilename.IsEmpty() )
	{
		m_pFilename->SetText( g_MapFilename );
	}

	m_pPublishButton = dynamic_cast< vgui::Button * >( FindChildByName( "ButtonPublish" ) );

	// If we're updating, change the context of the button
	if ( !m_bAddingNewFile )
	{
		m_pPublishButton->SetText( "Update" );
		m_pPublishButton->SetCommand( "Update" );
	}

	// Setup our initial state for the edit fields
	PopulateEditFields();
	SetPublishButtonState();
}

//-----------------------------------------------------------------------------
// Purpose: Helper to build thumbnail name
//-----------------------------------------------------------------------------
void CFilePublishDialog::GetPreviewFilename( char *szOut, size_t outLen )
{
	char szMapShortName[MAX_PATH];
	Q_FileBase( g_MapFilename, szMapShortName, sizeof(szMapShortName) );
	Q_snprintf( szOut, outLen, "%s_thumb.jpg", szMapShortName );
}

//-----------------------------------------------------------------------------
// Purpose: Helper to build prepared file name
//-----------------------------------------------------------------------------
void CFilePublishDialog::GetPreparedFilename( char *szOut, size_t outLen )
{
	V_ComposeFileName( WORKSHOP_TEMP_UPLOAD_DIR, V_GetFileName( g_MapFilename ),
	                   szOut, outLen );
}

//-----------------------------------------------------------------------------
// Purpose: Callback when our create item has completed. Need to do initial update.
//-----------------------------------------------------------------------------
void CFilePublishDialog::Steam_OnCreateItem( CreateItemResult_t *pResult, bool bError )
{
	bError = bError || pResult->m_eResult != k_EResultOK;
	m_nFileID = pResult->m_nPublishedFileId;

	if ( bError )
	{
		HideStatusWindow();
		ErrorMessage( kFailedToPublishFile );
		if ( m_nFileID != k_PublishedFileIdInvalid )
		{
			// TODO ISteamUGC is conspicuously missing a delete call, but shares IDs with SteamRemoteStorage.
			//      Once this is fixed in steam, this call should probably be moved
			steamapicontext->SteamRemoteStorage()->DeletePublishedFile( m_nFileID );
			m_nFileID = k_PublishedFileIdInvalid;
		}
	}
	else
	{
		StartPrepareFile();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Callback from our map compression thread finishing
//-----------------------------------------------------------------------------
void CFilePublishDialog::OnFilePrepared( ErrorCode_t eResult )
{
	if ( eResult == kNoError )
	{
		// Move on to final publishing
		if ( !UpdateFileInternal() )
		{
			eResult = kFailedToUpdateFile;
		}
	}

	if ( eResult == kNoError )
	{
		// Done, waiting on file publish callback
		return;
	}

	// Failure

	// This is after OnCreateItem for new files, so cleanup the incomplete item on failure from either the compress or
	// kicking off the update.
	if ( m_bAddingNewFile && m_nFileID != k_PublishedFileIdInvalid )
	{
		// TODO ISteamUGC is conspicuously missing a delete call, but shares IDs with SteamRemoteStorage.
		//      Once this is fixed in steam, this call should probably be moved
		steamapicontext->SteamRemoteStorage()->DeletePublishedFile( m_nFileID );
		m_nFileID = k_PublishedFileIdInvalid;
	}

	HideStatusWindow();
	ErrorMessage( eResult );
}

//-----------------------------------------------------------------------------
// Purpose: Callback when our publish call has completed
//-----------------------------------------------------------------------------
void CFilePublishDialog::Steam_OnPublishFile( SubmitItemUpdateResult_t *pResult, bool bError )
{
	// Remove prepared map
	char szPreparedMap[MAX_PATH] = { 0 };
	GetPreparedFilename( szPreparedMap, sizeof( szPreparedMap ) );
	g_pFullFileSystem->RemoveFile( szPreparedMap, UGC_PATHID );

	// Remove local thumbnail
	CUtlBuffer bufData;
	char szPreviewFilename[MAX_PATH];
	GetPreviewFilename( szPreviewFilename, sizeof( szPreviewFilename ) );
	g_pFullFileSystem->RemoveFile( szPreviewFilename, UGC_PATHID );

	HideStatusWindow();

	if ( bError || pResult->m_eResult != k_EResultOK )
	{
		if ( m_bAddingNewFile && m_nFileID != k_PublishedFileIdInvalid )
		{
			// TODO ISteamUGC is conspicuously missing a delete call, but shares IDs with SteamRemoteStorage.
			//      Once this is fixed in steam, this call should probably be moved
			steamapicontext->SteamRemoteStorage()->DeletePublishedFile( m_nFileID );
			m_nFileID = k_PublishedFileIdInvalid;
		}
		ErrorMessage( kFailedToPublishFile );
	}
	else
	{
		EUniverse universe = GetUniverse();
		switch ( universe )
		{
		case k_EUniversePublic:
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStrMax( "http://steamcommunity.com/sharedfiles/filedetails/?id=%llu&requirelogin=true", m_nFileID ) );
			break;
		case k_EUniverseBeta:
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStrMax( "http://beta.steamcommunity.com/sharedfiles/filedetails/?id=%llu&requirelogin=true", m_nFileID ) );
			break;
		case k_EUniverseDev:
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStrMax( "http://localhost/community/sharedfiles/filedetails/?id=%llu&requirelogin=true", m_nFileID ) );
			break;
		}

		// Tell our parent what happened
		KeyValues *pkvActionSignal = new KeyValues( "ChangedFile" );
		pkvActionSignal->SetUint64( "nPublishedFileID", m_nFileID );
		PostActionSignal( pkvActionSignal );

		// Close down the window
		CloseModal();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Share the file with Steam Cloud and return the handle for later usage
//-----------------------------------------------------------------------------
bool CFilePublishDialog::PublishFile()
{
	// Must be a valid file
	ErrorCode_t errorCode = ValidateFile( g_MapFilename );
#ifdef TF_CLIENT_DLL
	const char *pExt = V_GetFileExtension( g_MapFilename );
	if ( errorCode == kNoError && pExt && V_strcmp( pExt, "bsp" ) == 0 )
	{
		if ( !CTFMapsWorkshop::IsValidOriginalFileNameForMap( CUtlString( V_GetFileName( g_MapFilename ) ) ) )
		{
			errorCode = kInvalidMapName;
		}
	}
#endif
	if ( errorCode != kNoError )
	{
		ErrorMessage( errorCode );
		return false;
	}

	ShowStatusWindow( kPublishing );

	EWorkshopFileType eFileType = WorkshipFileTypeForFile( g_MapFilename );

	// Create file on UGC
	SteamAPICall_t hSteamAPICall = steamapicontext->SteamUGC()->CreateItem( GetTargetAppID(), eFileType );

	// Set the callback
	m_callbackCreateItem.Set( hSteamAPICall, this, &CFilePublishDialog::Steam_OnCreateItem );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Kick off the map compression thread
//-----------------------------------------------------------------------------
void CFilePublishDialog::StartPrepareFile( void )
{
	// Ensure temp dir exists
	g_pFullFileSystem->CreateDirHierarchy( WORKSHOP_TEMP_UPLOAD_DIR, UGC_PATHID );

	char szOutPath[MAX_PATH] = { 0 };
	GetPreparedFilename( szOutPath, sizeof( szOutPath ) );

	// Ensure this file isn't leftover in output dir
	g_pFullFileSystem->RemoveFile( szOutPath, UGC_PATHID );

	// Start thread
	Assert( !m_pPrepareFileThread );
	m_pPrepareFileThread = new CPrepareFileThread( g_MapFilename, szOutPath );
	m_pPrepareFileThread->Start();
}

//-----------------------------------------------------------------------------
// Purpose: Parse commands coming in from the VGUI dialog
//-----------------------------------------------------------------------------
void CFilePublishDialog::SetPublishButtonState( void )
{
	if ( m_bAddingNewFile )
	{
		if ( m_bValidFile && m_bValidJpeg )
		{
			m_pPublishButton->SetEnabled( true );
		}
		else
		{
			m_pPublishButton->SetEnabled( false );
		}
	}
	else // Updating a previous entry
	{
		// m_pPublishButton->SetEnabled( (m_nFileDetailsChanges!=0) );
		m_pPublishButton->SetEnabled( true ); // For now, always allow it. Worst case it's a no-op
	}
}

//-----------------------------------------------------------------------------
// Purpose: Parse commands coming in from the VGUI dialog
//-----------------------------------------------------------------------------
bool CFilePublishDialog::UpdateFile( void )
{
	// We should have been created for an existing file or published already, both of which set our ID.
	Assert( m_nFileID != k_PublishedFileIdInvalid );
	ShowStatusWindow( kUpdating );

	if ( m_bAddingNewFile || m_nFileDetailsChanges & PFILE_FIELD_FILE )
	{
		StartPrepareFile();
	}
	else
	{
		// Not updating map, go straight to update step
		if ( !UpdateFileInternal() )
		{
			HideStatusWindow();
			ErrorMessage( kFailedToUpdateFile );
		}
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Update a file, used by the create and update pathways to fill in a UGC item
//-----------------------------------------------------------------------------
bool CFilePublishDialog::UpdateFileInternal()
{
	ISteamUGC *pUGC = steamapicontext->SteamUGC();

	UGCUpdateHandle_t hItem = pUGC->StartItemUpdate( GetTargetAppID(), m_nFileID );
	if ( hItem == k_UGCUpdateHandleInvalid )
	{
		UGCWarning( "StartItemUpdate failed\n" );
		return false;
	}

	bool bError = false;

	// create thumbnail
	CUtlBuffer bufData;
	char szPreviewFilename[MAX_PATH];
	GetPreviewFilename( szPreviewFilename, sizeof( szPreviewFilename ) );

	if ( !bError && ImgUtl_SaveBitmapToBuffer( bufData, GetPreviewBitmap(), kImageFileFormat_JPG ) == CE_SUCCESS )
	{
		bError = !g_pFullFileSystem->WriteFile( szPreviewFilename, UGC_PATHID, bufData );

		// Get full path to give steam
		g_pFullFileSystem->RelativePathToFullPath( szPreviewFilename, UGC_PATHID, szPreviewFilename, sizeof( szPreviewFilename ) );
	}
	else
	{
		bError = true;
	}

	// Get the compressed map out of the upload directory
	char szPreparedMap[MAX_PATH] = { 0 };
	char szFullPreparedPath[MAX_PATH] = { 0 };
	if ( m_bAddingNewFile || m_nFileDetailsChanges & PFILE_FIELD_FILE )
	{
		GetPreparedFilename( szPreparedMap, sizeof( szPreparedMap ) );

		g_pFullFileSystem->RelativePathToFullPath( szPreparedMap, UGC_PATHID,
		                                           szFullPreparedPath,
		                                           sizeof( szFullPreparedPath ) );

		bError |= !*szFullPreparedPath;
	}

	if ( !bError )
	{
		// Set title
		char szTitle[k_cchPublishedDocumentTitleMax];
		m_pFileTitle->GetText( szTitle, sizeof(szTitle) );
		Q_AggressiveStripPrecedingAndTrailingWhitespace( szTitle );

		bError |= !pUGC->SetItemTitle( hItem, szTitle );

		// Set descriptor
		char szDesc[k_cchPublishedDocumentDescriptionMax];
		m_pFileDescription->GetText( szDesc, sizeof(szDesc) );
		Q_AggressiveStripPrecedingAndTrailingWhitespace( szDesc );

		bError |= !pUGC->SetItemDescription( hItem, szDesc );

		// Set thumbnail
		if ( m_bAddingNewFile || m_nFileDetailsChanges & PFILE_FIELD_PREVIEW )
		{
			bError |= !pUGC->SetItemPreview( hItem, szPreviewFilename );
		}

		// Set file
		if ( m_bAddingNewFile || m_nFileDetailsChanges & PFILE_FIELD_FILE )
		{
			if ( *szFullPreparedPath )
			{
				bError |= !pUGC->SetItemContent( hItem, szFullPreparedPath );
				// Metadata for our files is just the original filename, since they are currently all single files
				bError |= !pUGC->SetItemMetadata( hItem, V_GetFileName( g_MapFilename.Get() ) );
			}
			else
			{
				UGCWarning( "Prepared map does not appear to exist\n" );
				bError = true;
			}
		}

		// Tags
		SteamParamStringArray_t strArray;
		PopulateTags( strArray );
		bError |= !pUGC->SetItemTags( hItem, &strArray );

		// Visibility
		bError |= !pUGC->SetItemVisibility( hItem, k_ERemoteStoragePublishedFileVisibilityPublic );
	}
	else
	{
		bError = true;
	}

	if ( !bError )
	{
		SteamAPICall_t hSteamAPICall = steamapicontext->SteamUGC()->SubmitItemUpdate( hItem, NULL );
		m_callbackPublishFile.Set( hSteamAPICall, this, &CFilePublishDialog::Steam_OnPublishFile );
		return true;
	}

	// Failed, cleanup prepared map
	g_pFullFileSystem->RemoveFile( szPreparedMap, UGC_PATHID );

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFilePublishDialog::PerformLayout()
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Parse commands coming in from the VGUI dialog
//-----------------------------------------------------------------------------
void CFilePublishDialog::OnCommand( const char *command )
{
	if ( Q_stricmp( command, "Publish" ) == 0 )
	{
		// Verify they've filled everything out properly
		bool bHasTitle = ( m_pFileTitle->GetTextLength() > 0 );
		bool bHasDesc = ( m_pFileDescription->GetTextLength() > 0 );
		if ( !bHasTitle || !bHasDesc )
		{
			ErrorMessage( kNeedTitleAndDescription );
			return;
		}

		// Get our title
		char szTitle[k_cchPublishedDocumentTitleMax];
		m_pFileTitle->GetText( szTitle, sizeof(szTitle) );
		Q_AggressiveStripPrecedingAndTrailingWhitespace( szTitle );

		// Get our descriptor
		char szDesc[k_cchPublishedDocumentDescriptionMax];
		m_pFileDescription->GetText( szDesc, sizeof(szDesc) );
		Q_AggressiveStripPrecedingAndTrailingWhitespace( szDesc );

		bHasTitle = Q_strlen( szTitle ) != 0;
		bHasDesc = Q_strlen( szDesc ) != 0;
		if ( !bHasTitle || !bHasDesc )
		{
			ErrorMessage( kNeedTitleAndDescription );
			return;
		}

		PublishFile();
	}
	else if ( Q_stricmp( command, "Update" ) == 0 )
	{
		UpdateFile();
	}
	else if ( Q_stricmp( command, "MainFileMaps" ) == 0 )
	{
		m_fileOpenMode = FILEOPEN_MAIN_FILE;

		// Create a new dialog
		vgui::FileOpenDialog *pDlg = new vgui::FileOpenDialog( NULL, "Select File", true );
		pDlg->AddFilter( GetFileTypes( IMPORT_FILTER_MAP ), GetFileTypeDescriptions( IMPORT_FILTER_MAP ), true );
		if ( !FStrEq( publish_file_last_dir.GetString(), "" ) )
		{
			pDlg->SetStartDirectory( publish_file_last_dir.GetString() );
		}

		char textBuffer[1024];
		m_pFilename->GetText( textBuffer, sizeof( textBuffer ) );

		char szFilePath[MAX_PATH];
		g_pFullFileSystem->GetCurrentDirectory( szFilePath, sizeof(szFilePath) );

		strcat( szFilePath, "/" );
		strcat( szFilePath, textBuffer );

		// Get the currently set dir and use that as the start
		// pDlg->ExpandTreeToPath( szFilePath );
		pDlg->MoveToCenterOfScreen();
		pDlg->AddActionSignalTarget( this );
		pDlg->SetDeleteSelfOnClose( true );
		pDlg->DoModal();
		pDlg->Activate();
	}
	else if ( Q_stricmp( command, "MainFileOther" ) == 0 )
	{
		m_fileOpenMode = FILEOPEN_MAIN_FILE;

		// Create a new dialog
		vgui::FileOpenDialog *pDlg = new vgui::FileOpenDialog( NULL, "Select File", true );
		pDlg->AddFilter( GetFileTypes( IMPORT_FILTER_OTHER ), GetFileTypeDescriptions( IMPORT_FILTER_OTHER ), true );
		if ( !FStrEq( publish_file_last_dir.GetString(), "" ) )
		{
			pDlg->SetStartDirectory( publish_file_last_dir.GetString() );
		}

		char textBuffer[1024];
		m_pFilename->GetText( textBuffer, sizeof( textBuffer ) );

		char szFilePath[MAX_PATH];
		g_pFullFileSystem->GetCurrentDirectory( szFilePath, sizeof( szFilePath ) );

		strcat( szFilePath, "/" );
		strcat( szFilePath, textBuffer );

		// Get the currently set dir and use that as the start
		// pDlg->ExpandTreeToPath( szFilePath );
		pDlg->MoveToCenterOfScreen();
		pDlg->AddActionSignalTarget( this );
		pDlg->SetDeleteSelfOnClose( true );
		pDlg->DoModal();
		pDlg->Activate();
	}
	else if ( Q_stricmp( command, "PreviewBrowse" ) == 0 )
	{
		m_fileOpenMode = FILEOPEN_PREVIEW;
		
		// Create a new dialog
		vgui::FileOpenDialog *pDlg = new vgui::FileOpenDialog( NULL, "Select File", true );
		pDlg->AddFilter( GetPreviewFileTypes(), GetPreviewFileTypeDescriptions(), true );
		if ( !FStrEq( publish_file_last_dir.GetString(), "" ) )
		{
			pDlg->SetStartDirectory( publish_file_last_dir.GetString() );
		}

		char szFilePath[MAX_PATH];
		g_pFullFileSystem->GetCurrentDirectory( szFilePath, sizeof(szFilePath) );

		strcat( szFilePath, "/" );
		strcat( szFilePath, g_PreviewFilename );

		// Get the currently set dir and use that as the start
		// pDlg->ExpandTreeToPath( szFilePath );
		pDlg->MoveToCenterOfScreen();
		pDlg->AddActionSignalTarget( this );
		pDlg->SetDeleteSelfOnClose( true );
		pDlg->DoModal();
		pDlg->Activate();
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Take a filename, shorten it for display but retain the full path internally
//-----------------------------------------------------------------------------
CFilePublishDialog::ErrorCode_t CFilePublishDialog::ValidateFile( const char *lpszFilename )
{
	NoteUnused( lpszFilename );
	return kNoError;
}

//-----------------------------------------------------------------------------
// Purpose: Take a filename, shorten it for display but retain the full path internally
//-----------------------------------------------------------------------------
void CFilePublishDialog::SetFile( const char *lpszFilename, bool bImported )
{
	// Must be a valid file
	ErrorCode_t errorCode = ValidateFile( lpszFilename );
	if ( errorCode != kNoError )
	{
		ErrorMessage( errorCode );
		return;
	}

	m_bValidFile = true;
	g_MapFilename = lpszFilename;

	char szShortName[ MAX_PATH ];
	Q_FileBase( g_MapFilename, szShortName, sizeof(szShortName) );
	const char *szExt = Q_GetFileExtension( lpszFilename );
	Q_SetExtension( szShortName, CFmtStr( ".%s", szExt ).Access(), sizeof(szShortName ) );
	m_pFilename->SetText( szShortName );

	// Notify of the change
	m_nFileDetailsChanges |= PFILE_FIELD_FILE;

	SetPublishButtonState();
}

//-----------------------------------------------------------------------------
// Purpose: Notify us that the directory dialog has returned a new entry
//-----------------------------------------------------------------------------
void CFilePublishDialog::OnFileSelected( const char *fullPath )
{
	char basepath[ MAX_PATH ];
	Q_ExtractFilePath( fullPath, basepath, sizeof( basepath ) );
	publish_file_last_dir.SetValue( basepath );

	if ( m_fileOpenMode == FILEOPEN_MAIN_FILE )
	{
		SetFile( fullPath );
	}
	else if ( m_fileOpenMode == FILEOPEN_PREVIEW )
	{
		// Notify of the change
		m_nFileDetailsChanges |= PFILE_FIELD_PREVIEW;

		SetPreviewImage( fullPath );
	}
}
