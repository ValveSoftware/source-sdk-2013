//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef IMPORTINTOVCD_H
#define IMPORTINTOVCD_H
#ifdef _WIN32
#pragma once
#endif

class CChoreoScene;


//-----------------------------------------------------------------------------
// Compression info
//-----------------------------------------------------------------------------
struct ImportVCDInfo_t
{
	float m_flSimplificationThreshhold;
	int m_nInterpolationType;
	bool m_bIgnorePhonemes;
};

//-----------------------------------------------------------------------------
// Main entry point for importing a .fac file into a .vcd file
//-----------------------------------------------------------------------------
bool ImportLogsIntoVCD( const char *pFacFullPath, CChoreoScene *pChoreoScene, const ImportVCDInfo_t& info );
bool ImportLogsIntoVCD( const char *pFacFullPath, const char *pVCDInPath, const char *pVCDOutPath, const ImportVCDInfo_t& info );


#endif // IMPORTINTOVCD_H
