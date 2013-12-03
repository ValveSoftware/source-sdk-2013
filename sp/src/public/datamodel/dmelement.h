//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DMELEMENT_H
#define DMELEMENT_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlmap.h"
#include "tier1/utlhash.h"
#include "tier1/utlvector.h"
#include "tier1/utlsymbol.h"
#include "datamodel/attributeflags.h"
#include "datamodel/idatamodel.h"
#include "datamodel/dmvar.h"


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
typedef bool (CDmElement::*pfnCommandMethod)( const char *command, const char *args );

// element/element array traversal path item - assumes the full path does NOT contain cycles
struct ElementPathItem_t
{
	ElementPathItem_t( DmElementHandle_t hElem = DMELEMENT_HANDLE_INVALID,
						DmAttributeHandle_t hAttr = DMATTRIBUTE_HANDLE_INVALID,
						int idx = -1 )
		: hElement( hElem ), hAttribute( hAttr ), nIndex( idx )
	{
	}

	// only uses hElement so that it can be used to search for elements
	bool operator==( const ElementPathItem_t &that ) const
	{
		return hElement == that.hElement;
	}

	DmElementHandle_t hElement;
	DmAttributeHandle_t hAttribute;
	int nIndex;
};


//-----------------------------------------------------------------------------
// singly-linked attribute list
//-----------------------------------------------------------------------------
struct DmAttributeList_t
{
	DmAttributeList_t() : m_hAttribute( DMATTRIBUTE_HANDLE_INVALID ), m_pNext( NULL ) {}
	DmAttributeHandle_t m_hAttribute;
	DmAttributeList_t *m_pNext;
};

//-----------------------------------------------------------------------------
// helper class to allow CDmeHandle access to g_pDataModelImp
//-----------------------------------------------------------------------------
class CDmeElementRefHelper
{
protected:
	void Ref  ( DmElementHandle_t hElement, bool bStrong );
	void Unref( DmElementHandle_t hElement, bool bStrong );
};

//-----------------------------------------------------------------------------
// element reference struct - containing attribute referrers and handle refcount
//-----------------------------------------------------------------------------
struct DmElementReference_t
{
	explicit DmElementReference_t( DmElementHandle_t hElement = DMELEMENT_HANDLE_INVALID ) :
		m_hElement( hElement ), m_nWeakHandleCount( 0 ), m_nStrongHandleCount( 0 )
	{
	}
	DmElementReference_t( const DmElementReference_t &that ) :
		m_hElement( that.m_hElement ), m_nWeakHandleCount( that.m_nWeakHandleCount ),
		m_nStrongHandleCount( that.m_nStrongHandleCount ), m_attributes( that.m_attributes )
	{
	}
	DmElementReference_t &operator=( const DmElementReference_t &that )
	{
		m_hElement = that.m_hElement;
		m_nWeakHandleCount = that.m_nWeakHandleCount;
		m_nStrongHandleCount = that.m_nStrongHandleCount;
		m_attributes.m_hAttribute = that.m_attributes.m_hAttribute;
		m_attributes.m_pNext = that.m_attributes.m_pNext;
		return *this;
	}
	~DmElementReference_t()
	{
		//		Assert( !IsStronglyReferenced() );
	}

	void AddAttribute( CDmAttribute *pAttribute );
	void RemoveAttribute( CDmAttribute *pAttribute );

	bool IsStronglyReferenced() // should this element be kept around (even if it's DmElementHandle_t is invalidated)
	{
		return m_attributes.m_hAttribute != DMATTRIBUTE_HANDLE_INVALID || m_nStrongHandleCount > 0;
	}

	bool IsWeaklyReferenced() // should we keep this element's DmElementHandle_t mapped to it's id (even if the element is deleted)
	{
		return IsStronglyReferenced() || m_nWeakHandleCount > 0;
	}

	int EstimateMemoryOverhead()
	{
		int nBytes = 0;
		for ( DmAttributeList_t *pLink = m_attributes.m_pNext; pLink; pLink = pLink->m_pNext )
		{
			nBytes += sizeof( DmAttributeList_t );
		}
		return nBytes;
	}

