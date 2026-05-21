//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#ifdef SOURCESDK

#include "ow_hero_registry.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "tf_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static OWGameModeType_t s_OWMapMode = OW_MODE_AUTO;

//-----------------------------------------------------------------------------
static int OW_ClassNameToIndex( const char *pszClass )
{
	if ( !pszClass )
	{
		return TF_CLASS_UNDEFINED;
	}

	if ( Q_stricmp( pszClass, "scout" ) == 0 ) return TF_CLASS_SCOUT;
	if ( Q_stricmp( pszClass, "sniper" ) == 0 ) return TF_CLASS_SNIPER;
	if ( Q_stricmp( pszClass, "soldier" ) == 0 ) return TF_CLASS_SOLDIER;
	if ( Q_stricmp( pszClass, "demoman" ) == 0 ) return TF_CLASS_DEMOMAN;
	if ( Q_stricmp( pszClass, "medic" ) == 0 ) return TF_CLASS_MEDIC;
	if ( Q_stricmp( pszClass, "heavy" ) == 0 || Q_stricmp( pszClass, "heavyweapons" ) == 0 ) return TF_CLASS_HEAVYWEAPONS;
	if ( Q_stricmp( pszClass, "pyro" ) == 0 ) return TF_CLASS_PYRO;
	if ( Q_stricmp( pszClass, "spy" ) == 0 ) return TF_CLASS_SPY;
	if ( Q_stricmp( pszClass, "engineer" ) == 0 ) return TF_CLASS_ENGINEER;

	return TF_CLASS_UNDEFINED;
}

//-----------------------------------------------------------------------------
static int OW_AbilityNameToType( const char *pszType )
{
	if ( !pszType )
	{
		return OW_ABILITY_COND;
	}

	if ( Q_stricmp( pszType, "heal_aura" ) == 0 ) return OW_ABILITY_HEAL_AURA;
	if ( Q_stricmp( pszType, "damage_boost" ) == 0 ) return OW_ABILITY_DAMAGE_BOOST;
	if ( Q_stricmp( pszType, "shield" ) == 0 ) return OW_ABILITY_SHIELD;
	if ( Q_stricmp( pszType, "ult_rage" ) == 0 ) return OW_ABILITY_ULT_RAGE;
	if ( Q_stricmp( pszType, "ult_heal" ) == 0 ) return OW_ABILITY_ULT_HEAL;
	if ( Q_stricmp( pszType, "ult_sniper" ) == 0 ) return OW_ABILITY_ULT_SNIPER;

	return OW_ABILITY_COND;
}

//-----------------------------------------------------------------------------
static int OW_CondNameToIndex( const char *pszCond )
{
	if ( !pszCond || !pszCond[0] )
	{
		return TF_COND_SPEED_BOOST;
	}

	if ( Q_stricmp( pszCond, "speed_boost" ) == 0 ) return TF_COND_SPEED_BOOST;
	if ( Q_stricmp( pszCond, "defense_buff" ) == 0 ) return TF_COND_DEFENSEBUFF;
	if ( Q_stricmp( pszCond, "offense_buff" ) == 0 ) return TF_COND_OFFENSEBUFF;
	if ( Q_stricmp( pszCond, "invulnerable" ) == 0 ) return TF_COND_INVULNERABLE;
	if ( Q_stricmp( pszCond, "critboost" ) == 0 ) return TF_COND_CRITBOOSTED;

	return TF_COND_SPEED_BOOST;
}

//-----------------------------------------------------------------------------
COWHeroRegistry &COWHeroRegistry::Instance( void )
{
	static COWHeroRegistry s_Instance;
	return s_Instance;
}

//-----------------------------------------------------------------------------
void COWHeroRegistry::Clear( void )
{
	m_nHeroCount = 0;
	memset( m_Heroes, 0, sizeof( m_Heroes ) );
}

