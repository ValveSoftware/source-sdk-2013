//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_GAMEINTERFACE_H
#define SRCPY_GAMEINTERFACE_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"
#include "ehandle.h"
#include <bspfile.h>
#include "GameEventListener.h"

#include <boost/python.hpp>

#ifndef CLIENT_DLL
	#include "steam/steamclientpublic.h"
	#include "gameinterface.h"
	#include "mapentities.h"
#endif

#include "filesystem.h"

namespace bp = boost::python;

class IGameEvent;
struct model_t;

// model_t wrap for python
struct wrap_model_t 
{
	wrap_model_t( model_t *pM ) { pModel = pM; }
    model_t *pModel;
};

//-----------------------------------------------------------------------------
// Purpose: Python function for deleting a directory
//-----------------------------------------------------------------------------
void PyRemoveFile( char const* pRelativePath, const char *pathID = 0 );
void PyRemoveDirectory( char const* pPath, const char *pathID = 0 );
boost::python::object PyGetModPath();

// Special use case
void PyAsyncFinishAllWrites( void );

//-----------------------------------------------------------------------------
// Purpose: Shutting down vars
//-----------------------------------------------------------------------------
bool PyShutdownConVar( const char *pName );
bool PyShutdownConCommand( const char *pName );

//-----------------------------------------------------------------------------
// Purpose: Python version of concommand
//-----------------------------------------------------------------------------
void PyDummyCallback( const CCommand &args );
class PyConCommand : public ConCommand
{
public:
	typedef ConCommand BaseClass;

	PyConCommand( const char *pName, bp::object method, const char *helpstring = 0, int flags = 0, 
		bp::object completionfunc = bp::object(), bool useweakref = false );
	// We must cleanup if we are destroyed
	~PyConCommand();

	// Invoke the function
	virtual void Dispatch( const CCommand &command );

	// Auto complete
	virtual int AutoCompleteSuggest( const char *partial, CUtlVector< CUtlString > &commands );

	virtual bool CanAutoComplete( void );

	void		 Shutdown();

private:
	bool m_bUsesWeakRef;
	bp::object m_pyCommandCallback;
	bp::object m_pyCompletionFunc;
};

//-----------------------------------------------------------------------------
// Purpose: Python version of convar
//-----------------------------------------------------------------------------
class PyConVar : public ConVar
{
public:
	typedef ConVar BaseClass;

	PyConVar(const char *name, const char *defaultvalue, int flags = 0);
	PyConVar( const char *name, const char *defaultvalue, int flags, 
		const char *helpstring );
	PyConVar( const char *name, const char *defaultvalue, int flags, 
		const char *helpstring, bool bMin, float fMin, bool bMax, float fMax );
	PyConVar( const char *name, const char *defaultvalue, int flags, 
		const char *helpstring, bp::object callback );
	PyConVar( const char *name, const char *defaultvalue, int flags, 
		const char *helpstring, bool bMin, float fMin, bool bMax, float fMax,
		bp::object callback );

	~PyConVar();

#if 0 // TODO
	// Need to override these for our python callback
	// Can't override ChangeStringValue :(
	// (or need to make changes to convar.h)
	virtual void				SetValue( const char *value );
	virtual void				SetValue( float value );
	virtual void				SetValue( int value );
#endif // 0

	void						Shutdown();

//private:
	//virtual void				Create( const char *pName, const char *pDefaultValue, int flags = 0,
	//								const char *pHelpString = 0, bool bMin = false, float fMin = 0.0,
	//								bool bMax = false, float fMax = false, FnChangeCallback_t callback = 0 );

private:
	// Call this function when ConVar changes
	bp::object m_pyChangeCallback;
};