	DmElementHandle_t m_hElement;
	unsigned short m_nWeakHandleCount;		// CDmeHandle<T> - for auto-hookup once the element comes back, mainly used by UI
	unsigned short m_nStrongHandleCount;	// CDmeCountedElementRef - for preventing elements from being truly deleted, mainly used by undo and file root
	DmAttributeList_t m_attributes;
};


//-----------------------------------------------------------------------------
// Base DmElement we inherit from in higher-level classes 
//-----------------------------------------------------------------------------
class CDmElement
{
public:
	// Can be overridden by derived classes
	virtual	void		OnAttributeChanged( CDmAttribute *pAttribute ) {}
	virtual void		PreAttributeChanged( CDmAttribute *pAttribute ) {}
	virtual void		OnAttributeArrayElementAdded( CDmAttribute *pAttribute, int nFirstElem, int nLastElem ) {}
	virtual void		OnAttributeArrayElementRemoved( CDmAttribute *pAttribute, int nFirstElem, int nLastElem ) {}
	virtual void		Resolve() {}
	virtual	bool		IsA( UtlSymId_t typeSymbol ) const;
	virtual int			GetInheritanceDepth( UtlSymId_t typeSymbol ) const;
	virtual void		OnElementUnserialized() {}
	virtual int			AllocatedSize() const { return sizeof( CDmElement ); }

	// Returns the element handle
	DmElementHandle_t	GetHandle() const;

	// Attribute iteration, finding
	// NOTE: Passing a type into GetAttribute will return NULL if the attribute exists but isn't that type
	bool				HasAttribute( const char *pAttributeName, DmAttributeType_t type = AT_UNKNOWN ) const;
	CDmAttribute		*GetAttribute( const char *pAttributeName, DmAttributeType_t type = AT_UNKNOWN );
	const CDmAttribute	*GetAttribute( const char *pAttributeName, DmAttributeType_t type = AT_UNKNOWN ) const;
	int					AttributeCount() const;
	CDmAttribute*		FirstAttribute();
	const CDmAttribute*	FirstAttribute() const;

	// Element name, type, ID
	// WARNING: SetType() should only be used by format conversion methods (dmxconvert)
	UtlSymId_t			GetType() const;
	const char *		GetTypeString() const;
	const char *		GetName() const;
	const DmObjectId_t&	GetId() const;
	void				SetType( const char *pType );
	void				SetName( const char* pName );

	// Attribute management
	CDmAttribute *		AddAttribute( const char *pAttributeName, DmAttributeType_t type );
	template< class E > CDmAttribute* AddAttributeElement( const char *pAttributeName );
	template< class E > CDmAttribute* AddAttributeElementArray( const char *pAttributeName );
	void				RemoveAttribute( const char *pAttributeName );
	void				RemoveAttributeByPtr( CDmAttribute *pAttributeName );
	void				RenameAttribute( const char *pAttributeName, const char *pNewName );

	// get attribute value
	template< class T > const T& GetValue( const char *pAttributeName ) const;
	template< class T > const T& GetValue( const char *pAttributeName, const T& defaultValue ) const;
	const char *		GetValueString( const char *pAttributeName ) const;
	template< class E > E* GetValueElement( const char *pAttributeName ) const;

	// set attribute value
	CDmAttribute*		SetValue( const char *pAttributeName, const void *value, size_t size );
	template< class T > CDmAttribute* SetValue( const char *pAttributeName, const T& value );
	template< class E >	CDmAttribute* SetValue( const char *pAttributeName, E* value );

	// set attribute value if the attribute doesn't already exist
	CDmAttribute*		InitValue( const char *pAttributeName, const void *value, size_t size );
	template< class T > CDmAttribute* InitValue( const char *pAttributeName, const T& value );
	template< class E >	CDmAttribute* InitValue( const char *pAttributeName, E* value );

