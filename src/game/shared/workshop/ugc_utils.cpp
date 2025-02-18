//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Utility helper functions for dealing with UGC files
//
//==========================================================================//
#include "cbase.h"

#include "steam/steam_api.h"
#include "ugc_utils.h"
#include "fmtstr.h"

// utime() and stat()
#if defined( _WIN32 )
#include <sys/utime.h>
#elif defined(OSX)
#include <utime.h>
#else
#include <sys/types.h>
#include <utime.h>
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ISteamUGC *GetSteamUGC()
{
#ifdef GAME_DLL
	// Use steamgameserver context if this isn't a client/listenserver.
	// While we can use steamgameserver in listenservers, we want to always use client-side UGC there currently.
	if ( engine->IsDedicatedServer() )
	{
		return steamgameserverapicontext ? steamgameserverapicontext->SteamUGC() : NULL;
	}
#endif
	return steamapicontext ? steamapicontext->SteamUGC() : NULL;
}

ISteamRemoteStorage *GetSteamRemoteStorage()
{
	return steamapicontext ? steamapicontext->SteamRemoteStorage() : NULL;
}

//=============================================================================
//
//  File request helper class for older ISteamRemoteStorage UGC files.
//  Prefer ISteamUGC when possible.
//
//=============================================================================

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CUGCFileRequest::CUGCFileRequest( void ) :
m_hCloudID(k_UGCHandleInvalid),
	m_UGCStatus(UGCFILEREQUEST_READY),
	m_AsyncControl(NULL)
{
	// Start with these disabled
	m_szFileName[0] = '\0';
	m_szTargetDirectory[0] = '\0';
	m_szTargetFilename[0] = '\0';
	m_szErrorText[0] = '\0';

#ifdef FILEREQUEST_IO_STALL
	m_nIOStallType = FILEREQUEST_STALL_DOWNLOAD;
	m_flIOStallDuration = FILEREQUEST_IO_STALL_DELAY;	// seconds
#endif // FILEREQUEST_IO_STALL
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
CUGCFileRequest::~CUGCFileRequest( void )
{
	// Finish the file i/o
	if ( m_AsyncControl != NULL )
	{
		g_pFullFileSystem->AsyncFinish( m_AsyncControl );
		g_pFullFileSystem->AsyncRelease( m_AsyncControl );
		m_AsyncControl = NULL;
	}

	// Clear our internal buffer
	m_bufContents.Clear();
}

//-----------------------------------------------------------------------------
// Purpose: Start a download by handle
//-----------------------------------------------------------------------------

UGCFileRequestStatus_t CUGCFileRequest::StartDownload( UGCHandle_t hFileHandle, const char *lpszTargetDirectory /*= NULL*/, const char *lpszTargetFilename /*= NULL*/ )
{
	// Start with the assumption of failure
	m_UGCStatus = UGCFILEREQUEST_ERROR;

	// Start the download request
	SteamAPICall_t hSteamAPICall = GetSteamRemoteStorage()->UGCDownload( hFileHandle, 0 );
	m_callbackUGCDownload.Set( hSteamAPICall, this, &CUGCFileRequest::Steam_OnUGCDownload );

	if ( hSteamAPICall != k_uAPICallInvalid )
	{
#ifdef LOG_FILEREQUEST_PROGRESS
		Msg( "Started download of cloud file %s/%s (%08X%08X)\n", lpszTargetDirectory, lpszTargetFilename, (uint32)(hFileHandle>>32), (uint32)hFileHandle );
#endif // LOG_FILEREQUEST_PROGRESS

		// Mark download as in progress
		m_UGCStatus = UGCFILEREQUEST_DOWNLOADING;
		m_hCloudID = hFileHandle;

		// Take a target directory for the file
		if ( lpszTargetDirectory != NULL )
		{
			V_strncpy( m_szTargetDirectory, lpszTargetDirectory, MAX_PATH );
		}

		// Take a target filename for the file
		if ( lpszTargetFilename != NULL )
		{
			V_strncpy( m_szTargetFilename,	lpszTargetFilename, MAX_PATH );
		}

#ifdef FILEREQUEST_IO_STALL
		m_flIOStallStart = gpGlobals->curtime;
#endif // FILEREQUEST_IO_STALL

		// Done!
		return m_UGCStatus;
	}

	// We were unable to start our download through the Steam API
	return ThrowError( "Failed to initiate download of file from cloud\n" );
}

//-----------------------------------------------------------------------------
// Purpose: Start an upload of a buffer by filename
//-----------------------------------------------------------------------------

UGCFileRequestStatus_t CUGCFileRequest::StartUpload( CUtlBuffer &buffer, const char *lpszFilename )
{
	// Start with the assumption of failure
	m_UGCStatus = UGCFILEREQUEST_ERROR;

	// Write the local copy of the file
#ifdef LOG_FILEREQUEST_PROGRESS
	Msg( "Saving %s to user cloud...\n", lpszFilename );
#endif // LOG_FILEREQUEST_PROGRESS

	ISteamRemoteStorage *pRemoteStorage = GetSteamRemoteStorage();
	if ( !pRemoteStorage || !pRemoteStorage->FileWrite( lpszFilename, buffer.Base(), buffer.TellPut() ) )
		return ThrowError( "Failed to write file to cloud\n" );

	// Now share the file (uploads it to the cloud)
	SteamAPICall_t hSteamAPICall = pRemoteStorage->FileShare( lpszFilename );
	m_callbackFileShare.Set( hSteamAPICall, this, &CUGCFileRequest::Steam_OnFileShare);

#ifdef FILEREQUEST_IO_STALL
	m_flIOStallStart = gpGlobals->curtime;
#endif // FILEREQUEST_IO_STALL

	m_UGCStatus = UGCFILEREQUEST_UPLOADING;
	return m_UGCStatus;
}

//-----------------------------------------------------------------------------
// Purpose: FileShare complete for a file request
//-----------------------------------------------------------------------------
void CUGCFileRequest::Steam_OnFileShare( RemoteStorageFileShareResult_t *pResult, bool bError )
{
	if ( bError )
	{
		ThrowError( "Upload of file to Steam cloud failed\n" );
		return;
	}

#ifdef LOG_FILEREQUEST_PROGRESS
	Msg( "Custom map uploaded to cloud completed OK, assigned UGC ID %08X%08X\n", (uint32)(pResult->m_hFile >> 32), (uint32)(pResult->m_hFile) );
#endif // LOG_FILEREQUEST_PROGRESS

	// Save the return handle
	m_hCloudID = pResult->m_hFile;

	MarkCompleteAndFree();
}

//-----------------------------------------------------------------------------
// Purpose: UGDownload complete for a file request
//-----------------------------------------------------------------------------
void CUGCFileRequest::Steam_OnUGCDownload( RemoteStorageDownloadUGCResult_t *pResult, bool bError )
{
	// Completed.  Did we succeed?
	if ( bError || pResult->m_eResult != k_EResultOK )
	{
		ThrowError( "Download of file from cloud failed!\n" );
		return;
	}

	// Make sure we got back the file we were expecting
	Assert( pResult->m_hFile == m_hCloudID );

	// Fetch file details
	AppId_t nAppID;
	char *pchName;
	int32 nFileSizeInBytes = -1;
	CSteamID steamIDOwner;
	ISteamRemoteStorage *pRemoteStorage = GetSteamRemoteStorage();

	if ( !pRemoteStorage->GetUGCDetails( m_hCloudID, &nAppID, &pchName, &nFileSizeInBytes, &steamIDOwner ) || nFileSizeInBytes <= 0 )
	{
		ThrowError( "Unable to retrieve cloud file info from Steam\n" );
		return;
	}

	// Allocate a temporary buffer
	m_bufContents.Clear();
	m_bufContents.SeekPut( CUtlBuffer::SEEK_HEAD, nFileSizeInBytes );

	// Read in the data
	if ( pRemoteStorage->UGCRead( m_hCloudID, m_bufContents.Base( ), nFileSizeInBytes, 0, k_EUGCRead_ContinueReadingUntilFinished ) != nFileSizeInBytes )
	{
		ThrowError( "Failed call to UGCRead on cloud file\n" );
		return;
	}

	// Save our name
	V_strncpy( m_szFileName, pchName, sizeof(m_szFileName) );

	// Take this as our target if we haven't specified one
	if ( m_szTargetFilename[0] == '\0' )
	{
		V_strncpy( m_szTargetFilename, pchName, sizeof(m_szTargetFilename) );
	}

#ifdef LOG_FILEREQUEST_PROGRESS
	Msg( "Read file %s/%s (%08X%08X)\n", m_szTargetDirectory, m_szTargetFilename, (uint32)(m_hCloudID>>32), (uint32)m_hCloudID );
#endif // LOG_FILEREQUEST_PROGRESS

	// FIXME: Is this already in scope?
	// Done downloading, so commit it to the local disc
	const char *lpszFilename = V_UnqualifiedFileName( GetFileName() );

	char szLocalFilename[MAX_PATH];

	// Make sure the directory exists if we're creating one
	if ( m_szTargetDirectory[0] != '\0' )
	{
		V_snprintf( szLocalFilename, sizeof(szLocalFilename), "%s/%s", m_szTargetDirectory, lpszFilename );
		g_pFullFileSystem->CreateDirHierarchy( m_szTargetDirectory, "DEFAULT_WRITE_PATH" );
	}
	else
	{
		V_snprintf( szLocalFilename, sizeof(szLocalFilename), "%s", lpszFilename );

		/*
		char szDirectory[MAX_PATH];
		Q_FileBase( GetFileName(), szDirectory, sizeof(szDirectory) );
		g_pFullFileSystem->CreateDirHierarchy( szDirectory, "DEFAULT_WRITE_PATH" );
		*/
	}

	// Async write this to disc with monitoring
	if ( g_pFullFileSystem->AsyncWrite( szLocalFilename, m_bufContents.Base(), m_bufContents.TellPut(), false, false, &m_AsyncControl ) < 0 )
	{
		// Async write failed immediately!
		ThrowError( CFmtStr( "Async write of downloaded file %s failed\n", szLocalFilename ) );
		return;
	}

#ifdef LOG_FILEREQUEST_PROGRESS
	Msg( "Async write started for %s (%08X%08X)\n", szLocalFilename, (uint32)(m_hCloudID>>32), (uint32)m_hCloudID );
#endif // LOG_FILEREQUEST_PROGRESS

	// Mark us as having started out download
	m_UGCStatus = UGCFILEREQUEST_DOWNLOAD_WRITING;
}

//-----------------------------------------------------------------------------
// Purpose: Poll for status and drive the process forward
//-----------------------------------------------------------------------------

UGCFileRequestStatus_t CUGCFileRequest::Update( void )
{
	switch ( m_UGCStatus )
	{
		// Handle the async write of the file to disc
	case UGCFILEREQUEST_DOWNLOAD_WRITING:
		{
#ifdef FILEREQUEST_IO_STALL
			if ( m_nIOStallType == FILEREQUEST_STALL_WRITE )
			{
				if ( ( gpGlobals->curtime - m_flIOStallStart ) < m_flIOStallDuration )
					return UGCFILEREQUEST_DOWNLOAD_WRITING;
			}
#endif // FILEREQUEST_IO_STALL

			// Monitor the async write progress and clean up after we're done
			if ( m_AsyncControl )
			{
				FSAsyncStatus_t status = g_pFullFileSystem->AsyncStatus( m_AsyncControl );
				switch ( status )
				{
				case FSASYNC_STATUS_PENDING:
				case FSASYNC_STATUS_INPROGRESS:
				case FSASYNC_STATUS_UNSERVICED:
					return UGCFILEREQUEST_DOWNLOAD_WRITING;

				case FSASYNC_ERR_FILEOPEN:
					return ThrowError( "Unable to write file to disc!\n" );
				}

				// Finish the read
				g_pFullFileSystem->AsyncFinish( m_AsyncControl );
				g_pFullFileSystem->AsyncRelease( m_AsyncControl );
				m_AsyncControl = NULL;

#ifdef LOG_FILEREQUEST_PROGRESS
				Msg( "Async write completed for %s/%s (%08X%08X)\n", m_szTargetDirectory, m_szTargetFilename, (uint32)(m_hCloudID>>32), (uint32)m_hCloudID );
#endif // LOG_FILEREQUEST_PROGRESS

				MarkCompleteAndFree();
				return m_UGCStatus;
			}

			// Somehow we lost the handle to our async status or got a spurious call in here!
			return ThrowError( "Lost handle to async handle for downloaded file write!" );
		}
		break;

		// Handle starting up a download
	case UGCFILEREQUEST_READY:
	case UGCFILEREQUEST_DOWNLOADING:
	case UGCFILEREQUEST_UPLOADING:
	case UGCFILEREQUEST_FINISHED:
		return m_UGCStatus;
		break;

		// An error has occurred while trying to handle the user's request
	default:
	case UGCFILEREQUEST_ERROR:
		return UGCFILEREQUEST_ERROR;
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the local file name on disk, accounting for target directories and filenames
//-----------------------------------------------------------------------------
void CUGCFileRequest::GetLocalFileName( char *pDest, size_t strSize )
{
	if ( m_szTargetDirectory[0] == '\0' )
	{
		V_strncpy( pDest, GetFileName(), strSize );
	}
	else
	{
		V_snprintf( pDest, strSize, "%s/%s", m_szTargetDirectory, GetFileName() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the local directory on disk, accounting for target directories
//-----------------------------------------------------------------------------
void CUGCFileRequest::GetLocalDirectory( char *pDest, size_t strSize )
{
	if ( m_szTargetDirectory[0] == '\0' )
	{
		V_strncpy( pDest, "\0", strSize );
	}
	else
	{
		V_strncpy( pDest, m_szTargetDirectory, strSize );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the modified/access times of a file, taking care to avoid a win32 CRT bug.
//-----------------------------------------------------------------------------
bool UGC_SetFileTime( const char *pFileRelativePath, RTime32 uTimestamp )
{
	char chFullFilePathForTimestamp[ MAX_PATH ] = {0};
	char const *pchFullPath = g_pFullFileSystem->RelativePathToFullPath( pFileRelativePath,
	                                                                     UGC_PATHID,
	                                                                     chFullFilePathForTimestamp,
	                                                                     sizeof( chFullFilePathForTimestamp ) );
	if ( pchFullPath )
	{
		struct utimbuf tbuffer;
		tbuffer.modtime = tbuffer.actime = uTimestamp;
		int iResultCode = utime( pchFullPath, &tbuffer );

#if defined ( _WIN32 )
		// In MSVC2013 and earlier, utime() incorrectly factors in daylight savings.
		// Prior to MSVC2013 stat() also has this bug.
		// This means for MSVC2013's CRT specifically, stat() stops canceling out the error and returns something
		// different from what utime() sets.
		// Seriously.
		// https://connect.microsoft.com/VisualStudio/feedback/details/811534/utime-sometimes-fails-to-set-the-correct-file-times-in-visual-c-2013

		// Check if what we wrote is being offset, then re-set the time canceling out this offset.
		RTime32 unFileTimeFromStat = (RTime32)g_pFullFileSystem->GetFileTime( pFileRelativePath, "MOD" );
		if ( unFileTimeFromStat != uTimestamp )
		{
			int32 nDLSOffset = unFileTimeFromStat - uTimestamp;
			tbuffer.modtime = tbuffer.actime = uTimestamp - nDLSOffset;
			iResultCode = utime( pchFullPath, &tbuffer );
#if defined ( DEBUG )
			unFileTimeFromStat = (RTime32)g_pFullFileSystem->GetFileTime( pFileRelativePath, "MOD" );
			Assert( unFileTimeFromStat == uTimestamp );
#endif
		}
#endif

		return ( iResultCode == 0 );
	}
	return false;
}
