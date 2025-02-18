//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef EIFACE_H
#define EIFACE_H

#ifdef _WIN32
#pragma once
#endif

#include "convar.h"
#include "icvar.h"
#include "edict.h"
#include "mathlib/vplane.h"
#include "iserverentity.h"
#include "engine/ivmodelinfo.h"
#include "soundflags.h"
#include "bitvec.h"
#include "engine/iserverplugin.h"
#include "tier1/bitbuf.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class	SendTable;
class	ServerClass;
class	IMoveHelper;
struct  Ray_t;
class	CGameTrace;
typedef	CGameTrace trace_t;
struct	typedescription_t;
class	CSaveRestoreData;
struct	datamap_t;
class	SendTable;
class	ServerClass;
class	IMoveHelper;
struct  Ray_t;
struct	studiohdr_t;
class	CBaseEntity;
class	CRestore;
class	CSave;
class	variant_t;
struct	vcollide_t;
class	IRecipientFilter;
class	CBaseEntity;
class	ITraceFilter;
struct	client_textmessage_t;
class	INetChannelInfo;
class	ISpatialPartition;
class IScratchPad3D;
class CStandardSendProxies;
class IAchievementMgr;
class CGamestatsData;
class CSteamID;
class IReplayFactory;
class IReplaySystem;
class IServer;

typedef struct player_info_s player_info_t;

//-----------------------------------------------------------------------------
// defines
//-----------------------------------------------------------------------------

#ifdef _WIN32
#define DLLEXPORT __stdcall
#else
#define DLLEXPORT /* */
#endif

#define INTERFACEVERSION_VENGINESERVER_VERSION_21	"VEngineServer021"
#define INTERFACEVERSION_VENGINESERVER_VERSION_22	"VEngineServer022"
#define INTERFACEVERSION_VENGINESERVER				"VEngineServer023"
#define INTERFACEVERSION_VENGINESERVER_INT			23

struct bbox_t
{
	Vector mins;
	Vector maxs;
};

struct WorkshopMapDesc_t
{
	char	szMapName[MAX_PATH];
	char	szOriginalMapName[MAX_PATH];
	uint32	uTimestamp;
	bool	bDownloaded;
};

