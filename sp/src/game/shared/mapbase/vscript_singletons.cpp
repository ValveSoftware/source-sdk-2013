//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: This file contains brand new VScript singletons and singletons replicated from API
//			documentation in other games.
//
//			See vscript_funcs_shared.cpp for more information.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <vgui_controls/Controls.h> 
#include <vgui/ILocalize.h>
#include "ammodef.h"
#include "tier1/utlcommon.h"

#ifndef CLIENT_DLL
#include "ai_squad.h"
#endif // !CLIENT_DLL

#include "usermessages.h"
#include "filesystem.h"
#include "igameevents.h"
#include "engine/ivdebugoverlay.h"

#ifdef CLIENT_DLL
#include "IEffects.h"
#include "fx.h"
#include "itempents.h"
#include "c_te_legacytempents.h"
#include "iefx.h"
#include "dlight.h"
#endif

#include "vscript_singletons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IScriptManager *scriptmanager;
CNetMsgScriptHelper *g_ScriptNetMsg = new CNetMsgScriptHelper();

//=============================================================================
// Net Prop Manager
// Based on L4D2 API
//=============================================================================
class CScriptNetPropManager
{
public:

#ifdef CLIENT_DLL
	RecvProp *RecurseTable( RecvTable *pTable, const char *pszPropName )
#else
	SendProp *RecurseTable( SendTable *pTable, const char *pszPropName )
#endif
	{
#ifdef CLIENT_DLL
		RecvProp *pProp = NULL;
#else
		SendProp *pProp = NULL;
#endif
		for (int i = 0; i < pTable->GetNumProps(); i++)
		{
			pProp = pTable->GetProp( i );
			if (pProp->GetType() == DPT_DataTable)
			{
				pProp = RecurseTable(pProp->GetDataTable(), pszPropName);
				if (pProp)
					return pProp;
			}
			else
			{
				if (FStrEq( pProp->GetName(), pszPropName ))
					return pProp;
			}
		}

		return NULL;
	}

#ifdef CLIENT_DLL
	RecvProp *RecurseNetworkClass( ClientClass *pClass, const char *pszPropName )
#else
	SendProp *RecurseNetworkClass( ServerClass *pClass, const char *pszPropName )
#endif
	{
#ifdef CLIENT_DLL
		RecvProp *pProp = RecurseTable( pClass->m_pRecvTable, pszPropName );
#else
		SendProp *pProp = RecurseTable( pClass->m_pTable, pszPropName );
#endif
		if (pProp)
			return pProp;

		if (pClass->m_pNext)
			return RecurseNetworkClass( pClass->m_pNext, pszPropName );
		else
			return NULL;
	}

#ifdef CLIENT_DLL
	RecvProp *GetPropByName( CBaseEntity *pEnt, const char *pszPropName )
	{
		if (pEnt)
		{
			return RecurseNetworkClass( pEnt->GetClientClass(), pszPropName );
		}

		return NULL;
	}
#else
	SendProp *GetPropByName( CBaseEntity *pEnt, const char *pszPropName )
	{
		if (pEnt)
		{
			return RecurseNetworkClass( pEnt->GetServerClass(), pszPropName );
		}

		return NULL;
	}
#endif

	int GetPropArraySize( HSCRIPT hEnt, const char *pszPropName )
	{
		CBaseEntity *pEnt = ToEnt( hEnt );
		auto *pProp = GetPropByName( pEnt, pszPropName );
		if (pProp)
		{
			// TODO: Is this what this function wants?
			return pProp->GetNumElements();
		}

		return -1;
	}

	#define GetPropFunc( name, varType, propType, defaultval ) \
	varType name( HSCRIPT hEnt, const char *pszPropName ) \
	{ \
		CBaseEntity *pEnt = ToEnt( hEnt ); \
		auto *pProp = GetPropByName( pEnt, pszPropName ); \
		if (pProp && pProp->GetType() == propType) \
		{ \
			return *(varType*)((char *)pEnt + pProp->GetOffset()); \
		} \
		return defaultval; \
	} \

	#define GetPropFuncArray( name, varType, propType, defaultval ) \
	varType name( HSCRIPT hEnt, const char *pszPropName, int iArrayElement ) \
	{ \
		CBaseEntity *pEnt = ToEnt( hEnt ); \
		auto *pProp = GetPropByName( pEnt, pszPropName ); \
		if (pProp && pProp->GetType() == propType) \
		{ \
			return ((varType*)((char *)pEnt + pProp->GetOffset()))[iArrayElement]; \
		} \
		return defaultval; \
	} \

	GetPropFunc( GetPropFloat, float, DPT_Float, -1 );
	GetPropFuncArray( GetPropFloatArray, float, DPT_Float, -1 );
	GetPropFunc( GetPropInt, int, DPT_Int, -1 );
	GetPropFuncArray( GetPropIntArray, int, DPT_Int, -1 );
	GetPropFunc( GetPropVector, Vector, DPT_Vector, vec3_invalid );
	GetPropFuncArray( GetPropVectorArray, Vector, DPT_Vector, vec3_invalid );

	HSCRIPT GetPropEntity( HSCRIPT hEnt, const char *pszPropName )
	{
		CBaseEntity *pEnt = ToEnt( hEnt );
		auto *pProp = GetPropByName( pEnt, pszPropName );
		if (pProp && pProp->GetType() == DPT_Int)
		{
			return ToHScript( *(CHandle<CBaseEntity>*)((char *)pEnt + pProp->GetOffset()) );
		}

		return NULL;
	}

	HSCRIPT GetPropEntityArray( HSCRIPT hEnt, const char *pszPropName, int iArrayElement )
	{
		CBaseEntity *pEnt = ToEnt( hEnt );
		auto *pProp = GetPropByName( pEnt, pszPropName );
		if (pProp && pProp->GetType() == DPT_Int)
		{
			return ToHScript( ((CHandle<CBaseEntity>*)((char *)pEnt + pProp->GetOffset()))[iArrayElement] );
		}

		return NULL;
	}

	const char *GetPropString( HSCRIPT hEnt, const char *pszPropName )
	{
		CBaseEntity *pEnt = ToEnt( hEnt );
		auto *pProp = GetPropByName( pEnt, pszPropName );
		if (pProp && pProp->GetType() == DPT_Int)
		{
			return (const char*)((char *)pEnt + pProp->GetOffset());
		}

		return NULL;
	}

	const char *GetPropStringArray( HSCRIPT hEnt, const char *pszPropName, int iArrayElement )
	{
		CBaseEntity *pEnt = ToEnt( hEnt );
		auto *pProp = GetPropByName( pEnt, pszPropName );
		if (pProp && pProp->GetType() == DPT_Int)
		{
			return ((const char**)((char *)pEnt + pProp->GetOffset()))[iArrayElement];
		}

		return NULL;
	}

	const char *GetPropType( HSCRIPT hEnt, const char *pszPropName )
	{
		CBaseEntity *pEnt = ToEnt( hEnt );
		auto *pProp = GetPropByName( pEnt, pszPropName );
		if (pProp)
		{
			switch (pProp->GetType())
			{
			case DPT_Int:		return "integer";
			case DPT_Float:		return "float";
			case DPT_Vector:	return "vector";
			case DPT_VectorXY:	return "vector2d";
			case DPT_String:	return "string";
			case DPT_Array:		return "array";
			case DPT_DataTable:	return "datatable";
			}
		}

		return NULL;
	}

	bool HasProp( HSCRIPT hEnt, const char *pszPropName )
	{
		CBaseEntity *pEnt = ToEnt( hEnt );
		return GetPropByName( pEnt, pszPropName ) != NULL;
	}

	#define SetPropFunc( name, varType, propType ) \
	void name( HSCRIPT hEnt, const char *pszPropName, varType value ) \
	{ \
		CBaseEntity *pEnt = ToEnt( hEnt ); \
		auto *pProp = GetPropByName( pEnt, pszPropName ); \
		if (pProp && pProp->GetType() == propType) \
		{ \
			*(varType*)((char *)pEnt + pProp->GetOffset()) = value; \
		} \
	} \

	#define SetPropFuncArray( name, varType, propType ) \
	void name( HSCRIPT hEnt, const char *pszPropName, varType value, int iArrayElement ) \
	{ \
		CBaseEntity *pEnt = ToEnt( hEnt ); \
		auto *pProp = GetPropByName( pEnt, pszPropName ); \
		if (pProp && pProp->GetType() == propType) \
		{ \
			((varType*)((char *)pEnt + pProp->GetOffset()))[iArrayElement] = value; \
		} \
	} \

	SetPropFunc( SetPropFloat, float, DPT_Float );
	SetPropFuncArray( SetPropFloatArray, float, DPT_Float );
	SetPropFunc( SetPropInt, int, DPT_Int );
	SetPropFuncArray( SetPropIntArray, int, DPT_Int );
	SetPropFunc( SetPropVector, Vector, DPT_Vector );
	SetPropFuncArray( SetPropVectorArray, Vector, DPT_Vector );
	SetPropFunc( SetPropString, const char*, DPT_String );
	SetPropFuncArray( SetPropStringArray, const char*, DPT_String );

	void SetPropEntity( HSCRIPT hEnt, const char *pszPropName, HSCRIPT value )
	{
		CBaseEntity *pEnt = ToEnt( hEnt );
		auto *pProp = GetPropByName( pEnt, pszPropName );
		if (pProp && pProp->GetType() == DPT_Int)
		{
			*((CHandle<CBaseEntity>*)((char *)pEnt + pProp->GetOffset())) = ToEnt(value);
		}
	}

	HSCRIPT SetPropEntityArray( HSCRIPT hEnt, const char *pszPropName, HSCRIPT value, int iArrayElement )
	{
		CBaseEntity *pEnt = ToEnt( hEnt );
		auto *pProp = GetPropByName( pEnt, pszPropName );
		if (pProp && pProp->GetType() == DPT_Int)
		{
			((CHandle<CBaseEntity>*)((char *)pEnt + pProp->GetOffset()))[iArrayElement] = ToEnt(value);
		}

		return NULL;
	}

private:
} g_ScriptNetPropManager;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptNetPropManager, "CNetPropManager", SCRIPT_SINGLETON "Allows reading and updating the network properties of an entity." )
	DEFINE_SCRIPTFUNC( GetPropArraySize, "Returns the size of an netprop array, or -1." )
	DEFINE_SCRIPTFUNC( GetPropEntity, "Reads an EHANDLE valued netprop (21 bit integer). Returns the script handle of the entity." )
	DEFINE_SCRIPTFUNC( GetPropEntityArray, "Reads an EHANDLE valued netprop (21 bit integer) from an array. Returns the script handle of the entity." )
	DEFINE_SCRIPTFUNC( GetPropFloat, "Reads a float valued netprop." )
	DEFINE_SCRIPTFUNC( GetPropFloatArray, "Reads a float valued netprop from an array." )
	DEFINE_SCRIPTFUNC( GetPropInt, "Reads an integer valued netprop." )
	DEFINE_SCRIPTFUNC( GetPropIntArray, "Reads an integer valued netprop from an array." )
	DEFINE_SCRIPTFUNC( GetPropString, "Reads a string valued netprop." )
	DEFINE_SCRIPTFUNC( GetPropStringArray, "Reads a string valued netprop from an array." )
	DEFINE_SCRIPTFUNC( GetPropVector, "Reads a 3D vector valued netprop." )
	DEFINE_SCRIPTFUNC( GetPropVectorArray, "Reads a 3D vector valued netprop from an array." )
	DEFINE_SCRIPTFUNC( GetPropType, "Returns the name of the netprop type as a string." )
	DEFINE_SCRIPTFUNC( HasProp, "Checks if a netprop exists." )
	DEFINE_SCRIPTFUNC( SetPropEntity, "Sets an EHANDLE valued netprop (21 bit integer) to reference the specified entity." )
	DEFINE_SCRIPTFUNC( SetPropEntityArray, "Sets an EHANDLE valued netprop (21 bit integer) from an array to reference the specified entity." )
	DEFINE_SCRIPTFUNC( SetPropFloat, "Sets a netprop to the specified float." )
	DEFINE_SCRIPTFUNC( SetPropFloatArray, "Sets a netprop from an array to the specified float." )
	DEFINE_SCRIPTFUNC( SetPropInt, "Sets a netprop to the specified integer." )
	DEFINE_SCRIPTFUNC( SetPropIntArray, "Sets a netprop from an array to the specified integer." )
	DEFINE_SCRIPTFUNC( SetPropString, "Sets a netprop to the specified string." )
	DEFINE_SCRIPTFUNC( SetPropStringArray, "Sets a netprop from an array to the specified string." )
	DEFINE_SCRIPTFUNC( SetPropVector, "Sets a netprop to the specified vector." )
	DEFINE_SCRIPTFUNC( SetPropVectorArray, "Sets a netprop from an array to the specified vector." )
END_SCRIPTDESC();

//=============================================================================
// Localization Interface
// Unique to Mapbase
//=============================================================================
class CScriptLocalize
{
public:

	const char *GetTokenAsUTF8( const char *pszToken )
	{
		const char *pText = g_pVGuiLocalize->FindAsUTF8( pszToken );
		if ( pText )
		{
			return pText;
		}

		return NULL;
	}

