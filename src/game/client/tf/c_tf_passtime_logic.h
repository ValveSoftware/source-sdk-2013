//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_TF_PASSTIME_LOGIC_H
#define C_TF_PASSTIME_LOGIC_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"
#include "c_tf_passtime_ball.h"
#include "utlvector.h"

//-----------------------------------------------------------------------------
class C_LocalTempEntity;
class C_TFPlayer;
class C_PasstimeReticle;
class C_TFPasstimeLogic : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_TFPasstimeLogic, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_TFPasstimeLogic();
	virtual ~C_TFPasstimeLogic();
	virtual void Spawn() OVERRIDE;
	virtual void ClientThink() OVERRIDE;
	virtual void PostDataUpdate( DataUpdateType_t updateType ) OVERRIDE;

	C_PasstimeBall *GetBall() const { return m_hBall.Get(); }
	void GetTrackPoints( Vector (&points)[16] );
	int GetNumSections() const { return m_iNumSections; }
	int GetCurrentSection() const { return m_iCurrentSection; }

	bool GetBallReticleTarget( C_BaseEntity **ppEnt, bool *bHomingActive ) const;
	bool BCanPlayerPickUpBall( C_TFPlayer *pPlayer ) const;

	float GetMaxPassRange() const { return m_flMaxPassRange; }
	int GetBallPower() const { return m_iBallPower; }

private:
	bool GetImportantEntities( C_PasstimeBall **ppBall, C_TFPlayer **ppCarrier, C_TFPlayer **ppHomingTarget ) const;
	void DestroyBeam( int i, C_PasstimeBall *pBall );
	
	void DestroyBeams( C_PasstimeBall *pBall );
	void UpdateBeams();

	C_PasstimeReticle *m_pBallReticle;
	CUtlVector<C_PasstimeReticle*> m_pGoalReticles;
	C_PasstimeReticle *m_pPassReticle;
	CNewParticleEffect *m_apPackBeams[MAX_PLAYERS_ARRAY_SAFE];
	bool m_bPlayerIsPackMember[MAX_PLAYERS_ARRAY_SAFE];

	CNetworkHandle( C_PasstimeBall, m_hBall );
	CNetworkArray( Vector, m_trackPoints, 16 );
	CNetworkVar( int, m_iNumSections );
	CNetworkVar( int, m_iCurrentSection );
	CNetworkVar( float, m_flMaxPassRange );
	CNetworkVar( int, m_iBallPower );
	CNetworkVar( float, m_flPackSpeed );
};

//-----------------------------------------------------------------------------
extern C_TFPasstimeLogic* g_pPasstimeLogic;

#endif // C_TF_PASSTIME_LOGIC_H  
