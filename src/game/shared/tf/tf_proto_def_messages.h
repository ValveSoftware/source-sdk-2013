//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This file defines all of our over-the-wire net protocols for the
//			Game Coordinator for Team Fortress.  Note that we never use types
//			with undefined length (like int).  Always use an explicit type 
//			(like int32).
//
//=============================================================================

#ifndef TF_PROTO_DEF_MESSAGES_H
#define TF_PROTO_DEF_MESSAGES_H
#ifdef _WIN32
#pragma once
#endif


// Protobuf headers interfere with the valve min/max/malloc overrides. so we need to do all
// this funky wrapping to make the include happy.
#include <tier0/valve_minmax_off.h>

#include "tf_proto_def_messages.pb.h"

#include <tier0/valve_minmax_on.h>


#endif // TF_PROTO_DEF_MESSAGES_H
