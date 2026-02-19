//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: EconItemSchema: Defines a schema for econ items
//
//=============================================================================

#include "cbase.h"
#include "econ_item_schema.h"
#include "tier1/fmtstr.h"
#include "tier1/UtlSortVector.h"
#include "tier2/tier2.h"
#include "filesystem.h"
#include "schemainitutils.h"
#include "gcsdk/gcsdk_auto.h"
#include "rtime.h"
#include "item_selection_criteria.h"
#include "checksum_sha1.h"

#include <google/protobuf/text_format.h>
#include <string.h>

#include "materialsystem/imaterialsystem.h"
#include "materialsystem/itexture.h"
#include "materialsystem/itexturecompositor.h"
#include <tf_proto_script_obj_def.h>

#if ( defined( _MSC_VER ) && _MSC_VER >= 1900 )
#define timezone _timezone
#define daylight _daylight
#endif

// For holiday-limited loot lists.
#include "econ_holidays.h"

#if defined(CLIENT_DLL) || defined(GAME_DLL)
	#include "econ_item_system.h"
	#include "econ_item.h"
	#include "activitylist.h"

	#if defined(TF_CLIENT_DLL) || defined(TF_DLL)
		#include "tf_gcmessages.h"
	#endif
#endif


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace GCSDK;


CEconItemSchema & GEconItemSchema()
{
#if defined( EXTERNALTESTS_DLL )
	static CEconItemSchema g_econItemSchema;
	return g_econItemSchema;
#else
	return *ItemSystem()->GetItemSchema();
#endif
}

const char *g_szDropTypeStrings[] =
{
	"",		 // Blank and none mean the same thing: stay attached to the body.
	"none",
	"drop",	 // The item drops off the body.
	"break", // Not implemented, but an example of a type that could be added.
};

const char *g_TeamVisualSections[TEAM_VISUAL_SECTIONS] = 
{
	"visuals",		// TF_TEAM_UNASSIGNED. Visual changes applied to both teams.
	NULL,			// TF_TEAM_SPECTATOR. Unused.
	"visuals_red",	// TF_TEAM_RED
	"visuals_blu",	// TF_TEAM_BLUE
	"visuals_mvm_boss",	// Hack to override things in MvM at a general level
};

int GetTeamVisualsFromString( const char *pszString )
{
	for ( int i = 0; i < TEAM_VISUAL_SECTIONS; i++ )
	{
		// There's a NULL hidden in g_TeamVisualSections
		if ( g_TeamVisualSections[i] && !Q_stricmp( pszString, g_TeamVisualSections[i] ) )
			return i;
	}
	return -1;
}

#if defined(CLIENT_DLL) || defined(GAME_DLL)
// Used to convert strings to ints for wearable animation types
const char *g_WearableAnimTypeStrings[ NUM_WAP_TYPES ] =
{
	"on_spawn",			// WAP_ON_SPAWN,
	"start_building",	// WAP_START_BUILDING,
	"stop_building",	// WAP_STOP_BUILDING,
	"start_taunting",		// WAP_START_TAUNTING,
	"stop_taunting",	// WAP_STOP_TAUNTING,
};
#endif

const char *g_AttributeDescriptionFormats[] =
{
	"value_is_percentage",				// ATTDESCFORM_VALUE_IS_PERCENTAGE,
	"value_is_inverted_percentage",		// ATTDESCFORM_VALUE_IS_INVERTED_PERCENTAGE
	"value_is_additive",				// ATTDESCFORM_VALUE_IS_ADDITIVE
	"value_is_additive_percentage",		// ATTDESCFORM_VALUE_IS_ADDITIVE_PERCENTAGE
	"value_is_or",						// ATTDESCFORM_VALUE_IS_OR
	"value_is_date",					// ATTDESCFORM_VALUE_IS_DATE
	"value_is_account_id",				// ATTDESCFORM_VALUE_IS_ACCOUNT_ID
	"value_is_particle_index",			// ATTDESCFORM_VALUE_IS_PARTICLE_INDEX -> Could change to "string index"
	"value_is_killstreakeffect_index",	// ATTDESCFORM_VALUE_IS_KILLSTREAKEFFECT_INDEX -> Could change to "string index"
	"value_is_killstreak_idleeffect_index",  // ATTDESCFORM_VALUE_IS_KILLSTREAK_IDLEEFFECT_INDEX
	"value_is_item_def",				// ATTDESCFORM_VALUE_IS_ITEM_DEF
	"value_is_from_lookup_table",		// ATTDESCFORM_VALUE_IS_FROM_LOOKUP_TABLE
};

const char *g_EffectTypes[NUM_EFFECT_TYPES] =
{
	"neutral",		// ATTRIB_EFFECT_NEUTRAL = 0,
	"positive",		// ATTRIB_EFFECT_POSITIVE,
	"negative",		// ATTRIB_EFFECT_NEGATIVE,
};

//-----------------------------------------------------------------------------
// Purpose: Set the capabilities bitfield based on whether the entry is true/false.
//-----------------------------------------------------------------------------
const char *g_Capabilities[] =
{
	"paintable",				// ITEM_CAP_PAINTABLE
	"nameable",					// ITEM_CAP_NAMEABLE
	"decodable",				// ITEM_CAP_DECODABLE
	"can_craft_if_purchased",	// ITEM_CAP_CAN_BE_CRAFTED_IF_PURCHASED
	"can_customize_texture",	// ITEM_CAP_CAN_CUSTOMIZE_TEXTURE
	"usable",					// ITEM_CAP_USABLE
	"usable_gc",				// ITEM_CAP_USABLE_GC
	"can_gift_wrap",			// ITEM_CAP_CAN_GIFT_WRAP
	"usable_out_of_game",		// ITEM_CAP_USABLE_OUT_OF_GAME
	"can_collect",				// ITEM_CAP_CAN_COLLECT
	"can_craft_count",			// ITEM_CAP_CAN_CRAFT_COUNT
	"can_craft_mark",			// ITEM_CAP_CAN_CRAFT_MARK
	"paintable_team_colors",	// ITEM_CAP_PAINTABLE_TEAM_COLORS
	"can_be_restored",			// ITEM_CAP_CAN_BE_RESTORED
	"strange_parts",			// ITEM_CAP_CAN_USE_STRANGE_PARTS
	"can_card_upgrade",			// ITEM_CAP_CAN_CARD_UPGRADE
	"can_strangify",			// ITEM_CAP_CAN_STRANGIFY
	"can_killstreakify",		// ITEM_CAP_CAN_KILLSTREAKIFY
	"can_consume",				// ITEM_CAP_CAN_CONSUME_ITEMS
	"can_spell_page",			// ITEM_CAP_CAN_SPELLBOOK_PAGE
	"has_slots",				// ITEM_CAP_HAS_SLOTS
	"duck_upgradable",			// ITEM_CAP_DUCK_UPGRADABLE
	"can_unusualify",			// ITEM_CAP_CAN_UNUSUALIFY
};
COMPILE_TIME_ASSERT( ARRAYSIZE(g_Capabilities) == NUM_ITEM_CAPS );

#define RETURN_ATTRIBUTE_STRING( attrib_name, default_string ) \
	static CSchemaAttributeDefHandle pAttribString( attrib_name ); \
	const char *pchResultAttribString = default_string; \
	FindAttribute_UnsafeBitwiseCast< CAttribute_String >( this, pAttribString, &pchResultAttribString ); \
	return pchResultAttribString;

#define RETURN_ATTRIBUTE_STRING_F( func_name, attrib_name, default_string ) \
	const char *func_name( void ) const { RETURN_ATTRIBUTE_STRING( attrib_name, default_string ) }

static void ParseCapability( item_capabilities_t &capsBitfield, KeyValues* pEntry )
{
	int idx = StringFieldToInt(  pEntry->GetName(), g_Capabilities, ARRAYSIZE(g_Capabilities) );
	if ( idx < 0 )
	{
		return;
	}
	int bit = 1 << idx;
	if ( pEntry->GetBool() )
	{
		(int&)capsBitfield |= bit;
	}
	else
	{
		(int&)capsBitfield &= ~bit;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEconColorDefinition::BInitFromKV( KeyValues *pKVColor, CUtlVector<CUtlString> *pVecErrors /* = NULL */ )
{
	m_strName		= pKVColor->GetName();
	m_strColorName	= pKVColor->GetString( "color_name" );

	SCHEMA_INIT_CHECK(
		!m_strColorName.IsEmpty(),
		"Quality definition %s: missing \"color_name\"", GetName() );


	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
	#define GC_SCH_REFERENCE( TAttribSchType )

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int Internal_GetAttributeTypeUniqueIdentifierNextValue()
{
	static unsigned int s_unUniqueCounter = 0;

	unsigned int unCounter = s_unUniqueCounter;
	s_unUniqueCounter++;
	return unCounter;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template < GC_SCH_REFERENCE( typename TAttribSchType ) typename TAttribInMemoryType >
class CSchemaAttributeTypeBase : public ISchemaAttributeTypeBase<TAttribInMemoryType>
{
public:
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template < GC_SCH_REFERENCE( typename TAttribSchType ) typename TProtobufValueType >
class CSchemaAttributeTypeProtobufBase : public CSchemaAttributeTypeBase< GC_SCH_REFERENCE( TAttribSchType ) TProtobufValueType >
{
public:
	virtual void ConvertTypedValueToByteStream( const TProtobufValueType& typedValue, ::std::string *out_psBytes ) const OVERRIDE
	{
		DbgVerify( typedValue.SerializeToString( out_psBytes ) );
	}

	virtual void ConvertByteStreamToTypedValue( const ::std::string& sBytes, TProtobufValueType *out_pTypedValue ) const OVERRIDE
	{
		DbgVerify( out_pTypedValue->ParseFromString( sBytes ) );
	}

	virtual bool BConvertStringToEconAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const char *pszValue, union attribute_data_union_t *out_pValue, bool bEnableTerribleBackwardsCompatibilitySchemaParsingCode ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_pValue );
		
		std::string sValue( pszValue );
		TProtobufValueType typedValue;
		if ( !google::protobuf::TextFormat::ParseFromString( sValue, &typedValue ) )
			return false;

		this->ConvertTypedValueToEconAttributeValue( typedValue, out_pValue );
		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconItemAttributeDefinition *pAttrDef, const attribute_data_union_t& value, std::string *out_ps ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_ps );

		google::protobuf::TextFormat::PrintToString( this->GetTypedValueContentsFromEconAttributeValue( value ), out_ps );
	}
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CSchemaAttributeType_String : public CSchemaAttributeTypeProtobufBase< GC_SCH_REFERENCE( CSchItemAttributeString ) CAttribute_String >
{
public:

	// We intentionally override the convert-to-/convert-from-string functions for strings so that string literals can be
	// specified in the schema, etc. without worrying about the protobuf text format.
	virtual bool BConvertStringToEconAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const char *pszValue, union attribute_data_union_t *out_pValue, bool bEnableTerribleBackwardsCompatibilitySchemaParsingCode ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_pValue );

		CAttribute_String typedValue;
		typedValue.set_value( pszValue );

		this->ConvertTypedValueToEconAttributeValue( typedValue, out_pValue );

		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconItemAttributeDefinition *pAttrDef, const attribute_data_union_t& value, std::string *out_ps ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_ps );

		*out_ps = this->GetTypedValueContentsFromEconAttributeValue( value ).value().c_str();
	}
};

void CopyStringAttributeValueToCharPointerOutput( const CAttribute_String *pValue, const char **out_pValue )
{
	Assert( pValue );
	Assert( out_pValue );

	*out_pValue = pValue->value().c_str();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CSchemaAttributeType_DynamicRecipeComponentDefinedItem : public CSchemaAttributeTypeProtobufBase< GC_SCH_REFERENCE( CSchItemAttributeDynamicRecipeComponentDefinedItem ) CAttribute_DynamicRecipeComponent >
{
public:
};


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CSchemaAttributeType_ItemSlotCriteria : public CSchemaAttributeTypeProtobufBase< GC_SCH_REFERENCE( CSchItemAttributeItemSlotCriteria ) CAttribute_ItemSlotCriteria >
{
public:

	virtual bool BConvertStringToEconAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const char *pszValue, union attribute_data_union_t *out_pValue, bool bEnableTerribleBackwardsCompatibilitySchemaParsingCode ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_pValue );

		std::string sValue( pszValue );
		CAttribute_ItemSlotCriteria typedValue;
		if ( !google::protobuf::TextFormat::ParseFromString( sValue, &typedValue ) )
			return false;

		this->ConvertTypedValueToEconAttributeValue( typedValue, out_pValue );

		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconItemAttributeDefinition *pAttrDef, const attribute_data_union_t& value, std::string *out_ps ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_ps );

		this->ConvertEconAttributeValueToString( pAttrDef, value, out_ps );
	}
};


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CSchemaAttributeType_WorldItemPlacement : public CSchemaAttributeTypeProtobufBase < GC_SCH_REFERENCE( CSchItemAttributeWorldItemPlacement ) CAttribute_WorldItemPlacement >
{
public:

	virtual bool BConvertStringToEconAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const char *pszValue, union attribute_data_union_t *out_pValue, bool bEnableTerribleBackwardsCompatibilitySchemaParsingCode ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_pValue );

		CAttribute_WorldItemPlacement typedValue;

		uint32 unValue = ( pszValue ) ? atoi( pszValue ) : 0;
		
		// Item forcing us to create the attribute (via force_gc_to_generate)
		if ( unValue == 0 )
		{
			typedValue.set_original_item_id( INVALID_ITEM_ID );
			typedValue.set_pos_x( 0.f );
			typedValue.set_pos_y( 0.f );
			typedValue.set_pos_z( 0.f );
			typedValue.set_ang_x( 0.f );
			typedValue.set_ang_y( 0.f );
			typedValue.set_ang_z( 0.f );
		}
		else
		{
			std::string sValue( pszValue );
			if ( !google::protobuf::TextFormat::ParseFromString( sValue, &typedValue ) )
				return false;
		}
		
		this->ConvertTypedValueToEconAttributeValue( typedValue, out_pValue );
		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconItemAttributeDefinition *pAttrDef, const attribute_data_union_t& value, std::string *out_ps ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_ps );

		this->ConvertEconAttributeValueToString( pAttrDef, value, out_ps );
	}
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CSchemaAttributeType_Float : public CSchemaAttributeTypeBase< GC_SCH_REFERENCE( CSchItemAttributeFloat ) float >
{
public:

	virtual bool BConvertStringToEconAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const char *pszValue, union attribute_data_union_t *out_pValue, bool bEnableTerribleBackwardsCompatibilitySchemaParsingCode ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_pValue );

		out_pValue->asFloat = Q_atof( pszValue );
		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconItemAttributeDefinition *pAttrDef, const attribute_data_union_t& value, std::string *out_ps ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_ps );

		*out_ps = CFmtStr( "%f", value.asFloat ).Get();
	}
	
	virtual void ConvertTypedValueToByteStream( const float& typedValue, ::std::string *out_psBytes ) const OVERRIDE
	{
		Assert( out_psBytes );
		Assert( out_psBytes->size() == 0 );

		out_psBytes->resize( sizeof( float ) );
		*reinterpret_cast<float *>( &((*out_psBytes)[0]) ) = typedValue;		// overwrite string contents (sizeof( float ) bytes)
	}

	virtual void ConvertByteStreamToTypedValue( const ::std::string& sBytes, float *out_pTypedValue ) const OVERRIDE
	{
		Assert( out_pTypedValue );
		Assert( sBytes.size() == sizeof( float ) );

		*out_pTypedValue = *reinterpret_cast<const float *>( &sBytes[0] );
	}

	virtual bool BSupportsGameplayModificationAndNetworking() const OVERRIDE
	{
		return true;
	}
};


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CSchemaAttributeType_UInt64 : public CSchemaAttributeTypeBase< GC_SCH_REFERENCE( CSchItemAttributeUInt64 ) uint64 >
{
public:

	virtual bool BConvertStringToEconAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const char *pszValue, union attribute_data_union_t *out_pValue, bool bEnableTerribleBackwardsCompatibilitySchemaParsingCode ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_pValue );

		out_pValue->asUint32 = V_atoui64( pszValue );
		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconItemAttributeDefinition *pAttrDef, const attribute_data_union_t& value, std::string *out_ps ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_ps );

		uint64 ulValue;
		ConvertEconAttributeValueToTypedValue( value, &ulValue );

		*out_ps = CFmtStr( "%llu", ulValue ).Get();
	}
	
	virtual void ConvertTypedValueToByteStream( const uint64& typedValue, ::std::string *out_psBytes ) const OVERRIDE
	{
		Assert( out_psBytes );
		Assert( out_psBytes->size() == 0 );

		out_psBytes->resize( sizeof( uint64 ) );
		*reinterpret_cast<uint64 *>( &((*out_psBytes)[0]) ) = typedValue;		// overwrite string contents (sizeof( uint64 ) bytes)
	}

	virtual void ConvertByteStreamToTypedValue( const ::std::string& sBytes, uint64 *out_pTypedValue ) const OVERRIDE
	{
		Assert( out_pTypedValue );
		Assert( sBytes.size() == sizeof( uint64 ) );

		*out_pTypedValue = *reinterpret_cast<const uint64 *>( &sBytes[0] );
	}
};


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CSchemaAttributeType_Default : public CSchemaAttributeTypeBase< GC_SCH_REFERENCE( CSchItemAttribute ) attrib_value_t >
{
public:

	virtual bool BConvertStringToEconAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const char *pszValue, union attribute_data_union_t *out_pValue, bool bEnableTerribleBackwardsCompatibilitySchemaParsingCode ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_pValue );

		if ( bEnableTerribleBackwardsCompatibilitySchemaParsingCode )
		{
			// Not having any value specified is valid -- we interpret this as "default", or 0 as both an in int and a float.
			out_pValue->asFloat = pszValue
								? atof( pszValue )
								: 0.0f;
		}
		// This is terrible backwards-compatibility code to support the pulling of values from econ asset classes.
		else
		{
			if ( pAttrDef->IsStoredAsInteger() )
			{
				out_pValue->asUint32 = (uint32)Q_atoui64( pszValue );
			}
			else if ( pAttrDef->IsStoredAsFloat() )
			{
				out_pValue->asFloat = Q_atof( pszValue );
			}
			else
			{
				Assert( !"Unknown storage type for CSchemaAttributeType_Default::BConvertStringToEconAttributeValue()!" );
				return false;
			}
		}

		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconItemAttributeDefinition *pAttrDef, const attribute_data_union_t& value, std::string *out_ps ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_ps );

		if( pAttrDef->IsStoredAsFloat() )
		{
			*out_ps = CFmtStr( "%f", value.asFloat ).Get();
		}
		else if( pAttrDef->IsStoredAsInteger() )
		{
			*out_ps = CFmtStr( "%u", value.asUint32 ).Get();
		}
		else
		{
			Assert( !"Unknown storage type for CSchemaAttributeType_Default::ConvertEconAttributeValueToString()!" );
		}
	}
	
	virtual void ConvertTypedValueToByteStream( const attrib_value_t& typedValue, ::std::string *out_psBytes ) const OVERRIDE
	{
		Assert( out_psBytes );
		Assert( out_psBytes->size() == 0 );

		out_psBytes->resize( sizeof( attrib_value_t ) );
		*reinterpret_cast<attrib_value_t *>( &((*out_psBytes)[0]) ) = typedValue;		// overwrite string contents (sizeof( attrib_value_t ) bytes)
	}

	virtual void ConvertByteStreamToTypedValue( const ::std::string& sBytes, attrib_value_t *out_pTypedValue ) const OVERRIDE
	{
		Assert( out_pTypedValue );
		// Game clients and servers may have partially out-of-date information, or may have downloaded a new schema
		// but not know how to parse an attribute of a certain type, etc. In these cases, because we know we
		// aren't on the GC, temporarily failing to load these values until the client shuts down and updates
		// is about the best we can hope for.
		if ( sBytes.size() < sizeof( attrib_value_t ) )
		{
			*out_pTypedValue = attrib_value_t();
			return;
		}

		*out_pTypedValue = *reinterpret_cast<const attrib_value_t *>( &sBytes[0] );
	}

	virtual bool BSupportsGameplayModificationAndNetworking() const OVERRIDE
	{
		return true;
	}
	
