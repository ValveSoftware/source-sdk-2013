//================ Copyright (c) 1996-2009 Valve Corporation. All Rights Reserved. =================
//
//
//
//==================================================================================================

#ifndef VMPI_LOGFILE_H
#define VMPI_LOGFILE_H
#ifdef _WIN32
#pragma once
#endif


// This checks the registry for HKLM\Software\Valve\VMPI\LogFile. If that exists,
// then it will log anything that comes through here to that file.
//
// VMPI will automatically do this for any apps that use VMPI_Init() (if that registry key exists).
void VMPI_WriteToLogFile( const char *pMsg, ... );

bool VMPI_IsLogFileEnabled();


#endif // VMPI_LOGFILE_H
