//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Provide a shared place for library fucntions to report progress % for display
//
//=============================================================================//

#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H
#ifdef _WIN32
#pragma once
#endif


PLATFORM_INTERFACE void ReportProgress(char const *job_name, int total_units_to_do, 
									   int n_units_completed);

typedef void (*ProgressReportHandler_t)( char const*, int, int );

// install your own handler. returns previous handler
PLATFORM_INTERFACE ProgressReportHandler_t InstallProgressReportHandler( ProgressReportHandler_t pfn);


#endif
