//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Due to this being a custom integration of VScript based on the Alien Swarm SDK, we don't have access to
//			some of the code normally available in games like L4D2 or Valve's original VScript DLL.
//			Instead, that code is recreated here, shared between server and client.
//
//			It also contains other functions unique to Mapbase.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "matchers.h"
#include "takedamageinfo.h"
#include "ammodef.h"
#include <vgui_controls/Controls.h> 
#include <vgui/ILocalize.h>

#ifndef CLIENT_DLL
#include "globalstate.h"
#include "vscript_server.h"

#include "usermessages.h"
#endif // !CLIENT_DLL

#include "filesystem.h"
#include "igameevents.h"
#include "icommandline.h"
#include "con_nprint.h"
#include "particle_parse.h"

#include "vscript_funcs_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CScriptConvarLookup
{
public:

#ifndef CLIENT_DLL
	const char *GetClientConvarValue( const char *pszConVar, int entindex )
	{
		return engine->GetClientConVarValue( entindex, pszConVar );
	}
#endif

	const char *GetStr( const char *pszConVar )
	{
		ConVarRef cvar( pszConVar );
		return cvar.GetString();
	}

	float GetFloat( const char *pszConVar )
	{
		ConVarRef cvar( pszConVar );
		return cvar.GetFloat();
	}

	void SetValue( const char *pszConVar, const char *pszValue )
	{
		ConVarRef cvar( pszConVar );
		if (!cvar.IsValid())
			return;

		// FCVAR_NOT_CONNECTED can be used to protect specific convars from nefarious interference
		if (cvar.IsFlagSet(FCVAR_NOT_CONNECTED))
			return;

		cvar.SetValue( pszValue );
	}

private:
} g_ScriptConvarLookup;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptConvarLookup, "CConvars", SCRIPT_SINGLETON "Provides an interface for getting and setting convars." )
#ifndef CLIENT_DLL
	DEFINE_SCRIPTFUNC( GetClientConvarValue, "Returns the convar value for the entindex as a string. Only works with client convars with the FCVAR_USERINFO flag." )
#endif
	DEFINE_SCRIPTFUNC( GetStr, "Returns the convar as a string. May return null if no such convar." )
	DEFINE_SCRIPTFUNC( GetFloat, "Returns the convar as a float. May return null if no such convar." )
	DEFINE_SCRIPTFUNC( SetValue, "Sets the value of the convar. Supported types are bool, int, float, string." )
END_SCRIPTDESC();

#ifndef CLIENT_DLL
void EmitSoundOn( const char *pszSound, HSCRIPT hEnt )
{
	CBaseEntity *pEnt = ToEnt( hEnt );
	if (!pEnt)
		return;

	pEnt->EmitSound( pszSound );
}

void EmitSoundOnClient( const char *pszSound, HSCRIPT hEnt, HSCRIPT hPlayer )
{
	CBaseEntity *pEnt = ToEnt( hEnt );
	CBasePlayer *pPlayer = ToBasePlayer( ToEnt( hPlayer ) );
	if (!pEnt || !pPlayer)
		return;

	CSingleUserRecipientFilter filter( pPlayer );

	EmitSound_t params;
	params.m_pSoundName = pszSound;
	params.m_flSoundTime = 0.0f;
	params.m_pflSoundDuration = NULL;
	params.m_bWarnOnDirectWaveReference = true;

	pEnt->EmitSound( filter, pEnt->entindex(), params );
}

void AddThinkToEnt( HSCRIPT entity, const char *pszFuncName )
{
	CBaseEntity *pEntity = ToEnt( entity );
	if (!pEntity)
		return;

	pEntity->ScriptSetThinkFunction(pszFuncName, TICK_INTERVAL);
}

HSCRIPT EntIndexToHScript( int index )
{
	return ToHScript( UTIL_EntityByIndex( index ) );
}

void ParseScriptTableKeyValues( CBaseEntity *pEntity, HSCRIPT hKV )
{
	int nIterator = -1;
	ScriptVariant_t varKey, varValue;
	while ((nIterator = g_pScriptVM->GetKeyValue( hKV, nIterator, &varKey, &varValue )) != -1)
	{
		switch (varValue.m_type)
		{
			case FIELD_CSTRING:		pEntity->KeyValue( varKey.m_pszString, varValue.m_pszString ); break;
			case FIELD_FLOAT:		pEntity->KeyValue( varKey.m_pszString, varValue.m_float ); break;
			case FIELD_VECTOR:		pEntity->KeyValue( varKey.m_pszString, *varValue.m_pVector ); break;
		}

		g_pScriptVM->ReleaseValue( varKey );
		g_pScriptVM->ReleaseValue( varValue );
	}
}

void PrecacheEntityFromTable( const char *pszClassname, HSCRIPT hKV )
{
	if ( IsEntityCreationAllowedInScripts() == false )
	{
		Warning( "VScript error: A script attempted to create an entity mid-game. Due to the server's settings, entity creation from scripts is only allowed during map init.\n" );
		return;
	}

	// This is similar to UTIL_PrecacheOther(), but we can't check if we can only precache it once.
	// Probably for the best anyway, as similar classes can still have different precachable properties.
	CBaseEntity *pEntity = CreateEntityByName( pszClassname );
	if (!pEntity)
	{
		Assert( !"PrecacheEntityFromTable: only works for CBaseEntities" );
		return;
	}

	ParseScriptTableKeyValues( pEntity, hKV );

	pEntity->Precache();

	UTIL_RemoveImmediate( pEntity );
}

