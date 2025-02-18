//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef ECONITEMDESCRIPTION_H
#define ECONITEMDESCRIPTION_H

#ifdef _WIN32
#pragma once
#endif

#include "localization_provider.h"		// needed for locchar_t type

#if defined( TF_DLL ) || defined( TF_CLIENT_DLL ) || defined( TF_GC_DLL )
	#define PROJECT_TF
#endif


	#define TF_ANTI_IDLEBOT_VERIFICATION_ONLY_COMMA
	#define TF_ANTI_IDLEBOT_VERIFICATION_ONLY_ARG( arg )
	#define TF_ANTI_IDLEBOT_VERIFICATION_ONLY_ARG_BOOL_TRUE( arg ) true



class IEconItemInterface;
namespace GCSDK
{
	class CSharedObjectTypeCache;
}

//-----------------------------------------------------------------------------
// Purpose: Generate a description block for an IEconItemInterface. What the
//			client does with the description is anyone's guess, but this will
//			generate a block of UTF16 lines of text with meta/color data that
//			can be used for whatever.
//-----------------------------------------------------------------------------

enum EDescriptionLineMetaFlags
{
	kDescLineFlag_Name		= 0x001,						// the item name (can be renamed by user)
	kDescLineFlag_Type		= 0x002,						// the item type (ie., "Level 5 Rocket Launcher")
	kDescLineFlag_Desc		= 0x004,						// base item description (description from the item definition, level, etc.)
	kDescLineFlag_Attribute	= 0x008,						// some sort of gameplay-affecting attribute
	kDescLineFlag_Misc		= 0x010,						// not an attribute, not name/level
	kDescLineFlag_Empty		= 0x020,						// line with no content that needs to be displayed; meant for spacing
	kDescLineFlag_Set		= 0x040,						// this line is associated with item sets somehow
	kDescLineFlag_LimitedUse= 0x080,						// this is a limited use item
	kDescLineFlag_SetName	= 0x100,						// this line is the title for an item set
	kDescLineFlag_Collection			= 0x200,			// this line is associated with item collections
	kDescLineFlag_CollectionCurrentItem	= 0x400,			// this line is the current item being describe
	kDescLineFlag_CollectionName		= 0x800,			// this line is the collection name
	kDescLineFlag_CaseBonusContent		= 0x1000,			// this line is the case bonus content
	kDescLineFlag_MouseOverPanel		= 0x2000,			// this line is for mouse over panel only
	kDescLineFlag_UserProvided			= 0x4000,			// user-generated content

	kDescLineFlagSet_DisplayInAttributeBlock = ~(kDescLineFlag_Name | kDescLineFlag_Type),
};

struct econ_item_description_line_t
{
	attrib_colors_t					eColor;						// desired color type for this line -- will likely be looked up either in VGUI or in the schema
	uint32							unMetaType;					// type information for this line -- "item name"? "item level"?; etc.; can be game-specific
	CUtlConstStringBase<locchar_t>	sText;						// actual text for this, post-localization
	item_definition_index_t			unDefIndex;					// item def index for description lines which represent names of other items (used by bundles)
	bool							bIsItemForSale;				// if this line is an item (eg in the case where a bundle description lists its contained items) - is the given item for sale?
};

class IEconItemDescription
{
public:
	// This may yield on the GC and should never yield on the client.
	static void YieldingFillOutEconItemDescription( IEconItemDescription *out_pDescription, CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );

public:
	IEconItemDescription() { }
	virtual ~IEconItemDescription() { }

	uint32 GetLineCount() const { return m_vecDescLines.Count(); }
	const econ_item_description_line_t& GetLine( int i ) const { return m_vecDescLines[i]; }

	// Finds and returns the first line with *all* of the passed in search flags. Will return NULL if a line
	// will all of the flags cannot be found.
	const econ_item_description_line_t *GetFirstLineWithMetaType( uint32 unMetaTypeSearchFlags ) const;

private:
	// When generating an item description, this is guaranteed to be called once and only once before GenerateDescription()
	// is called. Any data that may yield but will be needed somewhere deep inside GenerateDescription() should be determined
	// and cached off here.
	virtual void YieldingCacheDescriptionData( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem ) { }

