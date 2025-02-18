//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: common helpers for reuse among various Utl containers
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef UTLCOMMON_H
#define UTLCOMMON_H
#pragma once

//-----------------------------------------------------------------------------
// Henry Goffin (henryg) was here. Questions? Bugs? Go slap him around a bit.
//-----------------------------------------------------------------------------

// empty_t is the canonical "no-value" type which is fully defined but empty.
struct empty_t {};

// undefined_t is the canonical "undefined" type, used mostly for typedefs;
// parameters of type undefined_t will not compile, which is actually useful
// behavior when it comes to template programming. Google "SFINAE" for info.
struct undefined_t;

// CTypeSelect<sel,A,B>::type is a typedef of A if sel is nonzero, else B
template <int sel, typename A, typename B>
struct CTypeSelect { typedef A type; };

template <typename A, typename B>
struct CTypeSelect<0, A, B> { typedef B type; };

// CTypeEquals<A, B>::value is nonzero if A and B are the same type
template <typename A, typename B, bool bIgnoreConstVolatile = false, bool bIgnoreReference = false>
struct CTypeEquals { enum { value = 0 }; };

template <typename Same>
struct CTypeEquals<Same, Same, false, false> { enum { value = 1 }; };

template <typename A, typename B>
struct CTypeEquals<A, B, true, true> : CTypeEquals< const volatile A&, const volatile B& > {};

template <typename A, typename B>
struct CTypeEquals<A, B, true, false> : CTypeEquals< const volatile A, const volatile B > {};

template <typename A, typename B>
struct CTypeEquals<A, B, false, true> : CTypeEquals< A&, B& > {};

// CUtlKeyValuePair is intended for use with key-lookup containers.
// Because it is specialized for "empty_t" values, one container can
// function as either a set of keys OR a key-value dictionary while
// avoiding storage waste or padding for the empty_t value objects.
template <typename K, typename V>
class CUtlKeyValuePair
{
public:
	typedef V ValueReturn_t;
	K m_key;
	V m_value;

	CUtlKeyValuePair() {}

	template < typename KInit >
	explicit CUtlKeyValuePair( const KInit &k ) : m_key( k ) {}

	template < typename KInit, typename VInit >
	CUtlKeyValuePair( const KInit &k, const VInit &v ) : m_key( k ), m_value( v ) {}

	V &GetValue() { return m_value; }
	const V &GetValue() const { return m_value; }
};

template <typename K>
class CUtlKeyValuePair<K, empty_t>
{
public:
	typedef const K ValueReturn_t;
	K m_key;

	CUtlKeyValuePair() {}

	template < typename KInit >
	explicit CUtlKeyValuePair( const KInit &k ) : m_key( k ) {}

	template < typename KInit >
	CUtlKeyValuePair( const KInit &k, empty_t ) : m_key( k ) {}

	CUtlKeyValuePair( const K &k, const empty_t& ) : m_key( k ) {}
	const K &GetValue() const { return m_key; }
};


// Default functors. You can specialize these if your type does
// not implement operator== or operator< in an efficient way for
// some odd reason.
template <typename T> struct DefaultLessFunctor;
template <typename T> struct DefaultEqualFunctor;

// Hashing functor used by hash tables. You can either specialize
// for types which are widely used, or plug a custom functor directly
// into the hash table. If you do roll your own, please read up on
// bit-mixing and the avalanche property; be sure that your values
// are reasonably well-distributed across the entire 32-bit range.
//  http://en.wikipedia.org/wiki/Avalanche_effect
//  http://home.comcast.net/~bretm/hash/5.html
// 
template <typename T> struct DefaultHashFunctor;

// Argument type information. Struct currently contains one or two typedefs:
//   typename Arg_t = primary argument type. Usually const T&, sometimes T.
//   typename Alt_t = optional alternate type. Usually *undefined*.
//
// Any specializations should be implemented via simple inheritance
// from ArgumentTypeInfoImpl< BestArgType, [optional] AlternateArgType >
//
template <typename T> struct ArgumentTypeInfo;


// Some fundamental building-block functors...
struct StringLessFunctor { bool operator()( const char *a, const char *b ) const { return Q_strcmp( a, b ) < 0; } };
struct StringEqualFunctor { bool operator()( const char *a, const char *b ) const { return Q_strcmp( a, b ) == 0; } };
struct CaselessStringLessFunctor { bool operator()( const char *a, const char *b ) const { return Q_strcasecmp( a, b ) < 0; } };
struct CaselessStringEqualFunctor { bool operator()( const char *a, const char *b ) const { return Q_strcasecmp( a, b ) == 0; } };