	// Parses an attribute from a string
	// Doesn't create an attribute if it doesn't exist and always preserves attribute type
	void				SetValueFromString( const char *pAttributeName, const char *value );
	const char			*GetValueAsString( const char *pAttributeName, char *pBuffer, size_t buflen ) const;

	// Helpers for our RTTI
	template< class E > bool IsA() const;
	bool				IsA( const char *pTypeName ) const;
	int					GetInheritanceDepth( const char *pTypeName ) const;
	static CUtlSymbol	GetStaticTypeSymbol();

	// Indicates whether this element should be copied or not
	void				SetShared( bool bShared );
	bool				IsShared() const;

	// Copies an element and all its attributes
	CDmElement*			Copy( TraversalDepth_t depth = TD_DEEP ) const;

	// Copies attributes from a specified element
	void				CopyAttributesTo( CDmElement *pCopy, TraversalDepth_t depth = TD_DEEP ) const;

	// recursively set fileid's, with option to only change elements in the matched file
	void				SetFileId( DmFileId_t fileid, TraversalDepth_t depth, bool bOnlyIfMatch = false );
	DmFileId_t			GetFileId() const;

	bool				IsAccessible() const;
	void				MarkAccessible( bool bAccessible );
	void				MarkAccessible( TraversalDepth_t depth = TD_ALL );

	// returns the first path to the element found traversing all element/element 
	// array attributes - not necessarily the shortest.
	// cycle-safe (skips any references to elements in the current path) 
	// but may re-traverse elements via different paths
	bool				FindElement( const CDmElement *pElement, CUtlVector< ElementPathItem_t > &elementPath, TraversalDepth_t depth ) const;
	bool				FindReferer( DmElementHandle_t hElement, CUtlVector< ElementPathItem_t > &elementPath, TraversalDepth_t depth ) const;
	void				RemoveAllReferencesToElement( CDmElement *pElement );
	bool				IsStronglyReferenced() { return m_ref.IsStronglyReferenced(); }

	// Estimates the memory usage of the element, its attributes, and child elements
	int					EstimateMemoryUsage( TraversalDepth_t depth = TD_DEEP );

protected:
	// NOTE: These are protected to ensure that the factory is the only thing that can create these
						CDmElement( DmElementHandle_t handle, const char *objectType, const DmObjectId_t &id, const char *objectName, DmFileId_t fileid );
	virtual				~CDmElement();

	// Used by derived classes to do construction and setting up CDmaVars
	void				OnConstruction() { }
	void				OnDestruction() { }																
	virtual void		PerformConstruction();
	virtual void		PerformDestruction();

	// Internal methods related to RTII
	static void			SetTypeSymbol( CUtlSymbol sym );
	static bool			IsA_Implementation( CUtlSymbol typeSymbol );
	static int			GetInheritanceDepth_Implementation( CUtlSymbol typeSymbol, int nCurrentDepth );

	// Internal method for creating a copy of this element
	CDmElement*			CopyInternal( TraversalDepth_t depth = TD_DEEP ) const;

	// helper for making attributevarelementarray cleanup easier
	template< class T >	static void DeleteAttributeVarElementArray( T &array );

private:
	typedef CUtlMap< DmElementHandle_t, DmElementHandle_t, int > CRefMap;

	// Bogus constructor
	CDmElement();

	// internal recursive copy method - builds refmap of old element's handle -> copy's handle, and uses it to fixup references
	void				CopyAttributesTo( CDmElement *pCopy, CRefMap &refmap, TraversalDepth_t depth ) const;
	void				CopyElementAttribute( const CDmAttribute *pAttr, CDmAttribute *pCopyAttr, CRefMap &refmap, TraversalDepth_t depth ) const;
	void				CopyElementArrayAttribute( const CDmAttribute *pAttr, CDmAttribute *pCopyAttr, CRefMap &refmap, TraversalDepth_t depth ) const;
	void				FixupReferences( CUtlHashFast< DmElementHandle_t > &visited, const CRefMap &refmap, TraversalDepth_t depth );

