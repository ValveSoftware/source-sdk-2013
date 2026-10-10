#ifndef TF_3DPING_H
#define TF_3DPING_H
#ifdef _WIN32
#pragma once
#endif

class CTFPlayer;
class CCommand;

void TF3DPing_OnPlayerPingCommand( CTFPlayer *pSender, const CCommand &args );

#endif // TF_3DPING_H
