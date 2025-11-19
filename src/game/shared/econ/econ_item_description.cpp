//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "econ_item_description.h"
#include "econ_item_interface.h"
#include "econ_item_tools.h"
#include "econ_holidays.h"
#include "econ_store.h"
#include "tier1/ilocalize.h"
#include "localization_provider.h"
#include "rtime.h"
#include "econ_dynamic_recipe.h"
#include "econ_paintkit.h"


	#ifndef EXTERNALTESTS_DLL
		#include "econ_item_inventory.h"
	#endif

	#ifdef CLIENT_DLL
		#include "gc_clientsystem.h"
		#include "client_community_market.h"			// for Market data in tooltips
		#include "econ_ui.h"							// for money-value-to-display-string formatting
		#include "store/store_panel.h"					// for money-value-to-display-string formatting
	#endif // CLIENT_DLL


#ifdef PROJECT_TF
	#include "tf_duel_summary.h"
	#include "econ_contribution.h"
	#include "tf_player_info.h"
	#include "tf_wardata.h"

	#ifdef TF_CLIENT_DLL
		#include "tf_gamerules.h"
		#include "tf_mapinfo.h"
	#endif 
#endif

#ifdef VPROF_ENABLED
	static const char *g_pszEconDescriptionVprofGroup = _T("Econ Description");
#endif

extern const char *GetWearLocalizationString( float flWear );

