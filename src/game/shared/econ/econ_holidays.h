//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef ECON_HOLIDAYS_H
#define ECON_HOLIDAYS_H

#ifdef _WIN32
#pragma once
#endif

bool EconHolidays_IsHolidayActive( int iHolidayIndex, const class CRTime& timeCurrent );
int	EconHolidays_GetHolidayForString( const char* pszHolidayName );
const char *EconHolidays_GetActiveHolidayString();

#if defined(TF_CLIENT_DLL) || defined(TF_DLL) || defined(TF_GC_DLL)
RTime32 EconHolidays_TerribleHack_GetHalloweenEndData();
#endif // defined(TF_CLIENT_DLL) || defined(TF_DLL) || defined(TF_GC_DLL)

#endif // ECON_HOLIDAYS_H
