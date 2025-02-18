//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: EconItemSchema: Defines a schema for econ items
//
//=============================================================================

#ifndef ECONITEMSCHEMA_H
#define ECONITEMSCHEMA_H
#ifdef _WIN32
#pragma once
#endif

// Valve code doesn't play nicely with standard headers on some platforms sometimes.
#ifdef min
	#undef min
#endif

#ifdef max
	#undef max
#endif

#include <string>

#include "KeyValues.h"
#include "tier1/utldict.h"
#include "tier1/utlhashmaplarge.h"
#include "econ_item_constants.h"

#include "item_selection_criteria.h"
#include "bitvec.h"
#include "language.h"
#include "smartptr.h"
#include "rtime.h"
#include "checksum_sha1.h"

#if defined(CLIENT_DLL) || defined(GAME_DLL)
#include "engine/ivmodelinfo.h"
#include "engine/ivmodelrender.h"
#include "ilocalize.h"
#endif
#include "gamestringpool.h"

class CEconItemSchema;
class CEconItem;
class CEconSharedObjectCache;
class CSOItemRecipe;
class CQuestLootlist;
class CQuestObjectiveDefinition;

union attribute_data_union_t
{
	float asFloat;
	uint32 asUint32;
	byte *asBlobPointer;
};

struct static_attrib_t
{
	static_attrib_t()
	{
		iDefIndex = 0;
		m_value.asBlobPointer = NULL;
	}

	~static_attrib_t()
	{
	}

	static_attrib_t( const static_attrib_t& rhs )
	{
		iDefIndex = rhs.iDefIndex;
		m_value = rhs.m_value;
	}

	attrib_definition_index_t	iDefIndex;
	attribute_data_union_t m_value;

	// Parses a single subsection from a multi-line attribute block that looks like:
	//
	//		"attributes"
	//		{
	//			"cannot trade"
	//			{
	//				"attribute_class"	"cannot_trade"
	//				"value"				"1"
	//			}
	//			"kill eater"
	//			{
	//				"attribute_class"	"kill_eater"
	//				"force_gc_to_generate" "1"
	//				"use_custom_logic"	"gifts_given_out"
	//			}
	//		}
	//
	// The "force_gc_to_generate" and "use_custom_logic" fields will only be parsed on the GC. Will return
	// true/false based on whether the whole attribute and value parsed successfully.
	bool BInitFromKV_MultiLine( const char *pszContext, KeyValues *pKVAttribute, CUtlVector<CUtlString> *pVecErrors );

	// Parses a single subsection from a single-line attribute block that looks like:
	//
	//		CharacterAttributes 
	//		{
	//			"increase buff duration"	9.0
	//			"damage bonus"	2.0 
	//		}
	//
	// It's impossible to specify GC-generated attributes in this format. Will return true/false based on
	// whether the whole attribute and value parsed successfully.
	bool BInitFromKV_SingleLine( const char *pszContext, KeyValues *pKVAttribute, CUtlVector<CUtlString> *pVecErrors, bool bEnableTerribleBackwardsCompatibilitySchemaParsingCode = true );

	// Data access helpers.
	const class CEconItemAttributeDefinition *GetAttributeDefinition() const;
	const class ISchemaAttributeType *GetAttributeType() const;
};

typedef	uint16	equipped_class_t;
typedef uint16	equipped_slot_t;
typedef uint8	equipped_preset_t;

#define INVALID_EQUIPPED_SLOT	((equipped_slot_t)-1)
#define INVALID_STYLE_INDEX		((style_index_t)-1)
#define INVALID_PRESET_INDEX	((equipped_preset_t)-1)

enum EEquipType_t
{
	EQUIP_TYPE_CLASS = 0,
	EQUIP_TYPE_ACCOUNT,

	EQUIP_TYPE_INVALID,
};

// Streamable weapons cause stutters when people enter PVS. Turn it off for now.
// #define WITH_STREAMABLE_WEAPONS

//-----------------------------------------------------------------------------
// IEconItemPropertyGenerator
//-----------------------------------------------------------------------------
class IEconItemPropertyGenerator
{
public:
	virtual ~IEconItemPropertyGenerator() { }

	MUST_CHECK_RETURN virtual bool BGenerateProperties( CEconItem *pItem ) const = 0;
};

//-----------------------------------------------------------------------------
// Item Series
//-----------------------------------------------------------------------------
class CEconItemSeriesDefinition
{
public:
	CEconItemSeriesDefinition( void );
	CEconItemSeriesDefinition( const CEconItemSeriesDefinition &that );
	CEconItemSeriesDefinition &operator=( const CEconItemSeriesDefinition& rhs );

	~CEconItemSeriesDefinition( void ) { }

	bool		BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors = NULL );

	int32		GetDBValue( void ) const			{ return m_nValue; }
	const char	*GetName( void ) const				{ return !m_strName.IsEmpty() ? m_strName.String() : "unknown"; }
	const char	*GetLocKey( void ) const			{ return !m_strLockKey.IsEmpty() ? m_strLockKey.String() : "unknown"; }
	const char	*GetUiFile( void ) const			{ return !m_strUiFile.IsEmpty() ? m_strUiFile.String() : "unknown"; }

private:

	// The value that the game/DB will know this series by
	int32		m_nValue;

	CUtlString	m_strName;			// Key Name
	CUtlString	m_strLockKey;		// Localization key
	CUtlString	m_strUiFile;		// Ui File (.res file)
};
//-----------------------------------------------------------------------------
// CEconItemRarityDefinition
//-----------------------------------------------------------------------------
class CEconItemRarityDefinition
{
public:
	CEconItemRarityDefinition( void );
	
	~CEconItemRarityDefinition( void ) { }

	bool		BInitFromKV( KeyValues *pKVItem, KeyValues *pKVRarityWeights, CEconItemSchema &pschema, CUtlVector<CUtlString> *pVecErrors = NULL );

	int32		GetDBValue( void ) const			{ return m_nValue; }
	const char	*GetName( void ) const				{ return !m_strName.IsEmpty() ? m_strName.String() : "unknown"; }
	const char  *GetLocKey( void ) const			{ return m_strLocKey.String(); }
	const char  *GetWepLocKey( void ) const			{ return m_strWepLocKey.String(); }
	const char  *GetDropSound( void ) const			{ return m_strDropSound.String(); }
	attrib_colors_t		GetAttribColor( void ) const		{ return m_iAttribColor; }
	const char	*GetNextRarity( void ) const		{ return m_strNextRarity.String(); }
	int32		GetLootlistWeight( void ) const		{ return m_nLootlistWeight; }

private:

	// The value that the game/DB will know this rarity by
	int32		m_nValue;

	attrib_colors_t		m_iAttribColor;

	// The English name of the rarity
	CUtlString	m_strName;

	// The localization key for this rarity.
	CUtlString  m_strLocKey;
	// The localization key for this rarity, for weapons.
	CUtlString  m_strWepLocKey;

	// The loot list name associated with this rarity.
	CUtlString  m_strDropSound;

	CUtlString  m_strNextRarity;

	int32		m_nLootlistWeight;

};

//-----------------------------------------------------------------------------
// CEconItemQualityDefinition
// Template Definition of a randomly created item
//-----------------------------------------------------------------------------
class CEconItemQualityDefinition
{
public:
	CEconItemQualityDefinition( void );
	CEconItemQualityDefinition( const CEconItemQualityDefinition &that );
	CEconItemQualityDefinition &operator=( const CEconItemQualityDefinition& rhs );

	~CEconItemQualityDefinition( void ) { }

	bool		BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors = NULL );


	int32		GetDBValue( void ) const			{ return m_nValue; }
	const char	*GetName( void ) const				{ return !m_strName.IsEmpty() ? m_strName.Get() : "unknown"; }
	bool		CanSupportSet( void ) const			{ return m_bCanSupportSet; }
	const char	*GetHexColor( void ) const			{ return !m_strHexColor.IsEmpty() ? m_strHexColor.Get() : "B2B2B2"; }

#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, const char *pchName )
	{
		VALIDATE_SCOPE();
		ValidateObj( m_strName );
	}
#endif // DBGFLAG_VALIDATE

private:

	// The value that the game/DB will know this quality by
	int32			m_nValue;

	// The English name of the quality
	CUtlConstString	m_strName;

	// if this is true the support tool is allowed to set this quality level on any item
	bool			m_bCanSupportSet;

	// A hex string representing the color this quality should display as. Used primarily for display on the Web.
	CUtlConstString	m_strHexColor;
};

//-----------------------------------------------------------------------------
// CEconColorDefinition
//-----------------------------------------------------------------------------
class CEconColorDefinition
{
public:
	bool		BInitFromKV( KeyValues *pKVColor, CUtlVector<CUtlString> *pVecErrors = NULL );

	const char *GetName( void ) const			{ return m_strName.Get(); }
	const char *GetColorName( void ) const		{ return m_strColorName.Get(); }		// meant for passing into VGUI styles, etc.
	const char *GetHexColor( void ) const		{ return m_strHexColor.Get(); }

private:
	// The English name of this color. Only used for lookup.
	CUtlConstString m_strName;

	// The VGUI name of the color in our schema. This will be used to set values
	// for VGUI controls.
	CUtlConstString m_strColorName;

	// The hex string value of this color. This will be used for Web display.
	CUtlConstString m_strHexColor;
};

//-----------------------------------------------------------------------------
// CEconItemSetDefinition
// Definition of an item set
//-----------------------------------------------------------------------------
class CEconItemSetDefinition
{
public:
	CEconItemSetDefinition( void );
	CEconItemSetDefinition( const CEconItemSetDefinition &that );
	CEconItemSetDefinition &operator=( const CEconItemSetDefinition& rhs );

	~CEconItemSetDefinition( void ) {}

	bool	BInitFromKV( KeyValues *pKVItemSet, CUtlVector<CUtlString> *pVecErrors = NULL );

	void	IterateAttributes( class IEconItemAttributeIterator *pIterator ) const;

public:

	CUtlString							    m_strName;
	const char							   *m_pszLocalizedName;
	CUtlVector<item_definition_index_t>		m_iItemDefs;
	int										m_iBundleItemDef;	// Item def of the store bundle for this set, if any
	bool									m_bIsHiddenSet;		// If true, this set and any bonuses will only be visible if the whole set is equipped.

	struct itemset_attrib_t
	{
		attrib_definition_index_t		m_iAttribDefIndex;
		float							m_flValue;
	};
	CUtlVector<itemset_attrib_t>	m_iAttributes;
};

//-----------------------------------------------------------------------------
class CEconItemCollectionDefinition
{
public:
	CEconItemCollectionDefinition( void );
	~CEconItemCollectionDefinition( void ) {}

	bool	BInitFromKV( KeyValues *pKVItemCollection, CUtlVector<CUtlString> *pVecErrors = NULL );
	bool	BPostSchemaInit( CUtlVector<CUtlString> *pVecErrors );

	uint8	GetMinRarity() const { return m_iRarityMin; }
	uint8	GetMaxRarity() const { return m_iRarityMax; }

public:
	CUtlString							    m_strName;
	const char							   *m_pszLocalizedName;
	const char							   *m_pszLocalizedDesc;
	CUtlVector<item_definition_index_t>		m_iItemDefs;

private:
	bool	m_bIsReferenceCollection;

	uint8	m_iRarityMin;
	uint8	m_iRarityMax;
};

//-----------------------------------------------------------------------------
class CEconOperationDefinition
{
public:
	CEconOperationDefinition( void );
	~CEconOperationDefinition( void );

	bool	BInitFromKV( KeyValues *pKVOperation, CUtlVector<CUtlString> *pVecErrors = NULL );

	const char *GetName() const { return m_pszName; }
	operation_definition_index_t GetOperationID() const { return m_unOperationID; }
	item_definition_index_t GetRequiredItemDefIndex() const { return m_unRequiredItemDefIndex; }
	item_definition_index_t GetGatewayItemDefIndex() const { return m_unGatewayItemDefIndex; }

	KeyValues *GetKVP() { return m_pKVItem; }

	// use the date that we stop giving things to players as expiry date
	bool	IsExpired() const { return CRTime::RTime32TimeCur() > GetStopGivingToPlayerDate(); }
	bool	IsActive() const { return CRTime::RTime32TimeCur() >= GetStartDate() && !IsExpired(); }

	const char *GetQuestLogOverrideResFile() const { return m_pszQuestLogResFile; }
	const char *GetQuestListOverrideResFile() const { return m_pszQuestListResFile; }

	RTime32	GetStartDate() const { return m_OperationStartDate; }
	RTime32 GetStopGivingToPlayerDate() const { return m_StopGivingToPlayerDate; }
	RTime32 GetStopAddingToQueueDate() const { return m_StopAddingToQueueDate; }
	RTime32 GetStopContractsDate() const { return m_ContractProgressEndDate; }

	const char *GetOperationLootlist() const { return m_pszOperationLootList; }
	bool	IsCampaign() const { return m_bIsCampaign; }
	bool	UsesCredits() const { return m_bUsesCredits; }
	uint32	GetMaxDropCount() const { return m_unMaxDropCount; }

	int32 GetKillEaterEventType_Contracts() const { return m_nKillEaterEventType_Contracts; }
	int32 GetKillEaterEventType_Points() const { return m_nKillEaterEventType_Points; }


private:
	const char			*m_pszName;
	operation_definition_index_t	m_unOperationID;

	// things operation periodically drops
	const char			*m_pszOperationLootList;
	bool				m_bIsCampaign;
	bool				m_bUsesCredits;
	int32				m_nKillEaterEventType_Contracts;
	int32				m_nKillEaterEventType_Points;
	uint32				m_unMaxDropCount;

	const char			*m_pszQuestLogResFile;
	const char			*m_pszQuestListResFile;

	item_definition_index_t m_unRequiredItemDefIndex;
	item_definition_index_t m_unGatewayItemDefIndex; // Defindex of the item users need to acquire in order to get the required item.  Could be the required item itself.

	RTime32				m_OperationStartDate;		// when the operation starts and gives out rewards
	RTime32				m_StopGivingToPlayerDate;	// when the operation stops giving quests to player
	RTime32				m_StopAddingToQueueDate;	// when the operation stops adding more quests to the bucket
	RTime32				m_ContractProgressEndDate;	// When players can no longer accept or work on Contracts associated with this operation


	KeyValues				   *m_pKVItem;
};


//-----------------------------------------------------------------------------
// CEconLootListDefinition
// Definition of a loot list
//-----------------------------------------------------------------------------
class IEconLootList
{
public:
	virtual ~IEconLootList() { }

	MUST_CHECK_RETURN virtual bool BPublicListContents() const = 0;
	MUST_CHECK_RETURN virtual const char *GetLootListHeaderLocalizationKey() const = 0;
	MUST_CHECK_RETURN virtual const char *GetLootListFooterLocalizationKey() const = 0;
	MUST_CHECK_RETURN virtual const char *GetLootListCollectionReference() const = 0;

	class IEconLootListIterator
	{
	public:
		virtual ~IEconLootListIterator() { }
		virtual void OnIterate( item_definition_index_t unItemDefIndex ) = 0;
	};

	virtual void EnumerateUserFacingPotentialDrops( IEconLootListIterator *pIt ) const = 0;

};

struct drop_period_t
{
	bool IsValidForTime( const RTime32& time ) const;

	RTime32		m_DropStartDate;
	RTime32		m_DropEndDate;
};

struct drop_item_t
{
	int m_iItemOrLootlistDef;			// negative values indicate nested loot lists
	float m_flWeight;
	drop_period_t m_dropPeriod;
};

typedef CUtlVector< CItemSelectionCriteria* > ItemSelectionCriteriaVec_t;

struct lootlist_attrib_t
{
	lootlist_attrib_t()
		:	m_pVecCriteria( NULL ),
			m_flWeight( 1.f ),
			m_bAllowDuplicate( false )
	{
	}

	static_attrib_t	m_staticAttrib;
	ItemSelectionCriteriaVec_t *m_pVecCriteria; // this points to the one in random_attrib_t
	float	m_flWeight;
	bool	m_bAllowDuplicate;

	bool BInitFromKV( const char *pszContext, KeyValues *pKVKey, CEconItemSchema &pschema, CUtlVector<CUtlString> *pVecErrors );
	bool BHasAnyCriteria() const { return m_pVecCriteria != NULL; }
	bool BItemPassAllCriteria( const CEconItemDefinition* pItemDef ) const;
};

typedef CUtlVector< lootlist_attrib_t > LootListAttributeVec_t;


struct random_attrib_t
{
	random_attrib_t()
	{
	}

	~random_attrib_t()
	{
	}

	float				m_flTotalAttributeWeight;
	LootListAttributeVec_t m_RandomAttributes;
	ItemSelectionCriteriaVec_t m_vecCriteria;

};