HSCRIPT SpawnEntityFromTable( const char *pszClassname, HSCRIPT hKV )
{
	if ( IsEntityCreationAllowedInScripts() == false )
	{
		Warning( "VScript error: A script attempted to create an entity mid-game. Due to the server's settings, entity creation from scripts is only allowed during map init.\n" );
		return NULL;
	}

	CBaseEntity *pEntity = CreateEntityByName( pszClassname );
	if ( !pEntity )
	{
		Assert( !"SpawnEntityFromTable: only works for CBaseEntities" );
		return NULL;
	}

	gEntList.NotifyCreateEntity( pEntity );

	ParseScriptTableKeyValues( pEntity, hKV );

	DispatchSpawn( pEntity );
	pEntity->Activate();

	return ToHScript( pEntity );
}
#endif

//-----------------------------------------------------------------------------
// Mapbase-specific functions start here
//-----------------------------------------------------------------------------

static void ScriptMsg( const char *msg )
{
	Msg( "%s", msg );
}

static void ScriptColorPrint( int r, int g, int b, const char *pszMsg )
{
	const Color clr(r, g, b, 255);
	ConColorMsg( clr, "%s", pszMsg );
}

static void ScriptColorPrintL( int r, int g, int b, const char *pszMsg )
{
	const Color clr(r, g, b, 255);
	ConColorMsg( clr, "%s\n", pszMsg );
}

#ifndef CLIENT_DLL
HSCRIPT SpawnEntityFromKeyValues( const char *pszClassname, HSCRIPT hKV )
{
	if ( IsEntityCreationAllowedInScripts() == false )
	{
		Warning( "VScript error: A script attempted to create an entity mid-game. Due to the server's settings, entity creation from scripts is only allowed during map init.\n" );
		return NULL;
	}

	CBaseEntity *pEntity = CreateEntityByName( pszClassname );
	if ( !pEntity )
	{
		Assert( !"SpawnEntityFromKeyValues: only works for CBaseEntities" );
		return NULL;
	}

	gEntList.NotifyCreateEntity( pEntity );

	CScriptKeyValues *pScriptKV = hKV ? HScriptToClass<CScriptKeyValues>( hKV ) : NULL;
	if (pScriptKV)
	{
		KeyValues *pKV = pScriptKV->m_pKeyValues;
		for (pKV = pKV->GetFirstSubKey(); pKV != NULL; pKV = pKV->GetNextKey())
		{
			pEntity->KeyValue( pKV->GetName(), pKV->GetString() );
		}
	}

	DispatchSpawn( pEntity );
	pEntity->Activate();

	return ToHScript( pEntity );
}

void ScriptDispatchSpawn( HSCRIPT hEntity )
{
	CBaseEntity *pEntity = ToEnt( hEntity );
	if (pEntity)
	{
		DispatchSpawn( pEntity );
	}
}
#endif // !CLIENT_DLL

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static HSCRIPT CreateDamageInfo( HSCRIPT hInflictor, HSCRIPT hAttacker, const Vector &vecForce, const Vector &vecDamagePos, float flDamage, int iDamageType )
{
	// The script is responsible for deleting this via DestroyDamageInfo().
	CTakeDamageInfo *damageInfo = new CTakeDamageInfo(ToEnt(hInflictor), ToEnt(hAttacker), flDamage, iDamageType);
	HSCRIPT hScript = g_pScriptVM->RegisterInstance( damageInfo, true );

	damageInfo->SetDamagePosition( vecDamagePos );
	damageInfo->SetDamageForce( vecForce );

	return hScript;
}

static void DestroyDamageInfo( HSCRIPT hDamageInfo )
{
	if (hDamageInfo)
	{
		CTakeDamageInfo *pInfo = (CTakeDamageInfo*)g_pScriptVM->GetInstanceValue( hDamageInfo, GetScriptDescForClass( CTakeDamageInfo ) );
		if (pInfo)
		{
			g_pScriptVM->RemoveInstance( hDamageInfo );
			delete pInfo;
		}
	}
}

