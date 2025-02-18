//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#ifndef TFITEMSCHEMA_H
#define TFITEMSCHEMA_H
#ifdef _WIN32
#pragma once
#endif

#include "econ_item_schema.h"
#include "tf_item_constants.h"
#include "tf_shareddefs.h"
#include "tf_matchmaking_shared.h"

#include "util_shared.h"

class CQuestMapDefinition;
class CQuestMapNodeDefinition;
class CQuest;
class CQuestThemeDefinition;
class CQuestObjectiveConditionsDefinition;
class CQuestDefinition;

const int k_iMvmMissionIndex_Any = -1;
const int k_iMvmMissionIndex_NotInSchema = -2;

//#ifndef STAGING_ONLY
#define USE_MVM_TOUR 1
//#endif // !STAGING_ONLY

const int k_iMvmTourIndex_Empty = -1; // empty tour name
const int k_iMvmTourIndex_NotInSchema = -2;
const int k_iMvmTourIndex_NotMannedUp = -3; // special value used when asking for the selected tour when not manned up

const uint32 k_unMvMMaxPointsPerBadgeLevel = 3; // require 3 missions to level up a badge

class CRandomChanceString
{
public:
	CRandomChanceString();

	void AddString( const char *pszString, int nChance );
	const char *GetRandomString() const;

//private:
	CUtlVector< std::pair< const char *, int > > m_vecChoices;
	int			m_unTotalChance;
};

class CTFTauntInfo
{
public:
	CTFTauntInfo();

	bool BInitFromKV( KeyValues *pKV, CUtlVector<CUtlString> *pVecErrors );

	int GetIntroSceneCount( int iClass ) const { Assert( iClass >= 0 && iClass < LOADOUT_COUNT ); return m_vecIntroScenes[iClass].Count(); }
	const char *GetIntroScene( int iClass, int iSceneIndex ) const
	{
		Assert( iSceneIndex >= 0 && iSceneIndex < GetIntroSceneCount( iClass ) );
		return m_vecIntroScenes[iClass][iSceneIndex];
	}

	int GetOutroSceneCount( int iClass ) const { Assert( iClass >= 0 && iClass < LOADOUT_COUNT ); return m_vecOutroScenes[iClass].Count(); }
	const char *GetOutroScene( int iClass, int iSceneIndex ) const
	{
		Assert( iSceneIndex >= 0 && iSceneIndex < GetOutroSceneCount( iClass ) );
		return m_vecOutroScenes[iClass][iSceneIndex];
	}

	int GetPartnerTauntInitiatorSceneCount( int iClass ) const { Assert( iClass >= 0 && iClass < LOADOUT_COUNT ); return m_vecPartnerTauntInitiatorScenes[iClass].Count(); }
	const char *GetPartnerTauntInitiatorScene( int iClass, int iSceneIndex ) const
	{
		Assert( iSceneIndex >= 0 && iSceneIndex < GetPartnerTauntInitiatorSceneCount( iClass ) );
		return m_vecPartnerTauntInitiatorScenes[iClass][iSceneIndex];
	}

	int GetPartnerTauntReceiverSceneCount( int iClass ) const { Assert( iClass >= 0 && iClass < LOADOUT_COUNT ); return m_vecPartnerTauntReceiverScenes[iClass].Count(); }
	const char *GetPartnerTauntReceiverScene( int iClass, int iSceneIndex ) const
	{
		Assert( iSceneIndex >= 0 && iSceneIndex < GetPartnerTauntReceiverSceneCount( iClass ) );
		return m_vecPartnerTauntReceiverScenes[iClass][iSceneIndex];
	}
	
	const char *GetProp( int iClass ) const { Assert( iClass >= 0 && iClass < LOADOUT_COUNT ); return m_pszProp[iClass]; }
	const char *GetPropIntroScene( int iClass ) const { Assert( iClass >= 0 && iClass < LOADOUT_COUNT ); return m_pszPropIntroScene[iClass]; }
	const char *GetPropOutroScene( int iClass ) const { Assert( iClass >= 0 && iClass < LOADOUT_COUNT ); return m_pszPropOutroScene[iClass]; }
	
	float		GetTauntSeparationForwardDistance() const { return m_flTauntSeparationForwardDistance; }
	float		GetTauntSeparationRightDistance() const { return m_flTauntSeparationRightDistance; }
	float		GetMinTauntTime() const { return m_flMinTauntTime; }

	bool		IsPartnerTaunt() const { return m_bIsPartnerTaunt; }
	bool		ShouldStopTauntIfMoved() const { return m_bStopTauntIfMoved; }

