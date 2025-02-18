//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_classdata.h"

extern bool UseHWMorphModels();

// Player class files
#define TF_CLASS_UNDEFINED_FILE			""
#define TF_CLASS_SCOUT_FILE				"scripts/playerclasses/scout"
#define TF_CLASS_SNIPER_FILE			"scripts/playerclasses/sniper"
#define TF_CLASS_SOLDIER_FILE			"scripts/playerclasses/soldier"
#define TF_CLASS_DEMOMAN_FILE			"scripts/playerclasses/demoman"
#define TF_CLASS_MEDIC_FILE				"scripts/playerclasses/medic"
#define TF_CLASS_HEAVYWEAPONS_FILE		"scripts/playerclasses/heavyweapons"
#define TF_CLASS_PYRO_FILE				"scripts/playerclasses/pyro"
#define TF_CLASS_SPY_FILE				"scripts/playerclasses/spy"
#define TF_CLASS_ENGINEER_FILE			"scripts/playerclasses/engineer"
#define TF_CLASS_CIVILIAN_FILE			"scripts/playerclasses/civilian"

const char *s_aPlayerClassFiles[] =
{
	TF_CLASS_UNDEFINED_FILE,
	TF_CLASS_SCOUT_FILE,
	TF_CLASS_SNIPER_FILE,
	TF_CLASS_SOLDIER_FILE,
	TF_CLASS_DEMOMAN_FILE,
	TF_CLASS_MEDIC_FILE,
	TF_CLASS_HEAVYWEAPONS_FILE,
	TF_CLASS_PYRO_FILE,
	TF_CLASS_SPY_FILE,
	TF_CLASS_ENGINEER_FILE,
	TF_CLASS_CIVILIAN_FILE
};