private:
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CEconItemAttributeDefinition::CEconItemAttributeDefinition( void )
:	m_pKVAttribute( NULL ),
	m_pAttrType( NULL ),
	m_bHidden( false ),
	m_bWebSchemaOutputForced( false ),
	m_bStoredAsInteger( false ),
	m_bInstanceData( false ),
	m_bIsSetBonus( false ),
	m_iUserGenerationType( 0 ),
	m_iEffectType( ATTRIB_EFFECT_NEUTRAL ),
	m_iDescriptionFormat( 0 ),
	m_pszDescriptionString( NULL ),
	m_pszDefinitionName( NULL ),
	m_pszAttributeClass( NULL ),
	m_ItemDefinitionTag( INVALID_ECON_TAG_HANDLE ),
	m_bCanAffectRecipeComponentName( false )
  , m_iszAttributeClass( NULL_STRING )
{
}


//-----------------------------------------------------------------------------
// Purpose:	Copy constructor
//-----------------------------------------------------------------------------
CEconItemAttributeDefinition::CEconItemAttributeDefinition( const CEconItemAttributeDefinition &that )
{
	(*this) = that;
}


//-----------------------------------------------------------------------------
// Purpose:	Operator=
//-----------------------------------------------------------------------------
CEconItemAttributeDefinition &CEconItemAttributeDefinition::operator=( const CEconItemAttributeDefinition &rhs )
{
	m_nDefIndex = rhs.m_nDefIndex;
	m_pAttrType = rhs.m_pAttrType;
	m_bHidden = rhs.m_bHidden;
	m_bWebSchemaOutputForced = rhs.m_bWebSchemaOutputForced;
	m_bStoredAsInteger = rhs.m_bStoredAsInteger;
	m_iUserGenerationType = rhs.m_iUserGenerationType;
	m_bInstanceData = rhs.m_bInstanceData;
	m_bIsSetBonus = rhs.m_bIsSetBonus;
	m_iEffectType = rhs.m_iEffectType;
	m_iDescriptionFormat = rhs.m_iDescriptionFormat;
	m_pszDescriptionString = rhs.m_pszDescriptionString;
	m_pszDefinitionName = rhs.m_pszDefinitionName;
	m_pszAttributeClass = rhs.m_pszAttributeClass;
	m_ItemDefinitionTag = rhs.m_ItemDefinitionTag;
	m_bCanAffectRecipeComponentName = rhs.m_bCanAffectRecipeComponentName;
	m_iszAttributeClass = rhs.m_iszAttributeClass;

	m_pKVAttribute = NULL;
	if ( NULL != rhs.m_pKVAttribute )
	{
		m_pKVAttribute = rhs.m_pKVAttribute->MakeCopy();

		// Re-assign string pointers
		m_pszDefinitionName = m_pKVAttribute->GetString("name");
		m_pszDescriptionString = m_pKVAttribute->GetString( "description_string", NULL );
		m_pszAttributeClass = m_pKVAttribute->GetString( "attribute_class", NULL );

		Assert( V_strcmp( m_pszDefinitionName, rhs.m_pszDefinitionName ) == 0 );
		Assert( V_strcmp( m_pszDescriptionString, rhs.m_pszDescriptionString ) == 0 );
		Assert( V_strcmp( m_pszAttributeClass, rhs.m_pszAttributeClass ) == 0 );
	}
	else
	{
		Assert( m_pszDefinitionName == NULL );
		Assert( m_pszDescriptionString == NULL );
		Assert( m_pszAttributeClass == NULL );
	}
	return *this;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CEconItemAttributeDefinition::~CEconItemAttributeDefinition( void )
{
	if ( m_pKVAttribute )
		m_pKVAttribute->deleteThis();
	m_pKVAttribute = NULL;
}


//-----------------------------------------------------------------------------
// Purpose:	Initialize the attribute definition
// Input:	pKVAttribute - The KeyValues representation of the attribute
//			schema - The overall item schema for this attribute
//			pVecErrors - An optional vector that will contain error messages if 
//				the init fails.
// Output:	True if initialization succeeded, false otherwise
//-----------------------------------------------------------------------------
bool CEconItemAttributeDefinition::BInitFromKV( KeyValues *pKVAttribute, CUtlVector<CUtlString> *pVecErrors /* = NULL */ )
{
	m_pKVAttribute = pKVAttribute->MakeCopy();
	m_nDefIndex = Q_atoi( m_pKVAttribute->GetName() );
	
	m_pszDefinitionName = m_pKVAttribute->GetString("name", "(unnamed)");
	m_bHidden = m_pKVAttribute->GetInt( "hidden", 0 ) != 0;
	m_bWebSchemaOutputForced = m_pKVAttribute->GetInt( "force_output_description", 0 ) != 0;
	m_bStoredAsInteger = m_pKVAttribute->GetInt( "stored_as_integer", 0 ) != 0;
	m_bIsSetBonus = m_pKVAttribute->GetBool( "is_set_bonus", false );
	m_bCanAffectRecipeComponentName = m_pKVAttribute->GetBool( "can_affect_recipe_component_name", false );
	m_iUserGenerationType = m_pKVAttribute->GetInt( "is_user_generated", 0 );
	m_iEffectType = (attrib_effect_types_t)StringFieldToInt( m_pKVAttribute->GetString("effect_type"), g_EffectTypes, ARRAYSIZE(g_EffectTypes) );
	m_iDescriptionFormat = StringFieldToInt( m_pKVAttribute->GetString("description_format"), g_AttributeDescriptionFormats, ARRAYSIZE(g_AttributeDescriptionFormats) );
	m_pszDescriptionString = m_pKVAttribute->GetString( "description_string", NULL );
	m_pszAttributeClass = m_pKVAttribute->GetString( "attribute_class", NULL );
	m_bInstanceData = pKVAttribute->GetBool( "instance_data", false );

	const char *pszTag = m_pKVAttribute->GetString( "apply_tag_to_item_definition", NULL );
	m_ItemDefinitionTag = pszTag ? GetItemSchema()->GetHandleForTag( pszTag ) : INVALID_ECON_TAG_HANDLE;

#if defined(CLIENT_DLL) || defined(GAME_DLL)
	m_iszAttributeClass = NULL_STRING;
#endif
	const char *pszAttrType = m_pKVAttribute->GetString( "attribute_type", NULL );		// NULL implies "default type" for backwards compatibility
	m_pAttrType = GetItemSchema()->GetAttributeType( pszAttrType );

	SCHEMA_INIT_CHECK( 
		NULL != m_pKVAttribute->FindKey( "name" ), 
		"Attribute definition %s: Missing required field \"name\"", m_pKVAttribute->GetName() );

	SCHEMA_INIT_CHECK(
		NULL != m_pAttrType,
		"Attribute definition %s: Unable to find attribute data type '%s'", m_pszDefinitionName, pszAttrType ? pszAttrType : "(default)" );

	if ( m_bIsSetBonus )
	{
		SCHEMA_INIT_CHECK(
			m_pAttrType->BSupportsGameplayModificationAndNetworking(),
			"Attribute definition %s: set as set bonus attribute but does not support gameplay modification/networking!", m_pszDefinitionName );
	}

	m_unAssetClassBucket = pKVAttribute->GetInt( "asset_class_bucket", 0 );
	m_eAssetClassAttrExportRule = k_EAssetClassAttrExportRule_Default;
	if ( char const *szRule = pKVAttribute->GetString( "asset_class_export", NULL ) )
	{
		if ( !V_stricmp( szRule, "skip" ) )
		{
			m_eAssetClassAttrExportRule = k_EAssetClassAttrExportRule_Skip;
		}
		else if ( !V_stricmp( szRule, "gconly" ) )
		{
			m_eAssetClassAttrExportRule = EAssetClassAttrExportRule_t( k_EAssetClassAttrExportRule_GCOnly | k_EAssetClassAttrExportRule_Skip );
		}
		else if ( !V_stricmp( szRule, "bucketed" ) )
		{
			SCHEMA_INIT_CHECK( m_unAssetClassBucket, "Attribute definition %s: Asset class export rule '%s' is incompatible", m_pszDefinitionName, szRule );
			m_eAssetClassAttrExportRule = k_EAssetClassAttrExportRule_Bucketed;
		}
		else if ( !V_stricmp( szRule, "default" ) )
		{
			m_eAssetClassAttrExportRule = k_EAssetClassAttrExportRule_Default;
		}
		else
		{
			SCHEMA_INIT_CHECK( false, "Attribute definition %s: Invalid asset class export rule '%s'", m_pszDefinitionName, szRule );
		}
	}

	// Check for misuse of asset class bucket
	SCHEMA_INIT_CHECK( ( !m_unAssetClassBucket || m_bInstanceData ), "Attribute definition %s: Cannot use \"asset_class_bucket\" on class-level attributes", m_pKVAttribute->GetName() );


	return SCHEMA_INIT_SUCCESS();
}



//-----------------------------------------------------------------------------
// Purpose:	Constructor
//-----------------------------------------------------------------------------
CEconItemDefinition::CEconItemDefinition( void )
:	m_pKVItem( NULL ),
m_bEnabled( false ),
m_bLoadOnDemand( false ),
m_nPopularitySeed( 0 ),
m_pszDefinitionName( NULL ),
m_pszItemClassname( NULL ),
m_pszItemBaseName( NULL ),
m_pszItemTypeName( NULL ),
m_pszItemDesc( NULL ),
m_pszInventoryModel( NULL ),
m_pszInventoryImage( NULL ),
m_iSubType( 0 ),
m_pszBaseDisplayModel( NULL ),
m_iDefaultSkin( -1 ),
m_pszWorldDisplayModel( NULL ),
m_pszWorldExtraWearableModel( NULL ),
m_pszWorldExtraWearableViewModel( NULL ),
m_pszVisionFilteredDisplayModel( NULL ),
m_pszBrassModelOverride( NULL ),
m_bHideBodyGroupsDeployedOnly( false ),
m_bAttachToHands( false ),
m_bAttachToHandsVMOnly( false ),
m_bProperName( false ),
m_bFlipViewModel( false ),
m_bActAsWearable( false ),
m_bActAsWeapon( false ),
m_iDropType( 1 ),
m_bHidden( false ),
m_bBaseItem( false ),
m_pszItemLogClassname( NULL ),
m_pszItemIconClassname( NULL ),
m_pszDatabaseAuditTable( NULL ),
m_bImported( false ),
m_pszBaseFunctionalItemName( NULL ),
m_pszParticleSuffix( NULL ),
m_nRemappedDefIndex( INVALID_ITEM_DEF_INDEX )
{
	for ( int team = 0; team < TEAM_VISUAL_SECTIONS; team++ )
	{
		m_PerTeamVisuals[team] = NULL;
	}

	m_pDictIcons = new CUtlDict< CUtlString >;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CEconItemDefinition::~CEconItemDefinition( void )
{
	for ( int i = 0; i < ARRAYSIZE( m_PerTeamVisuals ); i++ )
		delete m_PerTeamVisuals[i];


	if ( m_pKVItem )
		m_pKVItem->deleteThis();
	m_pKVItem = NULL;
	delete m_pDictIcons;
}

#if defined(CLIENT_DLL) || defined(GAME_DLL)
//-----------------------------------------------------------------------------
// Purpose: Stomp our base data with extra testing data specified by the player
//-----------------------------------------------------------------------------
bool CEconItemDefinition::BInitFromTestItemKVs( int iNewDefIndex, KeyValues *pKVItem, CUtlVector<CUtlString>* pVecErrors )
{
	// The KeyValues are stored in the player entity, so we can cache our name there

	m_nDefIndex = iNewDefIndex;

	bool bTestingExistingItem = pKVItem->GetBool( "test_existing_item", false );
	if ( !bTestingExistingItem )
	{
		m_pszDefinitionName = pKVItem->GetString( "name", NULL );
		m_pszItemBaseName = pKVItem->GetString( "name", NULL );

#ifdef CLIENT_DLL
		pKVItem->SetString( "name", VarArgs("Test Item %d", iNewDefIndex) );
#else
		pKVItem->SetString( "name", UTIL_VarArgs("Test Item %d", iNewDefIndex) );
#endif

		m_pszBaseDisplayModel = pKVItem->GetString( "model_player", NULL );
		m_pszVisionFilteredDisplayModel = pKVItem->GetString( "model_vision_filtered", NULL );
		m_bAttachToHands = pKVItem->GetInt( "attach_to_hands", 0 ) != 0;

		BInitVisualBlockFromKV( pKVItem );
	}

	// Handle attributes
	m_vecStaticAttributes.Purge();
	int iPaintCanIndex = pKVItem->GetInt("paintcan_index", 0);
	if ( iPaintCanIndex )
	{
		static CSchemaAttributeDefHandle pAttrDef_PaintRGB( "set item tint RGB" );

		const CEconItemDefinition *pCanDef = GetItemSchema()->GetItemDefinition(iPaintCanIndex);
		
		float flRGBVal;
		if ( pCanDef && pAttrDef_PaintRGB && FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pCanDef, pAttrDef_PaintRGB, &flRGBVal ) )
		{
			static_attrib_t& StaticAttrib = m_vecStaticAttributes[ m_vecStaticAttributes.AddToTail() ];

			StaticAttrib.iDefIndex = pAttrDef_PaintRGB->GetDefinitionIndex();
			StaticAttrib.m_value.asFloat = flRGBVal;							// this is bad! but we're in crazy hack code for UI customization of item definitions that don't exist so
		}
	}

	int iUnusualEffectIndex = pKVItem->GetInt( "unusual_index", 0 );
	if ( iUnusualEffectIndex )
	{
		static CSchemaAttributeDefHandle pAttrDef_AttachParticleStatic( "attach particle effect static" );

		const attachedparticlesystem_t *pSystem = GetItemSchema()->GetAttributeControlledParticleSystem( iUnusualEffectIndex );

		if ( pAttrDef_AttachParticleStatic && pSystem )
		{
			static_attrib_t& StaticAttrib = m_vecStaticAttributes[ m_vecStaticAttributes.AddToTail() ];

			StaticAttrib.iDefIndex = pAttrDef_AttachParticleStatic->GetDefinitionIndex();
			StaticAttrib.m_value.asFloat = iUnusualEffectIndex;					// this is bad! but we're in crazy hack code for UI customization of item definitions that don't exist so
		}
	}

	return true;
}

animation_on_wearable_t *GetOrCreateAnimationActivity( perteamvisuals_t *pVisData, const char *pszActivityName )
{
	FOR_EACH_VEC( pVisData->m_Animations, i )
	{
		if ( Q_stricmp(pVisData->m_Animations[i].pszActivity, pszActivityName) == 0 )
			return &pVisData->m_Animations[i];
	}

	animation_on_wearable_t *pEntry = &pVisData->m_Animations[pVisData->m_Animations.AddToTail()];

	pEntry->iActivity = kActivityLookup_Unknown;	// We can't look it up yet, the activity list hasn't been populated.
	pEntry->pszActivity = pszActivityName;
	pEntry->iReplacement = kActivityLookup_Unknown;
	pEntry->pszReplacement = NULL;
	pEntry->pszSequence = NULL;
	pEntry->pszScene = NULL;
	pEntry->pszRequiredItem = NULL;

	return pEntry;
}

activity_on_wearable_t *GetOrCreatePlaybackActivity( perteamvisuals_t *pVisData, wearableanimplayback_t iPlayback )
{
	FOR_EACH_VEC( pVisData->m_Animations, i )
	{
		if ( pVisData->m_Activities[i].iPlayback == iPlayback )
			return &pVisData->m_Activities[i];
	}

	activity_on_wearable_t *pEntry = &pVisData->m_Activities[pVisData->m_Activities.AddToTail()];

	pEntry->iPlayback = iPlayback;
	pEntry->iActivity = kActivityLookup_Unknown;	// We can't look it up yet, the activity list hasn't been populated.
	pEntry->pszActivity = NULL;

	return pEntry;
}

#endif // defined(CLIENT_DLL) || defined(GAME_DLL)

//-----------------------------------------------------------------------------
// Purpose: Handle parsing the per-team visual block from the keyvalues
//-----------------------------------------------------------------------------
void CEconItemDefinition::BInitVisualBlockFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors )
{
	// Visuals
	for ( int team = 0; team < TEAM_VISUAL_SECTIONS; team++ )
	{
		m_PerTeamVisuals[team] = NULL;

		if ( !g_TeamVisualSections[team] )
			continue;

		KeyValues *pVisualsKV = pKVItem->FindKey( g_TeamVisualSections[team] );
		if ( pVisualsKV )
		{
			perteamvisuals_t *pVisData = new perteamvisuals_t();
#if defined(CLIENT_DLL) || defined(GAME_DLL)
			KeyValues *pKVEntry = pVisualsKV->GetFirstSubKey();
			while ( pKVEntry )
			{
				const char *pszEntry = pKVEntry->GetName();

				if ( !Q_stricmp( pszEntry, "use_visualsblock_as_base" ) )
				{
					// Start with a copy of an existing PerTeamVisuals
					const char *pszString = pKVEntry->GetString();
					int nOverrideTeam = GetTeamVisualsFromString( pszString );
					if ( nOverrideTeam != -1 )
					{
						*pVisData = *m_PerTeamVisuals[nOverrideTeam];
					}
					else
					{
						pVecErrors->AddToTail( CFmtStr( "Unknown visuals block: %s", pszString ).Access() );
					}
				}
				else if ( !Q_stricmp( pszEntry, "attached_models" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVAttachedModelData )
					{
						int iAtt = pVisData->m_AttachedModels.AddToTail();
						pVisData->m_AttachedModels[iAtt].m_iModelDisplayFlags = pKVAttachedModelData->GetInt( "model_display_flags", kAttachedModelDisplayFlag_MaskAll );
						pVisData->m_AttachedModels[iAtt].m_pszModelName = pKVAttachedModelData->GetString( "model", NULL );
					}
				}
				else if ( !Q_stricmp( pszEntry, "attached_models_festive" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVAttachedModelData )
					{
						int iAtt = pVisData->m_AttachedModelsFestive.AddToTail();
						pVisData->m_AttachedModelsFestive[iAtt].m_iModelDisplayFlags = pKVAttachedModelData->GetInt( "model_display_flags", kAttachedModelDisplayFlag_MaskAll );
						pVisData->m_AttachedModelsFestive[iAtt].m_pszModelName = pKVAttachedModelData->GetString( "model", NULL );
					}
				}
				else if ( !Q_stricmp( pszEntry, "attached_particlesystems" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVAttachedParticleSystemData )
					{
						int iAtt = pVisData->m_AttachedParticles.AddToTail();
						pVisData->m_AttachedParticles[iAtt].pszSystemName = pKVAttachedParticleSystemData->GetString( "system", NULL );
						pVisData->m_AttachedParticles[iAtt].pszControlPoints[0] = pKVAttachedParticleSystemData->GetString( "attachment", NULL );
						pVisData->m_AttachedParticles[iAtt].bFollowRootBone = pKVAttachedParticleSystemData->GetBool( "attach_to_rootbone" );
						pVisData->m_AttachedParticles[iAtt].iCustomType = 0;
					}
				}
				else if ( !Q_stricmp( pszEntry, "custom_particlesystem2" ) )
				{
					int iAtt = pVisData->m_AttachedParticles.AddToTail();
					pVisData->m_AttachedParticles[iAtt].pszSystemName = pKVEntry->GetString( "system", NULL );
					pVisData->m_AttachedParticles[iAtt].iCustomType = 2;
				}
				else if ( !Q_stricmp( pszEntry, "custom_particlesystem" ) )
				{
					int iAtt = pVisData->m_AttachedParticles.AddToTail();
					pVisData->m_AttachedParticles[iAtt].pszSystemName = pKVEntry->GetString( "system", NULL );
					pVisData->m_AttachedParticles[iAtt].iCustomType = 1;
				}
				else if ( !Q_stricmp( pszEntry, "playback_activity" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVSubKey )
					{
						int iPlaybackInt = StringFieldToInt( pKVSubKey->GetName(), g_WearableAnimTypeStrings, ARRAYSIZE(g_WearableAnimTypeStrings) );
						if ( iPlaybackInt >= 0 )
						{
							activity_on_wearable_t *pEntry = GetOrCreatePlaybackActivity( pVisData, (wearableanimplayback_t)iPlaybackInt );
							pEntry->pszActivity = pKVSubKey->GetString();
						}
					}
				}
				else if ( !Q_stricmp( pszEntry, "animation_replacement" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVSubKey )
					{
						animation_on_wearable_t *pEntry = GetOrCreateAnimationActivity( pVisData, pKVSubKey->GetName() );
						pEntry->pszReplacement = pKVSubKey->GetString();
					}
				}
				else if ( !Q_stricmp( pszEntry, "animation_sequence" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVSubKey )
					{
						animation_on_wearable_t *pEntry = GetOrCreateAnimationActivity( pVisData, pKVSubKey->GetName() );
						pEntry->pszSequence = pKVSubKey->GetString();
					}
				}
				else if ( !Q_stricmp( pszEntry, "animation_scene" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVSubKey )
					{
						animation_on_wearable_t *pEntry = GetOrCreateAnimationActivity( pVisData, pKVSubKey->GetName() );
						pEntry->pszScene = pKVSubKey->GetString();
					}
				}
				else if ( !Q_stricmp( pszEntry, "animation_required_item" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVSubKey )
					{
						animation_on_wearable_t *pEntry = GetOrCreateAnimationActivity( pVisData, pKVSubKey->GetName() );
						pEntry->pszRequiredItem = pKVSubKey->GetString();
					}
				}
				else if ( !Q_stricmp( pszEntry, "player_poseparam" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVSubKey )
					{
						poseparamtable_t *pPoseParam = pVisData->m_PlayerPoseParams.AddToTailGetPtr();
						pPoseParam->strName = pKVSubKey->GetName();
						pPoseParam->flValue = pKVSubKey->GetFloat();
					}
				}
				else if ( !Q_stricmp( pszEntry, "item_poseparam" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVSubKey )
					{
						poseparamtable_t *pPoseParam = pVisData->m_ItemPoseParams.AddToTailGetPtr();
						pPoseParam->strName = pKVSubKey->GetName();
						pPoseParam->flValue = pKVSubKey->GetFloat();
					}
				}
				else if ( !Q_stricmp( pszEntry, "player_bodygroups" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVBodygroupKey )
					{
						const char *pszBodygroupName = pKVBodygroupKey->GetName();
						int iValue = pKVBodygroupKey->GetInt();

						// Track bodygroup information for this item in particular.
						pVisData->m_Maps.m_ModifiedBodyGroupNames.Insert( pszBodygroupName, iValue );

						// Track global schema state.
						GetItemSchema()->AssignDefaultBodygroupState( pszBodygroupName, iValue );
					}
				}
				else if ( !Q_stricmp( pszEntry, "skin" ) )
				{
					pVisData->iSkin = pKVEntry->GetInt();
				}
				else if ( !Q_stricmp( pszEntry, "use_per_class_bodygroups" ) )
				{
					pVisData->bUsePerClassBodygroups = pKVEntry->GetBool();
				}
				else if ( !Q_stricmp( pszEntry, "muzzle_flash" ) )
				{
					pVisData->pszMuzzleFlash = pKVEntry->GetString();
				}
				else if ( !Q_stricmp( pszEntry, "tracer_effect" ) )
				{
					pVisData->pszTracerEffect = pKVEntry->GetString();
				}
				else if ( !Q_stricmp( pszEntry, "particle_effect" ) )
				{
					pVisData->pszParticleEffect = pKVEntry->GetString();
				}
				else if ( !Q_strnicmp( pszEntry, "custom_sound", 12 ) )			// intentionally comparing prefixes
				{
					int iIndex = 0;
					if ( pszEntry[12] )
					{
						iIndex = clamp( atoi( &pszEntry[12] ), 0, MAX_VISUALS_CUSTOM_SOUNDS-1 );
					}
					pVisData->pszCustomSounds[iIndex] = pKVEntry->GetString();
				}
				else if ( !Q_stricmp( pszEntry, "material_override" ) )
				{
					pVisData->pszMaterialOverride = pKVEntry->GetString();
				}
				else if ( !Q_strnicmp( pszEntry, "sound_", 6 ) )				// intentionally comparing prefixes
				{
					int iIndex = GetWeaponSoundFromString( &pszEntry[6] );
					if ( iIndex != -1 )
					{
						pVisData->pszWeaponSoundReplacements[iIndex] = pKVEntry->GetString();
					}
				}
				else if ( !Q_stricmp( pszEntry, "code_controlled_bodygroup" ) )
				{
					const char *pBodyGroupName = pKVEntry->GetString( "bodygroup", NULL );
					const char *pFuncName = pKVEntry->GetString( "function", NULL );
					if ( pBodyGroupName && pFuncName )
					{
						codecontrolledbodygroupdata_t ccbgd = { pFuncName, NULL };
						pVisData->m_Maps.m_CodeControlledBodyGroupNames.Insert( pBodyGroupName, ccbgd );
					}
				}
				else if ( !Q_stricmp( pszEntry, "vm_bodygroup_override" ) )
				{
					pVisData->m_iViewModelBodyGroupOverride = pKVEntry->GetInt();
				}
				else if ( !Q_stricmp( pszEntry, "vm_bodygroup_state_override" ) )
				{
					pVisData->m_iViewModelBodyGroupStateOverride = pKVEntry->GetInt();
				}
				else if ( !Q_stricmp( pszEntry, "wm_bodygroup_override" ) )
				{
					pVisData->m_iWorldModelBodyGroupOverride = pKVEntry->GetInt();
				}
				else if ( !Q_stricmp( pszEntry, "wm_bodygroup_state_override" ) )
				{
					pVisData->m_iWorldModelBodyGroupStateOverride = pKVEntry->GetInt();
				}

				pKVEntry = pKVEntry->GetNextKey();
			}
#endif // defined(CLIENT_DLL) || defined(GAME_DLL)
			KeyValues *pStylesDataKV = pVisualsKV->FindKey( "styles" );
			if ( pStylesDataKV )
			{
				// Styles are only valid in the base "visuals" section.
				if ( team == 0 )
				{
					BInitStylesBlockFromKV( pStylesDataKV, pVisData, pVecErrors );
				}
				// ...but they used to be valid everywhere, so spit out a warning if people are trying to use
				// the old style of per-team styles.
				else
				{
					pVecErrors->AddToTail( "Per-team styles blocks are no longer valid. Use \"skin_red\" and \"skin_blu\" in a style entry instead." );
				}
			}

			m_PerTeamVisuals[team] = pVisData;
		}
	}
}

