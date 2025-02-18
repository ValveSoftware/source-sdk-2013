//========= Copyright (c) Valve Corporation. All Rights Reserved. ============//

#pragma once

#include <type_traits>

// is_copy_cheap is for types that can be moved around as efficiently as
// a scalar.  (E.g. they fit in a register, copy trivially, etc.)
//
// By default, we just assume that T is can be copied cheaply if it's
// trivially copyable and 8 bytes or smaller.  If you have a class
// that you feel is cheap to copy but doesn't meet this criterial,
// than you can tag your type as being cheap to copy like this::
//
//	template<> struct is_copy_cheap<MyType> : public std::true_type {}
//
// Note that a common reason for a type not satisfying std::is_trivially_copyable
// is the presence of a user-defined copy constructor, and for classes that
// really are cheap to copy, it's also very common that the default compiler
// constructor would work just fine, so the solution might just be to delete
// your code that does what the compiler would have done anyway.
#if defined( __GNUC__ ) && ( __GNUC__ < 5 ) && !defined( __clang__ )
	template<typename T> struct is_copy_cheap : std::integral_constant<bool, ( std::has_trivial_copy_constructor<T>::value && sizeof(T) <= 8 ) > {};
#else
	template<typename T> struct is_copy_cheap : std::integral_constant<bool, ( std::is_trivially_copy_constructible<T>::value && sizeof(T) <= 8 ) > {};
#endif

//
// INCLUDE NOTE: This file only defines an enum and macros and so has no include
// requirements. This is deliberate so that it can be included at any point.
// However places that materialize a noncastable type will need to have the
// appropriate includes at the point where the macros are used.
//

// Noncastable integers support constructing from an arbitrary integer argument
// with an appropriate value check to make sure that data isn't lost
// (very handy for constructing one from a protobuf message field, for example).
// This enum type serves as a placeholder to disambiguate the constructor overload.
// Use as NoncastableType_t( NoncastableCheck, nSomeInt ).
enum NoncastableCheckFlag_t
{
	NoncastableCheckValue,
};

#define MAKE_NONCASTABLE_INTEGER_TYPE_BODY( className, inttype, defaultValue )										\
	public:																											\
	enum { IS_BOXED_INTEGER_TYPE = 1 };																				\
																													\
	className( InternalIntType_t val = defaultValue ) : m_nBoxedTypeSafeInt( val )									\
	{																												\
	}																												\
	template< typename _ArgT >																						\
	className( NoncastableCheckFlag_t ignoredFlag, _ArgT val )														\
	{																												\
		m_nBoxedTypeSafeInt = AssertCast< InternalIntType_t >( val );												\
	}																												\
	InternalIntType_t GetRaw() const																				\
	{																												\
		return m_nBoxedTypeSafeInt;																					\
	}																												\
	void SetRaw( InternalIntType_t other )																			\
	{																												\
		m_nBoxedTypeSafeInt = other;																				\
	}																												\
	InternalIntType_t& GetRawRef()																					\
	{																												\
		return m_nBoxedTypeSafeInt;																					\
	}																												\
	const InternalIntType_t& GetRawRef() const																		\
	{																												\
		return m_nBoxedTypeSafeInt;																					\
	}																												\
	InternalIntType_t *GetRawPtrForWrite()																			\
	{																												\
		return &m_nBoxedTypeSafeInt;																				\
	}																												\
	bool operator==( const className &other ) const { return other.m_nBoxedTypeSafeInt == m_nBoxedTypeSafeInt; }	\
	bool operator!=( const className &other ) const { return other.m_nBoxedTypeSafeInt != m_nBoxedTypeSafeInt; }	\
	bool operator<( const className &other ) const { return m_nBoxedTypeSafeInt < other.m_nBoxedTypeSafeInt; }		\
	bool operator<=( const className &other ) const { return m_nBoxedTypeSafeInt <= other.m_nBoxedTypeSafeInt; }	\
	bool operator>( const className &other ) const { return m_nBoxedTypeSafeInt > other.m_nBoxedTypeSafeInt; }		\
	bool operator>=( const className &other ) const { return m_nBoxedTypeSafeInt >= other.m_nBoxedTypeSafeInt; }	\
	className &operator++() { ++m_nBoxedTypeSafeInt; return *this; }												\
	className operator++(int) { className copy( *this ); ++(*this); return copy; }									\
	className &operator--() { --m_nBoxedTypeSafeInt; return *this; }												\
	className operator--(int) { className copy( *this ); --(*this); return copy; }									\