struct Mix32HashFunctor { unsigned int operator()( uint32 s ) const; };
struct Mix64HashFunctor { unsigned int operator()( uint64 s ) const; };
struct StringHashFunctor { unsigned int operator()( const char* s ) const; };
struct CaselessStringHashFunctor { unsigned int operator()( const char* s ) const; };

struct PointerLessFunctor { bool operator()( const void *a, const void *b ) const { return a < b; } };
struct PointerEqualFunctor { bool operator()( const void *a, const void *b ) const { return a == b; } };
#if defined( PLATFORM_64BITS )
struct PointerHashFunctor { unsigned int operator()( const void* s ) const { return Mix64HashFunctor()( ( uintp ) s ); } };
#else
struct PointerHashFunctor { unsigned int operator()( const void* s ) const { return Mix32HashFunctor()( ( uintp ) s ); } };
#endif


// Generic implementation of Less and Equal functors
template < typename T >
struct DefaultLessFunctor
{
	bool operator()( typename ArgumentTypeInfo< T >::Arg_t a, typename ArgumentTypeInfo< T >::Arg_t b ) const { return a < b; }
	bool operator()( typename ArgumentTypeInfo< T >::Alt_t a, typename ArgumentTypeInfo< T >::Arg_t b ) const { return a < b; }
	bool operator()( typename ArgumentTypeInfo< T >::Arg_t a, typename ArgumentTypeInfo< T >::Alt_t b ) const { return a < b; }
};

template < typename T >
struct DefaultEqualFunctor
{
	bool operator()( typename ArgumentTypeInfo< T >::Arg_t a, typename ArgumentTypeInfo< T >::Arg_t b ) const { return a == b; }
	bool operator()( typename ArgumentTypeInfo< T >::Alt_t a, typename ArgumentTypeInfo< T >::Arg_t b ) const { return a == b; }
	bool operator()( typename ArgumentTypeInfo< T >::Arg_t a, typename ArgumentTypeInfo< T >::Alt_t b ) const { return a == b; }
};

// Hashes for basic types
template <> struct DefaultHashFunctor<char> : Mix32HashFunctor { };
template <> struct DefaultHashFunctor<signed char> : Mix32HashFunctor { };
template <> struct DefaultHashFunctor<unsigned char> : Mix32HashFunctor { };
template <> struct DefaultHashFunctor<signed short> : Mix32HashFunctor { };
template <> struct DefaultHashFunctor<unsigned short> : Mix32HashFunctor { };
template <> struct DefaultHashFunctor<signed int> : Mix32HashFunctor { };
template <> struct DefaultHashFunctor<unsigned int> : Mix32HashFunctor { };
#if !defined(PLATFORM_64BITS) || defined(_WIN32)
template <> struct DefaultHashFunctor<signed long> : Mix32HashFunctor { };
template <> struct DefaultHashFunctor<unsigned long> : Mix32HashFunctor { };
#elif defined(POSIX)
template <> struct DefaultHashFunctor<signed long> : Mix64HashFunctor { };
template <> struct DefaultHashFunctor<unsigned long> : Mix64HashFunctor { };
#endif
template <> struct DefaultHashFunctor<signed long long> : Mix64HashFunctor { };
template <> struct DefaultHashFunctor<unsigned long long> : Mix64HashFunctor { };
template <> struct DefaultHashFunctor<void*> : PointerHashFunctor { };
template <> struct DefaultHashFunctor<const void*> : PointerHashFunctor { };
#if !defined(_MSC_VER) || defined(_NATIVE_WCHAR_T_DEFINED)
template <> struct DefaultHashFunctor<wchar_t> : Mix32HashFunctor { };
#endif

// String specializations. If you want to operate on raw values, use
// PointerLessFunctor and friends from the "building-block" section above
template <> struct DefaultLessFunctor<char*> : StringLessFunctor { };
template <> struct DefaultLessFunctor<const char*> : StringLessFunctor { };
template <> struct DefaultEqualFunctor<char*> : StringEqualFunctor { };
template <> struct DefaultEqualFunctor<const char*> : StringEqualFunctor { };
template <> struct DefaultHashFunctor<char*> : StringHashFunctor { };
template <> struct DefaultHashFunctor<const char*> : StringHashFunctor { };

// CUtlString/CUtlConstString are specialized here and not in utlstring.h
// because I consider string datatypes to be fundamental, and don't feel
// comfortable making that header file dependent on this one. (henryg)
class CUtlString;
template < typename T > class CUtlConstStringBase;

