//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#ifdef HL2_EPISODIC

#include "hl2_gamerules.h"
#include "ammodef.h"
#include "hl2_shareddefs.h"
#include "filesystem.h"
#include <KeyValues.h>

#ifdef CLIENT_DLL

#else
#include "player.h"
#include "game.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "hl2_player.h"
#include "voice_gamemgr.h"
#include "globalstate.h"
#include "ai_basenpc.h"
#include "weapon_physcannon.h"
#include "ammodef.h"
#endif

#ifdef CLIENT_DLL
#define CHalfLife2Survival C_HalfLife2Survival
#define CHalfLife2SurvivalProxy C_HalfLife2SurvivalProxy
#endif

ConVar gamerules_survival( "gamerules_survival", "0", FCVAR_REPLICATED );

class CHalfLife2SurvivalProxy : public CGameRulesProxy
{
public:
	DECLARE_CLASS( CHalfLife2SurvivalProxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CSurvivalAmmo
{
public:

	char	m_szAmmoName[256];
	int		m_iAmount;
};

class CSurvivalSettings
{
public:

	CSurvivalSettings();

	CUtlVector<char*, CUtlMemory<char*> > m_Loadout;
	int		 m_iSpawnHealth;
	string_t m_szPickups;
	CUtlVector<CSurvivalAmmo> m_Ammo;
};

CSurvivalSettings::CSurvivalSettings()
{
	m_iSpawnHealth = 100;
}

class CHalfLife2Survival : public CHalfLife2
{
public:
	DECLARE_CLASS( CHalfLife2Survival, CHalfLife2 );

#ifdef CLIENT_DLL

	DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars.

#else

	DECLARE_SERVERCLASS_NOBASE(); // This makes datatables able to access our private vars.

	CHalfLife2Survival();
	virtual ~CHalfLife2Survival() {}

	virtual void Think( void );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual bool IsAllowedToSpawn( CBaseEntity *pEntity );
	virtual void CreateStandardEntities();

	void	ReadSurvivalScriptFile( void );
	void	ParseSurvivalSettings( KeyValues *pSubKey );
	void	ParseSurvivalAmmo( KeyValues *pSubKey );

private:
	bool			  m_bActive;
	CSurvivalSettings m_SurvivalSettings;
#endif

};

//-----------------------------------------------------------------------------
// Gets us at the Half-Life 2 game rules
//-----------------------------------------------------------------------------
inline CHalfLife2Survival* HL2SurvivalGameRules()
{
	return static_cast<CHalfLife2Survival*>(g_pGameRules);
}

REGISTER_GAMERULES_CLASS( CHalfLife2Survival );

BEGIN_NETWORK_TABLE_NOBASE( CHalfLife2Survival, DT_HL2SurvivalGameRules )
END_NETWORK_TABLE()


LINK_ENTITY_TO_CLASS( hl2_survival_gamerules, CHalfLife2SurvivalProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( HalfLife2SurvivalProxy, DT_HalfLife2SurvivalProxy )

#ifdef CLIENT_DLL
	void RecvProxy_HL2SurvivalGameRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CHalfLife2Survival *pRules = HL2SurvivalGameRules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CHalfLife2SurvivalProxy, DT_HalfLife2SurvivalProxy )
	RecvPropDataTable( "hl2_survival_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_HL2SurvivalGameRules ), RecvProxy_HL2SurvivalGameRules )
	END_RECV_TABLE()
	#else
	void* SendProxy_HL2SurvivalGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CHalfLife2Survival *pRules = HL2SurvivalGameRules();
		Assert( pRules );
		pRecipients->SetAllRecipients();
		return pRules;
	}

	BEGIN_SEND_TABLE( CHalfLife2SurvivalProxy, DT_HalfLife2SurvivalProxy )
	SendPropDataTable( "hl2_survival_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_HL2SurvivalGameRules ), SendProxy_HL2SurvivalGameRules )
	END_SEND_TABLE()
#endif

#ifndef CLIENT_DLL

CHalfLife2Survival::CHalfLife2Survival()
{
	m_bActive = false;
}

void CHalfLife2Survival::Think( void )
{

}

bool CHalfLife2Survival::IsAllowedToSpawn( CBaseEntity *pEntity )
{
	if ( !m_bActive )
		return BaseClass::IsAllowedToSpawn( pEntity );

	const char *pPickups = STRING( m_SurvivalSettings.m_szPickups );
	if ( !pPickups )
		return false;

	if ( Q_stristr( pPickups, "everything" ) )
		return true;

	if ( Q_stristr( pPickups, pEntity->GetClassname() ) ||  Q_stristr( pPickups, STRING( pEntity->GetEntityName() ) ) )
		return true;

	return false;
}

void CHalfLife2Survival::PlayerSpawn( CBasePlayer *pPlayer )
{
	BaseClass::PlayerSpawn( pPlayer );

	if ( !m_bActive )
		return;

	pPlayer->EquipSuit();
	pPlayer->SetHealth( m_SurvivalSettings.m_iSpawnHealth );

	for ( int i = 0; i < m_SurvivalSettings.m_Loadout.Count(); ++i )
	{
		pPlayer->GiveNamedItem( m_SurvivalSettings.m_Loadout[i] );
	}

	for ( int i = 0; i < m_SurvivalSettings.m_Ammo.Count(); ++i )
	{
		pPlayer->CBasePlayer::GiveAmmo( m_SurvivalSettings.m_Ammo[i].m_iAmount , m_SurvivalSettings.m_Ammo[i].m_szAmmoName );
	}
}

void CHalfLife2Survival::ParseSurvivalSettings( KeyValues *pSubKey )
{
	if ( pSubKey == NULL )
		return;

	m_SurvivalSettings.m_szPickups = NULL_STRING;
	m_SurvivalSettings.m_iSpawnHealth = 100;

	KeyValues *pTestKey = pSubKey->GetFirstSubKey();

	while ( pTestKey )
	{
		if ( !stricmp( pTestKey->GetName(), "weapons" ) )
		{
			const char *pLoadout =  pTestKey->GetString();
			Q_SplitString( pLoadout, ";", m_SurvivalSettings.m_Loadout );
		}
		else if ( !stricmp( pTestKey->GetName(), "spawnhealth" ) )
		{
			m_SurvivalSettings.m_iSpawnHealth = pTestKey->GetInt( NULL, 100 );
		}
		else if ( !stricmp( pTestKey->GetName(), "allowedpickups" ) )
		{
			m_SurvivalSettings.m_szPickups = MAKE_STRING( pTestKey->GetString() );
		}
		
		pTestKey = pTestKey->GetNextKey();
	}
}

void CHalfLife2Survival::ParseSurvivalAmmo( KeyValues *pSubKey )
{
	if ( pSubKey )
	{
		KeyValues *pAmmoKey = pSubKey->GetFirstSubKey();

		while ( pAmmoKey )
		{
			CSurvivalAmmo ammo;

			Q_strcpy( ammo.m_szAmmoName, pAmmoKey->GetName() );
			ammo.m_iAmount = pAmmoKey->GetInt();

			m_SurvivalSettings.m_Ammo.AddToTail( ammo );

			pAmmoKey = pAmmoKey->GetNextKey();
		}
	}
}

void CHalfLife2Survival::ReadSurvivalScriptFile( void )
{
	char szFullName[512];
	Q_snprintf( szFullName, sizeof( szFullName ), "maps/%s_survival.txt", STRING(gpGlobals->mapname) );

	KeyValues *pkvFile = new KeyValues( "Survival" );
	if ( pkvFile->LoadFromFile( filesystem, szFullName, "MOD" ) )
	{
		ParseSurvivalSettings( pkvFile->FindKey( "settings" ) );
		ParseSurvivalAmmo( pkvFile->FindKey( "ammo" ) );

		CUtlVector <CBaseEntity*> entities;
		UTIL_LoadAndSpawnEntitiesFromScript( entities, szFullName, "Survival", true );

		// It's important to turn on survival mode after we create all the entities
		// in the script, so that we don't remove them if they violate survival rules.
		// i.e. we want the player to start with a shotgun, but prevent all future shotguns from spawning.
		m_bActive = true;
	}
	else
	{
		m_bActive = false;
	}
}

void CHalfLife2Survival::CreateStandardEntities( void )
{
	ReadSurvivalScriptFile();
}

#endif

#endif