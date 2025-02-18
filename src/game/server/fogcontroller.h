//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FOGCONTROLLER_H
#define FOGCONTROLLER_H
#ifdef _WIN32
#pragma once
#endif

#include "playernet_vars.h"
#include "igamesystem.h"

// Spawn Flags
#define SF_FOG_MASTER		0x0001

//=============================================================================
//
// Class Fog Controller:
// Compares a set of integer inputs to the one main input
// Outputs true if they are all equivalant, false otherwise
//
class CFogController : public CBaseEntity
{
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DECLARE_CLASS( CFogController, CBaseEntity );

	CFogController();
	~CFogController();

	// Parse data from a map file
	virtual void Activate();
	virtual int UpdateTransmitState();

	// Input handlers
	void InputSetStartDist(inputdata_t &data);
	void InputSetEndDist(inputdata_t &data);
	void InputTurnOn(inputdata_t &data);
	void InputTurnOff(inputdata_t &data);
	void InputSetColor(inputdata_t &data);
	void InputSetColorSecondary(inputdata_t &data);
	void InputSetFarZ( inputdata_t &data );
	void InputSetAngles( inputdata_t &inputdata );
	void InputSetRadial( inputdata_t& inputdata );
	void InputSetMaxDensity( inputdata_t &inputdata );

	void InputSetColorLerpTo(inputdata_t &data);
	void InputSetColorSecondaryLerpTo(inputdata_t &data);
	void InputSetStartDistLerpTo(inputdata_t &data);
	void InputSetEndDistLerpTo(inputdata_t &data);

	void InputStartFogTransition(inputdata_t &data);

	int DrawDebugTextOverlays(void);

	void SetLerpValues( void );
	void Spawn( void );

	bool IsMaster( void )					{ return HasSpawnFlags( SF_FOG_MASTER ); }

public:

	CNetworkVarEmbedded( fogparams_t, m_fog );
	bool					m_bUseAngles;
	int						m_iChangedVariables;
};

//=============================================================================
//
// Fog Controller System.
//
class CFogSystem : public CAutoGameSystem
{
public:

	// Creation/Init.
	CFogSystem( char const *name ) : CAutoGameSystem( name ) 
	{
		m_pMasterController = NULL;
	}

	~CFogSystem()
	{
		m_pMasterController = NULL;
	}

	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();
	CFogController *GetMasterFogController( void )			{ return m_pMasterController; }

private:

	CFogController	*m_pMasterController;
};

CFogSystem *FogSystem( void );

#endif // FOGCONTROLLER_H
