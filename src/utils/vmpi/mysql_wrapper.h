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


class IMySQL;


// This is a helper class to build queries like the stream IO.
class CMySQLQuery
{
friend class CMySQL;

public:
	// This is like a sprintf, but it will grow the string as necessary.
	void				Format( PRINTF_FORMAT_STRING const char *pFormat, ... );

private:
	CUtlVector<char>	m_QueryText;
};


class CColumnValue
{
public:

					CColumnValue( CMySQL *pSQL, int iColumn );

	const char*		String();
	long			Int32();
	unsigned long	UInt32();

private:
	CMySQL		*m_pSQL;
	int			m_iColumn;
};


class IMySQL
{
public:
	virtual void			Release() = 0;

	// These execute SQL commands. They return 0 if the query was successful.
	virtual int				Execute( const char *pString ) = 0;
	virtual int				Execute( CMySQLQuery &query ) = 0;

	// If you just inserted rows into a table with an AUTO_INCREMENT column,
	// then this returns the (unique) value of that column.
	virtual unsigned long	InsertID() = 0;

	// If you just executed a select statement, then you can use these functions to
	// iterate over the result set.

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

	// You can call this to get the index of a column for faster lookups with GetColumnValue( int ).
	// Returns -1 if the column can't be found.
	virtual int				GetColumnIndex( const char *pColumnName ) = 0;
};


IMySQL* InitMySQL( const char *pDBName, const char *pHostName="", const char *pUserName="", const char *pPassword="" );



// ------------------------------------------------------------------------------------------------ //
// Inlines.
// ------------------------------------------------------------------------------------------------ //

inline CColumnValue::CColumnValue( CMySQL *pSQL, int iColumn )
{
	m_pSQL = pSQL;
	m_iColumn = iColumn;
}


#endif // MYSQL_WRAPPER_H
