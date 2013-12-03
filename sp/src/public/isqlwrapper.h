//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Sql wrapper, encapsulates basic SQL functionality 
//
// $NoKeywords: $
//=============================================================================
#ifndef ISQLWRAPPER_H
#define ISQLWRAPPER_H

#include "tier0/platform.h"

#ifdef _WIN32
#pragma once
#endif

// SQL column types
enum EColumnType
{
	SQL_NONE = 0,
	SQL_INT,
	SQL_STRING,
	SQL_FLOAT,
	SQL_TIME,
	SQL_UINT64
};


//-----------------------------------------------------------------------------
// encapsulates a table's description
//-----------------------------------------------------------------------------
class ISQLTable
{
public:
	virtual int GetCSQLColumn() const = 0;
	virtual const char *PchColumnName( int iSQLColumn ) const = 0;
	virtual EColumnType GetEColumnType( int iSQLColumn ) const = 0;
	virtual const char *PchName() const = 0;
};


//-----------------------------------------------------------------------------
// encapsulates a database worth of tables
//-----------------------------------------------------------------------------
class ISQLTableSet
{
public:
	virtual int GetCSQLTable() const = 0;
	virtual const ISQLTable *PSQLTable( int iSQLTable ) const = 0;
};


//-----------------------------------------------------------------------------
// encapsulates a single returned row from a valid SQL query
//-----------------------------------------------------------------------------
class ISQLRow
{
public:
	virtual int GetCSQLRowData() const = 0;
	virtual const char *PchData( int iSQLColumn ) const = 0;
	virtual int NData( int iSQLColumn ) const = 0;
	virtual uint64 UlData( int iSQLColumn ) const = 0;
	virtual float FlData( int iSQLColumn ) const = 0;
	virtual uint64 UlTime( int iSQLColumn ) const = 0;
	virtual bool BData( int iSQLColumn ) const = 0;
	virtual EColumnType GetEColumnType( int iSQLColumn ) const = 0;
};


//-----------------------------------------------------------------------------
// encapsulates a result set, which is made up of a series of rows
// You need to call PNextResult() NRow() times to get all the results 
// (there is no random access to the result set).
//-----------------------------------------------------------------------------
class IResultSet
{
public:
	virtual int GetCSQLRow() const = 0;
	virtual const ISQLRow *PSQLRowNextResult() = 0; // note, not const on the class because it makes a new row object
};


//-----------------------------------------------------------------------------
// This interface encapsulates a database connection and lets you operate on it. 
// Use the ISQLWrapperFactory factory to get access to the interface.
// NOTE - you can only have one outstanding query at a time. When you are done with a query call FreeResult()
//-----------------------------------------------------------------------------
class ISQLWrapper
{
public:
	// run a SQL statement against the DB, typically an insert statement as no data is returned
	virtual bool BInsert( const char *pchQueryString ) = 0;
	// get a description of the tables associated with the db you connected to
	virtual const ISQLTableSet *PSQLTableSetDescription() = 0;
	// run a query against the db and then iterate the result set (you can only have 1 outstanding query at a time, and call FreeResults() when you are done)
	virtual IResultSet *PResultSetQuery( const char *pchQueryString ) = 0; 
	// you MUST call then after you finish with the IResultSet from the above query
	virtual void FreeResult() = 0;
};


//-----------------------------------------------------------------------------
// This is a factory to create objects that let you interact with a MySQL database.
// Make sure you Free() any interfaces you create.
//-----------------------------------------------------------------------------
class ISQLWrapperFactory
{
public:
	// setup details about this db connection
	virtual ISQLWrapper *Create( const char *pchDB, const char *pchHost, const char *pchUsername, const char *pchPassword ) = 0;
	virtual void Free( ISQLWrapper *pSQLWrapper ) = 0;
};

#define INTERFACEVERSION_ISQLWRAPPER	"ISQLWRAPPER001"

#endif // ISQLWRAPPER_H

