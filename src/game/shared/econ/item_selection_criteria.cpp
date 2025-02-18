//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CItemSelectionCriteria, which serves as a criteria for selection
//			of a econ item
//
//=============================================================================


#include "cbase.h"
#include "item_selection_criteria.h"


#if defined(TF_CLIENT_DLL) || defined(TF_DLL)
#include "tf_gcmessages.h"
#endif

#include "gcsdk/enumutils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// copied from \common\econ_item_view.h
#define AE_USE_SCRIPT_VALUE			9999		// Can't be -1, due to unsigned ints used on the backend

ENUMSTRINGS_START( EItemCriteriaOperator )
{ k_EOperator_String_EQ,		"string==" },
{ k_EOperator_String_Not_EQ,	"!string==" },
{ k_EOperator_Float_EQ,			"float==" },
{ k_EOperator_Float_Not_EQ,		"!float==" },
{ k_EOperator_Float_LT,			"float<" },
{ k_EOperator_Float_Not_LT,		"!float<" },
{ k_EOperator_Float_LTE,		"float<=" },
{ k_EOperator_Float_Not_LTE,	"!float<=" },
{ k_EOperator_Float_GT,			"float>" },
{ k_EOperator_Float_Not_GT,		"!float>" },
{ k_EOperator_Float_GTE,		"float>=" },
{ k_EOperator_Float_Not_GTE,	"!float>=" },
{ k_EOperator_Subkey_Contains,	"contains" },
{ k_EOperator_Subkey_Not_Contains, "!contains" },
ENUMSTRINGS_REVERSE( EItemCriteriaOperator, k_EItemCriteriaOperator_Count )

using namespace GCSDK;

//-----------------------------------------------------------------------------
// Purpose: Copy Constructor
//-----------------------------------------------------------------------------
CItemSelectionCriteria::CItemSelectionCriteria( const CItemSelectionCriteria &that )
{
	(*this) = that;
}


