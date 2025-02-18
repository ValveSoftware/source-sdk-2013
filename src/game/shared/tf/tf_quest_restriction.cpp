//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "KeyValues.h"
#include "schemainitutils.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include <igameresources.h>
#include "tf_gc_client.h"
#else
#include "tf_player.h"
#include "iscorer.h"
#endif // #ifdef CLIENT_DLL

#include "tf_quest_restriction.h"
#include "tf_gamerules.h"
#include "tf_item_schema.h"
#include "econ_item_system.h"
#include "tf_logic_robot_destruction.h"
#include "tier2/fileutils.h"
#include "steamworks_gamestats.h"
#include "tf_quickplay_shared.h"
#include "econ_wearable.h"

static const int s_nMinConnectedPlayersForQuestProgress = 2;
static const int s_nMaxInputCount = 100;
static const char *g_skQuestEventsFile = "tf/scripts/items/unencrypted/_quest_events.txt";

template<typename CTFQuestConditionSubClass_t>
CTFQuestCondition *CreateCTFQuestConditionSubClass()
{
	return new CTFQuestConditionSubClass_t();
}
typedef CTFQuestCondition *(*pfnQuestCreate)();

#define FIELD_NONE						0
#define FIELD_PLAYER					1<<0
#define FIELD_OBJECT					1<<1
#define FIELD_WEAPON_NAME				1<<2
#define FIELD_SCORER					1<<3
#define FIELD_CUSTOM_DAMAGE				1<<4
#define FIELD_WEAPON_TYPE				1<<5
#define FIELD_FLAG_EVENT				1<<6
#define FIELD_TEAM						1<<7
#define FIELD_LOADOUT_POSITION			1<<8
#define FIELD_CONDITION					1<<9
#define FIELD_CRIT						1<<10
#define FIELD_WEAPON_DEF_INDEX			1<<11
#define FIELD_HALLOWEEN_BOSS_TYPE		1<<12
#define FIELD_HALLOWEEN_MINIGAME_TYPE	1<<13
#define FIELD_WEAPON_CLASS				1<<14
#define FIELD_BONUSEFFECT				1<<15
#define FIELD_DEFLECTED_PROJECTILE		1<<16
#define FIELD_NUM_HIT					1<<17
#define FIELD_NUM_DIRECT_HIT			1<<18
#define FIELD_VAR						1<<19
#define FIELD_LAST_FIELD				FIELD_VAR


const char* k_pszQuestConditionRequiredFieldStrings[] =
{
	"player",				// FIELD_PLAYER
	"object_type",			// FIELD_OBJECT
	"weapon_name",			// FIELD_WEAPON_NAME
	"scorer",				// FIELD_SCORER
	"custom_damage",		// FIELD_CUSTOM_DAMAGE
	"weapon_type",			// FIELD_WEAPON_TYPE
	"flag_event",			// FIELD_FLAG_EVENT
	"team_restriction",		// FIELD_TEAM
	"loadout_position",		// FIELD_LOADOUT_POSITION
	"condition",			// FIELD_CONDITION
	"crit_kill",			// FIELD_CRIT
	"weapon_def_index",		// FIELD_WEAPON_DEF_INDEX
	"halloween_boss_type",	// FIELD_HALLOWEEN_BOSS_TYPE
	"minigame_type",		// FIELD_HALLOWEEN_MINIGAME_TYPE
	"weapon_class",			// FIELD_WEAPON_CLASS
	"bonuseffect",			// FIELD_BONUSEFFECT
	"deflected_projectile", // FIELD_DEFLECTED_PROJECTILE
	"num_hit",				// FIELD_NUM_HIT
	"num_direct_hit",		// FIELD_NUM_DIRECT_HIT
	"var",					// FIELD_VAR
};


struct QuestConditionEntry_t;
CUtlMap< const char*, QuestConditionEntry_t* > k_mapConditions( StringLessThan );
struct QuestConditionEntry_t
{
	QuestConditionEntry_t( const char* pszName, int nRequiredField, pfnQuestCreate pfnCreate )
		: m_nRequiredFields( nRequiredField )
		, m_pfnQuestCreate( pfnCreate )
		, m_pszFieldName( pszName )
	{
		k_mapConditions.Insert( pszName, this );
	}
	int m_nRequiredFields;
	pfnQuestCreate m_pfnQuestCreate;
	const char* m_pszFieldName;
};

#define REGISTER_QUEST_CONDITION_SUB_CLASS( derivedClass, condName, nCondReqFields ) QuestConditionEntry_t k_s##condName##RegisteredEntry( #condName, nCondReqFields, CreateCTFQuestConditionSubClass< derivedClass > );

void IsValidServerForQuests( CSteamID steamIDQuestOwner, InvalidReasonsContainer_t& invalidReasons )
{
	// Check if we're on beta.  If so, allow it.
	if ( ( engine->GetAppID() == 810 || engine->GetAppID() == 440 )
		&& ( steamIDQuestOwner.GetEUniverse() == k_EUniverseBeta || steamIDQuestOwner.GetEUniverse() == k_EUniverseDev ) )
		return;

	if ( TFGameRules() )
	{
		// TODO Do we want to exclude quest progress after the match is over or during warm-up? We'd need another function
		//      and to check it in appropriate spots -- this guy returning false if the match is over gives the user
		//      "Invalid server" status on their quest display and so on.

		// We only allow for quests to be tracked on Valve servers -- check if we joined via MM. Don't care if the match is
		// still running.
		ETFMatchGroup eMatchGroup = TFGameRules()->GetCurrentMatchGroup();
		bool bTrustedMatch = ( eMatchGroup != k_eTFMatchGroup_Invalid ) && GetMatchGroupDescription( eMatchGroup )->BIsTrustedServersOnly();
		if ( !bTrustedMatch )
		{
			invalidReasons.m_bits.Set( INVALID_QUEST_REASON_VALVE_SERVERS_ONLY );
		}

		// Cannot do quests when not in a match or in a match that doesn't allow for quests 
		auto pMatchDesc = GetMatchGroupDescription( eMatchGroup );

		if ( !pMatchDesc || !pMatchDesc->BAllowsQuestProgress() )
		{
			invalidReasons.m_bits.Set( INVALID_QUEST_REASON_MATCH_TYPE );
		}
	}


	return;
}

void GetInvalidReasonsNames( const InvalidReasonsContainer_t& invalidReasons, CUtlVector< CUtlString >& vecStrings )
{
	static const char* arReasons[] = 
	{
		"#TF_QuestInvalid_WrongMap",			// INVALID_QUEST_REASON_WRONG_MAP = 0,
		"#TF_QuestInvalid_WrongClass",			// INVALID_QUEST_REASON_WRONG_CLASS,
		"#TF_QuestInvalid_GameMode",			// INVALID_QUEST_REASON_WRONG_GAME_MODE,
		"#TF_QuestInvalid_NotEnoughPlayers",	// INVALID_QUEST_REASON_NOT_ENOUGH_PLAYERS,
		"#TF_QuestInvalid_ValveServers",		// INVALID_QUEST_REASON_VALVE_SERVERS_ONLY,
		"#TF_QuestInvalid_MatchType",			// INVALID_QUEST_REASON_MATCH_TYPE
	};

	for( int i=0; i < invalidReasons.m_bits.GetNumBits(); ++i )
	{
		if ( invalidReasons.m_bits.IsBitSet( i ) )
		{
			vecStrings.AddToTail( arReasons[i] );
		}
	}
}

KeyValues* GetQuestEventsKeyValues()
{
	static KeyValues *pQuestEvents = NULL;
	if ( pQuestEvents == NULL )
	{
		CUtlBuffer bufRawData;
		bufRawData.SetBufferType( true, true );
		bool bReadFileOK = g_pFullFileSystem->ReadFile( g_skQuestEventsFile, NULL, bufRawData );
		if ( !bReadFileOK )
		{
			AssertMsg1( false, "Couldn't load quest events file: %s!", g_skQuestEventsFile );
			return NULL;
		}
	
		pQuestEvents = new KeyValues( "quest_events" );
		pQuestEvents->LoadFromBuffer( NULL, bufRawData );
	}

	return pQuestEvents;
}

