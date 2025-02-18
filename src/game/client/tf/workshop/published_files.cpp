//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"

#include "imageutils.h"
#include "econ_controls.h"
#include "econ/confirm_dialog.h"
#include "game/client/iviewport.h"
#include "ienginevgui.h"
#include "tf_hud_mainmenuoverride.h"
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "vgui/ISystem.h"
#include "vgui_bitmappanel.h"
#include <vgui_controls/FileOpenDialog.h>


#include "steampublishedfiles/publish_file_dialog.h"

#include "tier1/checksum_crc.h"

// for hud element
#include "iclientmode.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#define TF2_PREVIEW_IMAGE_HEIGHT 512
#define TF2_PREVIEW_IMAGE_WIDTH 512

#define COMMUNITY_DEV_HOST "http://localhost/"

extern ConVar publish_file_last_dir;

// milliseconds
ConVar tf_steam_workshop_query_timeout( "tf_steam_workshop_query_timeout", "10", FCVAR_CLIENTDLL, "Time in seconds to allow communication with the Steam Workshop server." );
ConVar tf_steam_workshop_page_skip( "tf_steam_workshop_page_skip", "10", FCVAR_ARCHIVE, "Number of pages to skip in the Steam Workshop dialog.", true, 1, true, 100 );

//-----------------------------------------------------------------------------
// Purpose: Utility function
//-----------------------------------------------------------------------------
static void MakeModalAndBringToFront( vgui::EditablePanel *dialog )
{
	dialog->SetVisible( true );
	if ( dialog->GetParent() == NULL )
	{
		dialog->MakePopup();
	}
	dialog->SetZPos( 10000 );
	dialog->MoveToFront();
	dialog->SetKeyBoardInputEnabled( true );
	dialog->SetMouseInputEnabled( true );
	TFModalStack()->PushModal( dialog );
}

//-----------------------------------------------------------------------------
// Purpose: helper class that contains the list of published files for a user
//-----------------------------------------------------------------------------
class CPublishedFiles
{
public:
	enum
	{
		kState_Initialized,
		kState_PopulatingFileList,
		kState_ErrorOccurred,
		kState_DeletingFile,
		kState_ErrorCannotDeleteFile,
		kState_Timeout,
		kState_Done,
	};

	CPublishedFiles();
	~CPublishedFiles();

	bool QueryHasTimedOut( void );
	void PopulateFileList( void );
	bool EnumerateUserPublishedFiles( uint32 unPage );
	void RefreshPublishedFileDetails( uint64 nPublishedFileID );
	void DeletePublishedFile( uint64 nPublishedFileID );
	void ViewPublishedFile( uint64 nPublishedFileID );
	const SteamUGCDetails_t *GetPublishedFileDetails( uint64 nPublishedFileID ) const;

	// Enumerate subscribed files
	CCallResult<CPublishedFiles, SteamUGCQueryCompleted_t> m_callbackEnumeratePublishedFiles;
	void Steam_OnEnumeratePublishedFiles( SteamUGCQueryCompleted_t *pResult, bool bError );

	// Callback for deleting files
	CCallResult<CPublishedFiles, RemoteStorageDeletePublishedFileResult_t> m_callbackDeletePublishedFile;
	void Steam_OnDeletePublishedFile( RemoteStorageDeletePublishedFileResult_t *pResult, bool bError );

	unsigned int m_nTotalFilesToQuery;
	CUtlQueue< PublishedFileId_t >	m_FilesToQuery;
	CUtlMap< PublishedFileId_t, SteamUGCDetails_t >	m_FileDetails;

	long	m_nFileQueryTime;
	bool	m_bQueryErrorOccurred;
	uint32  m_state;
	uint32	m_unEnumerateStartPage;
	PublishedFileId_t m_nCurrentQueryPublishedFileID;
};

CPublishedFiles::CPublishedFiles()
	: m_nTotalFilesToQuery( 0 )
	, m_nFileQueryTime( 0 )
	, m_bQueryErrorOccurred( false )
	, m_state( kState_Initialized )
	, m_unEnumerateStartPage( 0 )
	, m_nCurrentQueryPublishedFileID( 0 )
{
	m_FileDetails.SetLessFunc( DefLessFunc( PublishedFileId_t ) );
}

CPublishedFiles::~CPublishedFiles()
{
}


