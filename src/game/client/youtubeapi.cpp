//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#if defined( WIN32 ) && !defined( _X360 )
#include "winlite.h"
#include <WinInet.h>
#endif

#include "youtubeapi.h"

#include "platform.h"
#include "convar.h"
#include "fmtstr.h"
#include "igamesystem.h"
#include "strtools.h"
#include "cdll_util.h"
#include "utlmap.h"
#include "utlstring.h"
#include "vstdlib/jobthread.h"
#include <steam/steam_api.h>
#include <steam/isteamhttp.h>
#include "cdll_client_int.h"
#include "utlbuffer.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//=============================================================================

static ConVar youtube_http_proxy( "youtube_http_proxy", "", FCVAR_ARCHIVE, "HTTP proxy.  Specify if you have have problems uploading to YouTube." );

//=============================================================================

static bool GetStandardTagValue( const char *pTag, const char *pXML, CUtlString &strResult )
{
	const char *pStart = strstr( pXML, pTag );
	if ( pStart != NULL )
	{
		pStart = strstr( pStart, ">" );
		if ( pStart != NULL )
		{
			pStart += 1;
			const char *pEnd = strstr( pStart, CFmtStr1024( "</%s", pTag ) );
			if ( pEnd != NULL )
			{
				strResult.SetDirect( pStart, pEnd - pStart );
				return true;
			}
		}
	}
	return false;
}

//=============================================================================

class CYouTubeRetrieveInfoJob;

namespace
{
	class CYouTubeSystem : public CAutoGameSystemPerFrame
	{
	public:

		CYouTubeSystem();
		~CYouTubeSystem();

		// CAutoGameSystem
		virtual bool Init();
		virtual void PostInit();
		virtual void Shutdown();
		virtual bool IsPerFrame() { return true; }
		virtual void Update( float frametime );

		void Login( const char *pUserName, const char *pPassword, const char *pSource );
		void LoginCancel();
		YouTubeUploadHandle_t Upload( const char* pFilePath, const char *pMimeType, const char *pTitle, const char *pDescription, const char *pCategory, const char *pKeywords, eYouTubeAccessControl access );

		bool IsUploadFinished( YouTubeUploadHandle_t handle );
		bool GetUploadProgress( YouTubeUploadHandle_t handle, double &ultotal, double &ulnow );
		bool GetUploadResults( YouTubeUploadHandle_t handle, bool &bSuccess, CUtlString &strURLToVideo, CUtlString &strURLToVideoStats );
		void ClearUploadResults( YouTubeUploadHandle_t handle );
		void CancelUpload( YouTubeUploadHandle_t handle );
		void SetUploadFinished( YouTubeUploadHandle_t handle, bool bSuccess, const char *pURLToVideo, const char *pURLToVideoStats );

		void SetUserProfile( const char *pUserProfile );
		bool GetProfileURL( CUtlString &strProfileURL ) const;

		YouTubeInfoHandle_t GetInfo( const char *pURLToVideoStats, CYouTubeResponseHandler &responseHandler );
		void CancelGetInfo( YouTubeInfoHandle_t handle );

		const char *GetDeveloperKey() const;
		const char *GetDeveloperTag() const;
		void SetDeveloperSettings( const char *pDeveloperKey, const char *pDeveloperTag );

		const char *GetLoginName() const;
		eYouTubeLoginStatus GetLoginStatus() const;
		void SetLoginStatus( eYouTubeLoginStatus status );

		const char* GetAuthToken() const;
		void SetAuthToken( const char *pAuthToken );

	private:
		struct uploadstatus_t
		{
			bool bFinished;
			bool bSuccess;
			CUtlString strURLToVideo;
			CUtlString strURLToVideoStats;
		};

		uploadstatus_t *GetStatus( YouTubeUploadHandle_t handle );

		eYouTubeLoginStatus m_eLoginStatus;
		CUtlString m_strYouTubeUserName;
		CUtlString m_strDeveloperKey;
		CUtlString m_strDeveloperTag;
		CUtlString m_strAuthToken;
		CUtlString m_strUserProfile;
		CThreadMutex m_Mutex;
		IThreadPool* m_pThreadPool;
		CUtlMap< YouTubeUploadHandle_t, uploadstatus_t > m_mapUploads;
		CUtlVector< CYouTubeRetrieveInfoJob * > m_vecRetrieveInfoJobs;
	};
};

