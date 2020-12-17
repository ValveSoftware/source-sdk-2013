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

#ifndef CLIENT_DLL
#include "usermessages.h"
#include "ai_squad.h"
#endif // !CLIENT_DLL

#include "filesystem.h"
#include "igameevents.h"

#include "vscript_singletons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IScriptManager *scriptmanager;

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
class CScriptGameEventListener : public IGameEventListener2, public CAutoGameSystem
{
public:
	CScriptGameEventListener() : m_bActive(false) {}
	~CScriptGameEventListener()
	{
		StopListeningForEvent();
	}

	intptr_t ListenToGameEvent( const char* szEvent, HSCRIPT hFunc, const char* szContext );
	void StopListeningForEvent();

public:
	static bool StopListeningToGameEvent( intptr_t listener );
	static void StopListeningToAllGameEvents( const char* szContext );

public:
	void FireGameEvent( IGameEvent *event );
	void LevelShutdownPreEntity();

private:
	bool m_bActive;
	const char *m_pszContext;
	HSCRIPT m_hCallback;

	static const char *FindContext( const char *szContext, CScriptGameEventListener *pIgnore = NULL );
	//inline const char *GetContext( CScriptGameEventListener *p );
	//inline const char *GetContext();

public:
	static void DumpEventListeners();
#ifndef CLIENT_DLL
	static void LoadAllEvents();
	static void LoadEventsFromFile( const char *filename, const char *pathID = NULL );
	static void WriteEventData( IGameEvent *event, HSCRIPT hTable );
#endif // !CLIENT_DLL

private:
#ifndef CLIENT_DLL
	static CUtlVector< KeyValues* > s_GameEvents;
#endif // !CLIENT_DLL
	static CUtlVectorAutoPurge< CScriptGameEventListener* > s_GameEventListeners;

};

#ifndef CLIENT_DLL
CUtlVector< KeyValues* > CScriptGameEventListener::s_GameEvents;
#endif // !CLIENT_DLL
CUtlVectorAutoPurge< CScriptGameEventListener* > CScriptGameEventListener::s_GameEventListeners;

#if 0
#ifdef CLIENT_DLL
CON_COMMAND_F( cl_dump_script_game_event_listeners, "Dump all game event listeners created from script.", FCVAR_CHEAT )
{
	CScriptGameEventListener::DumpEventListeners();
}
#else // GAME_DLL
CON_COMMAND_F( dump_script_game_event_listeners, "Dump all game event listeners created from script.", FCVAR_CHEAT )
{
	CScriptGameEventListener::DumpEventListeners();
}
#endif // CLIENT_DLL
#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CScriptGameEventListener::DumpEventListeners()
{
	CGMsg( 0, CON_GROUP_VSCRIPT, "--- Script game event listener dump start\n" );
	FOR_EACH_VEC( s_GameEventListeners, i )
	{
		CGMsg( 0, CON_GROUP_VSCRIPT, " %d   (0x%p) %d : %s\n", i,s_GameEventListeners[i],
										s_GameEventListeners[i],
										s_GameEventListeners[i]->m_pszContext ? s_GameEventListeners[i]->m_pszContext : "");
	}
	CGMsg( 0, CON_GROUP_VSCRIPT, "--- Script game event listener dump end\n" );
}

void CScriptGameEventListener::FireGameEvent( IGameEvent *event )
{
	ScriptVariant_t hTable;
	g_pScriptVM->CreateTable( hTable );
	// TODO: pass event data on client
#ifdef GAME_DLL
	WriteEventData( event, hTable );
#endif
	g_pScriptVM->SetValue( hTable, "game_event_listener", reinterpret_cast<intptr_t>(this) ); // POINTER_TO_INT
	// g_pScriptVM->SetValue( hTable, "game_event_name", event->GetName() );
	g_pScriptVM->ExecuteFunction( m_hCallback, &hTable, 1, NULL, NULL, true );
	g_pScriptVM->ReleaseScript( hTable );
}