class CEconLootListDefinition;

struct loot_list_additional_drop_t
{

	bool		m_bPremiumOnly;
	const char *m_pszOwnerName;
	const char *m_pszLootListDefName;
	int		    m_iRequiredHolidayIndex;
	drop_period_t m_dropPeriod;
};

class CLootlistJob
{
public:
	CLootlistJob( const char *pszOwnerName );
	~CLootlistJob();
	bool BInitFromKV( const char *pszContext, KeyValues *pKVKey, CEconItemSchema &pschema, CUtlVector<CUtlString> *pVecErrors );
	bool BPostInit( CUtlVector<CUtlString> *pVecErrors );

	struct RandomAttributeInfo_t
	{
		random_attrib_t* m_pRandomAttributes;
		bool m_bFromTemplate;
	};
	const CUtlVector< RandomAttributeInfo_t >& GetAttributes() const { return m_vecAttributes; }
	const CUtlVector<loot_list_additional_drop_t>& GetAdditionalDrops() const { return m_vecAdditionalDrops; }


private:
	bool AddRandomAtrributes( KeyValues *pRandomAttributesKV, CEconItemSchema &pschema, CUtlVector<CUtlString> *pVecErrors = NULL );
	bool AddRandomAttributesFromTemplates( KeyValues *pRandomAttributesKV, CEconItemSchema &pschema, CUtlVector<CUtlString> *pVecErrors = NULL );

	const char *		m_pszOwnerName;
	float				m_flChanceToRunJob;

	CUtlVector< RandomAttributeInfo_t > m_vecAttributes;
	CUtlVector< loot_list_additional_drop_t > m_vecAdditionalDrops;
};

class CEconLootListDefinition : public IEconLootList
{
public:

	virtual ~CEconLootListDefinition();
	
	bool BInitFromKV( KeyValues *pKVLootList, CEconItemSchema &pschema, CUtlVector<CUtlString> *pVecErrors );
	bool BPostInit( CUtlVector<CUtlString> *pVecErrors );

	const char *GetName() const { return m_strName; }
	virtual const char *GetLootListHeaderLocalizationKey() const OVERRIDE { return m_pszLootListHeader; }
	virtual const char *GetLootListFooterLocalizationKey() const OVERRIDE { return m_pszLootListFooter; }
	virtual const char *GetLootListCollectionReference() const OVERRIDE { return m_pszCollectionReference; }
		
	const CUtlVector<drop_item_t>& GetLootListContents() const { return m_DropList; }

	const CUtlVector<CLootlistJob*>& GetLootlistJobs() const { return m_jobs; }

	virtual void EnumerateUserFacingPotentialDrops( IEconLootListIterator *pIt ) const OVERRIDE;

	virtual bool BPublicListContents() const OVERRIDE
	{
		return m_bPublicListContents;
	}


private:

	CUtlString			 m_strName;
	const char			*m_pszLootListHeader;
	const char			*m_pszLootListFooter;
	const char			*m_pszCollectionReference;
	CUtlVector<drop_item_t> m_DropList;

	bool				m_bPublicListContents;	// do not show loot list contents to users (ie., when listing crate contents on Steam)

	bool AddLootlistJob( KeyValues *pLootlistJobKV, CEconItemSchema &pschema, CUtlVector<CUtlString> *pVecErrors = NULL );

	CUtlVector<CLootlistJob*>						m_jobs;

};

struct LootListInfo_t
{
	CUtlVector< random_attrib_t* > m_vecAttributes;
	CUtlVector< item_definition_index_t > m_vecItems;
	CUtlVector< item_definition_index_t > m_vecAdditionalItems;
};
bool GetClientLootListInfo( const CEconLootListDefinition *pLootList, LootListInfo_t &lootListInfo );
bool GetClientLootListInfo( const char *pszLootListName, LootListInfo_t &lootListInfo );
bool GetClientLootListInfo( const IEconItemInterface *pEconItem, LootListInfo_t &lootListInfo );

//-----------------------------------------------------------------------------
// CEconCraftingRecipeDefinition
// Template Definition of an item recipe
//-----------------------------------------------------------------------------
class CEconCraftingRecipeDefinition
{
public:
	CEconCraftingRecipeDefinition( void );
	virtual ~CEconCraftingRecipeDefinition( void ) { }

	bool		BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors = NULL );


	virtual void CopyPolymorphic( const CEconCraftingRecipeDefinition *pSourceDef ) { *this = *pSourceDef; }

	void		SetDefinitionIndex( uint32 iIndex ) { m_nDefIndex = iIndex; }
	int32		GetDefinitionIndex( void ) const	{ return m_nDefIndex; }
	const char	*GetName( void ) const				{ return !m_strName.IsEmpty() ? m_strName.String() : "unknown"; }
	const char	*GetName_A( void ) const				{ return !m_strN_A.IsEmpty() ? m_strN_A.String() : "unknown"; }
	const char	*GetDescInputs( void ) const				{ return !m_strDescInputs.IsEmpty() ? m_strDescInputs.String() : "unknown"; }
	const char	*GetDescOutputs( void ) const				{ return !m_strDescOutputs.IsEmpty() ? m_strDescOutputs.String() : "unknown"; }

	const char	*GetDescI_A( void ) const				{ return !m_strDI_A.IsEmpty() ? m_strDI_A.String() : "unknown"; }
	const char	*GetDescI_B( void ) const				{ return !m_strDI_B.IsEmpty() ? m_strDI_B.String() : "unknown"; }
	const char	*GetDescI_C( void ) const				{ return !m_strDI_C.IsEmpty() ? m_strDI_C.String() : "unknown"; }
	const char	*GetDescO_A( void ) const				{ return !m_strDO_A.IsEmpty() ? m_strDO_A.String() : "unknown"; }
	const char	*GetDescO_B( void ) const				{ return !m_strDO_B.IsEmpty() ? m_strDO_B.String() : "unknown"; }
	const char	*GetDescO_C( void ) const				{ return !m_strDO_C.IsEmpty() ? m_strDO_C.String() : "unknown"; }

	bool		IsDisabled( void ) const { return m_bDisabled; }
	bool		RequiresAllSameClass( void ) { return m_bRequiresAllSameClass; }
	bool		RequiresAllSameSlot( void ) { return m_bRequiresAllSameSlot; }
	bool		IsPremiumAccountOnly( void ) const { return m_bPremiumAccountOnly; }
	recipecategories_t	GetCategory( void ) const { return m_iCategory; }
	int			GetTotalInputItemsRequired( void ) const;
	int			GetTotalOutputItems( void ) const { return m_OutputItemsCriteria.Count(); }

	// Returns true if the vector contains a set of items that matches the inputs for this recipe
	virtual bool ItemListMatchesInputs( CUtlVector<CEconItem*> *vecCraftingItems, KeyValues *out_pCraftParams = NULL, bool bIgnoreSlop = false, CUtlVector<uint64> *vecChosenItems = NULL ) const;

	const CUtlVector<CItemSelectionCriteria> *GetInputItems( void ) const { return &m_InputItemsCriteria; }
	const CUtlVector<uint32>				 &GetInputItemDupeCounts( void ) const { return m_InputItemDupeCounts; }
	const CUtlVector<CItemSelectionCriteria> &GetOutputItems( void ) const { return m_OutputItemsCriteria; }

#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, const char *pchName )
	{
		VALIDATE_SCOPE();
		ValidateObj( m_InputItemsCriteria );
		ValidateObj( m_InputItemDupeCounts );
		ValidateObj( m_OutputItemsCriteria );
	}
#endif // DBGFLAG_VALIDATE

	// Serializes the criteria to and from messages
	bool		BSerializeToMsg( CSOItemRecipe & msg ) const;
	bool		BDeserializeFromMsg( const CSOItemRecipe & msg );

protected:
	// The number used to refer to this definition in the DB
	int32		m_nDefIndex;

	// Localization key strings
	CUtlString	m_strName; 
	CUtlString	m_strN_A; 
	CUtlString	m_strDescInputs; 
	CUtlString	m_strDescOutputs; 
	CUtlString	m_strDI_A;
	CUtlString	m_strDI_B;
	CUtlString	m_strDI_C;
	CUtlString	m_strDO_A;
	CUtlString	m_strDO_B;
	CUtlString	m_strDO_C;

	bool		m_bDisabled;
	bool		m_bRequiresAllSameClass;
	bool		m_bRequiresAllSameSlot;
	int			m_iCacheClassUsageForOutputFromItem;
	int			m_iCacheSlotUsageForOutputFromItem;
	int			m_iCacheSetForOutputFromItem;
	bool		m_bPremiumAccountOnly;
	recipecategories_t	m_iCategory;

	// The list of items that a required to make this recipe
	CUtlVector<CItemSelectionCriteria>	m_InputItemsCriteria;
	CUtlVector<uint32>					m_InputItemDupeCounts;

	// The list of items that are generated by this recipe
	CUtlVector<CItemSelectionCriteria>	m_OutputItemsCriteria;
};

//-----------------------------------------------------------------------------
// Purpose: Attribute definition details
//-----------------------------------------------------------------------------
enum
{
	ATTDESCFORM_VALUE_IS_PERCENTAGE,			// Printed as:	((m_flValue*100)-100.0)
	ATTDESCFORM_VALUE_IS_INVERTED_PERCENTAGE,	// Printed as:	((m_flValue*100)-100.0) if it's > 1.0, or ((1.0-m_flModifier)*100) if it's < 1.0
	ATTDESCFORM_VALUE_IS_ADDITIVE,				// Printed as:	m_flValue
	ATTDESCFORM_VALUE_IS_ADDITIVE_PERCENTAGE,	// Printed as:	(m_flValue*100)
	ATTDESCFORM_VALUE_IS_OR,					// Printed as:  m_flValue, but results are ORd together instead of added
	ATTDESCFORM_VALUE_IS_DATE,					// Printed as a date
	ATTDESCFORM_VALUE_IS_ACCOUNT_ID,			// Printed as steam user name
	ATTDESCFORM_VALUE_IS_PARTICLE_INDEX,		// Printed as a particle description
	ATTDESCFORM_VALUE_IS_KILLSTREAKEFFECT_INDEX,// Printed as killstreak effect description
	ATTDESCFORM_VALUE_IS_KILLSTREAK_IDLEEFFECT_INDEX,  // Printed as idle effect description
	ATTDESCFORM_VALUE_IS_ITEM_DEF,				// Printed as item name
	ATTDESCFORM_VALUE_IS_FROM_LOOKUP_TABLE,		// Printed as a string from a lookup table, specified by the attribute definition name
};

// Coloring for attribute lines
enum attrib_effect_types_t
{
	ATTRIB_EFFECT_UNUSUAL = 0,
	ATTRIB_EFFECT_STRANGE,
	ATTRIB_EFFECT_NEUTRAL,
	ATTRIB_EFFECT_POSITIVE,
	ATTRIB_EFFECT_NEGATIVE,
	
	NUM_EFFECT_TYPES,
};

enum EAssetClassAttrExportRule_t
{
	k_EAssetClassAttrExportRule_Default = 0,
	k_EAssetClassAttrExportRule_Bucketed = ( 1 << 0 ),	// attribute exports bucketed value to Steam Community
	k_EAssetClassAttrExportRule_Skip = ( 1 << 1 ),	// attribute value is not exported to Steam Community
	k_EAssetClassAttrExportRule_GCOnly = ( 1 << 2 ),	// attribute only lives on GC and not exported to any external request
};

//-----------------------------------------------------------------------------
// CEconItemAttributeDefinition
// Template definition of a randomly created attribute
//-----------------------------------------------------------------------------
class CEconItemAttributeDefinition
{
public:
	CEconItemAttributeDefinition( void );
	CEconItemAttributeDefinition( const CEconItemAttributeDefinition &that );
	CEconItemAttributeDefinition &operator=( const CEconItemAttributeDefinition& rhs );

	~CEconItemAttributeDefinition( void );

	bool	BInitFromKV( KeyValues *pKVAttribute, CUtlVector<CUtlString> *pVecErrors = NULL );

	attrib_definition_index_t GetDefinitionIndex( void ) const	{ return m_nDefIndex; }
	// Attribute name referenced in the db.
	const char	*GetDefinitionName( void ) const	{ return m_pszDefinitionName; }
	
	KeyValues	*GetRawDefinition( void ) const		{ return m_pKVAttribute; }

	// Data accessing
	bool		IsHidden( void ) const						{ return m_bHidden; }
	bool		BForceWebSchemaOutput( void ) const			{ return m_bWebSchemaOutputForced; }
	bool		BIsSetBonusAttribute( void ) const			{ return m_bIsSetBonus; }
	bool		CanAffectMarketName( void ) const			{ return m_bCanAffectMarketName; }
	bool		CanAffectRecipeComponentName( void ) const	{ return m_bCanAffectRecipeComponentName; }
	bool		IsStoredAsInteger( void ) const				{ return m_bStoredAsInteger; }
	bool		IsStoredAsFloat( void ) const				{ return !m_bStoredAsInteger; }
	int			GetUserGenerationType( void ) const			{ return m_iUserGenerationType; }
	bool		IsInstanceData() const						{ return m_bInstanceData; }
	EAssetClassAttrExportRule_t GetAssetClassAttrExportRule() const			{ return m_eAssetClassAttrExportRule; }
	uint32		GetAssetClassBucket() const			{ return m_unAssetClassBucket; }
	int			GetDescriptionFormat( void ) const			{ return m_iDescriptionFormat; }
	const char *GetDescriptionString( void ) const			{ return m_pszDescriptionString; }
	const char *GetArmoryDescString( void ) const			{ return m_pszArmoryDesc; }
	const char *GetAttributeClass( void ) const				{ return m_pszAttributeClass; }
	econ_tag_handle_t GetItemDefinitionTag( void ) const	{ return m_ItemDefinitionTag; }
	attrib_effect_types_t GetEffectType( void ) const		{ return m_iEffectType; }

	const class ISchemaAttributeType *GetAttributeType( void ) const { return m_pAttrType; }

	void		ClearStringCache( void ) const		{ m_iszAttributeClass = NULL_STRING; }
	string_t	GetCachedClass( void ) const
	{
		if ( m_iszAttributeClass == NULL_STRING && m_pszAttributeClass )
		{
			m_iszAttributeClass = AllocPooledString( m_pszAttributeClass );
		}
		return m_iszAttributeClass;
	}

#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, const char *pchName )
	{
		VALIDATE_SCOPE();
		ValidatePtr( m_pKVAttribute );
	}
#endif // DBGFLAG_VALIDATE

private:
	// The raw keyvalues for this attribute definition.
	KeyValues	*m_pKVAttribute;

	// Required valued from m_pKVAttribute:

	// The number used to refer to this definition in the DB
	attrib_definition_index_t	m_nDefIndex;

	// A pointer to the schema-global type data for this attribute. This maps attribute types to functionality
	// for loading/storing, both to memory and the DB.
	const class ISchemaAttributeType *m_pAttrType;

	// ---------------------------------------------
	// Display related data
	// ---------------------------------------------
	// If true, this attribute isn't shown in the item description
	bool		m_bHidden;

	// If true, this attribute's description is always output in web api calls regardless of the hidden flag.
	bool		m_bWebSchemaOutputForced;

	// Whether or not the value is stored as an integer in the DB.
	bool		m_bStoredAsInteger;

	// If this is true the attribute is counted as "instance" data for purposes of asset class in the Steam Economy. Non-instance
	// properties are considered things that can differentiate items at a fundamental level (ie., definition index, quality); instance
	// properties are more things like additional customizations -- score for strange items, paint color, etc.
	bool		m_bInstanceData;
	EAssetClassAttrExportRule_t	m_eAssetClassAttrExportRule;			// if this is true the attribute will not be exported for asset class
	uint32		m_unAssetClassBucket;									// if this is set then attribute value is bucketed when exported for asset class

	// Set item bonus attributes use a different attribute parser and make assumptions about memory layout. We
	// don't really use these for any new content currently and it isn't worth touching all the old code.
	//
	// At runtime, this flag is used to determine whether or not to rebuild dynamic attributes attached to
	// players on respawn.
	bool		m_bIsSetBonus;

	// Whether or not this attribute is supposed to only come from user actions. These attributes are used for
	// player item upgrades, etc. and cannot be set on items directly in the schema.
	int			m_iUserGenerationType;

	// Overall positive/negative effect. Used to color the attribute.
	attrib_effect_types_t m_iEffectType;

	// Contains the description format & string for this attribute
	int			m_iDescriptionFormat;
	const char	*m_pszDescriptionString;

	// Contains information on how to describe items with this attribute in the Armory
	const char	*m_pszArmoryDesc;

	// Used to allow unique items to specify attributes by name.
	const char	*m_pszDefinitionName;

