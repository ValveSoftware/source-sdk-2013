//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Used to influence the initial force for a dying NPC's ragdoll. 
//			Passive entity. Just represents position in the world, radius, force
//
// $NoKeywords: $
//=============================================================================//
#pragma once

#ifndef CRAGDOLLMAGNET_H
#define CRAGDOLLMAGNET_H

#define SF_RAGDOLLMAGNET_BAR	0x00000002	// this is a bar magnet.

class CRagdollMagnet : public CPointEntity
{
public:
	DECLARE_CLASS( CRagdollMagnet, CPointEntity );
	DECLARE_DATADESC();

	Vector GetForceVector( CBaseEntity *pNPC );
	float GetRadius( void ) { return m_radius; }
	Vector GetAxisVector( void ) { return m_axis - GetAbsOrigin(); }
	float DistToPoint( const Vector &vecPoint );

	bool IsEnabled( void ) { return !m_bDisabled; }
	
	int IsBarMagnet( void ) { return (m_spawnflags & SF_RAGDOLLMAGNET_BAR); }

	static CRagdollMagnet *FindBestMagnet( CBaseEntity *pNPC );

	void Enable( bool bEnable ) { m_bDisabled = !bEnable; }

	// Inputs
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

private:
	bool	m_bDisabled;
	float	m_radius;
	float	m_force;
	Vector	m_axis;
};

#endif //CRAGDOLLMAGNET_H