//-----------------------------------------------------------------------------
// Purpose:	Determine if our file query has timed out
//-----------------------------------------------------------------------------
bool CPublishedFiles::QueryHasTimedOut( void )
{
	return ( ( system()->GetTimeMillis() - m_nFileQueryTime ) > tf_steam_workshop_query_timeout.GetInt() * 1000 );
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
void CPublishedFiles::PopulateFileList( void )
{
	// Start the process of showing all of our published files
	if ( EnumerateUserPublishedFiles( 1 ) )
	{
		m_unEnumerateStartPage = 1;

		// Get our starting tick
		m_nFileQueryTime = system()->GetTimeMillis();

		m_state = kState_PopulatingFileList;
	}
}

//-----------------------------------------------------------------------------
// Purpose:	Wrapper for creating and sending a CreateQueryUserUGCRequest for this user's published files
//-----------------------------------------------------------------------------
bool CPublishedFiles::EnumerateUserPublishedFiles( uint32 unPage )
{
	ISteamUGC *pUGC = steamapicontext->SteamUGC();
	ISteamUser *pUser = steamapicontext->SteamUser();
	if ( pUGC && pUser )
	{
		AccountID_t nAccountID = pUser->GetSteamID().GetAccountID();

		UGCQueryHandle_t ugcHandle = pUGC->CreateQueryUserUGCRequest( nAccountID,
		                                                              k_EUserUGCList_Published,
		                                                              k_EUGCMatchingUGCType_Items,
		                                                              k_EUserUGCListSortOrder_CreationOrderDesc,
		                                                              engine->GetAppID(), engine->GetAppID(), unPage );

		// make sure we get the entire description and not a truncated version
		pUGC->SetReturnLongDescription( ugcHandle, true );

		SteamAPICall_t hSteamAPICall = pUGC->SendQueryUGCRequest( ugcHandle );
		m_callbackEnumeratePublishedFiles.Set( hSteamAPICall, this, &CPublishedFiles::Steam_OnEnumeratePublishedFiles );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
void CPublishedFiles::DeletePublishedFile( uint64 nPublishedFileID )
{
	m_state = kState_DeletingFile;
	SteamAPICall_t hSteamAPICall = steamapicontext->SteamRemoteStorage()->DeletePublishedFile( nPublishedFileID );
	m_callbackDeletePublishedFile.Set( hSteamAPICall, this, &CPublishedFiles::Steam_OnDeletePublishedFile ); 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const SteamUGCDetails_t *CPublishedFiles::GetPublishedFileDetails( uint64 nPublishedFileID ) const
{
	int idx = m_FileDetails.Find( nPublishedFileID );
	if ( idx != m_FileDetails.InvalidIndex() )
	{
		return &m_FileDetails[idx];
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPublishedFiles::ViewPublishedFile( uint64 nPublishedFileID )
{
	EUniverse universe = GetUniverse();
	switch ( universe )
	{
	case k_EUniversePublic:
		steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStrMax( "http://steamcommunity.com/sharedfiles/filedetails/?id=%llu", nPublishedFileID ) );
		break;
	case k_EUniverseBeta:
		steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStrMax( "http://beta.steamcommunity.com/sharedfiles/filedetails/?id=%llu", nPublishedFileID ) );
		break;
	case k_EUniverseDev:
		steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStrMax( COMMUNITY_DEV_HOST "sharedfiles/filedetails/?id=%llu", nPublishedFileID ) );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
void CPublishedFiles::RefreshPublishedFileDetails( uint64 nPublishedFileID )
{
	if ( m_state != kState_Done )
	{
		// Would confuse the refresh in progress
		AssertMsg( m_state == kState_Done, "Shouldn't be refreshing file details while other operations are already happening" );
		return;
	}

	ISteamUGC *pUGC = steamapicontext->SteamUGC();
	Assert( pUGC );
	if ( pUGC  )
	{
		UGCQueryHandle_t ugcHandle = pUGC->CreateQueryUGCDetailsRequest( &nPublishedFileID, 1 );
		SteamAPICall_t hSteamAPICall = pUGC->SendQueryUGCRequest( ugcHandle );
		m_callbackEnumeratePublishedFiles.Set( hSteamAPICall, this, &CPublishedFiles::Steam_OnEnumeratePublishedFiles );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPublishedFiles::Steam_OnDeletePublishedFile( RemoteStorageDeletePublishedFileResult_t *pResult, bool bError )
{
	if ( pResult && !bError )
	{
		if ( pResult->m_eResult != k_EResultOK )
		{
			m_state = kState_ErrorOccurred;
			if ( pResult->m_eResult == k_EResultAccessDenied )
			{
				m_state = kState_ErrorCannotDeleteFile;
			}
		}
		else
		{
			m_state = kState_Done;
			m_FileDetails.Remove( pResult->m_nPublishedFileId );
		}
	}
	else 
	{
		m_state = kState_ErrorOccurred;
	}
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
void CPublishedFiles::Steam_OnEnumeratePublishedFiles( SteamUGCQueryCompleted_t *pResult, bool bError )
{
	// Make sure we succeeded
	if ( bError || pResult->m_eResult != k_EResultOK )
	{
		m_state = kState_ErrorOccurred;
		return;
	}

	if ( m_state == kState_PopulatingFileList && m_unEnumerateStartPage == 1 )
	{
		// Start from scratch
		m_FilesToQuery.Purge();
		m_FileDetails.Purge();
	}

	const int nNumFiles = pResult->m_unNumResultsReturned;
	for ( int i = 0; i < nNumFiles; i++ )
	{
		SteamUGCDetails_t sDetails = { 0 };
		steamapicontext->SteamUGC()->GetQueryUGCResult( pResult->m_handle, i, &sDetails );
		m_FileDetails.InsertOrReplace( sDetails.m_nPublishedFileId, sDetails );
	}

	if ( m_state == kState_Done )
	{
		// This was a one-off request from e.g. RefreshPublishedFileDetails
		return;
	}

	if ( !nNumFiles || m_FileDetails.Count() >= pResult->m_unTotalMatchingResults )
	{
		m_state = kState_Done;
		return;
	}

	EnumerateUserPublishedFiles( ++m_unEnumerateStartPage );
}

// Tags
static const char *kClassTags[TF_LAST_NORMAL_CLASS] = {
	"",			// TF_CLASS_UNDEFINED
	"Scout",	// TF_CLASS_SCOUT
	"Sniper",	// TF_CLASS_SNIPER,
	"Soldier",	// TF_CLASS_SOLDIER,
	"Demoman",	// TF_CLASS_DEMOMAN,
	"Medic",	// TF_CLASS_MEDIC,
	"Heavy",	//TF_CLASS_HEAVYWEAPONS,
	"Pyro",		// TF_CLASS_PYRO,
	"Spy",		// TF_CLASS_SPY,
	"Engineer",	// TF_CLASS_ENGINEER,
};

struct TagPair_t
{
	const char *pCheckboxElementName;
	const char *pTag;
};
static TagPair_t kOtherTags[] = {
	{ "TagCheckbox_Headgear", "Headgear" },
	{ "TagCheckbox_Weapon", "Weapon" },
	{ "TagCheckbox_Misc", "Misc" },
	{ "TagCheckbox_SoundDevice", "Sound Device" },
	{ "TagCheckbox_Halloween", "Halloween" },
	{ "TagCheckbox_Taunt", "Taunt" },
	{ "TagCheckbox_UnusualEffect", "Unusual Effect" },
	{ "TagCheckbox_Jungle", "Jungle" },
	{ "TagCheckbox_WarPaint", "War Paint" },
	{ "TagCheckbox_Smissmas", "Smissmas" },
	{ "TagCheckbox_Summer", "Summer" },
	{ "TagCheckbox_CommunityFix", "Community Fix" },
};
static uint32 kNumOtherTags = ARRAYSIZE( kOtherTags );

// Map Tags
static TagPair_t kMapTags[] = {
	{ "MapsCheckBox_CTF", "Capture the Flag" }, 
	{ "MapsCheckBox_CP", "Control Point" },
	{ "MapsCheckBox_Escort", "Payload" },
	{ "MapsCheckBox_EscortRace", "Payload Race" },
	{ "MapsCheckBox_Arena", "Arena" },
	{ "MapsCheckBox_Koth", "King of the Hill" },
	{ "MapsCheckBox_AttackDefense", "Attack / Defense" },
	{ "MapsCheckBox_SD", "Special Delivery" },
	{ "MapsCheckBox_RobotDestruction", "Robot Destruction" },
	{ "MapsCheckBox_MVM", "Mann vs. Machine" },
	{ "MapsCheckBox_Powerup", "Mannpower" },
	{ "MapsCheckBox_Medieval", "Medieval" },
	{ "MapsCheckBox_PassTime", "PASS Time" },
	{ "MapsCheckBox_Specialty", "Specialty" },
	{ "MapsCheckBox_Halloween", "Halloween" },
	{ "MapsCheckbox_Smissmas", "Smissmas" },
	{ "MapsCheckbox_Night", "Night" },
	{ "MapsCheckbox_Jungle", "Jungle" },
	{ "MapsCheckBox_PD", "Player Destruction" },
	{ "MapsCheckBox_Summer", "Summer" },
};
static uint32 kNumMapTags = ARRAYSIZE( kMapTags );

static const char *kImportedTag = "Certified Compatible";

//-----------------------------------------------------------------------------
// Purpose: Publish file dialog
//-----------------------------------------------------------------------------
class CTFFilePublishDialog : public CFilePublishDialog
{
	DECLARE_CLASS_SIMPLE( CTFFilePublishDialog, CFilePublishDialog );
public:
	CTFFilePublishDialog( Panel *parent, const char *name, PublishedFileDetails_t *pDetails ) : CFilePublishDialog( parent, name, pDetails ), m_bImported(false) {}

	virtual ErrorCode_t ValidateFile( const char *lpszFilename ) OVERRIDE
	{
		if( !g_pFullFileSystem->FileExists( lpszFilename ) )
			return kFailedFileNotFound;

		return kNoError;
	}
	virtual void OnFilePrepared( ErrorCode_t eResult ) OVERRIDE
	{
		if ( eResult == kNoError )
		{
			// Fail now if our prepared file didn't make it below the size limit.  We don't do this in ValidateFile
			// because preparing files can shrink them -- maps are compressed as their prepare step.
			char szPreparedFile[ MAX_PATH ] = { 0 };
			GetPreparedFilename( szPreparedFile, sizeof( szPreparedFile ) );

			// 400MB is the current nominal max of SteamUGC, softly encourage people to keep it below 200MB compressed
			// for now, however.
			const uint32 kMaxFileSize = 200 * 1024 * 1024;
			unsigned int unFileSize = g_pFullFileSystem->Size( szPreparedFile );
			if ( unFileSize == 0 || unFileSize > kMaxFileSize )
			{
				Warning( "TF Workshop: File was %u bytes after prepare step, max %u\n", unFileSize, kMaxFileSize );
				eResult = kFailedFileTooLarge;
			}
		}

		BaseClass::OnFilePrepared( eResult );
	}
	virtual AppId_t	GetTargetAppID( void ) { return engine->GetAppID(); }
	virtual unsigned int DesiredPreviewHeight( void ) { return TF2_PREVIEW_IMAGE_HEIGHT; }
	virtual unsigned int DesiredPreviewWidth( void ) { return TF2_PREVIEW_IMAGE_WIDTH; }
	virtual bool BForceSquarePreviewImage( void ) { return true; }
	virtual EWorkshopFileType WorkshipFileTypeForFile( const char *pszFileName ) {
		const char *pExt = V_GetFileExtension( pszFileName );
		if ( pExt && V_strcmp( pExt, "bsp" ) == 0 )
		{
			return k_EWorkshopFileTypeCommunity;
		}

		AssertMsg( pExt && V_strcmp( pExt, "zip" ) == 0, "Unrecognized file type, defaulting to microtransaction\n" );
		return k_EWorkshopFileTypeMicrotransaction;
	}
	virtual const char *GetPreviewFileTypes( void )	{ return "*.tga,*.jpg,*.png"; }
	virtual const char *GetPreviewFileTypeDescriptions( void ) { return "#TF_SteamWorkshop_Images"; }
	virtual const char *GetFileTypes( eFilterType_t eType = IMPORT_FILTER_NONE ) OVERRIDE
	{
		if ( eType == IMPORT_FILTER_MAP )
		{
			return "*.bsp";
		}
		return "*.zip";
	}
	virtual const char *GetFileTypeDescriptions( eFilterType_t eType = IMPORT_FILTER_NONE ) OVERRIDE
	{
		if ( eType == IMPORT_FILTER_MAP )
		{
			return "#TF_SteamWorkshop_AcceptableFilesMaps";
		}
		return "#TF_SteamWorkshop_AcceptableFiles"; 
	}
	virtual const char *GetResFile() const { return "Resource/UI/PublishFileDialog.res"; }

	virtual void SetFile( const char *lpszFilename, bool bImported )
	{
		BaseClass::SetFile( lpszFilename, bImported );
		SetTagsVisible( true, WorkshipFileTypeForFile( lpszFilename ) );

		m_sFilePath = lpszFilename;

		CUtlBuffer buffer;
		g_pFullFileSystem->ReadFile( lpszFilename, NULL, buffer );
		m_fileCRC = CRC32_ProcessSingleBuffer( buffer.Base(), buffer.Size() );

		m_bImported = bImported;
	}

	void SetTagsVisible( bool visible, EWorkshopFileType eFileType = k_EWorkshopFileTypeCommunity )
	{
		uint32 i;
		vgui::CheckButton *pButton;

		vgui::Label *pLabel = FindControl<vgui::Label>( "TagsTitle", true );
		if ( pLabel )
		{
			pLabel->SetVisible( visible );
		}

		if ( !visible )
		{
			// no tags visible, so hide everything
			// class tags
			for ( i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; i++ )
			{
				pButton = FindControl<vgui::CheckButton>( VarArgs( "ClassCheckBox%d", i ), true );
				if ( pButton )
				{
					pButton->SetVisible( visible );
				}
			}

			// other tags
			for ( i = 0; i < kNumOtherTags; ++i )
			{
				pButton = FindControl<vgui::CheckButton>( kOtherTags[i].pCheckboxElementName, true );
				if ( pButton )
				{
					pButton->SetVisible( visible );
				}
			}

			// map tags
			for ( i = 0; i < kNumMapTags; ++i )
			{
				pButton = FindControl<vgui::CheckButton>( kMapTags[i].pCheckboxElementName, true );
				if ( pButton )
				{
					pButton->SetVisible( visible );
				}
			}
		}
		else
		{
			bool bIsMap = ( eFileType == k_EWorkshopFileTypeCommunity );

			// class tags
			for ( i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; i++ )
			{
				pButton = FindControl<vgui::CheckButton>( VarArgs( "ClassCheckBox%d", i ), true );
				if ( pButton )
				{
					pButton->SetVisible( !bIsMap );

					if ( bIsMap && pButton->IsSelected() )
					{
						pButton->SetSelected( false ); // reset this button if we're using the maps tags
					}
				}
			}

			// other tags
			for ( i = 0; i < kNumOtherTags; ++i )
			{
				pButton = FindControl<vgui::CheckButton>( kOtherTags[i].pCheckboxElementName, true );
				if ( pButton )
				{
					pButton->SetVisible( !bIsMap );

					if ( bIsMap && pButton->IsSelected() )
					{
						pButton->SetSelected( false ); // reset this button if we're using the maps tags
					}
				}
			}

			// map tags
			for ( i = 0; i < kNumMapTags; ++i )
			{
				pButton = FindControl<vgui::CheckButton>( kMapTags[i].pCheckboxElementName, true );
				if ( pButton )
				{
					pButton->SetVisible( bIsMap );

					if ( !bIsMap && pButton->IsSelected() )
					{
						pButton->SetSelected( false ); // reset this button if we're not using the maps tags
					}
				}
			}
		}
	}

	virtual void PopulateTags( SteamParamStringArray_t &strArray )
	{
		m_vecTags.RemoveAll();

		// class tags
		vgui::EditablePanel* pClassUsagePanel = dynamic_cast<vgui::EditablePanel*>( FindChildByName( "ClassUsagePanel" ) );
		if ( pClassUsagePanel )
		{
			for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; i++ )
			{
				if ( IsChildButtonSelected( pClassUsagePanel, VarArgs("ClassCheckBox%d",i), false ) )
				{
					m_vecTags.AddToTail( kClassTags[i] );
				}
			}
		}

		// other tags
		for ( uint32 i = 0; i < kNumOtherTags; ++i )
		{
			if ( IsChildButtonSelected( this, kOtherTags[i].pCheckboxElementName, true ) )
			{
				m_vecTags.AddToTail( kOtherTags[i].pTag );
			}
		}

		// map tags
		for ( uint32 i = 0; i < kNumMapTags; ++i )
		{
			if ( IsChildButtonSelected( this, kMapTags[i].pCheckboxElementName, true ) )
			{
				m_vecTags.AddToTail( kMapTags[i].pTag );
			}
		}

		if ( m_bImported )
		{
			m_vecTags.AddToTail( kImportedTag );
		}

		strArray.m_ppStrings = m_vecTags.Base();
		strArray.m_nNumStrings = m_vecTags.Count();
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		// Center it, keeping requested size
		int x, y, ww, wt, wide, tall;
		vgui::surface()->GetWorkspaceBounds( x, y, ww, wt );
		GetSize(wide, tall);
		SetPos(x + ((ww - wide) / 2), y + ((wt - tall) / 2));
	}

	virtual bool PrepSteamCloudFilePath( const char *lpszFileName, CUtlString &steamCloudFileName )
	{
		char szShortName[ MAX_PATH ];
		Q_FileBase( lpszFileName, szShortName, sizeof( szShortName ) );
		const char *szExt = Q_GetFileExtension( lpszFileName );
		Q_SetExtension( szShortName, CFmtStr( ".%s", szExt ).Access(), sizeof(szShortName ) );
		steamCloudFileName.Format( "steamworkshop/tf2/%s", szShortName );
		return true;
	}

	virtual void ErrorMessage( ErrorCode_t errorCode, KeyValues *pkvTokens )
	{
		switch ( errorCode )
		{
		case kFailedToPublishFile:
			ShowMessageBox( "#TF_PublishFile_Error", "#TF_PublishFile_kFailedToPublishFile", "#GameUI_OK" );
			break;
		case kFailedToPrepareFile:
			ShowMessageBox( "#TF_PublishFile_Error", "#TF_PublishFile_kFailedToPrepareFile", "#GameUI_OK" );
			break;
		case kFailedToUpdateFile:
			ShowMessageBox( "#TF_PublishFile_Error", "#TF_PublishFile_kFailedToUpdateFile", "#GameUI_OK" );
			break;
		case kSteamCloudNotAvailable:
			ShowMessageBox( "#TF_PublishFile_Error", "#TF_PublishFile_kSteamCloudNotAvailable", "#GameUI_OK" );
			break;
		case kSteamExceededCloudQuota:
			ShowMessageBox( "#TF_PublishFile_Error", "#TF_PublishFile_kSteamExceededCloudQuota", "#GameUI_OK" );
			break;
		case kFailedToWriteToSteamCloud:
			ShowMessageBox( "#TF_PublishFile_Error", "#TF_PublishFile_kFailedToWriteToSteamCloud", "#GameUI_OK" );
			break;
		case kFileNotFound:
			ShowMessageBox( "#TF_PublishFile_Error", "#TF_PublishFile_kFileNotFound", "#GameUI_OK" );
			break;
		case kNeedTitleAndDescription:
			ShowMessageBox( "#TF_PublishFile_Error", "#TF_PublishFile_kNeedTitleAndDescription", "#GameUI_OK" );
			break;
		case kFailedFileValidation:
			ShowMessageBox( "#TF_PublishFile_Error", "#TF_PublishFile_kFailedFileValidation", "#GameUI_OK" );
			break;
		case kFailedFileTooLarge:
			ShowMessageBox( "#TF_PublishFile_Error", "#TF_PublishFile_kFailedFileTooLarge", "#GameUI_OK" );
			break;
		case kFailedFileNotFound:
			ShowMessageBox( "#TF_PublishFile_Error", "#TF_PublishFile_kFailedFileNotFound", "#GameUI_OK" );
			break;
		case kFailedUserModifiedFile:
			ShowMessageBox( "#TF_PublishFile_Error", "#TF_PublishFile_kFailedUserModifiedFile", "#GameUI_OK" );
			break;
		case kInvalidMapName:
			ShowMessageBox( "#TF_PublishFile_Error", "#TF_PublishFile_kInvalidMapName", "#GameUI_OK" );
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

	void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		if ( !m_bAddingNewFile )
		{
			bool bIsMap = ( m_FileDetails.publishedFileDetails.m_eFileType == k_EWorkshopFileTypeCommunity );

			CExImageButton *pImageButton = FindControl<CExImageButton>( "ButtonSourceCosmetics", true );
			if ( pImageButton )
			{
				pImageButton->SetVisible( !bIsMap );
			}

			vgui::Button *pButton = FindControl<Button>( "ButtonSourceOther", true );
			if ( pButton )
			{
				pButton->SetVisible( !bIsMap );
			}

			pImageButton = FindControl<CExImageButton>( "ButtonSourceMaps", true );
			if ( pImageButton )
			{
				pImageButton->SetVisible( bIsMap );
			}
		}
	}

protected:

	virtual void PopulateEditFields( void )
	{
		BaseClass::PopulateEditFields();
		
		// class tags
		vgui::EditablePanel* pClassUsagePanel = dynamic_cast<vgui::EditablePanel*>( FindChildByName( "ClassUsagePanel" ) );
		if ( pClassUsagePanel )
		{
			for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; i++ )
			{
				bool bHasTag = Q_strstr( m_FileDetails.publishedFileDetails.m_rgchTags, kClassTags[i] ) != 0;
				SetChildButtonSelected( pClassUsagePanel, VarArgs("ClassCheckBox%d",i), bHasTag );
			}
		}

		// other tags
		for ( uint32 i = 0; i < kNumOtherTags; ++i )
		{
			bool bHasTag = Q_strstr( m_FileDetails.publishedFileDetails.m_rgchTags, kOtherTags[i].pTag ) != 0;
			SetChildButtonSelected( this, kOtherTags[i].pCheckboxElementName, bHasTag, true );
		}

		// map tags
		for ( uint32 i = 0; i < kNumMapTags; ++i )
		{
			bool bHasTag = Q_strstr( m_FileDetails.publishedFileDetails.m_rgchTags, kMapTags[i].pTag ) != 0;
			SetChildButtonSelected( this, kMapTags[i].pCheckboxElementName, bHasTag, true );
		}

		if ( Q_strstr( m_FileDetails.publishedFileDetails.m_rgchTags, kImportedTag ) )
		{
			m_bImported = true;
		}
		else
		{
			m_bImported = false;
		}

		// Default the tags hidden until there is a file set
		if ( !m_FileDetails.lpszFilename )
		{
			SetTagsVisible( false );
		}
		else
		{
			SetTagsVisible( true, m_FileDetails.publishedFileDetails.m_eFileType );
		}

		vgui::Button *pImportButton = FindControl<vgui::Button>( "ButtonSourceCosmetics" );
		if ( pImportButton )
			pImportButton->SetVisible( false );
	}

	const char* GetStatusString( StatusCode_t statusCode )
	{
		switch ( statusCode )
		{
		case kPublishing:
			return "#TF_PublishFile_Publishing";
			break;
		case kUpdating:
			return "#TF_PublishFile_Updating";
			break;
		}
		return "";
	}

	void ShowStatusWindow( StatusCode_t statusCode )
	{
		ShowWaitingDialog( new CGenericWaitingDialog( this ), GetStatusString( statusCode ), true, false, 0.0f );
	}

	void HideStatusWindow( void )
	{
		CloseWaitingDialog();
	}

	virtual void OnCommand( const char *command )
	{	
		if ( V_stricmp( command, "MainFileCosmetics" ) == 0 )
		{
		}
		else if ( V_stricmp( command, "Publish" ) == 0 || V_stricmp( command, "Update" ) == 0 )
		{
			if ( m_bImported )
			{
				CUtlBuffer buffer;
				g_pFullFileSystem->ReadFile( m_sFilePath, NULL, buffer );
				CRC32_t crc = CRC32_ProcessSingleBuffer( buffer.Base(), buffer.Size() );
				if ( crc != m_fileCRC )
				{
					ErrorMessage( kFailedUserModifiedFile, NULL );
					m_bImported = false;
					return;
				}
			}

			BaseClass::OnCommand( command );
		}
		else
		{
			BaseClass::OnCommand( command );
		}
	}

private:
	CUtlVector< const char* > m_vecTags;

	CRC32_t m_fileCRC;
	CUtlString m_sFilePath;
	bool m_bImported;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CUGCFileRequestCache
{
public:
	CUGCFileRequestCache()
	{
		m_mapPreviewFileRequests.SetLessFunc( DefLessFunc( UGCHandle_t ) );
	}

	~CUGCFileRequestCache()
	{
		m_mapPreviewFileRequests.PurgeAndDeleteElements();
	}

	CUGCFileRequest *GetFileRequest( UGCHandle_t fileHandle )
	{
		int idx = m_mapPreviewFileRequests.Find( fileHandle );
		if ( idx == m_mapPreviewFileRequests.InvalidIndex() )
		{
			idx = m_mapPreviewFileRequests.Insert( fileHandle, new CUGCFileRequest() );
		}
		return m_mapPreviewFileRequests[idx];
	}

private:
	CUtlMap< UGCHandle_t, CUGCFileRequest* > m_mapPreviewFileRequests;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSteamWorkshopItemPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CSteamWorkshopItemPanel, vgui::EditablePanel );
public:
	CSteamWorkshopItemPanel( vgui::Panel *parent, const char *panelName, CUGCFileRequestCache &previewFileCache )
		: vgui::EditablePanel( parent, panelName )
		, m_bSelected( false )
		, m_bPreviewDownloadPending( false )
		, m_pUGCPreviewFileRequest( NULL )
		, m_previewFileCache( previewFileCache )

	{
		memset( &m_FileDetails, 0, sizeof( m_FileDetails ) );
		m_pCroppedTextureImagePanel = new CBitmapPanel( this, "PreviewImage" );	
		m_pHighlightPanel = new vgui::EditablePanel( this, "HighlightPanel" );
		vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
	}

	void SetPublishedFileDetails( const SteamUGCDetails_t *pDetails )
	{
		if ( pDetails )
		{
			m_FileDetails = *pDetails;

			SetDialogVariable( "title", m_FileDetails.m_rgchTitle );
			DownloadPreviewImage();

			SetVisible( true );
			InvalidateLayout( true );
		}
		else
		{
			m_bPreviewDownloadPending = false;
			SetVisible( false );
			m_pUGCPreviewFileRequest = NULL;
		}

		m_bSelected = false;
		UpdateBorder();
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );
		LoadControlSettings( "Resource/UI/SteamWorkshopItem.res" );
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();
	}

	virtual void OnTick()
	{
		BaseClass::OnTick();

		if ( m_bPreviewDownloadPending && m_pUGCPreviewFileRequest )
		{
			UGCFileRequestStatus_t ugcStatus = m_pUGCPreviewFileRequest->Update();
			switch ( ugcStatus )
			{
			case UGCFILEREQUEST_ERROR:
				m_bPreviewDownloadPending = false;
				break;

			case UGCFILEREQUEST_FINISHED:
				SetPreviewImage();
				m_bPreviewDownloadPending = false;
				break;

			default:
				// Working, continue to wait...
				return;
				break;
			}
		}
	}

	virtual void OnCursorEntered()
	{
		m_pHighlightPanel->SetVisible( true );
	}

	virtual void OnCursorExited()
	{
		UpdateBorder();
	}

	virtual void OnMousePressed(vgui::MouseCode code)
	{
		if ( code != MOUSE_LEFT )
			return;
		
		PostActionSignal( new KeyValues( "ItemPanelMousePressed" ) );
	}

	void UpdateBorder()
	{
		m_pHighlightPanel->SetVisible( m_bSelected );
	}

	void SetSelected( bool bSelected ) { m_bSelected = bSelected; }
	bool GetSelected() const { return m_bSelected; }

	uint64 GetPublishedFileID() const { return m_FileDetails.m_nPublishedFileId; }

protected:
	void DownloadPreviewImage()
	{
		m_pCroppedTextureImagePanel->SetImage( NULL );
		m_pUGCPreviewFileRequest = m_previewFileCache.GetFileRequest( m_FileDetails.m_hPreviewFile );

		switch ( m_pUGCPreviewFileRequest->GetStatus() )
		{
			case UGCFILEREQUEST_READY:
			{
				// Start off our download
				char szTargetFilename[MAX_PATH];
				V_snprintf( szTargetFilename, sizeof(szTargetFilename), "%llu_thumb.jpg", m_FileDetails.m_nPublishedFileId );
				m_pUGCPreviewFileRequest->StartDownload( m_FileDetails.m_hPreviewFile, "downloads", szTargetFilename );
				m_bPreviewDownloadPending = true;
			}
			break;

			case UGCFILEREQUEST_DOWNLOADING:
			case UGCFILEREQUEST_DOWNLOAD_WRITING:
			{
				// download already going for another preview panel
				m_bPreviewDownloadPending = true;
			}
			break;
			
			case UGCFILEREQUEST_FINISHED:
			{
				SetPreviewImage();
			}
			break;

			default:
			{
				m_pCroppedTextureImagePanel->SetVisible( false );
			}
			break;
		} // switch
	}

	void SetPreviewImage()
	{
		// Update our image preview
		char szLocalFilename[MAX_PATH];
		m_pUGCPreviewFileRequest->GetLocalFileName( szLocalFilename, sizeof(szLocalFilename) );
		char szLocalPath[ _MAX_PATH ];
		g_pFullFileSystem->GetLocalPath( szLocalFilename, szLocalPath, sizeof(szLocalPath) );
		SetPreviewImage( szLocalPath );
	}

	void SetPreviewImage( const char *lpszFilename )
	{
		if ( lpszFilename == NULL )
			return;

		ConversionErrorType nErrorCode = ImgUtl_LoadBitmap( lpszFilename, m_imgSource );
		if ( nErrorCode == CE_SUCCESS )
		{
			m_pCroppedTextureImagePanel->SetBitmap( m_imgSource );
			m_pCroppedTextureImagePanel->SetVisible( true );
		}
	}

	// data
	bool m_bSelected;
	bool m_bPreviewDownloadPending;
	Bitmap_t m_imgSource; // original resolution and aspect
	CBitmapPanel *m_pCroppedTextureImagePanel;
	vgui::EditablePanel *m_pHighlightPanel;
	CUGCFileRequest	*m_pUGCPreviewFileRequest;
	SteamUGCDetails_t m_FileDetails;
	CUGCFileRequestCache &m_previewFileCache;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define MAX_ITEMS_VIEWABLE 4

class CSteamWorkshopDialog : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CSteamWorkshopDialog, vgui::EditablePanel );
public:
	CSteamWorkshopDialog( vgui::Panel *parent ) 
		: vgui::EditablePanel( parent, "SteamWorkshopDialog" )
		, m_lastPublishedFilesState( CPublishedFiles::kState_Initialized )
	{
		vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme" );
		SetScheme(scheme);
		SetProportional( true );

		m_pContainer = new vgui::EditablePanel( this, "Container" );
		m_pItemsContainer = new vgui::EditablePanel( m_pContainer, "ItemsContainer" );

		vgui::ivgui()->AddTickSignal( GetVPanel(), 0 );

		for ( uint32 i = 0; i < MAX_ITEMS_VIEWABLE; ++i )
		{
			m_items[i] = new CSteamWorkshopItemPanel( m_pItemsContainer, VarArgs("SteamWorkshopItem%d", i ), m_previewFileCache );
			m_items[i]->AddActionSignalTarget( this );
		}
	}

	virtual ~CSteamWorkshopDialog()
	{
	}

	virtual void OnTick( void )
	{
		BaseClass::OnTick();

		if ( m_lastPublishedFilesState != m_publishedFiles.m_state )
		{
			CloseWaitingDialog();
			switch ( m_publishedFiles.m_state )
			{
			case CPublishedFiles::kState_PopulatingFileList:
				ShowWaitingDialog( new CGenericWaitingDialog( this ), "#TF_SteamWorkshop_PopulatingList", true, false, 30.0f );
				break;
			case CPublishedFiles::kState_ErrorOccurred:
				ShowMessageBox( "#TF_SteamWorkshop_Error", "#TF_SteamWorkshop_ErrorText", "#GameUI_OK" );
				break;
			case CPublishedFiles::kState_DeletingFile:
				ShowWaitingDialog( new CGenericWaitingDialog( this ), "#TF_SteamWorkshop_DeletingFile", true, false, 30.0f );
				break;
			case CPublishedFiles::kState_ErrorCannotDeleteFile:
				ShowMessageBox( "#TF_SteamWorkshop_Error", "#TF_SteamWorkshop_CannotDeleteFile", "#GameUI_OK" );
				break;
			case CPublishedFiles::kState_Timeout:
				ShowMessageBox( "#TF_SteamWorkshop_Error", "#TF_SteamWorkshop_Timeout", "#GameUI_OK" );
				break;
			case CPublishedFiles::kState_Done:
				RefreshItemsUI();
				break;
			}
			m_lastPublishedFilesState = m_publishedFiles.m_state;
		}
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );
		
		LoadControlSettings( "Resource/ui/SteamWorkshopDialog.res" );

		SetupButton( "LearnMoreButton" );
		SetupButton( "LearnMore2Button" );
		SetupButton( "BrowseButton" );
		SetupButton( "PublishButton" );
		SetupButton( "ViewPublishedButton" );
		SetupButton( "LoadTestMapButton" );
		SetupButton( "ViewLegalAgreementButton" );

		SetupButton( "PrevPageButton" );
		SetupButton( "PrevPageSkipButton" );
		SetupButton( "SkipToStartButton" );
		SetupButton( "NextPageButton" );
		SetupButton( "NextPageSkipButton" );
		SetupButton( "SkipToEndButton" );
		SetupButton( "ViewButton" );
		SetupButton( "EditButton" );
		SetupButton( "DeleteButton" );

		SetupButton( "CancelButton" );

		SetChildPanelEnabled( this, "LoadTestMapButton", !engine->IsInGame() || !FStrEq( engine->GetLevelName(), "maps/itemtest.bsp" ), true );
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		// Center it, keeping requested size
		int x, y, ww, wt, wide, tall;
		vgui::surface()->GetWorkspaceBounds( x, y, ww, wt );
		GetSize(wide, tall);
		SetPos(x + ((ww - wide) / 2), y + ((wt - tall) / 2));
	}

	virtual void Show()
	{
		TFModalStack()->PushModal( this );

		// Make sure we're signed on
		if ( !CheckSteamSignOn() )
		{
			Close();
			return;
		}

		SetVisible( true );
		MakePopup();
		MoveToFront();
		SetKeyBoardInputEnabled( true );
		SetMouseInputEnabled( true );

		// start download
		m_publishedFiles.PopulateFileList();
	}

	virtual void OnCommand( const char *pCommand )
	{
		if ( FStrEq( pCommand, "cancel" ) )
		{
			Close();
		}
		else if ( FStrEq( pCommand, "publish" ) )
		{
			ShowPublishFileDialog();
		}
		else if ( FStrEq( pCommand, "learn_more" ) )
		{
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "http://www.teamfortress.com/contribute/" );
		}
		else if ( FStrEq( pCommand, "view_files" ) )
		{
			EUniverse universe = GetUniverse();
			uint64 ulSteamID = steamapicontext->SteamUser()->GetSteamID().ConvertToUint64();
			switch ( universe )
			{
			case k_EUniversePublic:
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStrMax( "http://steamcommunity.com/profiles/%llu/mysharedfiles/", ulSteamID ) );
				break;
			case k_EUniverseBeta:
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStrMax( "http://beta.steamcommunity.com/profiles/%llu/mysharedfiles/", ulSteamID ) );
				break;
			case k_EUniverseDev:
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStrMax( COMMUNITY_DEV_HOST "profiles/%llu/mysharedfiles/", ulSteamID ) );
				break;
			}
		}
		else if ( FStrEq( pCommand, "view_legal_agreement" ) )
		{
			EUniverse universe = GetUniverse();
			switch ( universe )
			{
			case k_EUniversePublic:
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStrMax( "http://steamcommunity.com/workshop/workshoplegalagreement/?appid=%d", engine->GetAppID() ) );
				break;
			case k_EUniverseBeta:
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStrMax( "http://beta.steamcommunity.com/workshop/workshoplegalagreement/?appid=%d", engine->GetAppID() ) );
				break;
			case k_EUniverseDev:
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStrMax( COMMUNITY_DEV_HOST "workshop/workshoplegalagreement/?appid=%d", engine->GetAppID() ) );
				break;
			}
		}
		else if ( FStrEq( pCommand, "browse" ) )
		{
			EUniverse universe = GetUniverse();
			switch ( universe )
			{
			case k_EUniversePublic:
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStrMax( "http://steamcommunity.com/workshop/browse?appid=%d", engine->GetAppID() ) );
				break;
			case k_EUniverseBeta:
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStrMax( "http://beta.steamcommunity.com/workshop/browse?appid=%d", engine->GetAppID() ) );
				break;
			case k_EUniverseDev:
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStrMax( COMMUNITY_DEV_HOST "workshop/browse?appid=%d", engine->GetAppID() ) );
				break;
			}
		}
		else if ( FStrEq( pCommand, "itemtest" ) )
		{
			Close();
			engine->ClientCmd_Unrestricted( "disconnect\nwait\nwait\n\nprogress_enable\nmap itemtest\n" );
		}
		else if ( FStrEq( pCommand, "prevpage" ) )
		{
			if ( m_unCurrentPage > 0 )
			{
				--m_unCurrentPage;
			}
			else
			{
				m_unCurrentPage = ceil( (float)m_publishedFiles.m_FileDetails.Count() / (float)MAX_ITEMS_VIEWABLE ) - 1;
			}
			PopulatePublishedFilesUI();
		}
		else if ( FStrEq( pCommand, "prevpageskip" ) )
		{
			for ( int i = 0; i < tf_steam_workshop_page_skip.GetInt(); i++ )
			{
				if ( m_unCurrentPage > 0 )
				{
					--m_unCurrentPage;
				}
				else
				{
					m_unCurrentPage = ceil( (float)m_publishedFiles.m_FileDetails.Count() / (float)MAX_ITEMS_VIEWABLE ) - 1;
				}
			}
			PopulatePublishedFilesUI();
		}
		else if ( FStrEq( pCommand, "skiptostart" ) )
		{
			m_unCurrentPage = 0;
			PopulatePublishedFilesUI();
		}
		else if ( FStrEq( pCommand, "nextpage" ) )
		{
			++m_unCurrentPage;
			if ( m_unCurrentPage > ceil( (float)m_publishedFiles.m_FileDetails.Count() / (float)MAX_ITEMS_VIEWABLE ) - 1 )
			{
				m_unCurrentPage = 0;
			}
			PopulatePublishedFilesUI();
		}
		else if ( FStrEq( pCommand, "nextpageskip" ) )
		{
			uint32 unNumPages = ceil( (float)m_publishedFiles.m_FileDetails.Count() / (float)MAX_ITEMS_VIEWABLE );
			m_unCurrentPage = ( ( m_unCurrentPage + tf_steam_workshop_page_skip.GetInt() ) % unNumPages );
			PopulatePublishedFilesUI();
		}
		else if ( FStrEq( pCommand, "skiptoend" ) )
		{
			m_unCurrentPage = ceil( (float)m_publishedFiles.m_FileDetails.Count() / (float)MAX_ITEMS_VIEWABLE ) - 1;
			PopulatePublishedFilesUI();
		}
		else if ( FStrEq( pCommand, "view" ) )
		{
			for ( uint32 i = 0; i < MAX_ITEMS_VIEWABLE; ++i )
			{
				if ( m_items[i]->GetSelected() )
				{
					ViewPublishedFile( m_items[i]->GetPublishedFileID() );
					break;
				}
			}
		}
		else if ( FStrEq( pCommand, "edit" ) )
		{
			for ( uint32 i = 0; i < MAX_ITEMS_VIEWABLE; ++i )
			{
				if ( m_items[i]->GetSelected() )
				{
					EditPublishedFile( m_items[i]->GetPublishedFileID() );
					break;
				}
			}
		}
		else if ( FStrEq( pCommand, "delete_item" ) )
		{
			for ( uint32 i = 0; i < MAX_ITEMS_VIEWABLE; ++i )
			{
				if ( m_items[i]->GetSelected() )
				{
					DeletePublishedFile( m_items[i]->GetPublishedFileID() );
					break;
				}
			}
		}
		else 
		{
			BaseClass::OnCommand( pCommand );
		}
	}

	virtual void OnKeyCodeTyped( vgui::KeyCode code )
	{
		if( code == KEY_ESCAPE )
		{
			OnCommand( "cancel" );
		}
		else
		{
			BaseClass::OnKeyCodeTyped( code );
		}
	}

	virtual void OnKeyCodePressed( vgui::KeyCode code )
	{
		if( GetBaseButtonCode( code ) == KEY_XBUTTON_B )
		{
			OnCommand( "cancel" );
		}
		else
		{
			BaseClass::OnKeyCodePressed( code );
		}
	}

	MESSAGE_FUNC_PTR( OnItemPanelMousePressed, "ItemPanelMousePressed", panel )
	{
		CSteamWorkshopItemPanel *pItemPanel = static_cast< CSteamWorkshopItemPanel * >( panel );
		for ( uint32 i = 0; i < MAX_ITEMS_VIEWABLE; ++i )
		{
			m_items[i]->SetSelected( m_items[i] == pItemPanel );
			m_items[i]->UpdateBorder();
		}

		// enable per item controls
		SetChildPanelEnabled( m_pItemsContainer, "ViewButton", true );
		SetChildPanelEnabled( m_pItemsContainer, "EditButton", true );
		SetChildPanelEnabled( m_pItemsContainer, "DeleteButton", true );
	}

	MESSAGE_FUNC_UINT64( OnChangedFile, "ChangedFile", nPublishedFileID )
	{
		m_publishedFiles.RefreshPublishedFileDetails( nPublishedFileID );
	}