	int			GetFOV() const { return m_nFOV; }
	float		GetCameraDist() const { return m_flCameraDist; }
	float		GetCameraDistUp() const { return m_flCameraDistUp; }

	const char	*GetParticleAttachment() const { return m_pszParticleAttachment; }

	struct TauntInputRemap_t
	{
		TauntInputRemap_t()
		{
			m_iButton = 0;
		}
		int m_iButton;
		CUtlVector< const char* > m_vecButtonPressedScenes[LOADOUT_COUNT];
		CUtlVector< const char* > m_vecButtonReleasedScenes[LOADOUT_COUNT];
	};
	int GetTauntInputRemapCount() const { return m_vecTauntInputRemap.Count(); }
	const TauntInputRemap_t &GetTauntInputRemapScene( int iButtonIndex ) const
	{
		return m_vecTauntInputRemap[iButtonIndex];
	}

	int GetTauntPropInputRemapCount() const { return m_vecTauntPropInputRemap.Count(); }
	const TauntInputRemap_t &GetTauntPropInputRemapScene( int iButtonIndex ) const
	{
		return m_vecTauntPropInputRemap[ iButtonIndex ];
	}

private:

	bool InitTauntInputRemap( KeyValues *pKV, CUtlVector<TauntInputRemap_t>( &outputArray ), CUtlVector<CUtlString> *pVecErrors );

	CUtlVector< const char* >	m_vecIntroScenes[LOADOUT_COUNT];
	CUtlVector< const char* >	m_vecOutroScenes[LOADOUT_COUNT];
	CUtlVector< const char* >	m_vecPartnerTauntInitiatorScenes[LOADOUT_COUNT];
	CUtlVector< const char* >	m_vecPartnerTauntReceiverScenes[LOADOUT_COUNT];
	CUtlVector< TauntInputRemap_t >	m_vecTauntInputRemap;
	CUtlVector< TauntInputRemap_t >	m_vecTauntPropInputRemap;
	const char		*m_pszProp[LOADOUT_COUNT];
	const char		*m_pszPropIntroScene[LOADOUT_COUNT];
	const char		*m_pszPropOutroScene[LOADOUT_COUNT];
	const char		*m_pszParticleAttachment;
	float			m_flTauntSeparationForwardDistance;
	float			m_flTauntSeparationRightDistance;
	float			m_flMinTauntTime;
	bool			m_bIsPartnerTaunt;
	bool			m_bStopTauntIfMoved;

	int				m_nFOV;
	float			m_flCameraDist;
	float			m_flCameraDistUp;
};



//-----------------------------------------------------------------------------
// CTFRequiredQuestItemsSet
//-----------------------------------------------------------------------------
class CTFRequiredQuestItemsSet
{
public:
	CTFRequiredQuestItemsSet( void ) {}

	bool BInitFromKV( KeyValues *pKV, CUtlVector<CUtlString> *pVecErrors = NULL );
	bool BPostInit( CUtlVector<CUtlString> *pVecErrors = NULL );
	
	bool OwnsRequiredItems( const CUtlVector< item_definition_index_t >& vecOwnedItemDefs ) const;
	const item_definition_index_t& GetLoanerItemDef() const { return m_LoanerItemDef; }

private:
	CUtlVector< item_definition_index_t > m_vecQualifyingItemDefs;
	item_definition_index_t m_LoanerItemDef;
};



//-----------------------------------------------------------------------------
// Wars
//-----------------------------------------------------------------------------
class CWarDefinition
{
public:

	CWarDefinition();

	bool BInitFromKV( KeyValues *pKV, CUtlVector<CUtlString> *pVecErrors );

	struct CWarSideDefinition_t
	{
		CWarSideDefinition_t() 
			: m_pszLeaderboardName( NULL )
			, m_pszLocalizedName( NULL )
			, m_nSideIndex( INVALID_WAR_SIDE )
		{}

		bool BInitFromKV( const char* pszContainingWarName, KeyValues *pKVSide, CUtlVector<CUtlString> *pVecErrors );

		const char* m_pszLocalizedName;
		const char* m_pszLeaderboardName;
		war_side_t	m_nSideIndex;
	};
	typedef CUtlMap< war_side_t, CWarSideDefinition_t > SidesMap_t;

	const SidesMap_t& GetSides() const { return m_mapSides; }
	const CWarSideDefinition_t* GetSide( war_side_t nSide ) const;
	war_definition_index_t GetDefIndex() const { return m_nDefIndex; }
	const char* GetDefName() const { return m_pszDefName; }
	bool IsActive() const;
	bool IsValidSide( war_side_t nSide ) const;
	RTime32 GetStartDate() const { return m_rtTimeStart; }
	RTime32 GetEndDate() const { return m_rtTimeEnd; }
private:

	const char* m_pszLocalizedWarname;
	const char* m_pszDefName;
	SidesMap_t m_mapSides;
	RTime32 m_rtTimeStart;
	RTime32 m_rtTimeEnd;
	war_definition_index_t m_nDefIndex;
};
typedef CUtlMap< war_definition_index_t, const CWarDefinition* > WarDefinitionMap_t;

const char *GetPlayerClassName( int iClass );
const char *GetPlayerClassLocalizationKey( int iClass );
itemid_t GetAssociatedQuestID( const IEconItemInterface *pEconItem );

class CTFItemDefinition : public CEconItemDefinition
{
public:

	CTFItemDefinition()
	{
		InternalInitialize();
	}

	~CTFItemDefinition()
	{
		if ( m_pTauntData )
		{
			delete m_pTauntData;
			m_pTauntData = NULL;
		}
	}

	// CEconItemDefinition interface.
	virtual bool	BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors = NULL ) OVERRIDE;
#if defined(CLIENT_DLL) || defined(GAME_DLL)
	virtual bool	BInitFromTestItemKVs( int iNewDefIndex, KeyValues *pKVItem, CUtlVector<CUtlString>* pVecErrors = NULL ) OVERRIDE;
	virtual void	CopyPolymorphic( const CEconItemDefinition *pSourceDef );
	virtual void	GeneratePrecacheModelStrings( bool bDynamicLoad, CUtlVector<const char *> *out_pVecModelStrings ) const;
#endif // defined(CLIENT_DLL) || defined(GAME_DLL)

	int			GetAnimSlot( void ) const			{ return m_iAnimationSlot; }

	// Class & Slot handling
	int			GetDefaultLoadoutSlot( void ) const { return m_iDefaultLoadoutSlot; }
	const CBitVec<LOADOUT_COUNT> *GetClassUsability( void ) const { return &m_vbClassUsability; }
	void		FilloutSlotUsage( CBitVec<LOADOUT_COUNT> *pBV ) const;
	bool		CanBeUsedByClass( int iClass ) const { return iClass == GEconItemSchema().GetAccountIndex() ? m_eEquipType == EQUIP_TYPE_ACCOUNT : m_vbClassUsability.IsBitSet( iClass ); }
	bool		CanBeUsedByAllClasses( void ) const;
	EEquipType_t	GetEquipType( void ) const { return m_eEquipType; }
	bool		CanBePlacedInSlot( int nSlot ) const;
	const char	*GetPlayerDisplayModel( int iClass ) const	{ Assert( iClass >= 0 && iClass < LOADOUT_COUNT ); return m_pszPlayerDisplayModel[iClass]; }
	virtual const char	*GetPlayerDisplayModelAlt( int iClass = 0 ) const	{ Assert( iClass >= 0 && iClass < LOADOUT_COUNT ); return m_pszPlayerDisplayModelAlt[iClass]; }

	int			GetLoadoutSlot( int iLoadoutClass ) const;
	bool		IsAWearable() const;
	bool		IsContentStreamable() const;
	const char* GetAdTextToken() const { return m_pszAdText; }
	const char* GetAdResFile() const { return m_pszAdResFile; }
	const CUtlVector< uint32 >& GetValidPaintkits() const;

	CTFTauntInfo *GetTauntData() const { return m_pTauntData; }

#ifdef CLIENT_DLL
	bool		HasDetailedIcon() const { return m_bHasDetailedIcon; }
	bool		CanBackpackInspect() const { return m_bCanBackpackInspect; }
#endif // CLIENT_DLL

	bool		IsChanceRestricted() const { return m_bChanceRestricted; }

private:
	void InternalInitialize();

	// The load-out slot that this item can be placed into.
	int				m_iDefaultLoadoutSlot;
	int				m_iAnimationSlot;

	// taunt item data
	CTFTauntInfo	*m_pTauntData;

	// The .mdl file used for this item when it's being carried by a player.
	const char		*m_pszPlayerDisplayModel[LOADOUT_COUNT];
	const char		*m_pszPlayerDisplayModelAlt[LOADOUT_COUNT];

	const char* m_pszAdText;
	const char* m_pszAdResFile;

	// Specifies which class can use this item.
	CBitVec<LOADOUT_COUNT> m_vbClassUsability;
	int				m_iLoadoutSlots[LOADOUT_COUNT];		// Slot that each class places the item into.
	EEquipType_t	m_eEquipType;
	bool			m_bChanceRestricted = false;

	mutable CUtlVector< uint32 > m_vecValidPaintkitDefs;
	mutable bool m_bValidPaintkitsGenerated;