void ScriptCalculateExplosiveDamageForce( HSCRIPT info, const Vector &vecDir, const Vector &vecForceOrigin, float flScale ) { CalculateExplosiveDamageForce( HScriptToClass<CTakeDamageInfo>(info), vecDir, vecForceOrigin, flScale ); }
void ScriptCalculateBulletDamageForce( HSCRIPT info, int iBulletType, const Vector &vecBulletDir, const Vector &vecForceOrigin, float flScale ) { CalculateBulletDamageForce( HScriptToClass<CTakeDamageInfo>(info), iBulletType, vecBulletDir, vecForceOrigin, flScale ); }
void ScriptCalculateMeleeDamageForce( HSCRIPT info, const Vector &vecMeleeDir, const Vector &vecForceOrigin, float flScale ) { CalculateMeleeDamageForce( HScriptToClass<CTakeDamageInfo>( info ), vecMeleeDir, vecForceOrigin, flScale ); }
void ScriptGuessDamageForce( HSCRIPT info, const Vector &vecForceDir, const Vector &vecForceOrigin, float flScale ) { GuessDamageForce( HScriptToClass<CTakeDamageInfo>( info ), vecForceDir, vecForceOrigin, flScale ); }

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BEGIN_SCRIPTDESC_ROOT_NAMED( CTraceInfoAccessor, "CGameTrace", "Handle for accessing trace_t info." )
	DEFINE_SCRIPT_CONSTRUCTOR()

	DEFINE_SCRIPTFUNC( DidHitWorld, "Returns whether the trace hit the world entity or not." )
	DEFINE_SCRIPTFUNC( DidHitNonWorldEntity, "Returns whether the trace hit something other than the world entity." )
	DEFINE_SCRIPTFUNC( GetEntityIndex, "Returns the index of whatever entity this trace hit." )
	DEFINE_SCRIPTFUNC( DidHit, "Returns whether the trace hit anything." )

	DEFINE_SCRIPTFUNC( FractionLeftSolid, "If this trace started within a solid, this is the point in the trace's fraction at which it left that solid." )
	DEFINE_SCRIPTFUNC( HitGroup, "Returns the specific hit group this trace hit if it hit an entity." )
	DEFINE_SCRIPTFUNC( PhysicsBone, "Returns the physics bone this trace hit if it hit an entity." )
	DEFINE_SCRIPTFUNC( Entity, "Returns the entity this trace has hit." )
	DEFINE_SCRIPTFUNC( HitBox, "Returns the hitbox of the entity this trace has hit. If it hit the world entity, this returns the static prop index." )

	DEFINE_SCRIPTFUNC( IsDispSurface, "Returns whether this trace hit a displacement." )
	DEFINE_SCRIPTFUNC( IsDispSurfaceWalkable, "Returns whether DISPSURF_FLAG_WALKABLE is ticked on the displacement this trace hit." )
	DEFINE_SCRIPTFUNC( IsDispSurfaceBuildable, "Returns whether DISPSURF_FLAG_BUILDABLE is ticked on the displacement this trace hit." )
	DEFINE_SCRIPTFUNC( IsDispSurfaceProp1, "Returns whether DISPSURF_FLAG_SURFPROP1 is ticked on the displacement this trace hit." )
	DEFINE_SCRIPTFUNC( IsDispSurfaceProp2, "Returns whether DISPSURF_FLAG_SURFPROP2 is ticked on the displacement this trace hit." )

	DEFINE_SCRIPTFUNC( StartPos, "Gets the trace's start position." )
	DEFINE_SCRIPTFUNC( EndPos, "Gets the trace's end position." )

	DEFINE_SCRIPTFUNC( Fraction, "Gets the fraction of the trace completed. For example, if the trace stopped exactly halfway to the end position, this would be 0.5." )
	DEFINE_SCRIPTFUNC( Contents, "Gets the contents of the surface the trace has hit." )
	DEFINE_SCRIPTFUNC( DispFlags, "Gets the displacement flags of the surface the trace has hit." )

	DEFINE_SCRIPTFUNC( AllSolid, "Returns whether the trace is completely within a solid." )
	DEFINE_SCRIPTFUNC( StartSolid, "Returns whether the trace started within a solid." )

	DEFINE_SCRIPTFUNC( Surface, "Returns the trace's surface." )
	DEFINE_SCRIPTFUNC( Plane, "Returns the trace's plane." )

	DEFINE_SCRIPTFUNC( Destroy, "Deletes this instance. Important for preventing memory leaks." )
END_SCRIPTDESC();

BEGIN_SCRIPTDESC_ROOT_NAMED( surfacedata_t, "surfacedata_t", "Handle for accessing surface data." )
	DEFINE_SCRIPTFUNC( GetFriction, "The surface's friction." )
	DEFINE_SCRIPTFUNC( GetThickness, "The surface's thickness." )

	DEFINE_SCRIPTFUNC( GetJumpFactor, "The surface's jump factor." )
	DEFINE_SCRIPTFUNC( GetMaterialChar, "The surface's material character." )
END_SCRIPTDESC();

BEGIN_SCRIPTDESC_ROOT_NAMED( CSurfaceScriptAccessor, "csurface_t", "Handle for accessing csurface_t info." )
	DEFINE_SCRIPTFUNC( Name, "The surface's name." )
	DEFINE_SCRIPTFUNC( SurfaceProps, "The surface's properties." )

	DEFINE_SCRIPTFUNC( Destroy, "Deletes this instance. Important for preventing memory leaks." )
END_SCRIPTDESC();

CPlaneTInstanceHelper g_PlaneTInstanceHelper;

BEGIN_SCRIPTDESC_ROOT( cplane_t, "Handle for accessing cplane_t info." )
	DEFINE_SCRIPT_INSTANCE_HELPER( &g_PlaneTInstanceHelper )
END_SCRIPTDESC();

static HSCRIPT ScriptTraceLineComplex( const Vector &vecStart, const Vector &vecEnd, HSCRIPT entIgnore, int iMask, int iCollisionGroup )
{
	// The script is responsible for deleting this via Destroy().
	CTraceInfoAccessor *traceInfo = new CTraceInfoAccessor();
	HSCRIPT hScript = g_pScriptVM->RegisterInstance( traceInfo, true );

	CBaseEntity *pLooker = ToEnt(entIgnore);
	UTIL_TraceLine( vecStart, vecEnd, iMask, pLooker, iCollisionGroup, &traceInfo->GetTrace());

	// The trace's destruction should destroy this automatically
	CSurfaceScriptAccessor *surfaceInfo = new CSurfaceScriptAccessor( traceInfo->GetTrace().surface );
	HSCRIPT hSurface = g_pScriptVM->RegisterInstance( surfaceInfo );
	traceInfo->SetSurface( hSurface );

	HSCRIPT hPlane = g_pScriptVM->RegisterInstance( &(traceInfo->GetTrace().plane) );
	traceInfo->SetPlane( hPlane );

	return hScript;
}

