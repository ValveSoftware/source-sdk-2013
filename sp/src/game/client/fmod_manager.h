#ifndef FMOD_MANAGER_H
#define FMOD_MANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "fmod.hpp"

class CFMODManager
{
public:
	CFMODManager();
	~CFMODManager();

	int StartFMOD();
	void StopFMOD();
};

extern CFMODManager* FMODManager();

#endif // FMOD_MANAGER_H