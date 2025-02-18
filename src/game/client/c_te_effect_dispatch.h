//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_TE_EFFECT_DISPATCH_H
#define C_TE_EFFECT_DISPATCH_H
#ifdef _WIN32
#pragma once
#endif


#include "effect_dispatch_data.h"


typedef void (*ClientEffectCallback)( const CEffectData &data );


class CClientEffectRegistration
{
public:
	CClientEffectRegistration( const char *pEffectName, ClientEffectCallback fn );

public:
	const char *m_pEffectName;
	ClientEffectCallback m_pFunction;
	CClientEffectRegistration *m_pNext;

	static CClientEffectRegistration *s_pHead;
};


//
// Use this macro to register a client effect callback. 
// If you do DECLARE_CLIENT_EFFECT( "MyEffectName", MyCallback ), then MyCallback will be 
// called when the server does DispatchEffect( "MyEffect", data )
//
#define DECLARE_CLIENT_EFFECT( effectName, callbackFunction ) \
	static CClientEffectRegistration ClientEffectReg_##callbackFunction( effectName, callbackFunction );

void DispatchEffectToCallback( const char *pEffectName, const CEffectData &m_EffectData );
void DispatchEffect( const char *pName, const CEffectData &data );

#endif // C_TE_EFFECT_DISPATCH_H
