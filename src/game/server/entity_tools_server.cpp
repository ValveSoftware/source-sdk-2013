//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "const.h"
#include "toolframework/itoolentity.h"
#include "entitylist.h"
#include "toolframework/itoolsystem.h"
#include "KeyValues.h"
#include "icliententity.h"
#include "iserverentity.h"
#include "sceneentity.h"
#include "particles/particles.h"


//-----------------------------------------------------------------------------
// Interface from engine to tools for manipulating entities
//-----------------------------------------------------------------------------
class CServerTools : public IServerTools
{
public:
	// Inherited from IServerTools
	virtual IServerEntity *GetIServerEntity( IClientEntity *pClientEntity );
	virtual bool GetPlayerPosition( Vector &org, QAngle &ang, IClientEntity *pClientPlayer = NULL );
	virtual bool SnapPlayerToPosition( const Vector &org, const QAngle &ang, IClientEntity *pClientPlayer = NULL );
	virtual int GetPlayerFOV( IClientEntity *pClientPlayer = NULL );
	virtual bool SetPlayerFOV( int fov, IClientEntity *pClientPlayer = NULL );
	virtual bool IsInNoClipMode( IClientEntity *pClientPlayer = NULL );
	virtual CBaseEntity *FirstEntity( void );
	virtual CBaseEntity *NextEntity( CBaseEntity *pEntity );
	virtual CBaseEntity *FindEntityByHammerID( int iHammerID );
	virtual bool GetKeyValue( CBaseEntity *pEntity, const char *szField, char *szValue, int iMaxLen );
	virtual bool SetKeyValue( CBaseEntity *pEntity, const char *szField, const char *szValue );
	virtual bool SetKeyValue( CBaseEntity *pEntity, const char *szField, float flValue );
	virtual bool SetKeyValue( CBaseEntity *pEntity, const char *szField, const Vector &vecValue );
	virtual CBaseEntity *CreateEntityByName( const char *szClassName );
	virtual void DispatchSpawn( CBaseEntity *pEntity );
	virtual void ReloadParticleDefintions( const char *pFileName, const void *pBufData, int nLen );
	virtual void AddOriginToPVS( const Vector &org );
	virtual void MoveEngineViewTo( const Vector &vPos, const QAngle &vAngles );
	virtual bool DestroyEntityByHammerId( int iHammerID );
	virtual CBaseEntity *GetBaseEntityByEntIndex( int iEntIndex );
	virtual void RemoveEntity( CBaseEntity *pEntity );
	virtual void RemoveEntityImmediate( CBaseEntity *pEntity );
	virtual IEntityFactoryDictionary *GetEntityFactoryDictionary( void );
	virtual void SetMoveType( CBaseEntity *pEntity, int val );
	virtual void SetMoveType( CBaseEntity *pEntity, int val, int moveCollide );
	virtual void ResetSequence( CBaseAnimating *pEntity, int nSequence );
	virtual void ResetSequenceInfo( CBaseAnimating *pEntity );
	virtual void ClearMultiDamage( void );
	virtual void ApplyMultiDamage( void );
	virtual void AddMultiDamage( const CTakeDamageInfo &pTakeDamageInfo, CBaseEntity *pEntity );
	virtual void RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore );

	virtual ITempEntsSystem *GetTempEntsSystem( void );
	virtual CBaseTempEntity *GetTempEntList( void );
	virtual CGlobalEntityList *GetEntityList( void );
	virtual bool IsEntityPtr( void *pTest );
	virtual CBaseEntity *FindEntityByClassname( CBaseEntity *pStartEntity, const char *szName );
	virtual CBaseEntity *FindEntityByName( CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity = NULL, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL, IEntityFindFilter *pFilter = NULL );
	virtual CBaseEntity *FindEntityInSphere( CBaseEntity *pStartEntity, const Vector &vecCenter, float flRadius );
	virtual CBaseEntity *FindEntityByTarget( CBaseEntity *pStartEntity, const char *szName );
	virtual CBaseEntity *FindEntityByModel( CBaseEntity *pStartEntity, const char *szModelName );
	virtual CBaseEntity *FindEntityByNameNearest( const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity = NULL, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );
	virtual CBaseEntity *FindEntityByNameWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity = NULL, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );
	virtual CBaseEntity *FindEntityByClassnameNearest( const char *szName, const Vector &vecSrc, float flRadius );
	virtual CBaseEntity *FindEntityByClassnameWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius );
	virtual CBaseEntity *FindEntityByClassnameWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecMins, const Vector &vecMaxs );
	virtual CBaseEntity *FindEntityGeneric( CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity = NULL, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );
	virtual CBaseEntity *FindEntityGenericWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity = NULL, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );
	virtual CBaseEntity *FindEntityGenericNearest( const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity = NULL, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );
	virtual CBaseEntity *FindEntityNearestFacing( const Vector &origin, const Vector &facing, float threshold );
	virtual CBaseEntity *FindEntityClassNearestFacing( const Vector &origin, const Vector &facing, float threshold, char *classname );
	virtual CBaseEntity *FindEntityProcedural( const char *szName, CBaseEntity *pSearchingEntity = NULL, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );
};


