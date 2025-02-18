//========== Copyright � 2008, Valve Corporation, All rights reserved. ========
//
// Purpose: Variants
//
// Overview
// --------
// Variants are structures which may contain a number of different data types
// You can query which data type is currently being used
//
//=============================================================================

#ifndef VARIANT_H
#define VARIANT_H

#if defined( COMPILER_MSVC )
#pragma once
#endif

#include "datamap.h"
#include "basehandle.h"
#include "tier1/strtools.h"
#include "tier1/utlstring.h"
#include "../../game/shared/ehandle.h"
//#include "tier1/utlstringtoken.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
FORWARD_DECLARE_HANDLE( HSCRIPT );


//-----------------------------------------------------------------------------
// Types not supported in datamap.h
//-----------------------------------------------------------------------------
enum ExtendedFieldType_t
{
	FIELD_TYPEUNKNOWN = FIELD_TYPECOUNT,
	FIELD_CSTRING,
	FIELD_HSCRIPT,
	FIELD_VARIANT,
	FIELD_UINT64,
	FIELD_FLOAT64,
	FIELD_POSITIVEINTEGER_OR_NULL,
	FIELD_HSCRIPT_NEW_INSTANCE,
	FIELD_UINT,				// Unsigned integer - it's not declared in fieldtype_t
	FIELD_UTLSTRINGTOKEN,
	FIELD_QANGLE,
};

DECLARE_FIELD_SIZE( FIELD_UINT64,		sizeof(uint64) )
DECLARE_FIELD_SIZE( FIELD_FLOAT64,		sizeof(float64) )
DECLARE_FIELD_SIZE( FIELD_POSITIVEINTEGER_OR_NULL, sizeof(int) )
DECLARE_FIELD_SIZE( FIELD_UINT,			sizeof( uint ) )
DECLARE_FIELD_SIZE( FIELD_UTLSTRINGTOKEN,		sizeof( uint32 ) )
DECLARE_FIELD_SIZE( FIELD_QANGLE,		sizeof( QAngle ) )

typedef int VariantDataType_t;



//---------------------------------------------------------
// A simple variant type. Intentionally not full featured (no implicit conversion, no memory management)
//---------------------------------------------------------
enum SVFlags_t
{
	SV_FREE = 0x01,
};

#pragma warning(push)
#pragma warning(disable:4800)

class CVariantDefaultAllocator
{
public:
	enum { ALWAYS_COPY = 0 };

	static void* Allocate( uint nSize )
	{
		return malloc( nSize );
	}

	static void Free( void *pMemory )
	{
		free( pMemory );
	}
};

template< class CValueAllocator = CVariantDefaultAllocator >
class CVariantBase
{
public:
	CVariantBase() :					m_flags( 0 ), m_type( FIELD_VOID )		{ m_pVector = 0; }
	CVariantBase( int val ) :			m_flags( 0 ), m_type( FIELD_INTEGER )	{ m_int = val; }
	CVariantBase( uint32 val ) :		m_flags( 0 ), m_type( FIELD_UINT )		{ m_uint = val; }
//	CVariantBase( int64 val ) :			m_flags( 0 ), m_type( FIELD_INTEGER64 )	{ m_int64 = val; }
	CVariantBase( uint64 val ) :		m_flags( 0 ), m_type( FIELD_UINT64 )	{ m_uint64 = val; }
	CVariantBase( float val ) :			m_flags( 0 ), m_type( FIELD_FLOAT )		{ m_float = val; }
	CVariantBase( float64 val ) :		m_flags( 0 ), m_type( FIELD_FLOAT64 )	{ m_float64 = val; }
	CVariantBase( char val ) :			m_flags( 0 ), m_type( FIELD_CHARACTER )	{ m_char = val; }
	CVariantBase( bool val ) :			m_flags( 0 ), m_type( FIELD_BOOLEAN )	{ m_bool = val; }
	CVariantBase( HSCRIPT val ) :		m_flags( 0 ), m_type( FIELD_HSCRIPT )	{ m_hScript = val; }
	CVariantBase( CBaseHandle val ) :	m_flags( 0 ), m_type( FIELD_EHANDLE )	{ m_hEntity = val.ToInt(); }
//	CVariantBase( CUtlStringToken val ) :	m_flags( 0 ), m_type( FIELD_UTLSTRINGTOKEN )	{ m_utlStringToken = val.m_nHashCode; }

	CVariantBase( const Vector &val, bool bCopy = false );
	CVariantBase( const Vector2D &val, bool bCopy = false );
//	CVariantBase( const Vector4D &val, bool bCopy = false );
	CVariantBase( const Vector *val, bool bCopy = false );
	CVariantBase( const char *val , bool bCopy = false );
	CVariantBase( const Quaternion &val, bool bCopy = false );

	CVariantBase( const CVariantBase<CValueAllocator> &variant ) :m_flags( 0 ), m_type( FIELD_VOID )	{ variant.AssignTo( this ); }
	void operator=( const CVariantBase<CValueAllocator> &variant )										{ variant.AssignTo( this ); }

	bool IsNull() const						{ return (m_type == FIELD_VOID ); }

