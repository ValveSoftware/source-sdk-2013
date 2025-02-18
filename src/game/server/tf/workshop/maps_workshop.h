//====== Copyright  Valve Corporation, All rights reserved. =================
//
// Requests subscribed maps from the workshop, holds a list of them along with metadata.
//
//=============================================================================

#if !defined TF_MAPS_WORKSHOP_H
#define TF_MAPS_WORKSHOP_H
#if defined( COMPILER_MSVC )
#pragma once
#endif

#include "igamesystem.h"

// Enable verbose debug spew to DevMsg
// #define TF_WORKSHOP_DEBUG

#define TFWorkshopMsg(...) Msg("[TF Workshop] " __VA_ARGS__)
#define TFWorkshopWarning(...) Warning("[TF Workshop] " __VA_ARGS__)

#ifdef TF_WORKSHOP_DEBUG
#define TFWorkshopDebug(...) DevMsg("[TF Workshop Debug] " __VA_ARGS__)
#else // TF_WORKSHOP_DEBUG
#define TFWorkshopDebug(...)
#endif // TF_WORKSHOP_DEBUG

class CTFMapsWorkshop;

CTFMapsWorkshop *TFMapsWorkshop();

// Represents a workshop map
class CTFWorkshopMap
{
public:
	// Rechecks local files and steam for map status. Currently triggers a synchronous fstat(), so only call during
	// initialization/user-action.
	// If eRefresh_HighPriority is passed, we will ask UGC to retreive any available updates as high priority.
	enum eRefreshType { eRefresh_Normal, eRefresh_HighPriority };
	void Refresh( eRefreshType refreshType = eRefresh_Normal );

	enum eState
	{
		eState_Refreshing,
		eState_Error,
		eState_Downloading,
		eState_Downloaded
	};
	eState State() const { return m_eState; }

	// Returns true if downloaded. Optionally returns progress, which is [0, 1]
	// Any map that returns IsValid() is either downloaded or attempting to download/sync
	bool Downloaded( /* out */ float *flProgress = NULL );

	// Only known after map state leaves refreshing
	const char *CanonicalName() const { return m_strCanonicalName.Length() ? m_strCanonicalName.Get() : NULL; }

	bool GetLocalFile( /* out */ CUtlString &strLocalFile );

	PublishedFileId_t FileID() const { return m_nFileID; }

	const char* OriginalName() const { return m_strMapName.Length() ? m_strMapName.Get() : NULL; }
	uint32 TimeUpdated() const { return m_rtimeUpdated; }

private:
	friend class CTFMapsWorkshop;
	CTFWorkshopMap( PublishedFileId_t nMapID );

	// Forwarded callback from maps workshop about map downloads
	void OnUGCDownload( DownloadItemResult_t *pResult );
	void OnUGCItemInstalled( ItemInstalled_t *pResult );

	// Update the map name and local filename.
	// Requires download complete due the way ISteamUGC currently works.
	// Currently triggers a sync directory enumeration :-/
	void UpdateMapName();

	CCallResult<CTFWorkshopMap, SteamUGCQueryCompleted_t> m_callbackQueryUGCDetails;
	void Steam_OnQueryUGCDetails( SteamUGCQueryCompleted_t *pResult, bool bError );

	PublishedFileId_t m_nFileID;
	uint32 m_rtimeUpdated;
	int32 m_nFileSize;
	CUtlString m_strCanonicalName;
	CUtlString m_strMapName;
	eState m_eState;
	bool m_bHighPriority;
};

// Autogamesystem to request user maps on startup and call update on the workshop manager.
class CTFMapsWorkshop : public CAutoGameSystemPerFrame
{
public:
	CTFMapsWorkshop();

	bool Init( void ) OVERRIDE;
	void Shutdown( void ) OVERRIDE;
	virtual const char* Name( void ) OVERRIDE { return "TFMapsWorkshop"; }

	// Recheck subscriptions and on-disk maps for sync
	void Refresh();

	// Is this a valid original filename for a uploaded workshop map. Checked on upload and against workshop files
	// before considering them for download. (e.g. cp_foo.bsp)
	static inline bool IsValidOriginalFileNameForMap( const CUtlString &originalName );
	// Is valid for the display name of a workshop map, (e.g. cp_foo)
	static inline bool IsValidDisplayNameForMap( const CUtlString &originalName );

	// Is user currently subscribed to this map
	bool IsSubscribed( PublishedFileId_t nFileID );

	// Build a canonical map name given its ID and original file name.
	bool CanonicalNameForMap( PublishedFileId_t, const CUtlString &strOriginalName, /* out */ CUtlString &strCanonName );

	enum eNameType
	{
		// Map name looks like a workshop map, but we don't know its proper name. Returns e.g. "workshop/12345".
		eName_Incomplete,
		// Map ID is known and canonical name provided
		eName_Canon
	};
	eNameType GetMapName( PublishedFileId_t nMapID, /* out */ CUtlString &mapName );