// getting key of params that are unique per event
void GetValidParamsKeyFromEvent( const char *pszKeyName, const char *pszRestrictionName, const char *pszEventName, KeyValues *pRequiredKeys )
{
	KeyValues *pQuestEvents = GetQuestEventsKeyValues();

	if ( pQuestEvents )
	{
		KeyValues *pEvent = pQuestEvents->FindKey( pszEventName );
		AssertMsg1( pEvent, "Failed to find specified event name %s", pszEventName );
		if ( pEvent )
		{
			KeyValues *pRestriction = pEvent->FindKey( pszRestrictionName );
			AssertMsg2( pRestriction, "Failed to find specified restriction name %s :: %s", pszEventName, pszRestrictionName );
			if ( pRestriction )
			{
				KeyValues *pParamsKey = pRestriction->FindKey( pszKeyName );
				AssertMsg3( pParamsKey, "Failed to find specified param key name %s :: %s :: %s", pszEventName, pszRestrictionName, pszKeyName );
				if ( pParamsKey )
				{
					if ( pParamsKey->GetString( "uses_method", NULL ) )
					{
						KeyValues* pKVMethod = pParamsKey->FindKey( "method" );
						AssertMsg3( pKVMethod, "Failed to find method block in %s :: %s :: %s", pszEventName, pszRestrictionName, pszKeyName );
						if ( pKVMethod )
						{
							const char* pszType = pKVMethod->GetString( "type", NULL );
							AssertMsg3( pszType, "Missing method type in %s :: %s :: %s", pszEventName, pszRestrictionName, pszKeyName );
							if ( FStrEq( pszType, "weapon_def_index" ) )
							{
								KeyValues *pWeaponNames = new KeyValues( "value" );

								const CTFItemDefinition* pItemDef = NULL;
								const CEconItemSchema::SortedItemDefinitionMap_t& mapItems = GetItemSchema()->GetSortedItemDefinitionMap();
								FOR_EACH_MAP( mapItems, it )
								{
									pItemDef = static_cast< const CTFItemDefinition* >( mapItems[it] );
									Assert( pItemDef->GetDefinitionIndex() != INVALID_ITEM_DEF_INDEX );
									if ( pItemDef->GetDefaultLoadoutSlot() == LOADOUT_POSITION_PRIMARY
									  || pItemDef->GetDefaultLoadoutSlot() == LOADOUT_POSITION_SECONDARY 
									  || pItemDef->GetDefaultLoadoutSlot() == LOADOUT_POSITION_MELEE 
									  || pItemDef->GetDefaultLoadoutSlot() == LOADOUT_POSITION_PDA
									  || pItemDef->GetDefaultLoadoutSlot() == LOADOUT_POSITION_PDA2
									  || pItemDef->GetDefaultLoadoutSlot() == LOADOUT_POSITION_BUILDING )
									{
										KeyValues *pNewWeapon = pWeaponNames->CreateNewKey();
										pNewWeapon->SetName( CFmtStr( "%d", pItemDef->GetDefinitionIndex() ) );
										pNewWeapon->SetString( "english_name", pItemDef->GetDefinitionName() );
									}
								}

								pWeaponNames->AddSubKey( new KeyValues( "$var1" ) );
								pWeaponNames->AddSubKey( new KeyValues( "$var2" ) );
								pWeaponNames->AddSubKey( new KeyValues( "$var3" ) );

								pRequiredKeys->AddSubKey( pWeaponNames );
							}
							else if ( FStrEq( pszType, "weapon_name" ) )
							{
								KeyValues *pWeaponNames = new KeyValues( "value" );

								// Add a few here that we magically use in the code elsewhere.  Sigh...
								pWeaponNames->AddSubKey( new KeyValues( "obj_attachment_sapper" ) );
								pWeaponNames->AddSubKey( new KeyValues( "building_carried_destroyed" ) );

								const CEconItemDefinition* pItemDef = NULL;
								const CEconItemSchema::SortedItemDefinitionMap_t& mapItems = GetItemSchema()->GetSortedItemDefinitionMap();
								for ( int it = mapItems.FirstInorder(); it != mapItems.InvalidIndex(); it = mapItems.NextInorder( it ) )
								{
									pItemDef = mapItems[it];
									if ( pItemDef->GetIconClassname() )
									{
										pWeaponNames->AddSubKey( new KeyValues( pItemDef->GetIconClassname() ) );
									}
								}

								pWeaponNames->AddSubKey( new KeyValues( "$var1" ) );
								pWeaponNames->AddSubKey( new KeyValues( "$var2" ) );
								pWeaponNames->AddSubKey( new KeyValues( "$var3" ) );

								pRequiredKeys->AddSubKey( pWeaponNames );
							}
							else if ( FStrEq( pszType, "weapon_type" ) )
							{
								KeyValues *pWeaponType = new KeyValues( "value" );
								for ( int i=0; i<TF_WEAPON_COUNT; ++i )
								{
									const char *pszWeaponName = WeaponIdToAlias( i );
									KeyValues *pNewWeapon = pWeaponType->CreateNewKey();
									pNewWeapon->SetName( CFmtStr( "%d", i ) );
									pNewWeapon->SetString( "english_name", pszWeaponName );
								}

								pRequiredKeys->AddSubKey( pWeaponType );
							}
							else if ( FStrEq( pszType, "item_class" ) )
							{
								KeyValues *pItemClassNames = new KeyValues( "value" );

								const CEconItemDefinition* pItemDef = NULL;
								const CEconItemSchema::SortedItemDefinitionMap_t& mapItems = GetItemSchema()->GetSortedItemDefinitionMap();
								CUtlMap< const char*, int > mapTypeNames( StringLessThan );
								for ( int it = mapItems.FirstInorder(); it != mapItems.InvalidIndex(); it = mapItems.NextInorder( it ) )
								{
									pItemDef = mapItems[it];
									const char *pszClass =  pItemDef->GetItemClass();
									if ( pszClass && mapTypeNames.Find( pszClass ) == mapTypeNames.InvalidIndex() )
									{
										mapTypeNames.Insert( pszClass );
										pItemClassNames->AddSubKey( new KeyValues( pItemDef->GetItemClass() ) );
									}
								}

								pRequiredKeys->AddSubKey( pItemClassNames );
							}
							else
							{
								AssertMsg5( false, "Type %s in %s :: %s :: %s didnt match any types defined in %s", pszType, pszEventName, pszRestrictionName, pszKeyName, __FUNCTION__ );
							}
						}
					}
					else
					{
						pRequiredKeys->AddSubKey( pParamsKey->MakeCopy() );
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFQuestCondition::CTFQuestCondition()
	: m_pParent( NULL )
{}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFQuestCondition::~CTFQuestCondition()
{
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFQuestCondition::BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ )
{
	// check if restriction name matches the keyvalue name
	//const char *pszType = pKVItem->GetString( "type" );
	//SCHEMA_INIT_CHECK( FStrEq( pszType, GetConditionName() ), "%s", CFmtStr( "Invalid quest restriction name '%s' for '%s'", pszType, GetConditionName() ).Get() )

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Print out debug text
//-----------------------------------------------------------------------------
void CTFQuestCondition::PrintDebugText() const
{
	DevMsg( "'%s %s'", GetConditionName(), GetValueString() );
}


//-----------------------------------------------------------------------------
// Purpose: Get the owner player
//-----------------------------------------------------------------------------
const CTFPlayer *CTFQuestCondition::GetQuestOwner() const
{
	if ( m_pParent )
		return m_pParent->GetQuestOwner();

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Global quest objective validation checks
//-----------------------------------------------------------------------------
bool CTFQuestCondition::IsValidForPlayer( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
{
	if ( pOwner )
	{
		CSteamID steamIDOwner;
		const_cast< CTFPlayer* >( pOwner )->GetSteamID( &steamIDOwner ); // Ugh

		// Can only do quests on Valve servers
		IsValidServerForQuests( steamIDOwner, invalidReasons );
	}

	return true;
}

void CTFQuestCondition::GetValidRestrictions( CUtlVector< const char* >& vecOutValidChildren ) const
{
	const CTFQuestCondition* pParent = this;
	const CTFQuestEvaluator* pEvaluatorParent = NULL;
	while( pParent && !pEvaluatorParent )
	{
		pEvaluatorParent = dynamic_cast< const CTFQuestEvaluator* >( pParent );
		pParent = pParent->GetParent();
	}

	KeyValues* pKVQuestEvents = GetQuestEventsKeyValues();
	KeyValues* pKVEvent = pKVQuestEvents->FindKey( pEvaluatorParent->GetEventName() );
	FOR_EACH_MAP( k_mapConditions, i )
	{
		QuestConditionEntry_t* pCondEntry = k_mapConditions[ i ];
		// All fields need to be present in the event for the condition to be a valid child
		bool bAnyFound = false;
		bool bAllFound = true;
		if ( pCondEntry->m_nRequiredFields != FIELD_NONE )
		{
			int nMaxCount = Q_log2( FIELD_LAST_FIELD );
			for( int nField = 0; nField <= nMaxCount && bAllFound; ++nField )
			{
				if ( pCondEntry->m_nRequiredFields & (1<<nField) )
				{
					bAnyFound = true;
					bAllFound &= pKVEvent->FindKey( k_pszQuestConditionRequiredFieldStrings[ nField ] ) != NULL;
				}
			}
		}
		else
		{
			bAllFound = true;
			bAnyFound = true;
		}

		if ( bAnyFound && bAllFound )
		{
			vecOutValidChildren.AddToTail( k_mapConditions.Key( i ) );
		}
	}
}

void CTFQuestCondition::GetValidEvaluators( CUtlVector< const char* >& vecOutValidChildren ) const
{
	vecOutValidChildren.AddToTail( "event_listener" );
	vecOutValidChildren.AddToTail( "counter" );
}

void CTFQuestRestriction::GetValidTypes( CUtlVector< const char* >& vecOutValidChildren ) const
{
	GetValidRestrictions( vecOutValidChildren );
}

void CTFQuestRestriction::GetValidChildren( CUtlVector< const char* >& vecOutValidChildren ) const
{
	GetValidRestrictions( vecOutValidChildren );
}


CTFQuestEvaluator::CTFQuestEvaluator()
{
	m_pszAction = NULL;
}

CTFQuestEvaluator::~CTFQuestEvaluator()
{
	m_vecModifiers.PurgeAndDeleteElements();
}

void CTFQuestEvaluator::GetOutputKeyValues( KeyValues *pOutputKeys )
{
	if ( m_pszAction )
	{
		pOutputKeys->SetString( "action", m_pszAction );
	}
}

void CTFQuestEvaluator::GetValidTypes( CUtlVector< const char* >& vecOutValidChildren ) const
{
	GetValidEvaluators( vecOutValidChildren );
}

void CTFQuestEvaluator::GetValidChildren( CUtlVector< const char* >& vecOutValidChildren ) const
{
	GetValidRestrictions( vecOutValidChildren );
}

void CTFQuestEvaluator::AddModifiers( ITFQuestModifier* pModifier )
{
	m_vecModifiers.AddToTail( pModifier );
}

bool CTFQuestEvaluator::IsValidForPlayer( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
{
	FOR_EACH_VEC( m_vecModifiers, i )
	{
		if ( !m_vecModifiers[ i ]->BPassesModifier( pOwner, invalidReasons ) )
			return false;
	}

	return CTFQuestCondition::IsValidForPlayer( pOwner, invalidReasons );
}


//-----------------------------------------------------------------------------
// Purpose: base quest operator restriction
//-----------------------------------------------------------------------------
class CTFQuestOperatorRestriction: public CTFQuestRestriction
{
public:
	DECLARE_CLASS( CTFQuestOperatorRestriction, CTFQuestRestriction )

	virtual ~CTFQuestOperatorRestriction()
	{
		m_vecRestrictions.PurgeAndDeleteElements();
	}

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestRestriction::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		int nInputCount = 0;
		FOR_EACH_TRUE_SUBKEY( pKVItem, pSubKey )
		{
			if ( GetMaxInputCount() > 0 )
			{
				SCHEMA_INIT_CHECK( nInputCount < GetMaxInputCount(), "%s", CFmtStr( "Too many input for operator '%s'. expected %d input(s)", GetConditionName(), GetMaxInputCount() ).Get() );
			}
			
			const char *pszType = pSubKey->GetString( "type" );
			CTFQuestRestriction *pNewRestriction = CreateRestrictionByName( pszType, this );
			SCHEMA_INIT_CHECK( pNewRestriction != NULL, "%s", CFmtStr( "Failed to create quest restriction name '%s' for '%s'", pszType, GetConditionName() ).Get() );

			SCHEMA_INIT_CHECK( pNewRestriction->BInitFromKV( pSubKey, pVecErrors ), "Failed to init from KeyValues" );

			m_vecRestrictions.AddToTail( pNewRestriction );
			nInputCount++;
		}
		SCHEMA_INIT_CHECK( nInputCount > 0 && nInputCount <= GetMaxInputCount(), "%s", CFmtStr( "Invalid number of specified input. Expected from 0 to %d inputs.", GetMaxInputCount() ).Get() );

		return true;
	}

	virtual bool IsOperator() const OVERRIDE { return true; }

	virtual void PrintDebugText() const OVERRIDE
	{
		DevMsg( "( " );
		FOR_EACH_VEC( m_vecRestrictions, i )
		{
			if ( i == 0 )
			{
				m_vecRestrictions[i]->PrintDebugText();
			}
			else
			{
				DevMsg( " %s ", GetConditionName() );
				m_vecRestrictions[i]->PrintDebugText();
			}
		}
		DevMsg( " )" );
	}

	CTFQuestCondition* AddChildByName( const char *pszChildName ) OVERRIDE
	{
		if ( m_vecRestrictions.Count() >= GetMaxInputCount() )
		{
			Assert( m_vecRestrictions.Count() < GetMaxInputCount() );
			return NULL;
		}

		CTFQuestRestriction *pNewRestriction = CreateRestrictionByName( pszChildName, this );
		if ( pNewRestriction )
		{
			m_vecRestrictions.AddToTail( pNewRestriction );
		}

		return pNewRestriction;
	}

	virtual int GetChildren( CUtlVector< CTFQuestCondition* >& vecChildren ) OVERRIDE
	{
		FOR_EACH_VEC( m_vecRestrictions, i )
		{
			vecChildren.AddToTail( m_vecRestrictions[i] );
		}

		return vecChildren.Count();
	}

	bool RemoveAndDeleteChild( CTFQuestCondition *pChild ) OVERRIDE
	{
		CTFQuestRestriction *pRestrictionChild = assert_cast< CTFQuestRestriction* >( pChild );
		bool bRemoved = m_vecRestrictions.FindAndFastRemove( pRestrictionChild );
		Assert( bRemoved );

		if ( bRemoved )
		{
			delete pChild;
		}

		return bRemoved;
	}


	virtual int GetMaxInputCount() const OVERRIDE { return s_nMaxInputCount; }

	protected:

	CUtlVector< CTFQuestRestriction* > m_vecRestrictions;
};

//-----------------------------------------------------------------------------
// Purpose: AND quest operator restriction
//-----------------------------------------------------------------------------
class CTFQuestAndOperatorRestriction : public CTFQuestOperatorRestriction
{
public:
	DECLARE_CLASS( CTFQuestAndOperatorRestriction, CTFQuestOperatorRestriction )

	virtual bool PassesRestrictions( IGameEvent *pEvent ) const OVERRIDE
	{
		FOR_EACH_VEC( m_vecRestrictions, i )
		{
			if ( !m_vecRestrictions[i]->PassesRestrictions( pEvent ) )
				return false;
		}

		return true;
	}

	virtual bool IsValidForPlayer( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
	{
		BaseClass::IsValidForPlayer( pOwner, invalidReasons );

		bool bIsForLocalPlayer = false;
		InvalidReason operatorReasons;
		FOR_EACH_VEC( m_vecRestrictions, i )
		{			
			bIsForLocalPlayer |= m_vecRestrictions[i]->IsValidForPlayer( pOwner, operatorReasons );
		}

		if ( bIsForLocalPlayer )
		{
			invalidReasons.m_bits.Or( operatorReasons.m_bits, &invalidReasons.m_bits );
		}

		return bIsForLocalPlayer;
	}
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFQuestAndOperatorRestriction, AND, FIELD_NONE );

//-----------------------------------------------------------------------------
// Purpose: OR quest operator restriction
//-----------------------------------------------------------------------------
class CTFQuestOrOperatorRestriction : public CTFQuestOperatorRestriction
{
public:
	DECLARE_CLASS( CTFQuestOrOperatorRestriction, CTFQuestOperatorRestriction )

	virtual bool PassesRestrictions( IGameEvent *pEvent ) const OVERRIDE
	{
		FOR_EACH_VEC( m_vecRestrictions, i )
		{
			if ( m_vecRestrictions[i]->PassesRestrictions( pEvent ) )
				return true;
		}

		return false;
	}

	virtual bool IsValidForPlayer( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
	{
		BaseClass::IsValidForPlayer( pOwner, invalidReasons );

		bool bIsForLocalPlayer = false;

		FOR_EACH_VEC( m_vecRestrictions, i )
		{
			InvalidReason operatorReason;
			if ( m_vecRestrictions[i]->IsValidForPlayer( pOwner, operatorReason ) )
			{
				bIsForLocalPlayer = true;
				invalidReasons.m_bits.Or( operatorReason.m_bits, &invalidReasons.m_bits );
			}
		}

		return bIsForLocalPlayer;
	}
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFQuestOrOperatorRestriction, OR, FIELD_NONE );

//-----------------------------------------------------------------------------
// Purpose: NOT quest operator restriction
//-----------------------------------------------------------------------------
class CTFQuestNotOperatorRestriction : public CTFQuestOperatorRestriction
{
public:
	virtual bool PassesRestrictions( IGameEvent *pEvent ) const OVERRIDE
	{
		return !m_vecRestrictions[0]->PassesRestrictions( pEvent );
	}

	virtual bool IsValidForPlayer( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
	{
		return false;
	}
	
	virtual void PrintDebugText() const OVERRIDE
	{
		CTFQuestRestriction *pRestriction = m_vecRestrictions[0];
		if ( pRestriction->IsOperator() )
		{
			DevMsg( "%s ", GetConditionName() );
			pRestriction->PrintDebugText();
		}
		else
		{
			// add () for non-operator to keep the debug text format consistent
			DevMsg( "%s ( ", GetConditionName() );
			pRestriction->PrintDebugText();
			DevMsg( " )" );
		}
	}

protected:
	virtual int GetMaxInputCount() const OVERRIDE { return 1; }
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFQuestNotOperatorRestriction, NOT, FIELD_NONE );

class CTFGenericStringRestriction : public CTFQuestRestriction
{
public:
	CTFGenericStringRestriction()
		: m_pszKeyName( NULL )
		, m_pszValue( NULL )
	{}
	virtual const char *GetConditionName() const OVERRIDE { return m_pszFieldName; }

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestRestriction::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		m_pszKeyName = pKVItem->GetString( "key_to_lookup" );
		SCHEMA_INIT_CHECK( m_pszKeyName != NULL, "Missing key to lookup for generic_string restriction!" );

		m_pszValue = pKVItem->GetString( "value" );
		SCHEMA_INIT_CHECK( m_pszValue != NULL, "Missing value to compare against for generic_string restriction!" );

		m_bStringsEqual = pKVItem->GetBool( "strings_equal", "1" );

		return true;
	}

	virtual bool PassesRestrictions( IGameEvent *pEvent ) const OVERRIDE
	{
		const char* pszValue = pEvent->GetString( m_pszKeyName );
		if ( pszValue )
		{
			if ( m_bStringsEqual )
			{
				return FStrEq( pszValue, m_pszValue );
			}
			else
			{
				return V_strnicmp( pszValue, m_pszValue, Min( V_strlen( pszValue ), V_strlen( m_pszValue ) ) ) == 0;
			}
		}

		return false;
	}

	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE
	{
		CTFQuestRestriction::GetOutputKeyValues( pOutputKeys );

		pOutputKeys->SetString( "key_to_lookup", m_pszKeyName );
		pOutputKeys->SetString( "value", m_pszValue );
	}

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE
	{
		CTFQuestRestriction::GetRequiredParamKeys( pRequiredKeys );

		GetValidParamsKeyFromEvent( "key_to_lookup", m_pszFieldName, m_pszEventName, pRequiredKeys );
		GetValidParamsKeyFromEvent( "value", m_pszFieldName, m_pszEventName, pRequiredKeys );
	}

protected:

	const char *m_pszKeyName;
	const char *m_pszValue;
	bool m_bStringsEqual = true;
};

class CTFGenericSubStringRestriction : public CTFGenericStringRestriction
{
public:
	CTFGenericSubStringRestriction()
	{}

	virtual bool PassesRestrictions( IGameEvent *pEvent ) const OVERRIDE
	{
		const char* pszValue = pEvent->GetString( m_pszKeyName );
		if ( pszValue )
		{
			return V_stristr( pszValue, m_pszValue ) != NULL;
		}

		return false;
	}
};

class CTFWeaponClassRestriction : public CTFGenericStringRestriction
{
public:
	CTFWeaponClassRestriction()
	{}

	virtual bool PassesRestrictions( IGameEvent *pEvent ) const OVERRIDE
	{
		const char* pszValue = pEvent->GetString( m_pszKeyName );
		if ( pszValue )
		{
			const CEconItemDefinition* pItemDef = GetItemSchema()->GetItemDefinition( atoi( pszValue ) );
			Assert( pItemDef );
			if ( pItemDef )
			{
				return FStrEq( pItemDef->GetItemClass(), m_pszValue );
			}
		}

		return false;
	}

};

class CTFWeaponDefindexRestriction : public CTFGenericStringRestriction
{
public:
	CTFWeaponDefindexRestriction()
	{}

	virtual bool PassesRestrictions( IGameEvent *pEvent ) const OVERRIDE
	{
		const char* pszValue = pEvent->GetString( m_pszKeyName );
		if ( pszValue )
		{
			const CEconItemDefinition* pItemDef = GetItemSchema()->GetItemDefinition( atoi( pszValue ) );
			const CEconItemDefinition* pRequiredItemDef = GetItemSchema()->GetItemDefinition( atoi( m_pszValue ) );
			Assert( pItemDef );
			Assert( pRequiredItemDef );
			if ( pItemDef && pRequiredItemDef )
			{
				if ( pRequiredItemDef == pItemDef )
					return true;

				if ( pItemDef->GetXifierRemapClass() && pRequiredItemDef->GetXifierRemapClass() &&
					 FStrEq( pItemDef->GetXifierRemapClass(), pRequiredItemDef->GetXifierRemapClass() ) )
					return true;
			}
		}

		return false;
	}

};

REGISTER_QUEST_CONDITION_SUB_CLASS( CTFGenericStringRestriction, crit_kill, FIELD_CRIT );
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFWeaponDefindexRestriction, weapon_def_index, FIELD_WEAPON_DEF_INDEX );
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFGenericStringRestriction, weapon_name, FIELD_WEAPON_NAME );
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFGenericStringRestriction, halloween_boss_type, FIELD_HALLOWEEN_BOSS_TYPE );
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFGenericStringRestriction, minigame_type, FIELD_HALLOWEEN_MINIGAME_TYPE );
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFGenericStringRestriction, bonuseffect, FIELD_BONUSEFFECT );
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFWeaponClassRestriction, weapon_class, FIELD_WEAPON_CLASS );
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFGenericSubStringRestriction, deflected_projectile, FIELD_DEFLECTED_PROJECTILE );
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFGenericStringRestriction, num_hit, FIELD_NUM_HIT );
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFGenericStringRestriction, num_direct_hit, FIELD_NUM_DIRECT_HIT );
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFGenericStringRestriction, var, FIELD_VAR );

//-----------------------------------------------------------------------------
// Purpose: quest player restriction
//-----------------------------------------------------------------------------
class CTFQuestBasePlayerRestriction : public CTFQuestRestriction
{
public:
	CTFQuestBasePlayerRestriction()
		: m_pszPlayerKey( NULL )
		, m_pszPlayerMethod( NULL )
	{}

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestRestriction::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		m_pszPlayerKey = pKVItem->GetString( "player_key", NULL );
		Assert( m_pszPlayerKey );
		SCHEMA_INIT_CHECK( m_pszPlayerKey != NULL, "missing 'player_key'" );

		static const char *s_pszValidGetMethod[] =
		{
			"by_id",
			"by_entindex",
			"by_cappers"
		};
		m_pszPlayerMethod = pKVItem->GetString( "get_player" );
		bool bIsValidMethod = false;
		for ( int i=0; i<ARRAYSIZE( s_pszValidGetMethod ); ++i )
		{
			if ( FStrEq( m_pszPlayerMethod, s_pszValidGetMethod[i] ) )
			{
				bIsValidMethod = true;
				break;
			}
		}
		SCHEMA_INIT_CHECK( bIsValidMethod, "Invalid 'get_player'" );
		

		return true;
	}

	virtual bool PassesRestrictions( IGameEvent *pEvent ) const OVERRIDE
	{
		CTFPlayer *pPlayer = GetPlayerFromEvent( pEvent );
		if ( !pPlayer )
			return false;

		return BPlayerCheck( pPlayer, pEvent );
	}

	CTFPlayer *GetPlayerFromEvent( IGameEvent *pEvent ) const
	{
		CTFPlayer *pPlayer = NULL;
		if ( FStrEq( m_pszPlayerMethod, "by_id" ) )
		{
			int iPlayerID = pEvent->GetInt( m_pszPlayerKey );
			pPlayer = ToTFPlayer( UTIL_PlayerByUserId( iPlayerID ) );
		}
		else if ( FStrEq( m_pszPlayerMethod, "by_entindex" ) )
		{
			int iPlayerIndex = pEvent->GetInt( m_pszPlayerKey );
			pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
		}
		else if ( FStrEq( m_pszPlayerMethod, "by_cappers" ) )
		{
			Assert( FStrEq( m_pszPlayerKey, "cappers" ) );
			const CTFPlayer *pQuestOwner = GetQuestOwner();
			const char *cappers = pEvent->GetString( m_pszPlayerKey );
			for ( int i = 0; i < Q_strlen( cappers ); i++ )
			{
				int iPlayerIndex = (int)cappers[i];
				CTFPlayer *pCapper = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
				if ( pCapper == pQuestOwner )
				{
					pPlayer = pCapper;
					break;
				}
			}
		}

		return pPlayer;
	}

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE
	{
		CTFQuestRestriction::GetRequiredParamKeys( pRequiredKeys );

		GetValidParamsKeyFromEvent( "player_key", "player", m_pszEventName, pRequiredKeys );
		GetValidParamsKeyFromEvent( "get_player", "player", m_pszEventName, pRequiredKeys );
	}

	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE
	{
		CTFQuestRestriction::GetOutputKeyValues( pOutputKeys );

		pOutputKeys->SetString( "player_key", m_pszPlayerKey );
		pOutputKeys->SetString( "get_player", m_pszPlayerMethod );
	}

protected:

	virtual bool BPlayerCheck( const CTFPlayer* pPlayer, IGameEvent *pEvent ) const = 0;

	const char *m_pszPlayerKey;
	const char *m_pszPlayerMethod;
};

