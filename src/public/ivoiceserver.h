//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This module defines the IVoiceServer interface, which is used by
//			game code to control which clients are listening to which other
//			clients' voice streams.
//
// $NoKeywords: $
//=============================================================================//

#ifndef IVOICESERVER_H
#define IVOICESERVER_H


#include "interface.h"


#define INTERFACEVERSION_VOICESERVER	"VoiceServer002"


abstract_class IVoiceServer
{
public:
	virtual			~IVoiceServer()	{}

	// Use these to setup who can hear whose voice.
	// Pass in client indices (which are their ent indices - 1).
	virtual bool	GetClientListening(int iReceiver, int iSender) = 0;
	virtual bool	SetClientListening(int iReceiver, int iSender, bool bListen) = 0;
	virtual bool	SetClientProximity(int iReceiver, int iSender, bool bUseProximity) = 0;
};


#endif

