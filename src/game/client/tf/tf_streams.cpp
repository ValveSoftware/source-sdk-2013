//========= Copyright (C) 1996-2013, Valve Corporation, All rights reserved. ============//
//
// Purpose: Helper for access to news
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "gc_clientsystem.h"
#include "filesystem.h"
#include "workshop/ugc_utils.h"
#include "econ/econ_controls.h"

#include "ienginevgui.h"
#include "vgui/ISystem.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui_bitmapimage.h"
#include "bitmap/bitmap.h"
#include "imageutils.h"

#include "tier2/fileutils.h"

#include "checksum_sha1.h"
//#include "matchmaking/imatchframework.h"
//#include "engine/inetsupport.h"

#include "rtime.h"

#include "tf_streams.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

// GC hello information
//extern CMsgGCCStrike15_v2_MatchmakingGC2ClientHello g_GC2ClientHello;

ConVar cl_streams_request_url( "cl_streams_request_url",
	"https://api.twitch.tv/helix/streams?game_id=16676&first=5"
	, FCVAR_DEVELOPMENTONLY, "Number of streams requested for display" );
ConVar cl_streams_image_sfurl( "cl_streams_image_sfurl",
	"img://loadjpeg:(320x200):"
	, FCVAR_DEVELOPMENTONLY, "Format of Scaleform image representing the stream" );

ConVar cl_streams_request_count( "cl_streams_request_count", "6", FCVAR_DEVELOPMENTONLY, "How many streams are displayed in main menu" );

ConVar cl_streams_refresh_interval( "cl_streams_refresh_interval", "60", FCVAR_DEVELOPMENTONLY, "How often to refresh streams list" );
ConVar cl_streams_write_response_file( "cl_streams_write_response_file", "", FCVAR_DEVELOPMENTONLY, "When set will save streams info file for diagnostics" );
ConVar cl_streams_override_global_version( "cl_streams_override_global_version", "", FCVAR_DEVELOPMENTONLY, "When set will override global API version" );

ConVar cl_streams_mytwitchtv_nolink( "cl_streams_mytwitchtv_nolink", "https://www.twitch.tv/settings/connections", FCVAR_DEVELOPMENTONLY, "Twitch.tv account linking URL" );
ConVar cl_streams_mytwitchtv_channel( "cl_streams_mytwitchtv_channel", "https://www.twitch.tv/", FCVAR_DEVELOPMENTONLY, "Twitch.tv account channel URL" );

static const char *s_pszCacheImagePath = "streams";


//////////////////////////////////////////////////////////////////////////
//
// Files downloader
//

class CHelperStreamDownloadUrlToLocalFile;

static CUtlVector< CHelperStreamDownloadUrlToLocalFile * > s_arrDeleteCHelperStreamDownloadUrlToLocalFile;

class CHelperStreamDownloadUrlToLocalFile
{
public:
	CHelperStreamDownloadUrlToLocalFile( char const *szUrlGet, char const *szLocalFile, long lTimeStampLocal, CTFStreamPanel *pStreamPanel )
	{
		m_sUrlGet = szUrlGet;
		m_sLocalFile = szLocalFile;
		m_lTimestampLocal = lTimeStampLocal;

		m_hHTTPRequestHandle = steamapicontext->SteamHTTP()->CreateHTTPRequest( k_EHTTPMethodGET, m_sUrlGet.Get() );

		SteamAPICall_t hCall = NULL;
		if ( m_hHTTPRequestHandle && steamapicontext->SteamHTTP()->SendHTTPRequest( m_hHTTPRequestHandle, &hCall ) && hCall )
		{
			m_CallbackOnHTTPRequestCompleted.Set( hCall, this, &CHelperStreamDownloadUrlToLocalFile::Steam_OnHTTPRequestCompleted );
		}
		else
		{
			if ( m_hHTTPRequestHandle )
				steamapicontext->SteamHTTP()->ReleaseHTTPRequest( m_hHTTPRequestHandle );
			m_hHTTPRequestHandle = NULL;
		}

		m_hStreamPanel = pStreamPanel;
	}

private:
	CUtlString m_sUrlGet;
	CUtlString m_sLocalFile;
	long m_lTimestampLocal;
	HTTPRequestHandle m_hHTTPRequestHandle;
	CCallResult< CHelperStreamDownloadUrlToLocalFile, HTTPRequestCompleted_t > m_CallbackOnHTTPRequestCompleted;

