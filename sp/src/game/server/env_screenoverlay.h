//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity to control screen overlays on a player
//
//=============================================================================//
#ifndef ENVSCREENOVERLAY_H
#define ENVSCREENOVERLAY_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEnvScreenOverlay : public CPointEntity
{
	DECLARE_CLASS(CEnvScreenOverlay, CPointEntity);
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CEnvScreenOverlay();

	// Always transmit to clients
	virtual int UpdateTransmitState();
	virtual void Spawn(void);
	virtual void Precache(void);

	void	InputStartOverlay(inputdata_t &inputdata);
	void	InputStopOverlay(inputdata_t &inputdata);
	void	InputSwitchOverlay(inputdata_t &inputdata);

	void	SetActive(bool bActive) { m_bIsActive = bActive; }

protected:
	CNetworkArray(string_t, m_iszOverlayNames, MAX_SCREEN_OVERLAYS);
	CNetworkArray(float, m_flOverlayTimes, MAX_SCREEN_OVERLAYS);
	CNetworkVar(float, m_flStartTime);
	CNetworkVar(int, m_iDesiredOverlay);
	CNetworkVar(bool, m_bIsActive);
};

// ====================================================================================
//
//  Screen-space effects
//
// ====================================================================================

class CEnvScreenEffect : public CPointEntity
{
	DECLARE_CLASS(CEnvScreenEffect, CPointEntity);
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	// We always want to be sent to the client
	CEnvScreenEffect(void) { AddEFlags(EFL_FORCE_CHECK_TRANSMIT); }
	virtual int UpdateTransmitState(void) { return SetTransmitState(FL_EDICT_ALWAYS); }
	virtual void Spawn(void);
	virtual void Precache(void);

private:

	void InputStartEffect(inputdata_t &inputdata);
	void InputStopEffect(inputdata_t &inputdata);

	CNetworkVar(float, m_flDuration);
	CNetworkVar(int, m_nType);
};

#endif // ENVSCREENOVERLAY_H