//-----------------------------------------------------------------------------
// Purpose: Interface the engine exposes to the game DLL
//-----------------------------------------------------------------------------
abstract_class IVEngineServer
{
public:
	// Tell engine to change level ( "changelevel s1\n" or "changelevel2 s1 s2\n" )
	virtual void		ChangeLevel( const char *s1, const char *s2 ) = 0;

	// Ask engine whether the specified map is a valid map file (exists and has valid version number).
	virtual int			IsMapValid( const char *filename ) = 0;

	// Is this a dedicated server?
	virtual bool		IsDedicatedServer( void ) = 0;

	// Is in Hammer editing mode?
	virtual int			IsInEditMode( void ) = 0;

	// Add to the server/client lookup/precache table, the specified string is given a unique index
	// NOTE: The indices for PrecacheModel are 1 based
	//  a 0 returned from those methods indicates the model or sound was not correctly precached
	// However, generic and decal are 0 based
	// If preload is specified, the file is loaded into the server/client's cache memory before level startup, otherwise
	//  it'll only load when actually used (which can cause a disk i/o hitch if it occurs during play of a level).
	virtual int			PrecacheModel( const char *s, bool preload = false ) = 0;
	virtual int			PrecacheSentenceFile( const char *s, bool preload = false ) = 0;
	virtual int			PrecacheDecal( const char *name, bool preload = false ) = 0;
	virtual int			PrecacheGeneric( const char *s, bool preload = false ) = 0;

	// Check's if the name is precached, but doesn't actually precache the name if not...
	virtual bool		IsModelPrecached( char const *s ) const = 0;
	virtual bool		IsDecalPrecached( char const *s ) const = 0;
	virtual bool		IsGenericPrecached( char const *s ) const = 0;

	// Note that sounds are precached using the IEngineSound interface

	// Special purpose PVS checking
	// Get the cluster # for the specified position
	virtual int			GetClusterForOrigin( const Vector &org ) = 0;
	// Get the PVS bits for a specified cluster and copy the bits into outputpvs.  Returns the number of bytes needed to pack the PVS
	virtual int			GetPVSForCluster( int cluster, int outputpvslength, unsigned char *outputpvs ) = 0;
	// Check whether the specified origin is inside the specified PVS
	virtual bool		CheckOriginInPVS( const Vector &org, const unsigned char *checkpvs, int checkpvssize ) = 0;
	// Check whether the specified worldspace bounding box is inside the specified PVS
	virtual bool		CheckBoxInPVS( const Vector &mins, const Vector &maxs, const unsigned char *checkpvs, int checkpvssize ) = 0;

	// Returns the server assigned userid for this player.  Useful for logging frags, etc.  
	//  returns -1 if the edict couldn't be found in the list of players.
	virtual int			GetPlayerUserId( const edict_t *e ) = 0; 
	virtual const char	*GetPlayerNetworkIDString( const edict_t *e ) = 0;

	// Return the current number of used edict slots
	virtual int			GetEntityCount( void ) = 0;
	// Given an edict, returns the entity index
	virtual int			IndexOfEdict( const edict_t *pEdict ) = 0;
	// Given and entity index, returns the corresponding edict pointer
	virtual edict_t		*PEntityOfEntIndex( int iEntIndex ) = 0;
	
	// Get stats info interface for a client netchannel
	virtual INetChannelInfo* GetPlayerNetInfo( int playerIndex ) = 0;
	
	// Allocate space for string and return index/offset of string in global string list
	// If iForceEdictIndex is not -1, then it will return the edict with that index. If that edict index
	// is already used, it'll return null.
	virtual edict_t		*CreateEdict( int iForceEdictIndex = -1 ) = 0;
	// Remove the specified edict and place back into the free edict list
	virtual void		RemoveEdict( edict_t *e ) = 0;
	
	// Memory allocation for entity class data
	virtual void		*PvAllocEntPrivateData( long cb ) = 0;
	virtual void		FreeEntPrivateData( void *pEntity ) = 0;

	// Save/restore uses a special memory allocator (which zeroes newly allocated memory, etc.)
	virtual void		*SaveAllocMemory( size_t num, size_t size ) = 0;
	virtual void		SaveFreeMemory( void *pSaveMem ) = 0;
	
	// Emit an ambient sound associated with the specified entity
	virtual void		EmitAmbientSound( int entindex, const Vector &pos, const char *samp, float vol, soundlevel_t soundlevel, int fFlags, int pitch, float delay = 0.0f ) = 0;

	// Fade out the client's volume level toward silence (or fadePercent)
	virtual void        FadeClientVolume( const edict_t *pEdict, float fadePercent, float fadeOutSeconds, float holdTime, float fadeInSeconds ) = 0;
	
	// Sentences / sentence groups
	virtual int			SentenceGroupPick( int groupIndex, char *name, int nameBufLen ) = 0;
	virtual int			SentenceGroupPickSequential( int groupIndex, char *name, int nameBufLen, int sentenceIndex, int reset ) = 0;
	virtual int			SentenceIndexFromName( const char *pSentenceName ) = 0;
	virtual const char *SentenceNameFromIndex( int sentenceIndex ) = 0;
	virtual int			SentenceGroupIndexFromName( const char *pGroupName ) = 0;
	virtual const char *SentenceGroupNameFromIndex( int groupIndex ) = 0;
	virtual float		SentenceLength( int sentenceIndex ) = 0;

	// Issue a command to the command parser as if it was typed at the server console.	
	virtual void		ServerCommand( const char *str ) = 0;
	// Execute any commands currently in the command parser immediately (instead of once per frame)
	virtual void		ServerExecute( void ) = 0;
	// Issue the specified command to the specified client (mimics that client typing the command at the console).
	virtual void		ClientCommand( edict_t *pEdict, PRINTF_FORMAT_STRING const char *szFmt, ... ) = 0;

	// Set the lightstyle to the specified value and network the change to any connected clients.  Note that val must not 
	//  change place in memory (use MAKE_STRING) for anything that's not compiled into your mod.
	virtual void		LightStyle( int style, const char *val ) = 0;

	// Project a static decal onto the specified entity / model (for level placed decals in the .bsp)
	virtual void		StaticDecal( const Vector &originInEntitySpace, int decalIndex, int entityIndex, int modelIndex, bool lowpriority ) = 0;
	
	// Given the current PVS(or PAS) and origin, determine which players should hear/receive the message
	virtual void		Message_DetermineMulticastRecipients( bool usepas, const Vector& origin, CBitVec< ABSOLUTE_PLAYER_LIMIT >& playerbits ) = 0;

	// Begin a message from a server side entity to its client side counterpart (func_breakable glass, e.g.)
	virtual bf_write	*EntityMessageBegin( int ent_index, ServerClass * ent_class, bool reliable ) = 0;
	// Begin a usermessage from the server to the client .dll
	virtual bf_write	*UserMessageBegin( IRecipientFilter *filter, int msg_type ) = 0;
	// Finish the Entity or UserMessage and dispatch to network layer
	virtual void		MessageEnd( void ) = 0;

	// Print szMsg to the client console.
	virtual void		ClientPrintf( edict_t *pEdict, const char *szMsg ) = 0;

	// SINGLE PLAYER/LISTEN SERVER ONLY (just matching the client .dll api for this)
	// Prints the formatted string to the notification area of the screen ( down the right hand edge
	//  numbered lines starting at position 0
	virtual void		Con_NPrintf( int pos, PRINTF_FORMAT_STRING const char *fmt, ... ) = 0;
	// SINGLE PLAYER/LISTEN SERVER ONLY(just matching the client .dll api for this)
	// Similar to Con_NPrintf, but allows specifying custom text color and duration information
	virtual void		Con_NXPrintf( const struct con_nprint_s *info, PRINTF_FORMAT_STRING const char *fmt, ... ) = 0;

	// Change a specified player's "view entity" (i.e., use the view entity position/orientation for rendering the client view)
	virtual void		SetView( const edict_t *pClient, const edict_t *pViewent ) = 0;

	// Get a high precision timer for doing profiling work
	virtual float		Time( void ) = 0;

	// Set the player's crosshair angle
	virtual void		CrosshairAngle( const edict_t *pClient, float pitch, float yaw ) = 0;

	// Get the current game directory (hl2, tf2, hl1, cstrike, etc.)
	virtual void        GetGameDir( char *szGetGameDir, int maxlength ) = 0;

	// Used by AI node graph code to determine if .bsp and .ain files are out of date
	virtual int 		CompareFileTime( const char *filename1, const char *filename2, int *iCompare ) = 0;

	// Locks/unlocks the network string tables (.e.g, when adding bots to server, this needs to happen).
	// Be sure to reset the lock after executing your code!!!
	virtual bool		LockNetworkStringTables( bool lock ) = 0;

	// Create a bot with the given name.  Returns NULL if fake client can't be created
	virtual edict_t		*CreateFakeClient( const char *netname ) = 0;

	// Get a convar keyvalue for s specified client
	virtual const char	*GetClientConVarValue( int clientIndex, const char *name ) = 0;
	
	// Parse a token from a file
	virtual const char	*ParseFile( const char *data, char *token, int maxlen ) = 0;
	// Copies a file
	virtual bool		CopyFile( const char *source, const char *destination ) = 0;

	// Reset the pvs, pvssize is the size in bytes of the buffer pointed to by pvs.
	// This should be called right before any calls to AddOriginToPVS
	virtual void		ResetPVS( byte *pvs, int pvssize ) = 0;
	// Merge the pvs bits into the current accumulated pvs based on the specified origin ( not that each pvs origin has an 8 world unit fudge factor )
	virtual void		AddOriginToPVS( const Vector &origin ) = 0;
	
	// Mark a specified area portal as open/closed.
	// Use SetAreaPortalStates if you want to set a bunch of them at a time.
	virtual void		SetAreaPortalState( int portalNumber, int isOpen ) = 0;
	
	// Queue a temp entity for transmission
	virtual void		PlaybackTempEntity( IRecipientFilter& filter, float delay, const void *pSender, const SendTable *pST, int classID  ) = 0;
	// Given a node number and the specified PVS, return with the node is in the PVS
	virtual int			CheckHeadnodeVisible( int nodenum, const byte *pvs, int vissize ) = 0;
	// Using area bits, cheeck whether area1 flows into area2 and vice versa (depends on area portal state)
	virtual int			CheckAreasConnected( int area1, int area2 ) = 0;
	// Given an origin, determine which area index the origin is within
	virtual int			GetArea( const Vector &origin ) = 0;
	// Get area portal bit set
	virtual void		GetAreaBits( int area, unsigned char *bits, int buflen ) = 0;
	// Given a view origin (which tells us the area to start looking in) and a portal key,
	// fill in the plane that leads out of this area (it points into whatever area it leads to).
	virtual bool		GetAreaPortalPlane( Vector const &vViewOrigin, int portalKey, VPlane *pPlane ) = 0;

	// Save/restore wrapper - FIXME:  At some point we should move this to it's own interface
	virtual bool		LoadGameState( char const *pMapName, bool createPlayers ) = 0;
	virtual void		LoadAdjacentEnts( const char *pOldLevel, const char *pLandmarkName ) = 0;
	virtual void		ClearSaveDir() = 0;

	// Get the pristine map entity lump string.  (e.g., used by CS to reload the map entities when restarting a round.)
	virtual const char*	GetMapEntitiesString() = 0;

	// Text message system -- lookup the text message of the specified name
	virtual client_textmessage_t *TextMessageGet( const char *pName ) = 0;

	// Print a message to the server log file
	virtual void		LogPrint( const char *msg ) = 0;

	// Builds PVS information for an entity
	virtual void		BuildEntityClusterList( edict_t *pEdict, PVSInfo_t *pPVSInfo ) = 0;

	// A solid entity moved, update spatial partition
	virtual void SolidMoved( edict_t *pSolidEnt, ICollideable *pSolidCollide, const Vector* pPrevAbsOrigin, bool testSurroundingBoundsOnly ) = 0;
	// A trigger entity moved, update spatial partition
	virtual void TriggerMoved( edict_t *pTriggerEnt, bool testSurroundingBoundsOnly ) = 0;
	
	// Create/destroy a custom spatial partition
	virtual ISpatialPartition *CreateSpatialPartition( const Vector& worldmin, const Vector& worldmax ) = 0;
	virtual void 		DestroySpatialPartition( ISpatialPartition * ) = 0;

	// Draw the brush geometry in the map into the scratch pad.
	// Flags is currently unused.
	virtual void		DrawMapToScratchPad( IScratchPad3D *pPad, unsigned long iFlags ) = 0;

	// This returns which entities, to the best of the server's knowledge, the client currently knows about.
	// This is really which entities were in the snapshot that this client last acked.
	// This returns a bit vector with one bit for each entity.
	//
	// USE WITH CARE. Whatever tick the client is really currently on is subject to timing and
	// ordering differences, so you should account for about a quarter-second discrepancy in here.
	// Also, this will return NULL if the client doesn't exist or if this client hasn't acked any frames yet.
	// 
	// iClientIndex is the CLIENT index, so if you use pPlayer->entindex(), subtract 1.
	virtual const CBitVec<MAX_EDICTS>* GetEntityTransmitBitsForClient( int iClientIndex ) = 0;
	
	// Is the game paused?
	virtual bool		IsPaused() = 0;
	
	// Marks the filename for consistency checking.  This should be called after precaching the file.
	virtual void		ForceExactFile( const char *s ) = 0;
	virtual void		ForceModelBounds( const char *s, const Vector &mins, const Vector &maxs ) = 0;
	virtual void		ClearSaveDirAfterClientLoad() = 0;

	// Sets a USERINFO client ConVar for a fakeclient
	virtual void		SetFakeClientConVarValue( edict_t *pEntity, const char *cvar, const char *value ) = 0;
	
	// Marks the material (vmt file) for consistency checking.  If the client and server have different
	// contents for the file, the client's vmt can only use the VertexLitGeneric shader, and can only
	// contain $baseTexture and $bumpmap vars.
	virtual void		ForceSimpleMaterial( const char *s ) = 0;

	// Is the engine in Commentary mode?
	virtual int			IsInCommentaryMode( void ) = 0;
	

	// Mark some area portals as open/closed. It's more efficient to use this
	// than a bunch of individual SetAreaPortalState calls.
	virtual void		SetAreaPortalStates( const int *portalNumbers, const int *isOpen, int nPortals ) = 0;

	// Called when relevant edict state flags change.
	virtual void		NotifyEdictFlagsChange( int iEdict ) = 0;
	
	// Only valid during CheckTransmit. Also, only the PVS, networked areas, and
	// m_pTransmitInfo are valid in the returned strucutre.
	virtual const CCheckTransmitInfo* GetPrevCheckTransmitInfo( edict_t *pPlayerEdict ) = 0;
	
	virtual CSharedEdictChangeInfo* GetSharedEdictChangeInfo() = 0;

	// Tells the engine we can immdiately re-use all edict indices
	// even though we may not have waited enough time
	virtual void			AllowImmediateEdictReuse( ) = 0;	

	// Returns true if the engine is an internal build. i.e. is using the internal bugreporter.
	virtual bool		IsInternalBuild( void ) = 0;

	virtual IChangeInfoAccessor *GetChangeAccessor( const edict_t *pEdict ) = 0;	

	// Name of most recently load .sav file
	virtual char const *GetMostRecentlyLoadedFileName() = 0;
	virtual char const *GetSaveFileName() = 0;

	// Matchmaking
	virtual void MultiplayerEndGame() = 0;
	virtual void ChangeTeam( const char *pTeamName ) = 0;

	// Cleans up the cluster list
	virtual void CleanUpEntityClusterList( PVSInfo_t *pPVSInfo ) = 0;

	virtual void SetAchievementMgr( IAchievementMgr *pAchievementMgr ) =0;
	virtual IAchievementMgr *GetAchievementMgr() = 0;

	virtual int	GetAppID() = 0;
	
	virtual bool IsLowViolence() = 0;
	
	// Call this to find out the value of a cvar on the client.
	//
	// It is an asynchronous query, and it will call IServerGameDLL::OnQueryCvarValueFinished when
	// the value comes in from the client.
	//
	// Store the return value if you want to match this specific query to the OnQueryCvarValueFinished call.
	// Returns InvalidQueryCvarCookie if the entity is invalid.
	virtual QueryCvarCookie_t StartQueryCvarValue( edict_t *pPlayerEntity, const char *pName ) = 0;

	virtual void InsertServerCommand( const char *str ) = 0;

	// Fill in the player info structure for the specified player index (name, model, etc.)
	virtual bool GetPlayerInfo( int ent_num, player_info_t *pinfo ) = 0;

	// Returns true if this client has been fully authenticated by Steam
	virtual bool IsClientFullyAuthenticated( edict_t *pEdict ) = 0;

	// This makes the host run 1 tick per frame instead of checking the system timer to see how many ticks to run in a certain frame.
	// i.e. it does the same thing timedemo does.
	virtual void SetDedicatedServerBenchmarkMode( bool bBenchmarkMode ) = 0;

	// Methods to set/get a gamestats data container so client & server running in same process can send combined data
	virtual void SetGamestatsData( CGamestatsData *pGamestatsData ) = 0;
	virtual CGamestatsData *GetGamestatsData() = 0;

	// Returns the SteamID of the specified player. It'll be NULL if the player hasn't authenticated yet.
	virtual const CSteamID	*GetClientSteamID( edict_t *pPlayerEdict ) = 0;

	// Returns the SteamID of the game server
	virtual const CSteamID	*GetGameServerSteamID() = 0;

	// Send a client command keyvalues
	// keyvalues are deleted inside the function
	virtual void ClientCommandKeyValues( edict_t *pEdict, KeyValues *pCommand ) = 0;

	// Returns the SteamID of the specified player. It'll be NULL if the player hasn't authenticated yet.
	virtual const CSteamID	*GetClientSteamIDByPlayerIndex( int entnum ) = 0;
	// Gets a list of all clusters' bounds.  Returns total number of clusters.
	virtual int GetClusterCount() = 0;
	virtual int GetAllClusterBounds( bbox_t *pBBoxList, int maxBBox ) = 0;

	// Create a bot with the given name.  Returns NULL if fake client can't be created
	virtual edict_t		*CreateFakeClientEx( const char *netname, bool bReportFakeClient = true ) = 0;

	// Server version from the steam.inf, this will be compared to the GC version
	virtual int GetServerVersion() const = 0;

	// Get sv.GetTime()
	virtual float GetServerTime() const = 0;

	// Exposed for server plugin authors
	virtual IServer *GetIServer() = 0;

	virtual bool IsPlayerNameLocked( const edict_t *pEdict ) = 0;
	virtual bool CanPlayerChangeName( const edict_t *pEdict ) = 0;

	// Find the canonical name of a map, given a partial or non-canonical map name.
	// Except in the case of an exact match, pMapName is updated to the canonical name of the match.
	// NOTE That this is subject to the same limitation as ServerGameDLL::CanProvideLevel -- This is non-blocking, so it
	//      is possible that blocking ServerGameDLL::PrepareLevelResources call may be able to pull a better match than
	//      is immediately available to this call (e.g. blocking lookups of cloud maps)
	enum eFindMapResult {
		// A direct match for this name was found
		eFindMap_Found,
		// No match for this map name could be found.
		eFindMap_NotFound,
		// A fuzzy match for this mapname was found and pMapName was updated to the full name.
		// Ex: cp_dust -> cp_dustbowl
		eFindMap_FuzzyMatch,
		// A match for this map name was found, and the map name was updated to the canonical version of the
		// name.
		// Ex: workshop/1234 -> workshop/cp_qualified_name.ugc1234
		eFindMap_NonCanonical,
		// No currently available match for this map name could be found, but it may be possible to load ( see caveat
		// about PrepareLevelResources above )
		eFindMap_PossiblyAvailable
	};
	virtual eFindMapResult FindMap( /* in/out */ char *pMapName, int nMapNameMax ) = 0;
	
	virtual void SetPausedForced( bool bPaused, float flDuration = -1.f ) = 0;
};