#ifdef CLIENT_DLL
	bool			m_bHasDetailedIcon;
	bool			m_bCanBackpackInspect;
#endif // CLIENT_DLL
};

class CTFStyleInfo : public CEconStyleInfo
{
public:
	CTFStyleInfo()
	{
		for ( int i = 0; i < ARRAYSIZE( m_pszPlayerDisplayModel ); i++ )
		{
			for ( int j = 0; j < ARRAYSIZE( m_pszPlayerDisplayModel[i] ); j++ )
			{
				m_pszPlayerDisplayModel[i][j] = NULL;
			}
		}
	}

	virtual void BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors ) OVERRIDE;
#if defined(CLIENT_DLL) || defined(GAME_DLL)
	virtual void GeneratePrecacheModelStringsForStyle( CUtlVector<const char *> *out_pVecModelStrings ) const OVERRIDE;
#endif

	const char *GetPlayerDisplayModel( int iClass, int iTeam ) const;

private:
	// The .mdl file used for this item when it's being carried by a player.
	const char		*m_pszPlayerDisplayModel[2][LOADOUT_COUNT];
};

class CTFCraftingRecipeDefinition : public CEconCraftingRecipeDefinition
{
public:
	virtual bool ItemListMatchesInputs( CUtlVector<CEconItem*> *vecCraftingItems, KeyValues *out_pkvCraftParams, bool bIgnoreSlop, CUtlVector<uint64> *vecChosenItems ) const OVERRIDE;

	// A client function for testing to see if the contents of the player's backpack can match against this recipe.
	// Broken out into a separate function so we don't run the risk of its thorny logic introducing bugs into the backend crafting logic.
	bool CanMatchAgainstBackpack( CUtlVector<CEconItem*> *vecAllItems, CUtlVector<CEconItem*> vecItemsByClass[LOADOUT_COUNT], CUtlVector<CEconItem*> vecItemsBySlot[ CLASS_LOADOUT_POSITION_COUNT ], CUtlVector<uint64> *vecChosenItems ) const;

private:
	bool CheckSubItemListAgainstBackpack( CUtlVector<CEconItem*> *vecCraftingItems, CUtlVector<uint64> *vecChosenItems ) const;
};




//-----------------------------------------------------------------------------
// MvMMap_t
//-----------------------------------------------------------------------------
struct MvMMap_t
{
	CUtlConstString m_sMap; // name of the map file
	CUtlConstString m_sDisplayName; // Localization tag starting with '#'
	CUtlVector<int> m_vecMissions; // indexes into the schema's challenge list
};

enum EMvMChallengeDifficulty
{
	k_EMvMChallengeDifficulty_Invalid = -1,
	k_EMvMChallengeDifficulty_Normal = 1,
	k_EMvMChallengeDifficulty_Intermediate = 2,
	k_EMvMChallengeDifficulty_Advanced = 3,
	k_EMvMChallengeDifficulty_Expert = 4,
	k_EMvMChallengeDifficulty_Haunted = 5,

	k_EMvMChallengeDifficultyFirstValid = k_EMvMChallengeDifficulty_Normal,
	k_EMvMChallengeDifficultyLastValid = k_EMvMChallengeDifficulty_Haunted
};

extern EMvMChallengeDifficulty GetMvMChallengeDifficultyByInternalName( const char *pszEnglishID );
extern const char *GetMvMChallengeDifficultyLocName( EMvMChallengeDifficulty eDifficulty );

//-----------------------------------------------------------------------------
// MvMMission_t
//-----------------------------------------------------------------------------
struct MvMMission_t
{
	int m_iDisplayMapIndex; // Index into the schema's map list, for UI purposes
	CUtlConstString m_sPop; // name of the pop file
	CUtlConstString m_sDisplayName; // Localization tag starting with '#'
	CUtlConstString m_sMode; // Localization tag starting with '#'
	CUtlConstString m_sMapNameActual; // name of the map file to really load
	EMvMChallengeDifficulty m_eDifficulty;
	uint32 m_unMannUpPoints; // points for completing mission
};

//-----------------------------------------------------------------------------
// MvMTour_t
//-----------------------------------------------------------------------------
struct MvMTourMission_t
{
	int m_iMissionIndex; // index to the schema's challenge list
	int m_iBadgeSlot; // *index* (0...31) of the slot on the badge.  -1 if not assigned a slot.  (No bragging rights for this challenge.)
};