	DHANDLE<CTFStreamPanel> m_hStreamPanel;
	
	void Steam_OnHTTPRequestCompleted( HTTPRequestCompleted_t *p, bool bError )
	{
		if ( !m_hHTTPRequestHandle || ( p->m_hRequest != m_hHTTPRequestHandle ) )
			return;

		uint32 unBytes = 0;
		if ( !bError && p && steamapicontext->SteamHTTP()->GetHTTPResponseBodySize( p->m_hRequest, &unBytes ) && unBytes != 0 )
		{
			DevMsg( "Request for '%s' succeeded (code: %u, size: %u)...\n", m_sUrlGet.Get(), p ? p->m_eStatusCode : 0, unBytes );

			CUtlBuffer bufFile;
			bufFile.EnsureCapacity( unBytes );
			if ( steamapicontext->SteamHTTP()->GetHTTPResponseBodyData( p->m_hRequest,(uint8*)bufFile.Base(), unBytes ) )
			{
				bufFile.SeekPut( bufFile.SEEK_HEAD, unBytes );

				g_pFullFileSystem->WriteFile( m_sLocalFile.Get(), "GAME", bufFile );
				UGC_SetFileTime( m_sLocalFile.Get(), m_lTimestampLocal );
			}

			if ( m_hStreamPanel.Get() )
			{
				m_hStreamPanel.Get()->InvalidateLayout();
			}
		}
		else
		{
			DevMsg( "Request for '%s' failed '%s' (code: %u, size: %u)...\n", m_sUrlGet.Get(), bError ? "error" : "ok", p ? p->m_eStatusCode : 0, unBytes );
		}

		steamapicontext->SteamHTTP()->ReleaseHTTPRequest( p->m_hRequest );
		m_hHTTPRequestHandle = NULL;

		s_arrDeleteCHelperStreamDownloadUrlToLocalFile.AddToTail( this );
	}
};


//////////////////////////////////////////////////////////////////////////
//
// Helper functions
//

static long Helper_ParseUpdatedTimestamp( char const *pchParseUpdatedAt )
{
	// Twitch.tv format: "2013-03-04T05:27:27Z"
	return CRTime::RTime32FromRFC3339UTCString( pchParseUpdatedAt );
}