//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
static CServerTools g_ServerTools;

// VSERVERTOOLS_INTERFACE_VERSION_1 is compatible with the latest since we're only adding things to the end, so expose that as well.
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CServerTools, IServerTools001, VSERVERTOOLS_INTERFACE_VERSION_1, g_ServerTools );
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CServerTools, IServerTools002, VSERVERTOOLS_INTERFACE_VERSION_2, g_ServerTools );
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CServerTools, IServerTools, VSERVERTOOLS_INTERFACE_VERSION, g_ServerTools );

// When bumping the version to this interface, check that our assumption is still valid and expose the older version in the same way
COMPILE_TIME_ASSERT( VSERVERTOOLS_INTERFACE_VERSION_INT == 3 );


IServerEntity *CServerTools::GetIServerEntity( IClientEntity *pClientEntity )
{
	if ( pClientEntity == NULL )
		return NULL;

	CBaseHandle ehandle = pClientEntity->GetRefEHandle();
	if ( ehandle.GetEntryIndex() >= MAX_EDICTS )
		return NULL; // the first MAX_EDICTS entities are networked, the rest are client or server only

#if 0
	// this fails, since the server entities have extra bits in their serial numbers,
	// since 20 bits are reserved for serial numbers, except for networked entities, which are restricted to 10

	// Brian believes that everything should just restrict itself to 10 to make things simpler,
	// so if/when he changes NUM_SERIAL_NUM_BITS to 10, we can switch back to this simpler code

	IServerNetworkable *pNet = gEntList.GetServerNetworkable( ehandle );
	if ( pNet == NULL )
		return NULL;

	CBaseEntity *pServerEnt = pNet->GetBaseEntity();
	return pServerEnt;
#else
	IHandleEntity *pEnt = gEntList.LookupEntityByNetworkIndex( ehandle.GetEntryIndex() );
	if ( pEnt == NULL )
		return NULL;

	CBaseHandle h = gEntList.GetNetworkableHandle( ehandle.GetEntryIndex() );
	const int mask = ( 1 << NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS ) - 1;
	if ( !h.IsValid() || ( ( h.GetSerialNumber() & mask ) != ( ehandle.GetSerialNumber() & mask ) ) )
		return NULL;

	IServerUnknown *pUnk = static_cast< IServerUnknown* >( pEnt );
	return pUnk->GetBaseEntity();
#endif
}

bool CServerTools::GetPlayerPosition( Vector &org, QAngle &ang, IClientEntity *pClientPlayer )
{
	IServerEntity *pServerPlayer = GetIServerEntity( pClientPlayer );
	CBasePlayer *pPlayer = pServerPlayer ? ( CBasePlayer* )pServerPlayer : UTIL_GetLocalPlayer();
	if ( pPlayer == NULL )
		return false;

	org = pPlayer->EyePosition();
	ang = pPlayer->EyeAngles();
	return true;
}

bool CServerTools::SnapPlayerToPosition( const Vector &org, const QAngle &ang, IClientEntity *pClientPlayer )
{
	IServerEntity *pServerPlayer = GetIServerEntity( pClientPlayer );
	CBasePlayer *pPlayer = pServerPlayer ? ( CBasePlayer* )pServerPlayer : UTIL_GetLocalPlayer();
	if ( pPlayer == NULL )
		return false;

	pPlayer->SetAbsOrigin( org - pPlayer->GetViewOffset() );
	pPlayer->SnapEyeAngles( ang );

	// Disengage from hierarchy
	pPlayer->SetParent( NULL );

	return true;
}

int CServerTools::GetPlayerFOV( IClientEntity *pClientPlayer )
{
	IServerEntity *pServerPlayer = GetIServerEntity( pClientPlayer );
	CBasePlayer *pPlayer = pServerPlayer ? ( CBasePlayer* )pServerPlayer : UTIL_GetLocalPlayer();
	if ( pPlayer == NULL )
		return 0;

	return pPlayer->GetFOV();
}