struct MvMTour_t
{
	CUtlConstString m_sTourInternalName;
	CUtlConstString m_sTourNameLocalizationToken; // Localization tag starting with '#', shown to clients
	CUtlConstString m_sLootImageName;
	const CEconItemDefinition *m_pBadgeItemDef; // can be NULL if there is no badge reward. Implies all badge slots will be -1. Only really valid for practice tours.
	CCopyableUtlVector<MvMTourMission_t> m_vecMissions; // indexes into the schema's challenge list
	uint32 m_nAllChallengesBits;
	EMvMChallengeDifficulty m_eDifficulty;
	bool m_bIsNew;
};

//-----------------------------------------------------------------------------
// Maps
//-----------------------------------------------------------------------------

enum EGameCategory
{
	kGameCategory_Escort = 0,
	kGameCategory_CTF,
	kGameCategory_AttackDefense,
	kGameCategory_Koth,
	kGameCategory_CP,
	kGameCategory_EscortRace,
	kGameCategory_EventMix,
	kGameCategory_SD,
	kGameCategory_Quickplay,
	kGameCategory_Event247,
	kGameCategory_Arena,
	kGameCategory_RobotDestruction,
	kGameCategory_Powerup,
	kGameCategory_Featured,
	kGameCategory_Passtime,
	kGameCategory_Community_Update,
	kGameCategory_Misc,
	kGameCategory_Competitive_6v6,
	kGameCategory_Other,
	kGameCategory_Halloween,
	kGameCategory_Competitive_12v12,
	kGameCategory_Christmas,

	// Note: Don't reorder this list.  Only add to the end

	eNumGameCategories,
};

typedef uint32 map_identifier_t;

enum eQuickplayMatchType
{
	kQuickplay_AdvancedUsersOnly,
	kQuickplay_AllUsers,				// everyone
	kQuickplay_Disabled,				// no-one

	kQuickplayTypeCount
};

enum EMatchmakingGroupType
{
	kMatchmakingType_None = -1,

	kMatchmakingType_SpecialEvents,
	kMatchmakingType_Core,
	kMatchmakingType_Alternative,
	kMatchmakingType_Competitive_6v6,
	kMatchmakingType_Competitive_12v12,

	kMatchmakingTypeCount
};

enum EMatchmakingGameModeRestrictionType
{
	kMatchmakingGameModeRestrictionType_None = -1,

	kMatchmakingGameModeRestrictionType_Holiday,
	kMatchmakingGameModeRestrictionType_Operation,

	kMatchmakingGameModeRestrictionTypeCount
};

typedef uint32 MapDefIndex_t;

struct MapDef_t
{
	MapDef_t( const char* pszMapStampDefName )
		: mapStampDef( pszMapStampDefName )
		, m_nStatsIdentifier( (MapDefIndex_t)-1 )
	{}

	CSchemaItemDefHandle mapStampDef;
	MapDefIndex_t m_nDefIndex;
	const char* pszMapName;
	const char* pszMapNameLocKey;
	const char* pszAuthorsLocKey;		// if set, will be considered a community map in the UI
	const char* pszStrangePrefixLocKey;

	// The m_nStatsIdentifier field is used when looking up a map in a user's gamestats.
	// It's a relic from the quickplay days and how the maps were defined in the schema back then.
	// We've since switched to using a map defindex, which is easier to read and manage, but this
	// field still needs to be used to lookup map gamestats because millions of customers
	// have these maps identified by those numbers in their gamestats.  The old numbers for existing
	// maps is already defined in _maps.txt newly defined maps don't need to specify a "statsidentifier"
	// field, because they will generate their own unique identifier.
	map_identifier_t m_nStatsIdentifier;
	map_identifier_t GetStatsIdentifier() const { return m_nStatsIdentifier == -1 ? (m_nDefIndex << 16) : m_nStatsIdentifier; }
	bool IsCommunityMap() const { return pszAuthorsLocKey != NULL; }
	CUtlVector< EGameCategory > m_vecAssociatedGameCategories;
	CUtlVector<econ_tag_handle_t>	vecTags;
	// The rolling match tags for this map.  When a rolling match vote happens, only allow voting on
	// maps that have at least one matching tag with this map.
	struct WeightedNextMapCandidates_t
	{
		MapDefIndex_t	m_nDefIndex;
		float			m_flWeight;
	};
	CUtlVector< WeightedNextMapCandidates_t >		m_vecRollingMatchMaps;
	void AddMapAsTargetWithWeight( const WeightedNextMapCandidates_t& target )
	{
		FOR_EACH_VEC( m_vecRollingMatchMaps, i )
		{
			if ( m_vecRollingMatchMaps[ i ].m_nDefIndex == target.m_nDefIndex )
			{
				m_vecRollingMatchMaps[ i ].m_flWeight = Max( m_vecRollingMatchMaps[ i ].m_flWeight, target.m_flWeight );
				return;
			}
		}

		m_vecRollingMatchMaps.AddToTail( target );
	}

