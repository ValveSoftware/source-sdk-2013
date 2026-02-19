//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "game_item_schema.h"
#include "schemainitutils.h"
#include "tf_shareddefs.h"
#include "in_buttons.h"
#include "econ_holidays.h"

	#include "econ_item_system.h"
	#include "engine/IEngineSound.h"

	extern ISoundEmitterSystemBase *soundemitterbase;

#ifdef CLIENT_DLL
	#include "materialsystem/itexturecompositor.h"
#endif

// For a particular set of KeyValues, ensure that all of the one-level-deep subkeys are a subset of the values in testKeys.
// This ensures that there are no typos in the keynames. 
static bool ValidateKeysAreSubset( KeyValues* kv, const CUtlVector<const char *>& testKeys, CUtlVector<CUtlString> *pVecErrors )
{
	int numTestEntries = testKeys.Count();
	Assert(numTestEntries >= 0);
	if (numTestEntries == 0) 
		return true;

	// This currently is inefficient, it's O(len(_keyvalues) * len(_testKeys)). It could easily be made faster for large N, but for small lengths
	// cache dominates. It also has the nice property that it will show up on the profiler if it's a problem.
	for ( KeyValues *pKey = kv->GetFirstSubKey(); pKey; pKey = pKey->GetNextKey() )
	{
		bool matchAny = false;
		const char* testVal = pKey->GetName();

		for ( auto it = testKeys.begin(); it != testKeys.end(); ++it ) {
			if (0 == V_stricmp((*it), testVal)) {
				matchAny = true;
				break;
			}
		}

		if (!matchAny) 
		{
			if (pVecErrors) 
			{
				CUtlString choices(CFmtStr("Unexpected key '%s', expected one of: ", testVal));
				int numTestEntriesLessOne = numTestEntries - 1;
				for (int i = 0; i < numTestEntriesLessOne; ++i) 
				{
					choices.Append(CFmtStr("\"%s\", ", testKeys[i]));
				}

				choices.Append(CFmtStr("\"%s\".", testKeys[numTestEntriesLessOne]));
				pVecErrors->AddToTail(choices);
			}

			return false;
		}
	}

	return true;
}

template < typename ReturnType, typename DefIndexType >
ReturnType* GetDefinitionByDefIndex( const CUtlMap< DefIndexType, ReturnType* >& map, DefIndexType defindex )
{
	auto idx = map.Find( defindex );
	if ( idx == map.InvalidIndex() )
		return NULL;

	return map[ idx ];
}