bool CServerTools::SetPlayerFOV( int fov, IClientEntity *pClientPlayer )
{
	IServerEntity *pServerPlayer = GetIServerEntity( pClientPlayer );
	CBasePlayer *pPlayer = pServerPlayer ? ( CBasePlayer* )pServerPlayer : UTIL_GetLocalPlayer();
	if ( pPlayer == NULL )
		return false;

	pPlayer->SetDefaultFOV( fov );
	CBaseEntity *pFOVOwner = pPlayer->GetFOVOwner();
	return pPlayer->SetFOV( pFOVOwner ? pFOVOwner : pPlayer, fov );
}

bool CServerTools::IsInNoClipMode( IClientEntity *pClientPlayer )
{
	IServerEntity *pServerPlayer = GetIServerEntity( pClientPlayer );
	CBasePlayer *pPlayer = pServerPlayer ? ( CBasePlayer* )pServerPlayer : UTIL_GetLocalPlayer();
	if ( pPlayer == NULL )
		return true;

	return pPlayer->GetMoveType() == MOVETYPE_NOCLIP;
}

CBaseEntity *CServerTools::FirstEntity( void )
{
	return gEntList.FirstEnt();
}

CBaseEntity *CServerTools::NextEntity( CBaseEntity *pEntity )
{
	CBaseEntity *pEnt;

	if ( pEntity == NULL )
	{
		pEnt = gEntList.FirstEnt();
	}
	else
	{
		pEnt = gEntList.NextEnt( (CBaseEntity *)pEntity );
	}
	return pEnt;
}

CBaseEntity *CServerTools::FindEntityByHammerID( int iHammerID )
{
	CBaseEntity *pEntity = gEntList.FirstEnt();

	while (pEntity)
	{
		if (pEntity->m_iHammerID == iHammerID)
			return pEntity;
		pEntity = gEntList.NextEnt( pEntity );
	}
	return NULL;
}

bool CServerTools::GetKeyValue( CBaseEntity *pEntity, const char *szField, char *szValue, int iMaxLen )
{
	return pEntity->GetKeyValue( szField, szValue, iMaxLen );
}

bool CServerTools::SetKeyValue( CBaseEntity *pEntity, const char *szField, const char *szValue )
{
	return pEntity->KeyValue( szField, szValue );
}

bool CServerTools::SetKeyValue( CBaseEntity *pEntity, const char *szField, float flValue )
{
	return pEntity->KeyValue( szField, flValue );
}

bool CServerTools::SetKeyValue( CBaseEntity *pEntity, const char *szField, const Vector &vecValue )
{
	return pEntity->KeyValue( szField, vecValue );
}


//-----------------------------------------------------------------------------
// entity spawning
//-----------------------------------------------------------------------------
CBaseEntity *CServerTools::CreateEntityByName( const char *szClassName )
{
	return ::CreateEntityByName( szClassName );
}

void CServerTools::DispatchSpawn( CBaseEntity *pEntity )
{
	::DispatchSpawn( pEntity );
}


//-----------------------------------------------------------------------------
// Reload particle definitions
//-----------------------------------------------------------------------------
void CServerTools::ReloadParticleDefintions( const char *pFileName, const void *pBufData, int nLen )
{
	// FIXME: Use file name to determine if we care about this data
	CUtlBuffer buf( pBufData, nLen, CUtlBuffer::READ_ONLY );
	g_pParticleSystemMgr->ReadParticleConfigFile( buf, true );
}

void CServerTools::AddOriginToPVS( const Vector &org )
{
	engine->AddOriginToPVS( org );
}

void CServerTools::MoveEngineViewTo( const Vector &vPos, const QAngle &vAngles )
{
	CBasePlayer *pPlayer = UTIL_GetListenServerHost();
	if ( !pPlayer )
		return;

	extern void EnableNoClip( CBasePlayer *pPlayer );
	EnableNoClip( pPlayer );

	Vector zOffset = pPlayer->EyePosition() - pPlayer->GetAbsOrigin();

	pPlayer->SetAbsOrigin( vPos - zOffset );
	pPlayer->SnapEyeAngles( vAngles );
}

bool CServerTools::DestroyEntityByHammerId( int iHammerID )
{
	CBaseEntity *pEntity = (CBaseEntity*)FindEntityByHammerID( iHammerID );
	if ( !pEntity )
		return false;

	UTIL_Remove( pEntity );
	return true;
}

void CServerTools::RemoveEntity( CBaseEntity *pEntity )
{
	UTIL_Remove( pEntity );
}

void CServerTools::RemoveEntityImmediate( CBaseEntity *pEntity )
{
	UTIL_RemoveImmediate( pEntity );
}

