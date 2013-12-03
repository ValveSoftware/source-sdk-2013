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
#if !defined( BASETEMPENTITY_H )
#define BASETEMPENTITY_H
#ifdef _WIN32
#pragma once
#endif

#include "edict.h"

// This is the base class for TEMP ENTITIES that use the 
//  event system to propagate
class CBaseTempEntity
{
public:
	DECLARE_CLASS_NOBASE( CBaseTempEntity );
	DECLARE_SERVERCLASS();

						CBaseTempEntity( const char *name );
	virtual				~CBaseTempEntity( void );

	const char			*GetName( void );

	// Force all derived classes to implement a test
	virtual void		Test( const Vector& current_origin, const QAngle& current_angles );

	virtual	void		Create( IRecipientFilter& filter, float delay = 0.0 );

	virtual void		Precache( void );

	CBaseTempEntity		*GetNext( void );

	// Get list of tempentities
	static CBaseTempEntity *GetList( void );

	// Called at startup to allow temp entities to precache any models/sounds that they need
	static void			PrecacheTempEnts( void );

	void NetworkStateChanged() {}	// TE's are sent out right away so we don't track whether state changes or not,
									// but we want to allow CNetworkVars.
	void NetworkStateChanged( void *pVar ) {}

private:
	// Descriptive name, for when running tests
	const char			*m_pszName;

	// Next in chain
	CBaseTempEntity		*m_pNext;

	// ConVars add themselves to this list for the executable. Then ConVarMgr::Init() runs through 
	// all the console variables and registers them.
	static CBaseTempEntity	*s_pTempEntities;
};

#endif // BASETEMPENTITY_H