//-----------------------------------------------------------------------------
void COWHeroRegistry::Reload( void )
{
	Clear();

	KeyValues *pKV = new KeyValues( "OWHeroes" );
	if ( !pKV->LoadFromFile( filesystem, "scripts/ow_heroes.txt", "MOD" ) )
	{
		Warning( "OW: scripts/ow_heroes.txt not found — using built-in roster.\n" );
		pKV->deleteThis();
		// Built-in fallback
		struct BuiltinHero_t { const char *pszId; const char *pszName; const char *pszClass; int iHP; float flSpeed; }
		builtins[] = {
			{ "skirmisher", "Volt", "scout", 125, 1.15f },
			{ "tank", "Bulwark", "heavy", 300, 0.90f },
			{ "bruiser", "Ember", "pyro", 200, 1.0f },
			{ "sniper", "Hawkeye", "sniper", 150, 0.95f },
			{ "healer", "Lifeline", "medic", 150, 1.05f },
			{ "builder", "Forge", "engineer", 125, 1.0f },
		};
		for ( int i = 0; i < ARRAYSIZE( builtins ) && m_nHeroCount < OW_MAX_HEROES; ++i )
		{
			OWHeroDefinition_t &h = m_Heroes[m_nHeroCount];
			h.m_iHeroId = m_nHeroCount;
			Q_strncpy( h.m_szInternalName, builtins[i].pszId, sizeof( h.m_szInternalName ) );
			Q_strncpy( h.m_szName, builtins[i].pszName, sizeof( h.m_szName ) );
			h.m_iTFClass = OW_ClassNameToIndex( builtins[i].pszClass );
			h.m_iMaxHealth = builtins[i].iHP;
			h.m_flMoveSpeedScale = builtins[i].flSpeed;
			Q_strncpy( h.m_szPrimaryWeapon, "tf_weapon_pistol", sizeof( h.m_szPrimaryWeapon ) );
			Q_strncpy( h.m_szSecondaryWeapon, "tf_weapon_shotgun_primary", sizeof( h.m_szSecondaryWeapon ) );
			Q_strncpy( h.m_szMeleeWeapon, "tf_weapon_bottle", sizeof( h.m_szMeleeWeapon ) );
			h.m_flAbilityCooldown[0] = 6.0f;
			h.m_flAbilityCooldown[1] = 10.0f;
			h.m_flAbilityCooldown[2] = 12.0f;
			h.m_iAbilityType[0] = OW_ABILITY_COND;
			h.m_iAbilityCond[0] = TF_COND_SPEED_BOOST;
			h.m_flAbilityDuration[0] = 3.0f;
			h.m_flUltChargeRate = 1.0f;
			h.m_flUltDuration = 6.0f;
			h.m_iUltType = OW_ABILITY_ULT_RAGE;
			++m_nHeroCount;
		}
		return;
	}

	int iHeroIndex = 0;
	for ( KeyValues *pHero = pKV->GetFirstSubKey(); pHero && m_nHeroCount < OW_MAX_HEROES; pHero = pHero->GetNextKey() )
	{
		OWHeroDefinition_t &h = m_Heroes[m_nHeroCount];
		h.m_iHeroId = iHeroIndex++;
		Q_strncpy( h.m_szInternalName, pHero->GetName(), sizeof( h.m_szInternalName ) );
		Q_strncpy( h.m_szName, pHero->GetString( "name", pHero->GetName() ), sizeof( h.m_szName ) );
		h.m_iTFClass = OW_ClassNameToIndex( pHero->GetString( "tf_class", "scout" ) );
		h.m_iMaxHealth = pHero->GetInt( "max_health", 150 );
		h.m_flMoveSpeedScale = pHero->GetFloat( "move_speed", 1.0f );
		Q_strncpy( h.m_szPrimaryWeapon, pHero->GetString( "primary_weapon", "tf_weapon_pistol" ), sizeof( h.m_szPrimaryWeapon ) );
		Q_strncpy( h.m_szSecondaryWeapon, pHero->GetString( "secondary_weapon", "" ), sizeof( h.m_szSecondaryWeapon ) );
		Q_strncpy( h.m_szMeleeWeapon, pHero->GetString( "melee_weapon", "tf_weapon_bottle" ), sizeof( h.m_szMeleeWeapon ) );
		h.m_flAbilityCooldown[0] = pHero->GetFloat( "ability1_cooldown", 6.0f );
		h.m_flAbilityCooldown[1] = pHero->GetFloat( "ability2_cooldown", 10.0f );
		h.m_flAbilityCooldown[2] = pHero->GetFloat( "ability3_cooldown", 12.0f );
		h.m_iAbilityType[0] = OW_AbilityNameToType( pHero->GetString( "ability1_type", "cond" ) );
		h.m_iAbilityType[1] = OW_AbilityNameToType( pHero->GetString( "ability2_type", "cond" ) );
		h.m_iAbilityType[2] = OW_AbilityNameToType( pHero->GetString( "ability3_type", "cond" ) );
		h.m_iAbilityCond[0] = OW_CondNameToIndex( pHero->GetString( "ability1_cond", "speed_boost" ) );
		h.m_iAbilityCond[1] = OW_CondNameToIndex( pHero->GetString( "ability2_cond", "defense_buff" ) );
		h.m_iAbilityCond[2] = OW_CondNameToIndex( pHero->GetString( "ability3_cond", "offense_buff" ) );
		h.m_flAbilityDuration[0] = pHero->GetFloat( "ability1_duration", 3.0f );
		h.m_flAbilityDuration[1] = pHero->GetFloat( "ability2_duration", 4.0f );
		h.m_flAbilityDuration[2] = pHero->GetFloat( "ability3_duration", 4.0f );
		h.m_flUltChargeRate = pHero->GetFloat( "ult_charge_rate", 1.0f );
		h.m_flUltDuration = pHero->GetFloat( "ult_duration", 6.0f );
		h.m_iUltType = OW_AbilityNameToType( pHero->GetString( "ult_type", "ult_rage" ) );
		++m_nHeroCount;
	}

	pKV->deleteThis();
	Msg( "OW: loaded %d heroes from scripts/ow_heroes.txt\n", m_nHeroCount );
}