protected:

	bool CheckSteamSignOn()
	{
		// Make sure we are connected to steam, or they are going to be disappointed
		if ( steamapicontext == NULL
			|| steamapicontext->SteamUtils() == NULL
			|| steamapicontext->SteamMatchmakingServers() == NULL
			|| steamapicontext->SteamUser() == NULL
			|| !steamapicontext->SteamUser()->BLoggedOn()
		) {
			Warning( "Steam not properly initialized or connected.\n" );
			ShowMessageBox( "#TF_MM_GenericFailure_Title", "#TF_MM_GenericFailure", "#GameUI_OK" );
			return false;
		}
		return true;
	}

	void RefreshItemsUI()
	{
		SetChildPanelVisible( m_pContainer, "NoItemsContainer", m_publishedFiles.m_FileDetails.Count() == 0 );
		SetChildPanelVisible( m_pContainer, "ItemsContainer", m_publishedFiles.m_FileDetails.Count() != 0 );
		SetChildPanelVisible( m_pContainer, "LearnMore2Button", m_publishedFiles.m_FileDetails.Count() != 0 );
		SetChildPanelVisible( m_pContainer, "BrowseButton", m_publishedFiles.m_FileDetails.Count() == 0 );

		m_unCurrentPage = 0;
		PopulatePublishedFilesUI();
	}

	void PopulatePublishedFilesUI()
	{
		if ( m_publishedFiles.m_FileDetails.Count() != 0 )
		{
			uint32 unIndex = m_unCurrentPage * MAX_ITEMS_VIEWABLE;
			int nFileIdx = m_publishedFiles.m_FileDetails.FirstInorder();
			for ( uint32 i = 0; i < unIndex && nFileIdx != m_publishedFiles.m_FileDetails.InvalidIndex(); ++i )
			{
				nFileIdx = m_publishedFiles.m_FileDetails.NextInorder( nFileIdx );
			}
			bool bSelected = false;
			for ( uint32 i = 0; i < MAX_ITEMS_VIEWABLE; ++i )
			{
				CSteamWorkshopItemPanel *pItemPanel = m_items[i];
				if ( nFileIdx != m_publishedFiles.m_FileDetails.InvalidIndex() )
				{
					SteamUGCDetails_t &details = m_publishedFiles.m_FileDetails[ nFileIdx ];
					pItemPanel->SetPublishedFileDetails( &details );
					if ( !bSelected )
					{
						pItemPanel->SetSelected( true );
						pItemPanel->UpdateBorder();
						bSelected = true;
					}
					nFileIdx = m_publishedFiles.m_FileDetails.NextInorder( nFileIdx );
				}
				else
				{
					pItemPanel->SetPublishedFileDetails( NULL );
				}
			}

			// paging
			uint32 unNumPages = ceil( (float)m_publishedFiles.m_FileDetails.Count() / (float)MAX_ITEMS_VIEWABLE );
			bool bMultiplePages = ( unNumPages > 1 );
			if ( bMultiplePages )
			{
				m_pItemsContainer->SetDialogVariable( "page", CFmtStr( "%d/%d", m_unCurrentPage + 1, unNumPages ) );
			}
			else
			{
				m_pItemsContainer->SetDialogVariable( "page", "" );
			}
			SetChildPanelVisible( m_pItemsContainer, "PrevPageButton", bMultiplePages );
			SetChildPanelVisible( m_pItemsContainer, "PrevPageSkipButton", bMultiplePages );
			SetChildPanelVisible( m_pItemsContainer, "SkipToStartButton", bMultiplePages );
			SetChildPanelVisible( m_pItemsContainer, "NextPageButton", bMultiplePages );
			SetChildPanelVisible( m_pItemsContainer, "NextPageSkipButton", bMultiplePages );
			SetChildPanelVisible( m_pItemsContainer, "SkipToEndButton", bMultiplePages );

			// other controls
			SetChildPanelEnabled( m_pItemsContainer, "ViewButton", bSelected );
			SetChildPanelEnabled( m_pItemsContainer, "EditButton", bSelected );
			SetChildPanelEnabled( m_pItemsContainer, "DeleteButton", bSelected );
		}
		else
		{
			SetChildPanelEnabled( m_pItemsContainer, "ViewButton", false );
			SetChildPanelEnabled( m_pItemsContainer, "EditButton", false );
			SetChildPanelEnabled( m_pItemsContainer, "DeleteButton", false );
		}
	}

	void ShowPublishFileDialog()
	{
		CTFFilePublishDialog *pPublishDialog = new CTFFilePublishDialog( this, "PublishFileDialog", NULL );
		pPublishDialog->AddActionSignalTarget( this );
		pPublishDialog->SetDeleteSelfOnClose( true );
		pPublishDialog->SetSizeable( false );
		MakeModalAndBringToFront( pPublishDialog );
	}

	void ViewPublishedFile( uint64 nPublishedFileID )
	{
		EUniverse universe = GetUniverse();
		switch ( universe )
		{
		case k_EUniversePublic:
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStrMax( "http://steamcommunity.com/sharedfiles/filedetails/?id=%llu", nPublishedFileID ) );
			break;
		case k_EUniverseBeta:
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStrMax( "http://beta.steamcommunity.com/sharedfiles/filedetails/?id=%llu", nPublishedFileID ) );
			break;
		case k_EUniverseDev:
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStrMax( COMMUNITY_DEV_HOST "sharedfiles/filedetails/?id=%llu", nPublishedFileID ) );
			break;
		}
	}

	void EditPublishedFile( uint64 nPublishedFileID )
	{
		const SteamUGCDetails_t *pDetails = m_publishedFiles.GetPublishedFileDetails( nPublishedFileID );
		if ( pDetails )
		{

			PublishedFileDetails_t fileDetails;
			fileDetails.publishedFileDetails = *pDetails;
			fileDetails.lpszFilename = pDetails->m_pchFileName;

			CTFFilePublishDialog *pPublishDialog = new CTFFilePublishDialog( this, "PublishFileDialog", &fileDetails );
			pPublishDialog->AddActionSignalTarget( this );
			pPublishDialog->SetDeleteSelfOnClose( true );
			pPublishDialog->SetSizeable( false );
			MakeModalAndBringToFront( pPublishDialog );

			if ( fileDetails.publishedFileDetails.m_eFileType == k_EWorkshopFileTypeMicrotransaction &&
			     !Q_strstr( fileDetails.publishedFileDetails.m_rgchTags, kImportedTag ) )
			{
				ShowMessageBox( "#TF_ImportFile_Warning", "#TF_ImportFile_NotCompatible" );
			}
		}
	}

	static void ConfirmDeletePublishedFile( bool bConfirmed, void *pContext )
	{
		if ( bConfirmed )
		{
			CSteamWorkshopDialog *pDialog = static_cast< CSteamWorkshopDialog* >( pContext );
			uint64 nPublishedFileID = 0;
			for ( uint32 i = 0; i < MAX_ITEMS_VIEWABLE; ++i )
			{
				if ( pDialog->m_items[i]->GetSelected() )
				{
					nPublishedFileID = pDialog->m_items[i]->GetPublishedFileID();
					break;
				}
			}
			if ( nPublishedFileID != 0 )
			{
				const SteamUGCDetails_t *pDetails = pDialog->m_publishedFiles.GetPublishedFileDetails( nPublishedFileID );
				if ( pDetails )
				{
					pDialog->m_publishedFiles.DeletePublishedFile( nPublishedFileID );
				}
			}
		}
	}

	void DeletePublishedFile( uint64 nPublishedFileID )
	{
		ShowConfirmDialog( "#TF_SteamWorkshop_DeleteConfirmTitle", "#TF_SteamWorkshop_DeleteConfirmText", "#GameUI_OK", "#GameuI_CancelBold", &ConfirmDeletePublishedFile, this, this );	
	}

	void SetupButton( const char *pPanelName )
	{
		vgui::Panel *pPanel = m_pContainer->FindChildByName( pPanelName, true );
		if ( pPanel )
		{
			pPanel->AddActionSignalTarget( this );
		}
	}

	// called when the Cancel button is pressed
	void Close()
	{
		SetVisible( false );
		TFModalStack()->PopModal( this );
		MarkForDeletion();
	}

	vgui::EditablePanel *m_pContainer;
	vgui::EditablePanel *m_pItemsContainer;
	CPublishedFiles m_publishedFiles;
	uint32 m_lastPublishedFilesState;
	uint32 m_unCurrentPage;
	CSteamWorkshopItemPanel *m_items[MAX_ITEMS_VIEWABLE];
	CUGCFileRequestCache m_previewFileCache;
};
static vgui::DHANDLE<CSteamWorkshopDialog> g_pSteamWorkshopDialog;