	operator int() const					{ Assert( m_type == FIELD_INTEGER || m_type == FIELD_FLOAT );	return ( m_type == FIELD_INTEGER ) ? m_int : (int)m_float; }
	operator uint() const					{ Assert( m_type == FIELD_UINT );		return m_uint; }
//	operator int64() const					{ Assert( m_type == FIELD_INTEGER64 );	return m_int64; }
	operator uint64() const					{ Assert( m_type == FIELD_UINT64 );		return m_uint64; }
	operator float() const					{ ( m_type == FIELD_INTEGER || m_type == FIELD_FLOAT );		return ( m_type == FIELD_FLOAT ) ? m_float : (float)m_int; }
	operator float64() const				{ Assert( m_type == FIELD_FLOAT64 );	return m_float64; }
	operator const char *() const			{ Assert( m_type == FIELD_CSTRING );	return m_pszString; }
	operator const Vector2D &() const		{ Assert( m_type == FIELD_VECTOR2D );	return m_pData ? *(Vector2D*)m_pData : vec2_origin; }
	operator const Vector &() const			{ Assert( m_type == FIELD_VECTOR );		return (m_pVector) ? *m_pVector : vec3_origin; }
//	operator const Vector4D &() const		{ Assert( m_type == FIELD_VECTOR4D );	return m_pData ? *(Vector4D*)m_pData : vec4_origin; }
	operator const QAngle &() const			{ Assert( m_type == FIELD_QANGLE );		return m_pData ? *(QAngle*)m_pData : vec3_angle; }
	operator char() const					{ Assert( m_type == FIELD_CHARACTER );	return m_char; }
	operator bool() const					{ Assert( m_type == FIELD_BOOLEAN );	return m_bool; }
	operator HSCRIPT() const				{ Assert( m_type == FIELD_HSCRIPT );	return m_hScript; }
	operator CBaseHandle() const			{ Assert( m_type == FIELD_EHANDLE );	return CBaseHandle( m_hEntity ); }
        operator CBaseEntity*() const				{ Assert( m_type == FIELD_EHANDLE );    return CHandle<CBaseEntity>(CBaseHandle( m_hEntity )); }
	operator const Quaternion &() const		{ Assert( m_type == FIELD_QUATERNION );	return m_pData ? *(Quaternion*)m_pData : quat_identity; }
//	operator CUtlStringToken() const		{ Assert( m_type == FIELD_UTLSTRINGTOKEN );	CUtlStringToken t; t.m_nHashCode = m_utlStringToken; return t; }

	void operator=( int i ) 				{ Free(); m_type = FIELD_INTEGER; m_int = i; }
	void operator=( uint i ) 				{ Free(); m_type = FIELD_UINT; m_uint = i; }
//	void operator=( int64 i ) 				{ Free(); m_type = FIELD_INTEGER64; m_int64 = i; }
	void operator=( uint64 i ) 				{ Free(); m_type = FIELD_UINT64; m_uint64 = i; }
	void operator=( float f ) 				{ Free(); m_type = FIELD_FLOAT; m_float = f; }
	void operator=( float64 f ) 			{ Free(); m_type = FIELD_FLOAT64; m_float64 = f; }
	void operator=( const Vector2D &vec )	{ CopyData( vec ); }
	void operator=( const Vector &vec )		{ CopyData( vec ); }
//	void operator=( const Vector4D &vec )	{ CopyData( vec ); }
	void operator=( const QAngle &vec )		{ CopyData( vec ); }
	void operator=( const Quaternion &q )	{ CopyData( q ); }
	void operator=( const Vector2D *vec )	{ CopyData( *vec ); }
	void operator=( QAngle *a )				{ CopyData( *a ); }
	void operator=( const QAngle *a )		{ CopyData( *a ); }
	void operator=( Vector2D *vec )			{ CopyData( *vec ); }
	void operator=( const Vector *vec )		{ CopyData( *vec ); }
	void operator=( Vector *vec )			{ CopyData( *vec ); }
//	void operator=( const Vector4D *vec )	{ CopyData( *vec ); }
//	void operator=( Vector4D *vec )			{ CopyData( *vec ); }
	void operator=( const Quaternion *q )	{ CopyData( *q ); }
	void operator=( Quaternion *q )			{ CopyData( *q ); }
	void operator=( const char *psz )		{ CopyData( psz ); }
	void operator=( char c )				{ Free(); m_type = FIELD_CHARACTER; m_char = c; }
	void operator=( bool b ) 				{ Free(); m_type = FIELD_BOOLEAN; m_bool = b; }
	void operator=( HSCRIPT h ) 			{ Free(); m_type = FIELD_HSCRIPT; m_hScript = h; }
	void operator=( CBaseHandle h ) 		{ Free(); m_type = FIELD_EHANDLE; m_hEntity = h.ToInt(); }
//      void operator=( CBaseEntity *pEnt ) 	{  Free(); m_type = FIELD_EHANDLE; CBaseHandle h = pEnt; m_hEntity = h.ToInt(); }
//	void operator=( CUtlStringToken val ) 	{ Free(); m_type = FIELD_UTLSTRINGTOKEN; m_utlStringToken = val.m_nHashCode; }

	void Free();

