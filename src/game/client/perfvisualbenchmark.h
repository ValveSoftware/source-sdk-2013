//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#if !defined PERFVISUALBENCHMARK_H
#define PERFVISUALBENCHMARK_H

#define FPS_STABILIZE_TIME 1.5
#define FPS_STABILIZE_TIME_RELOAD_MATERIALS 10.0
#define FPS_MEASURE_TIME 2.0

#ifdef _WIN32
#pragma once
#endif

#include <igameevents.h>
#include <igamesystem.h>

class CPerfVisualBenchmark : public CBaseGameSystemPerFrame
{
	
public:
	CPerfVisualBenchmark();
	virtual ~CPerfVisualBenchmark();

public: // CBaseGameSystem overrides

	virtual char const *Name() { return "CPerfVisualBenchmark"; }

	virtual bool Init();
	virtual void PreRender( );

	void Start();
	void Stop();

private:
	void Print();
	struct RunInfo_t
	{
		const char *m_pVarName;
		const char *m_pOnVal;
		const char *m_pOffVal;
		const char *m_pDescription;
		float m_flStabilizeTime;
		float m_flFPS;
	};

private:
	CUtlVector<RunInfo_t> m_RunInfo;
	bool m_bIsOn;				//is this thing on?
	int m_iCurVar;				//what convar are we at?
	float m_flTimer;			//time since we started measuring the current convar
	float m_flStartMeasureTime;
	int m_nStartFrameCount;
	bool m_bSaveMouseEnable;	// remember this so that we can reset it after the benchmark
	bool m_bWaiting;
};

extern IGameSystem* PerfVisualBenchmark();

#endif // PERFVISUALBENCHMARK_H
