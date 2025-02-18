//========= Copyright Valve Corporation, All rights reserved. ============//
//
// A class representing a subselection of something like a mesh
//
//=============================================================================

#ifndef DMECOMPONENT_H
#define DMECOMPONENT_H

#ifdef _WIN32
#pragma once
#endif


#include "datamodel/dmelement.h"


//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------
class CDmeComponent : public CDmElement
{
	DEFINE_ELEMENT( CDmeComponent, CDmElement );

public:
	enum Component_t
	{
		COMP_INVALID = -1,

		COMP_VTX,
		COMP_FACE,

		STANDARD_COMP_COUNT
	};

	// resolve internal data from changed attributes
	virtual void Resolve();

	// What type of component is this
	Component_t Type() const;

	// How many pieces are in this component
	virtual int Count() const;

	// Are there no pieces in this component
	bool IsEmpty() const;

	// Are all possible pieces in this component
	bool IsComplete() const;

	// Is this an equivalent component to another component
	virtual bool IsEqual( const CDmeComponent *pRhs ) const;

	// Reset to an empty selection
	virtual void Clear();

protected:

	CDmaVar< int > m_Type;
	CDmaVar< bool > m_bComplete;
};


//-----------------------------------------------------------------------------
// The default type is invalid
//-----------------------------------------------------------------------------
inline CDmeComponent::Component_t CDmeComponent::Type() const
{
	const int type( m_Type.Get() );
	if ( type < 0 || type >= STANDARD_COMP_COUNT )
		return COMP_INVALID;

	return static_cast< Component_t >( type );
}


//-----------------------------------------------------------------------------
// Are there no pieces in this component
//-----------------------------------------------------------------------------
inline int CDmeComponent::Count() const
{
	Assert( 0 );
	return 0;
}


//-----------------------------------------------------------------------------
// Are there no pieces in this component
//-----------------------------------------------------------------------------
inline bool CDmeComponent::IsEmpty() const
{
	return Count() == 0;
}


//-----------------------------------------------------------------------------
// Are all possible pieces in this component
//-----------------------------------------------------------------------------
inline bool CDmeComponent::IsEqual( const CDmeComponent *pRhs ) const
{
	return Type() == pRhs->Type() && Count() == pRhs->Count() && IsComplete() == pRhs->IsComplete();
}


//-----------------------------------------------------------------------------
// Are all possible pieces in this component
//-----------------------------------------------------------------------------
inline bool CDmeComponent::IsComplete() const
{
	return m_bComplete.Get();
}


//-----------------------------------------------------------------------------
// Reset to an empty selection
//-----------------------------------------------------------------------------
inline void CDmeComponent::Clear()
{
	m_bComplete.Set( false );
}


//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------
class CDmeSingleIndexedComponent : public CDmeComponent
{
	DEFINE_ELEMENT( CDmeSingleIndexedComponent, CDmeComponent );

public:
	// resolve internal data from changed attributes
	virtual void Resolve();

	// From CDmeComponent
	virtual int Count() const;

	bool SetType( Component_t type );

	void AddComponent( int component, float weight = 1.0f );

	void AddComponents( const CUtlVector< int > &components );

	void AddComponents( const CUtlVector< int > &components, const CUtlVector< float > &weights );

	void RemoveComponent( int component );

	bool GetComponent( int index, int &component, float &weight ) const;

	bool GetWeight( int component, float &weight ) const;

	void GetComponents( CUtlVector< int > &components ) const;

	void GetComponents( CUtlVector< int > &components, CUtlVector< float > &weights ) const;

	void SetComplete( int nComplete );

	int GetComplete() const;

	bool HasComponent( int component ) const;

	virtual void Clear();

	void Add( const CDmeSingleIndexedComponent &rhs );

	void Add( const CDmeSingleIndexedComponent *pRhs ) { Assert( pRhs ); Add( *pRhs ); }

	void Union( const CDmeSingleIndexedComponent &rhs ) { Add( rhs ); }

	void Union( const CDmeSingleIndexedComponent *pRhs ) { Assert( pRhs ); Add( *pRhs ); }

	CDmeSingleIndexedComponent &operator+=( const CDmeSingleIndexedComponent &rhs ) { Add( rhs ); return *this; }

	void Subtract( const CDmeSingleIndexedComponent &rhs );

	void Subtract( const CDmeSingleIndexedComponent *pRhs ) { Assert( pRhs ); Subtract( *pRhs ); }

	void Complement( const CDmeSingleIndexedComponent &rhs ) { Subtract( rhs ); }

	void Complement( const CDmeSingleIndexedComponent *pRhs ) { Assert( pRhs ); Subtract( *pRhs ); }

	CDmeSingleIndexedComponent &operator-=( const CDmeSingleIndexedComponent &rhs ) { Subtract( rhs ); return *this; }

	void Intersection( const CDmeSingleIndexedComponent &rhs );

	void Intersection( const CDmeSingleIndexedComponent *pRhs ) { Assert( pRhs ); Intersection( *pRhs ); }

protected:
	int BinarySearch( int component ) const;

	CDmaVar< int > m_CompleteCount;
	CDmaArray< int > m_Components;
	CDmaArray< float > m_Weights;
};


#endif // DMECOMPONENT_H
