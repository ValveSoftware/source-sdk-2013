//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Utility helper functions for dealing with UGC files
//
//==========================================================================//

#ifndef UGC_UTILS_H
#define UGC_UTILS_H

#include "utlbuffer.h"
#include "filesystem.h"
#include "steam/steam_api.h"

#include "dbg.h"

// All UGC files are assumed to be on this path by default
#define UGC_PATHID "DEFAULT_WRITE_PATH"

// This will log the UGC file requests as they're serviced
// #define LOG_FILEREQUEST_PROGRESS

// Enable verbose debug spew to DevMsg
// #define UGC_DEBUG

#define UGCMsg(...) Msg("[UGC] " __VA_ARGS__)
#define UGCWarning(...) Warning("[UGC] " __VA_ARGS__)

#ifdef UGC_DEBUG
#define UGCDebug(...) DevMsg("[UGC Debug] " __VA_ARGS__)
#else // UGC_DEBUG
#define UGCDebug(...)
#endif // UGC_DEBUG

ISteamUGC *GetSteamUGC();
ISteamRemoteStorage *GetSteamRemoteStorage();

// Consistently set/get modified/access timestamps for UGC files.
bool    UGC_SetFileTime( const char *pFileRelativePath, RTime32 uTimestamp );

// Simulates stalling of the file IO for testing
// #define FILEREQUEST_IO_STALL
#define FILEREQUEST_IO_STALL_DELAY	1.0f // Seconds

enum UGCFileRequestStatus_t
{
	UGCFILEREQUEST_ERROR = -1,			// An error occurred while processing the file operation
	UGCFILEREQUEST_READY,				// File request is ready to do work
	UGCFILEREQUEST_DOWNLOADING,			// Currently downloading a file
	UGCFILEREQUEST_DOWNLOAD_WRITING,	// Async write of the downloaded file to the disc
	UGCFILEREQUEST_UPLOADING,			// Currently uploading a file
	UGCFILEREQUEST_FINISHED				// Operation complete, no work waiting
};

#ifdef FILEREQUEST_IO_STALL
enum
{
	FILEREQUEST_STALL_NONE,
	FILEREQUEST_STALL_DOWNLOAD,			// Download from UGC server
	FILEREQUEST_STALL_WRITE,			// Write to disc
};
#endif // FILEREQUEST_IO_STALL


//  FIXME(johns): This is superseded by the newer CUGCSyncedFile. Once the
//               remaining users of this are migrated it should be nuked.
class CUGCFileRequest
{
public:
	CUGCFileRequest( void );
	~CUGCFileRequest( void );

	UGCFileRequestStatus_t StartDownload( UGCHandle_t hFileHandle, const char *lpszTargetDirectory = NULL, const char *lpszTargetFilename = NULL );
	UGCFileRequestStatus_t StartUpload( CUtlBuffer &buffer, const char *lpszFilename );
	UGCFileRequestStatus_t Update( void );
	UGCFileRequestStatus_t GetStatus( void ) const { return m_UGCStatus; }

	// Accessors
	const char *GetFileName( void ) { return ( m_szTargetFilename[0] == '\0' ) ? m_szFileName : m_szTargetFilename; }
	const char *GetLastError( void ) const { return m_szErrorText; }
	UGCHandle_t	GetCloudHandle( void ) const { return m_hCloudID; }

	void GetLocalFileName( char *pDest, size_t strSize );
	void GetLocalDirectory( char *pDest, size_t strSize );

private:

	CCallResult<CUGCFileRequest, RemoteStorageDownloadUGCResult_t> m_callbackUGCDownload;
	void Steam_OnUGCDownload( RemoteStorageDownloadUGCResult_t *pResult, bool bError );

	CCallResult<CUGCFileRequest, RemoteStorageFileShareResult_t> m_callbackFileShare;
	void Steam_OnFileShare( RemoteStorageFileShareResult_t *pResult, bool bError );

	//
	// Marks the file request as complete and frees its internal buffers
	//

	void MarkCompleteAndFree( void )
	{
		m_bufContents.Clear();
		m_UGCStatus = UGCFILEREQUEST_FINISHED;
	}

	//
	//  Sets the file request into an error state
	//

	UGCFileRequestStatus_t ThrowError( const char *lpszDesc )
	{
		V_strncpy( m_szErrorText, lpszDesc, ARRAYSIZE(m_szErrorText) );
		Warning( "%s", m_szErrorText );
		Assert(0);
		m_UGCStatus = UGCFILEREQUEST_ERROR;

		return m_UGCStatus;
	}

private:
	char					m_szTargetDirectory[MAX_PATH];	// If specified, the directory the file will be placed in
	char					m_szTargetFilename[MAX_PATH];	// If specified, this name overrides the UGC filename
	char					m_szFileName[MAX_PATH];			// Filename of in the cloud structure

	SteamAPICall_t			m_hSteamAPICall;				// Used to track Steam API calls which are non-blocking
	CUtlBuffer				m_bufContents;					// Contents of the file once read from the cloud
	UGCHandle_t				m_hCloudID;						// Cloud handle of this request
	FSAsyncControl_t		m_AsyncControl;					// Handle for the async requests this class can initiate

	UGCFileRequestStatus_t	m_UGCStatus;					// The current status of this request
	char					m_szErrorText[512];				// Holds information if an error occurred

#ifdef FILEREQUEST_IO_STALL
	// Debug data
	float					m_flIOStallDuration;			// Amount of time (in seconds) to stall all IO operations
	int						m_nIOStallType;					// Type of stall (0 - none, 1 - download, 2 - write )
	float					m_flIOStallStart;
#endif // FILEREQUEST_IO_STALL
};

#endif //UGC_UTILS_H
