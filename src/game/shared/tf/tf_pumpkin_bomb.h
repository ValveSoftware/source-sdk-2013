//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Pumpkin Bomb
//
//=============================================================================
#ifndef TF_PUMPKIN_BOMB_H
#define TF_PUMPKIN_BOMB_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#define CTFPumpkinBomb C_TFPumpkinBomb
#endif

class CTFPumpkinBombShim : public CBaseAnimating
{
	virtual void PumpkinTouch( CBaseEntity *pOther ) = 0;
public:
	void	Touch( CBaseEntity *pOther ) { return PumpkinTouch( pOther ) ; }
};

DECLARE_AUTO_LIST( ITFPumpkinBomb );

class CTFPumpkinBomb : public CTFPumpkinBombShim, public ITFPumpkinBomb
{
	DECLARE_CLASS( CTFPumpkinBomb, CBaseAnimating );
	DECLARE_NETWORKCLASS();

public:
	CTFPumpkinBomb();
	~CTFPumpkinBomb() {}

	virtual void	Precache( void );
	virtual void	Spawn( void );
	void			Break( void );
	virtual void	PumpkinTouch( CBaseEntity *pOther ) OVERRIDE;

	void			SetInitParams( float scale, float damage, float radius, int iTeam, float flLifeTime );

	void			RemovePumpkin();

#ifdef GAME_DLL
	virtual void	Event_Killed( const CTakeDamageInfo &info );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );

	void			SetSpell( bool bSpell ) { m_bIsSpell = bSpell; }

	virtual bool	IsProjectileCollisionTarget( void ) const OVERRIDE { return true; }
#endif

private:
#ifdef GAME_DLL
	bool			m_bIsSpell;
#endif

	bool			m_bDead;
	bool			m_bPrecached;

	int				m_iTeam;
	float			m_flDamage;
	float			m_flScale;
	float			m_flRadius;
	float			m_flLifeTime;
};

#endif	//TF_PUMPKIN_BOMB_H
