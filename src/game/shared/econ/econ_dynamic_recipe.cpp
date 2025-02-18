//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Functions related to dynamic recipes
//
//=============================================================================


#include "cbase.h"
#include "econ_dynamic_recipe.h"
	#include "quest_objective_manager.h"

// This pattern was chosen to not be:
//		- a valid string acceptable for user-input (ie., custom name)
//		- a sensical float bit pattern
//		- a common int bit pattern
//		- meaningful Unicode data
const char *g_pszAttrEncodeSeparator = "|\x01\x02\x01\x03|\x01\x02\x01\x03|";

CRecipeComponentMatchingIterator::CRecipeComponentMatchingIterator( const IEconItemInterface *pSourceItem,
																	const IEconItemInterface *pTargetItem )
	: m_pSourceItem( pSourceItem )
	, m_pTargetItem( pTargetItem )
	, m_bIgnoreCompleted( true )
	, m_nInputsTotal( 0 )
	, m_nInputsFulfilled( 0 )
	, m_nOutputsTotal( 0 )
{}

bool CRecipeComponentMatchingIterator::OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_DynamicRecipeComponent& value )
{
	// Don't count ourselves as a match!
	if ( m_pSourceItem && m_pTargetItem && m_pSourceItem->GetID() == m_pTargetItem->GetID() )
		return true;

	// If this isn't a match and the item isn't NULL, we skip.  We consider NULL to mean
	// that we want to tally ALL attributes of this type
	if ( !DefinedItemAttribMatch( value, m_pTargetItem ) && m_pTargetItem != NULL )
		return true;

	// Dont let non-craftable items through
	if ( m_pTargetItem && !m_pTargetItem->IsUsableInCrafting() )
		return true;

	// Is this an output?
	if ( value.component_flags() & DYNAMIC_RECIPE_FLAG_IS_OUTPUT )
	{
		m_vecMatchingOutputs.AddToTail( pAttrDef );
		m_nOutputsTotal += value.num_required();
	}
	else
	{
		m_vecMatchingInputs.AddToTail( pAttrDef );
		m_nInputsTotal += value.num_required();
		m_nInputsFulfilled += value.num_fulfilled();
	}

	return true;
}


bool DefinedItemAttribMatch( const CAttribute_DynamicRecipeComponent& attribValue,  const IEconItemInterface* pItem )
{
	if ( !pItem )
		return false;

	// If our fulfilled count is what our item count is, then we're done.  We dont want any more matches.
	if ( attribValue.num_fulfilled() == attribValue.num_required() )
		return false;

	// If the item_def flag is set, and the item's item_def doesnt match then not a match
	if ( ( attribValue.component_flags() & DYNAMIC_RECIPE_FLAG_PARAM_ITEM_DEF_SET ) &&
		( attribValue.def_index() != (uint32)pItem->GetItemDefIndex() ) )
		return false;

	// If the quality flag is set, and the item's quality doesn't match, then not a match
	if ( ( attribValue.component_flags() & DYNAMIC_RECIPE_FLAG_PARAM_QUALITY_SET ) &&
		( attribValue.item_quality() != (uint32)pItem->GetQuality() ) )
		return false;

	// check if we have ALL required attributes
	if ( attribValue.component_flags() & DYNAMIC_RECIPE_FLAG_PARAM_ATTRIBUTE_SET_ALL )
	{
		CUtlVector<CEconItem::attribute_t> vecAttribs;
		if( !DecodeAttributeStringIntoAttributes( attribValue, vecAttribs ) )
		{
			AssertMsg2( 0, "%s: Unable to decode dynamic recipe attributes on item %llu", __FUNCTION__, pItem->GetID() );
			return false;
		}

		FOR_EACH_VEC( vecAttribs, i )
		{
			const CEconItemAttributeDefinition *pAttr = GetItemSchema()->GetAttributeDefinition( vecAttribs[i].m_unDefinitionIndex );
			Assert( pAttr );
			uint32 itemAttributeValue;
			if ( !pAttr || !pItem->FindAttribute( pAttr, &itemAttributeValue ) || itemAttributeValue != vecAttribs[i].m_value.asUint32 )
			{
				return false;
			}
		}
	}
	// check if we have ANY required attributes
	else if ( attribValue.component_flags() & DYNAMIC_RECIPE_FLAG_PARAM_ATTRIBUTE_SET_ANY )
	{
		CUtlVector<CEconItem::attribute_t> vecAttribs;
		if( !DecodeAttributeStringIntoAttributes( attribValue, vecAttribs ) )
		{
			AssertMsg2( 0, "%s: Unable to decode dynamic recipe attributes on item %llu", __FUNCTION__, pItem->GetID() );
			return false;
		}

		bool bHasAnyMatchingAttributes = false;
		FOR_EACH_VEC( vecAttribs, i )
		{
			const CEconItemAttributeDefinition *pAttr = GetItemSchema()->GetAttributeDefinition( vecAttribs[i].m_unDefinitionIndex );
			Assert( pAttr );
			uint32 itemAttributeValue;
			if ( pAttr && pItem->FindAttribute( pAttr, &itemAttributeValue ) && itemAttributeValue == vecAttribs[i].m_value.asUint32 )
			{
				bHasAnyMatchingAttributes = true;
				break;
			}
		}

		if ( !bHasAnyMatchingAttributes )
		{
			return false;
		}
	}

	return true;
}