	CUtlVector< econ_tag_handle_t >	m_vecRollingMatchTags;
	bool BHasRollingMatchTag( econ_tag_handle_t tag ) const 
	{
		FOR_EACH_VEC( m_vecRollingMatchTags, i )
		{
			if ( m_vecRollingMatchTags[ i ] == tag )
				return true;
		}

		return false;
	}

	struct WeightedNextMapTargets_t
	{
		econ_tag_handle_t	m_tag;
		float				m_flWeight;
	};
	CUtlVector< WeightedNextMapTargets_t >	m_vecRollingMatchTargets;

};

struct SchemaMMGameModeRestriction_t
{
	SchemaMMGameModeRestriction_t()
	{
		m_eType = kMatchmakingGameModeRestrictionType_None;
		m_nValue = -1;
	}

	EMatchmakingGameModeRestrictionType	m_eType;
	int									m_nValue;
	CUtlString							m_strValue;
};

struct SchemaMMGroup_t;
struct SchemaGameCategory_t
{
	SchemaGameCategory_t()
		: m_eGameCategory( eNumGameCategories )
		, m_pszLocalizedName( NULL )
		, m_pMMGroup( NULL )
		, m_pszLocalizedDesc( NULL )
		, m_pszListImage( NULL )
	{}

	SchemaGameCategory_t( const SchemaGameCategory_t& other )
	{
		m_eGameCategory = other.m_eGameCategory;
		m_pMMGroup = other.m_pMMGroup;
		m_pszLocalizedName = other.m_pszLocalizedName;
		m_pszLocalizedDesc = other.m_pszLocalizedDesc;
		m_pszListImage = other.m_pszListImage;
		m_vecMaps.Purge();
		m_vecMaps.CopyArray( other.m_vecMaps.Base(), other.m_vecMaps.Count() );
		m_vecRestrictions.Purge();
		m_vecRestrictions.CopyArray( other.m_vecRestrictions.Base(), other.m_vecRestrictions.Count() );
	}

	~SchemaGameCategory_t()
	{}

	void AddMap( const MapDef_t *pMap, bool bEnabled )
	{
		if ( !pMap )
			return;

		m_vecMaps.AddToTail( pMap );

		if ( bEnabled )
		{
			m_vecEnabledMaps.AddToTail( pMap );
		}
	}

	const MapDef_t *GetRandomMap( void ) const
	{
		Assert( m_vecEnabledMaps.Count() );
		return m_vecEnabledMaps[RandomInt( 0, m_vecEnabledMaps.Count() - 1 )];
	}

	bool PassesRestrictions() const;

	//void SerializeToKVs( KeyValues* pKV );

	EGameCategory					 m_eGameCategory;
	const SchemaMMGroup_t*			 m_pMMGroup;
	const char*						 m_pszName;
	const char*						 m_pszLocalizedName;
	const char*						 m_pszLocalizedDesc;
	const char*						 m_pszListImage;
	const char*						 m_pszMMType;
	CUtlVector< const MapDef_t* > m_vecEnabledMaps;
	CUtlVector< SchemaMMGameModeRestriction_t >	m_vecRestrictions;
	CUtlVector< const MapDef_t* > m_vecMaps;
};
typedef CUtlMap< EGameCategory, SchemaGameCategory_t* > GameCategoryMap_t;

struct SchemaMMGroup_t
{
	SchemaMMGroup_t()
		: m_eMMGroup( kMatchmakingType_None )
		, m_pszLocalizedName( NULL )
		, m_nMaxExcludes( 0 )
	{}

	SchemaMMGroup_t( const SchemaMMGroup_t& other )
	{
		m_eMMGroup = other.m_eMMGroup;
		m_pszLocalizedName = other.m_pszLocalizedName;
		m_nMaxExcludes = other.m_nMaxExcludes;
		m_vecModes.Purge();
		m_vecModes.CopyArray( other.m_vecModes.Base(), other.m_vecModes.Count() );
	}

	bool IsCategoryValid() const;

	~SchemaMMGroup_t()
	{}

