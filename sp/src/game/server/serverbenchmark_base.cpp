//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "serverbenchmark_base.h"
#include "props.h"
#include "filesystem.h"
#include "tier0/icommandline.h"


// Server benchmark. Only works on specified maps.
// Lasts for N ticks.
// Enable sv_stressbots.
// Create 20 players and move them around and have them shoot.
// At the end, report the # seconds it took to complete the test.
// Don't start measuring for the first N ticks to account for HD load.

static ConVar sv_benchmark_numticks( "sv_benchmark_numticks", "3300", 0, "If > 0, then it only runs the benchmark for this # of ticks." );
static ConVar sv_benchmark_autovprofrecord( "sv_benchmark_autovprofrecord", "0", 0, "If running a benchmark and this is set, it will record a vprof file over the duration of the benchmark with filename benchmark.vprof." );

static float s_flBenchmarkStartWaitSeconds = 3;	// Wait this many seconds after level load before starting the benchmark.

static int s_nBenchmarkBotsToCreate = 22;		// Create this many bots.
static int s_nBenchmarkBotCreateInterval = 50;	// Create a bot every N ticks.

static int s_nBenchmarkPhysicsObjects = 100;	// Create this many physics objects.


static double Benchmark_ValidTime()
{
	bool bOld = Plat_IsInBenchmarkMode();
	Plat_SetBenchmarkMode( false );
	double flRet = Plat_FloatTime();
	Plat_SetBenchmarkMode( bOld );
	
	return flRet;
}


// ---------------------------------------------------------------------------------------------- //
// CServerBenchmark implementation.
// ---------------------------------------------------------------------------------------------- //
class CServerBenchmark : public IServerBenchmark
{
public:
	CServerBenchmark()
	{
		m_BenchmarkState = BENCHMARKSTATE_NOT_RUNNING;
		
		// The benchmark should always have the same seed and do exactly the same thing on the same ticks.
		m_RandomStream.SetSeed( 1111 ); 
	}

	virtual bool StartBenchmark()
	{
		bool bBenchmark = (CommandLine()->FindParm( "-sv_benchmark" ) != 0);

		return InternalStartBenchmark( bBenchmark, s_flBenchmarkStartWaitSeconds );
	}

	// nBenchmarkMode: 0 = no benchmark
	//                 1 = benchmark
	//                 2 = exit out afterwards and write sv_benchmark.txt
	bool InternalStartBenchmark( int nBenchmarkMode, float flCountdown )
	{
		bool bWasRunningBenchmark = (m_BenchmarkState != BENCHMARKSTATE_NOT_RUNNING);

		if ( nBenchmarkMode == 0 )
		{
			// Tear down the previous benchmark environment if necessary.
			if ( bWasRunningBenchmark )
				EndBenchmark();
			return false;
		}

		m_nBenchmarkMode = nBenchmarkMode;

		if ( !CServerBenchmarkHook::s_pBenchmarkHook )
			Error( "This game doesn't support server benchmarks (no CServerBenchmarkHook found)." );

		m_BenchmarkState = BENCHMARKSTATE_START_WAIT;
		m_flBenchmarkStartTime = Plat_FloatTime();
		m_flBenchmarkStartWaitTime = flCountdown;

		m_nBotsCreated = 0;
		m_nStartWaitCounter = -1;

		// Setup the benchmark environment.
		engine->SetDedicatedServerBenchmarkMode( true );	// Run 1 tick per frame and ignore all timing stuff.

		// Tell the game-specific hook that we're starting.
		CServerBenchmarkHook::s_pBenchmarkHook->StartBenchmark();
		CServerBenchmarkHook::s_pBenchmarkHook->GetPhysicsModelNames( m_PhysicsModelNames );

		return true;
	}