// These only differ in new items added to the end
typedef IVEngineServer IVEngineServer021;
typedef IVEngineServer IVEngineServer022;


#define INTERFACEVERSION_SERVERGAMEDLL_VERSION_8	"ServerGameDLL008"
#define INTERFACEVERSION_SERVERGAMEDLL_VERSION_9	"ServerGameDLL009"
#define INTERFACEVERSION_SERVERGAMEDLL_VERSION_10	"ServerGameDLL010"
#define INTERFACEVERSION_SERVERGAMEDLL_VERSION_11	"ServerGameDLL011"
#define INTERFACEVERSION_SERVERGAMEDLL				"ServerGameDLL012"
#define INTERFACEVERSION_SERVERGAMEDLL_INT			12

class IServerGCLobby;

//-----------------------------------------------------------------------------
// Purpose: These are the interfaces that the game .dll exposes to the engine
//-----------------------------------------------------------------------------
abstract_class IServerGameDLL
{
public:
	// Initialize the game (one-time call when the DLL is first loaded )
	// Return false if there is an error during startup.
	virtual bool			DLLInit(	CreateInterfaceFn engineFactory, 
										CreateInterfaceFn physicsFactory, 
										CreateInterfaceFn fileSystemFactory, 
										CGlobalVars *pGlobals) = 0;

	// Setup replay interfaces on the server
	virtual bool			ReplayInit( CreateInterfaceFn fnReplayFactory ) = 0;

	// This is called when a new game is started. (restart, map)
	virtual bool			GameInit( void ) = 0;

	// Called any time a new level is started (after GameInit() also on level transitions within a game)
	virtual bool			LevelInit( char const *pMapName, 
									char const *pMapEntities, char const *pOldLevel, 
									char const *pLandmarkName, bool loadGame, bool background ) = 0;

	// The server is about to activate
	virtual void			ServerActivate( edict_t *pEdictList, int edictCount, int clientMax ) = 0;

	// The server should run physics/think on all edicts
	virtual void			GameFrame( bool simulating ) = 0;

	// Called once per simulation frame on the final tick
	virtual void			PreClientUpdate( bool simulating ) = 0;

	// Called when a level is shutdown (including changing levels)
	virtual void			LevelShutdown( void ) = 0;
	// This is called when a game ends (server disconnect, death, restart, load)
	// NOT on level transitions within a game
	virtual void			GameShutdown( void ) = 0;

	// Called once during DLL shutdown
	virtual void			DLLShutdown( void ) = 0;

	// Get the simulation interval (must be compiled with identical values into both client and game .dll for MOD!!!)
	// Right now this is only requested at server startup time so it can't be changed on the fly, etc.
	virtual float			GetTickInterval( void ) const = 0;

	// Give the list of datatable classes to the engine.  The engine matches class names from here with
	//  edict_t::classname to figure out how to encode a class's data for networking
	virtual ServerClass*	GetAllServerClasses( void ) = 0;

	// Returns string describing current .dll.  e.g., TeamFortress 2, Half-Life 2.  
	//  Hey, it's more descriptive than just the name of the game directory
	virtual const char     *GetGameDescription( void ) = 0;      
	
	// Let the game .dll allocate it's own network/shared string tables
	virtual void			CreateNetworkStringTables( void ) = 0;
	
	// Save/restore system hooks
	virtual CSaveRestoreData  *SaveInit( int size ) = 0;
	virtual void			SaveWriteFields( CSaveRestoreData *, const char *, void *, datamap_t *, typedescription_t *, int ) = 0;
	virtual void			SaveReadFields( CSaveRestoreData *, const char *, void *, datamap_t *, typedescription_t *, int ) = 0;
	virtual void			SaveGlobalState( CSaveRestoreData * ) = 0;
	virtual void			RestoreGlobalState( CSaveRestoreData * ) = 0;
	virtual void			PreSave( CSaveRestoreData * ) = 0;
	virtual void			Save( CSaveRestoreData * ) = 0;
	virtual void			GetSaveComment( char *comment, int maxlength, float flMinutes, float flSeconds, bool bNoTime = false ) = 0;
	virtual void			WriteSaveHeaders( CSaveRestoreData * ) = 0;
	virtual void			ReadRestoreHeaders( CSaveRestoreData * ) = 0;
	virtual void			Restore( CSaveRestoreData *, bool ) = 0;
	virtual bool			IsRestoring() = 0;

	// Returns the number of entities moved across the transition
	virtual int				CreateEntityTransitionList( CSaveRestoreData *, int ) = 0;
	// Build the list of maps adjacent to the current map
	virtual void			BuildAdjacentMapList( void ) = 0;

	// Retrieve info needed for parsing the specified user message
	virtual bool			GetUserMessageInfo( int msg_type, char *name, int maxnamelength, int& size ) = 0;

	// Hand over the StandardSendProxies in the game DLL's module.
	virtual CStandardSendProxies*	GetStandardSendProxies() = 0;

	// Called once during startup, after the game .dll has been loaded and after the client .dll has also been loaded
	virtual void			PostInit() = 0;
	// Called once per frame even when no level is loaded...
	virtual void			Think( bool finalTick ) = 0;

#ifdef _XBOX
	virtual void			GetTitleName( const char *pMapName, char* pTitleBuff, int titleBuffSize ) = 0;
#endif

	virtual void			PreSaveGameLoaded( char const *pSaveName, bool bCurrentlyInGame ) = 0;

	// Returns true if the game DLL wants the server not to be made public.
	// Used by commentary system to hide multiplayer commentary servers from the master.
	virtual bool			ShouldHideServer( void ) = 0;

	virtual void			InvalidateMdlCache() = 0;

	// * This function is new with version 6 of the interface.
	//
	// This is called when a query from IServerPluginHelpers::StartQueryCvarValue is finished.
	// iCookie is the value returned by IServerPluginHelpers::StartQueryCvarValue.
	// Added with version 2 of the interface.
	virtual void			OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue ) = 0;

	// Called after the steam API has been activated post-level startup
	virtual void			GameServerSteamAPIActivated( void ) = 0;

	// Called after the steam API has been shutdown post-level startup
	virtual void			GameServerSteamAPIShutdown( void ) = 0;

	virtual void			SetServerHibernation( bool bHibernating ) = 0;

	// interface to the new GC based lobby system
	virtual IServerGCLobby *GetServerGCLobby() = 0;

	// Return override string to show in the server browser
	// "map" column, or NULL to just use the default value
	// (the map name)
	virtual const char *GetServerBrowserMapOverride() = 0;

	// Get gamedata string to send to the master serer updater.
	virtual const char *GetServerBrowserGameData() = 0;

	// Called to add output to the status command
	virtual void 			Status( void (*print) (const char *fmt, ...) ) = 0;

	// Informs the game we would like to load this level, giving it a chance to prepare dynamic resources.
	//
	// - pszMapName is the name of the map we're looking for, and may be overridden to e.g. the canonical name of the
	//   map.
	//
	// - pszMapFile is the file we intend to use for this map ( e.g. maps/<mapname>.bsp ), and may be overridden to the
	//   file representing this map name. ( e.g. /path/to/steamapps/workshop/cp_mymap.ugc12345.bsp )
	//
	// This call is blocking, and may block for extended periods. See AsyncPrepareLevelResources below.
	virtual void PrepareLevelResources( /* in/out */ char *pszMapName, size_t nMapNameSize,
	                                    /* in/out */ char *pszMapFile, size_t nMapFileSize ) = 0;

	// Asynchronous version of PrepareLevelResources. Returns preparation status of map when called.
	// If passed, flProgress is filled with the current progress percentage [ 0.f to 1.f ] for the InProgress
	// result
	enum ePrepareLevelResourcesResult
	{
		// Good to go
		ePrepareLevelResources_Prepared,
		// Game DLL is async preparing (e.g. streaming resources). flProgress will be filled if passed.
		ePrepareLevelResources_InProgress
	};
	virtual ePrepareLevelResourcesResult AsyncPrepareLevelResources( /* in/out */ char *pszMapName, size_t nMapNameSize,
	                                                                 /* in/out */ char *pszMapFile, size_t nMapFileSize,
	                                                                 float *flProgress = NULL ) = 0;

	// Ask the game DLL to evaluate what it would do with this map name were it passed to PrepareLevelResources.
	// NOTE That this is this is syncronous and non-blocking, so it is possible that async PrepareLevelResources call
	//      may be able to pull a better match than is immediately available to this call (e.g. blocking lookups of
	//      cloud maps)
	enum eCanProvideLevelResult {
		// Have no knowledge of this level name, it will be up to the engine to provide. (e.g. as maps/levelname.bsp)
		eCanProvideLevel_CannotProvide,
		// Can provide resources for this level, and pMapName has been updated to the canonical name we would provide it
		// under (as with PrepareLevelResources)
		eCanProvideLevel_CanProvide,
		// We recognize this level name as something we might be able to prepare, but without a blocking/async call to
		// PrepareLevelResources, it is not possible to say whether it is available.
		eCanProvideLevel_Possibly
	};
	virtual eCanProvideLevelResult CanProvideLevel( /* in/out */ char *pMapName, int nMapNameMax ) = 0;

	// Called to see if the game server is okay with a manual changelevel or map command
	virtual bool			IsManualMapChangeOkay( const char **pszReason ) = 0;

	// Josh: Allows the engine over all workshop maps and get some information about them.
	// Used primarily for listmaps code.
	// Returns true if uIndex was valid, false if invalid.
	virtual bool			GetWorkshopMap( uint32 uIndex, WorkshopMapDesc_t *pDesc ) = 0;
};

