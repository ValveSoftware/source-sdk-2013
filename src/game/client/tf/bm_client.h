//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef BM_CLIENT_H
#define BM_CLIENT_H

class C_TFPlayer;
class CViewSetup;
class CUserCmd;

#ifdef SOURCESDK
void BM_ClientApplyBomberCameraState( C_TFPlayer *pLocalPlayer );
void BM_ClientClearBomberCameraState( C_TFPlayer *pLocalPlayer );
void BM_ClientOnLevelShutdown( void );
bool BM_ClientShouldControlView( C_TFPlayer *pLocalPlayer );
void BM_ClientCreateMove( C_TFPlayer *pLocalPlayer, CUserCmd *pCmd );
void BM_ClientApplyBomberViewCmd( C_TFPlayer *pLocalPlayer, CUserCmd *pCmd );
void BM_ClientApplyTopDownCamera( C_TFPlayer *pLocalPlayer, CViewSetup *pSetup );
void BM_ClientCalcView( C_TFPlayer *pLocalPlayer, Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar );
bool BM_ClientPushWorldClip( void );
void BM_ClientPopWorldClip( void );
#endif

#endif
