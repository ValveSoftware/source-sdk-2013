
#ifndef WORLDLIGHT_H
#define WORLDLIGHT_H
#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h" // CAutoGameSystem

class Vector;
struct dworldlight_t;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CWorldLights : public CAutoGameSystem
{
public:
	CWorldLights();
	~CWorldLights() { Clear(); }

	bool GetBrightestLightSource( const Vector &vecPosition, Vector &vecLightPos, Vector &vecLightBrightness );

	// CAutoGameSystem overrides
	bool Init() OVERRIDE;
	void LevelInitPreEntity() OVERRIDE;
	void LevelShutdownPostEntity() OVERRIDE { Clear(); }

private:
	void Clear();

private:
	int m_nWorldLights;
	dworldlight_t *m_pWorldLights;
};

// Singleton accessor
extern CWorldLights *g_pWorldLights;

#endif // WORLDLIGHT_H