static CYouTubeSystem gYouTube;

static ISteamHTTP *GetISteamHTTP()
{
	if ( steamapicontext != NULL && steamapicontext->SteamHTTP() )
	{
		return steamapicontext->SteamHTTP();
	}
#ifndef CLIENT_DLL
	if ( steamgameserverapicontext != NULL )
	{
		return steamgameserverapicontext->SteamHTTP();
	}
#endif
	return NULL;
}

//=============================================================================

// Base class for all YouTube jobs
class CYouTubeJob : public CJob
{
public:
	CYouTubeJob( CYouTubeSystem *pSystem )
	{
		SetFlags( JF_IO );
		// store local ones so we don't have to go through a mutex
		m_strDeveloperKey = pSystem->GetDeveloperKey();
		m_strDeveloperTag = pSystem->GetDeveloperTag();
	}

	virtual ~CYouTubeJob()
	{
	}

	void CancelUpload()
	{
		m_bCancelled = true;
	}

	bool IsCancelled() const
	{
		return m_bCancelled;
	}


protected:
	void OnHTTPRequestCompleted( HTTPRequestCompleted_t *pParam, bool bIOFailure )
	{
		m_bHTTPRequestPending = false;
		if ( (!m_bAllowRequestFailure && pParam->m_eStatusCode != k_EHTTPStatusCode200OK) || bIOFailure || !pParam->m_bRequestSuccessful )
		{
			Warning( "Failed to get youtube url: HTTP status %d fetching %s\n", pParam->m_eStatusCode, m_sURL.String() );
		}
		else
		{
			OnHTTPRequestCompleted( pParam );
		}
	}

	virtual void OnHTTPRequestCompleted( HTTPRequestCompleted_t *pParam ) { Assert( false ); }

	void DoRequest( HTTPRequestHandle hRequest, const char *pchRequestURL )
	{
		m_sURL = pchRequestURL;

		SteamAPICall_t hSteamAPICall;
		if ( GetISteamHTTP()->SendHTTPRequest( hRequest, &hSteamAPICall ) )
		{
			m_HTTPRequestCompleted.Set( hSteamAPICall, this, &CYouTubeJob::OnHTTPRequestCompleted );
		}

		// Wait for it to finish.
		while ( m_bHTTPRequestPending && !m_bCancelled )
		{
			ThreadSleep( 100 );
		}

		GetISteamHTTP()->ReleaseHTTPRequest( hSteamAPICall );
	}


	CCallResult<CYouTubeJob, HTTPRequestCompleted_t> m_HTTPRequestCompleted;

	CUtlString m_strDeveloperKey;
	CUtlString m_strDeveloperTag;
	CUtlString m_sURL;
	CUtlString m_strResponse;
	bool m_bHTTPRequestPending = true;
	bool m_bAllowRequestFailure = false;
	bool m_bCancelled = false;
};

//=============================================================================

class CYouTubeRetrieveUserProfile : public CYouTubeJob
{
public:
	CYouTubeRetrieveUserProfile( CYouTubeSystem *pSystem ) 
		: CYouTubeJob( pSystem )
	{
	}

private:
	void OnHTTPRequestCompleted( HTTPRequestCompleted_t *pParam ) OVERRIDE
	{
		uint32 unBodySize;
		if ( !GetISteamHTTP()->GetHTTPResponseBodySize( pParam->m_hRequest, &unBodySize ) )
		{
			Assert( false );
		}
		else
		{
			m_strResponse.SetLength( unBodySize );

			if ( GetISteamHTTP()->GetHTTPResponseBodyData( pParam->m_hRequest, (uint8*)m_strResponse.String(), unBodySize ) )
			{
				gYouTube.SetUserProfile( m_strResponse.Get() );
			}
		}
	}

	virtual JobStatus_t	DoExecute()
	{
		HTTPRequestHandle hRequest = GetISteamHTTP()->CreateHTTPRequest( k_EHTTPMethodGET, "http://gdata.youtube.com/feeds/api/users/default" );
		GetISteamHTTP()->SetHTTPRequestNetworkActivityTimeout( hRequest, 30 );
		GetISteamHTTP()->SetHTTPRequestHeaderValue( hRequest, "Authorization", CFmtStr1024( "GoogleLogin auth=%s", gYouTube.GetAuthToken() ) );

		DoRequest( hRequest, "http://gdata.youtube.com/feeds/api/users/default" );

		return JOB_OK;
	}
};

