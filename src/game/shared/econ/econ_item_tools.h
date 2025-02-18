//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef ECONITEMTOOLS_H
#define ECONITEMTOOLS_H
#ifdef _WIN32
#pragma once
#endif

enum EConsumptionAttemptResult
{
	kConsumptionResult_CannotConsume,			// could be bum definitions or doesnt meet criteria or anything -- this is failure
	kConsumptionResult_CanConsume,				// able to consume
	kConsumptionResult_WillCompleteCollection,	// able to consume and is the final item to be consumed
};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconSharedToolSupport
{
public:
	// Can the given tool instance apply to a specific instance of an item. This should be used in the general
	// case whenever a CEconItem or a CEconItemView is available.
	static bool ToolCanApplyTo( const IEconItemInterface *pToolDef, const IEconItemInterface *pToolSubject );

	// Can the given tool definition apply to an item definition? This will check things like restrictions,
	// matching tool capabilities, etc. but will ignore instance-specific properties. This should only be used
	// by code that doesn't have any access to an instance of the definition.
	static bool ToolCanApplyToDefinition( const GameItemDefinition_t *pToolDef, const GameItemDefinition_t *pToolSubjectDef );

	// Can the given tool definition apply to a base item definition?
	static bool ToolCanApplyToBaseItem( const GameItemDefinition_t *pToolDef, const GameItemDefinition_t *pToolSubjectDef );
};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_DuelingMinigame : public IEconTool
{
public:
	CEconTool_DuelingMinigame( const char *pszTypeName, const char *pszUseString ) : IEconTool( pszTypeName, pszUseString, NULL, ITEM_CAP_NONE ) { }

#ifdef CLIENT_DLL
	virtual void OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL

};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_Noisemaker : public IEconTool
{
public:
	CEconTool_Noisemaker( const char *pszTypeName, const char *pszUseString ) : IEconTool( pszTypeName, pszUseString, NULL, ITEM_CAP_NONE ) { }

#ifdef CLIENT_DLL
	virtual void OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL
};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_WrappedGift : public IEconTool
{
public:
	CEconTool_WrappedGift( const char *pszTypeName, const char *pszUseString, item_capabilities_t unCapabilities, KeyValues *pUsageKV );

	virtual bool BFinishInitialization() OVERRIDE;

	bool BIsGlobalGift() const { return m_bIsGlobalGift; }
	// Allows the item to be directly used rather than via the trading system
	bool BIsDirectGift() const { return m_bIsDirectGift; }
	const CEconItemDefinition *GetDeliveredItemDefinition() const { return m_pDeliveredGiftItemDef; }	// can return NULL! (means "don't change definitions on delivery")

#ifdef CLIENT_DLL
	virtual bool CanBeUsedNow( const IEconItemInterface *pItem ) const OVERRIDE;
	virtual bool ShouldShowContainedItemPanel( const IEconItemInterface *pItem ) const OVERRIDE;
	virtual const char *GetUseCommandLocalizationToken( const IEconItemInterface *pItem, int i = 0 ) const OVERRIDE;
	virtual void OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const OVERRIDE;
	virtual int GetUseCommandCount( const IEconItemInterface *pItem ) const OVERRIDE;
	virtual const char* GetUseCommand( const IEconItemInterface *pItem, int i = 0 ) const OVERRIDE;
#endif // CLIENT_DLL


private:
	const char *m_pszDeliveredGiftItemDefName;			// points to memory inside our init KV -- only valid between the constructor call and the BFinishInitialization() call (this is messy but Fletcher and I agree it makes more sense than switching to a full two-pass schema parse just for this)

	const CEconItemDefinition *m_pDeliveredGiftItemDef;
	bool m_bIsGlobalGift;
	bool m_bIsDirectGift;
};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_WeddingRing : public IEconTool
{
public:
	CEconTool_WeddingRing( const char *pszTypeName, const char *pszUseString, item_capabilities_t unCapabilities ) : IEconTool( pszTypeName, pszUseString, NULL, unCapabilities ) { }

	virtual bool RequiresToolEscrowPeriod() const { return false; }

#ifdef CLIENT_DLL
	virtual const char *GetUseCommandLocalizationToken( const IEconItemInterface *pItem, int i = 0 ) const;
	virtual void OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL

};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_TagsList
{
public:
	CEconTool_TagsList( KeyValues *pKVTags )
	{
		if ( pKVTags )
		{
			FOR_EACH_SUBKEY( pKVTags, pKVTag )
			{
				m_vecTags.AddToTail( GetItemSchema()->GetHandleForTag( pKVTag->GetName() ) );
			}
		}
	}

	const CUtlVector<econ_tag_handle_t>& GetTagsList() const { return m_vecTags; }

private:
	CUtlVector<econ_tag_handle_t> m_vecTags;
};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_StrangeCountTransfer : public IEconTool
{
public:
	CEconTool_StrangeCountTransfer( const char *pszTypeName, item_capabilities_t unCapabilities );
	
	static bool AreItemsEligibleForStrangeCountTransfer( const IEconItemInterface *pItem1, const IEconItemInterface *pItem2 );

#ifdef CLIENT_DLL
	virtual void OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const;
	//virtual void OnClientApplyCommit( CEconItemView *pTool, CEconItemView *pSubject ) const;

	bool SetItems( CEconItemView *pItem1, CEconItemView *pItem2 );

	CEconItemView *m_pItemSrc;
	CEconItemView *m_pItemDest;
#endif
};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_StrangePart : public IEconTool
{
public:
	CEconTool_StrangePart( const char *pszTypeName, const char *pszUseString, item_capabilities_t unCapabilities, KeyValues *pUsageKV )
		: IEconTool( pszTypeName, pszUseString, NULL, unCapabilities ) 
		, m_RequiredTags( pUsageKV ? pUsageKV->FindKey( "required_tags" ) : NULL )
		, m_RequiredMissingTags( pUsageKV ? pUsageKV->FindKey( "required_missing_tags" ) : NULL )
	{
		//
	}

	virtual bool CanApplyTo( const IEconItemInterface *pTool, const IEconItemInterface *pToolSubject ) const;

#ifdef CLIENT_DLL
	virtual bool ShouldDisplayAsUseableOnItemsInArmory() const OVERRIDE { return false; }

	virtual void OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL
 
private:
	CEconTool_TagsList m_RequiredTags;
	CEconTool_TagsList m_RequiredMissingTags;
};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_StrangePartRestriction : public IEconTool
{
public:
	CEconTool_StrangePartRestriction( const char *pszTypeName, const char *pszUseString, item_capabilities_t unCapabilities, KeyValues *pUsageKV );

	virtual bool CanApplyTo( const IEconItemInterface *pTool, const IEconItemInterface *pToolSubject ) const;
	virtual bool BFinishInitialization() OVERRIDE;
#ifdef CLIENT_DLL
	virtual bool ShouldDisplayAsUseableOnItemsInArmory() const OVERRIDE { return false; }

	virtual void OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL

	unsigned int GetRestrictionType() const { return m_eRestrictionType; }
	unsigned int GetRestrictionValue() const { return m_unRestrictionValue; }
 
private:
	unsigned int /*strange_event_restriction_t*/ m_eRestrictionType;

	const char *m_pszRestrictionValue;			// points to memory inside our init KV -- only valid between the constructor call and the BFinishInitialization() call (this is messy but Fletcher and I agree it makes more sense than switching to a full two-pass schema parse just for this)
	unsigned int m_unRestrictionValue;
};