static void Helper_ConfigureStreamInfoPreviewImages( CStreamInfo &info, CTFStreamPanel *pStreamPanel )
{
	// store global name for the info in the panel to ask for it when the image is ready to be displayed
	pStreamPanel->SetGlobalName( info.m_sGlobalName );

	char chPreviewImageLocal[ 2*MAX_PATH + 1 ] = {};
	char *pchLocalCur = chPreviewImageLocal, *pchLocalEnd = chPreviewImageLocal + Q_ARRAYSIZE( chPreviewImageLocal ) - 1;
	for ( char const *pch = info.m_sPreviewImage.Get(); *pch; ++ pch )
	{
		if ( pchLocalEnd - pchLocalCur < 4 )
			break;
		if ( ( ( pch[0] >= 'a' ) && ( pch[0] <= 'z' ) ) ||
			( ( pch[0] >= 'A' ) && ( pch[0] <= 'Z' ) ) ||
			( ( pch[0] >= '0' ) && ( pch[0] <= '9' ) ) ||
			( pch[0] == '-' ) || ( pch[0] == '.' ) )
		{
			* ( pchLocalCur ++ ) = *pch;
		}
		else
		{
			* ( pchLocalCur ++ ) = '_';
			int iHigh = ( ( ( unsigned char )( *pch ) & 0xF0u ) >> 4 ) & 0xF;
			if ( iHigh >= 0 && iHigh <= 9 )
				* ( pchLocalCur ++ ) = '0' + iHigh;
			else
				* ( pchLocalCur ++ ) = 'A' + iHigh;
			int iLow = ( ( ( unsigned char )( *pch ) & 0xFu ) );
			if ( iLow >= 0 && iLow <= 9 )
				* ( pchLocalCur ++ ) = '0' + iLow;
			else if ( iLow >= 10 && iLow <= 15 )
				* ( pchLocalCur ++ ) = 'A' + iLow - 10;
			* ( pchLocalCur ++ ) = '_';
		}
	}
	if ( chPreviewImageLocal[0] )
	{
		// We might need to download the file, so make sure directory structure is valid
		static bool s_bCreateDirHierarchyDone = false;
		if ( !s_bCreateDirHierarchyDone )
		{
			s_bCreateDirHierarchyDone = true;
				
			// Cleanup old cached files
			if ( long lDirectoryTime = g_pFullFileSystem->GetFileTime( s_pszCacheImagePath, "GAME" ) )
			{
				FileFindHandle_t hFind = NULL;
				int numRemove = 0;
				for ( char const *szFileName = g_pFullFileSystem->FindFirst( CFmtStr( "%s/*", s_pszCacheImagePath ), &hFind );
					szFileName && *szFileName; szFileName = g_pFullFileSystem->FindNext( hFind ) )
				{
					if ( !Q_strcmp( ".", szFileName ) || !Q_strcmp( "..", szFileName ) ) continue;
					CFmtStr fmtFilename( "%s/%s", s_pszCacheImagePath, szFileName );
					long lFileTime = g_pFullFileSystem->GetFileTime( fmtFilename, "GAME" );
					if ( ( lFileTime >= lDirectoryTime - 72*3600 ) && ( lFileTime <= lDirectoryTime + 72*3600 ) )
					{
						// DevMsg( "Keeping file %s (%u : %u)\n", fmtFilename.Access(), lFileTime, lDirectoryTime );
						continue;
					}
					else
					{
						++ numRemove;
						g_pFullFileSystem->RemoveFile( fmtFilename, "GAME" );
					}
				}
				DevMsg( 2, "Streams preview cache evicted %u files\n", numRemove );
				g_pFullFileSystem->FindClose( hFind );
			}

			g_pFullFileSystem->CreateDirHierarchy( s_pszCacheImagePath, "GAME" );
		}
			
		//
		// Work with the file cache
		//
		CFmtStr fmtLocalFile( "%s/%s", s_pszCacheImagePath, chPreviewImageLocal );
		info.m_sPreviewImageLocalFile = fmtLocalFile.Access();
		
		CFmtStr fmtPreviewSF( "%sstreams/%s", cl_streams_image_sfurl.GetString(), chPreviewImageLocal );
		info.m_sPreviewImageSF = fmtPreviewSF.Access();

		// See if we need to download that file
		long lFileTime = g_pFullFileSystem->GetFileTime( fmtLocalFile.Access(), "GAME" );
		// Parse "updated_at" attribute:
		long lUpdatedAtStamp = Helper_ParseUpdatedTimestamp( info.m_sUpdatedAtStamp.Get() );
		if ( lFileTime != lUpdatedAtStamp )
		{
			// Redownload the file
			DevMsg( 2, "%s -- Requesting download of preview image (%ld != %ld): %s -> %s\n", info.m_sGlobalName.Get(), lFileTime, lUpdatedAtStamp, info.m_sPreviewImage.Get(), info.m_sPreviewImageLocalFile.Get() );
			new CHelperStreamDownloadUrlToLocalFile( info.m_sPreviewImage.Get(), info.m_sPreviewImageLocalFile.Get(), lUpdatedAtStamp, pStreamPanel );
		}
		else
		{
			DevMsg( 2, "%s -- Preview image is up to date (%ld): %s -> %s\n", info.m_sGlobalName.Get(), lUpdatedAtStamp, info.m_sPreviewImage.Get(), info.m_sPreviewImageLocalFile.Get() );
			pStreamPanel->InvalidateLayout();
		}
	}
	else
	{
		DevMsg( 2, "%s -- No preview image\n", info.m_sGlobalName.Get() );
		info.m_sPreviewImageSF = cl_streams_image_sfurl.GetString();
	}
}


//////////////////////////////////////////////////////////////////////////
//
//

static CTFStreamManager g_streamManager;
CTFStreamManager* StreamManager()
{
	return &g_streamManager;
}

CTFStreamManager::CTFStreamManager()
{
	m_dblTimeStampLastUpdate = 0;
	m_hHTTPRequestHandle = NULL;
	m_hHTTPRequestHandleTwitchTv = NULL;
	m_pLoadingAccount = NULL;
}


CTFStreamManager::~CTFStreamManager()
{
	m_vecTwitchTvAccounts.PurgeAndDeleteElements();
}


bool CTFStreamManager::Init()
{
	RequestTopStreams();

	return true;
}


