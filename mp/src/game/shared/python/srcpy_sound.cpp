//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
//=============================================================================//

#include "cbase.h"
#include "srcpy_sound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static PyEngineSound s_pysoundengine;
PyEngineSound *pysoundengine = &s_pysoundengine;