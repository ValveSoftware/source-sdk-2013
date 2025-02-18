//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Pumpkin Bomb
//
//=============================================================================
#ifndef TF_TARGET_DUMMY_H
#define TF_TARGET_DUMMY_H
#ifdef _WIN32
#pragma once
#endif

#include "entity_capture_flag.h"

DECLARE_AUTO_LIST( ITFTargetDummy );

class CTFTargetDummy : public CBaseAnimating, public ITFTargetDummy
{
	DECLARE_CLASS( CTFTargetDummy, CBaseAnimating );

public:

	static CTFTargetDummy* Create( const Vector& vPosition, const QAngle& qAngles, CTFPlayer *pOwner );

	CTFTargetDummy();
	~CTFTargetDummy() {}

	virtual void	Precache( void );
	
	virtual void	Spawn( void );
	virtual void	Event_Killed( const CTakeDamageInfo &info );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );

	void DestroyThink();
	void Destroy();
	void SpewGibs();

	virtual bool	ShouldCollide( int collisionGroup, int contentsMask ) const;

private:
	// Gibs.
	CUtlVector<breakmodel_t>	m_aGibs;

};

#endif	//TF_TARGET_DUMMY_H