	void				SetFileId( DmFileId_t fileid );
	void				SetFileId_R( CUtlHashFast< DmElementHandle_t > &visited, DmFileId_t fileid, TraversalDepth_t depth, DmFileId_t match, bool bOnlyIfMatch );

	CDmAttribute*		CreateAttribute( const char *pAttributeName, DmAttributeType_t type );
	void				RemoveAttribute( CDmAttribute **pAttrRef );
	CDmAttribute*		AddExternalAttribute( const char *pAttributeName, DmAttributeType_t type, void *pMemory );
	CDmAttribute		*FindAttribute( const char *pAttributeName ) const;

	void				Purge();
	void				SetId( const DmObjectId_t &id );

	bool				IsDirty() const;
	void				MarkDirty( bool dirty = true );
	void				MarkAttributesClean();
	void				MarkBeingUnserialized( bool beingUnserialized = true );
	bool				IsBeingUnserialized() const;

	// Used by the undo system only.
	void				AddAttributeByPtr( CDmAttribute *ptr );
	void				RemoveAttributeByPtrNoDelete( CDmAttribute *ptr );

	// Should only be called from datamodel, who will take care of changing the fileset entry as well
	void				ChangeHandle( DmElementHandle_t handle );

	// returns element reference struct w/ list of referrers and handle count
	DmElementReference_t* GetReference();
	void				SetReference( const DmElementReference_t &ref );

	// Estimates memory usage
	int					EstimateMemoryUsage( CUtlHash< DmElementHandle_t > &visited, TraversalDepth_t depth, int *pCategories );

protected:
	CDmaString			m_Name;

private:
	CDmAttribute		*m_pAttributes;
	DmElementReference_t m_ref;
	UtlSymId_t			m_Type;
	bool				m_bDirty : 1;
	bool				m_bBeingUnserialized : 1;
	bool				m_bIsAcessible : 1;
	unsigned char		m_nReserved;	// Makes Id be quad aligned
	DmObjectId_t		m_Id;
	DmFileId_t			m_fileId;

	// Stores the type symbol
	static CUtlSymbol	m_classType;

	// Factories can access our constructors
	template <class T> friend class CDmElementFactory;
	template <class T> friend class CDmAbstractElementFactory;
	template< class T > friend class CDmaVar;
	template< class T >	friend class CDmaArray;
	template< class T > friend class CDmaElementArray;
	template< class T, class B > friend class CDmaDecorator;
	template< class T > friend class CDmrElementArray;

	friend class CDmElementFactoryDefault;
	friend class CDmeElementAccessor;
	friend class CDmeOperator;

	friend void CopyElements( const CUtlVector< CDmElement* > &from, CUtlVector< CDmElement* > &to, TraversalDepth_t depth );
};



inline void DestroyElement( CDmElement *pElement )
{
	if ( pElement )
	{
		g_pDataModel->DestroyElement( pElement->GetHandle() );
	}
}

void DestroyElement( CDmElement *pElement, TraversalDepth_t depth );


//-----------------------------------------------------------------------------
// copy groups of elements together so that references between them are maintained
//-----------------------------------------------------------------------------
void CopyElements( const CUtlVector< CDmElement* > &from, CUtlVector< CDmElement* > &to, TraversalDepth_t depth = TD_DEEP );


//-----------------------------------------------------------------------------
// allows elements to chain OnAttributeChanged up to their parents (or at least, referrers)
//-----------------------------------------------------------------------------
void InvokeOnAttributeChangedOnReferrers( DmElementHandle_t hElement, CDmAttribute *pChangedAttr );





//-----------------------------------------------------------------------------
// Returns the type, name, id, fileId
//-----------------------------------------------------------------------------
inline UtlSymId_t CDmElement::GetType() const 
{ 
	return m_Type;
}

inline const char *CDmElement::GetTypeString() const
{
	return g_pDataModel->GetString( m_Type );
}

inline const char *CDmElement::GetName() const 
{ 
	return m_Name.Get(); 
}