//-----------------------------------------------------------------------------
// Purpose: Operator=
//-----------------------------------------------------------------------------
CItemSelectionCriteria &CItemSelectionCriteria::operator=( const CItemSelectionCriteria &rhs )
{
	// Leverage the serialization code we already have for the copy
	CSOItemCriteria msgTemp;
	rhs.BSerializeToMsg( msgTemp );
	BDeserializeFromMsg( msgTemp );

	return *this;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CItemSelectionCriteria::~CItemSelectionCriteria( void )
{
	m_vecConditions.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Purpose: Look through our conditions and find the first of the specified type, 
//			and return the value it's looking for.
//-----------------------------------------------------------------------------
const char *CItemSelectionCriteria::GetValueForFirstConditionOfType( EItemCriteriaOperator eType ) const
{
	// Only supporting this for string conditions right now
	Assert( eType == k_EOperator_String_EQ || eType == k_EOperator_String_Not_EQ );

	FOR_EACH_VEC( m_vecConditions, i )
	{
		if ( m_vecConditions[i]->GetEOp() == eType )
			return m_vecConditions[i]->GetValue();
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Look through our conditions and find the first of the specified type, 
//			and return the value it's looking for.
//-----------------------------------------------------------------------------
const char *CItemSelectionCriteria::GetFieldForFirstConditionOfType( EItemCriteriaOperator eType ) const
{
	FOR_EACH_VEC( m_vecConditions, i )
	{
		if ( m_vecConditions[i]->GetEOp() == eType )
			return m_vecConditions[i]->GetField();
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize from a KV structure
//-----------------------------------------------------------------------------
bool CItemSelectionCriteria::BInitFromKV( KeyValues *pKVCriteria )
{
	// Read in the base fields
	if ( pKVCriteria->FindKey( "level" ) )
	{
		SetItemLevel( pKVCriteria->GetInt( "level" ) );
	}

	if ( pKVCriteria->FindKey( "quality" ) )
	{
		uint8 nQuality;
		if ( !GetItemSchema()->BGetItemQualityFromName( pKVCriteria->GetString( "quality" ), &nQuality ) )
			return false;

		SetQuality( nQuality );
	}

	if ( pKVCriteria->FindKey( "inventoryPos" ) )
	{
		SetInitialInventory( pKVCriteria->GetInt( "inventoryPos" ) );
	}

	if ( pKVCriteria->FindKey( "quantity" ) )
	{
		SetInitialQuantity( pKVCriteria->GetInt( "quantity" ) );
	}

	if ( pKVCriteria->FindKey( "ignore_enabled" ) )
	{
		SetIgnoreEnabledFlag( pKVCriteria->GetBool( "ignore_enabled" ) );
	}

	if ( pKVCriteria->FindKey( "tags" ) )
	{
		SetTags( pKVCriteria->GetString( "tags" ) );
	}

	if ( pKVCriteria->FindKey( "equip_regions" ) )
	{
		SetEquipRegions( pKVCriteria->GetString( "equip_regions" ) );
	}

	KeyValues *pKVConditions = pKVCriteria->FindKey( "conditions", true );

	FOR_EACH_TRUE_SUBKEY( pKVConditions, pKVElement )
	{
		// Check for required fields
		if ( !pKVElement->FindKey( "field" ) ||
			!pKVElement->FindKey( "operator" ) ||
			!pKVElement->FindKey( "value" ) )
			return false;

		const char *pszField = pKVElement->GetString( "field" );
		bool bRequired = pKVElement->GetBool( "required" );
		const char *pszValue = pKVElement->GetString( "value" );

		// Get the operator
		const char *pszOperator = pKVElement->GetString( "operator" );
		EItemCriteriaOperator eOp = EItemCriteriaOperatorFromName( pszOperator );
		if ( k_EItemCriteriaOperator_Count == eOp )
			return false;

		BAddCondition( pszField, eOp, pszValue, bRequired );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CItemSelectionCriteria::SetTags( const char *pszTags )
{
	m_vecTags.Purge();

	m_strTags = pszTags;
	CSplitString splitString( pszTags, " " );
	for ( int i=0; i<splitString.Count(); ++i )
	{
		econ_tag_handle_t tagHandle = GetItemSchema()->GetHandleForTag( splitString[i] );
		if ( !m_vecTags.HasElement( tagHandle ) )
		{
			m_vecTags.AddToTail( tagHandle );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CItemSelectionCriteria::SetEquipRegions( const char *pszEquipRegions )
{
	m_unEquipRegionMask = 0;

	m_strEquipRegions = pszEquipRegions;
	CSplitString splitString( pszEquipRegions, " " );
	for ( int i=0; i<splitString.Count(); ++i )
	{
		m_unEquipRegionMask |= GetItemSchema()->GetEquipRegionBitMaskByName( splitString[i] );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CItemSelectionCriteria::BAddCondition( CItemSelectionCriteria::ICondition *pCondition )
{
	CPlainAutoPtr<ICondition> pConditionPtr( pCondition );

	// Check for condition limit
	if ( UCHAR_MAX == GetConditionsCount() )
	{		
		AssertMsg( false, "Too many conditions on a a CItemSelectionCriteria. Max: 255" );
		return false;
	}

	m_vecConditions.AddToTail( pConditionPtr.Detach() );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a condition to the selection criteria
// Input:	pszField - Field to evaluate on
//			eOp - Operator to apply to the value of the field
//			flValue - The value to compare.
//			bRequired - When true, causes BEvauluate to fail if pszField doesn't
//				exist in the KV being checked.
// Output:	True if the condition was added, false otherwise
//-----------------------------------------------------------------------------
bool CItemSelectionCriteria::BAddCondition( const char *pszField, EItemCriteriaOperator eOp, float flValue, bool bRequired )
{
	// Enforce maximum string lengths
	if ( Q_strlen( pszField ) >= k_cchCreateItemLen )
		return false;

	// Create the appropriate condition for the operator
	switch ( eOp )
	{
	case k_EOperator_Float_EQ:
	case k_EOperator_Float_Not_EQ:
	case k_EOperator_Float_LT:
	case k_EOperator_Float_Not_LT:
	case k_EOperator_Float_LTE:
	case k_EOperator_Float_Not_LTE:
	case k_EOperator_Float_GT:
	case k_EOperator_Float_Not_GT:
	case k_EOperator_Float_GTE:
	case k_EOperator_Float_Not_GTE:
		return BAddCondition( new CFloatCondition( pszField, eOp, flValue, bRequired ) );

	default:
		AssertMsg1( false, "Bad operator (%d) passed to BAddCondition. Float based operator required for this overload.", eOp );
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds a condition to the selection criteria
// Input:	pszField - Field to evaluate on
//			eOp - Operator to apply to the value of the field
//			pszValue - The value to compare.
//			bRequired - When true, causes BEvauluate to fail if pszField doesn't
//				exist in the KV being checked.
// Output:	True if the condition was added, false otherwise
//-----------------------------------------------------------------------------
bool CItemSelectionCriteria::BAddCondition( const char *pszField, EItemCriteriaOperator eOp, const char * pszValue, bool bRequired )
{
	// Enforce maximum string lengths
	if ( Q_strlen( pszField ) >= k_cchCreateItemLen || Q_strlen( pszValue ) >= k_cchCreateItemLen )
		return false;

	// Create the appropriate condition for the operator
	switch ( eOp )
	{
	case k_EOperator_String_EQ:
	case k_EOperator_String_Not_EQ:
		return BAddCondition( new CStringCondition( pszField, eOp, pszValue, bRequired ) );
		return true;

	case k_EOperator_Subkey_Contains:
	case k_EOperator_Subkey_Not_Contains:
		return BAddCondition( new CSetCondition( pszField, eOp, pszValue, bRequired ) );
		return true;

	default:
		// Try the float operators
		return BAddCondition( pszField, eOp, Q_atof( pszValue ), bRequired );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks if a given item matches the item selection criteria
// Input:	itemDef - The item definition to evaluate against
// Output:	True is the item passes the filter, false otherwise
//-----------------------------------------------------------------------------
bool CItemSelectionCriteria::BEvaluate( const CEconItemDefinition* pItemDef ) const
{
	// Disabled items never match
	if ( !m_bIgnoreEnabledFlag && !pItemDef->BEnabled() )
		return false;

	// Filter against level
	if ( BItemLevelSet() && (GetItemLevel() != AE_USE_SCRIPT_VALUE) &&
		( GetItemLevel() < pItemDef->GetMinLevel() || GetItemLevel() > pItemDef->GetMaxLevel() ) )
		return false;

	// Filter against quality
	if ( BQualitySet() && (GetQuality() != AE_USE_SCRIPT_VALUE) )
	{
		if ( GetQuality() != pItemDef->GetQuality() )
		{
			// Filter out item defs that have a non-any quality if we have a non-matching & non-any quality criteria
			if ( k_unItemQuality_Any != GetQuality() && k_unItemQuality_Any != pItemDef->GetQuality() )
				return false;
		}
	}

	// Filter against the additional conditions
	FOR_EACH_VEC( m_vecConditions, i )
	{
		if ( !m_vecConditions[i]->BItemDefinitionPassesCriteria( pItemDef ) )
			return false;
	}

	// Check if we have "any" tags
	if ( m_vecTags.Count() > 0 )
	{
		bool bHasTag = false;
		FOR_EACH_VEC( m_vecTags, i )
		{
			if ( pItemDef->HasEconTag( m_vecTags[i] ) )
			{
				bHasTag = true;
				break;
			}
		}

		if ( !bHasTag )
		{
			return false;
		}
	}

	// check if we match "any" equip regions
	if ( m_unEquipRegionMask != 0 && ( m_unEquipRegionMask & pItemDef->GetEquipRegionMask() )== 0 )
	{
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Determines if the item matches this condition of the criteria
// Input:	pKVItem - Pointer to the raw KeyValues definition of the item
// Output:	True is the item matches, false otherwise
//-----------------------------------------------------------------------------
bool CItemSelectionCriteria::CCondition::BEvaluate( KeyValues *pKVItem ) const
{
	KeyValues *pKVField = pKVItem->FindKey( m_sField.String() );

	// Treat an empty string as a missing field as well.
	bool bIsEmptyString = false;
	if ( m_EOp == k_EOperator_String_EQ || m_EOp == k_EOperator_String_Not_EQ )
	{
		const char *pszItemVal = pKVField ? pKVField->GetString() : NULL;
		bIsEmptyString = ( pszItemVal == NULL || pszItemVal[0] == '\0' );
	}

	// Deal with missing fields
	if ( NULL == pKVField || bIsEmptyString )
	{
		if ( m_bRequired )
			return false;
		else
			return true;
	}

	// Run the operator specific check
	bool bRet = BInternalEvaluate( pKVItem );

	// If this is a "not" operator, reverse the result 
	if ( m_EOp & k_EOperator_Not )
		return !bRet;
	else
		return bRet;
}


//-----------------------------------------------------------------------------
// Purpose: Runs the operator specific check for this condition
// Input:	pKVItem - Pointer to the raw KeyValues definition of the item
// Output:	True is the item matches, false otherwise
//-----------------------------------------------------------------------------
bool CItemSelectionCriteria::CStringCondition::BInternalEvaluate( KeyValues *pKVItem ) const
{
	Assert( k_EOperator_String_EQ == m_EOp || k_EOperator_String_Not_EQ == m_EOp );
	if( !( k_EOperator_String_EQ == m_EOp || k_EOperator_String_Not_EQ == m_EOp ) )
		return false;

	const char *pszItemVal = pKVItem->GetString( m_sField.String() );
	return ( 0 == Q_stricmp( m_sValue.String(), pszItemVal ) );
}

bool CItemSelectionCriteria::CStringCondition::BSerializeToMsg( CSOItemCriteriaCondition & msg ) const
{
	CCondition::BSerializeToMsg( msg );
	msg.set_string_value( m_sValue.Get() );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Runs the operator specific check for this condition
// Input:	pKVItem - Pointer to the raw KeyValues definition of the item
// Output:	True is the item matches, false otherwise
//-----------------------------------------------------------------------------
bool CItemSelectionCriteria::CSetCondition::BInternalEvaluate( KeyValues *pKVItem ) const
{
	Assert( k_EOperator_Subkey_Contains == m_EOp || k_EOperator_Subkey_Not_Contains == m_EOp );
	if( !( k_EOperator_Subkey_Contains == m_EOp || k_EOperator_Subkey_Not_Contains == m_EOp ) )
		return false;

	return ( NULL != pKVItem->FindKey( m_sField.String() )->FindKey( m_sValue.String() ) );
}

bool CItemSelectionCriteria::CSetCondition::BSerializeToMsg( CSOItemCriteriaCondition & msg ) const
{
	CCondition::BSerializeToMsg( msg );
	msg.set_string_value( m_sValue.Get() );
	return true;
}

 
//-----------------------------------------------------------------------------
// Purpose: Runs the operator specific check for this condition
// Input:	pKVItem - Pointer to the raw KeyValues definition of the item
// Output:	True is the item matches, false otherwise
//-----------------------------------------------------------------------------
bool CItemSelectionCriteria::CFloatCondition::BInternalEvaluate( KeyValues *pKVItem ) const
{
	float itemValue = pKVItem->GetFloat( m_sField.String() );

	switch ( m_EOp )
	{
	case k_EOperator_Float_EQ:
	case k_EOperator_Float_Not_EQ:
		return ( itemValue == m_flValue );

	case k_EOperator_Float_LT:
	case k_EOperator_Float_Not_LT:
		return ( itemValue < m_flValue );

	case k_EOperator_Float_LTE:
	case k_EOperator_Float_Not_LTE:
		return ( itemValue <= m_flValue );

	case k_EOperator_Float_GT:
	case k_EOperator_Float_Not_GT:
		return ( itemValue > m_flValue );

	case k_EOperator_Float_GTE:
	case k_EOperator_Float_Not_GTE:
		return ( itemValue >= m_flValue );

	default:
		AssertMsg1( false, "Unknown operator: %d", m_EOp );
		return false;
	}
}

bool CItemSelectionCriteria::CFloatCondition::BSerializeToMsg( CSOItemCriteriaCondition & msg ) const
{
	CCondition::BSerializeToMsg( msg );
	msg.set_float_value( m_flValue );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Serialize the item selection criteria to the given message
// Input:	msg - The message to serialize to.
// Output:	True if the operation was successful, false otherwise.
//-----------------------------------------------------------------------------
bool CItemSelectionCriteria::BSerializeToMsg( CSOItemCriteria & msg ) const
{
	msg.set_item_level( m_unItemLevel );
	msg.set_item_quality( m_nItemQuality );
	msg.set_item_level_set( m_bItemLevelSet );
	msg.set_item_quality_set( m_bQualitySet );
	msg.set_initial_inventory( m_unInitialInventory );
	msg.set_initial_quantity( m_unInitialQuantity );
	msg.set_ignore_enabled_flag( m_bIgnoreEnabledFlag );
	msg.set_tags( m_strTags );
	msg.set_equip_regions( m_strEquipRegions );

	FOR_EACH_VEC( m_vecConditions, i )
	{
		CSOItemCriteriaCondition *pConditionMsg = msg.add_conditions();
		m_vecConditions[i]->BSerializeToMsg( *pConditionMsg );
	}
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Deserializes the item selection criteria from the given message
// Input:	msg - The message to deserialize from.
// Output:	True if the operation was successful, false otherwise.
//-----------------------------------------------------------------------------
bool CItemSelectionCriteria::BDeserializeFromMsg( const CSOItemCriteria & msg )
{
	m_unItemLevel = msg.item_level();
	m_nItemQuality = msg.item_quality();
	m_bItemLevelSet = msg.item_level_set();
	m_bQualitySet = msg.item_quality_set();
	m_unInitialInventory = msg.initial_inventory();
	m_unInitialQuantity = msg.initial_quantity();
	m_bIgnoreEnabledFlag = msg.ignore_enabled_flag();

	SetTags( msg.tags().c_str() );
	SetEquipRegions( msg.equip_regions().c_str() );

	uint32 unCount = msg.conditions_size();
	m_vecConditions.EnsureCapacity( unCount );

	for ( uint32 i = 0; i < unCount; i++ )
	{
		const CSOItemCriteriaCondition & cond = msg.conditions( i );
		EItemCriteriaOperator eOp = (EItemCriteriaOperator)cond.op();
		bool bRequired = cond.required();

		// Read the value specific to the condition and add the condition
		switch ( eOp )
		{
		case k_EOperator_Float_EQ:
		case k_EOperator_Float_Not_EQ:
		case k_EOperator_Float_LT:
		case k_EOperator_Float_Not_LT:
		case k_EOperator_Float_LTE:
		case k_EOperator_Float_Not_LTE:
		case k_EOperator_Float_GT:
		case k_EOperator_Float_Not_GT:
		case k_EOperator_Float_GTE:
		case k_EOperator_Float_Not_GTE:
			{
				if ( !BAddCondition( cond.field().c_str(), eOp, cond.float_value(), bRequired ) )	return false;
				break;
			}

		case k_EOperator_String_EQ:
		case k_EOperator_String_Not_EQ:
		case k_EOperator_Subkey_Contains:
		case k_EOperator_Subkey_Not_Contains:
			{
				if ( !BAddCondition( cond.field().c_str(), eOp, cond.string_value().c_str(), bRequired ) )	return false;
				break;
			}

		default:
			AssertMsg1( false, "Unknown operator (%d) read.", eOp );
			return false;
		}
	}


	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Serializes a condition to a message.
// Input:	msg - The message to serialize to.
// Output:	True if the operation was successful, false otherwise.
//-----------------------------------------------------------------------------
bool CItemSelectionCriteria::CCondition::BSerializeToMsg( CSOItemCriteriaCondition & msg ) const
{
	msg.set_op( m_EOp );
	msg.set_field( m_sField.String() );
	msg.set_required( m_bRequired );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CItemSelectionCriteria::CCondition::BItemDefinitionPassesCriteria( const CEconItemDefinition *pItemDef ) const
{
	return BEvaluate( pItemDef->GetRawDefinition() );
}

// Validation
#ifdef DBGFLAG_VALIDATE

//-----------------------------------------------------------------------------
// Purpose: Run a global validation pass on all of our data structures and memory
//			allocations.
// Input:	validator -		Our global validator object
//			pchName -		Our name (typically a member var in our container)
//-----------------------------------------------------------------------------
void CItemSelectionCriteria::Validate( CValidator &validator, const char *pchName )
{
	VALIDATE_SCOPE();
	ValidateObj( m_vecConditions );
	FOR_EACH_VEC( m_vecConditions, i )
	{
		ValidatePtr( m_vecConditions[i] );
	}
}

void CItemSelectionCriteria::CCondition::Validate( CValidator &validator, const char *pchName )
{
	ValidateObj( m_sField );
}

void CItemSelectionCriteria::CStringCondition::Validate( CValidator &validator, const char *pchName )
{
	CCondition::Validate( validator, pchName );
	ValidateObj( m_sValue );
}

void CItemSelectionCriteria::CSetCondition::Validate( CValidator &validator, const char *pchName )
{
	CCondition::Validate( validator, pchName );
	ValidateObj( m_sValue );
}

#endif
