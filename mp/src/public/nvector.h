//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef NVECTOR_H
#define NVECTOR_H
#ifdef _WIN32
#pragma once
#endif


#include <math.h>


#define NVectorN	NVector<N>
#define NVector3	NVector<3>


// N-dimensional vector.
template<int N>
class NVector
{
public:

					NVectorN() {}

	float&			operator[]( int i );
	float const&	operator[]( int i ) const;
	
	float			Dot( NVectorN const &b ) const;
	NVectorN		Cross( NVectorN const &b ) const;
	NVectorN		Normalize() const;
	float			Length() const;

	NVectorN		operator-() const;
	NVectorN		operator+( NVectorN const &b ) const;
	NVectorN const &operator+=( NVectorN const &b );
	NVectorN		operator-( NVectorN const &b ) const;
	NVectorN		operator*( float val ) const;


// Static helpers.
public:

	static NVectorN	SetupNVectorNull();	// Returns a vector of all zeros.


public:
	
	float			v[N];
};


template<int N>
inline float NDot( NVector<N> const &a, NVector<N> const &b )
{
	float ret = 0;
	for( int i=0; i < N; i++ )
		ret += a.v[i] * b.v[i];
	return ret;
}

template<int N>
Vector& ToVec( NVector<N> &vec )			{assert( N >= 3 ); return *((Vector*)&vec);}

template<int N>
Vector const& ToVec( NVector<N> const &vec ){assert( N >= 3 ); return *((Vector const*)&vec);}

NVector<3>&			ToNVec( Vector &vec )		{return *((NVector<3>*)&vec);}
NVector<3> const&	ToNVec( Vector const &vec )	{return *((NVector<3> const*)&vec);}


// ------------------------------------------------------------------------------------ //
// NVector inlines.
// ------------------------------------------------------------------------------------ //

template<int N>
NVectorN NVectorN::SetupNVectorNull()
{
	NVector<N> ret;
	memset( ret.v, 0, sizeof(float)*N );
	return ret;
}


template<int N>
float& NVectorN::operator[]( int i )
{
	assert( i >= 0 && i < N );
	return v[i];
}


template<int N>
float const& NVectorN::operator[]( int i ) const
{
	assert( i >= 0 && i < N );
	return v[i];
}


template<int N>
float NVectorN::Dot( NVectorN const &b ) const
{
	float ret = 0;
	
	for( int i=0; i < N; i++ )
		ret += v[i]*b.v[i];

	return ret;
}


template<int N>
NVectorN NVectorN::Cross( NVectorN const &b ) const
{
	NVector<N> ret;
	NMatrix<N-1> mat;
	
	for( int i=0; i < N; i++ )
	{
		for( y=0; y < N; y++ )
			for( x=0; x < N; x++ )
				mat.m[y][x] = 

		ret.v[i] = v[i]*b.v[i];
	}

	return ret;
}


template<int N>
NVectorN NVectorN::Normalize() const
{
	return *this * (1.0f / Length());
}


template<int N>
float NVectorN::Length() const
{
	return (float)sqrt( Dot(*this) );
}


template<int N>
NVectorN NVectorN::operator-() const
{
	NVectorN ret;
	for( int i=0; i < N; i++ )
		ret.v[i] = -v[i];
	return ret;
}


template<int N>
NVectorN NVectorN::operator+( NVectorN const &b ) const
{
	NVectorN ret;
	
	for( int i=0; i < N; i++ )
		ret.v[i] = v[i]+b.v[i];

	return ret;
}


template<int N>
NVectorN const &NVectorN::operator+=( NVectorN const &b )
{
	for( int i=0; i < N; i++ )
		v[i] += b.v[i];
	return *this;
}


template<int N>
NVectorN NVectorN::operator-( NVectorN const &b ) const
{
	NVectorN ret;
	
	for( int i=0; i < N; i++ )
		ret.v[i] = v[i]-b.v[i];

	return ret;
}

template<int N>
NVectorN NVectorN::operator*( float val ) const
{
	NVectorN ret;
	for( int i=0; i < N; i++ )
		ret.v[i] = v[i] * val;
	return ret;
}


#endif // NVECTOR_H

