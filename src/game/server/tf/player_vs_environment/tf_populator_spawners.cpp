//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: tf_populator_spawners
// Implementations of NPC Spawning Code for PvE related game modes (MvM)
//=============================================================================//

#include "cbase.h"

#include "tf_population_manager.h"
#include "tf_team.h"
#include "tf_obj_sentrygun.h"
#include "tf_weapon_medigun.h"
#include "tf_tank_boss.h"
#include "bot/behavior/engineer/mvm_engineer/tf_bot_mvm_engineer_idle.h"

#include "etwprof.h"

extern ConVar tf_mvm_skill;
extern ConVar tf_mm_trusted;

extern ConVar tf_populator_debug;
extern ConVar tf_populator_active_buffer_range;
extern ConVar tf_mvm_miniboss_scale;

ConVar tf_debug_placement_failure( "tf_debug_placement_failure", "0", FCVAR_CHEAT );

LINK_ENTITY_TO_CLASS( populator_internal_spawn_point, CPopulatorInternalSpawnPoint );
CHandle< CPopulatorInternalSpawnPoint > g_internalSpawnPoint = NULL;

//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if a player has room to spawn at the given position
 */
bool IsSpaceToSpawnHere( const Vector &where )
{
	// make sure a player will fit here
	trace_t result;
	float bloat = 5.0f;
	UTIL_TraceHull( where, where, VEC_HULL_MIN - Vector( bloat, bloat, 0 ), VEC_HULL_MAX + Vector( bloat, bloat, bloat ), MASK_SOLID | CONTENTS_PLAYERCLIP, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &result );

	if ( tf_debug_placement_failure.GetBool() && result.fraction < 1.0f )
	{
		NDebugOverlay::Cross3D( where, 5.0f, 255, 100, 0, true, 99999.9f );
	}

	return result.fraction >= 1.0;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
/*static*/ IPopulationSpawner *IPopulationSpawner::ParseSpawner( IPopulator *populator, KeyValues *data )
{
	const char *name = data->GetName();

	if ( Q_strlen( name ) <= 0 )
	{
		return NULL;
	}

	if ( !Q_stricmp( name, "TFBot" ) )
	{
		CTFBotSpawner *botSpawner = new CTFBotSpawner( populator );

		if ( botSpawner->Parse( data ) == false )
		{
			Warning( "Warning reading TFBot spawner definition\n" );
			delete botSpawner;
			return NULL;
		}

		return botSpawner;
	}
	else if ( !Q_stricmp( name, "Tank" ) )
	{
		CTankSpawner *tankSpawner = new CTankSpawner( populator );

		if ( tankSpawner->Parse( data ) == false )
		{
			Warning( "Warning reading Tank spawner definition\n" );
			delete tankSpawner;
			return NULL;
		}

		return tankSpawner;
	}
	else if ( !Q_stricmp( name, "SentryGun" ) )
	{
		CSentryGunSpawner *sentrySpawner = new CSentryGunSpawner( populator );

		if ( sentrySpawner->Parse( data ) == false )
		{
			Warning( "Warning reading SentryGun spawner definition\n" );
			delete sentrySpawner;
			return NULL;
		}

		return sentrySpawner;
	}
	else if ( !Q_stricmp( name, "Squad" ) )
	{
		CSquadSpawner *squadSpawner = new CSquadSpawner( populator );

		if ( squadSpawner->Parse( data ) == false )
		{
			Warning( "Warning reading Squad spawner definition\n" );
			delete squadSpawner;
			return NULL;
		}

		return squadSpawner;
	}
	else if ( !Q_stricmp( name, "Mob" ) )
	{
		CMobSpawner *mobSpawner = new CMobSpawner( populator );

		if ( mobSpawner->Parse( data ) == false )
		{
			Warning( "Warning reading Mob spawner definition\n" );
			delete mobSpawner;
			return NULL;
		}

		return mobSpawner;
	}
	else if ( !Q_stricmp( name, "RandomChoice" ) )
	{
		CRandomChoiceSpawner *randomSpawner = new CRandomChoiceSpawner( populator );

		if ( randomSpawner->Parse( data ) == false )
		{
			Warning( "Warning reading RandomChoice spawner definition\n" );
			delete randomSpawner;
			return NULL;
		}

		return randomSpawner;
	}

	return NULL;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
static EventInfo *ParseEvent( KeyValues *values )
{
	EventInfo *eventInfo = new EventInfo;

	for ( KeyValues *data = values->GetFirstSubKey(); data != NULL; data = data->GetNextKey() )
	{
		const char *name = data->GetName();

		if ( Q_strlen( name ) <= 0 )
		{
			continue;
		}

		if ( !Q_stricmp( name, "Target" ) )
		{
			eventInfo->m_target.sprintf( "%s", data->GetString() );
		}
		else if ( !Q_stricmp( name, "Action" ) )
		{
			eventInfo->m_action.sprintf( "%s", data->GetString() );
		}
		else if ( !Q_stricmp( name, "Param" ) )
        {
            eventInfo->m_param.SetString( AllocPooledString( data->GetString() ) );
        }
        else if ( !Q_stricmp( name, "Delay" ) )
        {
            eventInfo->m_delay = data->GetFloat();
        }
		else
		{
			Warning( "Unknown field '%s' in WaveSpawn event definition.\n", data->GetString() );
			delete eventInfo;
			return NULL;
		}
	}

	return eventInfo;
}

//-----------------------------------------------------------------------
// CRandomChoiceSpawner
//-----------------------------------------------------------------------
CRandomChoiceSpawner::CRandomChoiceSpawner( IPopulator *populator ) : IPopulationSpawner( populator )
{
	m_spawnerVector.RemoveAll();
	m_nRandomPickDecision.RemoveAll();
	m_nNumSpawned = 0;
}


//-----------------------------------------------------------------------
CRandomChoiceSpawner::~CRandomChoiceSpawner()
{
	for( int i=0; i<m_spawnerVector.Count(); ++i )
	{
		delete m_spawnerVector[i];
	}
	m_spawnerVector.RemoveAll();
	m_nRandomPickDecision.RemoveAll();
	m_nNumSpawned = 0;
}


//-----------------------------------------------------------------------
bool CRandomChoiceSpawner::Parse( KeyValues *values )
{
	for ( KeyValues *data = values->GetFirstSubKey(); data != NULL; data = data->GetNextKey() )
	{
		const char *name = data->GetName();

		if ( Q_strlen( name ) <= 0 )
		{
			continue;
		}

		IPopulationSpawner *spawner = ParseSpawner( m_populator, data );

		if ( spawner == NULL )
		{
			Warning( "Unknown attribute '%s' in RandomChoice definition.\n", name );
		}
		else
		{
			m_spawnerVector.AddToTail( spawner );
		}
	}

	return true;
}


//-----------------------------------------------------------------------
bool CRandomChoiceSpawner::Spawn( const Vector &here, EntityHandleVector_t *result )
{
	if ( m_spawnerVector.Count() < 1 )
		return false;

	int nCurrentCount = m_nRandomPickDecision.Count();
	int nNumToAdd = ( m_nNumSpawned + 1 ) - nCurrentCount;

	if ( nNumToAdd > 0 )
	{
		m_nRandomPickDecision.AddMultipleToTail( nNumToAdd );

		for ( int i = nCurrentCount; i < m_nNumSpawned + 1; ++i )
		{
			m_nRandomPickDecision[ i ] = RandomInt( 0, m_spawnerVector.Count() - 1 );
		}
	}
	
	bool bResult = m_spawnerVector[ m_nRandomPickDecision[ m_nNumSpawned ] ]->Spawn( here, result );

	m_nNumSpawned++;

	return bResult;
}

int CRandomChoiceSpawner::GetClass( int nSpawnNum /*= -1*/ )
{
	if ( nSpawnNum < 0 )
	{
		return TF_CLASS_UNDEFINED;
	}

	int nCurrentCount = m_nRandomPickDecision.Count();
	int nNumToAdd = ( nSpawnNum + 1 ) - nCurrentCount;

	if ( nNumToAdd > 0 )
	{
		m_nRandomPickDecision.AddMultipleToTail( nNumToAdd );

		for ( int i = nCurrentCount; i < nSpawnNum + 1; ++i )
		{
			m_nRandomPickDecision[ i ] = RandomInt( 0, m_spawnerVector.Count() - 1 );
		}
	}

	if ( !m_spawnerVector[ m_nRandomPickDecision[ nSpawnNum ] ]->IsVarious() )
	{
		return m_spawnerVector[ m_nRandomPickDecision[ nSpawnNum ] ]->GetClass();
	}
	else
	{
		// FIXME: Nested complex spawner types... need a method for counting these.
		Assert( 1 );
		DevWarning( "Nested complex spawner types... need a method for counting these." );
		return TF_CLASS_UNDEFINED;
	}
}

string_t CRandomChoiceSpawner::GetClassIcon( int nSpawnNum /*= -1*/ )
{
	if ( nSpawnNum < 0 )
	{
		return NULL_STRING;
	}

	int nCurrentCount = m_nRandomPickDecision.Count();
	int nNumToAdd = ( nSpawnNum + 1 ) - nCurrentCount;

	if ( nNumToAdd > 0 )
	{
		m_nRandomPickDecision.AddMultipleToTail( nNumToAdd );

		for ( int i = nCurrentCount; i < nSpawnNum + 1; ++i )
		{
			m_nRandomPickDecision[ i ] = RandomInt( 0, m_spawnerVector.Count() - 1 );
		}
	}

	if ( !m_spawnerVector[ m_nRandomPickDecision[ nSpawnNum ] ]->IsVarious() )
	{
		return m_spawnerVector[ m_nRandomPickDecision[ nSpawnNum ] ]->GetClassIcon();
	}
	else
	{
		// FIXME: Nested complex spawner types... need a method for counting these.
		Assert( 1 );
		DevWarning( "Nested complex spawner types... need a method for counting these." );
		return NULL_STRING;
	}
}

int CRandomChoiceSpawner::GetHealth( int nSpawnNum /*= -1*/ )
{
	if ( nSpawnNum < 0 )
	{
		return 0;
	}

	int nCurrentCount = m_nRandomPickDecision.Count();
	int nNumToAdd = ( nSpawnNum + 1 ) - nCurrentCount;

	if ( nNumToAdd > 0 )
	{
		m_nRandomPickDecision.AddMultipleToTail( nNumToAdd );

		for ( int i = nCurrentCount; i < nSpawnNum + 1; ++i )
		{
			m_nRandomPickDecision[ i ] = RandomInt( 0, m_spawnerVector.Count() - 1 );
		}
	}

	if ( !m_spawnerVector[ m_nRandomPickDecision[ nSpawnNum ] ]->IsVarious() )
	{
		return m_spawnerVector[ m_nRandomPickDecision[ nSpawnNum ] ]->GetHealth();
	}
	else
	{
		// FIXME: Nested complex spawner types... need a method for counting these.
		Assert( 1 );
		DevWarning( "Nested complex spawner types... need a method for counting these." );
		return 0;
	}
}


bool CRandomChoiceSpawner::IsMiniBoss( int nSpawnNum /*= -1*/ )
{
	if ( nSpawnNum < 0 )
	{
		return false;
	}

	int nCurrentCount = m_nRandomPickDecision.Count();
	int nNumToAdd = ( nSpawnNum + 1 ) - nCurrentCount;

	if ( nNumToAdd > 0 )
	{
		m_nRandomPickDecision.AddMultipleToTail( nNumToAdd );

		for ( int i = nCurrentCount; i < nSpawnNum + 1; ++i )
		{
			m_nRandomPickDecision[ i ] = RandomInt( 0, m_spawnerVector.Count() - 1 );
		}
	}

	if ( !m_spawnerVector[ m_nRandomPickDecision[ nSpawnNum ] ]->IsVarious() )
	{
		return m_spawnerVector[ m_nRandomPickDecision[ nSpawnNum ] ]->IsMiniBoss();
	}
	else
	{
		// FIXME: Nested complex spawner types... need a method for counting these.
		Assert( 1 );
		DevWarning( "Nested complex spawner types... need a method for counting these." );
		return false;
	}
}


bool CRandomChoiceSpawner::HasAttribute( CTFBot::AttributeType type, int nSpawnNum /*= -1*/ )
{
	if ( nSpawnNum < 0 )
	{
		return false;
	}

	int nCurrentCount = m_nRandomPickDecision.Count();
	int nNumToAdd = ( nSpawnNum + 1 ) - nCurrentCount;

	if ( nNumToAdd > 0 )
	{
		m_nRandomPickDecision.AddMultipleToTail( nNumToAdd );

		for ( int i = nCurrentCount; i < nSpawnNum + 1; ++i )
		{
			m_nRandomPickDecision[ i ] = RandomInt( 0, m_spawnerVector.Count() - 1 );
		}
	}

	if ( !m_spawnerVector[ m_nRandomPickDecision[ nSpawnNum ] ]->IsVarious() )
	{
		return m_spawnerVector[ m_nRandomPickDecision[ nSpawnNum ] ]->HasAttribute( type, nSpawnNum );
	}
	else
	{
		// FIXME: Nested complex spawner types... need a method for counting these.
		Assert( 1 );
		DevWarning( "Nested complex spawner types... need a method for counting these." );
		return false;
	}
}


bool CRandomChoiceSpawner::HasEventChangeAttributes( const char* pszEventName ) const
{
	for ( int i=0; i<m_spawnerVector.Count(); ++i )
	{
		m_spawnerVector[i]->HasEventChangeAttributes( pszEventName );
	}
	
	return false;
}


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
CTFBotSpawner::CTFBotSpawner( IPopulator *populator ) : IPopulationSpawner( populator )
{
	m_class = TF_CLASS_UNDEFINED;
	m_iszClassIcon = NULL_STRING;

	m_health = -1; // default health
	m_scale = -1.0f; // default scale
	m_flAutoJumpMin = m_flAutoJumpMax = 0.f; // default AutoJumpMin/Max

	m_defaultAttributes.Reset();
}

static void ParseCharacterAttributes( CTFBot::EventChangeAttributes_t& event, KeyValues *data )
{
	CUtlVector<static_attrib_t> vecStaticAttribs;

	FOR_EACH_SUBKEY( data, pKVAttribute )
	{
		CUtlVector<CUtlString> vecErrors;

		static_attrib_t staticAttrib;
		if ( !staticAttrib.BInitFromKV_SingleLine( "CharacterAttributes", pKVAttribute, &vecErrors ) )
		{
			FOR_EACH_VEC( vecErrors, i )
			{
				Warning( "TFBotSpawner: attribute error: '%s'\n", vecErrors[i].Get() );
			}
			continue;
		}

		vecStaticAttribs.AddToTail( staticAttrib );
	}

	// found an existing entry for this name -- stomp attribute values if present or add
	// new entries if not
	FOR_EACH_VEC( vecStaticAttribs, iNewAttribute )
	{
		const static_attrib_t& newStaticAttrib = vecStaticAttribs[iNewAttribute];
		bool bAdd = true;

		int iExistingAttribute;
		for ( iExistingAttribute = 0; iExistingAttribute < event.m_characterAttributes.Count(); iExistingAttribute++ )
		{
			// matching existing attribute? stomp value
			if ( event.m_characterAttributes[iExistingAttribute].iDefIndex == newStaticAttrib.iDefIndex )
			{
				// stomp value
				event.m_characterAttributes[iExistingAttribute].m_value = newStaticAttrib.m_value;
				bAdd = false;
				// move on to next new attribute
				break;
			}
		}

		if ( bAdd )
		{
			// couldn't find? add new attribute entry
			event.m_characterAttributes.AddToTail( newStaticAttrib );
		}
	}
}

static void ParseItemAttributes( CTFBot::EventChangeAttributes_t& event, KeyValues *data )
{
	const char *pszItemName = NULL;
	CUtlVector<static_attrib_t> vecStaticAttribs;

	FOR_EACH_SUBKEY( data, pKVSubkey )
	{
		if ( !Q_stricmp( pKVSubkey->GetName(), "ItemName" ) )
		{
			if ( pszItemName )
			{
				Warning( "TFBotSpawner: \"ItemName\" field specified multiple times ('%s' / '%s').\n", pszItemName, pKVSubkey->GetString() );
			}

			pszItemName = pKVSubkey->GetString();
		}
		else
		{
			CUtlVector<CUtlString> vecErrors;

			static_attrib_t staticAttrib;
			if ( !staticAttrib.BInitFromKV_SingleLine( "ItemAttributes", pKVSubkey, &vecErrors ) )
			{
				FOR_EACH_VEC( vecErrors, i )
				{
					Warning( "TFBotSpawner: attribute error: '%s'\n", vecErrors[i].Get() );
				}
				continue;
			}

			vecStaticAttribs.AddToTail( staticAttrib );
		}
	}

	if ( !pszItemName )
	{
		Warning( "TFBotSpawner: need to specify ItemName in ItemAttributes.\n" );
		return;
	}

	// check if the item name is already on the list
	FOR_EACH_VEC( event.m_itemsAttributes, i )
	{
		CTFBot::EventChangeAttributes_t::item_attributes_t& botItemAttrs = event.m_itemsAttributes[i];

		if ( !Q_stricmp( botItemAttrs.m_itemName, pszItemName ) )
		{
			// found an existing entry for this name -- stomp attribute values if present or add
			// new entries if not
			FOR_EACH_VEC( vecStaticAttribs, iNewAttribute )
			{
				const static_attrib_t& newStaticAttrib = vecStaticAttribs[iNewAttribute];

				int iExistingAttribute;
				for ( iExistingAttribute = 0; iExistingAttribute < botItemAttrs.m_attributes.Count(); iExistingAttribute++ )
				{
					// matching existing attribute? stomp value
					if ( botItemAttrs.m_attributes[iExistingAttribute].iDefIndex == newStaticAttrib.iDefIndex )
					{
						// stomp value
						botItemAttrs.m_attributes[iExistingAttribute].m_value = newStaticAttrib.m_value;

						// move on to next new attribute
						break;
					}
				}

				// couldn't find? add new attribute entry
				botItemAttrs.m_attributes.AddToTail( newStaticAttrib );
			}

			// only one entry expected -- done here, before we add a new one
			return;
		}
	}

	// new item
	CTFBot::EventChangeAttributes_t::item_attributes_t botItemAttributes;
	botItemAttributes.m_itemName = pszItemName;
	botItemAttributes.m_attributes.AddVectorToTail( vecStaticAttribs );

	event.m_itemsAttributes.AddToTail( botItemAttributes );
}

static bool ParseDynamicAttributes( CTFBot::EventChangeAttributes_t& event, KeyValues* data )
{
	const char *name = data->GetName();
	const char *value = data->GetString();

	if ( !Q_stricmp( name, "Skill" ) )
	{
		if ( !Q_stricmp( value, "Easy" ) )
		{
			event.m_skill = CTFBot::EASY;
		}
		else if ( !Q_stricmp( value, "Normal" ) )
		{
			event.m_skill = CTFBot::NORMAL;
		}
		else if ( !Q_stricmp( value, "Hard" ) )
		{
			event.m_skill = CTFBot::HARD;
		}
		else if ( !Q_stricmp( value, "Expert" ) )
		{
			event.m_skill = CTFBot::EXPERT;
		}
		else
		{
			Warning( "TFBotSpawner: Invalid skill '%s'\n", value );
			return false;
		}	
	}
	else if ( !Q_stricmp( name, "WeaponRestrictions" ) )
	{
		if ( !Q_stricmp( value, "MeleeOnly" ) )
		{
			event.m_weaponRestriction = CTFBot::MELEE_ONLY;
		}
		else if ( !Q_stricmp( value, "PrimaryOnly" ) )
		{
			event.m_weaponRestriction = CTFBot::PRIMARY_ONLY;
		}
		else if ( !Q_stricmp( value, "SecondaryOnly" ) )
		{
			event.m_weaponRestriction = CTFBot::SECONDARY_ONLY;
		}
		else
		{
			Warning( "TFBotSpawner: Invalid weapon restriction '%s'\n", value );
			return false;
		}
	}
	else if ( !Q_stricmp( name, "BehaviorModifiers" ) )
	{
		// modifying bot attribute flags here due to legacy code
		if ( !Q_stricmp( value, "Mobber" ) || !Q_stricmp( value, "Push" ) )
		{
			event.m_attributeFlags |= CTFBot::AGGRESSIVE;
		}
		else
		{
			Warning( "TFBotSpawner: Invalid behavior modifier '%s'\n", value );
			return false;
		}
	}
	else if ( !Q_stricmp( name, "Attributes" ) )
	{
		if ( !Q_stricmp( value, "RemoveOnDeath" ) )
		{
			event.m_attributeFlags |= CTFBot::REMOVE_ON_DEATH;
		}
		else if ( !Q_stricmp( value, "Aggressive" ) )
		{
			event.m_attributeFlags |= CTFBot::AGGRESSIVE;
		}
		else if ( !Q_stricmp( value, "SuppressFire" ) )
		{
			event.m_attributeFlags |= CTFBot::SUPPRESS_FIRE;
		}
		else if ( !Q_stricmp( value, "DisableDodge" ) )
		{
			event.m_attributeFlags |= CTFBot::DISABLE_DODGE;
		}
		else if ( !Q_stricmp( value, "BecomeSpectatorOnDeath" ) )
		{
			event.m_attributeFlags |= CTFBot::BECOME_SPECTATOR_ON_DEATH;
		}
		else if ( !Q_stricmp( value, "RetainBuildings" ) )
		{
			event.m_attributeFlags |= CTFBot::RETAIN_BUILDINGS;
		}
		else if ( !Q_stricmp( value, "SpawnWithFullCharge" ) )
		{
			event.m_attributeFlags |= CTFBot::SPAWN_WITH_FULL_CHARGE;
		}
		else if ( !Q_stricmp( value, "AlwaysCrit" ) )
		{
			event.m_attributeFlags |= CTFBot::ALWAYS_CRIT;
		}
		else if ( !Q_stricmp( value, "IgnoreEnemies" ) )
		{
			event.m_attributeFlags |= CTFBot::IGNORE_ENEMIES;
		}
		else if ( !Q_stricmp( value, "HoldFireUntilFullReload" ) )
		{
			event.m_attributeFlags |= CTFBot::HOLD_FIRE_UNTIL_FULL_RELOAD;
		}
		else if ( !Q_stricmp( value, "AlwaysFireWeapon" ) )
		{
			event.m_attributeFlags |= CTFBot::ALWAYS_FIRE_WEAPON;
		}
		else if ( !Q_stricmp( value, "TeleportToHint" ) )
		{
			event.m_attributeFlags |= CTFBot::TELEPORT_TO_HINT;
		}
		else if ( !Q_stricmp( value, "MiniBoss" ) )
		{
			event.m_attributeFlags |= CTFBot::MINIBOSS;
		}
		else if ( !Q_stricmp( value, "UseBossHealthBar" ) )
		{
			event.m_attributeFlags |= CTFBot::USE_BOSS_HEALTH_BAR;
		}
		else if ( !Q_stricmp( value, "IgnoreFlag" ) )
		{
			event.m_attributeFlags |= CTFBot::IGNORE_FLAG;
		}
		else if ( !Q_stricmp( value, "AutoJump" ) )
		{
			event.m_attributeFlags |= CTFBot::AUTO_JUMP;
		}
		else if ( !Q_stricmp( value, "AirChargeOnly" ) )
		{
			event.m_attributeFlags |= CTFBot::AIR_CHARGE_ONLY;
		}
		else if( !Q_stricmp( value, "VaccinatorBullets" ) )
		{
			event.m_attributeFlags |= CTFBot::PREFER_VACCINATOR_BULLETS;
		}
		else if( !Q_stricmp( value, "VaccinatorBlast" ) )
		{
			event.m_attributeFlags |= CTFBot::PREFER_VACCINATOR_BLAST;
		}
		else if( !Q_stricmp( value, "VaccinatorFire" ) )
		{
			event.m_attributeFlags |= CTFBot::PREFER_VACCINATOR_FIRE;
		}
		else if( !Q_stricmp( value, "BulletImmune" ) )
		{
			event.m_attributeFlags |= CTFBot::BULLET_IMMUNE;
		}
		else if( !Q_stricmp( value, "BlastImmune" ) )
		{
			event.m_attributeFlags |= CTFBot::BLAST_IMMUNE;
		}
		else if( !Q_stricmp( value, "FireImmune" ) )
		{
			event.m_attributeFlags |= CTFBot::FIRE_IMMUNE;
		}
		else if ( !Q_stricmp( value, "Parachute" ) )
		{
			event.m_attributeFlags |= CTFBot::PARACHUTE;
		}
		else if ( !Q_stricmp( value, "ProjectileShield" ) )
		{
			event.m_attributeFlags |= CTFBot::PROJECTILE_SHIELD;
		}
		else
		{
			Warning( "TFBotSpawner: Invalid attribute '%s'\n", value );
			return false;
		}
	}
	else if ( !Q_stricmp( name, "MaxVisionRange" ) )
	{
		event.m_maxVisionRange = data->GetFloat();
	}
	else if ( !Q_stricmp( name, "Item" ) )
	{
		event.m_items.CopyAndAddToTail( value );
	}
	else if ( !Q_stricmp( name, "ItemAttributes" ) )
	{
		ParseItemAttributes( event, data );
	}
	else if ( !Q_stricmp( name, "CharacterAttributes" ) )
	{
		ParseCharacterAttributes( event, data );
	}
	else if ( !Q_stricmp( name, "Tag" ) )
	{
		event.m_tags.CopyAndAddToTail( value );
	}
	else
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------
bool CTFBotSpawner::Parse( KeyValues *values )
{
	// Reset All values
	m_class = TF_CLASS_UNDEFINED;
	m_iszClassIcon = NULL_STRING;
	
	m_health = -1; // default health
	m_scale = -1.0f; // default scale
	m_flAutoJumpMin = m_flAutoJumpMax = 0.f; // default AutoJumpMin/Max

	m_defaultAttributes.Reset();
	m_eventChangeAttributes.RemoveAll();

	// First, see if we have any Template key
	KeyValues *pTemplate = values->FindKey("Template");
	if ( pTemplate )
	{
		KeyValues *pTemplateKV = GetPopulator()->GetManager()->GetTemplate( pTemplate->GetString() );
		if ( pTemplateKV )
		{
			// Pump all the keys into ourself now
			if ( Parse( pTemplateKV ) == false )
			{
				return false;
			}
		}
		else
		{
			Warning( "Unknown Template '%s' in TFBotSpawner definition\n", pTemplate->GetString() );
		}
	}

	for ( KeyValues *data = values->GetFirstSubKey(); data != NULL; data = data->GetNextKey() )
	{
		const char *name = data->GetName();
		const char *value = data->GetString();

		if ( Q_strlen( name ) <= 0 )
		{
			continue;
		}

		// Skip templates when looping through the rest of the keys
		if ( !Q_stricmp( name, "Template" ) )
			continue;

		if ( !Q_stricmp( name, "Class" ) )
		{
			m_class = GetClassIndexFromString( value );
			if ( m_class == TF_CLASS_UNDEFINED )
			{
				Warning( "TFBotSpawner: Invalid class '%s'\n", value );
				return false;
			}
			if ( m_name.IsEmpty() )
			{
				m_name = value;
			}
		}
		else if ( !Q_stricmp( name, "ClassIcon" ) )
		{
			m_iszClassIcon = AllocPooledString( value );
		}
		else if ( !Q_stricmp( name, "Health" ) )
		{
			m_health = data->GetInt();
		}
		else if ( !Q_stricmp( name, "Scale" ) )
		{
			m_scale = data->GetFloat();
		}
		else if ( !Q_stricmp( name, "Name" ) )
		{
			m_name = value;
		}
		else if ( !Q_stricmp( name, "TeleportWhere" ) )
		{
			m_teleportWhereName.CopyAndAddToTail( data->GetString() );
		}
		else if ( !Q_stricmp( name, "AutoJumpMin" ) )
		{
			m_flAutoJumpMin = data->GetFloat();
		}
		else if ( !Q_stricmp( name, "AutoJumpMax" ) )
		{
			m_flAutoJumpMax = data->GetFloat();
		}
		else if ( !Q_stricmp( name, "EventChangeAttributes" ) )
		{
			if ( !ParseEventChangeAttributes( data ) )
			{
				Warning( "TFBotSpawner: Failed to parse EventChangeAttributes\n" );
				return false;
			}
		}
		else if ( ParseDynamicAttributes( m_defaultAttributes, data ) )
		{
			// Do nothing on success
		}
		else
		{
			Warning( "TFBotSpawner: Unknown field '%s'\n", name );
			return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------
bool CTFBotSpawner::ParseEventChangeAttributes( KeyValues *data )
{
	for ( KeyValues *pKVEvent = data->GetFirstSubKey(); pKVEvent != NULL; pKVEvent = pKVEvent->GetNextKey() )
	{
		const char* pszEventName = pKVEvent->GetName();

		// Add new event
		int index = m_eventChangeAttributes.AddToTail();
		CTFBot::EventChangeAttributes_t& event = m_eventChangeAttributes[index];
		event.m_eventName = pszEventName;

		for ( KeyValues *pAttr=pKVEvent->GetFirstSubKey(); pAttr != NULL; pAttr = pAttr->GetNextKey() )
		{
			if ( !ParseDynamicAttributes( event, pAttr ) )
			{
				Warning( "TFBotSpawner EventChangeAttributes: Failed to parse event '%s' with unknown attribute '%s'\n", pKVEvent->GetName(), pAttr->GetName() );
				return false;
			}
		}

		// should override default attr?
		if ( !Q_stricmp( pszEventName, "default" ) )
		{
			m_defaultAttributes = event;
		}
	}

	return true;
}


//-----------------------------------------------------------------------
bool CTFBotSpawner::Spawn( const Vector &rawHere, EntityHandleVector_t *result )
{
	CETWScope timer( "CTFBotSpawner::Spawn" );
	VPROF_BUDGET( "CTFBotSpawner::Spawn", "NextBot" );

	CTFBot *newBot = NULL;

	Vector here = rawHere;

	CTFNavArea *area = (CTFNavArea *)TheTFNavMesh()->GetNavArea( here );
	if ( area && area->HasAttributeTF( TF_NAV_NO_SPAWNING ) )
	{
		if ( tf_populator_debug.GetBool() ) 
		{
			DevMsg( "CTFBotSpawner: %3.2f: *** Tried to spawn in a NO_SPAWNING area at (%f, %f, %f)\n", gpGlobals->curtime, here.x, here.y, here.z );
		}
		return false;
	}

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		// Only spawn bots while the round is running in MVM mode
		if ( TFGameRules()->State_Get() != GR_STATE_RND_RUNNING )
			return false;
	}

	// the ground may be variable here, try a few heights
	float z;
	for( z = 0.0f; z<StepHeight; z += 4.0f )
	{
		here.z = rawHere.z + StepHeight;

		if ( IsSpaceToSpawnHere( here ) )
		{
			break;
		}
	}

	if ( z >= StepHeight )
	{
		if ( tf_populator_debug.GetBool() ) 
		{
			DevMsg( "CTFBotSpawner: %3.2f: *** No space to spawn at (%f, %f, %f)\n", gpGlobals->curtime, here.x, here.y, here.z );
		}
		return false;
	}

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		if ( m_class == TF_CLASS_ENGINEER && m_defaultAttributes.m_attributeFlags & CTFBot::TELEPORT_TO_HINT && CTFBotMvMEngineerHintFinder::FindHint( true, false ) == false )
		{
			if ( tf_populator_debug.GetBool() ) 
			{
				DevMsg( "CTFBotSpawner: %3.2f: *** No teleporter hint for engineer\n", gpGlobals->curtime );
			}

			return false;
		}
	}


	// find dead bot we can re-use
	CTeam *deadTeam = GetGlobalTeam( TEAM_SPECTATOR );
	for( int i=0; i<deadTeam->GetNumPlayers(); ++i )
	{
		if ( !deadTeam->GetPlayer(i)->IsBot() )
			continue;

		// reuse this guy
		newBot = (CTFBot *)deadTeam->GetPlayer(i);
		newBot->ClearAllAttributes();
		break;
	}

	if ( newBot == NULL )
	{
		//AssertMsg( 0, "Bots should be preallocated. This block of code should never get called.\n" );
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			int nNumEnemyBots = 0;

			CUtlVector<CTFPlayer *> botVector;
			CPopulationManager::CollectMvMBots( &botVector );

			nNumEnemyBots = botVector.Count();

			if ( nNumEnemyBots >= tf_mvm_max_invaders.GetInt() )
			{
				// no room for more
				if ( tf_populator_debug.GetBool() ) 
				{
					DevMsg( "CTFBotSpawner: %3.2f: *** Can't spawn. Max number invaders already spawned.\n", gpGlobals->curtime );
				}

				// extra guard if we're over full on bots
				if ( nNumEnemyBots > tf_mvm_max_invaders.GetInt() )
				{
					// Kick bots until we are at the proper number starting with spectator bots
					int iNumberToKick = nNumEnemyBots - tf_mvm_max_invaders.GetInt();
					int iKickedBots = 0;

					// loop through spectators and invaders in that order
					// Kick Spectators first
					CUtlVector<CTFPlayer *> botsToKick;
					for ( int iTeam = 0; iTeam < 2; iTeam++ )
					{
						int targetTeam = TEAM_SPECTATOR;
						if ( iTeam == 1 )
						{
							targetTeam = TF_TEAM_PVE_INVADERS;
						}

						FOR_EACH_VEC( botVector, iBot )
						{
							if ( iKickedBots >= iNumberToKick )
								break;

							if ( botVector[iBot]->GetTeamNumber() == targetTeam )
							{
								botsToKick.AddToTail( botVector[iBot] );
								iKickedBots++;
							}
						}
					}

					FOR_EACH_VEC ( botsToKick, iKick )
					{
						engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", botsToKick[iKick]->GetUserID() ) );
					}
				}

				return false;
			}
		}
		else
		{
			// need to add another bot - is there room?
			int totalPlayerCount = 0;
			totalPlayerCount += GetGlobalTeam( TEAM_SPECTATOR )->GetNumPlayers();
			totalPlayerCount += GetGlobalTeam( TF_TEAM_BLUE )->GetNumPlayers();
			totalPlayerCount += GetGlobalTeam( TF_TEAM_RED )->GetNumPlayers();

			if ( totalPlayerCount >= gpGlobals->maxClients )
			{
				// no room for more
				if ( tf_populator_debug.GetBool() ) 
				{
					DevMsg( "CTFBotSpawner: %3.2f: *** Can't spawn. No free player slot.\n", gpGlobals->curtime );
				}
				return false;
			}
		}

		newBot = NextBotCreatePlayerBot< CTFBot >( "TFBot", false );
	}

	if ( newBot ) 
	{
		// remove any player attributes
		newBot->RemovePlayerAttributes( false );

		// clear any old TeleportWhere settings 
		newBot->ClearTeleportWhere();

		if ( g_internalSpawnPoint == NULL )
		{
			g_internalSpawnPoint = (CPopulatorInternalSpawnPoint *)CreateEntityByName( "populator_internal_spawn_point" );
			g_internalSpawnPoint->Spawn();
		}

		// set name
		engine->SetFakeClientConVarValue( newBot->edict(), "name", m_name.IsEmpty() ? "TFBot" : m_name.Get() );

		g_internalSpawnPoint->SetAbsOrigin( here );
		g_internalSpawnPoint->SetLocalAngles( vec3_angle );
		newBot->SetSpawnPoint( g_internalSpawnPoint );

		int team = TF_TEAM_RED;

		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			team = TF_TEAM_PVE_INVADERS;
		}

		newBot->ChangeTeam( team, false, true );

		newBot->AllowInstantSpawn();
		newBot->HandleCommand_JoinClass( GetPlayerClassData( m_class )->m_szClassName );
		newBot->GetPlayerClass()->SetClassIconName( GetClassIcon() );

		newBot->ClearEventChangeAttributes();
		for ( int i=0; i<m_eventChangeAttributes.Count(); ++i )
		{
			newBot->AddEventChangeAttributes( &m_eventChangeAttributes[i] );
		}

		// Request to Add in Endless
		if ( g_pPopulationManager->IsInEndlessWaves() )
		{
			g_pPopulationManager->EndlessSetAttributesForBot( newBot );
		}

		newBot->SetTeleportWhere( m_teleportWhereName );

		if ( m_defaultAttributes.m_attributeFlags & CTFBot::MINIBOSS )
		{
			newBot->SetIsMiniBoss( true );
		}

		if ( m_defaultAttributes.m_attributeFlags & CTFBot::USE_BOSS_HEALTH_BAR )
		{
			newBot->SetUseBossHealthBar( true );
		}

		if ( m_defaultAttributes.m_attributeFlags & CTFBot::AUTO_JUMP )
		{
			newBot->SetAutoJump( m_flAutoJumpMin, m_flAutoJumpMax );
		}

		if( m_defaultAttributes.m_attributeFlags & CTFBot::BULLET_IMMUNE )
		{
			newBot->m_Shared.AddCond( TF_COND_BULLET_IMMUNE );
		}

		if( m_defaultAttributes.m_attributeFlags & CTFBot::BLAST_IMMUNE )
		{
			newBot->m_Shared.AddCond( TF_COND_BLAST_IMMUNE );
		}

		if( m_defaultAttributes.m_attributeFlags & CTFBot::FIRE_IMMUNE )
		{
			newBot->m_Shared.AddCond( TF_COND_FIRE_IMMUNE );
		}

		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			// initialize currency to be dropped on death to zero
			newBot->SetCurrency( 0 );

			// announce Spies
			if ( m_class == TF_CLASS_SPY )
			{
				CUtlVector< CTFPlayer * > playerVector;
				CollectPlayers( &playerVector, TF_TEAM_PVE_INVADERS, COLLECT_ONLY_LIVING_PLAYERS );

				int spyCount = 0;
				for( int i=0; i<playerVector.Count(); ++i )
				{
					if ( playerVector[i]->IsPlayerClass( TF_CLASS_SPY ) )
					{
						++spyCount;
					}
				}

				IGameEvent *event = gameeventmanager->CreateEvent( "mvm_mission_update" );
				if ( event )
				{
					event->SetInt( "class", TF_CLASS_SPY );
					event->SetInt( "count", spyCount );
					gameeventmanager->FireEvent( event );
				}
			}
		}

		newBot->SetScaleOverride( m_scale );

		int nHealth = m_health;

		if ( nHealth <= 0.0f )
		{
			nHealth = newBot->GetMaxHealth();
		}

		nHealth *= g_pPopulationManager->GetHealthMultiplier( false );
		newBot->ModifyMaxHealth( nHealth );

		newBot->StartIdleSound();

		// Add our items first, they'll get replaced below by the normal MvM items if any are needed
		if ( TFGameRules()->IsMannVsMachineMode() && ( newBot->GetTeamNumber() == TF_TEAM_PVE_INVADERS ) )
		{
			// Apply the Rome 2 promo items to each bot. They'll be 
			// filtered out for clients that do not have Romevision.
			CMissionPopulator *pMission = dynamic_cast< CMissionPopulator* >( GetPopulator() );
			if ( pMission && ( pMission->GetMissionType() == CTFBot::MISSION_DESTROY_SENTRIES ) )
			{
				newBot->AddItem( "tw_sentrybuster" );
			}
			else
			{
				newBot->AddItem( g_szRomePromoItems_Hat[m_class] );
				newBot->AddItem( g_szRomePromoItems_Misc[m_class] );
			}
		}

		// apply default attributes
		const CTFBot::EventChangeAttributes_t* pEventChangeAttributes = newBot->GetEventChangeAttributes( g_pPopulationManager->GetDefaultEventChangeAttributesName() );
		if ( !pEventChangeAttributes )
		{
			pEventChangeAttributes = &m_defaultAttributes;
		}
		newBot->OnEventChangeAttributes( pEventChangeAttributes );

		CCaptureFlag *pFlag = newBot->GetFlagToFetch();
		if ( pFlag )
		{
			newBot->SetFlagTarget( pFlag );
		}

		if ( newBot->HasAttribute( CTFBot::SPAWN_WITH_FULL_CHARGE ) )
		{
			// charge up our weapons

			// Medigun Ubercharge
			CWeaponMedigun *medigun = dynamic_cast< CWeaponMedigun * >( newBot->Weapon_GetSlot( TF_WPN_TYPE_SECONDARY ) );
			if ( medigun )
			{
				medigun->AddCharge( 1.0f );
			}

			newBot->m_Shared.SetRageMeter( 100.0f );
		}

		int nClassIndex = ( newBot->GetPlayerClass() ? newBot->GetPlayerClass()->GetClassIndex() : TF_CLASS_UNDEFINED );

		if ( GetPopulator()->GetManager()->IsPopFileEventType( MVM_EVENT_POPFILE_HALLOWEEN ) )
		{
			// zombies use the original player models
			newBot->m_nSkin = 4;

			const char *name = g_aRawPlayerClassNamesShort[ nClassIndex ];

			newBot->AddItem( CFmtStr( "Zombie %s", name ) );
		}
		else
		{
			// use the nifty new robot model
			if ( nClassIndex >= TF_CLASS_SCOUT && nClassIndex <= TF_CLASS_ENGINEER )
			{
				if ( ( m_scale >= tf_mvm_miniboss_scale.GetFloat() || newBot->IsMiniBoss() ) && g_pFullFileSystem->FileExists( g_szBotBossModels[ nClassIndex ] ) )
				{
					newBot->GetPlayerClass()->SetCustomModel( g_szBotBossModels[ nClassIndex ], USE_CLASS_ANIMATIONS );
					newBot->UpdateModel();
					newBot->SetBloodColor( DONT_BLEED );
				}
				else if ( g_pFullFileSystem->FileExists( g_szBotModels[ nClassIndex ] ) )
				{
					newBot->GetPlayerClass()->SetCustomModel( g_szBotModels[ nClassIndex ], USE_CLASS_ANIMATIONS );
					newBot->UpdateModel();
					newBot->SetBloodColor( DONT_BLEED );
				}
			}
		}

		if ( result )
		{
			result->AddToTail( newBot );
		}

		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			if ( newBot->IsMiniBoss() )
			{
				TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_GIANT_CALLOUT, TF_TEAM_PVE_DEFENDERS );
			}
		}

		if ( tf_populator_debug.GetBool() )
		{
			if ( tf_populator_debug.GetBool() ) 
			{
				DevMsg( "%3.2f: Spawned TFBot '%s'\n", gpGlobals->curtime, m_name.IsEmpty() ? newBot->GetPlayerClass()->GetName() : m_name.Get() );
			}
		}
	}
	else
	{
		if ( tf_populator_debug.GetBool() )
		{
			DevMsg( "CTFBotSpawner: %3.2f: *** Can't create TFBot to spawn.\n", gpGlobals->curtime );
		}

		return false;
	}

	return true;
}

