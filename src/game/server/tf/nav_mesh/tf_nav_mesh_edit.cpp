//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_nav_mesh_edit.cpp
// TF specific nav mesh editing
// Michael Booth, May 2009

#include "cbase.h"
#include "tf_nav_mesh.h"


//--------------------------------------------------------------------------------------------------------
class CTFAttributeClearer
{
public:
	CTFAttributeClearer( TFNavAttributeType attribute )
	{
		m_attribute = attribute;
	}

	bool operator() ( CNavArea *baseArea )
	{
		CTFNavArea *area = (CTFNavArea *)baseArea;
		
		area->ClearAttributeTF( m_attribute );
		
		return true;
	}
	
	TFNavAttributeType m_attribute;
};

void TF_EditClearAllAttributes( void )
{
	CTFAttributeClearer clear( (TFNavAttributeType)0xFFFFFFFF );
	TheNavMesh->ForAllSelectedAreas( clear );
	TheNavMesh->ClearSelectedSet();
}
static ConCommand ClearAllAttributes( "tf_wipe_attributes", TF_EditClearAllAttributes, "Clear all TF-specific attributes of selected area.", FCVAR_CHEAT );


//--------------------------------------------------------------------------------------------------------
struct AttributeLookup
{
	const char *name;
	TFNavAttributeType attribute;	
};

static AttributeLookup s_TFAttributeTable[] =
{
	{ "BLUE_SETUP_GATE", TF_NAV_BLUE_SETUP_GATE },
	{ "RED_SETUP_GATE", TF_NAV_RED_SETUP_GATE },
	{ "BLOCKED_AFTER_POINT_CAPTURE", TF_NAV_BLOCKED_AFTER_POINT_CAPTURE },
	{ "BLOCKED_UNTIL_POINT_CAPTURE", TF_NAV_BLOCKED_UNTIL_POINT_CAPTURE },
	{ "BLUE_ONE_WAY_DOOR", TF_NAV_BLUE_ONE_WAY_DOOR },
	{ "RED_ONE_WAY_DOOR", TF_NAV_RED_ONE_WAY_DOOR },

	{ "SNIPER_SPOT", TF_NAV_SNIPER_SPOT },
	{ "SENTRY_SPOT", TF_NAV_SENTRY_SPOT },
	{ "NO_SPAWNING", TF_NAV_NO_SPAWNING },
	{ "RESCUE_CLOSET", TF_NAV_RESCUE_CLOSET },

	{ "DOOR_ALWAYS_BLOCKS", TF_NAV_DOOR_ALWAYS_BLOCKS },
	{ "DOOR_NEVER_BLOCKS", TF_NAV_DOOR_NEVER_BLOCKS },
	{ "UNBLOCKABLE", TF_NAV_UNBLOCKABLE },

	{ "WITH_SECOND_POINT", TF_NAV_WITH_SECOND_POINT },
	{ "WITH_THIRD_POINT", TF_NAV_WITH_THIRD_POINT },
	{ "WITH_FOURTH_POINT", TF_NAV_WITH_FOURTH_POINT },
	{ "WITH_FIFTH_POINT", TF_NAV_WITH_FIFTH_POINT },

	{ NULL, TF_NAV_INVALID }
};


/**
 * Can be used with any command that takes an attribute as its 2nd argument
 */
static int AttributeAutocomplete( const char *input, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	if ( Q_strlen( input ) >= COMMAND_COMPLETION_ITEM_LENGTH )
	{
		return 0;
	}
	
	char command[ COMMAND_COMPLETION_ITEM_LENGTH+1 ];
	Q_strncpy( command, input, sizeof( command ) );
	
	// skip to start of argument
	char *partialArg = Q_strrchr( command, ' ' );
	if ( partialArg == NULL )
	{
		return 0;
	}
	
	// chop command from partial argument
	*partialArg = '\000';
	++partialArg;
	
	int partialArgLength = Q_strlen( partialArg );

	int count = 0;
	for( unsigned int i=0; s_TFAttributeTable[i].name && count < COMMAND_COMPLETION_MAXITEMS; ++i )
	{
		if ( !Q_strnicmp( s_TFAttributeTable[i].name, partialArg, partialArgLength ) )
		{
			// Add to the autocomplete array
			Q_snprintf( commands[ count++ ], COMMAND_COMPLETION_ITEM_LENGTH, "%s %s", command, s_TFAttributeTable[i].name );
		}
	}

/* all of these are deprecated
	for( unsigned int i=0; TheNavAttributeTable[i].name && count < COMMAND_COMPLETION_MAXITEMS; ++i )
	{
		if ( !Q_strnicmp( TheNavAttributeTable[i].name, partialArg, partialArgLength ) )
		{
			// Add to the autocomplete array
			Q_snprintf( commands[ count++ ], COMMAND_COMPLETION_ITEM_LENGTH, "%s %s", command, TheNavAttributeTable[i].name );
		}
	}
*/

	return count;
}