//=============================================================================

class CYouTubeRetrieveInfoJob : public CYouTubeJob
{
public:
	CYouTubeRetrieveInfoJob( CYouTubeSystem *pSystem, const char *pVideoURL, CYouTubeResponseHandler &responseHandler ) 
		: CYouTubeJob( pSystem )
		, m_strURL( pVideoURL )
		, m_responseHandler( responseHandler )
	{
	}

	void NotifyResponseHandler()
	{
		m_responseHandler.HandleResponse( 200, m_strResponse.Get() );
	}

private:
	void OnHTTPRequestCompleted( HTTPRequestCompleted_t *pParam ) OVERRIDE
	{
		uint32 unBodySize;
		if ( !GetISteamHTTP()->GetHTTPResponseBodySize( pParam->m_hRequest, &unBodySize ) )
		{
			Assert( false );
		}
		else
		{
			m_strResponse.SetLength( unBodySize );
			GetISteamHTTP()->GetHTTPResponseBodyData( pParam->m_hRequest, (uint8*)m_strResponse.String(), unBodySize );
		}
	}

	virtual JobStatus_t	DoExecute()
	{
		HTTPRequestHandle hRequest = GetISteamHTTP()->CreateHTTPRequest( k_EHTTPMethodGET, m_strURL.Get() );
		GetISteamHTTP()->SetHTTPRequestNetworkActivityTimeout( hRequest, 30 );

		DoRequest( hRequest, m_strURL.Get() );

		return JOB_OK;
	}

	// data
	CUtlString m_strURL;
	CYouTubeResponseHandler &m_responseHandler;
};

class CYouTubeLoginJob : public CYouTubeJob
{
public:
	CYouTubeLoginJob( CYouTubeSystem *pSystem, const char *pUserName, const char *pPassword, const char *pSource ) 
		: CYouTubeJob( pSystem )
		, m_strUserName( pUserName )
		, m_strPassword( pPassword )
		, m_strSource( pSource )
	{
	}

private:
	
	void SetLoginResults( const char *pLoginResults )
	{
		const char *pStart = strstr( pLoginResults, "Auth=" );
		if ( pStart != NULL )
		{
			pStart += strlen( "Auth=" );			
			const char *pEnd = strstr( pStart, "\r\n" );
			if ( pEnd == NULL )
			{
				pEnd = strstr( pStart, "\n" );
			}
			CUtlString strAuthToken;
			if ( pEnd != NULL )
			{
				strAuthToken.SetDirect( pStart, pEnd - pStart );
			}
			else
			{
				strAuthToken.SetDirect( pStart, strlen( pStart ) );
			}	
			gYouTube.SetAuthToken( strAuthToken.Get() );
		}
	}

	void OnHTTPRequestCompleted( HTTPRequestCompleted_t *pParam ) OVERRIDE
	{
		eYouTubeLoginStatus loginStatus = kYouTubeLogin_LoggedIn;

		if ( pParam->m_eStatusCode == 403 )
		{
			loginStatus = kYouTubeLogin_Forbidden;
		}
		else if ( pParam->m_eStatusCode != 200 )
		{
			loginStatus = kYouTubeLogin_GenericFailure;
		}
		else
		{
			uint32 unBodySize;
			if ( !GetISteamHTTP()->GetHTTPResponseBodySize( pParam->m_hRequest, &unBodySize ) )
			{
				Assert( false );
			}
			else
			{
				m_strResponse.SetLength( unBodySize );
				if ( GetISteamHTTP()->GetHTTPResponseBodyData( pParam->m_hRequest, (uint8*)m_strResponse.String(), unBodySize ) )
				{
					SetLoginResults( m_strResponse );
				}
			}
		}

		gYouTube.SetLoginStatus( loginStatus );

	}

