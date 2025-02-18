//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CItemSelectionCriteria, which serves as a criteria for selection
//			of a econ item
//
//=============================================================================

#ifndef ITEM_SELECTION_CRITERIA_H
#define ITEM_SELECTION_CRITERIA_H
#ifdef _WIN32
#pragma once
#endif

// Maximum string length in item create APIs
const int k_cchCreateItemLen	= 64;	

// Operators for BAddNewItemCriteria
enum EItemCriteriaOperator
{
	k_EOperator_String_EQ = 0,				// Field is string equal to value
	k_EOperator_Not = 1,					// Logical not
	k_EOperator_String_Not_EQ = 1,			// Field is not string equal to value
	k_EOperator_Float_EQ = 2,				// Field as a float is equal to value
	k_EOperator_Float_Not_EQ = 3,			// Field as a float is not equal to value
	k_EOperator_Float_LT = 4,				// Field as a float is less than value
	k_EOperator_Float_Not_LT = 5,			// Field as a float is not less than value
	k_EOperator_Float_LTE = 6,				// Field as a float is less than or equal value
	k_EOperator_Float_Not_LTE = 7,			// Field as a float is not less than or equal value
	k_EOperator_Float_GT = 8,				// Field as a float is greater than value
	k_EOperator_Float_Not_GT = 9,			// Field as a float is not greater than value
	k_EOperator_Float_GTE = 10,				// Field as a float is greater than or equal value
	k_EOperator_Float_Not_GTE = 11,			// Field as a float is not greater than or equal value
	k_EOperator_Subkey_Contains = 12,		// Field contains value as a subkey
	k_EOperator_Subkey_Not_Contains = 13,	// Field does not contain value as a subkey

	// Must be last
	k_EItemCriteriaOperator_Count = 14,
};


EItemCriteriaOperator EItemCriteriaOperatorFromName( const char *pch );
const char *PchNameFromEItemCriteriaOperator( int eItemCriteriaOperator );

class CEconItemSchema;
class CEconItemDefinition;
class CSOItemCriteria;
class CSOItemCriteriaCondition;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// CItemSelectionCriteria
// A class that contains all the conditions a server needs to specify what
// kind of random item they wish to generate.
//-----------------------------------------------------------------------------
class CItemSelectionCriteria
{
public:
	// Constructors and destructor
	CItemSelectionCriteria() : 
	  m_bItemLevelSet( false ),
		  m_unItemLevel( 0 ),
		  m_bQualitySet( false ),
		  m_nItemQuality( k_unItemQuality_Any ), 
		  m_unInitialInventory( 0 ),
		  m_bInitialQuantitySet( false ),
		  m_unInitialQuantity( 1 ),
		  m_bIgnoreEnabledFlag( false ),
		  m_unEquipRegionMask( 0 )
	  { 
	  }

	  CItemSelectionCriteria( const CItemSelectionCriteria &that );
	  CItemSelectionCriteria &operator=( const CItemSelectionCriteria& rhs );
	  ~CItemSelectionCriteria();

	  // Accessors and Settors
	  bool			BItemLevelSet( void ) const					{ return m_bItemLevelSet; }
	  uint32		GetItemLevel( void ) const					{ Assert( m_bItemLevelSet ); return m_unItemLevel; }
	  void			SetItemLevel( uint32 unLevel )				{ m_unItemLevel = unLevel; m_bItemLevelSet = true; }
	  bool			BQualitySet( void ) const					{ return m_bQualitySet; }
	  int32			GetQuality( void ) const					{ Assert( m_bQualitySet ); return m_nItemQuality; }
	  void			SetQuality( int32 nQuality )				{ m_nItemQuality = nQuality; m_bQualitySet = true; }
	  uint32		GetInitialInventory( void ) const			{ return m_unInitialInventory; }
	  void			SetInitialInventory( uint32 unInventory )	{ m_unInitialInventory = unInventory; }
	  bool			BInitialQuantitySet( void ) const			{ return m_bQualitySet; }
	  uint32		GetInitialQuantity( void ) const			{ Assert( m_bQualitySet ); return m_unInitialQuantity; }
	  void			SetInitialQuantity( uint32 unQuantity )		{ m_unInitialQuantity = unQuantity; m_bInitialQuantitySet = true; }
	  void			SetIgnoreEnabledFlag( bool bIgnore )		{ m_bIgnoreEnabledFlag = bIgnore; }

