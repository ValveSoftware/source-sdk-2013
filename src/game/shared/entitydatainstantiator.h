//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ENTITYDATAINSTANTIATOR_H
#define ENTITYDATAINSTANTIATOR_H
#ifdef _WIN32
#pragma once
#endif

#include "utlhash.h"

#include "tier0/memdbgon.h"

// This is the hash key type, but it could just as easily be and int or void *
class CBaseEntity;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
abstract_class IEntityDataInstantiator
{
public:
	virtual ~IEntityDataInstantiator() {};

	virtual void *GetDataObject( const CBaseEntity *instance ) = 0;
	virtual void *CreateDataObject( const CBaseEntity *instance ) = 0;
	virtual void DestroyDataObject( const CBaseEntity *instance ) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template <class T>
class CEntityDataInstantiator : public IEntityDataInstantiator
{
public:
	CEntityDataInstantiator() : 
		m_HashTable( 64, 0, 0, CompareFunc, KeyFunc )
	{
	}

	virtual void *GetDataObject( const CBaseEntity *instance )
	{
		UtlHashHandle_t handle; 
		HashEntry entry;
		entry.key = instance;
		handle = m_HashTable.Find( entry );

		if ( handle != m_HashTable.InvalidHandle() )
		{
			return (void *)m_HashTable[ handle ].data;
		}

		return NULL;
	}

	virtual void *CreateDataObject( const CBaseEntity *instance )
	{
		UtlHashHandle_t handle; 
		HashEntry entry;
		entry.key = instance;
		handle = m_HashTable.Find( entry );

		// Create it if not already present
		if ( handle == m_HashTable.InvalidHandle() )
		{
			handle = m_HashTable.Insert( entry );
			Assert( handle != m_HashTable.InvalidHandle() );
			m_HashTable[ handle ].data = new T;
	
			// FIXME: We'll have to remove this if any objects we instance have vtables!!!
			Q_memset( m_HashTable[ handle ].data, 0, sizeof( T ) );
		}

		return (void *)m_HashTable[ handle ].data;
	}

	virtual void DestroyDataObject( const CBaseEntity *instance )
	{
		UtlHashHandle_t handle; 
		HashEntry entry;
		entry.key = instance;
		handle = m_HashTable.Find( entry );

		if ( handle != m_HashTable.InvalidHandle() )
		{
			delete m_HashTable[ handle ].data;
			m_HashTable.Remove( handle );
		}
	}

private:

	struct HashEntry
	{
		HashEntry()
		{
			key = NULL;
			data = NULL;
		}

		const CBaseEntity *key;
		T				*data;
	};

	static bool CompareFunc( const HashEntry &src1, const HashEntry &src2 )
	{
		return ( src1.key == src2.key );
	}


	static unsigned int KeyFunc( const HashEntry &src )
	{
		// Shift right to get rid of alignment bits and border the struct on a 16 byte boundary
		uint nKey = (uint) ( ( uintp(src.key) & 0xFFFFFFFF ) >> 10 );
		return nKey;
	}

	CUtlHash< HashEntry >	m_HashTable;
};

#include "tier0/memdbgoff.h"

#endif // ENTITYDATAINSTANTIATOR_H
