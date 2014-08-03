//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "perfvisualbenchmark.h"
#include "KeyValues.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define VAR		0
#define ON		1
#define OFF		2
#define DETAILS	3

static CPerfVisualBenchmark s_PerfVisualBenchmark; // singleton

IGameSystem* PerfVisualBenchmark() { return &s_PerfVisualBenchmark; }

#ifndef _XBOX
extern ConVar cl_mouseenable;
#endif

void usrCmd_Start()
{
	s_PerfVisualBenchmark.Start();
}

void usrCmd_Abort()
{
	s_PerfVisualBenchmark.Stop();
}

static ConCommand perfvisualbenchmark("perfvisualbenchmark", usrCmd_Start);
static ConCommand perfvisualbenchmark_abort("perfvisualbenchmark_abort", usrCmd_Abort);


CPerfVisualBenchmark::CPerfVisualBenchmark()
{
}

CPerfVisualBenchmark::~CPerfVisualBenchmark()
{
}

bool CPerfVisualBenchmark::Init()
{
	RunInfo_t runInfo;
	
	runInfo.m_pVarName = "";
	runInfo.m_pOnVal = "";
	runInfo.m_pOffVal = "";
	runInfo.m_pDescription = "Default";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "r_drawdetailprops";
	runInfo.m_pOnVal = "0";
	runInfo.m_pOffVal = "1";
	runInfo.m_pDescription = "detail props";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "r_drawworld";
	runInfo.m_pOnVal = "0";
	runInfo.m_pOffVal = "1";
	runInfo.m_pDescription = "world geometry";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "r_drawentities";
	runInfo.m_pOnVal = "0";
	runInfo.m_pOffVal = "1";
	runInfo.m_pDescription = "entities";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "r_3dsky";
	runInfo.m_pOnVal = "0";
	runInfo.m_pOffVal = "1";
	runInfo.m_pDescription = "3D skybox";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "r_drawdecals";
	runInfo.m_pOnVal = "0";
	runInfo.m_pOffVal = "1";
	runInfo.m_pDescription = "decals";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "mat_stub";
	runInfo.m_pOnVal = "1";
	runInfo.m_pOffVal = "0";
	runInfo.m_pDescription = "material system and below";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "mat_viewportscale";
	runInfo.m_pOnVal = ".2";
	runInfo.m_pOffVal = "1.0";
	runInfo.m_pDescription = "fillrate";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "r_drawstaticprops";
	runInfo.m_pOnVal = "0";
	runInfo.m_pOffVal = "1";
	runInfo.m_pDescription = "fillrate";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "r_drawbrushmodels";
	runInfo.m_pOnVal = "0";
	runInfo.m_pOffVal = "1";
	runInfo.m_pDescription = "brush models";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "r_renderoverlayfragment";
	runInfo.m_pOnVal = "0";
	runInfo.m_pOffVal = "1";
	runInfo.m_pDescription = "overlays";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "r_drawdisp";
	runInfo.m_pOnVal = "0";
	runInfo.m_pOffVal = "1";
	runInfo.m_pDescription = "displacements";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "r_drawviewmodel";
	runInfo.m_pOnVal = "0";
	runInfo.m_pOffVal = "1";
	runInfo.m_pDescription = "viewmodel";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "cl_drawhud";
	runInfo.m_pOnVal = "0";
	runInfo.m_pOffVal = "1";
	runInfo.m_pDescription = "hud";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "r_drawparticles";
	runInfo.m_pOnVal = "0";
	runInfo.m_pOffVal = "1";
	runInfo.m_pDescription = "particles";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "r_drawsprites";
	runInfo.m_pOnVal = "0";
	runInfo.m_pOffVal = "1";
	runInfo.m_pDescription = "sprites";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "mat_bumpmap";
	runInfo.m_pOnVal = "0";
	runInfo.m_pOffVal = "1";
	runInfo.m_pDescription = "bump mapping";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME_RELOAD_MATERIALS;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "mat_specular";
	runInfo.m_pOnVal = "0";
	runInfo.m_pOffVal = "1";
	runInfo.m_pDescription = "specularity";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME_RELOAD_MATERIALS;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "mat_drawwater";
	runInfo.m_pOnVal = "0";
	runInfo.m_pOffVal = "1";
	runInfo.m_pDescription = "water";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "r_dynamic";
	runInfo.m_pOnVal = "0";
	runInfo.m_pOffVal = "1";
	runInfo.m_pDescription = "dynamic lighting";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "r_shadows";
	runInfo.m_pOnVal = "0";
	runInfo.m_pOffVal = "1";
	runInfo.m_pDescription = "shadows";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME;
	m_RunInfo.AddToTail( runInfo );

	runInfo.m_pVarName = "r_drawropes";
	runInfo.m_pOnVal = "0";
	runInfo.m_pOffVal = "1";
	runInfo.m_pDescription = "ropes";
	runInfo.m_flStabilizeTime = FPS_STABILIZE_TIME;
	m_RunInfo.AddToTail( runInfo );

	m_bIsOn = false;					//is this thing on?
	return true;
}