inline void CDmElement::SetName( const char* pName )
{
	m_Name.Set( pName );
}

inline const DmObjectId_t& CDmElement::GetId() const 
{ 
	return m_Id;
}

inline DmFileId_t CDmElement::GetFileId() const
{
	return m_fileId;
}


//-----------------------------------------------------------------------------
// Controls whether the element should be copied by default
//-----------------------------------------------------------------------------
inline void CDmElement::SetShared( bool bShared )
{
	if ( bShared )
	{
		SetValue< bool >( "shared", true );
	}
	else
	{
		RemoveAttribute( "shared" );
	}
}

inline bool CDmElement::IsShared() const
{
	return GetValue< bool >( "shared" ); // if attribute doesn't exist, returns default bool value, which is false
}


//-----------------------------------------------------------------------------
// Copies attributes from a specified element
//-----------------------------------------------------------------------------
inline CDmElement* CDmElement::Copy( TraversalDepth_t depth ) const
{
	return CopyInternal( depth );
}


//-----------------------------------------------------------------------------
// RTTI
//-----------------------------------------------------------------------------
inline bool CDmElement::IsA_Implementation( CUtlSymbol typeSymbol )
{
	return ( m_classType == typeSymbol ) || ( UTL_INVAL_SYMBOL == typeSymbol );
}

inline int CDmElement::GetInheritanceDepth_Implementation( CUtlSymbol typeSymbol, int nCurrentDepth )
{
	return IsA_Implementation( typeSymbol ) ? nCurrentDepth : -1;
}

inline CUtlSymbol CDmElement::GetStaticTypeSymbol()
{
	return m_classType;
}

inline bool CDmElement::IsA( const char *pTypeName ) const
{												
	CUtlSymbol typeSymbol = g_pDataModel->GetSymbol( pTypeName ); 
	return IsA( typeSymbol );				
}

template< class E > inline bool CDmElement::IsA() const		
{											
	return IsA( E::GetStaticTypeSymbol() ); 
}


//-----------------------------------------------------------------------------
// Helper for finding elements that refer to this element
//-----------------------------------------------------------------------------
template< class T >
T *FindReferringElement( CDmElement *pElement, const char *pAttrName, bool bMustBeInSameFile = true )
{
	return FindReferringElement< T >( pElement, g_pDataModel->GetSymbol( pAttrName ), bMustBeInSameFile );
}


void RemoveElementFromRefereringAttributes( CDmElement *pElement, bool bPreserveOrder = true );



//-----------------------------------------------------------------------------
//
// element-specific unique name generation methods
//
//-----------------------------------------------------------------------------

// returns startindex if none found, 2 if only "prefix" found, and n+1 if "prefixn" found
int GenerateUniqueNameIndex( const char *prefix, const CUtlVector< DmElementHandle_t > &array, int startindex = -1 );

bool GenerateUniqueName( char *name, int memsize, const char *prefix, const CUtlVector< DmElementHandle_t > &array );

void MakeElementNameUnique( CDmElement *pElement, const char *prefix, const CUtlVector< DmElementHandle_t > &array, bool forceIndex = false );


//-----------------------------------------------------------------------------
// helper for making attributevarelementarray cleanup easier
//-----------------------------------------------------------------------------
template< class T >
inline void CDmElement::DeleteAttributeVarElementArray( T &array )
{
	int nElements = array.Count();
	for ( int i = 0; i < nElements; ++i )
	{
		g_pDataModel->DestroyElement( array.GetHandle( i ) );
	}
	array.RemoveAll();
}


//-----------------------------------------------------------------------------
// Default size computation
//-----------------------------------------------------------------------------
template< class T >
int DmeEstimateMemorySize( T* pElement )
{
	return sizeof( T );
}