//---------------------------------------------------------------------------------------
// Purpose: New crafting!  This new systems allows for dynamic crafting recipes to be
//			generated in the form of an item itself.  Players can "feed" in items on the
//			recipe's input list, either all at once or once at a time, until the inputs
//			are all fulfilled. Once that happens at which the outputs of the recipe are given to the player.
//			
//			This is done using new attribute types that encode the recipe's inputs and outputs.
//			Inputs and outputs can either be specific items of specific qualities, or lootlist
//			with a specific quality -- for now.  Lootlist will roll the specific item to be the input/output
//			when the recipe item is created.  Any gc generated attributes that would come
//			from the lootlists will also get encoded as a string in the recipe's attribute, so things like
//			unusual particle effects will get applied to outputs.  These string-encoded attributes
//			are ignored during input criteria matching for now.
//
//			Components are allowed to have nested components defined within them.  These child
//			components only roll their chance to apply if their parent successfully rolls their
//			chance to apply.
//---------------------------------------------------------------------------------------
class CEconTool_ItemDynamicRecipe : public IEconTool
{
public:

	// This enum lets the CDynamicRecipeComponentLootList class specify
	// "uniqueness" of the item def that it will roll.  This allows us to
	// do things like ensure that the output item will never be one of the
	// input items, or there's never duplicate inputs.
	enum EItemDefUniqueness_t
	{
		UNIQUE_AMONG_INPUTS = 0,
		UNIQUE_AMONG_OUTPUTS,
		UNIQUE_AMONG_EVERYTHING,
		UNIQUE_AMONG_NOTHING,
	};