//-----------------------------------------------------------------------------
// Purpose: quest player restriction
//-----------------------------------------------------------------------------
class CTFQuestPlayerDisguiseRestriction : public CTFQuestBasePlayerRestriction
{
public:
	CTFQuestPlayerDisguiseRestriction() {}

	enum EDisguiseTargetState_t
	{
		DISGUISE_STATE_OWNER_IS_PLAYER,
		DISGUISE_STATE_OWNER_IS_NOT_PLAYER,
		DISGUISE_STATE_PLAYER_IS_OWNER,
		DISGUISE_STATE_PLAYER_IS_NOT_OWNER,
		DISGUISE_STATE_DONT_CARE
	};

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestBasePlayerRestriction::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		m_eDisguiseState = (EDisguiseTargetState_t)pKVItem->GetInt( "disguise_target", DISGUISE_STATE_DONT_CARE );

		return true;
	}


	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE
	{
		CTFQuestBasePlayerRestriction::GetRequiredParamKeys( pRequiredKeys );

		// Disguise state
		{
			KeyValues *pKVDisguiseKeys = new KeyValues( "disguise_target" );
			pKVDisguiseKeys->SetString( "english_name",	"Disguise" );

			// DISGUISE_STATE_OWNER_IS_PLAYER
			KeyValues * pKVDisguiseState = pKVDisguiseKeys->CreateNewKey();
			pKVDisguiseState->SetName( "0" );
			pKVDisguiseState->SetString( "english_name", "Owner disguised as the player" );

			// DISGUISE_STATE_OWNER_IS_NOT_PLAYER
			pKVDisguiseState = pKVDisguiseKeys->CreateNewKey();
			pKVDisguiseState->SetName( "1" );
			pKVDisguiseState->SetString( "english_name", "Owner NOT disguised as the player" );

			// DISGUISE_STATE_PLAYER_IS_OWNER
			pKVDisguiseState = pKVDisguiseKeys->CreateNewKey();
			pKVDisguiseState->SetName( "2" );
			pKVDisguiseState->SetString( "english_name", "Player disguised as the owner" );

			// DISGUISE_STATE_PLAYER_IS_NOT_OWNER
			pKVDisguiseState = pKVDisguiseKeys->CreateNewKey();
			pKVDisguiseState->SetName( "3" );
			pKVDisguiseState->SetString( "english_name", "Player NOT disguised as the owner" );

			pRequiredKeys->AddSubKey( pKVDisguiseKeys );
		}
	}

	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE
	{
		CTFQuestBasePlayerRestriction::GetOutputKeyValues( pOutputKeys );

		pOutputKeys->SetInt( "disguise_target", m_eDisguiseState );
	}

protected:

	virtual bool BPlayerCheck( const CTFPlayer* pPlayer, IGameEvent *pEvent ) const OVERRIDE
	{
		// Disguise state check
		const CTFPlayer* pPlayerDisguiseTarget = pPlayer->m_Shared.GetDisguiseTarget();
		const CTFPlayer* pOwnerDisguiseTarget = GetQuestOwner()->m_Shared.GetDisguiseTarget();

		// owner in disguise
		if ( pOwnerDisguiseTarget )
		{
			// must disguise as same class to look the same
			if ( m_eDisguiseState == DISGUISE_STATE_OWNER_IS_PLAYER && pOwnerDisguiseTarget == pPlayer && GetQuestOwner()->m_Shared.GetDisguiseClass() == pPlayer->GetPlayerClass()->GetClassIndex() )
				return true;

			if ( m_eDisguiseState == DISGUISE_STATE_OWNER_IS_NOT_PLAYER && pOwnerDisguiseTarget != pPlayer )
				return true;
		}

		// player in disguise
		if ( pPlayerDisguiseTarget )
		{
			// must disguise as same class to look the same
			if ( m_eDisguiseState == DISGUISE_STATE_PLAYER_IS_OWNER && pPlayerDisguiseTarget == GetQuestOwner() && pPlayer->m_Shared.GetDisguiseClass() == GetQuestOwner()->GetPlayerClass()->GetClassIndex() )
				return true;

			if ( m_eDisguiseState == DISGUISE_STATE_PLAYER_IS_NOT_OWNER && pPlayerDisguiseTarget != GetQuestOwner() )
				return true;
		}

		return false;
	}

	EDisguiseTargetState_t m_eDisguiseState;
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFQuestPlayerDisguiseRestriction, player_disguise, FIELD_PLAYER );

