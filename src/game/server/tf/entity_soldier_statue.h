//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Dove entity for the Meet the Medic tease.
//
//=============================================================================//
#ifndef ENTITY_SOLDIER_STATUE_H
#define ENTITY_SOLDIER_STATUE_H

#ifdef _WIN32
#pragma once
#endif

DECLARE_AUTO_LIST( ISoldierStatueAutoList );


class CEntitySoldierStatue : public CBaseAnimating, public ISoldierStatueAutoList
{
	DECLARE_CLASS( CEntitySoldierStatue, CBaseAnimating );

public:
	virtual void	Spawn( void ) OVERRIDE;
	virtual void	Precache( void ) OVERRIDE;

private:
	void			StatueThink();
	void			PlaySound( void );

private: 
	float m_flNextSpeakTime = -1.f;
};

#endif // ENTITY_SOLDIER_STATUE_H