	void AddStringAsUTF8( const char *pszToken, const char *pszString )
	{
		wchar_t wpszString[256];
		g_pVGuiLocalize->ConvertANSIToUnicode( pszString, wpszString, sizeof(wpszString) );

		// TODO: This is a fake file name! Should "fileName" mean anything?
		g_pVGuiLocalize->AddString( pszToken, wpszString, "resource/vscript_localization.txt" );
	}

private:
} g_ScriptLocalize;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptLocalize, "CLocalize", SCRIPT_SINGLETON "Accesses functions related to localization strings." )

	DEFINE_SCRIPTFUNC( GetTokenAsUTF8, "Gets the current language's token as a UTF-8 string (not Unicode)." )

	DEFINE_SCRIPTFUNC( AddStringAsUTF8, "Adds a new localized token as a UTF-8 string (not Unicode)." )

END_SCRIPTDESC();

//=============================================================================
// Game Event Listener
// Based on Source 2 API
//=============================================================================

// Define to use the older code that loads all events manually independent from the game event manager.
// Otherwise access event descriptors directly from engine.
//#define USE_OLD_EVENT_DESCRIPTORS 1

class CScriptGameEventListener : public IGameEventListener2, public CAutoGameSystem
{
public:
	CScriptGameEventListener() : m_bActive(false) /*, m_nEventTick(0)*/ {}
	~CScriptGameEventListener()
	{
		StopListeningForEvent();
	}

	int ListenToGameEvent( const char* szEvent, HSCRIPT hFunc, const char* szContext );
	void StopListeningForEvent();

public:
	static bool StopListeningToGameEvent( int listener );
	static void StopListeningToAllGameEvents( const char* szContext );

public:
	void FireGameEvent( IGameEvent *event );
	void LevelShutdownPreEntity();

private:
	//int m_index;
	HSCRIPT m_hCallback;
	unsigned int m_iContextHash;
	bool m_bActive;
	//int m_nEventTick;

	static StringHashFunctor Hash;
	static inline unsigned int HashContext( const char* c ) { return (c && *c) ? Hash(c) : 0; }

	inline int GetIndex()
	{
		Assert( sizeof(CScriptGameEventListener*) == sizeof(int) );
		return reinterpret_cast<intptr_t>(this);
	}

public:
	enum // event data types, dependant on engine definitions
	{
		TYPE_LOCAL  = 0,
		TYPE_STRING = 1,
		TYPE_FLOAT  = 2,
		TYPE_LONG   = 3,
		TYPE_SHORT  = 4,
		TYPE_BYTE   = 5,
		TYPE_BOOL   = 6
	};
	static void WriteEventData( IGameEvent *event, HSCRIPT hTable );

#ifdef USE_OLD_EVENT_DESCRIPTORS
	static void LoadAllEvents();
	static void LoadEventsFromFile( const char *filename, const char *pathID = NULL );
	static CUtlMap< unsigned int, KeyValues* > s_GameEvents;
	static CUtlVector< KeyValues* > s_LoadedFiles;
#endif

public:
	//static int g_nIndexCounter;
	static CUtlVectorAutoPurge< CScriptGameEventListener* > s_Listeners;
#if _DEBUG
	static void DumpEventListeners();
#endif

};

CUtlVectorAutoPurge< CScriptGameEventListener* > CScriptGameEventListener::s_Listeners;
StringHashFunctor CScriptGameEventListener::Hash;

#ifdef USE_OLD_EVENT_DESCRIPTORS
CUtlMap< unsigned int, KeyValues* > CScriptGameEventListener::s_GameEvents( DefLessFunc(unsigned int) );
CUtlVector< KeyValues* > CScriptGameEventListener::s_LoadedFiles;
#endif


#if _DEBUG
#ifdef CLIENT_DLL
CON_COMMAND_F( cl_dump_script_game_event_listeners, "Dump all game event listeners created from script.", FCVAR_CHEAT )
{
	CScriptGameEventListener::DumpEventListeners();
}
#else
CON_COMMAND_F( dump_script_game_event_listeners, "Dump all game event listeners created from script.", FCVAR_CHEAT )
{
	CScriptGameEventListener::DumpEventListeners();
}
#endif
#endif


#ifdef USE_OLD_EVENT_DESCRIPTORS
//-----------------------------------------------------------------------------
// Executed in LevelInitPreEntity
//-----------------------------------------------------------------------------
void CScriptGameEventListener::LoadAllEvents()
{
	// Listed in the same order they are loaded in GameEventManager
	const char *filenames[] =
	{
		"resource/serverevents.res",
		"resource/gameevents.res",
		"resource/mapbaseevents.res",
		"resource/modevents.res"
	};

	const char *pathlist[] =
	{
		"GAME",
		"MOD"
	};

	// Destroy old KeyValues
	if ( s_LoadedFiles.Count() )
	{
		for ( int i = s_LoadedFiles.Count(); i--; )
			s_LoadedFiles[i]->deleteThis();
		s_LoadedFiles.Purge();
		s_GameEvents.Purge();
	}

	for ( int j = 0; j < ARRAYSIZE(pathlist); ++j )
		for ( int i = 0; i < ARRAYSIZE(filenames); ++i )
		{
			LoadEventsFromFile( filenames[i], pathlist[j] );
		}
}

//-----------------------------------------------------------------------------
// Load event files into a lookup array to be able to return the event data to the VM.
//-----------------------------------------------------------------------------
void CScriptGameEventListener::LoadEventsFromFile( const char *filename, const char *pathID )
{
	KeyValues *pKV = new KeyValues("GameEvents");

	if ( !pKV->LoadFromFile( filesystem, filename, pathID ) )
	{
		// CGMsg( 1, CON_GROUP_VSCRIPT, "CScriptGameEventListener::LoadEventsFromFile: Failed to load file [%s]%s\n", pathID, filename );
		pKV->deleteThis();
		return;
	}

	int count = 0;

	for ( KeyValues *key = pKV->GetFirstSubKey(); key; key = key->GetNextKey() )
	{
		for ( KeyValues *sub = key->GetFirstSubKey(); sub; sub = sub->GetNextKey() )
		{
			if ( sub->GetDataType() == KeyValues::TYPE_STRING )
			{
				const char *szVal = sub->GetString();
				if ( !V_stricmp( szVal, "string" ) )
				{
					sub->SetInt( NULL, TYPE_STRING );
				}
				else if ( !V_stricmp( szVal, "bool" ) )
				{
					sub->SetInt( NULL, TYPE_BOOL );
				}
				else if ( !V_stricmp( szVal, "byte" ) )
				{
					sub->SetInt( NULL, TYPE_BYTE );
				}
				else if ( !V_stricmp( szVal, "short" ) )
				{
					sub->SetInt( NULL, TYPE_SHORT );
				}
				else if ( !V_stricmp( szVal, "long" ) )
				{
					sub->SetInt( NULL, TYPE_LONG );
				}
				else if ( !V_stricmp( szVal, "float" ) )
				{
					sub->SetInt( NULL, TYPE_FLOAT );
				}
			}
			// none   : value is not networked
			// string : a zero terminated string
			// bool   : unsigned int, 1 bit
			// byte   : unsigned int, 8 bit
			// short  : signed int, 16 bit
			// long   : signed int, 32 bit
			// float  : float, 32 bit
		}

		// Store event subkeys
		// Replace key so modevents can overwrite gameevents.
		// It does not check for hash collisions, however.
		s_GameEvents.InsertOrReplace( Hash( key->GetName() ), key );
		++count;
	}

	// Store files (allocated KV)
	s_LoadedFiles.AddToTail( pKV );

	CGMsg( 2, CON_GROUP_VSCRIPT, "CScriptGameEventListener::LoadEventsFromFile: Loaded [%s]%s (%i)\n", pathID, filename, count );
}
#endif

#if _DEBUG
void CScriptGameEventListener::DumpEventListeners()
{
	CGMsg( 0, CON_GROUP_VSCRIPT, "--- Script game event listener dump start\n" );
	CGMsg( 0, CON_GROUP_VSCRIPT, "#    ADDRESS      ID         CONTEXT\n" );
	FOR_EACH_VEC( s_Listeners, i )
	{
		CGMsg( 0, CON_GROUP_VSCRIPT, " %d   (0x%p) %d : %u\n", i,
										(void*)s_Listeners[i],
										s_Listeners[i]->GetIndex(),
										s_Listeners[i]->m_iContextHash );
	}
	CGMsg( 0, CON_GROUP_VSCRIPT, "--- Script game event listener dump end\n" );
}
#endif

void CScriptGameEventListener::LevelShutdownPreEntity()
{
	s_Listeners.FindAndFastRemove(this);
	delete this;
}

void CScriptGameEventListener::FireGameEvent( IGameEvent *event )
{
	//m_nEventTick = gpGlobals->tickcount;
	ScriptVariant_t hTable;
	g_pScriptVM->CreateTable( hTable );
	WriteEventData( event, hTable );
	g_pScriptVM->SetValue( hTable, "game_event_listener", GetIndex() );
	// g_pScriptVM->SetValue( hTable, "game_event_name", event->GetName() );
	g_pScriptVM->ExecuteFunction( m_hCallback, &hTable, 1, NULL, NULL, true );
	g_pScriptVM->ReleaseScript( hTable );
}

struct CGameEventDescriptor
{
	byte _0[36];
	KeyValues *m_pEventKeys;
	//byte _1[22];
};

