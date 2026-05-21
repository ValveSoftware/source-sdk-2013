//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Rainbow Is Magic mod: team objective "Mr. Bear" prop.
//
//=============================================================================
#ifndef TF_MR_BEAR_H
#define TF_MR_BEAR_H

class CTFMrBear : public CBaseAnimating
{
	DECLARE_CLASS( CTFMrBear, CBaseAnimating );
	DECLARE_SERVERCLASS();

public:
	CTFMrBear();

	virtual void	Spawn( void );
	virtual void	Precache( void );
	void			BearBeaconThink( void );
	void			OutdoorVerifyThink( void );
	virtual void	Event_Killed( const CTakeDamageInfo &info );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual bool	IsAlive( void ) OVERRIDE { return m_lifeState == LIFE_ALIVE && GetHealth() > 0; }
	virtual bool	IsProjectileCollisionTarget( void ) const OVERRIDE { return true; }

	static CTFMrBear *Create( const Vector &vecOrigin, const QAngle &angles, int iTeam );
	static bool		FindSpawnLocation( int iTeam, Vector &vecOrigin, QAngle &angles );
	static bool		FindGuaranteedSpawnLocation( int iTeam, Vector &vecOrigin, QAngle &angles );
	static bool		FinalizeOutdoorPlacement( CTFMrBear *pBear );

	static void	RemoveAllMrBears( void );

	static bool	FindRelaxedSpawnLocation( int iTeam, Vector &vecOrigin, QAngle &angles );
	static void	RemoveAllRimObjectives( void );
};

#endif // TF_MR_BEAR_H
