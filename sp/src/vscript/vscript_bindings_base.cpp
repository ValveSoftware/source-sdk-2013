//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: VScript functions, constants, etc. registered within the library itself.
// 
//			This is for things which don't have to depend on server/client and can be accessed
//			from anywhere.
//
// $NoKeywords: $
//=============================================================================//

#include "vscript/ivscript.h"

#include "tier1/tier1.h"
#include "tier1/fmtstr.h"

#include <tier0/platform.h>
#include "icommandline.h"
#include "worldsize.h"
#include "bspflags.h"

#include <vstdlib/random.h>

#include "vscript_bindings_base.h"
#include "vscript_bindings_math.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IScriptManager *scriptmanager;

//=============================================================================
//
// Prints
// 
//=============================================================================
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

//=============================================================================
//
// Convar Lookup
// 
//=============================================================================
class CScriptConvarLookup
{
public:

	float GetFloat( const char *pszConVar )
	{
		ConVarRef cvar( pszConVar );
		return cvar.GetFloat();
	}

	int GetInt( const char *pszConVar )
	{
		ConVarRef cvar( pszConVar );
		return cvar.GetInt();
	}

	bool GetBool( const char *pszConVar )
	{
		ConVarRef cvar( pszConVar );
		return cvar.GetBool();
	}

	const char *GetStr( const char *pszConVar )
	{
		ConVarRef cvar( pszConVar );
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
		if (!cvar.IsValid())
			return;

		// FCVAR_NOT_CONNECTED can be used to protect specific convars from nefarious interference
		if (cvar.IsFlagSet(FCVAR_NOT_CONNECTED))
			return;

		cvar.SetValue( value );
	}

private:
} g_ScriptConvarLookup;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptConvarLookup, "CConvars", SCRIPT_SINGLETON "Provides an interface for getting and setting convars." )
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
//
// Command Line
// 
//=============================================================================
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

