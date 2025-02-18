//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef C_FUNC_CAPTURE_ZONE_H
#define C_FUNC_CAPTURE_ZONE_H
#ifdef _WIN32
#pragma once
#endif

class C_CaptureZone;

DECLARE_AUTO_LIST( ICaptureZoneAutoList );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_CaptureZone : public C_BaseEntity, public ICaptureZoneAutoList
{
	DECLARE_CLASS( C_CaptureZone, C_BaseEntity );

public:
	DECLARE_CLIENTCLASS();

	bool IsDisabled( void ){ return m_bDisabled; }

private:
	bool m_bDisabled;
};

#endif // C_FUNC_CAPTURE_ZONE_H