	// Take the properties off our pEconItem, and anything that we calculated in YieldingCacheDescriptionData() above and
	// fill out all of our description lines.
	virtual void GenerateDescriptionLines( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem ) = 0;

protected:
	CUtlVector<econ_item_description_line_t> m_vecDescLines;
};

// This will be defined as either 1 or 0 depending on which project we're in. We test its value explicitly
// rather than just checking defined() because otherwise failing to include this header file will silently
// result in it appearing to be undefined.
#define BUILD_ITEM_NAME_AND_DESC (defined( CLIENT_DLL ) || defined( GC_DLL ))

#if BUILD_ITEM_NAME_AND_DESC
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class IAccountPersonaLocalizer
{
public:
	virtual const locchar_t *FindAccountPersonaName( uint32 unAccountID ) const = 0;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEconItemDescription : public IEconItemDescription, public IAccountPersonaLocalizer
{
public:
	// Instances should be filled out via YieldingFillOutEconItemDescription().
	CEconItemDescription()
		: m_bUnknownPlayer( false )
		, m_bIsToolTip( false )
	{
		//
	}

	// External helper interface, also used internally. This should only be used to add lines
	// that are not a part of the properties of the item, but are instead a part of the environment
	// around the item (ie., "this item cannot be equipped in this slot because another item is
	// equipped that has conflicting regions").
	//
	// The final argument is an optional target array to use for the description lines instead of
	// our internal storage. We can use this to queue up and then batch-submit/-discard lines. Passing
	// in NULL means "use the internal array".
	virtual void AddDescLine( const locchar_t *pString, attrib_colors_t eColor, uint32 unMetaType, CUtlVector<econ_item_description_line_t> *out_pOptionalDescLineDest = NULL, item_definition_index_t unDefIndex = INVALID_ITEM_DEF_INDEX, bool bIsItemForSale = true );
	virtual void AddEmptyDescLine( CUtlVector<econ_item_description_line_t> *out_pOptionalDescLineDest = NULL );
	virtual void LocalizedAddDescLine( const CLocalizationProvider *pLocalizationProvider, const char *pLocalizationToken, attrib_colors_t eColor, uint32 unMetaType, CUtlVector<econ_item_description_line_t> *out_pOptionalDescLineDest = NULL, item_definition_index_t unDefIndex = INVALID_ITEM_DEF_INDEX, bool bIsItemForSale = true );

	// A helper class to iterate all attributes that we expect to appear on an item description. This
	// is useable from outside CEconItemDescription. Attributes can be accessed in iteration order or
	// manually sorted to be grouped by positive/negative status, etc.
	class CVisibleAttributeDisplayer : public IEconItemAttributeIterator
	{
	public:
		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, attrib_value_t value ) OVERRIDE;

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, float value ) OVERRIDE
		{
			return true;
		}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const uint64& value ) OVERRIDE
		{
			return true;
		}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_String& value ) OVERRIDE
		{
			return true;
		}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_DynamicRecipeComponent& value ) OVERRIDE
		{
			// Don't show these
			return true;
		}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_ItemSlotCriteria& value ) OVERRIDE
		{
			// Don't show these
			return true;
		}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_WorldItemPlacement& value ) OVERRIDE
		{
			// Don't show these
			return true;
		}

		void SortAttributes();
		void Finalize( const IEconItemInterface *pEconItem, CEconItemDescription *pEconItemDescription, const CLocalizationProvider *pLocalizationProvider );

	private:
		struct attrib_iterator_value_t
		{
			const CEconItemAttributeDefinition *m_pAttrDef;
			attrib_value_t m_value;
		};

		CUtlVector<attrib_iterator_value_t> m_vecAttributes;
	};

	class CRecipeNameAttributeDisplayer : public CVisibleAttributeDisplayer
	{
	public:
		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, attrib_value_t value ) OVERRIDE;
	};


	bool HasUnknownPlayer( ) const
	{
		return m_bUnknownPlayer;	
	}
	void SetIsToolTip( bool bIsToolTip )
	{
		m_bIsToolTip = bIsToolTip;
	}

