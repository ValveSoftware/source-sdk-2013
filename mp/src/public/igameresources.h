//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: IGameResources interface
//
// $NoKeywords: $
//=============================================================================//

#ifndef IGAMERESOURCES_H
#define IGAMERESOURCES_H

class Color;
class Vector;


abstract_class IGameResources
{
public:
	virtual	~IGameResources() {};

	// Team data access 
	virtual const char		*GetTeamName( int index ) = 0;
	virtual int				GetTeamScore( int index ) = 0;
	virtual const Color&	GetTeamColor( int index ) = 0;

	// Player data access
	virtual bool	IsConnected( int index ) = 0;
	virtual bool	IsAlive( int index ) = 0;
	virtual bool	IsFakePlayer( int index ) = 0;
	virtual bool	IsLocalPlayer( int index ) = 0;

	virtual const char *GetPlayerName( int index ) = 0;
	virtual int		GetPlayerScore( int index ) = 0;
	virtual int		GetPing( int index ) = 0;
//	virtual int		GetPacketloss( int index ) = 0;
	virtual int		GetDeaths( int index ) = 0;
	virtual int		GetFrags( int index ) = 0;
	virtual int		GetTeam( int index ) = 0;
	virtual int		GetHealth( int index ) = 0;
};

extern IGameResources *GameResources( void ); // singelton accessor

#endif