void CTFStreamManager::Update( float frametime )
{
	// Cleanup downloaders
	if ( s_arrDeleteCHelperStreamDownloadUrlToLocalFile.Count() )
	{
		FOR_EACH_VEC( s_arrDeleteCHelperStreamDownloadUrlToLocalFile, iDownloader )
		{
			delete s_arrDeleteCHelperStreamDownloadUrlToLocalFile[iDownloader];
		}
		s_arrDeleteCHelperStreamDownloadUrlToLocalFile.RemoveAll();
	}

	UpdateTwitchTvAccounts();
}

void CTFStreamManager::RequestTopStreams()
{
	// Check if it's time to update
	if ( !m_dblTimeStampLastUpdate || ( Plat_FloatTime() - m_dblTimeStampLastUpdate > cl_streams_refresh_interval.GetFloat() ) )
	{
		m_dblTimeStampLastUpdate = Plat_FloatTime();

		if ( !m_hHTTPRequestHandle && steamapicontext && steamapicontext->SteamHTTP() )
		{
			//
			// Create HTTP download job
			//
			m_hHTTPRequestHandle = steamapicontext->SteamHTTP()->CreateHTTPRequest( k_EHTTPMethodGET, cl_streams_request_url.GetString() );
			steamapicontext->SteamHTTP()->SetHTTPRequestHeaderValue( m_hHTTPRequestHandle, "Client-ID", "b7816vx0i8sng8bwy9es0dirdcsy3im" );
			DevMsg( "Requesting twitch.tv streams update...\n" );

			SteamAPICall_t hCall = NULL;
			if ( m_hHTTPRequestHandle && steamapicontext->SteamHTTP()->SendHTTPRequest( m_hHTTPRequestHandle, &hCall ) && hCall )
			{
				m_CallbackOnHTTPRequestCompleted.Set( hCall, this, &CTFStreamManager::Steam_OnHTTPRequestCompletedStreams );
			}
			else
			{
				if ( m_hHTTPRequestHandle )
					steamapicontext->SteamHTTP()->ReleaseHTTPRequest( m_hHTTPRequestHandle );
				m_hHTTPRequestHandle = NULL;
			}
		}	
	}
}


static int Helper_SortStreamsByViewersCount( const CStreamInfo *a, const CStreamInfo *b )
{
	if ( a->m_numViewers != b->m_numViewers )
		return ( a->m_numViewers > b->m_numViewers ) ? -1 : 1;
	
	return Q_stricmp( a->m_sGlobalName.Get(), b->m_sGlobalName.Get() );
}

static void Helper_ConvertLanguageToCountryCode( CUtlString &s )
{
	if ( !Q_stricmp(s, "en") )
		s = "gb";
}