	template <typename T>	T Get() const;
	template <typename T>	bool AssignTo( T *pDest ) const;
	bool AssignTo( float *pDest ) const;
	bool AssignTo( uint *pDest ) const;
	bool AssignTo( int *pDest ) const;
	bool AssignTo( bool *pDest ) const;
	bool AssignTo( Vector2D *pDest ) const;
	bool AssignTo( Vector *pDest ) const;
//	bool AssignTo( Vector4D *pDest ) const;
	bool AssignTo( QAngle *pDest ) const;
	bool AssignTo( Quaternion *pDest ) const;
	bool AssignTo( QuaternionAligned *pDest ) const { return AssignTo( (Quaternion*)pDest ); }
	bool AssignTo( VectorAligned *pDest ) const		{ return AssignTo( (Vector*)pDest ); }
	bool AssignTo( char *pDest, uint nBufLen ) const;
	bool AssignTo( CUtlString *pString ) const;
	bool AssignTo( const char **pszString ) const;
	bool AssignTo( HSCRIPT *pDest ) const;
	bool AssignTo( CBaseHandle *pDest ) const;
	bool AssignTo( CBaseEntity **pDest ) const;
//	bool AssignTo( CUtlStringToken *pDest ) const;
	template< typename T > bool AssignTo( CVariantBase<T> *pDest ) const;

	int GetType() const						{ return m_type; }
	uint16 GetFlags() const					{ return m_flags; }

	void ConvertToCopiedData(bool silent = false );

private:
	void *Allocate( uint nSize );
	template< typename T > T* Allocate();
	void Free( void* pMemory );
	template< typename T > void CopyData( const T &src, bool bForceCopy = false );
	void CopyData( const char *pString, bool bForceCopy = false );
	void CopyData( const Vector2D *src, bool bForceCopy = false );
	void CopyData( const Vector *src, bool bForceCopy = false );
//	void CopyData( const Vector4D *src, bool bForceCopy = false );
	void CopyData( const QAngle *src, bool bForceCopy = false );

	// This is not implemented. Use AssignTo( char *pDest, uint nBufLen ) instead.
	// Declaring this private to produce more sensible error messages.
	bool AssignTo( char **pDest ) const;

	union
	{
		int				m_int;
		uint			m_uint;
		float			m_float;
		const char *	m_pszString;
		const Vector *	m_pVector;
		void *			m_pData;
		char			m_char;
		bool			m_bool;
		HSCRIPT			m_hScript;
		uint32			m_hEntity;	// Can't use CBasEHANDLE in union because it has a nonempty constructor
		uint64			m_uint64;
		int64			m_int64;
		float64			m_float64;
//		uint32			m_utlStringToken; // Can't use CUtlStringToken in union because it has a nonempty constructor
	};

	int16				m_type;
	uint16				m_flags;

	friend class CLuaVM;
	friend class CPythonVM;
	friend class CSquirrelVM;
};

typedef CVariantBase<> CVariant;

template <typename T> struct VariantDeducer_t { enum { FIELD_TYPE = FIELD_TYPEUNKNOWN }; };
#define DECLARE_DEDUCE_FIELDTYPE( fieldType, type ) template<> struct VariantDeducer_t<type> { enum { FIELD_TYPE = fieldType }; };

DECLARE_DEDUCE_FIELDTYPE( FIELD_VOID,		void );
DECLARE_DEDUCE_FIELDTYPE( FIELD_FLOAT,		float );
//DECLARE_DEDUCE_FIELDTYPE( FIELD_INTEGER64,	int64 );
DECLARE_DEDUCE_FIELDTYPE( FIELD_UINT64,		uint64 );
DECLARE_DEDUCE_FIELDTYPE( FIELD_FLOAT64,	float64 );
DECLARE_DEDUCE_FIELDTYPE( FIELD_CSTRING,	const char * );
DECLARE_DEDUCE_FIELDTYPE( FIELD_CSTRING,	char * );
DECLARE_DEDUCE_FIELDTYPE( FIELD_VECTOR2D,	Vector2D );
DECLARE_DEDUCE_FIELDTYPE( FIELD_VECTOR2D,	const Vector2D & );
DECLARE_DEDUCE_FIELDTYPE( FIELD_VECTOR,		Vector );
DECLARE_DEDUCE_FIELDTYPE( FIELD_VECTOR,		const Vector & );
// DECLARE_DEDUCE_FIELDTYPE( FIELD_VECTOR4D,	Vector4D );
// DECLARE_DEDUCE_FIELDTYPE( FIELD_VECTOR4D,	const Vector4D & );
DECLARE_DEDUCE_FIELDTYPE( FIELD_QANGLE,		QAngle );
DECLARE_DEDUCE_FIELDTYPE( FIELD_QANGLE,		const QAngle & );
DECLARE_DEDUCE_FIELDTYPE( FIELD_INTEGER,	int );
DECLARE_DEDUCE_FIELDTYPE( FIELD_UINT,		uint );
DECLARE_DEDUCE_FIELDTYPE( FIELD_BOOLEAN,	bool );
DECLARE_DEDUCE_FIELDTYPE( FIELD_CHARACTER,	char );
DECLARE_DEDUCE_FIELDTYPE( FIELD_HSCRIPT,	HSCRIPT );
DECLARE_DEDUCE_FIELDTYPE( FIELD_VARIANT,	CVariant );
DECLARE_DEDUCE_FIELDTYPE( FIELD_EHANDLE,	CBaseHandle );
DECLARE_DEDUCE_FIELDTYPE( FIELD_QUATERNION,	Quaternion );
DECLARE_DEDUCE_FIELDTYPE( FIELD_QUATERNION,	const Quaternion & );
//DECLARE_DEDUCE_FIELDTYPE( FIELD_UTLSTRINGTOKEN,	CUtlStringToken );

