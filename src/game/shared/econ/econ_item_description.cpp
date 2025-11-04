//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "econ_item_description.h"
#include "econ_item_interface.h"
#include "econ_holidays.h"
#include "tier1/ilocalize.h"
#include "localization_provider.h"
#include "rtime.h"

	#ifndef EXTERNALTESTS_DLL
		#include "econ_item_inventory.h"
	#endif

	#ifdef CLIENT_DLL
		#include "gc_clientsystem.h"
	#endif // CLIENT_DLL


#ifdef PROJECT_TF
	#include "tf_duel_summary.h"
	#include "econ_contribution.h"
	#include "tf_player_info.h"

	#ifdef TF_CLIENT_DLL
		#include "tf_gamerules.h"
		#include "tf_mapinfo.h"
	#endif 
#endif

#ifdef VPROF_ENABLED
	static const char *g_pszEconDescriptionVprofGroup = _T("Econ Description");
#endif

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

#endif // PROJECT_TF
}

void CEconItemDescription::GenerateDescriptionLines( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	VPROF_BUDGET( "CEconItemDescription::GenerateDescriptionLines()", g_pszEconDescriptionVprofGroup );

	Assert( pLocalizationProvider );
	Assert( pEconItem );

	m_vecDescLines.Purge();

	Generate_ItemName( pLocalizationProvider, pEconItem );
	Generate_ItemLevelDesc( pLocalizationProvider, pEconItem );

	// If we decide that for performance reasons some descriptions only want the name/description
	// information and not all the details, this is the block to skip over.
	{
		Generate_StyleDesc( pLocalizationProvider, pEconItem );
		Generate_Painted( pLocalizationProvider, pEconItem );

		Generate_HolidayRestriction( pLocalizationProvider, pEconItem );	
#ifdef PROJECT_TF
		Generate_SaxxyAwardDesc( pLocalizationProvider, pEconItem );
#endif // PROJECT_TF
		Generate_VisibleAttributes( pLocalizationProvider, pEconItem );
		Generate_ItemDesc( pLocalizationProvider, pEconItem );
		Generate_GiftedBy( pLocalizationProvider, pEconItem );
#ifdef PROJECT_TF
		Generate_MapContributor( pLocalizationProvider, pEconItem );
		Generate_FriendlyHat( pLocalizationProvider, pEconItem );
		Generate_SquadSurplusClaimedBy( pLocalizationProvider, pEconItem );
		Generate_MvmChallenges( pLocalizationProvider, pEconItem );
		Generate_UnusualifierEffectList( pLocalizationProvider, pEconItem );
#endif // PROJECT_TF
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

	const CEconItemDefinition *pEconItemDefinition = pEconItem->GetItemDefinition();
	if ( !pEconItemDefinition )
	{
		out_pItemName[0] = (locchar_t)0;
		return;
	}

	bool bHasCustomName = false;

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
	
	static CSchemaAttributeDefHandle pAttrDef_IsFestivized( "is_festivized" );
	enum { kFestiveLength = 64, };
	locchar_t szIsFestivized[kFestiveLength] = LOCCHAR( "" );
	bool bFestivized = false;
	if ( pAttrDef_IsFestivized && pEconItem->FindAttribute( pAttrDef_IsFestivized ) )
	{
		bFestivized = true;
		bUseProperName = false;
	}

	// Generate our quality string.
	enum { kQualityLength = 128, };
	locchar_t szQuality[ kQualityLength ] = LOCCHAR("");

	{
		tmZone( TELEMETRY_LEVEL1, TMZF_NONE, "%s - Quality", __FUNCTION__ );
		// It's possible to get in here with a quality of -1 if we're dealing with an item view that has no
		// associated item. In that case we're probably doing something like browsing the armory, and in any
		// event don't have an item and so don't have a quality and so we just don't show a quality string.
		// If we have a quality text override, use that.

		{
			// Unique items use proper names, but not if we have a quality text override
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

	locchar_t *pNameLocalizationFormat = pLocalizationProvider->Find( pszLocalizationPattern );

	if ( pNameLocalizationFormat )
	{
		ILocalize::ConstructString_safe( out_pItemName,
									pNameLocalizationFormat,
									6,
									szQuality,
									szItemName,
									"",
									"",
									"",
									"" );
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

void CEconItemDescription::Generate_ItemLevelDesc_Default( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem, const locchar_t *locTypename )
{
	item_definition_index_t usDefIndex = pEconItem->GetItemDefIndex();

	if ( locTypename && *locTypename )
	{
		AddDescLine( locTypename, ATTRIB_COL_LEVEL, kDescLineFlag_Type, NULL, usDefIndex );
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

	// If we didn't generate a fancy strange name, we fall back to our default behavior.
	Generate_ItemLevelDesc_Default( pLocalizationProvider, pEconItem, locTypename );
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

		if ( !pInvItem->IsEquipped() )
			continue;

		*ppFoundSetItemDef = pInvItemDef;
		return true;
	}

	return false;
}

#endif // CLIENT_DLL 

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
	
	CUtlVector<localized_localplayer_line_t> vecLines;

	// Is this item in use? (ie., being used as part of a cross-game trade)
	if ( pEconItem->GetInUse() )
	{
		vecLines.AddToTail( localized_localplayer_line_t( "#Attrib_InUse", ATTRIB_COL_NEUTRAL ) );
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
	FOR_EACH_VEC( m_vecAttributes, i )
	{
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
	CGameItemDefinition_EconItemInterfaceWrapper( const CEconItemDefinition *pEconItemDefinition )
		: m_pEconItemDefinition( pEconItemDefinition )
	{
		Assert( m_pEconItemDefinition );
	}

	virtual const GameItemDefinition_t *GetItemDefinition() const { return assert_cast<const GameItemDefinition_t *>( m_pEconItemDefinition ); }

	virtual itemid_t		GetID() const { return INVALID_ITEM_ID; }
	virtual uint32			GetAccountID() const { return 0; }
	virtual style_index_t	GetStyle() const { return INVALID_STYLE_INDEX; }
	virtual uint8			GetFlags() const { return 0; }
	virtual eEconItemOrigin GetOrigin() const { return kEconItemOrigin_Invalid; }
	virtual int				GetQuantity() const { return 1; }
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
};

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
CEconItemLocalizedFullNameGenerator::CEconItemLocalizedFullNameGenerator( const CLocalizationProvider *pLocalizationProvider, const CEconItemDefinition *pItemDef, bool bUseProperName )
{
	Assert( pItemDef );

	CGameItemDefinition_EconItemInterfaceWrapper EconItemDefinitionWrapper( pItemDef );
	GenerateLocalizedFullItemName( m_loc_LocalizedItemName, pLocalizationProvider, &EconItemDefinitionWrapper, k_EGenerateLocalizedFullItemName_Default, bUseProperName );
}

#endif // BUILD_ITEM_NAME_AND_DESC
