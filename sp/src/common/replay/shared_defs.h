//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef SHARED_DEFS_H
#define SHARED_DEFS_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "platform.h"

//----------------------------------------------------------------------------------------

#define SUBDIR_REPLAY				"replay"
#define SUBDIR_REPLAYS				"replays"
#define SUBDIR_SESSIONS				"sessions"
#define SUBDIR_BLOCKS				"blocks"
#define SUBDIR_CLIENT				"client"
#define SUBDIR_MOVIES				"movies"
#define SUBDIR_PERFORMANCES			"edits"
#define SUBDIR_SERVER				"server"
#define SUBDIR_RENDERED				"rendered"
#define SUBDIR_TMP					"tmp"

//----------------------------------------------------------------------------------------

#define BLOCK_FILE_EXTENSION		"block"
#define GENERIC_FILE_EXTENSION		"dmx"
#define DEMO_FILE_EXTENSION			"dem"

//----------------------------------------------------------------------------------------

#define MOVIE_HANDLE_BASE			10000	// 10,000

//----------------------------------------------------------------------------------------

#define BUILD_CURL					( defined( WIN32 ) && !defined( _X360 ) ) || defined( POSIX )

//----------------------------------------------------------------------------------------

#define MIN_SERVER_DUMP_INTERVAL	10
#define MAX_SERVER_DUMP_INTERVAL	30

#define DOWNLOAD_TIMEOUT_THRESHOLD	90		// Timeout for a replay download - if no blocks
											// are added or updated after this many seconds,
											// the replay will be put in the error state.

//----------------------------------------------------------------------------------------

#define MAX_TIMES_TO_SHOW_REPLAY_WELCOME_DLG	1

//----------------------------------------------------------------------------------------

#define MAX_SESSIONNAME_LENGTH		260
#define MAX_REPLAY_TITLE_LENGTH		256
#define MAX_TAKE_TITLE_LENGTH		256

//----------------------------------------------------------------------------------------

#define DEFAULT_COMPRESSOR_TYPE		COMPRESSORTYPE_BZ2

//----------------------------------------------------------------------------------------

#define JOB_FAILED					( (JobStatus_t) -1 )

#define DOWNLOAD_MAX_SIZE			( 8 * 1024 * 1024 )		// 8 MB

//----------------------------------------------------------------------------------------

#endif // SHARED_DEFS_H