	virtual JobStatus_t	DoExecute()
	{
		m_bAllowRequestFailure = true;

		HTTPRequestHandle hRequest = GetISteamHTTP()->CreateHTTPRequest( k_EHTTPMethodPOST, "https://www.google.com/accounts/ClientLogin" );
		GetISteamHTTP()->SetHTTPRequestNetworkActivityTimeout( hRequest, 30 );

	//	GetISteamHTTP()->SetHTTPRequestHeaderValue( hRequest, "Content-Type", "application/x-www-form-urlencoded" );
		GetISteamHTTP()->SetHTTPRequestHeaderValue( hRequest, "X-GData-Key", CFmtStr1024( "key=%s", m_strDeveloperKey.Get() ) );

		char szUserNameEncoded[256];
		char szPasswordEncoded[256];
		char szSourceEncoded[256];
		Q_URLEncode( szUserNameEncoded, sizeof( szUserNameEncoded ), m_strUserName.Get(), m_strUserName.Length() );
		Q_URLEncode( szPasswordEncoded, sizeof( szPasswordEncoded ), m_strPassword.Get(), m_strPassword.Length() );
		Q_URLEncode( szSourceEncoded, sizeof( szSourceEncoded ), m_strSource.Get(), m_strSource.Length() );

		CFmtStr1024 data( "Email=%s&Passwd=%s&service=youtube&source=%s", szUserNameEncoded, szPasswordEncoded, szSourceEncoded );

		GetISteamHTTP()->SetHTTPRequestRawPostBody( hRequest, "application/x-www-form-urlencoded", (uint8 *)data.Access(), data.Length() );

		DoRequest( hRequest, "https://www.google.com/accounts/ClientLogin" );

		return JOB_OK;
	}

	// data
	CUtlString m_strUserName;
	CUtlString m_strPassword;
	CUtlString m_strSource;
};

// Job for uploading a file
class CYouTubeUploadJob : public CYouTubeJob
{
public:
	CYouTubeUploadJob( CYouTubeSystem *pSystem, const char* pFilePath, const char *pMimeType, const char *pTitle, const char *pDescription, const char *pCategory, const char *pKeywords, eYouTubeAccessControl access )
		: CYouTubeJob( pSystem )
		, m_strFilePath( pFilePath )
		, m_strMimeType( pMimeType )
		, m_strTitle( pTitle )
		, m_strDesc( pDescription )
		, m_strCategory( pCategory )
		, m_strKeywords( pKeywords )
		, m_eAccess( access )
	{
	}

	void GetProgress( double &ultotal, double &ulnow  )
	{
		ultotal = 0;
		ulnow = 0;
	}

private:

