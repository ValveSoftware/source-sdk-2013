//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "item_dynamic_resupply.h"
#include "props.h"
#include "items.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sk_dynamic_resupply_modifier( "sk_dynamic_resupply_modifier","1.0" );
extern ConVar sk_battery;
extern ConVar sk_healthkit;

ConVar g_debug_dynamicresupplies( "g_debug_dynamicresupplies", "0", FCVAR_NONE, "Debug item_dynamic_resupply spawning. Set to 1 to see text printouts of the spawning. Set to 2 to see lines drawn to other items factored into the spawning." );

struct DynamicResupplyItems_t
{
	const char *sEntityName;
	const char *sAmmoDef;
	int			iAmmoCount;
	float		flFullProbability;	// Probability of spawning if the player meeds all goals
};

struct SpawnInfo_t
{
	float m_flDesiredRatio;
	float m_flCurrentRatio;
	float m_flDelta;
	int	  m_iPotentialItems;
};


// Health types
static DynamicResupplyItems_t g_DynamicResupplyHealthItems[] =
{
	{ "item_healthkit", "Health",	0, 0.0f, },
	{ "item_battery",	"Armor",	0, 0.0f },
};

// Ammo types
static DynamicResupplyItems_t g_DynamicResupplyAmmoItems[] =
{
	{ "item_ammo_pistol",			"Pistol",		SIZE_AMMO_PISTOL,		0.5f },
	{ "item_ammo_smg1",				"SMG1",			SIZE_AMMO_SMG1,			0.4f },
	{ "item_ammo_smg1_grenade",		"SMG1_Grenade", SIZE_AMMO_SMG1_GRENADE, 0.0f },
	{ "item_ammo_ar2",				"AR2",			SIZE_AMMO_AR2,			0.0f },
	{ "item_box_buckshot",			"Buckshot",		SIZE_AMMO_BUCKSHOT,		0.0f },
	{ "item_rpg_round",				"RPG_Round",	SIZE_AMMO_RPG_ROUND,	0.0f },
	{ "weapon_frag",				"Grenade",		1,						0.1f },
	{ "item_ammo_357",				"357",			SIZE_AMMO_357,			0.0f },
	{ "item_ammo_crossbow",			"XBowBolt",		SIZE_AMMO_CROSSBOW,		0.0f },
	{ "item_ammo_ar2_altfire",		"AR2AltFire",	SIZE_AMMO_AR2_ALTFIRE,	0.0f },
};

#define DS_HEALTH_INDEX		0
#define DS_ARMOR_INDEX		1
#define DS_GRENADE_INDEX	6

#define NUM_HEALTH_ITEMS	(ARRAYSIZE(g_DynamicResupplyHealthItems))
#define NUM_AMMO_ITEMS		(ARRAYSIZE(g_DynamicResupplyAmmoItems))

#define DYNAMIC_ITEM_THINK		1.0

#define POTENTIAL_ITEM_RADIUS	1024

//-----------------------------------------------------------------------------
// Purpose: An item that dynamically decides what the player needs most and spawns that.
//-----------------------------------------------------------------------------
class CItem_DynamicResupply : public CPointEntity
{
	DECLARE_CLASS( CItem_DynamicResupply, CPointEntity );
public:
	DECLARE_DATADESC();

	CItem_DynamicResupply();

	void Spawn( void );
	void Precache( void );
	void Activate( void );
	void CheckPVSThink( void );

	// Inputs
	void InputKill( inputdata_t &data );
	void InputCalculateType( inputdata_t &data );
	void InputBecomeMaster( inputdata_t &data );

	float	GetDesiredHealthPercentage( void ) const { return m_flDesiredHealth[0]; }

private:
	friend void DynamicResupply_InitFromAlternateMaster( CBaseEntity *pTargetEnt, string_t iszMaster );
	void FindPotentialItems( int nCount, DynamicResupplyItems_t *pItems, int iDebug, SpawnInfo_t *pSpawnInfo );
	void ComputeHealthRatios( CItem_DynamicResupply* pMaster, CBasePlayer *pPlayer, int iDebug, SpawnInfo_t *pSpawnInfo );
	void ComputeAmmoRatios( CItem_DynamicResupply* pMaster, CBasePlayer *pPlayer, int iDebug, SpawnInfo_t *pSpawnInfo );
	bool SpawnItemFromRatio( int nCount, DynamicResupplyItems_t *pItems, int iDebug, SpawnInfo_t *pSpawnInfo, Vector *pVecSpawnOrigin );

