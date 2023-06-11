#ifndef FMOD_MANAGER_H
#define FMOD_MANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "fmod.hpp"
#include "convar.h"

class CFMODManager
{
public:
	CFMODManager();
	~CFMODManager();

	int StartFMOD();
	int StopFMOD();

private:
	const char* GetBankPath(const char* bankName);
};

extern CFMODManager* FMODManager();

#endif // FMOD_MANAGER_H