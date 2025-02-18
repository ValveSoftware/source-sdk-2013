//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef MPIVIS_H
#define MPIVIS_H
#ifdef _WIN32
#pragma once
#endif


void VVIS_SetupMPI( int &argc, char **&argv );


void RunMPIBasePortalVis();
void RunMPIPortalFlow();


#endif // MPIVIS_H