//-----------------------------------------------------------------------------
const OWHeroDefinition_t *COWHeroRegistry::GetHeroById( int iHeroId ) const
{
	for ( int i = 0; i < m_nHeroCount; ++i )
	{
		if ( m_Heroes[i].m_iHeroId == iHeroId )
		{
			return &m_Heroes[i];
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
const OWHeroDefinition_t *COWHeroRegistry::GetHeroByTFClass( int iTFClass ) const
{
	for ( int i = 0; i < m_nHeroCount; ++i )
	{
		if ( m_Heroes[i].m_iTFClass == iTFClass )
		{
			return &m_Heroes[i];
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
const OWHeroDefinition_t *COWHeroRegistry::GetHeroByIndex( int iIndex ) const
{
	if ( iIndex < 0 || iIndex >= m_nHeroCount )
	{
		return NULL;
	}
	return &m_Heroes[iIndex];
}

//-----------------------------------------------------------------------------
int COWHeroRegistry::GetTFClassForHeroId( int iHeroId ) const
{
	const OWHeroDefinition_t *pHero = GetHeroById( iHeroId );
	return pHero ? pHero->m_iTFClass : TF_CLASS_UNDEFINED;
}

//-----------------------------------------------------------------------------
bool COWHeroRegistry::IsTFClassEnabled( int iTFClass ) const
{
	return GetHeroByTFClass( iTFClass ) != NULL;
}

//-----------------------------------------------------------------------------
void COWHeroRegistry::PrintRoster( void ) const
{
	Msg( "=== OW Hero Roster (%d) ===\n", m_nHeroCount );
	for ( int i = 0; i < m_nHeroCount; ++i )
	{
		const OWHeroDefinition_t &h = m_Heroes[i];
		Msg( "  [%d] %s (%s) class=%d HP=%d speed=%.2f\n",
			h.m_iHeroId, h.m_szName, h.m_szInternalName, h.m_iTFClass, h.m_iMaxHealth, h.m_flMoveSpeedScale );
	}
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
void OW_LoadModeConfig( void )
{
	s_OWMapMode = OW_MODE_AUTO;
	KeyValues *pKV = new KeyValues( "OWModes" );
	if ( !pKV->LoadFromFile( filesystem, "scripts/ow_modes.txt", "MOD" ) )
	{
		pKV->deleteThis();
		return;
	}

	const char *pszMap = gpGlobals->mapname.ToCStr();
	if ( !pszMap || !pszMap[0] )
	{
		pKV->deleteThis();
		return;
	}

	char szMapBase[MAX_PATH];
	V_FileBase( pszMap, szMapBase, sizeof( szMapBase ) );

	for ( KeyValues *pMode = pKV->GetFirstSubKey(); pMode; pMode = pMode->GetNextKey() )
	{
		const char *pszPrefix = pMode->GetName();
		if ( pszPrefix && Q_stristr( szMapBase, pszPrefix ) != NULL )
		{
			const char *pszType = pMode->GetString( "type", "auto" );
			if ( Q_stricmp( pszType, "escort" ) == 0 ) s_OWMapMode = OW_MODE_ESCORT;
			else if ( Q_stricmp( pszType, "control" ) == 0 ) s_OWMapMode = OW_MODE_CONTROL;
			else if ( Q_stricmp( pszType, "assault" ) == 0 ) s_OWMapMode = OW_MODE_ASSAULT;
			Msg( "OW: map '%s' uses mode '%s'\n", szMapBase, pszType );
			break;
		}
	}

	pKV->deleteThis();
}

//-----------------------------------------------------------------------------
OWGameModeType_t OW_GetModeForMap( const char *pszMapName )
{
	return s_OWMapMode;
}
#endif // GAME_DLL

#endif // SOURCESDK
