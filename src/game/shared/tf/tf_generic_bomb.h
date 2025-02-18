//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Generic Bomb
//
//=============================================================================
#ifndef TF_GENERIC_BOMB_H
#define TF_GENERIC_BOMB_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#define CTFGenericBomb C_TFGenericBomb
#endif

class CTFGenericBombShim : public CBaseAnimating
{
	virtual void GenericTouch( CBaseEntity *pOther ) = 0;
public:
	void	Touch( CBaseEntity *pOther ) { return GenericTouch( pOther ) ; }
};

DECLARE_AUTO_LIST( ITFGenericBomb );

class CTFGenericBomb : public CTFGenericBombShim, public ITFGenericBomb
{
	DECLARE_CLASS( CTFGenericBomb, CBaseAnimating );
	DECLARE_NETWORKCLASS();

	enum EWhoToDamage
	{
		DAMAGE_ATTACKER_AND_ATTACKER_ENEMIES,
		DAMAGE_EVERYONE
	};

public:
	CTFGenericBomb();
	~CTFGenericBomb() {}

	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual void	GenericTouch( CBaseEntity *pOther ) OVERRIDE;

#ifdef GAME_DLL
	DECLARE_DATADESC();

	virtual void	Event_Killed( const CTakeDamageInfo &info );
	virtual bool	IsProjectileCollisionTarget( void ) const OVERRIDE { return true; }
#endif

private:
#ifdef GAME_DLL
	void Detonate( inputdata_t& inputdata );
	COutputEvent m_OnDetonate;
#endif

	bool			m_bDead;
	bool			m_bPrecached;

	int				m_iTeam;
	float			m_flDamage;
	int				m_nHealth;
	float			m_flRadius;
	string_t		m_strExplodeParticleName;
	string_t		m_strHitParticleName;
	string_t		m_strExplodeSoundName;
	EWhoToDamage	m_eWhoToDamage;
	bool			m_bPassActivator;
};

#endif	//TF_GENERIC_BOMB_H