	CEconTool_ItemDynamicRecipe( const char *pszTypeName, const char *pszUseString, item_capabilities_t unCapabilities, KeyValues *pUsageKV );
	~CEconTool_ItemDynamicRecipe();

	virtual bool CanApplyTo( const IEconItemInterface *pTool, const IEconItemInterface *pToolSubject ) const;
	virtual bool BFinishInitialization() OVERRIDE;

#ifdef CLIENT_DLL
	virtual bool ShouldDisplayAsUseableOnItemsInArmory() const OVERRIDE { return false; }
#endif // CLIENT_DLL


	virtual bool BInitFromKV( KeyValues *pKVDefinition, CUtlVector<CUtlString> *pVecErrors );

	class CBaseRecipeComponent
	{
	public:

		struct StringEncodedAttribute_t
		{
			attrib_definition_index_t m_AttrIndex;
			CUtlConstString m_strAttrData;
		};

		struct CountChance_t
		{
			int m_nMinCount;
			int m_nMaxCount;
			float m_flChance;
		};

		typedef CUtlVector<const CEconItemAttributeDefinition *> ComponentAttribVector_t;

		CBaseRecipeComponent( bool bIsOutput, const CBaseRecipeComponent* pParent );
		virtual ~CBaseRecipeComponent();

		static bool ParseComponentsBlock( KeyValues *pKV, CUtlVector<CBaseRecipeComponent*>& vecComponents, CUtlVector<CUtlString> *pVecErrors, const CBaseRecipeComponent* pParent );
		static bool ParseComponents( KeyValues *pKV, CUtlVector<CBaseRecipeComponent*>& vecComponents, bool bIsOutput, CUtlVector<CUtlString> *pVecErrors, const CBaseRecipeComponent* pParent );
		virtual bool ParseKV( KeyValues *pKV, CUtlVector<CUtlString> *pVecErrors );
		void SetIsOutput( bool bIsOutput ) { m_bIsOutput = bIsOutput; }
		void SetParent( CBaseRecipeComponent* pParent ) { m_pParent = pParent; }
		void SetChanceOfApplying( float flChance );
		virtual bool BFinishInitialization_Internal( CUtlVector<CUtlString>* pVecErrors, ComponentAttribVector_t* attribVec );

		bool GetIsOutput() const { return m_bIsOutput; }
		void GetIsGuaranteed( int &nFlags ) const;
		const CUtlVector< CountChance_t >& GetRollChances() const { return m_vecCountChances; }
	protected: 
		const CBaseRecipeComponent* m_pParent;
		CUtlVector< CBaseRecipeComponent* > m_vecAdditionalComponents;
		float m_flChanceOfApplying;
		bool m_bIsOutput;
		CUtlVector< CountChance_t > m_vecCountChances;
		float m_flTotalWeights;

		EEconItemQuality	m_eQuality;

		enum EAttributesMatchingType_t
		{
			ATTRIBUTES_MATCH_NONE = 0,
			ATTRIBUTES_MATCH_ALL,
			ATTRIBUTES_MATCH_ANY,
		};

		EAttributesMatchingType_t m_attributesMatchingType;
		CUtlVector< StringEncodedAttribute_t > m_vecDynamicAttributes;

		CUtlString m_strName;

		static const char* m_pszUseParentNameIdentifier;
	};

public:

	class CDynamicRecipeComponentLootList;
	// Defined item type: Use this when you want to quickly define a specific item as
	//					  a component for a recipe.  A "defined item" is considered an
	//					  itemdef, a quality, and any additional attributes.
	class CDynamicRecipeComponentDefinedItem : public CBaseRecipeComponent
	{
		typedef CBaseRecipeComponent BaseClass;
	public:
		CDynamicRecipeComponentDefinedItem( bool bIsOutput, const CBaseRecipeComponent* pParent );
		virtual ~CDynamicRecipeComponentDefinedItem();
		virtual bool BFinishInitialization_Internal( CUtlVector<CUtlString>* pVecErrors, ComponentAttribVector_t* attribVec ) OVERRIDE;
	protected:

		virtual bool ParseKV( KeyValues *pKV, CUtlVector<CUtlString> *pVecErrors ) OVERRIDE;

		friend class CDynamicRecipeComponentLootList;
	};

