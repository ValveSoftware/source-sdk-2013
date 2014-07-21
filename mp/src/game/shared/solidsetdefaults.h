//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SOLIDSETDEFAULTS_H
#define SOLIDSETDEFAULTS_H
#ifdef _WIN32
#pragma once
#endif

// solid_t parsing
class CSolidSetDefaults : public IVPhysicsKeyHandler
{
public:
	virtual void ParseKeyValue( void *pData, const char *pKey, const char *pValue );
	virtual void SetDefaults( void *pData );

	unsigned int GetContentsMask() { return m_contentsMask; }

private:
	unsigned int m_contentsMask;
};

extern CSolidSetDefaults g_SolidSetup;

#endif // SOLIDSETDEFAULTS_H
