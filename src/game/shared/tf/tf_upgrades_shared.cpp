//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Load item upgrade data from KeyValues
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include "tf_shareddefs.h"
#include "tf_upgrades_shared.h"
#include "filesystem.h"
#include "econ_item_system.h"
#include "tf_gamerules.h"
#include "tf_item_powerup_bottle.h"


CMannVsMachineUpgradeManager g_MannVsMachineUpgrades;

CMannVsMachineUpgradeManager::CMannVsMachineUpgradeManager()
{
	SetDefLessFunc( m_AttribMap );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineUpgradeManager::LevelInitPostEntity()
{
	LoadUpgradesFile();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineUpgradeManager::LevelShutdownPostEntity()
{
	m_Upgrades.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineUpgradeManager::ParseUpgradeBlockForUIGroup( KeyValues *pKV, int iDefaultUIGroup )
{
	if ( !pKV )
		return;

	for ( KeyValues *pData = pKV->GetFirstSubKey(); pData != NULL; pData = pData->GetNextKey() )
	{
		// Check that the expected data is there
		KeyValues *pkvAttribute = pData->FindKey( "attribute" );
		KeyValues *pkvIcon = pData->FindKey( "icon" );
		KeyValues *pkvIncrement = pData->FindKey( "increment" );
		KeyValues *pkvCap = pData->FindKey( "cap" );
		KeyValues *pkvCost = pData->FindKey( "cost" );
		if ( !pkvAttribute || !pkvIcon || !pkvIncrement || !pkvCap || !pkvCost )
		{
			Warning( "Upgrades: One or more upgrades missing attribute, icon, increment, cap, or cost value.\n" );
			return;
		}

		int index = m_Upgrades.AddToTail();

		const char *pszAttrib = pData->GetString( "attribute" );
		V_strncpy( m_Upgrades[ index ].szAttrib, pszAttrib, sizeof( m_Upgrades[ index ].szAttrib ) );
		const CEconItemSchema *pSchema = ItemSystem()->GetItemSchema();
		if ( pSchema )
		{
			// If we can't find a matching attribute, nuke this entry completely
			const CEconItemAttributeDefinition *pAttr = pSchema->GetAttributeDefinitionByName( m_Upgrades[ index ].szAttrib );
			if ( !pAttr )
			{
				Warning( "Upgrades: Invalid attribute reference! -- %s.\n", m_Upgrades[ index ].szAttrib );
				m_Upgrades.Remove( index );
				continue;
			}
			Assert( pAttr->GetAttributeType() );
			if ( !pAttr->GetAttributeType()->BSupportsGameplayModificationAndNetworking() )
			{
				Warning( "Upgrades: Invalid attribute '%s' is of a type that doesn't support networking!\n", m_Upgrades[ index ].szAttrib );
				m_Upgrades.Remove( index );
				continue;
			}
			if ( !pAttr->IsStoredAsFloat() || pAttr->IsStoredAsInteger() )
			{
				Warning( "Upgrades: Attribute reference '%s' is not stored as a float!\n", m_Upgrades[ index ].szAttrib );
				m_Upgrades.Remove( index );
				continue;
			}
		}

		V_strncpy( m_Upgrades[index].szIcon, pData->GetString( "icon" ), sizeof( m_Upgrades[ index ].szIcon ) );
		m_Upgrades[ index ].flIncrement = pData->GetFloat( "increment" );
		m_Upgrades[ index ].flCap = pData->GetFloat( "cap" );
		m_Upgrades[ index ].nCost = pData->GetInt( "cost" );
		m_Upgrades[ index ].nUIGroup = pData->GetInt( "ui_group", iDefaultUIGroup );
		m_Upgrades[ index ].nQuality = pData->GetInt( "quality", MVM_UPGRADE_QUALITY_NORMAL );
		m_Upgrades[ index ].nTier = pData->GetInt( "tier", 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMannVsMachineUpgradeManager::GetAttributeIndexByName( const char* pszAttributeName )
{
	// Already in the map?
	if( m_AttribMap.Find( pszAttributeName ) != m_AttribMap.InvalidIndex() )
	{
		return m_AttribMap.Element( m_AttribMap.Find( pszAttributeName ) );
	}

	// Not in the map.  Find it in the vector and add it to the map
	for( int i=0, nCount = m_Upgrades.Count() ; i<nCount; ++i )
	{
		// Find the index
		const char* pszAttrib = m_Upgrades[i].szAttrib;
		if( FStrEq( pszAttributeName, pszAttrib ) )
		{
			// Add to map
			m_AttribMap.Insert( pszAttributeName, i );
			// Return value
			return i;
		}
	}

	AssertMsg1( 0, "Attribute \"%s\" not found!", pszAttributeName );
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineUpgradeManager::LoadUpgradesFile( void )
{
	// Determine the upgrades file to load
	const char *pszPath = "scripts/items/mvm_upgrades.txt";

	// Allow map to override
	const char *pszCustomUpgradesFile = TFGameRules()->GetCustomUpgradesFile();
	if ( TFGameRules() && pszCustomUpgradesFile && pszCustomUpgradesFile[0] )
	{
		pszPath = pszCustomUpgradesFile;
	}

	LoadUpgradesFileFromPath( pszPath );
}


//-----------------------------------------------------------------------------
// Purpose: Loads an upgrade file from a specific path
//-----------------------------------------------------------------------------
void CMannVsMachineUpgradeManager::LoadUpgradesFileFromPath( const char *pszPath )
{
	// Check that the path is valid
	const char *pszExtension = V_GetFileExtension( pszPath );
	if ( V_strstr( pszPath, ".." ) || V_strstr( pszPath, " " ) ||
		V_strstr( pszPath, "\r" ) || V_strstr( pszPath, "\n" ) ||
		V_strstr( pszPath, ":" ) || V_strstr( pszPath, "\\\\" ) ||
		V_IsAbsolutePath( pszPath ) ||
		pszExtension == NULL || V_strcmp( pszExtension, "txt" ) != 0 )
	{
		return;
	}

	KeyValues *pKV = new KeyValues( "Upgrades" );
	if ( !pKV->LoadFromFile( filesystem, pszPath, "GAME" ) )
	{
		Warning( "Can't open %s\n", pszPath );
		pKV->deleteThis();
		return;
	}

	m_Upgrades.RemoveAll();

	// Parse upgrades.txt
	ParseUpgradeBlockForUIGroup( pKV->FindKey( "ItemUpgrades" ), 0 );
	ParseUpgradeBlockForUIGroup( pKV->FindKey( "PlayerUpgrades" ), 1 );

	pKV->deleteThis();
}


int GetUpgradeStepData( CTFPlayer *pPlayer, int nWeaponSlot, int nUpgradeIndex, int &nCurrentStep, bool &bOverCap )
{
	if ( !pPlayer )
		return 0;

	// Get the item entity. We use the entity, not the item in the loadout, because we want
	// the dynamic attributes that have already been purchases and attached.
	CEconEntity *pEntity = NULL;
	const CEconItemView *pItemData = CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot( pPlayer, nWeaponSlot, &pEntity );

	const CMannVsMachineUpgrades *pMannVsMachineUpgrade = &( g_MannVsMachineUpgrades.m_Upgrades[ nUpgradeIndex ] );

	CEconItemAttributeDefinition *pAttribDef = ItemSystem()->GetStaticDataForAttributeByName( pMannVsMachineUpgrade->szAttrib );
	if ( !pAttribDef )
		return 0;

	// Special-case short-circuit logic for the powerup bottle. I don't know why we do this, but
	// we did before so this seems like the safest way of not breaking anything.
	const CTFPowerupBottle *pPowerupBottle = dynamic_cast< CTFPowerupBottle* >( pEntity );
	if ( pPowerupBottle )
	{
		Assert( pMannVsMachineUpgrade->nUIGroup == UIGROUP_POWERUPBOTTLE );

		nCurrentStep = ::FindAttribute( pItemData, pAttribDef )
					 ? pPowerupBottle->GetNumCharges()
					 : 0;
		bOverCap = nCurrentStep == pPowerupBottle->GetMaxNumCharges();
		
		return pPowerupBottle->GetMaxNumCharges();
	}

	Assert( pAttribDef->IsStoredAsFloat() );
	Assert( !pAttribDef->IsStoredAsInteger() );

	int nFormat = pAttribDef->GetDescriptionFormat();

	bool bPercentage = nFormat == ATTDESCFORM_VALUE_IS_PERCENTAGE || nFormat == ATTDESCFORM_VALUE_IS_INVERTED_PERCENTAGE;

	// Find the baseline value for this attribute. We start by assuming that it has no default value on
	// the item level (CEconItem) and defaulting to 100% for percentages and 0 for anything else.
	float flBase = bPercentage ? 1.0f : 0.0f;
	
	// If the item has a backing store, we pull from that to find the attribute value before any
	// gameplay-specific (CEconItemView-level) attribute modifications. If we're a player we don't have
	// any persistent backing store. This will either stomp our above value if found or leave it unchanged
	// if not found.
	if ( pItemData && pItemData->GetSOCData() )
	{
		::FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pItemData->GetSOCData(), pAttribDef, &flBase );
	}

	// ...
	float flCurrentAttribValue = bPercentage ? 1.0f : 0.0f;
	
	if ( pMannVsMachineUpgrade->nUIGroup == UIGROUP_UPGRADE_ATTACHED_TO_PLAYER )
	{
		::FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pPlayer->GetAttributeList(), pAttribDef, &flCurrentAttribValue );
	}
	else
	{
		Assert( pMannVsMachineUpgrade->nUIGroup == UIGROUP_UPGRADE_ATTACHED_TO_ITEM );
		if ( pItemData )
		{
			::FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pItemData, pAttribDef, &flCurrentAttribValue );
		}
	}

	// ...
	const float flIncrement = pMannVsMachineUpgrade->flIncrement;
	
	// Figure out the cap value for this attribute. We start by trusting whatever is specified in our
	// upgrade config but if we're dealing with an item that specifies different properties at a level
	// before MvM upgrades (ie., the Soda Popper already specifies "Reload time decreased") then we
	// need to make sure we consider that the actual high end for UI purposes.
	const float flCap = pMannVsMachineUpgrade->flCap;

	if ( BIsAttributeValueWithDeltaOverCap( flCurrentAttribValue, flIncrement, flCap ) )
	{
		// Early out here -- we know we're over the cap already, so just fill out and return values
		// that show that.
		bOverCap = true;
		nCurrentStep = RoundFloatToInt( fabsf( ( flCurrentAttribValue - flBase ) / flIncrement ) );

		return nCurrentStep;			// Include the 0th step
	}

	// Calculate the the total number of upgrade levels and current upgrade level
	int nNumSteps = 0;
	
	// ...
	nNumSteps = RoundFloatToInt( fabsf( ( flCap - flBase ) / flIncrement ) );
	nCurrentStep = RoundFloatToInt( fabsf( ( flCurrentAttribValue - flBase ) / flIncrement ) );

	// Include the 0th step
	return nNumSteps;
}