	// Lootlist type: Use this when you want a random item from a lootlist to be a 
	//				  component in a recipe.  You must specify a quality in the definition
	//				  but it can get stomped if the lootlist-generated item gets an "elevate quality"
	//				  attribute rolled onto it.
	class CDynamicRecipeComponentLootList : public CBaseRecipeComponent
	{
		typedef CBaseRecipeComponent BaseClass;
	public:
		CDynamicRecipeComponentLootList( bool bIsOutput, const CBaseRecipeComponent* pParent );
		virtual ~CDynamicRecipeComponentLootList();
		virtual bool BFinishInitialization_Internal( CUtlVector<CUtlString>* pVecErrors, ComponentAttribVector_t* attribVec ) OVERRIDE;
	protected:
		virtual bool ParseKV( KeyValues *pKV, CUtlVector<CUtlString> *pVecErrors ) OVERRIDE;
	
	private:
	};

	class CRecipeComponentInputDefIndexIterator : public CEconItemSpecificAttributeIterator
	{
	public:
		CRecipeComponentInputDefIndexIterator( EItemDefUniqueness_t eUniqueness );
		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef,
											  const CAttribute_DynamicRecipeComponent& value ) OVERRIDE;

		const CUtlVector< item_definition_index_t >& GetMatchingComponentInputs() const { return m_vecInputItemDefs; }

	private:

		CUtlVector< item_definition_index_t > m_vecInputItemDefs;
		EItemDefUniqueness_t m_eUniqueness;
	};

	const CUtlVector<CBaseRecipeComponent*>& GetComponents() const { return m_vecComponents; }

private:

	// All the different components for this recipe
	CUtlVector<CBaseRecipeComponent*> m_vecComponents;
	// Errors we ecounter during initialization
	CUtlVector<CUtlString> m_vecErrors;
};

class CWorldItemPlacementAttributeIterator : public CEconItemSpecificAttributeIterator
{
public:
	CWorldItemPlacementAttributeIterator() {}

	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_WorldItemPlacement &value ) OVERRIDE
	{
		Assert( pAttrDef );
		if ( pAttrDef )
		{
			m_vecPlacementAttributes.AddToTail( pAttrDef );
		}

		return true;
	}

	const CUtlVector< const CEconItemAttributeDefinition* > &GetPlacementAttributes( void ) const { return m_vecPlacementAttributes; }
private:

	CUtlVector< const CEconItemAttributeDefinition* > m_vecPlacementAttributes;
};

//---------------------------------------------------------------------------------------
// Purpose: Turns valid target items in to strange
//---------------------------------------------------------------------------------------
class CEconTool_Xifier : public IEconTool
{
public:
	CEconTool_Xifier( const char *pszTypeName, const char *pszUseString, item_capabilities_t unCapabilities, KeyValues *pUsageKV )
		: IEconTool( pszTypeName, pszUseString, NULL, unCapabilities ) 
		, m_RequiredTags( pUsageKV ? pUsageKV->FindKey( "required_tags" ) : NULL )
		, m_ItemRarityRestriction( k_unItemRarity_Any )
	{
		if ( pUsageKV )
		{
			KeyValues *pKVItemDefRestrictions = pUsageKV->FindKey( "itemdef_restrictions" );
			if ( pKVItemDefRestrictions )
			{
				FOR_EACH_SUBKEY( pKVItemDefRestrictions, pKVTag )
				{
					m_ItemDefTargetRestrictions.AddToTail( atoi(pKVTag->GetName()) );
				}
			}

			m_ItemRarityRestriction = pUsageKV->GetInt( "itemrarity_restrictions", k_unItemRarity_Any );
		}

		m_sItemDescLocToken = pUsageKV ? pUsageKV->GetString( "item_desc_tool_target", "" ) : "";
	}

	virtual bool CanApplyTo( const IEconItemInterface *pTool, const IEconItemInterface *pToolSubject ) const;

	const char *GetItemDescToolTargetLocToken() const { return m_sItemDescLocToken.String(); }


#ifdef CLIENT_DLL
	virtual bool ShouldDisplayAsUseableOnItemsInArmory() const OVERRIDE { return false; }
	virtual void OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL

protected:

	uint8 GetRarityRestriction() const { return m_ItemRarityRestriction; }

private:
	bool ItemDefMatch( const CEconItemDefinition* pTargetItemDef, const CEconItemDefinition* pSubjectItemDef ) const;

	CUtlString m_sItemDescLocToken;
	CEconTool_TagsList m_RequiredTags;
	CUtlVector<item_definition_index_t> m_ItemDefTargetRestrictions;
	uint8 m_ItemRarityRestriction;
};