	EMatchmakingGroupType					m_eMMGroup;
	const char*								m_pszName;
	const char*								m_pszLocalizedName;
	int										m_nMaxExcludes;
	CBitVec<ETFMatchGroup_ARRAYSIZE>		m_bitsValidMMGroups;
	CUtlVector< const SchemaGameCategory_t* >	m_vecModes;
};
typedef CUtlMap< EMatchmakingGroupType, SchemaMMGroup_t* > MMGroupMap_t;


//-----------------------------------------------------------------------------
// CTFItemSchema
//-----------------------------------------------------------------------------
class CTFItemSchema : public CEconItemSchema
{
public:
	CTFItemSchema();

	virtual void Reset();

	CTFItemDefinition *GetTFItemDefinition( int iItemIndex )
	{
		return (CTFItemDefinition *)GetItemDefinition( iItemIndex );
	}

	CTFCraftingRecipeDefinition *GetTFCraftingRecipeDefinition( int iRecipeIndex )
	{
		return (CTFCraftingRecipeDefinition *)GetRecipeDefinition( iRecipeIndex );
	}

	const CQuestObjectiveConditionsDefinition* GetQuestObjectiveConditionByDefIndex( ObjectiveConditionDefIndex_t nDefIndex ) const;

	const CWarDefinition *GetWarDefinitionByIndex( war_definition_index_t nDefIndex ) const;
	const CWarDefinition *GetWarDefinitionByName( const char* pszDefName ) const;
	const WarDefinitionMap_t& GetWarDefinitions() const { return m_mapWars; }

	const CUtlVector<const char *>& GetClassUsabilityStrings() const { return m_vecClassUsabilityStrings; }
	const CUtlVector<const char *>& GetLoadoutStrings( EEquipType_t eType ) const { return eType == EQUIP_TYPE_CLASS ? m_vecClassLoadoutStrings : m_vecAccountLoadoutStrings; }
	const CUtlVector<const char *>& GetLoadoutStringsForDisplay( EEquipType_t eType ) const { return eType == EQUIP_TYPE_CLASS ? m_vecClassLoadoutStringsForDisplay : m_vecAccountLoadoutStringsForDisplay; }
	const CUtlVector<const char *>& GetWeaponTypeSubstrings() const { return m_vecWeaponTypeSubstrings; }

	static const char k_rchOverrideItemLevelDescStringAttribName[];

	static const char k_rchMvMTicketItemDefName[];
	static const char k_rchMvMSquadSurplusVoucherItemDefName[];
	static const char k_rchMvMPowerupBottleItemDefName[];
	static const char k_rchMvMChallengeCompletedMaskAttribName[];
	static const char k_rchLadderPassItemDefName[];
	 
	static const char *GetMvMBadgeContractPointsAttributeName( EMvMChallengeDifficulty difficulty );
	static const char *GetMvMBadgeContractLevelAttributeName( EMvMChallengeDifficulty difficulty );

	const CUtlVector<MvMMap_t>& GetMvmMaps() const { return m_vecMvMMaps; }
	const CUtlVector<MvMMission_t>& GetMvmMissions() const { return m_vecMvMMissions; }
	const CUtlVector<MvMTour_t>& GetMvmTours() const { return m_vecMvMTours; }
//
	/// Return index into mission list, or one of these special values:
	/// k_iMvmMissionIndex_Any if empty string is passed
	/// k_iMvmMissionIndex_NotInSchema if not found
	///
	/// Input is the full pop filename, but without the directory or extension
	int FindMvmMissionByName( const char *pszChallengeName ) const;

	/// Get pop filename (without extension) given the challenge index.
	/// Handles k_iMvmMissionIndex_Any and k_iMvmMissionIndex_NotInSchema
	const char *GetMvmMissionName( int iChallengeIndex ) const;

	/// Return index into tour list, or one of these special values:
	/// k_iMvmTourIndex_Any if empty string is passed
	/// k_iMvmTourIndex_NotInSchema if not found
	///
	/// Input is the value of MvMTour_t::m_sTourInternalName
	int FindMvmTourByName( const char *pszTourName ) const;

	/// Find mission within a particular tour, and return index into MvMTour_t::m_vecMissions.
	/// Returns -1 if invalid tour index or mission is not part of the tour
	int FindMvmMissionInTour( int idxTour, int idxMissionInSchema ) const;

	/// Get badge slot corresponding to particular mission, for a given tour.
	/// Returns bit index MvMTourMission_t::m_iBadgeSlot (NOT BITMASK), or -1 if
	/// invalid tour index of mission is not part of the tour
	int GetMvmMissionBadgeSlotForTour( int idxTour, int idxMissionInSchema ) const;