//-----------------------------------------------------------------------------
// Purpose: quest player jumping-state restriction
//-----------------------------------------------------------------------------
class CTFQuestPlayerJumpingRestriction : public CTFQuestBasePlayerRestriction
{
public:
	CTFQuestPlayerJumpingRestriction() {}

	enum EJumpingState_t
	{
		JUMPING_STATE_IS_NOT_JUMPING = 0,
		JUMPING_STATE_IS_JUMPING,
		JUMPING_STATE_IS_DOUBLE_JUMPING,
		JUMPING_STATE_IS_TRIPLE_JUMPING,
	};

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestBasePlayerRestriction::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		m_eJumpingState = (EJumpingState_t)pKVItem->GetInt( "jumping_state", JUMPING_STATE_IS_NOT_JUMPING );

		return true;
	}

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE
	{
		CTFQuestBasePlayerRestriction::GetRequiredParamKeys( pRequiredKeys );

		// Jumping state
		{
			KeyValues *pKVJumpingKeys = new KeyValues( "jumping_state" );
			pKVJumpingKeys->SetString( "english_name",	"Jumping" );
			KeyValues * pKVJumpState = pKVJumpingKeys->CreateNewKey();
			pKVJumpState->SetName( "0" );
			pKVJumpState->SetString( "english_name", "Must NOT be jumping" );
		
			pKVJumpState = pKVJumpingKeys->CreateNewKey();
			pKVJumpState->SetName( "1" );
			pKVJumpState->SetString( "english_name", "Must be at least jumping" );

			pKVJumpState = pKVJumpingKeys->CreateNewKey();
			pKVJumpState->SetName( "2" );
			pKVJumpState->SetString( "english_name", "Must be at least double-jumping" );

			pKVJumpState = pKVJumpingKeys->CreateNewKey();
			pKVJumpState->SetName( "3" );
			pKVJumpState->SetString( "english_name", "Must be at least triple-jumping" );

			pRequiredKeys->AddSubKey( pKVJumpingKeys );
		}
	}

	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE
	{
		CTFQuestBasePlayerRestriction::GetOutputKeyValues( pOutputKeys );
		pOutputKeys->SetInt( "jumping_state", m_eJumpingState );
	}

protected:

	virtual bool BPlayerCheck( const CTFPlayer* pPlayer, IGameEvent *pEvent ) const OVERRIDE
	{
#ifdef GAME_DLL
		CTFPlayer* pNonConstPlayer = const_cast< CTFPlayer* >( pPlayer );

		int nNumJumps = pNonConstPlayer->GetGroundEntity() == NULL ? 1 : 0;
		nNumJumps += pPlayer->m_Shared.GetAirDash();
		nNumJumps += pPlayer->m_bScattergunJump;

		if ( m_eJumpingState == JUMPING_STATE_IS_NOT_JUMPING )
		{
			return nNumJumps == 0;
		}
		else if ( m_eJumpingState == JUMPING_STATE_IS_JUMPING )
		{
			return nNumJumps >= 1;
		}
		else if ( m_eJumpingState == JUMPING_STATE_IS_DOUBLE_JUMPING )
		{
			return nNumJumps >= 2;
		}
		else if ( m_eJumpingState == JUMPING_STATE_IS_TRIPLE_JUMPING )
		{
			return nNumJumps >= 3;
		}
		else
		{
			AssertMsg1( false, "Unhandled EJumpingState_t case %d!", m_eJumpingState );
		}
#endif
		return false;
	}

	EJumpingState_t m_eJumpingState;
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFQuestPlayerJumpingRestriction, player_jumping, FIELD_PLAYER );

//-----------------------------------------------------------------------------
// Purpose: quest player alive-state restriction
//-----------------------------------------------------------------------------
class CTFQuestPlayerAliveRestriction : public CTFQuestBasePlayerRestriction
{
public:
	CTFQuestPlayerAliveRestriction() {}

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestBasePlayerRestriction::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		m_bAliveState = pKVItem->GetBool( "alive_state", true );

		return true;
	}

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE
	{
		CTFQuestBasePlayerRestriction::GetRequiredParamKeys( pRequiredKeys );

		KeyValues *pKVAliveKeys = new KeyValues( "alive_state" );
		pKVAliveKeys->SetString( "english_name",	"Alive" );
		KeyValues * pKVAliveState = pKVAliveKeys->CreateNewKey();
		pKVAliveState->SetName( "1" );
		pKVAliveState->SetString( "english_name", "Must be alive" );

		pKVAliveState = pKVAliveKeys->CreateNewKey();
		pKVAliveState->SetName( "0" );
		pKVAliveState->SetString( "english_name", "Must be dead" );

		pRequiredKeys->AddSubKey( pKVAliveKeys );
	}

	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE
	{
		CTFQuestBasePlayerRestriction::GetOutputKeyValues( pOutputKeys );
		pOutputKeys->SetInt( "alive_state", m_bAliveState );
	}

protected:

	virtual bool BPlayerCheck( const CTFPlayer* pPlayer, IGameEvent *pEvent ) const OVERRIDE
	{
		return ( pPlayer->m_iHealth > 0 ) == m_bAliveState;
	}

	bool m_bAliveState;
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFQuestPlayerAliveRestriction, player_alive, FIELD_PLAYER );

//-----------------------------------------------------------------------------
// Purpose: quest player distance restriction relative to the owner
//-----------------------------------------------------------------------------
class CTFQuestPlayerDistanceRestriction : public CTFQuestBasePlayerRestriction
{
public:
	CTFQuestPlayerDistanceRestriction() {}

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestBasePlayerRestriction::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		m_eCheckType = (EDistanceCheck_t)pKVItem->GetInt( "distance_check_type", INVALID_CHECK_TYPE );
		SCHEMA_INIT_CHECK( m_eCheckType != INVALID_CHECK_TYPE, "Invalid distance_check_type %d!", m_eCheckType );

		m_nDistance = pKVItem->GetInt( "distance_to_check", 0 );
		SCHEMA_INIT_CHECK( m_eCheckType != 0, "Distance must be non-zero!" );

		return true;
	}

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE
	{
		CTFQuestBasePlayerRestriction::GetRequiredParamKeys( pRequiredKeys );

		KeyValues *pKVDistanceCheckTypeKeys = new KeyValues( "distance_check_type" );
		pKVDistanceCheckTypeKeys->SetString( "english_name",	"Alive" );
		KeyValues * pKVType = pKVDistanceCheckTypeKeys->CreateNewKey();
		pKVType->SetName( "1" );
		pKVType->SetString( "english_name", "Closer than" );

		pKVType = pKVDistanceCheckTypeKeys->CreateNewKey();
		pKVType->SetName( "2" );
		pKVType->SetString( "english_name", "Further than" );

		pRequiredKeys->AddSubKey( pKVDistanceCheckTypeKeys );

		KeyValues *pDistanceKey = new KeyValues( "distance_to_check" );
		pDistanceKey->SetString( "control_type", "text_entry" );

		pRequiredKeys->AddSubKey( pDistanceKey );
	}

	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE
	{
		CTFQuestBasePlayerRestriction::GetOutputKeyValues( pOutputKeys );
		pOutputKeys->SetInt( "distance_check_type", m_eCheckType );
		pOutputKeys->SetInt( "distance_to_check", m_nDistance );
	}

protected:

	enum EDistanceCheck_t
	{
		INVALID_CHECK_TYPE = 0,
		CLOSER_THAN,
		FURTHER_THAN
	};

	virtual bool BPlayerCheck( const CTFPlayer* pPlayer, IGameEvent *pEvent ) const OVERRIDE
	{
		int nTestDist = m_nDistance * m_nDistance;
		int nPlayerDistSq = ( pPlayer->GetAbsOrigin() - GetQuestOwner()->GetAbsOrigin() ).LengthSqr();

		if ( m_eCheckType == CLOSER_THAN )
		{
			return nPlayerDistSq < nTestDist;
		}
		else
		{
			return nPlayerDistSq > nTestDist;
		}
	}

	EDistanceCheck_t m_eCheckType;
	float m_nDistance;
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFQuestPlayerDistanceRestriction, player_distance, FIELD_PLAYER );


//-----------------------------------------------------------------------------
// Purpose: quest player restriction
//-----------------------------------------------------------------------------
class CTFQuestPlayerIsOwnerRestriction : public CTFQuestBasePlayerRestriction
{
public:
	CTFQuestPlayerIsOwnerRestriction() {}

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestBasePlayerRestriction::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		// should check if this player is quest owner?
		m_bIsOwner = pKVItem->GetBool( "is_owner" );

		return true;
	}

	virtual bool IsValidForPlayer( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
	{
		CTFQuestBasePlayerRestriction::IsValidForPlayer( pOwner, invalidReasons );

		return m_bIsOwner && pOwner == GetQuestOwner();
	}

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE
	{
		CTFQuestBasePlayerRestriction::GetRequiredParamKeys( pRequiredKeys );

		KeyValues *pIsOwnerKey = new KeyValues( "is_owner" );
		pIsOwnerKey->SetString( "english_name", "is_owner" );

		KeyValues *pIsOwnerKeyChoiceTrue = new KeyValues( "1" );
		pIsOwnerKeyChoiceTrue->SetString( "english_name", "true" );

		KeyValues *pIsOwnerKeyChoiceFalse = new KeyValues( "0" );
		pIsOwnerKeyChoiceFalse->SetString( "english_name", "false" );

		pIsOwnerKey->AddSubKey( pIsOwnerKeyChoiceTrue );
		pIsOwnerKey->AddSubKey( pIsOwnerKeyChoiceFalse );

		pRequiredKeys->AddSubKey( pIsOwnerKey );
	}

	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE
	{
		CTFQuestBasePlayerRestriction::GetOutputKeyValues( pOutputKeys );
		pOutputKeys->SetBool( "is_owner", m_bIsOwner );
	}

private:

	virtual bool BPlayerCheck( const CTFPlayer* pPlayer, IGameEvent *pEvent ) const OVERRIDE
	{
		return m_bIsOwner == ( pPlayer == GetQuestOwner() );
	}

	bool m_bIsOwner;
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFQuestPlayerIsOwnerRestriction, player_is_owner, FIELD_PLAYER );

//-----------------------------------------------------------------------------
// Purpose: quest player restriction
//-----------------------------------------------------------------------------
class CTFQuestPlayerIsEnemyRestriction : public CTFQuestBasePlayerRestriction
{
public:
	CTFQuestPlayerIsEnemyRestriction() {}

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestBasePlayerRestriction::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		// should check if this player is an enemy of the quest owner
		m_bIsEnemy = pKVItem->GetBool( "is_enemy" );

		return true;
	}

	virtual bool IsValidForPlayer( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
	{
		return true;
	}

private:

	virtual bool BPlayerCheck( const CTFPlayer* pPlayer, IGameEvent *pEvent ) const OVERRIDE
	{
		Assert( pPlayer != GetQuestOwner() );
		bool bSameTeam = pPlayer->GetTeamNumber() == GetQuestOwner()->GetTeamNumber();
		return m_bIsEnemy == !bSameTeam;
	}

	bool m_bIsEnemy;
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFQuestPlayerIsOwnerRestriction, player_is_enemy, FIELD_PLAYER );

//-----------------------------------------------------------------------------
// Purpose: quest class restriction
//-----------------------------------------------------------------------------
class CTFQuestPlayerClassRestriction : public CTFQuestBasePlayerRestriction
{
public:
	DECLARE_CLASS( CTFQuestPlayerClassRestriction, CTFQuestBasePlayerRestriction )

	CTFQuestPlayerClassRestriction()
	{
		m_iClass = TF_CLASS_UNDEFINED;
	}

	virtual const char *GetValueString() const OVERRIDE
	{
		return GetItemSchema()->GetClassUsabilityStrings()[ m_iClass ];
	}

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestBasePlayerRestriction::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		m_strValue = pKVItem->GetString( "value", NULL );
		m_iClass = StringFieldToInt( m_strValue, GetItemSchema()->GetClassUsabilityStrings(), true );
		SCHEMA_INIT_CHECK( IsValidTFPlayerClass( m_iClass ), "%s", CFmtStr( "Invalid owner class restriction '%s' for quest objective", m_strValue.Get() ).Get() );

