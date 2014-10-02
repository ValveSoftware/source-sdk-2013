//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DMATTRIBUTE_H
#define DMATTRIBUTE_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/attributeflags.h"
#include "datamodel/idatamodel.h"
#include "datamodel/dmattributetypes.h"
#include "datamodel/dmelement.h"
#include "datamodel/dmvar.h"
#include "tier1/utlhash.h"

//-----------------------------------------------------------------------------
// Fast dynamic cast
//-----------------------------------------------------------------------------
template< class E >
inline E *CastElement( CDmElement *pElement )
{
	if ( pElement && pElement->IsA( E::GetStaticTypeSymbol() ) )
		return static_cast< E* >( pElement );
	return NULL;
}


//-----------------------------------------------------------------------------
// type-safe element creation and accessor helpers - infers type name string from actual type
//-----------------------------------------------------------------------------
template< class E >
inline E *GetElement( DmElementHandle_t hElement )
{
	CDmElement *pElement = g_pDataModel->GetElement( hElement );
	return CastElement< E >( pElement );
}

//-----------------------------------------------------------------------------
// Typesafe element creation + destruction
//-----------------------------------------------------------------------------
template< class E >
inline E *CreateElement( const char *pObjectName, DmFileId_t fileid = DMFILEID_INVALID, const DmObjectId_t *pObjectID = NULL )
{
	return GetElement< E >( g_pDataModel->CreateElement( E::GetStaticTypeSymbol(), pObjectName, fileid, pObjectID ) );
}

template< class E >
inline E *CreateElement( const char *pElementType, const char *pObjectName, DmFileId_t fileid = DMFILEID_INVALID, const DmObjectId_t *pObjectID = NULL )
{
	return GetElement< E >( g_pDataModel->CreateElement( pElementType, pObjectName, fileid, pObjectID ) );
}


//-----------------------------------------------------------------------------
// Used for attribute change callbacks
//-----------------------------------------------------------------------------
typedef unsigned short DmMailingList_t;
enum
{
	DMMAILINGLIST_INVALID = (DmMailingList_t)~0
};


//-----------------------------------------------------------------------------
// Purpose: A general purpose pAttribute.  Eventually will be extensible to arbitrary user types
//-----------------------------------------------------------------------------
class CDmAttribute
{
public:
	// Returns the type
	DmAttributeType_t GetType() const;
	const char *GetTypeString() const;
	template< class T > bool IsA() const;

	// Returns the name. NOTE: The utlsymbol
	// can be turned into a string by using g_pDataModel->String();
	const char *GetName() const;
	UtlSymId_t	GetNameSymbol() const;
	void		SetName( const char *newName );

	// Gets the attribute value
	// NOTE: GetValueUntyped is used with GetType() for use w/ SetValue( type, void* )
	template< class T > const T& GetValue() const;
	template< class T > const T& GetValue( const T& defaultValue ) const;
	const char					*GetValueString() const;
	template< class E > E		*GetValueElement() const;
	const void					*GetValueUntyped() const; 

	// Sets the attribute value
	template< class T > void SetValue( const T &value );
	template< class E > void SetValue( E* pValue );
	void	SetValue( const void *pValue, size_t nSize );

	// Copies w/ type conversion (if possible) from another attribute
	void	SetValue( const CDmAttribute *pAttribute );
	void	SetValue( CDmAttribute *pAttribute );
	void	SetValue( DmAttributeType_t valueType, const void *pValue );

	// Sets the attribute to its default value based on its type
	void	SetToDefaultValue();

	// Convert to and from string
	void SetValueFromString( const char *pValue );
	const char *GetValueAsString( char *pBuffer, size_t nBufLen ) const;

	// Used for element and element array attributes; it specifies which type of
	// elements are valid to be referred to by this attribute
	void		SetElementTypeSymbol( UtlSymId_t typeSymbol );
	UtlSymId_t	GetElementTypeSymbol() const;

	// Returns the next attribute
	CDmAttribute *NextAttribute();
	const CDmAttribute *NextAttribute() const;

	// Returns the owner
	CDmElement *GetOwner();

	// Methods related to flags
	void	AddFlag( int flags );
	void	RemoveFlag( int flags );
	void	ClearFlags();
	int		GetFlags() const;
	bool	IsFlagSet( int flags ) const;