#if defined(CLIENT_DLL) || defined(GAME_DLL)
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconItemDefinition::GeneratePrecacheModelStrings( bool bDynamicLoad, CUtlVector<const char *> *out_pVecModelStrings ) const
{
	Assert( out_pVecModelStrings );

	// Add base model.
	out_pVecModelStrings->AddToTail( GetBasePlayerDisplayModel() );

	// Add styles.
	if ( GetNumStyles() )
	{
		for ( style_index_t i=0; i<GetNumStyles(); ++i )
		{
			const CEconStyleInfo *pStyle = GetStyleInfo( i );
			Assert( pStyle );

			pStyle->GeneratePrecacheModelStringsForStyle( out_pVecModelStrings );
		}
	}

	// Precache all the attached models
	for ( int team = 0; team < TEAM_VISUAL_SECTIONS; team++ )
	{
		perteamvisuals_t *pPerTeamVisuals = GetPerTeamVisual( team );
		if ( !pPerTeamVisuals )
			continue;

		for ( int model = 0; model < pPerTeamVisuals->m_AttachedModels.Count(); model++ )
		{
			out_pVecModelStrings->AddToTail( pPerTeamVisuals->m_AttachedModels[model].m_pszModelName );
		}

		// Festive
		for ( int model = 0; model < pPerTeamVisuals->m_AttachedModelsFestive.Count(); model++ )
		{
			out_pVecModelStrings->AddToTail( pPerTeamVisuals->m_AttachedModelsFestive[model].m_pszModelName );
		}
	}

	if ( GetExtraWearableModel() )
	{
		out_pVecModelStrings->AddToTail( GetExtraWearableModel() );
	}

	if ( GetExtraWearableViewModel() )
	{
		out_pVecModelStrings->AddToTail( GetExtraWearableViewModel() );
	}

	if ( GetVisionFilteredDisplayModel() )
	{
		out_pVecModelStrings->AddToTail( GetVisionFilteredDisplayModel() );
	}

	// We don't need to cache the inventory model, because it's never loaded by the game
}

void CEconItemDefinition::GeneratePrecacheSoundStrings( bool bDynamicLoad, CUtlVector<const char *> *out_pVecSoundStrings ) const
{
	Assert( out_pVecSoundStrings );

	for ( int iTeam = 0; iTeam < TEAM_VISUAL_SECTIONS; ++iTeam )
	{
		for ( int iSound = 0; iSound < MAX_VISUALS_CUSTOM_SOUNDS; ++iSound )
		{
			const char *pSoundName = GetCustomSound( iTeam, iSound );
			if ( pSoundName && pSoundName[ 0 ] != '\0' )
			{
				out_pVecSoundStrings->AddToTail( pSoundName );
			}
		}
	}
}
#endif // #if defined(CLIENT_DLL) || defined(GAME_DLL)

