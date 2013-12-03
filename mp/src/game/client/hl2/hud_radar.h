//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HUD_RADAR_H
#define HUD_RADAR_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <vgui_controls/Label.h>
#include "hl2_vehicle_radar.h"
#include "c_vguiscreen.h"

class CRadarContact
{
public:
	Vector	m_vecOrigin;
	int		m_iType;
	float	m_flTimeToRemove;
};

class CHudRadar : public CVGuiScreenPanel
{
public:
	DECLARE_CLASS_SIMPLE( CHudRadar, CVGuiScreenPanel );

	
	CHudRadar( vgui::Panel *parent, const char *panelName );
	~CHudRadar();

	virtual void Paint();
	void VidInit(void);
	virtual bool Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData );
	virtual void SetVisible(bool state);

	void MsgFunc_UpdateRadar(bf_read &msg );
	void SetVehicle( C_BaseEntity *pVehicle )	{ m_pVehicle = pVehicle; }

	void AddRadarContact( const Vector &vecOrigin, int iType, float flTimeToLive );
	int FindRadarContact( const Vector &vecOrigin );
	void MaintainRadarContacts();


	void ClearAllRadarContacts()	{ m_iNumRadarContacts = 0; }

public:
	bool			m_bUseFastUpdate;
	int				m_ghostAlpha;			// How intense the alpha channel is for CRT ghosts
	float			m_flTimeStopGhosting;	
	float			m_flTimeStartGhosting;

private:

	bool WorldToRadar( const Vector location, const Vector origin, const QAngle angles, float &x, float &y, float &z_delta, float &scale );

	void DrawPositionOnRadar( Vector vecPos, C_BasePlayer *pLocalPlayer, int type, int flags, int r, int g, int b, int a );
	void DrawIconOnRadar( Vector vecPos, C_BasePlayer *pLocalPlayer, int type, int flags, int r, int g, int b, int a );

	void FillRect( int x, int y, int w, int h );
	void DrawRadarDot( int x, int y, float z_diff, int iBaseDotSize, int flags, int r, int g, int b, int a );

	CRadarContact	m_radarContacts[RADAR_MAX_CONTACTS];
	int				m_iNumRadarContacts;
	C_BaseEntity	*m_pVehicle;
	int				m_iImageID;
	int				m_textureID_IconLambda;
	int				m_textureID_IconBuster;
	int				m_textureID_IconStrider;
	int				m_textureID_IconDog;
	int				m_textureID_IconBase;
};

extern CHudRadar *GetHudRadar();
#endif // HUD_RADAR_H
