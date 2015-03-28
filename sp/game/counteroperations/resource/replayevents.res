//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//=============================================================================

// Don't change "server_*" events 
// No spaces in event names, max length 32
// All strings are case sensitive
//
// valid data key types are:
//   string : a zero terminated string
//   bool   : unsigned int, 1 bit
//   byte   : unsigned int, 8 bit
//   short  : signed int, 16 bit
//   long   : signed int, 32 bit
//   float  : float, 32 bit
//   local : any data, dont network this field
//
// following keys names are reserved:
//   local      : if set to 1, event is not networked to clients
//   reliable   : if set to 0, event is networked unreliable


"replayevents"
{

	//////////////////////////////////////////////////////////////////////
	// replay specific events
	//////////////////////////////////////////////////////////////////////
	
	"replay_startrecord"	// Sent when the server begins recording - only used to display UI
	{
	}
	
	"replay_sessioninfo"	// Sent when the server begins recording, or when a client first connects - only sent once per recording session
	{
		"sn"	"string"	// session name
		"di"	"byte"		// dump interval
		"cb"	"long"		// current block
		"st"	"long"		// session start tick
	}
	
	"replay_endrecord"
	{
	}
	
	"replay_replaysavailable"
	{
	}
	
	"replay_servererror"
	{
		"error"	"string"
	}
}