	virtual JobStatus_t	DoExecute()
	{
		m_bAllowRequestFailure = true;

		HTTPRequestHandle hRequest = GetISteamHTTP()->CreateHTTPRequest( k_EHTTPMethodPUT, "http://uploads.gdata.youtube.com/feeds/api/users/default/uploads" );
		GetISteamHTTP()->SetHTTPRequestNetworkActivityTimeout( hRequest, 30 );

		const char *pFileName = Q_UnqualifiedFileName( m_strFilePath.Get() );

		GetISteamHTTP()->SetHTTPRequestHeaderValue( hRequest, "Authorization", CFmtStr1024( "GoogleLogin auth=%s", gYouTube.GetAuthToken() ) );
		GetISteamHTTP()->SetHTTPRequestHeaderValue( hRequest, "X-GData-Key", CFmtStr1024( "key=%s", m_strDeveloperKey.Get() ) );
		GetISteamHTTP()->SetHTTPRequestHeaderValue( hRequest, "GData-Version", "2" );
		//GetISteamHTTP()->SetHTTPRequestHeaderValue( hRequest, "Content-Type", "multipart/form-data;boundary=-x" );
		GetISteamHTTP()->SetHTTPRequestHeaderValue( hRequest, "Slug", CFmtStr1024( "%s", pFileName ) );

		const char *pPrivateString = "";
		const char *pAccessControlString = "";

		switch ( m_eAccess )
		{
		case kYouTubeAccessControl_Public:
			break;
		case kYouTubeAccessControl_Private:
			pPrivateString = "<yt:private/>";
			break;
		case kYouTubeAccessControl_Unlisted:
			pAccessControlString = "<yt:accessControl action=\"list\" permission=\"denied\"/>";
			break;
		}

		CFmtStr1024 strAPIRequest( "<?xml version=\"1.0\"?>"
			"<entry "
			"xmlns=\"http://www.w3.org/2005/Atom\" "
			"xmlns:media=\"http://search.yahoo.com/mrss/\" "
			"xmlns:yt=\"http://gdata.youtube.com/schemas/2007\">"
			"<media:group>"
			"<media:title type=\"plain\">%s</media:title>"
			"<media:description type=\"plain\">%s</media:description>"
			"<media:category scheme=\"http://gdata.youtube.com/schemas/2007/categories.cat\">"
			"%s"
			"</media:category>"
			"%s"
			"<media:keywords>%s</media:keywords>"
			"<media:category scheme=\"http://gdata.youtube.com/schemas/2007/developertags.cat\">"
			"%s"
			"</media:category>"
			"</media:group>"
			"%s"
			"</entry>",
			m_strTitle.Get(),
			m_strDesc.Get(),
			m_strCategory.Get(),
			pPrivateString,
			m_strKeywords.Get(),
			m_strDeveloperTag.Get(),
			pAccessControlString );

		CFmtStr1024 fmtstrBody(
			"\r\nContent-Type: application/atom+xml; charset=UTF-8\r\n"
			"\r\n---x\r\nContent-Disposition: form-data; name=\"apirequest\"\r\n\r\n%s"
			"\r\n---x\r\nContent-Disposition: form-data; name=\"video\"\r\nContent-Type: %s\r\nContent-Transfer-Encoding: binary\r\n\r\n",
			strAPIRequest.Access(), m_strMimeType.Get()
		);

		CUtlBuffer postDataRaw( 0, 0, 0 );

		postDataRaw.Put( fmtstrBody.Access(), fmtstrBody.Length() );

		CUtlBuffer fileData( 0, 0, 0 );
		bool bReadFileOK = g_pFullFileSystem->ReadFile( m_strFilePath, nullptr, fileData );

		if ( bReadFileOK )
		{
			postDataRaw.Put( fileData.Base(), fileData.TellPut() );

			fileData.Clear();

			static const char rgchFooter[] = "\r\n---x--\r\n";

			postDataRaw.Put( rgchFooter, V_strlen( rgchFooter ) );

			GetISteamHTTP()->SetHTTPRequestRawPostBody( hRequest, "multipart/form-data;boundary=-x", (uint8 *)postDataRaw.Base(), postDataRaw.TellPut() );

			// BUGBUG: use SendHTTPRequestAndStreamResponse
			DoRequest( hRequest, "http://uploads.gdata.youtube.com/feeds/api/users/default/uploads" );
		}	

		return JOB_OK;
	}

	void OnHTTPRequestCompleted( HTTPRequestCompleted_t *pParam ) OVERRIDE
	{
		bool bSuccess = false;

		CUtlString strURLToVideoStats;
		CUtlString strURLToVideo;

		if ( pParam->m_eStatusCode == 200 )
		{
			bSuccess = true;

			uint32 unBodySize;
			if ( !GetISteamHTTP()->GetHTTPResponseBodySize( pParam->m_hRequest, &unBodySize ) )
			{
				Assert( false );
			}
			else
			{
				m_strResponse.SetLength( unBodySize );
				if ( GetISteamHTTP()->GetHTTPResponseBodyData( pParam->m_hRequest, (uint8*)m_strResponse.String(), unBodySize ) )
				{
					// @note Tom Bui: wish I had an xml parser...
					{
						strURLToVideo = "";
						// "<link rel='alternate' type='text/html' href='http://www.youtube.com/watch?v=7D4pb3irM_0&feature=youtube_gdata' /> ";
						const char *kLinkStartTag = "<link rel='alternate' type='text/html' href='";
						const char *kLinkTagEnd = "'/>";
						const char *pStart = strstr( m_strResponse.Get(), kLinkStartTag );
						if ( pStart != NULL )
						{
							pStart += strlen( kLinkStartTag );
							const char *pEnd = strstr( pStart, kLinkTagEnd );
							if ( pEnd != NULL )
							{
								strURLToVideo.SetDirect( pStart, pEnd - pStart );
							}
						}
					}
					{
						strURLToVideoStats = "";
						// "<link rel='self' type='text/html' href='http://www.youtube.com/watch?v=7D4pb3irM_0&feature=youtube_gdata' /> ";				
						const char *kLinkStartTag = "<link rel='self' type='application/atom+xml' href='";
						const char *kLinkTagEnd = "'/>";
						const char *pStart = strstr( m_strResponse.Get(), kLinkStartTag );
						if ( pStart != NULL )
						{
							pStart += strlen( kLinkStartTag );
							const char *pEnd = strstr( pStart, kLinkTagEnd );
							if ( pEnd != NULL )
							{
								strURLToVideoStats.SetDirect( pStart, pEnd - pStart );
								// @note Tom Bui: we want at least version 2
								if ( V_strstr( strURLToVideoStats.Get(), "?v=" ) == NULL )
								{
									strURLToVideoStats += "?v=2";
								}
							}
						}
					}
				}
			}
		}

		gYouTube.SetUploadFinished( (YouTubeUploadHandle_t)this, bSuccess, strURLToVideo.Get(), strURLToVideoStats.Get() );
	}