private:																											\
	inttype m_nBoxedTypeSafeInt;																					\
	className &operator =( InternalIntType_t other );

#define MAKE_NONCASTABLE_INTEGER_TYPE( className, inttype, defaultValue )											\
class className																										\
{																													\
public:																												\
	using InternalIntType_t = inttype;																				\
	MAKE_NONCASTABLE_INTEGER_TYPE_BODY( className, inttype, defaultValue );											\
};																													\
COMPILE_TIME_ASSERT( ::is_copy_cheap< className >::value );

#define MAKE_SCHEMATIZED_NONCASTABLE_INTEGER_TYPE( className, inttype, defaultValue )								\
schema class className																								\
{																													\
public:																												\
	DECLARE_SCHEMA_DATA_CLASS( className );																			\
	TYPEMETA( MIsBoxedIntegerType );																				\
	typedef inttype InternalIntType_t;																				\
	MAKE_NONCASTABLE_INTEGER_TYPE_BODY( className, inttype, defaultValue );											\
};																													\
COMPILE_TIME_ASSERT( ::is_copy_cheap< className >::value );

#define MAKE_NONAUTOCONSTRUCT_INTEGER_TYPE_BODY( className, inttype, defaultValue )									\
	enum { IS_BOXED_INTEGER_TYPE = 1 };																				\
																													\
	className() : m_nBoxedTypeSafeInt( defaultValue ) {}															\
    explicit constexpr className( InternalIntType_t val ) : m_nBoxedTypeSafeInt( val ) {}							\
	template< typename _ArgT >																						\
	className( NoncastableCheckFlag_t ignoredFlag, _ArgT val )														\
	{																												\
		m_nBoxedTypeSafeInt = AssertCast< InternalIntType_t >( val );												\
	}																												\
	constexpr InternalIntType_t GetRaw() const																		\
	{																												\
		return m_nBoxedTypeSafeInt;																					\
	}																												\
	void SetRaw( InternalIntType_t other )																			\
	{																												\
		m_nBoxedTypeSafeInt = other;																				\
	}																												\
	InternalIntType_t& GetRawRef()																					\
	{																												\
		return m_nBoxedTypeSafeInt;																					\
	}																												\
	const InternalIntType_t& GetRawRef() const																		\
	{																												\
		return m_nBoxedTypeSafeInt;																					\
	}																												\
	InternalIntType_t *GetRawPtrForWrite()																			\
	{																												\
		return &m_nBoxedTypeSafeInt;																				\
	}																												\
	bool operator==( const className &other ) const { return other.m_nBoxedTypeSafeInt == m_nBoxedTypeSafeInt; }	\
	bool operator!=( const className &other ) const { return other.m_nBoxedTypeSafeInt != m_nBoxedTypeSafeInt; }	\
	bool operator<( const className &other ) const { return m_nBoxedTypeSafeInt < other.m_nBoxedTypeSafeInt; }		\
	bool operator<=( const className &other ) const { return m_nBoxedTypeSafeInt <= other.m_nBoxedTypeSafeInt; }	\
	bool operator>( const className &other ) const { return m_nBoxedTypeSafeInt > other.m_nBoxedTypeSafeInt; }		\
	bool operator>=( const className &other ) const { return m_nBoxedTypeSafeInt >= other.m_nBoxedTypeSafeInt; }	\
	className &operator++() { ++m_nBoxedTypeSafeInt; return *this; }												\
	className operator++(int) { className copy( *this ); ++(*this); return copy; }									\
	className &operator--() { --m_nBoxedTypeSafeInt; return *this; }												\
	className operator--(int) { className copy( *this ); --(*this); return copy; }									\
private:																											\
	inttype m_nBoxedTypeSafeInt;																					\
	className &operator =( InternalIntType_t other );