// --------------------------------------------------------------------------
// Local Helper
// --------------------------------------------------------------------------
const size_t k_VerboseStringBufferSize = 128;
static char *BuildVerboseStrings( char buf[k_VerboseStringBufferSize], bool bIsVerbose, const char *format, ... )
{
	if ( !bIsVerbose )
		return NULL;

	va_list argptr;
	va_start( argptr, format );
	Q_vsnprintf( buf, k_VerboseStringBufferSize, format, argptr );
	va_end(argptr);

	return buf;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
static bool IsStorePreviewItem( const IEconItemInterface *pEconItem )
{
	Assert( pEconItem );

#ifdef CLIENT_DLL
	return pEconItem->GetFlags() & kEconItemFlagClient_StoreItem;
#else
	return false;
#endif
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void IEconItemDescription::YieldingFillOutEconItemDescription( IEconItemDescription *out_pDescription, CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	VPROF_BUDGET( "IEconItemDescription::YieldingFillOutEconItemDescription()", g_pszEconDescriptionVprofGroup );

	Assert( out_pDescription );
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	out_pDescription->YieldingCacheDescriptionData( pLocalizationProvider, pEconItem );
	out_pDescription->GenerateDescriptionLines( pLocalizationProvider, pEconItem );
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
const econ_item_description_line_t *IEconItemDescription::GetFirstLineWithMetaType( uint32 unMetaTypeSearchFlags ) const
{
	for ( unsigned int i = 0; i < GetLineCount(); i++ )
	{
		const econ_item_description_line_t& pLine = GetLine(i);
		if ( (pLine.unMetaType & unMetaTypeSearchFlags) == unMetaTypeSearchFlags )
			return &pLine;
	}

	return NULL;
}

#ifdef BUILD_ITEM_NAME_AND_DESC

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
CLocalizedStringArg<CLocalizedRTime32>::CLocalizedStringArg( const CLocalizedRTime32& cTimeIn )
{

	CRTime cTime( cTimeIn.m_unTime );

	// The GC will always display time in GMT. "Local time" isn't a useful thing from a client's perspective
	// when viewing an item in the Steam Community, etc.
	cTime.SetToGMT( cTimeIn.m_bForceGMTOnClient );

	const locchar_t *loc_LocalizationFormat = cTimeIn.m_pLocalizationProvider->Find( cTime.BIsGMT() ? "Econ_DateFormat_GMT" : "Econ_DateFormat" );

	time_t tTime = cTime.GetRTime32();
	struct tm tmStruct;
	struct tm *ptm = cTime.BIsGMT() ? Plat_gmtime( &tTime, &tmStruct ) : Plat_localtime( &tTime, &tmStruct );

	time_t tFinalTime = mktime( ptm );
	
	char rgchDateBuf[ 128 ];
	BGetLocalFormattedDate( tFinalTime, rgchDateBuf, sizeof( rgchDateBuf ) );

	KeyValues *pKeyValues = new KeyValues( "DateTokens" );

	pKeyValues->SetString( "day", &rgchDateBuf[0] );
	pKeyValues->SetInt( "hour", cTime.GetHour() );
	pKeyValues->SetString( "min", CFmtStr( "%02u", cTime.GetMinute() ).Access() );
	pKeyValues->SetString( "sec", CFmtStr( "%02u", cTime.GetSecond() ).Access() );

	m_Str = CConstructLocalizedString( loc_LocalizationFormat, pKeyValues );

	pKeyValues->deleteThis();
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItemDescription::YieldingFillOutAccountPersonaName( const CLocalizationProvider *pLocalizationProvider, uint32 unAccountID )
{
	Assert( pLocalizationProvider );
 
	// Never cache invalid accounts.
	if ( unAccountID == 0 )
	return;
 
	// Make sure we have a cache entry for this account ID. If we're hashing, we won't fill
	// this with real data to avoid discrepancies between the GC view of a persona name and the
	// client view, both of which are cached differently. If we're not hashing, we'll do our best
	// to find the current name. Either way, by the time this function ends we expect to have a
	// value stored for this account ID.
	CEconItemDescription::steam_account_persona_name_t& AccountPersona = vecPersonaNames[ vecPersonaNames.AddToTail() ];
	AccountPersona.unAccountID = unAccountID;
 
	{
		const char *utf8_PersonaName = NULL;
 
		utf8_PersonaName = InventoryManager()->PersonaName_Get( unAccountID );

#if defined( CLIENT_DLL )
		m_bUnknownPlayer = Q_strncmp( utf8_PersonaName, "[unknown]", ARRAYSIZE( "[unknown]" ) ) == 0;
#endif
 
		// We should have filled this in with something by now, even if that something is "we couldn't
		// find useful information".
		Assert( utf8_PersonaName );
 
		// Convert our UTF8 to whatever we're using for localized display, and done.
		pLocalizationProvider->ConvertUTF8ToLocchar( utf8_PersonaName, &AccountPersona.loc_sPersonaName );
	}
 
	Assert( !AccountPersona.loc_sPersonaName.IsEmpty() );
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
const locchar_t *CEconItemDescription::FindAccountPersonaName( uint32 unAccountID ) const
{
	FOR_EACH_VEC( vecPersonaNames, i )
	{
		if ( vecPersonaNames[i].unAccountID == unAccountID )
			return vecPersonaNames[i].loc_sPersonaName.Get();
	}

	// FIXME: add localization token
	return LOCCHAR("Unknown User");
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItemDescription::YieldingFillOutAccountTypeCache( uint32 unAccountID, int nClassID )
{
	if( !unAccountID )
		return;

	EUniverse eUniverse = GetUniverse();

	if ( eUniverse == k_EUniverseInvalid )
		return;

	GCSDK::CGCClientSharedObjectCache *pSOCache = GCClientSystem()->GetSOCache( CSteamID( unAccountID, eUniverse, k_EAccountTypeIndividual ) );
	if ( !pSOCache )
		return;

	GCSDK::CSharedObjectTypeCache *pTypeCache = pSOCache->FindTypeCache( nClassID );
	if ( !pTypeCache )
		return;

	CEconItemDescription::steam_account_type_cache_t& AccountTypeCache = vecTypeCaches[ vecTypeCaches.AddToTail() ];

	AccountTypeCache.unAccountID = unAccountID;
	AccountTypeCache.nClassID	 = nClassID;
	AccountTypeCache.pTypeCache	 = pTypeCache;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
GCSDK::CSharedObjectTypeCache *CEconItemDescription::FindAccountTypeCache( uint32 unAccountID, int nClassID ) const
{
	FOR_EACH_VEC( vecTypeCaches, i )
	{
		if ( vecTypeCaches[i].unAccountID == unAccountID &&
			 vecTypeCaches[i].nClassID == nClassID )
		{
			return vecTypeCaches[i].pTypeCache;
		}
	}

	return NULL;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
template < typename T >
const T *CEconItemDescription::FindAccountTypeCacheSingleton( uint32 unAccountID, int nClassID ) const
{
	GCSDK::CSharedObjectTypeCache *pSOTypeCache = FindAccountTypeCache( unAccountID, nClassID );
	if ( !pSOTypeCache )
		return NULL;

	if ( pSOTypeCache->GetCount() != 1 )
		return NULL;

	return dynamic_cast<T *>( pSOTypeCache->GetObject( 0 ) );
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItemDescription::YieldingCacheDescriptionData( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	VPROF_BUDGET( "CEconItemDescription::YieldingCacheDescriptionData()", g_pszEconDescriptionVprofGroup );
	
	vecPersonaNames.Purge();
	vecTypeCaches.Purge();

	// For each attribute that is set to display as an account ID, load the persona name for that account
	// ID in advance so that we don't yield somewhere crazy down below.
	
	// Walk our attribute list and accumulate IDs.
	CSteamAccountIDAttributeCollector AccountIDCollector;
	pEconItem->IterateAttributes( &AccountIDCollector );
	const CUtlVector<uint32>& vecSteamAccountIDs = AccountIDCollector.GetAccountIDs();

	// Look up the persona names for each account referenced by an attribute directly.
	FOR_EACH_VEC( vecSteamAccountIDs, i )
	{
		YieldingFillOutAccountPersonaName( pLocalizationProvider, vecSteamAccountIDs[i] );
	}

	// Look up the persona names for each account referencing an attribute indirectly (ie., just stuffed
	// into 32 bits).
	for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
	{
		uint32 unRestrictionType;
		if ( pEconItem->FindAttribute( GetKillEaterAttr_Restriction(i), &unRestrictionType ) &&
			 unRestrictionType == kStrangeEventRestriction_VictimSteamAccount )
		{
			uint32 unAccountID;
			DbgVerify( pEconItem->FindAttribute( GetKillEaterAttr_RestrictionValue(i), &unAccountID ) );
			YieldingFillOutAccountPersonaName( pLocalizationProvider, unAccountID );
		}
	}

#ifdef PROJECT_TF
	uint32 unAccountID = pEconItem->GetAccountID();

	// Duel summary.
	{
		// We'll need to access other information about our duel stats later, but we also need to precache
		// the account name of whoever our last kill was beforehand.
		YieldingFillOutAccountTypeCache( unAccountID, CTFDuelSummary::k_nTypeID );

		// In TF, we also store information about our previous duel target, stored way way down inside some
		// other structures.
		const CTFDuelSummary *pDuelSummary = FindAccountTypeCacheSingleton<CTFDuelSummary>( unAccountID, CTFDuelSummary::k_nTypeID );

		if ( pDuelSummary )
		{
			YieldingFillOutAccountPersonaName( pLocalizationProvider, pDuelSummary->Obj().last_duel_account_id() );
		}
	}

	// Map contributions.
	YieldingFillOutAccountTypeCache( unAccountID, CTFMapContribution::k_nTypeID );

	// New users helped.
	YieldingFillOutAccountTypeCache( unAccountID, CTFPlayerInfo::k_nTypeID );

	// War data
	YieldingFillOutAccountTypeCache( unAccountID, CWarData::k_nTypeID );

#ifdef CLIENT_DLL
	// Duck LeaderBoards
	{
		static CSchemaAttributeDefHandle pAttrDef_DisplayDuckLeaderboard( "display duck leaderboard" );
		if ( pEconItem->FindAttribute( pAttrDef_DisplayDuckLeaderboard ) )
		{
			CUtlVector< AccountID_t > accountIds;
			Leaderboards_GetDuckLeaderboardSteamIDs( accountIds );

			FOR_EACH_VEC( accountIds, i )
			{
				// Look up the persona names for each account referenced in the leaderboard
				YieldingFillOutAccountPersonaName( pLocalizationProvider, accountIds[i] );
			}
		}
	}
#endif // CLIENT_DLL


#endif // PROJECT_TF
}

void CEconItemDescription::GenerateDescriptionLines( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	VPROF_BUDGET( "CEconItemDescription::GenerateDescriptionLines()", g_pszEconDescriptionVprofGroup );

	Assert( pLocalizationProvider );
	Assert( pEconItem );

	m_vecDescLines.Purge();

	Generate_ItemName( pLocalizationProvider, pEconItem );
	Generate_ItemRarityDesc( pLocalizationProvider, pEconItem );
	Generate_ItemLevelDesc( pLocalizationProvider, pEconItem );
	//Generate_WearAmountDesc( pLocalizationProvider, pEconItem );

	// If we decide that for performance reasons some descriptions only want the name/description
	// information and not all the details, this is the block to skip over.
	{
		Generate_CraftTag( pLocalizationProvider, pEconItem );
		Generate_StyleDesc( pLocalizationProvider, pEconItem );
		Generate_Painted( pLocalizationProvider, pEconItem );

		Generate_HolidayRestriction( pLocalizationProvider, pEconItem );	
#ifdef PROJECT_TF
		Generate_SaxxyAwardDesc( pLocalizationProvider, pEconItem );
#endif // PROJECT_TF
		Generate_VisibleAttributes( pLocalizationProvider, pEconItem );

		Generate_QualityDesc( pLocalizationProvider, pEconItem );		
		Generate_ItemDesc( pLocalizationProvider, pEconItem );
		Generate_Bundle( pLocalizationProvider, pEconItem );
		Generate_GiftedBy( pLocalizationProvider, pEconItem );
#ifdef PROJECT_TF
		Generate_DuelingMedal( pLocalizationProvider, pEconItem );
		Generate_MapContributor( pLocalizationProvider, pEconItem );
		Generate_MapStampBundleTooltip( pLocalizationProvider, pEconItem );
		Generate_FriendlyHat( pLocalizationProvider, pEconItem );
		Generate_SquadSurplusClaimedBy( pLocalizationProvider, pEconItem );
		Generate_MvmChallenges( pLocalizationProvider, pEconItem );
		Generate_DynamicRecipe( pLocalizationProvider, pEconItem );
#endif // PROJECT_TF
		Generate_XifierToolTargetItem( pLocalizationProvider, pEconItem );
		Generate_LootListDesc( pLocalizationProvider, pEconItem );
		Generate_EventDetail( pLocalizationProvider, pEconItem );
		Generate_ItemSetDesc( pLocalizationProvider, pEconItem );
#ifdef PROJECT_TF
		Generate_UnusualifierEffectList( pLocalizationProvider, pEconItem );
#endif // PROJECT_TF
		Generate_CollectionDesc( pLocalizationProvider, pEconItem );
		Generate_BonusContentDesc( pLocalizationProvider, pEconItem );
		Generate_ExpirationDesc( pLocalizationProvider, pEconItem );
		Generate_DropPeriodDesc( pLocalizationProvider, pEconItem ); 

		Generate_MarketInformation( pLocalizationProvider, pEconItem );
		Generate_DirectX8Warning( pLocalizationProvider, pEconItem );
	}

	// Certain information (tradeability, etc.) used to only get displayed if we were the owning player, or
	// if we were looking at an unowned item (ie., a store preview) and want to show what it will look like
	// when it *is* owned. Unfortunately this led to problems where you wouldn't know if the item you were
	// about to be traded (currently not owned by you) would be craftable, etc.
	Generate_FlagsAttributes( pLocalizationProvider, pEconItem );
}

// --------------------------------------------------------------------------
// Purpose: Code to build up the item display name, including any relevant quality
//			strings, custom renaming, craft numbers, and anything else we decide
//			to throw at it.
// --------------------------------------------------------------------------

/*static*/ uint32 GetScoreTypeForKillEaterAttr( const IEconItemInterface *pEconItem, const CEconItemAttributeDefinition *pAttribDef )
{
	// What sort of event are we tracking? If we don't have an attribute at all we're one of the
	// old kill-eater weapons that didn't specify what it was tracking.
	uint32 unKillEaterEventType = 0;

	// This will overwrite our default 0 value if we have a value set but leave it if not.
	{
		float fKillEaterEventType;
		if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pEconItem, pAttribDef, &fKillEaterEventType ) )
		{
			unKillEaterEventType = fKillEaterEventType;
		}
	}

	return unKillEaterEventType;
}

// The item backend may add craft numbers well past what we want to display in the game. This
// function determines whether a given number should be visible rather than always showing
// whatever the GC shows.
bool ShouldDisplayCraftCounterValue( int iValue )
{
	return iValue > 0 && iValue <= 100;
}

// This function will return the localized string (ie., "Face-Melting") for a specific item based
// on the score it has accumulated.

class CStrangeRankLocalizationGenerator
{
public:
	CStrangeRankLocalizationGenerator( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem, bool bHashContextOff );

	bool IsValid() const { return m_bValid; }

	const locchar_t *GetRankLocalized() const { Assert( m_bValid ); return m_loc_Rank; }
	const locchar_t *GetRankSecondaryLocalized() const { Assert( m_bValid ); return m_loc_SecondaryRank; }

	uint32 GetStrangeType() const { Assert( m_bValid ); return m_unType; }
	uint32 GetStrangeScore() const { Assert( m_bValid ); return m_unScore; }
	uint32 GetUsedStrangeSlot() const { Assert( m_bValid ); return m_unUsedStrangeSlot; }

private:
	bool m_bValid;

	const locchar_t *m_loc_Rank;
	const locchar_t *m_loc_SecondaryRank;

	uint32 m_unType;
	uint32 m_unScore;
	uint32 m_unUsedStrangeSlot;
};

CStrangeRankLocalizationGenerator::CStrangeRankLocalizationGenerator( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem, bool bHashContextOff )
	: m_bValid( false )
	, m_loc_Rank( NULL )
	, m_loc_SecondaryRank( NULL )
	, m_unType( kKillEaterEvent_PlayerKill )
	, m_unScore( 0 )
	, m_unUsedStrangeSlot( 0 )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	static CSchemaAttributeDefHandle pAttrDef_StrangeScoreSelector( "strange score selector" );

	// Do we have a strange score selector attribute? If so, the value of this attribute will tell us which strange
	// attribute we're actually going to use to generate a name. Leaving this value as 0 will fall back to the
	// default behavior of looking at the base "kill eater" attribute.
	if ( pEconItem->FindAttribute( pAttrDef_StrangeScoreSelector, &m_unUsedStrangeSlot ) )
	{
		// Make sure the value we pulled from the database is within range.
		m_unUsedStrangeSlot = MIN( m_unUsedStrangeSlot, static_cast<uint32>( GetKillEaterAttrCount() ) );
	}


	// Use the strange prefix if the weapon has one.
	if ( !pEconItem->FindAttribute( GetKillEaterAttr_Score( m_unUsedStrangeSlot ), &m_unScore ) )
		return;

	// What type of event are we tracking and how does it describe itself?
	m_unType = GetScoreTypeForKillEaterAttr( pEconItem, GetKillEaterAttr_Type( m_unUsedStrangeSlot ) );

	const char *pszLevelingDataName = GetItemSchema()->GetKillEaterScoreTypeLevelingDataName( m_unType );
	if ( !pszLevelingDataName )
	{
		pszLevelingDataName = KILL_EATER_RANK_LEVEL_BLOCK_NAME;
	}

	uint32 uUsedScore = m_unScore;

	// For TF - Strange Scores reset on Trade, sharing that information is actually misleading so we'll always display base strange name

	const CItemLevelingDefinition *pLevelDef = GetItemSchema()->GetItemLevelForScore( pszLevelingDataName, uUsedScore );
	if ( !pLevelDef )
		return;

	// Primary rank established!
	m_loc_Rank = pLocalizationProvider->Find( pLevelDef->GetNameLocalizationKey() );
	m_bValid = true;

	// Does this score slot have a restriction that adds additional text somewhere in the localization token?
	uint32 unFilterType;
	uint32 unFilterValue;
	if ( pEconItem->FindAttribute( GetKillEaterAttr_Restriction( m_unUsedStrangeSlot ), &unFilterType ) &&
		 pEconItem->FindAttribute( GetKillEaterAttr_RestrictionValue( m_unUsedStrangeSlot ), &unFilterValue ) )
	{
		// Game-specific code doesn't belong here. "We're shipping soon" hack fun!
#ifdef PROJECT_TF
		if ( unFilterType == kStrangeEventRestriction_Map )
		{
			const MapDef_t *pMap = GetItemSchema()->GetMasterMapDefByIndex( unFilterValue );
			if ( pMap && pMap->pszStrangePrefixLocKey )
			{
				m_loc_SecondaryRank = pLocalizationProvider->Find( pMap->pszStrangePrefixLocKey );
			}
		}
		else if (unFilterType == kStrangeEventRestriction_Competitive)
		{
			m_loc_SecondaryRank = pLocalizationProvider->Find( "TF_StrangeFilter_Prefix_Competitive" );
		}
#endif // PROJECT_TF
	}
}

// ---------------------------------------------------------------------------------------------------------------------------
void Econ_ConcatPaintKitName( locchar_t( &out_pItemName )[MAX_ITEM_NAME_LENGTH], locchar_t *pPaintKitStr, const CLocalizationProvider *pLocalizationProvider, const CEconItemDefinition *pItemDef )
{
	if ( !pItemDef )
		return;

	if ( !pPaintKitStr )
	{
		Assert( pPaintKitStr );
		return;
	}

	locchar_t tempName[MAX_ITEM_NAME_LENGTH];
	loc_scpy_safe( tempName, out_pItemName );

	const wchar_t *wpszFormatString = g_pVGuiLocalize->Find( "#ToolPaintKit_ItemDescFormat" );
	if ( !wpszFormatString )
	{
		wpszFormatString = L"%s1 %s2";
	}
	g_pVGuiLocalize->ConstructString_safe( out_pItemName,
			wpszFormatString,
			2,
			pPaintKitStr,
			tempName );
}

// ---------------------------------------------------------------------------------------------------------------------------
void Econ_ConcatPaintKitWear( locchar_t( &out_pItemName )[MAX_ITEM_NAME_LENGTH], const CLocalizationProvider *pLocalizationProvider, float flWear )
{
	if ( flWear <= 0.0 )
	{
	}

	locchar_t tempName[MAX_ITEM_NAME_LENGTH];
	loc_scpy_safe( tempName, out_pItemName );

	g_pVGuiLocalize->ConstructString_safe( out_pItemName,
		LOCCHAR( "%s1 (%s2)" ),
		2,
		tempName,
		pLocalizationProvider->Find( GetWearLocalizationString( flWear ) ) 
	);
}
// ---------------------------------------------------------------------------------------------------------------------------
static bool GetLocalizedBaseItemName( locchar_t (&szItemName)[MAX_ITEM_NAME_LENGTH], const CLocalizationProvider *pLocalizationProvider, const CEconItemDefinition *pEconItemDefinition )
{
	if ( pEconItemDefinition->GetItemBaseName() )
	{
		const locchar_t *pLocalizedItemName = pLocalizationProvider->Find( pEconItemDefinition->GetItemBaseName() );
		if ( !pLocalizedItemName || !pLocalizedItemName[0] )
		{
			// Couldn't localize it, just use it raw
			pLocalizationProvider->ConvertUTF8ToLocchar( pEconItemDefinition->GetItemBaseName(), szItemName, ARRAYSIZE( szItemName ) );
		}
		else
		{
			loc_scpy_safe( szItemName, pLocalizedItemName );
		}

		return true;
	}
	
	return false;
}

// Given the item in pEconItem and the localization provider passed in, stuff the correct *localized*
// string into out_pItemName.
static void GenerateLocalizedFullItemName
(
	locchar_t (&out_pItemName)[MAX_ITEM_NAME_LENGTH],
	const CLocalizationProvider	*pLocalizationProvider,
	const IEconItemInterface	*pEconItem,
	EGenerateLocalizedFullItemNameFlag_t eFlagsMask,
	bool						bHashContextOff
)
{
	tmZone( TELEMETRY_LEVEL1, TMZF_NONE, "%s", __FUNCTION__ );
	bool bUseProperName = bHashContextOff;
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	static const locchar_t *s_pUnknownItemName = LOCCHAR("Unknown Item");
	const uint8 unQuality = pEconItem->GetMarketQuality();

	const CEconItemDefinition *pEconItemDefinition = pEconItem->GetItemDefinition();
	if ( !pEconItemDefinition )
	{
		out_pItemName[0] = (locchar_t)0;
		return;
	}

	bool bIgnoreWear = false;
	bool bIgnoreQuality = false;
	bool bHasCustomName = false;
	uint32 unPaintKitDefIndex = 0;
	bool bIsPaintKitItem = GetPaintKitDefIndex( pEconItem, &unPaintKitDefIndex );
	if ( bIsPaintKitItem )
	{
		if ( eFlagsMask == k_EGenerateLocalizedFullItemName_Default )
		{
			bIgnoreWear = true;

			if ( ( unQuality != AE_STRANGE ) && ( unQuality != AE_SELFMADE ) )
			{
				bIgnoreQuality = true;
			}
		}
	}

	// Figure out which localization pattern we're using. By default we assume we're using the common "[Quality] [Item Name]"
	// format, but if we're a unique item with an article we'll change this later on.
	const char *pszLocalizationPattern = "ItemNameFormat";

	// Start with the base name.
	locchar_t szItemName[ MAX_ITEM_NAME_LENGTH ];

	static CSchemaAttributeDefHandle pAttrDef_ItemNameTextOverride( "item name text override" );
	CAttribute_String attrItemNameTextOverride;
	// Check if we ahve a item name override
	if ( pEconItem->FindAttribute( pAttrDef_ItemNameTextOverride, &attrItemNameTextOverride ) )
	{
		const locchar_t *pNameOverrideString = pLocalizationProvider->Find( attrItemNameTextOverride.value().c_str() );
		if ( pNameOverrideString )
		{
			loc_scpy_safe( szItemName, pNameOverrideString );
			bHasCustomName = true;
		}
	}
	else if( !GetLocalizedBaseItemName( szItemName, pLocalizationProvider, pEconItemDefinition ) )
	{
		loc_scpy_safe( szItemName, s_pUnknownItemName );
	}

	// Check for killstreak attribute
	enum { kKillStreakLength = 64, };
	locchar_t szKillStreak[ kKillStreakLength ] = LOCCHAR("");
	static CSchemaAttributeDefHandle pAttrDef_KillStreak( "killstreak tier" );
	uint32 nKillStreakValue;

	if ( pEconItem->FindAttribute( pAttrDef_KillStreak, &nKillStreakValue ) && !bIgnoreQuality )
	{
		nKillStreakValue = (float&)(nKillStreakValue);

		// if you have the eyeballs you are automatically higher tier
		static CSchemaAttributeDefHandle pAttrDef_KillStreakEyes( "killstreak effect" );
		static CSchemaAttributeDefHandle pAttrDef_KillStreakSheen( "killstreak idleeffect" );
		if ( pEconItem->FindAttribute( pAttrDef_KillStreakEyes ) )
		{
			nKillStreakValue = 3;	// professional
		}
		else if ( pEconItem->FindAttribute( pAttrDef_KillStreakSheen ) )
		{
			nKillStreakValue = 2;	// specialized
		}

		const locchar_t *pKillStreakLocalizedString = NULL;
		
		// All tier-1 killstreaks have idle effect 1
		if ( nKillStreakValue == 1 )
		{
			pKillStreakLocalizedString = pLocalizationProvider->Find( "ItemNameKillStreakv0" );
		}
		else if ( nKillStreakValue == 2 )
		{
			pKillStreakLocalizedString = pLocalizationProvider->Find( "ItemNameKillStreakv1" );
		}
		else // Tier-2's are things above 1
		{
			pKillStreakLocalizedString = pLocalizationProvider->Find( "ItemNameKillStreakv2" );
		}

		if ( pKillStreakLocalizedString )
		{
			loc_scpy_safe( szKillStreak, pKillStreakLocalizedString );
			//  If we're appending some sort of killstreak identifier, dont use the proper name
			bUseProperName = false;
		}
	}

	
	static CSchemaAttributeDefHandle pAttrDef_IsFestivized( "is_festivized" );
	enum { kFestiveLength = 64, };
	locchar_t szIsFestivized[kFestiveLength] = LOCCHAR( "" );
	bool bFestivized = false;
	if ( pAttrDef_IsFestivized && pEconItem->FindAttribute( pAttrDef_IsFestivized ) )
	{
		bFestivized = true;
		bUseProperName = false;
	}

	// Check to see if we have a quality text override attribute.  We can get this when a temporary item
	// comes in from a crafting recipe that needs to get its name generated, and wants to specify that it 
	// takes in any quality
	static CSchemaAttributeDefHandle pAttrDef_QualityTextOverride( "quality text override" );
	CAttribute_String attrQualityTextOverride;
	pEconItem->FindAttribute( pAttrDef_QualityTextOverride, &attrQualityTextOverride );

	// Generate our quality string.
	enum { kQualityLength = 128, };
	locchar_t szQuality[ kQualityLength ] = LOCCHAR("");

	// Unique names may have a prefix or not, and so use a different format. (This is less to deal
	// with the space after "The" and more to deal with foreign languages that want to display unique
	// and non-unique items differently.

	if ( unQuality == AE_SELFMADE || ( !bIgnoreQuality ) )
	{
		tmZone( TELEMETRY_LEVEL1, TMZF_NONE, "%s - Quality", __FUNCTION__ );
		// It's possible to get in here with a quality of -1 if we're dealing with an item view that has no
		// associated item. In that case we're probably doing something like browsing the armory, and in any
		// event don't have an item and so don't have a quality and so we just don't show a quality string.
		// If we have a quality text override, use that.
		const char *pszQualityLocalizationString = attrQualityTextOverride.has_value() 
												 ? attrQualityTextOverride.value().c_str()
												 : EconQuality_GetLocalizationString( (EEconItemQuality)unQuality );

		if ( unQuality > 0 && pszQualityLocalizationString && unQuality != AE_PAINTKITWEAPON )
		{
			// Unique items use proper names, but not if we have a quality text override
			if ( unQuality == AE_UNIQUE && !attrQualityTextOverride.has_value() )
			{
				const locchar_t *pszArticleContent = NULL;				
				if ( bUseProperName && pEconItemDefinition->HasProperName() )
				{
					pszArticleContent = pLocalizationProvider->Find( "TF_Unique_Prepend_Proper" );
				}
				
				// If the language isn't supposed to have articles or we just haven't provided one yet, fall
				// back to the empty string.
				if ( !pszArticleContent )
				{
					pszArticleContent = LOCCHAR("");
				}

				loc_scpy_safe( szQuality, pszArticleContent );
			}
			// Any quality besides unique ignores "proper name" articles.
			else
			{
				const locchar_t *pQualityLocalizedString = pLocalizationProvider->Find( pszQualityLocalizationString );
				if ( pQualityLocalizedString )
				{
					loc_scpy_safe( szQuality, pQualityLocalizedString );
					loc_scat_safe( szQuality, pLocalizationProvider->FindSafe( "#Rarity_Spacer" ) );
				}
			}
		}

		{
			static CSchemaAttributeDefHandle pAttrDef_HideStrangePrefix( "hide_strange_prefix" );
			if ( !pAttrDef_HideStrangePrefix || !pEconItem->FindAttribute( pAttrDef_HideStrangePrefix ) )
			{
				//
				CStrangeRankLocalizationGenerator RankGenerator( pLocalizationProvider, pEconItem, bHashContextOff );
				if ( RankGenerator.IsValid() )
				{
					// If the quality of this item is special (not just strange) persist and append that value
					// Otherwise the ranker will replace the 'strange' quality tag with a strange rank
					if ( unQuality == AE_STRANGE )
					{
						loc_scpy_safe( szQuality,
								   CConstructLocalizedString( LOCCHAR("%s1%s2%s3"),
															  RankGenerator.GetRankLocalized(),
															  RankGenerator.GetRankSecondaryLocalized() ? RankGenerator.GetRankSecondaryLocalized() : LOCCHAR(""),
															  pLocalizationProvider->FindSafe( "#Strange_Spacer" ) ) );
					}
					else // Strange Unusual Something
					{
						loc_scpy_safe( szQuality,
								   CConstructLocalizedString( LOCCHAR("%s1%s2 %s3"),
															  RankGenerator.GetRankLocalized(),
															  RankGenerator.GetRankSecondaryLocalized() ? RankGenerator.GetRankSecondaryLocalized() : LOCCHAR(""),
															  szQuality) );
					}
				}
			}
		}
	}

	static CSchemaAttributeDefHandle pAttrDef_IsAustralium( "is australium item" );
	enum { kAustraliumLength = 64, };
	locchar_t szAustraliumSkin[ kAustraliumLength ] = LOCCHAR("");
	if ( pAttrDef_IsAustralium && pEconItem->FindAttribute( pAttrDef_IsAustralium ) )
	{
		const locchar_t *pAustraliumLocalizedString = pLocalizationProvider->Find( "ItemNameAustralium" );
		if ( pAustraliumLocalizedString )
		{
			loc_scpy_safe( szAustraliumSkin, pAustraliumLocalizedString );
		}
	}
	
	// Festivized
	if ( bFestivized )
	{
		const locchar_t *pFestivizedLocalizedString = pLocalizationProvider->Find( "ItemNameFestive" );
		if ( pFestivizedLocalizedString )
		{
			loc_scpy_safe( szIsFestivized, pFestivizedLocalizedString );
		}
	}

	const char* pszQualityFormat = ( !attrQualityTextOverride.has_value() && ( unQuality == AE_NORMAL || unQuality == AE_UNIQUE || unQuality == AE_PAINTKITWEAPON || bIgnoreQuality ) && unQuality != AE_SELFMADE ) 
								 ? "ItemNameNormalOrUniqueQualityFormat" 
								 : "ItemNameQualityFormat";

	// TODO : Make Generic
	// Journal Leveling
	uint32 unDuckBadgeLevel;
	static CSchemaAttributeDefHandle pAttrDef_DuckBadgeLevel( "duck rating" );
	enum { kDuckBadgeLength = 64, };
	locchar_t szDuckBadge[kDuckBadgeLength] = LOCCHAR("");
	{	//if ( pItem && FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pItem, pAttr_DuckLevelBadge, &iDuckBadgeLevel ) )
		if ( pAttrDef_DuckBadgeLevel && FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pEconItem, pAttrDef_DuckBadgeLevel, &unDuckBadgeLevel ) && unDuckBadgeLevel != 0 )
		{
			const CItemLevelingDefinition *pLevelDef = GetItemSchema()->GetItemLevelForScore( "Journal_DuckBadge", unDuckBadgeLevel );
			if ( pLevelDef )
			{
				loc_scpy_safe( szDuckBadge, pLocalizationProvider->Find( pLevelDef->GetNameLocalizationKey() ) );
				loc_scat_safe( szDuckBadge, LOCCHAR(" ") );
			}
		}
	}

	// Strange Unusual Festive Killstreak Australium ducks
	loc_scpy_safe( szQuality, CConstructLocalizedString( pLocalizationProvider->Find( pszQualityFormat ), szQuality, szIsFestivized, szKillStreak, szAustraliumSkin, szDuckBadge ) );

	enum { kLocalizedCrateSeriesLength = 128, };
	locchar_t szLocalizedCrateSeries[ kLocalizedCrateSeriesLength ] = LOCCHAR("");

#ifdef PROJECT_TF
	static CSchemaAttributeDefHandle pAttrDef_SupplyCrateSeries( "set supply crate series" );
	static CSchemaAttributeDefHandle pAttrDef_HideSeries( "hide crate series number" );
	uint32 unHideSeriesNumber = 0;
	bool bHideSeriesNumber = pAttrDef_HideSeries && FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pEconItem, pAttrDef_HideSeries, &unHideSeriesNumber ) && ( unHideSeriesNumber != 0 );
	// do not display series number for crates that have a collection reference
	if ( !bHideSeriesNumber && pAttrDef_SupplyCrateSeries && pEconItemDefinition->GetItemClass() && !Q_stricmp( pEconItemDefinition->GetItemClass(), "supply_crate" ) && !pEconItemDefinition->GetCollectionReference() )
	{
		// It's a crate, find a series #
		uint32 unSupplyCrateSeries;
		float fSupplyCrateSeries;
		if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pEconItem, pAttrDef_SupplyCrateSeries, &fSupplyCrateSeries ) && fSupplyCrateSeries != 0.0f )
		{
			unSupplyCrateSeries = fSupplyCrateSeries;

			loc_scpy_safe( szLocalizedCrateSeries,
						   CConstructLocalizedString( pLocalizationProvider->Find( "ItemNameCraftSeries" ),
													  unSupplyCrateSeries ) );
		}
	}

	// This is not "crate series number"; this is "release series number", ie., "a series 3 chemistry kit".
	static CSchemaAttributeDefHandle pAttrDef_SeriesNumber( "series number" );
	if ( pAttrDef_SeriesNumber )
	{
		float flSeriesNumber = 0.0f;
		if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pEconItem, pAttrDef_SeriesNumber, &flSeriesNumber ) && flSeriesNumber != 0.0f )
		{
			uint32 unSeriesNumber = flSeriesNumber;

			loc_scpy_safe( szLocalizedCrateSeries,
						   CConstructLocalizedString( pLocalizationProvider->Find( "ItemNameCraftSeries" ),
													  unSeriesNumber ) );
		}
	}
#endif

	// Were we one of the first couple that were crafted? If so, output our craft number as well.
	locchar_t *pCraftNumberLocFormat = pLocalizationProvider->Find( "ItemNameCraftNumberFormat" );

	enum { kLocalizedCraftIndexLength = 128, };
	locchar_t szLocalizedCraftIndex[ kLocalizedCraftIndexLength ] = LOCCHAR("");

	if ( pCraftNumberLocFormat )
	{
		static CSchemaAttributeDefHandle pAttrDef_UniqueCraftIndex( "unique craft index" );

		uint32 unCraftIndex;
		if ( pEconItem->FindAttribute( pAttrDef_UniqueCraftIndex, &unCraftIndex ) &&
			 ShouldDisplayCraftCounterValue( unCraftIndex ) )
		{
			locchar_t szCraftNumber[ kLocalizedCraftIndexLength ];
			loc_sprintf_safe( szCraftNumber, LOCCHAR( "%i" ), unCraftIndex );
				
			ILocalize::ConstructString_safe( szLocalizedCraftIndex,
										pCraftNumberLocFormat,
										1,
										szCraftNumber );
		}
	}

	// Generate tool application string
	enum { kToolApplicationNameLength = 128, };
	locchar_t szToolTargetNameName[ kToolApplicationNameLength ] = LOCCHAR("");
	locchar_t szDynamicRecipeOutputName[ kToolApplicationNameLength ] = LOCCHAR("");

	static CSchemaAttributeDefHandle pAttribDef_ToolTarget( "tool target item" );
	if( pAttribDef_ToolTarget && pEconItem->GetItemDefinition()->GetItemClass() && !Q_stricmp( pEconItem->GetItemDefinition()->GetItemClass(), "tool" ) )
	{
		tmZone( TELEMETRY_LEVEL1, TMZF_NONE, "%s - Tool", __FUNCTION__ );
		// It's a tool, see if it has a tool target item attribute
		float flItemDef;
		if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pEconItem, pAttribDef_ToolTarget, &flItemDef ) )
		{
			const item_definition_index_t unItemDef = flItemDef;

			locchar_t szTargetItemName[ MAX_ITEM_NAME_LENGTH ] = LOCCHAR("Unknown Item");

			// Get base name of target item
			const CEconItemDefinition *pEconTargetDef = GetItemSchema()->GetItemDefinition( unItemDef );
			if ( pEconTargetDef )
			{
				GetLocalizedBaseItemName( szTargetItemName, pLocalizationProvider, pEconTargetDef );
			}

			loc_scpy_safe( szToolTargetNameName,
					   CConstructLocalizedString( pLocalizationProvider->Find( "ItemNameToolTargetNameFormat" ),
					                              szTargetItemName ) );
		}
		else
		{
			CRecipeComponentMatchingIterator componentIterator( pEconItem, NULL );
			pEconItem->IterateAttributes( &componentIterator );

			// It only makes sense to mention the output if there's only 1 output
			if( componentIterator.GetMatchingComponentOutputs().Count() == 1 )
			{
				CAttribute_DynamicRecipeComponent attribValue;
				pEconItem->FindAttribute( componentIterator.GetMatchingComponentOutputs().Head(), &attribValue );

				CEconItem tempItem;
				if ( !DecodeItemFromEncodedAttributeString( attribValue, &tempItem ) )
				{
					AssertMsg2( 0, "%s: Unable to decode dynamic recipe output attribute on item %llu.", __FUNCTION__, pEconItem->GetID() );
				}
				else
				{
					locchar_t loc_ItemName[MAX_ITEM_NAME_LENGTH];
					GenerateLocalizedFullItemName( loc_ItemName, pLocalizationProvider, &tempItem, k_EGenerateLocalizedFullItemName_Default, false );
			
					loc_scpy_safe( szDynamicRecipeOutputName,
						   CConstructLocalizedString( pLocalizationProvider->Find( "ItemNameDynamicRecipeTargetNameFormat" ),
													  loc_ItemName ) );
				}
			}
		}
	}


	// PaintKit and Wear
	if ( !bHasCustomName )
	{
		if ( bIsPaintKitItem )
		{
			tmZone( TELEMETRY_LEVEL1, TMZF_NONE, "%s - Paintkit", __FUNCTION__ );
			// find paintkit name
			const CPaintKitDefinition* pPaintKitDef = assert_cast< const CPaintKitDefinition* >( GetProtoScriptObjDefManager()->GetDefinition( ProtoDefID_t( DEF_TYPE_PAINTKIT_DEFINITION, unPaintKitDefIndex ) ) );
			locchar_t *pPaintKitStr = pPaintKitDef ? pLocalizationProvider->FindSafe( pPaintKitDef->GetDescriptionToken() ) : NULL;

			Econ_ConcatPaintKitName( szItemName, pPaintKitStr, pLocalizationProvider, pEconItemDefinition );
			if ( !bIgnoreWear )
			{
				float flWear = 0;
				if ( GetPaintKitWear( pEconItem, flWear ) )
				{
					Econ_ConcatPaintKitWear( szItemName, pLocalizationProvider, flWear );
				}
			}
		}
	}

	locchar_t *pNameLocalizationFormat = pLocalizationProvider->Find( pszLocalizationPattern );

	if ( pNameLocalizationFormat )
	{
		ILocalize::ConstructString_safe( out_pItemName,
									pNameLocalizationFormat,
									6,
									szQuality,
									szItemName,
									szLocalizedCraftIndex,
									szLocalizedCrateSeries,
									szToolTargetNameName,
									szDynamicRecipeOutputName);
	}
	else
	{
		loc_scpy_safe( out_pItemName, s_pUnknownItemName );
	}
}


// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItemDescription::Generate_ItemName( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	// If this item has a custom name, use it instead of doing our crazy name compositing based on quality,
	// type, etc.
	const char *utf8_CustomName = pEconItem->GetCustomName();

	if ( utf8_CustomName && utf8_CustomName[0] )
	{
		locchar_t loc_CustomName[ MAX_ITEM_NAME_LENGTH ];
		pLocalizationProvider->ConvertUTF8ToLocchar( utf8_CustomName, loc_CustomName, sizeof( loc_CustomName ) );

		// Store it in the item name, wrapped in quotes to prevent item name spoofing
		// We use two single quotes, because the double quote isn't very visible in the TF2 font
		locchar_t loc_CustomNameWithQuotes[ MAX_ITEM_NAME_LENGTH ];
		loc_scpy_safe( loc_CustomNameWithQuotes, LOCCHAR("''") );
		loc_scat_safe( loc_CustomNameWithQuotes, loc_CustomName );
		loc_scat_safe( loc_CustomNameWithQuotes, LOCCHAR("''") );

		AddDescLine( loc_CustomNameWithQuotes, /* this will be ignored: */ ATTRIB_COL_LEVEL, kDescLineFlag_Name );
	}
	else
	{
		locchar_t loc_ItemName[MAX_ITEM_NAME_LENGTH];

		EGenerateLocalizedFullItemNameFlag_t eNameFlag = k_EGenerateLocalizedFullItemName_Default;
	 

		GenerateLocalizedFullItemName( loc_ItemName, pLocalizationProvider, pEconItem, eNameFlag, TF_ANTI_IDLEBOT_VERIFICATION_ONLY_ARG_BOOL_TRUE( m_pHashContext == NULL ) );

		AddDescLine( loc_ItemName, /* this will be ignored: */ ATTRIB_COL_LEVEL, kDescLineFlag_Name );
	}
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
const locchar_t *GetLocalizedStringForKillEaterTypeAttr( const CLocalizationProvider *pLocalizationProvider, uint32 unKillEaterEventType )
{
	Assert( pLocalizationProvider );
	
	// Generate localized string.
	const char *pszLocString = GetItemSchema()->GetKillEaterScoreTypeLocString( unKillEaterEventType );

	return pszLocString != NULL
		 ? pLocalizationProvider->Find( pszLocString )
		 : LOCCHAR("");
}

class CStrangeRestrictionAttrWrapper
{
public:
	CStrangeRestrictionAttrWrapper( const CLocalizationProvider *pLocalizationProvider, const locchar_t *loc_In )
		: m_str( loc_In ? pLocalizationProvider->Find( "ItemTypeDescStrangeFilterSubStr" ) : LOCCHAR(""), loc_In ? loc_In : LOCCHAR("") )
	{
		//
	}

	const locchar_t *operator *() const
	{
		return static_cast<const locchar_t *>( m_str );
	}

private:
	CConstructLocalizedString m_str;
};

const locchar_t *CEconItemDescription::GetLocalizedStringForStrangeRestrictionAttr( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem, int iAttrIndex ) const
{
	uint32 unRestrictionType;
	uint32 unRestrictionValue;
	if ( !pEconItem->FindAttribute( GetKillEaterAttr_Restriction( iAttrIndex ), &unRestrictionType ) ||
		 !pEconItem->FindAttribute( GetKillEaterAttr_RestrictionValue( iAttrIndex ), &unRestrictionValue ) ||
		 unRestrictionType == kStrangeEventRestriction_None )
	{
		return NULL;
	}

	switch ( unRestrictionType )
	{
#ifdef PROJECT_TF
	case kStrangeEventRestriction_Map:
	{
		const MapDef_t *pMap = GetItemSchema()->GetMasterMapDefByIndex( unRestrictionValue );
		if ( pMap )
			return pLocalizationProvider->Find( pMap->pszMapNameLocKey );
	}
	case kStrangeEventRestriction_Competitive:
	{
		return pLocalizationProvider->Find( "ItemTypeDescStrangeFilterCompetitive" );
	}
#endif // PROJECT_TF

	case kStrangeEventRestriction_VictimSteamAccount:
		return FindAccountPersonaName( unRestrictionValue );
	}

	return NULL;
}

bool CEconItemDescription::BGenerate_ItemLevelDesc_StrangeNameAndStats( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem, const locchar_t *locTypename )
{
	CStrangeRankLocalizationGenerator RankGenerator( pLocalizationProvider, pEconItem, TF_ANTI_IDLEBOT_VERIFICATION_ONLY_ARG_BOOL_TRUE( m_pHashContext == NULL ) );
	if ( !RankGenerator.IsValid() )
		return false;
	
	// For Collection Items
	if ( GetPaintKitDefIndex( pEconItem ) )
	{
		AddDescLine( CConstructLocalizedString( pLocalizationProvider->Find( "Attrib_stattrakmodule" ), RankGenerator.GetRankLocalized() ), 
			ATTRIB_COL_STRANGE, 
			kDescLineFlag_Misc 
		);
		
		// Are we tracking alternate stats as well?
		for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
		{
			const CEconItemAttributeDefinition *pKillEaterAltAttrDef = GetKillEaterAttr_Score( i ),
				*pKillEaterAltScoreTypeAttrDef = GetKillEaterAttr_Type( i );
			if ( !pKillEaterAltAttrDef || !pKillEaterAltScoreTypeAttrDef )
				continue;

			uint32 unKillEaterAltScore;
			if ( !pEconItem->FindAttribute( pKillEaterAltAttrDef, &unKillEaterAltScore ) )
				continue;

			// Older items can optionally not specify a type attribute at all and have an implicit "I'm tracking
			// kills" zeroth attribute. We require a score type for any slot besides that.
			if ( i != 0 && !pEconItem->FindAttribute( pKillEaterAltScoreTypeAttrDef ) )
				continue;

			const uint32 unKillEaterAltType = GetScoreTypeForKillEaterAttr( pEconItem, pKillEaterAltScoreTypeAttrDef );

			AddDescLine( CConstructLocalizedString( pLocalizationProvider->Find( "ItemTypeDescKillEaterAltv2" ),
				unKillEaterAltScore,
				GetLocalizedStringForKillEaterTypeAttr( pLocalizationProvider, unKillEaterAltType ),
				*CStrangeRestrictionAttrWrapper( pLocalizationProvider, GetLocalizedStringForStrangeRestrictionAttr( pLocalizationProvider, pEconItem, i ) ) ),
				ATTRIB_COL_LEVEL,
				kDescLineFlag_Misc );		// strange item scores past the first are not considered part of the type
		}

		return true;
	} // End Collection Items

	// Normal old way

	// Look for Limited Item Attr
	bool bLimitedQuantity = false;
	static CSchemaAttributeDefHandle pAttrDef_LimitedQuantityItem( "limited quantity item" );
	bLimitedQuantity = pEconItem->FindAttribute( pAttrDef_LimitedQuantityItem );

	AddDescLine( CConstructLocalizedString( pLocalizationProvider->Find( "ItemTypeDescKillEater" ),
												RankGenerator.GetRankLocalized(),
												locTypename ? locTypename : LOCCHAR(""),
												RankGenerator.GetStrangeScore(),
												GetLocalizedStringForKillEaterTypeAttr( pLocalizationProvider, RankGenerator.GetStrangeType() ),
												*CStrangeRestrictionAttrWrapper( pLocalizationProvider, GetLocalizedStringForStrangeRestrictionAttr( pLocalizationProvider, pEconItem, RankGenerator.GetUsedStrangeSlot() ) ),
												RankGenerator.GetRankSecondaryLocalized() ? RankGenerator.GetRankSecondaryLocalized() : LOCCHAR(""), 
												bLimitedQuantity ? pLocalizationProvider->Find( "LimitedQualityDesc" ) : LOCCHAR("")
												),
					bLimitedQuantity ? ATTRIB_COL_LIMITED_QUANTITY : ATTRIB_COL_LEVEL,
					kDescLineFlag_Type );

	// Are we tracking alternate stats as well?
	for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
	{
		const CEconItemAttributeDefinition *pKillEaterAltAttrDef			= GetKillEaterAttr_Score(i),
										   *pKillEaterAltScoreTypeAttrDef	= GetKillEaterAttr_Type(i);
		if ( !pKillEaterAltAttrDef || !pKillEaterAltScoreTypeAttrDef )
			continue;

		uint32 unKillEaterAltScore;
		if ( !pEconItem->FindAttribute( pKillEaterAltAttrDef, &unKillEaterAltScore ) )
			continue;

		// Older items can optionally not specify a type attribute at all and have an implicit "I'm tracking
		// kills" zeroth attribute. We require a score type for any slot besides that.
		if ( i != 0 && !pEconItem->FindAttribute( pKillEaterAltScoreTypeAttrDef ) )
			continue;

		const uint32 unKillEaterAltType = GetScoreTypeForKillEaterAttr( pEconItem, pKillEaterAltScoreTypeAttrDef );
			
		// Skip if this is our primary stat and we already output it above.
		if ( unKillEaterAltType == RankGenerator.GetStrangeType() )
			continue;

		AddDescLine( CConstructLocalizedString( pLocalizationProvider->Find( "ItemTypeDescKillEaterAlt" ),
												unKillEaterAltScore,
												GetLocalizedStringForKillEaterTypeAttr( pLocalizationProvider, unKillEaterAltType ),
												*CStrangeRestrictionAttrWrapper( pLocalizationProvider, GetLocalizedStringForStrangeRestrictionAttr( pLocalizationProvider, pEconItem, i ) ) ),
					 ATTRIB_COL_LEVEL,
					 kDescLineFlag_Misc );		// strange item scores past the first are not considered part of the type
	}

	return true;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
uint32 GetItemDescriptionDisplayLevel( const IEconItemInterface *pEconItem )
{
	Assert( pEconItem );

	static CSchemaAttributeDefHandle pAttrDef_WideItemLevel( "wide item level" );

	uint32 unWideLevelValue;
	if ( pEconItem->FindAttribute( pAttrDef_WideItemLevel, &unWideLevelValue ) )
		return unWideLevelValue;

	return pEconItem->GetItemLevel();
}

void CEconItemDescription::Generate_ItemLevelDesc_Default( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem, const locchar_t *locTypename )
{
	// By default, items will only show the level if there is an item type to go along with it.
	// Combined, these will build a string like "Level 10 Shotgun". We allow a custom attribute
	// to force the level to be displayed by itself even if there is no item class ("Level 10").
	static CSchemaAttributeDefHandle pAttrDef_ForceLevelDisplay( "force_level_display" );

	item_definition_index_t usDefIndex = pEconItem->GetItemDefIndex();


#ifdef CLIENT_DLL
	const bool bIsStoreItem = IsStorePreviewItem( pEconItem );
	const bool bIsPreviewItem = pEconItem->GetFlags() & kEconItemFlagClient_Preview;

	// If the item doesn't have a valid itemID, we'll just use the locTypename for the item level description. 
	// We don't want to display "Level 0 Hat" in places like the Mann Co. Store and Armory. We'll just display "Hat".
	if ( bIsStoreItem || bIsPreviewItem || pEconItem->GetItemDefinition()->GetRarity() != k_unItemRarity_Any )
	{
		if ( locTypename && *locTypename )
		{
			AddDescLine( locTypename, ATTRIB_COL_LEVEL, kDescLineFlag_Type, NULL, usDefIndex );
		}
		return;
	}
#endif

	float fForceLevelDisplayValue;
	bool bForceLevelDisplay = FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pEconItem, pAttrDef_ForceLevelDisplay, &fForceLevelDisplayValue )
							&& fForceLevelDisplayValue > 0.0f;
		
	if ( ( locTypename && *locTypename ) || bForceLevelDisplay )
	{
		if ( locTypename )
		{
			// How are we going to format our level number and base type string?
			const locchar_t *pszFormatString = NULL;

#ifdef PROJECT_TF
			static CSchemaAttributeDefHandle pAttrDef_OverrideItemLevelDescString( CTFItemSchema::k_rchOverrideItemLevelDescStringAttribName );
			static const char *s_pszCustomItemLevelDescLocalizationTokens[] =
			{
				"ItemTypeDescCustomLevelString_MvMTour",
			};

			// ...are we going to use a custom format string specified in an attribute?
			uint32 unOverrideItemLevelDescString = 0;
			if ( pEconItem->FindAttribute( pAttrDef_OverrideItemLevelDescString, &unOverrideItemLevelDescString )
				&& unOverrideItemLevelDescString != 0
				&& unOverrideItemLevelDescString <= ARRAYSIZE( s_pszCustomItemLevelDescLocalizationTokens ) ) 
			{
				const char *pszLevelLocalizationToken = s_pszCustomItemLevelDescLocalizationTokens[ unOverrideItemLevelDescString - 1 ];
				Assert( pszLevelLocalizationToken );

				pszFormatString = pLocalizationProvider->Find( pszLevelLocalizationToken );
			}
#endif // PROJECT_TF

			// Either we didn't have a custom override attribute, or we did and it had an invalid value, or it had a valid
			// value but the localization system failed to find something for that key. In any event, we fall back to our default
			// format string here.
			if ( pszFormatString == NULL )
			{
				bool bLimitedQuantity = false;
				static CSchemaAttributeDefHandle pAttrDef_LimitedQuantityItem( "limited quantity item" );
				bLimitedQuantity = pEconItem->FindAttribute( pAttrDef_LimitedQuantityItem );

#if defined( TF_CLIENT_DLL )
				if ( pEconItem->GetItemDefinition()->GetItemClass() && V_strcmp( pEconItem->GetItemDefinition()->GetItemClass(), "map_token" ) == 0 )
				{
					// For map stamps on the client we can show how many hours they've played each map
					// And how many times they've donated to it instead of the generic "level"
					for ( int i = 0; i < GetItemSchema()->GetMapCount(); i++ )
					{
						const MapDef_t* pMap = GetItemSchema()->GetMasterMapDefByIndex( i );

						if ( pMap->mapStampDef != pEconItem->GetItemDefinition() )
							continue;

						int nItemLevel = MapInfo_GetDonationAmount( steamapicontext->SteamUser()->GetSteamID().GetAccountID(), pMap->pszMapName );

						MapStats_t &mapStats = GetMapStats( pMap->GetStatsIdentifier() );
						int nNumHours = ( mapStats.accumulated.m_iStat[TFMAPSTAT_PLAYTIME] ) / ( 60 /*sec*/ * 60 /*min*/ );

						AddDescLine( CConstructLocalizedString( pLocalizationProvider->Find( "ItemTypeDescCustomLevelString_MapStamp" ), (uint32)nItemLevel, (uint32)nNumHours ), ATTRIB_COL_LEVEL, kDescLineFlag_Type );
						return;
					}
				}
				else 
#endif
				{
					if ( bLimitedQuantity )
					{
						// Limited Item Description
						pszFormatString = pLocalizationProvider->Find( "ItemTypeDescLimited" );
						AddDescLine( CConstructLocalizedString(
								pszFormatString,
								GetItemDescriptionDisplayLevel( pEconItem ),
								locTypename,
								pLocalizationProvider->Find( "LimitedQualityDesc" ) ),
							ATTRIB_COL_LIMITED_QUANTITY,
							kDescLineFlag_Type,
							NULL,
							usDefIndex
							);
						return;
					}
					pszFormatString = pLocalizationProvider->Find( "ItemTypeDesc" );
				}
			}

			// If we still don't have a format string here, it means our default also failed, but CConstructLocalizedString will
			// handle that safely.
			AddDescLine( CConstructLocalizedString( pszFormatString, GetItemDescriptionDisplayLevel( pEconItem ), locTypename ), ATTRIB_COL_LEVEL, kDescLineFlag_Type, NULL, usDefIndex );
		}
		else
		{
			Assert( bForceLevelDisplay );

			AddDescLine( CConstructLocalizedString( pLocalizationProvider->Find( "ItemTypeDescNoLevel" ), GetItemDescriptionDisplayLevel( pEconItem ) ), ATTRIB_COL_LEVEL, kDescLineFlag_Type, NULL, usDefIndex );
		}
	}
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItemDescription::Generate_ItemLevelDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	const GameItemDefinition_t *pItemDef = pEconItem->GetItemDefinition();
	if ( !pItemDef )
		return;

	const locchar_t *locTypename = pLocalizationProvider->Find( pItemDef->GetItemTypeName() );

	// Kill-eating weapons replace the standard weapon name/level line with a label
	// describing the current class of the item instead of the level. This overrides
	// even "force_level_display".
	if ( BGenerate_ItemLevelDesc_StrangeNameAndStats( pLocalizationProvider, pEconItem, locTypename ) )
		return;

	// Not strange, but if you are paint kitted or have a collection reference dont create this
	if ( GetPaintKitDefIndex( pEconItem ) || pItemDef->GetCollectionReference() )
		return;

	// If we didn't generate a fancy strange name, we fall back to our default behavior.
	Generate_ItemLevelDesc_Default( pLocalizationProvider, pEconItem, locTypename );
}


// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItemDescription::Generate_CraftTag( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	static CSchemaAttributeDefHandle pAttribDef_MakersMarkId( "makers mark id" );

	attrib_value_t value;
	if ( !pEconItem->FindAttribute( pAttribDef_MakersMarkId, &value ) )
		return;

	AddAttributeDescription( pLocalizationProvider, pAttribDef_MakersMarkId, value );
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItemDescription::Generate_StyleDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	const GameItemDefinition_t *pItemDef = pEconItem->GetItemDefinition();
	if ( !pItemDef )
		return;

	const CEconStyleInfo *pStyle = pItemDef->GetStyleInfo( pEconItem->GetStyle() );
	if ( !pStyle )
		return;

	const locchar_t *loc_StyleName = pLocalizationProvider->Find( pStyle->GetName() );
	if ( !loc_StyleName )
		return;
	
	AddDescLine( CConstructLocalizedString( pLocalizationProvider->Find( "#Econ_Style_Desc" ), loc_StyleName ), ATTRIB_COL_LEVEL, kDescLineFlag_Misc );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDescription::Generate_HolidayRestriction( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	const GameItemDefinition_t *pItemDef = pEconItem->GetItemDefinition();
	if ( !pItemDef )
		return;

	const char *pszHolidayRestriction = pItemDef->GetHolidayRestriction();
	if ( !pszHolidayRestriction )
		return;

	// Report any special restrictions. We'll output in a different color depending on whether or not
	// the restriction currently prevents the item from showing up.
	LocalizedAddDescLine( pLocalizationProvider,
						  CFmtStr( "Econ_holiday_restriction_%s", pszHolidayRestriction ).Access(),
						  EconHolidays_IsHolidayActive( EconHolidays_GetHolidayForString( pszHolidayRestriction ), CRTime::RTime32TimeCur() ) ? ATTRIB_COL_LEVEL : ATTRIB_COL_NEGATIVE,
						  kDescLineFlag_Misc );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDescription::Generate_QualityDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	// Does this item quality have additional description information that goes along with
	// besides the usual name/coloration changes?
	const char *pszQualityDescLocalizationKey = NULL;

	switch( pEconItem->GetQuality() )
	{
	case AE_SELFMADE:
		pszQualityDescLocalizationKey = "Attrib_Selfmade_Description";
		break;
	case AE_COMMUNITY:
		pszQualityDescLocalizationKey = "Attrib_Community_Description";
		break;
	}

	// We don't need to do anything special.
	if ( !pszQualityDescLocalizationKey )
		return;

	// If this item has a particle system attached but doesn't have the attribute that we usually use
	// to attach particles, we hack it and dump out an extra line to show the particle system description
	// as well.
	static CSchemaAttributeDefHandle pAttrDef_ParticleEffect( "attach particle effect" );
	static attachedparticlesystem_t *pSparkleSystem = GetItemSchema()->FindAttributeControlledParticleSystem( "community_sparkle" );

	// If the schema understands these properties...
	if ( pAttrDef_ParticleEffect && pSparkleSystem )
	{
		// ...and we don't have a real particle effect attribute attribute...
		if ( !pEconItem->FindAttribute( pAttrDef_ParticleEffect ) )
		{
			// check for Unusual Cap def index (1173)
			// We manually assign unusual effect to content author. No community sparkle
			if ( pEconItem->GetItemDefIndex() != 1173 )
			{
				// ...then manually add the description as if we did.
				float flSystemID = pSparkleSystem->nSystemID;
				AddAttributeDescription( pLocalizationProvider, pAttrDef_ParticleEffect, *(uint32*)&flSystemID );
			}
		}
	}

	LocalizedAddDescLine( pLocalizationProvider, pszQualityDescLocalizationKey, ATTRIB_COL_NEUTRAL, kDescLineFlag_Misc );
}