//-----------------------------------------------------------------------------
// Purpose: Callback to open the game menus
//-----------------------------------------------------------------------------
static void CL_OpenSteamWorkshopDialog( const CCommand &args )
{
	if ( g_pSteamWorkshopDialog.Get() == NULL )
	{
		IViewPortPanel *pMMOverride = ( gViewPortInterface->FindPanelByName( PANEL_MAINMENUOVERRIDE ) );
		g_pSteamWorkshopDialog = vgui::SETUP_PANEL( new CSteamWorkshopDialog( (CHudMainMenuOverride*)pMMOverride ) );
	}
	engine->ExecuteClientCmd( "gameui_activate" );
	g_pSteamWorkshopDialog->Show();
}

// the console commands
static ConCommand steamworkshopdialog( "OpenSteamWorkshopDialog", &CL_OpenSteamWorkshopDialog, "" );

//-----------------------------------------------------------------------------

class CItemTestHUDPanel : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CItemTestHUDPanel, vgui::EditablePanel );
public:
	CItemTestHUDPanel( const char *pElementName ) 
		: CHudElement( pElementName )
		, BaseClass( NULL, "ItemTestHUDPanel" )
	{
		vgui::Panel *pParent = g_pClientMode->GetViewport();
		SetParent( pParent );

		SetHiddenBits( HIDEHUD_MISCSTATUS );
	}

	virtual ~CItemTestHUDPanel()
	{

	}

	virtual bool ShouldDraw( void )
	{
		if ( !CHudElement::ShouldDraw() )
			return false;

		if ( !engine->IsInGame() )
			return false;

		if ( TFGameRules() && TFGameRules()->IsInItemTestingMode() )
			return true;

		return FStrEq( engine->GetLevelName(), "maps/itemtest.bsp" );
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		if ( m_pBGPanel_Blue && m_pBGPanel_Red && C_TFPlayer::GetLocalTFPlayer() )
		{
			bool bRed = ( C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() == TF_TEAM_RED );
			m_pBGPanel_Blue->SetVisible( !bRed );
			m_pBGPanel_Red->SetVisible( bRed );
		}
	}

	virtual void ApplySchemeSettings( vgui::IScheme *scheme )
	{
		if ( g_pFullFileSystem->FileExists( "resource/UI/ItemTestHUDPanel.res" ) )
		{
			LoadControlSettings( "resource/UI/ItemTestHUDPanel.res" );
		}

		BaseClass::ApplySchemeSettings( scheme );

		m_pBGPanel_Blue = FindChildByName("Background_Blue");
		m_pBGPanel_Red = FindChildByName("Background_Red");

		if ( m_pBGPanel_Blue )
		{
			m_pBGPanel_Blue->SetVisible( true );
		}
	}

	int	HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
	{
		if ( !IsVisible() )
			return 1; // key not handled

		if ( !down )
			return 1; // key not handled

		switch ( keynum )
		{
			case KEY_F7:
			{
				engine->ClientCmd_Unrestricted( "itemtest" );
				return 0;
			}
			break;
			case KEY_F8:
			{				
				engine->ClientCmd_Unrestricted( "itemtest_botcontrols" );
				return 0;
			}
			break;
			case KEY_F9:
			{				
				InvalidateLayout( true, true );
				return 0;
			}
			break;
		}

		return 1; // key not handled
	}

protected:
	vgui::Panel			*m_pBGPanel_Blue;
	vgui::Panel			*m_pBGPanel_Red;
};

DECLARE_HUDELEMENT( CItemTestHUDPanel );

//-----------------------------------------------------------------------------

// @return true if the item test HUD handled the input, false otherwise
bool ItemTestHandlesKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	CItemTestHUDPanel *pItemTestHUDPanel = ( CItemTestHUDPanel * )GET_HUDELEMENT( CItemTestHUDPanel );
	if ( pItemTestHUDPanel )
	{
		return pItemTestHUDPanel->HudElementKeyInput( down, keynum, pszCurrentBinding ) == 0;
	}
	return false;
}

