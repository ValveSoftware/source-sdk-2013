//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef HL2_VEHICLE_RADAR_H
#define HL2_VEHICLE_RADAR_H

#define RADAR_MAX_CONTACTS			24
#define RADAR_CONTACT_TYPE_BITS		3	// Max 8 types of contacts (for networking)
#define RADAR_UPDATE_FREQUENCY		1.5f
#define RADAR_UPDATE_FREQUENCY_FAST	0.5f

enum // If we have more than 16 types of contacts, RADAR_CONTACT_TYPE_BITS
{
	RADAR_CONTACT_NONE = -1,
	RADAR_CONTACT_GENERIC = 0,
	RADAR_CONTACT_MAGNUSSEN_RDU,
	RADAR_CONTACT_DOG,
	RADAR_CONTACT_ALLY_INSTALLATION,
	RADAR_CONTACT_ENEMY,			// 'regular' sized enemy (Hunter)
	RADAR_CONTACT_LARGE_ENEMY,		// Large enemy (Strider)
};

#endif