//-----------------------------------------------------------------------------
// Helper macro to create an element; this is used for elements that are helper base classes 
//-----------------------------------------------------------------------------
#define DEFINE_UNINSTANCEABLE_ELEMENT( className, baseClassName )	\
	protected:														\
		className( DmElementHandle_t handle, const char *pElementTypeName, const DmObjectId_t &id, const char *pElementName, DmFileId_t fileid ) :	\
			baseClassName( handle, pElementTypeName, id, pElementName, fileid )					\
		{																						\
		}																						\
		virtual ~className()																	\
		{																						\
		}																						\
		void OnConstruction();																	\
		void OnDestruction();																	\
		virtual void PerformConstruction()														\
		{																						\
			BaseClass::PerformConstruction();													\
			OnConstruction();																	\
		}																						\
		virtual void PerformDestruction()														\
		{																						\
			OnDestruction();																	\
			BaseClass::PerformDestruction();													\
		}																						\
		virtual int AllocatedSize() const { return DmeEstimateMemorySize( this ); }				\
																								\
	private:																					\
		typedef baseClassName BaseClass; 														\


//-----------------------------------------------------------------------------
// Helper macro to create the class factory 
//-----------------------------------------------------------------------------
#define DEFINE_ELEMENT( className, baseClassName )	\
	public:											\
		virtual bool IsA( UtlSymId_t typeSymbol ) const	\
		{											\
			return IsA_Implementation( typeSymbol );\
		}											\
													\
		bool IsA( const char *pTypeName ) const		\
		{											\
			CUtlSymbol typeSymbol = g_pDataModel->GetSymbol( pTypeName ); \
			return IsA( typeSymbol );				\
		}											\
													\
		template< class T > bool IsA() const		\
		{											\
			return IsA( T::GetStaticTypeSymbol() ); \
		}											\
													\
		virtual int GetInheritanceDepth( UtlSymId_t typeSymbol ) const	\
		{											\
			return GetInheritanceDepth_Implementation( typeSymbol, 0 );	\
		}											\
													\
		static CUtlSymbol GetStaticTypeSymbol( )	\
		{											\
			return m_classType;						\
		}											\
													\
		className* Copy( TraversalDepth_t depth = TD_DEEP ) const		\
		{																\
			return static_cast< className* >( CopyInternal( depth ) );	\
		}																\
	protected:															\
		className( DmElementHandle_t handle, const char *pElementTypeName, const DmObjectId_t &id, const char *pElementName, DmFileId_t fileid ) :	\
			baseClassName( handle, pElementTypeName, id, pElementName, fileid )					\
		{																						\
		}																						\
		virtual ~className()																	\
		{																						\
		}																						\
		void OnConstruction();																	\
		void OnDestruction();																	\
		virtual void PerformConstruction()														\
		{																						\
			BaseClass::PerformConstruction();													\
			OnConstruction();																	\
		}																						\
		virtual void PerformDestruction()														\
		{																						\
			OnDestruction();																	\
			BaseClass::PerformDestruction();													\
		}																						\
		static void SetTypeSymbol( CUtlSymbol typeSymbol )										\
		{																						\
			m_classType = typeSymbol;															\
		}																						\
																								\
		static bool IsA_Implementation( CUtlSymbol typeSymbol )									\
		{																						\
			if ( typeSymbol == m_classType )													\
				return true;																	\
			return BaseClass::IsA_Implementation( typeSymbol );									\
		}																						\
																								\
		static int GetInheritanceDepth_Implementation( CUtlSymbol typeSymbol, int nCurrentDepth )	\
		{																						\
			if ( typeSymbol == m_classType )													\
				return nCurrentDepth;															\
			return BaseClass::GetInheritanceDepth_Implementation( typeSymbol, nCurrentDepth+1 );\
		}																						\
		virtual int AllocatedSize() const { return DmeEstimateMemorySize( this ); }				\
																								\
	private:																					\
		typedef baseClassName BaseClass; 														\
		template <class T> friend class CDmElementFactory;										\
		template <class T> friend class CDmAbstractElementFactory;										\
		static CUtlSymbol m_classType

#define IMPLEMENT_ELEMENT( className ) \
	CUtlSymbol className::m_classType = UTL_INVAL_SYMBOL;


#endif // DMELEMENT_H
