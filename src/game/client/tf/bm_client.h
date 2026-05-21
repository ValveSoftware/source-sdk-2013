//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef BM_CLIENT_H
#define BM_CLIENT_H

class C_TFPlayer;
class CViewSetup;
class CUserCmd;

#ifdef SOURCESDK
void BM_ClientCreateMove( C_TFPlayer *pLocalPlayer, CUserCmd *pCmd );
void BM_ClientApplyTopDownCamera( C_TFPlayer *pLocalPlayer, CViewSetup *pSetup );
void BM_ClientCalcView( C_TFPlayer *pLocalPlayer, Vector &eyeOrigin, QAngle &eyeAngles );
void BM_ClientUpdateCameraMode( C_TFPlayer *pLocalPlayer );
bool BM_ClientPushWorldClip( void );
void BM_ClientPopWorldClip( void );
#endif

#endif