template < typename ReturnType, typename DefIndexType >
const ReturnType* GetDefinitionByDefIndex( const CUtlMap< DefIndexType, const ReturnType* >& map, DefIndexType defindex )
{
	auto idx = map.Find( defindex );
	if ( idx == map.InvalidIndex() )
		return NULL;

	return map[ idx ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void InitPerClassStringArray( KeyValues *pPerClassData, const char *(&outputArray)[LOADOUT_COUNT] )
{
	if ( pPerClassData )
	{
		const char* pszBaseName = pPerClassData->GetString( "basename", NULL );

		for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; i++ )
		{
			if ( outputArray[i] && *outputArray[i] )
			{
				delete outputArray[i];
				outputArray[i] = NULL;
			}

			char* pszOut = NULL;
	
			CUtlString strClassString( pPerClassData->GetString( GetItemSchema()->GetClassUsabilityStrings()[i], NULL ) );

			// If there's a class specific string defined, use that
			if ( !strClassString.IsEmpty() )
			{
				size_t nLength = strClassString.Length() + 1;
				pszOut = new char[ nLength ];
				V_strncpy( pszOut, strClassString, nLength );
			}
			else if ( pszBaseName )
			{
				// If we have a basename specified, use that to construct our class-specific string
				// ( ex. models/badge_%s.mdl turns into models/badge_scout.mdl, etc. )

				// So this is fun.  ClassUsabilityStrings refers to the "Demoman", but the vast majority of his models are whatever_demo.mdl
				// The RIGHT fix would be to either: 
				//		1) change all the model and content files to whatever_demoman.mdl
				//		2) fixup the schema so every reference to "demoman" is changed to "demo" and update GetClassUsabilityStrings
				//				and fix everything that breaks
				// But we're not doing that right now.  If this class is the TF_CLASS_DEMOMAN, just force "demo"
				CFmtStr fmtStr;
				if ( i == TF_CLASS_DEMOMAN )
				{
					fmtStr.sprintf( pszBaseName, "demo", "demo", "demo" );
				}
				else
				{
					fmtStr.sprintf( pszBaseName, GetItemSchema()->GetClassUsabilityStrings()[i], GetItemSchema()->GetClassUsabilityStrings()[i], GetItemSchema()->GetClassUsabilityStrings()[i] );
				}

				int nLength = fmtStr.Length() + 1;
				pszOut = new char[ nLength ];
				V_strncpy( pszOut, fmtStr, nLength );
			}

			outputArray[i] = pszOut;

			if ( outputArray[0] == NULL )
			{
				outputArray[0] = outputArray[i];
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static bool InitPerClassStringVectorArray( KeyValues *pPerClassData, CUtlVector< const char * > (&outputArray)[LOADOUT_COUNT], CUtlVector<CUtlString>* pVecErrors )
{
	if ( pPerClassData )
	{
		if ( !ValidateKeysAreSubset( pPerClassData, GetItemSchema()->GetClassUsabilityStrings(), pVecErrors ) )
		{
			return false;
		}

		for ( int i = 1; i < LOADOUT_COUNT; i++ )
		{
			KeyValues *pClassKey = pPerClassData->FindKey( GetItemSchema()->GetClassUsabilityStrings()[i] );
			if ( pClassKey )
			{
				// check single line case
				const char *pszValue = pClassKey->GetString();
				if ( pszValue && *pszValue )
				{
					outputArray[i].AddToTail( pszValue );
				}
				
				// check multi line case
				FOR_EACH_SUBKEY( pClassKey, pValueKey )
				{
					pszValue = pValueKey->GetString();
					if ( pszValue && *pszValue )
					{
						outputArray[i].AddToTail( pszValue );
					}
				}
			}
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRandomChanceString::CRandomChanceString()
{
	m_unTotalChance = 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRandomChanceString::AddString( const char *pszString, int nChance )
{
	Assert( nChance > 0 );
	std::pair< const char *, int > toAdd( pszString, nChance );
	m_vecChoices.AddToTail( toAdd );
	m_unTotalChance += nChance;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CRandomChanceString::GetRandomString() const
{
	int nRandomRoll = RandomInt( 1, m_unTotalChance );
	int nStartWindow = 0;
	FOR_EACH_VEC( m_vecChoices, i )
	{
		int nEndWindow = nStartWindow + m_vecChoices[i].second;
		if ( nRandomRoll > nStartWindow && nRandomRoll <= nEndWindow )
		{
			return m_vecChoices[i].first;
		}
		nStartWindow = nEndWindow;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static bool ParseRandomChanceStringFromKV( KeyValues *pClassKey, CRandomChanceString *pOut )
{
	Assert( pClassKey );
	Assert( pOut );

	// check single line case
	const char *pszName = pClassKey->GetString();
	if ( pszName && *pszName )
	{
		// there's only one choice
		pOut->AddString( pszName, 1 );
	}
	else
	{
		// check multi line case
		FOR_EACH_SUBKEY( pClassKey, pValueKey )
		{
			const char *pszChoice = pValueKey->GetName();
			int nChance = pValueKey->GetInt();
			if ( pszChoice && *pszChoice && nChance > 0 )
			{
				pOut->AddString( pszChoice, nChance );
			}
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool InitPerClassRandomChanceStringArray( KeyValues *pPerClassData, CRandomChanceString (&outputArray)[LOADOUT_COUNT], CUtlVector<CUtlString>* pVecErrors )
{
	if ( pPerClassData )
	{
		if ( !ValidateKeysAreSubset( pPerClassData, GetItemSchema()->GetClassUsabilityStrings(), pVecErrors ) )
		{
			return false;
		}

		for ( int i = 0; i < LOADOUT_COUNT; i++ )
		{
			KeyValues *pClassKey = pPerClassData->FindKey( GetItemSchema()->GetClassUsabilityStrings()[i] );
			if ( pClassKey )
			{
				ParseRandomChanceStringFromKV( pClassKey, &outputArray[i] );
			}
		}
	}

	return true;
}


CTFTauntInfo::CTFTauntInfo()
{
	for ( int i=0; i<LOADOUT_COUNT; ++i )
	{
		m_pszProp[i] = NULL;
		m_pszPropIntroScene[i] = NULL;
		m_pszPropOutroScene[i] = NULL;
	}

	m_pszParticleAttachment = NULL;

	m_flTauntSeparationForwardDistance = 0;
	m_flTauntSeparationRightDistance = 0;
	m_flMinTauntTime = 2.f;
	m_bIsPartnerTaunt = false;
	m_bStopTauntIfMoved = false;

	m_nFOV = 0;
	m_flCameraDist = 0;
	m_flCameraDistUp = -15;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFTauntInfo::InitTauntInputRemap( KeyValues *pKV, CUtlVector<TauntInputRemap_t>( &outputArray ), CUtlVector<CUtlString> *pVecErrors )
{
	static const char *s_pszAllowedTauntInputButtonNames[] =
	{
		"IN_ATTACK",
		"IN_ATTACK2",
		"IN_FORWARD",
		"IN_BACK"
	};
	static int s_iAllowedTauntInputButtons[] =
	{
		IN_ATTACK,
		IN_ATTACK2,
		IN_FORWARD,
		IN_BACK
	};
	COMPILE_TIME_ASSERT( ARRAYSIZE( s_pszAllowedTauntInputButtonNames ) == ARRAYSIZE( s_iAllowedTauntInputButtons ) );

	FOR_EACH_SUBKEY( pKV, pButtonKey )
	{
		const char *pszButtonName = pButtonKey->GetName();
		int iButton = 0;
		for ( int i=0; i<ARRAYSIZE( s_pszAllowedTauntInputButtonNames ); i++ )
		{
			if ( !V_strcmp( pszButtonName, s_pszAllowedTauntInputButtonNames[i] ) )
			{
				iButton = s_iAllowedTauntInputButtons[i];
				break;
			}
		}

		if ( iButton == 0 )
		{
			AssertMsg( 0, "Taunt input button [%s] is not valid.\n", pszButtonName );
			return false;
		}

		KeyValues *pPressedKey = pButtonKey->FindKey( "pressed" );
		KeyValues *pReleasedKey = pButtonKey->FindKey( "released" );
		if ( pPressedKey || pReleasedKey )
		{
			int iNew = outputArray.AddToTail();
			outputArray[iNew].m_iButton = iButton;

			if ( !InitPerClassStringVectorArray( pPressedKey, outputArray[iNew].m_vecButtonPressedScenes, pVecErrors ) )
				return false;

			if ( !InitPerClassStringVectorArray( pReleasedKey, outputArray[iNew].m_vecButtonReleasedScenes, pVecErrors ) )
				return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFTauntInfo::BInitFromKV( KeyValues *pKV, CUtlVector<CUtlString> *pVecErrors )
{
	FOR_EACH_SUBKEY( pKV, pSubKey )
	{
		const char *pszKeyName = pSubKey->GetName();
		if ( !V_strcmp( pszKeyName, "custom_taunt_scene_per_class" ) )
		{
			if ( !InitPerClassStringVectorArray( pSubKey, m_vecIntroScenes, pVecErrors ) )
				return false;
		}
		else if ( !V_strcmp( pszKeyName, "custom_taunt_outro_scene_per_class" ) )
		{
			if ( !InitPerClassStringVectorArray( pSubKey, m_vecOutroScenes, pVecErrors ) ) 
				return false;
		}
		else if ( !V_strcmp( pszKeyName, "custom_partner_taunt_per_class" ) )
		{
			if ( !InitPerClassStringVectorArray( pSubKey, m_vecPartnerTauntInitiatorScenes, pVecErrors ) ) 
				return false;
			if ( !InitPerClassStringVectorArray( pSubKey, m_vecPartnerTauntReceiverScenes, pVecErrors ) )
				return false;
		}
		else if ( !V_strcmp( pszKeyName, "custom_partner_taunt_initiator_per_class" ) )
		{
			if ( !InitPerClassStringVectorArray( pSubKey, m_vecPartnerTauntInitiatorScenes, pVecErrors ) )
				return false;
		}
		else if ( !V_strcmp( pszKeyName, "custom_partner_taunt_receiver_per_class" ) )
		{
			if ( !InitPerClassStringVectorArray( pSubKey, m_vecPartnerTauntReceiverScenes, pVecErrors ) )
				return false;
		}
		else if ( !V_strcmp( pszKeyName, "custom_taunt_input_remap" ) )
		{
			if ( !InitTauntInputRemap( pSubKey, m_vecTauntInputRemap, pVecErrors ) )
			{
				return false;
			}
		}
		else if ( !V_strcmp( pszKeyName, "custom_taunt_prop_input_remap" ) )
		{
			if ( !InitTauntInputRemap( pSubKey, m_vecTauntPropInputRemap, pVecErrors ) )
			{
				return false;
			}
		}
		else if ( !V_strcmp( pszKeyName, "custom_taunt_prop_per_class" ) )
		{
			InitPerClassStringArray( pSubKey, m_pszProp );
		}
		else if ( !V_strcmp( pszKeyName, "custom_taunt_prop_scene_per_class" ) )
		{
			InitPerClassStringArray( pSubKey, m_pszPropIntroScene );
		}
		else if ( !V_strcmp( pszKeyName, "custom_taunt_prop_outro_scene_per_class" ) )
		{
			InitPerClassStringArray( pSubKey, m_pszPropOutroScene );
		}
		else if ( !V_strcmp( pszKeyName, "taunt_separation_forward_distance" ) )
		{
			m_flTauntSeparationForwardDistance = pSubKey->GetFloat();
		}
		else if ( !V_strcmp( pszKeyName, "taunt_separation_right_distance" ) )
		{
			m_flTauntSeparationRightDistance = pSubKey->GetFloat();
		}
		else if ( !V_strcmp( pszKeyName, "min_taunt_time" ) )
		{
			m_flMinTauntTime = pSubKey->GetFloat();
		}
		else if ( !V_strcmp( pszKeyName, "is_partner_taunt" ) )
		{
			m_bIsPartnerTaunt = pSubKey->GetBool();
		}
		else if ( !V_strcmp( pszKeyName, "stop_taunt_if_moved" ) )
		{
			m_bStopTauntIfMoved = pSubKey->GetBool();
		}
		else if ( !V_strcmp( pszKeyName, "fov" ) )
		{
			m_nFOV = pSubKey->GetInt();
		}
		else if ( !V_strcmp( pszKeyName, "camera_dist" ) )
		{
			m_flCameraDist = pSubKey->GetFloat();
		}
		else if ( !V_strcmp( pszKeyName, "camera_dist_up" ) )
		{
			m_flCameraDistUp = pSubKey->GetFloat();
		}
		else if ( !V_strcmp( pszKeyName, "particle_attachment" ) )
		{
			m_pszParticleAttachment = pSubKey->GetString();
		}
		else
		{
			AssertMsg( 0, "'%s' key is invalid", pszKeyName );
			return false;
		}
	}

	return true;
}




//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFItemDefinition::InternalInitialize()
{
	m_eEquipType = EQUIP_TYPE_INVALID;
	m_iDefaultLoadoutSlot = LOADOUT_POSITION_INVALID;
	m_iAnimationSlot = -1;

	m_vbClassUsability.ClearAll();
	for ( int i = 0; i < ARRAYSIZE( m_iLoadoutSlots ); i++ )
	{
		m_iLoadoutSlots[i] = LOADOUT_POSITION_INVALID;
		m_pszPlayerDisplayModel[i] = NULL;
		m_pszPlayerDisplayModelAlt[i] = NULL;
	}

	m_pTauntData = NULL;

	m_pszAdText = NULL;
	m_pszAdResFile = NULL;

#ifdef CLIENT_DLL
	m_bHasDetailedIcon = false;
	m_bCanBackpackInspect = true;
#endif // CLIENT_DLL
}
#include "filesystem.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFItemDefinition::BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors )
{
	CEconItemDefinition::BInitFromKV( pKVItem, pVecErrors );

	// Our superclass should initialize our raw definition, including any prefab work.
	KeyValues *pKVInitValues = GetRawDefinition();
	Assert( pKVInitValues );

	// Reset default properties.
	InternalInitialize();

	CUtlDict< EEquipType_t > dictEquipType;
	dictEquipType.Insert( "account", EQUIP_TYPE_ACCOUNT );
	dictEquipType.Insert( "class", EQUIP_TYPE_CLASS );
	// Default to class equip type
	const char *pszEquipType = pKVInitValues->GetString( "equip_type", "class" );
	auto idx = dictEquipType.Find( pszEquipType );
	if ( idx != dictEquipType.InvalidIndex() )
	{
		m_eEquipType = dictEquipType[ idx ];
	}
	SCHEMA_INIT_CHECK( m_eEquipType != EQUIP_TYPE_INVALID,
		"Item definition %i \"%s\" used uknown equip type: %s!", GetDefinitionIndex(), GetItemBaseName(), pszEquipType );

	// Get the default loadout slot
	const char *pszLoadoutSlot = pKVInitValues->GetString("item_slot", "");
	if ( *pszLoadoutSlot )
	{
		if ( !V_strcmp( pszLoadoutSlot, "head" ) )
		{
			pszLoadoutSlot = "misc";
		}

		m_iDefaultLoadoutSlot = StringFieldToInt( pszLoadoutSlot, GetItemSchema()->GetLoadoutStrings( m_eEquipType ), true );
		SCHEMA_INIT_CHECK(
			m_iDefaultLoadoutSlot >= 0,
			"Item definition %i \"%s\" used unknown loadout slot: %s!", GetDefinitionIndex(), GetItemBaseName(), pszLoadoutSlot );
	}
	
	// Class usability--use our copy of kv item
	KeyValues *pClasses = pKVInitValues->FindKey( "used_by_classes" );
	if ( pClasses )
	{
		KeyValues *pKVClass = pClasses->GetFirstSubKey();
		while ( pKVClass )
		{
			int iClass = StringFieldToInt( pKVClass->GetName(), GetItemSchema()->GetClassUsabilityStrings() );
			if ( iClass > -1 )
			{
				m_vbClassUsability.Set(iClass);
				m_iLoadoutSlots[iClass] = m_iDefaultLoadoutSlot;

				// If the value is "1", the class uses this item in the default loadout slot.
				const char *pszValue = pKVClass->GetString();
				if ( pszValue[0] != '1' )
				{
					int iSlot = StringFieldToInt( pszValue, GetItemSchema()->GetLoadoutStrings( EQUIP_TYPE_CLASS ) );
					Assert( iSlot != -1 );
					if ( iSlot != -1 )
					{
						m_iLoadoutSlots[iClass] = iSlot;
					}
				}
			}

			pKVClass = pKVClass->GetNextKey();
		}

		// add "all_class" if applicable
		if ( CanBeUsedByAllClasses() )
		{
			KeyValues *pKVAllClassKey = new KeyValues( "all_class", "all_class", "1" );
			pClasses->AddSubKey( pKVAllClassKey );
		}
	}

	// Verify that no items are set up to be equipped in a wearable slot for some classes and a
	// non-wearable slot other times. "Is this in a wearable slot?" is used to determine whether
	// or not content can be allowed to stream, so we don't allow an item to overlap.
	bool bHasAnyWearableSlots = false,
		 bHasAnyNonwearableSlots = false;

	for ( int i = 0; i < LOADOUT_COUNT; i++ )
	{
		if ( m_iLoadoutSlots[i] != LOADOUT_POSITION_INVALID )
		{
			const bool bThisIsWearableSlot = IsWearableSlot( m_iLoadoutSlots[i] );

			(bThisIsWearableSlot ? bHasAnyWearableSlots : bHasAnyNonwearableSlots) = true;
		}
	}

	SCHEMA_INIT_CHECK(
		!(bHasAnyWearableSlots && bHasAnyNonwearableSlots),
		"Item definition %i \"%s\" used in both wearable and not wearable slots!", GetDefinitionIndex(), GetItemBaseName() );

	// "anim_slot"
	const char *pszAnimSlot = pKVInitValues->GetString("anim_slot");
	if ( pszAnimSlot && pszAnimSlot[0] )
	{
		if ( Q_stricmp(pszAnimSlot, "FORCE_NOT_USED") == 0 )
		{
			m_iAnimationSlot = -2;
		}
		else
		{
			m_iAnimationSlot = StringFieldToInt( pszAnimSlot, GetItemSchema()->GetWeaponTypeSubstrings() );
		}
	}

	InitPerClassStringArray( pKVInitValues->FindKey( "model_player_per_class" ), m_pszPlayerDisplayModel );
	InitPerClassStringArray( pKVInitValues->FindKey( "model_player_per_class_alt" ), m_pszPlayerDisplayModelAlt );

#if defined DEBUG && defined CLIENT_DLL
	for ( int i = 0; i < m_vbClassUsability.GetNumBits(); i++ )
	{
		if ( m_vbClassUsability[i] && m_pszPlayerDisplayModel[ i ] )
		{
		//	SCHEMA_INIT_CHECK( g_pFullFileSystem->FileExists( m_pszPlayerDisplayModel[ i ] ), "Missing model %s specified in model_player_per_class in item %s", m_pszPlayerDisplayModel[ i ], GetItemBaseName() );
		}
	}
#endif

	KeyValues *pTauntKV = pKVInitValues->FindKey( "taunt" );
	if ( pTauntKV )
	{
		Assert( !m_pTauntData );
		m_pTauntData = new CTFTauntInfo();
		SCHEMA_INIT_CHECK(
			m_pTauntData->BInitFromKV( pTauntKV, pVecErrors ),
			"Item definition %i \"%s\" failed to initialize taunt data!", GetDefinitionIndex(), GetItemBaseName() 
		);
	}

	if ( GetDefinitionIndex() == 7509 && !pTauntKV )
	{
		KeyValuesDumpAsDevMsg( pKVItem );
	}

	// Stomp duplicate properties.
	if ( !m_pszPlayerDisplayModel[0] )
	{
		m_pszPlayerDisplayModel[0] = GetBasePlayerDisplayModel();
	}

	// Auto-generated tags based on slot/class.
	m_vecTags.AddToTail( GetItemSchema()->GetHandleForTag( CFmtStr( "auto__slot_%s", pszLoadoutSlot ).Get() ) );

	for ( int i = 0; i < m_vbClassUsability.GetNumBits(); i++ )
	{
		if ( m_vbClassUsability[i] )
		{
			m_vecTags.AddToTail( GetItemSchema()->GetHandleForTag( CFmtStr( "auto__class_%s", GetItemSchema()->GetClassUsabilityStrings()[i] ).Get() ) );
		}
	}

	m_pszAdText = pKVInitValues->GetString( "ad_text", NULL );
	m_pszAdResFile = pKVInitValues->GetString( "ad_res_file", "Resource/UI/econ/ItemAdDefault.res" );

#ifdef CLIENT_DLL
	m_bHasDetailedIcon = pKVInitValues->GetBool( "has_detailed_icon" );
	m_bCanBackpackInspect = !pKVInitValues->GetBool( "disable_backpack_inspect" );
#endif // CLIENT_DLL

	return SCHEMA_INIT_SUCCESS();
}

#if defined(CLIENT_DLL) || defined(GAME_DLL)
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFItemDefinition::BInitFromTestItemKVs( int iNewDefIndex, KeyValues *pKVItem, CUtlVector<CUtlString>* pVecErrors )
{
	if ( !CEconItemDefinition::BInitFromTestItemKVs( iNewDefIndex, pKVItem, pVecErrors ) )
		return false;

	// Use the tester's class usage choices, even when testing existing items
	m_vbClassUsability.ClearAll();
	int iClassUsage = pKVItem->GetInt( "class_usage", 0 );
	for ( int i = 0; i < LOADOUT_COUNT; i++ )
	{
		if ( iClassUsage & (1 << i) || (iClassUsage & 1) )
		{
			m_vbClassUsability.Set(i);
			m_iLoadoutSlots[i] = m_iDefaultLoadoutSlot;
		}
	}

	// Initialize player display model.
	for ( int i = 0; i < LOADOUT_COUNT; i++ )
	{
		m_pszPlayerDisplayModel[i] = NULL;
		m_pszPlayerDisplayModelAlt[i] = NULL;
	}

	InitPerClassStringArray( pKVItem->FindKey( "model_player_per_class" ), m_pszPlayerDisplayModel );
	InitPerClassStringArray( pKVItem->FindKey( "model_player_per_class_alt" ), m_pszPlayerDisplayModelAlt );

	KeyValues *pTauntKV = pKVItem->FindKey( "taunt" );
	if ( pTauntKV )
	{
		Assert( !m_pTauntData );
		m_pTauntData = new CTFTauntInfo();
		if ( !m_pTauntData->BInitFromKV( pTauntKV, pVecErrors ) )
			return false;
	}

	// Stomp duplicate properties.
	if ( !m_pszPlayerDisplayModel[0] )
	{
		m_pszPlayerDisplayModel[0] = GetBasePlayerDisplayModel();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFItemDefinition::CopyPolymorphic( const CEconItemDefinition *pSourceDef )
{
	Assert( dynamic_cast<const CTFItemDefinition *>( pSourceDef ) != NULL );

	*this = *(const CTFItemDefinition *)pSourceDef;
}
#endif // defined(CLIENT_DLL) || defined(GAME_DLL)

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStyleInfo::BInitFromKV( KeyValues *pKVStyle, CUtlVector<CUtlString> *pVecErrors )
{
	Assert( pKVStyle );

	m_iSkins[ TF_TEAM_RED ] = pKVStyle->GetInt( "skin_red", 0 );
	m_iSkins[ TF_TEAM_BLUE ] = pKVStyle->GetInt( "skin_blu", 0 );

	m_iViewmodelSkins[ TF_TEAM_RED ] = pKVStyle->GetInt( "v_skin_red", -1 );
	m_iViewmodelSkins[ TF_TEAM_BLUE ] = pKVStyle->GetInt( "v_skin_blu", -1 );

	const char *pszPlayerModel = pKVStyle->GetString( "model_player", NULL );
	if ( pszPlayerModel )
	{
		for ( int i = 0; i < LOADOUT_COUNT; i++ )
		{
			m_pszPlayerDisplayModel[0][i] = pszPlayerModel;
		}
	}
	else
	{	
		InitPerClassStringArray( pKVStyle->FindKey( "model_player_per_class" ), m_pszPlayerDisplayModel[0] );
		InitPerClassStringArray( pKVStyle->FindKey( "model_player_per_class_red" ), m_pszPlayerDisplayModel[0] );
		InitPerClassStringArray( pKVStyle->FindKey( "model_player_per_class_blue" ), m_pszPlayerDisplayModel[1] );
	}

	CEconStyleInfo::BInitFromKV( pKVStyle, pVecErrors );
}

#if defined(CLIENT_DLL) || defined(GAME_DLL)
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemDefinition::GeneratePrecacheModelStrings( bool bDynamicLoad, CUtlVector<const char *> *out_pVecModelStrings ) const
{
	Assert( out_pVecModelStrings );

	// Is this definition supposed to use dynamic-loaded content or precache it?
	if ( !bDynamicLoad || !IsContentStreamable() )
	{
		// Parent class base meshes, if relevant.
		CEconItemDefinition::GeneratePrecacheModelStrings( bDynamicLoad, out_pVecModelStrings );

		for ( int i = 0; i < LOADOUT_COUNT; i++ )
		{
			// Per-class models.
			const char *pszModel = GetPlayerDisplayModel(i);
			if ( pszModel && pszModel[0] )
			{
				out_pVecModelStrings->AddToTail( pszModel );
			}
			
			// Per-class alt-models
			const char *pszModelAlt = GetPlayerDisplayModelAlt(i);
			if ( pszModelAlt && pszModelAlt[0] )
			{
				out_pVecModelStrings->AddToTail( pszModelAlt );
			}

			// Per-class custom taunt prop
			if ( GetTauntData() )
			{
				const char *pszCustomTauntProp = GetTauntData()->GetProp(i);
				if ( pszCustomTauntProp && pszCustomTauntProp[0] )
				{
					out_pVecModelStrings->AddToTail( pszCustomTauntProp );
				}
			}
		}

		const char *pszModel = GetWorldDisplayModel();
		if ( pszModel && pszModel[0] )
		{
			out_pVecModelStrings->AddToTail( pszModel );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFStyleInfo::GeneratePrecacheModelStringsForStyle( CUtlVector<const char *> *out_pVecModelStrings ) const
{
	Assert( out_pVecModelStrings );

	for ( int i = 0; i < ARRAYSIZE( m_pszPlayerDisplayModel ); i++ )
	{
		for ( int j = 0; j < ARRAYSIZE( m_pszPlayerDisplayModel[i] ); j++ )
		{
			const char* pszModelName = m_pszPlayerDisplayModel[i][j];
			if ( pszModelName && *pszModelName )
			{
				out_pVecModelStrings->AddToTail( pszModelName );
			}
		}
	}

	CEconStyleInfo::GeneratePrecacheModelStringsForStyle( out_pVecModelStrings );
}
#endif // defined(CLIENT_DLL) || defined(GAME_DLL)

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CTFStyleInfo::GetPlayerDisplayModel( int iClass, int iTeam ) const
{
	Assert( iClass >= 0 );
	Assert( iClass < ARRAYSIZE( m_pszPlayerDisplayModel[0] ) );

	const char *pszBlueModel = m_pszPlayerDisplayModel[1][iClass];
	if ( iTeam == TF_TEAM_BLUE && pszBlueModel && *pszBlueModel )
	{
		return pszBlueModel;
	}

	// always return red team as default
	return m_pszPlayerDisplayModel[0][iClass];
}

//-----------------------------------------------------------------------------
// Purpose: Return the load-out slot that this item must be placed into
//-----------------------------------------------------------------------------
int CTFItemDefinition::GetLoadoutSlot( int iLoadoutClass ) const
{
	if ( iLoadoutClass == GEconItemSchema().GetAccountIndex() )
	{
		return GetDefaultLoadoutSlot();
	}

	if ( iLoadoutClass <= 0 || iLoadoutClass >= LOADOUT_COUNT )
		return m_iDefaultLoadoutSlot;

	return m_iLoadoutSlots[iLoadoutClass];
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this item is in a wearable slot, or is acting as a wearable
//-----------------------------------------------------------------------------
bool CTFItemDefinition::IsAWearable() const
{
	if ( IsWearableSlot( GetDefaultLoadoutSlot() ) && !IsActingAsAWeapon() )
		return true;

	if ( IsActingAsAWearable() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the content for this item view should be streamed. If false,
//			it should be preloaded.
//-----------------------------------------------------------------------------
ConVar item_enable_content_streaming( "item_enable_content_streaming", "1", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED );

bool CTFItemDefinition::IsContentStreamable() const
{
#if defined( WITH_STREAMABLE_WEAPONS )
    extern ConVar tf_loadondemand_default;

    // If we support streamable weapons and loadondemand_default is true, then we do not want to restrict demand loading
    // to wearables only, so skip that check.
    if (!tf_loadondemand_default.GetBool())
#endif
    {
        if (!IsAWearable())
            return false;
    }
	return item_enable_content_streaming.GetBool()
		&& CEconItemDefinition::IsContentStreamable();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFItemDefinition::FilloutSlotUsage( CBitVec<LOADOUT_COUNT> *pBV ) const
{
	pBV->ClearAll();

	for ( int i = 0; i < LOADOUT_COUNT; i++ )
	{
		if ( m_iLoadoutSlots[i] == LOADOUT_POSITION_INVALID )
			continue;

		pBV->Set( m_iLoadoutSlots[i] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFItemDefinition::CanBeUsedByAllClasses( void ) const
{
	// Right now, Civilian isn't a real class, so we only have 9 classes in this check
	for ( int iClass = 1; iClass < (LOADOUT_COUNT-1); iClass++ )
	{
		if ( !CanBeUsedByClass(iClass) )
			return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFItemDefinition::CanBePlacedInSlot( int nSlot ) const
{
	for ( int i = 0; i < LOADOUT_COUNT; i++ )
	{
		if ( m_iLoadoutSlots[i] == nSlot )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
// Used to convert strings to ints for class usability
struct PlayerClassInfo_t
{
	const char	*m_pchName;
	const char	*m_pchLocalizationKey;
};

static PlayerClassInfo_t gs_PlayerClassData[] =
{
	{ "Undefined",	"#TF_Class_Name_Undefined" },
	{ "Scout",		"#TF_Class_Name_Scout" },
	{ "Sniper",		"#TF_Class_Name_Sniper" },
	{ "Soldier",	"#TF_Class_Name_Soldier" },
	{ "Demoman",	"#TF_Class_Name_Demoman" },
	{ "Medic",		"#TF_Class_Name_Medic" },
	{ "Heavy",		"#TF_Class_Name_HWGuy" },
	{ "Pyro",		"#TF_Class_Name_Pyro" },
	{ "Spy",		"#TF_Class_Name_Spy" },
	{ "Engineer",	"#TF_Class_Name_Engineer" },
	{ "Invalid",	"" }						// lots of code loops over these classes based on LOADOUT_COUNT, which is wrong, but this allows them to do it safely
};

bool BIsPlayerClassValid( int iClass )
{
	return iClass >= 0 && iClass < ARRAYSIZE( gs_PlayerClassData );
}

const char *GetPlayerClassName( int iClass )
{
	if ( !BIsPlayerClassValid( iClass ) )
		return NULL;

	return gs_PlayerClassData[ iClass ].m_pchName;
}

const char *GetPlayerClassLocalizationKey( int iClass )
{
	if ( !BIsPlayerClassValid( iClass ) )
		return NULL;

	return gs_PlayerClassData[ iClass ].m_pchLocalizationKey;
}

itemid_t GetAssociatedQuestID( const IEconItemInterface *pEconItem )
{
	static CSchemaAttributeDefHandle pLoanerIDLowAttrib( "quest loaner id low" );
	static CSchemaAttributeDefHandle pLoanerIDHiAttrib( "quest loaner id hi" );
	if ( !pLoanerIDLowAttrib || !pLoanerIDHiAttrib )
		return INVALID_ITEM_ID;

	itemid_t questItemID = INVALID_ITEM_ID;
	uint32 nLow, nHi;
	if ( pEconItem->FindAttribute( pLoanerIDLowAttrib, &nLow ) && pEconItem->FindAttribute( pLoanerIDHiAttrib, &nHi ) )
	{
		// Reconstruct the itemID
		itemid_t nIDLow = 0x00000000FFFFFFFF & (itemid_t)nLow;
		itemid_t nIDHi =  0xFFFFFFFF00000000 & (itemid_t)nHi << 32;
		questItemID = nIDLow | nIDHi;
	}

	return questItemID;
}

// Loadout positions
const char *g_szLoadoutStrings[] = 
{
	// Weapons & Equipment
	"primary",		// LOADOUT_POSITION_PRIMARY = 0,
	"secondary",	// LOADOUT_POSITION_SECONDARY,
	"melee",		// LOADOUT_POSITION_MELEE,
	"utility",		// LOADOUT_POSITION_UTILITY,
	"building",		// LOADOUT_POSITION_BUILDING,
	"pda",			// LOADOUT_POSITION_PDA,
	"pda2",			// LOADOUT_POSITION_PDA2,

	// Wearables
	"head",			// LOADOUT_POSITION_HEAD,
	"misc",			// LOADOUT_POSITION_MISC,
	"action",		// LOADOUT_POSITION_ACTION,
	"",				// LOADOUT_POSITION_MISC2

	"taunt",		// LOADOUT_POSITION_TAUNT
	"",				// LOADOUT_POSITION_TAUNT2
	"",				// LOADOUT_POSITION_TAUNT3
	"",				// LOADOUT_POSITION_TAUNT4
	"",				// LOADOUT_POSITION_TAUNT5
	"",				// LOADOUT_POSITION_TAUNT6
	"",				// LOADOUT_POSITION_TAUNT7
	"",				// LOADOUT_POSITION_TAUNT8

};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_szLoadoutStrings ) <= CLASS_LOADOUT_POSITION_COUNT );	// we don't support mapping directly to slots like "misc2", "taunt2-8", etc.

// Loadout positions used to display loadout slots to players (localized)
const char *g_szLoadoutStringsForDisplay[] =
{
	// Weapons & Equipment
	"#LoadoutSlot_Primary",		// LOADOUT_POSITION_PRIMARY = 0,
	"#LoadoutSlot_Secondary",	// LOADOUT_POSITION_SECONDARY,
	"#LoadoutSlot_Melee",		// LOADOUT_POSITION_MELEE,
	"#LoadoutSlot_Utility",		// LOADOUT_POSITION_UTILITY,
	"#LoadoutSlot_Building",	// LOADOUT_POSITION_BUILDING,
	"#LoadoutSlot_pda",			// LOADOUT_POSITION_PDA,
	"#LoadoutSlot_pda2",		// LOADOUT_POSITION_PDA2

	// Wearables
	"#LoadoutSlot_Misc",		// LOADOUT_POSITION_HEAD
	"#LoadoutSlot_Misc",		// LOADOUT_POSITION_MISC
	"#LoadoutSlot_Action",		// LOADOUT_POSITION_ACTION
	"#LoadoutSlot_Misc",		// LOADOUT_POSITION_MISC2
	
	"#LoadoutSlot_Taunt",		// LOADOUT_POSITION_TAUNT,
	"#LoadoutSlot_Taunt2",		// LOADOUT_POSITION_TAUNT2,
	"#LoadoutSlot_Taunt3",		// LOADOUT_POSITION_TAUNT3,
	"#LoadoutSlot_Taunt4",		// LOADOUT_POSITION_TAUNT4,
	"#LoadoutSlot_Taunt5",		// LOADOUT_POSITION_TAUNT5,
	"#LoadoutSlot_Taunt6",		// LOADOUT_POSITION_TAUNT6,
	"#LoadoutSlot_Taunt7",		// LOADOUT_POSITION_TAUNT7,
	"#LoadoutSlot_Taunt8",		// LOADOUT_POSITION_TAUNT8,

};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_szLoadoutStringsForDisplay ) == CLASS_LOADOUT_POSITION_COUNT );

// Loadout positions
const char *g_szAccountLoadoutStrings[] = 
{
	"quest",
	""
	""
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_szAccountLoadoutStrings ) <= ACCOUNT_LOADOUT_POSITION_COUNT );	// we don't support mapping directly to slots like "misc2", "taunt2-8", etc.

// Loadout positions used to display loadout slots to players (localized)
const char *g_szAccountLoadoutStringsForDisplay[] =
{
	"#LoadoutSlot_Account1",
	"#LoadoutSlot_Account2",
	"#LoadoutSlot_Account3",
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_szAccountLoadoutStringsForDisplay ) == ACCOUNT_LOADOUT_POSITION_COUNT );

// Weapon types
const char *g_szWeaponTypeSubstrings[] =
{
	// Weapons & Equipment
	"PRIMARY",
	"SECONDARY",
	"MELEE",
	"GRENADE",
	"BUILDING",
	"PDA",
	"ITEM1",
	"ITEM2",
	"HEAD",
	"MISC",
	"MELEE_ALLCLASS",
	"SECONDARY2",
	"PRIMARY2",
	"ITEM3",
	"ITEM4"
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_szWeaponTypeSubstrings ) == TF_WPN_TYPE_COUNT );

CTFItemSchema::CTFItemSchema()
{
	// Runs at global constructor time, please don't put anything in here, especially asserts
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFItemSchema::Reset()
{
	m_vecMvMMaps.Purge();
	m_vecMvMMissions.Purge();
	m_vecMvMTours.Purge();

	CEconItemSchema::Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFItemSchema::InitializeStringTable( const char **ppStringTable, unsigned int unStringCount, CUtlVector<const char *> *out_pvecStringTable )
{
	Assert( ppStringTable != NULL );
	Assert( out_pvecStringTable != NULL );
	Assert( out_pvecStringTable->Size() == 0 );

	for ( unsigned int i = 0; i < unStringCount; i++ )
	{
		Assert( ppStringTable[i] != NULL );
		out_pvecStringTable->AddToTail( ppStringTable[i] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Parses game specific items_master data.
//-----------------------------------------------------------------------------
bool CTFItemSchema::BInitSchema( KeyValues *pKVRawDefinition, CUtlVector<CUtlString> *pVecErrors )
{
	// First time through, prepare string tables. Must happen before calling parent BInitSchema.
	if ( m_vecClassUsabilityStrings.Size() == 0 )
	{
		// Special case, since player class data is an array of structs, not an array of strings
		for ( unsigned int i = 0; i < ARRAYSIZE( gs_PlayerClassData ); i++ )
		{
			m_vecClassUsabilityStrings.AddToTail( gs_PlayerClassData[i].m_pchName );
		}
		Assert( m_vecClassUsabilityStrings.Size() == LOADOUT_COUNT );
	
		InitializeStringTable( &g_szLoadoutStrings[0],				ARRAYSIZE(g_szLoadoutStrings),				&m_vecClassLoadoutStrings );
		Assert( m_vecClassLoadoutStrings.Size() <= CLASS_LOADOUT_POSITION_COUNT );

		InitializeStringTable( &g_szLoadoutStringsForDisplay[0],	ARRAYSIZE(g_szLoadoutStringsForDisplay),	&m_vecClassLoadoutStringsForDisplay );
		Assert( m_vecClassLoadoutStringsForDisplay.Size() == CLASS_LOADOUT_POSITION_COUNT );

		InitializeStringTable( &g_szAccountLoadoutStrings[0],		ARRAYSIZE(g_szAccountLoadoutStrings),				&m_vecAccountLoadoutStrings );
		Assert( m_vecAccountLoadoutStrings.Size() <= ACCOUNT_LOADOUT_POSITION_COUNT );

		InitializeStringTable( &g_szAccountLoadoutStringsForDisplay[0],	ARRAYSIZE(g_szAccountLoadoutStringsForDisplay),	&m_vecAccountLoadoutStringsForDisplay );
		Assert( m_vecAccountLoadoutStringsForDisplay.Size() == ACCOUNT_LOADOUT_POSITION_COUNT );

		InitializeStringTable( &g_szWeaponTypeSubstrings[0],		ARRAYSIZE(g_szWeaponTypeSubstrings),		&m_vecWeaponTypeSubstrings );
		Assert( m_vecWeaponTypeSubstrings.Size() == TF_WPN_TYPE_COUNT );
	}
	
	SCHEMA_INIT_SUBSTEP( CEconItemSchema::BInitSchema( pKVRawDefinition, pVecErrors ) );

	KeyValues *pKVMvmMaps = pKVRawDefinition->FindKey( "mvm_maps" );
	SCHEMA_INIT_SUBSTEP( BInitMvmMissions( pKVMvmMaps, pVecErrors ) );

	KeyValues *pKVMvmTours = pKVRawDefinition->FindKey( "mvm_tours" );
	SCHEMA_INIT_SUBSTEP( BInitMvmTours( pKVMvmTours, pVecErrors ) );

	KeyValues *pKVMasterMaps = pKVRawDefinition->FindKey( "master_maps_list" );
	SCHEMA_INIT_SUBSTEP( BInitMaps( pKVMasterMaps, pVecErrors ) );

#ifdef GAME_DLL
	IGameEvent * event = gameeventmanager->CreateEvent( "schema_updated" );
	if ( event )
	{
		gameeventmanager->FireEvent( event, true );
	}
#elif defined( CLIENT_DLL )
	IGameEvent * event = gameeventmanager->CreateEvent( "schema_updated" );
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}
#endif

	return SCHEMA_INIT_SUCCESS();
}

bool CTFItemSchema::BPostSchemaInit( CUtlVector<CUtlString> *pVecErrors )
{
	bool bAllSuccessful = true;
	bAllSuccessful &= CEconItemSchema::BPostSchemaInit( pVecErrors );

	return bAllSuccessful;
}


const char CTFItemSchema::k_rchOverrideItemLevelDescStringAttribName[] = "override item level desc string";

const char CTFItemSchema::k_rchMvMTicketItemDefName[] = "Tour of Duty Ticket";
const char CTFItemSchema::k_rchMvMSquadSurplusVoucherItemDefName[] = "MvM Squad Surplus Voucher";
const char CTFItemSchema::k_rchMvMPowerupBottleItemDefName[] = "Power Up Canteen (MvM)";
const char CTFItemSchema::k_rchMvMChallengeCompletedMaskAttribName[] = "mvm completed challenges bitmask";
const char CTFItemSchema::k_rchLadderPassItemDefName[] = "Competitive Matchmaking Official";

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFItemSchema::BInitMaps( KeyValues *pKVMaps, CUtlVector<CUtlString> *pVecErrors )
{
	m_vecMasterListOfMaps.PurgeAndDeleteElements();

	FOR_EACH_TRUE_SUBKEY( pKVMaps, pKVMap )
	{
		MapDef_t* pMap = new MapDef_t();

		pMap->pszMapName = pKVMap->GetString( "name", NULL );
		SCHEMA_INIT_CHECK( pMap->pszMapName != NULL,
			"BInitMaps(): missing map name for master map entry %s.", pKVMap->GetName() );
	
		pMap->m_nDefIndex				= V_atoi( pKVMap->GetName() );
		pMap->pszMapNameLocKey			= pKVMap->GetString( "localizedname", NULL );
		pMap->pszAuthorsLocKey			= pKVMap->GetString( "authors", NULL );

		m_vecMasterListOfMaps.AddToTail( pMap );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Inits data for MVM maps / missions
//-----------------------------------------------------------------------------
bool CTFItemSchema::BInitMvmMissions( KeyValues *pKVMvmMaps, CUtlVector<CUtlString> *pVecErrors )
{
	m_vecMvMMaps.RemoveAll();
	m_vecMvMMissions.RemoveAll();

	if ( NULL == pKVMvmMaps )
		return true;

	// initialize the rewards sections
	bool bResult = true;
	
	FOR_EACH_TRUE_SUBKEY( pKVMvmMaps, pKVMap )
	{
		int nMapIndex = m_vecMvMMaps.AddToTail();
		MvMMap_t &map = m_vecMvMMaps[nMapIndex];
		map.m_sMap = pKVMap->GetName();
		int nMapNameLen = strlen( map.m_sMap.Get() );
		map.m_sDisplayName = pKVMap->GetString( "display_name" );

		// Locate missions
		KeyValues *pKVMissions = pKVMap->FindKey( "missions" );
		if ( pKVMissions )
		{
			// Parse mission subkeys
			FOR_EACH_TRUE_SUBKEY( pKVMissions, pKVMission )
			{
				int nMissionIndex = m_vecMvMMissions.AddToTail();		// global mission index (not map-specific)
				MvMMission_t &mission = m_vecMvMMissions[nMissionIndex];
				map.m_vecMissions.AddToTail( nMissionIndex );			// index from map -> global mission list
				mission.m_sPop = pKVMission->GetName();
				mission.m_iDisplayMapIndex = nMapIndex;
				mission.m_sDisplayName = pKVMission->GetString( "display_name" );
				mission.m_sMode = pKVMission->GetString( "mode" );
				mission.m_sMapNameActual = pKVMission->GetString( "map_file_override", map.m_sMap.Get() );
				const char *pszDiff = pKVMission->GetString( "difficulty", "" );
				mission.m_eDifficulty = GetMvMChallengeDifficultyByInternalName( pszDiff );
				mission.m_unMannUpPoints = pKVMission->GetInt( "mannup_points" );
				if ( mission.m_eDifficulty == k_EMvMChallengeDifficulty_Invalid )
				{
					pVecErrors->AddToTail( CUtlString( CFmtStr(
						"MvM mission '%s' on map %s has missing or invalid 'difficulty' value", mission.m_sPop.Get(), map.m_sMap.Get() ) ) );
					bResult = false;
					continue;
				}

				// Pop filenames are required to obey a naming convention.
				if ( ( Q_stricmp( mission.m_sPop.Get(), map.m_sMap.Get() ) != 0 )
					&& ( Q_strnicmp( mission.m_sPop.Get(), map.m_sMap.Get(), nMapNameLen ) != 0
					|| mission.m_sPop.Get()[nMapNameLen] != '_' ) )
				{
					pVecErrors->AddToTail( CUtlString( CFmtStr(
						"MvM mission '%s' on map %s does not obey map prefix naming convention", mission.m_sPop.Get(), map.m_sMap.Get() ) ) );
					bResult = false;
					continue;
				}
			}
		}
		SCHEMA_INIT_CHECK( map.m_vecMissions.Count() > 0,
			"MvM map %s doesn't have any associated missions", map.m_sMap.Get() );
	}

	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: Inits data for MVM tours (sets of missions)
//-----------------------------------------------------------------------------
bool CTFItemSchema::BInitMvmTours( KeyValues *pKVMvmTours, CUtlVector<CUtlString> *pVecErrors )
{
	m_vecMvMTours.RemoveAll();

	if ( NULL == pKVMvmTours )
		return true;

	// initialize the rewards sections
	bool bResult = true;
	
	FOR_EACH_TRUE_SUBKEY( pKVMvmTours, pKVTour )
	{
		MvMTour_t tour;
		tour.m_sTourInternalName = pKVTour->GetName();
		SCHEMA_INIT_CHECK( FindMvmMissionByName( tour.m_sTourInternalName.Get() ) < 0,
			"Duplicate MvM tour \"%s\"", tour.m_sTourInternalName.Get() );

		tour.m_sTourNameLocalizationToken = pKVTour->GetString( "tour_name" );
		SCHEMA_INIT_CHECK( tour.m_sTourNameLocalizationToken.Get() && tour.m_sTourNameLocalizationToken.Get()[0] == '#',
			"MvM tour \"%s\" didn't specify valid localization token for 'tour_name'", tour.m_sTourInternalName.Get() );
				
		const char *pszBadgeItemDefName = pKVTour->GetString( "badge_item_def" );
		tour.m_pBadgeItemDef = pszBadgeItemDefName ? GetItemSchema()->GetItemDefinitionByName( pszBadgeItemDefName ) : NULL;
		SCHEMA_INIT_CHECK( (pszBadgeItemDefName == NULL) == (tour.m_pBadgeItemDef == NULL),
			"MvM tour \"%s\" specified invalid badge definition name '%s'", tour.m_sTourInternalName.Get(), pszBadgeItemDefName );

		const char *pszDiff = pKVTour->GetString( "difficulty", "" );
		tour.m_eDifficulty = GetMvMChallengeDifficultyByInternalName( pszDiff );
		SCHEMA_INIT_CHECK( tour.m_eDifficulty != k_EMvMChallengeDifficulty_Invalid,
			"MvM tour \"%s\" specified invalid difficulty '%s'", tour.m_sTourInternalName.Get(), pszDiff );

		tour.m_bIsNew = pKVTour->GetBool( "is_new", false );

		tour.m_sLootImageName = pKVTour->GetString( "loot_image" );
		SCHEMA_INIT_CHECK( tour.m_sTourNameLocalizationToken.Get() && tour.m_sTourNameLocalizationToken.Get()[0] == '#',
			"MvM tour \"%s\" didn't specify valid localization token for 'tour_name'", tour.m_sTourInternalName.Get() );



		// Locate missions
		tour.m_nAllChallengesBits = 0;

		KeyValues *pKVMissions = pKVTour->FindKey( "missions" );
		if ( pKVMissions )
		{
			bool bContainsNonBitMissions = false;		// does this contain any missions with bit set to -1 (practice)?
			bool bContainsBitMissions = false;			// does this contain any missions with bit set to anything besides -1 (non-practice)?

			// Parse mission values
			FOR_EACH_VALUE( pKVMissions, pKVMission )
			{
				const char *pszMissionName = pKVMission->GetName();
				int iMissionBit = pKVMission->GetInt();

				// Bounds check our bits. -1 is valid because it means "don't track".
				SCHEMA_INIT_CHECK( iMissionBit >= -1 && iMissionBit <= 31,
					"MvM tour \"%s\" mission \"%s\" specifies invalid tour completion bit %i", tour.m_sTourInternalName.Get(), pszMissionName, iMissionBit );

				// Find our mission information to link to.
				int iMissionIndex = FindMvmMissionByName( pszMissionName );
				SCHEMA_INIT_CHECK( m_vecMvMMissions.IsValidIndex( iMissionIndex ),
					"MvM tour \"%s\" unable to locate mission \"%s\"", tour.m_sTourInternalName.Get(), pszMissionName );

				// Make sure the same tour doesn't contain both badge-adjusting missions and non-adjusting
				// missions.
				bContainsNonBitMissions |= (iMissionBit == -1);
				bContainsBitMissions |= (iMissionBit != -1);

				SCHEMA_INIT_CHECK( !bContainsNonBitMissions || !bContainsBitMissions,
					"MvM tour \"%s\" mission \"%s\" contains both practice (-1 bit) and non-practice missions", tour.m_sTourInternalName.Get(), pszMissionName );

				// Make sure we haven't already used this bit for this tour.
				if ( iMissionBit >= 0 )
				{
					uint32 unMask = 1U << (unsigned int)iMissionBit;
					SCHEMA_INIT_CHECK( (tour.m_nAllChallengesBits & unMask) == 0,
						"MvM tour \"%s\" mission \"%s\" re-uses bit %i", tour.m_sTourInternalName.Get(), pszMissionName, iMissionBit );

					tour.m_nAllChallengesBits |= unMask;
				}

				// Success for this mission.
				MvMTourMission_t mission;
				mission.m_iMissionIndex = iMissionIndex;
				mission.m_iBadgeSlot = iMissionBit;

				tour.m_vecMissions.AddToTail( mission );
			}
		}

		SCHEMA_INIT_CHECK( tour.m_vecMissions.Count() > 0,
			"MvM tour \"%s\" has no missions specified", tour.m_sTourInternalName.Get() );

		m_vecMvMTours.AddToTail( tour );
	}

	return bResult;
}

//-----------------------------------------------------------------------------
const char *CTFItemSchema::GetMvmMissionName( int iChallengeIndex ) const
{
	if ( iChallengeIndex == k_iMvmMissionIndex_Any )
		return "(any)";

	if ( m_vecMvMMissions.IsValidIndex( iChallengeIndex ) )
		return m_vecMvMMissions[iChallengeIndex].m_sPop.Get();

	Assert( iChallengeIndex == k_iMvmMissionIndex_NotInSchema );
	return "(invalid)";
	
}

//-----------------------------------------------------------------------------
int CTFItemSchema::FindMvmMissionByName( const char *pszMissionName ) const
{
	if ( pszMissionName == NULL || *pszMissionName == '\0' )
		return k_iMvmMissionIndex_Any;

	FOR_EACH_VEC( m_vecMvMMissions, i )
	{
		if ( !V_stricmp( m_vecMvMMissions[i].m_sPop.Get(), pszMissionName ) )
			return i;
	}
	return k_iMvmMissionIndex_NotInSchema;
}

//-----------------------------------------------------------------------------
int CTFItemSchema::FindMvmTourByName( const char *pszTourName ) const
{
	if ( pszTourName == NULL || *pszTourName == '\0' )
		return k_iMvmTourIndex_Empty;

	FOR_EACH_VEC( m_vecMvMTours, i )
	{
		if ( !V_stricmp( m_vecMvMTours[i].m_sTourInternalName.Get(), pszTourName ) )
			return i;
	}
	return k_iMvmTourIndex_NotInSchema;
}

//-----------------------------------------------------------------------------
int CTFItemSchema::FindMvmMissionInTour( int idxTour, int idxMissionInSchema ) const
{
	if ( idxTour < 0 || idxTour >= m_vecMvMTours.Count() )
		return -1;
	const MvMTour_t &tour = m_vecMvMTours[idxTour];
	FOR_EACH_VEC( tour.m_vecMissions, i )
	{
		if ( tour.m_vecMissions[i].m_iMissionIndex == idxMissionInSchema )
			return i;
	}

	// Not found
	return -1;
}

//-----------------------------------------------------------------------------
int CTFItemSchema::GetMvmMissionBadgeSlotForTour( int idxTour, int idxMissionInSchema ) const
{
	int idxMissionInTour = FindMvmMissionInTour( idxTour, idxMissionInSchema );
	if ( idxMissionInTour < 0 )
		return -1;
	int iBadgeSlot = m_vecMvMTours[idxTour].m_vecMissions[ idxMissionInTour ].m_iBadgeSlot;
	Assert( iBadgeSlot >= 0 );
	return iBadgeSlot;
}



//-----------------------------------------------------------------------------
const MapDef_t *CTFItemSchema::GetMasterMapDefByName( const char *pszSearchName ) const
{
	FOR_EACH_VEC( m_vecMasterListOfMaps, i )
	{
		if ( !V_stricmp( m_vecMasterListOfMaps[i]->pszMapName, pszSearchName ) )
			return m_vecMasterListOfMaps[i];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
const MapDef_t *CTFItemSchema::GetMasterMapDefByIndex( MapDefIndex_t unIndex ) const
{
	FOR_EACH_VEC( m_vecMasterListOfMaps, i )
	{
		if ( m_vecMasterListOfMaps[i]->m_nDefIndex == unIndex )
			return m_vecMasterListOfMaps[i];
	}

	return NULL;
}

RTime32 CTFItemSchema::GetCustomExpirationDate( const char *pszExpirationDate ) const
{
	if ( !V_stricmp( pszExpirationDate, "end_of_halloween" ) )
		return EconHolidays_TerribleHack_GetHalloweenEndData();

	return CEconItemSchema::GetCustomExpirationDate( pszExpirationDate );
}

EMvMChallengeDifficulty GetMvMChallengeDifficultyByInternalName( const char *pszEnglishID )
{
	if ( !Q_stricmp( pszEnglishID, "normal" ) )
		return k_EMvMChallengeDifficulty_Normal;
	if ( !Q_stricmp( pszEnglishID, "intermediate" ) )
		return k_EMvMChallengeDifficulty_Intermediate;
	if ( !Q_stricmp( pszEnglishID, "advanced" ) )
		return k_EMvMChallengeDifficulty_Advanced;
	if ( !Q_stricmp( pszEnglishID, "expert" ) )
		return k_EMvMChallengeDifficulty_Expert;
	if ( !Q_stricmp( pszEnglishID, "haunted" ) )
		return k_EMvMChallengeDifficulty_Haunted;
	return k_EMvMChallengeDifficulty_Invalid;
}

const char *GetMvMChallengeDifficultyLocName( EMvMChallengeDifficulty eDifficulty )
{
	switch ( eDifficulty )
	{
		default:
			AssertMsg( false, "Bogus challenge difficulty" );
		case k_EMvMChallengeDifficulty_Normal:
			return "#TF_MvM_Normal";
		case k_EMvMChallengeDifficulty_Intermediate:
			return "#TF_MvM_Intermediate";
		case k_EMvMChallengeDifficulty_Advanced:
			return "#TF_MvM_Advanced";
		case k_EMvMChallengeDifficulty_Expert:
			return "#TF_MvM_Expert";
		case k_EMvMChallengeDifficulty_Haunted:
			return "#TF_MvM_Haunted";
	}
}