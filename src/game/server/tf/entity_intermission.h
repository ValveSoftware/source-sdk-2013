//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Triggers an intermission
//
//=============================================================================//
#ifndef ENTITY_INTERMISSION_H
#define ENTITY_INTERMISSION_H

#ifdef _WIN32
#pragma once
#endif

//=============================================================================
//
// CTF Intermission class.
//

class CTFIntermission : public CLogicalEntity
{
public:
	DECLARE_CLASS( CTFIntermission, CLogicalEntity );

	void InputActivate( inputdata_t &inputdata );

	DECLARE_DATADESC();
};

#endif // ENTITY_INTERMISSION_H

