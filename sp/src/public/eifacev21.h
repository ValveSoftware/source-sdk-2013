//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Defines entity interface between engine and DLLs.
// This header file included by engine files and DLL files.
//
// Before including this header, DLLs must:
//		include edict.h
// This is conveniently done for them in extdll.h
//
// $NoKeywords: $
//
//=============================================================================

#ifndef EIFACEV21_H
#define EIFACEV21_H

#ifdef _WIN32
#pragma once
#endif

#include "convar.h"
#include "icvar.h"
#include "edict.h"
#include "iserverentity.h"
#include "engine/ivmodelinfo.h"
#include "soundflags.h"
#include "bitvec.h"

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
class	bf_read;
class	bf_write;
class	IRecipientFilter;
class	CBaseEntity;
class	ITraceFilter;
struct	client_textmessage_t;
class	INetChannelInfo;
class	ISpatialPartition;
class IScratchPad3D;
class CStandardSendProxiesV1;


// Terrain Modification Types
enum TerrainModType
{
	TMod_Sphere = 0,			// sphere that pushes all vertices out along their normal.
	TMod_Suck,
	TMod_AABB
};

class CTerrainModParams
{
public:

	// Flags for m_Flags.
	enum
	{
		TMOD_SUCKTONORMAL	   = ( 1 << 0 ),	// For TMod_Suck, suck into m_Normal rather than on +Z.
		TMOD_STAYABOVEORIGINAL = ( 1 << 1 )		// For TMod_Suck, don't go below the original vert on Z.
	};

	CTerrainModParams() { m_Flags = 0; }		// people always forget to init this

	Vector		m_vCenter;
	Vector		m_vNormal;						// If TMod_Suck and TMOD_SUCKTONORMAL is set.
	int			m_Flags;						// Combination of TMOD_ flags.
	float		m_flRadius;
	Vector		m_vecMin;						// Bounding box.
	Vector		m_vecMax;
	float		m_flStrength;					// for TMod_Suck
	float		m_flMorphTime;					// time over which the morph takes place
};

class CSpeculativeTerrainModVert
{
public:
	Vector		m_vOriginal;		// vertex position before any mods
	Vector		m_vCurrent;			// current vertex position
	Vector		m_vNew;				// vertex position if the mod were applied
};

//-----------------------------------------------------------------------------
// Terrain modification interface
//-----------------------------------------------------------------------------
class ITerrainMod
{
public:

	//---------------------------------------------------------------------
	// Initialize the terrain modifier.
	//---------------------------------------------------------------------
	virtual void	Init( const CTerrainModParams &params ) = 0;

	//---------------------------------------------------------------------
	// Apply the terrain modifier to the surface.  The vertex should be
	// moved from its original position to the target position.
	// Return true if the position is modified.
	//---------------------------------------------------------------------
	virtual bool	ApplyMod( Vector &vecTargetPos, Vector const &vecOriginalPos ) = 0;

	//---------------------------------------------------------------------
	// Apply the terrain modifier to the surface.  The vertex should from 
	// its original position toward the target position bassed on the
	// morph time.
	// Return true if the posistion is modified.
	//---------------------------------------------------------------------
	virtual	bool	ApplyModAtMorphTime( Vector &vecTargetPos, const Vector&vecOriginalPos, 
		                                 float flCurrentTime, float flMorphTime ) = 0;

	//---------------------------------------------------------------------
	// Get the bounding box for things that this mod can affect (note that
	// it CAN move things outside of this bounding box).
	//---------------------------------------------------------------------
	virtual void	GetBBox( Vector &vecBBMin, Vector &vecBBMax ) = 0;
};



//-----------------------------------------------------------------------------
// Purpose: Interface the engine exposes to the game DLL
//-----------------------------------------------------------------------------
#define VENGINESERVER_INTERFACEVERSION_21	"VEngineServer021"

namespace VEngineServerV21
{

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

	// For ConCommand parsing or parsing client commands issued by players typing at their console
	// Retrieves the raw command string (untokenized)
	virtual const char	*Cmd_Args( void ) = 0;		
	// Returns the number of tokens in the command string
	virtual int			Cmd_Argc( void ) = 0;		
	// Retrieves a specified token
	virtual char		*Cmd_Argv( int argc ) = 0;	 

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
	// Mark a specified area portal as open/closes
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