void CTFStreamManager::Steam_OnHTTPRequestCompletedStreams( HTTPRequestCompleted_t *p, bool bError )
{
	if ( !m_hHTTPRequestHandle || ( p->m_hRequest != m_hHTTPRequestHandle ) )
		return;

	uint32 unBytes = 0;
	if ( !bError && p && steamapicontext->SteamHTTP()->GetHTTPResponseBodySize( p->m_hRequest, &unBytes ) && unBytes != 0 )
	{
		DevMsg( "Request for twitch.tv streams succeeded (code: %u, size: %u)...\n", p ? p->m_eStatusCode : 0, unBytes );

		CUtlVector< CStreamInfo > arrStreamInfos;
		CUtlBuffer bufFile;
		bufFile.EnsureCapacity( unBytes );
		if ( steamapicontext->SteamHTTP()->GetHTTPResponseBodyData( p->m_hRequest,(uint8*)bufFile.Base(), unBytes ) )
		{
			bufFile.SeekPut( bufFile.SEEK_HEAD, unBytes );

			if ( cl_streams_write_response_file.GetString()[0] )
			{
				g_pFullFileSystem->WriteFile( cl_streams_write_response_file.GetString(), "GAME", bufFile );
			}
			
			// Parse JSON from the received file
			GCSDK::CWebAPIValues *pValues = GCSDK::CWebAPIValues::ParseJSON( bufFile );
			if ( pValues )
			{
				if ( GCSDK::CWebAPIValues *pvStreams = pValues->FindChild( "data" ) )
				{
					for ( GCSDK::CWebAPIValues *pvStream = pvStreams->GetFirstChild(); pvStream; pvStream = pvStream->GetNextChild() )
					{
#if 0	// this is the code to print JSON output as DevMsgs to figure out which element is where in the response
						DevMsg( "----STREAM----\n" );
						for ( GCSDK::CWebAPIValues *pTest = pvStream->GetFirstChild(); pTest; pTest = pTest->GetNextChild() )
						{
							CUtlString sValueText;
							pTest->GetStringValue( sValueText );
							DevMsg( "child: %s = (%u) %s\n", pTest->GetName(), pTest->GetUInt32Value(), sValueText.Get() );

							for ( GCSDK::CWebAPIValues *pTest2 = pTest->GetFirstChild(); pTest2; pTest2 = pTest2->GetNextChild() )
							{
								pTest2->GetStringValue( sValueText );
								DevMsg( "     child2: %s = (%u) %s\n", pTest2->GetName(), pTest2->GetUInt32Value(), sValueText.Get() );
							}
						}
#endif

						CStreamInfo info;
						info.m_numViewers = pvStream->GetChildUInt32Value( "viewer_count" );
						pvStream->GetChildStringValue( info.m_sGlobalName, "user_name", "" );
						pvStream->GetChildStringValue( info.m_sTextDescription, "title", "" );
						pvStream->GetChildStringValue( info.m_sUpdatedAtStamp, "started_at", "" );

						// grab the template url and replace the values for the "medium" image we used to request with v3
						pvStream->GetChildStringValue( info.m_sPreviewImage, "thumbnail_url", "" );
						static char pTempURL[512];
						V_StrSubst( info.m_sPreviewImage.Get(), "{width}", "320", pTempURL, 512 );
						info.m_sPreviewImage.Set( pTempURL );
						V_StrSubst( info.m_sPreviewImage.Get(), "{height}", "180", pTempURL, 512 );
						info.m_sPreviewImage.Set( pTempURL );

						if ( ( info.m_numViewers > 0 ) &&
							!info.m_sGlobalName.IsEmpty() &&
							!info.m_sTextDescription.IsEmpty() )
						{
							//DevMsg( 2, "Channel: %s (%u viewers) -- %s [[%s]]\n", info.m_sGlobalName.Get(), info.m_numViewers, info.m_sTextDescription.Get(), info.m_sVideoFeedUrl.Get() );

							arrStreamInfos.AddToTail( info );
						}
					}
				}
			}

			delete pValues;
		}

		if ( arrStreamInfos.Count() )
		{
			arrStreamInfos.Sort( Helper_SortStreamsByViewersCount );
			if ( arrStreamInfos.Count() > cl_streams_request_count.GetInt() )
				arrStreamInfos.SetCountNonDestructively( cl_streams_request_count.GetInt() );

			m_streamInfoVec.Swap( arrStreamInfos );	// Set the new information live and notify all systems

			IGameEvent *event = gameeventmanager->CreateEvent( "top_streams_request_finished" );
			if ( event )
			{
				gameeventmanager->FireEventClientSide( event );
			}
		}
	}
	else
	{
		DevMsg( "Request for twitch.tv streams failed: %s (code: %u, size: %u)...\n",
			bError ? "error" : "ok", p ? p->m_eStatusCode : 0, unBytes );
	}

	steamapicontext->SteamHTTP()->ReleaseHTTPRequest( p->m_hRequest );
	m_hHTTPRequestHandle = NULL;
	m_dblTimeStampLastUpdate = Plat_FloatTime(); // push the update counter to not update for a little bit
}

CStreamInfo* CTFStreamManager::GetStreamInfoByName( char const *szName )
{
	if ( !szName )
		return NULL;
	
	for ( int idx = 0; idx < m_streamInfoVec.Count(); ++ idx )
	{
		if ( !V_strcmp( szName, m_streamInfoVec[idx].m_sGlobalName.Get() ) )
			return &m_streamInfoVec[idx];
	}

	return NULL;
}


