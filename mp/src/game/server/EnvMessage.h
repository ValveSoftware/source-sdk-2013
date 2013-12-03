//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ENVMESSAGE_H
#define ENVMESSAGE_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "entityoutput.h"


#define SF_MESSAGE_ONCE			0x0001		// Fade in, not out
#define SF_MESSAGE_ALL			0x0002		// Send to all clients

class CMessage : public CPointEntity
{
public:
	DECLARE_CLASS( CMessage, CPointEntity );

	void	Spawn( void );
	void	Precache( void );

	inline void SetMessage( string_t iszMessage ) { m_iszMessage = iszMessage; }

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

private:

	void InputShowMessage( inputdata_t &inputdata );

	string_t m_iszMessage;		// Message to display.
	float m_MessageVolume;
	int m_MessageAttenuation;
	float m_Radius;

	DECLARE_DATADESC();

	string_t m_sNoise;
	COutputEvent m_OnShowMessage;
};

#endif // ENVMESSAGE_H