typedef IServerGameDLL IServerGameDLL008;

//-----------------------------------------------------------------------------
// Just an interface version name for the random number interface
// See vstdlib/random.h for the interface definition
// NOTE: If you change this, also change VENGINE_CLIENT_RANDOM_INTERFACE_VERSION in cdll_int.h
//-----------------------------------------------------------------------------
#define VENGINE_SERVER_RANDOM_INTERFACE_VERSION	"VEngineRandom001"

#define INTERFACEVERSION_SERVERGAMEENTS			"ServerGameEnts001"
//-----------------------------------------------------------------------------
// Purpose: Interface to get at server entities
//-----------------------------------------------------------------------------
abstract_class IServerGameEnts
{
public:
	virtual					~IServerGameEnts()	{}

	// Only for debugging. Set the edict base so you can get an edict's index in the debugger while debugging the game .dll
	virtual void			SetDebugEdictBase(edict_t *base) = 0;
	
	// The engine wants to mark two entities as touching
	virtual void			MarkEntitiesAsTouching( edict_t *e1, edict_t *e2 ) = 0;

	// Frees the entity attached to this edict
	virtual void			FreeContainingEntity( edict_t * ) = 0; 

	// This allows the engine to get at edicts in a CGameTrace.
	virtual edict_t*		BaseEntityToEdict( CBaseEntity *pEnt ) = 0;
	virtual CBaseEntity*	EdictToBaseEntity( edict_t *pEdict ) = 0;

	// This sets a bit in pInfo for each edict in the list that wants to be transmitted to the 
	// client specified in pInfo.
	//
	// This is also where an entity can force other entities to be transmitted if it refers to them
	// with ehandles.
	virtual void			CheckTransmit( CCheckTransmitInfo *pInfo, const unsigned short *pEdictIndices, int nEdicts ) = 0;
};