static HSCRIPT ScriptTraceHullComplex( const Vector &vecStart, const Vector &vecEnd, const Vector &hullMin, const Vector &hullMax,
	HSCRIPT entIgnore, int iMask, int iCollisionGroup )
{
	// The script is responsible for deleting this via Destroy().
	CTraceInfoAccessor *traceInfo = new CTraceInfoAccessor();
	HSCRIPT hScript = g_pScriptVM->RegisterInstance( traceInfo, true );

	CBaseEntity *pLooker = ToEnt(entIgnore);
	UTIL_TraceHull( vecStart, vecEnd, hullMin, hullMax, iMask, pLooker, iCollisionGroup, &traceInfo->GetTrace());

	// The trace's destruction should destroy this automatically
	CSurfaceScriptAccessor *surfaceInfo = new CSurfaceScriptAccessor( traceInfo->GetTrace().surface );
	HSCRIPT hSurface = g_pScriptVM->RegisterInstance( surfaceInfo );
	traceInfo->SetSurface( hSurface );

	HSCRIPT hPlane = g_pScriptVM->RegisterInstance( &(traceInfo->GetTrace().plane) );
	traceInfo->SetPlane( hPlane );

	return hScript;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BEGIN_SCRIPTDESC_ROOT( FireBulletsInfo_t, "Handle for accessing FireBulletsInfo_t info." )
	DEFINE_SCRIPT_CONSTRUCTOR()

	DEFINE_SCRIPTFUNC( GetShots, "Gets the number of shots which should be fired." )
	DEFINE_SCRIPTFUNC( SetShots, "Sets the number of shots which should be fired." )

	DEFINE_SCRIPTFUNC( GetSource, "Gets the source of the bullets." )
	DEFINE_SCRIPTFUNC( SetSource, "Sets the source of the bullets." )
	DEFINE_SCRIPTFUNC( GetDirShooting, "Gets the direction of the bullets." )
	DEFINE_SCRIPTFUNC( SetDirShooting, "Sets the direction of the bullets." )
	DEFINE_SCRIPTFUNC( GetSpread, "Gets the spread of the bullets." )
	DEFINE_SCRIPTFUNC( SetSpread, "Sets the spread of the bullets." )

	DEFINE_SCRIPTFUNC( GetDistance, "Gets the distance the bullets should travel." )
	DEFINE_SCRIPTFUNC( SetDistance, "Sets the distance the bullets should travel." )

	DEFINE_SCRIPTFUNC( GetAmmoType, "Gets the ammo type the bullets should use." )
	DEFINE_SCRIPTFUNC( SetAmmoType, "Sets the ammo type the bullets should use." )

	DEFINE_SCRIPTFUNC( GetTracerFreq, "Gets the tracer frequency." )
	DEFINE_SCRIPTFUNC( SetTracerFreq, "Sets the tracer frequency." )

	DEFINE_SCRIPTFUNC( GetDamage, "Gets the damage the bullets should deal. 0 = use ammo type" )
	DEFINE_SCRIPTFUNC( SetDamage, "Sets the damage the bullets should deal. 0 = use ammo type" )
	DEFINE_SCRIPTFUNC( GetPlayerDamage, "Gets the damage the bullets should deal when hitting the player. 0 = use regular damage" )
	DEFINE_SCRIPTFUNC( SetPlayerDamage, "Sets the damage the bullets should deal when hitting the player. 0 = use regular damage" )

	DEFINE_SCRIPTFUNC( GetFlags, "Gets the flags the bullets should use." )
	DEFINE_SCRIPTFUNC( SetFlags, "Sets the flags the bullets should use." )

	DEFINE_SCRIPTFUNC( GetDamageForceScale, "Gets the scale of the damage force applied by the bullets." )
	DEFINE_SCRIPTFUNC( SetDamageForceScale, "Sets the scale of the damage force applied by the bullets." )

	DEFINE_SCRIPTFUNC_NAMED( ScriptGetAttacker, "GetAttacker", "Gets the entity considered to be the one who fired the bullets." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetAttacker, "SetAttacker", "Sets the entity considered to be the one who fired the bullets." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetAdditionalIgnoreEnt, "GetAdditionalIgnoreEnt", "Gets the optional entity which the bullets should ignore." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetAdditionalIgnoreEnt, "SetAdditionalIgnoreEnt", "Sets the optional entity which the bullets should ignore." )

	DEFINE_SCRIPTFUNC( GetPrimaryAttack, "Gets whether the bullets came from a primary attack." )
	DEFINE_SCRIPTFUNC( SetPrimaryAttack, "Sets whether the bullets came from a primary attack." )

	//DEFINE_SCRIPTFUNC( Destroy, "Deletes this instance. Important for preventing memory leaks." )
END_SCRIPTDESC();

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
HSCRIPT FireBulletsInfo_t::ScriptGetAttacker()
{
	return ToHScript( m_pAttacker );
}

void FireBulletsInfo_t::ScriptSetAttacker( HSCRIPT value )
{
	m_pAttacker = ToEnt( value );
}

HSCRIPT FireBulletsInfo_t::ScriptGetAdditionalIgnoreEnt()
{
	return ToHScript( m_pAdditionalIgnoreEnt );
}

void FireBulletsInfo_t::ScriptSetAdditionalIgnoreEnt( HSCRIPT value )
{
	m_pAdditionalIgnoreEnt = ToEnt( value );
}

static HSCRIPT CreateFireBulletsInfo( int cShots, const Vector &vecSrc, const Vector &vecDirShooting,
	const Vector &vecSpread, float iDamage, HSCRIPT pAttacker )
{
	// The script is responsible for deleting this via DestroyFireBulletsInfo().
	FireBulletsInfo_t *info = new FireBulletsInfo_t();
	HSCRIPT hScript = g_pScriptVM->RegisterInstance( info, true );

	info->SetShots( cShots );
	info->SetSource( vecSrc );
	info->SetDirShooting( vecDirShooting );
	info->SetSpread( vecSpread );
	info->SetDamage( iDamage );
	info->ScriptSetAttacker( pAttacker );

	return hScript;
}

static void DestroyFireBulletsInfo( HSCRIPT hBulletsInfo )
{
	g_pScriptVM->RemoveInstance( hBulletsInfo );
}

// For the function in baseentity.cpp
FireBulletsInfo_t *GetFireBulletsInfoFromInfo( HSCRIPT hBulletsInfo )
{
	return HScriptToClass<FireBulletsInfo_t>( hBulletsInfo );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BEGIN_SCRIPTDESC_ROOT( CUserCmd, "Handle for accessing CUserCmd info." )
	DEFINE_SCRIPTFUNC( GetCommandNumber, "For matching server and client commands for debugging." )

	DEFINE_SCRIPTFUNC_NAMED( ScriptGetTickCount, "GetTickCount", "The tick the client created this command." )

	DEFINE_SCRIPTFUNC( GetViewAngles, "Player instantaneous view angles." )
	DEFINE_SCRIPTFUNC( SetViewAngles, "Sets player instantaneous view angles." )

	DEFINE_SCRIPTFUNC( GetForwardMove, "Forward velocity." )
	DEFINE_SCRIPTFUNC( SetForwardMove, "Sets forward velocity." )
	DEFINE_SCRIPTFUNC( GetSideMove, "Side velocity." )
	DEFINE_SCRIPTFUNC( SetSideMove, "Sets side velocity." )
	DEFINE_SCRIPTFUNC( GetUpMove, "Up velocity." )
	DEFINE_SCRIPTFUNC( SetUpMove, "Sets up velocity." )

	DEFINE_SCRIPTFUNC( GetButtons, "Attack button states." )
	DEFINE_SCRIPTFUNC( SetButtons, "Sets attack button states." )
	DEFINE_SCRIPTFUNC( GetImpulse, "Impulse command issued." )
	DEFINE_SCRIPTFUNC( SetImpulse, "Sets impulse command issued." )

	DEFINE_SCRIPTFUNC( GetWeaponSelect, "Current weapon id." )
	DEFINE_SCRIPTFUNC( SetWeaponSelect, "Sets current weapon id." )
	DEFINE_SCRIPTFUNC( GetWeaponSubtype, "Current weapon subtype id." )
	DEFINE_SCRIPTFUNC( SetWeaponSubtype, "Sets current weapon subtype id." )

	DEFINE_SCRIPTFUNC( GetRandomSeed, "For shared random functions." )

	DEFINE_SCRIPTFUNC( GetMouseX, "Mouse accum in x from create move." )
	DEFINE_SCRIPTFUNC( SetMouseX, "Sets mouse accum in x from create move." )
	DEFINE_SCRIPTFUNC( GetMouseY, "Mouse accum in y from create move." )
	DEFINE_SCRIPTFUNC( SetMouseY, "Sets mouse accum in y from create move." )
END_SCRIPTDESC();

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BEGIN_SCRIPTDESC_ROOT( IPhysicsObject, "VPhysics object class." )

	DEFINE_SCRIPTFUNC( IsStatic, "" )
	DEFINE_SCRIPTFUNC( IsAsleep, "" )
	DEFINE_SCRIPTFUNC( IsTrigger, "" )
	DEFINE_SCRIPTFUNC( IsFluid, "" )
	DEFINE_SCRIPTFUNC( IsHinged, "" )
	DEFINE_SCRIPTFUNC( IsCollisionEnabled, "" )
	DEFINE_SCRIPTFUNC( IsGravityEnabled, "" )
	DEFINE_SCRIPTFUNC( IsDragEnabled, "" )
	DEFINE_SCRIPTFUNC( IsMotionEnabled, "" )
	DEFINE_SCRIPTFUNC( IsMoveable, "" )
	DEFINE_SCRIPTFUNC( IsAttachedToConstraint, "" )

	DEFINE_SCRIPTFUNC( EnableCollisions, "" )
	DEFINE_SCRIPTFUNC( EnableGravity, "" )
	DEFINE_SCRIPTFUNC( EnableDrag, "" )
	DEFINE_SCRIPTFUNC( EnableMotion, "" )

	DEFINE_SCRIPTFUNC( Wake, "" )
	DEFINE_SCRIPTFUNC( Sleep, "" )

	DEFINE_SCRIPTFUNC( SetMass, "" )
	DEFINE_SCRIPTFUNC( GetMass, "" )
	DEFINE_SCRIPTFUNC( GetInvMass, "" )
	DEFINE_SCRIPTFUNC( GetInertia, "" )
	DEFINE_SCRIPTFUNC( GetInvInertia, "" )
	DEFINE_SCRIPTFUNC( SetInertia, "" )

	DEFINE_SCRIPTFUNC( ApplyForceCenter, "" )
	DEFINE_SCRIPTFUNC( ApplyForceOffset, "" )
	DEFINE_SCRIPTFUNC( ApplyTorqueCenter, "" )

	DEFINE_SCRIPTFUNC( GetName, "" )

END_SCRIPTDESC();

static const Vector &GetPhysVelocity( HSCRIPT hPhys )
{
	IPhysicsObject *pPhys = HScriptToClass<IPhysicsObject>( hPhys );
	if (!pPhys)
		return vec3_origin;

	static Vector vecVelocity;
	pPhys->GetVelocity( &vecVelocity, NULL );
	return vecVelocity;
}

static const Vector &GetPhysAngVelocity( HSCRIPT hPhys )
{
	IPhysicsObject *pPhys = HScriptToClass<IPhysicsObject>( hPhys );
	if (!pPhys)
		return vec3_origin;

	static Vector vecAngVelocity;
	pPhys->GetVelocity( NULL, &vecAngVelocity );
	return vecAngVelocity;
}

static void SetPhysVelocity( HSCRIPT hPhys, const Vector& vecVelocity, const Vector& vecAngVelocity )
{
	IPhysicsObject *pPhys = HScriptToClass<IPhysicsObject>( hPhys );
	if (!pPhys)
		return;

	pPhys->SetVelocity( &vecVelocity, &vecAngVelocity );
}

static void AddPhysVelocity( HSCRIPT hPhys, const Vector& vecVelocity, const Vector& vecAngVelocity )
{
	IPhysicsObject *pPhys = HScriptToClass<IPhysicsObject>( hPhys );
	if (!pPhys)
		return;

	pPhys->AddVelocity( &vecVelocity, &vecAngVelocity );
}

//=============================================================================
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
	Msg("--- Script game event listener dump start\n");
	FOR_EACH_VEC( s_GameEventListeners, i )
	{
		Msg(" %d   (0x%p) %d : %s\n", i,s_GameEventListeners[i],
										s_GameEventListeners[i],
										s_GameEventListeners[i]->m_pszContext ? s_GameEventListeners[i]->m_pszContext : "");
	}
	Msg("--- Script game event listener dump end\n");
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
		// DevMsg( "CScriptGameEventListener::LoadEventsFromFile: Failed to load file %s, %s\n", filename, pathID );
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

	DevMsg( 2, "CScriptGameEventListener::LoadEventsFromFile: Loaded %s, %s\n", filename, pathID );

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
//=============================================================================

static int ScriptPrecacheModel( const char *modelname )
{
	return CBaseEntity::PrecacheModel( modelname );
}

static void ScriptPrecacheOther( const char *classname )
{
	UTIL_PrecacheOther( classname );
}

//=============================================================================
//=============================================================================

static void ScriptEntitiesInBox( HSCRIPT hTable, int listMax, const Vector &hullMin, const Vector &hullMax, int iMask )
{
	CBaseEntity *list[1024];
	int count = UTIL_EntitiesInBox( list, listMax, hullMin, hullMax, iMask );

	for ( int i = 0; i < count; i++ )
	{
		g_pScriptVM->ArrayAppend( hTable, ToHScript(list[i]) );
	}
}

static void ScriptEntitiesAtPoint( HSCRIPT hTable, int listMax, const Vector &point, int iMask )
{
	CBaseEntity *list[1024];
	int count = UTIL_EntitiesAtPoint( list, listMax, point, iMask );

	for ( int i = 0; i < count; i++ )
	{
		g_pScriptVM->ArrayAppend( hTable, ToHScript(list[i]) );
	}
}

static void ScriptEntitiesInSphere( HSCRIPT hTable, int listMax, const Vector &center, float radius, int iMask )
{
	CBaseEntity *list[1024];
	int count = UTIL_EntitiesInSphere( list, listMax, center, radius, iMask );

	for ( int i = 0; i < count; i++ )
	{
		g_pScriptVM->ArrayAppend( hTable, ToHScript(list[i]) );
	}
}

//-----------------------------------------------------------------------------

static void ScriptDecalTrace( HSCRIPT hTrace, const char *decalName )
{
	CTraceInfoAccessor *traceInfo = HScriptToClass<CTraceInfoAccessor>(hTrace);
	UTIL_DecalTrace( &traceInfo->GetTrace(), decalName );
}

//-----------------------------------------------------------------------------
// Simple particle effect dispatch
//-----------------------------------------------------------------------------
static void ScriptDispatchParticleEffect( const char *pszParticleName, const Vector &vecOrigin, const QAngle &vecAngles )
{
	DispatchParticleEffect( pszParticleName, vecOrigin, vecAngles );
}

//=============================================================================
//=============================================================================

bool ScriptMatcherMatch( const char *pszQuery, const char *szValue ) { return Matcher_Match( pszQuery, szValue ); }

//=============================================================================
//=============================================================================

#ifndef CLIENT_DLL
bool IsDedicatedServer()
{
	return engine->IsDedicatedServer();
}
#endif

bool ScriptIsServer()
{
#ifdef GAME_DLL
	return true;
#else
	return false;
#endif
}

bool ScriptIsClient()
{
#ifdef CLIENT_DLL
	return true;
#else
	return false;
#endif
}

// Notification printing on the right edge of the screen
void NPrint( int pos, const char* fmt )
{
	engine->Con_NPrintf(pos, fmt);
}

void NXPrint( int pos, int r, int g, int b, bool fixed, float ftime, const char* fmt )
{
	static con_nprint_t *info = new con_nprint_t;

	info->index = pos;
	info->time_to_live = ftime;
	info->color[0] = r / 255.f;
	info->color[1] = g / 255.f;
	info->color[2] = b / 255.f;
	info->fixed_width_font = fixed;

	engine->Con_NXPrintf(info, fmt);

	// delete info;
}

class CGlobalSys
{
public:
	const char* ScriptGetCommandLine()
	{
		return CommandLine()->GetCmdLine();
	}

	bool CommandLineCheck(const char* name)
	{
		return !!CommandLine()->FindParm(name);
	}

	const char* CommandLineCheckStr(const char* name)
	{
		return CommandLine()->ParmValue(name);
	}

	float CommandLineCheckFloat(const char* name)
	{
		return CommandLine()->ParmValue(name, 0);
	}

	int CommandLineCheckInt(const char* name)
	{
		return CommandLine()->ParmValue(name, 0);
	}
} g_ScriptGlobalSys;

BEGIN_SCRIPTDESC_ROOT_NAMED( CGlobalSys, "CGlobalSys", SCRIPT_SINGLETON "GlobalSys" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetCommandLine, "GetCommandLine", "returns the command line" )
	DEFINE_SCRIPTFUNC( CommandLineCheck, "returns true if the command line param was used, otherwise false." )
	DEFINE_SCRIPTFUNC( CommandLineCheckStr, "returns the command line param as a string." )
	DEFINE_SCRIPTFUNC( CommandLineCheckFloat, "returns the command line param as a float." )
	DEFINE_SCRIPTFUNC( CommandLineCheckInt, "returns the command line param as an int." )
END_SCRIPTDESC();


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

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#define SCRIPT_MAX_FILE_READ_SIZE  (16 * 1024)			// 16KB
#define SCRIPT_MAX_FILE_WRITE_SIZE (64 * 1024 * 1024)	// 64MB
#define SCRIPT_RW_PATH_ID "MOD"
#define SCRIPT_RW_FULL_PATH_FMT "export/%s"

class CScriptReadWriteFile : public CAutoGameSystem
{
public:
	static bool ScriptFileWrite( const char *szFile, const char *szInput );
	static const char *ScriptFileRead( const char *szFile );
	//static const char *CRC32_Checksum( const char *szFilename );

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
#undef SCRIPT_MAX_FILE_READ_SIZE
#undef SCRIPT_MAX_FILE_WRITE_SIZE
#undef SCRIPT_RW_PATH_ID
#undef SCRIPT_RW_FULL_PATH_FMT

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
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

//=============================================================================
//=============================================================================

extern void RegisterMathScriptFunctions();

void RegisterSharedScriptFunctions()
{
	// 
	// Due to this being a custom integration of VScript based on the Alien Swarm SDK, we don't have access to
	// some of the code normally available in games like L4D2 or Valve's original VScript DLL.
	// Instead, that code is recreated here, shared between server and client.
	// 

#ifndef CLIENT_DLL
	ScriptRegisterFunction( g_pScriptVM, EmitSoundOn, "Play named sound on an entity." );
	ScriptRegisterFunction( g_pScriptVM, EmitSoundOnClient, "Play named sound only on the client for the specified player." );

	ScriptRegisterFunction( g_pScriptVM, AddThinkToEnt, "This will put a think function onto an entity, or pass null to remove it. This is NOT chained, so be careful." );
	ScriptRegisterFunction( g_pScriptVM, EntIndexToHScript, "Returns the script handle for the given entity index." );
	ScriptRegisterFunction( g_pScriptVM, PrecacheEntityFromTable, "Precache an entity from KeyValues in a table." );
	ScriptRegisterFunction( g_pScriptVM, SpawnEntityFromTable, "Native function for entity spawning." );
#endif // !CLIENT_DLL

	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptSaveRestoreUtil::SaveTable, "SaveTable", "Store a table with primitive values that will persist across level transitions and save loads." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptSaveRestoreUtil::RestoreTable, "RestoreTable", "Retrieves a table from storage. Write into input table." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptSaveRestoreUtil::ClearSavedTable, "ClearSavedTable", "Removes the table with the given context." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptReadWriteFile::ScriptFileWrite, "StringToFile", "Stores the string into the file" );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptReadWriteFile::ScriptFileRead, "FileToString", "Returns the string from the file, null if no file or file is too big." );

	ScriptRegisterFunction( g_pScriptVM, ListenToGameEvent, "Register as a listener for a game event from script." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptGameEventListener::StopListeningToGameEvent, "StopListeningToGameEvent", "Stop the specified event listener." );
	ScriptRegisterFunctionNamed( g_pScriptVM, CScriptGameEventListener::StopListeningToAllGameEvents, "StopListeningToAllGameEvents", "Stop listening to all game events within a specific context." );
	ScriptRegisterFunction( g_pScriptVM, FireGameEvent, "Fire a game event." );
#ifndef CLIENT_DLL
	ScriptRegisterFunction( g_pScriptVM, FireGameEventLocal, "Fire a game event without broadcasting to the client." );
#endif

	g_pScriptVM->RegisterInstance( &g_ScriptConvarLookup, "Convars" );
	g_pScriptVM->RegisterInstance( &g_ScriptNetPropManager, "NetProps" );
	g_pScriptVM->RegisterInstance( &g_ScriptGlobalSys, "GlobalSys" );

	//-----------------------------------------------------------------------------
	// Functions, singletons, etc. unique to Mapbase
	//-----------------------------------------------------------------------------

	g_pScriptVM->RegisterInstance( GetAmmoDef(), "AmmoDef" );
	g_pScriptVM->RegisterInstance( &g_ScriptLocalize, "Localize" );
#ifndef CLIENT_DLL
	g_pScriptVM->RegisterInstance( &g_ScriptNetMsg, "NetMsg" );
#endif

	//-----------------------------------------------------------------------------

	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptMsg, "Msg", "" );
	ScriptRegisterFunction( g_pScriptVM, NPrint, "Notification print" );
	ScriptRegisterFunction( g_pScriptVM, NXPrint, "Notification print, customised" );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptColorPrint, "printc", "Version of print() which takes a color before the message." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptColorPrintL, "printcl", "Version of printl() which takes a color before the message." );

