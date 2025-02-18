//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IPREDICTIONSYSTEM_H
#define IPREDICTIONSYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "predictable_entity.h"

class CBaseEntity;

//-----------------------------------------------------------------------------
// Purpose: Interfaces derived from this are able to filter out the local player
//  when doing prediction on the client, this includes not sending network data to
//  the local player from the server if needed.
//-----------------------------------------------------------------------------
class IPredictionSystem
{
public:
	IPredictionSystem()
	{
		m_pNextSystem = g_pPredictionSystems;
		g_pPredictionSystems = this;

		m_bSuppressEvent = false;
		m_pSuppressHost = NULL;

		m_nStatusPushed = 0;
	};

	virtual ~IPredictionSystem() {};

	IPredictionSystem *GetNext()
	{
		return m_pNextSystem;
	}

	void SetSuppressEvent( bool state )
	{
		m_bSuppressEvent = state;
	}

	void SetSuppressHost( CBaseEntity *host )
	{
		m_pSuppressHost = host;
	}

	CBaseEntity const *GetSuppressHost( void )
	{
		if ( DisableFiltering() )
		{
			return NULL;
		}

		return m_pSuppressHost;
	}

	bool CanPredict( void ) const
	{
		if ( DisableFiltering() )
		{
			return false;
		}

		return !m_bSuppressEvent;
	}

	static IPredictionSystem *g_pPredictionSystems;

	static void SuppressEvents( bool state )
	{
		IPredictionSystem *sys = g_pPredictionSystems;
		while ( sys )
		{
			sys->SetSuppressEvent( state );
			sys = sys->GetNext();
		}
	}

	static void SuppressHostEvents( CBaseEntity *host )
	{
		IPredictionSystem *sys = g_pPredictionSystems;
		while ( sys )
		{
			sys->SetSuppressHost( host );
			sys = sys->GetNext();
		}
	}

private:

	static void Push( void )
	{
		IPredictionSystem *sys = g_pPredictionSystems;
		while ( sys )
		{
			sys->_Push();
			sys = sys->GetNext();
		}
	}

	static void Pop( void )
	{
		IPredictionSystem *sys = g_pPredictionSystems;
		while ( sys )
		{
			sys->_Pop();
			sys = sys->GetNext();
		}
	}

	void _Push( void )
	{
		++m_nStatusPushed;
	}
	void _Pop( void )
	{
		--m_nStatusPushed;
	}

	bool DisableFiltering( void ) const
	{
		return ( m_nStatusPushed > 0  ) ? true : false;
	}

	IPredictionSystem	*m_pNextSystem;
	bool				m_bSuppressEvent;
	CBaseEntity			*m_pSuppressHost;

	int					m_nStatusPushed;

	friend class CDisablePredictionFiltering;
};

class CDisablePredictionFiltering
{
public:
	CDisablePredictionFiltering( bool disable = true )
	{
		m_bDisabled = disable;
		if ( m_bDisabled )
		{
			IPredictionSystem::Push();
		}
	}

	~CDisablePredictionFiltering( void )
	{
		if ( m_bDisabled )
		{
			IPredictionSystem::Pop();
		}
	}
private:
	bool	m_bDisabled;
};

#endif // IPREDICTIONSYSTEM_H