		return true;
	}

	virtual bool IsValidForPlayer( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
	{
		BaseClass::IsValidForPlayer( pOwner, invalidReasons );

		if ( !BPlayerCheck( pOwner, NULL ) )
			invalidReasons.m_bits.Set( INVALID_QUEST_REASON_WRONG_CLASS );

		return false;
	}

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE
	{
		CTFQuestBasePlayerRestriction::GetRequiredParamKeys( pRequiredKeys );

		KeyValues *pClassesKey = new KeyValues( "value" );
		for ( int i=0; i<GetItemSchema()->GetClassUsabilityStrings().Count(); ++i )
		{
			const char *pszClassName = GetItemSchema()->GetClassUsabilityStrings()[i];
			pClassesKey->AddSubKey( new KeyValues( pszClassName ) );
		}

		pClassesKey->AddSubKey( new KeyValues( "$var1" ) );
		pClassesKey->AddSubKey( new KeyValues( "$var2" ) );
		pClassesKey->AddSubKey( new KeyValues( "$var3" ) );

		pRequiredKeys->AddSubKey( pClassesKey );
	}

	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE
	{
		CTFQuestBasePlayerRestriction::GetOutputKeyValues( pOutputKeys );

		if ( m_iClass >= 0 && m_iClass <= GetItemSchema()->GetClassUsabilityStrings().Count() )
		{
			pOutputKeys->SetString( "value", GetValueString() );
		}
		else
		{
			pOutputKeys->SetString( "value", m_strValue );
		}
	}

private:

	virtual bool BPlayerCheck( const CTFPlayer* pPlayer, IGameEvent *pEvent ) const OVERRIDE
	{
		// Check if the classes match
		int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
		return m_iClass == iClass;
	}


	CUtlString m_strValue;
	int m_iClass;
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFQuestPlayerClassRestriction, player_class, FIELD_PLAYER );

//-----------------------------------------------------------------------------
// Purpose: quest player condition restriction
//-----------------------------------------------------------------------------
class CTFQuestPlayerConditionRestriction : public CTFQuestBasePlayerRestriction
{
public:
	virtual const char *GetValueString() const OVERRIDE
	{
		return GetTFConditionName( m_eCondition );
	}

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestBasePlayerRestriction::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		const char *pszConditionName = pKVItem->GetString( "value", NULL );
		m_eCondition = GetTFConditionFromName( pszConditionName );
		SCHEMA_INIT_CHECK( m_eCondition != TF_COND_INVALID, "%s", CFmtStr( "Invalid %s restriction '%s' for quest objective", GetConditionName(), pszConditionName ).Get() );

		m_bOwnerMustBeProvider = pKVItem->GetBool( "provider_must_be_owner", false );

		return true;
	}

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE
	{
		CTFQuestBasePlayerRestriction::GetRequiredParamKeys( pRequiredKeys );

		KeyValues *pConditionsKey = new KeyValues( "value" );
		for ( int i=0; i<TF_COND_LAST; ++i )
		{
			const char *pszConditionName = GetTFConditionName( ETFCond( i ) );
			pConditionsKey->AddSubKey( new KeyValues( pszConditionName ) );
		}
		pRequiredKeys->AddSubKey( pConditionsKey );

		KeyValues *pRequireOwnerIsProvider = new KeyValues( "provider_must_be_owner" );

		KeyValues *pIsOwnerKeyChoiceTrue = new KeyValues( "1" );
		pIsOwnerKeyChoiceTrue->SetString( "english_name", "true" );
		pRequireOwnerIsProvider->AddSubKey( pIsOwnerKeyChoiceTrue );

		KeyValues *pIsOwnerKeyChoiceFalse = new KeyValues( "0" );
		pIsOwnerKeyChoiceFalse->SetString( "english_name", "false" );
		pRequireOwnerIsProvider->AddSubKey( pIsOwnerKeyChoiceFalse );

		pRequiredKeys->AddSubKey( pRequireOwnerIsProvider );
	}

	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE
	{
		CTFQuestBasePlayerRestriction::GetOutputKeyValues( pOutputKeys );

		pOutputKeys->SetString( "value", GetValueString() );
		pOutputKeys->SetBool( "provider_must_be_owner", m_bOwnerMustBeProvider );
	}

private:

	virtual bool BPlayerCheck( const CTFPlayer* pPlayer, IGameEvent *pEvent ) const OVERRIDE
	{
		// Must be in the condition
		if ( !pPlayer->m_Shared.InCond( m_eCondition ) )
			return false;

		// Might require the owner is the provider
		if ( m_bOwnerMustBeProvider && pPlayer->m_Shared.GetConditionProvider( m_eCondition ) != GetQuestOwner() )
			return false;
		
		return true;
	}

	ETFCond m_eCondition;
	bool m_bOwnerMustBeProvider;
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFQuestPlayerConditionRestriction, player_condition, FIELD_PLAYER );

//-----------------------------------------------------------------------------
// Purpose: quest player condition restriction
//-----------------------------------------------------------------------------
class CTFQuestPlayerObjectRestriction : public CTFQuestBasePlayerRestriction
{
public:
	DECLARE_CLASS( CTFQuestPlayerObjectRestriction, CTFQuestBasePlayerRestriction )

	CTFQuestPlayerObjectRestriction()
		: m_pszObjectKey( NULL ),
		  m_eObjectType( OBJ_DISPENSER )
	{}

	virtual const char *GetValueString() const OVERRIDE
	{
		return GetObjectInfo( m_eObjectType )->m_pObjectName;
	}

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestBasePlayerRestriction::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		m_pszObjectKey = pKVItem->GetString( "object_key", NULL );
		SCHEMA_INIT_CHECK( m_pszObjectKey != NULL, "Missing object_key" );

		const char *pszObjectTypeName = pKVItem->GetString( "value", NULL );
		m_eObjectType = (ObjectType_t)GetBuildableId( pszObjectTypeName );
		SCHEMA_INIT_CHECK( m_eObjectType != OBJ_LAST, "%s", CFmtStr( "Invalid %s restriction '%s' for quest objective", GetConditionName(), pszObjectTypeName ).Get() );

		return true;
	}

	virtual bool IsValidForPlayer( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
	{
		CTFQuestBasePlayerRestriction::IsValidForPlayer( pOwner, invalidReasons );

		if ( !pOwner->IsPlayerClass( TF_CLASS_ENGINEER ) )
			invalidReasons.m_bits.Set( INVALID_QUEST_REASON_WRONG_CLASS );

		return false;
	}

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE
	{
		CTFQuestBasePlayerRestriction::GetRequiredParamKeys( pRequiredKeys );

		GetValidParamsKeyFromEvent( "object_key", CTFQuestPlayerObjectRestriction::GetConditionName(), m_pszEventName, pRequiredKeys );

		KeyValues *pObjectKey = new KeyValues( "value" );
		for ( int i=0; i<OBJ_LAST; ++i )
		{
			const char *pszObjectName = GetObjectInfo( ObjectType_t(i) )->m_pObjectName;
			pObjectKey->AddSubKey( new KeyValues( pszObjectName ) );
		}

		pRequiredKeys->AddSubKey( pObjectKey );
	}

	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE
	{
		CTFQuestBasePlayerRestriction::GetOutputKeyValues( pOutputKeys );

		pOutputKeys->SetString( "object_key", m_pszObjectKey );
		pOutputKeys->SetString( "value", GetValueString() );
	}

private:

	virtual bool BPlayerCheck( const CTFPlayer* pPlayer, IGameEvent *pEvent ) const  OVERRIDE
	{
		int nEntIndex = pEvent->GetInt( m_pszObjectKey );
#ifdef GAME_DLL
		CBaseEntity *pObj = UTIL_EntityByIndex( nEntIndex );
#else
		CBaseEntity *pObj = ClientEntityList().GetEnt( nEntIndex );
#endif
		CBaseObject *pPlayerObj = pPlayer->GetObjectOfType( m_eObjectType );
		return pObj && pPlayerObj && pObj == pPlayerObj;
	}
	const char *m_pszObjectKey;
	ObjectType_t m_eObjectType;
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFQuestPlayerObjectRestriction, object_type, FIELD_PLAYER | FIELD_OBJECT );

//-----------------------------------------------------------------------------
// Purpose: quest player condition restriction
//-----------------------------------------------------------------------------
class CTFQuestScorerRestriction : public CTFQuestBasePlayerRestriction
{
public:
	CTFQuestScorerRestriction()
		: m_pszScorerKey( NULL )
	{}

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestBasePlayerRestriction::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		m_pszScorerKey = pKVItem->GetString( "scorer_key", NULL );
		SCHEMA_INIT_CHECK( m_pszScorerKey != NULL, "Missing scorer key" );

		return true;
	}

	virtual bool IsValidForPlayer( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const OVERRIDE
	{
		BaseClass::IsValidForPlayer( pOwner, invalidReasons );

		if ( !pOwner->IsPlayerClass( TF_CLASS_ENGINEER ) )
			invalidReasons.m_bits.Set( INVALID_QUEST_REASON_WRONG_CLASS );

		return false;
	}

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE
	{
		CTFQuestBasePlayerRestriction::GetRequiredParamKeys( pRequiredKeys );

		GetValidParamsKeyFromEvent( "scorer_key", GetConditionName(), m_pszEventName, pRequiredKeys );
	}

	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE
	{
		CTFQuestBasePlayerRestriction::GetOutputKeyValues( pOutputKeys );

		pOutputKeys->SetString( "scorer_key", m_pszScorerKey );
	}

private:

	virtual bool BPlayerCheck( const CTFPlayer* pPlayer, IGameEvent *pEvent ) const OVERRIDE
	{
#ifdef GAME_DLL
		int nEntIndex = pEvent->GetInt( m_pszScorerKey );

		CBaseEntity *pEnt = UTIL_EntityByIndex( nEntIndex );
		IScorer* pScorer = dynamic_cast< IScorer* >( pEnt );
		
		if ( !pScorer )
			return false;

		return pScorer->GetScorer() == pPlayer;
#else
		return true;
#endif
	}

	const char *m_pszScorerKey;
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFQuestScorerRestriction, scorer, FIELD_PLAYER | FIELD_SCORER );

//-----------------------------------------------------------------------------
// Purpose: quest weapon restriction
//-----------------------------------------------------------------------------
class CTFQuestWeaponTypeRestriction : public CTFGenericStringRestriction
{
public:
	virtual const char *GetValueString() const OVERRIDE
	{
		return WeaponIdToAlias( m_eWeaponType );
	}

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFGenericStringRestriction::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		m_eWeaponType = (ETFWeaponType)atoi( m_pszValue );
		SCHEMA_INIT_CHECK( m_eWeaponType != TF_WEAPON_NONE, "%s", CFmtStr( "Invalid weapon restriction '%s' for quest objective", m_pszValue ).Get() );

		return true;
	}

	virtual bool PassesRestrictions( IGameEvent *pEvent ) const OVERRIDE
	{
		ETFWeaponType weaponID = (ETFWeaponType)pEvent->GetInt( m_pszKeyName );
		return m_eWeaponType == weaponID;
	}

private:
	ETFWeaponType m_eWeaponType;
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFQuestWeaponTypeRestriction, weapon_type, FIELD_WEAPON_TYPE );


//-----------------------------------------------------------------------------
// Purpose: quest custom damage restriction
//-----------------------------------------------------------------------------
class CTFQuestCustomDamageRestriction : public CTFQuestRestriction
{
public:
	virtual const char *GetValueString() const OVERRIDE
	{
		return m_strValue;
	}

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestRestriction::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		m_strValue = pKVItem->GetString( "value", NULL );
		m_eCustomDamageType = GetCustomDamageFromName( m_strValue );
		SCHEMA_INIT_CHECK( m_eCustomDamageType != TF_DMG_CUSTOM_NONE, "%s", CFmtStr( "Invalid weapon restriction '%s' for quest objective", m_strValue.Get() ).Get() );

		m_pszCustomDamageKey = pKVItem->GetString( "custom_damage_key", NULL );
		SCHEMA_INIT_CHECK( m_pszCustomDamageKey != NULL, "Invalid custom_damage_key!" );

		return true;
	}

	virtual bool PassesRestrictions( IGameEvent *pEvent ) const OVERRIDE
	{
		ETFDmgCustom customDamageType = (ETFDmgCustom)pEvent->GetInt( m_pszCustomDamageKey );
		return m_eCustomDamageType == customDamageType;
	}

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE
	{
		CTFQuestRestriction::GetRequiredParamKeys( pRequiredKeys );

		GetValidParamsKeyFromEvent( "custom_damage_key", GetConditionName(), m_pszEventName, pRequiredKeys );

		KeyValues *pDamageKey = new KeyValues( "value" );
		for ( int i=0; i<TF_DMG_CUSTOM_END; ++i )
		{
			const char *pszCustomDamageName = GetCustomDamageName( ETFDmgCustom( i ) );
			pDamageKey->AddSubKey( new KeyValues( pszCustomDamageName ) );
		}

		pDamageKey->AddSubKey( new KeyValues( "$var1" ) );
		pDamageKey->AddSubKey( new KeyValues( "$var2" ) );
		pDamageKey->AddSubKey( new KeyValues( "$var3" ) );

		pRequiredKeys->AddSubKey( pDamageKey );
	}

	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE
	{
		CTFQuestRestriction::GetOutputKeyValues( pOutputKeys );

		pOutputKeys->SetString( "value", GetValueString() );
	}