	  // Tags
	  void			SetTags( const char *pszTags );

	  // equip regions
	  void			SetEquipRegions( const char *pszEquipRegions );

	  // Add conditions to the criteria
	  class ICondition
	  {
		public:
			virtual ~ICondition() { }

			virtual bool BItemDefinitionPassesCriteria( const CEconItemDefinition *pItemDef ) const = 0;

			virtual EItemCriteriaOperator GetEOp() const { return k_EItemCriteriaOperator_Count; }
			virtual const char *GetField() const { return ""; }
			virtual const char *GetValue() const { return ""; }

			virtual bool BSerializeToMsg( CSOItemCriteriaCondition & msg ) const { Assert( !"BSerializeToMsg() called on for unimplementing ICondition!" ); return false; }
	  };

	  bool			BAddCondition( const char *pszField, EItemCriteriaOperator eOp, float flValue, bool bRequired );
	  bool			BAddCondition( const char *pszField, EItemCriteriaOperator eOp, const char * pszValue, bool bRequired );
	  bool			BAddCondition( ICondition *pCondition );
	  int			GetConditionsCount() { return m_vecConditions.Count(); }
	  const char	*GetValueForFirstConditionOfType( EItemCriteriaOperator eType ) const;
	  const char	*GetFieldForFirstConditionOfType( EItemCriteriaOperator eType ) const;

	  // Alternate ways of initializing
	  bool			BInitFromKV( KeyValues *pKVCriteria );

	  // Serializes the criteria to and from messages
	  bool			BSerializeToMsg( CSOItemCriteria & msg ) const;
	  bool			BDeserializeFromMsg( const CSOItemCriteria & msg );

	  // Evaluates an item definition against this criteria. Returns true if 
	  // the definition passes the filter
	  bool			BEvaluate( const CEconItemDefinition* pItemDef ) const;

	  // Validation
#ifdef DBGFLAG_VALIDATE
	  void Validate( CValidator &validator, const char *pchName );
#endif

private:
	//-----------------------------------------------------------------------------
	// CItemSelectionCriteria::CCondition
	// Represents one condition of the criteria
	//-----------------------------------------------------------------------------
	class CCondition : public ICondition
	{
	public:
		CCondition( const char *pszField, EItemCriteriaOperator eOp, bool bRequired )
			: m_sField( pszField ), m_EOp( eOp ), m_bRequired( bRequired )
		{
		}

		// ICondition interface.
		virtual bool BItemDefinitionPassesCriteria( const CEconItemDefinition *pItemDef ) const OVERRIDE;

		// Serializes the condition to the message
		virtual bool BSerializeToMsg( CSOItemCriteriaCondition & msg ) const;

		// Validation
#ifdef DBGFLAG_VALIDATE
		virtual void Validate( CValidator &validator, const char *pchName );
#endif

		EItemCriteriaOperator	GetEOp( void ) const OVERRIDE { return m_EOp; }
		virtual	const char		*GetField( void ) const OVERRIDE  { return m_sField.Get(); }
		virtual	const char		*GetValue( void ) const OVERRIDE  { Assert(0); return NULL; }

	private:
		// Returns if the given KeyValues block passes this condition 
		// Performs common checks and calls BInternalEvaluate
		bool BEvaluate( KeyValues *pKVItem ) const;

	protected:
		// Returns true if applying the element's operator on m_sField of
		// pKVItem returns true. This is only called if m_pszField exists in pKVItem
		virtual bool BInternalEvaluate( KeyValues *pKVItem ) const = 0;