	// data 
	CUtlString m_strFilePath;
	CUtlString m_strMimeType;
	CUtlString m_strTitle;
	CUtlString m_strDesc;
	CUtlString m_strCategory;
	CUtlString m_strKeywords;
	eYouTubeAccessControl m_eAccess;
};

//=============================================================================

CYouTubeSystem::CYouTubeSystem()
	: m_eLoginStatus( kYouTubeLogin_NotLoggedIn )
	, m_pThreadPool( NULL )
	, m_mapUploads( DefLessFunc( YouTubeUploadHandle_t ) )
{
}

CYouTubeSystem::~CYouTubeSystem()
{
}

bool CYouTubeSystem::Init()
{
	m_pThreadPool = CreateThreadPool();
	m_pThreadPool->Start( ThreadPoolStartParams_t( false, 4 ), "YouTubeSystem" );
	return true;
}

void CYouTubeSystem::PostInit()
{
}

void CYouTubeSystem::Shutdown()
{
	DestroyThreadPool( m_pThreadPool );
	m_pThreadPool = NULL;
}

void CYouTubeSystem::Update( float frametime )
{
	AUTO_LOCK( m_Mutex );
	FOR_EACH_VEC( m_vecRetrieveInfoJobs, i )
	{
		CYouTubeRetrieveInfoJob *pJob = m_vecRetrieveInfoJobs[i];
		if ( pJob->IsFinished() )
		{
			pJob->NotifyResponseHandler();
			// cleanup the job
			pJob->Release();
			// and remove
			m_vecRetrieveInfoJobs.FastRemove( i );
		}
		else
		{
			++i;
		}
	}
}

void CYouTubeSystem::Login( const char *pUserName, const char *pPassword, const char *pSource )
{
	m_eLoginStatus = kYouTubeLogin_NotLoggedIn;
	CYouTubeLoginJob *pJob = new CYouTubeLoginJob( this, pUserName, pPassword, pSource );
	m_pThreadPool->AddJob( pJob );
	pJob->Release();
}

void CYouTubeSystem::LoginCancel()
{
	m_eLoginStatus = kYouTubeLogin_Cancelled;
}

YouTubeUploadHandle_t CYouTubeSystem::Upload( const char* pFilePath, const char *pMimeType, const char *pTitle, const char *pDescription, const char *pCategory, const char *pKeywords, eYouTubeAccessControl access )
{
	if ( m_eLoginStatus != kYouTubeLogin_LoggedIn )
	{
		return NULL;
	}
	CYouTubeUploadJob *pJob = new CYouTubeUploadJob( this, pFilePath, pMimeType, pTitle, pDescription, pCategory, pKeywords, access );
	m_pThreadPool->AddJob( pJob );
	uploadstatus_t status = { false, false, "", "" };
	m_mapUploads.Insert( (YouTubeUploadHandle_t)pJob, status );
	return (YouTubeUploadHandle_t)pJob;
}

CYouTubeSystem::uploadstatus_t *CYouTubeSystem::GetStatus( YouTubeUploadHandle_t handle )
{
	int idx = m_mapUploads.Find( handle );
	if ( m_mapUploads.IsValidIndex( idx ) )
	{
		return &m_mapUploads[idx];
	}
	return NULL;
}