	// The class name of this attribute. Used in creation, and to hook the attribute into the actual code that uses it.
	const char	*m_pszAttributeClass;

	// Allowed to affect the market bucketization name.  We dont want things like the strange level to affect the name,
	// but we do want things like crate series number and strangifier targets to get their own buckets.
	bool		m_bCanAffectMarketName;

	// Allowed to list itself in the name of an item in the recipe component description.
	bool		m_bCanAffectRecipeComponentName;

	// Do item definitions with this attribute specified automatically get an additional tag applied?
	econ_tag_handle_t	m_ItemDefinitionTag;

	mutable string_t	m_iszAttributeClass;	// Same as the above, but used for fast lookup when applying attributes.
};


//-----------------------------------------------------------------------------
// Visual data storage in item definitions
//-----------------------------------------------------------------------------
#define TEAM_VISUAL_SECTIONS			5
#define MAX_VISUALS_CUSTOM_SOUNDS		10

struct attachedparticlesystem_t
{
	attachedparticlesystem_t() :
		pszSystemName( NULL )
		, bFollowRootBone( NULL )
		, iCustomType( 0 )
		, nSystemID( 0 )
		, fRefireTime( 0 )			// only works for taunt effects, currently
		, bDrawInViewModel( false )
		, bUseSuffixName( false )
	{
		V_memset( pszControlPoints, 0, sizeof( pszControlPoints ) );
	}

	const char *pszSystemName;
	bool		bFollowRootBone;
	int			iCustomType;
	int			nSystemID;
	float		fRefireTime;				// only works for taunt effects, currently
	bool		bDrawInViewModel;
	bool		bUseSuffixName;

	const char *pszControlPoints[7];
};


#if defined(CLIENT_DLL) || defined(GAME_DLL)
enum
{
	kAttachedModelDisplayFlag_WorldModel = 0x01,
	kAttachedModelDisplayFlag_ViewModel	 = 0x02,

	kAttachedModelDisplayFlag_MaskAll	 = kAttachedModelDisplayFlag_WorldModel | kAttachedModelDisplayFlag_ViewModel,
};

struct attachedmodel_t
{
	const char *m_pszModelName;
	int m_iModelDisplayFlags;
};

enum wearableanimplayback_t
{
	WAP_ON_SPAWN,				// Play this animation immediately on spawning the wearable
	WAP_START_BUILDING,			// Game code will start this anim whenever a player wearing this item deploys their builder weapon.
	WAP_STOP_BUILDING,			// Game code will start this anim whenever a player wearing this item holsters their builder weapon.
	WAP_START_TAUNTING,			// Game code will start this anim whenever a player wearing this item taunts
	WAP_STOP_TAUNTING,				// Game code will start this anim whenever a player wearing this item stops taunting

	NUM_WAP_TYPES,
};

struct animation_on_wearable_t
{
	int						iActivity;
	const char				*pszActivity;
	const char				*pszReplacement;
	int						iReplacement; // Replacement activity to play. Might be set to one of kActivityLookup_Unknown/kActivityLookup_Missing.
	const char				*pszSequence;
	const char				*pszRequiredItem;
	const char				*pszScene;
};

struct activity_on_wearable_t
{
	wearableanimplayback_t	iPlayback;
	int						iActivity;
	const char				*pszActivity;
};

struct codecontrolledbodygroupdata_t
{
	const char *pFuncName;
	void *pFunc;
};

// This is a workaround because Source practice is to disable operator=() for CUtlMap.
struct perteamvisuals_maps_t
{
	perteamvisuals_maps_t()
		: m_ModifiedBodyGroupNames( k_eDictCompareTypeCaseSensitive )
		, m_CodeControlledBodyGroupNames( k_eDictCompareTypeCaseSensitive )
	{}

	void operator=( const perteamvisuals_maps_t& other )
	{
		FOR_EACH_DICT_FAST( other.m_ModifiedBodyGroupNames, i )
		{
			m_ModifiedBodyGroupNames.Insert( other.m_ModifiedBodyGroupNames.GetElementName(i), other.m_ModifiedBodyGroupNames[i] );
		}
		FOR_EACH_DICT_FAST( other.m_CodeControlledBodyGroupNames, i )
		{
			m_CodeControlledBodyGroupNames.Insert( other.m_CodeControlledBodyGroupNames.GetElementName(i), other.m_CodeControlledBodyGroupNames[i] );
		}
	}

	CUtlDict<int> m_ModifiedBodyGroupNames; // Better method: hide multiple body groups by name.
	CUtlDict<codecontrolledbodygroupdata_t> m_CodeControlledBodyGroupNames;
};

struct poseparamtable_t
{
	CUtlString strName;
	float      flValue;
};

#endif // defined(CLIENT_DLL) || defined(GAME_DLL)
class CEconStyleInfo
{
public:
	CEconStyleInfo()
	{
		for ( int i = 0; i < TEAM_VISUAL_SECTIONS; i++ )
		{
			m_iSkins[i] = 0;
			m_iViewmodelSkins[i] = -1;
		}

		m_pszName = NULL;
		m_pszBasePlayerModel = NULL;
		m_bIsSelectable = true;
		m_bUseSmokeParticleEffect = true;
		m_pszInventoryImage = NULL;

		m_pszBodygroupName = NULL;
		m_iBodygroupSubmodelIndex = -1;

		m_sIconURLSmall = "";
		m_sIconURLLarge = "";
	}

	virtual ~CEconStyleInfo()
	{
		//
	}

	virtual void BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors );

#if defined(CLIENT_DLL) || defined(GAME_DLL)
	virtual void GeneratePrecacheModelStringsForStyle( CUtlVector<const char *> *out_pVecModelStrings ) const;
#endif

	int GetSkin( int iTeam, bool bViewmodel ) const
	{
		Assert( iTeam >= 0 ); 
		Assert( iTeam < TEAM_VISUAL_SECTIONS );

		if ( bViewmodel && m_iViewmodelSkins[ iTeam ] != -1 )
		{
			return m_iViewmodelSkins[ iTeam ];
		}
		
		return m_iSkins[iTeam];
	}

	const char *GetName() const { return m_pszName; }
	const char *GetBasePlayerDisplayModel() const { return m_pszBasePlayerModel; }
	const CUtlVector<CUtlString>& GetAdditionalHideBodygroups() const { return m_vecAdditionalHideBodygroups; }
	bool IsSelectable() const { return m_bIsSelectable; }
	bool UseSmokeParticleEffect() const { return m_bUseSmokeParticleEffect; }
	const char *GetInventoryImage() const { return m_pszInventoryImage; }

	const char *GetBodygroupName() const { return m_pszBodygroupName; }
	int GetBodygroupSubmodelIndex() const { return m_iBodygroupSubmodelIndex; }

	const char  *GetIconURLSmall() const			{ return m_sIconURLSmall; }
	const char  *GetIconURLLarge() const			{ return m_sIconURLLarge; }
	void	SetIconURLSmall( const char *szURL )	{ m_sIconURLSmall = szURL; }
	void	SetIconURLLarge( const char *szURL )	{ m_sIconURLLarge = szURL; }

protected:
	int m_iSkins[TEAM_VISUAL_SECTIONS];
	int m_iViewmodelSkins[TEAM_VISUAL_SECTIONS];
	const char *m_pszName;
	const char *m_pszBasePlayerModel;
	bool m_bIsSelectable;
	const char *m_pszInventoryImage;
	bool m_bUseSmokeParticleEffect;

	const char *m_pszBodygroupName;
	int m_iBodygroupSubmodelIndex;

	CUtlVector<CUtlString> m_vecAdditionalHideBodygroups;

private:

	CUtlString		m_sIconURLSmall;
	CUtlString		m_sIconURLLarge;
};

struct perteamvisuals_t
{
	perteamvisuals_t()
	{
#if defined(CLIENT_DLL) || defined(GAME_DLL)
		iHideParentBodyGroup = -1;

		iSkin = -1;
		bUsePerClassBodygroups = false;
		pszMaterialOverride = NULL;
		pszMuzzleFlash = NULL;
		pszTracerEffect = NULL;
		pszParticleEffect = NULL;
		for ( int i = 0; i < MAX_VISUALS_CUSTOM_SOUNDS; i++ )
		{
			pszCustomSounds[i] = NULL;
		}

		for ( int i = 0; i < NUM_SHOOT_SOUND_TYPES; i++ )
		{
			pszWeaponSoundReplacements[i] = NULL;
		}

		m_iViewModelBodyGroupOverride = -1;
		m_iViewModelBodyGroupStateOverride = -1;
		m_iWorldModelBodyGroupOverride = -1;
		m_iWorldModelBodyGroupStateOverride = -1;

#endif // defined(CLIENT_DLL) || defined(GAME_DLL)
	}

	~perteamvisuals_t()
	{
		m_Styles.PurgeAndDeleteElements();
	}

#if defined(CLIENT_DLL) || defined(GAME_DLL)	
	int iHideParentBodyGroup;

	// Properties necessary for the game client/server but not for the GC.
	perteamvisuals_maps_t m_Maps;
	int			iSkin;
	bool		bUsePerClassBodygroups;
	CUtlVector<attachedmodel_t>	m_AttachedModels;
	CUtlVector<attachedmodel_t>	m_AttachedModelsFestive;	// Attr controlled Festive Attachments
	CUtlVector<attachedparticlesystem_t> m_AttachedParticles;
	CUtlVector<animation_on_wearable_t> m_Animations;
	CUtlVector<activity_on_wearable_t> m_Activities;
	CUtlVector<poseparamtable_t> m_PlayerPoseParams;
	CUtlVector<poseparamtable_t> m_ItemPoseParams;
	const char *pszCustomSounds[MAX_VISUALS_CUSTOM_SOUNDS];
	const char *pszMaterialOverride;
	const char *pszMuzzleFlash;
	const char *pszTracerEffect;
	const char *pszParticleEffect;
	const char *pszWeaponSoundReplacements[NUM_SHOOT_SOUND_TYPES];
	int m_iViewModelBodyGroupOverride;
	int m_iViewModelBodyGroupStateOverride;
	int m_iWorldModelBodyGroupOverride;
	int m_iWorldModelBodyGroupStateOverride;
#endif // defined(CLIENT_DLL) || defined(GAME_DLL)

	// The GC does care about styles.
	CUtlVector<CEconStyleInfo *> m_Styles;
};

enum item_capabilities_t
{
	ITEM_CAP_NONE					= 0,
	ITEM_CAP_PAINTABLE				= 1 << 0,
	ITEM_CAP_NAMEABLE				= 1 << 1,
	ITEM_CAP_DECODABLE				= 1 << 2,
	ITEM_CAP_CAN_BE_CRAFTED_IF_PURCHASED = 1 << 3,		// was ITEM_CAP_CAN_MOD_SOCKET
	ITEM_CAP_CAN_CUSTOMIZE_TEXTURE	= 1 << 4,
	ITEM_CAP_USABLE					= 1 << 5,
	ITEM_CAP_USABLE_GC				= 1 << 6,
	ITEM_CAP_CAN_GIFT_WRAP			= 1 << 7,
	ITEM_CAP_USABLE_OUT_OF_GAME		= 1 << 8,
	ITEM_CAP_CAN_COLLECT			= 1 << 9,
	ITEM_CAP_CAN_CRAFT_COUNT		= 1 << 10,
	ITEM_CAP_CAN_CRAFT_MARK			= 1 << 11,
	ITEM_CAP_PAINTABLE_TEAM_COLORS	= 1 << 12,
	ITEM_CAP_CAN_BE_RESTORED		= 1 << 13,		// can users remove properties (paint, nametag, etc.) from this item via the in-game UI?
	ITEM_CAP_CAN_USE_STRANGE_PARTS	= 1 << 14,
	ITEM_CAP_CAN_CARD_UPGRADE		= 1 << 15,
	ITEM_CAP_CAN_STRANGIFY			= 1 << 16,
	ITEM_CAP_CAN_KILLSTREAKIFY		= 1 << 17,
	ITEM_CAP_CAN_CONSUME			= 1 << 18,
	ITEM_CAP_CAN_SPELLBOOK_PAGE		= 1 << 19,		// IT'S A VERB OKAY
	ITEM_CAP_HAS_SLOTS				= 1 << 20,
	ITEM_CAP_DUCK_UPGRADABLE		= 1 << 21,
	ITEM_CAP_CAN_UNUSUALIFY			= 1 << 22,
	NUM_ITEM_CAPS					= 23,
};

enum { ITEM_CAP_DEFAULT		 = ITEM_CAP_CAN_CRAFT_MARK | ITEM_CAP_CAN_BE_RESTORED | ITEM_CAP_CAN_USE_STRANGE_PARTS | ITEM_CAP_CAN_CARD_UPGRADE | ITEM_CAP_CAN_STRANGIFY | ITEM_CAP_CAN_KILLSTREAKIFY | ITEM_CAP_CAN_CONSUME | ITEM_CAP_CAN_GIFT_WRAP };	// what are the default capabilities on an item?
enum { ITEM_CAP_TOOL_DEFAULT = ITEM_CAP_NONE };																										// what are the default capabilities of a tool?

struct bundleinfo_t
{
	CUtlVector<CEconItemDefinition *> vecItemDefs;
};


#ifdef CLIENT_DLL
namespace vgui
{
	class Panel;
}
#endif // CLIENT_DLL

class IEconTool
{
	friend class CEconSharedToolSupport;

public:
	IEconTool( const char *pszTypeName, const char *pszUseString, const char *pszUsageRestriction, item_capabilities_t unCapabilities )
		: m_pszTypeName( pszTypeName )
		, m_pszUseString( pszUseString )
		, m_pszUsageRestriction( pszUsageRestriction )
		, m_unCapabilities( unCapabilities )
	{
		//
	}

	virtual ~IEconTool() { }

	// Shared code.
	const char *GetUsageRestriction() const { return m_pszUsageRestriction; }
	item_capabilities_t GetCapabilities() const { return m_unCapabilities; }

	virtual bool CanApplyTo( const IEconItemInterface *pTool, const IEconItemInterface *pToolSubject ) const { Assert( pTool ); Assert( pToolSubject ); return true; }
	virtual bool ShouldDisplayQuantity( const IEconItemInterface *pTool ) const;
	virtual bool RequiresToolEscrowPeriod() const { return false; }

	// We don't support throwing exceptions from tool construction so this is intended to be checked afterwards
	// whenever a new tool is created. (See CreateEconToolImpl().)
	virtual bool BFinishInitialization() { return true; }
	
	// Used by the GC only for WebAPI responses and for some weird internal code.
	const char *GetTypeName() const { return m_pszTypeName; }		// would like to disable on the client so we aren't tempted to check against it, but used for building a unique tool list
	const char *GetUseString() const { return m_pszUseString; }

#ifdef CLIENT_DLL
	virtual bool CanBeUsedNow( const IEconItemInterface *pItem ) const { return true; }
	virtual bool ShouldShowContainedItemPanel( const IEconItemInterface *pItem ) const { Assert( !"IEconTool::ShouldShowContainedItemPanel(): we don't expect this to be called on anything besides gifts!" ); return false; }
	virtual bool ShouldDisplayAsUseableOnItemsInArmory() const { return true; }
	virtual const char *GetUseCommandLocalizationToken( const IEconItemInterface *pItem, int i = 0 ) const;
	virtual int GetUseCommandCount( const IEconItemInterface *pItem ) const { return 1; }
	virtual const char* GetUseCommand( const IEconItemInterface *pItem, int i = 0 ) const;


	// Client "do something" interface. At least one of these functions must be implemented or your tool
	// won't do anything on the client. Some tools (ie., collections) will implement both because they
	// have one application behavior and one client-UI behavior.

	// When the client attempts to use a consumable item of any kind, this function will be called. This
	// is called from the UI in response to things like using dueling pistols, using a noisemaker, etc.
	// Usually this opens up some UI, sends off a GC message, etc.
	//
	// There is a "default" implementation of this function in ClientConsumableTool_Generic() that can
	// be called if specific behavior isn't needed.
	virtual void OnClientUseConsumable( class C_EconItemView *pItem, vgui::Panel *pParent ) const
	{
		Assert( !"IEconTool::OnClientUseConsumable(): unimplemented call!" );
	}

	// When the client attempts to apply a tool to a specific other item in their inventory, this function
	// will be called. This is called from the UI is response to things like putting paint on an item,
	// using a key to unlock a crate, etc.
	virtual void OnClientApplyTool( class C_EconItemView *pTool, class C_EconItemView *pSubject, vgui::Panel *pParent ) const
	{
		Assert( !"IEconTool::OnClientApplyTool(): unimplemented call!" );
	}
#endif // CLIENT_DLL


private:
	const char *m_pszTypeName;
	const char *m_pszUseString;
	const char *m_pszUsageRestriction;
	item_capabilities_t m_unCapabilities;
};

