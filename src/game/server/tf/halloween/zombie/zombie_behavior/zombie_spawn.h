//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef ZOMBIE_SPAWN_H
#define ZOMBIE_SPAWN_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CZombieSpawn : public Action< CZombie >
{
public:
	virtual ActionResult< CZombie >	OnStart( CZombie *me, Action< CZombie > *priorAction );
	virtual ActionResult< CZombie >	Update( CZombie *me, float interval );

	virtual const char *GetName( void ) const	{ return "Spawn"; }		// return name of this action

private:
};

#endif // ZOMBIE_SPAWN_H