CBaseEntity *CServerTools::GetBaseEntityByEntIndex( int iEntIndex )
{
	edict_t *pEdict = INDEXENT( iEntIndex );
	if ( pEdict )
		return CBaseEntity::Instance( pEdict );
	else
		return NULL;
}

IEntityFactoryDictionary *CServerTools::GetEntityFactoryDictionary( void )
{
	return ::EntityFactoryDictionary();
}


void CServerTools::SetMoveType( CBaseEntity *pEntity, int val )
{
	pEntity->SetMoveType( (MoveType_t)val );
}

void CServerTools::SetMoveType( CBaseEntity *pEntity, int val, int moveCollide )
{
	pEntity->SetMoveType( (MoveType_t)val, (MoveCollide_t)moveCollide );
}

void CServerTools::ResetSequence( CBaseAnimating *pEntity, int nSequence )
{
	pEntity->ResetSequence( nSequence );
}

void CServerTools::ResetSequenceInfo( CBaseAnimating *pEntity )
{
	pEntity->ResetSequenceInfo();
}


void CServerTools::ClearMultiDamage( void )
{
	::ClearMultiDamage();
}

void CServerTools::ApplyMultiDamage( void )
{
	::ApplyMultiDamage();
}

void CServerTools::AddMultiDamage( const CTakeDamageInfo &pTakeDamageInfo, CBaseEntity *pEntity )
{
	::AddMultiDamage( pTakeDamageInfo, pEntity );
}

void CServerTools::RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore )
{
	::RadiusDamage( info, vecSrc, flRadius, iClassIgnore, pEntityIgnore );
}


ITempEntsSystem *CServerTools::GetTempEntsSystem( void )
{
	return (ITempEntsSystem *)te;
}

CBaseTempEntity *CServerTools::GetTempEntList( void )
{
	return CBaseTempEntity::GetList();
}

CGlobalEntityList *CServerTools::GetEntityList( void )
{
	return &gEntList;
}

bool CServerTools::IsEntityPtr( void *pTest )
{
	return gEntList.IsEntityPtr( pTest );
}

CBaseEntity *CServerTools::FindEntityByClassname( CBaseEntity *pStartEntity, const char *szName )
{
	return gEntList.FindEntityByClassname( pStartEntity, szName );
}

CBaseEntity *CServerTools::FindEntityByName( CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller, IEntityFindFilter *pFilter )
{
	return gEntList.FindEntityByName( pStartEntity, szName, pSearchingEntity, pActivator, pCaller, pFilter );
}

CBaseEntity *CServerTools::FindEntityInSphere( CBaseEntity *pStartEntity, const Vector &vecCenter, float flRadius )
{
	return gEntList.FindEntityInSphere( pStartEntity, vecCenter, flRadius );
}

CBaseEntity *CServerTools::FindEntityByTarget( CBaseEntity *pStartEntity, const char *szName )
{
	return gEntList.FindEntityByTarget( pStartEntity, szName );
}

CBaseEntity *CServerTools::FindEntityByModel( CBaseEntity *pStartEntity, const char *szModelName )
{
	return gEntList.FindEntityByModel( pStartEntity, szModelName );
}

CBaseEntity *CServerTools::FindEntityByNameNearest( const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	return gEntList.FindEntityByNameNearest( szName, vecSrc, flRadius, pSearchingEntity, pActivator, pCaller );
}

CBaseEntity *CServerTools::FindEntityByNameWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	return gEntList.FindEntityByNameWithin( pStartEntity, szName, vecSrc, flRadius, pSearchingEntity, pActivator, pCaller );
}

CBaseEntity *CServerTools::FindEntityByClassnameNearest( const char *szName, const Vector &vecSrc, float flRadius )
{
	return gEntList.FindEntityByClassnameNearest( szName, vecSrc, flRadius );
}

CBaseEntity *CServerTools::FindEntityByClassnameWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius )
{
	return gEntList.FindEntityByClassnameWithin( pStartEntity, szName, vecSrc, flRadius );
}

CBaseEntity *CServerTools::FindEntityByClassnameWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecMins, const Vector &vecMaxs )
{
	return gEntList.FindEntityByClassnameWithin( pStartEntity, szName, vecMins, vecMaxs );
}

CBaseEntity *CServerTools::FindEntityGeneric( CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	return gEntList.FindEntityGeneric( pStartEntity, szName, pSearchingEntity, pActivator, pCaller );
}

