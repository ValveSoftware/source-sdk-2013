#include "cbase.h"
#include "fmod_manager.h"
#include "fmod_studio.hpp"
#include "fmod_errors.h"

using namespace FMOD;

CFMODManager gFMODMng;
CFMODManager* FMODManager()
{
	return &gFMODMng;
}

CFMODManager::CFMODManager()
{

}

CFMODManager::~CFMODManager()
{

}

// Starts FMOD
int CFMODManager::StartFMOD(void)
{
	
	Msg("Starting FMOD");
	
	FMOD_RESULT result;
	FMOD::Studio::System* system = NULL;

	result = FMOD::Studio::System::create(&system); // Create the Studio System object.
	if (result != FMOD_OK)
	{
		printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		return(-1);
	}

	// Initialize FMOD Studio, which will also initialize FMOD Core
	result = system->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, 0);
	if (result != FMOD_OK)
	{
		printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		return(-1);
	}

	Msg("FMOD successfully started");
	return(0);
}

// Stops FMOD
void CFMODManager::StopFMOD(void)
{
	Msg("Stopping FMOD");
}