private:
	ETFDmgCustom m_eCustomDamageType;
	const char *m_pszCustomDamageKey;
	CUtlString m_strValue;
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFQuestCustomDamageRestriction, custom_damage, FIELD_CUSTOM_DAMAGE );


//-----------------------------------------------------------------------------
// Purpose: quest flag event restriction
//-----------------------------------------------------------------------------
class CTFFlagEventTypeRestriction : public CTFQuestRestriction
{
public:

	CTFFlagEventTypeRestriction()
		: m_eEventType( TF_FLAGEVENT_CAPTURE )
		, m_pszKeyName( NULL )
	{}

	virtual const char *GetValueString() const OVERRIDE
	{
		return GetCTFEventName( m_eEventType );
	}

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestRestriction::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		const char *pszEventType = pKVItem->GetString( "value", NULL );
		m_eEventType = GetCTFEventTypeFromName( pszEventType );
		SCHEMA_INIT_CHECK( m_eEventType != TF_NUM_FLAG_EVENTS, "%s", CFmtStr( "Invalid CTF Event Type '%s' for quest objective", pszEventType ).Get() );

		m_pszKeyName = pKVItem->GetString( "event_key", "eventtype" );
		SCHEMA_INIT_CHECK( m_pszKeyName != NULL, "Missing \"event_key\" for flag_event_type" );

		return true;
	}

	virtual bool PassesRestrictions( IGameEvent *pEvent ) const OVERRIDE
	{
		ETFFlagEventTypes eEventType = (ETFFlagEventTypes)pEvent->GetInt( m_pszKeyName );
		return m_eEventType == eEventType;
	}

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE
	{
		CTFQuestRestriction::GetRequiredParamKeys( pRequiredKeys );

		GetValidParamsKeyFromEvent( "event_key", "flag_event", m_pszEventName, pRequiredKeys );

		KeyValues *pEventKey = new KeyValues( "value" );
		for ( int i=1; i<TF_NUM_FLAG_EVENTS; ++i )
		{
			const char *pszEventType = GetCTFEventName( ETFFlagEventTypes( i ) );
			pEventKey->AddSubKey( new KeyValues( pszEventType ) );
		}

		pRequiredKeys->AddSubKey( pEventKey );
	}

	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE
	{
		CTFQuestRestriction::GetOutputKeyValues( pOutputKeys );

		pOutputKeys->SetString( "value", GetValueString() );
		pOutputKeys->SetString( "event_key", m_pszKeyName );
	}

private:
	ETFFlagEventTypes m_eEventType;
	const char* m_pszKeyName;
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFFlagEventTypeRestriction, flag_event_type, FIELD_FLAG_EVENT );

static const char *s_pszTeamRestrictionNames[] = 
{
	"TEAM_ANY",
	"TEAM_IS_OWNERS",
	"TEAM_IS_NOT_OWNERS",
};
//-----------------------------------------------------------------------------
// Purpose: quest custom damage restriction
//-----------------------------------------------------------------------------
class CTFQuestTeamRestriction : public CTFQuestRestriction
{
public:

	enum ETeamRestriction
	{
		TEAM_RESTRICTION_ANY = 0,
		TEAM_RESTRICTION_IS_OWNERS,
		TEAM_RESTRICTION_IS_NOT_OWNERS,

		TEAM_RESTRICTION_MAX
	};

	CTFQuestTeamRestriction()
		: m_eTeamRestriction( TEAM_RESTRICTION_ANY )
		, m_pszTeamKey( NULL )
	{}

	virtual const char *GetValueString() const OVERRIDE
	{
		return s_pszTeamRestrictionNames[ m_eTeamRestriction ];
	}

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestRestriction::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		m_eTeamRestriction = (ETeamRestriction)pKVItem->GetInt( "team_requirement", TEAM_RESTRICTION_ANY );

		m_pszTeamKey = pKVItem->GetString( "team_key", NULL );
		SCHEMA_INIT_CHECK( m_pszTeamKey != NULL, "Missing \"m_pszTeamKey\" in team_restriction" );

		return true;
	}

	virtual bool PassesRestrictions( IGameEvent *pEvent ) const OVERRIDE
	{
		int nTeam = pEvent->GetInt( m_pszTeamKey, TEAM_INVALID );
		if ( nTeam == TEAM_INVALID )
		{
			AssertMsg( 0, "This event doesn't specify a team." );
			return false;
		}

		const CTFPlayer *pOwner = GetQuestOwner();
		bool bTeamIsOwners = nTeam == pOwner->GetTeamNumber();
		if ( ( m_eTeamRestriction == TEAM_RESTRICTION_IS_OWNERS && !bTeamIsOwners ) ||
			( m_eTeamRestriction == TEAM_RESTRICTION_IS_NOT_OWNERS && bTeamIsOwners ) )
		{
			return false;
		}

		return true;
	}

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE
	{
		CTFQuestRestriction::GetRequiredParamKeys( pRequiredKeys );

		GetValidParamsKeyFromEvent( "team_key", GetConditionName(), m_pszEventName, pRequiredKeys );

		KeyValues *pTeamReq = new KeyValues( "team_requirement" );

		for( int i=0; i<TEAM_RESTRICTION_MAX; ++i )
		{
			KeyValues *pTeam = new KeyValues( CFmtStr( "%d", i ) );
			pTeam->SetString( "english_name", s_pszTeamRestrictionNames[ i ] );
			pTeamReq->AddSubKey( pTeam );
		}

		pRequiredKeys->AddSubKey( pTeamReq );
	}

	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE
	{
		CTFQuestRestriction::GetOutputKeyValues( pOutputKeys );

		pOutputKeys->SetInt( "team_requirement", m_eTeamRestriction );
		pOutputKeys->SetString( "team_key", m_pszTeamKey );
	}

private:
	ETeamRestriction m_eTeamRestriction;
	const char* m_pszTeamKey;
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFQuestTeamRestriction, team_restriction, FIELD_TEAM );

//-----------------------------------------------------------------------------
// Purpose: quest map restriction
//-----------------------------------------------------------------------------
class CTFQuestMapRestriction : public CTFQuestRestriction
{
public:
	DECLARE_CLASS( CTFQuestMapRestriction, CTFQuestRestriction )

	CTFQuestMapRestriction()
		: m_pszMapName( NULL )
	{
	}

	virtual const char *GetValueString() const OVERRIDE
	{
		return m_pszMapName;
	}

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestRestriction::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		m_pszMapName = pKVItem->GetString( "value", NULL );
		SCHEMA_INIT_CHECK( m_pszMapName != NULL, "Missing map name." );
	
		// TODO: validate map name against some white list here

		return true;
	}

	virtual bool PassesRestrictions( IGameEvent *pEvent ) const OVERRIDE
	{
		return IsValidMap();
	}

	virtual bool IsValidForPlayer( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
	{
		BaseClass::IsValidForPlayer( pOwner, invalidReasons );

		if ( !IsValidMap() )
			invalidReasons.m_bits.Set( INVALID_QUEST_REASON_WRONG_MAP );

		return true;
	}

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE
	{
		//const CUtlVector<SchemaMap_t>& maps = GetItemSchema()->GetMaps();
		
		KeyValues *pMapKey = new KeyValues( "value" );
		pMapKey->SetString( "control_type", "text_entry" );
		/*for ( int i=0; i<maps.Count(); ++i )
		{
			const char *pszMapName = maps[i].pszMapName;
			pMapKey->AddSubKey( new KeyValues( pszMapName ) );
		}*/

		pRequiredKeys->AddSubKey( pMapKey );
	}

	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE
	{
		CTFQuestRestriction::GetOutputKeyValues( pOutputKeys );

		pOutputKeys->SetString( "value", GetValueString() );
	}

private:
	bool IsValidMap() const
	{
#ifdef CLIENT_DLL
		const char *pszMapName = TFGameRules()->MapName();
#else
		const char *pszMapName = gpGlobals->mapname.ToCStr();
#endif
		
		return !V_stricmp( m_pszMapName, pszMapName );
	}

	const char *m_pszMapName;
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFQuestMapRestriction, map, FIELD_NONE );


//-----------------------------------------------------------------------------
// Purpose: quest game type restriction
//-----------------------------------------------------------------------------
class CTFQuestGameTypeRestriction : public CTFQuestRestriction
{
public:
	DECLARE_CLASS( CTFQuestGameTypeRestriction, CTFQuestRestriction )

	CTFQuestGameTypeRestriction()
		: m_eGameType( (ETFGameType)0 )
	{}

	virtual const char *GetValueString() const OVERRIDE
	{
		return GetEnumGameTypeName( m_eGameType );
	}

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestRestriction::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		const char *pszVal = pKVItem->GetString( "value", NULL );
		SCHEMA_INIT_CHECK( pszVal != NULL, "Missing game_type name." );
		m_eGameType = GetGameTypeFromName( pszVal );
		SCHEMA_INIT_CHECK( m_eGameType != TF_GAMETYPE_UNDEFINED, "Invalid game_type name." );

		return true;
	}

	virtual bool PassesRestrictions( IGameEvent *pEvent ) const OVERRIDE
	{
		return TFGameRules()->GetGameType() == m_eGameType;
	}

	virtual bool IsValidForPlayer( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
	{
		BaseClass::IsValidForPlayer( pOwner, invalidReasons );

		if ( TFGameRules()->GetGameType() != m_eGameType ) 
			invalidReasons.m_bits.Set( INVALID_QUEST_REASON_WRONG_GAME_MODE );

		return false;
	}

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE
	{
		CTFQuestRestriction::GetRequiredParamKeys( pRequiredKeys );

		KeyValues *pGameType = new KeyValues( "value" );
		for ( int i=0; i<TF_GAMETYPE_COUNT; ++i )
		{
			const char *pszGameType = GetEnumGameTypeName( ETFGameType( i ) );
			pGameType->AddSubKey( new KeyValues( pszGameType ) );
		}

		pRequiredKeys->AddSubKey( pGameType );
	}

	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE
	{
		CTFQuestRestriction::GetOutputKeyValues( pOutputKeys );

		pOutputKeys->SetString( "value", GetValueString() );
	}

private:
	ETFGameType m_eGameType;
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFQuestGameTypeRestriction, game_type, FIELD_NONE );


//-----------------------------------------------------------------------------
// Purpose: quest loadout position restriction
//-----------------------------------------------------------------------------
class CTFQuestLoadoutPositionRestriction : public CTFQuestBasePlayerRestriction
{
public:
	DECLARE_CLASS( CTFQuestLoadoutPositionRestriction, CTFQuestRestriction )

	CTFQuestLoadoutPositionRestriction()
		: m_eLoadoutPosition( LOADOUT_POSITION_INVALID )
		, m_pszLoadoutKey( NULL )
	{}

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestBasePlayerRestriction::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		const char *pszVal = pKVItem->GetString( "value", NULL );
		SCHEMA_INIT_CHECK( pszVal != NULL, "Missing loadout_position name." );
		m_eLoadoutPosition = GetLoadoutPositionByName( pszVal );
		SCHEMA_INIT_CHECK( m_eLoadoutPosition != LOADOUT_POSITION_INVALID, "Invalid loadout_position name." );

		m_pszPlayerKey = pKVItem->GetString( "player_key", NULL );
		SCHEMA_INIT_CHECK( m_pszPlayerKey != NULL, "Missing \"player_key\" in loadout_position" );

		m_pszLoadoutKey = pKVItem->GetString( "loadout_key", NULL );
		SCHEMA_INIT_CHECK( m_pszLoadoutKey != NULL, "Missing \"loadout_key\" in loadout_position" );

		return true;
	}

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE
	{
		CTFQuestBasePlayerRestriction::GetRequiredParamKeys( pRequiredKeys );

		GetValidParamsKeyFromEvent( "loadout_key", GetConditionName(), m_pszEventName, pRequiredKeys );

		KeyValues *pLoadoutPositions = new KeyValues( "value" );
		for ( int i=0; i<CLASS_LOADOUT_POSITION_COUNT; ++i )
		{
			const char *pszLoadoutPosition = GetLoadoutPositionName( loadout_positions_t( i ) );
			pLoadoutPositions->AddSubKey( new KeyValues( pszLoadoutPosition ) );
		}

		pRequiredKeys->AddSubKey( pLoadoutPositions );
	}

	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE
	{
		CTFQuestBasePlayerRestriction::GetOutputKeyValues( pOutputKeys );

		pOutputKeys->SetString( "value", GetLoadoutPositionName( m_eLoadoutPosition ) );

		pOutputKeys->SetString( "loadout_key", m_pszLoadoutKey );
	}

