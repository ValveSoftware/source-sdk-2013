//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
#ifndef ENGINECALLBACK_H
#define ENGINECALLBACK_H

#ifndef EIFACE_H
#include "eiface.h"
#endif

class IFileSystem;				// include filesystem.h
class IEngineSound;				// include engine/IEngineSound.h
class IVEngineServer;			
class IVoiceServer;
class IStaticPropMgrServer;
class ISpatialPartition;
class IVModelInfo;
class IEngineTrace;
class IGameEventManager2;
class IVDebugOverlay;
class IDataCache;
class IMDLCache;
class IServerEngineTools;
class IXboxSystem;
class CSteamAPIContext;
class CSteamGameServerAPIContext;

extern IVEngineServer			*engine;
extern IVoiceServer				*g_pVoiceServer;
extern IFileSystem				*filesystem;
extern IStaticPropMgrServer		*staticpropmgr;
extern ISpatialPartition		*partition;
extern IEngineSound				*enginesound;
extern IVModelInfo				*modelinfo;
extern IEngineTrace				*enginetrace;
extern IGameEventManager2		*gameeventmanager;
extern IVDebugOverlay			*debugoverlay;
extern IDataCache				*datacache;
extern IMDLCache				*mdlcache;
extern IServerEngineTools		*serverenginetools;
extern IXboxSystem				*xboxsystem; // 360 only
extern CSteamAPIContext			*steamapicontext; // available on game clients
extern CSteamGameServerAPIContext *steamgameserverapicontext; //available on game servers



//-----------------------------------------------------------------------------
// Precaches a material
//-----------------------------------------------------------------------------
void PrecacheMaterial( const char *pMaterialName );

//-----------------------------------------------------------------------------
// Converts a previously precached material into an index
//-----------------------------------------------------------------------------
int GetMaterialIndex( const char *pMaterialName );

//-----------------------------------------------------------------------------
// Converts a previously precached material index into a string
//-----------------------------------------------------------------------------
const char *GetMaterialNameFromIndex( int nMaterialIndex );


//-----------------------------------------------------------------------------
// Precache-related methods for particle systems
//-----------------------------------------------------------------------------
void PrecacheParticleSystem( const char *pParticleSystemName );
int GetParticleSystemIndex( const char *pParticleSystemName );
const char *GetParticleSystemNameFromIndex( int nIndex );


class IRecipientFilter;
void EntityMessageBegin( CBaseEntity * entity, bool reliable = false );
void UserMessageBegin( IRecipientFilter& filter, const char *messagename );
void MessageEnd( void );

// bytewise
void MessageWriteByte( int iValue);
void MessageWriteChar( int iValue);
void MessageWriteShort( int iValue);
void MessageWriteWord( int iValue );
void MessageWriteLong( int iValue);
void MessageWriteFloat( float flValue);
void MessageWriteAngle( float flValue);
void MessageWriteCoord( float flValue);
void MessageWriteVec3Coord( const Vector& rgflValue);
void MessageWriteVec3Normal( const Vector& rgflValue);
void MessageWriteAngles( const QAngle& rgflValue);
void MessageWriteString( const char *sz );
void MessageWriteEntity( int iValue);
void MessageWriteEHandle( CBaseEntity *pEntity ); //encoded as a long


// bitwise
void MessageWriteBool( bool bValue );
void MessageWriteUBitLong( unsigned int data, int numbits );
void MessageWriteSBitLong( int data, int numbits );
void MessageWriteBits( const void *pIn, int nBits );

#ifndef NO_STEAM

/// Returns Steam ID, given player index.   Returns an invalid SteamID upon
/// failure
extern CSteamID GetSteamIDForPlayerIndex( int iPlayerIndex );

#endif


// Bytewise
#define WRITE_BYTE		(MessageWriteByte)
#define WRITE_CHAR		(MessageWriteChar)
#define WRITE_SHORT		(MessageWriteShort)
#define WRITE_WORD		(MessageWriteWord)
#define WRITE_LONG		(MessageWriteLong)
#define WRITE_FLOAT		(MessageWriteFloat)
#define WRITE_ANGLE		(MessageWriteAngle)
#define WRITE_COORD		(MessageWriteCoord)
#define WRITE_VEC3COORD	(MessageWriteVec3Coord)
#define WRITE_VEC3NORMAL (MessageWriteVec3Normal)
#define WRITE_ANGLES	(MessageWriteAngles)
#define WRITE_STRING	(MessageWriteString)
#define WRITE_ENTITY	(MessageWriteEntity)
#define WRITE_EHANDLE	(MessageWriteEHandle)

// Bitwise
#define WRITE_BOOL		(MessageWriteBool)
#define WRITE_UBITLONG	(MessageWriteUBitLong)
#define WRITE_SBITLONG	(MessageWriteSBitLong)
#define WRITE_BITS		(MessageWriteBits)

#endif		//ENGINECALLBACK_H
