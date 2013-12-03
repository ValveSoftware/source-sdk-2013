//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DMXELEMENT_H
#define DMXELEMENT_H

#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmattributetypes.h"
#include "tier1/utlvector.h"
#include "tier1/utlrbtree.h"
#include "tier1/utlsymbol.h"
#include "tier1/mempool.h"
#include "tier1/UtlSortVector.h"
#include "dmxloader/dmxattribute.h"


//-----------------------------------------------------------------------------
// Sort functor class for attributes 
//-----------------------------------------------------------------------------
class CDmxAttributeLess
{
public:
	bool Less( const CDmxAttribute * pAttribute1, const CDmxAttribute *pAttribute2, void *pContext )
	{
		return pAttribute1->GetNameSymbol() < pAttribute2->GetNameSymbol();
	}
};


//-----------------------------------------------------------------------------
// Used to unpack elements into a structure. Does not recurse
// Also does not work with arrays.
//-----------------------------------------------------------------------------
struct DmxElementUnpackStructure_t
{
	const char *m_pAttributeName;
	const char *m_pDefaultString;
	DmAttributeType_t m_AttributeType;
	int m_nOffset;
	int m_nSize;
	const void *m_pUserData;	// If you want to associate some app-specific data with each field
};

#define DECLARE_DMXELEMENT_UNPACK() \
	template <typename T> friend DmxElementUnpackStructure_t *DmxElementUnpackInit(T *);

#define BEGIN_DMXELEMENT_UNPACK( _structName )				\
	template <typename T> DmxElementUnpackStructure_t *DmxElementUnpackInit(T *); \
	template <> DmxElementUnpackStructure_t *DmxElementUnpackInit<_structName>( _structName * ); \
	namespace _structName##_UnpackInit \
	{ \
		static DmxElementUnpackStructure_t *s_pUnpack = DmxElementUnpackInit( (_structName *)NULL ); \
	} \
	\
	template <> DmxElementUnpackStructure_t *DmxElementUnpackInit<_structName>( _structName * ) \
	{ \
		typedef _structName DestStructType_t; \
		static DmxElementUnpackStructure_t unpack[] = \
		{ \

#define DMXELEMENT_UNPACK_FLTX4( _attributeName, _defaultString, _varName )	\
	{ _attributeName, _defaultString, CDmAttributeInfo<float>::AttributeType(), offsetof( DestStructType_t, _varName ), sizeof( fltx4 ), NULL },
#define DMXELEMENT_UNPACK_FIELD( _attributeName, _defaultString, _type, _varName )	\
	{ _attributeName, _defaultString, CDmAttributeInfo<_type>::AttributeType(), offsetof( DestStructType_t, _varName ), sizeof( ((DestStructType_t *)0)->_varName), NULL },
#define DMXELEMENT_UNPACK_FIELD_STRING( _attributeName, _defaultString, _varName )	\
	{ _attributeName, _defaultString, AT_STRING, offsetof( DestStructType_t, _varName ), sizeof( ((DestStructType_t *)0)->_varName), NULL },

#define DMXELEMENT_UNPACK_FIELD_USERDATA( _attributeName, _defaultString, _type, _varName, _userData )	\
	{ _attributeName, _defaultString, CDmAttributeInfo<_type>::AttributeType(), offsetof( DestStructType_t, _varName ), sizeof( ((DestStructType_t *)0)->_varName), _userData },
#define DMXELEMENT_UNPACK_FIELD_STRING_USERDATA( _attributeName, _defaultString, _varName, _userData )	\
	{ _attributeName, _defaultString, AT_STRING, offsetof( DestStructType_t, _varName ), sizeof( ((DestStructType_t *)0)->_varName), _userData },

#define END_DMXELEMENT_UNPACK( _structName, _varName )			\
			{ NULL, NULL, AT_UNKNOWN, 0, 0, NULL }				\
		};														\
		return unpack;											\
	}															\
	DmxElementUnpackStructure_t *_varName = _structName##_UnpackInit::s_pUnpack;

