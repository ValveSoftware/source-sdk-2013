//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MPIVRAD_H
#define MPIVRAD_H
#ifdef _WIN32
#pragma once
#endif


#define VMPI_VRAD_PACKET_ID						1
	// Sub packet IDs.
	#define VMPI_SUBPACKETID_VIS_LEAFS			0
	#define VMPI_SUBPACKETID_BUILDFACELIGHTS	1
	#define VMPI_SUBPACKETID_PLIGHTDATA_RESULTS	2


// Called first thing in the exe.
void		VRAD_SetupMPI( int &argc, char **&argv );

void		RunMPIBuildFacelights(void);
void		RunMPIBuildVisLeafs(void);
void		VMPI_DistributeLightData();

// This handles disconnections. They're usually not fatal for the master.
void		HandleMPIDisconnect( int procID );


#endif // MPIVRAD_H
