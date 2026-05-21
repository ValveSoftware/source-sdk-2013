//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef BM_CLIENT_H
#define BM_CLIENT_H

class C_TFPlayer;
class CViewSetup;

#ifdef SOURCESDK
void BM_ClientApplyTopDownCamera( C_TFPlayer *pLocalPlayer, CViewSetup *pSetup );
void BM_ClientUpdateCameraMode( C_TFPlayer *pLocalPlayer );
#endif

#endif
