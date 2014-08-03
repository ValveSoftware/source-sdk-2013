//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DMVAR_H
#define DMVAR_H
#ifdef _WIN32
#pragma once
#endif


class CDmAttribute;

//-----------------------------------------------------------------------------
// Helper template for external attributes
//-----------------------------------------------------------------------------
template< typename T >
class CDmaVar
{
	typedef typename CDmAttributeInfo< T >::StorageType_t D;

public:
	CDmaVar( );

	// Setup to be used in OnConstruction methods of DmElements
	void Init( CDmElement *pOwner, const char *pAttributeName, int flags = 0 );
	void InitAndSet( CDmElement *pOwner, const char *pAttributeName, const T &value, int flags = 0 );

	// Set/get
	const T& Set( const T &val );
	const T& Get() const;

	// Cast operators
	operator const T&() const;
	const T* operator->() const;

	// Assignment operator
	const CDmaVar<T>& operator=( const CDmaVar<T>& src );

	// Math utility operations
	const T& operator=( const T &val );
	const T& operator+=( const T &val ); 
	const T& operator-=( const T &val ); 
	const T& operator/=( const T &val ); 
	const T& operator*=( const T &val ); 
	const T& operator^=( const T &val ); 
	const T& operator|=( const T &val ); 
	const T& operator&=( const T &val ); 
	T operator++();
	T operator--();
	T operator++( int ); // postfix version..
	T operator--( int ); // postfix version..

	// Returns the attribute associated with the var
	CDmAttribute *GetAttribute();
	const CDmAttribute *GetAttribute() const;

	// Is the attribute dirty?
	bool IsDirty() const;

protected:
	const T& Value() const;
	T& Value();
	const D& Storage() const;
	D& Storage();

private:
	D m_Storage;

protected:
	CDmAttribute *m_pAttribute;
};

//-----------------------------------------------------------------------------
// Specialization for string
//-----------------------------------------------------------------------------
class CDmaString : public CDmaVar< CUtlString >
{
public:
	const char *Get( ) const;
	operator const char*() const;

	void Set( const char *pValue );
	CDmaString &operator=( const char *src );
	const CDmaString& operator=( const CDmaString& src );

	// Returns strlen
	int	Length() const;
};

#endif // DMVAR_H