	// Attempt to work out a map id from a local name, either the full canonical name ( workshop/cp_map.ugc12345 ) or a
	// sufficient shorthand name ( workshop/12345 ).
	//
	// NOTE This does not validate the friendly name of the map: workshop/cp_bogus_name.ugc12345 will return 12345 just the
	// same.
	PublishedFileId_t MapIDFromName( CUtlString mapName );

	// Add this map to our list for this session, triggering download/etc as if it were subscribed
	bool AddMap( PublishedFileId_t nFileID );

	// *blocking*
	// Synchronously prepare a map for use, including downloading and optionally copying it to the local disk.
	enum eSyncType
	{
		eSync_LocalDisk,
		eSync_SteamOnly
	};

	// Forwarded IServerGameDLL hooks to prepare workshop maps on demand.
	IServerGameDLL::ePrepareLevelResourcesResult
		AsyncPrepareLevelResources( /* in/out */ char *pszMapName, size_t nMapNameSize,
		                            /* in/out */ char *pszMapFile, size_t nMapFileSize,
		                            float *flProgress = NULL );

	// Blocking version of AsyncPrepareLevelResources
	void PrepareLevelResources( /* in/out */ char *pszMapName, size_t nMapNameSize,
	                            /* in/out */ char *pszMapFile, size_t nMapFileSize );

	IServerGameDLL::eCanProvideLevelResult OnCanProvideLevel( /* in/out */ char *pMapName, int nMapNameMax );

	// When the gameserver steam context becomes available.
	void GameServerSteamAPIActivated();

	// Spews a list of current maps and their status to console
	void PrintStatusToConsole();

	bool GetWorkshopMapDesc( uint32 uIndex, WorkshopMapDesc_t *pDesc );

private:
	CCallback<CTFMapsWorkshop, DownloadItemResult_t, false> m_callbackDownloadItem;
	CCallback<CTFMapsWorkshop, ItemInstalled_t, false> m_callbackItemInstalled;

	// gameserver API variants
	CCallback<CTFMapsWorkshop, DownloadItemResult_t, true> m_callbackDownloadItem_GameServer;
	CCallback<CTFMapsWorkshop, ItemInstalled_t, true> m_callbackItemInstalled_GameServer;
	void Steam_OnUGCDownload( DownloadItemResult_t *pResult );
	void Steam_OnUGCItemInstalled( ItemInstalled_t *pResult );

	// See if we have any tracked workshop maps that this name matches, canonical or otherwise
	CTFWorkshopMap *FindMapByName( const char *pMapName );
	// Will create a tracked map if this name looks like a workshop map
	CTFWorkshopMap *FindOrCreateMapByName( const char *pMapName );

	// All managed workshop maps
	CUtlMap< PublishedFileId_t, CTFWorkshopMap * > m_mapMaps;
	CUtlVector< PublishedFileId_t > m_vecSubscribedMaps;

	PublishedFileId_t m_nPreparingMap;
};

//
// Util
//

// inline so we can access this from client dll for the uploader
inline bool CTFMapsWorkshop::IsValidOriginalFileNameForMap( const CUtlString &originalName )
{
	// Matching: ([a-z0-9]+_)*[a-z0-9]\.bsp

	int len = originalName.Length();
	const unsigned int nMaxFileName = MAX_DISPLAY_MAP_NAME + 4; // Map minus extension must be within MAX_DISPLAY_MAP_NAME
	if ( len < 6 || len > nMaxFileName || originalName.Slice( len - 4 ) != ".bsp" )
	{
		TFWorkshopWarning( "Map filename must be at least 6 characters and not more than %u characters ending in .bsp\n", nMaxFileName );
		return false;
	}

	CUtlString baseName = originalName.Slice( 0, len - 4 );
	return IsValidDisplayNameForMap( baseName );
}

inline bool CTFMapsWorkshop::IsValidDisplayNameForMap( const CUtlString &originalName )
{
	// Matching: ([a-z0-9]+_)*[a-z0-9]

	int len = originalName.Length();
	const unsigned int nMaxDisplayName = MAX_DISPLAY_MAP_NAME;
	if ( len < 2 || len > nMaxDisplayName )
	{
		TFWorkshopWarning( "Map display name must be at least 2 characters and not more than %u characters\n", nMaxDisplayName );
		return false;
	}

	for ( int i = 0; i < len; i++ )
	{
		char c = originalName[i];
		if ( !( c >= 'a' && c <= 'z' ) && !( c >= '0' && c <= '9' ) && c != '_' )
		{
			TFWorkshopWarning( "Invalid character %c in map name\n", c );
			return false;
		}

		if ( c == '_' && ( i == 0 || i == len - 1 || originalName[ i - 1 ] == '_' ) )
		{
			TFWorkshopWarning( "Invalid map name: _ cannot appear consecutively nor at the beginning/end of a map name\n" );
			return false;
		}
	}

	return true;
}

#endif // TF_MAPS_WORKSHOP_H
