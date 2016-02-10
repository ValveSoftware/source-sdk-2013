#ifndef MOM_SHAREDDEFS_H
#define MOM_SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif


#include "const.h"
#include "shareddefs.h"

// Gamemode for momentum
typedef enum MOMGM
{
    MOMGM_UNKNOWN = 0,
    MOMGM_SURF,
    MOMGM_BHOP,
    MOMGM_SCROLL,
    MOMGM_ALLOWED, //not "official gamemode" but must be allowed for other reasons
    
} GAMEMODES;
#endif // MOM_SHAREDDEFS_H