CTFPlayerClassDataMgr s_TFPlayerClassDataMgr;
CTFPlayerClassDataMgr *g_pTFPlayerClassDataMgr = &s_TFPlayerClassDataMgr; 

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
TFPlayerClassData_t::TFPlayerClassData_t()
{
	m_szClassName[0] = '\0';
	m_szModelName[0] = '\0';
	m_szHWMModelName[0] = '\0';
	m_szHandModelName[0] = '\0';
	m_szLocalizableName[0] = '\0';
	m_flMaxSpeed = 0.0f;
	m_nMaxHealth = 0;
	m_nMaxArmor = 0;

#ifdef GAME_DLL
	for ( int i = 0; i < ARRAYSIZE( m_szDeathSound ); ++i )
	{
		m_szDeathSound[ i ][ 0 ] = '\0';
	}
#endif

	for ( int iWeapon = 0; iWeapon < TF_PLAYER_WEAPON_COUNT; ++iWeapon )
	{
		m_aWeapons[iWeapon] = TF_WEAPON_NONE;
	}

	for ( int iGrenade = 0; iGrenade < TF_PLAYER_GRENADE_COUNT; ++iGrenade )
	{
		m_aGrenades[iGrenade] = TF_WEAPON_NONE;
	}

	for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
	{
		m_aAmmoMax[iAmmo] = TF_AMMO_DUMMY;
	}

	for ( int iBuildable = 0; iBuildable < TF_PLAYER_BLUEPRINT_COUNT; ++iBuildable )
	{
		m_aBuildable[iBuildable] = OBJ_LAST;
	}

	m_bParsed = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *TFPlayerClassData_t::GetModelName() const
{
#ifdef CLIENT_DLL
	if ( UseHWMorphModels() )
	{
		if ( m_szHWMModelName[0] != '\0' )
		{
			return m_szHWMModelName;
		}
	}

	return m_szModelName;
#else
	return m_szModelName;
#endif
}

#ifdef GAME_DLL
const char *TFPlayerClassData_t::GetDeathSound( int nType )
{
	return m_szDeathSound[ nType ];
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TFPlayerClassData_t::Parse( const char *szName )
{
	// Have we parsed this file already?
	if ( m_bParsed )
		return;

	// Parse class file.
	const unsigned char *pKey = GetTFEncryptionKey();
	KeyValues *pKV = ReadEncryptedKVFile( filesystem, szName, pKey );
	if ( pKV )
	{
		ParseData( pKV );
		pKV->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TFPlayerClassData_t::ParseData( KeyValues *pKeyValuesData )
{
	// Attributes.
	Q_strncpy( m_szClassName, pKeyValuesData->GetString( "name" ), TF_NAME_LENGTH );

	// Load the high res model or the lower res model.
	if ( !IsX360() )
	{
		Q_strncpy( m_szHWMModelName, pKeyValuesData->GetString( "model_hwm" ), TF_NAME_LENGTH );
	}
	Q_strncpy( m_szModelName, pKeyValuesData->GetString( "model" ), TF_NAME_LENGTH );
	Q_strncpy( m_szHandModelName, pKeyValuesData->GetString( "model_hands" ), TF_NAME_LENGTH );
	Q_strncpy( m_szLocalizableName, pKeyValuesData->GetString( "localize_name" ), TF_NAME_LENGTH );

	m_flMaxSpeed = pKeyValuesData->GetFloat( "speed_max" );
	m_nMaxHealth = pKeyValuesData->GetInt( "health_max" );
	m_nMaxArmor = pKeyValuesData->GetInt( "armor_max" );

	// Weapons.
	int i;
	char buf[32];
	for ( i=0;i<TF_PLAYER_WEAPON_COUNT;i++ )
	{
		Q_snprintf( buf, sizeof(buf), "weapon%d", i+1 );		
		m_aWeapons[i] = GetWeaponId( pKeyValuesData->GetString( buf ) );
	}

	// Grenades.
	m_aGrenades[0] = GetWeaponId( pKeyValuesData->GetString( "grenade1" ) );
	m_aGrenades[1] = GetWeaponId( pKeyValuesData->GetString( "grenade2" ) );

	// Ammo Max.
	KeyValues *pAmmoKeyValuesData = pKeyValuesData->FindKey( "AmmoMax" );
	if ( pAmmoKeyValuesData )
	{
		for ( int iAmmo = 1; iAmmo < TF_AMMO_COUNT; ++iAmmo )
		{
			m_aAmmoMax[iAmmo] = pAmmoKeyValuesData->GetInt( GetAmmoName( iAmmo ), 0 );
		}
	}

	// Buildables
	for ( i=0;i<TF_PLAYER_BLUEPRINT_COUNT;i++ )
	{
		Q_snprintf( buf, sizeof(buf), "buildable%d", i+1 );		
		m_aBuildable[i] = GetBuildableId( pKeyValuesData->GetString( buf ) );		
	}

	// Temp animation flags
	m_bDontDoAirwalk = ( pKeyValuesData->GetInt( "DontDoAirwalk", 0 ) > 0 );
	m_bDontDoNewJump = ( pKeyValuesData->GetInt( "DontDoNewJump", 0 ) > 0 );

	m_vecThirdPersonOffset.x = pKeyValuesData->GetFloat( "cameraoffset_forward" );
	m_vecThirdPersonOffset.y = pKeyValuesData->GetFloat( "cameraoffset_right" );
	m_vecThirdPersonOffset.z = pKeyValuesData->GetFloat( "cameraoffset_up" );

#ifdef GAME_DLL		// right now we only emit these sounds from server. if that changes we can do this in both dlls

	// Death Sounds
	Q_strncpy( m_szDeathSound[ DEATH_SOUND_GENERIC ], pKeyValuesData->GetString( "sound_death", "Player.Death" ), MAX_PLAYERCLASS_SOUND_LENGTH );
	Q_strncpy( m_szDeathSound[ DEATH_SOUND_CRIT ], pKeyValuesData->GetString( "sound_crit_death", "TFPlayer.CritDeath" ), MAX_PLAYERCLASS_SOUND_LENGTH );
	Q_strncpy( m_szDeathSound[ DEATH_SOUND_MELEE ], pKeyValuesData->GetString( "sound_melee_death", "Player.MeleeDeath" ), MAX_PLAYERCLASS_SOUND_LENGTH );
	Q_strncpy( m_szDeathSound[ DEATH_SOUND_EXPLOSION ], pKeyValuesData->GetString( "sound_explosion_death", "Player.ExplosionDeath" ), MAX_PLAYERCLASS_SOUND_LENGTH );
#endif

	// The file has been parsed.
	m_bParsed = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TFPlayerClassData_t::AddAdditionalPlayerDeathSounds( void )
{
#ifdef GAME_DLL	
	for ( int i = DEATH_SOUND_FIRST; i <= DEATH_SOUND_LAST; ++i )
	{
		CopySoundNameWithModifierToken( m_szDeathSound[ i + DEATH_SOUND_MVM_FIRST ], m_szDeathSound[ i ], ARRAYSIZE( m_szDeathSound[0] ), "MVM_" );
		CopySoundNameWithModifierToken( m_szDeathSound[ i + DEATH_SOUND_GIANT_MVM_FIRST ], m_szDeathSound[ i ], ARRAYSIZE( m_szDeathSound[0] ), "M_MVM_" );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayerClassDataMgr::CTFPlayerClassDataMgr()
{

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerClassDataMgr::Init( void )
{
	// Special case the undefined class.
	TFPlayerClassData_t *pClassData = &m_aTFPlayerClassData[TF_CLASS_UNDEFINED];
	Assert( pClassData );
	Q_strncpy( pClassData->m_szClassName, "undefined", TF_NAME_LENGTH );
	Q_strncpy( pClassData->m_szModelName, "models/player/scout.mdl", TF_NAME_LENGTH );	// Undefined players still need a model
	Q_strncpy( pClassData->m_szLocalizableName, "undefined", TF_NAME_LENGTH );

	// Initialize the classes.
	for ( int iClass = 1; iClass < TF_CLASS_COUNT_ALL; ++iClass )
	{
		COMPILE_TIME_ASSERT( ARRAYSIZE( s_aPlayerClassFiles ) == TF_CLASS_COUNT_ALL );
		pClassData = &m_aTFPlayerClassData[iClass];
		Assert( pClassData );
		pClassData->Parse( s_aPlayerClassFiles[iClass] );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to get player class data.
//-----------------------------------------------------------------------------
TFPlayerClassData_t *CTFPlayerClassDataMgr::Get( unsigned int iClass )
{
	Assert ( iClass < TF_CLASS_COUNT_ALL );
	return &m_aTFPlayerClassData[iClass];
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
TFPlayerClassData_t *GetPlayerClassData( unsigned int iClass )
{
	return g_pTFPlayerClassDataMgr->Get( iClass );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerClassDataMgr::AddAdditionalPlayerDeathSounds( void )
{
	for ( int iClass = 1; iClass < TF_CLASS_COUNT_ALL; ++iClass )
	{
		TFPlayerClassData_t *pClassData = &m_aTFPlayerClassData[iClass];
		Assert( pClassData );
		pClassData->AddAdditionalPlayerDeathSounds();
	}
}