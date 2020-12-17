#pragma once

#include "GameEventListener.h"
#include "postprocess_shared.h"

// Spawn Flags
#define SF_POSTPROCESS_MASTER		0x0001

//=============================================================================
// Class Postprocess Controller:
//=============================================================================
class CPostProcessController : public CBaseEntity
{
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DECLARE_CLASS( CPostProcessController, CBaseEntity );

	CPostProcessController();
	virtual ~CPostProcessController();

	virtual int UpdateTransmitState();

	// Input handlers
	void InputSetFadeTime(inputdata_t &data);
	void InputSetLocalContrastStrength(inputdata_t &data);
	void InputSetLocalContrastEdgeStrength(inputdata_t &data);
	void InputSetVignetteStart(inputdata_t &data);
	void InputSetVignetteEnd(inputdata_t &data);
	void InputSetVignetteBlurStrength(inputdata_t &data);
	void InputSetFadeToBlackStrength(inputdata_t &data);
	void InputSetDepthBlurFocalDistance(inputdata_t &data);
	void InputSetDepthBlurStrength(inputdata_t &data);
	void InputSetScreenBlurStrength(inputdata_t &data);
	void InputSetFilmGrainStrength(inputdata_t &data);

	void InputTurnOn(inputdata_t &data);
	void InputTurnOff(inputdata_t &data);

	void Spawn();

	bool IsMaster() const { return HasSpawnFlags( SF_FOG_MASTER ); }

public:
	CNetworkArray( float, m_flPostProcessParameters, POST_PROCESS_PARAMETER_COUNT );

	CNetworkVar( bool, m_bMaster );
};

//=============================================================================
//
// Postprocess Controller System.
//
class CPostProcessSystem : public CAutoGameSystem, public CGameEventListener
{
public:

	// Creation/Init.
	CPostProcessSystem( char const *name ) : CAutoGameSystem( name ) 
	{
		m_hMasterController = nullptr;
	}

	~CPostProcessSystem()
	{
		m_hMasterController = nullptr;
	}

	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();
	virtual void FireGameEvent( IGameEvent *pEvent );
	CPostProcessController *GetMasterPostProcessController()			{ return m_hMasterController; }

private:

	void InitMasterController();
	CHandle< CPostProcessController > m_hMasterController;
};

CPostProcessSystem *PostProcessSystem();
