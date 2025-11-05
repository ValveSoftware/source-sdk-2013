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
		Generate_VisibleAttributes( pLocalizationProvider, pEconItem );
		Generate_ItemDesc( pLocalizationProvider, pEconItem );
	}
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
void CEconItemDescription::Generate_ItemLevelDesc( const CLocalizationProvider *pLocalizationProvider, const IEconItemInterface *pEconItem )
{
	Assert( pLocalizationProvider );
	Assert( pEconItem );

	const GameItemDefinition_t *pItemDef = pEconItem->GetItemDefinition();
	if ( !pItemDef )
		return;

	const locchar_t *locTypename = pLocalizationProvider->Find( pItemDef->GetItemTypeName() );

	if ( locTypename && *locTypename )
	{
		AddDescLine( locTypename, ATTRIB_COL_LEVEL, kDescLineFlag_Type, NULL, pEconItem->GetItemDefIndex() );
	}
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
	virtual eEconItemOrigin GetOrigin() const { return kEconItemOrigin_Invalid; }

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
