#ifndef CGSTRING_PLAYER_H
#define CGSTRING_PLAYER_H

#include "hl2_player.h"

class CGstringPlayer : public CHL2_Player
{
	DECLARE_CLASS( CGstringPlayer, CHL2_Player );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
public:

	CGstringPlayer();

	void Precache();

	bool IsNightvisionActive() const;
	void SetNightvisionActive( bool bActive );

	virtual void ImpulseCommands();
	virtual void PhysicsSimulate();

protected:

private:
	void ToggleNightvision();
	CNetworkVar( bool, m_bNightvisionActive );

	CNetworkVar( bool, m_bHasUseEntity );

};

inline CGstringPlayer *ToGstringPlayer( CBaseEntity *pPlayer )
{
	return assert_cast< CGstringPlayer* >( pPlayer );
}

inline CGstringPlayer *LocalGstringPlayer()
{
	return ToGstringPlayer( UTIL_GetLocalPlayer() );
}

#endif