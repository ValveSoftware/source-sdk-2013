//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#if !defined( PROTO_VERSION_H )
#define PROTO_VERSION_H
#ifdef _WIN32
#pragma once
#endif

// The current network protocol version.  Changing this makes clients and servers incompatible
#define PROTOCOL_VERSION    24

#define DEMO_BACKWARDCOMPATABILITY

// For backward compatibility of demo files (NET_MAX_PAYLOAD_BITS went away)
#define PROTOCOL_VERSION_23		23

// For backward compatibility of demo files (sound index bits used to = 13 )
#define PROTOCOL_VERSION_22		22

// For backward compatibility of demo files (before the special DSP was shipped to public)
#define PROTOCOL_VERSION_21		21

// For backward compatibility of demo files (old-style dynamic model loading)
#define PROTOCOL_VERSION_20		20

// For backward compatibility of demo files (post Halloween sound flag extra bit)
#define PROTOCOL_VERSION_19		19

// For backward compatibility of demo files (pre Halloween sound flag extra bit)
#define PROTOCOL_VERSION_18		18

// For backward compatibility of demo files (MD5 in map version)
#define PROTOCOL_VERSION_17		17

// For backward compatibility of demo files (create string tables compression flag)
#define PROTOCOL_VERSION_14		14

// For backward compatibility of demo files
#define PROTOCOL_VERSION_12		12

// The PROTOCOL_VERSION when replay shipped to public
#define PROTOCOL_VERSION_REPLAY		16

#endif
