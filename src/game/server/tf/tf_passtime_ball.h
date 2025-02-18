//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_PASSTIME_BALL_H
#define TF_PASSTIME_BALL_H
#ifdef _WIN32
#pragma once
#endif

#include "passtime_ballcontroller_playerseek.h"
#include "predictable_entity.h"
#include "util_shared.h"
#include "baseanimating.h"
#include "utllinkedlist.h"

class CSpriteTrail;
class CBallPlayerToucher;
//-----------------------------------------------------------------------------
class CPasstimeBall : public CBaseAnimating
{
public:
	DECLARE_CLASS( CPasstimeBall, CBaseAnimating );
	DECLARE_NETWORKCLASS();
	CPasstimeBall();
	~CPasstimeBall();

	virtual void Spawn() OVERRIDE;
	virtual void Precache() OVERRIDE;
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent ) OVERRIDE;
	virtual int	OnTakeDamage( const CTakeDamageInfo &info ) OVERRIDE;
	virtual unsigned int PhysicsSolidMaskForEntity() const OVERRIDE;
	virtual void ChangeTeam( int iTeamNum ) OVERRIDE;
	virtual bool IsDeflectable() OVERRIDE;
	virtual void Deflected( CBaseEntity *pDeflectedBy, Vector& vecDir ) OVERRIDE;
	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const OVERRIDE;
	virtual int UpdateTransmitState() OVERRIDE;

	CTFPlayer *GetCarrier() const;
	CTFPlayer *GetPrevCarrier() const;
	CTFPlayer *GetThrower() const;
	int GetCollisionCount() const;
	int GetCarryDuration() const;

	void ResetTrail();
	void HideTrail();

	void MoveTo( const Vector &pos, const Vector &vel );
	void MoveToSpawner( const Vector &pos );

	void SetStateOutOfPlay();
	void SetStateFree();
	void SetStateCarried( CTFPlayer *pCarrier );
	bool BOutOfPlay() const;

	static CPasstimeBall *Create( Vector position, QAngle angles );

	void SetHomingTarget( CTFPlayer *pPlayer );
	CTFPlayer *GetHomingTarget() const;
	float GetAirtimeSec() const;
	float GetAirtimeDistance() const;

	void StartLagCompensation( CBasePlayer *player, CUserCmd *cmd );
	void FinishLagCompensation( CBasePlayer *player );

private:
	friend class CBallPlayerToucher;
	void OnTouch( CBaseEntity *pOther );
	void DefaultThink();
	void TouchPlayer( CTFPlayer *pPlayer );
	void BlockReflect( CTFPlayer *pPlayer, const Vector& origin, const Vector& ballvel );
	void BlockDamage( CTFPlayer *pPlayer, const Vector& ballvel );
	bool BIgnorePlayer( CTFPlayer *pPlayer );
	void OnCollision();
	void UpdateLagCompensationHistory();
	void SetThrower( CTFPlayer *pPlayer );
	void OnBecomeNotCarried();
	void SetIdleRespawnTime();
	void DisableIdleRespawnTime();
	bool BShouldPanicRespawn() const;
	bool CreateModelCollider();
	void CreateSphereCollider();

	enum EState
	{
		STATE_OUT_OF_PLAY,
		STATE_FREE,
		STATE_CARRIED
	};
	
	EState m_eState;
	CHandle<CTFPlayer> m_hThrower;
	EHANDLE m_hBlocker;
	CSpriteTrail *m_pTrail;
	bool m_bTrailActive;
	bool m_bLeftOwner;
	CSoundPatch	*m_pHumLoop;
	CSoundPatch	*m_pBeepLoop;
	CBaseEntity *m_pPlayerToucher;
	CPasstimeBallControllerPlayerSeek m_playerSeek;
	bool m_bTouchedSinceSpawn;
	float m_flLastCollisionTime;
	float m_flAirtimeDistance;
	Vector m_vecPrevOrigin; // note: C_BaseEntity has m_vecOldOrigin in client code only
	float m_flLastTeamChangeTime; // for stats
	float m_flBeginCarryTime;
	float m_flIdleRespawnTime;

	struct LagRecord 
	{
		float flSimulationTime;
		Vector vecOrigin;
	};
	
	CUtlFixedLinkedList<LagRecord> m_lagCompensationHistory;
	LagRecord m_lagCompensationRestore;
	bool m_bLagCompensationNeedsRestore;
	float m_flLagCompensationTeleportDistanceSqr;

	CNetworkVar( int, m_iCollisionCount );
	CNetworkHandle( CTFPlayer, m_hHomingTarget );
	CNetworkHandle( CTFPlayer, m_hCarrier );
	CNetworkHandle( CTFPlayer, m_hPrevCarrier );
};

#endif // TF_PASSTIME_BALL_H  