	virtual void UpdateBenchmark()
	{
		// No benchmark running?	
		if ( m_BenchmarkState == BENCHMARKSTATE_NOT_RUNNING )
			return;

		// Wait a certain number of ticks to start the benchmark.
		if ( m_BenchmarkState == BENCHMARKSTATE_START_WAIT )
		{
			if ( (Plat_FloatTime() - m_flBenchmarkStartTime) < m_flBenchmarkStartWaitTime )
			{
				UpdateStartWaitCounter();
				return;
			}
			else
			{
				// Ok, now we're officially starting it.
				Msg( "Starting benchmark!\n" );
				m_flLastBenchmarkCounterUpdate = m_flBenchmarkStartTime = Plat_FloatTime();
				m_fl_ValidTime_BenchmarkStartTime = Benchmark_ValidTime();
				m_nBenchmarkStartTick = gpGlobals->tickcount;
				m_nLastPhysicsObjectTick = m_nLastPhysicsForceTick = 0;
				m_BenchmarkState = BENCHMARKSTATE_RUNNING;

				StartVProfRecord();

				RandomSeed( 0 );
				m_RandomStream.SetSeed( 0 );
			}
		}

		int nTicksRunSoFar = gpGlobals->tickcount - m_nBenchmarkStartTick;
		UpdateBenchmarkCounter();
	
		// Are we finished with the benchmark?
		if ( nTicksRunSoFar >= sv_benchmark_numticks.GetInt() )
		{
			EndVProfRecord();
			OutputResults();
			EndBenchmark();
			return;
		}

		// Ok, update whatever we're doing in the benchmark.
		UpdatePlayerCreation();
		UpdateVPhysicsObjects();
		CServerBenchmarkHook::s_pBenchmarkHook->UpdateBenchmark();
	}

	void StartVProfRecord()
	{
		if ( sv_benchmark_autovprofrecord.GetInt() )
		{
			engine->ServerCommand( "vprof_record_start benchmark\n" );
			engine->ServerExecute();
		}
	}

	void EndVProfRecord()
	{
		if ( sv_benchmark_autovprofrecord.GetInt() )
		{
			engine->ServerCommand( "vprof_record_stop\n" );
			engine->ServerExecute();
		}
	}

	virtual void EndBenchmark( void )
	{
		// Write out the results if we're running the build scripts.
		float flRunTime = Benchmark_ValidTime() - m_fl_ValidTime_BenchmarkStartTime;
		if ( m_nBenchmarkMode == 2 )
		{
			FileHandle_t fh = filesystem->Open( "sv_benchmark_results.txt", "wt", "DEFAULT_WRITE_PATH" );
			
			// If this file doesn't get written out, then the build script will generate an email that there's a problem somewhere.
			if ( fh )
			{
				filesystem->FPrintf( fh, "sv_benchmark := %.2f\n", flRunTime );
			}
			filesystem->Close( fh );

			// Quit out.
			engine->ServerCommand( "quit\n" );
		}
		
		m_BenchmarkState = BENCHMARKSTATE_NOT_RUNNING;
		engine->SetDedicatedServerBenchmarkMode( false );
	}

	virtual bool IsLocalBenchmarkPlayer( CBasePlayer *pPlayer )
	{
		if ( m_BenchmarkState != BENCHMARKSTATE_NOT_RUNNING )
		{
			if( !engine->IsDedicatedServer() && pPlayer->entindex() == 1 )
				return true;
		}
		
		return false;
	}