bool CYouTubeSystem::IsUploadFinished( YouTubeUploadHandle_t handle )
{
	AUTO_LOCK( m_Mutex );
	uploadstatus_t *pStatus = GetStatus( handle );
	if ( pStatus != NULL )
	{
		return pStatus->bFinished;
	}
	return true;
}

bool CYouTubeSystem::GetUploadProgress( YouTubeUploadHandle_t handle, double &ultotal, double &ulnow )
{
	AUTO_LOCK( m_Mutex );
	uploadstatus_t *pStatus = GetStatus( handle );
	if ( pStatus != NULL )
	{
		CYouTubeUploadJob *pJob = (CYouTubeUploadJob*)handle;
		pJob->GetProgress( ultotal, ulnow );
		return true;
	}
	return false;
}

bool CYouTubeSystem::GetUploadResults( YouTubeUploadHandle_t handle, bool &bSuccess, CUtlString &strURLToVideo, CUtlString &strURLToVideoStats )
{
	AUTO_LOCK( m_Mutex );
	uploadstatus_t *pStatus = GetStatus( handle );
	if ( pStatus != NULL )
	{
		bSuccess = pStatus->bSuccess;
		strURLToVideo = pStatus->strURLToVideo;
		strURLToVideoStats = pStatus->strURLToVideoStats;
		return true;
	}
	return false;
}

void CYouTubeSystem::ClearUploadResults( YouTubeUploadHandle_t handle )
{
	AUTO_LOCK( m_Mutex );
	if ( m_mapUploads.Remove( handle ) )
	{
		CYouTubeUploadJob *pJob = (CYouTubeUploadJob*)handle;
		pJob->Release();
	}
}

void CYouTubeSystem::CancelUpload( YouTubeUploadHandle_t handle )
{
	AUTO_LOCK( m_Mutex );
	if ( m_mapUploads.Remove( handle ) )
	{
		CYouTubeUploadJob *pJob = (CYouTubeUploadJob*)handle;
		pJob->CancelUpload();
		pJob->Release();
	}
}

void CYouTubeSystem::SetUploadFinished( YouTubeUploadHandle_t handle, bool bSuccess, const char *pURLToVideo, const char *pURLToVideoStats )
{
	AUTO_LOCK( m_Mutex );
	uploadstatus_t *pStatus = GetStatus( handle );
	if ( pStatus )
	{
		pStatus->bFinished = true;
		pStatus->bSuccess = bSuccess;
		pStatus->strURLToVideo = pURLToVideo;
		pStatus->strURLToVideoStats = pURLToVideoStats;
	}
}

void CYouTubeSystem::SetUserProfile( const char *pUserProfile )
{
	AUTO_LOCK( m_Mutex );
	m_strUserProfile = pUserProfile;
	CUtlString author;
	GetStandardTagValue( "author", m_strUserProfile.Get(), author );
	GetStandardTagValue( "name", author.Get(), m_strYouTubeUserName );
}

YouTubeInfoHandle_t CYouTubeSystem::GetInfo( const char *pURLToVideoStats, CYouTubeResponseHandler &responseHandler )
{
	AUTO_LOCK( m_Mutex );
	CYouTubeRetrieveInfoJob *pJob = new CYouTubeRetrieveInfoJob( this, pURLToVideoStats, responseHandler );
	m_pThreadPool->AddJob( pJob );
	m_vecRetrieveInfoJobs.AddToTail( pJob );
	return (YouTubeInfoHandle_t)pJob;
}

void CYouTubeSystem::CancelGetInfo( YouTubeInfoHandle_t handle )
{
	AUTO_LOCK( m_Mutex );
	int idx = m_vecRetrieveInfoJobs.Find( (CYouTubeRetrieveInfoJob*)handle );
	if ( idx >= 0 && idx < m_vecRetrieveInfoJobs.Count() )
	{
		((CYouTubeRetrieveInfoJob*)handle)->Release();
		m_vecRetrieveInfoJobs.FastRemove( idx );
	}
}

void CYouTubeSystem::SetDeveloperSettings( const char *pDeveloperKey, const char *pDeveloperTag )
{
	AUTO_LOCK( m_Mutex );
	m_strDeveloperKey = pDeveloperKey;
	m_strDeveloperTag = pDeveloperTag;
}