#define INTERFACEVERSION_SERVERGAMECLIENTS_VERSION_3	"ServerGameClients003"
#define INTERFACEVERSION_SERVERGAMECLIENTS_VERSION_4	"ServerGameClients004"
#define INTERFACEVERSION_SERVERGAMECLIENTS				"ServerGameClients005"

//-----------------------------------------------------------------------------
// Purpose: Player / Client related functions
//-----------------------------------------------------------------------------
abstract_class IServerGameClients
{
public:
	// Get server maxplayers and lower bound for same
	virtual void			GetPlayerLimits( int& minplayers, int& maxplayers, int &defaultMaxPlayers ) const = 0;

	// Client is connecting to server ( return false to reject the connection )
	//	You can specify a rejection message by writing it into reject
	virtual bool			ClientConnect( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen ) = 0;

	// Client is going active
	// If bLoadGame is true, don't spawn the player because its state is already setup.
	virtual void			ClientActive( edict_t *pEntity, bool bLoadGame ) = 0;
	
	// Client is disconnecting from server
	virtual void			ClientDisconnect( edict_t *pEntity ) = 0;
	
	// Client is connected and should be put in the game
	virtual void			ClientPutInServer( edict_t *pEntity, char const *playername ) = 0;
	
	// The client has typed a command at the console
	virtual void			ClientCommand( edict_t *pEntity, const CCommand &args ) = 0;

	// Sets the client index for the client who typed the command into his/her console
	virtual void			SetCommandClient( int index ) = 0;
	
	// A player changed one/several replicated cvars (name etc)
	virtual void			ClientSettingsChanged( edict_t *pEdict ) = 0;
	
	// Determine PVS origin and set PVS for the player/viewentity
	virtual void			ClientSetupVisibility( edict_t *pViewEntity, edict_t *pClient, unsigned char *pvs, int pvssize ) = 0;
	
	// A block of CUserCmds has arrived from the user, decode them and buffer for execution during player simulation
	virtual float			ProcessUsercmds( edict_t *player, bf_read *buf, int numcmds, int totalcmds,
								int dropped_packets, bool ignore, bool paused ) = 0;
	
	// Let the game .dll do stuff after messages have been sent to all of the clients once the server frame is complete
	virtual void			PostClientMessagesSent_DEPRECIATED( void ) = 0;

	// For players, looks up the CPlayerState structure corresponding to the player
	virtual CPlayerState	*GetPlayerState( edict_t *player ) = 0;

	// Get the ear position for a specified client
	virtual void			ClientEarPosition( edict_t *pEntity, Vector *pEarOrigin ) = 0;

	// returns number of delay ticks if player is in Replay mode (0 = no delay)
	virtual int				GetReplayDelay( edict_t *player, int& entity ) = 0;

	// Anything this game .dll wants to add to the bug reporter text (e.g., the entity/model under the picker crosshair)
	//  can be added here
	virtual void			GetBugReportInfo( char *buf, int buflen ) = 0;

	// A user has had their network id setup and validated 
	virtual void			NetworkIDValidated( const char *pszUserName, const char *pszNetworkID ) = 0;

	// The client has submitted a keyvalues command
	virtual void			ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues ) = 0;

	// Hook for player spawning
	virtual void			ClientSpawned( edict_t *pPlayer ) = 0;

	// Hook for player voice
	virtual void			ClientVoice( edict_t *pPlayer ) = 0;
};