#ifndef CLIENT_DLL
	ScriptRegisterFunction( g_pScriptVM, SpawnEntityFromKeyValues, "Spawns an entity with the keyvalues in a CScriptKeyValues handle." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptDispatchSpawn, "DispatchSpawn", "Spawns an unspawned entity." );
#endif

	ScriptRegisterFunction( g_pScriptVM, CreateDamageInfo, "Creates damage info." );
	ScriptRegisterFunction( g_pScriptVM, DestroyDamageInfo, "Destroys damage info." );
	ScriptRegisterFunction( g_pScriptVM, ImpulseScale, "Returns an impulse scale required to push an object." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptCalculateExplosiveDamageForce, "CalculateExplosiveDamageForce", "Fill out a damage info handle with a damage force for an explosive." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptCalculateBulletDamageForce, "CalculateBulletDamageForce", "Fill out a damage info handle with a damage force for a bullet impact." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptCalculateMeleeDamageForce, "CalculateMeleeDamageForce", "Fill out a damage info handle with a damage force for a melee impact." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptGuessDamageForce, "GuessDamageForce", "Try and guess the physics force to use." );

	ScriptRegisterFunction( g_pScriptVM, CreateFireBulletsInfo, "Creates FireBullets info." );
	ScriptRegisterFunction( g_pScriptVM, DestroyFireBulletsInfo, "Destroys FireBullets info." );

	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptTraceLineComplex, "TraceLineComplex", "Complex version of TraceLine which takes 2 points, an ent to ignore, a trace mask, and a collision group. Returns a handle which can access all trace info." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptTraceHullComplex, "TraceHullComplex", "Takes 2 points, min/max hull bounds, an ent to ignore, a trace mask, and a collision group to trace to a point using a hull. Returns a handle which can access all trace info." );

	// 
	// VPhysics
	// 
	ScriptRegisterFunction( g_pScriptVM, GetPhysVelocity, "Gets physics velocity for the given VPhysics object" );
	ScriptRegisterFunction( g_pScriptVM, GetPhysAngVelocity, "Gets physics angular velocity for the given VPhysics object" );
	ScriptRegisterFunction( g_pScriptVM, SetPhysVelocity, "Sets physics velocity for the given VPhysics object" );
	ScriptRegisterFunction( g_pScriptVM, AddPhysVelocity, "Adds physics velocity for the given VPhysics object" );

	// 
	// Precaching
	// 
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptPrecacheModel, "PrecacheModel", "Precaches a model for later usage." );
	ScriptRegisterFunction( g_pScriptVM, PrecacheMaterial, "Precaches a material for later usage." );
	ScriptRegisterFunction( g_pScriptVM, PrecacheParticleSystem, "Precaches a particle system for later usage." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptPrecacheOther, "PrecacheOther", "Precaches an entity class for later usage." );

	// 
	// Misc. Utility
	// 
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptEntitiesInBox, "EntitiesInBox", "Gets all entities which are within a worldspace box." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptEntitiesAtPoint, "EntitiesAtPoint", "Gets all entities which are intersecting a point in space." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptEntitiesInSphere, "EntitiesInSphere", "Gets all entities which are within a sphere." );

	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptDecalTrace, "DecalTrace", "Creates a dynamic decal based on the given trace info. The trace information can be generated by TraceLineComplex() and the decal name must be from decals_subrect.txt." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptDispatchParticleEffect, "DispatchParticleEffect", "Dispatches a one-off particle system" );

	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptMatcherMatch, "Matcher_Match", "Compares a string to a query using Mapbase's matcher system, supporting wildcards, RS matchers, etc." );
	ScriptRegisterFunction( g_pScriptVM, Matcher_NamesMatch, "Compares a string to a query using Mapbase's matcher system using wildcards only." );
	ScriptRegisterFunction( g_pScriptVM, AppearsToBeANumber, "Checks if the given string appears to be a number." );

#ifndef CLIENT_DLL
	ScriptRegisterFunction( g_pScriptVM, IsDedicatedServer, "Is this a dedicated server?" );
#endif
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptIsServer, "IsServer", "Returns true if the script is being run on the server." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptIsClient, "IsClient", "Returns true if the script is being run on the client." );

	ScriptRegisterFunction( g_pScriptVM, GetCPUUsage, "Get CPU usage percentage." );

	RegisterMathScriptFunctions();

#ifndef CLIENT_DLL
	CScriptGameEventListener::LoadAllEvents();
#endif // !CLIENT_DLL

#ifdef CLIENT_DLL
	VScriptRunScript( "vscript_client", true );
#else
	VScriptRunScript( "vscript_server", true );
#endif
}