private:
	// IEconItemDescription interface.
	virtual void YieldingCacheDescriptionData( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void GenerateDescriptionLines( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );

private:
	// Internal.
	virtual void AddAttributeDescription( const CLocalizationProvider *pLocalizationProvider, const CEconItemAttributeDefinition *pAttribDef, attrib_value_t value, attrib_colors_t eOverrideDisplayColor = NUM_ATTRIB_COLORS, uint32 unAdditionalMetaType = 0 );
	
	virtual void Generate_ItemName( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_ItemLevelDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_CraftTag( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_StyleDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_HolidayRestriction( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_QualityDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_ItemRarityDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_WearAmountDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_ItemDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_Bundle( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_GiftedBy( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
#ifdef PROJECT_TF
	virtual void Generate_DuelingMedal( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_MapContributor( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_MapStampBundleTooltip( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_FriendlyHat( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_SaxxyAwardDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_MvmChallenges( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_SquadSurplusClaimedBy( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_DynamicRecipe( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_UnusualifierEffectList( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
#endif // PROJECT_TF
	virtual void Generate_XifierToolTargetItem( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_Painted( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_Uses( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_LootListDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_EventDetail( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_ItemSetDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_CollectionDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_BonusContentDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_ExpirationDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_MarketInformation( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_FlagsAttributes( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_DropPeriodDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	
	virtual void Generate_VisibleAttributes( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );
	virtual void Generate_DirectX8Warning( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem );

	// Helpers for the above.
	virtual void Generate_ItemLevelDesc_Default( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem, const locchar_t *locTypename );
	virtual bool BGenerate_ItemLevelDesc_StrangeNameAndStats( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem, const locchar_t *locTypename );		// returns true if generated a level/desc based on strange stats or false if nothing was generated
	const locchar_t *GetLocalizedStringForStrangeRestrictionAttr( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem, int iAttrIndex ) const;

	// Internal data.
	void YieldingFillOutAccountPersonaName( const CLocalizationProvider *pLocalizationProvider, uint32 unAccountID );
	const locchar_t *FindAccountPersonaName( uint32 unAccountID ) const;

	void YieldingFillOutAccountTypeCache( uint32 unAccountID, int nClassID );
	GCSDK::CSharedObjectTypeCache *FindAccountTypeCache( uint32 unAccountID, int nClassID ) const;

	// Defined in source file -- not meant for external access.
	template < typename T >
	const T *FindAccountTypeCacheSingleton( uint32 unAccountID, int nClassID ) const;

	// Precache data.
	struct steam_account_persona_name_t
	{
		uint32							unAccountID;
		CUtlConstStringBase<locchar_t>	loc_sPersonaName;
	};

	CUtlVector<steam_account_persona_name_t> vecPersonaNames;

	struct steam_account_type_cache_t
	{
		uint32							unAccountID;
		int								nClassID;
		GCSDK::CSharedObjectTypeCache  *pTypeCache;
	};

	CUtlVector<steam_account_type_cache_t> vecTypeCaches;

	bool m_bUnknownPlayer;
	bool m_bIsToolTip;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEconAttributeDescription
{
private:
	// Internal constructor.
	void InternalConstruct
	(
		const CLocalizationProvider *pLocalizationProvider,
		const CEconItemAttributeDefinition *pAttribDef,
		attrib_value_t value,
		TF_ANTI_IDLEBOT_VERIFICATION_ONLY_ARG( MD5Context_t *pHashContext ) TF_ANTI_IDLEBOT_VERIFICATION_ONLY_COMMA
		IAccountPersonaLocalizer *pOptionalAccountPersonaLocalizer
	);

public:
	// Outward-facing constructor. Pass in whatever you want for "value" and we'll
	// use the raw bits for their value interpreted however the attribute says.
	template < typename T >
	CEconAttributeDescription
	(
		const CLocalizationProvider *pLocalizationProvider,
		const CEconItemAttributeDefinition *pAttribDef,
		T value,
		TF_ANTI_IDLEBOT_VERIFICATION_ONLY_ARG( MD5Context_t *pHashContext = NULL ) TF_ANTI_IDLEBOT_VERIFICATION_ONLY_COMMA
		IAccountPersonaLocalizer *pOptionalAccountPersonaLocalizer = NULL
	)
	{
		COMPILE_TIME_ASSERT( sizeof( T ) == sizeof( attrib_value_t ) );

		InternalConstruct( pLocalizationProvider, pAttribDef, *(attrib_value_t *)&value, TF_ANTI_IDLEBOT_VERIFICATION_ONLY_ARG( pHashContext ) TF_ANTI_IDLEBOT_VERIFICATION_ONLY_COMMA pOptionalAccountPersonaLocalizer  );
	}

	const CUtlConstStringBase<locchar_t>& GetDescription() const { return m_loc_sValue; }
	const CUtlConstStringBase<locchar_t>& GetShortDescription() const { return m_loc_sShortValue; }
	attrib_colors_t GetDefaultColor() const { return m_eDefaultColor; }

private:
	CUtlConstStringBase<locchar_t> m_loc_sValue;
	CUtlConstStringBase<locchar_t> m_loc_sShortValue;
	attrib_colors_t m_eDefaultColor;
};

//-----------------------------------------------------------------------------
// Purpose: control how item name is generated
//-----------------------------------------------------------------------------
enum EGenerateLocalizedFullItemNameFlag_t
{
	k_EGenerateLocalizedFullItemName_Default = 0,
	k_EGenerateLocalizedFullItemName_WithPaintWear = ( 1 << 0 ),
	k_EGenerateLocalizedFullItemName_WithoutCustomName = ( 1 << 1 ),
	k_EGenerateLocalizedFullItemName_WithoutQuality = ( 1 << 2 ),
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEconItemLocalizedFullNameGenerator
{
public:
	CEconItemLocalizedFullNameGenerator( const CLocalizationProvider *pLocalizationProvider, const CEconItemDefinition *pItemDef, bool bUseingHashContext = true, entityquality_t eQuality = AE_UNIQUE );

	const locchar_t *GetFullName() const { return m_loc_LocalizedItemName; }

private:
	locchar_t m_loc_LocalizedItemName[ MAX_ITEM_NAME_LENGTH ];
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEconItemLocalizedMarketNameGenerator
{
public:
	CEconItemLocalizedMarketNameGenerator( const CLocalizationProvider *pLocalizationProvider, IEconItemInterface *pItem, bool bUseingHashContext = true );

	const locchar_t *GetFullName() const { return m_loc_LocalizedItemName; }

private:
	locchar_t m_loc_LocalizedItemName[ MAX_ITEM_NAME_LENGTH ];
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSteamAccountIDAttributeCollector : public CEconItemSpecificAttributeIterator
{
public:
	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, attrib_value_t value ) OVERRIDE
	{
		if ( pAttrDef->GetDescriptionFormat() == ATTDESCFORM_VALUE_IS_ACCOUNT_ID )
		{
			m_vecSteamAccountIDs.AddToTail( value );
		}

		return true;
	}

	// Data access.
	const CUtlVector<uint32>& GetAccountIDs()
	{
		return m_vecSteamAccountIDs;
	}

private:
	CUtlVector<uint32> m_vecSteamAccountIDs;
};

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------

#endif // BUILD_ITEM_NAME_AND_DESC

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
struct CLocalizedRTime32
{
	RTime32 m_unTime;
	bool m_bForceGMTOnClient;								// display this time in GMT on the client? by default, clients show local time; the GC will ignore this flag and always display GMT
	const CLocalizationProvider *m_pLocalizationProvider;
	TF_ANTI_IDLEBOT_VERIFICATION_ONLY_ARG( MD5Context_t *m_pHashContext; )
};

template < >
class CLocalizedStringArg<CLocalizedRTime32>
{
public:
	enum { kIsValid = true };

	CLocalizedStringArg( const CLocalizedRTime32& cTimeIn );

	const locchar_t *GetLocArg() const { return m_Str.Get(); }

private:
	CUtlConstStringBase<locchar_t> m_Str;
};

#endif // ECONITEMDESCRIPTION_H
