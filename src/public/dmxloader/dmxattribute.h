//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DMXATTRIBUTE_H
#define DMXATTRIBUTE_H

#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmattributetypes.h"
#include "tier1/utlvector.h"
#include "tier1/utlrbtree.h"
#include "tier1/utlsymbol.h"
#include "tier1/mempool.h"
#include "dmxloader/dmxloader.h"


//-----------------------------------------------------------------------------
// Forward declarations: 
//-----------------------------------------------------------------------------
class CDmxElement;


//-----------------------------------------------------------------------------
// Attribute info, modified for use in mod code
//-----------------------------------------------------------------------------
DECLARE_ATTRIBUTE_TYPE( CDmxElement*,		AT_ELEMENT,				"element",		value = 0; )
DECLARE_ATTRIBUTE_ARRAY_TYPE( CDmxElement*,	AT_ELEMENT_ARRAY,		"element_array" )


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CDmxAttribute
{
	DECLARE_DMX_ALLOCATOR( );

public:
	// Returns attribute name and type
	DmAttributeType_t GetType() const;
	const char *GetTypeString() const;
	template< class T > bool IsA() const;

	// Returns the name. NOTE: The utlsymbol
	// can be turned into a string by using g_pDataModel->String();
	const char *GetName() const;
	CUtlSymbol GetNameSymbol() const;
	void SetName( const char *pName );

	// Gets values
	template< class T >	const T& GetValue( ) const;
	template< class T > const CUtlVector< T >& GetArray( ) const;
	const char *GetValueString() const;

	// Sets values (+ type)
	template< class T > void SetValue( const T& value );
	void SetValue( const char *pString );
	void SetValue( const void *pBuffer, size_t nLen );
	void SetValue( const CDmxAttribute *pAttribute );

	// Method to set values in an array (just directly operate on the array)
	// NOTE: This will create a new array of the appropriate type if 
	// the type doesn't match the current type
	template< class T > CUtlVector< T >& GetArrayForEdit();

	// Sets the attribute to its default value based on its type
	void	SetToDefaultValue();

	// Convert to and from string
	void SetValueFromString( const char *pValue );
	const char *GetValueAsString( char *pBuffer, size_t nBufLen ) const;

	// Gets the size of an array, returns 0 if it's not an array type
	int GetArrayCount() const;

	// Read from file
	bool Unserialize( DmAttributeType_t type, CUtlBuffer &buf );
	bool UnserializeElement( DmAttributeType_t type, CUtlBuffer &buf );
	bool Serialize( CUtlBuffer &buf ) const;
	bool SerializeElement( int nIndex, CUtlBuffer &buf ) const;
	bool SerializesOnMultipleLines() const;

	// Returns the size of the variables storing the various attribute types
	static int AttributeDataSize( DmAttributeType_t type );

private:
	CDmxAttribute( const char *pAttributeName );
	CDmxAttribute( CUtlSymbol attributeName );
	~CDmxAttribute();

	// Allocate, free memory for data
	void AllocateDataMemory( DmAttributeType_t type );
	void FreeDataMemory( );

	// Untyped method for setting used by unpack
	void SetValue( DmAttributeType_t type, const void *pSrc, int nLen );

	DmAttributeType_t m_Type;
	CUtlSymbol m_Name;
	void *m_pData;

	static CUtlSymbolTableMT s_AttributeNameSymbols;

	friend class CDmxElement;
};


//-----------------------------------------------------------------------------
// Inline methods 
//-----------------------------------------------------------------------------
inline DmAttributeType_t CDmxAttribute::GetType() const
{
	return m_Type;
}

template< class T > inline bool CDmxAttribute::IsA() const
{
	return GetType() == CDmAttributeInfo< T >::ATTRIBUTE_TYPE;
}

inline CUtlSymbol CDmxAttribute::GetNameSymbol() const
{
	return m_Name;
}


//-----------------------------------------------------------------------------
// Sets a value in the attribute
//-----------------------------------------------------------------------------
template< class T > void CDmxAttribute::SetValue( const T& value )
{
	AllocateDataMemory( CDmAttributeInfo<T>::AttributeType() );
	CopyConstruct( (T*)m_pData, value );
}


//-----------------------------------------------------------------------------
// Returns data in the attribute
//-----------------------------------------------------------------------------
inline const char *CDmxAttribute::GetValueString() const
{
	if ( m_Type == AT_STRING )
		return *(CUtlString*)m_pData;
	return "";
}

template< class T >
inline const T& CDmxAttribute::GetValue( ) const
{
	if ( CDmAttributeInfo<T>::AttributeType() == m_Type )
		return *(T*)m_pData;

	static T defaultValue;
	CDmAttributeInfo<T>::SetDefaultValue( defaultValue );
	return defaultValue;
}

template< class T > 
inline const CUtlVector< T >& CDmxAttribute::GetArray( ) const
{
	if ( CDmAttributeInfo< CUtlVector< T > >::AttributeType() == m_Type )
		return *( CUtlVector< T > *)m_pData;

	static CUtlVector<T> defaultArray;
	return defaultArray;
}

template< class T > 
inline CUtlVector< T >& CDmxAttribute::GetArrayForEdit( )
{
	if ( CDmAttributeInfo< CUtlVector< T > >::AttributeType() == m_Type )
		return *( CUtlVector< T > *)m_pData;

	AllocateDataMemory( CDmAttributeInfo< CUtlVector< T > >::AttributeType() );
	Construct( (CUtlVector<T>*)m_pData );
	return *(CUtlVector< T > *)m_pData;
}

#endif // DMXATTRIBUTE_H
