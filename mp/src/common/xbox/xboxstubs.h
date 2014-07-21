//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Win32 replacements for XBox.
//
//=============================================================================

#if !defined( XBOXSTUBS_H ) && !defined( _X360 )
#define XBOXSTUBS_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"

//  Content creation/open flags
#define XCONTENTFLAG_NONE                           0x00
#define XCONTENTFLAG_CREATENEW                      0x00
#define XCONTENTFLAG_CREATEALWAYS                   0x00
#define XCONTENTFLAG_OPENEXISTING                   0x00
#define XCONTENTFLAG_OPENALWAYS                     0x00
#define XCONTENTFLAG_TRUNCATEEXISTING               0x00

//  Content attributes
#define XCONTENTFLAG_NOPROFILE_TRANSFER             0x00
#define XCONTENTFLAG_NODEVICE_TRANSFER              0x00
#define XCONTENTFLAG_STRONG_SIGNED                  0x00
#define XCONTENTFLAG_ALLOWPROFILE_TRANSFER          0x00
#define XCONTENTFLAG_MOVEONLY_TRANSFER              0x00

// Console device ports
#define XDEVICE_PORT0               0
#define XDEVICE_PORT1               1
#define XDEVICE_PORT2               2
#define XDEVICE_PORT3               3
#define XUSER_MAX_COUNT				4
#define XUSER_INDEX_NONE            0x000000FE

#define XBX_CLR_DEFAULT				0xFF000000
#define XBX_CLR_WARNING				0x0000FFFF
#define XBX_CLR_ERROR				0x000000FF

#define XBOX_MINBORDERSAFE			0
#define XBOX_MAXBORDERSAFE			0

typedef enum
{
	XK_NULL,
	XK_BUTTON_UP,
	XK_BUTTON_DOWN,
	XK_BUTTON_LEFT,
	XK_BUTTON_RIGHT,
	XK_BUTTON_START,
	XK_BUTTON_BACK,
	XK_BUTTON_STICK1,
	XK_BUTTON_STICK2,
	XK_BUTTON_A,
	XK_BUTTON_B,
	XK_BUTTON_X,
	XK_BUTTON_Y,
	XK_BUTTON_LEFT_SHOULDER,
	XK_BUTTON_RIGHT_SHOULDER,
	XK_BUTTON_LTRIGGER,
	XK_BUTTON_RTRIGGER,
	XK_STICK1_UP,
	XK_STICK1_DOWN,
	XK_STICK1_LEFT,
	XK_STICK1_RIGHT,
	XK_STICK2_UP,
	XK_STICK2_DOWN,
	XK_STICK2_LEFT,
	XK_STICK2_RIGHT,
	XK_MAX_KEYS,
} xKey_t;

//typedef enum
//{
//	XVRB_NONE,		// off
//	XVRB_ERROR,		// fatal error
//	XVRB_ALWAYS,	// no matter what
//	XVRB_WARNING,	// non-fatal warnings
//	XVRB_STATUS,	// status reports
//	XVRB_ALL,
//} xverbose_e;

typedef unsigned short WORD;
#ifndef POSIX
typedef unsigned long DWORD;
typedef void* HANDLE;
typedef unsigned __int64 ULONGLONG;
#endif

#ifdef POSIX
typedef DWORD COLORREF;
#endif

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#endif

// typedef struct {
// 	IN_ADDR     ina;                            // IP address (zero if not static/DHCP)
// 	IN_ADDR     inaOnline;                      // Online IP address (zero if not online)
// 	WORD        wPortOnline;                    // Online port
// 	BYTE        abEnet[6];                      // Ethernet MAC address
// 	BYTE        abOnline[20];                   // Online identification
// } XNADDR;

typedef int XNADDR;
typedef uint64 XUID;

typedef struct {
	BYTE        ab[8];                          // xbox to xbox key identifier
} XNKID;

typedef struct {
	BYTE        ab[16];                         // xbox to xbox key exchange key
} XNKEY;

typedef struct _XSESSION_INFO
{
	XNKID sessionID;                // 8 bytes
	XNADDR hostAddress;             // 36 bytes
	XNKEY keyExchangeKey;           // 16 bytes
} XSESSION_INFO, *PXSESSION_INFO;