const char *CYouTubeSystem::GetDeveloperKey() const
{ 
	AUTO_LOCK( m_Mutex );
	return m_strDeveloperKey.Get(); 
}

const char *CYouTubeSystem::GetDeveloperTag() const
{ 
	AUTO_LOCK( m_Mutex );
	return m_strDeveloperTag.Get(); 
}

const char *CYouTubeSystem::GetLoginName() const
{
	return m_strYouTubeUserName.Get();
}

eYouTubeLoginStatus CYouTubeSystem::GetLoginStatus() const
{
	return m_eLoginStatus;
}

bool CYouTubeSystem::GetProfileURL( CUtlString &strProfileURL ) const
{
	if ( m_eLoginStatus == kYouTubeLogin_LoggedIn )
	{
		strProfileURL = CFmtStr1024( "http://www.youtube.com/profile?user=%s", m_strYouTubeUserName.Get() );
		return true;
	}
	return false;
}

void CYouTubeSystem::SetLoginStatus( eYouTubeLoginStatus status )
{
	AUTO_LOCK( m_Mutex );
	m_eLoginStatus = status;
	if ( m_eLoginStatus == kYouTubeLogin_LoggedIn )
	{
		CYouTubeRetrieveUserProfile *pJob = new CYouTubeRetrieveUserProfile( this );
		m_pThreadPool->AddJob( pJob );
		pJob->Release();
	}
}	

void CYouTubeSystem::SetAuthToken( const char *pAuthToken )
{
	AUTO_LOCK( m_Mutex );
	m_strAuthToken = pAuthToken;
}

const char* CYouTubeSystem::GetAuthToken() const
{
	AUTO_LOCK( m_Mutex );
	return m_strAuthToken.Get();
}

//=============================================================================
// Public API

void YouTube_SetDeveloperSettings( const char *pDeveloperKey, const char *pDeveloperTag )
{
	gYouTube.SetDeveloperSettings( pDeveloperKey, pDeveloperTag );
}

void YouTube_Login( const char *pUserName, const char *pPassword, const char *pSource )
{
	if ( gYouTube.GetLoginStatus() == kYouTubeLogin_LoggedIn )
	{
		return;
	}
	gYouTube.Login( pUserName, pPassword, pSource );
}

void YouTube_LoginCancel()
{
	gYouTube.LoginCancel();
}

const char *YouTube_GetLoginName()
{
	return gYouTube.GetLoginName();
}

eYouTubeLoginStatus YouTube_GetLoginStatus()
{
	return gYouTube.GetLoginStatus();
}

bool YouTube_GetProfileURL( CUtlString &strProfileURL )
{
	return gYouTube.GetProfileURL( strProfileURL );
}

YouTubeUploadHandle_t YouTube_Upload( const char* pFilePath, const char *pMimeType, const char *pTitle, const char *pDescription, const char *pCategory, const char *pKeywords, eYouTubeAccessControl access )
{
	return gYouTube.Upload( pFilePath, pMimeType, pTitle, pDescription, pCategory, pKeywords, access );
}

bool YouTube_IsUploadFinished( YouTubeUploadHandle_t handle )
{
	return gYouTube.IsUploadFinished( handle );
}

bool YouTube_GetUploadProgress( YouTubeUploadHandle_t handle, double &ultotal, double &ulnow )
{
	return gYouTube.GetUploadProgress( handle, ultotal, ulnow );
}

bool YouTube_GetUploadResults( YouTubeUploadHandle_t handle, bool &bSuccess, CUtlString &strURLToVideo, CUtlString &strURLToVideoStats )
{
	return gYouTube.GetUploadResults( handle, bSuccess, strURLToVideo, strURLToVideoStats );
}

void YouTube_ClearUploadResults( YouTubeUploadHandle_t handle )
{
	gYouTube.ClearUploadResults( handle );
}

void YouTube_CancelUpload( YouTubeUploadHandle_t handle )
{
	gYouTube.CancelUpload( handle );
}

YouTubeInfoHandle_t YouTube_GetVideoInfo( const char *pURLToVideoStats, CYouTubeResponseHandler &responseHandler )
{
	return gYouTube.GetInfo( pURLToVideoStats, responseHandler );
}

void YouTube_CancelGetVideoInfo( YouTubeInfoHandle_t handle )
{
	gYouTube.CancelGetInfo( handle );
}
