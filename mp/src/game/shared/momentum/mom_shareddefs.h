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

#define PANEL_TIMES "times"

// Main Version (0 is alpha, 1 is beta, 2 is release)​.Main feature push (increment by one for each)​.​Small commits or hotfixes​
// When editing this, remember to also edit version.txt on the main dir of the repo
#define MOM_CURRENT_VERSION "0.0.1"

#endif // MOM_SHAREDDEFS_H