	// Apply a modification to the terrain.
	virtual void		ApplyTerrainMod( TerrainModType type, CTerrainModParams const &params ) = 0;

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
	virtual void SolidMoved( edict_t *pSolidEnt, ICollideable *pSolidCollide, const Vector* pPrevAbsOrigin ) = 0;
	// A trigger entity moved, update spatial partition
	virtual void TriggerMoved( edict_t *pTriggerEnt ) = 0;
	
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
	virtual bool IsPaused() = 0;

	// Marks the filename for consistency checking.  This should be called after precaching the file.
	virtual void		ForceExactFile( const char *s ) = 0;
	virtual void		ForceModelBounds( const char *s, const Vector &mins, const Vector &maxs ) = 0;
	virtual void		ClearSaveDirAfterClientLoad() = 0;

	// Sets a USERINFO client ConVar for a fakeclient
	virtual void SetFakeClientConVarValue( edict_t *pEntity, const char *cvar, const char *value ) = 0;
	
	virtual void InsertServerCommand( const char *str ) = 0;

	// Marks the material (vmt file) for consistency checking.  If the client and server have different
	// contents for the file, the client's vmt can only use the VertexLitGeneric shader, and can only
	// contain $baseTexture and $bumpmap vars.
	virtual void		ForceSimpleMaterial( const char *s ) = 0;


	// Is the engine in Commentary mode?
	virtual int			IsInCommentaryMode( void ) = 0;
};

} // end namespace


//-----------------------------------------------------------------------------
// Purpose: These are the interfaces that the game .dll exposes to the engine
//-----------------------------------------------------------------------------
#define SERVERGAMEDLL_INTERFACEVERSION_3			"ServerGameDLL003"

namespace ServerGameDLLV3
{

abstract_class IServerGameDLL
{
public:
	// Initialize the game (one-time call when the DLL is first loaded )
	// Return false if there is an error during startup.
	virtual bool			DLLInit(	CreateInterfaceFn engineFactory, 
										CreateInterfaceFn physicsFactory, 
										CreateInterfaceFn fileSystemFactory, 
										CGlobalVars *pGlobals) = 0;

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
	virtual void			GetSaveComment( char *comment, int maxlength ) = 0;
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
	virtual CStandardSendProxiesV1*	GetStandardSendProxies() = 0;
};

} // end namespace


//-----------------------------------------------------------------------------
// Just an interface version name for the random number interface
// See vstdlib/random.h for the interface definition
// NOTE: If you change this, also change VENGINE_CLIENT_RANDOM_INTERFACE_VERSION in cdll_int.h
//-----------------------------------------------------------------------------
#define VENGINE_SERVER_RANDOM_INTERFACE_VERSION_1	"VEngineRandom001"

//-----------------------------------------------------------------------------
// Purpose: Interface to get at server entities
//-----------------------------------------------------------------------------
#define SERVERGAMEENTS_INTERFACEVERSION_1			"ServerGameEnts001"

namespace ServerGameEntsV1
{

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

} // end namespace ServerGameEntsV1


//-----------------------------------------------------------------------------
// Purpose: Player / Client related functions
//-----------------------------------------------------------------------------
#define SERVERGAMECLIENTS_INTERFACEVERSION_3		"ServerGameClients003"

namespace ServerGameClientsV3
{

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
	virtual void			ClientCommand( edict_t *pEntity ) = 0;

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
	virtual int				GetReplayDelay( edict_t *player ) = 0;

	// Anything this game .dll wants to add to the bug reporter text (e.g., the entity/model under the picker crosshair)
	//  can be added here
	virtual void			GetBugReportInfo( char *buf, int buflen ) = 0;
};

} // end namespace ServerGameClientsV3


#define UPLOADGAMESTATS_INTERFACEVERSION_1		"ServerUploadGameStats001"

namespace UploadGameStatsV1
{

abstract_class IUploadGameStats
{
public:
	// Note that this call will block the server until the upload is completed, so use only at levelshutdown if at all.
	virtual bool UploadGameStats( 
		char const *mapname,				// Game map name
		unsigned int blobversion,			// Version of the binary blob data
		unsigned int blobsize,				// Size in bytes of blob data
		const void *pvBlobData ) = 0;		// Pointer to the blob data.
};

} // end namespace UploadGameStatsV1


#endif // EIFACEV21_H