class CEconTool_Strangifier : public CEconTool_Xifier
{
public:
	CEconTool_Strangifier( const char *pszTypeName, const char *pszUseString, item_capabilities_t unCapabilities, KeyValues *pUsageKV )
		: CEconTool_Xifier( pszTypeName, pszUseString, unCapabilities, pUsageKV ) {}

	virtual bool CanApplyTo( const IEconItemInterface *pTool, const IEconItemInterface *pToolSubject ) const;


#ifdef CLIENT_DLL
	virtual void OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL

};
//---------------------------------------------------------------------------------------
class CEconTool_KillStreakifier : public CEconTool_Xifier
{
public:
	CEconTool_KillStreakifier( const char *pszTypeName, const char *pszUseString, item_capabilities_t unCapabilities, KeyValues *pUsageKV )
		: CEconTool_Xifier( pszTypeName, pszUseString, unCapabilities, pUsageKV ) {}

	virtual bool CanApplyTo( const IEconItemInterface *pTool, const IEconItemInterface *pToolSubject ) const;


#ifdef CLIENT_DLL
	virtual void OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL
};
//---------------------------------------------------------------------------------------
class CEconTool_Festivizer : public CEconTool_Xifier
{
public:
	CEconTool_Festivizer( const char *pszTypeName, const char *pszUseString, item_capabilities_t unCapabilities, KeyValues *pUsageKV )
		: CEconTool_Xifier( pszTypeName, pszUseString, unCapabilities, pUsageKV )
	{
	}

	virtual bool CanApplyTo( const IEconItemInterface *pTool, const IEconItemInterface *pToolSubject ) const;


#ifdef CLIENT_DLL
	virtual void OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL
};

//---------------------------------------------------------------------------------------
class CEconTool_Unusualifier : public CEconTool_Xifier
{
public:
	CEconTool_Unusualifier( const char *pszTypeName, const char *pszUseString, item_capabilities_t unCapabilities, KeyValues *pUsageKV )
		: CEconTool_Xifier( pszTypeName, pszUseString, unCapabilities, pUsageKV ) {}

	virtual bool CanApplyTo( const IEconItemInterface *pTool, const IEconItemInterface *pToolSubject ) const;


#ifdef CLIENT_DLL
	virtual void OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL
};

//---------------------------------------------------------------------------------------
// Eats an item to give it charges
//---------------------------------------------------------------------------------------
class CEconTool_ItemEaterRecharger: public IEconTool
{
public:
	CEconTool_ItemEaterRecharger( const char *pszTypeName, const char *pszUseString, item_capabilities_t unCapabilities, KeyValues *pUsageKV )
		: IEconTool( pszTypeName, pszUseString, NULL, unCapabilities ) 
		, m_RequiredTags( pUsageKV ? pUsageKV->FindKey( "required_tags" ) : NULL )
	{
		if ( pUsageKV )
		{
			KeyValues *pKVItemDefRestrictions = pUsageKV->FindKey( "itemdef_restrictions" );

			if ( pKVItemDefRestrictions )
			{
				FOR_EACH_SUBKEY( pKVItemDefRestrictions, pKVTag )
				{
					m_ItemDefTargetRestrictions.AddToTail( atoi(pKVTag->GetName()) );
					m_ItemDefTargetChargeValues.AddToTail( pKVItemDefRestrictions->GetInt( pKVTag->GetName(), 0 ) );
				}
			}
		}
	}

	int GetChargesForItemDefId ( item_definition_index_t defIndex ) const;