typedef IServerGameClients IServerGameClients003;
typedef IServerGameClients IServerGameClients004;

#define INTERFACEVERSION_UPLOADGAMESTATS		"ServerUploadGameStats001"

abstract_class IUploadGameStats
{
public:
	// Note that this call will block the server until the upload is completed, so use only at levelshutdown if at all.
	virtual bool UploadGameStats( 
		char const *mapname,				// Game map name
		unsigned int blobversion,			// Version of the binary blob data
		unsigned int blobsize,				// Size in bytes of blob data
		const void *pvBlobData ) = 0;		// Pointer to the blob data.

	// Call when created to init the CSER connection
	virtual void InitConnection( void ) = 0;

	// Call periodically to poll steam for a CSER connection
	virtual void UpdateConnection( void ) = 0;

	// If user has disabled stats tracking, do nothing
	virtual bool IsGameStatsLoggingEnabled() = 0;

	// Gets a non-personally identifiable unique ID for this steam user, used for tracking total gameplay time across
	//  multiple stats sessions, but isn't trackable back to their Steam account or id.
	// Buffer should be 16 bytes, ID will come back as a hexadecimal string version of a GUID
	virtual void GetPseudoUniqueId( char *buf, size_t bufsize ) = 0;

	// For determining general % of users running using cyber cafe accounts...
	virtual bool IsCyberCafeUser( void ) = 0;

	// Only works in single player
	virtual bool IsHDREnabled( void ) = 0;
};

