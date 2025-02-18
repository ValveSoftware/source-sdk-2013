//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef IHASGENERICMETER_H
#define IHASGENERICMETER_H
#ifdef _WIN32
#pragma once
#endif

// Derive from this interface if your entity can have objects placed on build points on it
class IHasGenericMeter
{
public:
	// Tell me how many build points you have
	virtual bool		ShouldUpdateMeter() const { return true; }
	virtual float		GetMeterMultiplier() const { return 1.f; }
	virtual void		OnResourceMeterFilled() {}
	virtual float		GetChargeInterval() const { return 100.f; }

#ifdef CLIENT_DLL
	virtual bool		ShouldDrawMeter() const { return true; }
#endif // CLIENT_DLL
};

#endif // IHASGENERICMETER_H