#define VariantDeduceType( T ) ((fieldtype_t)VariantDeducer_t<T>::FIELD_TYPE)

template <typename T>
inline const char * VariantFieldTypeName() 
{
	return T::using_unknown_variant_type(); 
}

#define DECLARE_NAMED_FIELDTYPE( fieldType, strName ) template <> inline const char * VariantFieldTypeName<fieldType>() { return strName; }
DECLARE_NAMED_FIELDTYPE( void,	"void" );
DECLARE_NAMED_FIELDTYPE( float,	"float" );
DECLARE_NAMED_FIELDTYPE( const char *,	"cstring" );
DECLARE_NAMED_FIELDTYPE( char *,	"cstring" );
DECLARE_NAMED_FIELDTYPE( Vector2D,	"vector2d" );
DECLARE_NAMED_FIELDTYPE( const Vector2D&,	"vector2d" );
DECLARE_NAMED_FIELDTYPE( Vector,	"vector" );
DECLARE_NAMED_FIELDTYPE( const Vector&,	"vector" );
// DECLARE_NAMED_FIELDTYPE( Vector4D,	"vector4d" );
// DECLARE_NAMED_FIELDTYPE( const Vector4D&,	"vector4d" );
DECLARE_NAMED_FIELDTYPE( int,	"integer" );
DECLARE_NAMED_FIELDTYPE( int64,	"int64" );
DECLARE_NAMED_FIELDTYPE( uint64,"uint64" );
DECLARE_NAMED_FIELDTYPE( float64,	"float64" );
DECLARE_NAMED_FIELDTYPE( bool,	"boolean" );
DECLARE_NAMED_FIELDTYPE( char,	"character" );
DECLARE_NAMED_FIELDTYPE( HSCRIPT,	"hscript" );
DECLARE_NAMED_FIELDTYPE( CVariant,	"variant" );
DECLARE_NAMED_FIELDTYPE( CBaseHandle,	"ehandle" );
DECLARE_NAMED_FIELDTYPE( Quaternion,	"quaternion" );
//DECLARE_NAMED_FIELDTYPE( CUtlStringToken,	"utlstringtoken" );

inline const char * VariantFieldTypeName( int16 eType )
{
	switch( eType )
	{
	case FIELD_VOID:	return "void";
	case FIELD_FLOAT:	return "float";
	case FIELD_CSTRING:	return "cstring";
	case FIELD_VECTOR2D:return "vector2d";
	case FIELD_VECTOR:	return "vector";
//	case FIELD_VECTOR4D:return "vector4d";
	case FIELD_QANGLE:	return "qangle";
	case FIELD_INTEGER:	return "integer";
	case FIELD_UINT:	return "unsigned";
//	case FIELD_INTEGER64:	return "int64";
	case FIELD_UINT64:	return "uint64";
	case FIELD_FLOAT64:	return "float64";
	case FIELD_BOOLEAN:	return "boolean";
	case FIELD_CHARACTER:	return "character";
	case FIELD_HSCRIPT:	return "hscript";
	case FIELD_VARIANT:	return "variant";
	case FIELD_EHANDLE:	return "ehandle";
	case FIELD_QUATERNION:	return "quaternion";
	case FIELD_UTLSTRINGTOKEN:	return "utlstringtoken";

	default:	return "unknown_variant_type";
	}
}

typedef CVariantBase<> CVariant;

#define VARIANT_NULL CVariant()

#pragma warning(pop)


//-----------------------------------------------------------------------------
// Default allocator
//-----------------------------------------------------------------------------
template< class CValueAllocator >
inline void *CVariantBase<CValueAllocator>::Allocate( uint nSize )
{
	return CValueAllocator::Allocate( nSize );
}

template< class CValueAllocator >
template< typename T > 
inline T* CVariantBase<CValueAllocator>::Allocate()
{
//	ASSERT_MEMALLOC_WILL_ALIGN( T );
	return (T*)CValueAllocator::Allocate( sizeof(T) );
}

template< class CValueAllocator >
inline void CVariantBase<CValueAllocator>::Free( void* pMemory )
{
	return CValueAllocator::Free( pMemory );
}


//-----------------------------------------------------------------------------
// Constructors
//-----------------------------------------------------------------------------
template< class CValueAllocator >
inline CVariantBase<CValueAllocator>::CVariantBase( const Vector &val, bool bCopy ) : m_flags( 0 ), m_type( FIELD_VOID )
{ 
	CopyData( val, true );
}

template< class CValueAllocator >
inline CVariantBase<CValueAllocator>::CVariantBase( const Vector *val, bool bCopy ) : m_flags( 0 ), m_type( FIELD_VOID )
{ 
	CopyData( *val, bCopy );
}

template< class CValueAllocator >
inline CVariantBase<CValueAllocator>::CVariantBase( const Vector2D &val, bool bCopy ) : m_flags( 0 ), m_type( FIELD_VOID )
{ 
	CopyData( val, bCopy );
}

// template< class CValueAllocator >
// inline CVariantBase<CValueAllocator>::CVariantBase( const Vector4D &val, bool bCopy ) : m_flags( 0 ), m_type( FIELD_VOID )
// { 
// 	CopyData( val, bCopy );
// }