	void UpdateVPhysicsObjects()
	{
		int nPhysicsObjectInterval = sv_benchmark_numticks.GetInt() / s_nBenchmarkPhysicsObjects;

		int nNextSpawnTick = m_nLastPhysicsObjectTick + nPhysicsObjectInterval;
		if ( GetTickOffset() >= nNextSpawnTick )
		{
			m_nLastPhysicsObjectTick = nNextSpawnTick;
			
			if ( m_PhysicsObjects.Count() < s_nBenchmarkPhysicsObjects )
			{
				// Find a bot to spawn it from.
				CUtlVector<CBasePlayer*> curPlayers;
				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
					if ( pPlayer && (pPlayer->GetFlags() & FL_FAKECLIENT) )
					{
						curPlayers.AddToTail( pPlayer );
					}
				}

				if ( curPlayers.Count() > 0 && m_PhysicsModelNames.Count() > 0 )
				{
					int iModelName = this->RandomInt( 0, m_PhysicsModelNames.Count() - 1 );
					const char *pModelName = m_PhysicsModelNames[iModelName];

					int iPlayer = this->RandomInt( 0, curPlayers.Count() - 1 );
						
					Vector vSpawnPos = curPlayers[iPlayer]->EyePosition() + Vector( 0, 0, 50 );
					
					// We'll try 15 locations around the player to spawn this thing.
					for ( int i=0; i < 15; i++ )
					{
						Vector vOffset( this->RandomFloat( -2000, 2000 ), this->RandomFloat( -2000, 2000 ), 0 );
						CPhysicsProp *pProp = CreatePhysicsProp( pModelName, vSpawnPos, vSpawnPos+vOffset, curPlayers[iPlayer], false, "prop_physics_multiplayer" );
						if ( pProp )
						{
							m_PhysicsObjects.AddToTail( pProp );
							pProp->SetAbsVelocity( Vector( this->RandomFloat(-500,500), this->RandomFloat(-500,500), this->RandomFloat(-500,500) ) );
							break;
						}
					}
				}
			}
		}

		// Give them all a boost periodically.
		int nPhysicsForceInterval = sv_benchmark_numticks.GetInt() / 20;

		int nNextForceTick = m_nLastPhysicsForceTick + nPhysicsForceInterval;
		if ( GetTickOffset() >= nNextForceTick )
		{
			m_nLastPhysicsForceTick = nNextForceTick;

			for ( int i=0; i < m_PhysicsObjects.Count(); i++ )
			{
				CBaseEntity *pEnt = m_PhysicsObjects[i];
				if ( pEnt )
				{
					IPhysicsObject *pPhysicsObject = pEnt->VPhysicsGetObject();
					if ( pPhysicsObject )
					{
						float flAngImpulse = 300000;
						float flForce = 500000;
						AngularImpulse vAngularImpulse( this->RandomFloat(-flAngImpulse,flAngImpulse), this->RandomFloat(-flAngImpulse,flAngImpulse), this->RandomFloat(flAngImpulse,flAngImpulse) );
						pPhysicsObject->ApplyForceCenter( Vector( this->RandomFloat(-flForce,flForce), this->RandomFloat(-flForce,flForce), this->RandomFloat(0,flForce) ) );
					}
				}				
			}
		}
	}

	void UpdateStartWaitCounter()
	{
		int nSecondsLeft = (int)ceil( m_flBenchmarkStartWaitTime - (Plat_FloatTime() - m_flBenchmarkStartTime) );
		if ( m_nStartWaitCounter != nSecondsLeft )
		{
			Msg( "Starting benchmark in %d seconds...\n", nSecondsLeft );
			m_nStartWaitCounter = nSecondsLeft;
		}
	}

	void UpdateBenchmarkCounter()
	{
		float flCurTime = Plat_FloatTime();
		if ( (flCurTime - m_flLastBenchmarkCounterUpdate) > 3.0f )
		{
			m_flLastBenchmarkCounterUpdate = flCurTime;
			Msg( "Benchmark: %d%% complete.\n", ((gpGlobals->tickcount - m_nBenchmarkStartTick) * 100) / sv_benchmark_numticks.GetInt() );
		}
	}

	virtual bool IsBenchmarkRunning()
	{
		return (m_BenchmarkState == BENCHMARKSTATE_RUNNING);
	}

	virtual int GetTickOffset()
	{
		if ( m_BenchmarkState == BENCHMARKSTATE_RUNNING )
		{
			Assert( gpGlobals->tickcount >= m_nBenchmarkStartTick );
			return gpGlobals->tickcount - m_nBenchmarkStartTick;
		}
		else
		{
			return gpGlobals->tickcount;
		}
	}


	void UpdatePlayerCreation()
	{
		if ( m_nBotsCreated >= s_nBenchmarkBotsToCreate )
			return;

		// Spawn the player.
		int nTicksRunSoFar = gpGlobals->tickcount - m_nBenchmarkStartTick;

		if ( (nTicksRunSoFar % s_nBenchmarkBotCreateInterval) == 0 )
		{
			CServerBenchmarkHook::s_pBenchmarkHook->CreateBot();
			++m_nBotsCreated;
		}
	}

	void OutputResults()
	{
		float flRunTime = Benchmark_ValidTime() - m_fl_ValidTime_BenchmarkStartTime;

		Warning( "------------------ SERVER BENCHMARK RESULTS ------------------\n" );
		Warning( "Total time          : %.2f seconds\n", flRunTime );
		Warning( "Num ticks simulated : %d\n", sv_benchmark_numticks.GetInt() );
		Warning( "Ticks per second    : %.2f\n", sv_benchmark_numticks.GetInt() / flRunTime );
		Warning( "Benchmark CRC       : %d\n", CalculateBenchmarkCRC() );
		Warning( "--------------------------------------------------------------\n" );
	}

	int CalculateBenchmarkCRC()
	{
		int crc = 0;

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
			if ( pPlayer && (pPlayer->GetFlags() & FL_FAKECLIENT) )
			{
				crc += pPlayer->GetTeamNumber();
				crc += (int)pPlayer->GetAbsOrigin().x; 
				crc += (int)pPlayer->GetAbsOrigin().y; 
			}
		}

		return crc;
	}

	virtual int RandomInt( int nMin, int nMax )
	{
		return m_RandomStream.RandomInt( nMin, nMax );
	}

	virtual float RandomFloat( float nMin, float nMax )
	{
		return m_RandomStream.RandomInt( nMin, nMax );
	}