	// Serialization
	bool	Serialize( CUtlBuffer &buf ) const;
	bool	Unserialize( CUtlBuffer &buf );

	// Serialization of a single element. 
	// First version of UnserializeElement adds to tail if it worked
	// Second version overwrites, but does not add, the element at the specified index 
	bool	SerializeElement( int nElement, CUtlBuffer &buf ) const;
	bool	UnserializeElement( CUtlBuffer &buf );
	bool	UnserializeElement( int nElement, CUtlBuffer &buf );

	// Does this attribute serialize on multiple lines?
	bool	SerializesOnMultipleLines() const;

	// Get the attribute/create an attribute handle
	DmAttributeHandle_t GetHandle( bool bCreate = true );

	// Notify external elements upon change ( Calls OnAttributeChanged )
	// Pass false here to stop notification
	void	NotifyWhenChanged( DmElementHandle_t h, bool bNotify );

	// estimate memory overhead
	int		EstimateMemoryUsage( TraversalDepth_t depth ) const;

private:
	// Class factory
	static CDmAttribute *CreateAttribute( CDmElement *pOwner, DmAttributeType_t type, const char *pAttributeName );
	static CDmAttribute *CreateExternalAttribute( CDmElement *pOwner, DmAttributeType_t type, const char *pAttributeName, void *pExternalMemory );
	static void DestroyAttribute( CDmAttribute *pAttribute );

	// Constructor, destructor
	CDmAttribute( CDmElement *pOwner, DmAttributeType_t type, const char *pAttributeName );
	CDmAttribute( CDmElement *pOwner, DmAttributeType_t type, const char *pAttributeName, void *pMemory );
	~CDmAttribute();

	// Used when constructing CDmAttributes
	void Init( CDmElement *pOwner, DmAttributeType_t type, const char *pAttributeName );

	// Used when shutting down, indicates DmAttributeHandle_t referring to this are invalid
	void InvalidateHandle();

	// Used when shutting down, indicates no more change notifications will be sent to listening elements
	void CleanupMailingList();

	// Called when the attribute changes
	void PreChanged();
	void OnChanged( bool bArrayCountChanged = false, bool bIsTopological = false );

	// Is modification allowed in this phase?
	bool ModificationAllowed() const;

	// Mark the attribute as being dirty
	bool MarkDirty();

	// Is the data inline in a containing element class?
	bool IsDataInline() const;

	// Allocates, frees internal data storage
	void CreateAttributeData();
	void DeleteAttributeData();

	// Gets at the internal data storage 
	void* GetAttributeData();
	const void*	GetAttributeData() const;
	template < class T > typename CDmAttributeInfo< T >::StorageType_t* GetData();
	template < class T > const typename CDmAttributeInfo< T >::StorageType_t* GetData() const;
	template < class T > typename CDmAttributeInfo< CUtlVector< T > >::StorageType_t* GetArrayData();
	template < class T > const typename CDmAttributeInfo< CUtlVector< T > >::StorageType_t* GetArrayData() const;

	// Used by CDmElement to manage the list of attributes it owns
	CDmAttribute **GetNextAttributeRef();

	// Implementational function used for memory consumption estimation computation
	int EstimateMemoryUsageInternal( CUtlHash< DmElementHandle_t > &visited, TraversalDepth_t depth, int *pCategories ) const;

	// Called by elements after unserialization of their attributes is complete
	void OnUnserializationFinished();

	template< class T > bool IsTypeConvertable() const;
	template< class T > bool ShouldModify( const T& src );
	template< class T > void CopyData( const T& src );
	template< class T > void CopyDataOut( T& dest ) const;

private:
	CDmAttribute *m_pNext;
	void *m_pData;
	CDmElement *m_pOwner;
	int m_nFlags;
	DmAttributeHandle_t m_Handle;
	CUtlSymbol m_Name;
	DmMailingList_t m_hMailingList;

