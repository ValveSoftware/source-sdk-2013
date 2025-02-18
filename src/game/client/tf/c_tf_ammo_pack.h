//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef C_TF_AMMO_PACK_H
#define C_TF_AMMO_PACK_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseanimating.h"
#include "engine/ivdebugoverlay.h"
#include "c_tf_player.h"
#include "engine/IEngineSound.h"
#include "soundenvelope.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_TFAmmoPack : public C_BaseAnimating, public ITargetIDProvidesHint
{
	DECLARE_CLASS( C_TFAmmoPack, C_BaseAnimating );

public:

	DECLARE_CLIENTCLASS();

	C_TFAmmoPack( void );
	~C_TFAmmoPack( void );

	virtual int		DrawModel( int flags );
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual int		GetWorldModelIndex( void );
	virtual void	ValidateModelIndex( void );
	virtual bool	Interpolate( float currentTime );

	// ITargetIDProvidesHint
public:
	virtual void	DisplayHintTo( C_BasePlayer *pPlayer );

private:

	Vector		m_vecInitialVelocity;
	short		m_nWorldModelIndex;
};

#endif // C_TF_AMMO_PACK_H