//-----------------------------------------------------------------------------
// Engine function wrappers
//-----------------------------------------------------------------------------
inline const char *GetLevelName( void )
{
#ifdef CLIENT_DLL
	return engine->GetLevelName();
#else
	return STRING(gpGlobals->mapname);
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Game events
//-----------------------------------------------------------------------------
class PyGameEvent
{
public:
	PyGameEvent();
	PyGameEvent( const char *event_name, bool force = false );
	void Init( IGameEvent *pEvent );
	inline IGameEvent *GetEvent() { return m_pEvent; }

	const char *GetName() const;	// get event name

	bool  IsReliable() const; // if event handled reliable
	bool  IsLocal() const; // if event is never networked
	bool  IsEmpty(const char *keyName = NULL); // check if data field exists

	// Data access
	bool  GetBool( const char *keyName = NULL, bool defaultValue = false );
	int   GetInt( const char *keyName = NULL, int defaultValue = 0 );
	float GetFloat( const char *keyName = NULL, float defaultValue = 0.0f );
	const char *GetString( const char *keyName = NULL, const char *defaultValue = "" );

	void SetBool( const char *keyName, bool value );
	void SetInt( const char *keyName, int value );
	void SetFloat( const char *keyName, float value );
	void SetString( const char *keyName, const char *value );
private:
	IGameEvent *m_pEvent;
	bool m_bValid;
};

void PyFireGameEvent( PyGameEvent *pEvent, bool bDontBroadcast=false );
void PyFireGameEventClientSide( PyGameEvent *pEvent );

class PyGameEventListener : public CGameEventListener
{
public:
	virtual void PyFireGameEvent( boost::python::object event ) {}

private:
	virtual void FireGameEvent( IGameEvent *event );
};

//-----------------------------------------------------------------------------
// Purpose: Commands
//-----------------------------------------------------------------------------
// Engine player info, no game related infos here
// If you change this, change the two byteswap defintions: 
// cdll_client_int.cpp and cdll_engine_int.cpp
typedef struct py_player_info_s
{
	// scoreboard information
	boost::python::object name;
	// local server user ID, unique while server is running
	int				userID;
	// global unique player identifer
	boost::python::object guid;
	// friends identification number
	uint32			friendsID;
	// friends name
	boost::python::object friendsName;
	// true, if player is a bot controlled by game.dll
	bool			fakeplayer;
	// true if player is the HLTV proxy
	bool			ishltv;
	// custom files CRC for this player
	CRC32_t			customFiles[MAX_CUSTOM_FILES];
	// this counter increases each time the server downloaded a new file
	unsigned char	filesDownloaded;
} py_player_info_t;

void PyAddSearchPath( const char *pPath, const char *pathID, SearchPathAdd_t addType = PATH_ADD_TO_TAIL );
bool PyRemoveSearchPath( const char *pPath, const char *pathID = 0 );
boost::python::object PyGetSearchPath( const char *pPathID, bool bGetPackFiles );

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
dheader_t PyGetMapHeader( const char *mapname );

//-----------------------------------------------------------------------------
// Wrap engine
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
class PyVEngineClient
{
public:

	// Get the lighting intensivty for a specified point
	// If bClamp is specified, the resulting Vector is restricted to the 0.0 to 1.0 for each element
	virtual Vector			GetLightForPoint(const Vector &pos, bool clamp) { return engine->GetLightForPoint(pos, clamp); }

	// Gets the dimensions of the game window
	inline void				GetScreenSize( int& width, int& height ) { return engine->GetScreenSize(width, height); }

	// Forwards szCmdString to the server, sent reliably if bReliable is set
	inline void				ServerCommand( const char *szCmdString, bool bReliable = true ) { if(szCmdString) engine->ServerCmd(szCmdString, bReliable); }
	// Inserts szCmdString into the command buffer as if it was typed by the client to his/her console.
	// Note: Calls to this are checked against FCVAR_CLIENTCMD_CAN_EXECUTE (if that bit is not set, then this function can't change it).
	//       Call ClientCmd_Unrestricted to have access to FCVAR_CLIENTCMD_CAN_EXECUTE vars.
	inline void				ClientCommand( const char *szCmdString ) { if(szCmdString) engine->ClientCmd(szCmdString); }

	// Fill in the player info structure for the specified player index (name, model, etc.)
	bool					GetPlayerInfo( int ent_num, py_player_info_t *pinfo );

	// Retrieve the player entity number for a specified userID
	inline int					GetPlayerForUserID( int userID ) { return engine->GetPlayerForUserID(userID); }

	// Returns true if the console is visible
	inline bool				Con_IsVisible( void ) { return engine->Con_IsVisible(); }

	// Get the entity index of the local player
	inline int					GetLocalPlayer( void ) { return engine->GetLocalPlayer(); }

	// Client DLL is hooking a model, loads the model into memory and returns  pointer to the model_t
	model_t*		LoadModel( const char *pName, bool bProp = false );

	// Get the exact server timesstamp ( server time ) from the last message received from the server
	inline float				GetLastTimeStamp( void ) { return engine->GetLastTimeStamp(); }

	// Copy current view orientation into va
	inline void				GetViewAngles( QAngle& va ) { engine->GetViewAngles(va); }
	// Set current view orientation from va
	inline void				SetViewAngles( QAngle& va ) { engine->SetViewAngles(va); }

	// Retrieve the current game's maxclients setting
	inline int					GetMaxClients( void ) { return engine->GetMaxClients(); }

	// Given the string pBinding which may be bound to a key, 
	//  returns the string name of the key to which this string is bound. Returns NULL if no such binding exists
	inline	const char			*Key_LookupBinding( const char *pBinding ) { return engine->Key_LookupBinding(pBinding); }

	// Given the name of the key "mouse1", "e", "tab", etc., return the string it is bound to "+jump", "impulse 50", etc.
	inline const char			*Key_BindingForKey( ButtonCode_t code ) { return engine->Key_BindingForKey(code); }

	// key trapping (for binding keys)
	inline void				StartKeyTrapMode( void ) { engine->StartKeyTrapMode(); }
	inline bool				CheckDoneKeyTrapping( ButtonCode_t code ) {return engine->CheckDoneKeyTrapping(code); }

	// Returns true if the player is fully connected and active in game (i.e, not still loading)
	inline bool				IsInGame( void ) { return engine->IsInGame(); }
	// Returns true if the player is connected, but not necessarily active in game (could still be loading)
	inline bool				IsConnected( void ) { return engine->IsConnected(); }
	// Returns true if the loading plaque should be drawn
	inline bool				IsDrawingLoadingImage( void ) { return engine->IsDrawingLoadingImage(); }

	// Prints the formatted string to the notification area of the screen ( down the right hand edge
	//  numbered lines starting at position 0
	inline void		Con_NPrintf( int pos, const char *fmt ) { if( !fmt ) return; engine->Con_NPrintf( pos, fmt ); }

	// Similar to Con_NPrintf, but allows specifying custom text color and duration information
	//virtual void				Con_NXPrintf( const struct con_nprint_s *info, const char *fmt, ... ) = 0;

	// Is the specified world-space bounding box inside the view frustum?
	inline int					IsBoxVisible( const Vector& mins, const Vector& maxs ) { return engine->IsBoxVisible(mins, maxs); }

	// Is the specified world-space boudning box in the same PVS cluster as the view origin?
	inline int					IsBoxInViewCluster( const Vector& mins, const Vector& maxs ) { return engine->IsBoxInViewCluster(mins, maxs); }

	// Returns true if the specified box is outside of the view frustum and should be culled
	inline bool				CullBox( const Vector& mins, const Vector& maxs ) { return engine->CullBox(mins, maxs); }

	// Allow the sound system to paint additional data (during lengthy rendering operations) to prevent stuttering sound.
	inline void				Sound_ExtraUpdate( void ) { engine->Sound_ExtraUpdate(); }

	// Get the current game directory ( e.g., hl2, tf2, cstrike, hl1 )
	inline const char			*GetGameDirectory( void ) { return engine->GetGameDirectory(); }

	// Get access to the world to screen transformation matrix
	inline const VMatrix& 		WorldToScreenMatrix() { return engine->WorldToScreenMatrix(); }

	// Get the matrix to move a point from world space into view space
	// (translate and rotate so the camera is at the origin looking down X).
	inline const VMatrix& 		WorldToViewMatrix() { return engine->WorldToViewMatrix(); }

	// The .bsp file can have mod-specified data lumps. These APIs are for working with such game lumps.

	// Get mod-specified lump version id for the specified game data lump
	inline int					GameLumpVersion( int lumpId ) const { return engine->GameLumpVersion(lumpId); }
	// Get the raw size of the specified game data lump.
	inline int					GameLumpSize( int lumpId ) const { return engine->GameLumpSize(lumpId); }

	// Returns the number of leaves in the level
	inline int					LevelLeafCount() const { return engine->LevelLeafCount(); }

	// Convert texlight to gamma...
	//void		LinearToGamma( float* linear, float* gamma ) = 0;

	// Get the lightstyle value
	inline float		LightStyleValue( int style ) { return engine->LightStyleValue(style); }

	// Computes light due to dynamic lighting at a point
	// If the normal isn't specified, then it'll return the maximum lighting
	inline void		ComputeDynamicLighting( const Vector& pt, const Vector* pNormal, Vector& color ) { engine->ComputeDynamicLighting(pt, pNormal, color); }

	// Returns the color of the ambient light
	inline void		GetAmbientLightColor( Vector& color ) { return engine->GetAmbientLightColor(color); }

	// Returns the dx support level
	inline int			GetDXSupportLevel() { return engine->GetDXSupportLevel(); }

	// GR - returns the HDR support status
	inline bool        SupportsHDR() { return engine->SupportsHDR(); }

	// Get the name of the current map
	boost::python::object GetChapterName();
	inline char const	*GetLevelName( void ) { return engine->GetLevelName(); }

	// Returns true if the box touches the specified area's frustum.
	inline bool		DoesBoxTouchAreaFrustum( const Vector &mins, const Vector &maxs, int iArea ) { return engine->DoesBoxTouchAreaFrustum(mins, maxs, iArea); }

	// Computes light due to dynamic lighting at a point
	// If the normal isn't specified, then it'll return the maximum lighting
	// If pBoxColors is specified (it's an array of 6), then it'll copy the light contribution at each box side.
	inline void		ComputeLighting( const Vector& pt, const Vector* pNormal, bool bClamp, Vector& color, Vector *pBoxColors=NULL )
	{
		engine->ComputeLighting(pt, pNormal, bClamp, color, pBoxColors);
	}

	// Activates/deactivates an occluder...
	inline void		ActivateOccluder( int nOccluderIndex, bool bActive ) { engine->ActivateOccluder(nOccluderIndex, bActive); }
	inline bool		IsOccluded( const Vector &vecAbsMins, const Vector &vecAbsMaxs ) { return engine->IsOccluded(vecAbsMins, vecAbsMaxs); }

	// returns info interface for client netchannel
	//inline INetChannelInfo	*GetNetChannelInfo( void ) = 0;

	// This can be used to notify test scripts that we're at a particular spot in the code.
	inline void		CheckPoint( const char *pName ) { engine->CheckPoint(pName); }
	// Draw portals if r_DrawPortals is set (Debugging only)
	inline void		DrawPortals() { engine->DrawPortals(); }
	// Determine whether the client is playing back or recording a demo
	inline bool		IsPlayingDemo( void ) { return engine->IsPlayingDemo(); }
	inline bool		IsRecordingDemo( void ) { return engine->IsRecordingDemo(); }
	inline bool		IsPlayingTimeDemo( void ) { return engine->IsPlayingTimeDemo(); }
	// Is the game paused?
	inline bool		IsPaused( void ) { return engine->IsPaused(); }
	// Is the game currently taking a screenshot?
	inline bool		IsTakingScreenshot( void ) { return engine->IsTakingScreenshot(); }
	// Is this a HLTV broadcast ?
	inline bool		IsHLTV( void ) { return engine->IsHLTV(); }
	// is this level loaded as just the background to the main menu? (active, but unplayable)
	inline bool		IsLevelMainMenuBackground( void ) { return engine->IsLevelMainMenuBackground(); }
	// returns the name of the background level
	boost::python::object		GetMainMenuBackgroundName();

	// What language is the user expecting to hear .wavs in, "english" or another...
	boost::python::object		GetUILanguage();

	// Get the pristine map entity lump string.  (e.g., used by CS to reload the map entities when restarting a round.)
	inline const char*	GetMapEntitiesString() { return engine->GetMapEntitiesString(); }

	// Is the engine in map edit mode ?
	inline bool		IsInEditMode( void ) { return engine->IsInEditMode(); }

	// current screen aspect ratio (eg. 4.0f/3.0f, 16.0f/9.0f)
	inline float		GetScreenAspectRatio() { return engine->GetScreenAspectRatio(); }

	// allow other modules to know about engine versioning (one use is a proxy for network compatability)
	inline unsigned int	GetEngineBuildNumber() { return engine->GetEngineBuildNumber(); } // engines build
	inline const char *	GetProductVersionString() { return engine->GetProductVersionString(); } // mods version number (steam.inf)

	// Communicates to the color correction editor that it's time to grab the pre-color corrected frame
	// Passes in the actual size of the viewport
	inline void			GrabPreColorCorrectedFrame( int x, int y, int width, int height ) { engine->GrabPreColorCorrectedFrame(x, y, width, height); }

	inline bool			IsHammerRunning( ) const { return engine->IsHammerRunning(); }

	// Inserts szCmdString into the command buffer as if it was typed by the client to his/her console.
	// And then executes the command string immediately (vs ClientCmd() which executes in the next frame)
	//
	// Note: this is NOT checked against the FCVAR_CLIENTCMD_CAN_EXECUTE vars.
	inline void			ExecuteClientCmd( const char *szCmdString ) { engine->ExecuteClientCmd(szCmdString); }

	// returns if the loaded map was processed with HDR info. This will be set regardless
	// of what HDR mode the player is in.
	inline bool MapHasHDRLighting(void) { return engine->MapHasHDRLighting(); }

	inline int	GetAppID() { return engine->GetAppID(); }

	// Just get the leaf ambient light - no caching, no samples
	inline Vector			GetLightForPointFast(const Vector &pos, bool bClamp) { return engine->GetLightForPointFast(pos, bClamp); }

	// This version does NOT check against FCVAR_CLIENTCMD_CAN_EXECUTE.
	inline void			ClientCmd_Unrestricted( const char *szCmdString ) { engine->ClientCmd_Unrestricted(szCmdString); }

	// This used to be accessible through the cl_restrict_server_commands cvar.
	// By default, Valve games restrict the server to only being able to execute commands marked with FCVAR_SERVER_CAN_EXECUTE.
	// By default, mods are allowed to execute any server commands, and they can restrict the server's ability to execute client
	// commands with this function.
	inline void			SetRestrictServerCommands( bool bRestrict ) { engine->SetRestrictServerCommands(bRestrict); }

	// If set to true (defaults to true for Valve games and false for others), then IVEngineClient::ClientCmd
	// can only execute things marked with FCVAR_CLIENTCMD_CAN_EXECUTE.
	inline void			SetRestrictClientCommands( bool bRestrict ) { engine->SetRestrictClientCommands(bRestrict); }

	inline bool			CopyFrameBufferToMaterial( const char *pMaterialName ) { return engine->CopyFrameBufferToMaterial(pMaterialName); }

	// Causes the engine to read in the user's configuration on disk
	inline void			ReadConfiguration( const int iController ) { engine->ReadConfiguration( iController ); }

	inline bool			IsLowViolence() { return engine->IsLowViolence(); }
	inline const char		*GetMostRecentSaveGame( void ) { return engine->GetMostRecentSaveGame(); }
	inline void			SetMostRecentSaveGame( const char *lpszFilename )  { engine->SetMostRecentSaveGame(lpszFilename); }
};

extern PyVEngineClient *pyengine;

#else
class PyVEngineServer
{
public:
	// Tell engine to change level ( "changelevel s1\n" or "changelevel2 s1 s2\n" )
	inline void		ChangeLevel( const char *s1, const char *s2 = NULL ) { engine->ChangeLevel(s1, s2); }

	// Ask engine whether the specified map is a valid map file (exists and has valid version number).
	inline int			IsMapValid( const char *filename ) { return engine->IsMapValid(filename); }

	// Is this a dedicated server?
	inline bool		IsDedicatedServer( void ) { return engine->IsDedicatedServer(); }

	// Is in Hammer editing mode?
	inline int			IsInEditMode( void ) { return engine->IsInEditMode(); }

	// Add to the server/client lookup/precache table, the specified string is given a unique index
	// NOTE: The indices for PrecacheModel are 1 based
	//  a 0 returned from those methods indicates the model or sound was not correctly precached
	// However, generic and decal are 0 based
	// If preload is specified, the file is loaded into the server/client's cache memory before level startup, otherwise
	//  it'll only load when actually used (which can cause a disk i/o hitch if it occurs during play of a level).
	inline int			PrecacheModel( const char *s, bool preload = false ) { return engine->PrecacheModel(s, preload); }
	inline int			PrecacheSentenceFile( const char *s, bool preload = false ) { return engine->PrecacheSentenceFile(s, preload); }
	inline int			PrecacheDecal( const char *name, bool preload = false ) { return engine->PrecacheDecal(name, preload); }
	inline int			PrecacheGeneric( const char *s, bool preload = false ) { return engine->PrecacheGeneric(s, preload); }

	// Check's if the name is precached, but doesn't actually precache the name if not...
	inline bool		IsModelPrecached( char const *s ) const { return engine->IsModelPrecached(s); }
	inline bool		IsDecalPrecached( char const *s ) const { return engine->IsDecalPrecached(s); }
	inline bool		IsGenericPrecached( char const *s ) const { return engine->IsGenericPrecached(s); }

	// Note that sounds are precached using the IEngineSound interface

	// Special purpose PVS checking
	// Get the cluster # for the specified position
	//virtual int			GetClusterForOrigin( const Vector &org ) = 0;
	// Get the PVS bits for a specified cluster and copy the bits into outputpvs.  Returns the number of bytes needed to pack the PVS
	//virtual int			GetPVSForCluster( int cluster, int outputpvslength, unsigned char *outputpvs ) = 0;
	// Check whether the specified origin is inside the specified PVS
	//virtual bool		CheckOriginInPVS( const Vector &org, const unsigned char *checkpvs, int checkpvssize ) = 0;
	// Check whether the specified worldspace bounding box is inside the specified PVS
	//virtual bool		CheckBoxInPVS( const Vector &mins, const Vector &maxs, const unsigned char *checkpvs, int checkpvssize ) = 0;

	// Returns the server assigned userid for this player.  Useful for logging frags, etc.  
	//  returns -1 if the edict couldn't be found in the list of players.
	inline int			GetPlayerUserId( CBasePlayer *player ) 
			{ if( !player ) {return engine->GetPlayerUserId(NULL);} return engine->GetPlayerUserId(player->edict()); }
	inline const char	*GetPlayerNetworkIDString( CBasePlayer *player )
			{ if( !player ) {return engine->GetPlayerNetworkIDString(NULL);} return engine->GetPlayerNetworkIDString(player->edict()); }

	// Return the current number of used edict slots
	inline int			GetEntityCount( void ) { return engine->GetEntityCount(); }

	// Get stats info interface for a client netchannel
	//virtual INetChannelInfo* GetPlayerNetInfo( int playerIndex ) = 0;

	// Emit an ambient sound associated with the specified entity
	inline void		EmitAmbientSound( int entindex, const Vector &pos, const char *samp, float vol, soundlevel_t soundlevel, int fFlags, int pitch, float delay = 0.0f ) 
	{
		engine->EmitAmbientSound(entindex, pos, samp, vol, soundlevel, fFlags, pitch, delay);
	}

	// Fade out the client's volume level toward silence (or fadePercent)
	inline void        FadeClientVolume( CBasePlayer *player, float fadePercent, float fadeOutSeconds, float holdTime, float fadeInSeconds ) 
	{
		if( !player )
			return;
		engine->FadeClientVolume(player->edict(), fadePercent, fadeOutSeconds, holdTime, fadeInSeconds);
	}

	// Issue a command to the command parser as if it was typed at the server console.	
	inline void		ServerCommand( const char *str ) { if(str) engine->ServerCommand(str); }
	// Execute any commands currently in the command parser immediately (instead of once per frame)
	inline void		ServerExecute( void ) { engine->ServerExecute(); }
	// Issue the specified command to the specified client (mimics that client typing the command at the console).
	inline void		ClientCommand( CBasePlayer *player, const char *command )
	{
		if( !player || !command )
			return;
		engine->ClientCommand(player->edict(), command );
	}

	// Set the lightstyle to the specified value and network the change to any connected clients.  Note that val must not 
	//  change place in memory (use MAKE_STRING) for anything that's not compiled into your mod.
	inline void		LightStyle( int style, const char *val ) { engine->LightStyle(style, val); }

	// Project a static decal onto the specified entity / model (for level placed decals in the .bsp)
	inline void		StaticDecal( const Vector &originInEntitySpace, int decalIndex, int entityIndex, int modelIndex, bool lowpriority )
	{
		engine->StaticDecal( originInEntitySpace, decalIndex, entityIndex, modelIndex, lowpriority );
	}

	// Given the current PVS(or PAS) and origin, determine which players should hear/receive the message
	//inline void		Message_DetermineMulticastRecipients( bool usepas, const Vector& origin, CBitVec< ABSOLUTE_PLAYER_LIMIT >& playerbits ) = 0;

	// Print szMsg to the client console.
	inline void		ClientPrintf( CBasePlayer *player, const char *szMsg )
	{
		if( !player )
			return;
		engine->ClientPrintf(player->edict(), szMsg);
	}

	// SINGLE PLAYER/LISTEN SERVER ONLY (just matching the client .dll api for this)
	// Prints the formatted string to the notification area of the screen ( down the right hand edge
	//  numbered lines starting at position 0
	inline void		Con_NPrintf( int pos, const char *fmt ) { if( !fmt ) return; engine->Con_NPrintf( pos, fmt ); }

	// SINGLE PLAYER/LISTEN SERVER ONLY(just matching the client .dll api for this)
	// Similar to Con_NPrintf, but allows specifying custom text color and duration information
	//virtual void		Con_NXPrintf( const struct con_nprint_s *info, const char *fmt, ... ) = 0;

	// Change a specified player's "view entity" (i.e., use the view entity position/orientation for rendering the client view)
	//inline void		SetView( const edict_t *pClient, const edict_t *pViewent ) = 0;

	// Set the player's crosshair angle
	//inline void		CrosshairAngle( const edict_t *pClient, float pitch, float yaw );

	// Get the current game directory (hl2, tf2, hl1, cstrike, etc.)
	boost::python::object        GetGameDir();

	// Used by AI node graph code to determine if .bsp and .ain files are out of date
	//inline int 		CompareFileTime( const char *filename1, const char *filename2, int *iCompare ) { return engine->CompareFileTime(filename1, filename2, iCompare); }

	// Get a convar keyvalue for s specified client
	inline const char	*GetClientConVarValue( int clientIndex, const char *name ) { return engine->GetClientConVarValue(clientIndex, name); }

	// Parse a token from a file
	//inline const char	*ParseFile( const char *data, char *token, int maxlen ) = 0;
	// Copies a file
	//inline bool		CopyFile( const char *source, const char *destination ) = 0;

	// Reset the pvs, pvssize is the size in bytes of the buffer pointed to by pvs.
	// This should be called right before any calls to AddOriginToPVS
	//inline void		ResetPVS( byte *pvs, int pvssize ) = 0;
	// Merge the pvs bits into the current accumulated pvs based on the specified origin ( not that each pvs origin has an 8 world unit fudge factor )
	//inline void		AddOriginToPVS( const Vector &origin ) = 0;

	// Mark a specified area portal as open/closed.
	// Use SetAreaPortalStates if you want to set a bunch of them at a time.
	inline void		SetAreaPortalState( int portalNumber, int isOpen ) { engine->SetAreaPortalState(portalNumber, isOpen); }

	// Queue a temp entity for transmission
	//virtual void		PlaybackTempEntity( IRecipientFilter& filter, float delay, const void *pSender, const SendTable *pST, int classID  ) = 0;
	// Given a node number and the specified PVS, return with the node is in the PVS
	//virtual int			CheckHeadnodeVisible( int nodenum, const byte *pvs, int vissize ) = 0;
	// Using area bits, cheeck whether area1 flows into area2 and vice versa (depends on area portal state)
	//virtual int			CheckAreasConnected( int area1, int area2 ) = 0;
	// Given an origin, determine which area index the origin is within
	//virtual int			GetArea( const Vector &origin ) = 0;
	// Get area portal bit set
	//virtual void		GetAreaBits( int area, unsigned char *bits, int buflen ) = 0;
	// Given a view origin (which tells us the area to start looking in) and a portal key,
	// fill in the plane that leads out of this area (it points into whatever area it leads to).
	//virtual bool		GetAreaPortalPlane( Vector const &vViewOrigin, int portalKey, VPlane *pPlane ) = 0;

	// Get the pristine map entity lump string.  (e.g., used by CS to reload the map entities when restarting a round.)
	inline const char*	GetMapEntitiesString() { return engine->GetMapEntitiesString(); }

	// Text message system -- lookup the text message of the specified name
	//inline client_textmessage_t *TextMessageGet( const char *pName ) { }

	// Print a message to the server log file
	inline void		LogPrint( const char *msg ) { engine->LogPrint(msg); }

	// Builds PVS information for an entity
	//virtual void		BuildEntityClusterList( edict_t *pEdict, PVSInfo_t *pPVSInfo ) = 0;

	// A solid entity moved, update spatial partition
	//inline void SolidMoved( edict_t *pSolidEnt, ICollideable *pSolidCollide, const Vector* pPrevAbsOrigin, bool testSurroundingBoundsOnly ) = 0;
	// A trigger entity moved, update spatial partition
	//inline void TriggerMoved( edict_t *pTriggerEnt, bool testSurroundingBoundsOnly ) = 0;

	// Draw the brush geometry in the map into the scratch pad.
	// Flags is currently unused.
	//inline void		DrawMapToScratchPad( IScratchPad3D *pPad, unsigned long iFlags ) = 0;

	// This returns which entities, to the best of the server's knowledge, the client currently knows about.
	// This is really which entities were in the snapshot that this client last acked.
	// This returns a bit vector with one bit for each entity.
	//
	// USE WITH CARE. Whatever tick the client is really currently on is subject to timing and
	// ordering differences, so you should account for about a quarter-second discrepancy in here.
	// Also, this will return NULL if the client doesn't exist or if this client hasn't acked any frames yet.
	// 
	// iClientIndex is the CLIENT index, so if you use pPlayer->entindex(), subtract 1.
	//inline const CBitVec<MAX_EDICTS>* GetEntityTransmitBitsForClient( int iClientIndex ) = 0;

	// Is the game paused?
	inline bool		IsPaused() { return engine->IsPaused(); }

	// Marks the filename for consistency checking.  This should be called after precaching the file.
	inline void		ForceExactFile( const char *s ) { engine->ForceExactFile(s); }
	inline void		ForceModelBounds( const char *s, const Vector &mins, const Vector &maxs ) { engine->ForceModelBounds(s, mins, maxs); }
	inline void		ClearSaveDirAfterClientLoad() { engine->ClearSaveDirAfterClientLoad(); }

	// Sets a USERINFO client ConVar for a fakeclient
	//inline void		SetFakeClientConVarValue( edict_t *pEntity, const char *cvar, const char *value ) = 0;

	// Marks the material (vmt file) for consistency checking.  If the client and server have different
	// contents for the file, the client's vmt can only use the VertexLitGeneric shader, and can only
	// contain $baseTexture and $bumpmap vars.
	inline void		ForceSimpleMaterial( const char *s ) { engine->ForceSimpleMaterial(s); }

	// Is the engine in Commentary mode?
	inline int			IsInCommentaryMode( void ) { return engine->IsInCommentaryMode(); }


	// Mark some area portals as open/closed. It's more efficient to use this
	// than a bunch of individual SetAreaPortalState calls.
	//inline void		SetAreaPortalStates( const int *portalNumbers, const int *isOpen, int nPortals ) = 0;

	// Called when relevant edict state flags change.
	//inline void		NotifyEdictFlagsChange( int iEdict ) = 0;

	// Only valid during CheckTransmit. Also, only the PVS, networked areas, and
	// m_pTransmitInfo are valid in the returned strucutre.
	//inline const CCheckTransmitInfo* GetPrevCheckTransmitInfo( edict_t *pPlayerEdict ) = 0;

	//inline CSharedEdictChangeInfo* GetSharedEdictChangeInfo() = 0;

	// Tells the engine we can immdiately re-use all edict indices
	// even though we may not have waited enough time
	inline void			AllowImmediateEdictReuse( ) { engine->AllowImmediateEdictReuse(); }

	// Returns true if the engine is an internal build. i.e. is using the internal bugreporter.
	inline bool		IsInternalBuild( void ) { return engine->IsInternalBuild(); }

	//inline IChangeInfoAccessor *GetChangeAccessor( const edict_t *pEdict ) = 0;	

	// Name of most recently load .sav file
	inline char const *GetMostRecentlyLoadedFileName() { return engine->GetMostRecentlyLoadedFileName(); }
	inline char const *GetSaveFileName() { return engine->GetSaveFileName(); }

	// Cleans up the cluster list
	//inline void CleanUpEntityClusterList( PVSInfo_t *pPVSInfo ) = 0;

	//virtual void SetAchievementMgr( IAchievementMgr *pAchievementMgr ) =0;
	//virtual IAchievementMgr *GetAchievementMgr() = 0;

	inline int	GetAppID() { return engine->GetAppID(); }

	inline bool IsLowViolence() { return engine->IsLowViolence(); }

	// Call this to find out the value of a cvar on the client.
	//
	// It is an asynchronous query, and it will call IServerGameDLL::OnQueryCvarValueFinished when
	// the value comes in from the client.
	//
	// Store the return value if you want to match this specific query to the OnQueryCvarValueFinished call.
	// Returns InvalidQueryCvarCookie if the entity is invalid.
	//virtual QueryCvarCookie_t StartQueryCvarValue( edict_t *pPlayerEntity, const char *pName ) = 0;

	inline void InsertServerCommand( const char *str ) { engine->InsertServerCommand(str); }

	// Fill in the player info structure for the specified player index (name, model, etc.)
	bool GetPlayerInfo( int ent_num, py_player_info_t *pinfo );

	// Returns true if this client has been fully authenticated by Steam
	inline bool IsClientFullyAuthenticated( CBasePlayer *player ) 
	{
		if( !player )
			return false;
		return engine->IsClientFullyAuthenticated(player->edict());
	}

	// This makes the host run 1 tick per frame instead of checking the system timer to see how many ticks to run in a certain frame.
	// i.e. it does the same thing timedemo does.
	inline void SetDedicatedServerBenchmarkMode( bool bBenchmarkMode ) { engine->SetDedicatedServerBenchmarkMode(bBenchmarkMode); }

	// Methods to set/get a gamestats data container so client & server running in same process can send combined data
	//virtual void SetGamestatsData( CGamestatsData *pGamestatsData ) = 0;
	//virtual CGamestatsData *GetGamestatsData() = 0;

	// Returns the SteamID of the specified player. It'll be NULL if the player hasn't authenticated yet.
	boost::python::object GetClientSteamID( CBasePlayer *player );
};

extern PyVEngineServer *pyengine;

#endif 

class PyVModelInfo
{
public:
	inline model_t *			GetModel( int modelindex ) { return (model_t *)modelinfo->GetModel(modelindex); }
	inline int					GetModelIndex( const char *name ) { return modelinfo->GetModelIndex(name); }
	boost::python::tuple		GetModelBounds( model_t *pModel );
	boost::python::object		GetModelName( model_t *model );
	model_t *					FindOrLoadModel( const char *name );
};
extern PyVModelInfo *pymodelinfo;

#ifndef CLIENT_DLL
void PyMapEntity_ParseAllEntities(const char *mapdata, IMapEntityFilter *filter, bool activateentities);

CMapEntityRef PyGetMapEntityRef( int it );
int PyGetMapEntityRefIteratorHead(); 
int PyGetMapEntityRefIteratorNext( int it ); 

#endif // CLIENT_DLL

#endif // SRCPY_GAMEINTERFACE_H