	friend class CDmElement;
	friend class CDmAttributeAccessor;
	template< class T > friend class CDmrElementArray;
	template< class E > friend class CDmrElementArrayConst;
	template< class T > friend class CDmaArrayAccessor;
	template< class T, class B > friend class CDmrDecorator;
	template< class T, class B > friend class CDmrDecoratorConst;
	template< class T > friend class CDmArrayAttributeOp;
};

	 
//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline DmAttributeType_t CDmAttribute::GetType() const
{
	return (DmAttributeType_t)( m_nFlags & FATTRIB_TYPEMASK );
}

template< class T > inline bool CDmAttribute::IsA() const
{
	return GetType() == CDmAttributeInfo< T >::AttributeType();
}

inline const char *CDmAttribute::GetName() const
{
	return g_pDataModel->GetString( m_Name );
}

inline UtlSymId_t CDmAttribute::GetNameSymbol() const
{
	return m_Name;
}


//-----------------------------------------------------------------------------
// Iteration
//-----------------------------------------------------------------------------
inline CDmAttribute *CDmAttribute::NextAttribute()
{
	return m_pNext;
}

inline const CDmAttribute *CDmAttribute::NextAttribute() const
{
	return m_pNext;
}


//-----------------------------------------------------------------------------
// Returns the owner
//-----------------------------------------------------------------------------
inline CDmElement *CDmAttribute::GetOwner()
{
	return m_pOwner;
}


//-----------------------------------------------------------------------------
// Value getting methods
//-----------------------------------------------------------------------------
template< class T > 
inline const T& CDmAttribute::GetValue( const T& defaultValue ) const
{
	if ( GetType() == ( DmAttributeType_t )( CDmAttributeInfo< T >::ATTRIBUTE_TYPE ) )
		return *reinterpret_cast< const T* >( m_pData );

	if ( IsTypeConvertable< T >() )
	{
		static T tempVal;
		CopyDataOut( tempVal );
		return tempVal;
	}

	Assert( 0 );
	return defaultValue;
}

template< class T > 
inline const T& CDmAttribute::GetValue() const
{
	static CDmaVar< T > defaultVal;
	return GetValue( defaultVal.Get() );
}

inline const char *CDmAttribute::GetValueString() const
{
	Assert( GetType() == AT_STRING );
	if ( GetType() != AT_STRING )
		return NULL;

	return GetValue< CUtlString >();
}

// used with GetType() for use w/ SetValue( type, void* )
inline const void* CDmAttribute::GetValueUntyped() const
{ 
	return m_pData; 
} 

template< class E > 
inline E* CDmAttribute::GetValueElement() const
{
	Assert( GetType() == AT_ELEMENT );
	if ( GetType() == AT_ELEMENT )
		return GetElement<E>( this->GetValue< DmElementHandle_t >() );
	return NULL;
}


//-----------------------------------------------------------------------------
// Value setting methods
//-----------------------------------------------------------------------------
template< class E > 
inline void CDmAttribute::SetValue( E* pValue )
{
	Assert( GetType() == AT_ELEMENT );
	if ( GetType() == AT_ELEMENT )
	{
		SetValue( pValue ? pValue->GetHandle() : DMELEMENT_HANDLE_INVALID );
	}
}

template<>
inline void CDmAttribute::SetValue( const char *pValue )
{
	int nLen = pValue ? Q_strlen( pValue ) + 1 : 0;
	CUtlString str( pValue, nLen );
	return SetValue( str );
}

template<>
inline void CDmAttribute::SetValue( char *pValue )
{
	return SetValue( (const char *)pValue );
}

inline void CDmAttribute::SetValue( const void *pValue, size_t nSize )
{
	CUtlBinaryBlock buf( pValue, (int)nSize );
	return SetValue( buf );
}


//-----------------------------------------------------------------------------
// Methods related to flags
//-----------------------------------------------------------------------------
inline void CDmAttribute::AddFlag( int nFlags )
{
	m_nFlags |= nFlags;
}

inline void CDmAttribute::RemoveFlag( int nFlags )
{
	m_nFlags &= ~nFlags;
}

inline void CDmAttribute::ClearFlags()
{
	m_nFlags = 0;
}

inline int CDmAttribute::GetFlags() const
{
	return m_nFlags;
}

inline bool CDmAttribute::IsFlagSet( int nFlags ) const
{
	return ( nFlags & m_nFlags ) ? true : false;
}