TFNavAttributeType NameToTFAttribute( const char *name )
{
	for( unsigned int i=0; s_TFAttributeTable[i].name; ++i )
	{
		if ( !Q_stricmp( s_TFAttributeTable[i].name, name ) )
		{
			return s_TFAttributeTable[i].attribute;
		}
	}
	
	return TF_NAV_INVALID;
}


const char *TFAttributeToName( TFNavAttributeType attribute )
{
	for( unsigned int i=0; s_TFAttributeTable[i].name; ++i )
	{
		if ( s_TFAttributeTable[i].attribute == attribute )
		{
			return s_TFAttributeTable[i].name;
		}
	}

	return NULL;
}


//--------------------------------------------------------------------------------------------------------
void TF_EditClearAttribute( const CCommand &args )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "Usage: %s <attribute1> [attribute2...]\n", args[0] );
		return;
	}
	
	for ( int i = 1; i < args.ArgC(); ++i )
	{
		TFNavAttributeType spawnAttribute = NameToTFAttribute( args[i] );
		NavAttributeType navAttribute = NameToNavAttribute( args[i] );

		if ( spawnAttribute != TF_NAV_INVALID )
		{
			CTFAttributeClearer clear( spawnAttribute );
			TheNavMesh->ForAllSelectedAreas( clear );
		}
		else if ( navAttribute != NAV_MESH_INVALID )
		{
			NavAttributeClearer clear( navAttribute );
			TheNavMesh->ForAllSelectedAreas( clear );
		}
		else
		{
			Msg( "Unknown attribute '%s'", args[i] );		
		}
	}

	TheNavMesh->ClearSelectedSet();
}
static ConCommand ClearAttributeTF( "tf_clear_attribute", TF_EditClearAttribute, "Remove given attribute from all areas in the selected set.", FCVAR_CHEAT, AttributeAutocomplete );


//--------------------------------------------------------------------------------------------------------
class CTFAttributeToggler
{
public:
	CTFAttributeToggler( TFNavAttributeType attribute )
	{
		m_attribute = attribute;
	}
	
	bool operator() ( CNavArea *baseArea )
	{
		CTFNavArea *area = (CTFNavArea *)baseArea;
		
		// only toggle if dealing with a single selected area
		if ( TheNavMesh->IsSelectedSetEmpty() && area->HasAttributeTF( m_attribute ) )
		{
			area->ClearAttributeTF( m_attribute );
		}
		else
		{
			area->SetAttributeTF( m_attribute );
		}
		
		return true;
	}
		
	TFNavAttributeType m_attribute;
};


//--------------------------------------------------------------------------------------------------------
void TF_EditMarkAttribute( const CCommand &args )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "Usage: %s <attribute> [attribute2...]\n", args[0] );
		return;
	}

	for ( int i = 1; i < args.ArgC(); ++i )
	{
		TFNavAttributeType spawnAttribute = NameToTFAttribute( args[i] );
		NavAttributeType navAttribute = NameToNavAttribute( args[i] );

		if ( spawnAttribute != TF_NAV_INVALID )
		{
			CTFAttributeToggler toggle( spawnAttribute );	
			TheNavMesh->ForAllSelectedAreas( toggle );
		}
		else if ( navAttribute != NAV_MESH_INVALID )
		{
			NavAttributeToggler clear( navAttribute );
			TheNavMesh->ForAllSelectedAreas( clear );
		}
		else
		{
			Msg( "Unknown attribute '%s'", args[i] );	
		}
	}

	TheNavMesh->ClearSelectedSet();
}
static ConCommand MarkAttribute( "tf_mark", TF_EditMarkAttribute, "Set attribute of selected area.", FCVAR_CHEAT, AttributeAutocomplete );


//--------------------------------------------------------------------------------------------------------
void TF_EditSelectWithAttribute( const CCommand &args )
{
	TheNavMesh->ClearSelectedSet();

	if ( args.ArgC() != 2 )
	{
		Msg( "Usage: %s <attribute>\n", args[0] );
		return;
	}

	TFNavAttributeType spawnAttribute = NameToTFAttribute( args[1] );

	int count = 0;

	if ( spawnAttribute != TF_NAV_INVALID )
	{
		FOR_EACH_VEC( TheNavAreas, it )
		{
			CTFNavArea *area = (CTFNavArea *)TheNavAreas[ it ];

			if ( area->HasAttributeTF( spawnAttribute ) )
			{
				TheNavMesh->AddToSelectedSet( area );
				++count;
			}
		}
	}
	else
	{
		NavAttributeType navAttribute = NameToNavAttribute( args[1] );
		if ( navAttribute != NAV_MESH_INVALID )
		{
			FOR_EACH_VEC( TheNavAreas, it )
			{
				CNavArea *area = TheNavAreas[ it ];

				if ( area->GetAttributes() & navAttribute )
				{
					TheNavMesh->AddToSelectedSet( area );
					++count;
				}
			}
		}
		else
		{
			Msg( "Unknown attribute '%s'", args[1] );		
		}
	}

	Msg( "%d areas added to selection\n", count );
}
static ConCommand SelectWithAttribute( "tf_select_with_attribute", TF_EditSelectWithAttribute, "Selects areas with the given attribute.", FCVAR_CHEAT, AttributeAutocomplete );