TwitchTvAccountInfo_t* CTFStreamManager::GetTwitchTvAccountInfo( uint64 uiSteamID )
{
	TwitchTvAccountInfo_t *pInfo = NULL;
	FOR_EACH_VEC( m_vecTwitchTvAccounts, i )
	{
		if ( m_vecTwitchTvAccounts[i]->m_uiSteamID == uiSteamID )
		{
			pInfo = m_vecTwitchTvAccounts[i];

			// Uncomment this if we really need to keep checking twitch status
			// info needs update if it's been longer than 300 secs from the last update
			/*if ( pInfo->m_dblTimeStampTwitchTvUpdate && ( Plat_FloatTime() - pInfo->m_dblTimeStampTwitchTvUpdate > 300 ) )
			{
				m_vecTwitchTvAccounts.Remove( i );
				pInfo = NULL;
			}*/

			break;
		}
	}

	// add one if not already on the list
	if ( !pInfo )
	{
		pInfo = new TwitchTvAccountInfo_t;
		if ( pInfo )
		{
			pInfo->m_uiSteamID = uiSteamID;
			pInfo->m_eTwitchTvState = k_ETwitchTvState_None;
			pInfo->m_dblTimeStampTwitchTvUpdate = 0;
			pInfo->m_uiTwitchTvUserId = 0;
			pInfo->m_sTwitchTvChannel = cl_streams_mytwitchtv_nolink.GetString();
			m_vecTwitchTvAccounts.AddToTail( pInfo );
		}
	}

	return pInfo;
}


void CTFStreamManager::UpdateTwitchTvAccounts()
{
	if ( m_pLoadingAccount )
		return;

	// find the new account in the list to load
	FOR_EACH_VEC( m_vecTwitchTvAccounts, i )
	{
		if ( m_vecTwitchTvAccounts[i]->m_eTwitchTvState == k_ETwitchTvState_None )
		{
			m_pLoadingAccount = m_vecTwitchTvAccounts[i];
			break;
		}
	}

	// nothing needs to be loaded
	if ( !m_pLoadingAccount )
		return;

	// When requesting a refresh reset all known linking state
	m_pLoadingAccount->m_eTwitchTvState = k_ETwitchTvState_Loading;
	m_pLoadingAccount->m_dblTimeStampTwitchTvUpdate = Plat_FloatTime();

	Assert( !m_hHTTPRequestHandleTwitchTv );
	if ( m_hHTTPRequestHandleTwitchTv ) return;
	//
	// Create HTTP download job
	//
	// If we ever end up using this we'll need to update to the most recent API. v3 of the API (which we were using for this)
	// is being shut down. It's not currently being used so I'm just going to comment it out for now.
	//m_hHTTPRequestHandleTwitchTv = steamapicontext->SteamHTTP()->CreateHTTPRequest( k_EHTTPMethodGET, CFmtStr( "http://api.twitch.tv/api/steam/%llu", m_pLoadingAccount->m_uiSteamID ) );
	//steamapicontext->SteamHTTP()->SetHTTPRequestHeaderValue( m_hHTTPRequestHandleTwitchTv, "Accept", cl_streams_request_accept.GetString() );
	DevMsg( "Requesting twitch.tv account link...\n" );

	SteamAPICall_t hCall = NULL;
	if ( m_hHTTPRequestHandleTwitchTv && steamapicontext->SteamHTTP()->SendHTTPRequest( m_hHTTPRequestHandleTwitchTv, &hCall ) && hCall )
	{
		m_CallbackOnHTTPRequestCompletedTwitchTv.Set( hCall, this, &CTFStreamManager::Steam_OnHTTPRequestCompletedMyTwitchTv );
	}
	else
	{
		if ( m_hHTTPRequestHandleTwitchTv )
			steamapicontext->SteamHTTP()->ReleaseHTTPRequest( m_hHTTPRequestHandleTwitchTv );
		m_hHTTPRequestHandleTwitchTv = NULL;
	}
}

