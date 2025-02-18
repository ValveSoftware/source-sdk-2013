//========= Copyright, Valve Corporation, All rights reserved. ================//
//
// std::pair style container; exists to work easily in our CUtlMap/CUtlHashMap classes
//
//=============================================================================//

#ifndef UTLPAIR_H
#define UTLPAIR_H

#ifdef _WIN32
#pragma once
#endif


// std::pair style container; exists to work easily in our CUtlMap/CUtlHashMap classes
template<typename T1, typename T2>
class CUtlPair
{
public:
	CUtlPair() {}
	CUtlPair( T1 t1, T2 t2 ) : first( t1 ), second( t2 ) {}

	bool operator<( const CUtlPair<T1,T2> &rhs ) const {
		if ( first != rhs.first )
			return first < rhs.first;
		return second < rhs.second;
	}

	bool operator==( const CUtlPair<T1,T2> &rhs ) const {
		return first == rhs.first && second == rhs.second;
	}

	T1 first;
	T2 second;
};

// utility to make a CUtlPair without having to specify template parameters
template<typename T1, typename T2>
inline CUtlPair<T1,T2> MakeUtlPair( T1 t1, T2 t2 ) 
{ 
	return CUtlPair<T1,T2>(t1, t2); 
}

//// HashItem() overload that works automatically with our hash containers
//template<typename T1, typename T2>
//inline uint32 HashItem( const CUtlPair<T1,T2> &item )
//{
//	return HashItem( (uint64)HashItem( item.first ) + ((uint64)HashItem( item.second ) << 32) );
//}


#endif // UTLPAIR_H
