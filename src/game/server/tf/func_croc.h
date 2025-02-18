//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Place where crocs live.
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_FUNC_CROC_H
#define TF_FUNC_CROC_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_player.h"
#include "ndebugoverlay.h"
#include "triggers.h"

class CEntityCroc : public CBaseAnimating
{
	DECLARE_CLASS( CEntityCroc, CBaseAnimating );
public:

	CEntityCroc();
	void InitCroc( CBaseEntity *pOther, const char *pszModel );
	virtual void Think( void ) OVERRIDE;

private:
	void CrocAttack( void );
	CHandle< CTFPlayer > m_hTarget;
};

class CFuncCroc : public CBaseTrigger
{
	DECLARE_CLASS( CFuncCroc, CBaseTrigger );
public:
	CFuncCroc();

	DECLARE_DATADESC();

	virtual void Spawn( void ) OVERRIDE;
	virtual void Precache( void ) OVERRIDE;
	virtual int UpdateTransmitState( void ) OVERRIDE;
	virtual int ShouldTransmit( const CCheckTransmitInfo *pInfo ) OVERRIDE;
	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const OVERRIDE;

	virtual void StartTouch( CBaseEntity *pOther ) OVERRIDE;

	void FireOutputs( CTFPlayer *pTFPlayer );

	const char *GetCrocModel( void );

private:

	string_t m_iszModel;

	COutputEvent m_OnEat;
	COutputEvent m_OnEatRed;
	COutputEvent m_OnEatBlue;
};

#endif // TF_FUNC_CROC_H