private:

	virtual bool BPlayerCheck( const CTFPlayer* pPlayer, IGameEvent *pEvent ) const OVERRIDE
	{
		int iClass = pPlayer->GetPlayerClass()->GetClassIndex();

		item_definition_index_t iItemDef = pEvent->GetInt( m_pszLoadoutKey, INVALID_ITEM_DEF_INDEX );
		if ( iItemDef != INVALID_ITEM_DEF_INDEX )
		{
			GameItemDefinition_t *pDef = ItemSystem()->GetStaticDataForItemByDefIndex( iItemDef );
			Assert( pDef );
			if ( pDef )
			{
				return pDef->GetLoadoutSlot( iClass ) == m_eLoadoutPosition;
			}
		}
		return false;
	}

	loadout_positions_t m_eLoadoutPosition;
	
	const char* m_pszLoadoutKey;
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFQuestLoadoutPositionRestriction, loadout_position, FIELD_PLAYER | FIELD_LOADOUT_POSITION );

//-----------------------------------------------------------------------------
// Purpose: quest player condition restriction
//-----------------------------------------------------------------------------
class CTFConditionQuestCondition : public CTFQuestRestriction
{
public:
	CTFConditionQuestCondition()
		: m_pszKeyName( NULL )
	{}

	virtual const char *GetValueString() const OVERRIDE
	{
		return GetTFConditionName( m_eCondition );
	}

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		const char *pszKeyName = pKVItem->GetString( "condition_key", NULL );
		SCHEMA_INIT_CHECK( pszKeyName != NULL, "Missing key_to_lookup in condition" );

		const char *pszConditionName = pKVItem->GetString( "value" );
		m_eCondition = GetTFConditionFromName( pszConditionName );
		SCHEMA_INIT_CHECK( m_eCondition != TF_COND_INVALID, "%s", CFmtStr( "Invalid %s restriction '%s' for quest objective", GetConditionName(), pszConditionName ).Get() );

		return true;
	}

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE
	{
		KeyValues *pConditionsKey = new KeyValues( "value" );
		for ( int i=0; i<TF_COND_LAST; ++i )
		{
			const char *pszConditionName = GetTFConditionName( ETFCond( i ) );
			pConditionsKey->AddSubKey( new KeyValues( pszConditionName ) );
		}

		GetValidParamsKeyFromEvent( "condition_key", GetConditionName(), m_pszEventName, pRequiredKeys );

		pRequiredKeys->AddSubKey( pConditionsKey );
	}

	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE
	{
		pOutputKeys->SetString( "value", GetValueString() );
		pOutputKeys->SetString( "condition_key", m_pszKeyName );
	}

private:

	virtual bool PassesRestrictions( IGameEvent *pEvent ) const OVERRIDE
	{
		return pEvent->GetInt( m_pszKeyName ) == m_eCondition;
	}

	ETFCond m_eCondition;
	const char *m_pszKeyName;
};
REGISTER_QUEST_CONDITION_SUB_CLASS( CTFConditionQuestCondition, condition, FIELD_CONDITION );

CTFQuestRestriction *CreateRestrictionByName( const char *pszName, CTFQuestCondition* pParent )
{
	CTFQuestRestriction *pNewRestriction = NULL;
	auto idx = k_mapConditions.Find( pszName );
	if ( idx != k_mapConditions.InvalidIndex() )
	{
		pNewRestriction = static_cast<CTFQuestRestriction*>( k_mapConditions[ idx ]->m_pfnQuestCreate() );
		pNewRestriction->SetFieldName( k_mapConditions[ idx ]->m_pszFieldName );
		pNewRestriction->SetTypeName( k_mapConditions.Key( idx ) );
	}

	if ( pNewRestriction )
	{
		pNewRestriction->SetParent( pParent );

		const char *pszEventName = pParent->GetEventName();

		// in the case that parent has no event name (new node from editor)
		// default the event name to the first valid event from the global event list
		if ( !pszEventName )
		{
			KeyValues *pQuestEvents = GetQuestEventsKeyValues();
			if ( pQuestEvents )
			{
				const KeyValues *pFirstKey = pQuestEvents->GetFirstTrueSubKey();
				if ( pFirstKey )
				{
					pszEventName = pFirstKey->GetName();
				}
			}
		}

		pNewRestriction->SetEventName( pszEventName );
	}
	
	AssertMsg( pNewRestriction, "Invalid quest restriction type '%s'", pszName );
	return pNewRestriction;
}


//-----------------------------------------------------------------------------
// Purpose: quest event
//-----------------------------------------------------------------------------
class CTFQuestEventListener : public CTFQuestEvaluator, public CGameEventListener
{
public:
	DECLARE_CLASS( CTFQuestEventListener, CTFQuestEvaluator )

	CTFQuestEventListener()
	{
		m_pRestrictions = NULL;
		m_pszEventName = NULL;
		m_pszOverrideScoreKeyName = NULL;
	}

	virtual ~CTFQuestEventListener()
	{
		if ( m_pRestrictions )
		{
			delete m_pRestrictions;
		}
	}

	virtual const char *GetConditionName() const OVERRIDE { return "event_listener"; }
	virtual const char *GetValueString() const OVERRIDE { return m_pszEventName; }

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestEvaluator::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		m_pszEventName = pKVItem->GetString( "event_name", NULL );
		SCHEMA_INIT_CHECK( m_pszEventName != NULL, "%s", CFmtStr( "Invalid %s condition. Missing 'event_name'", GetConditionName() ).Get() );
#ifdef GAME_DLL // Only the server needs to listen for events
		ListenForGameEvent( m_pszEventName );
#endif

		m_pszOverrideScoreKeyName = pKVItem->GetString( "score_key_name", "none" );
		SCHEMA_INIT_CHECK( m_pszOverrideScoreKeyName != NULL, "Missing score_key_name" );

		FOR_EACH_TRUE_SUBKEY( pKVItem, pSubKey )
		{
			SCHEMA_INIT_CHECK( !m_pRestrictions, "%s", CFmtStr( "Too many input for operator '%s'.", GetConditionName() ).Get() );

			const char *pszType = pSubKey->GetString( "type" );
			m_pRestrictions = CreateRestrictionByName( pszType, this );
			SCHEMA_INIT_CHECK( m_pRestrictions != NULL, "%s", CFmtStr( "Failed to create quest restriction name '%s' for '%s'", pszType, GetConditionName() ).Get() );

			SCHEMA_INIT_CHECK( m_pRestrictions->BInitFromKV( pSubKey, pVecErrors ), "Failed to init from KeyValues" );
		}

		return true;
	}

	virtual bool IsValidForPlayer( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const OVERRIDE
	{
		bool bValid = BaseClass::IsValidForPlayer( pOwner, invalidReasons );

		int nNumFound = 0;
		for ( int i = 1; i <= gpGlobals->maxClients && nNumFound < s_nMinConnectedPlayersForQuestProgress; ++i )
		{
#ifdef CLIENT_DLL
			IGameResources *gr = GameResources();
			if ( !gr || !gr->IsConnected( i ) )
				continue;
#else
			CBasePlayer* pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( !pPlayer )
				continue;
#endif

			++nNumFound;
		}

		if ( nNumFound < s_nMinConnectedPlayersForQuestProgress )
		{
			invalidReasons.m_bits.Set( INVALID_QUEST_REASON_NOT_ENOUGH_PLAYERS );
			bValid = false;
		}

		if ( m_pRestrictions )
		{
			bValid &= m_pRestrictions->IsValidForPlayer( pOwner, invalidReasons );
		}

		return bValid;
	}

	virtual void FireGameEvent( IGameEvent *pEvent ) OVERRIDE
	{
		// This can happen when the player's SteamID isn't setup yet after a 
		// disconnect -> reconnect
		if ( GetQuestOwner() == NULL )
			return;

		InvalidReasonsContainer_t invalidReasons;
		if ( !IsValidForPlayer( GetQuestOwner(), invalidReasons ) )
			return;

		if ( !invalidReasons.m_bits.IsAllClear() )
			return;

		const char *pszEventName = pEvent->GetName();
		if ( FStrEq( m_pszEventName, pszEventName ) )
		{
			if ( !m_pRestrictions || m_pRestrictions->PassesRestrictions( pEvent ) )
			{
				int nScoreOverride = m_pszOverrideScoreKeyName ? pEvent->GetInt( m_pszOverrideScoreKeyName, 1 ) : 1;

				EvaluateCondition( this, nScoreOverride );
			}
		}
	}

	virtual void EvaluateCondition( CTFQuestEvaluator *pSender, int nScore ) OVERRIDE
	{
		Assert( GetParent() && GetParent()->IsEvaluator() );
		assert_cast< CTFQuestEvaluator* >( GetParent() )->EvaluateCondition( pSender, nScore ) ;
	}


	virtual void ResetCondition() OVERRIDE
	{
		// DO NOTHING
	}

	CTFQuestCondition* AddChildByName( const char *pszChildName ) OVERRIDE
	{
		if ( m_pRestrictions )
		{
			Assert( m_pRestrictions == NULL );
			return NULL;
		}
		
		m_pRestrictions = CreateRestrictionByName( pszChildName, this );
		Assert( m_pRestrictions );
		return m_pRestrictions;
	}

	virtual int GetChildren( CUtlVector< CTFQuestCondition* >& vecChildren ) OVERRIDE
	{
		if ( m_pRestrictions )
		{
			vecChildren.AddToTail( m_pRestrictions );
			return vecChildren.Count();
		}

		return 0;
	}

	bool RemoveAndDeleteChild( CTFQuestCondition *pChild ) OVERRIDE
	{
		bool bRemoved = m_pRestrictions == pChild;
		Assert( bRemoved );

		if ( bRemoved )
		{
			delete m_pRestrictions;
			m_pRestrictions = NULL;
		}

		return bRemoved;
	}

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE
	{
		CTFQuestEvaluator::GetRequiredParamKeys( pRequiredKeys );

		KeyValues *pQuestEvents = GetQuestEventsKeyValues();
		if ( pQuestEvents )
		{
			KeyValues *pScoreKeys = new KeyValues( "score_key_name" );
			KeyValues *pEventsKey = new KeyValues( "event_name" );
			FOR_EACH_TRUE_SUBKEY( pQuestEvents, pSubKey )
			{
				const char *pszEventName = pSubKey->GetName();
				pEventsKey->AddSubKey( new KeyValues( pszEventName ) );

				if ( m_pszEventName && FStrEq( pszEventName, m_pszEventName ) )
				{
					KeyValues* pScoreSubKeys = pSubKey->FindKey( "score_keys" );
					if ( pScoreSubKeys )
					{
						FOR_EACH_TRUE_SUBKEY( pScoreSubKeys, pScore )
						{
							pScoreKeys->AddSubKey( new KeyValues( pScore->GetName() ) );
						}
					}
				}
			}

			pRequiredKeys->AddSubKey( pEventsKey );
			if ( pScoreKeys->GetFirstTrueSubKey() )
			{
				pScoreKeys->AddSubKey( new KeyValues( "none" ) );
				pRequiredKeys->AddSubKey( pScoreKeys );
			}
		}

	}

	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE
	{
		CTFQuestEvaluator::GetOutputKeyValues( pOutputKeys );

		pOutputKeys->SetString( "event_name", GetValueString() );
		pOutputKeys->SetString( "score_key_name", m_pszOverrideScoreKeyName );
	}

	virtual const char* GetEventName() const OVERRIDE
	{ 
		return m_pszEventName;
	}

	virtual void SetEventName( const char *pszEventName ) OVERRIDE
	{
		m_pszEventName = pszEventName;
	}

	virtual int GetMaxInputCount() const { return 1; }

protected:


	const char *m_pszEventName;
	const char *m_pszOverrideScoreKeyName;
	CTFQuestRestriction *m_pRestrictions;
};


//-----------------------------------------------------------------------------
// Purpose: count evaluator
//-----------------------------------------------------------------------------
class CTFQuestCountEvaluator : public CTFQuestEvaluator
{
public:
	DECLARE_CLASS( CTFQuestCountEvaluator, CTFQuestEvaluator )

	virtual ~CTFQuestCountEvaluator()
	{
		m_vecChildren.PurgeAndDeleteElements();
	}

	virtual const char *GetConditionName() const OVERRIDE { return "counter"; }

