//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef ECON_ITEM_SYSTEM_H
#define ECON_ITEM_SYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "econ_item_view.h"
#include "game_item_schema.h"

//==================================================================================
// ITEM SYSTEM
//==================================================================================

#define GC_MOTD_CACHE_FILE			"cfg/motd_entries.txt"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CEconItemSystem
{
public:
	CEconItemSystem( void );
	virtual ~CEconItemSystem( void );

	// Setup & parse in the item data files.
	void		Init( void );
	void		PostInit( void );
	void		Shutdown( void );

	// Return the static item data for the specified item index
	GameItemDefinition_t *GetStaticDataForItemByDefIndex( item_definition_index_t iItemDefIndex ) 
	{
		return (GameItemDefinition_t *)m_itemSchema.GetItemDefinition( iItemDefIndex );
	}
	CEconItemDefinition *GetStaticDataForItemByName( const char *pszDefName ) 
	{ 
		return m_itemSchema.GetItemDefinitionByName( pszDefName );
	}
	CEconItemAttributeDefinition *GetStaticDataForAttributeByDefIndex( attrib_definition_index_t iAttribDefinitionIndex ) 
	{ 
		return m_itemSchema.GetAttributeDefinition( iAttribDefinitionIndex );
	}
	CEconItemAttributeDefinition *GetStaticDataForAttributeByName( const char *pszDefName ) 
	{ 
		return m_itemSchema.GetAttributeDefinitionByName( pszDefName );
	}

	// Select and return a random item's definition index matching the specified criteria
	item_definition_index_t	GenerateRandomItem( CItemSelectionCriteria *pCriteria, entityquality_t *outEntityQuality );

	// Select and return the base item definition index for a class's load-out slot 
	// Note: baseitemcriteria_t is game-specific and/or may not exist!
	virtual item_definition_index_t GenerateBaseItem( struct baseitemcriteria_t *pCriteria ) { return INVALID_ITEM_DEF_INDEX; }

	// Return a random item quality
	entityquality_t GetRandomQualityForItem( bool bPreventUnique = false );

	// Decrypt the item files and return the keyvalue
	bool	DecryptItemFiles( KeyValues *pKV, const char *pName );		

	GameItemSchema_t *GetItemSchema() { return &m_itemSchema; }

	// Open the server's whitelist, and if it exists, set the appropriate items allowed.
	void		ReloadWhitelist( void );

	void		ResetAttribStringCache( void );

protected:
	// Read the specified item schema file. Init the item schema with the contents
	void		ParseItemSchemaFile( const char *pFilename );

	// Key to decrypt the item description files
	const unsigned char *GetEncryptionKey( void ) { return (unsigned char *)"A5fSXbf7"; }

private:
	GameItemSchema_t m_itemSchema;
};

CEconItemSystem *ItemSystem( void );

#endif // ECON_ITEM_SYSTEM_H