//-----------------------------------------------------------------------------
void CEconItemDescription::Generate_ItemRarityDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	const CEconItemDefinition* pItemDef = pEconItem->GetItemDefinition();
	const CEconItemRarityDefinition* pItemRarity = GetItemSchema()->GetRarityDefinition( pEconItem->GetRarity() );
	if ( !pItemRarity )
		return;
	
	const char *pszTooltip = "TFUI_InvTooltip_Rarity";

	attrib_colors_t colorRarity = pItemRarity->GetAttribColor();

	const locchar_t *loc_RarityText = pLocalizationProvider->Find( pItemRarity->GetLocKey() );
	const locchar_t *locTypename = pLocalizationProvider->Find( pItemDef->GetItemTypeName() );
	const locchar_t *loc_WearText = LOCCHAR("");

	float flWear = 0;
	if ( GetPaintKitWear( pEconItem, flWear ) )
	{
		loc_WearText = pLocalizationProvider->Find( GetWearLocalizationString( flWear ) );
	}
	else
	{
		pszTooltip = "TFUI_InvTooltip_RarityNoWear";
	}

	AddDescLine( CConstructLocalizedString( pLocalizationProvider->Find( pszTooltip ), loc_RarityText, locTypename, loc_WearText ), colorRarity, kDescLineFlag_Misc );
}


