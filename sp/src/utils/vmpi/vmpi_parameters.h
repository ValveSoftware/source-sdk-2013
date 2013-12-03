//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

VMPI_PARAM( mpi_Worker,						0,						"Workers use this to connect to a VMPI job. Specify the IP address of the master. Example: -mpi_worker 1.2.3.4  or  -mpi_worker 1.2.3.4:242" )
VMPI_PARAM( mpi_Port,						0,						"Use this on the master to force it to bind to a specified port. Otherwise it binds to 23311 (and ascending port numbers if 23311 doesn't work)." )
VMPI_PARAM( mpi_Graphics,					0,						"Show a graphical representation of work units [grey=work unit not sent yet, red=sent, green=completed, blue=in-process]" )
VMPI_PARAM( mpi_Retry,						0,						"Use this on the worker to have it retry connecting to the master forever. Otherwise it will exit if it can't connect to the master immediately." )
VMPI_PARAM( mpi_AutoRestart,				0,						"Use this on the worker to have it restart with the same command line parameters after completing a job. Useful in conjunction with -mpi_Retry to have an always-on worker ready to do work." )
VMPI_PARAM( mpi_TrackEvents,				0,						"Enables a debug menu during jobs (press D to access). Note: -mpi_Graphics automatically enables -mpi_TrackEvents." )
VMPI_PARAM( mpi_ShowDistributeWorkStats,	0,						"After finishing a stage in the work unit processing, shows statistics." )
VMPI_PARAM( mpi_TimingWait,					0,						"Causes the master to wait for a keypress to start so workers can connect before it starts. Used for performance measurements." )
VMPI_PARAM( mpi_WorkerCount,				0,						"Set the maximum number of workers allowed in the job." )
VMPI_PARAM( mpi_AutoLocalWorker,			0,						"Used on the master's machine. Automatically spawn a worker on the local machine. Used for testing." )
VMPI_PARAM( mpi_FileTransmitRate,			0,						"VMPI file transmission rate in kB/sec." )
VMPI_PARAM( mpi_Verbose,					0,						"Set to 0, 1, or 2 to control verbosity of debug output." )
VMPI_PARAM( mpi_NoMasterWorkerThreads,		0,						"Don't process work units locally (in the master). Only used by the SDK work unit distributor." )
VMPI_PARAM( mpi_SDKMode,					VMPI_PARAM_SDK_HIDDEN,	"Force VMPI to run in SDK mode." )
VMPI_PARAM( mpi_UseSDKDistributor,			VMPI_PARAM_SDK_HIDDEN,	"Use the SDK work unit distributor. Optimized for low numbers of workers and higher latency. Note that this will automatically be used in SDK distributions." )
VMPI_PARAM( mpi_UseDefaultDistributor,		VMPI_PARAM_SDK_HIDDEN,	"Use the default work unit distributor. Optimized for high numbers of workers, higher numbers of work units, and lower latency. Note that this will automatically be used in non-SDK distributions." )
VMPI_PARAM( mpi_NoTimeout,					VMPI_PARAM_SDK_HIDDEN,	"Don't timeout VMPI sockets. Used for testing." )
VMPI_PARAM( mpi_DontSetThreadPriorities,	VMPI_PARAM_SDK_HIDDEN,	"Don't set worker thread priorities to idle." )
VMPI_PARAM( mpi_GroupPackets,				VMPI_PARAM_SDK_HIDDEN,	"Delay and group some of the worker packets instead of sending immediately." )
VMPI_PARAM( mpi_Stats,						VMPI_PARAM_SDK_HIDDEN,	"Enables the use of a database to store compile statistics." )
VMPI_PARAM( mpi_Stats_TextOutput,			VMPI_PARAM_SDK_HIDDEN,	"Enables the workers storing all of their text output into the stats database." )
VMPI_PARAM( mpi_pw,							VMPI_PARAM_SDK_HIDDEN,	"Non-SDK only. Sets a password on the VMPI job. Workers must also use the same -mpi_pw [password] argument or else the master will ignore their requests to join the job." )
VMPI_PARAM( mpi_CalcShuffleCRC,				VMPI_PARAM_SDK_HIDDEN,	"Calculate a CRC for shuffled work unit arrays in the SDK work unit distributor." )
VMPI_PARAM( mpi_Job_Watch,					VMPI_PARAM_SDK_HIDDEN,	"Automatically launches vmpi_job_watch.exe on the job." )
VMPI_PARAM( mpi_Local,						VMPI_PARAM_SDK_HIDDEN,	"Similar to -mpi_AutoLocalWorker, but the automatically-spawned worker's console window is hidden." )