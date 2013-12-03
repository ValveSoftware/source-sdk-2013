//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#if !defined( ICLIENTENTITYLIST_H )
#define ICLIENTENTITYLIST_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

class IClientEntity;
class ClientClass;
class IClientNetworkable;
class CBaseHandle;
class IClientUnknown;


//-----------------------------------------------------------------------------
// Purpose: Exposes IClientEntity's to engine
//-----------------------------------------------------------------------------
abstract_class IClientEntityList
{
public:
	// Get IClientNetworkable interface for specified entity
	virtual IClientNetworkable*	GetClientNetworkable( int entnum ) = 0;
	virtual IClientNetworkable*	GetClientNetworkableFromHandle( CBaseHandle hEnt ) = 0;
	virtual IClientUnknown*		GetClientUnknownFromHandle( CBaseHandle hEnt ) = 0;

	// NOTE: This function is only a convenience wrapper.
	// It returns GetClientNetworkable( entnum )->GetIClientEntity().
	virtual IClientEntity*		GetClientEntity( int entnum ) = 0;
	virtual IClientEntity*		GetClientEntityFromHandle( CBaseHandle hEnt ) = 0;

	// Returns number of entities currently in use
	virtual int					NumberOfEntities( bool bIncludeNonNetworkable ) = 0;

	// Returns highest index actually used
	virtual int					GetHighestEntityIndex( void ) = 0;

	// Sizes entity list to specified size
	virtual void				SetMaxEntities( int maxents ) = 0;
	virtual int					GetMaxEntities( ) = 0;
};

extern IClientEntityList *entitylist;

#define VCLIENTENTITYLIST_INTERFACE_VERSION	"VClientEntityList003"

#endif // ICLIENTENTITYLIST_H
