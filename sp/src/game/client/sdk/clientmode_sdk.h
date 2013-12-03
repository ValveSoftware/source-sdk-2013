//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SDK_CLIENTMODE_H
#define SDK_CLIENTMODE_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include "sdkviewport.h"

class ClientModeSDKNormal : public ClientModeShared 
{
DECLARE_CLASS( ClientModeSDKNormal, ClientModeShared );

private:

// IClientMode overrides.
public:

					ClientModeSDKNormal();
	virtual			~ClientModeSDKNormal();

	virtual void	InitViewport();

	virtual float	GetViewModelFOV( void );

	int				GetDeathMessageStartHeight( void );

	virtual void	PostRenderVGui();

	
private:
	
	//	void	UpdateSpectatorMode( void );

};


extern IClientMode *GetClientModeNormal();
extern ClientModeSDKNormal* GetClientModeSDKNormal();


#endif // SDK_CLIENTMODE_H