class CGameEvent__// : public IGameEvent
{
public:
	virtual ~CGameEvent__();				// [0]
	CGameEventDescriptor *m_pDescriptor;	// 0x04
	//KeyValues *m_pEventData;				// 0x08
};

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CScriptGameEventListener::WriteEventData( IGameEvent *event, HSCRIPT hTable )
{
#ifdef USE_OLD_EVENT_DESCRIPTORS
	int i = s_GameEvents.Find( Hash( event->GetName() ) );
	if ( i == s_GameEvents.InvalidIndex() )
		return;
	KeyValues *pKV = s_GameEvents[i];
#endif

#if defined(_DEBUG) && !defined(USE_OLD_EVENT_DESCRIPTORS)
	try
	{
#endif

#if !defined(USE_OLD_EVENT_DESCRIPTORS)
	KeyValues *pKV = reinterpret_cast< CGameEvent__* >(event)->m_pDescriptor->m_pEventKeys;
#endif

	for ( KeyValues *sub = pKV->GetFirstSubKey(); sub; sub = sub->GetNextKey() )
	{
		const char *szKey = sub->GetName();
		switch ( sub->GetInt() )
		{
			case TYPE_LOCAL:
			case TYPE_STRING: g_pScriptVM->SetValue( hTable, szKey, event->GetString( szKey ) ); break;
			case TYPE_FLOAT:  g_pScriptVM->SetValue( hTable, szKey, event->GetFloat ( szKey ) ); break;
			case TYPE_BOOL:   g_pScriptVM->SetValue( hTable, szKey, event->GetBool  ( szKey ) ); break;
			default:          g_pScriptVM->SetValue( hTable, szKey, event->GetInt   ( szKey ) );
		}
	}

#if defined(_DEBUG) && !defined(USE_OLD_EVENT_DESCRIPTORS)
	// Access a bunch of KeyValues functions to validate it is the correct address.
	// This may not always throw an exception when it is incorrect, but eventually it will.
	}
	catch (...)
	{
		// CGameEvent or CGameEventDescriptor offsets did not match!
		// This should mean these were modified in engine.dll.
		//
		// Implement this utility yourself by adding a function to get event descriptor keys
		// either on CGameEventManager or on CGameEvent interfaces.
		// On CGameEventManager downcast IGameEvent input to CGameEvent, then return event->descriptor->keys
		// On CGameEvent return (member) descriptor->keys
		//
		// Finally assign it to pKV above.

		Warning("CScriptGameEventListener::WriteEventData internal error\n");
		Assert(0);
	}
#endif
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int CScriptGameEventListener::ListenToGameEvent( const char* szEvent, HSCRIPT hFunc, const char* szContext )
{
	bool bValid;

	if ( gameeventmanager && hFunc )
#ifdef CLIENT_DLL
		bValid = gameeventmanager->AddListener( this, szEvent, false );
#else
		bValid = gameeventmanager->AddListener( this, szEvent, true );
#endif
	else bValid = false;

	if ( bValid )
	{
		m_iContextHash = HashContext( szContext );
		m_hCallback = hFunc;
		m_bActive = true;

		s_Listeners.AddToTail( this );

		return GetIndex();
	}
	else
	{
		delete this;
		return 0;
	}
}

//-----------------------------------------------------------------------------
// Free stuff. Called from the destructor, does not remove itself from the listener list.
//-----------------------------------------------------------------------------
void CScriptGameEventListener::StopListeningForEvent()
{
	if ( !m_bActive )
		return;

	if ( g_pScriptVM )
		g_pScriptVM->ReleaseScript( m_hCallback );

	m_hCallback = NULL;
	m_bActive = false;

	if ( gameeventmanager )
		gameeventmanager->RemoveListener( this );

	// Event listeners are iterated forwards in the game event manager,
	// removing while iterating will cause it to skip one listener.
	// This could be prevented by writing a custom game event manager.
	//if ( m_nEventTick == gpGlobals->tickcount )
	//{
	//	Warning("CScriptGameEventListener stopped in the same frame it was fired. This will break other event listeners!\n");
	//}
}

//-----------------------------------------------------------------------------
// Stop the specified event listener.
//-----------------------------------------------------------------------------
bool CScriptGameEventListener::StopListeningToGameEvent( int listener )
{
	CScriptGameEventListener *p = reinterpret_cast<CScriptGameEventListener*>(listener); // INT_TO_POINTER	

	bool bRemoved = s_Listeners.FindAndFastRemove(p);
	if ( bRemoved )
	{
		delete p;
	}

	return bRemoved;
}

//-----------------------------------------------------------------------------
// Stops listening to all events within a context.
//-----------------------------------------------------------------------------
void CScriptGameEventListener::StopListeningToAllGameEvents( const char* szContext )
{
	unsigned int hash = HashContext( szContext );

	// Iterate from the end so they can be safely removed as they are deleted
	for ( int i = s_Listeners.Count(); i--; )
	{
		CScriptGameEventListener *pCur = s_Listeners[i];
		if ( pCur->m_iContextHash == hash )
		{
			s_Listeners.Remove(i); // keep list order
			delete pCur;
		}
	}
}

//=============================================================================
//=============================================================================

static int ListenToGameEvent( const char* szEvent, HSCRIPT hFunc, const char* szContext )
{
	CScriptGameEventListener *p = new CScriptGameEventListener();
	return p->ListenToGameEvent( szEvent, hFunc, szContext );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static void FireGameEvent( const char* szEvent, HSCRIPT hTable )
{
	IGameEvent *event = gameeventmanager->CreateEvent( szEvent );
	if ( event )
	{
		ScriptVariant_t key, val;
		int nIterator = -1;
		while ( ( nIterator = g_pScriptVM->GetKeyValue( hTable, nIterator, &key, &val ) ) != -1 )
		{
			switch ( val.m_type )
			{
				case FIELD_FLOAT:   event->SetFloat ( key.m_pszString, val.m_float     ); break;
				case FIELD_INTEGER: event->SetInt   ( key.m_pszString, val.m_int       ); break;
				case FIELD_BOOLEAN: event->SetBool  ( key.m_pszString, val.m_bool      ); break;
				case FIELD_CSTRING: event->SetString( key.m_pszString, val.m_pszString ); break;
			}

			g_pScriptVM->ReleaseValue(key);
			g_pScriptVM->ReleaseValue(val);
		}

#ifdef CLIENT_DLL
		gameeventmanager->FireEventClientSide(event);
#else
		gameeventmanager->FireEvent(event);
#endif
	}
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Copy of FireGameEvent, server only with no broadcast to clients.
//-----------------------------------------------------------------------------
static void FireGameEventLocal( const char* szEvent, HSCRIPT hTable )
{
	IGameEvent *event = gameeventmanager->CreateEvent( szEvent );
	if ( event )
	{
		ScriptVariant_t key, val;
		int nIterator = -1;
		while ( ( nIterator = g_pScriptVM->GetKeyValue( hTable, nIterator, &key, &val ) ) != -1 )
		{
			switch ( val.m_type )
			{
				case FIELD_FLOAT:   event->SetFloat ( key.m_pszString, val.m_float     ); break;
				case FIELD_INTEGER: event->SetInt   ( key.m_pszString, val.m_int       ); break;
				case FIELD_BOOLEAN: event->SetBool  ( key.m_pszString, val.m_bool      ); break;
				case FIELD_CSTRING: event->SetString( key.m_pszString, val.m_pszString ); break;
			}

			g_pScriptVM->ReleaseValue(key);
			g_pScriptVM->ReleaseValue(val);
		}

		gameeventmanager->FireEvent(event,true);
	}
}
#endif // !CLIENT_DLL

//=============================================================================
// Save/Restore Utility
// Based on L4D2 API
//=============================================================================
class CScriptSaveRestoreUtil : public CAutoGameSystem
{
public:
	static void SaveTable( const char *szId, HSCRIPT hTable );
	static void RestoreTable( const char *szId, HSCRIPT hTable );
	static void ClearSavedTable( const char *szId );

public: // IGameSystem

	void OnSave()
	{
		if ( g_pScriptVM )
		{
			HSCRIPT hFunc = g_pScriptVM->LookupFunction( "OnSave" );
			if ( hFunc )
			{
				g_pScriptVM->Call( hFunc );
				g_pScriptVM->ReleaseScript( hFunc );
			}
		}
	}

#ifdef CLIENT_DLL
	// On the client, OnRestore() is called before VScript is actually restored, so this has to be called manually from VScript save/restore instead
	void OnVMRestore()
#else
	void OnRestore()
#endif
	{
		if ( g_pScriptVM )
		{
			HSCRIPT hFunc = g_pScriptVM->LookupFunction( "OnRestore" );
			if ( hFunc )
			{
				g_pScriptVM->Call( hFunc );
				g_pScriptVM->ReleaseScript( hFunc );
			}
		}
	}

	void Shutdown()
	{
		FOR_EACH_MAP_FAST( m_Lookup, i )
			m_Lookup[i]->deleteThis();
		m_Lookup.Purge();
	}

private:
	static StringHashFunctor Hash;
	static CUtlMap< unsigned int, KeyValues* > m_Lookup;

} g_ScriptSaveRestoreUtil;

#ifdef CLIENT_DLL
void VScriptSaveRestoreUtil_OnVMRestore()
{
	g_ScriptSaveRestoreUtil.OnVMRestore();
}
#endif

CUtlMap< unsigned int, KeyValues* > CScriptSaveRestoreUtil::m_Lookup( DefLessFunc(unsigned int) );
StringHashFunctor CScriptSaveRestoreUtil::Hash;

//-----------------------------------------------------------------------------
// Store a table with primitive values that will persist across level transitions and save loads.
// Case sensitive
//-----------------------------------------------------------------------------
void CScriptSaveRestoreUtil::SaveTable( const char *szId, HSCRIPT hTable )
{
	KeyValues *pKV;
	unsigned int hash = Hash(szId);

	int idx = m_Lookup.Find( hash );
	if ( idx == m_Lookup.InvalidIndex() )
	{
		pKV = new KeyValues("ScriptSavedTable");
		m_Lookup.Insert( hash, pKV );
	}
	else
	{
		pKV = m_Lookup[idx];
		pKV->Clear();
	}

	ScriptVariant_t key, val;
	int nIterator = -1;
	while ( ( nIterator = g_pScriptVM->GetKeyValue( hTable, nIterator, &key, &val ) ) != -1 )
	{
		switch ( val.m_type )
		{
			case FIELD_FLOAT:   pKV->SetFloat ( key.m_pszString, val.m_float     ); break;
			case FIELD_INTEGER: pKV->SetInt   ( key.m_pszString, val.m_int       ); break;
			case FIELD_BOOLEAN: pKV->SetBool  ( key.m_pszString, val.m_bool      ); break;
			case FIELD_CSTRING: pKV->SetString( key.m_pszString, val.m_pszString ); break;
		}

		g_pScriptVM->ReleaseValue(key);
		g_pScriptVM->ReleaseValue(val);
	}
}

//-----------------------------------------------------------------------------
// Retrieves a table from storage. Write into input table.
//-----------------------------------------------------------------------------
void CScriptSaveRestoreUtil::RestoreTable( const char *szId, HSCRIPT hTable )
{
	int idx = m_Lookup.Find( Hash(szId) );
	if ( idx == m_Lookup.InvalidIndex() )
	{
		// DevWarning( 2, "RestoreTable could not find saved table with context '%s'\n", szId );
		return;
	}

	KeyValues *pKV = m_Lookup[idx];
	FOR_EACH_SUBKEY( pKV, key )
	{
		switch ( key->GetDataType() )
		{
			case KeyValues::TYPE_STRING: g_pScriptVM->SetValue( hTable, key->GetName(), key->GetString() ); break;
			case KeyValues::TYPE_INT:    g_pScriptVM->SetValue( hTable, key->GetName(), key->GetInt()    ); break;
			case KeyValues::TYPE_FLOAT:  g_pScriptVM->SetValue( hTable, key->GetName(), key->GetFloat()  ); break;
		}
	}
}

//-----------------------------------------------------------------------------
// Remove a saved table.
//-----------------------------------------------------------------------------
void CScriptSaveRestoreUtil::ClearSavedTable( const char *szId )
{
	int idx = m_Lookup.Find( Hash(szId) );
	if ( idx != m_Lookup.InvalidIndex() )
	{
		m_Lookup[idx]->deleteThis();
		m_Lookup.RemoveAt( idx );
	}
	else
	{
		// DevWarning( 2, "ClearSavedTable could not find saved table with context '%s'\n", szId );
	}
}

//=============================================================================
// Read/Write to File
// Based on L4D2/Source 2 API
//=============================================================================
#define SCRIPT_MAX_FILE_READ_SIZE  (16 * 1024)			// 16KB
#define SCRIPT_MAX_FILE_WRITE_SIZE (64 * 1024 * 1024)	// 64MB
#define SCRIPT_RW_PATH_ID "MOD"
#define SCRIPT_RW_FULL_PATH_FMT "vscript_io/%s"

class CScriptReadWriteFile : public CAutoGameSystem
{
	// A singleton class with all static members is used to be able to free the read string on level shutdown,
	// and register script funcs directly. Same reason applies to CScriptSaveRestoreUtil
public:
	static bool FileWrite( const char *szFile, const char *szInput );
	static const char *FileRead( const char *szFile );

	// NOTE: These two functions are new with Mapbase and have no Valve equivalent
	static bool KeyValuesWrite( const char *szFile, HSCRIPT hInput );
	static HSCRIPT KeyValuesRead( const char *szFile );

	void LevelShutdownPostEntity()
	{
		if ( m_pszReturnReadFile )
		{
			delete[] m_pszReturnReadFile;
			m_pszReturnReadFile = NULL;
		}
	}

private:
	static const char *m_pszReturnReadFile;

} g_ScriptReadWrite;

const char *CScriptReadWriteFile::m_pszReturnReadFile = NULL;

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CScriptReadWriteFile::FileWrite( const char *szFile, const char *szInput )
{
	size_t len = strlen(szInput);
	if ( len > SCRIPT_MAX_FILE_WRITE_SIZE )
	{
		DevWarning( 2, "Input is too large for a ScriptFileWrite ( %s / %d MB )\n", V_pretifymem(len,2,true), (SCRIPT_MAX_FILE_WRITE_SIZE >> 20) );
		return false;
	}

	char pszFullName[MAX_PATH];
	V_snprintf( pszFullName, sizeof(pszFullName), SCRIPT_RW_FULL_PATH_FMT, szFile );

	if ( !V_RemoveDotSlashes( pszFullName, CORRECT_PATH_SEPARATOR, true ) )
	{
		DevWarning( 2, "Invalid file location : %s\n", szFile );
		return false;
	}

	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	buf.PutString(szInput);

	int nSize = V_strlen(pszFullName) + 1;
	char *pszDir = (char*)stackalloc(nSize);
	V_memcpy( pszDir, pszFullName, nSize );
	V_StripFilename( pszDir );

	g_pFullFileSystem->CreateDirHierarchy( pszDir, SCRIPT_RW_PATH_ID );
	bool res = g_pFullFileSystem->WriteFile( pszFullName, SCRIPT_RW_PATH_ID, buf );
	buf.Purge();
	return res;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
const char *CScriptReadWriteFile::FileRead( const char *szFile )
{
	char pszFullName[MAX_PATH];
	V_snprintf( pszFullName, sizeof(pszFullName), SCRIPT_RW_FULL_PATH_FMT, szFile );

	if ( !V_RemoveDotSlashes( pszFullName, CORRECT_PATH_SEPARATOR, true ) )
	{
		DevWarning( 2, "Invalid file location : %s\n", szFile );
		return NULL;
	}

	unsigned int size = g_pFullFileSystem->Size( pszFullName, SCRIPT_RW_PATH_ID );
	if ( size >= SCRIPT_MAX_FILE_READ_SIZE )
	{
		DevWarning( 2, "File '%s' (from '%s') is too large for a ScriptFileRead ( %s / %u bytes )\n", pszFullName, szFile, V_pretifymem(size,2,true), SCRIPT_MAX_FILE_READ_SIZE );
		return NULL;
	}

	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	if ( !g_pFullFileSystem->ReadFile( pszFullName, SCRIPT_RW_PATH_ID, buf, SCRIPT_MAX_FILE_READ_SIZE ) )
	{
		return NULL;
	}

	// first time calling, allocate
	if ( !m_pszReturnReadFile )
		m_pszReturnReadFile = new char[SCRIPT_MAX_FILE_READ_SIZE];

	V_strncpy( const_cast<char*>(m_pszReturnReadFile), (const char*)buf.Base(), buf.Size() );
	buf.Purge();
	return m_pszReturnReadFile;
}

//-----------------------------------------------------------------------------
// Get the checksum of any file. Can be used to check the existence or validity of a file.
// Returns unsigned int as hex string.
//-----------------------------------------------------------------------------
/*
const char *CScriptReadWriteFile::CRC32_Checksum( const char *szFilename )
{
	CUtlBuffer buf( 0, 0, CUtlBuffer::READ_ONLY );
	if ( !g_pFullFileSystem->ReadFile( szFilename, NULL, buf ) )
		return NULL;

	// first time calling, allocate
	if ( !m_pszReturnCRC32 )
		m_pszReturnCRC32 = new char[9]; // 'FFFFFFFF\0'

	V_snprintf( const_cast<char*>(m_pszReturnCRC32), 9, "%X", CRC32_ProcessSingleBuffer( buf.Base(), buf.Size()-1 ) );
	buf.Purge();

	return m_pszReturnCRC32;
}
*/

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CScriptReadWriteFile::KeyValuesWrite( const char *szFile, HSCRIPT hInput )
{
	KeyValues *pKV = scriptmanager->GetKeyValuesFromScriptKV( g_pScriptVM, hInput );
	if (!pKV)
	{
		return false;
	}

	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	pKV->RecursiveSaveToFile( buf, 0 );

	if ( buf.Size() > SCRIPT_MAX_FILE_WRITE_SIZE )
	{
		DevWarning( 2, "Input is too large for a ScriptKeyValuesWrite ( %s / %d MB )\n", V_pretifymem(buf.Size(),2,true), (SCRIPT_MAX_FILE_WRITE_SIZE >> 20) );
		buf.Purge();
		return false;
	}

	char pszFullName[MAX_PATH];
	V_snprintf( pszFullName, sizeof(pszFullName), SCRIPT_RW_FULL_PATH_FMT, szFile );

	if ( !V_RemoveDotSlashes( pszFullName, CORRECT_PATH_SEPARATOR, true ) )
	{
		DevWarning( 2, "Invalid file location : %s\n", szFile );
		buf.Purge();
		return false;
	}

	int nSize = V_strlen(pszFullName) + 1;
	char *pszDir = (char*)stackalloc(nSize);
	V_memcpy( pszDir, pszFullName, nSize );
	V_StripFilename( pszDir );

	g_pFullFileSystem->CreateDirHierarchy( pszDir, SCRIPT_RW_PATH_ID );
	bool res = g_pFullFileSystem->WriteFile( pszFullName, SCRIPT_RW_PATH_ID, buf );
	buf.Purge();
	return res;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
HSCRIPT CScriptReadWriteFile::KeyValuesRead( const char *szFile )
{
	char pszFullName[MAX_PATH];
	V_snprintf( pszFullName, sizeof(pszFullName), SCRIPT_RW_FULL_PATH_FMT, szFile );

	if ( !V_RemoveDotSlashes( pszFullName, CORRECT_PATH_SEPARATOR, true ) )
	{
		DevWarning( 2, "Invalid file location : %s\n", szFile );
		return NULL;
	}

	unsigned int size = g_pFullFileSystem->Size( pszFullName, SCRIPT_RW_PATH_ID );
	if ( size >= SCRIPT_MAX_FILE_READ_SIZE )
	{
		DevWarning( 2, "File '%s' (from '%s') is too large for a ScriptKeyValuesRead ( %s / %u bytes )\n", pszFullName, szFile, V_pretifymem(size,2,true), SCRIPT_MAX_FILE_READ_SIZE );
		return NULL;
	}

	KeyValues *pKV = new KeyValues( szFile );
	if ( !pKV->LoadFromFile( g_pFullFileSystem, pszFullName, SCRIPT_RW_PATH_ID ) )
	{
		pKV->deleteThis();
		return NULL;
	}

	HSCRIPT hScript = scriptmanager->CreateScriptKeyValues( g_pScriptVM, pKV, true ); // bAllowDestruct is supposed to automatically remove the involved KV

	return hScript;
}
#undef SCRIPT_MAX_FILE_READ_SIZE
#undef SCRIPT_MAX_FILE_WRITE_SIZE
#undef SCRIPT_RW_PATH_ID
#undef SCRIPT_RW_FULL_PATH_FMT

//=============================================================================
// Network message helper
// (Unique to mapbase)
//
// Uses usermessages for server to client, UserCmd for client to server communication.
// The custom message name is hashed and sent as word with the message.
//=============================================================================


#ifdef GAME_DLL
#define m_MsgIn_() m_MsgIn->
#define DLL_LOC_STR "[Server]"
#else
#define m_MsgIn_() m_MsgIn.
#define DLL_LOC_STR "[Client]"
#endif


void CNetMsgScriptHelper::WriteToBuffer( bf_write *bf )
{
	bf->WriteBits( m_MsgOut.GetData(), m_MsgOut.GetNumBitsWritten() );
}

//-----------------------------------------------------------------------------
// Reset the current network message buffer
//-----------------------------------------------------------------------------
void CNetMsgScriptHelper::Reset()
{
	m_MsgOut.StartWriting( m_MsgData, sizeof(m_MsgData), 0 );
#ifdef GAME_DLL
	m_filter.Reset();
#else
	m_MsgIn_()Reset();
	m_bWriteReady = false;
#endif
}

//-----------------------------------------------------------------------------
// Create the storage for the receiver callback functions.
// Functions are handled in the VM, the storage table is here.
//-----------------------------------------------------------------------------
void CNetMsgScriptHelper::InitPostVM()
{
	ScriptVariant_t hHooks;
	g_pScriptVM->CreateTable( hHooks );
#if _DEBUG
	g_pScriptVM->SetValue( NULL, "__NetMsg_hooks", hHooks );
#endif
	m_Hooks = (HSCRIPT)hHooks;
}

void CNetMsgScriptHelper::LevelShutdownPreVM()
{
	Reset();
	if ( m_Hooks )
	{
		g_pScriptVM->ReleaseScript( m_Hooks );
	}
	m_Hooks = NULL;
}

#ifdef CLIENT_DLL

bool CNetMsgScriptHelper::Init() // IGameSystem
{
	usermessages->HookMessage( "ScriptMsg", __MsgFunc_ScriptMsg );
	return true;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CNetMsgScriptHelper::__MsgFunc_ScriptMsg( bf_read &msg )
{
	g_ScriptNetMsg->ReceiveMessage( msg );
}

#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#ifdef GAME_DLL
void CNetMsgScriptHelper::ReceiveMessage( bf_read *msg, CBaseEntity *pPlayer )
{
	m_MsgIn = msg;
#else
void CNetMsgScriptHelper::ReceiveMessage( bf_read &msg )
{
	m_MsgIn.StartReading( msg.m_pData, msg.m_nDataBytes );
#endif

	word hash = m_MsgIn_()ReadWord();

	// Don't do anything if there's no VM here. This can happen if a message from the server goes to a VM-less client, or vice versa.
	if ( !g_pScriptVM )
	{
		CGWarning( 0, CON_GROUP_VSCRIPT, DLL_LOC_STR " CNetMsgScriptHelper: No VM on receiving side\n" );
		return;
	}

	ScriptVariant_t hfn;
	if ( g_pScriptVM->GetValue( m_Hooks, hash, &hfn ) )
	{
#ifdef GAME_DLL
		if ( g_pScriptVM->Call( hfn, NULL, true, NULL, pPlayer->m_hScriptInstance ) == SCRIPT_ERROR )
#else
		if ( g_pScriptVM->ExecuteFunction( hfn, NULL, 0, NULL, NULL, true ) == SCRIPT_ERROR )
#endif
		{
			DevWarning( 2, DLL_LOC_STR " NetMsg: invalid callback [%d]\n", hash );
		}
		g_pScriptVM->ReleaseValue( hfn );
	}
	else
	{
		DevWarning( 2, DLL_LOC_STR " NetMsg hook not found [%d]\n", hash );
	}
}

//-----------------------------------------------------------------------------
// Start writing new custom network message
//-----------------------------------------------------------------------------
void CNetMsgScriptHelper::Start( const char *msg )
{
	Reset();
	m_MsgOut.WriteWord( HashStringCaseless(msg) );
}

//-----------------------------------------------------------------------------
// server -> client
//
// Sends an exclusive usermessage.
//-----------------------------------------------------------------------------
#ifdef GAME_DLL
void CNetMsgScriptHelper::Send( HSCRIPT player, bool bReliable )
{
	CBaseEntity *pPlayer = ToEnt(player);
	if ( pPlayer )
	{
		m_filter.AddRecipient( (CBasePlayer*)pPlayer );
	}

	if ( bReliable )
	{
		m_filter.MakeReliable();
	}

	Assert( usermessages->LookupUserMessage( "ScriptMsg" ) != -1 );

	DoSendUserMsg( &m_filter, usermessages->LookupUserMessage( "ScriptMsg" ) );
}
#else // CLIENT_DLL
//-----------------------------------------------------------------------------
// client -> server
//
// Mark UserCmd delta ready.
//-----------------------------------------------------------------------------
void CNetMsgScriptHelper::Send()
{
	m_bWriteReady = true;
}
#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CNetMsgScriptHelper::Receive( const char *msg, HSCRIPT func )
{
	if ( func )
		g_pScriptVM->SetValue( m_Hooks, int( HashStringCaseless(msg) ), func );
	else
		g_pScriptVM->ClearValue( m_Hooks, int( HashStringCaseless(msg) ) );
}

#ifdef GAME_DLL
void CNetMsgScriptHelper::DoSendUserMsg( CRecipientFilter *filter, int type )
{
	WriteToBuffer( engine->UserMessageBegin( filter, type ) );
	engine->MessageEnd();
}

void CNetMsgScriptHelper::DoSendEntityMsg( CBaseEntity *entity, bool reliable )
{
	WriteToBuffer( engine->EntityMessageBegin( entity->entindex(), entity->GetServerClass(), reliable ) );
	engine->MessageEnd();
}

//-----------------------------------------------------------------------------
// Send a usermessage from the server to the client
//-----------------------------------------------------------------------------
void CNetMsgScriptHelper::SendUserMessage( HSCRIPT hPlayer, const char *msg, bool bReliable )
{
	int msg_type = usermessages->LookupUserMessage(msg);
	if ( msg_type == -1 )
	{
		g_pScriptVM->RaiseException( UTIL_VarArgs("SendUserMessage: Unregistered message '%s'", msg) );
		return;
	}

	CBaseEntity *pPlayer = ToEnt(hPlayer);
	if ( pPlayer )
	{
		m_filter.AddRecipient( (CBasePlayer*)pPlayer );
	}

	if ( bReliable )
	{
		m_filter.MakeReliable();
	}

	DoSendUserMsg( &m_filter, msg_type );
}

//-----------------------------------------------------------------------------
// Send a message from a server side entity to its client side counterpart
//-----------------------------------------------------------------------------
void CNetMsgScriptHelper::SendEntityMessage( HSCRIPT hEnt, bool bReliable )
{
	CBaseEntity *entity = ToEnt(hEnt);
	if ( !entity )
	{
		g_pScriptVM->RaiseException("SendEntityMessage: invalid entity");
		return;
	}

	DoSendEntityMsg( entity, bReliable );
}
#else
//-----------------------------------------------------------------------------
// Dispatch a usermessage on client
//-----------------------------------------------------------------------------
void CNetMsgScriptHelper::DispatchUserMessage( const char *msg )
{
	bf_read buffer( m_MsgOut.GetData(), m_MsgOut.GetNumBytesWritten() );
	usermessages->DispatchUserMessage( usermessages->LookupUserMessage(msg), buffer );
}
#endif // GAME_DLL

#ifdef GAME_DLL
void CNetMsgScriptHelper::AddRecipient( HSCRIPT player )
{
	CBaseEntity *pPlayer = ToEnt(player);
	if ( pPlayer )
	{
		m_filter.AddRecipient( (CBasePlayer*)pPlayer );
	}
}

void CNetMsgScriptHelper::AddRecipientsByPVS( const Vector &pos )
{
	m_filter.AddRecipientsByPVS(pos);
}

void CNetMsgScriptHelper::AddRecipientsByPAS( const Vector &pos )
{
	m_filter.AddRecipientsByPAS(pos);
}

void CNetMsgScriptHelper::AddAllPlayers()
{
	m_filter.AddAllPlayers();
}
#endif // GAME_DLL

void CNetMsgScriptHelper::WriteInt( int iValue, int bits )
{
	m_MsgOut.WriteSBitLong( iValue, bits );
}

void CNetMsgScriptHelper::WriteUInt( int iValue, int bits )
{
	m_MsgOut.WriteUBitLong( iValue, bits );
}

void CNetMsgScriptHelper::WriteByte( int iValue )
{
	m_MsgOut.WriteByte( iValue );
}

void CNetMsgScriptHelper::WriteChar( int iValue )
{
	m_MsgOut.WriteChar( iValue );
}

void CNetMsgScriptHelper::WriteShort( int iValue )
{
	m_MsgOut.WriteShort( iValue );
}

void CNetMsgScriptHelper::WriteWord( int iValue )
{
	m_MsgOut.WriteWord( iValue );
}

void CNetMsgScriptHelper::WriteLong( int iValue )
{
	m_MsgOut.WriteLong( iValue );
}

void CNetMsgScriptHelper::WriteFloat( float flValue )
{
	m_MsgOut.WriteFloat( flValue );
}

void CNetMsgScriptHelper::WriteNormal( float flValue )
{
	m_MsgOut.WriteBitNormal( flValue );
}

void CNetMsgScriptHelper::WriteAngle( float flValue )
{
	m_MsgOut.WriteBitAngle( flValue, 8 );
}

void CNetMsgScriptHelper::WriteCoord( float flValue )
{
	m_MsgOut.WriteBitCoord( flValue );
}

void CNetMsgScriptHelper::WriteVec3Coord( const Vector& rgflValue )
{
	m_MsgOut.WriteBitVec3Coord( rgflValue );
}

void CNetMsgScriptHelper::WriteVec3Normal( const Vector& rgflValue )
{
	m_MsgOut.WriteBitVec3Normal( rgflValue );
}

void CNetMsgScriptHelper::WriteAngles( const QAngle& rgflValue )
{
	m_MsgOut.WriteBitAngles( rgflValue );
}

void CNetMsgScriptHelper::WriteString( const char *sz )
{
	m_MsgOut.WriteString( sz );
}

void CNetMsgScriptHelper::WriteBool( bool bValue )
{
	m_MsgOut.WriteOneBit( bValue ? 1 : 0 );
}

void CNetMsgScriptHelper::WriteEntity( HSCRIPT hEnt )
{
	CBaseEntity *p = ToEnt(hEnt);
	int i = p ? p->entindex() : -1;
	m_MsgOut.WriteSBitLong( i, MAX_EDICT_BITS );
}

void CNetMsgScriptHelper::WriteEHandle( HSCRIPT hEnt )
{
	CBaseEntity *pEnt = ToEnt( hEnt );
	long iEncodedEHandle;
	if ( pEnt )
	{
		EHANDLE hEnt = pEnt;
		int iSerialNum = hEnt.GetSerialNumber() & (1 << NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS) - 1;
		iEncodedEHandle = hEnt.GetEntryIndex() | (iSerialNum << MAX_EDICT_BITS);
	}
	else
	{
		iEncodedEHandle = INVALID_NETWORKED_EHANDLE_VALUE;
	}
	m_MsgOut.WriteLong( iEncodedEHandle );
}

int CNetMsgScriptHelper::ReadInt( int bits )
{
	return m_MsgIn_()ReadSBitLong(bits);
}

int CNetMsgScriptHelper::ReadUInt( int bits )
{
	return m_MsgIn_()ReadUBitLong(bits);
}

int CNetMsgScriptHelper::ReadByte()
{
	return m_MsgIn_()ReadByte();
}

int CNetMsgScriptHelper::ReadChar()
{
	return m_MsgIn_()ReadChar();
}

int CNetMsgScriptHelper::ReadShort()
{
	return m_MsgIn_()ReadShort();
}

int CNetMsgScriptHelper::ReadWord()
{
	return m_MsgIn_()ReadWord();
}

int CNetMsgScriptHelper::ReadLong()
{
	return m_MsgIn_()ReadLong();
}

float CNetMsgScriptHelper::ReadFloat()
{
	return m_MsgIn_()ReadFloat();
}

float CNetMsgScriptHelper::ReadNormal()
{
	return m_MsgIn_()ReadBitNormal();
}

float CNetMsgScriptHelper::ReadAngle()
{
	return m_MsgIn_()ReadBitAngle( 8 );
}

float CNetMsgScriptHelper::ReadCoord()
{
	return m_MsgIn_()ReadBitCoord();
}

const Vector& CNetMsgScriptHelper::ReadVec3Coord()
{
	static Vector vec3;
	//vec3.Init();
	m_MsgIn_()ReadBitVec3Coord(vec3);
	return vec3;
}

const Vector& CNetMsgScriptHelper::ReadVec3Normal()
{
	static Vector vec3;
	//vec3.Init();
	m_MsgIn_()ReadBitVec3Normal(vec3);
	return vec3;
}

const QAngle& CNetMsgScriptHelper::ReadAngles()
{
	static QAngle vec3;
	//vec3.Init();
	m_MsgIn_()ReadBitAngles(vec3);
	return vec3;
}

const char* CNetMsgScriptHelper::ReadString()
{
	static char buf[512];
	m_MsgIn_()ReadString( buf, sizeof(buf) );
	return buf;
}

bool CNetMsgScriptHelper::ReadBool()
{
	return m_MsgIn_()ReadOneBit();
}

HSCRIPT CNetMsgScriptHelper::ReadEntity()
{
	int index = m_MsgIn_()ReadSBitLong( MAX_EDICT_BITS );
#ifdef GAME_DLL
	edict_t *e = INDEXENT(index);
	if ( e && !e->IsFree() )
	{
		return ToHScript( GetContainingEntity(e) );
	}
#else // CLIENT_DLL
	if ( index < NUM_ENT_ENTRIES )
	{
		return ToHScript( CBaseEntity::Instance(index) );
	}
#endif
	return NULL;
}

HSCRIPT CNetMsgScriptHelper::ReadEHandle()
{
	int iEncodedEHandle = m_MsgIn_()ReadLong();
	if ( iEncodedEHandle == INVALID_NETWORKED_EHANDLE_VALUE )
		return NULL;
	int iEntry = iEncodedEHandle & ( (1 << MAX_EDICT_BITS) - 1 );
	int iSerialNum = iEncodedEHandle >> MAX_EDICT_BITS;
	return ToHScript( EHANDLE( iEntry, iSerialNum ) );
}

int CNetMsgScriptHelper::GetNumBitsWritten()
{
	return m_MsgOut.GetNumBitsWritten();
}

#undef m_MsgIn_

BEGIN_SCRIPTDESC_ROOT_NAMED( CNetMsgScriptHelper, "CNetMsg", SCRIPT_SINGLETON "Network messages" )

#ifdef GAME_DLL
	DEFINE_SCRIPTFUNC( SendUserMessage, "Send a usermessage from the server to the client" )
	DEFINE_SCRIPTFUNC( SendEntityMessage, "Send a message from a server side entity to its client side counterpart" )
	DEFINE_SCRIPTFUNC( AddRecipient, "" )
	//DEFINE_SCRIPTFUNC( RemoveRecipient, "" )
	DEFINE_SCRIPTFUNC( AddRecipientsByPVS, "" )
	DEFINE_SCRIPTFUNC( AddRecipientsByPAS, "" )
	DEFINE_SCRIPTFUNC( AddAllPlayers, "" )
#else
	DEFINE_SCRIPTFUNC( DispatchUserMessage, "Dispatch a usermessage on client" )
#endif

	DEFINE_SCRIPTFUNC( Reset, "Reset the current network message buffer" )
	DEFINE_SCRIPTFUNC( Start, "Start writing new custom network message" )
	DEFINE_SCRIPTFUNC( Receive, "Set custom network message callback" )
	DEFINE_SCRIPTFUNC_NAMED( Receive, "Recieve", SCRIPT_HIDE ) // This was a typo until v6.3
#ifdef GAME_DLL
	DEFINE_SCRIPTFUNC( Send, "Send a custom network message from the server to the client (max 252 bytes)" )
#else
	DEFINE_SCRIPTFUNC( Send, "Send a custom network message from the client to the server (max 2045 bytes)" )
#endif

	DEFINE_SCRIPTFUNC( WriteInt, "variable bit signed int" )
	DEFINE_SCRIPTFUNC( WriteUInt, "variable bit unsigned int" )
	DEFINE_SCRIPTFUNC( WriteByte, "8 bit unsigned char" )
	DEFINE_SCRIPTFUNC( WriteChar, "8 bit char" )
	DEFINE_SCRIPTFUNC( WriteShort, "16 bit short" )
	DEFINE_SCRIPTFUNC( WriteWord, "16 bit unsigned short" )
	DEFINE_SCRIPTFUNC( WriteLong, "32 bit long" )
	DEFINE_SCRIPTFUNC( WriteFloat, "32 bit float" )
	DEFINE_SCRIPTFUNC( WriteNormal, "12 bit" )
	DEFINE_SCRIPTFUNC( WriteAngle, "8 bit unsigned char" )
	DEFINE_SCRIPTFUNC( WriteCoord, "" )
	DEFINE_SCRIPTFUNC( WriteVec3Coord, "" )
	DEFINE_SCRIPTFUNC( WriteVec3Normal, "27 bit" )
	DEFINE_SCRIPTFUNC( WriteAngles, "" )
	DEFINE_SCRIPTFUNC( WriteString, "" )
	DEFINE_SCRIPTFUNC( WriteBool, "1 bit" )
	DEFINE_SCRIPTFUNC( WriteEntity, "11 bit (entindex)" )
	DEFINE_SCRIPTFUNC( WriteEHandle, "32 bit long" )

	DEFINE_SCRIPTFUNC( ReadInt, "" )
	DEFINE_SCRIPTFUNC( ReadUInt, "" )
	DEFINE_SCRIPTFUNC( ReadByte, "" )
	DEFINE_SCRIPTFUNC( ReadChar, "" )
	DEFINE_SCRIPTFUNC( ReadShort, "" )
	DEFINE_SCRIPTFUNC( ReadWord, "" )
	DEFINE_SCRIPTFUNC( ReadLong, "" )
	DEFINE_SCRIPTFUNC( ReadFloat, "" )
	DEFINE_SCRIPTFUNC( ReadNormal, "" )
	DEFINE_SCRIPTFUNC( ReadAngle, "" )
	DEFINE_SCRIPTFUNC( ReadCoord, "" )
	DEFINE_SCRIPTFUNC( ReadVec3Coord, "" )
	DEFINE_SCRIPTFUNC( ReadVec3Normal, "" )
	DEFINE_SCRIPTFUNC( ReadAngles, "" )
	DEFINE_SCRIPTFUNC( ReadString, "max 512 bytes at once" )
	DEFINE_SCRIPTFUNC( ReadBool, "" )
	DEFINE_SCRIPTFUNC( ReadEntity, "" )
	DEFINE_SCRIPTFUNC( ReadEHandle, "" )

	DEFINE_SCRIPTFUNC( GetNumBitsWritten, "" )

END_SCRIPTDESC();



#define RETURN_IF_CANNOT_DRAW_OVERLAY\
	if (engine->IsPaused())\
		return;
class CDebugOverlayScriptHelper
{
public:

	void Box( const Vector &origin, const Vector &mins, const Vector &maxs, int r, int g, int b, int a, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		debugoverlay->AddBoxOverlay(origin, mins, maxs, vec3_angle, r, g, b, a, flDuration);
	}
	void BoxDirection( const Vector &origin, const Vector &mins, const Vector &maxs, const Vector &forward, int r, int g, int b, int a, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		QAngle f_angles = vec3_angle;
		f_angles.y = UTIL_VecToYaw(forward);

		debugoverlay->AddBoxOverlay(origin, mins, maxs, f_angles, r, g, b, a, flDuration);
	}
	void BoxAngles( const Vector &origin, const Vector &mins, const Vector &maxs, const QAngle &angles, int r, int g, int b, int a, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		debugoverlay->AddBoxOverlay(origin, mins, maxs, angles, r, g, b, a, flDuration);
	}
	void SweptBox( const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, const QAngle & angles, int r, int g, int b, int a, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		debugoverlay->AddSweptBoxOverlay(start, end, mins, maxs, angles, r, g, b, a, flDuration);
	}
	void EntityBounds( HSCRIPT pEntity, int r, int g, int b, int a, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		CBaseEntity *pEnt = ToEnt(pEntity);
		if (!pEnt)
			return;

		const CCollisionProperty *pCollide = pEnt->CollisionProp();
		debugoverlay->AddBoxOverlay(pCollide->GetCollisionOrigin(), pCollide->OBBMins(), pCollide->OBBMaxs(), pCollide->GetCollisionAngles(), r, g, b, a, flDuration);
	}
	void Line( const Vector &origin, const Vector &target, int r, int g, int b, bool noDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		debugoverlay->AddLineOverlay(origin, target, r, g, b, noDepthTest, flDuration);
	}
	void Triangle( const Vector &p1, const Vector &p2, const Vector &p3, int r, int g, int b, int a, bool noDepthTest, float duration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		debugoverlay->AddTriangleOverlay(p1, p2, p3, r, g, b, a, noDepthTest, duration);
	}
	void EntityText( int entityID, int text_offset, const char *text, float flDuration, int r, int g, int b, int a )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		debugoverlay->AddEntityTextOverlay(entityID, text_offset, flDuration,
				(int)clamp(r * 255.f, 0.f, 255.f), (int)clamp(g * 255.f, 0.f, 255.f), (int)clamp(b * 255.f, 0.f, 255.f),
				(int)clamp(a * 255.f, 0.f, 255.f), text);
	}
	void EntityTextAtPosition( const Vector &origin, int text_offset, const char *text, float flDuration, int r, int g, int b, int a )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		debugoverlay->AddTextOverlayRGB(origin, text_offset, flDuration, r, g, b, a, "%s", text);
	}
	void Grid( const Vector &vPosition )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		debugoverlay->AddGridOverlay(vPosition);
	}
	void Text( const Vector &origin, const char *text, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		debugoverlay->AddTextOverlay(origin, flDuration, "%s", text);
	}
	void ScreenText( float fXpos, float fYpos, const char *text, int r, int g, int b, int a, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		debugoverlay->AddScreenTextOverlay(fXpos, fYpos, flDuration, r, g, b, a, text);
	}
	void Cross3D( const Vector &position, float size, int r, int g, int b, bool noDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Line( position + Vector(size,0,0), position - Vector(size,0,0), r, g, b, noDepthTest, flDuration );
		Line( position + Vector(0,size,0), position - Vector(0,size,0), r, g, b, noDepthTest, flDuration );
		Line( position + Vector(0,0,size), position - Vector(0,0,size), r, g, b, noDepthTest, flDuration );
	}
	void Cross3DOriented( const Vector &position, const QAngle &angles, float size, int r, int g, int b, bool noDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Vector forward, right, up;
		AngleVectors( angles, &forward, &right, &up );

		forward *= size;
		right *= size;
		up *= size;

		Line( position + right, position - right, r, g, b, noDepthTest, flDuration );
		Line( position + forward, position - forward, r, g, b, noDepthTest, flDuration );
		Line( position + up, position - up, r, g, b, noDepthTest, flDuration );
	}
	void DrawTickMarkedLine( const Vector &startPos, const Vector &endPos, float tickDist, int tickTextDist, int r, int g, int b, bool noDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Vector	lineDir = (endPos - startPos);
		float	lineDist = VectorNormalize(lineDir);
		int		numTicks = lineDist / tickDist;

		Vector  upVec = Vector(0,0,4);
		Vector	sideDir;
		Vector	tickPos = startPos;
		int		tickTextCnt = 0;

		CrossProduct(lineDir, upVec, sideDir);

		Line(startPos, endPos, r, g, b, noDepthTest, flDuration);

		for (int i = 0; i<numTicks + 1; i++)
		{
			Vector tickLeft = tickPos - sideDir;
			Vector tickRight = tickPos + sideDir;

			if (tickTextCnt == tickTextDist)
			{
				char text[25];
				Q_snprintf(text, sizeof(text), "%i", i);
				Vector textPos = tickLeft + Vector(0, 0, 8);
				Line(tickLeft, tickRight, 255, 255, 255, noDepthTest, flDuration);
				Text(textPos, text, flDuration);
				tickTextCnt = 0;
			}
			else
			{
				Line(tickLeft, tickRight, r, g, b, noDepthTest, flDuration);
			}

			tickTextCnt++;

			tickPos = tickPos + (tickDist * lineDir);
		}
	}
	void HorzArrow( const Vector &startPos, const Vector &endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Vector	lineDir		= (endPos - startPos);
		VectorNormalize( lineDir );
		Vector  upVec		= Vector( 0, 0, 1 );
		Vector	sideDir;
		float   radius		= width / 2.0;

		CrossProduct(lineDir, upVec, sideDir);

		Vector p1 =	startPos - sideDir * radius;
		Vector p2 = endPos - lineDir * width - sideDir * radius;
		Vector p3 = endPos - lineDir * width - sideDir * width;
		Vector p4 = endPos;
		Vector p5 = endPos - lineDir * width + sideDir * width;
		Vector p6 = endPos - lineDir * width + sideDir * radius;
		Vector p7 =	startPos + sideDir * radius;

		Line(p1, p2, r,g,b,noDepthTest,flDuration);
		Line(p2, p3, r,g,b,noDepthTest,flDuration);
		Line(p3, p4, r,g,b,noDepthTest,flDuration);
		Line(p4, p5, r,g,b,noDepthTest,flDuration);
		Line(p5, p6, r,g,b,noDepthTest,flDuration);
		Line(p6, p7, r,g,b,noDepthTest,flDuration);

		if ( a > 0 )
		{
			Triangle( p5, p4, p3, r, g, b, a, noDepthTest, flDuration );
			Triangle( p1, p7, p6, r, g, b, a, noDepthTest, flDuration );
			Triangle( p6, p2, p1, r, g, b, a, noDepthTest, flDuration );

			Triangle( p3, p4, p5, r, g, b, a, noDepthTest, flDuration );
			Triangle( p6, p7, p1, r, g, b, a, noDepthTest, flDuration );
			Triangle( p1, p2, p6, r, g, b, a, noDepthTest, flDuration );
		}
	}
	void YawArrow( const Vector &startPos, float yaw, float length, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Vector forward = UTIL_YawToVector( yaw );
		HorzArrow( startPos, startPos + forward * length, width, r, g, b, a, noDepthTest, flDuration );
	}
	void VertArrow( const Vector &startPos, const Vector &endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Vector	lineDir		= (endPos - startPos);
		VectorNormalize( lineDir );
		Vector  upVec;
		Vector	sideDir;
		float   radius		= width / 2.0;

		VectorVectors( lineDir, sideDir, upVec );

		Vector p1 =	startPos - upVec * radius;
		Vector p2 = endPos - lineDir * width - upVec * radius;
		Vector p3 = endPos - lineDir * width - upVec * width;
		Vector p4 = endPos;
		Vector p5 = endPos - lineDir * width + upVec * width;
		Vector p6 = endPos - lineDir * width + upVec * radius;
		Vector p7 =	startPos + upVec * radius;

		Line(p1, p2, r,g,b,noDepthTest,flDuration);
		Line(p2, p3, r,g,b,noDepthTest,flDuration);
		Line(p3, p4, r,g,b,noDepthTest,flDuration);
		Line(p4, p5, r,g,b,noDepthTest,flDuration);
		Line(p5, p6, r,g,b,noDepthTest,flDuration);
		Line(p6, p7, r,g,b,noDepthTest,flDuration);

		if ( a > 0 )
		{
			Triangle( p5, p4, p3, r, g, b, a, noDepthTest, flDuration );
			Triangle( p1, p7, p6, r, g, b, a, noDepthTest, flDuration );
			Triangle( p6, p2, p1, r, g, b, a, noDepthTest, flDuration );

			Triangle( p3, p4, p5, r, g, b, a, noDepthTest, flDuration );
			Triangle( p6, p7, p1, r, g, b, a, noDepthTest, flDuration );
			Triangle( p1, p2, p6, r, g, b, a, noDepthTest, flDuration );
		}
	}
	void Axis( const Vector &position, const QAngle &angles, float size, bool noDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Vector xvec, yvec, zvec;
		AngleVectors( angles, &xvec, &yvec, &zvec );

		xvec = position + (size * xvec);
		yvec = position - (size * yvec);
		zvec = position + (size * zvec);

		Line( position, xvec, 255, 0, 0, noDepthTest, flDuration );
		Line( position, yvec, 0, 255, 0, noDepthTest, flDuration );
		Line( position, zvec, 0, 0, 255, noDepthTest, flDuration );
	}
	void Sphere( const Vector &center, float radius, int r, int g, int b, bool noDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		Vector edge, lastEdge;

		float axisSize = radius;
		Line( center + Vector( 0, 0, -axisSize ), center + Vector( 0, 0, axisSize ), r, g, b, noDepthTest, flDuration );
		Line( center + Vector( 0, -axisSize, 0 ), center + Vector( 0, axisSize, 0 ), r, g, b, noDepthTest, flDuration );
		Line( center + Vector( -axisSize, 0, 0 ), center + Vector( axisSize, 0, 0 ), r, g, b, noDepthTest, flDuration );

		lastEdge = Vector( radius + center.x, center.y, center.z );
		float angle;
		for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
		{
			edge.x = radius * cosf( angle / 180.0f * M_PI ) + center.x;
			edge.y = center.y;
			edge.z = radius * sinf( angle / 180.0f * M_PI ) + center.z;

			Line( edge, lastEdge, r, g, b, noDepthTest, flDuration );

			lastEdge = edge;
		}

		lastEdge = Vector( center.x, radius + center.y, center.z );
		for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
		{
			edge.x = center.x;
			edge.y = radius * cosf( angle / 180.0f * M_PI ) + center.y;
			edge.z = radius * sinf( angle / 180.0f * M_PI ) + center.z;

			Line( edge, lastEdge, r, g, b, noDepthTest, flDuration );

			lastEdge = edge;
		}

		lastEdge = Vector( center.x, radius + center.y, center.z );
		for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
		{
			edge.x = radius * cosf( angle / 180.0f * M_PI ) + center.x;
			edge.y = radius * sinf( angle / 180.0f * M_PI ) + center.y;
			edge.z = center.z;

			Line( edge, lastEdge, r, g, b, noDepthTest, flDuration );

			lastEdge = edge;
		}
	}
	void CircleOriented( const Vector &position, const QAngle &angles, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		matrix3x4_t xform;
		AngleMatrix(angles, position, xform);
		Vector xAxis, yAxis;
		MatrixGetColumn(xform, 2, xAxis);
		MatrixGetColumn(xform, 1, yAxis);
		Circle(position, xAxis, yAxis, radius, r, g, b, a, bNoDepthTest, flDuration);
	}
	void Circle( const Vector &position, const Vector &xAxis, const Vector &yAxis, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration )
	{
		RETURN_IF_CANNOT_DRAW_OVERLAY

		const unsigned int nSegments = 16;
		const float flRadStep = (M_PI*2.0f) / (float) nSegments;

		Vector vecLastPosition;
		Vector vecStart = position + xAxis * radius;
		Vector vecPosition = vecStart;

		for ( int i = 1; i <= nSegments; i++ )
		{
			vecLastPosition = vecPosition;

			float flSin, flCos;
			SinCos( flRadStep*i, &flSin, &flCos );
			vecPosition = position + (xAxis * flCos * radius) + (yAxis * flSin * radius);

			Line( vecLastPosition, vecPosition, r, g, b, bNoDepthTest, flDuration );

			if ( a && i > 1 )
			{		
				debugoverlay->AddTriangleOverlay( vecStart, vecLastPosition, vecPosition, r, g, b, a, bNoDepthTest, flDuration );
			}
		}
	}
#ifndef CLIENT_DLL
	void SetDebugBits( HSCRIPT hEntity, int bit ) // DebugOverlayBits_t
	{
		CBaseEntity *pEnt = ToEnt(hEntity);
		if (!pEnt)
			return;

		if (pEnt->m_debugOverlays & bit)
		{
			pEnt->m_debugOverlays &= ~bit;
		}
		else
		{
			pEnt->m_debugOverlays |= bit;

#ifdef AI_MONITOR_FOR_OSCILLATION
			if (pEnt->IsNPC())
			{
				pEnt->MyNPCPointer()->m_ScheduleHistory.RemoveAll();
			}
#endif//AI_MONITOR_FOR_OSCILLATION
		}
	}
#endif
	void ClearAllOverlays()
	{
#ifndef CLIENT_DLL
		// Clear all entities of their debug overlays
		for (CBaseEntity *pEntity = gEntList.FirstEnt(); pEntity; pEntity = gEntList.NextEnt(pEntity))
		{
			pEntity->m_debugOverlays = 0;
		}
#endif

		debugoverlay->ClearAllOverlays();
	}

private:
} g_ScriptDebugOverlay;

BEGIN_SCRIPTDESC_ROOT( CDebugOverlayScriptHelper, SCRIPT_SINGLETON "CDebugOverlayScriptHelper" )
	DEFINE_SCRIPTFUNC( Box, "Draws a world-space axis-aligned box. Specify bounds in world space." )
	DEFINE_SCRIPTFUNC( BoxDirection, "Draw box oriented to a Vector direction" )
	DEFINE_SCRIPTFUNC( BoxAngles, "Draws an oriented box at the origin. Specify bounds in local space." )
	DEFINE_SCRIPTFUNC( SweptBox, "Draws a swept box. Specify endpoints in world space and the bounds in local space." )
	DEFINE_SCRIPTFUNC( EntityBounds, "Draws bounds of an entity" )
	DEFINE_SCRIPTFUNC( Line, "Draws a line between two points" )
	DEFINE_SCRIPTFUNC( Triangle, "Draws a filled triangle. Specify vertices in world space." )
	DEFINE_SCRIPTFUNC( EntityText, "Draws text on an entity" )
	DEFINE_SCRIPTFUNC( EntityTextAtPosition, "Draw entity text overlay at a specific position" )
	DEFINE_SCRIPTFUNC( Grid, "Add grid overlay" )
	DEFINE_SCRIPTFUNC( Text, "Draws 2D text. Specify origin in world space." )
	DEFINE_SCRIPTFUNC( ScreenText, "Draws 2D text. Specify coordinates in screen space." )
	DEFINE_SCRIPTFUNC( Cross3D, "Draws a world-aligned cross. Specify origin in world space." )
	DEFINE_SCRIPTFUNC( Cross3DOriented, "Draws an oriented cross. Specify origin in world space." )
	DEFINE_SCRIPTFUNC( DrawTickMarkedLine, "Draws a dashed line. Specify endpoints in world space." )
	DEFINE_SCRIPTFUNC( HorzArrow, "Draws a horizontal arrow. Specify endpoints in world space." )
	DEFINE_SCRIPTFUNC( YawArrow, "Draws a arrow associated with a specific yaw. Specify endpoints in world space." )
	DEFINE_SCRIPTFUNC( VertArrow, "Draws a vertical arrow. Specify endpoints in world space." )
	DEFINE_SCRIPTFUNC( Axis, "Draws an axis. Specify origin + orientation in world space." )
	DEFINE_SCRIPTFUNC( Sphere, "Draws a wireframe sphere. Specify center in world space." )
	DEFINE_SCRIPTFUNC( CircleOriented, "Draws a circle oriented. Specify center in world space." )
	DEFINE_SCRIPTFUNC( Circle, "Draws a circle. Specify center in world space." )
#ifndef CLIENT_DLL
	DEFINE_SCRIPTFUNC( SetDebugBits, "Set debug bits on entity" )
#endif
	DEFINE_SCRIPTFUNC( ClearAllOverlays, "Clear all debug overlays at once" )
END_SCRIPTDESC();



//=============================================================================
// ConVars
//=============================================================================
class CScriptConCommand : public ICommandCallback, public ICommandCompletionCallback
{
public:
	~CScriptConCommand()
	{
		Unregister();
		delete m_cmd;
	}

	CScriptConCommand( const char *name, HSCRIPT fn, const char *helpString, int flags )
	{
		m_cmd = new ConCommand( name, this, helpString, flags, 0 );
		m_hCallback = fn;
		m_hCompletionCallback = NULL;
		m_nCmdNameLen = V_strlen(name) + 1;

		Assert( m_nCmdNameLen - 1 <= 128 );
	}

	void CommandCallback( const CCommand &command )
	{
		int count = command.ArgC();
		ScriptVariant_t *vArgv = (ScriptVariant_t*)stackalloc( sizeof(ScriptVariant_t) * count );
		for ( int i = 0; i < count; ++i )
		{
			vArgv[i] = command[i];
		}
		if ( g_pScriptVM->ExecuteFunction( m_hCallback, vArgv, count, NULL, NULL, true ) == SCRIPT_ERROR )
		{
			DevWarning( 1, "CScriptConCommand: invalid callback for '%s'\n", command[0] );
		}
	}

	int CommandCompletionCallback( const char *partial, CUtlVector< CUtlString > &commands )
	{
		Assert( g_pScriptVM );
		Assert( m_hCompletionCallback );

		ScriptVariant_t hArray;
		g_pScriptVM->CreateArray( hArray );

		// split command name from partial, pass both separately to the script function
		char *cmdname = (char*)stackalloc( m_nCmdNameLen );
		V_memcpy( cmdname, partial, m_nCmdNameLen - 1 );
		cmdname[ m_nCmdNameLen - 1 ] = 0;

		char argPartial[256];
		V_StrRight( partial, V_strlen(partial) - m_nCmdNameLen, argPartial, sizeof(argPartial) );

		ScriptVariant_t args[3] = { cmdname, argPartial, hArray };
		if ( g_pScriptVM->ExecuteFunction( m_hCompletionCallback, args, 3, NULL, NULL, true ) == SCRIPT_ERROR )
		{
			DevWarning( 1, "CScriptConCommand: invalid command completion callback for '%s'\n", cmdname );
			g_pScriptVM->ReleaseScript( hArray );
			return 0;
		}

		int count = 0;
		ScriptVariant_t val;
		int it = -1;
		while ( ( it = g_pScriptVM->GetKeyValue( hArray, it, NULL, &val ) ) != -1 )
		{
			if ( val.m_type == FIELD_CSTRING )
			{
				CUtlString s = val.m_pszString;
				//s.SetLength( COMMAND_COMPLETION_ITEM_LENGTH - 1 );
				commands.AddToTail( s );
				++count;
			}
			g_pScriptVM->ReleaseValue(val);

			if ( count == COMMAND_COMPLETION_MAXITEMS )
				break;
		}
		g_pScriptVM->ReleaseScript( hArray );
		return count;
	}

	void SetCompletionCallback( HSCRIPT fn )
	{
		if ( m_hCompletionCallback )
			g_pScriptVM->ReleaseScript( m_hCompletionCallback );

		if (fn)
		{
			if ( !m_cmd->IsRegistered() )
				return;

			m_cmd->m_pCommandCompletionCallback = this;
			m_cmd->m_bHasCompletionCallback = true;
			m_hCompletionCallback = fn;
		}
		else
		{
			m_cmd->m_pCommandCompletionCallback = NULL;
			m_cmd->m_bHasCompletionCallback = false;
			m_hCompletionCallback = NULL;
		}
	}

	void SetCallback( HSCRIPT fn )
	{
		if (fn)
		{
			if ( !m_cmd->IsRegistered() )
				Register();

			if ( m_hCallback )
				g_pScriptVM->ReleaseScript( m_hCallback );
			m_hCallback = fn;
		}
		else
		{
			Unregister();
		}
	}

	inline void Unregister()
	{
		if ( g_pCVar && m_cmd->IsRegistered() )
			g_pCVar->UnregisterConCommand( m_cmd );

		if ( g_pScriptVM )
		{
			if ( m_hCallback )
			{
				g_pScriptVM->ReleaseScript( m_hCallback );
				m_hCallback = NULL;
			}

			SetCompletionCallback( NULL );
		}
	}

	inline void Register()
	{
		if ( g_pCVar )
			g_pCVar->RegisterConCommand( m_cmd );
	}

	HSCRIPT m_hCallback;
	HSCRIPT m_hCompletionCallback;
	int m_nCmdNameLen;
	ConCommand *m_cmd;
};

class CScriptConVar
{
public:
	~CScriptConVar()
	{
		Unregister();
		delete m_cvar;
	}

	CScriptConVar( const char *pName, const char *pDefaultValue, const char *pHelpString, int flags/*, float fMin, float fMax*/ )
	{
		m_cvar = new ConVar( pName, pDefaultValue, flags, pHelpString );
	}

	inline void Unregister()
	{
		if ( g_pCVar && m_cvar->IsRegistered() )
			g_pCVar->UnregisterConCommand( m_cvar );
	}

	ConVar *m_cvar;
};

class CScriptConvarAccessor : public CAutoGameSystem
{
public:
	static CUtlMap< unsigned int, bool > g_ConVarsBlocked;
	static CUtlMap< unsigned int, bool > g_ConCommandsOverridable;
	static CUtlMap< unsigned int, CScriptConCommand* > g_ScriptConCommands;
	static CUtlMap< unsigned int, CScriptConVar* > g_ScriptConVars;
	static inline unsigned int Hash( const char*sz ){ return HashStringCaseless(sz); }

public:
	inline void AddOverridable( const char *name )
	{
		g_ConCommandsOverridable.InsertOrReplace( Hash(name), true );
	}

	inline bool IsOverridable( unsigned int hash )
	{
		int idx = g_ConCommandsOverridable.Find( hash );
		if ( idx == g_ConCommandsOverridable.InvalidIndex() )
			return false;
		return g_ConCommandsOverridable[idx];
	}

	inline void AddBlockedConVar( const char *name )
	{
		g_ConVarsBlocked.InsertOrReplace( Hash(name), true );
	}

	inline bool IsBlockedConvar( const char *name )
	{
		int idx = g_ConVarsBlocked.Find( Hash(name) );
		if ( idx == g_ConVarsBlocked.InvalidIndex() )
			return false;
		return g_ConVarsBlocked[idx];
	}

public:
	void RegisterCommand( const char *name, HSCRIPT fn, const char *helpString, int flags );
	void SetCompletionCallback( const char *name, HSCRIPT fn );
	void UnregisterCommand( const char *name );
	void RegisterConvar( const char *name, const char *pDefaultValue, const char *helpString, int flags );

	HSCRIPT GetCommandClient()
	{
#ifdef GAME_DLL
		return ToHScript( UTIL_GetCommandClient() );
#else
		return ToHScript( C_BasePlayer::GetLocalPlayer() );
#endif
	}
#ifdef GAME_DLL
	const char *GetClientConvarValue( int index, const char* cvar )
	{
		return engine->GetClientConVarValue( index, cvar );
	}
#endif
public:
	bool Init();

	void LevelShutdownPostEntity()
	{
		g_ScriptConCommands.PurgeAndDeleteElements();
		g_ScriptConVars.PurgeAndDeleteElements();
	}

public:
	float GetFloat( const char *pszConVar )
	{
		ConVarRef cvar( pszConVar );
		if ( cvar.IsFlagSet( FCVAR_SERVER_CANNOT_QUERY ) )
			return NULL;
		return cvar.GetFloat();
	}

	int GetInt( const char *pszConVar )
	{
		ConVarRef cvar( pszConVar );
		if ( cvar.IsFlagSet( FCVAR_SERVER_CANNOT_QUERY ) )
			return NULL;
		return cvar.GetInt();
	}

	bool GetBool( const char *pszConVar )
	{
		ConVarRef cvar( pszConVar );
		if ( cvar.IsFlagSet( FCVAR_SERVER_CANNOT_QUERY ) )
			return NULL;
		return cvar.GetBool();
	}

	const char *GetStr( const char *pszConVar )
	{
		ConVarRef cvar( pszConVar );
		if ( cvar.IsFlagSet( FCVAR_SERVER_CANNOT_QUERY ) )
			return NULL;
		return cvar.GetString();
	}

	const char *GetDefaultValue( const char *pszConVar )
	{
		ConVarRef cvar( pszConVar );
		return cvar.GetDefault();
	}

	bool IsFlagSet( const char *pszConVar, int nFlags )
	{
		ConVarRef cvar( pszConVar );
		return cvar.IsFlagSet( nFlags );
	}

	void SetFloat( const char *pszConVar, float value )
	{
		SetValue( pszConVar, value );
	}

	void SetInt( const char *pszConVar, int value )
	{
		SetValue( pszConVar, value );
	}

	void SetBool( const char *pszConVar, bool value )
	{
		SetValue( pszConVar, value );
	}

	void SetStr( const char *pszConVar, const char *value )
	{
		SetValue( pszConVar, value );
	}

	template <typename T>
	void SetValue( const char *pszConVar, T value )
	{
		ConVarRef cvar( pszConVar );
		if ( !cvar.IsValid() )
			return;

		if ( cvar.IsFlagSet( FCVAR_NOT_CONNECTED | FCVAR_SERVER_CANNOT_QUERY ) )
			return;

		if ( IsBlockedConvar( pszConVar ) )
			return;

		cvar.SetValue( value );
	}

} g_ScriptConvarAccessor;


CUtlMap< unsigned int, bool > CScriptConvarAccessor::g_ConVarsBlocked( DefLessFunc(unsigned int) );
CUtlMap< unsigned int, bool > CScriptConvarAccessor::g_ConCommandsOverridable( DefLessFunc(unsigned int) );
CUtlMap< unsigned int, CScriptConCommand* > CScriptConvarAccessor::g_ScriptConCommands( DefLessFunc(unsigned int) );
CUtlMap< unsigned int, CScriptConVar* > CScriptConvarAccessor::g_ScriptConVars( DefLessFunc(unsigned int) );

void CScriptConvarAccessor::RegisterCommand( const char *name, HSCRIPT fn, const char *helpString, int flags )
{
	unsigned int hash = Hash(name);
	int idx = g_ScriptConCommands.Find(hash);
	if ( idx == g_ScriptConCommands.InvalidIndex() )
	{
		if ( g_pCVar->FindVar(name) || ( g_pCVar->FindCommand(name) && !IsOverridable(hash) ) )
		{
			DevWarning( 1, "CScriptConvarAccessor::RegisterCommand unable to register blocked ConCommand: %s\n", name );
			return;
		}

		if ( !fn )
			return;

		CScriptConCommand *p = new CScriptConCommand( name, fn, helpString, flags );
		g_ScriptConCommands.Insert( hash, p );
	}
	else
	{
		CScriptConCommand *pCmd = g_ScriptConCommands[idx];
		pCmd->SetCallback( fn );
		pCmd->m_cmd->AddFlags( flags );
		//CGMsg( 1, CON_GROUP_VSCRIPT, "CScriptConvarAccessor::RegisterCommand replacing command already registered: %s\n", name );
	}
}

void CScriptConvarAccessor::SetCompletionCallback( const char *name, HSCRIPT fn )
{
	unsigned int hash = Hash(name);
	int idx = g_ScriptConCommands.Find(hash);
	if ( idx != g_ScriptConCommands.InvalidIndex() )
	{
		g_ScriptConCommands[idx]->SetCompletionCallback( fn );
	}
}

void CScriptConvarAccessor::UnregisterCommand( const char *name )
{
	unsigned int hash = Hash(name);
	int idx = g_ScriptConCommands.Find(hash);
	if ( idx != g_ScriptConCommands.InvalidIndex() )
	{
		g_ScriptConCommands[idx]->Unregister();
	}
}

void CScriptConvarAccessor::RegisterConvar( const char *name, const char *pDefaultValue, const char *helpString, int flags )
{
	Assert( g_pCVar );
	unsigned int hash = Hash(name);
	int idx = g_ScriptConVars.Find(hash);
	if ( idx == g_ScriptConVars.InvalidIndex() )
	{
		if ( g_pCVar->FindVar(name) || g_pCVar->FindCommand(name) )
		{
			DevWarning( 1, "CScriptConvarAccessor::RegisterConvar unable to register blocked ConCommand: %s\n", name );
			return;
		}

		CScriptConVar *p = new CScriptConVar( name, pDefaultValue, helpString, flags );
		g_ScriptConVars.Insert( hash, p );
	}
	else
	{
		g_ScriptConVars[idx]->m_cvar->AddFlags( flags );
		//CGMsg( 1, CON_GROUP_VSCRIPT, "CScriptConvarAccessor::RegisterConvar convar %s already registered\n", name );
	}
}

bool CScriptConvarAccessor::Init()
{
	static bool bExecOnce = false;
	if ( bExecOnce )
		return true;
	bExecOnce = true;

	AddOverridable( "+attack" );
	AddOverridable( "+attack2" );
	AddOverridable( "+attack3" );
	AddOverridable( "+forward" );
	AddOverridable( "+back" );
	AddOverridable( "+moveleft" );
	AddOverridable( "+moveright" );
	AddOverridable( "+use" );
	AddOverridable( "+jump" );
	AddOverridable( "+zoom" );
	AddOverridable( "+reload" );
	AddOverridable( "+speed" );
	AddOverridable( "+walk" );
	AddOverridable( "+duck" );
	AddOverridable( "+strafe" );
	AddOverridable( "+alt1" );
	AddOverridable( "+alt2" );
	AddOverridable( "+grenade1" );
	AddOverridable( "+grenade2" );
	AddOverridable( "+showscores" );

	AddOverridable( "-attack" );
	AddOverridable( "-attack2" );
	AddOverridable( "-attack3" );
	AddOverridable( "-forward" );
	AddOverridable( "-back" );
	AddOverridable( "-moveleft" );
	AddOverridable( "-moveright" );
	AddOverridable( "-use" );
	AddOverridable( "-jump" );
	AddOverridable( "-zoom" );
	AddOverridable( "-reload" );
	AddOverridable( "-speed" );
	AddOverridable( "-walk" );
	AddOverridable( "-duck" );
	AddOverridable( "-strafe" );
	AddOverridable( "-alt1" );
	AddOverridable( "-alt2" );
	AddOverridable( "-grenade1" );
	AddOverridable( "-grenade2" );
	AddOverridable( "-showscores" );

	AddOverridable( "toggle_duck" );
	AddOverridable( "lastinv" );
	AddOverridable( "invnext" );
	AddOverridable( "invprev" );
	AddOverridable( "phys_swap" );
	AddOverridable( "slot1" );
	AddOverridable( "slot2" );
	AddOverridable( "slot3" );
	AddOverridable( "slot4" );
	AddOverridable( "slot5" );
	AddOverridable( "slot6" );
	AddOverridable( "slot7" );

	AddOverridable( "save" );
	AddOverridable( "load" );


	AddBlockedConVar( "con_enable" );
	AddBlockedConVar( "cl_allowdownload" );
	AddBlockedConVar( "cl_allowupload" );
	AddBlockedConVar( "cl_downloadfilter" );

	return true;
}

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptConvarAccessor, "CConvars", SCRIPT_SINGLETON "Provides an interface to convars." )
	DEFINE_SCRIPTFUNC( RegisterConvar, "register a new console variable." )
	DEFINE_SCRIPTFUNC( RegisterCommand, "register a console command." )
	DEFINE_SCRIPTFUNC( SetCompletionCallback, "callback is called with 3 parameters (cmdname, partial, commands), user strings must be appended to 'commands' array" )
	DEFINE_SCRIPTFUNC( UnregisterCommand, "unregister a console command." )
	DEFINE_SCRIPTFUNC( GetCommandClient, "returns the player who issued this console command." )
#ifdef GAME_DLL
	DEFINE_SCRIPTFUNC( GetClientConvarValue, "Get a convar keyvalue for a specified client" )
#endif
	DEFINE_SCRIPTFUNC( GetFloat, "Returns the convar as a float. May return null if no such convar." )
	DEFINE_SCRIPTFUNC( GetInt, "Returns the convar as an int. May return null if no such convar." )
	DEFINE_SCRIPTFUNC( GetBool, "Returns the convar as a bool. May return null if no such convar." )
	DEFINE_SCRIPTFUNC( GetStr, "Returns the convar as a string. May return null if no such convar." )
	DEFINE_SCRIPTFUNC( GetDefaultValue, "Returns the convar's default value as a string. May return null if no such convar." )
	DEFINE_SCRIPTFUNC( IsFlagSet, "Returns the convar's flags. May return null if no such convar." )
	DEFINE_SCRIPTFUNC( SetFloat, "Sets the value of the convar as a float." )
	DEFINE_SCRIPTFUNC( SetInt, "Sets the value of the convar as an int." )
	DEFINE_SCRIPTFUNC( SetBool, "Sets the value of the convar as a bool." )
	DEFINE_SCRIPTFUNC( SetStr, "Sets the value of the convar as a string." )
END_SCRIPTDESC();


//=============================================================================
// Effects
// (Unique to mapbase)
//
// At the moment only clientside until a filtering method on server is finalised.
//
// TEs most of the time call IEffects (g_pEffects) or ITempEnts (tempents) on client,
// but they also record for tools recording mode.
//
// On client no TE is suppressed.
// TE flags are found at tempent.h
//
// TODO:
//=============================================================================
#ifdef CLIENT_DLL

class CEffectsScriptHelper
{
private:
	C_RecipientFilter filter;

public:
	void DynamicLight( int index, const Vector& origin, int r, int g, int b, int exponent,
		float radius, float die, float decay, int style = 0, int flags = 0 )
	{
		//te->DynamicLight( filter, delay, &origin, r, g, b, exponent, radius, die, decay );
		dlight_t *dl = effects->CL_AllocDlight( index );
		dl->origin = origin;
		dl->color.r = r;
		dl->color.g = g;
		dl->color.b = b;
		dl->color.exponent = exponent;
		dl->radius = radius;
		dl->die = gpGlobals->curtime + die;
		dl->decay = decay;
		dl->style = style;
		dl->flags = flags;
	}

	void Explosion( const Vector& pos, float scale, int radius, int magnitude, int flags )
	{
		filter.AddAllPlayers();
		// framerate, modelindex, normal and materialtype are unused
		// radius for ragdolls
		extern short g_sModelIndexFireball;
		te->Explosion( filter, 0.0f, &pos, g_sModelIndexFireball, scale, 15, flags, radius, magnitude, &vec3_origin );
	}

//	void FXExplosion( const Vector& pos, const Vector& normal, int materialType = 'C' )
//	{
//		// just the particles
//		// materialtype only for debris. can be 'C','W' or anything else.
//		FX_Explosion( const_cast<Vector&>(pos), const_cast<Vector&>(normal), materialType );
//	}

//	void ConcussiveExplosion( const Vector& pos, const Vector& normal )
//	{
//		FX_ConcussiveExplosion( const_cast<Vector&>(pos), const_cast<Vector&>(normal) );
//	}

//	void MicroExplosion( const Vector& pos, const Vector& normal )
//	{
//		FX_MicroExplosion( const_cast<Vector&>(pos), const_cast<Vector&>(normal) );
//	}

//	void MuzzleFlash( int type, HSCRIPT hEntity, int attachment, bool firstPerson )
//	{
//		C_BaseEntity *p = ToEnt(hEntity);
//		ClientEntityHandle_t ent = p ? (ClientEntityList().EntIndexToHandle)( p->entindex() ) : NULL;;
//		tempents->MuzzleFlash( type, ent, attachment, firstPerson );
//	}

	void Sparks( const Vector& pos, int nMagnitude, int nTrailLength, const Vector& pDir )
	{
		//te->Sparks( filter, delay, &pos, nMagnitude, nTrailLength, &pDir );
		//g_pEffects->Sparks( pos, nMagnitude, nTrailLength, &pDir );
		FX_ElectricSpark( pos, nMagnitude, nTrailLength, &pDir );
	}

	void MetalSparks( const Vector& pos, const Vector& dir )
	{
		//g_pEffects->MetalSparks( pos, dir );
		FX_MetalSpark( pos, dir, dir );
	}

//	void Smoke( const Vector& pos, float scale, int framerate)
//	{
//		extern short g_sModelIndexSmoke;
//		//te->Smoke( filter, 0.0, &pos, g_sModelIndexSmoke, scale * 10.0f, framerate );
//		g_pEffects->Smoke( pos, g_sModelIndexSmoke, scale, framerate );
//	}

	void Dust( const Vector &pos, const Vector &dir, float size, float speed )
	{
		//te->Dust( filter, delay, pos, dir, size, speed );
		//g_pEffects->Dust( pos, dir, size, speed );
		FX_Dust( pos, dir, size, speed );
	}

	void Bubbles( const Vector &mins, const Vector &maxs, float height, int modelindex, int count, float speed )
	{
		//int bubbles = modelinfo->GetModelIndex( "sprites/bubble.vmt" );
		//te->Bubbles( filter, delay, &mins, &maxs, height, modelindex, count, speed );
		tempents->Bubbles( mins, maxs, height, modelindex, count, speed );
	}

//	void Fizz( const Vector& mins, const Vector& maxs, int modelIndex, int density, int current/*, int flags*/ )
//	{
//		//te->Fizz( filter, delay, ent, modelindex, density, current );
//		//tempents->FizzEffect( ToEnt(ent), modelindex, density, current );
//	}

	void Sprite( const Vector &pos, const Vector &dir, float scale, int modelIndex, int rendermode,
		int renderfx, int brightness, float life, int flags  )
	{
		//te->Sprite( filter, delay, &pos, modelindex, size, brightness );
		float a = (1.0 / 255.0) * brightness;
		tempents->TempSprite( pos, dir, scale, modelIndex, rendermode, renderfx, a, life, flags );
	}

//	void PhysicsProp( float delay, int modelindex, int skin, const Vector& pos, const QAngle &angles,
//		const Vector& vel, int flags, int effects )
//	{
//		//te->PhysicsProp( filter, delay, modelindex, skin, pos, angles, vel, flags, effects );
//		tempents->PhysicsProp( modelindex, skin, pos, angles, vel, flags, effects );
//	}

	void ClientProjectile( const Vector& vecOrigin, const Vector& vecVelocity, const Vector& vecAccel, int modelindex,
		int lifetime, HSCRIPT pOwner, const char *pszImpactEffect = NULL, const char *pszParticleEffect = NULL )
	{
		//te->ClientProjectile( filter, delay, &vecOrigin, &vecVelocity, modelindex, lifetime, ToEnt(pOwner) );
		if ( pszImpactEffect && !(*pszImpactEffect) )
			pszImpactEffect = NULL;
		if ( pszParticleEffect && !(*pszParticleEffect) )
			pszParticleEffect = NULL;
		tempents->ClientProjectile( vecOrigin, vecVelocity, vecAccel, modelindex, lifetime, ToEnt(pOwner), pszImpactEffect, pszParticleEffect );
	}

} g_ScriptEffectsHelper;

BEGIN_SCRIPTDESC_ROOT_NAMED( CEffectsScriptHelper, "CEffects", SCRIPT_SINGLETON "" )
	DEFINE_SCRIPTFUNC( DynamicLight, "" )
	DEFINE_SCRIPTFUNC( Explosion, "" )
	DEFINE_SCRIPTFUNC( Sparks, "" )
	DEFINE_SCRIPTFUNC( MetalSparks, "" )
	DEFINE_SCRIPTFUNC( Dust, "" )
	DEFINE_SCRIPTFUNC( Bubbles, "" )
	DEFINE_SCRIPTFUNC( Sprite, "" )
	DEFINE_SCRIPTFUNC( ClientProjectile, "" )
END_SCRIPTDESC();
#endif


void RegisterScriptSingletons()
{
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptSaveRestoreUtil::SaveTable, "SaveTable", "Store a table with primitive values that will persist across level transitions and save loads." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptSaveRestoreUtil::RestoreTable, "RestoreTable", "Retrieves a table from storage. Write into input table." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptSaveRestoreUtil::ClearSavedTable, "ClearSavedTable", "Removes the table with the given context." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptReadWriteFile::FileWrite, "StringToFile", "Stores the string into the file" );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptReadWriteFile::FileRead, "FileToString", "Returns the string from the file, null if no file or file is too big." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptReadWriteFile::KeyValuesWrite, "KeyValuesToFile", "Stores the CScriptKeyValues into the file" );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptReadWriteFile::KeyValuesRead, "FileToKeyValues", "Returns the CScriptKeyValues from the file, null if no file or file is too big." );

	ScriptRegisterFunction( g_pScriptVM, ListenToGameEvent, "Register as a listener for a game event from script." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptGameEventListener::StopListeningToGameEvent, "StopListeningToGameEvent", "Stop the specified event listener." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptGameEventListener::StopListeningToAllGameEvents, "StopListeningToAllGameEvents", "Stop listening to all game events within a specific context." );
	ScriptRegisterFunction( g_pScriptVM, FireGameEvent, "Fire a game event." );
#ifndef CLIENT_DLL
	ScriptRegisterFunction( g_pScriptVM, FireGameEventLocal, "Fire a game event without broadcasting to the client." );
#endif

	g_pScriptVM->RegisterInstance( &g_ScriptNetPropManager, "NetProps" );
	g_pScriptVM->RegisterInstance( &g_ScriptLocalize, "Localize" );
	g_pScriptVM->RegisterInstance( g_ScriptNetMsg, "NetMsg" );
	g_pScriptVM->RegisterInstance( &g_ScriptDebugOverlay, "debugoverlay" );
	g_pScriptVM->RegisterInstance( &g_ScriptConvarAccessor, "Convars" );
#ifdef CLIENT_DLL
	g_pScriptVM->RegisterInstance( &g_ScriptEffectsHelper, "effects" );
#endif

	// Singletons not unique to VScript (not declared or defined here)
	g_pScriptVM->RegisterInstance( GameRules(), "GameRules" );
	g_pScriptVM->RegisterInstance( GetAmmoDef(), "AmmoDef" );
#ifndef CLIENT_DLL
	g_pScriptVM->RegisterInstance( &g_AI_SquadManager, "Squads" );
#endif

#ifdef USE_OLD_EVENT_DESCRIPTORS
	CScriptGameEventListener::LoadAllEvents();
#endif

	g_ScriptNetMsg->InitPostVM();
}
