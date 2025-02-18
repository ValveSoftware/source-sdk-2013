//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Regenerate Zone.
//
//=============================================================================//
#ifndef FUNC_REGENERATE_ZONE_H
#define FUNC_REGENERATE_ZONE_H

#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "props.h"

//=============================================================================
//
// CTF Regenerate Zone class.
//
class CRegenerateZone : public CBaseTrigger
{
public:
	DECLARE_CLASS( CRegenerateZone, CBaseTrigger );

	CRegenerateZone();

	void	Spawn( void );
	void	Precache( void );
	void	Activate( void );
	void	Touch( CBaseEntity *pOther );
	virtual void	EndTouch( CBaseEntity *pOther );

	bool	IsDisabled( void );
	void	SetDisabled( bool bDisabled );
	void	Regenerate( CTFPlayer *pPlayer );

	// Input handlers
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );
	void	InputToggle( inputdata_t &inputdata );

private:
	bool					m_bDisabled;
	CHandle<CDynamicProp>	m_hAssociatedModel;
	string_t				m_iszAssociatedModel;

	DECLARE_DATADESC();
};

#endif // FUNC_REGENERATE_ZONE_H