// ----------------------------------------------------------------------------
// KeyValues access - CBaseEntity::ScriptGetKeyFromModel returns root KeyValues
// ----------------------------------------------------------------------------
BEGIN_SCRIPTDESC_ROOT( CScriptKeyValues, "Wrapper class over KeyValues instance" )
	DEFINE_SCRIPT_CONSTRUCTOR()	
	DEFINE_SCRIPTFUNC_NAMED( ScriptFindKey, "FindKey", "Given a KeyValues object and a key name, find a KeyValues object associated with the key name" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetFirstSubKey, "GetFirstSubKey", "Given a KeyValues object, return the first sub key object" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetNextKey, "GetNextKey", "Given a KeyValues object, return the next key object in a sub key group" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetKeyValueInt, "GetKeyInt", "Given a KeyValues object and a key name, return associated integer value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetKeyValueFloat, "GetKeyFloat", "Given a KeyValues object and a key name, return associated float value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetKeyValueBool, "GetKeyBool", "Given a KeyValues object and a key name, return associated bool value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetKeyValueString, "GetKeyString", "Given a KeyValues object and a key name, return associated string value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsKeyValueEmpty, "IsKeyEmpty", "Given a KeyValues object and a key name, return true if key name has no value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptReleaseKeyValues, "ReleaseKeyValues", "Given a root KeyValues object, release its contents" );

	DEFINE_SCRIPTFUNC( TableToSubKeys, "Converts a script table to KeyValues." );

	DEFINE_SCRIPTFUNC_NAMED( ScriptFindOrCreateKey, "FindOrCreateKey", "Given a KeyValues object and a key name, find or create a KeyValues object associated with the key name" );

	DEFINE_SCRIPTFUNC_NAMED( ScriptGetName, "GetName", "Given a KeyValues object, return its name" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetInt, "GetInt", "Given a KeyValues object, return its own associated integer value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetFloat, "GetFloat", "Given a KeyValues object, return its own associated float value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetString, "GetString", "Given a KeyValues object, return its own associated string value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetBool, "GetBool", "Given a KeyValues object, return its own associated bool value" );

	DEFINE_SCRIPTFUNC_NAMED( ScriptSetKeyValueInt, "SetKeyInt", "Given a KeyValues object and a key name, set associated integer value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetKeyValueFloat, "SetKeyFloat", "Given a KeyValues object and a key name, set associated float value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetKeyValueBool, "SetKeyBool", "Given a KeyValues object and a key name, set associated bool value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetKeyValueString, "SetKeyString", "Given a KeyValues object and a key name, set associated string value" );

	DEFINE_SCRIPTFUNC_NAMED( ScriptSetName, "SetName", "Given a KeyValues object, set its name" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetInt, "SetInt", "Given a KeyValues object, set its own associated integer value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetFloat, "SetFloat", "Given a KeyValues object, set its own associated float value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetBool, "SetBool", "Given a KeyValues object, set its own associated bool value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetString, "SetString", "Given a KeyValues object, set its own associated string value" );
END_SCRIPTDESC();

HSCRIPT CScriptKeyValues::ScriptFindKey( const char *pszName )
{
	KeyValues *pKeyValues = m_pKeyValues->FindKey(pszName);
	if ( pKeyValues == NULL )
		return NULL;

	CScriptKeyValues *pScriptKey = new CScriptKeyValues( pKeyValues );

	// UNDONE: who calls ReleaseInstance on this??
	HSCRIPT hScriptInstance = g_pScriptVM->RegisterInstance( pScriptKey );
	return hScriptInstance;
}

HSCRIPT CScriptKeyValues::ScriptGetFirstSubKey( void )
{
	KeyValues *pKeyValues = m_pKeyValues->GetFirstSubKey();
	if ( pKeyValues == NULL )
		return NULL;

	CScriptKeyValues *pScriptKey = new CScriptKeyValues( pKeyValues );

	// UNDONE: who calls ReleaseInstance on this??
	HSCRIPT hScriptInstance = g_pScriptVM->RegisterInstance( pScriptKey );
	return hScriptInstance;
}

HSCRIPT CScriptKeyValues::ScriptGetNextKey( void )
{
	KeyValues *pKeyValues = m_pKeyValues->GetNextKey();
	if ( pKeyValues == NULL )
		return NULL;

	CScriptKeyValues *pScriptKey = new CScriptKeyValues( pKeyValues );

	// UNDONE: who calls ReleaseInstance on this??
	HSCRIPT hScriptInstance = g_pScriptVM->RegisterInstance( pScriptKey );
	return hScriptInstance;
}

int CScriptKeyValues::ScriptGetKeyValueInt( const char *pszName )
{
	int i = m_pKeyValues->GetInt( pszName );
	return i;
}

float CScriptKeyValues::ScriptGetKeyValueFloat( const char *pszName )
{
	float f = m_pKeyValues->GetFloat( pszName );
	return f;
}

const char *CScriptKeyValues::ScriptGetKeyValueString( const char *pszName )
{
	const char *psz = m_pKeyValues->GetString( pszName );
	return psz;
}

bool CScriptKeyValues::ScriptIsKeyValueEmpty( const char *pszName )
{
	bool b = m_pKeyValues->IsEmpty( pszName );
	return b;
}

bool CScriptKeyValues::ScriptGetKeyValueBool( const char *pszName )
{
	bool b = m_pKeyValues->GetBool( pszName );
	return b;
}

void CScriptKeyValues::ScriptReleaseKeyValues( )
{
	m_pKeyValues->deleteThis();
	m_pKeyValues = NULL;
}

void CScriptKeyValues::TableToSubKeys( HSCRIPT hTable )
{
	int nIterator = -1;
	ScriptVariant_t varKey, varValue;
	while ((nIterator = g_pScriptVM->GetKeyValue( hTable, nIterator, &varKey, &varValue )) != -1)
	{
		switch (varValue.m_type)
		{
			case FIELD_CSTRING:		m_pKeyValues->SetString( varKey.m_pszString, varValue.m_pszString ); break;
			case FIELD_INTEGER:		m_pKeyValues->SetInt( varKey.m_pszString, varValue.m_int ); break;
			case FIELD_FLOAT:		m_pKeyValues->SetFloat( varKey.m_pszString, varValue.m_float ); break;
			case FIELD_BOOLEAN:		m_pKeyValues->SetBool( varKey.m_pszString, varValue.m_bool ); break;
			case FIELD_VECTOR:		m_pKeyValues->SetString( varKey.m_pszString, CFmtStr( "%f %f %f", varValue.m_pVector->x, varValue.m_pVector->y, varValue.m_pVector->z ) ); break;
		}

		g_pScriptVM->ReleaseValue( varKey );
		g_pScriptVM->ReleaseValue( varValue );
	}
}

HSCRIPT CScriptKeyValues::ScriptFindOrCreateKey( const char *pszName )
{
	KeyValues *pKeyValues = m_pKeyValues->FindKey(pszName, true);
	if ( pKeyValues == NULL )
		return NULL;

	CScriptKeyValues *pScriptKey = new CScriptKeyValues( pKeyValues );

	// UNDONE: who calls ReleaseInstance on this??
	HSCRIPT hScriptInstance = g_pScriptVM->RegisterInstance( pScriptKey );
	return hScriptInstance;
}

const char *CScriptKeyValues::ScriptGetName()
{
	const char *psz = m_pKeyValues->GetName();
	return psz;
}

int CScriptKeyValues::ScriptGetInt()
{
	int i = m_pKeyValues->GetInt();
	return i;
}

float CScriptKeyValues::ScriptGetFloat()
{
	float f = m_pKeyValues->GetFloat();
	return f;
}

const char *CScriptKeyValues::ScriptGetString()
{
	const char *psz = m_pKeyValues->GetString();
	return psz;
}

bool CScriptKeyValues::ScriptGetBool()
{
	bool b = m_pKeyValues->GetBool();
	return b;
}


void CScriptKeyValues::ScriptSetKeyValueInt( const char *pszName, int iValue )
{
	m_pKeyValues->SetInt( pszName, iValue );
}

void CScriptKeyValues::ScriptSetKeyValueFloat( const char *pszName, float flValue )
{
	m_pKeyValues->SetFloat( pszName, flValue );
}

void CScriptKeyValues::ScriptSetKeyValueString( const char *pszName, const char *pszValue )
{
	m_pKeyValues->SetString( pszName, pszValue );
}

void CScriptKeyValues::ScriptSetKeyValueBool( const char *pszName, bool bValue )
{
	m_pKeyValues->SetBool( pszName, bValue );
}

void CScriptKeyValues::ScriptSetName( const char *pszValue )
{
	m_pKeyValues->SetName( pszValue );
}

void CScriptKeyValues::ScriptSetInt( int iValue )
{
	m_pKeyValues->SetInt( NULL, iValue );
}

void CScriptKeyValues::ScriptSetFloat( float flValue )
{
	m_pKeyValues->SetFloat( NULL, flValue );
}

void CScriptKeyValues::ScriptSetString( const char *pszValue )
{
	m_pKeyValues->SetString( NULL, pszValue );
}

void CScriptKeyValues::ScriptSetBool( bool bValue )
{
	m_pKeyValues->SetBool( NULL, bValue );
}


// constructors
CScriptKeyValues::CScriptKeyValues( KeyValues *pKeyValues = NULL )
{
	if (pKeyValues == NULL)
	{
		m_pKeyValues = new KeyValues("CScriptKeyValues");
	}
	else
	{
		m_pKeyValues = pKeyValues;
	}
}

// destructor
CScriptKeyValues::~CScriptKeyValues( )
{
	if (m_pKeyValues)
	{
		m_pKeyValues->deleteThis();
	}
	m_pKeyValues = NULL;
}

//=============================================================================
//
// matrix3x4_t
// 
//=============================================================================
CScriptColorInstanceHelper g_ColorScriptInstanceHelper;

BEGIN_SCRIPTDESC_ROOT( Color, "" )

	DEFINE_SCRIPT_CONSTRUCTOR()
	DEFINE_SCRIPT_INSTANCE_HELPER( &g_ColorScriptInstanceHelper )

	DEFINE_SCRIPTFUNC( SetColor, "Sets the color." )

	DEFINE_SCRIPTFUNC( SetRawColor, "Sets the raw color integer." )
	DEFINE_SCRIPTFUNC( GetRawColor, "Gets the raw color integer." )

	DEFINE_MEMBERVAR( "r", FIELD_CHARACTER, "Member variable for red." )
	DEFINE_MEMBERVAR( "g", FIELD_CHARACTER, "Member variable for green." )
	DEFINE_MEMBERVAR( "b", FIELD_CHARACTER, "Member variable for blue." )
	DEFINE_MEMBERVAR( "a", FIELD_CHARACTER, "Member variable for alpha. (transparency)" )

END_SCRIPTDESC();

//-----------------------------------------------------------------------------

bool CScriptColorInstanceHelper::ToString( void *p, char *pBuf, int bufSize )
{
	Color *pClr = ((Color *)p);
	V_snprintf( pBuf, bufSize, "(color: (%i, %i, %i, %i))", pClr->r(), pClr->g(), pClr->b(), pClr->a() );
	return true; 
}

bool CScriptColorInstanceHelper::Get( void *p, const char *pszKey, ScriptVariant_t &variant )
{
	Color *pClr = ((Color *)p);
	if ( strlen(pszKey) == 1 )
	{
		switch (pszKey[0])
		{
			case 'r':
				variant = pClr->r();
				return true;
			case 'g':
				variant = pClr->g();
				return true;
			case 'b':
				variant = pClr->b();
				return true;
			case 'a':
				variant = pClr->a();
				return true;
		}
	}
	return false;
}

bool CScriptColorInstanceHelper::Set( void *p, const char *pszKey, ScriptVariant_t &variant )
{
	Color *pClr = ((Color *)p);
	if ( strlen(pszKey) == 1 )
	{
		int iVal;
		variant.AssignTo( &iVal );
		switch (pszKey[0])
		{
			// variant.AssignTo( &(*pClr)[0] );
			case 'r':
				(*pClr)[0] = iVal;
				return true;
			case 'g':
				(*pClr)[1] = iVal;
				return true;
			case 'b':
				(*pClr)[2] = iVal;
				return true;
			case 'a':
				(*pClr)[3] = iVal;
				return true;
		}
	}
	return false;
}

//=============================================================================
//=============================================================================

void RegisterBaseBindings( IScriptVM *pVM )
{
	ScriptRegisterFunctionNamed( pVM, ScriptMsg, "Msg", "" );
	ScriptRegisterFunctionNamed( pVM, ScriptColorPrint, "printc", "Version of print() which takes a color before the message." );
	ScriptRegisterFunctionNamed( pVM, ScriptColorPrintL, "printcl", "Version of printl() which takes a color before the message." );

	ScriptRegisterFunction( pVM, GetCPUUsage, "Get CPU usage percentage." );

	//-----------------------------------------------------------------------------

	pVM->RegisterInstance( &g_ScriptConvarLookup, "Convars" );
	pVM->RegisterInstance( &g_ScriptGlobalSys, "GlobalSys" );

	//-----------------------------------------------------------------------------

	pVM->RegisterClass( GetScriptDescForClass( CScriptKeyValues ) );

	pVM->RegisterClass( GetScriptDescForClass( Color ) );

	//-----------------------------------------------------------------------------

	//
	// Math/world
	//
	ScriptRegisterConstant( pVM, MAX_COORD_FLOAT, "Maximum float coordinate." );
	ScriptRegisterConstant( pVM, MAX_TRACE_LENGTH, "Maximum traceable distance (assumes cubic world and trace from one corner to opposite)." );

	// 
	// Trace Contents/Masks
	// 
	ScriptRegisterConstant( pVM, CONTENTS_EMPTY, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_SOLID, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_WINDOW, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_AUX, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_GRATE, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_SLIME, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_WATER, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_BLOCKLOS, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_OPAQUE, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_TESTFOGVOLUME, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_TEAM1, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_TEAM2, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_IGNORE_NODRAW_OPAQUE, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_MOVEABLE, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_AREAPORTAL, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_PLAYERCLIP, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_MONSTERCLIP, "Spatial content flags." );

	ScriptRegisterConstant( pVM, CONTENTS_CURRENT_0, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_CURRENT_90, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_CURRENT_180, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_CURRENT_270, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_CURRENT_UP, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_CURRENT_DOWN, "Spatial content flags." );

	ScriptRegisterConstant( pVM, CONTENTS_ORIGIN, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_MONSTER, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_DEBRIS, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_DETAIL, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_TRANSLUCENT, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_LADDER, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_HITBOX, "Spatial content flags." );

	ScriptRegisterConstant( pVM, LAST_VISIBLE_CONTENTS, "Contains last visible spatial content flags." );
	ScriptRegisterConstant( pVM, ALL_VISIBLE_CONTENTS, "Contains all visible spatial content flags." );

	ScriptRegisterConstant( pVM, MASK_SOLID, "Spatial content mask representing solid objects (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)" );
	ScriptRegisterConstant( pVM, MASK_PLAYERSOLID, "Spatial content mask representing objects solid to the player, including player clips (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)" );
	ScriptRegisterConstant( pVM, MASK_NPCSOLID, "Spatial content mask representing objects solid to NPCs, including NPC clips (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)" );
	ScriptRegisterConstant( pVM, MASK_WATER, "Spatial content mask representing water and slime solids (CONTENTS_WATER|CONTENTS_MOVEABLE|CONTENTS_SLIME)" );
	ScriptRegisterConstant( pVM, MASK_OPAQUE, "Spatial content mask representing objects which block lighting (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_OPAQUE)" );
	ScriptRegisterConstant( pVM, MASK_OPAQUE_AND_NPCS, "Spatial content mask equivalent to MASK_OPAQUE, but also including NPCs (MASK_OPAQUE|CONTENTS_MONSTER)" );
	ScriptRegisterConstant( pVM, MASK_BLOCKLOS, "Spatial content mask representing objects which block LOS for AI (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_BLOCKLOS)" );
	ScriptRegisterConstant( pVM, MASK_BLOCKLOS_AND_NPCS, "Spatial content mask equivalent to MASK_BLOCKLOS, but also including NPCs (MASK_BLOCKLOS|CONTENTS_MONSTER)" );
	ScriptRegisterConstant( pVM, MASK_VISIBLE, "Spatial content mask representing objects which block LOS for players (MASK_OPAQUE|CONTENTS_IGNORE_NODRAW_OPAQUE)" );
	ScriptRegisterConstant( pVM, MASK_VISIBLE_AND_NPCS, "Spatial content mask equivalent to MASK_VISIBLE, but also including NPCs (MASK_OPAQUE_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE)" );
	ScriptRegisterConstant( pVM, MASK_SHOT, "Spatial content mask representing objects solid to bullets (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_HITBOX)" );
	ScriptRegisterConstant( pVM, MASK_SHOT_HULL, "Spatial content mask representing objects solid to non-raycasted weapons, including grates (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_GRATE)" );
	ScriptRegisterConstant( pVM, MASK_SHOT_PORTAL, "Spatial content mask equivalent to MASK_SHOT, but excluding debris and not using expensive hitbox calculations (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER)" );
	ScriptRegisterConstant( pVM, MASK_SOLID_BRUSHONLY, "Spatial content mask equivalent to MASK_SOLID, but without NPCs (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_GRATE)" );
	ScriptRegisterConstant( pVM, MASK_PLAYERSOLID_BRUSHONLY, "Spatial content mask equivalent to MASK_PLAYERSOLID, but without NPCs (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_PLAYERCLIP|CONTENTS_GRATE)" );
	ScriptRegisterConstant( pVM, MASK_NPCSOLID_BRUSHONLY, "Spatial content mask equivalent to MASK_NPCSOLID, but without NPCs (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE)" );
	ScriptRegisterConstant( pVM, MASK_NPCWORLDSTATIC, "Spatial content mask representing objects static to NPCs, used for nodegraph rebuilding (CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE)" );
	ScriptRegisterConstant( pVM, MASK_SPLITAREAPORTAL, "Spatial content mask representing objects which can split areaportals (CONTENTS_WATER|CONTENTS_SLIME)" );

	// 
	// Misc. General
	// 
	ScriptRegisterConstant( pVM, FCVAR_NONE, "Empty convar flag." );
	ScriptRegisterConstant( pVM, FCVAR_UNREGISTERED, "If this convar flag is set, it isn't added to linked list, etc." );
	ScriptRegisterConstant( pVM, FCVAR_DEVELOPMENTONLY, "If this convar flag is set, it's hidden in \"retail\" DLLs." );
	ScriptRegisterConstant( pVM, FCVAR_GAMEDLL, "This convar flag is defined in server DLL convars." );
	ScriptRegisterConstant( pVM, FCVAR_CLIENTDLL, "This convar flag is defined in client DLL convars." );
	ScriptRegisterConstant( pVM, FCVAR_HIDDEN, "If this convar flag is set, it doesn't appear in the console or any searching tools, but it can still be set." );
	ScriptRegisterConstant( pVM, FCVAR_PROTECTED, "This convar flag prevents convars with secure data (e.g. passwords) from sending full data to clients, only sending 1 if non-zero and 0 otherwise." );
	ScriptRegisterConstant( pVM, FCVAR_SPONLY, "If this convar flag is set, it can't be changed by clients connected to a multiplayer server." );
	ScriptRegisterConstant( pVM, FCVAR_ARCHIVE, "If this convar flag is set, its value will be saved when the game is exited." );
	ScriptRegisterConstant( pVM, FCVAR_NOTIFY, "If this convar flag is set, it will notify players when it is changed." );
	ScriptRegisterConstant( pVM, FCVAR_USERINFO, "If this convar flag is set, it will be marked as info which plays a part in how the server identifies a client." );
	ScriptRegisterConstant( pVM, FCVAR_PRINTABLEONLY, "If this convar flag is set, it cannot contain unprintable characters. Used for player name cvars, etc." );
	ScriptRegisterConstant( pVM, FCVAR_UNLOGGED, "If this convar flag is set, it will not log its changes if a log is being created." );
	ScriptRegisterConstant( pVM, FCVAR_NEVER_AS_STRING, "If this convar flag is set, it will never be printed as a string." );
	ScriptRegisterConstant( pVM, FCVAR_REPLICATED, "If this convar flag is set, it will enforce a serverside value on any clientside counterparts. (also known as FCAR_SERVER)" );
	ScriptRegisterConstant( pVM, FCVAR_DEMO, "If this convar flag is set, it will be recorded when starting a demo file." );
	ScriptRegisterConstant( pVM, FCVAR_DONTRECORD, "If this convar flag is set, it will NOT be recorded when starting a demo file." );
	ScriptRegisterConstant( pVM, FCVAR_RELOAD_MATERIALS, "If this convar flag is set, it will force a material reload when it changes." );
	ScriptRegisterConstant( pVM, FCVAR_RELOAD_TEXTURES, "If this convar flag is set, it will force a texture reload when it changes." );
	ScriptRegisterConstant( pVM, FCVAR_NOT_CONNECTED, "If this convar flag is set, it cannot be changed by a client connected to the server." );
	ScriptRegisterConstant( pVM, FCVAR_MATERIAL_SYSTEM_THREAD, "This convar flag indicates it's read from the material system thread." );
	ScriptRegisterConstant( pVM, FCVAR_ARCHIVE_XBOX, "If this convar flag is set, it will be archived on the Xbox config." );
	ScriptRegisterConstant( pVM, FCVAR_ACCESSIBLE_FROM_THREADS, "If this convar flag is set, it will be accessible from the material system thread." );
	ScriptRegisterConstant( pVM, FCVAR_SERVER_CAN_EXECUTE, "If this convar flag is set, the server will be allowed to execute it as a client command." );
	ScriptRegisterConstant( pVM, FCVAR_SERVER_CANNOT_QUERY, "If this convar flag is set, the server will not be allowed to query its value." );
	ScriptRegisterConstant( pVM, FCVAR_CLIENTCMD_CAN_EXECUTE, "If this convar flag is set, any client will be allowed to execute this command." );

	//-----------------------------------------------------------------------------

	RegisterMathBaseBindings( pVM );
}
