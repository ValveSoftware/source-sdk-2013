//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF AmmoPack.
//
//=============================================================================//
#ifndef TF_HALLOWEEN_SOULS_PICUP_H
#define TF_HALLOWEEN_SOULS_PICUP_H



#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
	#define CHalloweenSoulPack C_HalloweenSoulPack
#endif


class CHalloweenSoulPack : public CBaseEntity
{
public:
	DECLARE_CLASS( CHalloweenSoulPack, CBaseEntity )

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CHalloweenSoulPack();
	~CHalloweenSoulPack();

	virtual void Spawn() OVERRIDE;
	virtual void Precache() OVERRIDE;

#ifdef GAME_DLL
	void SetAmount( int nAmount )			{ m_nAmount = nAmount; }
	void SetFlyDuration( float flDuration ) { m_flDuration = flDuration; }
	void SetTarget( CBaseEntity *pTarget ) 	{ m_hTarget = pTarget; }
	void ItemTouch( CBaseEntity *pOther );
	virtual int UpdateTransmitState() OVERRIDE;
#else
	virtual void OnDataChanged( DataUpdateType_t type ) OVERRIDE;
	virtual void ClientThink() OVERRIDE;
#endif
private:
	void FlyThink( void );
	void FlyTowardsTargetEntity( void );
	void InitSplineData( void );

#ifdef GAME_DLL
	int		m_nAmount;
	const char *m_pszParticleName;
#endif
	CNetworkHandle( CBaseEntity, m_hTarget );
	float	m_flCreationTime;

	CNetworkVector( m_vecPreCurvePos );
	CNetworkVector( m_vecStartCurvePos );
	CNetworkVar( float, m_flDuration );
};


#endif // TF_HALLOWEEN_SOULS_PICUP_H