inline bool CDmAttribute::IsDataInline() const
{ 
	return !IsFlagSet(FATTRIB_EXTERNAL); 
}


//-----------------------------------------------------------------------------
// Gets at the internal data storage 
//-----------------------------------------------------------------------------
inline void* CDmAttribute::GetAttributeData() 
{ 
	return m_pData; 
}

inline const void* CDmAttribute::GetAttributeData() const 
{ 
	return m_pData; 
}

template < class T >
inline typename CDmAttributeInfo< T >::StorageType_t* CDmAttribute::GetData()
{
	return ( typename CDmAttributeInfo< T >::StorageType_t* )m_pData;
}

template < class T >
inline typename CDmAttributeInfo< CUtlVector< T > >::StorageType_t* CDmAttribute::GetArrayData()
{
	return ( typename CDmAttributeInfo< CUtlVector< T > >::StorageType_t* )m_pData;
}

template < class T >
inline const typename CDmAttributeInfo< T >::StorageType_t* CDmAttribute::GetData() const
{
	return ( const typename CDmAttributeInfo< T >::StorageType_t* )m_pData;
}

template < class T >
inline const typename CDmAttributeInfo< CUtlVector< T > >::StorageType_t* CDmAttribute::GetArrayData() const
{
	return ( const typename CDmAttributeInfo< CUtlVector< T > >::StorageType_t* )m_pData;
}


//-----------------------------------------------------------------------------
// Used by CDmElement to manage the list of attributes it owns
//-----------------------------------------------------------------------------
inline CDmAttribute **CDmAttribute::GetNextAttributeRef()
{
	return &m_pNext;
}


//-----------------------------------------------------------------------------
// helper function for determining which attributes/elements to traverse during copy/find/save/etc.
//-----------------------------------------------------------------------------
inline bool ShouldTraverse( const CDmAttribute *pAttr, TraversalDepth_t depth )
{
	switch ( depth )
	{
	case TD_NONE:
		return false;

	case TD_SHALLOW:
		if ( !pAttr->IsFlagSet( FATTRIB_MUSTCOPY ) )
			return false;
		// fall-through intentional
	case TD_DEEP:
		if ( pAttr->IsFlagSet( FATTRIB_NEVERCOPY ) )
			return false;
		// fall-through intentional
	case TD_ALL:
		return true;
	}

	Assert( 0 );
	return false;
}


//-----------------------------------------------------------------------------
// Gets attributes
//-----------------------------------------------------------------------------
inline CDmAttribute *CDmElement::GetAttribute( const char *pAttributeName, DmAttributeType_t type )
{
	CDmAttribute *pAttribute = FindAttribute( pAttributeName );
	if ( ( type != AT_UNKNOWN ) && pAttribute && ( pAttribute->GetType() != type ) )
		return NULL;
	return pAttribute;
}

inline const CDmAttribute *CDmElement::GetAttribute( const char *pAttributeName, DmAttributeType_t type ) const
{
	CDmAttribute *pAttribute = FindAttribute( pAttributeName );
	if ( ( type != AT_UNKNOWN ) && pAttribute && ( pAttribute->GetType() != type ) )
		return NULL;
	return pAttribute;
}


//-----------------------------------------------------------------------------
// AddAttribute calls
//-----------------------------------------------------------------------------
inline CDmAttribute *CDmElement::AddAttribute( const char *pAttributeName, DmAttributeType_t type )
{
	CDmAttribute *pAttribute = FindAttribute( pAttributeName );
	if ( pAttribute )
		return ( pAttribute->GetType() == type ) ? pAttribute : NULL;
	pAttribute = CreateAttribute( pAttributeName, type );
	return pAttribute;
}

template< class E > inline CDmAttribute *CDmElement::AddAttributeElement( const char *pAttributeName )
{
	CDmAttribute *pAttribute = AddAttribute( pAttributeName, AT_ELEMENT );
	if ( !pAttribute )
		return NULL;
	
	// FIXME: If the attribute exists but has a different element type symbol, should we complain?
	pAttribute->SetElementTypeSymbol( E::GetStaticTypeSymbol() );
	return pAttribute;
}