template <> struct DefaultLessFunctor<CUtlString> : StringLessFunctor { };
template <> struct DefaultHashFunctor<CUtlString> : StringHashFunctor { };
template < typename T > struct DefaultLessFunctor< CUtlConstStringBase<T> > : StringLessFunctor { };
template < typename T > struct DefaultHashFunctor< CUtlConstStringBase<T> > : StringHashFunctor { };


// Helpers to deduce if a type defines a public AltArgumentType_t typedef:
template < typename T >
struct HasClassAltArgumentType
{
	template < typename X > static long Test( typename X::AltArgumentType_t* );
	template < typename X > static char Test( ... );
	enum { value = ( sizeof( Test< T >( NULL ) ) != sizeof( char ) ) };
};

template < typename T, bool = HasClassAltArgumentType< T >::value >
struct GetClassAltArgumentType { typedef typename T::AltArgumentType_t Result_t; };

template < typename T >
struct GetClassAltArgumentType< T, false > { typedef undefined_t Result_t; };

// Unwrap references; reference types don't have member typedefs.
template < typename T >
struct GetClassAltArgumentType< T&, false > : GetClassAltArgumentType< T > { };

// ArgumentTypeInfoImpl is the base for all ArgumentTypeInfo specializations.
template < typename ArgT, typename AltT = typename GetClassAltArgumentType<ArgT>::Result_t >
struct ArgumentTypeInfoImpl
{
	enum { has_alt = 1 };
	typedef ArgT Arg_t;
	typedef AltT Alt_t;
};

// Handle cases where AltArgumentType_t is typedef'd to undefined_t
template < typename ArgT >
struct ArgumentTypeInfoImpl< ArgT, undefined_t >
{
	enum { has_alt = 0 };
	typedef ArgT Arg_t;
	typedef undefined_t Alt_t;
};

// Handle cases where AltArgumentType_t is typedef'd to the primary type
template < typename ArgT >
struct ArgumentTypeInfoImpl< ArgT, ArgT >
{
	enum { has_alt = 0 };
	typedef ArgT Arg_t;
	typedef undefined_t Alt_t;
};


// By default, everything is passed via const ref and doesn't define an alternate type.
template <typename T> struct ArgumentTypeInfo : ArgumentTypeInfoImpl< const T& > { };

// Small native types are most efficiently passed by value.
template <> struct ArgumentTypeInfo< bool > : ArgumentTypeInfoImpl< bool > { };
template <> struct ArgumentTypeInfo< char > : ArgumentTypeInfoImpl< char > { };
template <> struct ArgumentTypeInfo< signed char > : ArgumentTypeInfoImpl< signed char > { };
template <> struct ArgumentTypeInfo< unsigned char > : ArgumentTypeInfoImpl< unsigned char > { };
template <> struct ArgumentTypeInfo< signed short > : ArgumentTypeInfoImpl< signed short > { };
template <> struct ArgumentTypeInfo< unsigned short > : ArgumentTypeInfoImpl< unsigned short > { };
template <> struct ArgumentTypeInfo< signed int > : ArgumentTypeInfoImpl< signed int > { };
template <> struct ArgumentTypeInfo< unsigned int > : ArgumentTypeInfoImpl< unsigned int > { };
template <> struct ArgumentTypeInfo< signed long > : ArgumentTypeInfoImpl< signed long > { };
template <> struct ArgumentTypeInfo< unsigned long > : ArgumentTypeInfoImpl< unsigned long > { };
template <> struct ArgumentTypeInfo< signed long long > : ArgumentTypeInfoImpl< signed long long > { };
template <> struct ArgumentTypeInfo< unsigned long long > : ArgumentTypeInfoImpl< unsigned long long > { };
template <> struct ArgumentTypeInfo< float > : ArgumentTypeInfoImpl< float > { };
template <> struct ArgumentTypeInfo< double > : ArgumentTypeInfoImpl< double > { };
template <> struct ArgumentTypeInfo< long double > : ArgumentTypeInfoImpl< long double > { };
#if !defined(_MSC_VER) || defined(_NATIVE_WCHAR_T_DEFINED)
template <> struct ArgumentTypeInfo< wchar_t > : ArgumentTypeInfoImpl< wchar_t > { };
#endif

// Pointers are also most efficiently passed by value.
template < typename T > struct ArgumentTypeInfo< T* > : ArgumentTypeInfoImpl< T* > { };


