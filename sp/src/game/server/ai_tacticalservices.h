//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=============================================================================

#ifndef AI_TACTICALSERVICES_H
#define AI_TACTICALSERVICES_H

#include "ai_component.h"

#if defined( _WIN32 )
#pragma once
#endif

class CAI_Network;
class CAI_Pathfinder;


enum FlankType_t
{
	FLANKTYPE_NONE = 0,
	FLANKTYPE_ARC,			// Stay flFlankParam degrees of arc away from vecFlankRefPos
	FLANKTYPE_RADIUS,		// Stay flFlankParam units away from vecFlankRefPos
};


//-----------------------------------------------------------------------------

class CAI_TacticalServices : public CAI_Component
{
public:
	CAI_TacticalServices( CAI_BaseNPC *pOuter )
	 :	CAI_Component(pOuter),
		m_pNetwork( NULL )
	{
		m_bAllowFindLateralLos = true;
	}
	
	void Init( CAI_Network *pNetwork );

	bool			FindLos( const Vector &threatPos, const Vector &threatEyePos, float minThreatDist, float maxThreatDist, float blockTime, Vector *pResult );
	bool			FindLos( const Vector &threatPos, const Vector &threatEyePos, float minThreatDist, float maxThreatDist, float blockTime, FlankType_t eFlankType, const Vector &VecFlankRefPos, float flFlankParam, Vector *pResult );
	bool			FindLateralLos( const Vector &threatPos, Vector *pResult );
	bool			FindBackAwayPos( const Vector &vecThreat, Vector *pResult );
	bool			FindCoverPos( const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist, Vector *pResult );
	bool			FindCoverPos( const Vector &vNearPos, const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist, Vector *pResult );
	bool			FindLateralCover( const Vector &vecThreat, float flMinDist, Vector *pResult );
	bool			FindLateralCover( const Vector &vecThreat, float flMinDist, float distToCheck, int numChecksPerDir, Vector *pResult );
	bool			FindLateralCover( const Vector &vNearPos, const Vector &vecThreat, float flMinDist, float distToCheck, int numChecksPerDir, Vector *pResult );

	void			AllowFindLateralLos( bool bAllow ) { m_bAllowFindLateralLos = bAllow; }

private:
	// Checks lateral cover
	bool			TestLateralCover( const Vector &vecCheckStart, const Vector &vecCheckEnd, float flMinDist );
	bool			TestLateralLos( const Vector &vecCheckStart, const Vector &vecCheckEnd );

	int				FindBackAwayNode( const Vector &vecThreat );
	int				FindCoverNode( const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist );
	int				FindCoverNode( const Vector &vNearPos, const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist );
	int				FindLosNode( const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinThreatDist, float flMaxThreatDist, float flBlockTime, FlankType_t eFlankType, const Vector &vThreatFacing, float flFlankParam );
	
	Vector			GetNodePos( int );

	CAI_Network *GetNetwork()				{ return m_pNetwork; }
	const CAI_Network *GetNetwork() const	{ return m_pNetwork; }

	CAI_Pathfinder *GetPathfinder()				{ return m_pPathfinder; }
	const CAI_Pathfinder *GetPathfinder() const	{ return m_pPathfinder; }
	
	CAI_Network *m_pNetwork;
	CAI_Pathfinder *m_pPathfinder;

	bool	m_bAllowFindLateralLos;	// Allows us to turn Lateral LOS checking on/off. 

	DECLARE_SIMPLE_DATADESC();
};

//-----------------------------------------------------------------------------

#endif // AI_TACTICALSERVICES_H
