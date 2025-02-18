//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef NETWORKVAR_H
#define NETWORKVAR_H
#ifdef _WIN32
#pragma once
#endif


#include "tier0/dbg.h"
#include "convar.h"

#if defined( CLIENT_DLL ) || defined( GAME_DLL )
	#include "basehandle.h"
#endif


#pragma warning( disable : 4284 ) // warning C4284: return type for 'CNetworkVarT<int>::operator ->' is 'int *' (ie; not a UDT or reference to a UDT.  Will produce errors if applied using infix notation)

#define MyOffsetOf( type, var ) ( (int)&((type*)0)->var )

#ifdef _DEBUG
	extern bool g_bUseNetworkVars;
	#define CHECK_USENETWORKVARS if(g_bUseNetworkVars)
#else
	#define CHECK_USENETWORKVARS // don't check for g_bUseNetworkVars
#endif



inline int InternalCheckDeclareClass( const char *pClassName, const char *pClassNameMatch, void *pTestPtr, void *pBasePtr )
{
	// This makes sure that casting from ThisClass to BaseClass works right. You'll get a compiler error if it doesn't
	// work at all, and you'll get a runtime error if you use multiple inheritance.
	Assert( pTestPtr == pBasePtr );
	
	// This is triggered by IMPLEMENT_SERVER_CLASS. It does DLLClassName::CheckDeclareClass( #DLLClassName ).
	// If they didn't do a DECLARE_CLASS in DLLClassName, then it'll be calling its base class's version
	// and the class names won't match.
	Assert( (void*)pClassName == (void*)pClassNameMatch );
	return 0;
}


template <typename T> 
inline int CheckDeclareClass_Access( T *, const char *pShouldBe )
{
	return T::CheckDeclareClass( pShouldBe );
}

#ifndef _STATIC_LINKED
#ifdef _MSC_VER
#if defined(_DEBUG) && (_MSC_VER > 1200 )
	#define VALIDATE_DECLARE_CLASS 1
#endif
#endif
#endif

