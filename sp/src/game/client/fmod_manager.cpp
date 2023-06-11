#include "cbase.h"
#include "fmod_manager.h"
#include "fmod_studio.hpp"
#include "fmod_errors.h"
#include "convar.h"

using namespace FMOD;

CFMODManager gFMODManager;

Studio::System *fmodStudioSystem;
Studio::Bank *fmodStudioBank;
Studio::Bank *fmodStudioStringsBank;
Studio::EventDescription *fmodStudioEventDescription;
Studio::EventInstance *fmodStudioEventInstance;

CFMODManager *FMODManager() {
    return &gFMODManager;
}

CFMODManager::CFMODManager()
= default;

CFMODManager::~CFMODManager()
= default;

// Starts FMOD
int CFMODManager::StartFMOD() {

    Msg("Starting FMOD");

    FMOD_RESULT result;

    result = FMOD::Studio::System::create(&fmodStudioSystem); // Create the Studio System object.
    if (result != FMOD_OK) {
        Msg("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }

    // Initialize FMOD Studio, which will also initialize FMOD Core
    result = fmodStudioSystem->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, 0);
    if (result != FMOD_OK) {
        Msg("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }

    Msg("FMOD successfully started");
    return (0);

}

// Stops FMOD
int CFMODManager::StopFMOD() {
    Msg("Stopping FMOD");
    FMOD_RESULT result;
    result = fmodStudioSystem->release();
    if (result != FMOD_OK) {
        Msg("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }
    Msg("FMOD successfully stopped");
    return (0);
}

// Get the path of a bank file in the /sound/fmod/banks folder
const char *CFMODManager::GetBankPath(const char *bankName) {
    char *bankPath = new char[512];
    Q_snprintf(bankPath, 512, "%s/sound/fmod/banks/%s", engine->GetGameDirectory(), bankName);
    // convert backwards slashes to forward slashes
    for (int i = 0; i < 512; i++) {
        if (bankPath[i] == '\\')
            bankPath[i] = '/';
    }
    return bankPath;
}

void PrintFMODStatus(const CCommand &args) {
    bool isValid = fmodStudioSystem->isValid();
    if (isValid) {
        Msg("FMOD Manager is currently running");
    } else {
        Msg("FMOD Manager is not running");
    }
}

static ConCommand getFMODStatus("fmod_getstatus", PrintFMODStatus, "FMOD: Get current status of the FMOD Manager");

void LoadSandboxBank() {
    FMOD_RESULT result;
    result = fmodStudioSystem->loadBankFile(CFMODManager::GetBankPath("Master.bank"), FMOD_STUDIO_LOAD_BANK_NORMAL, &fmodStudioBank);
    result = fmodStudioSystem->loadBankFile(CFMODManager::GetBankPath("Master.strings.bank"), FMOD_STUDIO_LOAD_BANK_NORMAL, &fmodStudioStringsBank);
}

static ConCommand loadSandboxBank("fmod_loadsandboxbank", LoadSandboxBank, "FMOD: Load the Sandbox bank");

void StartSandboxMusicEvent() {
    FMOD_RESULT result;
    result = fmodStudioSystem->getEvent("event:/Music", &fmodStudioEventDescription);
    result = fmodStudioEventDescription->createInstance(&fmodStudioEventInstance);
    result = fmodStudioEventInstance->start();
	fmodStudioSystem->update();
}

static ConCommand startSandboxMusicEvent("fmod_startsandboxmusicevent", StartSandboxMusicEvent, "FMOD: Start the Sandbox music event");
