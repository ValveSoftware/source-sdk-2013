//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: VScript-driven custom weapon class.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tier1/fmtstr.h"
#include "tier1/utlvector.h"
#include "weapon_custom_scripted.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
//=========================================================

BEGIN_DATADESC( CWeaponCustomScripted )

	DEFINE_AUTO_ARRAY( m_iszClientScripts, FIELD_CHARACTER ),
	DEFINE_AUTO_ARRAY( m_iszWeaponScriptName, FIELD_CHARACTER ),

END_DATADESC()

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponCustomScripted, DT_WeaponCustomScripted )

BEGIN_NETWORK_TABLE( CWeaponCustomScripted, DT_WeaponCustomScripted )

#ifdef CLIENT_DLL
	RecvPropString( RECVINFO(m_iszClientScripts) ),
	RecvPropString( RECVINFO(m_iszWeaponScriptName) ),
#else
	SendPropString( SENDINFO(m_iszClientScripts) ),
	SendPropString( SENDINFO(m_iszWeaponScriptName) ),
#endif

END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponCustomScripted )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_custom_scripted1, CWeaponCustomScripted );

// Only need one of the names
PRECACHE_WEAPON_REGISTER( weapon_custom_scripted1 );

//IMPLEMENT_ACTTABLE( CWeaponCustomScripted );

#define DEFINE_STATIC_HOOK( name ) ScriptHook_t	CWeaponCustomScripted::g_Hook_##name

DEFINE_STATIC_HOOK( HasAnyAmmo );
DEFINE_STATIC_HOOK( HasPrimaryAmmo );
DEFINE_STATIC_HOOK( HasSecondaryAmmo );

DEFINE_STATIC_HOOK( CanHolster );
DEFINE_STATIC_HOOK( CanDeploy );
DEFINE_STATIC_HOOK( Deploy );
DEFINE_STATIC_HOOK( Holster );

DEFINE_STATIC_HOOK( ItemPreFrame );
DEFINE_STATIC_HOOK( ItemPostFrame );
DEFINE_STATIC_HOOK( ItemBusyFrame );
DEFINE_STATIC_HOOK( ItemHolsterFrame );
DEFINE_STATIC_HOOK( WeaponIdle );
DEFINE_STATIC_HOOK( HandleFireOnEmpty );

DEFINE_STATIC_HOOK( CheckReload );
DEFINE_STATIC_HOOK( FinishReload );
DEFINE_STATIC_HOOK( AbortReload );
DEFINE_STATIC_HOOK( Reload );
DEFINE_STATIC_HOOK( Reload_NPC );

DEFINE_STATIC_HOOK( PrimaryAttack );
DEFINE_STATIC_HOOK( SecondaryAttack );

DEFINE_STATIC_HOOK( GetPrimaryAttackActivity );
DEFINE_STATIC_HOOK( GetSecondaryAttackActivity );
DEFINE_STATIC_HOOK( GetDrawActivity );
DEFINE_STATIC_HOOK( GetDefaultAnimSpeed );

DEFINE_STATIC_HOOK( GetBulletSpread );
DEFINE_STATIC_HOOK( GetBulletSpreadForProficiency );
DEFINE_STATIC_HOOK( GetFireRate );
DEFINE_STATIC_HOOK( GetMinBurst );
DEFINE_STATIC_HOOK( GetMaxBurst );
DEFINE_STATIC_HOOK( GetMinRestTime );
DEFINE_STATIC_HOOK( GetMaxRestTime );

DEFINE_STATIC_HOOK( AddViewKick );

#ifndef CLIENT_DLL
DEFINE_STATIC_HOOK( WeaponLOSCondition );
DEFINE_STATIC_HOOK( WeaponRangeAttack1Condition );
DEFINE_STATIC_HOOK( WeaponRangeAttack2Condition );
DEFINE_STATIC_HOOK( WeaponMeleeAttack1Condition );
DEFINE_STATIC_HOOK( WeaponMeleeAttack2Condition );
#endif

DEFINE_STATIC_HOOK( ActivityList );
DEFINE_STATIC_HOOK( ActivityListCount );

#define DEFINE_SIMPLE_WEAPON_HOOK( name, returnType, description ) DEFINE_SIMPLE_SCRIPTHOOK( CWeaponCustomScripted::g_Hook_##name, #name, returnType, description )
#define BEGIN_WEAPON_HOOK( name, returnType, description ) BEGIN_SCRIPTHOOK( CWeaponCustomScripted::g_Hook_##name, #name, returnType, description )

