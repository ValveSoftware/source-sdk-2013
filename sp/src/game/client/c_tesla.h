//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef C_TESLA_H
#define C_TESLA_H
#ifdef _WIN32
#pragma once
#endif


#include "c_baseentity.h"
#include "fx.h"
#include "utllinkedlist.h"


class C_Tesla : public C_BaseEntity
{
public:
	
	DECLARE_CLASS( C_Tesla, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_Tesla();

	virtual void ReceiveMessage( int classID, bf_read &msg );
	virtual void ClientThink();


public:

	CUtlLinkedList<CTeslaInfo,int> m_QueuedCommands;
	char m_SoundName[64];
	char m_iszSpriteName[256];
};


#endif // C_TESLA_H