typedef struct _XUSER_DATA
{
	BYTE                                type;

	union
	{
		int                            nData;     // XUSER_DATA_TYPE_INT32
		int64                        i64Data;   // XUSER_DATA_TYPE_INT64
		double                          dblData;   // XUSER_DATA_TYPE_DOUBLE
		struct                                     // XUSER_DATA_TYPE_UNICODE
		{
			uint                       cbData;    // Includes null-terminator
			char *                      pwszData;
		} string;
		float                           fData;     // XUSER_DATA_TYPE_FLOAT
		struct                                     // XUSER_DATA_TYPE_BINARY
		{
			uint                       cbData;
			char *                       pbData;
		} binary;
	};
} XUSER_DATA, *PXUSER_DATA;

typedef struct _XUSER_PROPERTY
{
	DWORD                               dwPropertyId;
	XUSER_DATA                          value;
} XUSER_PROPERTY, *PXUSER_PROPERTY;

typedef struct _XUSER_CONTEXT
{
	DWORD                               dwContextId;
	DWORD                               dwValue;
} XUSER_CONTEXT, *PXUSER_CONTEXT;

typedef struct _XSESSION_SEARCHRESULT
{
	XSESSION_INFO   info;
	DWORD           dwOpenPublicSlots;
	DWORD           dwOpenPrivateSlots;
	DWORD           dwFilledPublicSlots;
	DWORD           dwFilledPrivateSlots;
	DWORD           cProperties;
	DWORD           cContexts;
	PXUSER_PROPERTY pProperties;
	PXUSER_CONTEXT  pContexts;
} XSESSION_SEARCHRESULT, *PXSESSION_SEARCHRESULT;

typedef struct _XSESSION_SEARCHRESULT_HEADER
{
	DWORD dwSearchResults;
	XSESSION_SEARCHRESULT *pResults;
} XSESSION_SEARCHRESULT_HEADER, *PXSESSION_SEARCHRESULT_HEADER;

typedef struct _XSESSION_REGISTRANT
{
	uint64 qwMachineID;
	DWORD bTrustworthiness;
	DWORD bNumUsers;
	XUID *rgUsers;

} XSESSION_REGISTRANT;

typedef struct _XSESSION_REGISTRATION_RESULTS
{
	DWORD wNumRegistrants;
	XSESSION_REGISTRANT *rgRegistrants;
} XSESSION_REGISTRATION_RESULTS, *PXSESSION_REGISTRATION_RESULTS;

typedef struct {
	BYTE        bFlags;                         
	BYTE        bReserved;                    
	WORD        cProbesXmit;                   
	WORD        cProbesRecv;                   
	WORD        cbData;                        
	BYTE *      pbData;                        
	WORD        wRttMinInMsecs;                
	WORD        wRttMedInMsecs;                
	DWORD       dwUpBitsPerSec;                
	DWORD       dwDnBitsPerSec;                
} XNQOSINFO;

typedef struct {
	uint        cxnqos;                        
	uint        cxnqosPending;                 
	XNQOSINFO   axnqosinfo[1];                 
} XNQOS;

#define XSESSION_CREATE_HOST				0
#define XUSER_DATA_TYPE_INT32				0
#define XSESSION_CREATE_USES_ARBITRATION	0
#define XNET_QOS_LISTEN_ENABLE				0
#define XNET_QOS_LISTEN_DISABLE				0
#define XNET_QOS_LISTEN_SET_DATA			0

FORCEINLINE void			XBX_ProcessEvents() {}
FORCEINLINE unsigned int	XBX_GetSystemTime() { return 0; }
FORCEINLINE	int				XBX_GetPrimaryUserId() { return 0; }
FORCEINLINE	void			XBX_SetPrimaryUserId( DWORD idx ) {}
FORCEINLINE	int				XBX_GetStorageDeviceId() { return 0; }
FORCEINLINE	void			XBX_SetStorageDeviceId( DWORD idx ) {}
FORCEINLINE const char		*XBX_GetLanguageString() { return ""; }
FORCEINLINE bool			XBX_IsLocalized() { return false; }

#define XCONTENT_MAX_DISPLAYNAME_LENGTH	128
#define XCONTENT_MAX_FILENAME_LENGTH	42

#define XBX_INVALID_STORAGE_ID ((DWORD) -1)
#define XBX_STORAGE_DECLINED ((DWORD) -2)

#endif // XBOXSTUBS_H