	virtual bool CanApplyTo( const IEconItemInterface *pTool, const IEconItemInterface *pToolSubject ) const;

#ifdef CLIENT_DLL
	virtual bool ShouldDisplayAsUseableOnItemsInArmory() const OVERRIDE { return false; }
	virtual void OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const;
	virtual void OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL


private:
	CEconTool_TagsList m_RequiredTags;
	CUtlVector<item_definition_index_t> m_ItemDefTargetRestrictions;
	CUtlVector<int> m_ItemDefTargetChargeValues;
};
//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_UpgradeCard : public IEconTool
{
public:
	struct upgrade_card_attr_value_t
	{
		const CEconItemAttributeDefinition *m_pAttrDef; 
		attrib_value_t m_value;
	};

	typedef CUtlVectorFixedGrowable<upgrade_card_attr_value_t, 1>	UpgradeCardAttributeVec_t;

	CEconTool_UpgradeCard( const char *pszTypeName, const char *pszUseString, item_capabilities_t unCapabilities, KeyValues *pUsageKV )
		: IEconTool( pszTypeName, pszUseString, NULL, unCapabilities ) 
		, m_RequiredTags( pUsageKV ? pUsageKV->FindKey( "required_tags" ) : NULL )
	{
		COMPILE_TIME_ASSERT( sizeof( attrib_value_t ) == sizeof( uint32 ) );
		COMPILE_TIME_ASSERT( sizeof( attrib_value_t ) == sizeof( float ) );

		if ( pUsageKV )
		{
			KeyValues *pAttributesKV = pUsageKV->FindKey( "attributes" );
			if ( pAttributesKV )
			{
				FOR_EACH_SUBKEY( pAttributesKV, pAttrKV )
				{
					const char *pszAttributeName = pAttrKV->GetName();
					const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinitionByName( pszAttributeName );

					// Kyle says: this is bad, dumb code, and more importantly it's bad dumb code that doesn't
					//			  make any sense here, way down inside the "parse a tool" function.
					attrib_value_t value;

					const bool bParseAsFloat = pAttrDef && pAttrDef->IsStoredAsFloat();
					if ( bParseAsFloat )
					{
						*(float *)&value = pAttrKV->GetFloat();
					}
					else
					{
						*(uint32 *)&value = pAttrKV->GetInt();
					}

					// Add this attribute to our list. Adding a NULL pointer is safe here. We'll use that to check
					// later in BFinishInitialization() whether we had a successful init or not.
					upgrade_card_attr_value_t attrValue = { pAttrDef, value };
					m_vecAttributes.AddToTail( attrValue );
				}
			}
		}
	}

	virtual bool BFinishInitialization() OVERRIDE
	{
		// Make sure we didn't fail to find any attributes.
		FOR_EACH_VEC( m_vecAttributes, i )
		{
			if ( m_vecAttributes[i].m_pAttrDef == NULL )
				return false;
		}

		// Make sure we have a non-zero number of attributes. If we don't have at least one, applicable would be
		// a nonsensical action.
		return m_vecAttributes.Count() > 0
			&& IEconTool::BFinishInitialization();
	}

	const UpgradeCardAttributeVec_t& GetAttributes() const { return m_vecAttributes; }

	virtual bool CanApplyTo( const IEconItemInterface *pTool, const IEconItemInterface *pToolSubject ) const OVERRIDE;

#ifdef CLIENT_DLL
	virtual bool ShouldDisplayAsUseableOnItemsInArmory() const OVERRIDE { return false; }

	virtual void OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const OVERRIDE;
#endif // CLIENT_DLL
 
private:
	CEconTool_TagsList m_RequiredTags;
	UpgradeCardAttributeVec_t m_vecAttributes;
};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_ClassTransmogrifier : public IEconTool
{
public:
	CEconTool_ClassTransmogrifier( const char *pszTypeName, const char *pszUseString, item_capabilities_t unCapabilities, KeyValues *pUsageKV )
		: IEconTool( pszTypeName, pszUseString, NULL, unCapabilities ) 
		, m_RequiredTags( pUsageKV ? pUsageKV->FindKey( "required_tags" ) : NULL )
		, m_iClass( -1 )
	{
		KeyValues *pKVOutputClass = pUsageKV->FindKey( "output_class" );
		if ( pKVOutputClass )
		{
			m_iClass = StringFieldToInt( pKVOutputClass->GetString( "" ), GetItemSchema()->GetClassUsabilityStrings() );
		}
	}

	virtual bool BFinishInitialization() OVERRIDE
	{
		return m_iClass > 0
			&& m_iClass < LOADOUT_COUNT
			&& IEconTool::BFinishInitialization();
	}

	int GetOutputClass() const { return m_iClass; }
	const CEconTool_TagsList& GetRequiredTags() const { return m_RequiredTags; }

	virtual bool CanApplyTo( const IEconItemInterface *pTool, const IEconItemInterface *pToolSubject ) const;

#ifdef CLIENT_DLL
	virtual bool ShouldDisplayAsUseableOnItemsInArmory() const OVERRIDE { return false; }

	virtual void OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL

private:
	CEconTool_TagsList m_RequiredTags;			// required for both the input item and the output item
	int m_iClass;
};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_BackpackExpander : public IEconTool
{
public:
	CEconTool_BackpackExpander ( const char *pszTypeName, const char *pszUseString, KeyValues *pUsageKV )
		: IEconTool( pszTypeName, pszUseString, NULL, ITEM_CAP_NONE )
		, m_iBackpackSlots( 0 )
	{
		if ( pUsageKV )
		{
			m_iBackpackSlots = pUsageKV->GetInt( "backpack_slots", 0 );
		}
	}

	virtual bool BFinishInitialization() OVERRIDE
	{
		return m_iBackpackSlots > 0
			&& IEconTool::BFinishInitialization();
	}

	int GetBackpackSlots() const { return m_iBackpackSlots; }

#ifdef CLIENT_DLL
	virtual void OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const OVERRIDE;
#endif // CLIENT_DLL


private:
	int m_iBackpackSlots;
};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_AccountUpgradeToPremium : public IEconTool
{
public:
	CEconTool_AccountUpgradeToPremium( const char *pszTypeName, const char *pszUseString ) : IEconTool( pszTypeName, pszUseString, NULL, ITEM_CAP_NONE ) { }

#ifdef CLIENT_DLL
	virtual void OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL

};