//-----------------------------------------------------------------------------
const char	*CEconItemDefinition::GetDefinitionString( const char *pszKeyName, const char *pszDefaultValue ) const
{
	// !FIXME! Here we could do a dynamic lookup to apply the prefab overlay logic.
	// This could save a lot of duplicated data
	if ( m_pKVItem )
		return m_pKVItem->GetString( pszKeyName, pszDefaultValue );
	return pszDefaultValue;
}

//-----------------------------------------------------------------------------
KeyValues	*CEconItemDefinition::GetDefinitionKey( const char *pszKeyName ) const
{
	// !FIXME! Here we could do a dynamic lookup to apply the prefab overlay logic.
	// This could save a lot of duplicated data
	if ( m_pKVItem )
		return m_pKVItem->FindKey( pszKeyName );
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Parse the styles sub-section of the visuals block.
//-----------------------------------------------------------------------------
void CEconItemDefinition::BInitStylesBlockFromKV( KeyValues *pKVStyles, perteamvisuals_t *pVisData, CUtlVector<CUtlString> *pVecErrors )
{
	FOR_EACH_SUBKEY( pKVStyles, pKVStyle )
	{
		CEconStyleInfo *pStyleInfo = GetItemSchema()->CreateEconStyleInfo();
		Assert( pStyleInfo );

		pStyleInfo->BInitFromKV( pKVStyle, pVecErrors );

		pVisData->m_Styles.AddToTail( pStyleInfo );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Parse one style from the styles block.
//-----------------------------------------------------------------------------
void CEconStyleInfo::BInitFromKV( KeyValues *pKVStyle, CUtlVector<CUtlString> *pVecErrors )
{
	enum { kInvalidSkinKey = -1, };

	Assert( pKVStyle );

	// A "skin" entry means "use this index for all of our teams, no matter how many we have".
	int iCommonSkin = pKVStyle->GetInt( "skin", kInvalidSkinKey );
	if ( iCommonSkin != kInvalidSkinKey )
	{
		for ( int i = 0; i < TEAM_VISUAL_SECTIONS; i++ )
		{
			m_iSkins[i] = iCommonSkin;
		}
	}

	int iCommonViewmodelSkin = pKVStyle->GetInt( "v_skin", kInvalidSkinKey );
	if ( iCommonViewmodelSkin != kInvalidSkinKey )
	{
		for ( int i=0; i<TEAM_VISUAL_SECTIONS; i++ )
		{
			m_iViewmodelSkins[i] = iCommonViewmodelSkin;
		}
	}

	// If we don't have a base entry, we look for a unique entry for each team. This will be
	// handled in a subclass if necessary.

	// Are we hiding additional bodygroups when this style is active?
	KeyValues *pKVHideBodygroups = pKVStyle->FindKey( "additional_hidden_bodygroups" );
	if ( pKVHideBodygroups )
	{
		FOR_EACH_SUBKEY( pKVHideBodygroups, pKVBodygroup )
		{
			m_vecAdditionalHideBodygroups.AddToTail( pKVBodygroup->GetName() );
		}
	}

	// Remaining common properties.
	m_pszName = pKVStyle->GetString( "name", "#TF_UnknownStyle" );
	m_pszBasePlayerModel = pKVStyle->GetString( "model_player", NULL );
	m_bIsSelectable = pKVStyle->GetBool( "selectable", true );
	m_pszInventoryImage = pKVStyle->GetString( "image_inventory", NULL );
	m_bUseSmokeParticleEffect = pKVStyle->GetBool( "use_smoke_particle_effect", true );

	KeyValues *pKVBodygroup = pKVStyle->FindKey( "bodygroup" );
	if ( pKVBodygroup )
	{
		m_pszBodygroupName = pKVBodygroup->GetString( "name", NULL );
		Assert( m_pszBodygroupName );
		m_iBodygroupSubmodelIndex = pKVBodygroup->GetInt( "submodel_index", -1 );
		Assert( m_iBodygroupSubmodelIndex != -1 );
	}
}

#if defined(CLIENT_DLL) || defined(GAME_DLL)
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconStyleInfo::GeneratePrecacheModelStringsForStyle( CUtlVector<const char *> *out_pVecModelStrings ) const
{
	Assert( out_pVecModelStrings );

	if ( GetBasePlayerDisplayModel() != NULL )
	{
		out_pVecModelStrings->AddToTail( GetBasePlayerDisplayModel() );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose:	Item definition initialization helpers.
//-----------------------------------------------------------------------------
static void RecursiveInheritKeyValues( KeyValues *out_pValues, KeyValues *pInstance )
{
	KeyValues *pPrevSubKey = NULL;
	for ( KeyValues * pSubKey = pInstance->GetFirstSubKey(); pSubKey != NULL; pPrevSubKey = pSubKey, pSubKey = pSubKey->GetNextKey() )
	{
		// If this assert triggers, you have an item that uses a prefab but has multiple keys with the same name
		AssertMsg2 ( !pPrevSubKey || pPrevSubKey->GetNameSymbol() != pSubKey->GetNameSymbol(),
			"Item definition \"%s\" has multiple attributes of the same name (%s) can't use prefabs", pInstance->GetName(), pSubKey->GetName() );

		KeyValues::types_t eType = pSubKey->GetDataType();
		switch ( eType )
		{
		case KeyValues::TYPE_STRING:		out_pValues->SetString( pSubKey->GetName(), pSubKey->GetString() );			break;
		case KeyValues::TYPE_INT:			out_pValues->SetInt( pSubKey->GetName(), pSubKey->GetInt() );				break;
		case KeyValues::TYPE_FLOAT:			out_pValues->SetFloat( pSubKey->GetName(), pSubKey->GetFloat() );			break;
		case KeyValues::TYPE_WSTRING:		out_pValues->SetWString( pSubKey->GetName(), pSubKey->GetWString() );		break;
		case KeyValues::TYPE_COLOR:			out_pValues->SetColor( pSubKey->GetName(), pSubKey->GetColor() ) ;			break;
		case KeyValues::TYPE_UINT64:		out_pValues->SetUint64( pSubKey->GetName(), pSubKey->GetUint64() ) ;		break;

		// "NONE" means "KeyValues"
		case KeyValues::TYPE_NONE:
		{
			// We may already have this part of the tree to stuff data into/overwrite, or we
			// may have to make a new block.
			KeyValues *pNewChild = out_pValues->FindKey( pSubKey->GetName() );
			if ( !pNewChild )
			{
				pNewChild = out_pValues->CreateNewKey();
				pNewChild->SetName( pSubKey->GetName() );
			}

			RecursiveInheritKeyValues( pNewChild, pSubKey );
			break;
		}

		case KeyValues::TYPE_PTR:
		default:
			Assert( !"Unhandled data type for KeyValues inheritance!" );
			break;
		}
	}
}

void MergeDefinitionPrefab( KeyValues *pKVWriteItem, KeyValues *pKVSourceItem )
{
	Assert( pKVWriteItem );
	Assert( pKVSourceItem );

	const char *svPrefabName = pKVSourceItem->GetString( "prefab", NULL );
	
	if ( svPrefabName )
	{
		CUtlStringList vecPrefabs;

		Q_SplitString( svPrefabName, " ", vecPrefabs );

		// Iterate backwards so adjectives get applied over the noun prefab
		// e.g. wet scared cat would apply cat first, then scared and wet.
		FOR_EACH_VEC_BACK( vecPrefabs, i )
		{
			KeyValues *pKVPrefab = GetItemSchema()->FindDefinitionPrefabByName( vecPrefabs[i] );
			AssertMsg1( pKVPrefab, "Unable to find prefab \"%s\".", vecPrefabs[i] );

			if ( pKVPrefab )
			{
				MergeDefinitionPrefab( pKVWriteItem, pKVPrefab );
			}
		}
	}

	RecursiveInheritKeyValues( pKVWriteItem, pKVSourceItem );
}

KeyValues *CEconItemSchema::FindDefinitionPrefabByName( const char *pszPrefabName ) const
{
	int iIndex = m_dictDefinitionPrefabs.Find( pszPrefabName );
	if ( m_dictDefinitionPrefabs.IsValidIndex( iIndex ) )
		return m_dictDefinitionPrefabs[iIndex];

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CEconItemSchema::FindStringTableEntry( const char *pszTableName, int iIndex ) const
{
	SchemaStringTableDict_t::IndexType_t i = m_dictStringTable.Find( pszTableName );
	if ( !m_dictStringTable.IsValidIndex( i ) )
		return NULL;

	const CUtlVector< schema_string_table_entry_t >& vec = *m_dictStringTable[i];
	FOR_EACH_VEC( vec, j )
	{
		if ( vec[j].m_iIndex == iIndex )
			return vec[j].m_pszStr;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:	Initialize the item definition
// Input:	pKVItem - The KeyValues representation of the item
//			schema - The overall item schema for this item
//			pVecErrors - An optional vector that will contain error messages if 
//				the init fails.
// Output:	True if initialization succeeded, false otherwise
//-----------------------------------------------------------------------------


#if defined( WITH_STREAMABLE_WEAPONS )
#if defined( CLIENT_DLL )
    ConVar tf_loadondemand_default("cl_loadondemand_default", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, "The default value for whether items should be delay loaded (1) or loaded now (0).");
#elif defined( GAME_DLL )
    // The server doesn't load on demand by default because it can crash sometimes when this is set. We need to run that down, but in the meantime 
    // we just have it load on demand.
    ConVar tf_loadondemand_default("sv_loadondemand_default", "0", FCVAR_ARCHIVE | FCVAR_GAMEDLL, "The default value for whether items should be delay loaded (1) or loaded now (0).");
#else
#error "Need to add support for streamable weapons to this configuration, or disable streamable weapons here."
#endif
#endif // WITH_STREAMABLE_WEAPONS

bool CEconItemDefinition::BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ )
{
	// Set standard members
	m_pKVItem = new KeyValues( pKVItem->GetName() );
	MergeDefinitionPrefab( m_pKVItem, pKVItem );
	m_bEnabled = m_pKVItem->GetBool( "enabled" );

    // initializing this one first so that it will be available for all the errors below
    m_pszDefinitionName = m_pKVItem->GetString( "name", NULL );

#if defined( WITH_STREAMABLE_WEAPONS )
    bool bGotDefault = false;
    m_bLoadOnDemand = m_pKVItem->GetBool( "loadondemand", tf_loadondemand_default.GetBool(), &bGotDefault );

    // This logging is useful for tracking down bugs that crop up because we've (possibly) swapped the default value for loadondemand.
    // But it can be removed once we're satisfied there aren't any bugs as a result of the change (when we cleanup WITH_STREAMABLE_WEAPONS).
    if (bGotDefault)
    {
        DevMsg(10, "Item %s received default value for loadondemand\n", m_pszDefinitionName);
    }
#else
    // Keep the old behavior, which is that loadondemand is defaulted to false.
    m_bLoadOnDemand = m_pKVItem->GetBool("loadondemand");
#endif

	m_nDefIndex = Q_atoi( m_pKVItem->GetName() );

	m_nPopularitySeed = m_pKVItem->GetInt( "popularity_seed", 0 );

	// Check for required fields
	SCHEMA_INIT_CHECK( 
		NULL != m_pKVItem->FindKey( "name" ), 
		"Item definition %s: Missing required field \"name\"", m_pKVItem->GetName() );

	SCHEMA_INIT_CHECK( 
		NULL != m_pKVItem->FindKey( "item_class" ), 
		"Item definition %s: Missing required field \"item_class\"", m_pKVItem->GetName() );

	// Check value ranges
	SCHEMA_INIT_CHECK( 
		m_pKVItem->GetInt( "min_ilevel" ) >= 0, 
		"Item definition %s: \"min_ilevel\" must be greater than or equal to 0", GetDefinitionName() );

	SCHEMA_INIT_CHECK( 
		m_pKVItem->GetInt( "max_ilevel" ) >= 0, 
		"Item definition %s: \"max_ilevel\" must be greater than or equal to 0", GetDefinitionName() );

	// Check for consistency

	// Get the item class
	m_pszItemClassname = m_pKVItem->GetString( "item_class", NULL );

	// Display data
	m_pszItemBaseName = m_pKVItem->GetString( "item_name", "" ); // non-NULL to ensure we can sort
	m_pszItemTypeName = m_pKVItem->GetString( "item_type_name", "" ); // non-NULL to ensure we can sort
	m_pszItemDesc = m_pKVItem->GetString( "item_description", NULL );
	m_pszInventoryModel = m_pKVItem->GetString( "model_inventory", NULL );
	m_pszInventoryImage = m_pKVItem->GetString( "image_inventory", NULL );

	const char* pOverlay = m_pKVItem->GetString( "image_inventory_overlay", NULL );
	if ( pOverlay )
	{
		m_pszInventoryOverlayImages.AddToTail( pOverlay );
	}
	pOverlay = m_pKVItem->GetString( "image_inventory_overlay2", NULL );
	if ( pOverlay )
	{
		m_pszInventoryOverlayImages.AddToTail( pOverlay );
	}

	m_iInventoryImagePosition[0] = atoi( m_pKVItem->GetString( "image_inventory_pos_x", "0" ) );
	m_iInventoryImagePosition[1] = atoi( m_pKVItem->GetString( "image_inventory_pos_y", "0" ) );
	m_iInventoryImageSize[0] = atoi( m_pKVItem->GetString( "image_inventory_size_w", "128" ) );
	m_iInventoryImageSize[1] = atoi( m_pKVItem->GetString( "image_inventory_size_h", "82" ) );
	m_iInspectPanelDistance = m_pKVItem->GetInt( "inspect_panel_dist", 70 );
	m_nVisionFilterFlags = m_pKVItem->GetInt( "vision_filter_flags", 0 );
	m_iSubType = atoi( m_pKVItem->GetString( "subtype", "0" ) );
	m_pszBaseDisplayModel = m_pKVItem->GetString( "model_player", NULL );
	m_iDefaultSkin = m_pKVItem->GetInt( "default_skin", -1 );
	m_pszWorldDisplayModel = m_pKVItem->GetString( "model_world", NULL ); // Not the ideal method. c_models are better, but this is to solve a retrofit problem with the sticky launcher.
	m_pszWorldExtraWearableModel = m_pKVItem->GetString( "extra_wearable", NULL ); 
	m_pszWorldExtraWearableViewModel = m_pKVItem->GetString( "extra_wearable_vm", NULL );
	m_pszVisionFilteredDisplayModel = pKVItem->GetString( "model_vision_filtered", NULL );
	m_pszBrassModelOverride = m_pKVItem->GetString( "brass_eject_model", NULL );
	m_bHideBodyGroupsDeployedOnly = m_pKVItem->GetBool( "hide_bodygroups_deployed_only" );
	m_bAttachToHands = m_pKVItem->GetInt( "attach_to_hands", 0 ) != 0;
	m_bAttachToHandsVMOnly = m_pKVItem->GetInt( "attach_to_hands_vm_only", 0 ) != 0;
	m_bProperName = m_pKVItem->GetInt( "propername", 0 ) != 0;
	m_bFlipViewModel = m_pKVItem->GetInt( "flip_viewmodel", 0 ) != 0;
	m_bActAsWearable = m_pKVItem->GetInt( "act_as_wearable", 0 ) != 0;
	m_bActAsWeapon = m_pKVItem->GetInt( "act_as_weapon", 0 ) != 0;
	m_iDropType = StringFieldToInt( m_pKVItem->GetString("drop_type"), g_szDropTypeStrings, ARRAYSIZE(g_szDropTypeStrings) );

	// Creation data
	m_bHidden = m_pKVItem->GetInt( "hidden", 0 ) != 0;
	m_bBaseItem = m_pKVItem->GetInt( "baseitem", 0 ) != 0;
	m_pszItemLogClassname = m_pKVItem->GetString( "item_logname", NULL );
	m_pszItemIconClassname = m_pKVItem->GetString( "item_iconname", NULL );
	m_pszDatabaseAuditTable = m_pKVItem->GetString( "database_audit_table", NULL );
	m_bImported = m_pKVItem->FindKey( "import_from" ) != NULL;

	// capabilities
	m_iCapabilities = (item_capabilities_t)ITEM_CAP_DEFAULT;
	KeyValues *pCapsKV = m_pKVItem->FindKey( "capabilities" );
	if ( pCapsKV )
	{
		KeyValues *pEntry = pCapsKV->GetFirstSubKey();
		while ( pEntry )
		{
			ParseCapability( m_iCapabilities, pEntry );
			pEntry = pEntry->GetNextKey();
		}
	}

	// item_set
	SCHEMA_INIT_CHECK( (!m_pKVItem->GetString( "item_set", NULL )), "Item definition '%s' specifies deprecated \"item_set\" field. Items sets are now specified only in the set itself, not on the definition.", GetDefinitionName() );

	m_pszBaseFunctionalItemName = m_pKVItem->GetString( "base_item_name", "" );
	m_pszParticleSuffix = m_pKVItem->GetString( "particle_suffix", NULL );

	m_pszRemappedDefItemName = m_pKVItem->GetString( "remapped_item_def_index", NULL );

	// Init our visuals blocks.
	BInitVisualBlockFromKV( m_pKVItem, pVecErrors );

	// Calculate our equip region mask.
	{
		m_unEquipRegionMask = 0;
		m_unEquipRegionConflictMask = 0;

		// Our equip region will come from one of two places -- either we have an "equip_regions" (plural) section,
		// in which case we have any number of regions specified; or we have an "equip_region" (singular) section
		// which will have one and exactly one region. If we have "equip_regions" (plural), we ignore whatever is
		// in "equip_region" (singular).
		//
		// Yes, this is sort of dumb.
		CUtlVector<const char *> vecEquipRegionNames;

		KeyValues *pKVMultiEquipRegions = m_pKVItem->FindKey( "equip_regions" ),
				  *pKVSingleEquipRegion = m_pKVItem->FindKey( "equip_region" );

		// Maybe we have multiple entries?
		if ( pKVMultiEquipRegions )
		{
			for ( KeyValues *pKVRegion = pKVMultiEquipRegions->GetFirstSubKey(); pKVRegion; pKVRegion = pKVRegion->GetNextKey() )
			{
				vecEquipRegionNames.AddToTail( pKVRegion->GetName() );
			}
		}
		// This is our one-and-only-one equip region.
		else if ( pKVSingleEquipRegion )
		{
			const char *pEquipRegionName = pKVSingleEquipRegion->GetString( (const char *)NULL, NULL );
			if ( pEquipRegionName )
			{
				vecEquipRegionNames.AddToTail( pEquipRegionName );
			}
		}

		// For each of our regions, add to our conflict mask both ourself and all the regions
		// that we conflict with.
		FOR_EACH_VEC( vecEquipRegionNames, i )
		{
			const char *pszEquipRegionName = vecEquipRegionNames[i];
			equip_region_mask_t unThisRegionMask = GetItemSchema()->GetEquipRegionMaskByName( pszEquipRegionName );

			SCHEMA_INIT_CHECK(
				unThisRegionMask != 0,
				"Item definition %s: Unable to find equip region mask for region named \"%s\"", GetDefinitionName(), vecEquipRegionNames[i] );

			m_unEquipRegionMask |= GetItemSchema()->GetEquipRegionBitMaskByName( pszEquipRegionName );
			m_unEquipRegionConflictMask |= unThisRegionMask;
		}
	}

	// Single-line static attribute parsing.
	{
		KeyValues *pKVStaticAttrsKey = m_pKVItem->FindKey( "static_attrs" );
		if ( pKVStaticAttrsKey )
		{
			FOR_EACH_SUBKEY( pKVStaticAttrsKey, pKVKey )
			{
				static_attrib_t staticAttrib;

				SCHEMA_INIT_SUBSTEP( staticAttrib.BInitFromKV_SingleLine( GetDefinitionName(), pKVKey, pVecErrors, false ) );
				m_vecStaticAttributes.AddToTail( staticAttrib );

				// Does this attribute specify a tag to apply to this item definition?
				Assert( staticAttrib.GetAttributeDefinition() );
			}
		}
	}

	// Old style attribute parsing. Really only useful now for GC-generated attributes.
	KeyValues *pKVAttribKey = m_pKVItem->FindKey( "attributes" );
	if ( pKVAttribKey )
	{
		FOR_EACH_SUBKEY( pKVAttribKey, pKVKey )
		{
			static_attrib_t staticAttrib;

			SCHEMA_INIT_SUBSTEP( staticAttrib.BInitFromKV_MultiLine( GetDefinitionName(), pKVKey, pVecErrors ) );
			m_vecStaticAttributes.AddToTail( staticAttrib );

			// Does this attribute specify a tag to apply to this item definition?
			Assert( staticAttrib.GetAttributeDefinition() );
		}
	}

	// Initialize tags based on all static attributes for this item.
	for ( const static_attrib_t& attr : m_vecStaticAttributes )
	{
		const econ_tag_handle_t tag = attr.GetAttributeDefinition()->GetItemDefinitionTag();
		if ( tag != INVALID_ECON_TAG_HANDLE )
		{
			m_vecTags.AddToTail( tag );
		}
	}

	// Auto-generate tags based on capabilities.
	for ( int i = 0; i < NUM_ITEM_CAPS; i++ )
	{
		if ( m_iCapabilities & (1 << i) )
		{
			m_vecTags.AddToTail( GetItemSchema()->GetHandleForTag( CFmtStr( "auto__cap_%s", g_Capabilities[i] ).Get() ) );
		}
	}

	// Initialize used-specified tags for this item if present.
	KeyValues *pKVTags = m_pKVItem->FindKey( "tags" );
	if ( pKVTags )
	{
		FOR_EACH_SUBKEY( pKVTags, pKVTag )
		{
			m_vecTags.AddToTail( GetItemSchema()->GetHandleForTag( pKVTag->GetName() ) );
		}
	}

	return SCHEMA_INIT_SUCCESS();
}

bool CEconItemDefinition::BPostInit( CUtlVector<CUtlString> *pVecErrors /*= NULL*/ )
{
	if ( m_pszRemappedDefItemName )
	{
		const CEconItemDefinition *pDef = GetItemSchema()->GetItemDefinitionByName( m_pszRemappedDefItemName );
		SCHEMA_INIT_CHECK( pDef != NULL, "Can't find remapped item %s", m_pszRemappedDefItemName );

		m_nRemappedDefIndex = pDef->GetDefinitionIndex();
	}

	return SCHEMA_INIT_SUCCESS();
}


bool static_attrib_t::BInitFromKV_MultiLine( const char *pszContext, KeyValues *pKVAttribute, CUtlVector<CUtlString> *pVecErrors )
{
	const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinitionByName( pKVAttribute->GetName() );

	SCHEMA_INIT_CHECK( 
		NULL != pAttrDef,
		"Context '%s': Attribute \"%s\" in \"attributes\" did not match any attribute definitions", pszContext, pKVAttribute->GetName() );

	if ( pAttrDef )
	{
		iDefIndex = pAttrDef->GetDefinitionIndex();
			
		const ISchemaAttributeType *pAttrType = pAttrDef->GetAttributeType();
		Assert( pAttrType );

		pAttrType->InitializeNewEconAttributeValue( &m_value );

		const char *pszValue = pKVAttribute->GetString( "value", NULL );
		const bool bSuccessfullyLoadedValue = pAttrType->BConvertStringToEconAttributeValue( pAttrDef, pszValue, &m_value, true );

		SCHEMA_INIT_CHECK(
			bSuccessfullyLoadedValue,
			"Context '%s': Attribute \"%s\" could not parse value \"%s\"!", pszContext, pKVAttribute->GetName(), pszValue ? pszValue : "(null)" );

		SCHEMA_INIT_CHECK(
			!pAttrDef->BIsSetBonusAttribute(),
			"Context '%s': Attribute \"%s\" is a set bonus attribute and not supported here", pszContext, pKVAttribute->GetName() );
	}

	return SCHEMA_INIT_SUCCESS();
}

bool static_attrib_t::BInitFromKV_SingleLine( const char *pszContext, KeyValues *pKVAttribute, CUtlVector<CUtlString> *pVecErrors, bool bEnableTerribleBackwardsCompatibilitySchemaParsingCode /* = true */ )
{
	const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinitionByName( pKVAttribute->GetName() );

	SCHEMA_INIT_CHECK( 
		NULL != pAttrDef,
		"Context '%s': Attribute \"%s\" in \"attributes\" did not match any attribute definitions", pszContext, pKVAttribute->GetName() );

	if ( pAttrDef )
	{
		iDefIndex = pAttrDef->GetDefinitionIndex();
			
		const ISchemaAttributeType *pAttrType = pAttrDef->GetAttributeType();
		Assert( pAttrType );

		pAttrType->InitializeNewEconAttributeValue( &m_value );

		const char *pszValue = pKVAttribute->GetString();
		const bool bSuccessfullyLoadedValue = pAttrType->BConvertStringToEconAttributeValue( pAttrDef, pszValue, &m_value, bEnableTerribleBackwardsCompatibilitySchemaParsingCode );

		SCHEMA_INIT_CHECK(
			bSuccessfullyLoadedValue,
			"Context '%s': Attribute \"%s\" could not parse value \"%s\"!", pszContext, pKVAttribute->GetName(), pszValue ? pszValue : "(null)" );

		SCHEMA_INIT_CHECK(
			!pAttrDef->BIsSetBonusAttribute(),
			"Context '%s': Attribute \"%s\" is a set bonus attribute and not supported here", pszContext, pKVAttribute->GetName() );

	}

	return SCHEMA_INIT_SUCCESS();
}

const char* CEconItemDefinition::GetIconURL( const char* pszKey ) const
{
	auto idx = m_pDictIcons->Find( pszKey );
	if ( idx == m_pDictIcons->InvalidIndex() )
	{
		return NULL;
	}

	return (*m_pDictIcons)[ idx ];
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconItemDefinition::IterateAttributes( IEconItemAttributeIterator *pIterator ) const
{
	FOR_EACH_VEC( GetStaticAttributes(), i )
	{
		const static_attrib_t& staticAttrib = GetStaticAttributes()[i];
		

		const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinition( staticAttrib.iDefIndex );
		if ( !pAttrDef )
			continue;

		const ISchemaAttributeType *pAttrType = pAttrDef->GetAttributeType();
		Assert( pAttrType );

		if ( !pAttrType->OnIterateAttributeValue( pIterator, pAttrDef, staticAttrib.m_value ) )
			return;
	}
}

#if defined(CLIENT_DLL) || defined(GAME_DLL)
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Activity CEconItemDefinition::GetActivityOverride( int iTeam, Activity baseAct ) const
{
	int iAnims = GetNumAnimations( iTeam );
	for ( int i = 0; i < iAnims; i++ )
	{
		animation_on_wearable_t *pData = GetAnimationData( iTeam, i );
		if ( !pData )
			continue;
		if ( pData->iActivity == kActivityLookup_Unknown )
		{
			pData->iActivity = ActivityList_IndexForName( pData->pszActivity );
		}

		if ( pData->iActivity == baseAct )
		{
			if ( pData->iReplacement == kActivityLookup_Unknown )
			{
				pData->iReplacement = ActivityList_IndexForName( pData->pszReplacement );
			}

			if ( pData->iReplacement > 0 )
			{
				return (Activity) pData->iReplacement;
			}
		}
	}

	return baseAct;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CEconItemDefinition::GetActivityOverride( int iTeam, const char *pszActivity ) const
{
	int iAnims = GetNumAnimations( iTeam );
	for ( int i = 0; i < iAnims; i++ )
	{
		animation_on_wearable_t *pData = GetAnimationData( iTeam, i );
		if ( Q_stricmp( pszActivity, pData->pszActivity ) == 0 )
			return pData->pszReplacement;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CEconItemDefinition::GetReplacementForActivityOverride( int iTeam, Activity baseAct ) const
{
	int iAnims = GetNumAnimations( iTeam );
	for ( int i = 0; i < iAnims; i++ )
	{
		animation_on_wearable_t *pData = GetAnimationData( iTeam, i );
		if ( pData->iActivity == kActivityLookup_Unknown )
		{
			pData->iActivity = ActivityList_IndexForName( pData->pszActivity );
		}
		if ( pData && pData->iActivity == baseAct )
			return pData->pszReplacement;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the content for this item view should be streamed. If false,
//			it should be preloaded.
//-----------------------------------------------------------------------------

// DO NOT MERGE THIS CONSOLE VARIABLE TO REL WE SHOULD NOT SHIP THIS OH GOD

bool CEconItemDefinition::IsContentStreamable() const
{
	if ( !BLoadOnDemand() )
		return false;
		
	return true;
}
#endif // defined(CLIENT_DLL) || defined(GAME_DLL)

RETURN_ATTRIBUTE_STRING_F( CEconItemDefinition::GetIconDisplayModel, "icon display model", m_pszWorldDisplayModel );

//-----------------------------------------------------------------------------
// Purpose: Adds a foreign item definition to local definition mapping for a 
//			foreign app
//-----------------------------------------------------------------------------
void CForeignAppImports::AddMapping( uint16 unForeignDefIndex, const CEconItemDefinition *pDefn )
{
	m_mapDefinitions.InsertOrReplace( unForeignDefIndex, pDefn );
}


//-----------------------------------------------------------------------------
// Purpose: Adds a foreign item definition to local definition mapping for a 
//			foreign app
//-----------------------------------------------------------------------------
const CEconItemDefinition *CForeignAppImports::FindMapping( uint16 unForeignDefIndex ) const
{
	int i = m_mapDefinitions.Find( unForeignDefIndex );
	if( m_mapDefinitions.IsValidIndex( i ) )
		return m_mapDefinitions[i];
	else
		return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:	Constructor
//-----------------------------------------------------------------------------
CEconItemSchema::CEconItemSchema( )
: 	m_unResetCount( 0 )
,	m_pKVRawDefinition( NULL )
,	m_mapAttributes( DefLessFunc(int) )
,	m_mapItemsSorted( DefLessFunc(int) )
,	m_mapBaseItems( DefLessFunc(int) )
,	m_unVersion( 0 )
#if defined(CLIENT_DLL) || defined(GAME_DLL)
,	m_pDefaultItemDefinition( NULL )
#endif
,	m_dictDefinitionPrefabs( k_eDictCompareTypeCaseInsensitive )
,	m_mapAttributeControlledParticleSystems( DefLessFunc(int) )
,	m_dictDefaultBodygroupState( k_eDictCompareTypeCaseInsensitive )
#if   defined(CLIENT_DLL) || defined(GAME_DLL)
,	m_pDelayedSchemaData( NULL )
#endif
{
	Reset();
}

CItemSelectionCriteria *CEconItemSchema::CreateItemCriteria( const char *pszContext, KeyValues *pItemCriteriaKV, CUtlVector<CUtlString> *pVecErrors /*= NULL*/ )
{
	CItemSelectionCriteria *pCriteria = new CItemSelectionCriteria;
	if ( pCriteria->BInitFromKV( pItemCriteriaKV ) )
	{
		return pCriteria;
	}

	delete pCriteria;
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:	Resets the schema to before BInit was called
//-----------------------------------------------------------------------------
void CEconItemSchema::Reset( void )
{
	++m_unResetCount;

	m_unFirstValidClass = 0;
	m_unLastValidClass = 0;
	m_unAccoutClassIndex = 0;
	m_unFirstValidClassItemSlot = 0;
	m_unLastValidClassItemSlot = 0;
	m_unFirstValidAccountItemSlot = 0;
	m_unLastValidAccountItemSlot = 0;
	m_unNumItemPresets = 0;
	m_unVersion = 0;
	m_unSumQualityWeights = 0;
	FOR_EACH_VEC( m_vecAttributeTypes, i )
	{
		delete m_vecAttributeTypes[i].m_pAttrType;
	}
	m_vecAttributeTypes.Purge();
	m_mapItems.PurgeAndDeleteElements();
	m_mapItems.Purge();
	m_mapItemsSorted.Purge();
	m_mapBaseItems.Purge();
	m_mapAttributeControlledParticleSystems.Purge();
	m_vecAttributeControlledParticleSystemsCosmetics.Purge();
	m_vecAttributeControlledParticleSystemsWeapons.Purge();
	m_vecAttributeControlledParticleSystemsTaunts.Purge();

	m_mapAttributes.Purge();
	if ( m_pKVRawDefinition )
	{
		m_pKVRawDefinition->deleteThis();
		m_pKVRawDefinition = NULL;
	}

#if defined(CLIENT_DLL) || defined(GAME_DLL)
	delete m_pDefaultItemDefinition;
	m_pDefaultItemDefinition = NULL;
#endif

	for ( int idx = m_dictDefinitionPrefabs.First(); m_dictDefinitionPrefabs.IsValidIndex( idx ); idx = m_dictDefinitionPrefabs.Next( idx ) )
	{
		m_dictDefinitionPrefabs[idx]->deleteThis();
	}
	m_dictDefinitionPrefabs.Purge();

	m_vecEquipRegionsList.Purge();

	m_dictStringTable.PurgeAndDeleteElements();
}


//-----------------------------------------------------------------------------
// Purpose:	Operator=
//-----------------------------------------------------------------------------
CEconItemSchema &CEconItemSchema::operator=( CEconItemSchema &rhs )
{
	Reset();
	BInitSchema( rhs.m_pKVRawDefinition );
	return *this;
}

//-----------------------------------------------------------------------------
// Initializes the schema, given KV filename
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInit( const char *fileName, const char *pathID, CUtlVector<CUtlString> *pVecErrors /* = NULL */)
{
	Reset();

	// Read the raw data
	CUtlBuffer bufRawData;
	bool bReadFileOK = g_pFullFileSystem->ReadFile( fileName, pathID, bufRawData );
	SCHEMA_INIT_CHECK( bReadFileOK, "Cannot load file '%s'", fileName );

	// Do we need to check the signature?
	#if defined(TF_DLL) || defined(TF_CLIENT_DLL)
	{
		bool bSignatureValid = TF_CheckSignature(fileName, pathID, bufRawData);
		SCHEMA_INIT_CHECK(bSignatureValid, "'%s' is corrupt.  Please verify your local game files.  (https://support.steampowered.com/kb_article.php?ref=2037-QEUH-3335)", fileName);
		
	}
	#endif

	// Compute version hash
	CSHA1 sha1;
	sha1.Update( (unsigned char *)bufRawData.Base(), bufRawData.Size() );
	sha1.Final();
	sha1.GetHash( m_schemaSHA.m_shaDigest );

	// Wrap it with a text buffer reader
	CUtlBuffer bufText( bufRawData.Base(), bufRawData.TellPut(), CUtlBuffer::READ_ONLY | CUtlBuffer::TEXT_BUFFER );

	// Use the standard init path
	return BInitTextBuffer( bufText, pVecErrors );
}

//-----------------------------------------------------------------------------
// Initializes the schema, given KV in binary form
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitBinaryBuffer( CUtlBuffer &buffer, CUtlVector<CUtlString> *pVecErrors /* = NULL */ )
{
	Reset();
	m_pKVRawDefinition = new KeyValues( "CEconItemSchema" );
	if ( m_pKVRawDefinition->ReadAsBinary( buffer ) )
	{
		return BInitSchema( m_pKVRawDefinition, pVecErrors )
			&& BPostSchemaInit( pVecErrors );
	}
	if ( pVecErrors )
	{
		pVecErrors->AddToTail( "Error parsing keyvalues" );
	}
	return false;
}

unsigned char g_sha1ItemSchemaText[ k_cubHash ];

//-----------------------------------------------------------------------------
// Initializes the schema, given KV in text form
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitTextBuffer( CUtlBuffer &buffer, CUtlVector<CUtlString> *pVecErrors /* = NULL */ )
{
	// Save off the hash into a global variable, so VAC can check it
	// later
	GenerateHash( g_sha1ItemSchemaText, buffer.Base(), buffer.TellPut() );

	Reset();
	m_pKVRawDefinition = new KeyValues( "CEconItemSchema" );
	if ( m_pKVRawDefinition->LoadFromBuffer( NULL, buffer ) )
	{
		return BInitSchema( m_pKVRawDefinition, pVecErrors )
			&& BPostSchemaInit( pVecErrors );
	}
	if ( pVecErrors )
	{
		pVecErrors->AddToTail( "Error parsing keyvalues" );
	}
	return false;
}

bool CEconItemSchema::DumpItems ( const char *fileName, const char *pathID )
{
	// create a write file
	FileHandle_t f = g_pFullFileSystem->Open(fileName, "wb", pathID);

	if ( f == FILESYSTEM_INVALID_HANDLE )
	{
		DevMsg(1, "CEconItemSchema::DumpItems: couldn't open file \"%s\" in path \"%s\".\n", 
			fileName?fileName:"NULL", pathID?pathID:"NULL" );
		return false;
	}

	CUtlSortVector< KeyValues*, CUtlSortVectorKeyValuesByName > vecSortedItems;

	FOR_EACH_MAP_FAST( m_mapItems, i )
	{
		vecSortedItems.InsertNoSort( m_mapItems[ i ]->GetRawDefinition() );
	}
	vecSortedItems.RedoSort();

	CUtlBuffer buf;
	FOR_EACH_VEC( vecSortedItems, i )
	{
		vecSortedItems[i]->RecursiveSaveToFile( buf, 0, true );
	}

	int iBufSize = buf.GetBytesRemaining();
	bool bSuccess = false;
	if ( g_pFullFileSystem->Write(buf.PeekGet(), iBufSize, f) == iBufSize )
		bSuccess = true;

	g_pFullFileSystem->Close(f);

	return bSuccess;
}

//-----------------------------------------------------------------------------
// Called once the price sheet's been loaded
//-----------------------------------------------------------------------------


#if defined(CLIENT_DLL) || defined(GAME_DLL)
//-----------------------------------------------------------------------------
// Set up the buffer to use to reinitialize our schema next time we can do so safely.
//-----------------------------------------------------------------------------
bool CEconItemSchema::MaybeInitFromBuffer( IDelayedSchemaData *pDelayedSchemaData )
{
	bool bDidInit = false;

	// Use whatever our most current data block is.
	if ( m_pDelayedSchemaData )
	{
		delete m_pDelayedSchemaData;
	}

	m_pDelayedSchemaData = pDelayedSchemaData;

#ifdef CLIENT_DLL
	// If we aren't in a game we can parse immediately now.
	if ( !engine->IsInGame() )
	{
		BInitFromDelayedBuffer();
		bDidInit = true;
	}
#endif // CLIENT_DLL

	return bDidInit;
}

//-----------------------------------------------------------------------------
// We're in a safe place to change the contents of the schema, so do so and clean
// up whatever memory we were using.
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitFromDelayedBuffer()
{
	if ( !m_pDelayedSchemaData )
		return true;

	bool bSuccess = m_pDelayedSchemaData->InitializeSchema( this );
	delete m_pDelayedSchemaData;
	m_pDelayedSchemaData = NULL;
	 
	// We just got a new schema.  We need another PostInit()
	ItemSystem()->PostInit();

	return bSuccess;
}
#endif // !GC_DLL

static void CalculateKeyValuesCRCRecursive( KeyValues *pKV, CRC32_t *crc, bool bIgnoreName = false )
{
	// Hash in the key name in LOWERCASE.  Keyvalues files are not deterministic due
	// to the case insensitivity of the keys and the dependence on the existing
	// state of the name table upon entry.
	if ( !bIgnoreName )
	{
		const char *s = pKV->GetName();  
		for (;;)
		{
			unsigned char x = tolower(*s);
			CRC32_ProcessBuffer( crc, &x, 1 ); // !SPEED! This is slow, but it works.
			if (*s == '\0') break;
			++s;
		}
	}

	// Now hash in value, depending on type
	// !FIXME! This is not byte-order independent!
	switch ( pKV->GetDataType() )
	{
	case KeyValues::TYPE_NONE:
		{
			FOR_EACH_SUBKEY( pKV, pChild )
			{
				CalculateKeyValuesCRCRecursive( pChild, crc );
			}
			break;
		}
	case KeyValues::TYPE_STRING:
		{
			const char *val = pKV->GetString();
			CRC32_ProcessBuffer( crc, val, strlen(val)+1 );
			break;
		}

	case KeyValues::TYPE_INT:
		{
			int val = pKV->GetInt();
			CRC32_ProcessBuffer( crc, &val, sizeof(val) );
			break;
		}

	case KeyValues::TYPE_UINT64:
		{
			uint64 val = pKV->GetUint64();
			CRC32_ProcessBuffer( crc, &val, sizeof(val) );
			break;
		}

	case KeyValues::TYPE_FLOAT:
		{
			float val = pKV->GetFloat();
			CRC32_ProcessBuffer( crc, &val, sizeof(val) );
			break;
		}
	case KeyValues::TYPE_COLOR:
		{
			int val = pKV->GetColor().GetRawColor();
			CRC32_ProcessBuffer( crc, &val, sizeof(val) );
			break;
		}

	default:
	case KeyValues::TYPE_PTR:
	case KeyValues::TYPE_WSTRING:
		{
			Assert( !"Unsupport data type!" );
			break;
		}
	}
}

uint32 CEconItemSchema::CalculateKeyValuesVersion( KeyValues *pKV )
{
	CRC32_t crc;
	CRC32_Init( &crc );

	// Calc CRC recursively.  Ignore the very top-most
	// key name, which isn't set consistently
	CalculateKeyValuesCRCRecursive( pKV, &crc, true );
	CRC32_Final( &crc );
	return crc;
}

EEquipType_t CEconItemSchema::GetEquipTypeFromClassIndex( int iClass ) const
{
	if ( iClass == GetAccountIndex() )
		return EEquipType_t::EQUIP_TYPE_ACCOUNT;

	if ( iClass >= GetFirstValidClass() && iClass <= GetLastValidClass() )
		return EEquipType_t::EQUIP_TYPE_CLASS;

	return EEquipType_t::EQUIP_TYPE_INVALID;
}

//-----------------------------------------------------------------------------
// Purpose:	Initializes the schema
// Input:	pKVRawDefinition - The raw KeyValues representation of the schema
//			pVecErrors - An optional vector that will contain error messages if 
//				the init fails.
// Output:	True if initialization succeeded, false otherwise
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitSchema( KeyValues *pKVRawDefinition, CUtlVector<CUtlString> *pVecErrors /* = NULL */ )
{
	double flInitSchemaTime = Plat_FloatTime();

	m_unVersion = CalculateKeyValuesVersion( pKVRawDefinition );

	// Parse the prefabs block first so the prefabs will be populated in case anything else wants
	// to use them later.
	KeyValues *pKVPrefabs = pKVRawDefinition->FindKey( "prefabs" );
	if ( NULL != pKVPrefabs )
	{
		SCHEMA_INIT_SUBSTEP( BInitDefinitionPrefabs( pKVPrefabs, pVecErrors ) );
	}

	// Initialize the game info block
	KeyValues *pKVGameInfo = pKVRawDefinition->FindKey( "game_info" );
	SCHEMA_INIT_CHECK( NULL != pKVGameInfo, "Required key \"game_info\" missing.\n" );

	if ( NULL != pKVGameInfo )
	{
		SCHEMA_INIT_SUBSTEP( BInitGameInfo( pKVGameInfo, pVecErrors ) );
	}

	// Initialize our attribute types. We don't actually pull this data from the schema right now but it
	// still makes sense to initialize it at this point.
	SCHEMA_INIT_SUBSTEP( BInitAttributeTypes( pVecErrors ) );

	// Initialize the colors block
	KeyValues *pKVColors = pKVRawDefinition->FindKey( "colors" );
	SCHEMA_INIT_CHECK( NULL != pKVColors, "Required key \"colors\" missing.\n" );

	if ( NULL != pKVColors )
	{
		SCHEMA_INIT_SUBSTEP( BInitColors( pKVColors, pVecErrors ) );
	}

	// Initialize the attributes block
	KeyValues *pKVAttributes = pKVRawDefinition->FindKey( "attributes" );
	SCHEMA_INIT_CHECK( NULL != pKVAttributes, "Required key \"attributes\" missing.\n" );

	if ( NULL != pKVAttributes )
	{
		SCHEMA_INIT_SUBSTEP( BInitAttributes( pKVAttributes, pVecErrors ) );
	}


	// Initialize the "equip_regions_list" block -- this is an optional block
	KeyValues *pKVEquipRegions = pKVRawDefinition->FindKey( "equip_regions_list" );
	if ( NULL != pKVEquipRegions )
	{
		SCHEMA_INIT_SUBSTEP( BInitEquipRegions( pKVEquipRegions, pVecErrors ) );
	}

	// Initialize the "equip_conflicts" block -- this is an optional block, though it doesn't
	// make any sense and will probably fail internally if there is no corresponding "equip_regions"
	// block as well
	KeyValues *pKVEquipRegionConflicts = pKVRawDefinition->FindKey( "equip_conflicts" );
	if ( NULL != pKVEquipRegionConflicts )
	{
		SCHEMA_INIT_SUBSTEP( BInitEquipRegionConflicts( pKVEquipRegionConflicts, pVecErrors ) );
	}

	// Initialize the items block
	KeyValues *pKVItems = pKVRawDefinition->FindKey( "items" );
	SCHEMA_INIT_CHECK( NULL != pKVItems, "Required key \"items\" missing.\n" );

	if ( NULL != pKVItems )
	{
		SCHEMA_INIT_SUBSTEP( BInitItems( pKVItems, pVecErrors ) );
	}

	// Verify base item names are proper in item schema
	SCHEMA_INIT_SUBSTEP( BVerifyBaseItemNames( pVecErrors ) );

	// Particles
	KeyValues *pKVParticleSystems = pKVRawDefinition->FindKey( "attribute_controlled_attached_particles" );
	SCHEMA_INIT_SUBSTEP( BInitAttributeControlledParticleSystems( pKVParticleSystems, pVecErrors ) );

	// Initialize the string tables, if present
	KeyValues *pKVStringTables = pKVRawDefinition->FindKey( "string_lookups" );
	SCHEMA_INIT_SUBSTEP( BInitStringTables( pKVStringTables, pVecErrors ) );

	double flTotalTime = Plat_FloatTime() - flInitSchemaTime;

#ifdef GAME_DLL
	DevMsg( "*********Server InitSchema time = %f\n", flTotalTime );
#elif CLIENT_DLL
	DevMsg( "*********Client InitSchema time = %f\n", flTotalTime );
#else // GC_DLL
	DevMsg( "*********GC InitSchema time = %f\n", flTotalTime );
#endif

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:	Initializes the "game_info" section of the schema
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitGameInfo( KeyValues *pKVGameInfo, CUtlVector<CUtlString> *pVecErrors )
{
	m_unFirstValidClass = pKVGameInfo->GetInt( "first_valid_class", 0 );
	m_unLastValidClass = pKVGameInfo->GetInt( "last_valid_class", 0 );
	SCHEMA_INIT_CHECK( 0 < m_unFirstValidClass, "First valid class must be greater than 0." );
	SCHEMA_INIT_CHECK( m_unFirstValidClass <= m_unLastValidClass, "First valid class must be less than or equal to last valid class." );
	m_unAccoutClassIndex = pKVGameInfo->GetInt( "account_class_index", 0 );
	SCHEMA_INIT_CHECK( m_unAccoutClassIndex > m_unLastValidClass, "Account class index must be greater than 'last_valid_class'" );

	m_unFirstValidClassItemSlot = pKVGameInfo->GetInt( "first_valid_item_slot", INVALID_EQUIPPED_SLOT );
	m_unLastValidClassItemSlot = pKVGameInfo->GetInt( "last_valid_item_slot", INVALID_EQUIPPED_SLOT );
	SCHEMA_INIT_CHECK( INVALID_EQUIPPED_SLOT != m_unFirstValidClassItemSlot, "first_valid_item_slot not set!" );
	SCHEMA_INIT_CHECK( INVALID_EQUIPPED_SLOT != m_unFirstValidClassItemSlot, "last_valid_item_slot not set!" );
	SCHEMA_INIT_CHECK( m_unFirstValidClassItemSlot <= m_unLastValidClassItemSlot, "First valid item slot must be less than or equal to last valid item slot." );

	m_unFirstValidAccountItemSlot = pKVGameInfo->GetInt( "account_first_valid_item_slot", INVALID_EQUIPPED_SLOT );
	m_unLastValidAccountItemSlot  = pKVGameInfo->GetInt( "account_last_valid_item_slot", INVALID_EQUIPPED_SLOT );
	SCHEMA_INIT_CHECK( INVALID_EQUIPPED_SLOT != m_unFirstValidAccountItemSlot, "account_first_valid_item_slot not set!" );
	SCHEMA_INIT_CHECK( INVALID_EQUIPPED_SLOT != m_unLastValidAccountItemSlot, "account_last_valid_item_slot not set!" );
	SCHEMA_INIT_CHECK( m_unFirstValidAccountItemSlot <= m_unLastValidAccountItemSlot, "First vlid account item slot must be less than or equal to the last valid account item slot." );

	m_unNumItemPresets = pKVGameInfo->GetInt( "num_item_presets", -1 );
	SCHEMA_INIT_CHECK( (uint32)-1 != m_unNumItemPresets, "num_item_presets not set!" );

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitAttributeTypes( CUtlVector<CUtlString> *pVecErrors )
{
	FOR_EACH_VEC( m_vecAttributeTypes, i )
	{
		delete m_vecAttributeTypes[i].m_pAttrType;
	}
	m_vecAttributeTypes.Purge();

	m_vecAttributeTypes.AddToTail( attr_type_t( NULL,										new CSchemaAttributeType_Default ) );
	m_vecAttributeTypes.AddToTail( attr_type_t( "float",									new CSchemaAttributeType_Float ) );
	m_vecAttributeTypes.AddToTail( attr_type_t( "uint64",									new CSchemaAttributeType_UInt64 ) );
	m_vecAttributeTypes.AddToTail( attr_type_t( "string",									new CSchemaAttributeType_String ) );
	m_vecAttributeTypes.AddToTail( attr_type_t( "dynamic_recipe_component_defined_item",	new CSchemaAttributeType_DynamicRecipeComponentDefinedItem ) );
	m_vecAttributeTypes.AddToTail( attr_type_t( "item_slot_criteria",						new CSchemaAttributeType_ItemSlotCriteria ) );
	m_vecAttributeTypes.AddToTail( attr_type_t( "item_placement",							new CSchemaAttributeType_WorldItemPlacement ) );

	// Make sure that all attribute types specified have the item ID in the 0th column. We use this
	// when loading items to map between item IDs and the attributes they own.
	FOR_EACH_VEC( m_vecAttributeTypes, i )
	{
	}

	return SCHEMA_INIT_SUCCESS();
}


//-----------------------------------------------------------------------------
// Purpose:	Initializes the "prefabs" section of the schema
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitDefinitionPrefabs( KeyValues *pKVPrefabs, CUtlVector<CUtlString> *pVecErrors )
{
	FOR_EACH_TRUE_SUBKEY( pKVPrefabs, pKVPrefab )
	{
		const char *pszPrefabName = pKVPrefab->GetName();

		int nMapIndex = m_dictDefinitionPrefabs.Find( pszPrefabName );

		// Make sure the item index is correct because we use this index as a reference
		SCHEMA_INIT_CHECK( 
			!m_dictDefinitionPrefabs.IsValidIndex( nMapIndex ),
			"Duplicate prefab name (%s)", pszPrefabName );

		m_dictDefinitionPrefabs.Insert( pszPrefabName, pKVPrefab->MakeCopy() );
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitColors( KeyValues *pKVColors, CUtlVector<CUtlString> *pVecErrors )
{
	// initialize the color definitions
	if ( NULL != pKVColors )
	{
		FOR_EACH_TRUE_SUBKEY( pKVColors, pKVColor )
		{
			CEconColorDefinition *pNewColorDef = new CEconColorDefinition;

			SCHEMA_INIT_SUBSTEP( pNewColorDef->BInitFromKV( pKVColor, pVecErrors ) );
			m_vecColorDefs.AddToTail( pNewColorDef );
		}
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CEconItemSchema::GetEquipRegionIndexByName( const char *pRegionName ) const
{
	FOR_EACH_VEC( m_vecEquipRegionsList, i )
	{
		const char *szEntryRegionName = m_vecEquipRegionsList[i].m_sName.Get();
		if ( !V_stricmp( szEntryRegionName, pRegionName ) )
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
equip_region_mask_t CEconItemSchema::GetEquipRegionBitMaskByName( const char *pRegionName ) const
{
	int iRegionIndex = GetEquipRegionIndexByName( pRegionName );
	if ( !m_vecEquipRegionsList.IsValidIndex( iRegionIndex ) )
		return 0;

	equip_region_mask_t unRegionMask = 1 << m_vecEquipRegionsList[iRegionIndex].m_unBitIndex;
	Assert( unRegionMask > 0 );

	return unRegionMask;
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
void CEconItemSchema::SetEquipRegionConflict( int iRegion, unsigned int unBit )
{
	Assert( m_vecEquipRegionsList.IsValidIndex( iRegion ) );

	equip_region_mask_t unRegionMask = 1 << unBit;
	Assert( unRegionMask > 0 );

	m_vecEquipRegionsList[iRegion].m_unMask |= unRegionMask;
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
equip_region_mask_t CEconItemSchema::GetEquipRegionMaskByName( const char *pRegionName ) const
{
	int iRegionIdx = GetEquipRegionIndexByName( pRegionName );
	if ( iRegionIdx < 0 )
		return 0;

	return m_vecEquipRegionsList[iRegionIdx].m_unMask;
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
void CEconItemSchema::AssignDefaultBodygroupState( const char *pszBodygroupName, int iValue )
{
	// Flip the value passed in -- if we specify in the schema that a region should be off, we assume that it's
	// on by default.
	// actually the schemas are all authored assuming that the default is 0, so let's use that
	int iDefaultValue = 0; //iValue == 0 ? 1 : 0;

	// Make sure that we're constantly reinitializing our default value to the same default value. This is sort
	// of dumb but it works for everything we've got now. In the event that conflicts start cropping up it would
	// be easy enough to make a new schema section.
	int iIndex = m_dictDefaultBodygroupState.Find( pszBodygroupName );
	if ( (m_dictDefaultBodygroupState.IsValidIndex( iIndex ) && m_dictDefaultBodygroupState[iIndex] != iDefaultValue) ||
		 (iValue < 0 || iValue > 1) )
	{
		EmitWarning( SPEW_GC, 4, "Unable to get accurate read on whether bodygroup '%s' is enabled or disabled by default. (The schema is fine, but the code is confused and could stand to be made smarter.)\n", pszBodygroupName );
	}

	if ( !m_dictDefaultBodygroupState.IsValidIndex( iIndex ) )
	{
		m_dictDefaultBodygroupState.Insert( pszBodygroupName, iDefaultValue );
	}
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitEquipRegions( KeyValues *pKVEquipRegions, CUtlVector<CUtlString> *pVecErrors )
{
	CUtlVector<const char *> vecNames;

	FOR_EACH_SUBKEY( pKVEquipRegions, pKVRegion )
	{
		const char *pRegionKeyName = pKVRegion->GetName();

		vecNames.Purge();

		// The "shared" name is special for equip regions -- it means that all of the sub-regions specified
		// will use the same bit to store equipped-or-not data, but that one bit can be accessed by a whole
		// bunch of different names. This is useful in TF where different classes have different regions, but
		// those regions cannot possibly conflict with each other. For example, "scout_backpack" cannot possibly
		// overlap with "pyro_shoulder" because they can't even be equipped on the same character.
		if ( pRegionKeyName && !Q_stricmp( pRegionKeyName, "shared" ) )
		{
			FOR_EACH_SUBKEY( pKVRegion, pKVSharedRegionName )
			{
				vecNames.AddToTail( pKVSharedRegionName->GetName() );
			}
		}
		// We have a standard name -- this one entry is its own equip region.
		else
		{
			vecNames.AddToTail( pRegionKeyName );
		}

		// What bit will this equip region use to mask against conflicts? If we don't have any equip regions
		// at all, we'll use the base bit, otherwise we just grab one higher than whatever we used last.
		unsigned int unNewBitIndex = m_vecEquipRegionsList.Count() <= 0 ? 0 : m_vecEquipRegionsList.Tail().m_unBitIndex + 1;
		
		FOR_EACH_VEC( vecNames, i )
		{
			const char *pRegionName = vecNames[i];

			// Make sure this name is unique.
			if ( GetEquipRegionIndexByName( pRegionName ) >= 0 )
			{
				pVecErrors->AddToTail( CFmtStr( "Duplicate equip region named \"%s\".", pRegionName ).Access() );
				continue;
			}

			// Make a new region.
			EquipRegion newEquipRegion;
			newEquipRegion.m_sName		= pRegionName;
			newEquipRegion.m_unMask		= 0;				// we'll update this mask later
			newEquipRegion.m_unBitIndex	= unNewBitIndex;

			int iIdx = m_vecEquipRegionsList.AddToTail( newEquipRegion );

			// Tag this region to conflict with itself so that if nothing else two items in the same
			// region can't equip over each other.
			SetEquipRegionConflict( iIdx, unNewBitIndex );
		}
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitEquipRegionConflicts( KeyValues *pKVEquipRegionConflicts, CUtlVector<CUtlString> *pVecErrors )
{
	FOR_EACH_TRUE_SUBKEY( pKVEquipRegionConflicts, pKVConflict )
	{
		// What region is the base of this conflict?
		const char *pRegionName = pKVConflict->GetName();
		int iRegionIdx = GetEquipRegionIndexByName( pRegionName );
		if ( iRegionIdx < 0 )
		{
			pVecErrors->AddToTail( CFmtStr( "Unable to find base equip region named \"%s\" for conflicts.", pRegionName ).Access() );
			continue;
		}

		FOR_EACH_SUBKEY( pKVConflict, pKVConflictOther )
		{
			const char *pOtherRegionName = pKVConflictOther->GetName();
			int iOtherRegionIdx = GetEquipRegionIndexByName( pOtherRegionName );
			if ( iOtherRegionIdx < 0 )
			{
				pVecErrors->AddToTail( CFmtStr( "Unable to find other equip region named \"%s\" for conflicts.", pOtherRegionName ).Access() );
				continue;
			}

			SetEquipRegionConflict( iRegionIdx,		 m_vecEquipRegionsList[iOtherRegionIdx].m_unBitIndex );
			SetEquipRegionConflict( iOtherRegionIdx, m_vecEquipRegionsList[iRegionIdx].m_unBitIndex );
		}
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:	Initializes the attributes section of the schema
// Input:	pKVAttributes - The attributes section of the KeyValues 
//				representation of the schema
//			pVecErrors - An optional vector that will contain error messages if 
//				the init fails.
// Output:	True if initialization succeeded, false otherwise
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitAttributes( KeyValues *pKVAttributes, CUtlVector<CUtlString> *pVecErrors )
{
	// Initialize the attribute definitions
	FOR_EACH_TRUE_SUBKEY( pKVAttributes, pKVAttribute )
	{
		int nAttrIndex = Q_atoi( pKVAttribute->GetName() );
		int nMapIndex = m_mapAttributes.Find( nAttrIndex );

		// Make sure the index is positive
		SCHEMA_INIT_CHECK( 
			nAttrIndex >= 0,
			"Attribute definition index %d must be greater than or equal to zero", nAttrIndex );

		// Make sure the attribute index is not repeated
		SCHEMA_INIT_CHECK( 
			!m_mapAttributes.IsValidIndex( nMapIndex ),
			"Duplicate attribute definition index (%d)", nAttrIndex );

		nMapIndex = m_mapAttributes.Insert( nAttrIndex );

		SCHEMA_INIT_SUBSTEP( m_mapAttributes[nMapIndex].BInitFromKV( pKVAttribute, pVecErrors ) );
	}

	// Check the integrity of the attribute definitions

	// Check for duplicate attribute definition names
	CUtlRBTree<const char *> rbAttributeNames( CaselessStringLessThan );
	rbAttributeNames.EnsureCapacity( m_mapAttributes.Count() );
	FOR_EACH_MAP_FAST( m_mapAttributes, i )
	{
		int iIndex = rbAttributeNames.Find( m_mapAttributes[i].GetDefinitionName() );
		SCHEMA_INIT_CHECK( 
			!rbAttributeNames.IsValidIndex( iIndex ),
			"Attribute definition %d: Duplicate name \"%s\"", m_mapAttributes.Key( i ), m_mapAttributes[i].GetDefinitionName() );
		if( !rbAttributeNames.IsValidIndex( iIndex ) )
			rbAttributeNames.Insert( m_mapAttributes[i].GetDefinitionName() );
	}

	return SCHEMA_INIT_SUCCESS();
}


//-----------------------------------------------------------------------------
// Purpose:	Initializes the items section of the schema
// Input:	pKVItems - The items section of the KeyValues 
//				representation of the schema
//			pVecErrors - An optional vector that will contain error messages if 
//				the init fails.
// Output:	True if initialization succeeded, false otherwise
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitItems( KeyValues *pKVItems, CUtlVector<CUtlString> *pVecErrors )
{
	m_mapItems.PurgeAndDeleteElements();
	m_mapItemsSorted.Purge();
	m_mapBaseItems.Purge();
	m_vecBundles.Purge();

#if defined(CLIENT_DLL) || defined(GAME_DLL)
	if ( m_pDefaultItemDefinition )
	{
		delete m_pDefaultItemDefinition;
		m_pDefaultItemDefinition = NULL;
	}
#endif

	// initialize the item definitions
	if ( NULL != pKVItems )
	{
		FOR_EACH_TRUE_SUBKEY( pKVItems, pKVItem )
		{
			if ( Q_stricmp( pKVItem->GetName(), "default" ) == 0 )
			{
#if defined(CLIENT_DLL) || defined(GAME_DLL)
				SCHEMA_INIT_CHECK(
					m_pDefaultItemDefinition == NULL,
					"Duplicate 'default' item definition." );

				m_pDefaultItemDefinition = CreateEconItemDefinition();
				SCHEMA_INIT_SUBSTEP( m_pDefaultItemDefinition->BInitFromKV( pKVItem, pVecErrors ) );
#endif
			}
			else
			{
				int nItemIndex = Q_atoi( pKVItem->GetName() );
				int nMapIndex = m_mapItems.Find( nItemIndex );

				// Make sure the item index is correct because we use this index as a reference
				SCHEMA_INIT_CHECK( 
					!m_mapItems.IsValidIndex( nMapIndex ),
					"Duplicate item definition (%d)", nItemIndex );

				// Check to make sure the index is positive
				SCHEMA_INIT_CHECK( 
					nItemIndex >= 0,
					"Item definition index %d must be greater than or equal to zero", nItemIndex );

				CEconItemDefinition *pItemDef = CreateEconItemDefinition();
				nMapIndex = m_mapItems.Insert( nItemIndex, pItemDef );
				m_mapItemsSorted.Insert( nItemIndex, pItemDef );
				SCHEMA_INIT_SUBSTEP( m_mapItems[nMapIndex]->BInitFromKV( pKVItem, pVecErrors ) );

				if ( pItemDef->IsBaseItem() )
				{
					m_mapBaseItems.Insert( nItemIndex, pItemDef );
				}
			}
		}
	}

	// Check the integrity of the item definitions
	CUtlRBTree<const char *> rbItemNames( CaselessStringLessThan );
	rbItemNames.EnsureCapacity( m_mapItems.Count() );
	FOR_EACH_MAP_FAST( m_mapItems, i )
	{
		CEconItemDefinition *pItemDef = m_mapItems[ i ];

		// Check for duplicate item definition names
		int iIndex = rbItemNames.Find( pItemDef->GetDefinitionName() );
		SCHEMA_INIT_CHECK( 
			!rbItemNames.IsValidIndex( iIndex ),
			"Item definition %s: Duplicate name on index %d", pItemDef->GetDefinitionName(), m_mapItems.Key( i ) );
		if( !rbItemNames.IsValidIndex( iIndex ) )
			rbItemNames.Insert( m_mapItems[i]->GetDefinitionName() );
	}

	return SCHEMA_INIT_SUCCESS();
}

#if 0 // Compiled out until some DotA changes from the item editor are brought over
//-----------------------------------------------------------------------------
// Purpose:	Delete an item definition. Moderately dangerous as cached references will become bad.
// Intended for use by the item editor.
//-----------------------------------------------------------------------------
bool CEconItemSchema::DeleteItemDefinition( int iDefIndex )
{
	m_mapItemsSorted.Remove( iDefIndex );

	int nMapIndex = m_mapItems.Find( iDefIndex );
	if ( m_mapItems.IsValidIndex( nMapIndex ) )
	{
		CEconItemDefinition* pItemDef = m_mapItems[nMapIndex];
		if ( pItemDef )
		{
			m_mapItems.RemoveAt( nMapIndex );
			delete pItemDef;
			return true;
		}
	}
	return false;
}
#endif

//-----------------------------------------------------------------------------
bool CEconItemSchema::BVerifyBaseItemNames( CUtlVector<CUtlString> *pVecErrors )
{
	FOR_EACH_MAP_FAST( m_mapItems, i )
	{
		CEconItemDefinition *pItemDef = m_mapItems[i];

		// get base item name
		const char* pBaseName = pItemDef->GetBaseFunctionalItemName();

		if ( !pBaseName || pBaseName[0] == '\0' )
		{
			continue;
		}
		
		// look up base item name
		SCHEMA_INIT_CHECK( GetItemDefinitionByName( pBaseName ) != NULL, "Base item name not found %s.", pBaseName );
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:	Builds the name of a achievement in the form App<ID>.<AchName>
// Input:	unAppID - native app ID
//			pchNativeAchievementName - name of the achievement in its native app
// Returns: The combined achievement name
//-----------------------------------------------------------------------------
CUtlString CEconItemSchema::ComputeAchievementName( AppId_t unAppID, const char *pchNativeAchievementName ) 
{
	return CFmtStr1024( "App%u.%s", unAppID, pchNativeAchievementName ).Access();
}

static const char *s_particle_controlpoint_names[] =
{
	"attachment",
	"control_point_1",
	"control_point_2",
	"control_point_3",
	"control_point_4",
	"control_point_5",
	"control_point_6",
};

//-----------------------------------------------------------------------------
// Purpose:	Initializes the attribute-controlled-particle-systems section of the schema
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitAttributeControlledParticleSystems( KeyValues *pKVParticleSystems, CUtlVector<CUtlString> *pVecErrors )
{
	m_mapAttributeControlledParticleSystems.RemoveAll();
	m_vecAttributeControlledParticleSystemsCosmetics.RemoveAll();
	m_vecAttributeControlledParticleSystemsWeapons.RemoveAll();
	m_vecAttributeControlledParticleSystemsTaunts.RemoveAll();
	
	CUtlVector< int > *pVec = NULL;

	// Addictional groups we are tracking for.
	// "cosmetic_unusual_effects"
	// "weapon_unusual_effects"
	// "taunt_unusual_effects"

	if ( NULL != pKVParticleSystems )
	{
		FOR_EACH_TRUE_SUBKEY( pKVParticleSystems, pKVCategory )
		{
			// There is 3 Categories we want to track with additional info
			if ( !V_strcmp( pKVCategory->GetName(), "cosmetic_unusual_effects" ) )
			{
				pVec = &m_vecAttributeControlledParticleSystemsCosmetics;
			} 
			else if ( !V_strcmp( pKVCategory->GetName(), "weapon_unusual_effects" ) )
			{
				pVec = &m_vecAttributeControlledParticleSystemsWeapons;
			}
			else if ( !V_strcmp( pKVCategory->GetName(), "taunt_unusual_effects" ) )
			{
				pVec = &m_vecAttributeControlledParticleSystemsTaunts;
			}
			else
			{
				pVec = NULL; // reset
			}
			
			FOR_EACH_TRUE_SUBKEY( pKVCategory, pKVEntry )
			{
				int32 nItemIndex = atoi( pKVEntry->GetName() );
				// Check to make sure the index is positive
				SCHEMA_INIT_CHECK( 
					nItemIndex > 0,
					"Particle system index %d greater than zero", nItemIndex );
				if ( nItemIndex <= 0 )
					continue;
				int iIndex = m_mapAttributeControlledParticleSystems.Insert( nItemIndex );
				attachedparticlesystem_t &system = m_mapAttributeControlledParticleSystems[iIndex];
				system.pszSystemName = pKVEntry->GetString( "system", NULL );
				system.bFollowRootBone = pKVEntry->GetInt( "attach_to_rootbone", 0 ) != 0;
				system.iCustomType = 0;
				system.nSystemID = nItemIndex;
				system.fRefireTime = pKVEntry->GetFloat( "refire_time", 0.0f );
				system.bDrawInViewModel = pKVEntry->GetBool( "draw_in_viewmodel", false );
				system.bUseSuffixName = pKVEntry->GetBool( "use_suffix_name", false );

				COMPILE_TIME_ASSERT( ARRAYSIZE( system.pszControlPoints ) == ARRAYSIZE( s_particle_controlpoint_names ) );
				for ( int i=0; i<ARRAYSIZE( system.pszControlPoints ); ++i )
				{
					system.pszControlPoints[i] = pKVEntry->GetString( s_particle_controlpoint_names[i], NULL );
				}

				if ( pVec )
				{
					pVec->AddToTail( nItemIndex );
				}
			}
		}
	}
	return SCHEMA_INIT_SUCCESS();
}

#ifdef CLIENT_DLL
locchar_t *CEconItemSchema::GetParticleSystemLocalizedName( int index ) const
{
	const attachedparticlesystem_t *pSystem = GetItemSchema()->GetAttributeControlledParticleSystem( index );
	if ( !pSystem )
		return NULL;

	char particleNameEntry[128];
	Q_snprintf( particleNameEntry, ARRAYSIZE( particleNameEntry ), "#Attrib_Particle%d", pSystem->nSystemID );

	return g_pVGuiLocalize->Find( particleNameEntry );
}

#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitStringTables( KeyValues *pKVStringTables, CUtlVector<CUtlString> *pVecErrors )
{
	m_dictStringTable.PurgeAndDeleteElements();
	
	// initialize the rewards sections
	if ( NULL != pKVStringTables )
	{
		FOR_EACH_SUBKEY( pKVStringTables, pKVTable )
		{
			SCHEMA_INIT_CHECK( !m_dictStringTable.IsValidIndex( m_dictStringTable.Find( pKVTable->GetName() ) ),
				"Duplicate string table name '%s'.", pKVTable->GetName() );

			SchemaStringTableDict_t::IndexType_t i = m_dictStringTable.Insert( pKVTable->GetName(), new CUtlVector< schema_string_table_entry_t > );
			FOR_EACH_SUBKEY( pKVTable, pKVEntry )
			{
				schema_string_table_entry_t s = { atoi( pKVEntry->GetName() ), pKVEntry->GetString() };
				m_dictStringTable[i]->AddToTail( s );
			}
		}
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const ISchemaAttributeType *CEconItemSchema::GetAttributeType( const char *pszAttrTypeName ) const
{
	FOR_EACH_VEC( m_vecAttributeTypes, i )
	{
		if ( m_vecAttributeTypes[i].m_sName == pszAttrTypeName )
			return m_vecAttributeTypes[i].m_pAttrType;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
econ_tag_handle_t CEconItemSchema::GetHandleForTag( const char *pszTagName )
{
	EconTagDict_t::IndexType_t i = m_dictTags.Find( pszTagName );
	if ( m_dictTags.IsValidIndex( i ) )
		return i;

	return m_dictTags.Insert( pszTagName );
}


#if defined(CLIENT_DLL) || defined(GAME_DLL)
//-----------------------------------------------------------------------------
// Purpose:	Clones the specified item definition, and returns the new item def.
//-----------------------------------------------------------------------------
void CEconItemSchema::ItemTesting_CreateTestDefinition( int iCloneFromItemDef, int iNewDef, KeyValues *pNewKV )
{
	int nMapIndex = m_mapItems.Find( iNewDef );
	if ( !m_mapItems.IsValidIndex( nMapIndex ) )
	{
		nMapIndex = m_mapItems.Insert( iNewDef, CreateEconItemDefinition() );
		m_mapItemsSorted.Insert( iNewDef, m_mapItems[nMapIndex] );
	}

	// Find & copy the clone item def's data in
	CEconItemDefinition *pCloneDef = GetItemDefinition( iCloneFromItemDef );
	if ( !pCloneDef )
		return;
	m_mapItems[nMapIndex]->CopyPolymorphic( pCloneDef );

	// Then stomp it with the KV test contents
	m_mapItems[nMapIndex]->BInitFromTestItemKVs( iNewDef, pNewKV );
}

//-----------------------------------------------------------------------------
// Purpose:	Discards the specified item definition
//-----------------------------------------------------------------------------
void CEconItemSchema::ItemTesting_DiscardTestDefinition( int iDef )
{
	m_mapItems.Remove( iDef );
	m_mapItemsSorted.Remove( iDef );
}
#endif

//-----------------------------------------------------------------------------
// Purpose:	Gets an item definition for the specified definition index
// Input:	iItemIndex - The index of the desired definition.
// Output:	A pointer to the desired definition, or NULL if it is not found.
//-----------------------------------------------------------------------------
CEconItemDefinition *CEconItemSchema::GetItemDefinition( int iItemIndex )
{
#if defined(CLIENT_DLL) || defined(GAME_DLL)
#if !defined(CSTRIKE_DLL)
	AssertMsg( GetDefaultItemDefinition(), "No default item definition set up for item schema." );
#endif // CSTRIKE_DLL
#endif // defined(CLIENT_DLL) || defined(GAME_DLL)

	int iIndex = m_mapItems.Find( iItemIndex );
	if ( m_mapItems.IsValidIndex( iIndex ) )
		return m_mapItems[iIndex]; 

#if defined( GC_DLL ) || defined( EXTERNALTESTS_DLL )
	return NULL;
#else // !GC_DLL
	if ( GetDefaultItemDefinition() )
		return GetDefaultItemDefinition();

#if !defined(CSTRIKE_DLL)
	// We shouldn't ever get down here, but all the same returning a valid pointer is very slightly
	// a better plan than returning an invalid pointer to code that won't check to see if it's valid.
	static CEconItemDefinition *s_pEmptyDefinition = CreateEconItemDefinition();
	return s_pEmptyDefinition;
#else
	return NULL;
#endif // CSTRIKE_DLL

#endif // GC_DLL
}
const CEconItemDefinition *CEconItemSchema::GetItemDefinition( int iItemIndex ) const
{
	return const_cast<CEconItemSchema *>(this)->GetItemDefinition( iItemIndex );
}

//-----------------------------------------------------------------------------
// Purpose:	Gets an item definition that has a name matching the specified name.
// Input:	pszDefName - The name of the desired definition.
// Output:	A pointer to the desired definition, or NULL if it is not found.
//-----------------------------------------------------------------------------
CEconItemDefinition *CEconItemSchema::GetItemDefinitionByName( const char *pszDefName )
{
	// This shouldn't happen, but let's not crash if it ever does.
	Assert( pszDefName != NULL );
	if ( pszDefName == NULL )
		return NULL;

	FOR_EACH_MAP_FAST( m_mapItems, i )
	{
		if ( V_stricmp( pszDefName, m_mapItems[i]->GetDefinitionName()) == 0 )
			return m_mapItems[i]; 
	}
	return NULL;
}

const CEconItemDefinition *CEconItemSchema::GetItemDefinitionByName( const char *pszDefName ) const
{
	return const_cast<CEconItemSchema *>(this)->GetItemDefinitionByName( pszDefName );
}

//-----------------------------------------------------------------------------
// Purpose:	Gets an attribute definition for an index
// Input:	iAttribIndex - The index of the desired definition.
// Output:	A pointer to the desired definition, or NULL if it is not found.
//-----------------------------------------------------------------------------
CEconItemAttributeDefinition *CEconItemSchema::GetAttributeDefinition( int iAttribIndex )
{ 
	int iIndex = m_mapAttributes.Find( iAttribIndex );
	if ( m_mapAttributes.IsValidIndex( iIndex ) )
		return &m_mapAttributes[iIndex]; 
	return NULL;
}
const CEconItemAttributeDefinition *CEconItemSchema::GetAttributeDefinition( int iAttribIndex ) const
{
	return const_cast<CEconItemSchema *>(this)->GetAttributeDefinition( iAttribIndex );
}

CEconItemAttributeDefinition *CEconItemSchema::GetAttributeDefinitionByName( const char *pszDefName )
{
	Assert( pszDefName );
	if ( !pszDefName )
		return NULL;

	VPROF_BUDGET( "CEconItemSchema::GetAttributeDefinitionByName", VPROF_BUDGETGROUP_STEAM );
	FOR_EACH_MAP_FAST( m_mapAttributes, i )
	{
		Assert( m_mapAttributes[i].GetDefinitionName() );
		if ( !m_mapAttributes[i].GetDefinitionName() )
			continue;

		if ( V_stricmp( pszDefName, m_mapAttributes[i].GetDefinitionName() ) == 0 )
			return &m_mapAttributes[i]; 
	}
	return NULL;
}
const CEconItemAttributeDefinition *CEconItemSchema::GetAttributeDefinitionByName( const char *pszDefName ) const
{
	return const_cast<CEconItemSchema *>(this)->GetAttributeDefinitionByName( pszDefName );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CEconColorDefinition *CEconItemSchema::GetColorDefinitionByName( const char *pszDefName )
{
	FOR_EACH_VEC( m_vecColorDefs, i )
	{
		if ( !Q_stricmp( m_vecColorDefs[i]->GetName(), pszDefName ) )
			return m_vecColorDefs[i];
	}
	return NULL;
}
const CEconColorDefinition *CEconItemSchema::GetColorDefinitionByName( const char *pszDefName ) const
{
	return const_cast<CEconItemSchema *>(this)->GetColorDefinitionByName( pszDefName );
}

//-----------------------------------------------------------------------------
// Purpose:	Return the attribute specified attachedparticlesystem_t* associated with the given id.
//-----------------------------------------------------------------------------
attachedparticlesystem_t* CEconItemSchema::GetAttributeControlledParticleSystem( int id )
{
	int iIndex = m_mapAttributeControlledParticleSystems.Find( id );
	if ( m_mapAttributeControlledParticleSystems.IsValidIndex( iIndex ) )
		return &m_mapAttributeControlledParticleSystems[iIndex];
	return NULL;
}

attachedparticlesystem_t* CEconItemSchema::FindAttributeControlledParticleSystem( const char *pchSystemName )
{
	FOR_EACH_MAP_FAST( m_mapAttributeControlledParticleSystems, nSystem )
	{
		if( !Q_stricmp( m_mapAttributeControlledParticleSystems[nSystem].pszSystemName, pchSystemName ) )
			return &m_mapAttributeControlledParticleSystems[nSystem];
	}
	return NULL;
}

#if defined(CLIENT_DLL) || defined(GAME_DLL)
bool CEconItemSchema::SetupPreviewItemDefinition( KeyValues *pKV )
{
	int nMapIndex = m_mapItems.Find( PREVIEW_ITEM_DEFINITION_INDEX );
	if ( !m_mapItems.IsValidIndex( nMapIndex ) )
	{
		nMapIndex = m_mapItems.Insert( PREVIEW_ITEM_DEFINITION_INDEX, CreateEconItemDefinition() );
	}

	CEconItemDefinition *pItemDef = m_mapItems[ nMapIndex ];
	return pItemDef->BInitFromKV( pKV );
}
#endif // defined(CLIENT_DLL) || defined(GAME_DLL)

//-----------------------------------------------------------------------------
// Purpose: Ensure that all of our internal structures are consistent, and
//			account for all memory that we've allocated.
// Input:	validator -		Our global validator object
//			pchName -		Our name (typically a member var in our container)
//-----------------------------------------------------------------------------
#ifdef DBGFLAG_VALIDATE
void CEconItemSchema::Validate( CValidator &validator, const char *pchName )
{
	VALIDATE_SCOPE();
	ValidateObj( m_mapQualities );

	FOR_EACH_MAP_FAST( m_mapQualities, i )
	{
		ValidateObj( m_mapQualities[i] );
	}

	ValidateObj( m_mapItems );
	
	FOR_EACH_MAP_FAST( m_mapItems, i )
	{
		ValidateObj( m_mapItems[i] );
	}

	ValidateObj( m_mapUpgradeableBaseItems );

	FOR_EACH_MAP_FAST( m_mapUpgradeableBaseItems, i )
	{
		ValidateObj( m_mapUpgradeableBaseItems[i] );
	}

	ValidateObj( m_mapAttributes );

	FOR_EACH_MAP_FAST( m_mapAttributes, i )
	{
		ValidateObj( m_mapAttributes[i] );
	}

	ValidateObj( m_mapRecipes );

	FOR_EACH_MAP_FAST( m_mapRecipes, i )
	{
		ValidateObj( m_mapRecipes[i] );
	}

	FOR_EACH_VEC( m_vecTimedRewards, i )
	{
		ValidateObj( m_vecTimedRewards[i] );
	}
	ValidateObj( m_vecTimedRewards );

}
#endif // DBGFLAG_VALIDATE


bool CEconItemSchema::BPostSchemaInit( CUtlVector<CUtlString> *pVecErrors )
{
	// We need the protodefs to be initialized
	if ( !GetProtoScriptObjDefManager()->BDefinitionsLoaded() )
	{
		GetProtoScriptObjDefManager()->BInitDefinitions();
	}
	else
	{
		// If they were already initialized, do another PostInit as this might
		// be a new schema
		GetProtoScriptObjDefManager()->BPostDefinitionsLoaded();
	}

	bool bAllSuccess = true;

	// Make sure all of our tools are valid. We have to do this after the whole schema is initialized so
	// that we don't run into circular reference problems with items referencing loot lists that reference
	// items, etc.
	FOR_EACH_MAP_FAST( m_mapItems, i )
	{
		if ( !m_mapItems[i]->BPostInit( pVecErrors ) )
		{
			bAllSuccess = false;
		}
	}

	return bAllSuccess;
}