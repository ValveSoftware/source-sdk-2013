//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Hold definitions for all client animation events
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#if !defined( CL_ANIMEVENT_H )
#define CL_ANIMEVENT_H
#ifdef _WIN32
#pragma once
#endif

//Animation event codes
#define CL_EVENT_MUZZLEFLASH0		5001	// Muzzleflash on attachment 0
#define CL_EVENT_MUZZLEFLASH1		5011	// Muzzleflash on attachment 1
#define CL_EVENT_MUZZLEFLASH2		5021	// Muzzleflash on attachment 2
#define CL_EVENT_MUZZLEFLASH3		5031	// Muzzleflash on attachment 3
#define CL_EVENT_SPARK0				5002	// Spark on attachment 0
#define CL_EVENT_NPC_MUZZLEFLASH0	5003	// Muzzleflash on attachment 0 for third person views
#define CL_EVENT_NPC_MUZZLEFLASH1	5013	// Muzzleflash on attachment 1 for third person views
#define CL_EVENT_NPC_MUZZLEFLASH2	5023	// Muzzleflash on attachment 2 for third person views
#define CL_EVENT_NPC_MUZZLEFLASH3	5033	// Muzzleflash on attachment 3 for third person views
#define CL_EVENT_SOUND				5004	// Emit a sound // NOTE THIS MUST MATCH THE DEFINE AT CBaseEntity::PrecacheModel on the server!!!!!
#define CL_EVENT_EJECTBRASS1		6001	// Eject a brass shell from attachment 1

#define CL_EVENT_DISPATCHEFFECT0	9001	// Hook into a DispatchEffect on attachment 0
#define CL_EVENT_DISPATCHEFFECT1	9011	// Hook into a DispatchEffect on attachment 1
#define CL_EVENT_DISPATCHEFFECT2	9021	// Hook into a DispatchEffect on attachment 2
#define CL_EVENT_DISPATCHEFFECT3	9031	// Hook into a DispatchEffect on attachment 3
#define CL_EVENT_DISPATCHEFFECT4	9041	// Hook into a DispatchEffect on attachment 4
#define CL_EVENT_DISPATCHEFFECT5	9051	// Hook into a DispatchEffect on attachment 5
#define CL_EVENT_DISPATCHEFFECT6	9061	// Hook into a DispatchEffect on attachment 6
#define CL_EVENT_DISPATCHEFFECT7	9071	// Hook into a DispatchEffect on attachment 7
#define CL_EVENT_DISPATCHEFFECT8	9081	// Hook into a DispatchEffect on attachment 8
#define CL_EVENT_DISPATCHEFFECT9	9091	// Hook into a DispatchEffect on attachment 9

// These two events are used by c_env_spritegroup.
// FIXME: Should this be local to c_env_spritegroup?
#define CL_EVENT_SPRITEGROUP_CREATE		6002
#define CL_EVENT_SPRITEGROUP_DESTROY	6003
#define CL_EVENT_FOOTSTEP_LEFT			6004
#define CL_EVENT_FOOTSTEP_RIGHT			6005
#define CL_EVENT_MFOOTSTEP_LEFT			6006 // Footstep sounds based on material underfoot.
#define CL_EVENT_MFOOTSTEP_RIGHT		6007
#define CL_EVENT_MFOOTSTEP_LEFT_LOUD	6008 // Loud material impact sounds from feet attachments
#define CL_EVENT_MFOOTSTEP_RIGHT_LOUD	6009


#endif // CL_ANIMEVENT_H