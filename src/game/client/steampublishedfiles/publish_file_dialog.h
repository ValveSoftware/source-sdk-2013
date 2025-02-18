//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PUBLISH_FILE_DIALOG_H
#define PUBLISH_FILE_DIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/SectionedListPanel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/BitmapImagePanel.h>
#include <filesystem.h>
#include "vgui/MouseCode.h"
#include "vgui/IScheme.h"
#include "steam/steam_api.h"
#include "utlmap.h"
#include "bitmap/bitmap.h"
#include "workshop/ugc_utils.h"

struct PublishedFileDetails_t
{
	SteamUGCDetails_t  publishedFileDetails;
	const char        *lpszFilename;
};

#define APPID_PORTAL2	852

using namespace vgui;

class CBitmapPanel;

enum FileOpenMode_t {
	FILEOPEN_NONE,
	FILEOPEN_MAIN_FILE,
	FILEOPEN_PREVIEW
};

struct EntityToTagMap_t
{
	const char *lpszEntityName;
	const char *lpszTagName;
	const char *lpszKey;
	const char *lpszValue;
};

enum PublishedFileDetailsField_t
{
	PFILE_FIELD_TITLE			= (1<<0),
	PFILE_FIELD_DESCRIPTION		= (1<<1),
	PFILE_FIELD_FILE			= (1<<2),
	PFILE_FIELD_PREVIEW			= (1<<3),
};

enum eFilterType_t
{
	IMPORT_FILTER_NONE = 0,
	IMPORT_FILTER_COSMETIC,
	IMPORT_FILTER_OTHER,
	IMPORT_FILTER_MAP,
};

// TODO: Move to P2 version
#define TAG_GAME_MODE_SINGLEPLAYER	"Singleplayer"
#define TAG_GAME_MODE_COOP			"Co-Op"
//

// This is the size for the preview for P2 map preview images
#define PREVIEW_WIDTH	225
#define PREVIEW_HEIGHT	152

//-----------------------------------------------------------------------------
// Purpose: Main dialog for media browser
//-----------------------------------------------------------------------------
class CPrepareFileThread;
class CFilePublishDialog : public Frame
{
	DECLARE_CLASS_SIMPLE( CFilePublishDialog, Frame );

public:

	enum ErrorCode_t
	{
		kNoError,
		kFailedToPublishFile,
		kFailedToUpdateFile,
		kFailedToPrepareFile,
		kSteamCloudNotAvailable,
		kSteamExceededCloudQuota,
		kFailedToWriteToSteamCloud,
		kFileNotFound,
		kNeedTitleAndDescription,
		kFailedFileValidation,
		kFailedFileTooLarge,
		kFailedFileNotFound,
		kFailedUserModifiedFile,
		kInvalidMapName
	};

	enum StatusCode_t
	{
		kPublishing,
		kUpdating,
	};

	CFilePublishDialog( Panel *parent, const char *name, PublishedFileDetails_t *pDetails );
	virtual ~CFilePublishDialog();

	virtual void PerformLayout();

	virtual void SetFile( const char *lpszFilename, bool bImported = false );

protected:
	virtual void OnCommand( const char *command );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnTick( void );

	// Override these functions to publish files for different app types
	virtual ErrorCode_t ValidateFile( const char *lpszFilename );
	virtual AppId_t	GetTargetAppID( void ) { return APPID_PORTAL2; }
	virtual unsigned int DesiredPreviewHeight( void ) { return PREVIEW_HEIGHT; }
	virtual unsigned int DesiredPreviewWidth( void ) { return PREVIEW_WIDTH; }
	// Decides which type to use for publishing this file
	virtual EWorkshopFileType WorkshipFileTypeForFile( const char *pszFileName ) { return k_EWorkshopFileTypeCommunity; }
	virtual bool BForceSquarePreviewImage( void ) { return false; } // Force preview images to be square
	virtual const char *GetPreviewFileTypes( void ) { return "*.jpg"; }
	virtual const char *GetPreviewFileTypeDescriptions( void ) { return "JPEG Files (*.jpg)"; }
	virtual const char *GetFileTypes( eFilterType_t eType = IMPORT_FILTER_NONE ) { return "*.bsp"; }
	virtual const char *GetFileTypeDescriptions( eFilterType_t eType = IMPORT_FILTER_NONE ) { return "BSP Files (*.bsp)"; }
	virtual const char *GetResFile() const { return "PublishFileDialog.res"; }
	virtual void ErrorMessage( ErrorCode_t errorCode, KeyValues *pkvTokens = NULL );
	virtual void PopulateTags( SteamParamStringArray_t &strArray ) {}
	virtual const char* GetStatusString( StatusCode_t statusCode );
	virtual void ShowStatusWindow( StatusCode_t statusCode );
	virtual void HideStatusWindow( void );

	virtual void PopulateEditFields( void );

	virtual void OnFilePrepared( ErrorCode_t eResult );

	void DownloadPreviewImage( void );

	void SetPublishButtonState( void );

	bool UpdateFile( void );
	bool UpdateFileInternal( void );
	void GetPreviewFilename( char *szOut, size_t outLen );
	void GetPreparedFilename( char *szOut, size_t outLen );

	bool PublishFile();
	void SetPreviewImage( const char *lpszFilename );

	void PerformSquarize();
	Bitmap_t &GetPreviewBitmap();

	void ErrorMessage( const char *lpszText );

	void StartPrepareFile();

	inline bool IsSourceImageSquare() const
	{
		// We must know the size
		Assert( m_imgSource.IsValid() );
		return
			m_imgSource.Width()*99 < m_imgSource.Height()*100
			&& m_imgSource.Height()*99 < m_imgSource.Width()*100;
	}

	FileOpenMode_t	m_fileOpenMode;

	bool m_bValidFile;
	bool m_bValidJpeg;

	CBitmapPanel			*m_pCroppedTextureImagePanel;

	Bitmap_t m_imgSource; // original resolution and aspect
	Bitmap_t m_imgSquare;
	Bitmap_t m_imgTemp;

	PublishedFileId_t      m_nFileID;
	PublishedFileDetails_t m_FileDetails;
	unsigned int           m_nFileDetailsChanges;

	vgui::MessageBox *m_pStatusBox;
	vgui::TextEntry  *m_pFileTitle;
	vgui::TextEntry  *m_pFileDescription;
	vgui::Label      *m_pFilename;
	vgui::Button     *m_pPublishButton;

	CCallResult<CFilePublishDialog, CreateItemResult_t> m_callbackCreateItem;
	void Steam_OnCreateItem( CreateItemResult_t *pResult, bool bError );

	CCallResult<CFilePublishDialog, SubmitItemUpdateResult_t> m_callbackPublishFile;
	void Steam_OnPublishFile( SubmitItemUpdateResult_t *pResult, bool bError );

	// TODO Switch to using the auto-downloaded ISteamUGC previews
	CUGCFileRequest m_UGCPreviewFileRequest;
	bool            m_bPreviewDownloadPending;
	bool            m_bAddingNewFile;

	CPrepareFileThread *m_pPrepareFileThread;

	MESSAGE_FUNC_CHARPTR( OnFileSelected, "FileSelected", fullpath );
};


extern CFilePublishDialog *g_pSteamFilePublishDialog;


#endif // PUBLISH_FILE_DIALOG_H