CBaseEntity *CServerTools::FindEntityGenericWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	return gEntList.FindEntityGenericWithin( pStartEntity, szName, vecSrc, flRadius, pSearchingEntity, pActivator, pCaller );
}

CBaseEntity *CServerTools::FindEntityGenericNearest( const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	return gEntList.FindEntityGenericNearest( szName, vecSrc, flRadius, pSearchingEntity, pActivator, pCaller );
}

CBaseEntity *CServerTools::FindEntityNearestFacing( const Vector &origin, const Vector &facing, float threshold )
{
	return gEntList.FindEntityNearestFacing( origin, facing, threshold );
}

CBaseEntity *CServerTools::FindEntityClassNearestFacing( const Vector &origin, const Vector &facing, float threshold, char *classname )
{
	return gEntList.FindEntityClassNearestFacing( origin, facing, threshold, classname );
}

CBaseEntity *CServerTools::FindEntityProcedural( const char *szName, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	return gEntList.FindEntityProcedural( szName, pSearchingEntity, pActivator, pCaller );
}


// Interface from engine to tools for manipulating entities
class CServerChoreoTools : public IServerChoreoTools
{
public:
	// Iterates through ALL entities (separate list for client vs. server)
	virtual EntitySearchResult	NextChoreoEntity( EntitySearchResult currentEnt )
	{
		CBaseEntity *ent = reinterpret_cast< CBaseEntity* >( currentEnt );
		ent = gEntList.FindEntityByClassname( ent, "logic_choreographed_scene" );
		return reinterpret_cast< EntitySearchResult >( ent );
	}

	virtual const char			*GetSceneFile( EntitySearchResult sr )
	{
		CBaseEntity *ent = reinterpret_cast< CBaseEntity* >( sr );
		if ( !sr )
			return "";

		if ( Q_stricmp( ent->GetClassname(), "logic_choreographed_scene" ) )
			return "";

		return GetSceneFilename( ent );
	}

	// For interactive editing
	virtual int	GetEntIndex( EntitySearchResult sr )
	{
		CBaseEntity *ent = reinterpret_cast< CBaseEntity* >( sr );
		if ( !ent )
			return -1;

		return ent->entindex();
	}

	virtual void ReloadSceneFromDisk( int entindex )
	{
		CBaseEntity *ent = CBaseEntity::Instance( entindex );
		if ( !ent )
			return;

		::ReloadSceneFromDisk( ent );
	}
};


static CServerChoreoTools g_ServerChoreoTools;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CServerChoreoTools, IServerChoreoTools, VSERVERCHOREOTOOLS_INTERFACE_VERSION, g_ServerChoreoTools );


//------------------------------------------------------------------------------
// Applies keyvalues to the entity by hammer ID.
//------------------------------------------------------------------------------
void CC_Ent_Keyvalue( const CCommand &args )
{
	// Must have an odd number of arguments.
	if ( ( args.ArgC() < 4 ) || ( args.ArgC() & 1 ) )
	{
		Msg( "Format: ent_keyvalue <entity id> \"key1\" \"value1\" \"key2\" \"value2\" ... \"keyN\" \"valueN\"\n" );
		return;
	}

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	CBaseEntity *pEnt;
	if ( FStrEq( args[1], "" ) || FStrEq( args[1], "!picker" ) )
	{
		if (!pPlayer)
			return;

		extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );
		pEnt = FindPickerEntity( pPlayer );

		if ( !pEnt )
		{
			ClientPrint( pPlayer, HUD_PRINTCONSOLE, "No entity in front of player.\n" );
			return;
		}
	}
	else if ( FStrEq( args[1], "!self" ) || FStrEq( args[1], "!caller" ) || FStrEq( args[1], "!activator" ) )
	{
		if (!pPlayer)
			return;

		pEnt = pPlayer;
	}
	else
	{
		int nID = atoi( args[1] );

		pEnt = g_ServerTools.FindEntityByHammerID( nID );
		if ( !pEnt )
		{
			Msg( "Entity ID %d not found.\n", nID );
			return;
		}
	}

	int nArg = 2;
	while ( nArg < args.ArgC() )
	{
		const char *pszKey = args[ nArg ];
		const char *pszValue = args[ nArg + 1 ];
		nArg += 2;

		g_ServerTools.SetKeyValue( pEnt, pszKey, pszValue );
	}
} 

static ConCommand ent_keyvalue("ent_keyvalue", CC_Ent_Keyvalue, "Applies the comma delimited key=value pairs to the entity with the given Hammer ID.\n\tFormat: ent_keyvalue <entity id> <key1> <value1> <key2> <value2> ... <keyN> <valueN>\n", FCVAR_CHEAT);