#define END_DMXELEMENT_UNPACK_TEMPLATE( _structName, _varName )			\
			{ NULL, NULL, AT_UNKNOWN, 0, 0, NULL }				\
		};														\
		return unpack;											\
	}															\
	 template<> DmxElementUnpackStructure_t *_varName = _structName##_UnpackInit::s_pUnpack;


//-----------------------------------------------------------------------------
// Element used to read dmx files from mod code. Similar to keyvalues.
//-----------------------------------------------------------------------------
class CDmxElement
{
	DECLARE_DMX_ALLOCATOR( );

public:
	bool				HasAttribute( const char *pAttributeName ) const;
	CDmxAttribute		*GetAttribute( const char *pAttributeName );
	const CDmxAttribute *GetAttribute( const char *pAttributeName ) const;
	int					AttributeCount() const;
	CDmxAttribute		*GetAttribute( int nIndex );
	const CDmxAttribute *GetAttribute( int nIndex ) const;
	CUtlSymbol			GetType() const;
	const char*			GetTypeString() const;
	const char*			GetName() const;
	const DmObjectId_t &GetId() const;

	// Add+remove+rename can only occur during lock
	// NOTE: AddAttribute will find or add; returning an existing attribute if
	// one with the appropriate name exists
	void				LockForChanges( bool bLock );
	CDmxAttribute		*AddAttribute( const char *pAttributeName );
	void				RemoveAttribute( const char *pAttributeName );
	void				RemoveAttributeByPtr( CDmxAttribute *pAttribute );
	void				RemoveAllAttributes();
	void				RenameAttribute( const char *pAttributeName, const char *pNewName );

	// Simple methods to read attributes
	const char *GetValueString( const char *pAttributeName ) const;
	template< class T > const T& GetValue( const char *pAttributeName ) const;
	template< class T > const T& GetValue( const char *pAttributeName, const T& defaultValue ) const;

	template< class T > const CUtlVector<T>& GetArray( const char *pAttributeName ) const;
	template< class T > const CUtlVector<T>& GetArray( const char *pAttributeName, const CUtlVector<T>& defaultValue ) const;

	// Set methods
	CDmxAttribute* SetValue( const char *pAttributeName, const char *pString );
	CDmxAttribute* SetValue( const char *pAttributeName, void *pBuffer, int nLen );
	template< class T > CDmxAttribute* SetValue( const char *pAttributeName, const T& value );

	// Method to unpack data into a structure
	void UnpackIntoStructure( void *pData, size_t DataSizeInBytes, const DmxElementUnpackStructure_t *pUnpack ) const;

	// Creates attributes based on the unpack structure
	template <typename T>
	void AddAttributesFromStructure( const T *pData, const DmxElementUnpackStructure_t *pUnpack )
	{
		AddAttributesFromStructure_Internal( pData, sizeof(T), pUnpack );
	}

private:
	void AddAttributesFromStructure_Internal( const void *pData, size_t byteCount, const DmxElementUnpackStructure_t *pUnpack );
	typedef CUtlSortVector< CDmxAttribute*, CDmxAttributeLess > AttributeList_t;

	CDmxElement( const char *pType );
	~CDmxElement();

	// Removes all elements recursively
	void RemoveAllElementsRecursive();

	// Adds elements to delete to the deletion list
	void AddElementsToDelete( CUtlVector< CDmxElement * >& elementsToDelete );

	// Sorts the vector when a change has occurred
	void Resort( ) const;

	// Finds an attribute by name
	int FindAttribute( const char *pAttributeName ) const;
	int FindAttribute( CUtlSymbol attributeName ) const;

	// Sets the object id
	void SetId( const DmObjectId_t &id );

	// Are we locked?
	bool IsLocked() const;

	AttributeList_t m_Attributes;
	DmObjectId_t m_Id;	// We need this strictly because we support serialization
	CUtlSymbol m_Type;
	char m_nLockCount;
	mutable bool m_bResortNeeded : 1;
	bool m_bIsMarkedForDeletion : 1;