#define MAKE_NONAUTOCONSTRUCT_INTEGER_TYPE( className, inttype, defaultValue )										\
class className																										\
{																													\
public:																												\
	using InternalIntType_t = inttype;																				\
	MAKE_NONAUTOCONSTRUCT_INTEGER_TYPE_BODY( className, inttype, defaultValue );									\
};																													\
COMPILE_TIME_ASSERT( ::is_copy_cheap< className >::value );

#define MAKE_SCHEMATIZED_NONAUTOCONSTRUCT_INTEGER_TYPE( className, inttype, defaultValue )							\
schema class className																								\
{																													\
public:																												\
	DECLARE_SCHEMA_DATA_CLASS( className );																			\
	TYPEMETA( MIsBoxedIntegerType );																				\
	typedef inttype InternalIntType_t;																				\
	MAKE_NONAUTOCONSTRUCT_INTEGER_TYPE_BODY( className, inttype, defaultValue );									\
};																													\
COMPILE_TIME_ASSERT( ::is_copy_cheap< className >::value );

#define MAKE_NONCASTABLE_INTEGER_TYPE_NO_MATH_BODY( className, inttype, defaultValue )								\
	enum { IS_BOXED_INTEGER_TYPE = 1 };																				\
																													\
	className( InternalIntType_t val = defaultValue ) : m_nBoxedTypeSafeInt( val )									\
	{																												\
	}																												\
	template< typename _ArgT >																						\
	className( NoncastableCheckFlag_t ignoredFlag, _ArgT val )														\
	{																												\
		m_nBoxedTypeSafeInt = AssertCast< InternalIntType_t >( val );												\
	}																												\
	InternalIntType_t GetRaw() const																				\
	{																												\
		return m_nBoxedTypeSafeInt;																					\
	}																												\
	void SetRaw( InternalIntType_t other )																			\
	{																												\
		m_nBoxedTypeSafeInt = other;																				\
	}																												\
	InternalIntType_t& GetRawRef()																					\
	{																												\
		return m_nBoxedTypeSafeInt;																					\
	}																												\
	const InternalIntType_t& GetRawRef() const																		\
	{																												\
		return m_nBoxedTypeSafeInt;																					\
	}																												\
	InternalIntType_t *GetRawPtrForWrite()																			\
	{																												\
		return &m_nBoxedTypeSafeInt;																				\
	}																												\
	bool operator==( const className &other ) const { return other.m_nBoxedTypeSafeInt == m_nBoxedTypeSafeInt; }	\
	bool operator!=( const className &other ) const { return other.m_nBoxedTypeSafeInt != m_nBoxedTypeSafeInt; }	\
	bool operator<( const className &other ) const { return m_nBoxedTypeSafeInt < other.m_nBoxedTypeSafeInt; }		\
	bool operator<=( const className &other ) const { return m_nBoxedTypeSafeInt <= other.m_nBoxedTypeSafeInt; }	\
	bool operator>( const className &other ) const { return m_nBoxedTypeSafeInt > other.m_nBoxedTypeSafeInt; }		\
	bool operator>=( const className &other ) const { return m_nBoxedTypeSafeInt >= other.m_nBoxedTypeSafeInt; }	\
private:																											\
	inttype m_nBoxedTypeSafeInt;																					\
	className &operator =( InternalIntType_t other );

#define MAKE_NONCASTABLE_INTEGER_TYPE_NO_MATH( className, inttype, defaultValue )									\
class className																										\
{																													\
public:																												\
	using InternalIntType_t = inttype;																				\
	MAKE_NONCASTABLE_INTEGER_TYPE_NO_MATH_BODY( className, inttype, defaultValue )									\
};																													\
COMPILE_TIME_ASSERT( ::is_copy_cheap< className >::value );

#define MAKE_SCHEMATIZED_NONCASTABLE_INTEGER_TYPE_NO_MATH( className, inttype, defaultValue )						\
schema class className																								\
{																													\
public:																												\
	DECLARE_SCHEMA_DATA_CLASS( className );																			\
	TYPEMETA( MIsBoxedIntegerType );																				\
	typedef inttype InternalIntType_t;																				\
	MAKE_NONCASTABLE_INTEGER_TYPE_NO_MATH_BODY( className, inttype, defaultValue )									\
};																													\
COMPILE_TIME_ASSERT( ::is_copy_cheap< className >::value );