#ifdef  VALIDATE_DECLARE_CLASS

	#define DECLARE_CLASS( className, baseClassName ) \
		typedef baseClassName BaseClass; \
		typedef className ThisClass; \
		template <typename T> friend int CheckDeclareClass_Access(T *, const char *pShouldBe); \
		static int CheckDeclareClass( const char *pShouldBe ) \
		{ \
			InternalCheckDeclareClass( pShouldBe, #className, (ThisClass*)0xFFFFF, (BaseClass*)(ThisClass*)0xFFFFF ); \
			return CheckDeclareClass_Access( (BaseClass *)NULL, #baseClassName ); \
		}

	// Use this macro when you have a base class, but it's part of a library that doesn't use network vars
	// or any of the things that use ThisClass or BaseClass.
	#define DECLARE_CLASS_GAMEROOT( className, baseClassName ) \
		typedef baseClassName BaseClass; \
		typedef className ThisClass; \
		template <typename T> friend int CheckDeclareClass_Access(T *, const char *pShouldBe); \
		static int CheckDeclareClass( const char *pShouldBe ) \
		{ \
			return InternalCheckDeclareClass( pShouldBe, #className, (ThisClass*)0xFFFFF, (BaseClass*)(ThisClass*)0xFFFFF ); \
		}

	// Deprecated macro formerly used to work around VC++98 bug
	#define DECLARE_CLASS_NOFRIEND( className, baseClassName ) \
		DECLARE_CLASS( className, baseClassName )

	#define DECLARE_CLASS_NOBASE( className ) \
		typedef className ThisClass; \
		template <typename T> friend int CheckDeclareClass_Access(T *, const char *pShouldBe); \
		static int CheckDeclareClass( const char *pShouldBe ) \
		{ \
			return InternalCheckDeclareClass( pShouldBe, #className, 0, 0 ); \
		} 

#else
	#define DECLARE_CLASS( className, baseClassName ) \
		typedef baseClassName BaseClass; \
		typedef className ThisClass;

	#define DECLARE_CLASS_GAMEROOT( className, baseClassName )	DECLARE_CLASS( className, baseClassName )
	#define DECLARE_CLASS_NOFRIEND( className, baseClassName )	DECLARE_CLASS( className, baseClassName )

	#define DECLARE_CLASS_NOBASE( className )					typedef className ThisClass;
#endif




// All classes that contain CNetworkVars need a NetworkStateChanged() function. If the class is not an entity,
// it needs to forward the call to the entity it's in. These macros can help.
	
	// These macros setup an entity pointer in your class. Use IMPLEMENT_NETWORKVAR_CHAIN before you do
	// anything inside the class itself.
	class CBaseEntity;
	class CAutoInitEntPtr
	{
	public:
		CAutoInitEntPtr()
		{
			m_pEnt = NULL;
		}
		CBaseEntity *m_pEnt;
	};

	//TODO: Currently, these don't get the benefit of tracking changes to individual vars.
	// Would be nice if they did.
	#define DECLARE_NETWORKVAR_CHAIN() \
		CAutoInitEntPtr __m_pChainEntity; \
		void NetworkStateChanged() { CHECK_USENETWORKVARS __m_pChainEntity.m_pEnt->NetworkStateChanged(); } \
		void NetworkStateChanged( void *pVar ) { CHECK_USENETWORKVARS __m_pChainEntity.m_pEnt->NetworkStateChanged(); }

	#define IMPLEMENT_NETWORKVAR_CHAIN( varName ) \
		(varName)->__m_pChainEntity.m_pEnt = this;



// Use this macro when you want to embed a structure inside your entity and have CNetworkVars in it.
template< class T >
static inline void DispatchNetworkStateChanged( T *pObj )
{
	CHECK_USENETWORKVARS pObj->NetworkStateChanged();
}
template< class T >
static inline void DispatchNetworkStateChanged( T *pObj, void *pVar )
{
	CHECK_USENETWORKVARS pObj->NetworkStateChanged( pVar );
}


#define DECLARE_EMBEDDED_NETWORKVAR() \
	template <typename T> friend int ServerClassInit(T *);	\
	template <typename T> friend int ClientClassInit(T *); \
	virtual void NetworkStateChanged() {}  virtual void NetworkStateChanged( void *pProp ) {}

// NOTE: Assignment operator is disabled because it doesn't call copy constructors of scalar types within the aggregate, so they are not marked changed
#define CNetworkVarEmbedded( type, name ) \
	class NetworkVar_##name; \
	friend class NetworkVar_##name; \
	static inline int GetOffset_##name() { return MyOffsetOf(ThisClass,name); } \
	typedef ThisClass ThisClass_##name; \
	class NetworkVar_##name : public type \
	{ \
		template< class T > NetworkVar_##name& operator=( const T &val ) { *((type*)this) = val; return *this; } \
	public: \
		void CopyFrom( const type &src ) { *((type *)this) = src; NetworkStateChanged(); } \
		virtual void NetworkStateChanged() \
		{ \
			DispatchNetworkStateChanged( (ThisClass_##name*)( ((char*)this) - GetOffset_##name() ) ); \
		} \
		virtual void NetworkStateChanged( void *pVar ) \
		{ \
			DispatchNetworkStateChanged( (ThisClass_##name*)( ((char*)this) - GetOffset_##name() ), pVar ); \
		} \
	}; \
	NetworkVar_##name name; 

template<typename T>
FORCEINLINE void NetworkVarConstruct( T &x ) { x = T(0); }
FORCEINLINE void NetworkVarConstruct( color32_s &x ) { x.r = x.g = x.b = x.a = 0; }

template< class Type, class Changer >
class CNetworkVarBase
{
public:
	inline CNetworkVarBase()
	{
		NetworkVarConstruct( m_Value );
	}

	template< class C >
	const Type& operator=( const C &val ) 
	{ 
		return Set( ( const Type )val ); 
	}
	
	template< class C >
	const Type& operator=( const CNetworkVarBase< C, Changer > &val ) 
	{ 
		return Set( ( const Type )val.m_Value ); 
	}
	
	const Type& Set( const Type &val )
	{
		if ( memcmp( &m_Value, &val, sizeof(Type) ) )
		{
			NetworkStateChanged();
			m_Value = val;
		}
		return m_Value;
	}
	
	Type& GetForModify()
	{
		NetworkStateChanged();
		return m_Value;
	}

	template< class C >
	const Type& operator+=( const C &val ) 
	{
		return Set( m_Value + ( const Type )val ); 
	}

	template< class C >
	const Type& operator-=( const C &val ) 
	{
		return Set( m_Value - ( const Type )val ); 
	}
	
	template< class C >
	const Type& operator/=( const C &val ) 
	{
		return Set( m_Value / ( const Type )val ); 
	}
	
	template< class C >
	const Type& operator*=( const C &val ) 
	{
		return Set( m_Value * ( const Type )val ); 
	}
	
	template< class C >
	const Type& operator^=( const C &val ) 
	{
		return Set( m_Value ^ ( const Type )val ); 
	}

	template< class C >
	const Type& operator|=( const C &val ) 
	{
		return Set( m_Value | ( const Type )val ); 
	}

	const Type& operator++()
	{
		return (*this += 1);
	}

	Type operator--()
	{
		return (*this -= 1);
	}
	
	Type operator++( int ) // postfix version..
	{
		Type val = m_Value;
		(*this += 1);
		return val;
	}

	Type operator--( int ) // postfix version..
	{
		Type val = m_Value;
		(*this -= 1);
		return val;
	}
	
	// For some reason the compiler only generates type conversion warnings for this operator when used like 
	// CNetworkVarBase<unsigned char> = 0x1
	// (it warns about converting from an int to an unsigned char).
	template< class C >
	const Type& operator&=( const C &val ) 
	{	
		return Set( m_Value & ( const Type )val ); 
	}

	operator const Type&() const 
	{
		return m_Value; 
	}
	
	const Type& Get() const 
	{
		return m_Value; 
	}
	
	const Type* operator->() const 
	{
		return &m_Value; 
	}

	Type m_Value;

protected:
	inline void NetworkStateChanged()
	{
		Changer::NetworkStateChanged( this );
	}
};


template< class Type, class Changer >
class CNetworkColor32Base : public CNetworkVarBase< Type, Changer >
{
public:
	inline void Init( byte rVal, byte gVal, byte bVal )
	{
		SetR( rVal );
		SetG( gVal );
		SetB( bVal );
	}
	inline void Init( byte rVal, byte gVal, byte bVal, byte aVal )
	{
		SetR( rVal );
		SetG( gVal );
		SetB( bVal );
		SetA( aVal );
	}

	const Type& operator=( const Type &val ) 
	{ 
		return this->Set( val ); 
	}

	const Type& operator=( const CNetworkColor32Base<Type,Changer> &val ) 
	{ 
		return CNetworkVarBase<Type,Changer>::Set( val.m_Value );
	}
	
	inline byte GetR() const { return CNetworkColor32Base<Type,Changer>::m_Value.r; }
	inline byte GetG() const { return CNetworkColor32Base<Type,Changer>::m_Value.g; }
	inline byte GetB() const { return CNetworkColor32Base<Type,Changer>::m_Value.b; }
	inline byte GetA() const { return CNetworkColor32Base<Type,Changer>::m_Value.a; }
	inline void SetR( byte val ) { SetVal( CNetworkColor32Base<Type,Changer>::m_Value.r, val ); }
	inline void SetG( byte val ) { SetVal( CNetworkColor32Base<Type,Changer>::m_Value.g, val ); }
	inline void SetB( byte val ) { SetVal( CNetworkColor32Base<Type,Changer>::m_Value.b, val ); }
	inline void SetA( byte val ) { SetVal( CNetworkColor32Base<Type,Changer>::m_Value.a, val ); }

protected:
	inline void SetVal( byte &out, const byte &in )
	{
		if ( out != in )
		{
			CNetworkVarBase< Type, Changer >::NetworkStateChanged();
			out = in;
		}
	}
};


// Network vector wrapper.
template< class Type, class Changer >
class CNetworkVectorBase : public CNetworkVarBase< Type, Changer >
{
public:
	inline void Init( float ix=0, float iy=0, float iz=0 ) 
	{
		SetX( ix );
		SetY( iy );
		SetZ( iz );
	}
	
	const Type& operator=( const Type &val ) 
	{ 
		return CNetworkVarBase< Type, Changer >::Set( val ); 
	}

	const Type& operator=( const CNetworkVectorBase<Type,Changer> &val ) 
	{ 
		return CNetworkVarBase<Type,Changer>::Set( val.m_Value );
	}

	inline float GetX() const { return CNetworkVectorBase<Type,Changer>::m_Value.x; }
	inline float GetY() const { return CNetworkVectorBase<Type,Changer>::m_Value.y; }
	inline float GetZ() const { return CNetworkVectorBase<Type,Changer>::m_Value.z; }
	inline float operator[]( int i ) const { return CNetworkVectorBase<Type,Changer>::m_Value[i]; }

	inline void SetX( float val ) { DetectChange( CNetworkVectorBase<Type,Changer>::m_Value.x, val ); }
	inline void SetY( float val ) { DetectChange( CNetworkVectorBase<Type,Changer>::m_Value.y, val ); }
	inline void SetZ( float val ) { DetectChange( CNetworkVectorBase<Type,Changer>::m_Value.z, val ); }
	inline void Set( int i, float val ) { DetectChange( CNetworkVectorBase<Type,Changer>::m_Value[i], val ); }

	bool operator==( const Type &val ) const 
	{ 
		return CNetworkVectorBase<Type,Changer>::m_Value == (Type)val; 
	}

	bool operator!=( const Type &val ) const 
	{
		return CNetworkVectorBase<Type,Changer>::m_Value != (Type)val; 
	}

	const Type operator+( const Type &val ) const 
	{
		return CNetworkVectorBase<Type,Changer>::m_Value + val; 
	}

	const Type operator-( const Type &val ) const
	{ 
		return CNetworkVectorBase<Type,Changer>::m_Value - val; 
	}

	const Type operator*( const Type &val ) const
	{
		return CNetworkVectorBase<Type,Changer>::m_Value * val; 
	}

	const Type& operator*=( float val )
	{
		return CNetworkVarBase< Type, Changer >::Set( CNetworkVectorBase<Type,Changer>::m_Value * val );
	}

	const Type operator*( float val ) const
	{
		return CNetworkVectorBase<Type,Changer>::m_Value * val; 
	}

	const Type operator/( const Type &val ) const
	{
		return CNetworkVectorBase<Type,Changer>::m_Value / val; 
	}

private:
	inline void DetectChange( float &out, float in ) 
	{
		if ( out != in ) 
		{
			CNetworkVectorBase<Type,Changer>::NetworkStateChanged();
			out = in;
		}
	}
};


// Network vector wrapper.
template< class Type, class Changer >
class CNetworkQuaternionBase : public CNetworkVarBase< Type, Changer >
{
public:
	inline void Init( float ix=0, float iy=0, float iz=0, float iw = 0 ) 
	{
		SetX( ix );
		SetY( iy );
		SetZ( iz );
		SetW( iw );
	}
	
	const Type& operator=( const Type &val ) 
	{ 
		return CNetworkVarBase< Type, Changer >::Set( val ); 
	}

	const Type& operator=( const CNetworkQuaternionBase<Type,Changer> &val ) 
	{ 
		return CNetworkVarBase<Type,Changer>::Set( val.m_Value );
	}

	inline float GetX() const { return CNetworkQuaternionBase<Type,Changer>::m_Value.x; }
	inline float GetY() const { return CNetworkQuaternionBase<Type,Changer>::m_Value.y; }
	inline float GetZ() const { return CNetworkQuaternionBase<Type,Changer>::m_Value.z; }
	inline float GetW() const { return CNetworkQuaternionBase<Type,Changer>::m_Value.w; }
	inline float operator[]( int i ) const { return CNetworkQuaternionBase<Type,Changer>::m_Value[i]; }

	inline void SetX( float val ) { DetectChange( CNetworkQuaternionBase<Type,Changer>::m_Value.x, val ); }
	inline void SetY( float val ) { DetectChange( CNetworkQuaternionBase<Type,Changer>::m_Value.y, val ); }
	inline void SetZ( float val ) { DetectChange( CNetworkQuaternionBase<Type,Changer>::m_Value.z, val ); }
	inline void SetW( float val ) { DetectChange( CNetworkQuaternionBase<Type,Changer>::m_Value.w, val ); }
	inline void Set( int i, float val ) { DetectChange( CNetworkQuaternionBase<Type,Changer>::m_Value[i], val ); }

	bool operator==( const Type &val ) const 
	{ 
		return CNetworkQuaternionBase<Type,Changer>::m_Value == (Type)val; 
	}

	bool operator!=( const Type &val ) const 
	{
		return CNetworkQuaternionBase<Type,Changer>::m_Value != (Type)val; 
	}

	const Type operator+( const Type &val ) const 
	{
		return CNetworkQuaternionBase<Type,Changer>::m_Value + val; 
	}

	const Type operator-( const Type &val ) const
	{ 
		return CNetworkQuaternionBase<Type,Changer>::m_Value - val; 
	}

	const Type operator*( const Type &val ) const
	{
		return CNetworkQuaternionBase<Type,Changer>::m_Value * val; 
	}

	const Type& operator*=( float val )
	{
		return CNetworkQuaternionBase< Type, Changer >::Set( CNetworkQuaternionBase<Type,Changer>::m_Value * val );
	}

	const Type operator*( float val ) const
	{
		return CNetworkQuaternionBase<Type,Changer>::m_Value * val; 
	}

	const Type operator/( const Type &val ) const
	{
		return CNetworkQuaternionBase<Type,Changer>::m_Value / val; 
	}

private:
	inline void DetectChange( float &out, float in ) 
	{
		if ( out != in ) 
		{
			CNetworkQuaternionBase<Type,Changer>::NetworkStateChanged();
			out = in;
		}
	}
};


// Network ehandle wrapper.
#if defined( CLIENT_DLL ) || defined( GAME_DLL )
	inline void NetworkVarConstruct( CBaseHandle &x ) {}

	template< class Type, class Changer >
	class CNetworkHandleBase : public CNetworkVarBase< CBaseHandle, Changer >
	{
	public:
		const Type* operator=( const Type *val ) 
		{ 
			return Set( val ); 
		}
			
		const Type& operator=( const CNetworkHandleBase<Type,Changer> &val ) 
		{ 
			const CBaseHandle &handle = CNetworkVarBase<CBaseHandle,Changer>::Set( val.m_Value );
			return *(const Type*)handle.Get();
		}

		bool operator !() const 
		{ 
			return !CNetworkHandleBase<Type,Changer>::m_Value.Get(); 
		}
		
		operator Type*() const 
		{ 
			return static_cast< Type* >( CNetworkHandleBase<Type,Changer>::m_Value.Get() );
		}

		const Type* Set( const Type *val )
		{
			if ( CNetworkHandleBase<Type,Changer>::m_Value != val )
			{
				this->NetworkStateChanged();
				CNetworkHandleBase<Type,Changer>::m_Value = val;
			}
			return val;
		}
		
		Type* Get() const 
		{ 
			return static_cast< Type* >( CNetworkHandleBase<Type,Changer>::m_Value.Get() );
		}

		Type* operator->() const 
		{ 
			return static_cast< Type* >( CNetworkHandleBase<Type,Changer>::m_Value.Get() );
		}

		bool operator==( const Type *val ) const 
		{
			return CNetworkHandleBase<Type,Changer>::m_Value == val; 
		}

		bool operator!=( const Type *val ) const 
		{
			return CNetworkHandleBase<Type,Changer>::m_Value != val;
		}
	};



	#define CNetworkHandle( type, name ) CNetworkHandleInternal( type, name, NetworkStateChanged )

	#define CNetworkHandleInternal( type, name, stateChangedFn ) \
		NETWORK_VAR_START( type, name ) \
		NETWORK_VAR_END( type, name, CNetworkHandleBase, stateChangedFn )
#endif


// Use this macro to define a network variable.
#define CNetworkVar( type, name ) \
	NETWORK_VAR_START( type, name ) \
	NETWORK_VAR_END( type, name, CNetworkVarBase, NetworkStateChanged )


// Use this macro when you have a base class with a variable, and it doesn't have that variable in a SendTable,
// but a derived class does. Then, the entity is only flagged as changed when the variable is changed in
// an entity that wants to transmit the variable.
	#define CNetworkVarForDerived( type, name ) \
		virtual void NetworkStateChanged_##name() {} \
		virtual void NetworkStateChanged_##name( void *pVar ) {} \
		NETWORK_VAR_START( type, name ) \
		NETWORK_VAR_END( type, name, CNetworkVarBase, NetworkStateChanged_##name )

	#define CNetworkVectorForDerived( name ) \
		virtual void NetworkStateChanged_##name() {} \
		virtual void NetworkStateChanged_##name( void *pVar ) {} \
		CNetworkVectorInternal( Vector, name, NetworkStateChanged_##name )
		
	#define CNetworkHandleForDerived( type, name ) \
		virtual void NetworkStateChanged_##name() {} \
		virtual void NetworkStateChanged_##name( void *pVar ) {} \
		CNetworkHandleInternal( type, name, NetworkStateChanged_##name )
		
	#define CNetworkArrayForDerived( type, name, count ) \
		virtual void NetworkStateChanged_##name() {} \
		virtual void NetworkStateChanged_##name( void *pVar ) {} \
		CNetworkArrayInternal( type, name, count, NetworkStateChanged_##name )

	#define IMPLEMENT_NETWORK_VAR_FOR_DERIVED( name ) \
		virtual void NetworkStateChanged_##name() { CHECK_USENETWORKVARS NetworkStateChanged(); } \
		virtual void NetworkStateChanged_##name( void *pVar ) { CHECK_USENETWORKVARS NetworkStateChanged( pVar ); }


// This virtualizes the change detection on the variable, but it is ON by default.
// Use this when you have a base class in which MOST of its derived classes use this variable
// in their SendTables, but there are a couple that don't (and they
// can use DISABLE_NETWORK_VAR_FOR_DERIVED).
	#define CNetworkVarForDerived_OnByDefault( type, name ) \
		virtual void NetworkStateChanged_##name() { CHECK_USENETWORKVARS NetworkStateChanged(); } \
		virtual void NetworkStateChanged_##name( void *pVar ) { CHECK_USENETWORKVARS NetworkStateChanged( pVar ); } \
		NETWORK_VAR_START( type, name ) \
		NETWORK_VAR_END( type, name, CNetworkVarBase, NetworkStateChanged_##name )

	#define DISABLE_NETWORK_VAR_FOR_DERIVED( name ) \
		virtual void NetworkStateChanged_##name() {} \
		virtual void NetworkStateChanged_##name( void *pVar ) {}



// Vectors + some convenient helper functions.
#define CNetworkVector( name ) CNetworkVectorInternal( Vector, name, NetworkStateChanged )
#define CNetworkQAngle( name ) CNetworkVectorInternal( QAngle, name, NetworkStateChanged )

#define CNetworkVectorInternal( type, name, stateChangedFn ) \
	NETWORK_VAR_START( type, name ) \
	NETWORK_VAR_END( type, name, CNetworkVectorBase, stateChangedFn )

#define CNetworkQuaternion( name ) \
	NETWORK_VAR_START( Quaternion, name ) \
	NETWORK_VAR_END( Quaternion, name, CNetworkQuaternionBase, NetworkStateChanged )

// Helper for color32's. Contains GetR(), SetR(), etc.. functions.
#define CNetworkColor32( name ) \
	NETWORK_VAR_START( color32, name ) \
	NETWORK_VAR_END( color32, name, CNetworkColor32Base, NetworkStateChanged )


#define CNetworkString( name, length ) \
	class NetworkVar_##name; \
	friend class NetworkVar_##name; \
	typedef ThisClass MakeANetworkVar_##name; \
	class NetworkVar_##name \
	{ \
	public: \
		NetworkVar_##name() { m_Value[0] = '\0'; } \
		operator const char*() const { return m_Value; } \
		const char* Get() const { return m_Value; } \
		char* GetForModify() \
		{ \
			NetworkStateChanged(); \
			return m_Value; \
		} \
	protected: \
		inline void NetworkStateChanged() \
		{ \
		CHECK_USENETWORKVARS ((ThisClass*)(((char*)this) - MyOffsetOf(ThisClass,name)))->NetworkStateChanged(); \
		} \
	private: \
		char m_Value[length]; \
	}; \
	NetworkVar_##name name;




// Use this to define networked arrays.
// You can access elements for reading with operator[], and you can set elements with the Set() function.
#define CNetworkArrayInternal( type, name, count, stateChangedFn ) \
	class NetworkVar_##name; \
	friend class NetworkVar_##name; \
	typedef ThisClass MakeANetworkVar_##name; \
	class NetworkVar_##name \
	{ \
	public: \
		inline NetworkVar_##name() \
		{ \
			for ( int i = 0 ; i < count ; ++i ) \
				NetworkVarConstruct( m_Value[i] ); \
		} \
		template <typename T> friend int ServerClassInit(T *);	\
		const type& operator[]( int i ) const \
		{ \
			return Get( i ); \
		} \
		\
		const type& Get( int i ) const \
		{ \
			Assert( i >= 0 && i < count ); \
			return m_Value[i]; \
		} \
		\
		type& GetForModify( int i ) \
		{ \
			Assert( i >= 0 && i < count ); \
			NetworkStateChanged( i ); \
			return m_Value[i]; \
		} \
		\
		void Set( int i, const type &val ) \
		{ \
			Assert( i >= 0 && i < count ); \
			if( memcmp( &m_Value[i], &val, sizeof(type) ) ) \
			{ \
				NetworkStateChanged( i ); \
			       	m_Value[i] = val; \
			} \
		} \
		const type* Base() const { return m_Value; } \
		int Count() const { return count; } \
	protected: \
		inline void NetworkStateChanged( int index ) \
		{ \
			CHECK_USENETWORKVARS ((ThisClass*)(((char*)this) - MyOffsetOf(ThisClass,name)))->stateChangedFn( &m_Value[index] ); \
		} \
		type m_Value[count]; \
	}; \
	NetworkVar_##name name;


#define CNetworkArray( type, name, count )  CNetworkArrayInternal( type, name, count, NetworkStateChanged )


// Internal macros used in definitions of network vars.
#define NETWORK_VAR_START( type, name ) \
	class NetworkVar_##name; \
	friend class NetworkVar_##name; \
	typedef ThisClass MakeANetworkVar_##name; \
	class NetworkVar_##name \
	{ \
	public: \
		template <typename T> friend int ServerClassInit(T *);


#define NETWORK_VAR_END( type, name, base, stateChangedFn ) \
	public: \
		static inline void NetworkStateChanged( void *ptr ) \
		{ \
			CHECK_USENETWORKVARS ((ThisClass*)(((char*)ptr) - MyOffsetOf(ThisClass,name)))->stateChangedFn( ptr ); \
		} \
	}; \
	base< type, NetworkVar_##name > name;



#endif // NETWORKVAR_H