#define INTERFACEVERSION_PLUGINHELPERSCHECK		"PluginHelpersCheck001"

//-----------------------------------------------------------------------------
// Purpose: allows the game dll to control which plugin functions can be run
//-----------------------------------------------------------------------------
abstract_class IPluginHelpersCheck
{
public:
	virtual bool CreateMessage( const char *plugin, edict_t *pEntity, DIALOG_TYPE type, KeyValues *data ) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: Interface exposed from the client .dll back to the engine for specifying shared .dll IAppSystems (e.g., ISoundEmitterSystem)
//-----------------------------------------------------------------------------
abstract_class IServerDLLSharedAppSystems
{
public:
	virtual int	Count() = 0;
	virtual char const *GetDllName( int idx ) = 0;
	virtual char const *GetInterfaceName( int idx ) = 0;
};

#define SERVER_DLL_SHARED_APPSYSTEMS		"VServerDllSharedAppSystems001"

#define INTERFACEVERSION_SERVERGAMETAGS		"ServerGameTags001"

//-----------------------------------------------------------------------------
// Purpose: querying the game dll for Server cvar tags
//-----------------------------------------------------------------------------
abstract_class IServerGameTags
{
public:
	// Get the list of cvars that require tags to show differently in the server browser
	virtual void			GetTaggedConVarList( KeyValues *pCvarTagList ) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: Provide hooks for the GC based lobby system
//-----------------------------------------------------------------------------
abstract_class IServerGCLobby
{
public:
	virtual bool HasLobby() const = 0;
	virtual bool SteamIDAllowedToConnect( const CSteamID &steamId ) const = 0;
	virtual void UpdateServerDetails(void) = 0;
	virtual bool ShouldHibernate() = 0;

	virtual bool MatchAllowsNameChanges() = 0;
	virtual bool GetPlayerGCMatchName( const CSteamID &steamId, char *pszOutGCMatchName, size_t nGCMatchNameLen ) = 0;
};

#endif // EIFACE_H