	virtual bool BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ ) OVERRIDE
	{
		if ( !CTFQuestEvaluator::BInitFromKV( pKVItem, pVecErrors ) )
			return false;

		m_flPeriod = pKVItem->GetFloat( "period" );
		m_strEnd = pKVItem->GetString( "end" );
		m_nEnd = pKVItem->GetInt( "end" );

		FOR_EACH_TRUE_SUBKEY( pKVItem, pSubKey )
		{
			const char *pszType = pSubKey->GetString( "type" );
			CTFQuestEvaluator *pNewCond = assert_cast< CTFQuestEvaluator* >( CreateEvaluatorByName( pszType, this ) );
			SCHEMA_INIT_CHECK( pNewCond && pNewCond->BInitFromKV( pSubKey, pVecErrors ), "Failed to init from KeyValues" );

			const char *pszAction = pSubKey->GetString( "action", NULL );
			SCHEMA_INIT_CHECK( pszAction != NULL, "Missing action key" );
			pNewCond->SetAction( pszAction );

			m_vecChildren.AddToTail( pNewCond );
		}

		return true;
	}

	virtual bool IsValidForPlayer( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
	{
		bool bValid = BaseClass::IsValidForPlayer( pOwner, invalidReasons );

		FOR_EACH_VEC( m_vecChildren, i )
		{
			bValid &= m_vecChildren[i]->IsValidForPlayer( pOwner, invalidReasons );
		}

		return bValid;
	}

	virtual void EvaluateCondition( CTFQuestEvaluator *pSender, int nScore ) OVERRIDE
	{
		InvalidReasonsContainer_t invalidReasons;
		if ( !IsValidForPlayer( GetQuestOwner(), invalidReasons ) )
			return;

		if ( !invalidReasons.m_bits.IsAllClear() )
			return;

		const char *pszAction = pSender->GetAction();
		if ( FStrEq( pszAction, "increment" ) )
		{
			ScoreRecord_t& score = m_vecScoreRecords[ m_vecScoreRecords.AddToTail() ];
			score.m_nCount = nScore;
			score.m_flScoreTime = gpGlobals->curtime;
		}
		else if ( FStrEq( pszAction, "reset" ) )
		{
			m_vecScoreRecords.Purge();
		}
		else
		{
			AssertMsg( 0, "Invalid evaluation condition '%s' for '%s'", pSender->GetConditionName(), GetConditionName() );
		}

		int nTotalCount = GetTotalCount();

		// Check how many times over we've scored
		int nNumScored = 1;
		// If m_nEnd is 0, the event happening (regardless of the value) counts as 1 score
		if ( m_nEnd > 0 )
		{
			nNumScored = nTotalCount / m_nEnd;
		}

		if ( nNumScored > 0 )
		{
			Assert( GetParent() && GetParent()->IsEvaluator() );
			assert_cast< CTFQuestEvaluator* >( GetParent() )->EvaluateCondition( this, nNumScored ) ;

			// Consume all records
			m_vecScoreRecords.Purge();

			if ( m_nEnd > 0 )
			{
				// Check if there's a remainder
				nTotalCount -= ( nNumScored * m_nEnd );
			}
			else
			{
				// If m_nEnd is 0, no remainder
				nTotalCount = 0;
			}

			// Store the remainder in a new record
			if ( nTotalCount )
			{
				ScoreRecord_t& score = m_vecScoreRecords[ m_vecScoreRecords.AddToTail() ];
				score.m_flScoreTime = gpGlobals->curtime;
				score.m_nCount = nTotalCount;
			}
		}
	}

	virtual void ResetCondition() OVERRIDE
	{
		m_vecScoreRecords.Purge();
		
		FOR_EACH_VEC( m_vecChildren, i )
		{
			m_vecChildren[i]->ResetCondition();
		}
	}

	enum ECounterSubType
	{
		COUNTER_INCREMENT = 0,
		COUNTER_RESET,

		COUNTER_TYPE_COUNT
	};


	virtual int GetChildrenSubTypeCount() const { return COUNTER_TYPE_COUNT; }

	virtual int GetChildren( CUtlVector< CTFQuestCondition* >& vecChildren ) OVERRIDE
	{
		for ( int i=0; i<m_vecChildren.Count(); ++i )
		{
			vecChildren.AddToTail( m_vecChildren[i] );
		}
		return vecChildren.Count();
	}

	bool RemoveAndDeleteChild( CTFQuestCondition *pChild ) OVERRIDE
	{
		CTFQuestEvaluator *pEvaluatorChild = assert_cast< CTFQuestEvaluator* >( pChild );
		
		bool bRemoved = m_vecChildren.FindAndFastRemove( pEvaluatorChild );
		Assert( bRemoved );

		if ( bRemoved )
		{
			delete pChild;
		}

		return bRemoved;
	}

	CTFQuestCondition* AddChildByName( const char *pszChildName ) OVERRIDE
	{
		CTFQuestEvaluator *pNewEvaluator = CreateEvaluatorByName( pszChildName, this );
		if ( pNewEvaluator )
		{
			m_vecChildren.AddToTail( pNewEvaluator );
		}
		return pNewEvaluator;
	}

	virtual int GetMaxInputCount() const { return s_nMaxInputCount; }

	virtual void GetRequiredParamKeys( KeyValues *pRequiredKeys ) OVERRIDE
	{
		CTFQuestEvaluator::GetRequiredParamKeys( pRequiredKeys );

		KeyValues* pKVCounts = pRequiredKeys->CreateNewKey();
		pKVCounts->SetName( "end" );
		pKVCounts->SetString( "control_type", "text_entry" );
	}

	virtual void GetOutputKeyValues( KeyValues *pOutputKeys ) OVERRIDE
	{
		CTFQuestEvaluator::GetOutputKeyValues( pOutputKeys );

		pOutputKeys->SetString( "end", m_strEnd );
	}

	virtual void GetValidChildren( CUtlVector< const char* >& vecOutValidChildren ) const OVERRIDE
	{
		vecOutValidChildren.AddToTail( "event_listener" );
		vecOutValidChildren.AddToTail( "counter" );
	}

private:

	CUtlVector< CTFQuestEvaluator* > m_vecChildren;

	int GetTotalCount()
	{
		PruneOldRecords();

		int nAccum = 0;
		FOR_EACH_VEC( m_vecScoreRecords, i )
		{
			nAccum += m_vecScoreRecords[ i ].m_nCount;
		}

		return nAccum;
	}

	void PruneOldRecords()
	{
		// 0 period means don't use a period
		if ( m_flPeriod == 0.f )
			return;

		float flCutoffTime = gpGlobals->curtime - m_flPeriod;
		FOR_EACH_VEC_BACK( m_vecScoreRecords, i )
		{
			if ( m_vecScoreRecords[ i ].m_flScoreTime < flCutoffTime )
			{
				m_vecScoreRecords.Remove( i );
			}
		}
	}

	struct ScoreRecord_t
	{
		float m_flScoreTime;
		int m_nCount;
	};
	CUtlVector< ScoreRecord_t > m_vecScoreRecords;

	int m_nEnd;
	float m_flPeriod;
	CUtlString m_strEnd;
};


void CTFQuestEvaluator::GetRequiredParamKeys( KeyValues *pRequiredKeys )
{
	if ( GetParent() && dynamic_cast< CTFQuestCountEvaluator* >( GetParent() ) )
	{
		KeyValues *pActionsKey = new KeyValues( "action" );
		pActionsKey->AddSubKey( new KeyValues( "increment" ) );
		pActionsKey->AddSubKey( new KeyValues( "reset" ) );

		pRequiredKeys->AddSubKey( pActionsKey );
	}
}


CTFQuestEvaluator *CreateEvaluatorByName( const char *pszName, CTFQuestCondition* pParent )
{
	CTFQuestEvaluator *pNewEvaluator = NULL;
	if ( FStrEq( pszName, "event_listener" ) )
	{
		pNewEvaluator = new CTFQuestEventListener;
	}
	else if ( FStrEq( pszName, "counter" ) )
	{
		pNewEvaluator = new CTFQuestCountEvaluator;
	}

	if ( pNewEvaluator )
	{
		pNewEvaluator->SetParent( pParent );
	}
	
	AssertMsg( pNewEvaluator, "Invalid quest evaluator type '%s'", pszName );
	return pNewEvaluator;
}


bool CTFClassQuestModifier::BPassesModifier( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
{
	int iClass = pOwner->GetPlayerClass()->GetClassIndex();
	if ( ( m_nValidClassesMask & ( 1 << iClass ) ) == 0 )
	{
		invalidReasons.m_bits.Set( INVALID_QUEST_REASON_WRONG_CLASS );
		return false;
	}

	return true;
}

void CTFMapQuestModifier::AddMapName( const char* pszMapName )
{
	m_vecStrMapNames.AddToTail( pszMapName );
}

bool CTFMapQuestModifier::BPassesModifier( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
{
#ifdef CLIENT_DLL
	const char *pszMapName = TFGameRules()->MapName();
#else
	const char *pszMapName = gpGlobals->mapname.ToCStr();
#endif

	// Just need one to match
	FOR_EACH_VEC( m_vecStrMapNames, i )
	{
		if ( !V_stricmp( pszMapName, m_vecStrMapNames[ i ].Get() ) )
		{
			return true;
		}
	}

	invalidReasons.m_bits.Set( INVALID_QUEST_REASON_WRONG_MAP );
	return false;
}

bool CTFGameModeQuestModifier::BPassesModifier( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
{
	if ( !TFGameRules() )
		return false;

#ifdef CLIENT_DLL
	const MapDef_t* pMapDef = GetItemSchema()->GetMasterMapDefByName ( TFGameRules()->MapName() );
#else
	const MapDef_t* pMapDef = GetItemSchema()->GetMasterMapDefByName ( STRING( gpGlobals->mapname ) );
#endif
	if ( !pMapDef ) 
		return false;

	FOR_EACH_VEC( pMapDef->m_vecAssociatedGameCategories, i )
	{
		uint32 nMask = 1 << pMapDef->m_vecAssociatedGameCategories[ i ];
		if ( ( nMask & m_nValidGameModesMask ) != 0 )
			return true;
	}

	return false;
}

bool CTFTeamQuestModifier::BPassesModifier( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
{
	if ( pOwner->GetTeamNumber() != m_nTeamNum )
	{
		return false;
	}

	return true;
}

bool CTFConditionQuestModifier::BPassesModifier( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
{
	switch( m_Operation )
	{
		// Must have all of the conditions
		case LogicalOperation::AND:
		{
			FOR_EACH_VEC( m_vecRequiredConditions, i )
			{
				if ( !pOwner->m_Shared.InCond( m_vecRequiredConditions[ i ] ) )
					return false;
			}

			return true;
		}

		// Must have any of the conditions
		case LogicalOperation::OR:
		{
			FOR_EACH_VEC( m_vecRequiredConditions, i )
			{
				if ( pOwner->m_Shared.InCond( m_vecRequiredConditions[ i ] ) )
					return true;
			}

			return false;
		}

		// Can't have any of the conditions
		case LogicalOperation::NOT:
		{
			FOR_EACH_VEC( m_vecRequiredConditions, i )
			{
				if ( pOwner->m_Shared.InCond( m_vecRequiredConditions[ i ] ) )
					return false;
			}

			return true;
		}
	}

	return true;
}

bool CTFEquippedItemsQuestModifier::BPassesModifier( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
{
	auto lambdaIsItemEquipped = [ &pOwner ]( const CSchemaItemDefHandle& def ) -> bool
	{
		auto pNonConstOwner = const_cast< CTFPlayer* >( pOwner );
		for ( int i = LOADOUT_POSITION_PRIMARY ; i < LOADOUT_POSITION_ACTION; ++i )
		{
			CEconItemView *pCurItemData = CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot( pNonConstOwner, i );
			if ( !pCurItemData )
				continue;

			if ( pCurItemData->GetItemDefinition() == def )
			{
				return true;	
			}

			const char *pszRemapClass = pCurItemData->GetItemDefinition()->GetXifierRemapClass();
			if ( pszRemapClass )
			{
				if ( FStrEq( def.GetName(), pszRemapClass ) )
				{
					return true;
				}
			}
		}

		return false;
	};

	switch( m_Operation )
	{
		// Must have all of the items equipped
		case LogicalOperation::AND:
		{
			FOR_EACH_VEC( m_vecRequiredItemDefs, i )
			{
				if ( !lambdaIsItemEquipped( m_vecRequiredItemDefs[ i ] ) )
					return false;
			}

			return true;
		}

		// Must have any of the items equipped
		case LogicalOperation::OR:
		{
			FOR_EACH_VEC( m_vecRequiredItemDefs, i )
			{
				if ( lambdaIsItemEquipped( m_vecRequiredItemDefs[ i ] ) )
					return true;
			}

			return false;
		}

		// Can't have any of the items equipped
		case LogicalOperation::NOT:
		{
			FOR_EACH_VEC( m_vecRequiredItemDefs, i )
			{
				if ( lambdaIsItemEquipped( m_vecRequiredItemDefs[ i ] ) )
					return false;
			}

			return true;
		}
	}

	return true;
}

bool CTFJumpStateQuestModifier::BPassesModifier( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
{
#ifdef CLIENT_DLL
	return true;
#else
	int nNumJumps = const_cast< CTFPlayer* >( pOwner )->GetGroundEntity() == NULL ? 1 : 0;
	nNumJumps += pOwner->m_Shared.GetAirDash();
	nNumJumps += pOwner->m_bScattergunJump;

	// If we want them on the ground, make sure they're on the ground
	if ( m_nJumpCount == 0 )
		return nNumJumps == 0;

	// If we want them jumping, make sure they are at least as jumpy as
	// we want them to be
	return nNumJumps >= m_nJumpCount;
#endif
}