//-----------------------------------------------------------------------
int CTFBotSpawner::GetClass( int nSpawnNum /*= -1*/ )
{
	return m_class;
}

//-----------------------------------------------------------------------
string_t CTFBotSpawner::GetClassIcon( int nSpawnNum /*= -1*/ )
{
	if ( m_iszClassIcon != NULL_STRING )
		return m_iszClassIcon;

	return AllocPooledString( g_aRawPlayerClassNamesShort[ m_class ] );
}

//-----------------------------------------------------------------------
int CTFBotSpawner::GetHealth( int nSpawnNum /*= -1*/ )
{
	return m_health;
}

//-----------------------------------------------------------------------
bool CTFBotSpawner::IsMiniBoss( int nSpawnNum /*= -1*/ )
{
	return ( m_defaultAttributes.m_attributeFlags & CTFBot::MINIBOSS ) != 0;
}

//-----------------------------------------------------------------------
bool CTFBotSpawner::HasAttribute( CTFBot::AttributeType type, int nSpawnNum /*= -1*/ ) 
{ 
	return ( m_defaultAttributes.m_attributeFlags & type ) != 0;
}

//-----------------------------------------------------------------------
bool CTFBotSpawner::HasEventChangeAttributes( const char* pszEventName ) const
{
	for ( int i=0; i<m_eventChangeAttributes.Count(); ++i )
	{
		if ( FStrEq( pszEventName, m_eventChangeAttributes[i].m_eventName ) )
		{
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------
// CTankSpawner
//-----------------------------------------------------------------------
CTankSpawner::CTankSpawner( IPopulator *populator ) : IPopulationSpawner( populator )
{
	m_health = 50000;
	m_speed = 75;
	m_name = "Tank";
	m_skin = 0;
	m_startingPathTrackNodeName = NULL;
	m_onKilledOutput = NULL;
	m_onBombDroppedOutput = NULL;
}


//-----------------------------------------------------------------------
bool CTankSpawner::Parse( KeyValues *values )
{
	for ( KeyValues *data = values->GetFirstSubKey(); data != NULL; data = data->GetNextKey() )
	{
		const char *name = data->GetName();

		if ( Q_strlen( name ) <= 0 )
		{
			continue;
		}

		if ( !Q_stricmp( name, "Health" ) )
		{
			m_health = data->GetInt();
		}
		else if ( !Q_stricmp( name, "Speed" ) )
		{
			m_speed = data->GetFloat();
		}
		else if ( !Q_stricmp( name, "Name" ) )
		{
			m_name = data->GetString();
		}
		else if ( !Q_stricmp( name, "Skin" ) )
		{
			m_skin = data->GetInt();
		}
		else if ( !Q_stricmp( name, "StartingPathTrackNode" ) )
		{
			m_startingPathTrackNodeName = data->GetString();
		}
		else if ( !Q_stricmp( name, "OnKilledOutput" ) )
		{
			m_onKilledOutput = ParseEvent( data );
		}
		else if ( !Q_stricmp( name, "OnBombDroppedOutput" ) )
		{
			m_onBombDroppedOutput = ParseEvent( data );
		}
		else
		{
			Warning( "Invalid attribute '%s' in Tank definition\n", name );
			return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------
bool CTankSpawner::Spawn( const Vector &here, EntityHandleVector_t *result )
{
	CTFTankBoss *tank = (CTFTankBoss *)CreateEntityByName( "tank_boss" );
	if ( tank )
	{
		tank->SetAbsOrigin( here );
		tank->SetAbsAngles( vec3_angle );

		int nHealth = m_health * g_pPopulationManager->GetHealthMultiplier( true );
		tank->SetInitialHealth( nHealth );

		tank->SetMaxSpeed( m_speed );
		tank->SetName( MAKE_STRING( m_name ) );
		tank->SetSkin( m_skin );
		tank->SetStartingPathTrackNode( m_startingPathTrackNodeName.GetForModify() );

		tank->Spawn();

		tank->DefineOnKilledOutput( m_onKilledOutput );
		tank->DefineOnBombDroppedOutput( m_onBombDroppedOutput );

		if ( result )
		{
			result->AddToTail( tank );
		}

		return true;
	}

	if ( tf_populator_debug.GetBool() )
	{
		DevMsg( "CTankSpawner: %3.2f: Failed to create base_boss\n", gpGlobals->curtime );
	}
	return false;
}


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
CSentryGunSpawner::CSentryGunSpawner( IPopulator *populator ) : IPopulationSpawner( populator )
{
	m_level = 0;
}


//-----------------------------------------------------------------------
bool CSentryGunSpawner::Parse( KeyValues *values )
{
	for ( KeyValues *data = values->GetFirstSubKey(); data != NULL; data = data->GetNextKey() )
	{
		const char *name = data->GetName();

		if ( Q_strlen( name ) <= 0 )
		{
			continue;
		}

		if ( !Q_stricmp( name, "Level" ) )
		{
			m_level = data->GetInt();
		}
		else
		{
			Warning( "Invalid attribute '%s' in SentryGun definition\n", name );
			return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------
bool CSentryGunSpawner::Spawn( const Vector &here, EntityHandleVector_t *result )
{
	// directly create a sentry gun at the precise position and orientation desired
	CObjectSentrygun *sentry = (CObjectSentrygun *)CreateEntityByName( "obj_sentrygun" );
	if ( sentry )
	{
		sentry->SetAbsOrigin( here );
		sentry->SetAbsAngles( vec3_angle );

		sentry->Spawn();
		sentry->ChangeTeam( TF_TEAM_RED );

		sentry->m_nDefaultUpgradeLevel = m_level+1;

		sentry->InitializeMapPlacedObject();

		if ( result )
		{
			result->AddToTail( sentry );
		}

		return true;
	}

	if ( tf_populator_debug.GetBool() )
	{
		DevMsg( "CSentryGunSpawner: %3.2f: Failed to create obj_sentrygun\n", gpGlobals->curtime );
	}
	return false;
}


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
CSquadSpawner::CSquadSpawner( IPopulator *populator ) : IPopulationSpawner( populator )
{
	m_memberSpawnerVector.RemoveAll();
	m_formationSize = -1.0f;
	m_bShouldPreserveSquad = false;
}


//-----------------------------------------------------------------------
CSquadSpawner::~CSquadSpawner()
{
	for( int i=0; i<m_memberSpawnerVector.Count(); ++i )
	{
		delete m_memberSpawnerVector[i];
	}
	m_memberSpawnerVector.RemoveAll();
}


//-----------------------------------------------------------------------
bool CSquadSpawner::Parse( KeyValues *values )
{
	for ( KeyValues *data = values->GetFirstSubKey(); data != NULL; data = data->GetNextKey() )
	{
		const char *name = data->GetName();

		if ( V_strlen( name ) <= 0 )
		{
			continue;
		}

		if ( !V_stricmp( name, "FormationSize" ) )
		{
			m_formationSize = data->GetFloat();
			continue;
		}
		else if ( !V_stricmp( name, "ShouldPreserveSquad" ) )
		{
			m_bShouldPreserveSquad = data->GetBool();
			continue;
		}

		// NOTE: It doesn't make sense for Squads to contain SentryGun or Mobs, but this
		// allows for interesting trees of RandomChoice, etc.
		IPopulationSpawner *spawner = ParseSpawner( GetPopulator(), data );

		if ( spawner == NULL )
		{
			Warning( "Unknown attribute '%s' in Squad definition.\n", name );
		}
		else
		{
			m_memberSpawnerVector.AddToTail( spawner );
		}
	}

	return true;
}


//-----------------------------------------------------------------------
bool CSquadSpawner::Spawn( const Vector &here, EntityHandleVector_t *result )
{
	VPROF_BUDGET( "CSquadSpawner::Spawn", "NextBot" );

	if ( tf_populator_debug.GetBool() )
	{
		DevMsg( "CSquadSpawner: %3.2f: <<<< Spawning Squad >>>>\n", gpGlobals->curtime );
	}

	// Is there enough slots to spawn?
	CTeam *deadTeam = GetGlobalTeam( TEAM_SPECTATOR );
	if ( deadTeam->GetNumPlayers() < m_memberSpawnerVector.Count() )
	{
		return false;
	}

	// spawn all of the squad members
	bool isComplete = true;
	EntityHandleVector_t squadVector;
	for( int i=0; i<m_memberSpawnerVector.Count(); ++i )
	{
		if ( m_memberSpawnerVector[i]->Spawn( here, &squadVector ) == false )
		{
			isComplete = false;
			break;
		}
	}

	if ( !isComplete )
	{
		// unable to spawn entire squad
		if ( tf_populator_debug.GetBool() )
		{
			DevMsg( "%3.2f: CSquadSpawner: Unable to spawn entire squad\n", gpGlobals->curtime );
		}

		// unspawn partial squad
		// TODO: Respect TFBot attributes
		for( int i=0; i<squadVector.Count(); ++i )
		{
			CTFPlayer *player = ToTFPlayer( squadVector[i] );

			if ( player )
			{
				player->ChangeTeam( TEAM_SPECTATOR, false, true );
			}
			else
			{
				UTIL_Remove( squadVector[i] );
			}
		}

		return false;
	}

	// create the squad
	CTFBotSquad *squad = new CTFBotSquad;
	if ( squad )
	{
		squad->SetFormationSize( m_formationSize );
		squad->SetShouldPreserveSquad( m_bShouldPreserveSquad );

		for( int i=0; i<squadVector.Count(); ++i )
		{
			CTFBot *bot = ToTFBot( squadVector[i] );
			if ( bot )
			{
				bot->JoinSquad( squad );
			}
		}
	}

	if ( result )
	{
		result->AddVectorToTail( squadVector );
	}

	return true;
}

int CSquadSpawner::GetClass( int nSpawnNum /*= -1*/ )
{
	if ( nSpawnNum < 0 || m_memberSpawnerVector.Count() == 0 )
	{
		return TF_CLASS_UNDEFINED;
	}

	int nSpawner = nSpawnNum % m_memberSpawnerVector.Count();

	if ( !m_memberSpawnerVector[ nSpawner ]->IsVarious() )
	{
		return m_memberSpawnerVector[ nSpawner ]->GetClass();
	}
	else
	{
		// FIXME: Nested complex spawner types... need a method for counting these.
		Assert( 1 );
		DevWarning( "Nested complex spawner types... need a method for counting these." );
		return TF_CLASS_UNDEFINED;
	}
}

string_t CSquadSpawner::GetClassIcon( int nSpawnNum /*= -1*/ )
{
	if ( nSpawnNum < 0 || m_memberSpawnerVector.Count() == 0 )
	{
		return NULL_STRING;
	}

	int nSpawner = nSpawnNum % m_memberSpawnerVector.Count();

	if ( !m_memberSpawnerVector[ nSpawner ]->IsVarious() )
	{
		return m_memberSpawnerVector[ nSpawner ]->GetClassIcon();
	}
	else
	{
		// FIXME: Nested complex spawner types... need a method for counting these.
		Assert( 1 );
		DevWarning( "Nested complex spawner types... need a method for counting these." );
		return NULL_STRING;
	}
}

int CSquadSpawner::GetHealth( int nSpawnNum /*= -1*/ )
{
	if ( nSpawnNum < 0 || m_memberSpawnerVector.Count() == 0 )
	{
		return 0;
	}

	int nSpawner = nSpawnNum % m_memberSpawnerVector.Count();

	if ( !m_memberSpawnerVector[ nSpawner ]->IsVarious() )
	{
		return m_memberSpawnerVector[ nSpawner ]->GetHealth();
	}
	else
	{
		// FIXME: Nested complex spawner types... need a method for counting these.
		Assert( 1 );
		DevWarning( "Nested complex spawner types... need a method for counting these." );
		return 0;
	}
}

bool CSquadSpawner::IsMiniBoss( int nSpawnNum /*= -1*/ )
{
	if ( nSpawnNum < 0 || m_memberSpawnerVector.Count() == 0 )
	{
		return false;
	}

	int nSpawner = nSpawnNum % m_memberSpawnerVector.Count();

	if ( !m_memberSpawnerVector[ nSpawner ]->IsVarious() )
	{
		return m_memberSpawnerVector[ nSpawner ]->IsMiniBoss();
	}
	else
	{
		// FIXME: Nested complex spawner types... need a method for counting these.
		Assert( 1 );
		DevWarning( "Nested complex spawner types... need a method for counting these." );
		return false;
	}
}

bool CSquadSpawner::HasAttribute( CTFBot::AttributeType type, int nSpawnNum /*= -1*/ )
{
	if ( nSpawnNum < 0 || m_memberSpawnerVector.Count() == 0 )
	{
		return false;
	}

	int nSpawner = nSpawnNum % m_memberSpawnerVector.Count();

	if ( !m_memberSpawnerVector[ nSpawner ]->IsVarious() )
	{
		return m_memberSpawnerVector[ nSpawner ]->HasAttribute( type, nSpawnNum );
	}
	else
	{
		// FIXME: Nested complex spawner types... need a method for counting these.
		Assert( 1 );
		DevWarning( "Nested complex spawner types... need a method for counting these." );
		return false;
	}
}

bool CSquadSpawner::HasEventChangeAttributes( const char* pszEventName ) const
{
	for ( int i=0; i<m_memberSpawnerVector.Count(); ++i )
	{
		if ( m_memberSpawnerVector[i]->HasEventChangeAttributes( pszEventName ) )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
CMobSpawner::CMobSpawner( IPopulator *populator ) : IPopulationSpawner( populator )
{
	m_count = 0;
	m_spawner = NULL;

	m_mobSpawnTimer.Invalidate();
	m_mobLifetimeTimer.Invalidate();
	m_mobArea = NULL;
	m_mobCountRemaining = 0;
}


//-----------------------------------------------------------------------
CMobSpawner::~CMobSpawner()
{
	if ( m_spawner )
	{
		delete m_spawner;
	}
	m_spawner = NULL;
}


//-----------------------------------------------------------------------
bool CMobSpawner::Parse( KeyValues *values )
{
	for ( KeyValues *data = values->GetFirstSubKey(); data != NULL; data = data->GetNextKey() )
	{
		const char *name = data->GetName();

		if ( Q_strlen( name ) <= 0 )
		{
			continue;
		}

		if ( !Q_stricmp( name, "Count" ) )
		{
			m_count = data->GetInt();
		}
		else
		{
			// NOTE: It doesn't make sense for Mobs to contain SentryGuns, but this
			// allows for interesting trees of RandomChoice, etc.
			IPopulationSpawner *spawner = ParseSpawner( GetPopulator(), data );

			if ( spawner && m_spawner )
			{
				Warning( "CMobSpawner: Duplicate spawner encountered - discarding!\n" );
				delete spawner;
			}
			else if ( spawner == NULL )
			{
				Warning( "Unknown attribute '%s' in Mob definition.\n", name );
			}
			else
			{
				m_spawner = spawner;
			}
		}
	}

	return true;
}


//-----------------------------------------------------------------------
bool CMobSpawner::Spawn( const Vector &here, EntityHandleVector_t *result )
{
	if ( m_spawner == NULL )
		return false;

	// spawn the mob
	for( int i=0; i<m_count; ++i )
	{
		if ( m_spawner->Spawn( here, result ) == false )
		{
			return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------
bool CMobSpawner::HasEventChangeAttributes( const char* pszEventName ) const
{
	if ( m_spawner == NULL )
		return false;

	return m_spawner->HasEventChangeAttributes( pszEventName );
}