	int GetMapCount() const { return m_vecMasterListOfMaps.Count(); }
	const MapDef_t *GetMasterMapDefByName( const char *pszSearchName ) const;
	const MapDef_t *GetMasterMapDefByIndex( MapDefIndex_t unIndex ) const;
	const CUtlVector<MapDef_t*>& GetMasterMapsList() const { return m_vecMasterListOfMaps; }
	const GameCategoryMap_t& GetGameCategoryMap() const { return m_mapGameCategories; }
	const SchemaGameCategory_t* GetGameCategory( EGameCategory eType ) const;
	const MMGroupMap_t& GetMMGroupMap() const { return m_mapMMGroups; }
	const SchemaMMGroup_t* GetMMGroup( EMatchmakingGroupType eCat ) const;


public:
	// CEconItemSchema interface.
	virtual CEconItemDefinition				*CreateEconItemDefinition()			{ return new CTFItemDefinition; }
	virtual CEconCraftingRecipeDefinition	*CreateCraftingRecipeDefinition()	{ return new CTFCraftingRecipeDefinition; }
	virtual CEconStyleInfo					*CreateEconStyleInfo()				{ return new CTFStyleInfo; }

	virtual bool							 BCanStrangeFilterApplyToStrangeSlotInItem( uint32 /*strange_event_restriction_t*/ unRestrictionType, uint32 unRestrictionValue, const IEconItemInterface *pItem, int iStrangeSlot, uint32 *out_pOptionalScoreType ) const;

	virtual IEconTool						*CreateEconToolImpl( const char *pszToolType, const char *pszUseString, const char *pszUsageRestriction, item_capabilities_t unCapabilities, KeyValues *pUsageKV ) OVERRIDE;

	virtual bool BInitSchema( KeyValues *pKVRawDefinition, CUtlVector<CUtlString> *pVecErrors = NULL ) OVERRIDE;
	virtual bool BPostSchemaInit( CUtlVector<CUtlString> *pVecErrors ) OVERRIDE;

	virtual RTime32 GetCustomExpirationDate( const char *pszExpirationDate ) const OVERRIDE;
protected:
#ifdef TF_CLIENT_DLL
	virtual int CalculateNumberOfConcreteItems( const CEconItemDefinition *pItemDef );
#endif // TF_CLIENT_DLL

private:
	void InitializeStringTable( const char **ppStringTable, unsigned int unStringCount, CUtlVector<const char *> *out_pvecStringTable );

	bool BInitMvmMissions( KeyValues *pKVMvmMaps, CUtlVector<CUtlString> *pVecErrors );
	bool BInitMvmTours( KeyValues *pKVMvmTours, CUtlVector<CUtlString> *pVecErrors );
	bool BInitGameModes( KeyValues *pKVMaps, CUtlVector<CUtlString> *pVecErrors );
	bool BInitMaps( KeyValues *pKVMaps, CUtlVector<CUtlString> *pVecErrors );
	bool BInitMMCategories( KeyValues *pKVCategories, CUtlVector<CUtlString> *pVecErrors );
	bool BInitQuestObjectiveConditions( KeyValues *pKVConditionsBlock, CUtlVector<CUtlString> *pVecErrors );
	bool BObjectiveConditionsPostInit( CUtlVector<CUtlString> *pVecErrors );
	bool BInitWarDefs( KeyValues *pKVWarDefs, CUtlVector<CUtlString> *pVecErrors );

	bool BPostInitMaps( CUtlVector<CUtlString> *pVecErrors );


	CUtlVector<const char *> m_vecClassUsabilityStrings;
	CUtlVector<const char *> m_vecClassLoadoutStrings;
	CUtlVector<const char *> m_vecClassLoadoutStringsForDisplay;
	CUtlVector<const char *> m_vecAccountLoadoutStrings;
	CUtlVector<const char *> m_vecAccountLoadoutStringsForDisplay;
	CUtlVector<const char *> m_vecWeaponTypeSubstrings;

	CUtlVector<MvMMap_t> m_vecMvMMaps;
	CUtlVector<MvMMission_t> m_vecMvMMissions;
	CUtlVector<MvMTour_t> m_vecMvMTours;
	CUtlMap< ObjectiveConditionDefIndex_t, CQuestObjectiveConditionsDefinition* > m_mapQuestObjectiveConditions;

	CUtlVector<MapDef_t*> m_vecMasterListOfMaps;
	GameCategoryMap_t m_mapGameCategories;
	MMGroupMap_t m_mapMMGroups;
	WarDefinitionMap_t m_mapWars;

};

#endif // TFITEMSCHEMA_H