//-----------------------------------------------------------------------------
// CEconItemDefinition
// Template Definition of a randomly created item
//-----------------------------------------------------------------------------
class CEconItemDefinition
{
public:
	CEconItemDefinition( void );
	virtual ~CEconItemDefinition( void );

	// BInitFromKV can be implemented on subclasses to parse additional values.
	virtual bool	BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors = NULL );
	virtual bool	BPostInit( CUtlVector<CUtlString> *pVecErrors = NULL );
#if defined(CLIENT_DLL) || defined(GAME_DLL)
	virtual bool	BInitFromTestItemKVs( int iNewDefIndex, KeyValues *pKVItem, CUtlVector<CUtlString>* pVecErrors = NULL );
	virtual void	GeneratePrecacheModelStrings( bool bDynamicLoad, CUtlVector<const char *> *out_pVecModelStrings ) const;
	virtual void	GeneratePrecacheSoundStrings( bool bDynamicLoad, CUtlVector<const char *> *out_pVecSoundStrings ) const;
	virtual void	CopyPolymorphic( const CEconItemDefinition *pSourceDef ) { *this = *pSourceDef; }
#endif

	bool		BInitItemMappings( CUtlVector<CUtlString> *pVecErrors );

	void		BInitVisualBlockFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors = NULL );
	void		BInitStylesBlockFromKV( KeyValues *pKVStyles, perteamvisuals_t *pVisData, CUtlVector<CUtlString> *pVecErrors );

	item_definition_index_t	GetDefinitionIndex( void ) const	{ return m_nDefIndex; }
	item_definition_index_t GetRemappedItemDefIndex( void ) const { return m_nRemappedDefIndex != INVALID_ITEM_DEF_INDEX ? m_nRemappedDefIndex : m_nDefIndex; }
	bool		BEnabled( void ) const				{ return m_bEnabled; }
	bool		BLoadOnDemand( void ) const			{ return m_bLoadOnDemand; }
	bool		BHasBeenLoaded( void ) const		{ return m_bHasBeenLoaded; }
	const char	*GetDefinitionName( void ) const	{ return m_pszDefinitionName; }
	const char	*GetItemDefinitionName( void ) const	{ return m_pszDefinitionName; }
	const char	*GetItemClass( void ) const			{ return m_pszItemClassname; }
	const char	*GetItemBaseName( void ) const		{ return m_pszItemBaseName; }
	const char	*GetBrassModelOverride( void ) const{ return m_pszBrassModelOverride; }
	const char	*GetItemTypeName( void ) const		{ return m_pszItemTypeName; }
	uint8		GetMinLevel( void ) const			{ return m_unMinItemLevel; }
	uint8		GetMaxLevel( void ) const			{ return m_unMaxItemLevel; }
	uint8		GetItemSeries( void ) const			{ return m_unItemSeries; }
	uint8		GetQuality( void ) const			{ return m_nItemQuality; }
	void		SetRarity( uint8 nRarity )			{ Assert( m_nItemRarity == k_unItemRarity_Any ); m_nItemRarity = nRarity; } 
	uint8		GetRarity( void ) const				{ return m_nItemRarity; }
	uint8		GetForcedQuality( void ) const		{ return m_nForcedItemQuality; }
	uint16		GetDefaultDropQuantity( void ) const	{ return m_nDefaultDropQuantity; }
	KeyValues	*GetRawDefinition( void ) const		{ return m_pKVItem; }
	const char	*GetDefinitionString( const char *pszKeyName, const char *pszDefaultValue = "" ) const;
	KeyValues	*GetDefinitionKey( const char *pszKeyName ) const;
	const CUtlVector<static_attrib_t> &GetStaticAttributes( void ) const	{ return m_vecStaticAttributes; }
#ifdef TF_CLIENT_DLL
	uint32		GetNumConcreteItems() const			{ return m_unNumConcreteItems; }
#endif // TF_CLIENT_DLL

	// Data accessing
	bool		IsHidden( void ) const				{ return m_bHidden; }
	bool		IsImported( void ) const			{ return m_bImported; }
	bool		IsAllowedInMatch( void ) const		{ return m_bAllowedInThisMatch; }
	bool		IsBaseItem( void ) const			{ return m_bBaseItem; }
	bool		IsBundle( void ) const				{ return m_BundleInfo != NULL; }
	bool		HasProperName( void ) const			{ return m_bProperName; }
	const char	*GetClassToken( void ) const		{ return m_pszClassToken; }
	const char	*GetSlotToken( void ) const			{ return m_pszSlotToken; }
	bool		ShouldAttachToHands( void ) const	{ return m_bAttachToHands; }
	bool		ShouldAttachToHandsVMOnly( void ) const	{ return m_bAttachToHandsVMOnly; }
	bool		ShouldFlipViewmodels( void ) const	{ return m_bFlipViewModel; }
	int			GetInventoryImagePosition( int iIndex ) const	{ Assert( iIndex >= 0 && iIndex < 2); return m_iInventoryImagePosition[iIndex]; }
	int			GetInventoryImageSize( int iIndex ) const	{ Assert( iIndex >= 0 && iIndex < 2); return m_iInventoryImageSize[iIndex]; }
	int			GetDropType( void ) const			{ return m_iDropType; }
	const char	*GetHolidayRestriction( void ) const	{ return m_pszHolidayRestriction; }
	int			GetVisionFilterFlags( void ) const	{ return m_nVisionFilterFlags; }
	int			GetSubType( void ) const	{ return m_iSubType; }
	item_capabilities_t GetCapabilities( void ) const { return m_iCapabilities; }
	int			GetArmoryRemap( void ) const		{ return m_iArmoryRemap; }
	int			GetStoreRemap( void ) const			{ return m_iStoreRemap; }
	item_definition_index_t GetSetItemRemap() const { return m_unSetItemRemapDefIndex; }		// what def index do we consider ourself for purposes of determining "is an item equipped that satisfies this set slot?" (ie., Festive Huntsman -> Huntsman); default is to point to itself
	const char *GetXifierRemapClass() const			{ return m_pszXifierRemapClass; }
	const char	*GetBaseFunctionalItemName() const	{ return m_pszBaseFunctionalItemName; }
	const char *GetParticleSuffix() const			{ return m_pszParticleSuffix; }

	const CEconItemSetDefinition *GetItemSetDefinition( void ) const { return m_pItemSetDef; }
	void		SetItemSetDefinition( const CEconItemSetDefinition *pItemSetDef ) { Assert( !m_pItemSetDef ); m_pItemSetDef = pItemSetDef; }

	const CEconItemCollectionDefinition *GetItemCollectionDefinition( void ) const { return m_pItemCollectionDef; }
	void  SetItemCollectionDefinition( const CEconItemCollectionDefinition *pItemCollectionDef ) { Assert( !m_pItemCollectionDef ); m_pItemCollectionDef = pItemCollectionDef; }

	perteamvisuals_t	*GetPerTeamVisual( int iTeam ) const	{ return m_PerTeamVisuals[iTeam]; }

	bool IsTool() const									{ return m_bIsTool; }
	const IEconTool	*GetEconTool() const				{ return m_pTool; }
	template < class T >
	const T *GetTypedEconTool() const					{ return dynamic_cast<const T *>( GetEconTool() ); }

	const bundleinfo_t *GetBundleInfo( void ) const { return m_BundleInfo; }
	virtual int GetBundleItemCount( void ) const { return m_BundleInfo ? m_BundleInfo->vecItemDefs.Count() : 0; }
	virtual int GetBundleItem( int iIndex ) const { return m_BundleInfo ? m_BundleInfo->vecItemDefs[iIndex]->GetDefinitionIndex() : -1; }

	// Is this item contained in any bundles? GetContainingBundles() gets the CEconItemDefinitions for those bundles.
	const CUtlVector< const CEconItemDefinition * > &GetContainingBundles() const { return m_vecContainingBundleItemDefs; }
	uint32 GetContainingBundleCount() const { return m_vecContainingBundleItemDefs.Count(); }

	void AddSteamWorkshopContributor( uint32 unAccountID ) { if ( m_vecSteamWorkshopContributors.InvalidIndex() == m_vecSteamWorkshopContributors.Find( unAccountID ) ) { m_vecSteamWorkshopContributors.AddToTail( unAccountID ); } }
	const CUtlVector< uint32 > &GetSteamWorkshopContributors() const { return m_vecSteamWorkshopContributors; }
	bool BIsSteamWorkshopItem() const { return m_vecSteamWorkshopContributors.Count() > 0; }

	const char	*GetIconClassname( void ) const					{ return m_pszItemIconClassname; }
	const char	*GetLogClassname( void ) const					{ return m_pszItemLogClassname; }
	const char	*GetInventoryModel( void ) const				{ return m_pszInventoryModel; }
	const char	*GetInventoryImage( void ) const				{ return m_pszInventoryImage; }
	const char	*GetInventoryOverlayImage( int idx ) const		{ if ( m_pszInventoryOverlayImages.IsValidIndex( idx ) ) return m_pszInventoryOverlayImages[idx]; else return NULL; }
	int			GetInventoryOverlayImageCount( void ) const		{ return m_pszInventoryOverlayImages.Count(); }
	int			GetInspectPanelDistance( void ) const			{ return m_iInspectPanelDistance; }
	const char  *GetIconURLSmall() const						{ return GetIconURL( "s" ); } // Plain small
	const char  *GetIconURLLarge() const						{ return GetIconURL( "l" ); } // Plain large
	void		SetIconURL( const char* pszKey, const char *szURL )	{ m_pDictIcons->Insert( pszKey, CUtlString( szURL ) ); }
	const char  *GetIconURL( const char* pszKey ) const;
	const char	*GetBasePlayerDisplayModel() const				{ return m_pszBaseDisplayModel; }
	int			GetDefaultSkin() const							{ return m_iDefaultSkin; }
	const char  *GetWorldDisplayModel() const					{ return m_pszWorldDisplayModel; }
	const char  *GetCollectionReference() const					{ return m_pszCollectionReference; }

	// Some weapons need a custom model for icon generation. If this value is not present, the world model is used.
	virtual const char  *GetIconDisplayModel()	const;

	const char	*GetExtraWearableModel( void ) const			{ return m_pszWorldExtraWearableModel; }
	const char	*GetExtraWearableViewModel( void ) const		{ return m_pszWorldExtraWearableViewModel; }
	const char  *GetVisionFilteredDisplayModel() const			{ return m_pszVisionFilteredDisplayModel; }
	const char	*GetItemDesc( void ) const						{ return m_pszItemDesc; }
	const char	*GetArmoryDescString( void ) const				{ return m_pszArmoryDesc; }
	RTime32		GetExpirationDate( void ) const					{ return m_rtExpiration; }
	bool		ShouldShowInArmory( void ) const				{ return m_bShouldShowInArmory; }
	bool		IsActingAsAWearable( void ) const				{ return m_bActAsWearable; }
	bool		IsActingAsAWeapon( void ) const					{ return m_bActAsWeapon; }
	bool		GetHideBodyGroupsDeployedOnly( void ) const		{ return m_bHideBodyGroupsDeployedOnly; }
	bool		IsPackBundle( void ) const						{ return m_bIsPackBundle; }
	bool		IsPackItem( void ) const						{ return m_bIsPackItem; }
	CEconItemDefinition	*GetOwningPackBundle()					{ return m_pOwningPackBundle; }
	const CEconItemDefinition	*GetOwningPackBundle() const	{ return m_pOwningPackBundle; }
	const char	*GetDatabaseAuditTableName( void ) const		{ return m_pszDatabaseAuditTable; }

	void SetIsPackItem( bool bIsPackItem ) { m_bIsPackItem = bIsPackItem; }

	equip_region_mask_t GetEquipRegionMask( void ) const { return m_unEquipRegionMask; }
	equip_region_mask_t GetEquipRegionConflictMask( void ) const { return m_unEquipRegionConflictMask; }

	// Dynamic modification during gameplay
	void		SetAllowedInMatch( bool bAllowed )	{ m_bAllowedInThisMatch = bAllowed; }
	void		SetHasBeenLoaded( bool bLoaded )	{ m_bHasBeenLoaded = bLoaded; }

	// Generate and return a random level according to whatever leveling curve this definition uses.
	uint32		RollItemLevel( void ) const;

	const char *GetFirstSaleDate( void ) const;

	void		IterateAttributes( class IEconItemAttributeIterator *pIterator ) const;

#if defined(CLIENT_DLL) || defined(GAME_DLL)
	// Visuals
	// Attached models
	int						GetNumAttachedModels( int iTeam ) const;
	attachedmodel_t			*GetAttachedModelData( int iTeam, int iIdx ) const;

	int						GetNumAttachedModelsFestivized( int iTeam ) const;
	attachedmodel_t			*GetAttachedModelDataFestivized( int iTeam, int iIdx ) const;

	// Attached particle systems
	int						GetNumAttachedParticles( int iTeam ) const;
	attachedparticlesystem_t *GetAttachedParticleData( int iTeam, int iIdx ) const;
	// Activities
	int						GetNumPlaybackActivities( int iTeam ) const;
	activity_on_wearable_t	*GetPlaybackActivityData( int iTeam, int iIdx ) const;
	// Animations
	int						GetNumAnimations( int iTeam ) const;
	animation_on_wearable_t	*GetAnimationData( int iTeam, int iIdx ) const;
	// Animation Overrides
	Activity				GetActivityOverride( int iTeam, Activity baseAct ) const;
	const char				*GetActivityOverride( int iTeam, const char *pszActivity ) const;
	const char				*GetReplacementForActivityOverride( int iTeam, Activity baseAct ) const;
	// poseparam
	int						GetNumPlayerPoseParameters( int iTeam ) const;
	poseparamtable_t		*GetPlayerPoseParameters( int iTeam, int iIdx ) const;
	int						GetNumItemPoseParameters( int iTeam ) const;
	poseparamtable_t		*GetItemPoseParameters( int iTeam, int iIdx ) const;
	// Should the content (meshes, etc.) for this be streamed or preloaded?
	virtual bool			IsContentStreamable() const;
#endif // defined(CLIENT_DLL) || defined(GAME_DLL)

	// FX Overrides
	const char				*GetMuzzleFlash( int iTeam ) const;
	const char				*GetTracerEffect( int iTeam ) const;
	const char				*GetParticleEffect( int iTeam ) const;
	// Materials
	const char				*GetMaterialOverride( int iTeam ) const;
	// Sounds
	const char				*GetCustomSound( int iTeam, int iSound ) const;
	const char				*GetWeaponReplacementSound( int iTeam, /*WeaponSound_t*/ int iSound ) const;
	// Bodygroups
	int						GetHiddenParentBodygroup( int iTeam ) const;
	int						GetNumModifiedBodyGroups( int iTeam ) const;
	const char*				GetModifiedBodyGroup( int iTeam, int i, int& body ) const;
	bool					UsesPerClassBodygroups( int iTeam ) const;
	int						GetNumCodeControlledBodyGroups( int iTeam ) const;
	const char*				GetCodeControlledBodyGroup( int iIteam, int i, struct codecontrolledbodygroupdata_t &ccbgd ) const;

	style_index_t			GetNumStyles() const;
	style_index_t			GetNumSelectableStyles() const;
	const CEconStyleInfo   *GetStyleInfo( style_index_t unStyle ) const;

	int						GetViewmodelBodygroupOverride( int iTeam ) const;
	int						GetViewmodelBodygroupStateOverride( int iTeam ) const;
	int						GetWorldmodelBodygroupOverride( int iTeam ) const;
	int						GetWorldmodelBodygroupStateOverride( int iTeam ) const;

	int						GetPopularitySeed() const { return m_nPopularitySeed; }

	bool					HasEconTag( econ_tag_handle_t tag ) const { return m_vecTags.IsValidIndex( m_vecTags.Find( tag ) ); }

	bool					BValidForShuffle( void ) const { return m_bValidForShuffle; }
	bool					BValidForSelfMade( void ) const { return m_bValidForSelfMade; }

	const CUtlVector<CLootlistJob*>& GetLootlistJobs() const { return m_jobs; }


#if defined(CLIENT_DLL) || defined(GAME_DLL)
	int						GetStyleSkin( style_index_t unStyle, int iTeam, bool bViewmodel ) const;
	const char*				GetStyleInventoryImage( style_index_t unStyle ) const;
	int						GetBestVisualTeamData( int iTeam ) const;
#endif // defined(CLIENT_DLL) || defined(GAME_DLL)


#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, const char *pchName )
	{
		VALIDATE_SCOPE();
		ValidateObj( m_vecStaticAttributes );
		ValidatePtr( m_pKVItem );
		ValidatePtr( m_pProxyCriteria );
	}
