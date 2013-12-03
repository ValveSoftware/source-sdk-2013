//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A class to wrap data for transport over a boundary like a thread
//			or window.
//
//=============================================================================

#include "tier1/utlstring.h"
#include "tier0/basetypes.h"

#ifndef UTLENVELOPE_H
#define UTLENVELOPE_H

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------

class CUtlDataEnvelope
{
public:
	CUtlDataEnvelope( const void *pData, int nBytes );
	CUtlDataEnvelope( const CUtlDataEnvelope &from );
	~CUtlDataEnvelope();

	CUtlDataEnvelope &operator=( const CUtlDataEnvelope &from );

	operator void *();
	operator void *() const;

private:
	void Assign( const void *pData, int nBytes );
	void Assign( const CUtlDataEnvelope &from );
	void Purge();

	// TODO: switch to a reference counted array?
	union
	{
		byte *m_pData;
		byte m_data[4];
	};
	int m_nBytes;
};


//-----------------------------------------------------------------------------

template <typename T>
class CUtlEnvelope : protected CUtlDataEnvelope
{
public:
	CUtlEnvelope( const T *pData, int nElems = 1 );
	CUtlEnvelope( const CUtlEnvelope<T> &from );

	CUtlEnvelope<T> &operator=( const CUtlEnvelope<T> &from );

	operator T *();
	operator T *() const;

	operator void *();
	operator void *() const;
};

//-----------------------------------------------------------------------------

template <>
class CUtlEnvelope<const char *>
{
public:
	CUtlEnvelope( const char *pData )
	{
		m_string = pData;
	}

	CUtlEnvelope( const CUtlEnvelope<const char *> &from )
	{
		m_string = from.m_string;
	}

	CUtlEnvelope<const char *> &operator=( const CUtlEnvelope<const char *> &from )
	{
		m_string = from.m_string;
		return *this;
	}

	operator char *()
	{
		return (char *) m_string.Get();
	}

	operator char *() const
	{
		return (char *) m_string.Get();
	}

	operator void *()
	{
		return (void *) m_string.Get();
	}

	operator void *() const
	{
		return (void *) m_string.Get();
	}

private:
	CUtlString m_string;
};

//-----------------------------------------------------------------------------

#include "tier0/memdbgon.h"

inline void CUtlDataEnvelope::Assign( const void *pData, int nBytes )
{
	if ( pData )
	{
		m_nBytes = nBytes;
		if ( m_nBytes > 4 )
		{
			m_pData = new byte[nBytes];
			memcpy( m_pData, pData, nBytes );
		}
		else
		{
			memcpy( m_data, pData, nBytes );
		}
	}
	else
	{
		m_pData = NULL;
		m_nBytes = 0;
	}
}

inline void CUtlDataEnvelope::Assign( const CUtlDataEnvelope &from )
{
	Assign( from.operator void *(), from.m_nBytes );
}

inline void CUtlDataEnvelope::Purge()
{
	if (m_nBytes > 4)
		delete [] m_pData;
	m_nBytes = 0;
}

inline CUtlDataEnvelope::CUtlDataEnvelope( const void *pData, int nBytes )
{
	Assign( pData, nBytes );
}

inline CUtlDataEnvelope::CUtlDataEnvelope( const CUtlDataEnvelope &from )
{
	Assign( from );
}

inline CUtlDataEnvelope::~CUtlDataEnvelope()
{
	Purge();
}

inline CUtlDataEnvelope &CUtlDataEnvelope::operator=( const CUtlDataEnvelope &from )
{
	Purge();
	Assign( from );
	return *this;
}

inline CUtlDataEnvelope::operator void *()
{
	if ( !m_nBytes )
	{
		return NULL;
	}

	return ( m_nBytes > 4) ? m_pData : m_data;
}

inline CUtlDataEnvelope::operator void *() const
{
	if ( !m_nBytes )
	{
		return NULL;
	}

	return ( m_nBytes > 4) ? (void *)m_pData : (void *)m_data;
}

//-----------------------------------------------------------------------------

template <typename T>
inline CUtlEnvelope<T>::CUtlEnvelope( const T *pData, int nElems )
	: CUtlDataEnvelope( pData, sizeof(T) * nElems )
{
}

template <typename T>
inline CUtlEnvelope<T>::CUtlEnvelope( const CUtlEnvelope<T> &from )
	: CUtlDataEnvelope( from )
{
	
}

template <typename T>
inline CUtlEnvelope<T> &CUtlEnvelope<T>::operator=( const CUtlEnvelope<T> &from )
{
	CUtlDataEnvelope::operator=( from );
	return *this;
}

template <typename T>
inline CUtlEnvelope<T>::operator T *()
{
	return (T *)CUtlDataEnvelope::operator void *();
}

template <typename T>
inline CUtlEnvelope<T>::operator T *() const
{
	return (T *)( (const_cast<CUtlEnvelope<T> *>(this))->operator T *() );
}

template <typename T>
inline CUtlEnvelope<T>::operator void *()
{
	return CUtlDataEnvelope::operator void *();
}

template <typename T>
inline CUtlEnvelope<T>::operator void *() const
{
	return ( (const_cast<CUtlEnvelope<T> *>(this))->operator void *() );
}

//-----------------------------------------------------------------------------

#include "tier0/memdbgoff.h"

#endif // UTLENVELOPE_H