// Specializations to unwrap const-decorated types and references
template <typename T> struct ArgumentTypeInfo<const T> : ArgumentTypeInfo<T> { };
template <typename T> struct ArgumentTypeInfo<volatile T> : ArgumentTypeInfo<T> { };
template <typename T> struct ArgumentTypeInfo<const volatile T> : ArgumentTypeInfo<T> { };
template <typename T> struct ArgumentTypeInfo<T&> : ArgumentTypeInfo<T> { };

template <typename T> struct DefaultLessFunctor<const T> : DefaultLessFunctor<T> { };
template <typename T> struct DefaultLessFunctor<volatile T> : DefaultLessFunctor<T> { };
template <typename T> struct DefaultLessFunctor<const volatile T> : DefaultLessFunctor<T> { };
template <typename T> struct DefaultLessFunctor<T&> : DefaultLessFunctor<T> { };

template <typename T> struct DefaultEqualFunctor<const T> : DefaultEqualFunctor<T> { };
template <typename T> struct DefaultEqualFunctor<volatile T> : DefaultEqualFunctor<T> { };
template <typename T> struct DefaultEqualFunctor<const volatile T> : DefaultEqualFunctor<T> { };
template <typename T> struct DefaultEqualFunctor<T&> : DefaultEqualFunctor<T> { };

template <typename T> struct DefaultHashFunctor<const T> : DefaultHashFunctor<T> { };
template <typename T> struct DefaultHashFunctor<volatile T> : DefaultHashFunctor<T> { };
template <typename T> struct DefaultHashFunctor<const volatile T> : DefaultHashFunctor<T> { };
template <typename T> struct DefaultHashFunctor<T&> : DefaultHashFunctor<T> { };


// Hash all pointer types as raw pointers by default
template <typename T> struct DefaultHashFunctor< T * > : PointerHashFunctor { };


// Here follow the useful implementations.

// Bob Jenkins's 32-bit mix function.
inline unsigned int Mix32HashFunctor::operator()( uint32 n ) const
{
	// Perform a mixture of the bits in n, where each bit
	// of the input value has an equal chance to affect each
	// bit of the output. This turns tightly clustered input
	// values into a smooth distribution.
	//
	// This takes 16-20 cycles on modern x86 architectures;
	// that's roughly the same cost as a mispredicted branch.
	// It's also reasonably efficient on PPC-based consoles.
	//
	// If you're still thinking, "too many instructions!",
	// do keep in mind that reading one byte of uncached RAM
	// is about 30x slower than executing this code. It pays
	// to have a good hash function which minimizes collisions
	// (and therefore long lookup chains).
	n = ( n + 0x7ed55d16 ) + ( n << 12 );
	n = ( n ^ 0xc761c23c ) ^ ( n >> 19 );
	n = ( n + 0x165667b1 ) + ( n << 5 );
	n = ( n + 0xd3a2646c ) ^ ( n << 9 );
	n = ( n + 0xfd7046c5 ) + ( n << 3 );
	n = ( n ^ 0xb55a4f09 ) ^ ( n >> 16 );
	return n;
}

inline unsigned int Mix64HashFunctor::operator()( uint64 s ) const 
{
	// Thomas Wang hash, http://www.concentric.net/~ttwang/tech/inthash.htm
	s = ( ~s ) + ( s << 21 ); // s = (s << 21) - s - 1;
	s = s ^ ( s >> 24 );
	s = (s + ( s << 3 ) ) + ( s << 8 ); // s * 265
	s = s ^ ( s >> 14 );
	s = ( s + ( s << 2 ) ) + ( s << 4 ); // s * 21
	s = s ^ ( s >> 28 );
	s = s + ( s << 31 );
	return (unsigned int)s;
}


// Based on the widely-used FNV-1A string hash with a final
// mixing step to improve dispersion for very small and very
// large hash table sizes.
inline unsigned int StringHashFunctor::operator()( const char* s ) const
{
	uint32 h = 2166136261u;
	for ( ; *s; ++s )
	{
		uint32 c = (unsigned char) *s;
		h = (h ^ c) * 16777619;
	}
	return (h ^ (h << 17)) + (h >> 21);
}

// Equivalent to StringHashFunctor on lower-case strings.
inline unsigned int CaselessStringHashFunctor::operator()( const char* s ) const
{
	uint32 h = 2166136261u;
	for ( ; *s; ++s )
	{
		uint32 c = (unsigned char) *s;
		// Brutally fast branchless ASCII tolower():
		// if ((c >= 'A') && (c <= 'Z')) c += ('a' - 'A');
		c += (((('A'-1) - c) & (c - ('Z'+1))) >> 26) & 32;
		h = (h ^ c) * 16777619;
	}
	return (h ^ (h << 17)) + (h >> 21);
}


#endif // UTLCOMMON_H
