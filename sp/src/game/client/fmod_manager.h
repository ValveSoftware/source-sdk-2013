#ifndef FMOD_MANAGER_H
#define FMOD_MANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "fmod.hpp"
#include "convar.h"

class CFMODManager {
public:
    CFMODManager();

    ~CFMODManager();

    static int StartEngine();

    static int StopEngine();

    static int LoadBank(const char *bankName);

    static int StartEvent(const char *eventPath);

    static int SetGlobalParameter(const char *parameterName, float value);

    static const char *GetBankPath(const char *bankName);

private:

};

extern CFMODManager *FMODManager();

#endif // FMOD_MANAGER_H