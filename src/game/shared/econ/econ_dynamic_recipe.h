//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Functions related to dynamic recipes
//
//=============================================================================

#ifndef ECON_DYNAMIC_RECIPE
#define ECON_DYNAMIC_RECIPE
#ifdef _WIN32
#pragma once
#endif

#include "tf_gcmessages.h"
#include "game_item_schema.h"
#include "econ_item.h"

extern const char *g_pszAttrEncodeSeparator;

//-----------------------------------------------------------------------------
// Purpose: Stores off all CAttribute_DynamicRecipeComponent attributes
//			that consider m_pTargetItem to be a match.  If NULL is passed in for pTargetItem
//			then we consider all attributes to be a match
//-----------------------------------------------------------------------------
class CRecipeComponentMatchingIterator : public CEconItemSpecificAttributeIterator
{
public:
	CRecipeComponentMatchingIterator( const IEconItemInterface *pSourceItem,
									  const IEconItemInterface *pTargetItem );

	void SetSourceItem( const IEconItemInterface *m_pSourceItem );
	void SetTargetItem( const IEconItemInterface *pTargetItem );
	void SetIgnoreCompleted( bool bIgnoreCompleted ) { m_bIgnoreCompleted = bIgnoreCompleted; }

	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef,
										  const CAttribute_DynamicRecipeComponent& value ) OVERRIDE;

	const CUtlVector< const CEconItemAttributeDefinition* >& GetMatchingComponentInputs() const { return m_vecMatchingInputs; }
	const CUtlVector< const CEconItemAttributeDefinition* >& GetMatchingComponentOutputs() const { return m_vecMatchingOutputs; }

	int GetTotalInputs() const { return m_nInputsTotal; }
	int GetInputsFulfilled() const { return m_nInputsFulfilled; }
	int GetTotalOutputs() const { return m_nOutputsTotal; }
private:

	const IEconItemInterface *m_pSourceItem;
	const IEconItemInterface *m_pTargetItem;
	bool m_bIgnoreCompleted;

	CUtlVector< const CEconItemAttributeDefinition* > m_vecMatchingInputs;
	CUtlVector< const CEconItemAttributeDefinition* > m_vecMatchingOutputs;

	int m_nInputsTotal;
	int m_nInputsFulfilled;
	int m_nOutputsTotal;
};


//-----------------------------------------------------------------------------
// Purpose: Given a CAttribute_DynamicRecipeComponent and a IEconItemInterface,
//			returns whether the item pass the criteria of the attribute
//-----------------------------------------------------------------------------
bool DefinedItemAttribMatch( const CAttribute_DynamicRecipeComponent& attribValue, const IEconItemInterface* pItem );

//-----------------------------------------------------------------------------
// Purpose: Decodes the encoded attributes in attribValue and applies those attributes to pItem
//			Returns true on success, false if anything fails
//-----------------------------------------------------------------------------
bool DecodeAttributeStringIntoAttributes( const CAttribute_DynamicRecipeComponent& attribValue, CUtlVector<CEconItem::attribute_t>& vecAttribs);

//-----------------------------------------------------------------------------
// Purpose: Decodes the encoded attributes in attribValue forms pItem into the
//			item that it describes
//-----------------------------------------------------------------------------
bool DecodeItemFromEncodedAttributeString( const CAttribute_DynamicRecipeComponent& attribValue, CEconItem* pItem );

#endif //ECON_DYNAMIC_RECIPE
