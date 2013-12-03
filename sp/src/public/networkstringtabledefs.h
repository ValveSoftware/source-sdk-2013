//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef NETWORKSTRINGTABLEDEFS_H
#define NETWORKSTRINGTABLEDEFS_H
#ifdef _WIN32
#pragma once
#endif

typedef int TABLEID;

#define INVALID_STRING_TABLE -1
const unsigned short INVALID_STRING_INDEX = (unsigned short )-1;

// table index is sent in log2(MAX_TABLES) bits
#define MAX_TABLES	32  // Table id is 4 bits

#define INTERFACENAME_NETWORKSTRINGTABLESERVER "VEngineServerStringTable001"
#define INTERFACENAME_NETWORKSTRINGTABLECLIENT "VEngineClientStringTable001"

class INetworkStringTable;

typedef void (*pfnStringChanged)( void *object, INetworkStringTable *stringTable, int stringNumber, char const *newString, void const *newData );

//-----------------------------------------------------------------------------
// Purpose: Game .dll shared string table interfaces
//-----------------------------------------------------------------------------
class INetworkStringTable
{
public:

	virtual					~INetworkStringTable( void ) {};
		
	// Table Info
	virtual const char		*GetTableName( void ) const = 0;
	virtual TABLEID			GetTableId( void ) const = 0;
	virtual int				GetNumStrings( void ) const = 0;
	virtual int				GetMaxStrings( void ) const = 0;
	virtual int				GetEntryBits( void ) const = 0;

	// Networking
	virtual void			SetTick( int tick ) = 0;
	virtual bool			ChangedSinceTick( int tick ) const = 0;

	// Accessors (length -1 means don't change user data if string already exits)
	virtual int				AddString( bool bIsServer, const char *value, int length = -1, const void *userdata = 0 ) = 0; 

	virtual const char		*GetString( int stringNumber ) = 0;
	virtual void			SetStringUserData( int stringNumber, int length, const void *userdata ) = 0;
	virtual const void		*GetStringUserData( int stringNumber, int *length ) = 0;
	virtual int				FindStringIndex( char const *string ) = 0; // returns INVALID_STRING_INDEX if not found

	// Callbacks
	virtual void			SetStringChangedCallback( void *object, pfnStringChanged changeFunc ) = 0;
};

class INetworkStringTableContainer
{
public:
	
	virtual					~INetworkStringTableContainer( void ) {};
	
	// table creation/destruction
	virtual INetworkStringTable	*CreateStringTable( const char *tableName, int maxentries, int userdatafixedsize = 0, int userdatanetworkbits = 0 ) = 0;
	virtual void				RemoveAllTables( void ) = 0;
	
	// table infos
	virtual INetworkStringTable	*FindTable( const char *tableName ) const = 0;
	virtual INetworkStringTable	*GetTable( TABLEID stringTable ) const = 0;
	virtual int					GetNumTables( void ) const = 0;

	virtual INetworkStringTable	*CreateStringTableEx( const char *tableName, int maxentries, int userdatafixedsize = 0, int userdatanetworkbits = 0, bool bIsFilenames = false ) = 0;
	virtual void				SetAllowClientSideAddString( INetworkStringTable *table, bool bAllowClientSideAddString ) = 0;
};

#endif // NETWORKSTRINGTABLEDEFS_H