#endif // DBGFLAG_VALIDATE


private:
	// Pointer to the raw KeyValue definition of the item
	KeyValues *	m_pKVItem;

	// Required values from m_pKVItem:

	// The number used to refer to this definition in the DB
	item_definition_index_t	m_nDefIndex;
	item_definition_index_t	m_nRemappedDefIndex;
	const char *m_pszRemappedDefItemName;

	// False if this definition has been turned off and we're not using it to generate items
	bool		m_bEnabled;

	// These values specify the range of item levels that an item based off this definition can be generated within.
	uint8		m_unMinItemLevel;
	uint8		m_unMaxItemLevel;

	// This specifies an item quality that items from this definition must be set to. Used mostly to specify unique item definitions.
	uint8		m_nItemQuality;
	uint8		m_nForcedItemQuality;
	uint8		m_nItemRarity;

	// Default drop quantity
	uint16		m_nDefaultDropQuantity;

	// Item Series
	uint8		m_unItemSeries;

	// Static attributes (ones that are always on these items)
	CUtlVector<static_attrib_t> m_vecStaticAttributes;

	// Seeds the popular item list with this number of the item when the list is reset.
	uint8		m_nPopularitySeed;

	// ---------------------------------------------
	// Display related data
	// ---------------------------------------------
	// The base name of this item. i.e. "The Kritz-Krieg".
	const char		*m_pszItemBaseName;
	bool			m_bProperName;		// If set, the name will have "The" prepended to it, unless it's got a non-unique quality
										// in which case it'll have "A" prepended to the quality. i.e. A Community Kritzkrieg

	// The base type of this item. i.e. "Rocket Launcher" or "Shotgun".
	// This is often the same as the base name, but not always.
	const char		*m_pszItemTypeName;

	// The item's non-attribute description.
	const char		*m_pszItemDesc;

	// expiration time
	RTime32			m_rtExpiration;

	// The .mdl file used for this item when it's displayed in inventory-style boxes.
	const char		*m_pszInventoryModel;
	// Alternatively, the image used for this item when it's displayed in inventory-style boxes. If specified, it's used over the model.
	const char		*m_pszInventoryImage;
	// An optional image that's overlayed over the top of the base inventory image. It'll be RGB colored by the tint color of the item.
	CUtlVector<const char*>	m_pszInventoryOverlayImages;
	int				m_iInventoryImagePosition[2];
	int				m_iInventoryImageSize[2];
	int				m_iInspectPanelDistance;

	const char		*m_pszBaseDisplayModel;
	int				m_iDefaultSkin;
	bool			m_bLoadOnDemand;
	bool			m_bHasBeenLoaded;

	bool			m_bHideBodyGroupsDeployedOnly;

	// The .mdl file used for the world view.
	// This is inferior to using a c_model, but because the geometry of the sticky bomb launcher's
	// world model is significantly different from the view model the demoman pack requires
	// using two separate models for now.
	const char		*m_pszWorldDisplayModel;
	const char		*m_pszWorldExtraWearableModel;		// Some weapons attach an extra wearable item to the player
	const char		*m_pszWorldExtraWearableViewModel;	// Some weapons attach an extra wearable view model item to the player
	const char		*m_pszVisionFilteredDisplayModel;	// Some weapons display differently depending on the viewer's filters

	const char		*m_pszCollectionReference;			// Reference a colletion

	// If set, we use the base hands model for a viewmodel, and bonemerge the above player model
	bool			m_bAttachToHands;
	bool			m_bAttachToHandsVMOnly;

	// If set, we will force the view model to render flipped. Good for models built left handed.
	bool			m_bFlipViewModel;

	// This is a wearable that sits in a non-wearable loadout slot
	bool			m_bActAsWearable;	

	// This is a weapon that sits in a wearable slot (Action)
	bool			m_bActAsWeapon;

	// Is this Item a tool
	bool			m_bIsTool;

	// The set this item is a member of
	const CEconItemSetDefinition *m_pItemSetDef;
	const CEconItemCollectionDefinition *m_pItemCollectionDef;

	// A list of per-team visual data used to modify base model for visual recognition
	perteamvisuals_t	*m_PerTeamVisuals[TEAM_VISUAL_SECTIONS];

	// Optional override for specifying a custom shell ejection model
	const char		*m_pszBrassModelOverride;

	IEconTool		*m_pTool;
	bundleinfo_t	*m_BundleInfo;
	item_capabilities_t m_iCapabilities;

#ifdef TF_CLIENT_DLL
	uint32			m_unNumConcreteItems;		// This is the number of items that will actually end up in a user's inventory - this can be 0 for some items (e.g. map stamps in TF), 1 for a "regular" item, or many for bundles, etc.
#endif // TF_CLIENT_DLL

	CUtlDict< CUtlString >*	m_pDictIcons;

	// ---------------------------------------------
	// Creation related data
	// ---------------------------------------------
	// The entity classname for this item.
	const char		*m_pszItemClassname;

	// The entity name that will be displayed in log files.
	const char		*m_pszItemLogClassname;

	// The name of the icon used in the death notices.
	const char		*m_pszItemIconClassname;

	// This is the script file name of this definition. Used to generate items by script name.
	const char		*m_pszDefinitionName;

	// This is used for auditing purposes
	const char		*m_pszDatabaseAuditTable;

	bool			m_bHidden;
	bool			m_bShouldShowInArmory;
	bool			m_bBaseItem;
	bool			m_bImported;

	// A pack bundle is a bundle that contains items that are not for sale individually
	bool			m_bIsPackBundle;
	
	// A pack item is an item which is not for sale individually and is only for sale as part of a pack bundle. A 'regular' bundle can only include a pack bundle by explicitly including all of the pack bundle's items individually.
	// If this pointer is non-NULL, this item is considered to be a pack item (see CEconItemDefinition::IsPackItem()).
	CEconItemDefinition	*m_pOwningPackBundle;
	bool				m_bIsPackItem;

	// Contains information on how to describe items with this attribute in the Armory
	const char		*m_pszArmoryDesc;

	// Temporary(?) solution to allow xifiers to work on botkiller and festive variants of weapons
	const char		*m_pszXifierRemapClass;

	// Base item name -- used for grouping weapon functionality
	const char		*m_pszBaseFunctionalItemName;

	// For particle effects that have derivatives, what is the suffix for this item
	const char		*m_pszParticleSuffix;

	// ---------------------------------------------
	// Remapping data for armory/store
	// ---------------------------------------------
	int				m_iArmoryRemap;
	int				m_iStoreRemap;
	const char		*m_pszArmoryRemap;
	const char		*m_pszStoreRemap;

	// ---------------------------------------------
	// Crafting related data
	// ---------------------------------------------
	const char		*m_pszClassToken;
	const char		*m_pszSlotToken;

	// ---------------------------------------------
	// Gameplay related data
	// ---------------------------------------------
	// How to behave when the player wearing the item dies.
	int				m_iDropType;

	// Holiday restriction. Item only has an appearance when the holiday is in effect.
	const char		*m_pszHolidayRestriction;

	// Meet the pyro makes some items invisible unless you're wearing Pyro Goggles
	int				m_nVisionFilterFlags;

	// Temporary. Revisit this in the engineer update. Enables an additional buildable.
	int				m_iSubType;

	// Whitelist support for tournament mode
	bool			m_bAllowedInThisMatch;

	equip_region_mask_t	m_unEquipRegionMask;			// which equip regions does this item cover directly
	equip_region_mask_t m_unEquipRegionConflictMask;	// which equip regions does equipping this item prevent from having something in them

	item_definition_index_t m_unSetItemRemapDefIndex;	// reference to the definition index we want to consider this item for set matching purposes; see GetSetItemRemap()


	CUtlVector<CLootlistJob*>						m_jobs;

	// False if this definition is not allowed to be part of a shuffled crate's contents
	bool		m_bValidForShuffle;

	// False if this definition should not grant self-made items
	bool		m_bValidForSelfMade;

protected:
	// Protected to allow subclasses to add/remove game-specific tags.
	CUtlVector<econ_tag_handle_t>	m_vecTags;
	CUtlVector<const CEconItemDefinition *> m_vecContainingBundleItemDefs;	// Item definition indices for any bundles which contain this item
	CUtlVector<uint32> m_vecSteamWorkshopContributors;

	friend class CEconItemSchema;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline style_index_t CEconItemDefinition::GetNumStyles() const
{
	const perteamvisuals_t *pVisData = GetPerTeamVisual( 0 );

	if ( !pVisData )
		return 0;

	// Bad things will happen if we ever get more styles than will fit in our
	// style index type. Not Very Bad things, but bad things. Mostly we'll fail
	// to iterate over all our styles.
	return pVisData->m_Styles.Count();
}


//-----------------------------------------------------------------------------
// Purpose: Similar to GetNumStyles, but only the selectable styles
//-----------------------------------------------------------------------------
inline style_index_t CEconItemDefinition::GetNumSelectableStyles() const
{
	const perteamvisuals_t *pVisData = GetPerTeamVisual(0);

	if (!pVisData)
		return 0;

	style_index_t nCount = 0; 
	FOR_EACH_VEC( pVisData->m_Styles, i )
	{
		if( pVisData->m_Styles[i]->IsSelectable() )
		{
			++nCount;
		}
	}

	return nCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline const CEconStyleInfo *CEconItemDefinition::GetStyleInfo( style_index_t unStyle ) const
{
	const perteamvisuals_t *pBaseVisuals = GetPerTeamVisual( 0 );
	if ( !pBaseVisuals || !pBaseVisuals->m_Styles.IsValidIndex( unStyle ) )
		return NULL;

	return pBaseVisuals->m_Styles[unStyle];
}

#if defined(CLIENT_DLL) || defined(GAME_DLL)
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline int CEconItemDefinition::GetNumAttachedModels( int iTeam ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return 0;
	return GetPerTeamVisual(iTeam)->m_AttachedModels.Count(); 
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------
inline attachedmodel_t *CEconItemDefinition::GetAttachedModelData( int iTeam, int iIdx ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	perteamvisuals_t *pVisuals = GetPerTeamVisual(iTeam);
	Assert( pVisuals );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !pVisuals )
		return NULL;

	Assert( iIdx < pVisuals->m_AttachedModels.Count() );
	if ( iIdx >= pVisuals->m_AttachedModels.Count() )
		return NULL;

	return &pVisuals->m_AttachedModels[iIdx];
#else
	return NULL;
#endif
}
//-----------------------------------------------------------------------------
inline int CEconItemDefinition::GetNumAttachedModelsFestivized( int iTeam ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	perteamvisuals_t *pVisuals = GetPerTeamVisual(iTeam);
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !pVisuals )
		return 0;
	return pVisuals->m_AttachedModelsFestive.Count();
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------
inline attachedmodel_t *CEconItemDefinition::GetAttachedModelDataFestivized( int iTeam, int iIdx ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	perteamvisuals_t *pVisuals = GetPerTeamVisual(iTeam);
	Assert( pVisuals );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !pVisuals )
		return NULL;

	Assert( iIdx < pVisuals->m_AttachedModelsFestive.Count() );
	if ( iIdx >= pVisuals->m_AttachedModelsFestive.Count() )
		return NULL;

	return &pVisuals->m_AttachedModelsFestive[iIdx];
#else
	return NULL;
#endif
}
//-----------------------------------------------------------------------------
inline int CEconItemDefinition::GetNumPlaybackActivities( int iTeam ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return 0;
	return GetPerTeamVisual(iTeam)->m_Activities.Count(); 
#else
	return 0;
#endif
}