//---------------------------------------------------------------------------------------
class CEconTool_DuckToken: public IEconTool
{
public:
	CEconTool_DuckToken( const char *pszTypeName, item_capabilities_t unCapabilities ) : IEconTool( pszTypeName, NULL, NULL, unCapabilities ) { }

	virtual bool CanApplyTo( const IEconItemInterface *pTool, const IEconItemInterface *pToolSubject ) const;

#ifdef CLIENT_DLL
	virtual void OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const;
	virtual void OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL

};

//---------------------------------------------------------------------------------------
class CEconTool_GrantOperationPass : public IEconTool
{
public:
	CEconTool_GrantOperationPass( const char *pszTypeName, const char *pszUseString, item_capabilities_t unCapabilities, KeyValues *pUsageKV ) : IEconTool( pszTypeName, pszUseString, NULL, ITEM_CAP_NONE )
	{ 
		m_pOperationPassName = NULL;
		m_pOptionalBonusLootList = NULL;
		if ( pUsageKV )
		{
			// Find the Item
			m_pOperationPassName = pUsageKV->GetString( "operation_pass", NULL );
			m_pOptionalBonusLootList = pUsageKV->GetString( "bonus_lootlist", NULL );
		}
	}

#ifdef CLIENT_DLL
	virtual void OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL

	
	const char *m_pOperationPassName;
	const char *m_pOptionalBonusLootList;
};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_ClaimCode : public IEconTool
{
public:
	CEconTool_ClaimCode ( const char *pszTypeName, const char *pszUseString, KeyValues *pUsageKV )
		: IEconTool( pszTypeName, pszUseString, NULL, ITEM_CAP_NONE )
		, m_pszClaimType( NULL )
	{
		if ( pUsageKV )
		{
			m_pszClaimType = pUsageKV->GetString( "claim_type", NULL );
		}
	}

	virtual bool BFinishInitialization() OVERRIDE
	{
		return m_pszClaimType
			&& IEconTool::BFinishInitialization();
	}

	const char *GetClaimType() const { return m_pszClaimType; }

#ifdef CLIENT_DLL
	virtual void OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL


private:
	const char *m_pszClaimType;
};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
enum EGiftTargetRule
{
	kGiftTargetRule_OnlyOthers = 0,
	kGiftTargetRule_OnlySelf = 1,
};

class CEconTool_Gift : public IEconTool
{
public:
	CEconTool_Gift ( const char *pszTypeName, const char *pszUseString, KeyValues *pUsageKV )
		: IEconTool( pszTypeName, pszUseString, NULL, ITEM_CAP_NONE )
		, m_pszLootListName( NULL )
		, m_iMaxRecipients( 0 )
		, m_eTargetRule( kGiftTargetRule_OnlySelf )
	{
		if ( pUsageKV )
		{
			m_pszLootListName = pUsageKV->GetString( "loot_list", NULL );
			m_iMaxRecipients = pUsageKV->GetInt( "max_recipients", 0 );
			m_eTargetRule = !Q_stricmp( pUsageKV->GetString( "target_rule", "only_others" ), "only_self" )
						  ? kGiftTargetRule_OnlySelf
						  : kGiftTargetRule_OnlyOthers;
		}
	}

	virtual bool BFinishInitialization() OVERRIDE
	{
		return m_pszLootListName
			&& GetItemSchema()->GetLootListByName( m_pszLootListName )
			&& m_iMaxRecipients > 0
			&& IEconTool::BFinishInitialization();
	}

