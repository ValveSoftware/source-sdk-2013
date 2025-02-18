//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef ARCHER_PROXY_H
#define ARCHER_PROXY_H

class CTFPlayer;

class CTFArcherProxy : public CBaseAnimating
{
public:
	DECLARE_CLASS( CTFArcherProxy, CBaseAnimating );

	virtual void Precache( void );
	virtual void Spawn( void );

	void Update( void );

	void ShootArrowAt( CBaseEntity *target );
	void ShootGrenadeAt( CBaseEntity *target );

protected:
	CTFPlayer *SelectTarget( void );

	enum BehaviorStateType
	{
		HIDDEN,
		EMERGE,
		AIM_AND_FIRE,
		HIDE,
	}
	m_state;

	CountdownTimer m_timer;
	Vector m_homePos;
};

#endif // ARCHER_PROXY_H