private:
	
	enum EBenchmarkState
	{
		BENCHMARKSTATE_NOT_RUNNING,
		BENCHMARKSTATE_START_WAIT,
		BENCHMARKSTATE_RUNNING
	};
	EBenchmarkState m_BenchmarkState;

	float m_fl_ValidTime_BenchmarkStartTime;
	
	float m_flBenchmarkStartTime;
	float m_flLastBenchmarkCounterUpdate;
	float m_flBenchmarkStartWaitTime;

	int m_nBenchmarkStartTick;
	int m_nStartWaitCounter;
	int m_nLastPhysicsObjectTick;
	int m_nLastPhysicsForceTick;

	int m_nBotsCreated;
	CUtlVector< EHANDLE > m_PhysicsObjects;

	CUtlVector<char*> m_PhysicsModelNames;
	int m_nBenchmarkMode;

	CUniformRandomStream m_RandomStream;
};

static CServerBenchmark g_ServerBenchmark;
IServerBenchmark *g_pServerBenchmark = &g_ServerBenchmark;

CON_COMMAND( sv_benchmark_force_start, "Force start the benchmark. This is only for debugging. It's better to set sv_benchmark to 1 and restart the level." )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	g_ServerBenchmark.InternalStartBenchmark( 1, 1 );
}


// ---------------------------------------------------------------------------------------------- //
// CServerBenchmarkHook implementation.
// ---------------------------------------------------------------------------------------------- //

CServerBenchmarkHook *CServerBenchmarkHook::s_pBenchmarkHook = NULL;

CServerBenchmarkHook::CServerBenchmarkHook()
{
	if ( s_pBenchmarkHook )
		Error( "There can only be one CServerBenchmarkHook" );

	s_pBenchmarkHook = this;
}