void CScriptGameEventListener::LevelShutdownPreEntity()
{
	s_GameEventListeners.FindAndFastRemove(this);
	delete this;
}

//-----------------------------------------------------------------------------
// Executed in LevelInitPreEntity
//-----------------------------------------------------------------------------
#ifndef CLIENT_DLL
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
	if ( s_GameEvents.Count() )
	{
		for ( int i = 0; i < s_GameEvents.Count(); ++i )
			s_GameEvents[i]->deleteThis();
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
		// CGMsg( 1, CON_GROUP_VSCRIPT, "CScriptGameEventListener::LoadEventsFromFile: Failed to load file %s, %s\n", filename, pathID );
		pKV->deleteThis();
		return;
	}

	// Set the key value types to what they are from their string description values to read the correct data type in WriteEventData.
	// There might be a better way of doing this, but this is okay since it's only done on file load.
	for ( KeyValues *key = pKV->GetFirstSubKey(); key; key = key->GetNextKey() )
		for ( KeyValues *sub = key->GetFirstSubKey(); sub; sub = sub->GetNextKey() )
		{
			if ( sub->GetDataType() == KeyValues::TYPE_STRING )
			{
				const char *szVal = sub->GetString();
				if ( !V_stricmp( szVal, "byte" ) || !V_stricmp( szVal, "short" ) || !V_stricmp( szVal, "long" ) || !V_stricmp( szVal, "bool" ) )
				{
					sub->SetInt( NULL, 0 );
				}
				else if ( !V_stricmp( szVal, "float" ) )
				{
					sub->SetFloat( NULL, 0.0f );
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

	CGMsg( 2, CON_GROUP_VSCRIPT, "CScriptGameEventListener::LoadEventsFromFile: Loaded %s, %s\n", filename, pathID );

	s_GameEvents.AddToTail(pKV);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CScriptGameEventListener::WriteEventData( IGameEvent *event, HSCRIPT hTable )
{
	// TODO: Something more efficient than iterating through all the events that ever exist one by one

	const char *szEvent = event->GetName();
	for ( int i = 0; i < s_GameEvents.Count(); ++i )
	{
		KeyValues *pKV = s_GameEvents[i];
		for ( KeyValues *key = pKV->GetFirstSubKey(); key; key = key->GetNextKey() )
		{
			if ( !V_stricmp( key->GetName(), szEvent ) )
			{
				for ( KeyValues *sub = key->GetFirstSubKey(); sub; sub = sub->GetNextKey() )
				{
					const char *szKey = sub->GetName();
					switch ( sub->GetDataType() )
					{
						case KeyValues::TYPE_STRING: g_pScriptVM->SetValue( hTable, szKey, event->GetString( szKey ) ); break;
						case KeyValues::TYPE_INT:    g_pScriptVM->SetValue( hTable, szKey, event->GetInt   ( szKey ) ); break;
						case KeyValues::TYPE_FLOAT:  g_pScriptVM->SetValue( hTable, szKey, event->GetFloat ( szKey ) ); break;
						// default: DevWarning( 2, "CScriptGameEventListener::WriteEventData: unknown data type '%d' on key '%s' in event '%s'\n", sub->GetDataType(), szKey, szEvent );
					}
				}
				return;
			}
		}
	}
}
#endif // !CLIENT_DLL
//-----------------------------------------------------------------------------
// Find if context is in use by others; used to alloc/dealloc only when required.
// Returns allocated pointer to string
// Expects non-NULL context input
//-----------------------------------------------------------------------------
const char *CScriptGameEventListener::FindContext( const char *szContext, CScriptGameEventListener *pIgnore )
{
	for ( int i = s_GameEventListeners.Count(); i--; )
	{
		CScriptGameEventListener *pCur = s_GameEventListeners[i];
		if ( pCur != pIgnore )
		{
			if ( pCur->m_pszContext && !V_stricmp( szContext, pCur->m_pszContext ) )
			{
				return pCur->m_pszContext;
			}
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
intptr_t CScriptGameEventListener::ListenToGameEvent( const char* szEvent, HSCRIPT hFunc, const char* szContext )
{
	m_bActive = true;

	char *psz;

	if ( szContext && *szContext )
	{
		psz = const_cast<char*>(FindContext(szContext));
		if ( !psz )
		{
			int len = V_strlen(szContext) + 1;
			if ( len > 1 )
			{
				int size = min( len, 256 ); // arbitrary clamp
				psz = new char[size];
				V_strncpy( psz, szContext, size );
			}
		}
	}
	else
	{
		psz = NULL;
	}

	m_pszContext = psz;
	m_hCallback = hFunc;

	if ( gameeventmanager )
#ifdef CLIENT_DLL
		gameeventmanager->AddListener( this, szEvent, false );
#else
		gameeventmanager->AddListener( this, szEvent, true );
#endif
	s_GameEventListeners.AddToTail( this );

	return reinterpret_cast<intptr_t>(this); // POINTER_TO_INT
}

//-----------------------------------------------------------------------------
// Free stuff. Called from the destructor, does not remove itself from the listener list.
//-----------------------------------------------------------------------------
void CScriptGameEventListener::StopListeningForEvent()
{
	if ( !m_bActive )
		return;

	if ( g_pScriptVM )
	{
		g_pScriptVM->ReleaseScript( m_hCallback );
	}
	else if ( m_hCallback )
	{
		AssertMsg( 0, "LEAK (0x%p)\n", (void*)m_hCallback );
	}

	if ( m_pszContext )
	{
		if ( !FindContext( m_pszContext, this ) )
		{
			delete[] m_pszContext;
		}

		m_pszContext = NULL;
	}

	m_hCallback = NULL;

	if ( gameeventmanager )
		gameeventmanager->RemoveListener( this );

	m_bActive = false;
}

//-----------------------------------------------------------------------------
// Stop the specified event listener.
//-----------------------------------------------------------------------------
bool CScriptGameEventListener::StopListeningToGameEvent( intptr_t listener )
{
	CScriptGameEventListener *p = reinterpret_cast<CScriptGameEventListener*>(listener); // INT_TO_POINTER	

	bool bRemoved = s_GameEventListeners.FindAndFastRemove(p);
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
	if ( szContext )
	{
		if ( *szContext )
		{
			// Iterate from the end so they can be safely removed as they are deleted
			for ( int i = s_GameEventListeners.Count(); i--; )
			{
				CScriptGameEventListener *pCur = s_GameEventListeners[i];
				if ( pCur->m_pszContext && !V_stricmp( szContext, pCur->m_pszContext ) )
				{
					s_GameEventListeners.Remove(i); // keep list order
					delete pCur;
				}
			}
		}
		else // empty (NULL) context
		{
			for ( int i = s_GameEventListeners.Count(); i--; )
			{
				CScriptGameEventListener *pCur = s_GameEventListeners[i];
				if ( !pCur->m_pszContext )
				{
					s_GameEventListeners.Remove(i);
					delete pCur;
				}
			}
		}
	}
#if 0
	if ( !szContext )
	{
		for ( int i = s_GameEventListeners.Count(); i--; )
			delete s_GameEventListeners[i];
		s_GameEventListeners.Purge();
	}
#endif
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
// Based on Source 2 API
//=============================================================================
class CScriptSaveRestoreUtil : public CAutoGameSystem
{
public:
	static void SaveTable( const char *szId, HSCRIPT hTable );
	static void RestoreTable( const char *szId, HSCRIPT hTable );
	static void ClearSavedTable( const char *szId );

// IGameSystem interface
public:
	void OnSave()
	{
		if ( g_pScriptVM )
		{
			HSCRIPT hFunc = g_pScriptVM->LookupFunction( "OnSave" );
			if ( hFunc )
			{
				g_pScriptVM->Call( hFunc );
			}
		}
	}

	void OnRestore()
	{
		if ( g_pScriptVM )
		{
			HSCRIPT hFunc = g_pScriptVM->LookupFunction( "OnRestore" );
			if ( hFunc )
			{
				g_pScriptVM->Call( hFunc );
			}
		}
	}

	void Shutdown()
	{
		FOR_EACH_VEC( m_aKeyValues, i )
			m_aKeyValues[i]->deleteThis();
		m_aKeyValues.Purge();
		m_aContext.PurgeAndDeleteElements();
	}

private:
	static int GetIndexForContext( const char *szId );

	// indices must match, always remove keeping order
	static CUtlStringList m_aContext;
	static CUtlVector<KeyValues*> m_aKeyValues;

} g_ScriptSaveRestoreUtil;

CUtlStringList CScriptSaveRestoreUtil::m_aContext;
CUtlVector<KeyValues*> CScriptSaveRestoreUtil::m_aKeyValues;

int CScriptSaveRestoreUtil::GetIndexForContext( const char *szId )
{
	int idx = -1;
	FOR_EACH_VEC( m_aContext, i )
	{
		if ( !V_stricmp( szId, m_aContext[i] ) )
		{
			idx = i;
			break;
		}
	}
	return idx;
}

//-----------------------------------------------------------------------------
// Store a table with primitive values that will persist across level transitions and save loads.
//-----------------------------------------------------------------------------
void CScriptSaveRestoreUtil::SaveTable( const char *szId, HSCRIPT hTable )
{
	int idx = GetIndexForContext(szId);

	KeyValues *pKV;

	if ( idx == -1 )
	{
		pKV = new KeyValues("ScriptSavedTable");
		m_aKeyValues.AddToTail(pKV);

		if ( V_strlen(szId) > 255 ) // arbitrary clamp
		{
			char c[256];
			V_strncpy( c, szId, sizeof(c) );
			m_aContext.CopyAndAddToTail(c);
		}
		else
		{
			m_aContext.CopyAndAddToTail(szId);
		}
	}
	else
	{
		pKV = m_aKeyValues[idx];
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
	int idx = GetIndexForContext(szId);

	KeyValues *pKV;

	if ( idx == -1 )
	{
		// DevWarning( 2, "RestoreTable could not find saved table with context '%s'\n", szId );
		return;
	}
	else
	{
		pKV = m_aKeyValues[idx];
	}

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
	int idx = GetIndexForContext(szId);

	if ( idx == -1 )
	{
		// DevWarning( 2, "ClearSavedTable could not find saved table with context '%s'\n", szId );
		return;
	}

	m_aKeyValues[idx]->deleteThis();
	m_aKeyValues.Remove(idx);

	delete[] m_aContext[idx];
	m_aContext.Remove(idx);
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
public:
	static bool ScriptFileWrite( const char *szFile, const char *szInput );
	static const char *ScriptFileRead( const char *szFile );
	//static const char *CRC32_Checksum( const char *szFilename );

	// NOTE: These two functions are new with Mapbase and have no Valve equivalent
	static bool ScriptKeyValuesWrite( const char *szFile, HSCRIPT hInput );
	static HSCRIPT ScriptKeyValuesRead( const char *szFile );

	void LevelShutdownPostEntity()
	{
		if ( m_pszReturnReadFile )
		{
			delete[] m_pszReturnReadFile;
			m_pszReturnReadFile = NULL;
		}

		//if ( m_pszReturnCRC32 )
		//{
		//	delete[] m_pszReturnCRC32;
		//	m_pszReturnCRC32 = NULL;
		//}
	}

private:
	static const char *m_pszReturnReadFile;
	//static const char *m_pszReturnCRC32;

} g_ScriptReadWrite;

const char *CScriptReadWriteFile::m_pszReturnReadFile = NULL;
//const char *CScriptReadWriteFile::m_pszReturnCRC32 = NULL;

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CScriptReadWriteFile::ScriptFileWrite( const char *szFile, const char *szInput )
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
const char *CScriptReadWriteFile::ScriptFileRead( const char *szFile )
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
bool CScriptReadWriteFile::ScriptKeyValuesWrite( const char *szFile, HSCRIPT hInput )
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
		DevWarning( 2, "Input is too large for a ScriptFileWrite ( %s / %d MB )\n", V_pretifymem(buf.Size(),2,true), (SCRIPT_MAX_FILE_WRITE_SIZE >> 20) );
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
HSCRIPT CScriptReadWriteFile::ScriptKeyValuesRead( const char *szFile )
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
// User Message Helper
// Based on Source 2 API
//=============================================================================
#ifndef CLIENT_DLL
class CNetMsgScriptHelper
{
private:
	CRecipientFilter filter;
	bf_write message;
	byte data_msg[ MAX_USER_MSG_DATA ];

	inline void SendMsg( bf_write *bf )
	{
		bf_read buffer = bf_read();
		buffer.StartReading( message.GetData(), message.m_nDataBytes );
		bf->WriteBitsFromBuffer( &buffer, message.GetNumBitsWritten() );
		engine->MessageEnd();
	}

public:
	inline void Reset()
	{
		message.StartWriting( data_msg, sizeof(data_msg) );
		filter.Reset();
	}

	void SendUserMessage( HSCRIPT player, const char *msg, bool bReliable )
	{
		int msg_type = usermessages->LookupUserMessage(msg);
		if ( msg_type == -1 )
		{
			g_pScriptVM->RaiseException("UserMessageBegin: Unregistered message");
			return;
		}

		CBaseEntity *pPlayer = ToEnt(player);
		if ( pPlayer )
		{
			filter.AddRecipient( (CBasePlayer*)pPlayer );
		}

		if ( bReliable )
		{
			filter.MakeReliable();
		}

		SendMsg( engine->UserMessageBegin( &filter, msg_type ) );
	}

	void SendEntityMessage( HSCRIPT hEnt, bool bReliable )
	{
		CBaseEntity *entity = ToEnt(hEnt);
		if ( !entity )
		{
			g_pScriptVM->RaiseException("EntityMessageBegin: invalid entity");
			return;
		}

		SendMsg( engine->EntityMessageBegin( entity->entindex(), entity->GetServerClass(), bReliable ) );
	}

public:
	void AddRecipient( HSCRIPT player )
	{
		CBaseEntity *pPlayer = ToEnt(player);
		if ( pPlayer )
		{
			filter.AddRecipient( (CBasePlayer*)pPlayer );
		}
	}

	void AddRecipientsByPVS( const Vector &pos )
	{
		filter.AddRecipientsByPVS(pos);
	}

	void AddAllPlayers()
	{
		filter.AddAllPlayers();
	}

public:
	void WriteByte( int iValue )                    { message.WriteByte( iValue );             }
	void WriteChar( int iValue )                    { message.WriteChar( iValue );             }
	void WriteShort( int iValue )                   { message.WriteShort( iValue );            }
	void WriteWord( int iValue )                    { message.WriteWord( iValue );             }
	void WriteLong( int iValue )                    { message.WriteLong( iValue );             }
	void WriteFloat( float flValue )                { message.WriteFloat( flValue );           }
	void WriteAngle( float flValue )                { message.WriteBitAngle( flValue, 8 );     }
	void WriteCoord( float flValue )                { message.WriteBitCoord( flValue );        }
	void WriteVec3Coord( const Vector& rgflValue )  { message.WriteBitVec3Coord( rgflValue );  }
	void WriteVec3Normal( const Vector& rgflValue ) { message.WriteBitVec3Normal( rgflValue ); }
	void WriteAngles( const QAngle& rgflValue )     { message.WriteBitAngles( rgflValue );     }
	void WriteString( const char *sz )              { message.WriteString( sz );               }
	void WriteEntity( int iValue )                  { message.WriteShort( iValue );            }
	void WriteBool( bool bValue )                   { message.WriteOneBit( bValue ? 1 : 0 );   }
	void WriteEHandle( HSCRIPT hEnt )
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
		message.WriteLong( iEncodedEHandle );
	}

} g_ScriptNetMsg;

BEGIN_SCRIPTDESC_ROOT_NAMED( CNetMsgScriptHelper, "CNetMsg", SCRIPT_SINGLETON "NetworkMessages" )
	DEFINE_SCRIPTFUNC( Reset, "" )
	DEFINE_SCRIPTFUNC( SendUserMessage, "" )
	DEFINE_SCRIPTFUNC( SendEntityMessage, "" )
	DEFINE_SCRIPTFUNC( AddRecipient, "" )
	DEFINE_SCRIPTFUNC( AddRecipientsByPVS, "" )
	DEFINE_SCRIPTFUNC( AddAllPlayers, "" )
	DEFINE_SCRIPTFUNC( WriteByte, "" )
	DEFINE_SCRIPTFUNC( WriteChar, "" )
	DEFINE_SCRIPTFUNC( WriteShort, "" )
	DEFINE_SCRIPTFUNC( WriteWord, "" )
	DEFINE_SCRIPTFUNC( WriteLong, "" )
	DEFINE_SCRIPTFUNC( WriteFloat, "" )
	DEFINE_SCRIPTFUNC( WriteAngle, "" )
	DEFINE_SCRIPTFUNC( WriteCoord, "" )
	DEFINE_SCRIPTFUNC( WriteVec3Coord, "" )
	DEFINE_SCRIPTFUNC( WriteVec3Normal, "" )
	DEFINE_SCRIPTFUNC( WriteAngles, "" )
	DEFINE_SCRIPTFUNC( WriteString, "" )
	DEFINE_SCRIPTFUNC( WriteEntity, "" )
	DEFINE_SCRIPTFUNC( WriteEHandle, "" )
	DEFINE_SCRIPTFUNC( WriteBool, "" )
END_SCRIPTDESC();

#endif // !CLIENT_DLL

void RegisterScriptSingletons()
{
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptSaveRestoreUtil::SaveTable, "SaveTable", "Store a table with primitive values that will persist across level transitions and save loads." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptSaveRestoreUtil::RestoreTable, "RestoreTable", "Retrieves a table from storage. Write into input table." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptSaveRestoreUtil::ClearSavedTable, "ClearSavedTable", "Removes the table with the given context." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptReadWriteFile::ScriptFileWrite, "StringToFile", "Stores the string into the file" );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptReadWriteFile::ScriptFileRead, "FileToString", "Returns the string from the file, null if no file or file is too big." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptReadWriteFile::ScriptKeyValuesWrite, "KeyValuesToFile", "Stores the CScriptKeyValues into the file" );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptReadWriteFile::ScriptKeyValuesRead, "FileToKeyValues", "Returns the CScriptKeyValues from the file, null if no file or file is too big." );

	ScriptRegisterFunction( g_pScriptVM, ListenToGameEvent, "Register as a listener for a game event from script." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptGameEventListener::StopListeningToGameEvent, "StopListeningToGameEvent", "Stop the specified event listener." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptGameEventListener::StopListeningToAllGameEvents, "StopListeningToAllGameEvents", "Stop listening to all game events within a specific context." );
	ScriptRegisterFunction( g_pScriptVM, FireGameEvent, "Fire a game event." );
#ifndef CLIENT_DLL
	ScriptRegisterFunction( g_pScriptVM, FireGameEventLocal, "Fire a game event without broadcasting to the client." );
#endif

	g_pScriptVM->RegisterInstance( &g_ScriptNetPropManager, "NetProps" );
	g_pScriptVM->RegisterInstance( &g_ScriptLocalize, "Localize" );
#ifndef CLIENT_DLL
	g_pScriptVM->RegisterInstance( &g_ScriptNetMsg, "NetMsg" );
#endif

	// Singletons not unique to VScript (not declared or defined here)
	g_pScriptVM->RegisterInstance( GameRules(), "GameRules" );
	g_pScriptVM->RegisterInstance( GetAmmoDef(), "AmmoDef" );
#ifndef CLIENT_DLL
	g_pScriptVM->RegisterInstance( &g_AI_SquadManager, "Squads" );
#endif

#ifndef CLIENT_DLL
	CScriptGameEventListener::LoadAllEvents();
#endif // !CLIENT_DLL
}