	const char	   *GetLootListName() const		{ return m_pszLootListName; }
	int				GetMaxRecipients() const	{ return m_iMaxRecipients; }
	EGiftTargetRule GetTargetRule() const		{ return m_eTargetRule; }

#ifdef CLIENT_DLL
	virtual void OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL


private:
	const char *m_pszLootListName;
	int m_iMaxRecipients;
	EGiftTargetRule m_eTargetRule;
};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_PaintCan : public IEconTool
{
public:
	CEconTool_PaintCan( const char *pszTypeName, item_capabilities_t unCapabilities ) : IEconTool( pszTypeName, NULL, NULL, unCapabilities ) { }

#ifdef CLIENT_DLL
	virtual void OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const;
	virtual void OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL
};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_GiftWrap : public IEconTool
{
public:
	CEconTool_GiftWrap( const char *pszTypeName, const char *pszUseString, item_capabilities_t unCapabilities, KeyValues *pUsageKV );

	virtual bool BFinishInitialization() OVERRIDE;
	virtual bool CanApplyTo( const IEconItemInterface *pTool, const IEconItemInterface *pToolSubject ) const;
	virtual bool RequiresToolEscrowPeriod() const { return false; }

	const CEconItemDefinition *GetWrappedItemDefinition() const { Assert( m_pWrappedGiftItemDef ); return m_pWrappedGiftItemDef; }

#ifdef CLIENT_DLL
	virtual void OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL

private:
	const char *m_pszWrappedGiftItemDefName;			// points to memory inside our init KV -- only valid between the constructor call and the BFinishInitialization() call (this is messy but Fletcher and I agree it makes more sense than switching to a full two-pass schema parse just for this)
	const CEconItemDefinition *m_pWrappedGiftItemDef;
};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_NameTag : public IEconTool
{
public:
	CEconTool_NameTag( const char *pszTypeName, item_capabilities_t unCapabilities ) : IEconTool( pszTypeName, NULL, NULL, unCapabilities ) { }

#ifdef CLIENT_DLL
	virtual void OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL
};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_DescTag : public IEconTool
{
public:
	CEconTool_DescTag( const char *pszTypeName, item_capabilities_t unCapabilities ) : IEconTool( pszTypeName, NULL, NULL, unCapabilities ) { }

#ifdef CLIENT_DLL
	virtual void OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL
};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_CustomizeTexture : public IEconTool
{
public:
	CEconTool_CustomizeTexture( const char *pszTypeName, item_capabilities_t unCapabilities ) : IEconTool( pszTypeName, NULL, NULL, unCapabilities ) { }

#ifdef CLIENT_DLL
	virtual void OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL
};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_CrateKey : public IEconTool
{
public:
	CEconTool_CrateKey( const char *pszTypeName, const char *pszUsageRestriction, item_capabilities_t unCapabilities ) : IEconTool( pszTypeName, NULL, pszUsageRestriction, unCapabilities ) { }

#ifdef CLIENT_DLL
	virtual void OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL
};

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_KeylessCase : public IEconTool
{
public:
	CEconTool_KeylessCase( const char *pszTypeName, const char *pszUseString ) : IEconTool( pszTypeName, pszUseString, NULL, ITEM_CAP_NONE ) { }

#ifdef CLIENT_DLL
	virtual void OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL
};

//---------------------------------------------------------------------------------------
class CEconTool_PaintKit : public IEconTool
{
public:
	CEconTool_PaintKit( const char *pszTypeName, const char *pszUseString, item_capabilities_t unCapabilities )
		: IEconTool( pszTypeName, pszUseString, NULL, unCapabilities ) {}

#ifdef CLIENT_DLL
	virtual void OnClientUseConsumable( class C_EconItemView *pItem, vgui::Panel *pParent ) const OVERRIDE;
	virtual bool ShouldDisplayAsUseableOnItemsInArmory() const OVERRIDE { return false; }
#endif // CLIENT_DLL
};


//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
class CEconTool_Default : public IEconTool
{
public:
	CEconTool_Default( const char *pszTypeName, const char *pszUseString, const char *pszUsageRestriction, item_capabilities_t unCapabilities )
		: IEconTool( pszTypeName, pszUseString, pszUsageRestriction, unCapabilities )
	{
		Assert( pszTypeName );
	}

#ifdef CLIENT_DLL
	virtual void OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const;
#endif // CLIENT_DLL
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

class CCountUserGeneratedAttributeIterator : public IEconItemUntypedAttributeIterator
{
public:
	CCountUserGeneratedAttributeIterator() : m_iCount( 0 ) { }

	virtual bool OnIterateAttributeValueUntyped( const CEconItemAttributeDefinition *pAttrDef ) OVERRIDE
	{
		if ( pAttrDef->GetUserGenerationType() != 0 )
		{
			m_iCount++;
		}

		return true;
	}
	
	int GetCount() const { return m_iCount; }

private:
	int m_iCount;
};

#endif // ECONITEMTOOLS_H