template< class E > inline CDmAttribute *CDmElement::AddAttributeElementArray( const char *pAttributeName )
{
	CDmAttribute *pAttribute = AddAttribute( pAttributeName, AT_ELEMENT_ARRAY );
	if ( !pAttribute )
		return NULL;
	
	// FIXME: If the attribute exists but has a different element type symbol, should we complain?
	pAttribute->SetElementTypeSymbol( E::GetStaticTypeSymbol() );
	return pAttribute;
}

//-----------------------------------------------------------------------------
// GetValue methods
//-----------------------------------------------------------------------------

template< class T >
inline const T& CDmElement::GetValue( const char *pAttributeName ) const
{
	static CDmaVar<T> defaultVal;
	return GetValue( pAttributeName, defaultVal.Get() );
}

inline const char *CDmElement::GetValueString( const char *pAttributeName ) const
{
	return GetValue<CUtlString>( pAttributeName ).Get();
}

template< class E >
inline E* CDmElement::GetValueElement( const char *pAttributeName ) const
{
	DmElementHandle_t h = GetValue< DmElementHandle_t >( pAttributeName );
	return GetElement<E>( h );
}


template< class T >
inline const T& CDmElement::GetValue( const char *pAttributeName, const T& defaultVal ) const
{
	const CDmAttribute *pAttribute = FindAttribute( pAttributeName );
	if ( pAttribute != NULL )
		return pAttribute->GetValue<T>();
	return defaultVal;
}

//-----------------------------------------------------------------------------
// SetValue methods
//-----------------------------------------------------------------------------
template< class T >
inline CDmAttribute* CDmElement::SetValue( const char *pAttributeName, const T& value )
{
	CDmAttribute *pAttribute = FindAttribute( pAttributeName );
	if ( !pAttribute )
	{
		pAttribute = CreateAttribute( pAttributeName, CDmAttributeInfo<T>::AttributeType() );
	}
	if ( pAttribute )
	{
		pAttribute->SetValue( value );
		return pAttribute;
	}
	return NULL;
}

template< class E >
inline CDmAttribute* CDmElement::SetValue( const char *pAttributeName, E* pElement )
{
	DmElementHandle_t hElement = pElement ? pElement->GetHandle() : DMELEMENT_HANDLE_INVALID;
	return SetValue( pAttributeName, hElement );
}

template<>
inline CDmAttribute* CDmElement::SetValue( const char *pAttributeName, const char *pValue )
{
	int nLen = pValue ? Q_strlen( pValue ) + 1 : 0;
	CUtlString str( pValue, nLen );
	return SetValue( pAttributeName, str );
}

template<>
inline CDmAttribute* CDmElement::SetValue( const char *pAttributeName, char *pValue )
{
	return SetValue( pAttributeName, (const char *)pValue );
}

inline CDmAttribute* CDmElement::SetValue( const char *pAttributeName, const void *pValue, size_t nSize )
{
	CUtlBinaryBlock buf( pValue, (int)nSize );
	return SetValue( pAttributeName, buf );
}


//-----------------------------------------------------------------------------
// AddValue methods( set value if not found )
//-----------------------------------------------------------------------------
template< class T >
inline CDmAttribute* CDmElement::InitValue( const char *pAttributeName, const T& value )
{
	CDmAttribute *pAttribute = GetAttribute( pAttributeName );
	if ( !pAttribute )
		return SetValue( pAttributeName, value );
	return pAttribute;
}

template< class E >
inline CDmAttribute* CDmElement::InitValue( const char *pAttributeName, E* pElement )
{
	DmElementHandle_t hElement = pElement ? pElement->GetHandle() : DMELEMENT_HANDLE_INVALID;
	return InitValue( pAttributeName, hElement );
}

inline  CDmAttribute* CDmElement::InitValue( const char *pAttributeName, const void *pValue, size_t size )
{
	CDmAttribute *pAttribute = GetAttribute( pAttributeName );
	if ( !pAttribute )
		return SetValue( pAttributeName, pValue, size );
	return pAttribute;
}