template< class CValueAllocator >
inline CVariantBase<CValueAllocator>::CVariantBase( const char *val, bool bCopy ) : m_flags( 0 ), m_type( FIELD_VOID )
{ 
	CopyData( val, bCopy );
}

template< class CValueAllocator >
inline CVariantBase<CValueAllocator>::CVariantBase( const Quaternion &val, bool bCopy ) : m_flags( 0 ), m_type( FIELD_VOID )
{
	CopyData( val, bCopy );
}


//-----------------------------------------------------------------------------
// Data freeing
//-----------------------------------------------------------------------------
template< class CValueAllocator >
inline void CVariantBase<CValueAllocator>::Free()								
{ 
	if ( m_flags & SV_FREE ) 
	{
		Free( m_pData );
		m_flags &= ~SV_FREE;
	}
	m_int64 = 0LL;
	m_type = FIELD_VOID;
}


//-----------------------------------------------------------------------------
// Copy helper
//-----------------------------------------------------------------------------
template< class CValueAllocator >
template< typename T > 
inline void CVariantBase<CValueAllocator>::CopyData( const T &src, bool bForceCopy )
{
	COMPILE_TIME_ASSERT( ( ExtendedFieldType_t )VariantDeduceType( T ) != FIELD_TYPEUNKNOWN );

	Free();
	m_type = VariantDeduceType( T );
	if ( CValueAllocator::ALWAYS_COPY || bForceCopy )
	{
		m_pData = Allocate<T>();
		*(T*)m_pData = src; 
		m_flags |= SV_FREE;
	}
	else
	{
		m_pData = const_cast< T* >( &src );
	}
}

template< class CValueAllocator >
inline void CVariantBase<CValueAllocator>::CopyData( const Vector2D *src, bool bForceCopy )
{
	Free();
	m_type = FIELD_VECTOR2D;
	if ( CValueAllocator::ALWAYS_COPY || bForceCopy )
	{
		m_pData = Allocate<Vector2D>();
		*(Vector2D*)m_pData = *src; 
		m_flags |= SV_FREE;
	}
	else
	{
		m_pData = (void *)src;
	}
}

template< class CValueAllocator >
inline void CVariantBase<CValueAllocator>::CopyData( const Vector *src, bool bForceCopy )
{
	Free();
	m_type = FIELD_VECTOR;
	if ( CValueAllocator::ALWAYS_COPY || bForceCopy )
	{
		m_pData = Allocate<Vector>();
		*(Vector*)m_pData = *src; 
		m_flags |= SV_FREE;
	}
	else
	{
		m_pData = (void *)src;
	}
}

// template< class CValueAllocator >
// inline void CVariantBase<CValueAllocator>::CopyData( const Vector4D *src, bool bForceCopy )
// {
// 	Free();
// 	m_type = FIELD_VECTOR4D;
// 	if ( CValueAllocator::ALWAYS_COPY || bForceCopy )
// 	{
// 		m_pData = Allocate<Vector4D>();
// 		*(Vector4D*)m_pData = *src; 
// 		m_flags |= SV_FREE;
// 	}
// 	else
// 	{
// 		m_pData = (void *)src;
// 	}
// }

template< class CValueAllocator >
inline void CVariantBase<CValueAllocator>::CopyData( const QAngle *src, bool bForceCopy )
{
	Free();
	m_type = FIELD_QANGLE;
	if ( CValueAllocator::ALWAYS_COPY || bForceCopy )
	{
		m_pData = Allocate<QAngle>();
		*(QAngle*)m_pData = *src; 
		m_flags |= SV_FREE;
	}
	else
	{
		m_pData = (void *)src;
	}
}

template< class CValueAllocator >
inline void CVariantBase<CValueAllocator>::CopyData( const char *pString, bool bForceCopy )
{
	Free();
	m_type = FIELD_CSTRING;
	if ( CValueAllocator::ALWAYS_COPY || bForceCopy )
	{
		int nLen = V_strlen( pString ) + 1;
		m_pszString = (char *)Allocate( nLen );
		memcpy( const_cast< char* >( m_pszString ), pString, nLen );
		m_flags |= SV_FREE; 
	}
	else
	{
		m_pszString = pString; 
	}
}

