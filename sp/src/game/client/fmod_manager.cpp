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

//// HELPER FUNCTIONS
//-----------------------------------------------------------------------------
// Purpose: Concatenate 2 strings together
// Input:
// - str1: The starting string
// - str2: The ending string
// Output: The joined 2 strings
//-----------------------------------------------------------------------------
const char *concatenate(const char *str1, const char *str2) {
    size_t len1 = 0;
    size_t len2 = 0;
    while (str1[len1] != '\0')
        ++len1;
    while (str2[len2] != '\0')
        ++len2;
    char *result = new char[len1 + len2 + 1]; // +1 for the null terminator
    for (size_t i = 0; i < len1; ++i)
        result[i] = str1[i];
    for (size_t i = 0; i < len2; ++i)
        result[len1 + i] = str2[i];
    result[len1 + len2] = '\0';
    return result;
}
//// END HELPER FUNCTIONS

//-----------------------------------------------------------------------------
// Purpose: Start the FMOD Studio System and initialize it
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CFMODManager::StartEngine() {
    Msg("Starting FMOD Engine\n");
    FMOD_RESULT result;
    result = FMOD::Studio::System::create(&fmodStudioSystem);
    if (result != FMOD_OK) {
        Error("FMOD Engine error! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }
    result = fmodStudioSystem->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);
    if (result != FMOD_OK) {
        Error("FMOD Engine error! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }
    Log("FMOD Engine successfully started\n");
    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Stop the FMOD Studio System
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CFMODManager::StopEngine() {
    Msg("Stopping FMOD Engine\n");
    FMOD_RESULT result;
    result = fmodStudioSystem->release();
    if (result != FMOD_OK) {
        Error("FMOD Engine error! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }
    Log("FMOD Engine successfully stopped\n");
    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Sanitize the name of an FMOD Bank, adding ".bank" if it's not already present
// Input: The FMOD Bank name to sanitize
// Output: The sanitized Bank name (same as the initial if it was already ending with ".bank")
//-----------------------------------------------------------------------------
const char *SanitizeBankName(const char *bankName) {
    const char *bankExtension = ".bank";
    size_t bankNameLength = strlen(bankName);
    size_t bankExtensionLength = strlen(bankExtension);
    if (bankNameLength >= bankExtensionLength && strcmp(bankName + bankNameLength - bankExtensionLength, bankExtension) == 0) {
        return bankName;
    }
    return concatenate(bankName, bankExtension);
}

//-----------------------------------------------------------------------------
// Purpose: Get the path of a Bank file in the /sound/fmod/banks folder
// Input: The FMOD Bank name to locate
// Output: The FMOD Bank's full path from the file system
//-----------------------------------------------------------------------------
const char *CFMODManager::GetBankPath(const char *bankName) {
    const char *sanitizedBankName = SanitizeBankName(bankName);
    char *bankPath = new char[512];
    Q_snprintf(bankPath, 512, "%s/sound/fmod/banks/%s", engine->GetGameDirectory(), sanitizedBankName);
    // convert backwards slashes to forward slashes
    for (int i = 0; i < 512; i++) {
        if (bankPath[i] == '\\')
            bankPath[i] = '/';
    }
    return bankPath;
}

//-----------------------------------------------------------------------------
// Purpose: Provide a console command to print the FMOD Engine Status
//-----------------------------------------------------------------------------
void CC_GetStatus() {
    bool isValid = fmodStudioSystem->isValid();
    if (isValid) {
        Msg("FMOD Manager is currently running\n");
    } else {
        Msg("FMOD Manager is not running\n");
    }
}

static ConCommand getStatus("fmod_getstatus", CC_GetStatus, "FMOD: Get current status of the FMOD Manager");

//-----------------------------------------------------------------------------
// Purpose: Load an FMOD Bank
// Input: The name of the FMOD Bank to load
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CFMODManager::LoadBank(const char *bankName) {
    FMOD_RESULT result;
    result = fmodStudioSystem->loadBankFile(CFMODManager::GetBankPath(bankName), FMOD_STUDIO_LOAD_BANK_NORMAL, &fmodStudioBank);
    if (result != FMOD_OK) {
        Warning("Could not load FMOD bank (%s). Error: (%d) %s\n", bankName, result, FMOD_ErrorString(result));
        return (-1);
    }
    const char *bankStringsName = concatenate(bankName, ".strings");
    result = fmodStudioSystem->loadBankFile(CFMODManager::GetBankPath(bankStringsName), FMOD_STUDIO_LOAD_BANK_NORMAL, &fmodStudioStringsBank);
    if (result != FMOD_OK) {
        Warning("Could not load FMOD strings bank (%s). Error: (%d) %s\n", bankStringsName, result, FMOD_ErrorString(result));
        return (-1);
    }
    Log("FMOD bank successfully loaded (%s)\n", bankName);
    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Provide a console command to load a FMOD Bank
// Input: The name of the FMOD Bank to load as ConCommand argument
//-----------------------------------------------------------------------------
void CC_LoadBank(const CCommand &args) {
    if (args.ArgC() < 1 || strcmp(args.Arg(1), "") == 0) {
        Msg("Usage: fmod_loadbank <bankname>\n");
        return;
    }
    CFMODManager::LoadBank(args.Arg(1));
}

static ConCommand loadBank("fmod_loadbank", CC_LoadBank, "FMOD: Load a bank");

//-----------------------------------------------------------------------------
// Purpose: Start an FMOD Event
// Input: The name of the FMOD Event to start
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CFMODManager::StartEvent(const char *eventPath) {
    const char *fullEventPath = concatenate("event:/", eventPath);
    FMOD_RESULT result;
    result = fmodStudioSystem->getEvent(fullEventPath, &fmodStudioEventDescription);
    result = fmodStudioEventDescription->createInstance(&fmodStudioEventInstance);
    result = fmodStudioEventInstance->start();
    fmodStudioSystem->update();
    if (result != FMOD_OK) {
        Warning("Could not start FMOD event (%s). Error: (%d) %s\n", eventPath, result, FMOD_ErrorString(result));
        return (-1);
    }
    Log("FMOD event successfully started (%s)\n", eventPath);
    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Provide a console command to start an FMOD Event
// Input: The name of the FMOD Event to load as ConCommand argument
//-----------------------------------------------------------------------------
void CC_StartEvent(const CCommand &args) {
    if (args.ArgC() < 1 || strcmp(args.Arg(1), "") == 0) {
        Msg("Usage: fmod_startevent <eventpath>\n");
        return;
    }
    CFMODManager::StartEvent(args.Arg(1));
}

static ConCommand startEvent("fmod_startevent", CC_StartEvent, "FMOD: Start an event");

//-----------------------------------------------------------------------------
// Purpose: Set the value for a global FMOD Parameter
// Input:
// - parameterName: The name of the FMOD Parameter to set
// - value: The value to set the FMOD Parameter to
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CFMODManager::SetGlobalParameter(const char *parameterName, float value) {;
    FMOD_RESULT result;
    result = fmodStudioSystem->setParameterByName(parameterName, value);
    fmodStudioSystem->update();
    if (result != FMOD_OK) {
        Warning("Could not set FMOD global parameter value (%s) (%f). Error: (%d) %s\n", parameterName, value, result, FMOD_ErrorString(result));
        return (-1);
    }
    Log("FMOD parameter successfully set (%s) (%f)\n", parameterName, value);
    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Provide a console command to the value for a global FMOD Parameter
// Input:
// - Arg(1): The name of the FMOD Parameter to set (to load as ConCommand argument)
// - Arg(2): The value to set the FMOD Parameter to (to load as ConCommand argument)
//-----------------------------------------------------------------------------
void CC_SetGlobalParameter(const CCommand &args) {
    if (args.ArgC() < 2 || strcmp(args.Arg(1), "") == 0 || strcmp(args.Arg(2), "") == 0) {
        Msg("Usage: fmod_setglobalparameter <parametername> <value>\n");
        return;
    }
    CFMODManager::SetGlobalParameter(args.Arg(1), atof(args.Arg(2)));
}

static ConCommand setGlobalParameter("fmod_setglobalparameter", CC_SetGlobalParameter, "FMOD: Set a global parameter");