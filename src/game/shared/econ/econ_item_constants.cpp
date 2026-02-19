//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds constants for the econ item system
//
//=============================================================================

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSchemaColorDefHandle g_AttribColorDefs[] =
{
	CSchemaColorDefHandle( "desc_level" ),				// ATTRIB_COL_LEVEL
	CSchemaColorDefHandle( "desc_attrib_neutral" ),		// ATTRIB_COL_NEUTRAL
	CSchemaColorDefHandle( "desc_attrib_positive" ),	// ATTRIB_COL_POSITIVE
	CSchemaColorDefHandle( "desc_attrib_negative" ),	// ATTRIB_COL_NEGATIVE
};

COMPILE_TIME_ASSERT( ARRAYSIZE( g_AttribColorDefs ) == NUM_ATTRIB_COLORS );

attrib_colors_t GetAttribColorIndexForName( const char* pszName )
{
	for ( int i = 0; i < NUM_ATTRIB_COLORS; ++i )
	{
		if ( !Q_strcmp( g_AttribColorDefs[i].GetName(), pszName ) )
			return (attrib_colors_t)i;
	}

	return (attrib_colors_t)0;
}

const char *GetColorNameForAttribColor( attrib_colors_t unAttribColor )
{
	Assert( unAttribColor >= 0 );
	Assert( unAttribColor < NUM_ATTRIB_COLORS );

	return g_AttribColorDefs[unAttribColor]
		 ? g_AttribColorDefs[unAttribColor]->GetColorName()
		 : "ItemAttribNeutral";
}

const char *GetHexColorForAttribColor( attrib_colors_t unAttribColor )
{
	Assert( unAttribColor >= 0 );
	Assert( unAttribColor < NUM_ATTRIB_COLORS );

	return g_AttribColorDefs[unAttribColor]
	     ? g_AttribColorDefs[unAttribColor]->GetHexColor()
		 : "#ebe2ca";
}

const char *g_szRecipeCategoryStrings[] =
{
	"crafting",		// RECIPE_CATEGORY_CRAFTINGITEMS = 0,
	"commonitem",	// RECIPE_CATEGORY_COMMONITEMS,
	"rareitem",		// RECIPE_CATEGORY_RAREITEMS,
	"special",		// RECIPE_CATEGORY_SPECIAL,
};

COMPILE_TIME_ASSERT( ARRAYSIZE( g_szRecipeCategoryStrings ) == NUM_RECIPE_CATEGORIES );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
enum { kMaxCardUpgradesPerItem = 2 };

int GetMaxCardUpgradesPerItem()
{
	return kMaxCardUpgradesPerItem;
}

const CEconItemAttributeDefinition *GetCardUpgradeForIndex( const IEconItemInterface *pItem, int i )
{
	Assert( pItem );
	Assert( i >= 0 );
	Assert( i < kMaxCardUpgradesPerItem );

	class CGetNthUserGeneratedAttributeIterator : public IEconItemUntypedAttributeIterator
	{
	public:
		CGetNthUserGeneratedAttributeIterator( int iTargetIndex )
			: m_iCount( iTargetIndex )
			, m_pAttrDef( NULL )
		{
		}

		virtual bool OnIterateAttributeValueUntyped( const CEconItemAttributeDefinition *pAttrDef ) OVERRIDE
		{
			if ( pAttrDef->GetUserGenerationType() != 0 && m_iCount-- == 0 )
			{
				m_pAttrDef = pAttrDef;
				return false;
			}

			return true;
		}

		const CEconItemAttributeDefinition *GetAttrDef() const { return m_pAttrDef; }

	private:
		int m_iCount;
		const CEconItemAttributeDefinition *m_pAttrDef;
	};

	CGetNthUserGeneratedAttributeIterator findNthAttrIterator( i );
	pItem->IterateAttributes( &findNthAttrIterator );

	return findNthAttrIterator.GetAttrDef();
}