template< class CValueAllocator >
void CVariantBase<CValueAllocator>::ConvertToCopiedData( bool silent )
{
	if ( ( m_flags & SV_FREE ) == 0 )
	{
		switch( m_type )
		{
		case FIELD_CSTRING: CopyData( m_pszString, true ); break;
		case FIELD_VECTOR2D: CopyData( *(Vector2D*)m_pData, true ); break;
		case FIELD_VECTOR: CopyData( *m_pVector, true ); break;
//		case FIELD_VECTOR4D: CopyData( *(Vector4D*)m_pData, true ); break;
		case FIELD_QANGLE: CopyData( *(QAngle*)m_pData, true ); break;
		case FIELD_QUATERNION: CopyData( *(Quaternion*)m_pData, true ); break;
		default: 
			if (!silent)
				Warning( "Attempted to ConvertToCopiedData for unsupported type (%d)\n", m_type);
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Type converting get operations
//-----------------------------------------------------------------------------
template< class CValueAllocator >
template <typename T>
inline T CVariantBase<CValueAllocator>::Get() const
{
	T value{};
	AssignTo( &value );
	return value;
}

template< class CValueAllocator >
template <typename T>
inline bool CVariantBase<CValueAllocator>::AssignTo( T *pDest ) const
{
	VariantDataType_t destType = VariantDeduceType( T );
	if ( destType == FIELD_TYPEUNKNOWN )
	{
		Warning( "Unable to convert variant to unknown type\n" );
	}
	if ( destType == m_type )
	{
		*pDest = *this;
		return true;
	}

	if ( m_type != FIELD_VECTOR2D && m_type != FIELD_VECTOR && /*m_type != FIELD_VECTOR4D &&*/ m_type != FIELD_QANGLE &&m_type != FIELD_QUATERNION && m_type != FIELD_CSTRING && 
		destType != FIELD_VECTOR2D && destType != FIELD_VECTOR && /*destType != FIELD_VECTOR4D &&*/ destType != FIELD_QANGLE && destType != FIELD_QUATERNION && destType != FIELD_CSTRING )
	{
		switch ( m_type )
		{
		case FIELD_VOID:		*pDest = 0; break;
		case FIELD_INTEGER:		*pDest = m_int; return true;
		case FIELD_UINT:		*pDest = m_uint; return true;
		case FIELD_FLOAT:		*pDest = m_float; return true;
		case FIELD_CHARACTER:	*pDest = m_char; return true;
		case FIELD_BOOLEAN:		*pDest = m_bool; return true;

//		case FIELD_HSCRIPT:		*pDest = m_hScript; return true;
		}
	}
	else
	{
		Warning( "No free conversion of %s variant to %s right now\n",
			VariantFieldTypeName( m_type ), VariantFieldTypeName<T>() );
		if ( destType != FIELD_VECTOR )
		{
			*pDest = 0;
		}
	}
	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( HSCRIPT *pDest ) const
{
	switch( m_type )
	{
	case FIELD_HSCRIPT:	*pDest = m_hScript; return true;
	default:
		Warning( "No free conversion of %s variant to HSCRIPT right now\n", VariantFieldTypeName( m_type ) );
		break;
	}
	return false;
}

// we _could_ do crazy string conversions here, but probably shouldnt
template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( CBaseHandle *pDest ) const
{
	switch( m_type )
	{
	case FIELD_EHANDLE:	*pDest = CHandle< CBaseEntity >(m_hEntity); return true;
	default:
		Warning( "No free conversion of %s variant to EHANDLE right now\n", VariantFieldTypeName( m_type ) );
		break;
	}
	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( CBaseEntity **pDest ) const
{
	switch( m_type )
	{
	case FIELD_EHANDLE:	*pDest = CHandle< CBaseEntity >(m_hEntity); return true;
	default:
		Warning( "No free conversion of %s variant to CBaseEntity * right now\n", VariantFieldTypeName( m_type ) );
		break;
	}
	return false;
}

// template< class CValueAllocator >
// inline bool CVariantBase<CValueAllocator>::AssignTo( CUtlStringToken *pDest ) const
// {
// 	switch( m_type )
// 	{
// 	case FIELD_UTLSTRINGTOKEN:	pDest->m_nHashCode = m_utlStringToken; return true;
// 	case FIELD_CSTRING: *pDest = MakeStringToken( m_pszString ); return true;
// 	default:
// 		Warning( "No free conversion of %s variant to CUtlStringToken right now\n", VariantFieldTypeName( m_type ) );
// 		break;
// 	}
// 	return false;
// }

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( Vector2D *pDest ) const
{
	switch( m_type )
	{
	case FIELD_VOID:	*pDest = vec2_origin; return false;
	case FIELD_VECTOR2D:*pDest = *(Vector2D*)m_pData; return true;
	case FIELD_CSTRING:
		{
			int nParsed = sscanf( m_pszString, "%f %f", &pDest->x, &pDest->y );
			if ( nParsed == 2 )
				return true;
			*pDest = vec2_origin; 
			return false;
		}
		break;

	default:
		Warning( "No free conversion of %s variant to Vector2D right now\n", VariantFieldTypeName( m_type ) );
		break;
	}
	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( Vector *pDest ) const
{
	switch( m_type )
	{
	case FIELD_VOID:	*pDest = vec3_origin; return false;
	case FIELD_QANGLE:	*pDest = *(Vector*)m_pData; return true;	// Should we allow this free assignment?
	case FIELD_VECTOR:	*pDest = *m_pVector; return true;
	case FIELD_CSTRING:
		{
			int nParsed = sscanf( m_pszString, "%f %f %f", &pDest->x, &pDest->y, &pDest->z );
			if ( nParsed == 3 )
				return true;
			*pDest = vec3_origin; 
			return false;
		}
		break;

	default:
		Warning( "No free conversion of %s variant to Vector right now\n", VariantFieldTypeName( m_type ) );
		break;
	}
	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( QAngle *pDest ) const
{
	switch( m_type )
	{
	case FIELD_VOID:	*pDest = vec3_angle; return false;
	case FIELD_VECTOR:	*pDest = *(QAngle*)m_pVector; return true;	// Should we allow this free assignment?
	case FIELD_QANGLE:	*pDest = *(QAngle*)m_pData; return true;
	case FIELD_CSTRING:
		{
			int nParsed = sscanf( m_pszString, "%f %f %f", &pDest->x, &pDest->y, &pDest->z );
			if ( nParsed == 3 )
				return true;
			*pDest = vec3_angle; 
			return false;
		}
		break;

	case FIELD_QUATERNION:
		QuaternionAngles( *(Quaternion*)m_pData, *pDest );
		return true;

	default:
		Warning( "No free conversion of %s variant to QAngle right now\n", VariantFieldTypeName( m_type ) );
		break;
	}
	return false;
}

// template< class CValueAllocator >
// inline bool CVariantBase<CValueAllocator>::AssignTo( Vector4D *pDest ) const
// {
// 	switch( m_type )
// 	{
// 	case FIELD_VOID:	*pDest = vec4_origin; return false;
// 	case FIELD_VECTOR4D:*pDest = *(Vector4D*)m_pData; return true;
// 	case FIELD_CSTRING:
// 		{
// 			int nParsed = sscanf( m_pszString, "%f %f %f %f", &pDest->x, &pDest->y, &pDest->z, &pDest->w );
// 			if ( nParsed == 4 )
// 				return true;
// 			*pDest = vec4_origin; 
// 			return false;
// 		}
// 		break;
// 
// 	default:
// 		Warning( "No free conversion of %s variant to Vector4D right now\n", VariantFieldTypeName( m_type ) );
// 		break;
// 	}
// 	return false;
// }

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( Quaternion *pDest ) const
{
	switch( m_type )
	{
	case FIELD_VOID:		*pDest = quat_identity; return false;
	case FIELD_QUATERNION:	*pDest = *(Quaternion*)m_pData; return true;
	case FIELD_CSTRING:
		{
			int nParsed = sscanf( m_pszString, "%f %f %f %f", &pDest->x, &pDest->y, &pDest->z, &pDest->w );
			if ( nParsed == 4 )
				return true;
			*pDest = quat_identity; 
			return false;
		}
		break;

	case FIELD_QANGLE:
		AngleQuaternion( *(QAngle*)m_pData, *pDest );
		return true;

	default:
		Warning( "No free conversion of %s variant to Quaternion right now\n", VariantFieldTypeName( m_type ) );
		break;
	}
	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( float *pDest ) const
{
	switch( m_type )
	{
	case FIELD_VOID:		*pDest = 0; return false;
	case FIELD_INTEGER:		*pDest = m_int; return true;
	case FIELD_UINT:		*pDest = m_uint; return true;
	case FIELD_FLOAT:		*pDest = m_float; return true;
	case FIELD_FLOAT64:		*pDest = m_float64; return true;
	case FIELD_BOOLEAN:		*pDest = m_bool; return true;
	default:
		Warning( "No conversion from %s to float now\n", VariantFieldTypeName( m_type ) );
		return false;
	}
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( int *pDest ) const
{
	switch( m_type )
	{
	case FIELD_VOID:		*pDest = 0; return false;
	case FIELD_INTEGER:		*pDest = m_int; return true;
	case FIELD_UINT:		*pDest = m_uint; return true;
	case FIELD_FLOAT:		*pDest = m_float; return true;
	case FIELD_BOOLEAN:		*pDest = m_bool; return true;
	case FIELD_CSTRING:		*pDest = atoi( m_pszString ); return true;
	default:
		Warning( "No conversion from %s to int now\n", VariantFieldTypeName( m_type ) );
		return false;
	}
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( uint *pDest ) const
{
	switch( m_type )
	{
	case FIELD_VOID:		*pDest = 0; return false;
//	case FIELD_INTEGER64:	*pDest = (uint)clamp( m_int, UINT_MIN, UINT_MAX ); return true;
	case FIELD_UINT64:		*pDest = (uint)clamp( m_uint64, 0/*UINT_MIN*/, UINT_MAX ); return true;
	case FIELD_BOOLEAN:		*pDest = m_bool; return true;
	case FIELD_INTEGER:		if ( m_int < 0 ) return false; *pDest = (uint)clamp( m_int, 0, UINT_MAX ); return true;
	case FIELD_UINT:		*pDest = m_uint; return true;
	default:
		Warning( "No conversion from %s to int now\n", VariantFieldTypeName( m_type ) );
		return false;
	}
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( bool *pDest ) const
{
	switch( m_type )
	{
	case FIELD_VOID:		*pDest = 0; return false;
	case FIELD_INTEGER:		*pDest = (m_int != 0); return true;
	case FIELD_UINT:		*pDest = (m_uint != 0); return true;
	case FIELD_FLOAT:		*pDest = (m_float != 0.0f); return true;
	case FIELD_BOOLEAN:		*pDest = m_bool; return true;

	case FIELD_CSTRING:
		if( !V_strcmp( m_pszString, "0" ) || !V_strcmp( m_pszString, "false" ) || !V_strcmp( m_pszString, "no" ) )
		{
			*pDest = false;
			return true;
		}
		else if( !V_strcmp( m_pszString, "1" ) || !V_strcmp( m_pszString, "true" ) || !V_strcmp( m_pszString, "yes" ) )
		{
			*pDest = true;
			return true;
		}
		else
		{
			Warning("Invalid conversion : CString '%s' to bool\n", m_pszString );
			return false;
		}

	default:
		Warning( "No conversion from %s to bool now\n", VariantFieldTypeName( m_type ) );
		return false;
	}
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( char *pDest, uint nBufLen ) const
{
	switch( m_type )
	{
	case FIELD_VOID:		*pDest = 0; return false;
	case FIELD_INTEGER:		V_snprintf( pDest, nBufLen, "%d", m_int ); return true;
	case FIELD_UINT:		V_snprintf( pDest, nBufLen, "%u", m_uint ); return true;
	case FIELD_FLOAT:		V_snprintf( pDest, nBufLen, "%f", m_float ); return true;
	case FIELD_FLOAT64:		V_snprintf( pDest, nBufLen, "%f", m_float64 ); return true;
	case FIELD_BOOLEAN:		V_snprintf( pDest, nBufLen, "%s", m_bool ? "true" : "false" ); return true;
	case FIELD_CHARACTER:	V_snprintf( pDest, nBufLen, "%c", m_char ); return true;
	case FIELD_CSTRING:		V_strncpy( pDest, m_pszString, nBufLen ); return true;
	case FIELD_VECTOR2D:
		{ 
			Vector2D *q = (Vector2D*)m_pData; 
			V_snprintf( pDest, nBufLen, "%f %f", q->x, q->y ); 
			return true;
		}

	case FIELD_VECTOR:		V_snprintf( pDest, nBufLen, "%f %f %f", m_pVector->x, m_pVector->y, m_pVector->z ); return true;
	case FIELD_QANGLE:
		{
			QAngle *q = (QAngle*)m_pData; 
			V_snprintf( pDest, nBufLen, "%f %f %f", q->x, q->y, q->z ); 
			return true;
		}
// 	case FIELD_VECTOR4D:
// 		{ 
// 			Vector4D *q = (Vector4D*)m_pData; 
// 			V_snprintf( pDest, nBufLen, "%f %f %f %f", q->x, q->y, q->z, q->w ); 
// 			return true;
// 		}
	case FIELD_QUATERNION:	
		{ 
			Quaternion *q = (Quaternion*)m_pData; 
			V_snprintf( pDest, nBufLen, "%f %f %f %f", q->x, q->y, q->z, q->w ); 
			return true;
		}
	}

	Warning( "No conversion from %s to string at the moment!\n", VariantFieldTypeName( m_type ) );
	*pDest = 0;
	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( CUtlString *pString ) const
{
	int nLen = ( m_type != FIELD_CSTRING ) ? 256 : V_strlen( m_pszString ) + 1;
	pString->SetLength( nLen );
	return AssignTo( pString->GetForModify(), uint( nLen ) );
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( const char **pszString ) const
{
	if ( m_type != FIELD_CSTRING )
	{
		Warning( "CVariantBase<CValueAllocator>::AssignTo: Using const char * but type was not FIELD_CSTRING. You might want to use CUtlString instead or the script passed an invalid param to a string param/table. Returning NULL.\n" );
		return NULL;
	}

	return m_pszString;
}


template< class CValueAllocator >
template< class T >
inline bool CVariantBase<CValueAllocator>::AssignTo( CVariantBase<T> *pDest ) const
{
	pDest->Free();
	switch ( m_type )
	{
	case FIELD_CSTRING:
		pDest->CopyData( m_pszString, true );
		return true;
	
	case FIELD_VECTOR2D:
		pDest->CopyData( *(Vector2D*)m_pData, true );
		return true;

	case FIELD_VECTOR:
		pDest->CopyData( m_pVector, true );
		return true;

// 	case FIELD_VECTOR4D:
// 		pDest->CopyData( *(Vector4D*)m_pData, true );
// 		return true;

	case FIELD_QANGLE:
		pDest->CopyData( *(QAngle*)m_pData, true );
		return true;

	case FIELD_QUATERNION:
		pDest->CopyData( *(Quaternion*)m_pData, true );
		return true;
	
	default:
		pDest->m_type = m_type;
		memcpy( &pDest->m_int64, &m_int64, sizeof( m_int64 ) );
		return true;
	}
}

// overloading == as an external function
template< class CValueAllocator1, class CValueAllocator2 >
inline bool operator ==( const CVariantBase<CValueAllocator1> & v1, const CVariantBase<CValueAllocator2>& v2)
{
	if ( v1.m_type != v2.m_type )
		return false;

	switch ( v1.m_type )
	{
	case FIELD_INTEGER:		{ return v1.m_int == v2.m_int; }
	case FIELD_UINT:		{ return v1.m_uint == v2.m_uint; }
	case FIELD_FLOAT:		{ return v1.m_float == v2.m_float; }
	case FIELD_CHARACTER:	{ return v1.m_char == v2.m_char; }
	case FIELD_BOOLEAN:		{ return v1.m_bool == v2.m_bool; }
	case FIELD_HSCRIPT:		{ return v1.m_hScript == v2.m_hScript; }
	case FIELD_UTLSTRINGTOKEN:	{ return v1.m_utlStringToken == v2.m_utlStringToken; }
	}

	return false;
}

// overloading != as an external function
template< class CValueAllocator1, class CValueAllocator2 >
inline bool operator !=( const CVariantBase<CValueAllocator1> & v1, const CVariantBase<CValueAllocator2>& v2)
{
	return !( v1 == v2 );
}


#endif // VARIANT_H