void CTFStreamManager::Steam_OnHTTPRequestCompletedMyTwitchTv( HTTPRequestCompleted_t *p, bool bError )
{
	if ( !m_hHTTPRequestHandleTwitchTv || ( p->m_hRequest != m_hHTTPRequestHandleTwitchTv ) )
		return;

	Assert( m_pLoadingAccount->m_eTwitchTvState == k_ETwitchTvState_Loading );
	m_pLoadingAccount->m_eTwitchTvState = k_ETwitchTvState_Error;
	m_pLoadingAccount->m_sTwitchTvChannel.Clear();
	m_pLoadingAccount->m_uiTwitchTvUserId = 0ull;

	uint32 unBytes = 0;
	if ( !bError && p && steamapicontext->SteamHTTP()->GetHTTPResponseBodySize( p->m_hRequest, &unBytes ) && unBytes != 0 )
	{
		DevMsg( "Request for twitch.tv account link succeeded (code: %u, size: %u)...\n", p ? p->m_eStatusCode : 0, unBytes );

		CUtlBuffer bufFile;
		bufFile.EnsureCapacity( unBytes );
		if ( steamapicontext->SteamHTTP()->GetHTTPResponseBodyData( p->m_hRequest,(uint8*)bufFile.Base(), unBytes ) )
		{
			bufFile.SeekPut( bufFile.SEEK_HEAD, unBytes );

			if ( cl_streams_write_response_file.GetString()[0] )
			{
				g_pFullFileSystem->WriteFile( cl_streams_write_response_file.GetString(), "GAME", bufFile );
			}
			
			// Parse JSON from the received file
			GCSDK::CWebAPIValues *pValues = GCSDK::CWebAPIValues::ParseJSON( bufFile );
			if ( pValues )
			{
				pValues->GetChildStringValue( m_pLoadingAccount->m_sTwitchTvChannel, "name", "" );
				m_pLoadingAccount->m_uiTwitchTvUserId = pValues->GetChildUInt64Value( "_id" );

				if ( m_pLoadingAccount->m_sTwitchTvChannel.IsEmpty() )
				{
					m_pLoadingAccount->m_sTwitchTvChannel = cl_streams_mytwitchtv_nolink.GetString();
					m_pLoadingAccount->m_eTwitchTvState = k_ETwitchTvState_NoLink;
				}
				else
				{
					m_pLoadingAccount->m_sTwitchTvChannel = CFmtStr( "%s%s", cl_streams_mytwitchtv_channel.GetString(), m_pLoadingAccount->m_sTwitchTvChannel.Get() );
					m_pLoadingAccount->m_eTwitchTvState = k_ETwitchTvState_Linked;
				}
			}

			delete pValues;
		}
	}
	else
	{
		DevMsg( "Request for twitch.tv account link failed: %s (code: %u, size: %u)...\n",
			bError ? "error" : "ok", p ? p->m_eStatusCode : 0, unBytes );
	}

	steamapicontext->SteamHTTP()->ReleaseHTTPRequest( p->m_hRequest );
	m_hHTTPRequestHandleTwitchTv = NULL;
	m_pLoadingAccount->m_dblTimeStampTwitchTvUpdate = Plat_FloatTime(); // push the update counter to not update for a little bit

	// done loading
	m_pLoadingAccount = NULL;
}


CTFStreamPanel::CTFStreamPanel( Panel *parent, const char *panelName ) : EditablePanel( parent, panelName )
{
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);

	m_pPreviewImage = new ImagePanel( this, "PreviewImage" );
}

void CTFStreamPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/StreamPanel.res" );
}

void CTFStreamPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	UpdatePanels();
}

