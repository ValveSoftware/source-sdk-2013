//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MYSQL_WRAPPER_H
#define MYSQL_WRAPPER_H
#ifdef _WIN32
#pragma once
#endif


#include "utlvector.h"
#include "interface.h"


class IMySQLRowSet;


class CColumnValue
{
public:

					CColumnValue( IMySQLRowSet *pSQL, int iColumn );

	const char*		String();
	long			Int32();

private:
	IMySQLRowSet	*m_pSQL;
	int				m_iColumn;
};



class IMySQLRowSet
{
public:
	virtual void			Release() = 0;

	// Get the number of columns in the data returned from the last query (if it was a select statement).	
	virtual int				NumFields() = 0;
	
	// Get the name of each column returned by the last query.
	virtual const char*		GetFieldName( int iColumn ) = 0;

	// Call this in a loop until it returns false to iterate over all rows the query returned.
	virtual bool			NextRow() = 0;

	// You can call this to start iterating over the result set from the start again.
	// Note: after calling this, you have to call NextRow() to actually get the first row's value ready.
	virtual bool			SeekToFirstRow() = 0;

	virtual CColumnValue	GetColumnValue( int iColumn ) = 0;
	virtual CColumnValue	GetColumnValue( const char *pColumnName ) = 0;

	virtual const char*		GetColumnValue_String( int iColumn ) = 0;
	virtual long			GetColumnValue_Int( int iColumn ) = 0;

	// You can call this to get the index of a column for faster lookups with GetColumnValue( int ).
	// Returns -1 if the column can't be found.
	virtual int				GetColumnIndex( const char *pColumnName ) = 0;
};


class IMySQL : public IMySQLRowSet
{
public:
	virtual bool			InitMySQL( const char *pDBName, const char *pHostName="", const char *pUserName="", const char *pPassword="" ) = 0;
	virtual void			Release() = 0;

	// These execute SQL commands. They return 0 if the query was successful.
	virtual int				Execute( const char *pString ) = 0;

	// This reads in all of the data in the last row set you queried with Execute and builds a separate
	// copy. This is useful in some of the VMPI tools to have a thread repeatedly execute a slow query, then
	// store off the results for the main thread to parse.
	virtual IMySQLRowSet*	DuplicateRowSet() = 0;

	// If you just inserted rows into a table with an AUTO_INCREMENT column,
	// then this returns the (unique) value of that column.
	virtual unsigned long	InsertID() = 0;

	// Returns the last error message, if an error took place
	virtual const char *	GetLastError() = 0;
};


#define MYSQL_WRAPPER_VERSION_NAME "MySQLWrapper001"


// ------------------------------------------------------------------------------------------------ //
// Inlines.
// ------------------------------------------------------------------------------------------------ //

inline CColumnValue::CColumnValue( IMySQLRowSet *pSQL, int iColumn )
{
	m_pSQL = pSQL;
	m_iColumn = iColumn;
}

inline const char* CColumnValue::String()
{
	return m_pSQL->GetColumnValue_String( m_iColumn );
}

inline long	CColumnValue::Int32()
{
	return m_pSQL->GetColumnValue_Int( m_iColumn );
}


#endif // MYSQL_WRAPPER_H