	static CUtlSymbolTableMT s_TypeSymbols;

	friend class CDmxSerializer;
	friend class CDmxSerializerKeyValues2;
	friend void CleanupDMX( CDmxElement* pElement );
	friend CDmxElement* CreateDmxElement( const char *pType );
};


//-----------------------------------------------------------------------------
// inline methods
//-----------------------------------------------------------------------------

// Are we locked?
inline bool CDmxElement::IsLocked() const
{
	return m_nLockCount > 0;
}

inline const char *CDmxElement::GetValueString( const char *pAttributeName ) const
{
	const CDmxAttribute* pAttribute = GetAttribute( pAttributeName );
	if ( pAttribute )
		return pAttribute->GetValueString();
	return "";
}

template< class T > 
inline const T& CDmxElement::GetValue( const char *pAttributeName ) const
{
	const CDmxAttribute* pAttribute = GetAttribute( pAttributeName );
	if ( pAttribute )
		return pAttribute->GetValue<T>();

	static T defaultValue;
	CDmAttributeInfo<T>::SetDefaultValue( defaultValue );
	return defaultValue;
}

template< class T >
inline const T& CDmxElement::GetValue( const char *pAttributeName, const T& defaultValue ) const
{
	const CDmxAttribute* pAttribute = GetAttribute( pAttributeName );
	if ( pAttribute )
		return pAttribute->GetValue<T>();
	return defaultValue;
}

template< class T > 
inline const CUtlVector<T>& CDmxElement::GetArray( const char *pAttributeName ) const
{
	const CDmxAttribute* pAttribute = GetAttribute( pAttributeName );
	if ( pAttribute )
		return pAttribute->GetArray<T>();

	static CUtlVector<T> defaultValue;
	return defaultValue;
}

template< class T > 
inline const CUtlVector<T>& CDmxElement::GetArray( const char *pAttributeName, const CUtlVector<T>& defaultValue ) const
{
	const CDmxAttribute* pAttribute = GetAttribute( pAttributeName );
	if ( pAttribute )
		return pAttribute->GetArray<T>();
	return defaultValue;
}


//-----------------------------------------------------------------------------
// Creates a dmx element
//-----------------------------------------------------------------------------
CDmxElement* CreateDmxElement( const char *pType );


//-----------------------------------------------------------------------------
// Helper class to lock elements for changes
//-----------------------------------------------------------------------------
class CDmxElementModifyScope
{
public:
	CDmxElementModifyScope( CDmxElement *pElement ) : m_pElement( pElement )
	{
		m_pElement->LockForChanges( true );
	}
	~CDmxElementModifyScope()
	{
		Release();
	}
	void Release()
	{
		if ( m_pElement )
		{
			m_pElement->LockForChanges( false );
			m_pElement = NULL;
		}
	}
private:
	CDmxElement *m_pElement;
};


//-----------------------------------------------------------------------------
// Set methods
//-----------------------------------------------------------------------------
inline CDmxAttribute* CDmxElement::SetValue( const char *pAttributeName, const char *pString )
{
	CDmxElementModifyScope modify( this );
	CDmxAttribute *pAttribute = AddAttribute( pAttributeName );
	pAttribute->SetValue( pString );
	return pAttribute;
}

inline CDmxAttribute* CDmxElement::SetValue( const char *pAttributeName, void *pBuffer, int nLen )
{
	CDmxElementModifyScope modify( this );
	CDmxAttribute *pAttribute = AddAttribute( pAttributeName );
	pAttribute->SetValue( pBuffer, nLen );
	return pAttribute;
}

template< class T > 
inline CDmxAttribute* CDmxElement::SetValue( const char *pAttributeName, const T& value )
{
	CDmxElementModifyScope modify( this );
	CDmxAttribute *pAttribute = AddAttribute( pAttributeName );
	pAttribute->SetValue( value );
	return pAttribute;
}


#endif // DMXELEMENT_H
