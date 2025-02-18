//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base class for object upgrading objects
//
//=============================================================================//

#ifndef TF_OBJ_BASEUPGRADE_H
#define TF_OBJ_BASEUPGRADE_H
#ifdef _WIN32
#pragma once
#endif


#include "baseobject_shared.h"
#include "takedamageinfo.h"

#if defined( CLIENT_DLL )
#define CBaseObjectUpgrade C_BaseObjectUpgrade
#endif

// ------------------------------------------------------------------------ //
// Base class for object upgrading objects
// ------------------------------------------------------------------------ //
class CBaseObjectUpgrade : public CBaseObject
{
DECLARE_CLASS( CBaseObjectUpgrade, CBaseObject );

public:
	DECLARE_NETWORKCLASS();

	CBaseObjectUpgrade();

	virtual void	Spawn( void );
	virtual bool	IsAnUpgrade( void )			{ return true; }
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );

private:
	CBaseObjectUpgrade( const CBaseObjectUpgrade & ); // not defined, not accessible
};

#endif // TF_OBJ_BASEUPGRADE_H