void CTFStreamPanel::OnCommand( const char *command )
{
	if ( FStrEq( command, "stream" ) )
	{
		CStreamInfo *pInfo = GetStreamInfo();
		if ( pInfo )
		{
			vgui::system()->ShellExecute( "open", CFmtStr( "%s%s", cl_streams_mytwitchtv_channel.GetString(), pInfo->m_sGlobalName.Get() ) );
		}
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

CStreamInfo *CTFStreamPanel::GetStreamInfo() const
{
	if ( m_strStreamInfoGlobalName.IsEmpty() )
		return NULL;

	return g_streamManager.GetStreamInfoByName( m_strStreamInfoGlobalName.Get() );
}

void CTFStreamPanel::UpdatePanels()
{
	CStreamInfo *pInfo = GetStreamInfo();
	if ( pInfo )
	{
		SetDialogVariable( "display_name", pInfo->m_sGlobalName.Get() );
		SetDialogVariable( "viewer_count", CFmtStr( "%d viewers", pInfo->m_numViewers ) );
		SetDialogVariable( "text_description", pInfo->m_sTextDescription.Get() );

		SetPreviewImage( pInfo->m_sPreviewImageLocalFile.Get() );

		Panel* pLoadingPanel = FindChildByName( "LoadingPanel" );
		if ( pLoadingPanel )
		{
			pLoadingPanel->SetVisible( false );
		}
	}
	else
	{
		SetDialogVariable( "display_name", "" );
		SetDialogVariable( "viewer_count", "" );
		SetDialogVariable( "text_description", "" );

		SetPreviewImage( NULL );
	}

	Panel* pStreamButton = FindChildByName( "Stream_URLButton" );
	if ( pStreamButton )
	{
		pStreamButton->SetEnabled( pInfo != NULL );
	}
	
}

void CTFStreamPanel::SetPreviewImage( const char *pszPreviewImageFile )
{
	// clean up old image if there's one
	m_pPreviewImage->EvictImage();

	if ( !pszPreviewImageFile )
	{
		m_pPreviewImage->SetImage( (vgui::IImage *)0 );
		return;
	}

	char szImageAbsPath[MAX_PATH];
	if ( !GenerateFullPath( pszPreviewImageFile, "MOD", szImageAbsPath, ARRAYSIZE( szImageAbsPath ) ) )
	{
		Warning( "Failed to GenerateFullPath %s\n", pszPreviewImageFile );
		return;
	}

	Bitmap_t image;
	ConversionErrorType nErrorCode = ImgUtl_LoadBitmap( szImageAbsPath, image );
	if ( nErrorCode != CE_SUCCESS )
	{
		m_pPreviewImage->SetImage( (vgui::IImage *)0 );
		return;
	}

	int wide, tall;
	BitmapImage *pBitmapImage = new BitmapImage;
	pBitmapImage->SetBitmap( image );
	pBitmapImage->GetSize( wide, tall );

	// The ImagePanel needs to know the scaling factor for the BitmapImage, to
	// center it when it's going to be rendered, and then the BitmapImage needs
	// to know how big it should be rendered.
	float flPanelWidthScale = static_cast<float>( m_pPreviewImage->GetWide() ) / wide;
	m_pPreviewImage->SetShouldCenterImage( false );
	m_pPreviewImage->SetShouldScaleImage( true );
	m_pPreviewImage->SetScaleAmount( flPanelWidthScale );

	float flSubRectWidthScale = static_cast<float>( wide ) / image.Width();
	//float flSubRectHeightScale = static_cast<float>( tall ) / image.Height();
	pBitmapImage->SetRenderSize( flSubRectWidthScale * m_pPreviewImage->GetWide(), flSubRectWidthScale * flPanelWidthScale * tall );

	m_pPreviewImage->SetImage( pBitmapImage );

	float flPanelHeightScale = static_cast<float>( image.Height() ) / image.Width();
	m_pPreviewImage->SetSize( m_pPreviewImage->GetWide(), flPanelHeightScale * m_pPreviewImage->GetWide() );
}


CTFStreamListPanel::CTFStreamListPanel( Panel *parent, const char *panelName ) : EditablePanel( parent, panelName )
{
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	for ( int i=0; i<ARRAYSIZE( m_arrStreamPanels ); ++i )
	{
		m_arrStreamPanels[i] = new CTFStreamPanel( this, CFmtStr( "Stream%d", i + 1 ) );
	}

	ListenForGameEvent( "top_streams_request_finished" );
}

void CTFStreamListPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/StreamListPanel.res" );
}

void CTFStreamListPanel::OnThink()
{
	// this will fire "top_streams_request_finished" event when the job is done
	g_streamManager.RequestTopStreams();
}

void CTFStreamListPanel::OnCommand( const char *command )
{
	if ( FStrEq( command, "hide_streams" ) )
	{
		SetVisible( false );
	}
	else if ( FStrEq( command, "view_more" ) )
	{
		vgui::system()->ShellExecute( "open", "https://www.twitch.tv/directory/game/Team%20Fortress%202" );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

void CTFStreamListPanel::FireGameEvent( IGameEvent *event )
{
	const char *pszEventName = event->GetName();
	if ( FStrEq( pszEventName, "top_streams_request_finished" ) )
	{
		// update each stream panel
		for ( int i=0; i<ARRAYSIZE( m_arrStreamPanels ); ++i )
		{
			bool bVisible = i < g_streamManager.GetStreamInfoVec().Count();
			m_arrStreamPanels[i]->SetVisible( bVisible );
			if ( bVisible )
			{
				Helper_ConfigureStreamInfoPreviewImages( g_streamManager.GetStreamInfoVec()[i], m_arrStreamPanels[i] );
			}
		}
	}
}
