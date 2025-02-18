//========= Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ============//
#ifndef TIER1_HEAP_SORT
#define TIER1_HEAP_SORT

template <typename T, typename LessThan>
inline bool IsHeap( T *pArr, int nCount, const LessThan &lessThan )
{
	for ( int i = 1; i < nCount; ++i )
	{
		if ( lessThan( pArr[ ( i + 1 ) / 2 - 1 ], pArr[ i ] ) )
			return false;
	}
	return true;
}

template <typename T, typename LessThan>
inline void HeapSort( T *pArr, int nCount, const LessThan &lessThan )
{
	// heapify
	for ( int nEndOfHeap = 1; nEndOfHeap < nCount; ++nEndOfHeap )
	{
		int nIdx = nEndOfHeap;
		do
		{
			int nParent = ( nIdx + 1 ) / 2 - 1;
			if ( lessThan( pArr[ nParent ], pArr[ nIdx ] ) )
			{
				V_swap( pArr[ nParent ], pArr[ nIdx ] );
			}
			else
			{
				break;
			}
			nIdx = nParent;
		}
		while ( nIdx > 0 );
	}
	//AssertDbg( IsHeap( pArr, nCount, lessThan ) );

	// heap sort
	for ( int nEndOfHeap = nCount; nEndOfHeap-- > 1; )
	{
		//AssertDbg( !lessThan( pArr[ 0 ], pArr[ nEndOfHeap ] ) );
		V_swap( pArr[ 0 ], pArr[ nEndOfHeap ] );
		// re-heapify the heap
		int nIdx = 0;
		for ( ;; )
		{
			int nChild = nIdx * 2 + 1;
			if ( nChild >= nEndOfHeap )
				break;
			if ( nChild + 1 < nEndOfHeap )
			{
				// we have 2 children to compare against
				if ( lessThan( pArr[ nChild ], pArr[ nChild + 1 ] ) )
				{
					nChild++; // always compare the root against the greater child
				}
			}
			if ( lessThan( pArr[ nIdx ], pArr[ nChild ] ) )
			{
				V_swap( pArr[ nIdx ], pArr[ nChild ] );
				nIdx = nChild;
			}
			else
			{
				// the root is greater than any children now, we finished re-heapifying
				break;
			}
		}
		//AssertDbg( IsHeap( pArr, nEndOfHeap, lessThan ) );
	}
}

template <typename Array, typename LessThan>
inline void HeapSort( Array& arr, const LessThan &lessThan )
{
	HeapSort( arr.Base( ), arr.Count( ), lessThan );
}


template <typename T, typename LessThan>
inline void BubbleSort( T *pArr, int nCount, const LessThan &lessThan )
{
	for ( int i = 0; i + 1 < nCount; ++i )
	{
		for ( int j = i; j < nCount; ++j )
		{
			if ( lessThan( pArr[ j ], pArr[ i ] ) )
			{
				Swap( pArr[ j ], pArr[ i ] );
			}
		}
	}

#ifdef DBGFLAG_ASSERT
	for ( int i = 1; i < nCount; ++i )
	{
		Assert( lessThan( pArr[ i - 1 ], pArr[ i ] ) || !lessThan( pArr[ i ], pArr[ i - 1 ] ) );
	}
#endif
}

/*
inline void HeapSortUnitTest( )
{
	RandomSeed( 1 );
	for ( uint i = 0; i < 100000; ++i )
	{
		CUtlVector< int >arr;
		arr.SetCount( RandomInt( 0, 10000 ) );
		for ( int j = 0; j < arr.Count( ); ++j )
			arr[ j ] = RandomInt( 0, INT_MAX );
		HeapSort( arr, [=] ( int a, int b ) ->bool { return a < b; } );
		for ( int j = 1; j < arr.Count( ); ++j )
			Assert( arr[ j - 1 ] <= arr[ j ] );
	}
}
*/



template <typename Array>
inline void RemoveDuplicates( Array &arr )
{
	// skip the first unique items
	int nUniques = 1;
	while ( nUniques < arr.Count( ) && !( arr[ nUniques ] == arr[ nUniques - 1 ] ) )
	{
		nUniques++;
	}
	if ( nUniques < arr.Count( ) )
	{
		// found the first duplicate; start copying unique items back
		for ( int i = nUniques + 1; i < arr.Count( ); ++i )
		{
			if ( !( arr[ nUniques - 1 ] == arr[ i ] ) )
			{
				arr[ nUniques ] = arr[ i ]; // found another unique item
				nUniques++;
			}
		}
		arr.SetCountNonDestructively( nUniques );
	}
}


#endif