		// The field of the raw KeyValue form of the item definition to check
		CUtlString				m_sField;
		// The operator this clause uses
		EItemCriteriaOperator	m_EOp;
		// When true, BEvaluate returns false if m_sField does not exist in pKVItem
		bool					m_bRequired;
	};


	//-----------------------------------------------------------------------------
	// CItemSelectionCriteria::CStringCondition
	// CCondition that handles the string-based operators
	//-----------------------------------------------------------------------------
	class CStringCondition : public CCondition
	{
	public:
		CStringCondition( const char *pszField, EItemCriteriaOperator eOp, const char *pszValue, bool bRequired )
			: CCondition( pszField, eOp, bRequired ), m_sValue( pszValue )
		{
		}

		virtual ~CStringCondition( ) { }

		virtual	const char		*GetValue( void ) const OVERRIDE { return m_sValue.Get(); }

		// Validation
#ifdef DBGFLAG_VALIDATE
		virtual void Validate( CValidator &validator, const char *pchName );
#endif

	protected:
		virtual bool BInternalEvaluate( KeyValues *pKVItem ) const;
		virtual bool BSerializeToMsg( CSOItemCriteriaCondition & msg ) const;

		// The value to check against
		CUtlString		m_sValue;
	};


	//-----------------------------------------------------------------------------
	// CItemSelectionCriteria::CFloatCondition
	// CCondition that handles the float-based operators
	//-----------------------------------------------------------------------------
	class CFloatCondition : public CCondition
	{
	public:
		CFloatCondition( const char *pszField, EItemCriteriaOperator eOp, float flValue, bool bRequired )
			: CCondition( pszField, eOp, bRequired ), m_flValue( flValue )
		{
		}

		virtual ~CFloatCondition( ) { }

	protected:
		virtual bool BInternalEvaluate( KeyValues *pKVItem ) const;
		virtual bool BSerializeToMsg( CSOItemCriteriaCondition & msg ) const;

		// The value to check against
		float				m_flValue;
	};


	//-----------------------------------------------------------------------------
	// CItemSelectionCriteria::CSetCondition
	// CCondition that handles subkey checks
	//-----------------------------------------------------------------------------
	class CSetCondition : public CCondition
	{
	public:
		CSetCondition( const char *pszField, EItemCriteriaOperator eOp, const char *pszValue, bool bRequired )
			: CCondition( pszField, eOp, bRequired ), m_sValue( pszValue )
		{
		}

		virtual ~CSetCondition( ) { }

		// Validation
#ifdef DBGFLAG_VALIDATE
		virtual void Validate( CValidator &validator, const char *pchName );
#endif

	protected:
		virtual bool BInternalEvaluate( KeyValues *pKVItem ) const;

		virtual bool BSerializeToMsg( CSOItemCriteriaCondition & msg ) const;

		// The subkey to look for
		CUtlString			m_sValue;
	};

	// True if item level is specified in this criteria
	bool			m_bItemLevelSet;
	// The level of the item to generate
	uint32			m_unItemLevel;
	// True if quality is specified in this criteria
	bool			m_bQualitySet;
	// The quality of the item to generate
	int32			m_nItemQuality;
	// The initial inventory token of the item
	uint32			m_unInitialInventory;
	// True if initial quantity is specified in this criteria.
	bool			m_bInitialQuantitySet;
	// The initial quantity of the item
	uint32			m_unInitialQuantity;
	// Enforced explicit quality matching
	bool			m_bForcedQualityMatch;
	// Ignoring enabled flag (used when crafting)
	bool			m_bIgnoreEnabledFlag;

	// A list of tags
	CUtlString		m_strTags;
	CUtlVector<econ_tag_handle_t>	m_vecTags;

	// list of equip regions
	CUtlString		m_strEquipRegions;
	equip_region_mask_t	m_unEquipRegionMask;			// which equip regions does this item cover directly

	// A list of the conditions
	CUtlVector<ICondition *>	m_vecConditions;
};


#endif //ITEM_SELECTION_CRITERIA_H