BEGIN_ENT_SCRIPTDESC( CWeaponCustomScripted, CBaseCombatWeapon, "Special weapon class with tons of hooks" )

	DEFINE_SIMPLE_WEAPON_HOOK( HasAnyAmmo, FIELD_BOOLEAN, "Should return true if weapon has ammo" )
	DEFINE_SIMPLE_WEAPON_HOOK( HasPrimaryAmmo, FIELD_BOOLEAN, "Should return true if weapon has primary ammo" )
	DEFINE_SIMPLE_WEAPON_HOOK( HasSecondaryAmmo, FIELD_BOOLEAN, "Should return true if weapon has secondary ammo" )

	DEFINE_SIMPLE_WEAPON_HOOK( CanHolster, FIELD_BOOLEAN, "Should return true if weapon can be holstered" )
	DEFINE_SIMPLE_WEAPON_HOOK( CanDeploy, FIELD_BOOLEAN, "Should return true if weapon can be deployed" )
	DEFINE_SIMPLE_WEAPON_HOOK( Deploy, FIELD_BOOLEAN, "Called when weapon is being deployed" )
	BEGIN_WEAPON_HOOK( Holster, FIELD_BOOLEAN, "Called when weapon is being holstered" )
		DEFINE_SCRIPTHOOK_PARAM( "switchingto", FIELD_HSCRIPT )
	END_SCRIPTHOOK()

	DEFINE_SIMPLE_WEAPON_HOOK( ItemPreFrame, FIELD_VOID, "Called each frame by the player PreThink" )
	DEFINE_SIMPLE_WEAPON_HOOK( ItemPostFrame, FIELD_VOID, "Called each frame by the player PostThink" )
	DEFINE_SIMPLE_WEAPON_HOOK( ItemBusyFrame, FIELD_VOID, "Called each frame by the player PostThink, if the player's not ready to attack yet" )
	DEFINE_SIMPLE_WEAPON_HOOK( ItemHolsterFrame, FIELD_VOID, "Called each frame by the player PreThink, if the weapon is holstered" )
	DEFINE_SIMPLE_WEAPON_HOOK( WeaponIdle, FIELD_VOID, "Called when no buttons pressed" )
	DEFINE_SIMPLE_WEAPON_HOOK( HandleFireOnEmpty, FIELD_VOID, "Called when they have the attack button down but they are out of ammo. The default implementation either reloads, switches weapons, or plays an empty sound." )

	DEFINE_SIMPLE_WEAPON_HOOK( CheckReload, FIELD_VOID, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( FinishReload, FIELD_VOID, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( AbortReload, FIELD_VOID, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( Reload, FIELD_BOOLEAN, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( Reload_NPC, FIELD_VOID, "" )

	DEFINE_SIMPLE_WEAPON_HOOK( PrimaryAttack, FIELD_VOID, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( SecondaryAttack, FIELD_VOID, "" )

	DEFINE_SIMPLE_WEAPON_HOOK( GetPrimaryAttackActivity, FIELD_VARIANT, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( GetSecondaryAttackActivity, FIELD_VARIANT, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( GetDrawActivity, FIELD_VARIANT, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( GetDefaultAnimSpeed, FIELD_FLOAT, "" )

	DEFINE_SIMPLE_WEAPON_HOOK( GetBulletSpread, FIELD_VECTOR, "" )
	BEGIN_WEAPON_HOOK( GetBulletSpreadForProficiency, FIELD_VECTOR, "Returns the bullet spread of a specific proficiency level. If this isn't defined, it will fall back to GetBulletSpread." )
		DEFINE_SCRIPTHOOK_PARAM( "proficiency", FIELD_INTEGER )
	END_SCRIPTHOOK()
	DEFINE_SIMPLE_WEAPON_HOOK( GetFireRate, FIELD_FLOAT, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( GetMinBurst, FIELD_INTEGER, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( GetMaxBurst, FIELD_INTEGER, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( GetMinRestTime, FIELD_FLOAT, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( GetMaxRestTime, FIELD_FLOAT, "" )

	DEFINE_SIMPLE_WEAPON_HOOK( AddViewKick, FIELD_VOID, "" )

#ifndef CLIENT_DLL
	DEFINE_SIMPLE_WEAPON_HOOK( WeaponLOSCondition, FIELD_BOOLEAN, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( WeaponRangeAttack1Condition, FIELD_INTEGER, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( WeaponRangeAttack2Condition, FIELD_INTEGER, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( WeaponMeleeAttack1Condition, FIELD_INTEGER, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( WeaponMeleeAttack2Condition, FIELD_INTEGER, "" )
#endif

	DEFINE_SIMPLE_WEAPON_HOOK( ActivityList, FIELD_HSCRIPT, "" )
	DEFINE_SIMPLE_WEAPON_HOOK( ActivityListCount, FIELD_INTEGER, "" )

END_SCRIPTDESC();

CWeaponCustomScripted::CWeaponCustomScripted()
{
	//m_fMinRange1	= 65;
	//m_fMaxRange1	= 2048;
	//
	//m_fMinRange2	= 256;
	//m_fMaxRange2	= 1024;
	//
	//m_nShotsFired	= 0;
	//m_nVentPose		= -1;
	//
	//m_bAltFiresUnderwater = false;
}

bool CWeaponCustomScripted::RunWeaponHook( ScriptHook_t &hook, HSCRIPT &cached, ScriptVariant_t *retVal, ScriptVariant_t *pArgs )
{
	if ( !cached )
	{
		if ( m_ScriptScope.IsInitialized() && hook.CanRunInScope( m_ScriptScope ) )
		{
			cached = hook.m_hFunc;
		}
	}

	if (cached)
	{
		hook.m_hFunc = cached;
		return hook.Call( m_ScriptScope, retVal, pArgs, false );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCustomScripted::Spawn( void )
{
	BaseClass::Spawn();
}

bool CWeaponCustomScripted::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "vscripts_client" ) )
	{
		Q_strcpy( m_iszClientScripts.GetForModify(), szValue );
	}
	else if ( FStrEq( szKeyName, "weapondatascript_name" ) )
	{
		Q_strcpy( m_iszWeaponScriptName.GetForModify(), szValue );
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

bool CWeaponCustomScripted::GetKeyValue( const char *szKeyName, char *szValue, int iMaxLen )
{
	if ( FStrEq( szKeyName, "vscripts_client" ) )
	{
		Q_snprintf( szValue, iMaxLen, "%s", m_iszClientScripts.Get() );
		return true;
	}
	else if ( FStrEq( szKeyName, "weapondatascript_name" ) )
	{
		Q_snprintf( szValue, iMaxLen, "%s", m_iszWeaponScriptName.Get() );
		return true;
	}
	return BaseClass::GetKeyValue( szKeyName, szValue, iMaxLen );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define SIMPLE_VOID_OVERRIDE( name, pArgs ) ScriptVariant_t retVal; \
	if (RunWeaponHook( g_Hook_##name, m_Func_##name, &retVal, pArgs ) && retVal.m_bool == false) \
		return;

#define SIMPLE_BOOL_OVERRIDE( name, pArgs ) ScriptVariant_t retVal; \
	if (RunWeaponHook( g_Hook_##name, m_Func_##name, &retVal, pArgs ) && retVal.m_type == FIELD_BOOLEAN) \
		return retVal.m_bool;

#define SIMPLE_FLOAT_OVERRIDE( name, pArgs ) ScriptVariant_t retVal; \
	if (RunWeaponHook( g_Hook_##name, m_Func_##name, &retVal, pArgs ) && retVal.m_type == FIELD_FLOAT) \
		return retVal.m_float;

#define SIMPLE_INT_OVERRIDE( name, pArgs ) ScriptVariant_t retVal; \
	if (RunWeaponHook( g_Hook_##name, m_Func_##name, &retVal, pArgs ) && retVal.m_type == FIELD_INTEGER) \
		return retVal.m_int;

#define SIMPLE_VECTOR_OVERRIDE( name, pArgs ) ScriptVariant_t retVal; \
	if (RunWeaponHook( g_Hook_##name, m_Func_##name, &retVal, pArgs ) && retVal.m_type == FIELD_VECTOR) \
		return *retVal.m_pVector;

#define SIMPLE_VECTOR_REF_OVERRIDE( name, pArgs ) ScriptVariant_t retVal; \
	if (RunWeaponHook( g_Hook_##name, m_Func_##name, &retVal, pArgs ) && retVal.m_type == FIELD_VECTOR) \
	{ \
		static Vector vec = *retVal.m_pVector; \
		return vec; \
	}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponCustomScripted::HasAnyAmmo( void )
{
	SIMPLE_BOOL_OVERRIDE( HasAnyAmmo, NULL );

	return BaseClass::HasAnyAmmo();
}

bool CWeaponCustomScripted::HasPrimaryAmmo( void )
{
	SIMPLE_BOOL_OVERRIDE( HasPrimaryAmmo, NULL );

	return BaseClass::HasPrimaryAmmo();
}

bool CWeaponCustomScripted::HasSecondaryAmmo( void )
{
	SIMPLE_BOOL_OVERRIDE( HasSecondaryAmmo, NULL );

	return BaseClass::HasSecondaryAmmo();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponCustomScripted::CanHolster( void )
{
	SIMPLE_BOOL_OVERRIDE( CanHolster, NULL );

	return BaseClass::CanHolster();
}

bool CWeaponCustomScripted::CanDeploy( void )
{
	SIMPLE_BOOL_OVERRIDE( CanDeploy, NULL );

	return BaseClass::CanDeploy();
}

bool CWeaponCustomScripted::Deploy( void )
{
	SIMPLE_BOOL_OVERRIDE( Deploy, NULL );

	return BaseClass::Deploy();
}

bool CWeaponCustomScripted::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	ScriptVariant_t pArgs[] = { ToHScript( pSwitchingTo ) };
	SIMPLE_BOOL_OVERRIDE( Holster, pArgs );

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCustomScripted::ItemPreFrame( void )
{
	SIMPLE_VOID_OVERRIDE( ItemPreFrame, NULL );

	BaseClass::ItemPreFrame();
}

void CWeaponCustomScripted::ItemPostFrame( void )
{
	SIMPLE_VOID_OVERRIDE( ItemPostFrame, NULL );

	BaseClass::ItemPostFrame();
}

void CWeaponCustomScripted::ItemBusyFrame( void )
{
	SIMPLE_VOID_OVERRIDE( ItemBusyFrame, NULL );

	BaseClass::ItemBusyFrame();
}

void CWeaponCustomScripted::ItemHolsterFrame( void )
{
	SIMPLE_VOID_OVERRIDE( ItemHolsterFrame, NULL );

	BaseClass::ItemHolsterFrame();
}

void CWeaponCustomScripted::WeaponIdle( void )
{
	SIMPLE_VOID_OVERRIDE( WeaponIdle, NULL );

	BaseClass::WeaponIdle();
}

void CWeaponCustomScripted::HandleFireOnEmpty( void )
{
	SIMPLE_VOID_OVERRIDE( HandleFireOnEmpty, NULL );

	BaseClass::HandleFireOnEmpty();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCustomScripted::CheckReload( void )
{
	SIMPLE_VOID_OVERRIDE( CheckReload, NULL );

	BaseClass::CheckReload();
}

void CWeaponCustomScripted::FinishReload( void )
{
	SIMPLE_VOID_OVERRIDE( FinishReload, NULL );

	BaseClass::FinishReload();
}

void CWeaponCustomScripted::AbortReload( void )
{
	SIMPLE_VOID_OVERRIDE( AbortReload, NULL );

	BaseClass::AbortReload();
}

bool CWeaponCustomScripted::Reload( void )
{
	SIMPLE_BOOL_OVERRIDE( Reload, NULL );

	return BaseClass::Reload();
}

void CWeaponCustomScripted::Reload_NPC( bool bPlaySound )
{
	ScriptVariant_t pArgs[] = { bPlaySound };
	SIMPLE_VOID_OVERRIDE( Reload_NPC, pArgs );

	BaseClass::Reload_NPC();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCustomScripted::PrimaryAttack( void )
{
	SIMPLE_VOID_OVERRIDE( PrimaryAttack, NULL );

	BaseClass::PrimaryAttack();
}

void CWeaponCustomScripted::SecondaryAttack( void )
{
	SIMPLE_VOID_OVERRIDE( SecondaryAttack, NULL );

	BaseClass::SecondaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define ACTIVITY_FUNC_OVERRIDE( name ) ScriptVariant_t retVal; \
	if (RunWeaponHook( g_Hook_##name, m_Func_##name, &retVal ) && !retVal.IsNull()) \
	{ \
		if (retVal.m_type == FIELD_INTEGER) \
		{ \
			Activity activity = (Activity)retVal.m_int; \
			if (activity != ACT_INVALID) \
				return (Activity)retVal.m_int; \
		} \
		else \
		{ \
			Activity activity = (Activity)LookupActivity( retVal.m_pszString ); \
			if (activity != ACT_INVALID) \
				return activity; \
		} \
	}

Activity CWeaponCustomScripted::GetPrimaryAttackActivity( void )
{
	ACTIVITY_FUNC_OVERRIDE( GetPrimaryAttackActivity );

	return BaseClass::GetPrimaryAttackActivity();
}

Activity CWeaponCustomScripted::GetSecondaryAttackActivity( void )
{
	ACTIVITY_FUNC_OVERRIDE( GetSecondaryAttackActivity );

	return BaseClass::GetSecondaryAttackActivity();
}

Activity CWeaponCustomScripted::GetDrawActivity( void )
{
	ACTIVITY_FUNC_OVERRIDE( GetDrawActivity );

	return BaseClass::GetDrawActivity();
}

float CWeaponCustomScripted::GetDefaultAnimSpeed( void )
{
	SIMPLE_FLOAT_OVERRIDE( GetDefaultAnimSpeed, NULL );

	return BaseClass::GetDefaultAnimSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const Vector& CWeaponCustomScripted::GetBulletSpread( void )
{
	SIMPLE_VECTOR_REF_OVERRIDE( GetBulletSpread, NULL );

	// HACKHACK: Need to skip CBaseHLCombatWeapon here to recognize this overload for some reason
	return CBaseCombatWeapon::GetBulletSpread();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CWeaponCustomScripted::GetBulletSpread( WeaponProficiency_t proficiency )
{
	ScriptVariant_t pArgs[] = { (int)proficiency };
	SIMPLE_VECTOR_OVERRIDE( GetBulletSpreadForProficiency, pArgs );

	return BaseClass::GetBulletSpread( proficiency );
}

float CWeaponCustomScripted::GetFireRate( void )
{
	SIMPLE_FLOAT_OVERRIDE( GetFireRate, NULL );

	return BaseClass::GetFireRate();
}

int CWeaponCustomScripted::GetMinBurst( void )
{
	SIMPLE_INT_OVERRIDE( GetMinBurst, NULL );

	return BaseClass::GetMinBurst();
}

int CWeaponCustomScripted::GetMaxBurst( void )
{
	SIMPLE_INT_OVERRIDE( GetMaxBurst, NULL );

	return BaseClass::GetMaxBurst();
}

float CWeaponCustomScripted::GetMinRestTime( void )
{
	SIMPLE_FLOAT_OVERRIDE( GetMinRestTime, NULL );

	return BaseClass::GetMinRestTime();
}

float CWeaponCustomScripted::GetMaxRestTime( void )
{
	SIMPLE_FLOAT_OVERRIDE( GetMaxRestTime, NULL );

	return BaseClass::GetMaxRestTime();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCustomScripted::AddViewKick( void )
{
	SIMPLE_VOID_OVERRIDE( AddViewKick, NULL );

	return BaseClass::AddViewKick();
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponCustomScripted::WeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions )
{
	ScriptVariant_t pArgs[] = { ownerPos, targetPos, bSetConditions };
	SIMPLE_BOOL_OVERRIDE( WeaponLOSCondition, pArgs );

	return BaseClass::WeaponLOSCondition( ownerPos, targetPos, bSetConditions );
}

int CWeaponCustomScripted::WeaponRangeAttack1Condition( float flDot, float flDist )
{
	ScriptVariant_t pArgs[] = { flDot, flDist };
	SIMPLE_INT_OVERRIDE( WeaponRangeAttack1Condition, pArgs );

	return BaseClass::WeaponRangeAttack1Condition( flDot, flDist );
}

int CWeaponCustomScripted::WeaponRangeAttack2Condition( float flDot, float flDist )
{
	ScriptVariant_t pArgs[] = { flDot, flDist };
	SIMPLE_INT_OVERRIDE( WeaponRangeAttack2Condition, pArgs );

	return BaseClass::WeaponRangeAttack2Condition( flDot, flDist );
}

int CWeaponCustomScripted::WeaponMeleeAttack1Condition( float flDot, float flDist )
{
	ScriptVariant_t pArgs[] = { flDot, flDist };
	SIMPLE_INT_OVERRIDE( WeaponMeleeAttack1Condition, pArgs );

	return BaseClass::WeaponMeleeAttack1Condition( flDot, flDist );
}

int CWeaponCustomScripted::WeaponMeleeAttack2Condition( float flDot, float flDist )
{
	ScriptVariant_t pArgs[] = { flDot, flDist };
	SIMPLE_INT_OVERRIDE( WeaponMeleeAttack2Condition, pArgs );

	return BaseClass::WeaponMeleeAttack2Condition( flDot, flDist );
}

struct VScriptWeaponCustomData_s
{
	char cScripts[256];

	bool Parse(KeyValues* pKVWeapon)
	{
		Q_strncpy(cScripts, pKVWeapon->GetString("vscript_file"), 256);
		return true;
	}
};

DEFINE_CUSTOM_WEAPON_FACTORY(vscript, CWeaponCustomScripted, VScriptWeaponCustomData_s);
void CWeaponCustomScripted::InitCustomWeaponFromData(const void* pData, const char* pszWeaponScript)
{
	Q_FileBase(pszWeaponScript, m_iszWeaponScriptName.GetForModify(), 256);
	Q_strncpy(m_iszClientScripts.GetForModify(), static_cast<const VScriptWeaponCustomData_s *> (pData)->cScripts, 256);
}

extern ConVar sv_script_think_interval;
#else
void CWeaponCustomScripted::OnDataChanged(DataUpdateType_t type)
{
	BaseClass::OnDataChanged(type);

	if (!m_ScriptScope.IsInitialized())
	{
		RunVScripts();
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
acttable_t *CWeaponCustomScripted::ActivityList( void )
{
	// TODO

	return BaseClass::ActivityList();
}

int CWeaponCustomScripted::ActivityListCount( void )
{
	// TODO

	return BaseClass::ActivityListCount();
}

void CWeaponCustomScripted::RunVScripts()
{
#ifdef CLIENT_DLL
	if (m_iszClientScripts[0] != '\0' && ValidateScriptScope())
	{
		RunScriptFile(m_iszClientScripts);
	}
#else
	if (m_iszVScripts == NULL_STRING && m_iszClientScripts[0] == '\0')
	{
		return;
	}

#ifdef MAPBASE_VSCRIPT
	if (g_pScriptVM == NULL)
	{
		return;
	}
#endif

	ValidateScriptScope();

	// All functions we want to have call chained instead of overwritten
	// by other scripts in this entities list.
	static const char* sCallChainFunctions[] =
	{
		"OnPostSpawn",
		"Precache"
	};

	ScriptLanguage_t language = g_pScriptVM->GetLanguage();

	// Make a call chainer for each in this entities scope
	for (int j = 0; j < ARRAYSIZE(sCallChainFunctions); ++j)
	{

		if (language == SL_PYTHON)
		{
			// UNDONE - handle call chaining in python
			;
		}
		else if (language == SL_SQUIRREL)
		{
			//TODO: For perf, this should be precompiled and the %s should be passed as a parameter
			HSCRIPT hCreateChainScript = g_pScriptVM->CompileScript(CFmtStr("%sCallChain <- CSimpleCallChainer(\"%s\", self.GetScriptScope(), true)", sCallChainFunctions[j], sCallChainFunctions[j]));
			g_pScriptVM->Run(hCreateChainScript, (HSCRIPT)m_ScriptScope);
		}
	}

	CUtlStringList szScripts;
	if (m_iszVScripts != NULL_STRING)
	{
		V_SplitString(STRING(m_iszVScripts), " ", szScripts);
	}

	if (m_iszClientScripts[0] != '\0')
	{
		szScripts.AddToHead(strdup(m_iszClientScripts.Get()));
	}

	for (int i = 0; i < szScripts.Count(); i++)
	{
#ifdef MAPBASE
		CGMsg(0, CON_GROUP_VSCRIPT, "%s executing script: %s\n", GetDebugName(), szScripts[i]);
#else
		Log("%s executing script: %s\n", GetDebugName(), szScripts[i]);
#endif

		RunScriptFile(szScripts[i], IsWorld());

		for (int j = 0; j < ARRAYSIZE(sCallChainFunctions); ++j)
		{
			if (language == SL_PYTHON)
			{
				// UNDONE - handle call chaining in python
				;
			}
			else if (language == SL_SQUIRREL)
			{
				//TODO: For perf, this should be precompiled and the %s should be passed as a parameter.
				HSCRIPT hRunPostScriptExecute = g_pScriptVM->CompileScript(CFmtStr("%sCallChain.PostScriptExecute()", sCallChainFunctions[j]));
				g_pScriptVM->Run(hRunPostScriptExecute, (HSCRIPT)m_ScriptScope);
			}
		}
	}

	if (m_iszScriptThinkFunction != NULL_STRING)
	{
		SetContextThink(&CBaseEntity::ScriptThink, gpGlobals->curtime + sv_script_think_interval.GetFloat(), "ScriptThink");
	}
#endif
}