//-----------------------------------------------------------------------------
void CEconItemDescription::Generate_WearAmountDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	if ( !GetPaintKitDefIndex( pEconItem ) )
		return;

	Assert( pLocalizationProvider );
	Assert( pEconItem );

	float flWear = 0;
	if ( GetPaintKitWear( pEconItem, flWear ) )
	{
		locchar_t loc_WearText[MAX_ATTRIBUTE_DESCRIPTION_LENGTH];
		
		loc_scpy_safe( loc_WearText, pLocalizationProvider->Find( "#TFUI_InvTooltip_Wear" ) );
		loc_scat_safe( loc_WearText, LOCCHAR( " " ) );
		loc_scat_safe( loc_WearText, pLocalizationProvider->Find( GetWearLocalizationString( flWear ) ) );

		AddDescLine( loc_WearText, ATTRIB_COL_NEUTRAL, kDescLineFlag_Misc );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDescription::Generate_ItemDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	// Show the custom description if it has one.
	const char *utf8_CustomDesc = pEconItem->GetCustomDesc();
	if ( utf8_CustomDesc && utf8_CustomDesc[0] )
	{
		locchar_t loc_CustomDesc[ MAX_ITEM_DESC_LENGTH ];
		pLocalizationProvider->ConvertUTF8ToLocchar( utf8_CustomDesc, loc_CustomDesc, sizeof( loc_CustomDesc ) );

		locchar_t loc_CustomDescWithQuotes[ MAX_ITEM_DESC_LENGTH ];
		loc_scpy_safe( loc_CustomDescWithQuotes, LOCCHAR("''") );
		loc_scat_safe( loc_CustomDescWithQuotes, loc_CustomDesc );
		loc_scat_safe( loc_CustomDescWithQuotes, LOCCHAR("''") );

		AddDescLine( loc_CustomDescWithQuotes, ATTRIB_COL_NEUTRAL, kDescLineFlag_Desc | kDescLineFlag_UserProvided );
		return;
	}

	// No custom description -- see if the item has a default description as part of the definition.
	const GameItemDefinition_t *pItemDef = pEconItem->GetItemDefinition();
	if ( !pItemDef )
		return;

	// Add any additional item description
	if ( pItemDef->GetItemDesc() )
	{
		LocalizedAddDescLine( pLocalizationProvider, pItemDef->GetItemDesc(), ATTRIB_COL_NEUTRAL, kDescLineFlag_Desc );
	}

	// If we're a store preview item, show the available styles in the tooltip so potential buyers
	// have more information.
	if ( IsStorePreviewItem( pEconItem ) )
	{
		if ( pItemDef && pItemDef->GetNumStyles() > 0 )
		{
			AddEmptyDescLine();
			LocalizedAddDescLine( pLocalizationProvider, "#Store_AvailableStyles_Header", ATTRIB_COL_LEVEL, kDescLineFlag_Misc );
			
			for ( int i = 0; i < pItemDef->GetNumStyles(); i++ )
			{
				LocalizedAddDescLine( pLocalizationProvider, pItemDef->GetStyleInfo( i )->GetName(), ATTRIB_COL_LEVEL, kDescLineFlag_Misc );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

// If we have at least this many items in our bundle then display multiple entries
// per line. Otherwise display one item per line for clarity.
enum { kDescription_CompositeBundleEntriesCount = 15 };

void CEconItemDescription::Generate_Bundle( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	const GameItemDefinition_t *pItemDef = pEconItem->GetItemDefinition();
	if ( !pItemDef )
		return;

	const bundleinfo_t *pBundleInfo = pItemDef->GetBundleInfo();
	if ( !pBundleInfo )
		return;

#if defined( CLIENT_DLL )
	// handle the map stamp bundle differently when it's the tooltip
	if ( m_bIsToolTip && !Q_strcmp( pItemDef->GetItemBaseName(), "#TF_Bundle_MapTokens" ) )
		return;
#endif // CLIENT_DLL

	enum EBundleEntryDisplayStyle
	{
		kBundleDisplay_SingleEntry,			// one entry per line
		kBundleDisplay_PairEntry,			// "Some Item, Some Other Item," (with ending comma)
		kBundleDisplay_PairEntryFinal,		// "Some Item, Some Other Item" (with no ending comma)
	};


	CUtlVector< item_definition_index_t > vecPackBundlesAdded;

	FOR_EACH_VEC( pBundleInfo->vecItemDefs, i )
	{
		// Sanity check.
		const CEconItemDefinition *pBundleItemDef = pBundleInfo->vecItemDefs[i];
		if ( !pBundleItemDef )
			continue;

		// If the current item is part of a pack bundle, add the pack bundle to the description, rather than the individual item
#ifdef DOTA
		if ( pBundleItemDef->IsPackItem() )
		{
			const CUtlVector< CEconItemDefinition * > &vecPackBundleItemDefs = pBundleItemDef->GetOwningPackBundles();

			item_definition_index_t usPackBundleItemDefIndex = vecPackBundleItemDefs[i]->GetDefinitionIndex();
			if ( vecPackBundlesAdded.HasElement( usPackBundleItemDefIndex ) )
				continue;

			// Remember the def index so we don't add the reference to the pack bundle more than once
			vecPackBundlesAdded.AddToTail( usPackBundleItemDefIndex );

			// Now, point pBundleItemDef at the pack bundle itself and carry on
			pBundleItemDef = pPackBundleItemDef;
		}
#endif

		// Figure out which display style to use for this item. By default we put one item one each line...
		EBundleEntryDisplayStyle eDisplayStyle = kBundleDisplay_SingleEntry;
		
		// ...but if we have a whole bunch of items in a single bundle, we lump them together two per line to
		// save space. Only do this on the client. On the GC, use single lines so that link meta data can be passed
		// along per-line to the store bundle pages.
#if defined( CLIENT_DLL )
		if ( m_bIsToolTip )
		{
			if ( pBundleInfo->vecItemDefs.Count() >= kDescription_CompositeBundleEntriesCount )
			{
				const int iRemainingItems = pBundleInfo->vecItemDefs.Count() - i;

				// We distinguish between "there are at least three entries left", which means we'll end the line
				// with a comma, etc.
				if ( iRemainingItems > 2 )
				{
					eDisplayStyle = kBundleDisplay_PairEntry;
				}
				// ...or if these are our very last two items, we list our last two items and that's it.
				else if ( iRemainingItems == 2 )
				{
					eDisplayStyle = kBundleDisplay_PairEntryFinal;
				}
			}
		}
#endif // CLIENT_DLL

		if ( eDisplayStyle == kBundleDisplay_SingleEntry )
		{
			// pBundleItemDef will point at the pack bundle if pBundleItemDef is a pack item. In DotA, pack bundles *only* include pack items, whereas in TF, there are bundles which include some items where are individually for sale and others that are not. For example, the Scout Starter Bundle, etc.
#ifdef DOTA
			LocalizedAddDescLine( pLocalizationProvider, pBundleItemDef->GetItemBaseName(), ATTRIB_COL_BUNDLE_ITEM, kDescLineFlag_Misc, NULL, pBundleItemDef->GetDefinitionIndex() );
#else
			LocalizedAddDescLine( pLocalizationProvider, pBundleItemDef->GetItemBaseName(), pBundleItemDef->IsPackItem() ? ATTRIB_COL_NEUTRAL : ATTRIB_COL_BUNDLE_ITEM, kDescLineFlag_Misc, NULL, pBundleItemDef->IsPackItem() ? INVALID_ITEM_DEF_INDEX : pBundleItemDef->GetDefinitionIndex(), !pBundleItemDef->IsPackItem() );
#endif
		}
		else
		{
			Assert( eDisplayStyle == kBundleDisplay_PairEntry || eDisplayStyle == kBundleDisplay_PairEntryFinal );

			const CEconItemDefinition *pOtherBundleItem = pBundleInfo->vecItemDefs[i + 1];
			const char *pOtherBundleItemBaseName = pOtherBundleItem ? pOtherBundleItem->GetItemBaseName() : "";

			AddDescLine( CConstructLocalizedString( pLocalizationProvider->Find( eDisplayStyle == kBundleDisplay_PairEntryFinal ? "Econ_Bundle_Double" : "Econ_Bundle_DoubleContinued" ),
													pLocalizationProvider->Find( pBundleItemDef->GetItemBaseName() ),
													pLocalizationProvider->Find( pOtherBundleItemBaseName ) ),
						 ATTRIB_COL_BUNDLE_ITEM,
						 kDescLineFlag_Misc,
						 NULL,
						 pBundleItemDef->GetDefinitionIndex() );

			// We consumed a second element as well.
			i++;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDescription::Generate_GiftedBy( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	static CSchemaAttributeDefHandle pAttrDef_GiftedBy( "gifter account id" );
	static CSchemaAttributeDefHandle pAttrDef_EventDate( "event date" );

	attrib_value_t val_GifterId;
	if ( pAttrDef_GiftedBy && pEconItem->FindAttribute( pAttrDef_GiftedBy, &val_GifterId ) )
	{
		// Who gifted us this present?
		AddAttributeDescription( pLocalizationProvider, pAttrDef_GiftedBy, val_GifterId );

		// Do we also have (optional) information about when it happened?
		attrib_value_t val_EventData;
		if ( pAttrDef_EventDate && pEconItem->FindAttribute( pAttrDef_EventDate, &val_EventData ) )
		{
			AddAttributeDescription( pLocalizationProvider, pAttrDef_EventDate, val_EventData );
		}
	}
}

#ifdef PROJECT_TF
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static bool IsDuelingMedal( const GameItemDefinition_t *pItemDef )
{
	static CSchemaItemDefHandle pAttrDef_DuelingMedals[] =
	{
		CSchemaItemDefHandle( "Duel Medal Bronze" ),	
		CSchemaItemDefHandle( "Duel Medal Silver" ),
		CSchemaItemDefHandle( "Duel Medal Gold" ),
		CSchemaItemDefHandle( "Duel Medal Plat" ),
	};

	Assert( pItemDef );

	for ( int i = 0; i < ARRAYSIZE( pAttrDef_DuelingMedals ); i++ )
		if ( pItemDef == pAttrDef_DuelingMedals[i] )
			return true;

	return false;
}

void CEconItemDescription::Generate_DuelingMedal( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	static CSchemaAttributeDefHandle pAttrDef_EventDate( "event date" );

	const GameItemDefinition_t *pItemDef = pEconItem->GetItemDefinition();
	if ( !pItemDef )
		return;

	if ( !IsDuelingMedal( pItemDef ) )
		return;

	const CTFDuelSummary *pDuelSummary = FindAccountTypeCacheSingleton<CTFDuelSummary>( pEconItem->GetAccountID(), CTFDuelSummary::k_nTypeID );
	if ( !pDuelSummary )
		return;

	// Add the date received first.
	attrib_value_t value;
	if ( !pEconItem->FindAttribute( pAttrDef_EventDate, &value ) )
		return;

	// We feed our format-string parameters in via KeyValues.
	KeyValues *pKeyValues = new KeyValues( "DuelStrings" );

	CLocalizedRTime32 time = { pDuelSummary->Obj().last_duel_timestamp(), false, pLocalizationProvider TF_ANTI_IDLEBOT_VERIFICATION_ONLY_COMMA TF_ANTI_IDLEBOT_VERIFICATION_ONLY_ARG( m_pHashContext ) };

	TypedKeyValuesStringHelper<locchar_t>::Write( pKeyValues, "last_date",	 CLocalizedStringArg<CLocalizedRTime32>( time ).GetLocArg() );
	TypedKeyValuesStringHelper<locchar_t>::Write( pKeyValues, "wins",		 CLocalizedStringArg<uint32>( pDuelSummary->Obj().duel_wins() ).GetLocArg() );
	TypedKeyValuesStringHelper<locchar_t>::Write( pKeyValues, "last_target", FindAccountPersonaName( pDuelSummary->Obj().last_duel_account_id() ) );

	// What happened in our last duel? This will be used as a format string to wrap the above data.
	const char *pszTextFormat;
	switch ( pDuelSummary->Obj().last_duel_status() )
	{
	case kDuelStatus_Loss: 
		pszTextFormat = "#TF_Duel_Desc_Lost";
		break;
	case kDuelStatus_Tie:
		pszTextFormat = "#TF_Duel_Desc_Tied";
		break;
	case kDuelStatus_Win:
	default:
		pszTextFormat = "#TF_Duel_Desc_Won";
		break;
	}

	// Output our whole description.
	AddEmptyDescLine();
	AddAttributeDescription( pLocalizationProvider, pAttrDef_EventDate, value );
	AddEmptyDescLine();
	AddDescLine( CConstructLocalizedString( pLocalizationProvider->Find( pszTextFormat ), pKeyValues ), ATTRIB_COL_NEUTRAL, kDescLineFlag_Misc );

	pKeyValues->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDescription::Generate_MapContributor( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	static CSchemaItemDefHandle pItemDef_WorldTraveler( "World Traveler" );
	if ( !pItemDef_WorldTraveler || pEconItem->GetItemDefinition() != pItemDef_WorldTraveler )
		return;

	GCSDK::CSharedObjectTypeCache *pTypeCache = FindAccountTypeCache( pEconItem->GetAccountID(), CTFMapContribution::k_nTypeID );
	if ( !pTypeCache )
		return;

	static const char *kDonationLevels[] =
	{
		"#TF_MapDonationLevel_Bronze",
		"#TF_MapDonationLevel_Silver",
		"#TF_MapDonationLevel_Gold",
		"#TF_MapDonationLevel_Platinum",
		"#TF_MapDonationLevel_Diamond",
		"#TF_MapDonationLevel_Australium1",
		"#TF_MapDonationLevel_Australium2",
		"#TF_MapDonationLevel_Australium3",
		"#TF_MapDonationLevel_Unobtainium"
	};
	const int kNumDonationLevels = ARRAYSIZE( kDonationLevels );
	const int kNumDonationsPerLevel = 25;

	CUtlVector<const CTFMapContribution *> vecContributionsPerLevel[ kNumDonationLevels ];

	for ( uint32 i = 0; i < pTypeCache->GetCount(); ++i )
	{
		CTFMapContribution *pMapContribution = (CTFMapContribution*)( pTypeCache->GetObject( i ) );
		const CEconItemDefinition *pMapItemDef = GetItemSchema()->GetItemDefinition( pMapContribution->Obj().def_index() );
		if ( pMapItemDef )
		{
			int iLevel = MIN( pMapContribution->Obj().contribution_level() / kNumDonationsPerLevel, kNumDonationLevels - 1 );
			vecContributionsPerLevel[iLevel].AddToTail( pMapContribution );
		}
	}
	for ( int i = 0; i < kNumDonationLevels; ++i )
	{
		const CUtlVector<const CTFMapContribution *>& vecContributions = vecContributionsPerLevel[i];
		if ( vecContributions.Count() > 0 )
		{
			// Add header like "Silver:" to show the level of contribution for each of the maps following.
			LocalizedAddDescLine( pLocalizationProvider, kDonationLevels[i], ATTRIB_COL_ITEMSET_NAME, kDescLineFlag_Misc );

			// Add a label showing the map names and number of contributions for each map.
			locchar_t tempDescription[MAX_ITEM_DESCRIPTION_LENGTH] = { 0 };
			FOR_EACH_VEC( vecContributions, j )
			{
				const CTFMapContribution *pMapContribution = vecContributions[j];
				const CEconItemDefinition *pMapItemDef = GetItemSchema()->GetItemDefinition( pMapContribution->Obj().def_index() );
				Assert( pMapItemDef );

				const char *pszMapNameLocalizationToken = pMapItemDef->GetDefinitionString( "map_name", NULL );
				if ( pszMapNameLocalizationToken )
				{
					loc_sncat( tempDescription,	
							   CConstructLocalizedString( pLocalizationProvider->Find( "#Attrib_MapDonation" ),
														  pLocalizationProvider->Find( pszMapNameLocalizationToken ),
														  (uint32)pMapContribution->Obj().contribution_level() ),
							   MAX_ITEM_DESCRIPTION_LENGTH );

					if ( j < ( vecContributions.Count() - 1 ) )
					{
						loc_sncat( tempDescription, LOCCHAR( ", " ), MAX_ITEM_DESCRIPTION_LENGTH );
					}
				}
			}

			if ( tempDescription[0] )
			{
				AddDescLine( tempDescription, ATTRIB_COL_POSITIVE, kDescLineFlag_Misc );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDescription::Generate_MapStampBundleTooltip( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	if ( !m_bIsToolTip )
		return;

	const GameItemDefinition_t *pItemDef = pEconItem->GetItemDefinition();
	if ( !pItemDef )
		return;

	const bundleinfo_t *pBundleInfo = pItemDef->GetBundleInfo();
	if ( !pBundleInfo )
		return;

	// only handle the map stamp bundle
	if ( Q_strcmp( pItemDef->GetItemBaseName(), "#TF_Bundle_MapTokens" ) )
		return;

	locchar_t tempDescription[ MAX_ITEM_DESCRIPTION_LENGTH ] = { 0 };

	FOR_EACH_VEC( pBundleInfo->vecItemDefs, i )
	{
		// Sanity check.
		const CEconItemDefinition *pBundleItemDef = pBundleInfo->vecItemDefs[ i ];
		if ( !pBundleItemDef )
			continue;

		const char *pszMapNameLocalizationToken = pBundleItemDef->GetDefinitionString( "map_name", NULL );
		if ( pszMapNameLocalizationToken )
		{
			loc_sncat( tempDescription, pLocalizationProvider->FindSafe( pszMapNameLocalizationToken ), MAX_ITEM_DESCRIPTION_LENGTH );

			if ( i < ( pBundleInfo->vecItemDefs.Count() - 1 ) )
			{
				loc_sncat( tempDescription, LOCCHAR( ", " ), MAX_ITEM_DESCRIPTION_LENGTH );
			}
		}
	}

	if ( tempDescription[ 0 ] )
	{
		AddDescLine( tempDescription, ATTRIB_COL_BUNDLE_ITEM, kDescLineFlag_Misc );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDescription::Generate_FriendlyHat( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	static CSchemaItemDefHandle pItemDef_FriendlyHat( "Friendly Item" );
	if ( !pItemDef_FriendlyHat || pEconItem->GetItemDefinition() != pItemDef_FriendlyHat )
		return;

	const CTFPlayerInfo *pPlayerInfo = FindAccountTypeCacheSingleton<CTFPlayerInfo>( pEconItem->GetAccountID(), CTFPlayerInfo::k_nTypeID );
	if ( !pPlayerInfo )
		return;

	AddDescLine( CConstructLocalizedString( pLocalizationProvider->Find( "#Attrib_NewUsersHelped" ), (uint32)pPlayerInfo->Obj().num_new_users_helped() ), ATTRIB_COL_POSITIVE, kDescLineFlag_Misc );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDescription::Generate_SaxxyAwardDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	// Don't display anything for items besides the Saxxy itself.
	static CSchemaItemDefHandle pItemDef_Saxxy( "Saxxy" );
	static CSchemaItemDefHandle pItemDef_MemoryMaker( "Memory Maker" );
	if ( ( !pItemDef_Saxxy || pEconItem->GetItemDefinition() != pItemDef_Saxxy ) &&
		 ( !pItemDef_MemoryMaker || pEconItem->GetItemDefinition() != pItemDef_MemoryMaker ) )
	{
		return;
	}

	// Output our award category if present, or abort if absent.
	static CSchemaAttributeDefHandle pAttrDef_SaxxyAwardCategory( "saxxy award category" );
	static CSchemaAttributeDefHandle pAttrDef_EventDate( "event date" );

	uint32 unAwardCategory,
		   unEventDate;
	if ( !pEconItem->FindAttribute( pAttrDef_SaxxyAwardCategory, &unAwardCategory ) ||
		 !pEconItem->FindAttribute( pAttrDef_EventDate, &unEventDate ) )
	{
		return;
	}

	CRTime cTime( unEventDate );
	cTime.SetToGMT( false );

	const char *pszFormatString = "#Attrib_SaxxyAward";
	if ( pEconItem->GetItemDefinition() == pItemDef_MemoryMaker )
	{
		pszFormatString = "#Attrib_MemoryMakerAward";
	}

	AddDescLine( CConstructLocalizedString( pLocalizationProvider->Find( pszFormatString ),
											pLocalizationProvider->Find( CFmtStr( "Replay_Contest_Category%d", unAwardCategory ).Access() ),
											(uint32)cTime.GetYear() ),
			     ATTRIB_COL_POSITIVE,
				 kDescLineFlag_Misc );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDescription::Generate_MvmChallenges( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	// Look for our "challenges completed" attribute. If we have this, we assume we're a badge
	// of some kind. If we don't, we don't display MvM information. This would be a little weird
	// for level 0 badges that have no completed challenges, but those are something that currently
	// exist.
	static CSchemaAttributeDefHandle pAttrDef_ChallengesCompleted( CTFItemSchema::k_rchMvMChallengeCompletedMaskAttribName );

	uint32 unMask = 0;
	if ( !pEconItem->FindAttribute( pAttrDef_ChallengesCompleted, &unMask ) )
		return;
	
	// Look through our list of MvM tours to figure out which badge this came from. The badge itself
	// doesn't know and we need this information to figure out which completion bits map to which
	// missions.
	const MvMTour_t *pTour = NULL;

	FOR_EACH_VEC( GetItemSchema()->GetMvmTours(), i )
	{
		const MvMTour_t& tour = GetItemSchema()->GetMvmTours()[i];

		if ( tour.m_pBadgeItemDef == pEconItem->GetItemDefinition() )
		{
			pTour = &tour;
			break;
		}
	}

	// Couldn't find a tour matching this badge? (This can happen if a client has a busted schema or if
	// we remove a tour for some reason.)
	if ( !pTour )
		return;

	const CUtlVector<MvMMission_t>& vecAllMissions = GetItemSchema()->GetMvmMissions();
	CUtlVector<int> vecCompletedMissions;

	FOR_EACH_VEC( pTour->m_vecMissions, i )
	{
		// Make sure our mission index is valid based on our current schema. If we're a client playing a
		// game during a GC roll, we could wind up looking at someone else's badge where they have a
		// mission that we don't understand.
		const int iMissionIndex = pTour->m_vecMissions[i].m_iMissionIndex;
		if ( !vecAllMissions.IsValidIndex( iMissionIndex ) )
			continue;

		const int iBadgeSlot = pTour->m_vecMissions[i].m_iBadgeSlot;
		if ( iBadgeSlot >= 0 && ((unMask & (1U << iBadgeSlot)) != 0) )
		{
			vecCompletedMissions.AddToTail( iMissionIndex );
		}
	}

	// Add a summary line for the number they have completed
	AddDescLine(
		CConstructLocalizedString(
			pLocalizationProvider->Find( "#Attrib_MvMChallengesCompletedSummary" ),
			uint32( vecCompletedMissions.Count() )
		),
		ATTRIB_COL_POSITIVE,
		kDescLineFlag_Misc
	);

	// Detail lines for each completed challenge
	FOR_EACH_VEC( vecCompletedMissions, i )
	{
		const MvMMission_t& mission = vecAllMissions[ vecCompletedMissions[i] ];
		const MvMMap_t& map = GetItemSchema()->GetMvmMaps()[ mission.m_iDisplayMapIndex ];
		const locchar_t *pszLocFmt = pLocalizationProvider->Find( "#Attrib_MvMChallengeCompletedDetail" );
		const locchar_t *pszLocMap = pLocalizationProvider->Find( map.m_sDisplayName.Get() );
		const locchar_t *pszLocChal = pLocalizationProvider->Find( mission.m_sDisplayName.Get() );
		if ( pszLocFmt && pszLocMap && pszLocChal )
		{
			CConstructLocalizedString locLine(
				pszLocFmt,
				pszLocMap,
				pszLocChal
			);
			AddDescLine(
				locLine,
				ATTRIB_COL_POSITIVE,
				kDescLineFlag_Misc
			);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDescription::Generate_SquadSurplusClaimedBy( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	static CSchemaAttributeDefHandle pAttrDef_SquadSurplusClaimer( "squad surplus claimer id" );
	static CSchemaAttributeDefHandle pAttrDef_EventDate( "event date" );

	attrib_value_t val_GifterId;
	if ( pAttrDef_SquadSurplusClaimer&& pEconItem->FindAttribute( pAttrDef_SquadSurplusClaimer, &val_GifterId ) )
	{
		// Who gifted us this present?
		AddAttributeDescription( pLocalizationProvider, pAttrDef_SquadSurplusClaimer, val_GifterId );

		// Do we also have (optional) information about when it happened?
		attrib_value_t val_EventData;
		if ( pAttrDef_EventDate && pEconItem->FindAttribute( pAttrDef_EventDate, &val_EventData ) )
		{
			AddAttributeDescription( pLocalizationProvider, pAttrDef_EventDate, val_EventData );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDescription::Generate_DynamicRecipe( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	// Gather our attributes we care about
	CRecipeComponentMatchingIterator componentIterator( pEconItem, NULL );
	pEconItem->IterateAttributes( &componentIterator );

	// Nothing to say, bail!
	if( !componentIterator.GetMatchingComponentInputs().Count() && 
		!componentIterator.GetMatchingComponentOutputs().Count() )
	{
		return;
	}

	// Add the no partial complete tag if the attribute exists
	static CSchemaAttributeDefHandle pAttrib_NoPartialComplete( "recipe no partial complete" );
	if ( pEconItem->FindAttribute( pAttrib_NoPartialComplete ) )
	{
		LocalizedAddDescLine( pLocalizationProvider, "TF_ItemDynamic_Recipe_No_Partial_Completion", ATTRIB_COL_POSITIVE, kDescLineFlag_Misc );
	}

	AddEmptyDescLine();

	if ( componentIterator.GetMatchingComponentInputs().Count() )
	{
		// Print out item input header
		LocalizedAddDescLine( pLocalizationProvider, "TF_ItemDynamic_Recipe_Inputs", ATTRIB_COL_NEUTRAL, kDescLineFlag_Misc );
		// Print out inputs
		FOR_EACH_VEC( componentIterator.GetMatchingComponentInputs(), i )
		{
			CAttribute_DynamicRecipeComponent attribValue;
			pEconItem->FindAttribute( componentIterator.GetMatchingComponentInputs()[i], &attribValue );

			const GameItemDefinition_t *pItemDef = dynamic_cast<GameItemDefinition_t *>( GetItemSchema()->GetItemDefinition( attribValue.def_index() ) );
			if ( !pItemDef )
				continue;

			int nCount = attribValue.num_required() - attribValue.num_fulfilled();

			// This is a completed component.  We don't want to show it (for now)
			if( nCount == 0 )
				continue;

			CEconItem tempItem;
			if ( !DecodeItemFromEncodedAttributeString( attribValue, &tempItem ) )
			{
				AssertMsg2( 0, "%s: Unable to decode dynamic recipe input attribute on item %llu.", __FUNCTION__, pEconItem->GetID() );
				continue;
			}

			locchar_t lineItem[256];
			locchar_t loc_ItemName[MAX_ITEM_NAME_LENGTH];
			GenerateLocalizedFullItemName( loc_ItemName, pLocalizationProvider, &tempItem, k_EGenerateLocalizedFullItemName_Default, TF_ANTI_IDLEBOT_VERIFICATION_ONLY_ARG_BOOL_TRUE( m_pHashContext == NULL ) );

			loc_sprintf_safe( lineItem,
			                  ( LOCCHAR_FMT_LOCPRINTF LOCCHAR( " x %d" ) ),
			                  loc_ItemName,
			                  nCount
			);

			AddDescLine( lineItem, ATTRIB_COL_ITEMSET_MISSING, kDescLineFlag_Misc );
		}

		AddEmptyDescLine();
	}

	// Print out outputs
	LocalizedAddDescLine( pLocalizationProvider, "TF_ItemDynamic_Recipe_Outputs", ATTRIB_COL_NEUTRAL, kDescLineFlag_Misc );
	FOR_EACH_VEC( componentIterator.GetMatchingComponentOutputs(), i )
	{
		CAttribute_DynamicRecipeComponent attribValue;
		pEconItem->FindAttribute( componentIterator.GetMatchingComponentOutputs()[i], &attribValue );

		CEconItem tempItem;
		if ( !DecodeItemFromEncodedAttributeString( attribValue, &tempItem ) )
		{
			AssertMsg2( 0, "%s: Unable to decode dynamic recipe output attribute on item %llu.", __FUNCTION__, pEconItem->GetID() );
			continue;
		}

		locchar_t loc_ItemName[MAX_ITEM_NAME_LENGTH];
		GenerateLocalizedFullItemName( loc_ItemName, pLocalizationProvider, &tempItem, k_EGenerateLocalizedFullItemName_Default, TF_ANTI_IDLEBOT_VERIFICATION_ONLY_ARG_BOOL_TRUE( m_pHashContext == NULL ) );

		AddDescLine( loc_ItemName, /* this will be ignored: */ ATTRIB_COL_ITEMSET_MISSING, kDescLineFlag_Misc );

		// Iterate through the attributes on this temp item and have it store the attributes that should affect
		// this component's name.  Once we have that, have it fill out a temporary CEconItemDescription.
		CRecipeNameAttributeDisplayer recipeAttributeIterator;
		tempItem.IterateAttributes( &recipeAttributeIterator );
		recipeAttributeIterator.SortAttributes();
		CEconItemDescription tempDescription;
		recipeAttributeIterator.Finalize( &tempItem, &tempDescription, pLocalizationProvider );

		// Check if that temp CEconItemDescription has any attributes we want.  If so, steal them.
		if ( tempDescription.m_vecDescLines.Count() > 0 )
		{
			locchar_t loc_Attribs[MAX_ITEM_NAME_LENGTH] = LOCCHAR("");

			// Put the attributes on the next line in parenthesis
			loc_scat_safe( loc_Attribs, LOCCHAR("(") );

			// Put in each attribute
			FOR_EACH_VEC( tempDescription.m_vecDescLines, j )
			{
				// Comma separated
				if ( j > 0 )
				{
					loc_scat_safe( loc_Attribs, LOCCHAR(", ") );
				}

				loc_scat_safe( loc_Attribs, tempDescription.m_vecDescLines[j].sText.Get() );
			}

			loc_scat_safe( loc_Attribs, LOCCHAR(")") );

			// Print out in the same color as the item name above
			AddDescLine( loc_Attribs, /* this will be ignored: */ ATTRIB_COL_ITEMSET_MISSING, kDescLineFlag_Misc );
		}
	}
}

//-----------------------------------------------------------------------------
void CEconItemDescription::Generate_UnusualifierEffectList( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{

#ifdef TF_CLIENT_DLL
	static CSchemaItemDefHandle pItemDef_Unusualifier( "Taunt Unusualifier" );
	if ( pEconItem->GetItemDefinition() != pItemDef_Unusualifier )
		return;

	static CSchemaAttributeDefHandle pAttrDef_UnusualifierAttrTemplateName( "unusualifier_attribute_template_name" );
	CAttribute_String sAttrTemplateName;
	if ( !pEconItem->FindAttribute( pAttrDef_UnusualifierAttrTemplateName, &sAttrTemplateName ) )
		return;

	const char *pszResultAttribString = sAttrTemplateName.value().c_str();
	random_attrib_t *pRandomAttr = GetItemSchema()->GetRandomAttributeTemplateByName( pszResultAttribString );
	Assert( pRandomAttr );
	if ( !pRandomAttr )
		return;

	const locchar_t *loc_unusualEffects = pLocalizationProvider->Find( "TF_Unusualifier_UnusualEffects" );

	// Add a bit of spacing, this is only for the market
	// Add empty line
	AddEmptyDescLine();

	AddDescLine( CConstructLocalizedString( LOCCHAR( "%s1" ), loc_unusualEffects ), ATTRIB_COL_NEUTRAL, kDescLineFlag_Misc | kDescLineFlag_CaseBonusContent );

	static CSchemaAttributeDefHandle pattrDef_TauntParticleEffect( "taunt only unusual effect" );
	static CSchemaAttributeDefHandle pAttrDef_TauntParticle( "taunt attach particle index" );

	FOR_EACH_VEC( pRandomAttr->m_RandomAttributes, iAttr )
	{
		if ( pRandomAttr->m_RandomAttributes[iAttr].m_staticAttrib.iDefIndex == pattrDef_TauntParticleEffect->GetDefinitionIndex() )
		{
			AddAttributeDescription( pLocalizationProvider, pAttrDef_TauntParticle, pRandomAttr->m_RandomAttributes[iAttr].m_staticAttrib.m_value.asUint32, NUM_ATTRIB_COLORS, kDescLineFlag_CaseBonusContent );
		}
	}

	AddDescLine( CConstructLocalizedString( LOCCHAR( "%s1" ), pLocalizationProvider->Find( "TF_InspectForDetails" ) ), ATTRIB_COL_NEUTRAL, kDescLineFlag_Misc | kDescLineFlag_MouseOverPanel );
#endif // TF_CLIENT_DLL
}

#endif // PROJECT_TF

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const CEconItemDefinition *GetPaintItemDefinitionForPaintedItem( const IEconItemInterface *pEconItem )
{
	static CSchemaAttributeDefHandle pAttribDef_Paint( "set item tint RGB" );

	attrib_value_t unPaintRGBAttrBits;
	if ( !pAttribDef_Paint || !pEconItem->FindAttribute( pAttribDef_Paint, &unPaintRGBAttrBits ) )
		return NULL;

	const CEconItemSchema::ToolsItemDefinitionMap_t &toolDefs = GetItemSchema()->GetToolsItemDefinitionMap();

	FOR_EACH_MAP_FAST( toolDefs, i )
	{
		const CEconItemDefinition *pItemDef = toolDefs[i];

		// ignore everything that is not a paint can tool
		const IEconTool *pEconTool = pItemDef->GetEconTool();
		if ( pEconTool && !V_strcmp( pEconTool->GetTypeName(), "paint_can" ) ) 
		{
			attrib_value_t unPaintRGBAttrCompareBits;
			if ( FindAttribute( pItemDef, pAttribDef_Paint, &unPaintRGBAttrCompareBits ) && unPaintRGBAttrCompareBits == unPaintRGBAttrBits )
				return pItemDef;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Specify target (strangifiers, etc that can only be applied to specific items)
//-----------------------------------------------------------------------------
void CEconItemDescription::Generate_XifierToolTargetItem( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	// Make sure it's a tool of the appropriate type
	const CEconTool_Xifier *pTool = pEconItem->GetItemDefinition()->GetTypedEconTool<CEconTool_Xifier>();
	if ( pTool == NULL )
		return;

	// Make sure there is a specific target item
	static CSchemaAttributeDefHandle pAttribDef_ToolTargetItem( "tool target item" );
	float flItemDef;
	if( pAttribDef_ToolTargetItem && FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pEconItem, pAttribDef_ToolTargetItem, &flItemDef ) )
	{
		locchar_t szTargetItemName[ MAX_ITEM_NAME_LENGTH ] = LOCCHAR("Unknown Item");

		// It's a tool, see if it has a tool target item attribute
		const item_definition_index_t unItemDef = flItemDef;
		const CEconItemDefinition *pEconTargetDef = GetItemSchema()->GetItemDefinition( unItemDef );

		// Start with the base name.
		if ( pEconTargetDef )
		{
			GetLocalizedBaseItemName( szTargetItemName, pLocalizationProvider, pEconTargetDef );
		}

		const char *pszDesc = pTool->GetItemDescToolTargetLocToken();
		AssertMsg( pszDesc && *pszDesc, "%s: missing 'item_desc_tool_target' key", pTool->GetTypeName() );
		AddDescLine( CConstructLocalizedString( pLocalizationProvider->Find( pszDesc ),
					 szTargetItemName ),
					 ATTRIB_COL_NEUTRAL,
					 kDescLineFlag_Desc );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDescription::Generate_Painted( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	static CSchemaAttributeDefHandle pAttrDef_PaintEffect( "Paint Effect" );

	float fPaintEffectType;
	if ( pAttrDef_PaintEffect && FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pEconItem, pAttrDef_PaintEffect, &fPaintEffectType ) )
	{
		if ( fPaintEffectType == 1 )
		{
			LocalizedAddDescLine( pLocalizationProvider, "Econ_Paint_Effect_Oscillating", ATTRIB_COL_NEUTRAL, kDescLineFlag_Misc );
		}
		else if ( fPaintEffectType == 2 )
		{
			LocalizedAddDescLine( pLocalizationProvider, "Econ_Paint_Effect_Position", ATTRIB_COL_NEUTRAL, kDescLineFlag_Misc );
		}
		else if ( fPaintEffectType == 3 )
		{
			LocalizedAddDescLine( pLocalizationProvider, "Econ_Paint_Effect_LowHealthWarning", ATTRIB_COL_NEUTRAL, kDescLineFlag_Misc );
		}
	}

	// Find the name of the paint we have applied in the least efficient way imaginable!
	const CEconItemDefinition *pItemDef = pEconItem->GetItemDefinition();
	static CSchemaAttributeDefHandle pAttrDef_ShowPaint( "show paint description" );
	if ( pItemDef && ( !pItemDef->IsTool() || FindAttribute( pEconItem, pAttrDef_ShowPaint ) ) )
	{
		const CEconItemDefinition *pTempDef = GetPaintItemDefinitionForPaintedItem( pEconItem );
		if ( pTempDef )
		{
			const locchar_t *locLocalizedPaintName = pLocalizationProvider->Find( pTempDef->GetItemBaseName() );

			if ( locLocalizedPaintName )
			{
				AddDescLine( CConstructLocalizedString( pLocalizationProvider->Find( "Econ_Paint_Name" ),
							 locLocalizedPaintName ),
							 ATTRIB_COL_LEVEL,
							 kDescLineFlag_Misc );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemDescription::Generate_Uses( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	// don't display a quantity if we have the unlimited quantity attribute
	static CSchemaAttributeDefHandle unlimitedQuantityAttribute( "unlimited quantity" );
	if ( pEconItem->FindAttribute( unlimitedQuantityAttribute ) )
		return;

	// Collection tools don't display this.
	const GameItemDefinition_t *pItemDef = pEconItem->GetItemDefinition();
	if ( !pItemDef )
		return;

	const IEconTool *pEconTool = pItemDef->GetEconTool();
	if ( !pEconTool->ShouldDisplayQuantity( pEconItem ) )
		return;

	int iQuantity = pEconItem->GetQuantity();
	bool bIsTool = pItemDef->GetItemClass() && !Q_strcmp( pItemDef->GetItemClass(), "tool" );
	bool bIsConsumable = ( pItemDef->GetCapabilities() & ITEM_CAP_USABLE_GC ) != 0 && iQuantity != 0;	

	if ( bIsTool || bIsConsumable )
	{
		locchar_t wszQuantity[10];
		loc_sprintf_safe( wszQuantity, LOCCHAR( "%d" ), iQuantity );

		// Add an empty line before the usage display.
		AddEmptyDescLine();
		
		// Display our usage count.
		AddDescLine( CConstructLocalizedString( pLocalizationProvider->Find( "#Attrib_LimitedUse" ), &wszQuantity[0] ), ATTRIB_COL_LIMITED_USE, kDescLineFlag_Misc );
	}
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItemDescription::Generate_LootListDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	const CEconItemDefinition *pItemDef = pEconItem->GetItemDefinition();
	if ( !pItemDef )
		return;

	// Don't add this description if the item is a special crate type.
	const IEconTool *pEconTool = pItemDef->GetEconTool();
	const bool bIsRestrictedCrate = pEconTool && pEconTool->GetUsageRestriction()
								  ? !V_stricmp( pEconTool->GetUsageRestriction(), "winter" ) || !V_stricmp( pEconTool->GetUsageRestriction(), "summer" )
								  : false;

	if ( bIsRestrictedCrate )
		return;

	// Do we have a generation code we want to make public? We do this regardless of whether we're describing our
	// loot list contents in detail.
	{
		static CSchemaAttributeDefHandle pAttrDef_CrateGenerationCode( "crate generation code" );
		CAttribute_String sCrateGenerationCode;

		const locchar_t *pszCrateGenerationCodeLoc = pLocalizationProvider->Find( "Attrib_CrateGenerationCode" );

		if ( pEconItem->FindAttribute( pAttrDef_CrateGenerationCode, &sCrateGenerationCode ) && sCrateGenerationCode.value().length() > 0 )
		{
			CUtlConstStringBase<locchar_t> loc_sAttrValue;
			pLocalizationProvider->ConvertUTF8ToLocchar( sCrateGenerationCode.value().c_str(), &loc_sAttrValue );

			AddDescLine( CConstructLocalizedString( pszCrateGenerationCodeLoc, loc_sAttrValue.Get() ),
						 ATTRIB_COL_POSITIVE,
						 kDescLineFlag_Misc );
		}
	}

	// Grab the actual contents of our loot list.
	CCrateLootListWrapper LootListWrapper( pEconItem );
	const IEconLootList *pLootList = LootListWrapper.GetEconLootList();

	if ( pLootList == nullptr )
		return;

	// If our base loot list is set not to list contents, skip the header/footer as well and don't display anything.
	if ( !pLootList->BPublicListContents() )
		return;

	AddEmptyDescLine();

	if ( !pLootList->GetLootListCollectionReference() )
	{
		LocalizedAddDescLine( pLocalizationProvider, pLootList->GetLootListHeaderLocalizationKey(), ATTRIB_COL_NEUTRAL, kDescLineFlag_Misc );
	}
	else
	{

		int iCollectionIndex = GetItemSchema()->GetItemCollections().Find( pLootList->GetLootListCollectionReference() );
		if ( GetItemSchema()->GetItemCollections().IsValidIndex( iCollectionIndex ) )
		{
			LocalizedAddDescLine( pLocalizationProvider, (GetItemSchema()->GetItemCollections()[iCollectionIndex])->m_pszLocalizedDesc, ATTRIB_COL_NEUTRAL, kDescLineFlag_Misc );
		}
	}

	class CDescriptionLootListIterator : public IEconLootList::IEconLootListIterator
	{
	public:
		CDescriptionLootListIterator( CEconItemDescription *pThis, const CLocalizationProvider *pLocalizationProvider, bool bUseProperName )
			: m_pThis( pThis )
			, m_pLocalizationProvider( pLocalizationProvider )
			, m_bUseProperName( bUseProperName )
		{
		}

		virtual void OnIterate( item_definition_index_t unItemDefIndex ) OVERRIDE
		{
			const CEconItemDefinition *pItemDef = GetItemSchema()->GetItemDefinition( unItemDefIndex );
			if ( pItemDef )
			{
				// Check if this item is already owned
				bool bOwned = false;
				bool bUnusual = false;
#ifdef CLIENT_DLL
				CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();
				if ( pLocalInv )
				{
					for ( int i = 0; i < pLocalInv->GetItemCount(); ++i )
					{
						CEconItemView *pItem = pLocalInv->GetItem( i );
						if ( pItem->GetItemDefinition() == pItemDef )
						{
							bOwned = true;
							// Check Quality
							if ( pItem->BIsUnusual() ) 
							{
								bUnusual = true;
								break;
							}
						}
					}
				}
#endif
				const locchar_t * pCheckmark = bOwned ? m_pLocalizationProvider->Find( "TF_Checkmark" ) : m_pLocalizationProvider->Find( "TF_LackOfCheckmark" );
				if ( bOwned && bUnusual )
				{
					pCheckmark = m_pLocalizationProvider->Find( "TF_Checkmark_Unusual" );
				}

				attrib_colors_t colorRarity = ATTRIB_COL_RARITY_DEFAULT;
				const CEconItemRarityDefinition* pItemRarity = GetItemSchema()->GetRarityDefinition( pItemDef->GetRarity() );
				if ( pItemRarity )
				{
					colorRarity = pItemRarity->GetAttribColor();
				}

				m_pThis->AddDescLine(
					CConstructLocalizedString( LOCCHAR( "%s1%s2" ), pCheckmark,
					CEconItemLocalizedFullNameGenerator(
					m_pLocalizationProvider,
					pItemDef,
					m_bUseProperName
					).GetFullName() ),
					colorRarity,
					kDescLineFlag_Misc,
					NULL,
					pItemDef->GetDefinitionIndex()
					);
			}
		}

	private:
		CEconItemDescription *m_pThis;		// look at me I'm a lambda!
		const CLocalizationProvider *m_pLocalizationProvider;
		bool m_bUseProperName;
	};

	CDescriptionLootListIterator it( this, pLocalizationProvider, TF_ANTI_IDLEBOT_VERIFICATION_ONLY_ARG_BOOL_TRUE( m_pHashContext == NULL ) );
	pLootList->EnumerateUserFacingPotentialDrops( &it );

	if ( pLootList->GetLootListFooterLocalizationKey() )
	{
		LocalizedAddDescLine( pLocalizationProvider, pLootList->GetLootListFooterLocalizationKey(), ATTRIB_COL_POSITIVE, kDescLineFlag_Misc );
	}
	else
	{
		const char *pszRareLootListFooterLocalizationKey = pItemDef->GetDefinitionString( "loot_list_rare_item_footer", "#Econ_Revolving_Loot_List_Rare_Item" );
		LocalizedAddDescLine( pLocalizationProvider, pszRareLootListFooterLocalizationKey, ATTRIB_COL_POSITIVE, kDescLineFlag_Misc );
	}
}

// --------------------------------------------------------------------------
void CEconItemDescription::Generate_EventDetail( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	const CEconItemDefinition *pItemDef = pEconItem->GetItemDefinition();
	if ( !pItemDef )
		return;

	// Check to see if we should append any extra description information
	const char *pszEventLocalizationKey = pItemDef->GetDefinitionString( "event_desc_footer", NULL );
	if ( pszEventLocalizationKey )
	{
		AddEmptyDescLine();
		LocalizedAddDescLine( pLocalizationProvider, pszEventLocalizationKey, ATTRIB_COL_POSITIVE, kDescLineFlag_Misc );
	}
}
// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
#ifdef CLIENT_DLL

static bool IsItemEquipped( uint32 unAccountID, const CEconItemDefinition *pSearchItemDef, const GameItemDefinition_t **ppFoundSetItemDef )
{
	Assert( pSearchItemDef );
	Assert( ppFoundSetItemDef );

	CPlayerInventory *pInv = InventoryManager()->GetInventoryForAccount( unAccountID );
	if ( !pInv )
		return false;

	for ( int i = 0; i < pInv->GetItemCount(); i++ )
	{
		const CEconItemView *pInvItem = pInv->GetItem( i );
		if ( !pInvItem )
			continue;

		// This code is client-only so we expect to always get back an item definition pointer.
		const GameItemDefinition_t *pInvItemDef = pInvItem->GetItemDefinition();
		Assert( pInvItemDef );

		if ( pInvItemDef->GetSetItemRemap() != pSearchItemDef->GetDefinitionIndex() )
			continue;

		if ( !pInvItem->IsEquipped() )
			continue;

		*ppFoundSetItemDef = pInvItemDef;
		return true;
	}

	return false;
}

#endif // CLIENT_DLL

void CEconItemDescription::Generate_ItemSetDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	const GameItemDefinition_t *pItemDef = pEconItem->GetItemDefinition();
	if ( !pItemDef )
		return;

	const CEconItemSetDefinition *pItemSetDef = pItemDef->GetItemSetDefinition();
	if ( !pItemSetDef )
		return;

	bool bAllItemsEquipped = true;		// filled in below when iterating over items

	// Some item sets are tagged to only appear on items at all if the entire set is visible. Rather than
	// walk the whole set multiple times checking for item equipped state, we build up a set of description lines
	// that we *will* display if we display anything at all. Later, we either submit all those lines for real or
	// return before adding any of them.
	{
		CUtlVector<econ_item_description_line_t> vecPotentialDescLines;

		AddEmptyDescLine( &vecPotentialDescLines );
		LocalizedAddDescLine( pLocalizationProvider, pItemSetDef->m_pszLocalizedName, ATTRIB_COL_ITEMSET_NAME, kDescLineFlag_Set | kDescLineFlag_SetName, &vecPotentialDescLines, pItemSetDef->m_iBundleItemDef );

		// Kyle says: Jon wants different formatting on the GC for sets.

		// Iterate over the items in the set. We'll output a line in different colors to show
		// the current state of this item. For normal item sets, the color is based on whether
		// the owner has the item in question equipped (except on the GC, where we always say
		// "it's not equipped" to avoid confusion). For collections, the color is based on whether
		// the item has been collected.
		FOR_EACH_VEC( pItemSetDef->m_iItemDefs, i )
		{
			const GameItemDefinition_t *pOtherSetItem = dynamic_cast<const GameItemDefinition_t *>( GetItemSchema()->GetItemDefinition( pItemSetDef->m_iItemDefs[i] ) );
			if ( !pOtherSetItem )
				continue;

			item_definition_index_t usLinkItemDefIndex = pOtherSetItem->GetDefinitionIndex();

#ifdef DOTA
			// If the current item is part of a pack bundle, add the pack bundle to the description, rather than the individual item
			if ( pOtherSetItem->IsPackItem() )
			{
				// Link to the pack bundle, not the individual pack item
				usLinkItemDefIndex = pOtherSetItem->GetOwningPackBundle()->GetDefinitionIndex();
			}
#endif

			// Only used on non-GC in case we have an item misrepresenting itself intentionally for set
			// grouping purposes. NULL elsewhere.
			const GameItemDefinition_t *pFoundSetItemDef = NULL;

			const bool bItemPresent =
									IsItemEquipped( pEconItem->GetAccountID(), pOtherSetItem, &pFoundSetItemDef );	// non-GC display will find out whether the player in question has this item actively equipped

			AddDescLine( CEconItemLocalizedFullNameGenerator( 
				pLocalizationProvider, 
				pFoundSetItemDef ? pFoundSetItemDef : pOtherSetItem,
				TF_ANTI_IDLEBOT_VERIFICATION_ONLY_ARG_BOOL_TRUE( m_pHashContext == NULL )
			).GetFullName(), bItemPresent ? ATTRIB_COL_ITEMSET_EQUIPPED : ATTRIB_COL_ITEMSET_MISSING, kDescLineFlag_Set, &vecPotentialDescLines, usLinkItemDefIndex );

			bAllItemsEquipped &= bItemPresent;
		}

		// If the item set is set to be only displayed when the full set is equipped, give up here and
		// toss out our potential lines. Otherwise submit them for real.
		if ( pItemSetDef->m_bIsHiddenSet && !bAllItemsEquipped )
			return;

		FOR_EACH_VEC( vecPotentialDescLines, i )
		{
			AddDescLine( vecPotentialDescLines[i].sText.Get(), vecPotentialDescLines[i].eColor, vecPotentialDescLines[i].unMetaType, NULL, vecPotentialDescLines[i].unDefIndex );
		}
	}

	// Show the set only attributes if we have the entire set and we have bonus attributes to display.
	bool bHasVisible = false;
	FOR_EACH_VEC( pItemSetDef->m_iAttributes, i )
	{
		const CEconItemSetDefinition::itemset_attrib_t& attrib = pItemSetDef->m_iAttributes[i];
		const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinition( attrib.m_iAttribDefIndex );

		if ( !pAttrDef->IsHidden() )
		{
			bHasVisible = true;
			break;
		}
	}

	if ( !bHasVisible )
		return;

	AddEmptyDescLine();
	LocalizedAddDescLine( pLocalizationProvider, "#Econ_Set_Bonus", ATTRIB_COL_ITEMSET_NAME, kDescLineFlag_Set );

	FOR_EACH_VEC( pItemSetDef->m_iAttributes, i )
	{
		const CEconItemSetDefinition::itemset_attrib_t& attrib = pItemSetDef->m_iAttributes[i];
		const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinition( attrib.m_iAttribDefIndex );

		if ( pAttrDef )
		{
			// Add the attribute description. Override the color to be grayed out if we don't have the
			// full set equipped.
			AddAttributeDescription( pLocalizationProvider,
									 pAttrDef,
									 *(attrib_value_t *)&attrib.m_flValue,
									 bAllItemsEquipped ? /* "do not override": */ NUM_ATTRIB_COLORS : ATTRIB_COL_ITEMSET_MISSING );
		}
	}
}

// --------------------------------------------------------------------------
void CEconItemDescription::Generate_CollectionDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	tmZone( TELEMETRY_LEVEL1, TMZF_NONE, "%s", __FUNCTION__ );
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	const CEconItemDefinition *pItemDef = pEconItem->GetItemDefinition();
	if ( !pItemDef )
		return;

	// Adding a check for if the collection we are browsing contains cosmetics or decorated weapons.
	// We can handle decorated weapons the same as cosmetics, since they have proper schema entries
	bool bIsHatOrDecorated = false;

	// For War Painted items (not War Paints themselves) we want highlight the row of the
	// War Paint itself in the collection.  Look up our corresponding War Paint's item def
	// and use that as our own if there is one.
	uint32 nPaintkitDefindex = 0;
	if ( GetPaintKitDefIndex( pEconItem, &nPaintkitDefindex ) )
	{
		auto pPaintkitItemDef = GetItemSchema()->GetPaintKitItemDefinition( nPaintkitDefindex );
		if ( pPaintkitItemDef == NULL )
		{
			bIsHatOrDecorated = true;
		}
		pItemDef = pPaintkitItemDef ? pPaintkitItemDef : pItemDef;
	}

	const CEconItemCollectionDefinition *pCollection = GetCollection( pEconItem );
	if ( !pCollection )
		return;

	// Collection Header .. 
	const locchar_t *loc_name = pLocalizationProvider->Find( pCollection->m_pszLocalizedName );
	
	// Add a bit of spacing, this is only for the market
	// Add empty line
	AddDescLine( LOCCHAR( " " ), ATTRIB_COL_NEUTRAL, kDescLineFlag_CollectionName | kDescLineFlag_Empty );
	AddDescLine( LOCCHAR( " " ), ATTRIB_COL_NEUTRAL, kDescLineFlag_CollectionName | kDescLineFlag_Empty );

	AddDescLine(
		CConstructLocalizedString( LOCCHAR( "%s1" ), loc_name ), 
		ATTRIB_COL_NEUTRAL, 
		kDescLineFlag_CollectionName 
	);
	
	FOR_EACH_VEC( pCollection->m_iItemDefs, index )
	{
		int eFlag = kDescLineFlag_Collection;
		const CEconItemDefinition *pTempItemDef = GetItemSchema()->GetItemDefinition( pCollection->m_iItemDefs[index] );
		if ( pTempItemDef )
		{
			// Check if this item is already owned
			bool bOwned = false;
			bool bUnusual = false;

			auto lambdaItemMatch = [&eFlag, &bOwned, &bUnusual]( const IEconItemInterface *pMatchingItem, EDescriptionLineMetaFlags eFlagsToAdd )
			{
				if ( !pMatchingItem )
					return;

				bOwned = true;
				eFlag |= eFlagsToAdd;
				if ( pMatchingItem->BIsUnusual() )
				{
					bUnusual = true;
				}
			};

			if ( pTempItemDef == pItemDef )
			{
				lambdaItemMatch( pEconItem, kDescLineFlag_CollectionCurrentItem );
			}
#ifdef CLIENT_DLL
			else 
			{
				CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();
				if ( pLocalInv )
				{
					auto lambdaCheckHasItem = [ & ]( const CEconItemDefinition* pItemDef ) -> const CCopyableUtlVector< itemid_t >*
					{
						auto& vecItemsWithDefindex = pLocalInv->GetItemsWithDefindex( pItemDef->GetDefinitionIndex() );
						if ( !vecItemsWithDefindex.IsEmpty() )
							return &vecItemsWithDefindex;

						uint32 unPaintkitDefidnex = 0;
						if ( GetPaintKitDefIndex( pItemDef, &unPaintkitDefidnex ) && !bIsHatOrDecorated )
						{
							auto& vecItemsWithPaintkit = pLocalInv->GetItemsWithPaintkitDefindex( unPaintkitDefidnex );
							if ( !vecItemsWithPaintkit.IsEmpty() )
								return &vecItemsWithPaintkit;
						}

						return NULL;
					};

					const CEconItemCollectionDefinition *pRefCollection = GetItemSchema()->GetCollectionByName( pTempItemDef->GetCollectionReference() );
					// if item has a collection reference, we are looking for all those items and this item
					if ( pRefCollection )
					{
						tmZone( TELEMETRY_LEVEL1, TMZF_NONE, "%s - Collection Reference", __FUNCTION__ );
						FOR_EACH_VEC( pRefCollection->m_iItemDefs, iRefCollectionItem  )
						{
							const CEconItemDefinition* pCollectionItemDef = GetItemSchema()->GetItemDefinition( pRefCollection->m_iItemDefs[ iRefCollectionItem ] );
							if ( !pCollectionItemDef )
							{
								Assert( !"Collection references invalid defindex!" );
								continue;
							}

							if ( !lambdaCheckHasItem( pCollectionItemDef ) )
							{
								bOwned = false;
								break;
							}
						}
					}
					else 
					{	
						tmZone( TELEMETRY_LEVEL1, TMZF_NONE, "%s - Collection", __FUNCTION__ );
						// Normal Backpack scan
						auto pVecItems = lambdaCheckHasItem( pTempItemDef );
						if ( pVecItems )
						{
							decltype( *pVecItems ) vecItems = *pVecItems;
							FOR_EACH_VEC( vecItems, i )
							{
								auto pMatchingItem = pLocalInv->GetInventoryItemByItemID( vecItems[ i ] );
								lambdaItemMatch( pMatchingItem, (EDescriptionLineMetaFlags)0 );
							}
						}
					}
				}
			}
#endif
			
			const locchar_t * pCheckmark = bOwned ? pLocalizationProvider->Find( "TF_Checkmark" ) : pLocalizationProvider->Find( "TF_LackOfCheckmark" );
			if ( bOwned && bUnusual )
			{
				pCheckmark = pLocalizationProvider->Find( "TF_Checkmark_Unusual" );
			}

			attrib_colors_t colorRarity = ATTRIB_COL_RARITY_DEFAULT;
			const CEconItemRarityDefinition* pItemRarity = GetItemSchema()->GetRarityDefinition( pTempItemDef->GetRarity() );
			if ( pItemRarity )
			{
				colorRarity = pItemRarity->GetAttribColor();
			}

			AddDescLine( 
				CConstructLocalizedString( LOCCHAR("%s1%s2"), pCheckmark,
					CEconItemLocalizedFullNameGenerator(
					pLocalizationProvider,
					pTempItemDef,
					TF_ANTI_IDLEBOT_VERIFICATION_ONLY_ARG_BOOL_TRUE( m_pHashContext == NULL )
				).GetFullName() ), 
				colorRarity,
				eFlag,
				NULL,
				pTempItemDef->GetDefinitionIndex()
			);
		}
	}
}


void CEconItemDescription::Generate_BonusContentDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	// only do this for case with collection
	if ( pEconItem->GetItemDefinition()->GetCollectionReference() == NULL )
		return;

	LootListInfo_t lootlistInfo;
	if ( !GetClientLootListInfo( pEconItem, lootlistInfo ) )
		return;

	// exclusive case bonus items
	if ( lootlistInfo.m_vecAdditionalItems.Count() > 0 )
	{
		const locchar_t *loc_additionalItems = pLocalizationProvider->Find( "TF_CaseExclusiveBonusItems" );
	
		// Add a bit of spacing, this is only for the market
		// Add empty line
		AddEmptyDescLine();

		AddDescLine( CConstructLocalizedString( LOCCHAR( "%s1" ), loc_additionalItems ), ATTRIB_COL_NEUTRAL, kDescLineFlag_Misc | kDescLineFlag_CaseBonusContent );

		FOR_EACH_VEC( lootlistInfo.m_vecAdditionalItems, i )
		{
			item_definition_index_t iDefIndex = lootlistInfo.m_vecAdditionalItems[i];
			CEconItemDefinition *pLootItemDef = GetItemSchema()->GetItemDefinition( iDefIndex );
			if ( !pLootItemDef )
				continue;

			LocalizedAddDescLine( pLocalizationProvider, pLootItemDef->GetItemBaseName(), ATTRIB_COL_RARITY_DEFAULT, kDescLineFlag_Misc | kDescLineFlag_CaseBonusContent, NULL, iDefIndex );
		}
	}

	// global case unusual effects
	if ( lootlistInfo.m_vecAttributes.Count() > 0 )
	{
		const locchar_t *loc_unusualEffects = pLocalizationProvider->Find( "TF_CaseGlobalUnusualEffects" );
	
		// Add a bit of spacing, this is only for the market
		// Add empty line
		AddEmptyDescLine();

		AddDescLine( CConstructLocalizedString( LOCCHAR( "%s1" ), loc_unusualEffects ), ATTRIB_COL_NEUTRAL, kDescLineFlag_Misc | kDescLineFlag_CaseBonusContent );

		static CSchemaAttributeDefHandle pAttrDef_ParticleEffect( "attach particle effect" );
		static CSchemaAttributeDefHandle pattrDef_HatParticleEffect( "hat only unusual effect" );

		FOR_EACH_VEC( lootlistInfo.m_vecAttributes, i )
		{
			random_attrib_t *pRandom = lootlistInfo.m_vecAttributes[i];
			FOR_EACH_VEC( pRandom->m_RandomAttributes, iAttr )
			{
				if ( pRandom->m_RandomAttributes[iAttr].m_staticAttrib.iDefIndex == pAttrDef_ParticleEffect->GetDefinitionIndex() || 
					 pRandom->m_RandomAttributes[iAttr].m_staticAttrib.iDefIndex == pattrDef_HatParticleEffect->GetDefinitionIndex() )
				{
					AddAttributeDescription( pLocalizationProvider, pAttrDef_ParticleEffect, pRandom->m_RandomAttributes[iAttr].m_staticAttrib.m_value.asUint32, NUM_ATTRIB_COLORS, kDescLineFlag_CaseBonusContent );
				}
			}
		}
	}

	// global case bonus items
	AddEmptyDescLine();
	AddDescLine( CConstructLocalizedString( LOCCHAR( "%s1" ), pLocalizationProvider->Find( "TF_CaseGlobalBonusItems" ) ), ATTRIB_COL_NEUTRAL, kDescLineFlag_Misc | kDescLineFlag_CaseBonusContent );
	AddDescLine( CConstructLocalizedString( LOCCHAR( "%s1" ), pLocalizationProvider->Find( "TF_CaseGlobalBonusItemsDesc" ) ), ATTRIB_COL_NEUTRAL, kDescLineFlag_Desc | kDescLineFlag_CaseBonusContent );

	AddDescLine( CConstructLocalizedString( LOCCHAR( "%s1" ), pLocalizationProvider->Find( "TF_InspectForDetails" ) ), ATTRIB_COL_NEUTRAL, kDescLineFlag_Misc | kDescLineFlag_MouseOverPanel );
}


// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItemDescription::Generate_ExpirationDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	const CEconItemDefinition *pItemDef = pEconItem->GetItemDefinition();
	if ( !pItemDef )
		return;

	// Look for schema-specified static expiration date.
	RTime32 timeSchemaExpiration = pItemDef->GetExpirationDate();
	
	// Look also for a dynamic attribute -- this could come from item tryouts.
	static CSchemaAttributeDefHandle pAttrDef_ExpirationDate( "expiration date" );

	RTime32 timeAttrExpiration = 0;
	pEconItem->FindAttribute( pAttrDef_ExpirationDate, &timeAttrExpiration );		// if we don't have the attribute we'll use our starting value of 0
	
	// Which will have us expire first?
	RTime32 timeExpiration = MAX( timeSchemaExpiration, timeAttrExpiration );

	// If we still don't have an expiration date we don't display anything.
	if ( !timeExpiration )
		return;

	AddEmptyDescLine();

#ifdef TF_CLIENT_DLL
	// is this a loaner item?
	if ( GetAssociatedQuestID( pEconItem ) != INVALID_ITEM_ID )
	{
		AddDescLine( pLocalizationProvider->Find( "#Attrib_LoanerItemExpirationDate" ),
			ATTRIB_COL_NEGATIVE,
			kDescLineFlag_Misc );
	}
	else
#endif // TF_CLIENT_DLL
	{
		CLocalizedRTime32 time = { timeExpiration, false, pLocalizationProvider TF_ANTI_IDLEBOT_VERIFICATION_ONLY_COMMA TF_ANTI_IDLEBOT_VERIFICATION_ONLY_ARG( m_pHashContext ) };
		AddDescLine( CConstructLocalizedString( pLocalizationProvider->Find( "#Attrib_ExpirationDate" ),
			time ),
			ATTRIB_COL_NEGATIVE,
			kDescLineFlag_Misc );
	}
}


void CEconItemDescription::Generate_DropPeriodDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	static CSchemaAttributeDefHandle pAttr_EndDropDate( "end drop date" );

	// See if we have the drop date period end attribute
	CAttribute_String value;
	if ( !FindAttribute( pEconItem, pAttr_EndDropDate, &value ) )
		return;

	AddEmptyDescLine();

	// Convert the string value to an RTime32
	RTime32 endDate = CRTime::RTime32FromString( value.value().c_str() );
	// Is the time before or after now?  Use different strings for each
	const char* pszDropString =  endDate > CRTime::RTime32TimeCur()
							  ? "#Attrib_DropPeriodComing"
							  : "#Attrib_DropPeriodPast";

	CLocalizedRTime32 time = { endDate, false, pLocalizationProvider TF_ANTI_IDLEBOT_VERIFICATION_ONLY_COMMA TF_ANTI_IDLEBOT_VERIFICATION_ONLY_ARG( m_pHashContext ) };
	AddDescLine( CConstructLocalizedString( pLocalizationProvider->Find( pszDropString ),
											time ),
				 ATTRIB_COL_LEVEL,
				 kDescLineFlag_Misc );
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
#ifdef TF_CLIENT_DLL
	extern ConVar cl_showbackpackrarities;
	extern ConVar cl_show_market_data_on_items;
#endif 

void CEconItemDescription::Generate_MarketInformation( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	// Deprecated;
	// We now have right click go to market
	return;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
struct localized_localplayer_line_t
{
	localized_localplayer_line_t( const char *pLocalizationKey, attrib_colors_t eAttribColor, const char *pLocalizationSubKey = NULL )
		: m_pLocalizationKey( pLocalizationKey )
		, m_pLocalizationSubKey( pLocalizationSubKey )
		, m_eAttribColor( eAttribColor )
	{
		//
	}

	const char *m_pLocalizationKey;
	const char *m_pLocalizationSubKey;
	attrib_colors_t m_eAttribColor;
};

void CEconItemDescription::Generate_FlagsAttributes( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	const CEconItemDefinition *pItemDef = pEconItem->GetItemDefinition();
	if ( pItemDef && pItemDef->GetEconTool() && pItemDef->GetEconTool()->ShouldDisplayQuantity( pEconItem ) )
	{
		AddEmptyDescLine();
		AddDescLine( CConstructLocalizedString( pLocalizationProvider->Find( "#Attrib_LimitedUse" ), (uint32)pEconItem->GetQuantity() ),
					 ATTRIB_COL_LIMITED_USE,
					 kDescLineFlag_LimitedUse );
	}
	
	CUtlVector<localized_localplayer_line_t> vecLines;

	// Is this item in use? (ie., being used as part of a cross-game trade)
	if ( pEconItem->GetInUse() )
	{
		vecLines.AddToTail( localized_localplayer_line_t( "#Attrib_InUse", ATTRIB_COL_NEUTRAL ) );
	}

	static CSchemaAttributeDefHandle pAttrDef_TradableAfter( "tradable after date" );
	static CSchemaAttributeDefHandle pAttrDef_ToolEscrowUntil( "tool escrow until date" );
	static CSchemaAttributeDefHandle pAttrDef_AlwaysTradableAndUsableInCrafting( "always tradable" );

	uint32 unTradeTime = 0,
		   unEscrowTime = 0;
	const bool bHasTradableAfterDate = pEconItem->FindAttribute( pAttrDef_TradableAfter, &unTradeTime );
	const bool bHasToolEscrowUntilDate = pEconItem->FindAttribute( pAttrDef_ToolEscrowUntil, &unEscrowTime );
	const bool bHasExpiringTimer = bHasTradableAfterDate || bHasToolEscrowUntilDate;

#ifdef CLIENT_DLL
	const bool bIsStoreItem = IsStorePreviewItem( pEconItem );
	const bool bIsPreviewItem = pEconItem->GetFlags() & kEconItemFlagClient_Preview;

	if ( bIsStoreItem || bIsPreviewItem )
	{
		{
			// Does this item come with other packages on Steam?
			const econ_store_entry_t *pStoreEntry = GetEconPriceSheet() ? GetEconPriceSheet()->GetEntry( pItemDef->GetDefinitionIndex() ) : NULL;
			if ( pStoreEntry && pStoreEntry->GetGiftSteamPackageID() != 0 )
			{
				const char *pszSteamPackageLocalizationToken = GetItemSchema()->GetSteamPackageLocalizationToken( pStoreEntry->GetGiftSteamPackageID() );
				if ( pszSteamPackageLocalizationToken )
				{
					vecLines.AddToTail( localized_localplayer_line_t( "#Attrib_Store_IncludesSteamGiftPackage", ATTRIB_COL_POSITIVE, pszSteamPackageLocalizationToken ) );
					vecLines.AddToTail( localized_localplayer_line_t( NULL, ATTRIB_COL_POSITIVE ) );
				}
			}
		
			// While the above apply to store *and* preview items, the below only apply to store items.
			if ( bIsStoreItem )
			{
				// Don't display this line for map stamps because they can't be traded.
				if ( pItemDef && pItemDef->GetItemClass() && !FStrEq( pItemDef->GetItemClass(), "map_token" ) )
				{
					static CSchemaAttributeDefHandle pAttrib_CannotTrade( "cannot trade" );
					Assert( pAttrib_CannotTrade );

					// Some items cannot ever be traded, so don't indicate to the users that they'll be tradeable after a few days.
					if ( !FindAttribute( pItemDef, pAttrib_CannotTrade ) )
					{
						vecLines.AddToTail( localized_localplayer_line_t( "#Attrib_Store_TradableAfterDate", ATTRIB_COL_NEGATIVE ) );
					}

					if ( pItemDef->GetEconTool() && pItemDef->GetEconTool()->RequiresToolEscrowPeriod() )
					{
						vecLines.AddToTail( localized_localplayer_line_t( "#Attrib_Store_ToolEscrowUntilDate", ATTRIB_COL_NEGATIVE ) );
					}
				}

				if ( !pItemDef || (pItemDef->GetCapabilities() & ITEM_CAP_CAN_BE_CRAFTED_IF_PURCHASED) == 0 )
				{
					if ( pItemDef->IsBundle() )
					{
						vecLines.AddToTail( localized_localplayer_line_t( "#Attrib_CannotCraftWeapons", ATTRIB_COL_NEGATIVE ) );
					}
					else
					{
						vecLines.AddToTail( localized_localplayer_line_t( "#Attrib_CannotCraft", ATTRIB_COL_NEGATIVE ) );
					}
				}
			}
		}
	}
	else
#endif // CLIENT_DLL	
	if ( bHasExpiringTimer )
	{
		if ( unTradeTime > CRTime::RTime32TimeCur() )
		{
			AddAttributeDescription( pLocalizationProvider, pAttrDef_TradableAfter, unTradeTime );
		}

		if ( unEscrowTime > CRTime::RTime32TimeCur() )
		{
			AddAttributeDescription( pLocalizationProvider, pAttrDef_ToolEscrowUntil, unEscrowTime );
		}

		if ( !pEconItem->IsUsableInCrafting() )
		{
			vecLines.AddToTail( localized_localplayer_line_t( "#Attrib_CannotCraft", ATTRIB_COL_NEUTRAL ) );
		}
	}
	else if ( pEconItem->FindAttribute( pAttrDef_AlwaysTradableAndUsableInCrafting ) && pEconItem->IsTradable() )
	{
		// do nothing if we are always tradable or usable in crafting
		//
		// some items are marked as "always_tradable" in their itemDef but the specific item may have the 
		// "non_economy" flag, so we need to also check that this specific item is tradable before doing nothing 
	}
	else
	{
		const int32			  iQuality = pEconItem->GetQuality();
		const eEconItemOrigin eOrigin  = pEconItem->GetOrigin();

		if ( iQuality >= AE_COMMUNITY && iQuality <= AE_SELFMADE )
		{
			vecLines.AddToTail( localized_localplayer_line_t( "#Attrib_SpecialItem", ATTRIB_COL_NEUTRAL ) );
		}
		else if ( eOrigin == kEconItemOrigin_Achievement )
		{
			vecLines.AddToTail( localized_localplayer_line_t( "#Attrib_AchievementItem", ATTRIB_COL_NEUTRAL ) );
		}
		else if ( eOrigin == kEconItemOrigin_CollectionReward )
		{
			vecLines.AddToTail( localized_localplayer_line_t( "#Attrib_CollectionReward", ATTRIB_COL_NEUTRAL ) );
		}
		else if ( eOrigin == kEconItemOrigin_PreviewItem )
		{
			vecLines.AddToTail( localized_localplayer_line_t( "#Attrib_PreviewItem", ATTRIB_COL_NEUTRAL ) );
		}
		else if ( eOrigin == kEconItemOrigin_QuestLoanerItem )
		{
			vecLines.AddToTail( localized_localplayer_line_t( "#Attrib_LoanerItem", ATTRIB_COL_NEUTRAL ) );
		}
		else if ( eOrigin == kEconItemOrigin_Invalid )
		{
			// do nothing, but skip the below "cannot trade/cannot craft" block below"
		}
		else if ( (pEconItem->GetFlags() & kEconItemFlag_NonEconomy) != 0 )
		{
			vecLines.AddToTail( localized_localplayer_line_t( "#Attrib_NonEconomyItem", ATTRIB_COL_NEUTRAL ) );
		}
		else if ( eOrigin == kEconItemOrigin_UntradableFreeContractReward )
		{
			vecLines.AddToTail( localized_localplayer_line_t( "#Attrib_UntradableFreeContractReward", ATTRIB_COL_NEUTRAL ) );
		}
		else
		{
			const bool bIsTradable = pEconItem->IsTradable(),
					   bIsCraftable = pEconItem->IsUsableInCrafting();

			if ( !bIsTradable && !bIsCraftable )
			{
				vecLines.AddToTail( localized_localplayer_line_t( "#Attrib_CannotTradeOrCraft", ATTRIB_COL_NEUTRAL ) );
			}
			else
			{
				if ( !bIsTradable )
				{
					vecLines.AddToTail( localized_localplayer_line_t( "#Attrib_CannotTrade", ATTRIB_COL_NEUTRAL ) );
				}

				if ( !bIsCraftable )
				{
					vecLines.AddToTail( localized_localplayer_line_t( "#Attrib_CannotCraft", ATTRIB_COL_NEUTRAL ) );
				}
			}
		}
	}

	if ( vecLines.Count() > 0 )
	{
		const locchar_t *loc_AttribFormat_AdditionalNode = pLocalizationProvider->Find( "#AttribFormat_AdditionalNote" );
		if ( loc_AttribFormat_AdditionalNode )
		{
			AddEmptyDescLine();

			FOR_EACH_VEC( vecLines, i )
			{
				const char *pszLocalizationKey	  = vecLines[i].m_pLocalizationKey;
				const char *pszLocalizationSubKey = vecLines[i].m_pLocalizationSubKey;

				if ( pszLocalizationKey )
				{
					AddDescLine( pszLocalizationSubKey ?
									CConstructLocalizedString( pLocalizationProvider->Find( pszLocalizationKey ), pLocalizationProvider->Find( pszLocalizationSubKey ) ):	// has subtoken, doesn't use additional note format
									CConstructLocalizedString( loc_AttribFormat_AdditionalNode, pLocalizationProvider->Find( pszLocalizationKey ) ),						// no subtoken, uses base additional note format
								 vecLines[i].m_eAttribColor,
								 kDescLineFlag_Misc );
				}
				else
				{
					AddEmptyDescLine();
				}
			}
		}
	}
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------

bool CEconItemDescription::CVisibleAttributeDisplayer::OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, attrib_value_t value )
{
	if ( !pAttrDef->IsHidden() )
	{
		attrib_iterator_value_t attrVal = { pAttrDef, value };
		m_vecAttributes.AddToTail( attrVal );
	}

	return true;
}

void CEconItemDescription::CVisibleAttributeDisplayer::SortAttributes()
{
	// We need to make sure we process attributes in the same order when iterating on the GC and the client
	// when looking for agreement. We take advantage of this to also sort our attributes into a coherent
	// order for display -- first come neutral attributes, then positive, then negative. In the event of a
	// tie, we sort by attribute index, which is arbitrary but consistent across the client/GC.
	struct AttributeValueSorter
	{
		static int sSort( const attrib_iterator_value_t *pA, const attrib_iterator_value_t *pB )
		{
			const int iEffectTypeDelta = pA->m_pAttrDef->GetEffectType() - pB->m_pAttrDef->GetEffectType();
			if ( iEffectTypeDelta != 0 )
				return iEffectTypeDelta;

			return pA->m_pAttrDef->GetDefinitionIndex() - pB->m_pAttrDef->GetDefinitionIndex();
		}
	};

	m_vecAttributes.Sort( &AttributeValueSorter::sSort );
}

void CEconItemDescription::CVisibleAttributeDisplayer::Finalize( const IEconItemInterface *pEconItem, CEconItemDescription *pEconItemDescription, const CLocalizationProvider *pLocalizationProvider )
{
	// HACK so we dont show series number on select crates since they are self describing (Event Crates, Collection Crates)
	static CSchemaAttributeDefHandle pAttrDef_SupplyCrateSeries( "set supply crate series" );
	static CSchemaAttributeDefHandle pAttrDef_HideSeries( "hide crate series number" );

	FOR_EACH_VEC( m_vecAttributes, i )
	{
		if ( pEconItem && m_vecAttributes[i].m_pAttrDef == pAttrDef_SupplyCrateSeries && pEconItem->FindAttribute( pAttrDef_HideSeries ) )
			continue;

		pEconItemDescription->AddAttributeDescription( pLocalizationProvider, m_vecAttributes[i].m_pAttrDef, m_vecAttributes[i].m_value );
	}
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItemDescription::Generate_VisibleAttributes( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	CVisibleAttributeDisplayer AttributeDisplayer;
	pEconItem->IterateAttributes( &AttributeDisplayer );
	AttributeDisplayer.SortAttributes();
	AttributeDisplayer.Finalize( pEconItem, this, pLocalizationProvider );
}

// --------------------------------------------------------------------------
void CEconItemDescription::Generate_DirectX8Warning( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
#ifdef CLIENT_DLL
	static ConVarRef mat_dxlevel( "mat_dxlevel" );
	const CEconItemDefinition *pEconItemDefinition = pEconItem->GetItemDefinition();
	// If less than 90, we�re in DX8 mode. 
	// Display warning if you are looking at a painthit item or case
	if ( mat_dxlevel.GetInt() < 90 && pEconItemDefinition && ( pEconItemDefinition->GetItemCollectionDefinition() || pEconItemDefinition->GetCollectionReference() ) )
	{
		AddEmptyDescLine();
		AddDescLine( pLocalizationProvider->Find( "#Attrib_DirectX8Warning" ),
			ATTRIB_COL_NEGATIVE,
			kDescLineFlag_Misc );
	}

#endif
}


bool CEconItemDescription::CRecipeNameAttributeDisplayer::OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, attrib_value_t value )
{
	if ( pAttrDef->CanAffectRecipeComponentName() )
	{
		return CVisibleAttributeDisplayer::OnIterateAttributeValue( pAttrDef, value );
	}

	return true;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
static attrib_colors_t GetAttributeDefaultColor( const CEconItemAttributeDefinition *pAttribDef )
{
	// positive attribute?
	switch ( pAttribDef->GetEffectType() )
	{
	case ATTRIB_EFFECT_NEUTRAL:			return ATTRIB_COL_NEUTRAL;
	case ATTRIB_EFFECT_POSITIVE:		return ATTRIB_COL_POSITIVE;
	case ATTRIB_EFFECT_NEGATIVE:		return ATTRIB_COL_NEGATIVE;
	case ATTRIB_EFFECT_STRANGE:			return ATTRIB_COL_STRANGE;
	case ATTRIB_EFFECT_UNUSUAL:			return ATTRIB_COL_UNUSUAL;
	}

	// we don't know
	return ATTRIB_COL_NEUTRAL;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconAttributeDescription::InternalConstruct
(
	const CLocalizationProvider *pLocalizationProvider,
	const CEconItemAttributeDefinition *pAttribDef,
	attrib_value_t value,
	TF_ANTI_IDLEBOT_VERIFICATION_ONLY_ARG( MD5Context_t *pHashContext ) TF_ANTI_IDLEBOT_VERIFICATION_ONLY_COMMA
	IAccountPersonaLocalizer *pOptionalAccountPersonaLocalizer
)
{
	Assert( pAttribDef != NULL );

	const float& value_as_float = (float&)value;
	const uint32& value_as_uint32 = (uint32&)value;

	// Calculate our color first -- if we don't know what to do, we'll wind up as neutral.
	m_eDefaultColor = GetAttributeDefaultColor( pAttribDef );

	// Early out abort if we don't have a localization string for this attribute.
	locchar_t *loc_String = pAttribDef->GetDescriptionString() && pLocalizationProvider
						  ? pLocalizationProvider->Find( pAttribDef->GetDescriptionString() )
						  : NULL;

	if ( !loc_String )
		return;

	char szAttrShortDescToken[MAX_PATH];
	V_sprintf_safe( szAttrShortDescToken, "%s%s", pAttribDef->GetDescriptionString(), "_shortdesc" );

	locchar_t *loc_ShortString = pLocalizationProvider 
							   ? pLocalizationProvider->Find( szAttrShortDescToken ) 
							   : NULL;

	// How do we format an attribute value of this type?
	switch ( pAttribDef->GetDescriptionFormat() )
	{
	case ATTDESCFORM_VALUE_IS_ADDITIVE_PERCENTAGE:
		m_loc_sValue = CLocalizedStringArg<float>( value_as_float * 100.0f ).GetLocArg();
		break;

	case ATTDESCFORM_VALUE_IS_ACCOUNT_ID:
#ifdef CLIENT_DLL
		// If this assert fires, it means that the client fed in an attribute that should be localized
		// as a Steam persona name but didn't feed it any way to get that information. The GC won't
		// assert, but also won't generate anything for the attribute text.
		//
		// It's still totally fine to pass in NULL for the persona localizer as long as you don't
		// expect to have any attributes that have account IDs.
		Assert( pOptionalAccountPersonaLocalizer );
#endif		
		if ( pOptionalAccountPersonaLocalizer )
		{
			m_loc_sValue = pOptionalAccountPersonaLocalizer->FindAccountPersonaName( value_as_uint32 );
		}
		break;

	case ATTDESCFORM_VALUE_IS_ADDITIVE:
		m_loc_sValue = pAttribDef->IsStoredAsFloat()
					 ? CLocalizedStringArg<float>( value_as_float ).GetLocArg()
					 : CLocalizedStringArg<uint32>( value_as_uint32 ).GetLocArg();
		break;

	case ATTDESCFORM_VALUE_IS_INVERTED_PERCENTAGE:
		if ( value_as_float < 1.0 )
		{
			m_loc_sValue = CLocalizedStringArg<float>( (1.0 - value_as_float) * 100.0f ).GetLocArg();
			break;
		}

		// We intentionally fall through when value_as_float >= 1.0f to treat it the same as "value as
		// percentage".
	case ATTDESCFORM_VALUE_IS_PERCENTAGE:
		m_loc_sValue = CLocalizedStringArg<float>( (value_as_float * 100.0f) - 100.0f ).GetLocArg();
		break;

	case ATTDESCFORM_VALUE_IS_DATE:
		{
			bool bUseGMT = false;

#ifdef PROJECT_TF
			static CSchemaAttributeDefHandle pAttribDef_SetEmployeeNumber( "custom employee number" );

			// only use GMT for custom employee number -- not doing this generated a bunch of support
			// tickets because items were granted based on GC time but would display local time, causing
			// people on the border to think they deserved a better badge, etc.
			bUseGMT = (pAttribDef == pAttribDef_SetEmployeeNumber);
#endif // PROJECT_TF

			CLocalizedRTime32 time = { value_as_uint32, bUseGMT, pLocalizationProvider TF_ANTI_IDLEBOT_VERIFICATION_ONLY_COMMA TF_ANTI_IDLEBOT_VERIFICATION_ONLY_ARG( pHashContext ) };
			m_loc_sValue = CLocalizedStringArg<CLocalizedRTime32>( time ).GetLocArg();
			break;
		}

	case ATTDESCFORM_VALUE_IS_PARTICLE_INDEX:
		{
			// This is a horrible, horrible line of code. It exists because old particle references are
			// ints stored as floats as float bit patterns and new particle references are ints stored
			// as ints all the way through.
			CUtlConstString utf8_ParticleKeyName( CFmtStr( "#Attrib_Particle%i", pAttribDef->IsStoredAsInteger() ? value_as_uint32 : (int)value_as_float ).Access() );	// this value is stored as a float but interpreted as an int (1.0 -> 1)
			if ( utf8_ParticleKeyName.IsEmpty() )
				return;

			m_loc_sValue = pLocalizationProvider->Find( utf8_ParticleKeyName.Get() );
			break;
		}

	case ATTDESCFORM_VALUE_IS_KILLSTREAKEFFECT_INDEX:
		{
			CUtlConstString utf8_KeyName( CFmtStr( "#Attrib_KillStreakEffect%i", (int)value_as_float ).Access() );	// this value is stored as a float but interpreted as an int (1.0 -> 1)
			if ( utf8_KeyName.IsEmpty() )
				return;

			m_loc_sValue = pLocalizationProvider->Find( utf8_KeyName.Get() );
			break;
		}

	case ATTDESCFORM_VALUE_IS_KILLSTREAK_IDLEEFFECT_INDEX:
		{
			CUtlConstString utf8_KeyName( CFmtStr( "#Attrib_KillStreakIdleEffect%i", (int)value_as_float ).Access() );	// this value is stored as a float but interpreted as an int (1.0 -> 1)
			if ( utf8_KeyName.IsEmpty() )
				return;

			m_loc_sValue = pLocalizationProvider->Find( utf8_KeyName.Get() );
			break;
		}
	// Don't output any value for bitmasks, but let the attribute text display.
	case ATTDESCFORM_VALUE_IS_OR:
		break;

	default:
#ifdef CLIENT_DLL
		// Only assert on the client -- the GC will just silently fail rather than crash if we ever run into
		// this case, but if we are adding a new display type this will help us catch a reason why it isn't
		// showing up.
		Assert( !"Unhandled attribute value display type in CEconAttributeDescription." );

		// Anywhere besides the client, we intentionally fall through to return immediately.
#endif
	case ATTDESCFORM_VALUE_IS_ITEM_DEF:			// referencing definitions is handled per-attribute
		return;

	case ATTDESCFORM_VALUE_IS_FROM_LOOKUP_TABLE:
		{
			const char *pszLocalizationToken = GetItemSchema()->FindStringTableEntry( pAttribDef->GetDefinitionName(), (int)value_as_float );
			if ( !pszLocalizationToken )
				return;

			const locchar_t *loc_Entry = pLocalizationProvider->Find( pszLocalizationToken );
			if ( !loc_Entry )
				return;

			m_loc_sValue = loc_Entry;
			break;
		}
	}

	// Some attributes have a short description for the upgrade 
	if ( loc_ShortString )
	{
		m_loc_sShortValue = CConstructLocalizedString( loc_ShortString, m_loc_sValue.Get() );
	}

	// Combine the value string we just generated with the localized display for that value. (ie., the value
	// might be "10" and the display would be "health is increased by 10%".)
	m_loc_sValue = CConstructLocalizedString( loc_String, m_loc_sValue.Get() );

	// Is this an attribute that needs a custom wrapper around the default attribute text? (ie.,
	// if our string was "Damage +10%" we want that to be "(only on Hightower: Damage +10%)")
	if ( pAttribDef->GetUserGenerationType() )
	{
		const locchar_t *locUGTLocalizationKey = pLocalizationProvider->Find( CFmtStr( "#Econ_Attrib_UserGeneratedWrapper_%i", pAttribDef->GetUserGenerationType() ).Get() );

		if ( locUGTLocalizationKey )
		{
			m_loc_sValue = CConstructLocalizedString( locUGTLocalizationKey, m_loc_sValue.Get() );
		}
	}

	// If there's no short description, just copy the normal one
	if ( !loc_ShortString )
	{
		m_loc_sShortValue = m_loc_sValue;
	}
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItemDescription::AddAttributeDescription( const CLocalizationProvider *pLocalizationProvider, const CEconItemAttributeDefinition *pAttribDef, attrib_value_t value, attrib_colors_t eOverrideDisplayColor /* = NUM_ATTRIB_COLORS */, uint32 unAdditionalMetaType /*= 0*/ )
{
	Assert( pLocalizationProvider );
	Assert( pAttribDef );
	
	CEconAttributeDescription AttrDesc( pLocalizationProvider,
										pAttribDef,
										value,
										TF_ANTI_IDLEBOT_VERIFICATION_ONLY_ARG( m_pHashContext ) TF_ANTI_IDLEBOT_VERIFICATION_ONLY_COMMA
										this );

	if ( AttrDesc.GetDescription().IsEmpty() )
		return;

	// Is this an attribute that needs a custom wrapper around the default attribute text? (ie.,
	// if our string was "Damage +10%" we want that to be "(only on Hightower: Damage +10%)")
	attrib_colors_t eDefaultAttribColor = GetAttributeDefaultColor( pAttribDef );

#ifdef TF_CLIENT_DLL
	enum
	{
		kUserGeneratedAttributeType_None				= 0,
		kUserGeneratedAttributeType_MVMEngineering		= 1,
		kUserGeneratedAttributeType_HalloweenSpell		= 2
	};

	// On TF, these user-generated attributes can be from upgrade cards which only apply in MvM.
	// We then colorize them based on whether they'll be active, with the caveat that out-of-game
	// views always say yes (GC, loadout when not on a server, etc.).
	if ( pAttribDef->GetUserGenerationType() == kUserGeneratedAttributeType_MVMEngineering && TFGameRules() && !TFGameRules()->IsMannVsMachineMode() )
	{
		eDefaultAttribColor = ATTRIB_COL_ITEMSET_MISSING;
	}
	// They can also be from Halloween spells. These are intended to expire after Halloween in any
	// event, but for display purposes they'll appear in grey unless the holiday is active.
	else if ( pAttribDef->GetUserGenerationType() == kUserGeneratedAttributeType_HalloweenSpell && !EconHolidays_IsHolidayActive( kHoliday_Halloween, CRTime::RTime32TimeCur() ) )
	{
		eDefaultAttribColor = ATTRIB_COL_ITEMSET_MISSING;
	}
#endif // TF_CLIENT_DLL

	AddDescLine( AttrDesc.GetDescription().Get(),
				 eOverrideDisplayColor != NUM_ATTRIB_COLORS ?					// are we overriding the output color?
					eOverrideDisplayColor :										// we are
					eDefaultAttribColor,										// fall back to normal attribute color
				 kDescLineFlag_Attribute | unAdditionalMetaType );
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItemDescription::AddDescLine( const locchar_t *pString, attrib_colors_t eColor, uint32 unMetaType, CUtlVector<econ_item_description_line_t> *out_pOptionalDescLineDest /* = NULL */, item_definition_index_t unDefIndex /* = INVALID_ITEM_DEF_INDEX */, bool bIsItemForSale /*= true*/ )
{
	CUtlVector<econ_item_description_line_t>& vecTargetDescLines = out_pOptionalDescLineDest ? *out_pOptionalDescLineDest : m_vecDescLines;

	econ_item_description_line_t& line = vecTargetDescLines[ vecTargetDescLines.AddToTail() ];

	line.eColor		= eColor;
	line.unMetaType = unMetaType;
	line.sText		= pString;
	line.unDefIndex = unDefIndex;
	line.bIsItemForSale = bIsItemForSale;

}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItemDescription::AddEmptyDescLine( CUtlVector<econ_item_description_line_t> *out_pOptionalDescLineDest )
{
	AddDescLine( LOCCHAR(" "), ATTRIB_COL_NEUTRAL, kDescLineFlag_Empty, out_pOptionalDescLineDest );
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItemDescription::LocalizedAddDescLine( const CLocalizationProvider *pLocalizationProvider, const char *pLocalizationToken, attrib_colors_t eColor, uint32 unMetaType, CUtlVector<econ_item_description_line_t> *out_pOptionalDescLineDest /* = NULL */, item_definition_index_t unDefIndex /* = INVALID_ITEM_DEF_INDEX */, bool bIsItemForSale /* = true */ )
{
	Assert( pLocalizationToken );

	const locchar_t *pTextToAdd = pLocalizationProvider->Find( pLocalizationToken );

	if ( pTextToAdd )
	{
		AddDescLine( pTextToAdd, eColor, unMetaType, out_pOptionalDescLineDest, unDefIndex, bIsItemForSale );
	}
	else if ( pLocalizationToken && (pLocalizationToken[0] != '#') )
	{
		// If we couldn't localize correctly, we might be a string literal like "My temp item desc.". In
		// this case, we use that string as-is.
		CUtlConstStringBase<locchar_t> loc_sText;
		pLocalizationProvider->ConvertUTF8ToLocchar( pLocalizationToken, &loc_sText );
		AddDescLine( loc_sText.Get(), eColor, unMetaType, out_pOptionalDescLineDest, unDefIndex, bIsItemForSale );
	}
	else
	{
		// We couldn't localize this token, but also don't think it was a string meant to be user-facing so
		// just silently fail.
	}

}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
class CGameItemDefinition_EconItemInterfaceWrapper : public CMaterialOverrideContainer< IEconItemInterface >
{
public:
	CGameItemDefinition_EconItemInterfaceWrapper( const CEconItemDefinition *pEconItemDefinition, entityquality_t eQuality )
		: m_pEconItemDefinition( pEconItemDefinition )
		, m_eQuality( eQuality )
	{
		Assert( m_pEconItemDefinition );
	}

	virtual const GameItemDefinition_t *GetItemDefinition() const { return assert_cast<const GameItemDefinition_t *>( m_pEconItemDefinition ); }

	virtual itemid_t		GetID() const { return INVALID_ITEM_ID; }
	virtual uint32			GetAccountID() const { return 0; }
	virtual int32			GetQuality() const { return m_eQuality; }
	virtual style_index_t	GetStyle() const { return INVALID_STYLE_INDEX; }
	virtual uint8			GetFlags() const { return 0; }
	virtual eEconItemOrigin GetOrigin() const { return kEconItemOrigin_Invalid; }
	virtual int				GetQuantity() const { return 1; }
	virtual uint32			GetItemLevel() const { return 0; }
	virtual bool			GetInUse() const { return false; }

	virtual const char	   *GetCustomName() const { return NULL; }
	virtual const char	   *GetCustomDesc() const { return NULL; }

	// IEconItemInterface attribute iteration interface. This is not meant to be used for
	// attribute lookup! This is meant for anything that requires iterating over the full
	// attribute list.
	virtual void IterateAttributes( class IEconItemAttributeIterator *pIterator ) const OVERRIDE
	{
		Assert( pIterator );

		m_pEconItemDefinition->IterateAttributes( pIterator );
	}

private:
	const CEconItemDefinition *m_pEconItemDefinition;
	entityquality_t m_eQuality;
};

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
CEconItemLocalizedFullNameGenerator::CEconItemLocalizedFullNameGenerator( const CLocalizationProvider *pLocalizationProvider, const CEconItemDefinition *pItemDef, bool bUseProperName, entityquality_t eQuality )
{
	Assert( pItemDef );

	CGameItemDefinition_EconItemInterfaceWrapper EconItemDefinitionWrapper( pItemDef, eQuality );
	GenerateLocalizedFullItemName( m_loc_LocalizedItemName, pLocalizationProvider, &EconItemDefinitionWrapper, k_EGenerateLocalizedFullItemName_Default, bUseProperName );
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
class CMarketNameGenerator_EconItemInterfaceWrapper : public IEconItemInterface
{
public:
	CMarketNameGenerator_EconItemInterfaceWrapper( IEconItemInterface *pItem )
		: m_pItem( pItem )
	{
		Assert( m_pItem );
	}

	virtual const GameItemDefinition_t *GetItemDefinition() const { return m_pItem->GetItemDefinition(); }

	virtual itemid_t		GetID() const { return m_pItem->GetID(); }
	virtual uint32			GetAccountID() const { return 0; }
	virtual int32			GetQuality() const { return m_pItem->GetQuality(); }
	virtual style_index_t	GetStyle() const { return INVALID_STYLE_INDEX; }
	virtual uint8			GetFlags() const { return 0; }
	virtual eEconItemOrigin GetOrigin() const { return kEconItemOrigin_Invalid; }
	virtual int				GetQuantity() const { return 1; }
	virtual uint32			GetItemLevel() const { return 0; }
	virtual bool			GetInUse() const { return false; }

	virtual const char	   *GetCustomName() const { return NULL; }
	virtual const char	   *GetCustomDesc() const { return NULL; }

	virtual IMaterial	   *GetMaterialOverride( int iTeam ) OVERRIDE { return m_pItem->GetMaterialOverride( iTeam ); }

	// IEconItemInterface attribute iteration interface. This is not meant to be used for
	// attribute lookup! This is meant for anything that requires iterating over the full
	// attribute list.
	virtual void IterateAttributes( class IEconItemAttributeIterator *pIterator ) const OVERRIDE
	{
		Assert( pIterator );

		// Wrap their iterator in our iterator that will selectively let specific attributes
		// get iterated on by the wrapped iterator.
		CMarketNameGenerator_SelectiveAttributeIterator iteratorWrapper( pIterator );
		m_pItem->IterateAttributes( &iteratorWrapper );
	}

private:

	IEconItemInterface *m_pItem;

	// Iterator class that wraps another iterator and selectively allows specific attributes to be
	// iterated by the passed in iterator
	class CMarketNameGenerator_SelectiveAttributeIterator : public IEconItemAttributeIterator
	{
	public:
		CMarketNameGenerator_SelectiveAttributeIterator( IEconItemAttributeIterator* pIterator )
			: m_pIterator( pIterator )
		{}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, attrib_value_t value ) OVERRIDE
		{
			if( pAttrDef->CanAffectMarketName() )
			{
				m_pIterator->OnIterateAttributeValue( pAttrDef, value );
			}

			return true;
		}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, float value ) OVERRIDE
		{
			if( pAttrDef->CanAffectMarketName() )
			{
				m_pIterator->OnIterateAttributeValue( pAttrDef, value );
			}

			return true;
		}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const uint64& value ) OVERRIDE
		{
			if( pAttrDef->CanAffectMarketName() )
			{
				m_pIterator->OnIterateAttributeValue( pAttrDef, value );
			}

			return true;
		}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_String& value ) OVERRIDE
		{
			if( pAttrDef->CanAffectMarketName() )
			{
				m_pIterator->OnIterateAttributeValue( pAttrDef, value );
			}

			return true;
		}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_DynamicRecipeComponent& value ) OVERRIDE
		{
			if( pAttrDef->CanAffectMarketName() )
			{
				m_pIterator->OnIterateAttributeValue( pAttrDef, value );
			}

			return true;
		}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_ItemSlotCriteria& value ) OVERRIDE
		{
			if( pAttrDef->CanAffectMarketName() )
			{
				m_pIterator->OnIterateAttributeValue( pAttrDef, value );
			}

			return true;
		}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_WorldItemPlacement& value ) OVERRIDE
		{
			if ( pAttrDef->CanAffectMarketName() )
			{
				m_pIterator->OnIterateAttributeValue( pAttrDef, value );
			}

			return true;
		}

	private:

		IEconItemAttributeIterator *m_pIterator;
	};
};

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
CEconItemLocalizedMarketNameGenerator::CEconItemLocalizedMarketNameGenerator( const CLocalizationProvider *pLocalizationProvider, IEconItemInterface *pItem, bool bUseProperName )
{
	Assert( pItem );

	CMarketNameGenerator_EconItemInterfaceWrapper EconItemWrapper( pItem );
	GenerateLocalizedFullItemName( m_loc_LocalizedItemName, pLocalizationProvider, &EconItemWrapper, k_EGenerateLocalizedFullItemName_WithPaintWear, bUseProperName );
}

#endif // BUILD_ITEM_NAME_AND_DESC
