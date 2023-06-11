#include "cbase.h"
#include "fmod_manager.h"
#include "fmod_studio.hpp"
#include "fmod_errors.h"
#include "convar.h"

using namespace FMOD;

CFMODManager gFMODManager;

Studio::System *fmodStudioSystem;

CFMODManager* FMODManager()
{
	return &gFMODManager;
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

	result = FMOD::Studio::System::create(&fmodStudioSystem); // Create the Studio System object.
	if (result != FMOD_OK)
	{
		printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		return(-1);
	}

	// Initialize FMOD Studio, which will also initialize FMOD Core
	result = fmodStudioSystem->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, 0);
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

// Get the path of a bank file in the /sound/fmod/banks folder
const char* CFMODManager::GetBankPath(const char* bankName)
{
	char* bankPath = new char[512];
	Q_snprintf(bankPath, 512, "%s/sound/fmod/banks/%s", engine->GetGameDirectory(), bankName);
	// convert backwards slashes to forward slashes
	for (int i = 0; i < 512; i++)
	{
		if (bankPath[i] == '\\')
			bankPath[i] = '/';
	}
	return bankPath;
}

void GetFMODStatus(const CCommand &args)
{
	Msg("Hello");
}
static ConCommand getFMODStatus("fmod_getstatus", GetFMODStatus, "FMOD: Get current status of the FMOD Manager");