template< class T >
T *FindReferringElement( CDmElement *pElement, UtlSymId_t symAttrName, bool bMustBeInSameFile = true )
{
	DmAttributeReferenceIterator_t i = g_pDataModel->FirstAttributeReferencingElement( pElement->GetHandle() );
	while ( i != DMATTRIBUTE_REFERENCE_ITERATOR_INVALID )
	{
		CDmAttribute *pAttribute = g_pDataModel->GetAttribute( i );
		CDmElement *pDmeParent = pAttribute->GetOwner();
		if ( pDmeParent && pAttribute->GetNameSymbol() == symAttrName )
		{
			T *pParent = CastElement< T >( pDmeParent );
			if ( pParent )
			{
				if ( !bMustBeInSameFile || ( pParent->GetFileId() == pElement->GetFileId() ) )
					return pParent;
			}
		}
		i = g_pDataModel->NextAttributeReferencingElement( i );
	}
	
	return NULL;
}

template< class T >
T *FindAncestorReferencingElement( CDmElement *target )
{
	if ( !target )
		return NULL;
	
	for ( DmAttributeReferenceIterator_t it = g_pDataModel->FirstAttributeReferencingElement( target->GetHandle() );
		 it != DMATTRIBUTE_REFERENCE_ITERATOR_INVALID;
		 it = g_pDataModel->NextAttributeReferencingElement( it ) )
	{
		CDmAttribute *attr = g_pDataModel->GetAttribute( it );
		Assert( attr );
		CDmElement *element = attr->GetOwner();
		Assert( element );
		if ( !element )
			continue;
		T *t = CastElement< T >( element );
		if ( !t )
			continue;
		
		return t;
	}
	return NULL;
}

template< class T >
T *FindAncestorReferencingElement_R_Impl( CUtlRBTree< CDmElement * >& visited, CDmElement *check )
{
	if ( visited.Find( check ) != visited.InvalidIndex() )
		return NULL;
	
	visited.Insert( check );
	
	// Pass one, see if it's in this ancestor list
	DmAttributeReferenceIterator_t it;
	for ( it = g_pDataModel->FirstAttributeReferencingElement( check->GetHandle() );
		 it != DMATTRIBUTE_REFERENCE_ITERATOR_INVALID;
		 it = g_pDataModel->NextAttributeReferencingElement( it ) )
	{
		CDmAttribute *attr = g_pDataModel->GetAttribute( it );
		Assert( attr );
		CDmElement *element = attr->GetOwner();
		Assert( element );
		if ( !element )
			continue;
		T *t = CastElement< T >( element );
		if ( !t )
			continue;
		
		return t;
	}
	
	for ( it = g_pDataModel->FirstAttributeReferencingElement( check->GetHandle() );
		 it != DMATTRIBUTE_REFERENCE_ITERATOR_INVALID;
		 it = g_pDataModel->NextAttributeReferencingElement( it ) )
	{
		CDmAttribute *attr = g_pDataModel->GetAttribute( it );
		Assert( attr );
		CDmElement *element = attr->GetOwner();
		Assert( element );
		if ( !element )
			continue;
		
		T *found = FindAncestorReferencingElement_R_Impl< T >( visited, element );
		if ( found )
			return found;
	}
	return NULL;
}


template< class T >
void FindAncestorsReferencingElement( CDmElement *target, CUtlVector< T* >& list )
{
	if ( !target )
		return;
	
	list.RemoveAll();
	for ( DmAttributeReferenceIterator_t it = g_pDataModel->FirstAttributeReferencingElement( target->GetHandle() );
		 it != DMATTRIBUTE_REFERENCE_ITERATOR_INVALID;
		 it = g_pDataModel->NextAttributeReferencingElement( it ) )
	{
		CDmAttribute *attr = g_pDataModel->GetAttribute( it );
		Assert( attr );
		CDmElement *element = attr->GetOwner();
		Assert( element );
		if ( !element )
			continue;
		T* t = CastElement< T >( element );
		if ( !t )
			continue;
		
		if ( list.Find( t ) != list.InvalidIndex() )
			continue;
		
		list.AddToTail( t );
	}
}


template< class T >
T *FindAncestorReferencingElement_R( CDmElement *target )
{
	if ( !target )
		return NULL;
	
	CUtlRBTree< CDmElement * > visited( 0, 0, DefLessFunc( CDmElement * ) );
	return FindAncestorReferencingElement_R_Impl< T >( visited, target );
}


#endif // DMATTRIBUTE_H