void CPerfVisualBenchmark::Start()
{
#ifndef _XBOX
	m_bSaveMouseEnable = cl_mouseenable.GetBool();
	cl_mouseenable.SetValue( 0 );
#endif
	m_iCurVar = 0;
	m_flTimer = gpGlobals->realtime + FPS_STABILIZE_TIME;
	m_bWaiting = true;
	m_bIsOn = true;									// showtime!
	engine->ClientCmd_Unrestricted("cancelselect");				// exit menu and console
//	engine->ClientCmd_Unrestricted("wait");				
//	engine->ClientCmd_Unrestricted("setpause");					// pause the mofo
	engine->ClientCmd_Unrestricted("host_timescale 0.0001");					// pause the mofo
	
}

void CPerfVisualBenchmark::Stop()
{
#ifndef _XBOX
	cl_mouseenable.SetValue( m_bSaveMouseEnable );
#endif
	m_bIsOn = false;
	Print();
	engine->ClientCmd_Unrestricted("host_timescale 0");					// pause the mofo
//	engine->ClientCmd_Unrestricted("unpause");				// unpause the mofo
//	engine->ClientCmd_Unrestricted("wait");				
	engine->ClientCmd_Unrestricted("toggleconsole");
}

void CPerfVisualBenchmark::PreRender( )
{
	if (!m_bIsOn)
		return;

	// Wait for the timer
	if ( m_flTimer > gpGlobals->realtime )
		return;

	if ( m_bWaiting )
	{
		m_flTimer = gpGlobals->realtime + FPS_MEASURE_TIME;
		m_flStartMeasureTime = gpGlobals->realtime;
		m_nStartFrameCount = gpGlobals->framecount;
		m_bWaiting = false;
		return;
	}

	// Ok, we were measuring, lets calculate the results
	float flDenom = gpGlobals->realtime - m_flStartMeasureTime;
	if (flDenom == 0)
	{
		flDenom = 1.0f;
	}

	// note the current avged fps;
	float flAveFPS = (gpGlobals->framecount - m_nStartFrameCount) / flDenom;
	m_RunInfo[m_iCurVar].m_flFPS = flAveFPS;

	m_flTimer = gpGlobals->realtime + FPS_STABILIZE_TIME;
	m_bWaiting = true;

	char combuffer[255];

	// Turn off any previous value
	if ( m_RunInfo[m_iCurVar].m_pVarName )
	{
		Q_snprintf(combuffer, sizeof(combuffer), "%s %s\n", m_RunInfo[m_iCurVar].m_pVarName, m_RunInfo[m_iCurVar].m_pOffVal );	//turn off current var
		engine->ClientCmd_Unrestricted(combuffer);
	}

	// next var
	m_iCurVar++;				
	if (m_iCurVar == m_RunInfo.Count())
	{
		Stop();
		return;
	}

	Q_snprintf(combuffer, sizeof(combuffer), "%s %s\n",m_RunInfo[m_iCurVar].m_pVarName, m_RunInfo[m_iCurVar].m_pOnVal);  //turn on next var
	engine->ClientCmd_Unrestricted(combuffer);
}


void CPerfVisualBenchmark::Print()				//  sort and print into console
{
	for (int i = 0; i<m_RunInfo.Count(); i++)
	{
		int curMax = 0;
		for (int j = 0; j<m_RunInfo.Count(); j++)
		{
			if (m_RunInfo[j].m_flFPS > m_RunInfo[curMax].m_flFPS)
			{
				curMax = j;
			}
		}
		Msg("%.0f fps - %s\n",m_RunInfo[curMax].m_flFPS, m_RunInfo[curMax].m_pDescription);
		m_RunInfo[curMax].m_flFPS=-1;
	}
}

