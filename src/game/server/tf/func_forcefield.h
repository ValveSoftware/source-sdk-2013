//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef FUNC_FORCEFIELD_H
#define FUNC_FORCEFIELD_H
#ifdef _WIN32
#pragma once
#endif

#include "modelentities.h"

//-----------------------------------------------------------------------------
// Purpose: Visualizes a forcefield
//-----------------------------------------------------------------------------
DECLARE_AUTO_LIST( IFuncForceFieldAutoList );

class CFuncForceField : public CFuncBrush, public IFuncForceFieldAutoList
{
	DECLARE_CLASS( CFuncForceField, CFuncBrush );
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	virtual void	Spawn( void ) OVERRIDE;

	virtual int		UpdateTransmitState( void ) OVERRIDE;
	virtual int		ShouldTransmit( const CCheckTransmitInfo *pInfo ) OVERRIDE;
	virtual bool	ShouldCollide( int collisionGroup, int contentsMask ) const OVERRIDE;

	virtual void	TurnOff( void ) OVERRIDE;
	virtual void	TurnOn( void ) OVERRIDE;

private:
	void			SetActive( bool bActive );
};

bool PointsCrossForceField( const Vector& vecStart, const Vector &vecEnd, int nTeamToIgnore = TEAM_UNASSIGNED );
#endif // FUNC_FORCEFIELD_H
