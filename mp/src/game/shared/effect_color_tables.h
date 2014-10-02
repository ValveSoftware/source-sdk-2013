//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef EFFECT_COLOR_TABLES_H
#define EFFECT_COLOR_TABLES_H
#ifdef _WIN32
#pragma once
#endif

struct colorentry_t
{
	unsigned char	index;
	
	unsigned char	r;
	unsigned char	g;
	unsigned char	b;
};

#define COLOR_TABLE_SIZE(ct) sizeof(ct)/sizeof(colorentry_t)

// Commander mode indicators (HL2)
enum
{
	COMMAND_POINT_RED = 0,
	COMMAND_POINT_BLUE,
	COMMAND_POINT_GREEN,
	COMMAND_POINT_YELLOW,
};

// Commander mode table
static colorentry_t commandercolors[] =
{
	{ COMMAND_POINT_RED,	1,	0,	0	},
	{ COMMAND_POINT_BLUE,	0,	0,	1	},
	{ COMMAND_POINT_GREEN,	0,	1,	0	},
	{ COMMAND_POINT_YELLOW,	1,	1,	0	},
};

static colorentry_t bloodcolors[] =
{
	{ BLOOD_COLOR_RED,		72,		0,		0	},
	{ BLOOD_COLOR_YELLOW,	195,	195,	0	},
	{ BLOOD_COLOR_MECH,		20,		20,		20	},
	{ BLOOD_COLOR_GREEN,	195,	195,	0	},
};

#endif // EFFECT_COLOR_TABLES_H