bool DecodeAttributeStringIntoAttributes( const CAttribute_DynamicRecipeComponent& attribValue, CUtlVector<CEconItem::attribute_t>& vecAttribs )
{
	CUtlStringList vecAttributeStrings;	// Automatically free'd
	V_SplitString( attribValue.attributes_string().c_str(), g_pszAttrEncodeSeparator, vecAttributeStrings );
	
	if( vecAttributeStrings.Count() % 2 != 0 )
	{
		AssertMsg1( 0, "%s: Uneven count of encoded attribute strings!", __FUNCTION__ );
		return false;
	}

	for( int j = 0; j< vecAttributeStrings.Count(); j+=2 )
	{
		// Get the attribute definition that's stored in the string, and its type
		attrib_definition_index_t index = Q_atoi( vecAttributeStrings[j] );
		const CEconItemAttributeDefinition *pAttrDef = GEconItemSchema().GetAttributeDefinition( index );
		if ( !pAttrDef )
		{
			return false;
		}

		CEconItem::attribute_t& attrib = vecAttribs[vecAttribs.AddToTail()];
		attrib.m_unDefinitionIndex = pAttrDef->GetDefinitionIndex();
		
		// Now have the attribute read in the value stored in the string
		const ISchemaAttributeType* pAttrType = pAttrDef->GetAttributeType();
		pAttrType->InitializeNewEconAttributeValue( &attrib.m_value );

		// Don't fail us now!
		const char* pszAttribValue = vecAttributeStrings[j+1];
		if ( !pAttrType->BConvertStringToEconAttributeValue( pAttrDef, pszAttribValue, &attrib.m_value ) )
		{
			return false;
		}
	}

	return true;
}

bool DecodeItemFromEncodedAttributeString( const CAttribute_DynamicRecipeComponent& attribValue, CEconItem* pItem )
{
	// If the item_def flag is set, set that item def
	if ( attribValue.component_flags() & DYNAMIC_RECIPE_FLAG_PARAM_ITEM_DEF_SET )
	{
		pItem->SetDefinitionIndex( attribValue.def_index() );
	}
	else
	{
		// If the flag is not set, then we want the item name to be generic.  In english, we want to just call it "item".
		CAttribute_String attrStr;
		attrStr.set_value( "#TF_ItemName_Item" );

		static CSchemaAttributeDefHandle pAttrDef_ItemNameTextOverride( "item name text override" );
		pItem->SetDynamicAttributeValue( pAttrDef_ItemNameTextOverride, attrStr );
	}

	// If the quality flag is set, take the quality
	if ( attribValue.component_flags() & DYNAMIC_RECIPE_FLAG_PARAM_QUALITY_SET )
	{
		pItem->SetQuality( attribValue.item_quality() );

		// If there's no item def set and the quality specified is "unique", we want to be explicit
		// and have the item description actually say "unique item" so there's no confusion as to what
		// item quality we want as an input.
		if ( !( attribValue.component_flags() & DYNAMIC_RECIPE_FLAG_PARAM_ITEM_DEF_SET )
			&& pItem->GetQuality() == AE_UNIQUE )
		{
			CAttribute_String attrStr;
			attrStr.set_value( "#unique" );

			static CSchemaAttributeDefHandle pAttrDef_QualityTextOverride( "quality text override" );
			pItem->SetDynamicAttributeValue( pAttrDef_QualityTextOverride, attrStr );
		}
	}
	else
	{
		// If no quality was specified, we want to explicity say that we'll accept ANY quality.
		pItem->SetQuality( AE_UNIQUE );
		CAttribute_String attrStr;
		attrStr.set_value( "#TF_QualityText_Any" );

		static CSchemaAttributeDefHandle pAttrDef_QualityTextOverride( "quality text override" );
		pItem->SetDynamicAttributeValue( pAttrDef_QualityTextOverride, attrStr );
	}
	pItem->SetFlags( 0 );

	// Get all the attributes encoded into the attribute
	CUtlVector<CEconItem::attribute_t> vecAttribs;
	if( !DecodeAttributeStringIntoAttributes( attribValue, vecAttribs ) )
	{
		AssertMsg1( 0, " %s : Unable to decode dynamic recipe attributes", __FUNCTION__ );
		return false;
	}

	// Apply the attributes to the item
	FOR_EACH_VEC( vecAttribs, j )
	{
		// We don't expect to get here with any missing attributes.
		const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinition( vecAttribs[j].m_unDefinitionIndex );
		Assert( pAttrDef );

		const ISchemaAttributeType *pAttrType = pAttrDef->GetAttributeType();
		pAttrType->LoadEconAttributeValue( pItem, pAttrDef, vecAttribs[j].m_value );

		// Free up our attribute memory now that we're done with it.
		pAttrType->UnloadEconAttributeValue( &vecAttribs[j].m_value );
	}

	return true;
}