inline activity_on_wearable_t *CEconItemDefinition::GetPlaybackActivityData( int iTeam, int iIdx ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	Assert( GetPerTeamVisual(iTeam) );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return NULL;

	Assert( iIdx < GetPerTeamVisual(iTeam)->m_Activities.Count() );
	if ( iIdx >= GetPerTeamVisual(iTeam)->m_Activities.Count() )
		return NULL;

	return &GetPerTeamVisual(iTeam)->m_Activities[iIdx];
#else
	return NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline int CEconItemDefinition::GetNumAnimations( int iTeam ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return 0;
	return GetPerTeamVisual(iTeam)->m_Animations.Count(); 
#else
	return 0;
#endif
}
inline animation_on_wearable_t *CEconItemDefinition::GetAnimationData( int iTeam, int iIdx ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	Assert( GetPerTeamVisual(iTeam) );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return NULL;

	Assert( iIdx < GetPerTeamVisual(iTeam)->m_Animations.Count() );
	if ( iIdx >= GetPerTeamVisual(iTeam)->m_Animations.Count() )
		return NULL;

	return &GetPerTeamVisual(iTeam)->m_Animations[iIdx];
#else
	return NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline int CEconItemDefinition::GetNumPlayerPoseParameters( int iTeam ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return 0;
	return GetPerTeamVisual(iTeam)->m_PlayerPoseParams.Count(); 
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline poseparamtable_t *CEconItemDefinition::GetPlayerPoseParameters( int iTeam, int iIdx ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return NULL;

	if ( iIdx >= GetPerTeamVisual(iTeam)->m_PlayerPoseParams.Count() )
		return NULL;

	return &GetPerTeamVisual(iTeam)->m_PlayerPoseParams[iIdx];
#else
	return NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline int CEconItemDefinition::GetNumItemPoseParameters( int iTeam ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return 0;
	return GetPerTeamVisual(iTeam)->m_ItemPoseParams.Count(); 
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline poseparamtable_t *CEconItemDefinition::GetItemPoseParameters( int iTeam, int iIdx ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return NULL;

	if ( iIdx >= GetPerTeamVisual(iTeam)->m_ItemPoseParams.Count() )
		return NULL;

	return &GetPerTeamVisual(iTeam)->m_ItemPoseParams[iIdx];
#else
	return NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline int CEconItemDefinition::GetNumAttachedParticles( int iTeam ) const
{ 
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return 0;
	return GetPerTeamVisual(iTeam)->m_AttachedParticles.Count(); 
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline attachedparticlesystem_t *CEconItemDefinition::GetAttachedParticleData( int iTeam, int iIdx ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	Assert( GetPerTeamVisual(iTeam) );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return NULL;

	Assert( iIdx < GetPerTeamVisual(iTeam)->m_AttachedParticles.Count() );
	if ( iIdx >= GetPerTeamVisual(iTeam)->m_AttachedParticles.Count() )
		return NULL;

	return &GetPerTeamVisual(iTeam)->m_AttachedParticles[iIdx];
#else
	return NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline const char *CEconItemDefinition::GetMaterialOverride( int iTeam ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return NULL;
	return GetPerTeamVisual(iTeam)->pszMaterialOverride;
#else
	return NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline const char *CEconItemDefinition::GetMuzzleFlash( int iTeam ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return NULL;
	return GetPerTeamVisual(iTeam)->pszMuzzleFlash;
#else
	return NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline const char *CEconItemDefinition::GetTracerEffect( int iTeam ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return NULL;
	return GetPerTeamVisual(iTeam)->pszTracerEffect;
#else
	return NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline const char *CEconItemDefinition::GetParticleEffect( int iTeam ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return NULL;
	return GetPerTeamVisual(iTeam)->pszParticleEffect;
#else
	return NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline int CEconItemDefinition::GetHiddenParentBodygroup( int iTeam ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return -1;
	return GetPerTeamVisual(iTeam)->iHideParentBodyGroup;
#else
	return -1;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline int CEconItemDefinition::GetNumModifiedBodyGroups( int iTeam ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return -1;
	return GetPerTeamVisual(iTeam)->m_Maps.m_ModifiedBodyGroupNames.Count();
#else
	return -1;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline const char* CEconItemDefinition::GetModifiedBodyGroup( int iTeam, int i, int& body ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return NULL;
	body = GetPerTeamVisual(iTeam)->m_Maps.m_ModifiedBodyGroupNames[i];
	return GetPerTeamVisual(iTeam)->m_Maps.m_ModifiedBodyGroupNames.GetElementName(i);
#else
	return NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline int CEconItemDefinition::GetNumCodeControlledBodyGroups( int iTeam ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return -1;
	return GetPerTeamVisual(iTeam)->m_Maps.m_CodeControlledBodyGroupNames.Count();
#else
	return -1;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline const char* CEconItemDefinition::GetCodeControlledBodyGroup( int iTeam, int i, codecontrolledbodygroupdata_t &ccbgd ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return NULL;
	ccbgd = GetPerTeamVisual(iTeam)->m_Maps.m_CodeControlledBodyGroupNames[i];
	return GetPerTeamVisual(iTeam)->m_Maps.m_CodeControlledBodyGroupNames.GetElementName(i);
#else
	return NULL;
#endif
}

#if defined(CLIENT_DLL) || defined(GAME_DLL)
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline int CEconItemDefinition::GetStyleSkin( style_index_t unStyle, int iTeam, bool bViewmodel ) const
{
	const CEconStyleInfo *pStyle = GetStyleInfo( unStyle );

	// Return our skin if we have a style or our default skin of -1 otherwise.
	return pStyle
		 ? pStyle->GetSkin( iTeam, bViewmodel )
		 : GetDefaultSkin();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline const char* CEconItemDefinition::GetStyleInventoryImage( style_index_t unStyle ) const
{
	const CEconStyleInfo *pStyle = GetStyleInfo( unStyle );

	return pStyle ? pStyle->GetInventoryImage() : NULL;
}

#endif // defined(CLIENT_DLL) || defined(GAME_DLL)

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline int CEconItemDefinition::GetViewmodelBodygroupOverride( int iTeam ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return 0;
	return GetPerTeamVisual(iTeam)->m_iViewModelBodyGroupOverride;
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline int CEconItemDefinition::GetViewmodelBodygroupStateOverride( int iTeam ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return 0;
	return GetPerTeamVisual(iTeam)->m_iViewModelBodyGroupStateOverride;
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline int CEconItemDefinition::GetWorldmodelBodygroupOverride( int iTeam ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return 0;
	return GetPerTeamVisual(iTeam)->m_iWorldModelBodyGroupOverride;
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline int CEconItemDefinition::GetWorldmodelBodygroupStateOverride( int iTeam ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return 0;
	return GetPerTeamVisual(iTeam)->m_iWorldModelBodyGroupStateOverride;
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline bool CEconItemDefinition::UsesPerClassBodygroups( int iTeam ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return false;
	return GetPerTeamVisual(iTeam)->bUsePerClassBodygroups;
#else
	return false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline const char *CEconItemDefinition::GetCustomSound( int iTeam, int iSound ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return NULL;
	if ( iSound < 0 || iSound >= MAX_VISUALS_CUSTOM_SOUNDS )
		return NULL;
	return GetPerTeamVisual(iTeam)->pszCustomSounds[iSound];
#else
	return NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline const char *CEconItemDefinition::GetWeaponReplacementSound( int iTeam, /* WeaponSound_t */ int iSound ) const
{
#ifndef CSTRIKE_DLL
	iTeam = GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS || !GetPerTeamVisual(iTeam) )
		return NULL;
	if ( iSound < 0 || iSound >= NUM_SHOOT_SOUND_TYPES )
		return NULL;
	return GetPerTeamVisual(iTeam)->pszWeaponSoundReplacements[iSound];
#else
	return NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline int CEconItemDefinition::GetBestVisualTeamData( int iTeam ) const
{
#ifndef CSTRIKE_DLL
	Assert( iTeam >= 0 && iTeam < TEAM_VISUAL_SECTIONS );
	// If we don't have data for the specified team, try to fall back to the base
	//	if ( !GetStaticData() )
	//		return 0;
	if ( (iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS) || (iTeam > 0 && !GetPerTeamVisual(iTeam)) )
		return 0;
	return iTeam;
#else
	return 0;
#endif
}
#endif // defined(CLIENT_DLL) || defined(GAME_DLL)

//-----------------------------------------------------------------------------
// CTimedItemRewardDefinition
// Describes a periodic item reward
//-----------------------------------------------------------------------------
class CTimedItemRewardDefinition
{
public:
	CTimedItemRewardDefinition( void );
	CTimedItemRewardDefinition( const CTimedItemRewardDefinition &that );
	CTimedItemRewardDefinition &operator=( const CTimedItemRewardDefinition& rhs );

	~CTimedItemRewardDefinition( void ) { }

	bool		BInitFromKV( KeyValues *pKVTimedReward, CUtlVector<CUtlString> *pVecErrors = NULL );

	uint32		GetRandomFrequency( void ) const	{ return RandomFloat( m_unMinFreq, m_unMaxFreq ); }
	uint32		GetMinFrequency( void ) const		{ return m_unMinFreq; }
	uint32		GetMaxFrequency( void ) const		{ return m_unMaxFreq; }
	float		GetChance( void ) const				{ return m_flChance; }
	const CItemSelectionCriteria &GetCriteria( void ) const		{ return m_criteria; }
	const CEconLootListDefinition *GetLootList( void ) const	{ return m_pLootList; }

	bool BHasRequiredItem() const { return m_iRequiredItemDef != INVALID_ITEM_DEF_INDEX; }
	item_definition_index_t GetRequiredItem() const { return m_iRequiredItemDef; }

#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, const char *pchName )
	{
		VALIDATE_SCOPE();
		ValidateObj( m_criteria );
	}
#endif // DBGFLAG_VALIDATE

private:
	// Frequency of how often the item is awarded
	uint32		m_unMinFreq;
	uint32		m_unMaxFreq;

	// The chance, between 0 and 1, that the item is rewarded
	float		m_flChance;

	// The criteria to use to select the item to reward
	CItemSelectionCriteria m_criteria;
	// Alternatively, the loot_list to use instead
	const CEconLootListDefinition *m_pLootList;

	item_definition_index_t m_iRequiredItemDef;
};


//-----------------------------------------------------------------------------
// CItemLevelingDefinition
//-----------------------------------------------------------------------------
class CItemLevelingDefinition
{
public:
	CItemLevelingDefinition( void );
	CItemLevelingDefinition( const CItemLevelingDefinition &that );
	CItemLevelingDefinition &operator=( const CItemLevelingDefinition& rhs );

	~CItemLevelingDefinition( void );

	bool		BInitFromKV( KeyValues *pKVItemLevel, const char *pszLevelBlockName, CUtlVector<CUtlString> *pVecErrors = NULL );

	uint32		GetLevel( void ) const { return m_unLevel; }
	uint32		GetRequiredScore( void ) const { return m_unRequiredScore; }
	const char *GetNameLocalizationKey( void ) const { return m_pszLocalizedName_LocalStorage; }

private:
	uint32		m_unLevel;
	uint32		m_unRequiredScore;
	char	   *m_pszLocalizedName_LocalStorage;
};

//-----------------------------------------------------------------------------
// AchievementAward_t
// Holds the item to give away and the Data value to audit it with ( for cross
// game achievements)
//-----------------------------------------------------------------------------
struct AchievementAward_t
{
	AchievementAward_t( const AchievementAward_t & rhs )
		: m_sNativeName( rhs.m_sNativeName ),
		m_unSourceAppId( rhs.m_unSourceAppId ),
		m_unAuditData( rhs.m_unAuditData )
	{
		m_vecDefIndex.CopyArray( rhs.m_vecDefIndex.Base(), rhs.m_vecDefIndex.Count() );
	}
	AchievementAward_t(  ) {}

	CUtlString m_sNativeName;
	AppId_t m_unSourceAppId;
	uint32 m_unAuditData;
	CUtlVector<uint16> m_vecDefIndex;
};

enum eTimedRewardType
{
	kTimedRewards_RegularDrop,
	kTimedRewards_SupplyCrate,
	kTimedRewards_FreeTrialDrop,
	kTimedRewards_RecipeDrop,
	kTimedRewards_EventDrop02,
	kNumTimedRewards
};

struct kill_eater_score_type_t
{
	const char *m_pszTypeString;
	const char *m_pszLevelBlockName;
	bool		m_bAllowBotVictims;			// if true, we don't check for a valid Steam ID on the client before sending or a valid session on the GC before incrementing
};

// Index-to-string table, currently used for attribute value string lookups.
struct schema_string_table_entry_t
{
	int m_iIndex;
	const char *m_pszStr;
};

//-----------------------------------------------------------------------------
// CForeignAppImports
// Defines the way a single foreign app's items are mapped into this app
//-----------------------------------------------------------------------------

class CForeignAppImports
{
public:
	CForeignAppImports() : m_mapDefinitions( DefLessFunc( uint16 ) ) {}

	void AddMapping( uint16 unForeignDefIndex, const CEconItemDefinition *pDefn );
	const CEconItemDefinition *FindMapping( uint16 unForeignDefIndex ) const;

private:
	CUtlMap< uint16, const CEconItemDefinition *> m_mapDefinitions;
};

//-----------------------------------------------------------------------------
// ISchemaAttributeType
//-----------------------------------------------------------------------------

// ISchemaAttributeType is the base interface for a "type" of attribute, where "type" is defined as
// "something that describes the memory layout, the DB layout, how to convert between them, etc.".
// Most of the low-level work done with attributes, including DB reading/writing, packing/unpacking
// for wire traffic, and other leaf code works exclusively through this interface.
//
// The class hierarchy looks like:
//
//		ISchemaAttributeTypeBase< TAttribInMemoryType >:
//	
//			This describes a specific in-memory format for an attribute, without any association to
//			a particular DB, wire format, etc. We can't template the base class because it's an
//			interface. This implements about half of ISchemaAttributeType and has its own mini
//			interface consisting of ConvertTypedValueToByteStream()	and ConvertByteStreamToTypedValue(),
//			both of which do work on statically-typed values that don't exist at higher levels.
//
//		CSchemaAttributeTypeBase< TAttribSchType, TAttribInMemoryType >:
//
//			This handles the schema-related functions on ISchemaAttributeType. These exist at a lower
//			inheritance level than ISchemaAttributeTypeBase to allow code that needs to work type-safely
//			on attributes in memory, but that doesn't know or need to know anything about databases,
//			to exist. Examples of this include code that calls CEconItem::SetDynamicAttributeValue<T>().
//
//			Individual implementations of custom attribute type start making sense immediately as
//			subclasses of CSchemaAttributeTypeBase, for example CSchemaAttributeType_Default, which
//			implements all of the old, untyped attribute system logic.
//
//		CSchemaAttributeTypeProtobufBase< TAttribSchType, TProtobufValueType >
//
//			An easy way of automating most of the work for making a new attribute type is to have
//			the in-memory format be a protobuf object, allowing reflection, automatic network support,
//			etc.
//
// Creating a new custom protobuf attribute consists of three steps:
//
//		- create a new DB table that will hold your attribute data. This needs an itemid_t-sized item ID
//		  column named "ItemID", an attrib_definition_index_t-sized definition index column named "AttrDefIndex",
//		  and then whatever data you want to store.
//
//		- create a new protobuf message type that will hold your custom attribute contents. This exists
//		  on the client and the GC in the same format.
//
//		- implement a subclass of CSchemaAttributeTypeProtobufBase<>, for example:
//
//				class CSchemaAttributeType_StrangeScore : public CSchemaAttributeTypeProtobufBase< GC_SCH_REFERENCE( CSchItemAttributeStrangeScore ) CAttribute_StrangeScore >
//				{
//					virtual void ConvertEconAttributeValueToSch( itemid_t unItemId, const CEconItemAttributeDefinition *pAttrDef, const union attribute_data_union_t& value, GCSDK::CRecordBase *out_pSchRecord ) const OVERRIDE;
//					virtual void LoadSchToEconAttributeValue( CEconItem *pTargetItem, const CEconItemAttributeDefinition *pAttrDef, const GCSDK::CRecordBase *pSchRecord ) const OVERRIDE;
//				};
//
//		  Implement these two GC-only functions to convert from the in-memory format to the DB format and
//		  vice versa and you're good to go.
//
//		- register the new type in CEconItemSchema::BInitAttributeTypes().
//
// If the attribute type can't be silently converted to an already-existing attribute value type, a few other
// places will also fail to compile -- things like typed iteration, or compile-time type checking.
//
// Functions that start with "Convert" change the format of an attribute value (in a type-safe way wherever
// possible), copying the value from one of the passed-in parameters to the other. Functions that start with
// "Load" do a format conversion, but also add the post-conversion value to the passed-in CEconItem. This
// comes up most often when generating new items, either from the DB (LoadSch), the network (LoadByteSteam),
// or creation of a new item on the GC (LoadOrGenerate).
class ISchemaAttributeType
{
public:
	virtual ~ISchemaAttributeType() { }

	// Returns a unique integer describing the C++-in-memory-layout type used by this attribute type.
	// For example, something that stores "int" might return 0 and "CSomeFancyWideAttributeType" might
	// return 1. The actual values don't matter and can even differ between different runs of the game/GC.
	// The only important thing is that during a single run the value for a single type is consistent.
	virtual unsigned int GetTypeUniqueIdentifier() const = 0;


	// Have this attribute type copy the data out of the value union and type-copy it onto the item. This
	// is accessible on clients as well as the GC.
	virtual void LoadEconAttributeValue( CEconItem *pTargetItem, const CEconItemAttributeDefinition *pAttrDef, const union attribute_data_union_t& value ) const = 0;

	// ...
	virtual void ConvertEconAttributeValueToByteStream( const union attribute_data_union_t& value, std::string *out_psBytes ) const = 0;

	// ...
	virtual bool BConvertStringToEconAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const char *pszValue, union attribute_data_union_t *out_pValue, bool bEnableTerribleBackwardsCompatibilitySchemaParsingCode = false ) const = 0;

	// ...
	virtual void ConvertEconAttributeValueToString( const CEconItemAttributeDefinition *pAttrDef, const attribute_data_union_t& value, std::string *out_ps ) const = 0;

	// Used to deserialize a byte stream, probably from an on-wire protobuf message, instead an instance
	// of the attribute in memory. See ConvertByteStreamToTypedValue() for example implementation, or
	// ConvertTypedValueToByteStream() for an example of the byte-stream generator code.
	virtual void LoadByteStreamToEconAttributeValue( CEconItem *pTargetItem, const CEconItemAttributeDefinition *pAttrDef, const std::string& sBytes ) const = 0;

	// Give the subclass a chance to default-initialize a new value. For larger types, this may hit the
	// heap. This must be called before otherwise manipulating [out_pValue] through Convert*() functions.
	virtual void InitializeNewEconAttributeValue( attribute_data_union_t *out_pValue ) const = 0;

	// Free any heap-allocated memory from this attribute value. Is not responsible for zeroing out
	// pointers, etc.
	virtual void UnloadEconAttributeValue( union attribute_data_union_t *out_pValue ) const = 0;

	// ...
	virtual bool OnIterateAttributeValue( class IEconItemAttributeIterator *pIterator, const CEconItemAttributeDefinition *pAttrDef, const attribute_data_union_t& value ) const = 0;

	// This could also be called "BIsHackyMessyOldAttributeType()". This determines whether the attribute
	// can be set at runtime on a CEconItemView instance, whether the gameserver can replicate the value to
	// game clients, etc. It really only makes sense for value types 32 bits or smaller.
	virtual bool BSupportsGameplayModificationAndNetworking() const { return false; }
};

//-----------------------------------------------------------------------------
// CEconItemSchema
// Defines the way econ items can be used in a game
//-----------------------------------------------------------------------------
typedef CUtlDict<CUtlConstString, int> ArmoryStringDict_t;
typedef CUtlDict< CUtlVector<CItemLevelingDefinition> * > LevelBlockDict_t;
typedef CUtlMap<unsigned int, kill_eater_score_type_t>	KillEaterScoreMap_t;
typedef CUtlDict< CUtlVector< schema_string_table_entry_t > * >	SchemaStringTableDict_t;

struct attr_type_t
{
	CUtlConstString m_sName;
	const ISchemaAttributeType *m_pAttrType;

	attr_type_t( const char *pszName, const ISchemaAttributeType *pAttrType )
		: m_sName( pszName )
		, m_pAttrType( pAttrType )
	{
	}
};

#if defined(CLIENT_DLL) || defined(GAME_DLL)
class IDelayedSchemaData
{
public:
	virtual ~IDelayedSchemaData() {}
	virtual bool InitializeSchema( CEconItemSchema *pItemSchema ) = 0;

protected:
	// Passing '0' as the expected version means "we weren't expecting any version in particular" and will
	// skip the sanity checking.
	bool InitializeSchemaInternal( CEconItemSchema *pItemSchema, CUtlBuffer& bufRawData, bool bInitAsBinary, uint32 nExpectedVersion );
};
#endif // defined(CLIENT_DLL) || defined(GAME_DLL)

class CEconStorePriceSheet;

class CEconItemSchema
{
public:
	CEconItemSchema(  );

private:
	CEconItemSchema( const CEconItemSchema & rhs );
	CEconItemSchema &operator=( CEconItemSchema & rhs );

public:
	virtual ~CEconItemSchema( void ) { Reset(); };

	// Setup & parse in the item data files.
	virtual bool BInit( const char *fileName, const char *pathID, CUtlVector<CUtlString> *pVecErrors = NULL );
	bool		BInitBinaryBuffer( CUtlBuffer &buffer, CUtlVector<CUtlString> *pVecErrors = NULL );
	bool		BInitTextBuffer( CUtlBuffer &buffer, CUtlVector<CUtlString> *pVecErrors = NULL );

	uint32		GetVersion() const { return m_unVersion; }
	CSHA		GetSchemaSHA() const { return m_schemaSHA; }
	uint32		GetResetCount() const { return m_unResetCount; }

	// Dump the schema for debug purposes
	bool		DumpItems ( const char *fileName, const char *pathID = NULL );

	// Perform the computation used to calculate the schema version
	static uint32 CalculateKeyValuesVersion( KeyValues *pKV );

#if defined(CLIENT_DLL) || defined(GAME_DLL)
	// This function will immediately reinitialize the schema if it's safe to do so, or store off the data
	// if it isn't safe to update at the moment.
	bool		MaybeInitFromBuffer( IDelayedSchemaData *pDelayedSchemaData );

	// If there is saved schema initialization data, initialize it now. If there is no saved data, this
	// will return success.
	bool		BInitFromDelayedBuffer();
#endif // defined(CLIENT_DLL) || defined(GAME_DLL)

	// Accessors to the base properties
	EEquipType_t		GetEquipTypeFromClassIndex( int iClass ) const;
	equipped_class_t	GetAccountIndex() const								{ return m_unAccoutClassIndex; }
	equipped_class_t	GetFirstValidClass() const							{ return m_unFirstValidClass; }
	equipped_class_t	GetLastValidClass() const							{ return m_unLastValidClass; }
	bool				IsValidClass( equipped_class_t unClass )			{ return ( unClass >= m_unFirstValidClass && unClass <= m_unLastValidClass ) || unClass == GetAccountIndex(); }
	bool				IsValidItemSlot( equipped_slot_t unSlot, equipped_class_t unClass ) const { return IsValidItemSlot( unSlot, unClass == m_unAccoutClassIndex ? EQUIP_TYPE_ACCOUNT : EQUIP_TYPE_CLASS ); }
	bool				IsValidItemSlot( equipped_slot_t unSlot, EEquipType_t eType ) const	
	{
		return eType == EQUIP_TYPE_ACCOUNT ? unSlot >= m_unFirstValidAccountItemSlot && unSlot <= m_unLastValidAccountItemSlot
										   : unSlot >= m_unFirstValidClassItemSlot && unSlot <= m_unLastValidClassItemSlot; 
	}

	enum { kMaxItemPresetCount = 4 };
	uint32				GetNumAllowedItemPresets() const					{ return kMaxItemPresetCount; }
	bool				IsValidPreset( equipped_preset_t unPreset ) const	{ return unPreset <= GetNumAllowedItemPresets(); }

	uint32				GetMinLevel() const									{ return m_unMinLevel; }
	uint32				GetMaxLevel() const									{ return m_unMaxLevel; }

	// Accessors to the underlying sections
	typedef CUtlHashMapLarge<int, CEconItemDefinition*>	ItemDefinitionMap_t;
	const ItemDefinitionMap_t &GetItemDefinitionMap() const { return m_mapItems; }

	typedef CUtlMap<int, CEconItemDefinition*, int>	SortedItemDefinitionMap_t;
	const SortedItemDefinitionMap_t &GetSortedItemDefinitionMap() const { return m_mapItemsSorted; }

	typedef CUtlMap<int, CEconItemDefinition*, int>	ToolsItemDefinitionMap_t;
	const ToolsItemDefinitionMap_t &GetToolsItemDefinitionMap() const { return m_mapToolsItems; }

	typedef CUtlMap<int, CEconItemDefinition*, int>	BaseItemDefinitionMap_t;
	const BaseItemDefinitionMap_t &GetBaseItemDefinitionMap() const { return m_mapBaseItems; }

	typedef CUtlDict<CEconLootListDefinition *>	LootListDefinitionMap_t;
	const LootListDefinitionMap_t &GetLootLists() const { return m_dictLootLists; }

	typedef CUtlMap<int, CUtlString> RevolvingLootListDefinitionMap_t;
	const RevolvingLootListDefinitionMap_t  &GetRevolvingLootLists() const { return m_mapRevolvingLootLists; }

	typedef CUtlDict<int> BodygroupStateMap_t;
	const BodygroupStateMap_t  &GetDefaultBodygroupStateMap() const { return m_dictDefaultBodygroupState; }

	typedef CUtlVector<CEconColorDefinition *>	ColorDefinitionsList_t;

	typedef CUtlDict<KeyValues *> PrefabMap_t;


#if defined(CLIENT_DLL) || defined(GAME_DLL)
	CEconItemDefinition *GetDefaultItemDefinition() { return m_pDefaultItemDefinition; }

	bool SetupPreviewItemDefinition( KeyValues *pKV );
#endif

	const CUtlMap<int, CEconItemQualityDefinition, int > &GetQualityDefinitionMap() const { return m_mapQualities; }
	const CUtlMap<int, CEconItemAttributeDefinition, int > &GetAttributeDefinitionMap() const { return m_mapAttributes; }

	typedef CUtlMap<int, CEconCraftingRecipeDefinition*, int > RecipeDefinitionMap_t;
	const RecipeDefinitionMap_t &GetRecipeDefinitionMap() const { return m_mapRecipes; }

	typedef CUtlDict<CEconItemSetDefinition*> ItemSetMap_t;
	const ItemSetMap_t &GetItemSets() const { return m_dictItemSets; }

	typedef CUtlDict<CEconItemCollectionDefinition*> ItemCollectionMap_t;
	const ItemCollectionMap_t &GetItemCollections() const { return m_dictItemCollections; }

	typedef CUtlDict<CEconOperationDefinition*> OperationDefinitionMap_t;
	const OperationDefinitionMap_t &GetOperationDefinitions() const { return m_dictOperationDefinitions; }
	const CEconOperationDefinition* GetOperationByName( const char* pszName ) const;

	typedef CUtlMap< uint32, const CEconItemDefinition* > PaintKitItemDefinitionMap_t;
	const CEconItemDefinition *GetPaintKitItemDefinition( uint32 unPaintKitDefIndex ) const;
	const CEconItemCollectionDefinition *GetPaintKitCollectionFromItem( const IEconItemInterface *pItem, uint32 *pUnPaintKitDefIndex = NULL ) const;
	

#if defined(CLIENT_DLL) || defined(GAME_DLL)
	const ArmoryStringDict_t	&GetArmoryDataItemClasses() const { return m_dictArmoryItemClassesDataStrings; }
	const ArmoryStringDict_t	&GetArmoryDataItemTypes() const { return m_dictArmoryItemTypesDataStrings; }
	const ArmoryStringDict_t	&GetArmoryDataItems() const { return m_dictArmoryItemDataStrings; }
	const ArmoryStringDict_t	&GetArmoryDataAttributes() const { return m_dictArmoryAttributeDataStrings; }
#endif

	const CTimedItemRewardDefinition* GetTimedReward( eTimedRewardType type ) const;

	const CEconLootListDefinition* GetLootListByName( const char* pListName, int *out_piIndex = NULL ) const;
	const CEconLootListDefinition* GetLootListByIndex( int iIdx ) const { return m_dictLootLists.IsValidIndex(iIdx) ? m_dictLootLists[iIdx] : NULL; }

	uint8 GetDefaultQuality() const { return AE_UNIQUE; }

	void AssignDefaultBodygroupState( const char *pszBodygroupName, int iValue );

	equip_region_mask_t GetEquipRegionMaskByName( const char *pRegionName ) const;

	struct EquipRegion
	{
		CUtlConstString		m_sName;
		unsigned int		m_unBitIndex;		// which bit are we claiming ownership over? there might be multiple equip regions with the same bit if we're in a "shared" block
		equip_region_mask_t m_unMask;			// full region conflict mask
	};

	typedef CUtlVector<EquipRegion>		EquipRegionsList_t;
	const EquipRegionsList_t& GetEquipRegionsList() const { return m_vecEquipRegionsList; }

	equip_region_mask_t GetEquipRegionBitMaskByName( const char *pRegionName ) const;

	KeyValues *FindDefinitionPrefabByName( const char *pszPrefabName ) const;
	const PrefabMap_t& GetPrefabMap() const { return m_dictDefinitionPrefabs; }
	
	CUtlVector< CEconItemDefinition * > &GetBundles() { return m_vecBundles; }	// Retrieve a cached list of all bundles

	const char *FindStringTableEntry( const char *pszTableName, int iIndex ) const;

private:
	void SetEquipRegionConflict( int iRegion, unsigned int unBit );
	int GetEquipRegionIndexByName( const char *pRegionName ) const;

public:
	// Common lookup methods
	bool BGetItemQualityFromName( const char *pchName, uint8 *nQuality ) const;
	const CEconItemQualityDefinition *GetQualityDefinition( int nQuality ) const;
	const CEconItemQualityDefinition *GetQualityDefinitionByName( const char *pszDefName ) const;

	bool BGetItemRarityFromName( const char* pchName, uint8 *nRarity ) const;
	const CEconItemRarityDefinition *GetRarityDefinitionByMapIndex( int nRarityIndex ) const;
	const CEconItemRarityDefinition *GetRarityDefinition( int nRarity ) const;
	const CEconItemRarityDefinition *GetRarityDefinitionByName( const char *pszDefName ) const;
	virtual int GetRarityDefinitionCount( void ) const { return m_mapRarities.Count(); }
	virtual const char* GetRarityName( uint8 iRarity );
	virtual const char* GetRarityLocKey( uint8 iRarity );
	virtual const char* GetRarityColor( uint8 iRarity );
	virtual int GetRarityIndex( const char* pszRarity );

	const CEconItemCollectionDefinition *GetCollectionByName( const char* pCollectionName );

	virtual int GetItemSeriesDefinitionCount( void ) const { return m_mapItemSeries.Count(); }
	bool BGetItemSeries( const char* pchName, uint8 *nItemSeries ) const;
	const CEconItemSeriesDefinition *GetItemSeriesDefinition( int nRarity ) const;

	CEconItemDefinition *GetItemDefinition( int iItemIndex );
	const CEconItemDefinition *GetItemDefinition( int iItemIndex ) const;
	CEconItemAttributeDefinition *GetAttributeDefinition( int iAttribIndex );
	const CEconItemAttributeDefinition *GetAttributeDefinition( int iAttribIndex ) const;
	CEconItemAttributeDefinition *GetAttributeDefinitionByName( const char *pszDefName );
	const CEconItemAttributeDefinition *GetAttributeDefinitionByName( const char *pszDefName ) const;
	CEconCraftingRecipeDefinition *GetRecipeDefinition( int iRecipeIndex );
	CEconColorDefinition *GetColorDefinitionByName( const char *pszDefName );
	const CEconColorDefinition *GetColorDefinitionByName( const char *pszDefName ) const;
#ifdef CLIENT_DLL
	const char *GetSteamPackageLocalizationToken( uint32 unPackageId ) const;
#endif // CLIENT_DLL
	
	bool BCanGSCreateItems( uint32 unIP ) const;
	const AchievementAward_t *GetAchievementRewardByDefIndex( uint16 usDefIndex ) const;
	bool BHasAchievementRewards( void ) const { return (m_dictAchievementRewards.Count() > 0); }

	static CUtlString ComputeAchievementName( AppId_t unAppID, const char *pchNativeAchievementName );

	// Iterating over the item definitions. Game needs this to precache data.
	CEconItemDefinition *GetItemDefinitionByName( const char *pszDefName );
	const CEconItemDefinition *GetItemDefinitionByName( const char *pszDefName ) const;

	random_attrib_t *GetRandomAttributeTemplateByName( const char *pszAttrTemplateName ) const;
	CLootlistJob *GetLootlistJobTemplateByName( const char *pszLootlistJobTemplateName ) const;

	attachedparticlesystem_t* GetAttributeControlledParticleSystem( int id );
	attachedparticlesystem_t* FindAttributeControlledParticleSystem( const char *pchSystemName );
	typedef CUtlMap<int, attachedparticlesystem_t > ParticleDefinitionMap_t;
	const ParticleDefinitionMap_t& GetAttributeControlledParticleSystems() const { return m_mapAttributeControlledParticleSystems; }

	const CUtlVector< int > *GetWeaponUnusualParticleIndexes() const { return &m_vecAttributeControlledParticleSystemsWeapons; }
	const CUtlVector< int > *GetCosmeticUnusualParticleIndexes() const { return &m_vecAttributeControlledParticleSystemsCosmetics; }
	const CUtlVector< int > *GetTauntUnusualParticleIndexes() const { return &m_vecAttributeControlledParticleSystemsTaunts; }

#ifdef CLIENT_DLL
	locchar_t *GetParticleSystemLocalizedName( int index ) const;
#endif // CLIENT_DLL


	item_definition_index_t GetCommunityMarketRemappedDefinitionIndex( item_definition_index_t unSearchItemDef ) const;

	const CUtlVector<attr_type_t>& GetAttributeTypes() const { return m_vecAttributeTypes; }
	const ISchemaAttributeType *GetAttributeType( const char *pszAttrTypeName ) const;

	const LevelBlockDict_t&	GetItemLevelingDataDict() const { return m_vecItemLevelingData; }

	const CUtlVector<CItemLevelingDefinition> *GetItemLevelingData( const char *pszLevelBlockName ) const
	{
		LevelBlockDict_t::IndexType_t i = m_vecItemLevelingData.Find( pszLevelBlockName );
		if ( i == LevelBlockDict_t::InvalidIndex() )
			return NULL;

		return m_vecItemLevelingData[i];
	}

	const CItemLevelingDefinition *GetItemLevelForScore( const char *pszLevelBlockName, uint32 unScore ) const;
	const char *GetKillEaterScoreTypeLocString( uint32 unScoreType ) const;
	const char *GetKillEaterScoreTypeLevelingDataName( uint32 unScoreType ) const;
	bool GetKillEaterScoreTypeAllowsBotVictims( uint32 unScoreType ) const;

#if defined(CLIENT_DLL) || defined(GAME_DLL)
	void		ItemTesting_CreateTestDefinition( int iCloneFromItemDef, int iNewDef, KeyValues *pNewKV );
	void		ItemTesting_DiscardTestDefinition( int iDef );
#endif

#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, const char *pchName );
#endif // DBGFLAG_VALIDATE

	econ_tag_handle_t GetHandleForTag( const char *pszTagName );			// non-const because it may create a new tag handle

	typedef CUtlDict<econ_tag_handle_t> EconTagDict_t;

	virtual RTime32 GetCustomExpirationDate( const char *pszExpirationDate ) const { return k_RTime32Nil; }

public:
	// Subclass interface.
	virtual CEconItemDefinition				*CreateEconItemDefinition()			{ return new CEconItemDefinition; }
	virtual CEconCraftingRecipeDefinition	*CreateCraftingRecipeDefinition()	{ return new CEconCraftingRecipeDefinition; }
	virtual CEconStyleInfo					*CreateEconStyleInfo()				{ return new CEconStyleInfo; }
	virtual CQuestObjectiveDefinition		*CreateQuestDefinition();

	virtual IEconTool						*CreateEconToolImpl( const char *pszToolType, const char *pszUseString, const char *pszUsageRestriction, item_capabilities_t unCapabilities, KeyValues *pUsageKV );

	virtual CItemSelectionCriteria			*CreateItemCriteria( const char *pszContext, KeyValues *pItemCriteriaKV, CUtlVector<CUtlString> *pVecErrors = NULL );
	virtual random_attrib_t					*CreateRandomAttribute( const char *pszContext, KeyValues *pRandomAttributesKV, CUtlVector<CUtlString> *pVecErrors = NULL );
	virtual CLootlistJob					*CreateLootlistJob( const char *pszContext, KeyValues *pLootlistJobKV, CUtlVector<CUtlString> *pVecErrors = NULL );

	virtual bool							BCanStrangeFilterApplyToStrangeSlotInItem( uint32 /*strange_event_restriction_t*/ unRestrictionType, uint32 unRestrictionValue, const IEconItemInterface *pItem, int iStrangeSlot, uint32 *out_pOptionalScoreType ) const;

	bool BInsertLootlist( const char *pListName, KeyValues *pKVLootList, CUtlVector<CUtlString> *pVecErrors );

protected:
	virtual void Reset( void );

	virtual bool BInitSchema( KeyValues *pKVRawDefinition, CUtlVector<CUtlString> *pVecErrors = NULL );
	virtual bool BPostSchemaInit( CUtlVector<CUtlString> *pVecErrors );
#ifdef TF_CLIENT_DLL
	virtual int CalculateNumberOfConcreteItems( const CEconItemDefinition *pItemDef );	// Let derived classes handle custom item types
#endif // TF_CLIENT_DLL

private:
	bool BInitGameInfo( KeyValues *pKVGameInfo, CUtlVector<CUtlString> *pVecErrors );
	bool BInitAttributeTypes( CUtlVector<CUtlString> *pVecErrors );
	bool BInitDefinitionPrefabs( KeyValues *pKVPrefabs, CUtlVector<CUtlString> *pVecErrors );
	bool BInitItemSeries( KeyValues *pKVSeries, CUtlVector<CUtlString> *pVecErrors );
	bool BVerifyBaseItemNames( CUtlVector<CUtlString> *pVecErrors );
	bool BInitRarities( KeyValues *pKVRarities, KeyValues *pKVRarityWeights, CUtlVector<CUtlString> *pVecErrors );
	bool BInitQualities( KeyValues *pKVAttributes, CUtlVector<CUtlString> *pVecErrors );
	bool BInitColors( KeyValues *pKVColors, CUtlVector<CUtlString> *pVecErrors );
	bool BInitAttributes( KeyValues *pKVAttributes, CUtlVector<CUtlString> *pVecErrors );
	bool BInitEquipRegions( KeyValues *pKVEquipRegions, CUtlVector<CUtlString> *pVecErrors );
	bool BInitEquipRegionConflicts( KeyValues *pKVEquipRegions, CUtlVector<CUtlString> *pVecErrors );
	bool BInitItems( KeyValues *pKVAttributes, CUtlVector<CUtlString> *pVecErrors );
	bool BInitItemSets( KeyValues *pKVItemSets, CUtlVector<CUtlString> *pVecErrors );
	bool BInitTimedRewards( KeyValues *pKVTimeRewards, CUtlVector<CUtlString> *pVecErrors );
	bool BInitAchievementRewards( KeyValues *pKVTimeRewards, CUtlVector<CUtlString> *pVecErrors );
	bool BInitItemCriteriaTemplates( KeyValues *pKVItemCriteriaTemplates, CUtlVector<CUtlString> *pVecErrors );
	bool BInitRandomAttributeTemplates( KeyValues *pKVRandomAttributeTemplates, CUtlVector<CUtlString> *pVecErrors );
	bool BInitLootlistJobTemplates( KeyValues *pKVLootlistJobTemplates, CUtlVector<CUtlString> *pVecErrors );
	bool BInitRecipes( KeyValues *pKVRecipes, CUtlVector<CUtlString> *pVecErrors );
	bool BInitLootLists( KeyValues *pKVLootLists, CUtlVector<CUtlString> *pVecErrors );
	bool BInitRevolvingLootLists( KeyValues *pKVRevolvingLootLists, CUtlVector<CUtlString> *pVecErrors );
	bool BInitItemCollections( KeyValues *pKVItemSets, CUtlVector<CUtlString> *pVecErrors );
	bool BInitCollectionReferences( CUtlVector<CUtlString> *pVecErrors );
	bool BInitOperationDefinitions( KeyValues *pKVGameInfo, KeyValues *pOperations, CUtlVector<CUtlString> *pVecErrors );

#ifdef TF_CLIENT_DLL
	bool BInitConcreteItemCounts( CUtlVector<CUtlString> *pVecErrors );
	bool BInitSteamPackageLocalizationToken( KeyValues *pKVSteamPackages, CUtlVector<CUtlString> *pVecErrors );
#endif // TF_CLIENT_DLL
	bool BInitItemLevels( KeyValues *pKVItemLevels, CUtlVector<CUtlString> *pVecErrors );
	bool BInitKillEaterScoreTypes( KeyValues *pKVItemLevels, CUtlVector<CUtlString> *pVecErrors );
	bool BInitStringTables( KeyValues *pKVStringTables, CUtlVector<CUtlString> *pVecErrors );
	bool BInitCommunityMarketRemaps( KeyValues *pKVCommunityMarketRemaps, CUtlVector<CUtlString> *pVecErrors );

	bool BInitAttributeControlledParticleSystems( KeyValues *pKVParticleSystems, CUtlVector<CUtlString> *pVecErrors );

#if defined(CLIENT_DLL) || defined(GAME_DLL)
	bool BInitArmoryData( KeyValues *pKVArmoryData, CUtlVector<CUtlString> *pVecErrors );
#else
	bool BInitExperiements( KeyValues *pKVExperiments, CUtlVector<CUtlString> *pVecErrors );
	bool BInitForeignImports( CUtlVector<CUtlString> *pVecErrors );

	CForeignAppImports *FindOrAddAppImports( AppId_t unAppID );
#endif

	bool BVerifyLootListItemDropDates( const CEconLootListDefinition* pLootList, CUtlVector<CUtlString> *pVecErrors ) const;
	bool BRecurseiveVerifyLootListItemDropDates(  const CEconLootListDefinition* pLootList, const CEconLootListDefinition* pRootLootList, CUtlVector<CUtlString> *pVecErrors ) const;

	// Note: this returns pointers to the inside of a vector and/or NULL. Pointers are not intended to be
	// saved off and used later.
	const kill_eater_score_type_t *FindKillEaterScoreType( uint32 unScoreType ) const;

	uint32			m_unResetCount;

	KeyValues		*m_pKVRawDefinition;
	uint32			m_unVersion;
	CSHA			m_schemaSHA;

	// Class range
	equipped_class_t	m_unFirstValidClass;
	equipped_class_t	m_unLastValidClass;
	equipped_class_t	m_unAccoutClassIndex;

	// Item slot range
	equipped_slot_t		m_unFirstValidClassItemSlot;
	equipped_slot_t		m_unLastValidClassItemSlot;
	equipped_slot_t		m_unFirstValidAccountItemSlot;
	equipped_slot_t		m_unLastValidAccountItemSlot;

	// Number of allowed presets
	uint32			m_unNumItemPresets;

	// Allowable range of item levels for this app
	uint32			m_unMinLevel;
	uint32			m_unMaxLevel;

	// Total value of all the weights of the qualities
	uint32			m_unSumQualityWeights;

	// Name-to-implementation list of all unique attribute types (ie., "wide strange score").
	CUtlVector<attr_type_t>								m_vecAttributeTypes;

	// Contains the list of rarity definitions
	CUtlMap<int, CEconItemSeriesDefinition, int >		m_mapItemSeries;

	// Contains the list of rarity definitions
	CUtlMap<int, CEconItemRarityDefinition, int >		m_mapRarities;

	// Contains the list of item definitions read in from all data files.
	CUtlMap<int, CEconItemQualityDefinition, int >		m_mapQualities;

	// Contains the list of item definitions read in from all data files.
	ItemDefinitionMap_t									m_mapItems;

	CUtlMap<int, CQuestObjectiveDefinition*, int >		m_mapQuestObjectives;

	// A sorted version of the same map, for instances where we really want sorted data
	SortedItemDefinitionMap_t							m_mapItemsSorted;

	// List of all the tool items, is a sublist of mapItems
	ToolsItemDefinitionMap_t							m_mapToolsItems;

	// List of all paintkit tool item definitions
	PaintKitItemDefinitionMap_t							m_mapPaintKitTools;

	// List of all base items, is a sublist of mapItems
	BaseItemDefinitionMap_t								m_mapBaseItems;

#if defined(CLIENT_DLL) || defined(GAME_DLL)
	// What is the default item definition we'll return in the client code if we can't find the correct one?
	CEconItemDefinition								   *m_pDefaultItemDefinition;
#endif

	// Contains the list of attribute definitions read in from all data files.
	CUtlMap<int, CEconItemAttributeDefinition, int >	m_mapAttributes;

	// Contains the list of item recipes read in from all data files.
	RecipeDefinitionMap_t								m_mapRecipes;

	// Contains the list of item sets.
	ItemSetMap_t										m_dictItemSets;
	ItemCollectionMap_t									m_dictItemCollections;

	OperationDefinitionMap_t							m_dictOperationDefinitions;

	// Revolving loot lists.
	CUtlMap<int, CUtlString>							m_mapRevolvingLootLists;

	// Contains the list of loot lists.
	LootListDefinitionMap_t								m_dictLootLists;

	// List of events that award items based on time played
	CUtlVector<CTimedItemRewardDefinition>				m_vecTimedRewards;

	// list of items that will be awarded from achievements
	CUtlDict< AchievementAward_t *, int >				m_dictAchievementRewards;
	CUtlMap< uint32, AchievementAward_t * >				m_mapAchievementRewardsByData;

	CUtlDict< CItemSelectionCriteria* >					m_dictItemCriteriaTemplates;

	// list of random attribute templates
	CUtlDict< random_attrib_t * >						m_dictRandomAttributeTemplates;

	// list of lootlist job templates
	CUtlDict< CLootlistJob * >							m_dictLootlistJobTemplates;

	// Contains information for attribute attached particle systems
	CUtlMap<int, attachedparticlesystem_t >				m_mapAttributeControlledParticleSystems;
	CUtlVector< int >									m_vecAttributeControlledParticleSystemsCosmetics;
	CUtlVector< int >									m_vecAttributeControlledParticleSystemsWeapons;
	CUtlVector< int >									m_vecAttributeControlledParticleSystemsTaunts;


	// Contains information on which equip regions conflict with each other regions and how to
	// test for overlap.
	EquipRegionsList_t									m_vecEquipRegionsList;

	// Contains information about prefab KeyValues blocks that be can referenced elsewhere
	// in the schema.
	PrefabMap_t											m_dictDefinitionPrefabs;

	// Contains runtime color information, looked-up by name.
	ColorDefinitionsList_t								m_vecColorDefs;

	// Contains information about: a) every bodygroup that appears anywhere in the schema, and
	// b) whether they default to on or off.
	BodygroupStateMap_t									m_dictDefaultBodygroupState;

	// Various definitions can have any number of unique tags associated with them.
	EconTagDict_t										m_dictTags;


	// List of item leveling data.
	KillEaterScoreMap_t									m_mapKillEaterScoreTypes;

	SchemaStringTableDict_t m_dictStringTable;

	typedef CUtlMap< item_definition_index_t, item_definition_index_t, item_definition_index_t > CommunityMarketDefinitionRemapMap_t;
	CommunityMarketDefinitionRemapMap_t					m_mapCommunityMarketDefinitionIndexRemap;

#ifdef CLIENT_DLL
	// Steam-package-ID-to-localization-token map, used for modifying tooltips in the store.
	typedef CUtlMap< uint32, const char * > SteamPackageLocalizationTokenMap_t;
	SteamPackageLocalizationTokenMap_t					m_mapSteamPackageLocalizationTokens;
#endif // CLIENT_DLL

	LevelBlockDict_t m_vecItemLevelingData;

#if defined(CLIENT_DLL) || defined(GAME_DLL)
	// Contains Armory data key->localization string mappings
	ArmoryStringDict_t m_dictArmoryItemTypesDataStrings;
	ArmoryStringDict_t m_dictArmoryItemClassesDataStrings;
	ArmoryStringDict_t m_dictArmoryAttributeDataStrings;
	ArmoryStringDict_t m_dictArmoryItemDataStrings;

	// Used for delaying the parsing of the item schema until its safe to swap out the back end data.
	IDelayedSchemaData *m_pDelayedSchemaData;
#endif

	CUtlVector< CEconItemDefinition * > m_vecBundles;	// A cached list of all bundles
};


extern CEconItemSchema & GEconItemSchema();

//-----------------------------------------------------------------------------
// CSchemaFieldHandle
//-----------------------------------------------------------------------------
template < class T >
class CSchemaFieldHandle
{
public:
	explicit CSchemaFieldHandle( const char *szName )
		: m_szName( szName )
	{
		m_pRef = GetTypedRef();
		m_unSchemaGeneration = GEconItemSchema().GetResetCount();
#if _DEBUG
		m_unVersion_Debug = GEconItemSchema().GetVersion();
#endif
	}

	operator const T *( void ) const
	{
		uint32 unSchemaGeneration = GEconItemSchema().GetResetCount();
		if ( m_unSchemaGeneration != unSchemaGeneration )
		{
			m_pRef = GetTypedRef();
			m_unSchemaGeneration = unSchemaGeneration;
#if _DEBUG
			m_unVersion_Debug = GEconItemSchema().GetVersion();
#endif
		}

#if _DEBUG
		Assert( m_unVersion_Debug == GEconItemSchema().GetVersion() );
#endif
		return m_pRef;
	}

	const T *operator->( void ) const
	{
		return static_cast<const T *>( *this );
	}

	const char *GetName( void ) const
	{
		return m_szName;
	}

private:
	const T *GetTypedRef() const;

private:
	const char *m_szName;

	mutable const T *m_pRef;
	mutable uint32 m_unSchemaGeneration;
#if _DEBUG
	mutable uint32 m_unVersion_Debug;
#endif
};

template < >
inline const CEconColorDefinition *CSchemaFieldHandle<CEconColorDefinition>::GetTypedRef( void ) const
{
	return GEconItemSchema().GetColorDefinitionByName( m_szName );
}

template < >
inline const CEconItemAttributeDefinition *CSchemaFieldHandle<CEconItemAttributeDefinition>::GetTypedRef( void ) const
{
	return GEconItemSchema().GetAttributeDefinitionByName( m_szName );
}

template < >
inline const CEconItemDefinition *CSchemaFieldHandle<CEconItemDefinition>::GetTypedRef( void ) const
{
	return GEconItemSchema().GetItemDefinitionByName( m_szName );
}

template < >
inline const CEconLootListDefinition *CSchemaFieldHandle<CEconLootListDefinition>::GetTypedRef( void ) const
{
	return GEconItemSchema().GetLootListByName( m_szName );
}

template < >
inline const attachedparticlesystem_t *CSchemaFieldHandle<attachedparticlesystem_t>::GetTypedRef( void ) const
{
	return GEconItemSchema().FindAttributeControlledParticleSystem( m_szName );
}

typedef CSchemaFieldHandle<CEconColorDefinition>			CSchemaColorDefHandle;
typedef CSchemaFieldHandle<CEconItemAttributeDefinition>	CSchemaAttributeDefHandle;
typedef CSchemaFieldHandle<CEconItemDefinition>				CSchemaItemDefHandle;
typedef CSchemaFieldHandle<CEconLootListDefinition>			CSchemaLootListDefHandle;
typedef CSchemaFieldHandle<attachedparticlesystem_t>		CSchemaParticleHandle;


struct steam_market_gc_identifier_t
{
	item_definition_index_t m_unDefIndex;
	uint8 m_unQuality;

	bool operator<( const struct steam_market_gc_identifier_t& b ) const
	{
		return (m_unDefIndex < b.m_unDefIndex)
			|| ((m_unDefIndex == b.m_unDefIndex) && (m_unQuality < b.m_unQuality));
	}
};

// Implementation reliant on earlier class content.
inline const CEconItemAttributeDefinition *static_attrib_t::GetAttributeDefinition() const
{
	return GEconItemSchema().GetAttributeDefinition( iDefIndex );
}

inline const ISchemaAttributeType *static_attrib_t::GetAttributeType() const
{
	const CEconItemAttributeDefinition *pAttrDef = GetAttributeDefinition();
	if ( !pAttrDef )
		return NULL;

	return pAttrDef->GetAttributeType();
}

// Utility function to convert datafile strings to ints.
int StringFieldToInt( const char *szValue, const char **pValueStrings, int iNumStrings, bool bDontAssert = false );
int StringFieldToInt( const char *szValue, const CUtlVector<const char *>& vecValueStrings, bool bDontAssert = false );


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CAttributeLineItemLootList : public IEconLootList
{
public:
	static CSchemaAttributeDefHandle s_pAttrDef_RandomDropLineItems[4];
	static CSchemaAttributeDefHandle s_pAttrDef_RandomDropLineItemFooterDesc;

public:
	CAttributeLineItemLootList( const IEconItemInterface *pEconItem )
		: m_pEconItem( pEconItem )
	{
		//
	}

	virtual void EnumerateUserFacingPotentialDrops( IEconLootListIterator *pIt ) const OVERRIDE;
	virtual bool BPublicListContents() const OVERRIDE { return true; }		// any attribute data that clients have is public to them
	virtual const char *GetLootListHeaderLocalizationKey() const OVERRIDE;
	virtual const char *GetLootListFooterLocalizationKey() const OVERRIDE;
	virtual const char *GetLootListCollectionReference() const OVERRIDE;
	

private:
	const IEconItemInterface *m_pEconItem;
};

void MergeDefinitionPrefab( KeyValues *pKVWriteItem, KeyValues *pKVSourceItem );
bool IsUnusualAttribute( const CEconItemAttributeDefinition *pAttrDef );
bool ItemHasUnusualAttribute( const IEconItemInterface *pItem, const CEconItemAttributeDefinition **pUnusualAttribute = NULL, uint32 *pUnAttributeValue = NULL );
bool IsPaintKitTool( const CEconItemDefinition *pItemDef );
bool CheckValveSignature(const void *data, uint32 nDataSize, const void *signature, uint32 nSignatureSize);
bool TF_CheckSignature(const char* fileName, const char *pathID, CUtlBuffer& bufRawData);

#endif //ECONITEMSCHEMA_H
