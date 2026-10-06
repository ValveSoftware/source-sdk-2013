//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#if !defined( CLIENTMODE_HLNORMAL_H )
#define CLIENTMODE_HLNORMAL_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui/Cursor.h>

class CHudViewport;
class CEternalCityHud;
class C_BasePlayer;
class CViewSetup;

namespace vgui
{
	typedef unsigned long HScheme;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class ClientModeHLNormal : public ClientModeShared
{
public:
	DECLARE_CLASS( ClientModeHLNormal, ClientModeShared );

	ClientModeHLNormal();
	~ClientModeHLNormal();

	virtual void	Init();
	virtual void	LevelShutdown( void );
	virtual bool	ShouldDrawCrosshair( void );
	virtual bool	ShouldDrawLocalPlayer( C_BasePlayer *pPlayer );
	virtual bool	ShouldDrawViewModel();
	virtual void	OverrideView( CViewSetup *pSetup );
	virtual bool	CreateMove( float flInputSampleTime, CUserCmd *cmd );
	virtual int		KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

private:
	bool			ShouldUseEternalCityCamera() const;
	void			ReleaseEternalCityCamera();
	CEternalCityHud	*m_pEternalCityHud;
	bool			m_bOwnsThirdPerson;
};

extern IClientMode *GetClientModeNormal();
extern vgui::HScheme g_hVGuiCombineScheme;

#endif // CLIENTMODE_HLNORMAL_H
