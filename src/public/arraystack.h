//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef ARRAYSTACK_H
#define ARRAYSTACK_H
#pragma once

#include <assert.h>
#include "List.h"

template <class T> class ArrayStack
{
protected:
	T *data;
	int m_stackDepth;
	int m_maxNumElements;
	
public:
	ArrayStack( int maxNumElements )
	{
		data = new T[maxNumElements];
		m_maxNumElements = maxNumElements;
		m_stackDepth = 0;
		assert( data );
	}
	
	void Push( T elem )
	{
		data[m_stackDepth++] = elem;
		if( m_stackDepth > m_maxNumElements )
		{
			printf( "ArrayStack overflow\n" );
			assert( 0 );
		}
	}
	
	T Pop( void )
	{
		if( m_stackDepth == 0 )
		{
			printf( "ArrayStack underflow\n" );
			assert( 0 );
		}
		return data[--m_stackDepth];
	}

	bool IsEmpty()
	{
		return ( m_stackDepth == 0 );
	}
	
	int GetDepth()
	{
		return m_stackDepth;
	}
};


#endif // ARRAYSTACK_H