	// Spawns an item when the player is full
	void SpawnFullItem( CItem_DynamicResupply *pMaster, CBasePlayer *pPlayer, int iDebug );
	void SpawnDynamicItem( CBasePlayer *pPlayer );

	enum Versions
	{
		VERSION_0,
		VERSION_1_PERSISTENT_MASTER,

		VERSION_CURRENT = VERSION_1_PERSISTENT_MASTER,
	};

	int m_version;
	float	m_flDesiredHealth[ NUM_HEALTH_ITEMS ];
	float	m_flDesiredAmmo[ NUM_AMMO_ITEMS ];

	bool m_bIsMaster;
};

LINK_ENTITY_TO_CLASS(item_dynamic_resupply, CItem_DynamicResupply);

// Master
typedef CHandle<CItem_DynamicResupply> DynamicResupplyHandle_t;

static DynamicResupplyHandle_t	g_MasterResupply;


//-----------------------------------------------------------------------------
// Save/load: 
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CItem_DynamicResupply )

	DEFINE_THINKFUNC( CheckPVSThink ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Kill", InputKill ),
	DEFINE_INPUTFUNC( FIELD_VOID, "CalculateType", InputCalculateType ),
	DEFINE_INPUTFUNC( FIELD_VOID, "BecomeMaster", InputBecomeMaster ),

	DEFINE_KEYFIELD( m_flDesiredHealth[0], FIELD_FLOAT, "DesiredHealth" ),
	DEFINE_KEYFIELD( m_flDesiredHealth[1], FIELD_FLOAT, "DesiredArmor" ),
	DEFINE_KEYFIELD( m_flDesiredAmmo[0], FIELD_FLOAT, "DesiredAmmoPistol" ),
	DEFINE_KEYFIELD( m_flDesiredAmmo[1], FIELD_FLOAT, "DesiredAmmoSMG1" ),
	DEFINE_KEYFIELD( m_flDesiredAmmo[2], FIELD_FLOAT, "DesiredAmmoSMG1_Grenade" ),
	DEFINE_KEYFIELD( m_flDesiredAmmo[3], FIELD_FLOAT, "DesiredAmmoAR2" ),
	DEFINE_KEYFIELD( m_flDesiredAmmo[4], FIELD_FLOAT, "DesiredAmmoBuckshot" ),
	DEFINE_KEYFIELD( m_flDesiredAmmo[5], FIELD_FLOAT, "DesiredAmmoRPG_Round" ),
	DEFINE_KEYFIELD( m_flDesiredAmmo[6], FIELD_FLOAT, "DesiredAmmoGrenade" ),
	DEFINE_KEYFIELD( m_flDesiredAmmo[7], FIELD_FLOAT, "DesiredAmmo357" ),
	DEFINE_KEYFIELD( m_flDesiredAmmo[8], FIELD_FLOAT, "DesiredAmmoCrossbow" ),
	DEFINE_KEYFIELD( m_flDesiredAmmo[9], FIELD_FLOAT, "DesiredAmmoAR2_AltFire" ),

	DEFINE_FIELD( m_version, FIELD_INTEGER ),
	DEFINE_FIELD( m_bIsMaster, FIELD_BOOLEAN ),

	// Silence, Classcheck!
//	DEFINE_ARRAY( m_flDesiredHealth, FIELD_FLOAT,  NUM_HEALTH_ITEMS  ),
//	DEFINE_ARRAY( m_flDesiredAmmo, FIELD_FLOAT,  NUM_AMMO_ITEMS  ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItem_DynamicResupply::CItem_DynamicResupply( void )
{
	AddSpawnFlags( SF_DYNAMICRESUPPLY_USE_MASTER );
	m_version = VERSION_CURRENT;

	// Setup default values
	m_flDesiredHealth[0] = 1.0;	// Health
	m_flDesiredHealth[1] = 0.3;	// Armor
	m_flDesiredAmmo[0] = 0.5;	// Pistol
	m_flDesiredAmmo[1] = 0.5;	// SMG1
	m_flDesiredAmmo[2] = 0.1;	// SMG1 Grenade
	m_flDesiredAmmo[3] = 0.4;	// AR2
	m_flDesiredAmmo[4] = 0.5;	// Shotgun
	m_flDesiredAmmo[5] = 0.0;	// RPG Round
	m_flDesiredAmmo[6] = 0.1;	// Grenade
	m_flDesiredAmmo[7] = 0;		// 357
	m_flDesiredAmmo[8] = 0;		// Crossbow
	m_flDesiredAmmo[9] = 0;		// AR2 alt-fire
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem_DynamicResupply::Spawn( void )
{ 
	if ( g_pGameRules->IsAllowedToSpawn( this ) == false )
	{
		UTIL_Remove( this );
		return;
	}

	// Don't callback to spawn
	Precache();

	m_bIsMaster = HasSpawnFlags( SF_DYNAMICRESUPPLY_IS_MASTER );

	// Am I the master?
	if ( !HasSpawnFlags( SF_DYNAMICRESUPPLY_IS_MASTER | SF_DYNAMICRESUPPLY_ALTERNATE_MASTER ) )
	{
		// Stagger the thinks a bit so they don't all think at the same time
		SetNextThink( gpGlobals->curtime + RandomFloat(0.2f, 0.4f) );
		SetThink( &CItem_DynamicResupply::CheckPVSThink );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem_DynamicResupply::Activate( void )
{ 
	BaseClass::Activate();

	if ( HasSpawnFlags( SF_DYNAMICRESUPPLY_IS_MASTER ) )
	{
		if ( !g_MasterResupply && ( m_bIsMaster || m_version < VERSION_1_PERSISTENT_MASTER ) )
		{
			g_MasterResupply = this;
		}
		else
		{
			m_bIsMaster = false;
		}
	}
	if ( !HasSpawnFlags( SF_DYNAMICRESUPPLY_ALTERNATE_MASTER ) && HasSpawnFlags( SF_DYNAMICRESUPPLY_USE_MASTER ) && gpGlobals->curtime < 1.0 )
	{
		if ( !g_MasterResupply )
		{
			Warning( "item_dynamic_resupply set to 'Use Master', but no item_dynamic_resupply master exists.\n" );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem_DynamicResupply::Precache( void )
{
	// Precache all the items potentially spawned
	int i;
	for ( i = 0; i < NUM_HEALTH_ITEMS; i++ )
	{
		UTIL_PrecacheOther( g_DynamicResupplyHealthItems[i].sEntityName );
	}

	for ( i = 0; i < NUM_AMMO_ITEMS; i++ )
	{
		UTIL_PrecacheOther( g_DynamicResupplyAmmoItems[i].sEntityName );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem_DynamicResupply::CheckPVSThink( void )
{
	edict_t *pentPlayer = UTIL_FindClientInPVS( edict() );
	if ( pentPlayer )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pentPlayer );
		if ( pPlayer )
		{
			SpawnDynamicItem( pPlayer );
			return;
		}
	}

	SetNextThink( gpGlobals->curtime + DYNAMIC_ITEM_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void CItem_DynamicResupply::InputKill( inputdata_t &data )
{
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void CItem_DynamicResupply::InputCalculateType( inputdata_t &data )
{
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	SpawnDynamicItem( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void CItem_DynamicResupply::InputBecomeMaster( inputdata_t &data )
{
	if ( g_MasterResupply )
		g_MasterResupply->m_bIsMaster = false;

	g_MasterResupply = this;
	m_bIsMaster = true;

	// Stop thinking now that I am the master.
	SetThink( NULL );
}


//-----------------------------------------------------------------------------
// Chooses an item when the player is full
//-----------------------------------------------------------------------------
void CItem_DynamicResupply::SpawnFullItem( CItem_DynamicResupply *pMaster, CBasePlayer *pPlayer, int iDebug )
{
	// Can we not actually spawn the item?
	if ( !HasSpawnFlags(SF_DYNAMICRESUPPLY_ALWAYS_SPAWN) )
		return;

	float flRatio[NUM_AMMO_ITEMS];
	int i;
	float flTotalProb = 0.0f;
	for ( i = 0; i < NUM_AMMO_ITEMS; ++i )
	{
		int iAmmoType = GetAmmoDef()->Index( g_DynamicResupplyAmmoItems[i].sAmmoDef );
		bool bCanSpawn = pPlayer->Weapon_GetWpnForAmmo( iAmmoType ) != NULL;

		if ( bCanSpawn && ( g_DynamicResupplyAmmoItems[i].flFullProbability != 0 ) && ( pMaster->m_flDesiredAmmo[i] != 0.0f ) )
		{
			flTotalProb += g_DynamicResupplyAmmoItems[i].flFullProbability;
			flRatio[i] = flTotalProb;
		}
		else
		{
			flRatio[i] = -1.0f;
		}
	}

	if ( flTotalProb == 0.0f )
	{
		// If we're supposed to fallback to just a health vial, do that and finish.
		if ( pMaster->HasSpawnFlags(SF_DYNAMICRESUPPLY_FALLBACK_TO_VIAL) )
		{
			CBaseEntity::Create( "item_healthvial", GetAbsOrigin(), GetAbsAngles(), this );

			if ( iDebug )
			{
				Msg("Player is full, spawning item_healthvial due to spawnflag.\n");
			}
			return;
		}

		// Otherwise, spawn the first ammo item in the list
		flRatio[0] = 1.0f;
		flTotalProb = 1.0f;
	}
	
	float flChoice = random->RandomFloat( 0.0f, flTotalProb ); 
	for ( i = 0; i < NUM_AMMO_ITEMS; ++i )
	{
		if ( flChoice <= flRatio[i] )
		{
			CBaseEntity::Create( g_DynamicResupplyAmmoItems[i].sEntityName, GetAbsOrigin(), GetAbsAngles(), this );

			if ( iDebug )
			{
				Msg("Player is full, spawning %s \n", g_DynamicResupplyAmmoItems[i].sEntityName );
			}
			return;
		}
	}

	if ( iDebug )
	{
		Msg("Player is full on all health + ammo, is not spawning.\n" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem_DynamicResupply::FindPotentialItems( int nCount, DynamicResupplyItems_t *pItems, int iDebug, SpawnInfo_t *pSpawnInfo )
{
	int i;
	for ( i = 0; i < nCount; ++i )
	{
		pSpawnInfo[i].m_iPotentialItems = 0;
	}

	// Count the potential addition of items in the PVS
	CBaseEntity *pEntity = NULL;
	while ( (pEntity = UTIL_EntitiesInPVS( this, pEntity )) != NULL )
	{
		if ( pEntity->WorldSpaceCenter().DistToSqr( WorldSpaceCenter() ) > (POTENTIAL_ITEM_RADIUS * POTENTIAL_ITEM_RADIUS) )
			continue;

		for ( i = 0; i < nCount; ++i )
		{
			if ( !FClassnameIs( pEntity, pItems[i].sEntityName ) )
				continue;

			if ( iDebug == 2 )
			{
				NDebugOverlay::Line( WorldSpaceCenter(), pEntity->WorldSpaceCenter(), 0,255,0, true, 20.0 );
			}

			++pSpawnInfo[i].m_iPotentialItems;
			break;
		}
	}

	if ( iDebug )
	{
		Msg("Searching the PVS:\n");
		for ( int i = 0; i < nCount; i++ )
		{
			Msg("   Found %d '%s' in the PVS.\n", pSpawnInfo[i].m_iPotentialItems, pItems[i].sEntityName );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem_DynamicResupply::ComputeHealthRatios( CItem_DynamicResupply* pMaster, CBasePlayer *pPlayer, int iDebug, SpawnInfo_t *pSpawnInfo )
{
	for ( int i = 0; i < NUM_HEALTH_ITEMS; i++ )
	{
		// Figure out the current level of this resupply type
		float flMax;
		if ( i == DS_HEALTH_INDEX )
		{
			// Health
			flMax = pPlayer->GetMaxHealth();

			float flCurrentHealth = pPlayer->GetHealth() + (pSpawnInfo[i].m_iPotentialItems * sk_healthkit.GetFloat());
			pSpawnInfo[i].m_flCurrentRatio = (flCurrentHealth / flMax);
		}
		else if ( i == DS_ARMOR_INDEX )
		{
			// Armor 
			// Ignore armor if we don't have the suit
			if ( !pPlayer->IsSuitEquipped() )
			{
				pSpawnInfo[i].m_flCurrentRatio = 1.0;
			}
			else
			{
				flMax = MAX_NORMAL_BATTERY;
				float flCurrentArmor = pPlayer->ArmorValue() + (pSpawnInfo[i].m_iPotentialItems * sk_battery.GetFloat());
				pSpawnInfo[i].m_flCurrentRatio = (flCurrentArmor / flMax);
			}
		}

		pSpawnInfo[i].m_flDesiredRatio = pMaster->m_flDesiredHealth[i] * sk_dynamic_resupply_modifier.GetFloat();
		pSpawnInfo[i].m_flDelta = pSpawnInfo[i].m_flDesiredRatio - pSpawnInfo[i].m_flCurrentRatio;
		pSpawnInfo[i].m_flDelta = clamp( pSpawnInfo[i].m_flDelta, 0, 1 );
	}

	if ( iDebug )
	{
		Msg("Calculating desired health ratios & deltas:\n");
		for ( int i = 0; i < NUM_HEALTH_ITEMS; i++ )
		{
			Msg("   %s Desired Ratio: %.2f, Current Ratio: %.2f = Delta of %.2f\n", 
				g_DynamicResupplyHealthItems[i].sEntityName, pSpawnInfo[i].m_flDesiredRatio, pSpawnInfo[i].m_flCurrentRatio, pSpawnInfo[i].m_flDelta );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem_DynamicResupply::ComputeAmmoRatios( CItem_DynamicResupply* pMaster, CBasePlayer *pPlayer, int iDebug, SpawnInfo_t *pSpawnInfo )
{
	for ( int i = 0; i < NUM_AMMO_ITEMS; i++ )
	{
		// Get the ammodef's
		int iAmmoType = GetAmmoDef()->Index( g_DynamicResupplyAmmoItems[i].sAmmoDef );
		Assert( iAmmoType != -1 );

		// Ignore ammo types if we don't have a weapon that uses it (except for the grenade)
		if ( (i != DS_GRENADE_INDEX) && !pPlayer->Weapon_GetWpnForAmmo( iAmmoType ) )
		{
			pSpawnInfo[i].m_flCurrentRatio = 1.0;
		}
		else
		{
			float flMax = GetAmmoDef()->MaxCarry( iAmmoType );
			float flCurrentAmmo = pPlayer->GetAmmoCount( iAmmoType );
			flCurrentAmmo += (pSpawnInfo[i].m_iPotentialItems * g_DynamicResupplyAmmoItems[i].iAmmoCount);
			pSpawnInfo[i].m_flCurrentRatio = (flCurrentAmmo / flMax);
		}

		// Use the master if we're supposed to
		pSpawnInfo[i].m_flDesiredRatio = pMaster->m_flDesiredAmmo[i] * sk_dynamic_resupply_modifier.GetFloat();
		pSpawnInfo[i].m_flDelta = pSpawnInfo[i].m_flDesiredRatio - pSpawnInfo[i].m_flCurrentRatio;
		pSpawnInfo[i].m_flDelta = clamp( pSpawnInfo[i].m_flDelta, 0, 1 );
	}

	if ( iDebug )
	{
		Msg("Calculating desired ammo ratios & deltas:\n");
		for ( int i = 0; i < NUM_AMMO_ITEMS; i++ )
		{
			Msg("   %s Desired Ratio: %.2f, Current Ratio: %.2f = Delta of %.2f\n", 
				g_DynamicResupplyAmmoItems[i].sEntityName, pSpawnInfo[i].m_flDesiredRatio, pSpawnInfo[i].m_flCurrentRatio, pSpawnInfo[i].m_flDelta );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CItem_DynamicResupply::SpawnItemFromRatio( int nCount, DynamicResupplyItems_t *pItems, int iDebug, SpawnInfo_t *pSpawnInfo, Vector *pVecSpawnOrigin )
{
	// Now find the one we're farthest from
	float flFarthest = 0;
	int iSelectedIndex = -1;
	for ( int i = 0; i < nCount; ++i )
	{
		if ( pSpawnInfo[i].m_flDelta > flFarthest )
		{
			flFarthest = pSpawnInfo[i].m_flDelta;
			iSelectedIndex = i;
		}
	}

	if ( iSelectedIndex < 0 )
		return false;

	if ( iDebug )
	{
		Msg("Chosen item: %s (had farthest delta, %.2f)\n", pItems[iSelectedIndex].sEntityName, pSpawnInfo[iSelectedIndex].m_flDelta );
	}

	CBaseEntity *pEnt = CBaseEntity::Create( pItems[iSelectedIndex].sEntityName, *pVecSpawnOrigin, GetAbsAngles(), this );
	pEnt->SetAbsVelocity( GetAbsVelocity() );
	pEnt->SetLocalAngularVelocity( GetLocalAngularVelocity() );

	// Move the entity up so that it doesn't go below the spawn origin
	Vector vecWorldMins, vecWorldMaxs;
	pEnt->CollisionProp()->WorldSpaceAABB( &vecWorldMins, &vecWorldMaxs );
	if ( vecWorldMins.z < pVecSpawnOrigin->z )
	{
		float dz = pVecSpawnOrigin->z - vecWorldMins.z;
		pVecSpawnOrigin->z += dz;
		vecWorldMaxs.z += dz;
		pEnt->SetAbsOrigin( *pVecSpawnOrigin ); 
	}

	// Update the spawn position to spawn them on top of each other
	pVecSpawnOrigin->z = vecWorldMaxs.z + 6.0f;

	pVecSpawnOrigin->x += random->RandomFloat( -6, 6 );
	pVecSpawnOrigin->y += random->RandomFloat( -6, 6 );

	return true;
}

	
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem_DynamicResupply::SpawnDynamicItem( CBasePlayer *pPlayer )
{
	Assert( pPlayer );

	// If we're the master, we never want to spawn
	if ( g_MasterResupply == this )
		return;

	int iDebug = g_debug_dynamicresupplies.GetInt();
	if ( iDebug )
	{
		Msg("Spawning item_dynamic_resupply:\n");
	}

	SpawnInfo_t pAmmoInfo[ NUM_AMMO_ITEMS ];
	SpawnInfo_t pHealthInfo[ NUM_HEALTH_ITEMS ];

	// Count the potential addition of items in the PVS
	FindPotentialItems( NUM_HEALTH_ITEMS, g_DynamicResupplyHealthItems, iDebug, pHealthInfo );
	FindPotentialItems( NUM_AMMO_ITEMS, g_DynamicResupplyAmmoItems, iDebug, pAmmoInfo );

	// Use the master if we're supposed to
	CItem_DynamicResupply *pMaster = this;
	if ( HasSpawnFlags( SF_DYNAMICRESUPPLY_USE_MASTER ) && g_MasterResupply )
	{
		pMaster = g_MasterResupply;
	}

	// Compute desired ratios for health and ammo
	ComputeHealthRatios( pMaster, pPlayer, iDebug, pHealthInfo );
	ComputeAmmoRatios( pMaster, pPlayer, iDebug, pAmmoInfo );

	Vector vecSpawnOrigin = GetAbsOrigin();
	bool bHealthSpawned = SpawnItemFromRatio( NUM_HEALTH_ITEMS, g_DynamicResupplyHealthItems, iDebug, pHealthInfo, &vecSpawnOrigin );
	bool bAmmoSpawned = SpawnItemFromRatio( NUM_AMMO_ITEMS, g_DynamicResupplyAmmoItems, iDebug, pAmmoInfo, &vecSpawnOrigin );
	if ( !bHealthSpawned && !bAmmoSpawned )
	{
		SpawnFullItem( pMaster, pPlayer, iDebug );
	}

	SetThink( NULL );
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float DynamicResupply_GetDesiredHealthPercentage( void )
{
	// Return what the master supply dictates
	if ( g_MasterResupply != NULL )
		return g_MasterResupply->GetDesiredHealthPercentage();

	// Full health if they haven't specified otherwise
	return 1.0f;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void DynamicResupply_InitFromAlternateMaster( CBaseEntity *pTargetEnt, string_t iszMaster )
{
	if ( iszMaster== NULL_STRING )
	{
		return;
	}

	CItem_DynamicResupply *pTargetResupply = assert_cast<CItem_DynamicResupply *>( pTargetEnt );
	CBaseEntity *pMasterEnt = gEntList.FindEntityByName( NULL, iszMaster );

	if ( !pMasterEnt || !pMasterEnt->ClassMatches( pTargetResupply->GetClassname() ) )
	{
		DevWarning( "Invalid item_dynamic_resupply name %s\n", STRING( iszMaster ) );
		return;
	}

	CItem_DynamicResupply *pMasterResupply = assert_cast<CItem_DynamicResupply *>( pMasterEnt );

	pTargetResupply->RemoveSpawnFlags( SF_DYNAMICRESUPPLY_USE_MASTER );
	memcpy( pTargetResupply->m_flDesiredHealth, pMasterResupply->m_flDesiredHealth, sizeof( pMasterResupply->m_flDesiredHealth ) );
	memcpy( pTargetResupply->m_flDesiredAmmo, pMasterResupply->m_flDesiredAmmo, sizeof( pMasterResupply->m_flDesiredAmmo ) );

}