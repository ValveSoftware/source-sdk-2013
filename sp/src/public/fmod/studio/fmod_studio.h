/* ======================================================================================== */
/* FMOD Studio API - C header file.                                                         */
/* Copyright (c), Firelight Technologies Pty, Ltd. 2004-2023.                               */
/*                                                                                          */
/* Use this header in conjunction with fmod_studio_common.h (which contains all the         */
/* constants / callbacks) to develop using the C language.                                  */
/*                                                                                          */
/* For more detail visit:                                                                   */
/* https://fmod.com/docs/2.02/api/studio-api.html                                           */
/* ======================================================================================== */
#ifndef FMOD_STUDIO_H
#define FMOD_STUDIO_H

#include "fmod_studio_common.h"

#ifdef __cplusplus
extern "C" 
{
#endif

/*
    Global
*/
FMOD_RESULT F_API FMOD_Studio_ParseID(const char *idstring, FMOD_GUID *id);
FMOD_RESULT F_API FMOD_Studio_System_Create(FMOD_STUDIO_SYSTEM **system, unsigned int headerversion);

/*
    System
*/
FMOD_BOOL   F_API FMOD_Studio_System_IsValid(FMOD_STUDIO_SYSTEM *system);
FMOD_RESULT F_API FMOD_Studio_System_SetAdvancedSettings(FMOD_STUDIO_SYSTEM *system, FMOD_STUDIO_ADVANCEDSETTINGS *settings);
FMOD_RESULT F_API FMOD_Studio_System_GetAdvancedSettings(FMOD_STUDIO_SYSTEM *system, FMOD_STUDIO_ADVANCEDSETTINGS *settings);
FMOD_RESULT F_API FMOD_Studio_System_Initialize(FMOD_STUDIO_SYSTEM *system, int maxchannels, FMOD_STUDIO_INITFLAGS studioflags, FMOD_INITFLAGS flags, void *extradriverdata);
FMOD_RESULT F_API FMOD_Studio_System_Release(FMOD_STUDIO_SYSTEM *system);
FMOD_RESULT F_API FMOD_Studio_System_Update(FMOD_STUDIO_SYSTEM *system);
FMOD_RESULT F_API FMOD_Studio_System_GetCoreSystem(FMOD_STUDIO_SYSTEM *system, FMOD_SYSTEM **coresystem);
FMOD_RESULT F_API FMOD_Studio_System_GetEvent(FMOD_STUDIO_SYSTEM *system, const char *pathOrID, FMOD_STUDIO_EVENTDESCRIPTION **event);
FMOD_RESULT F_API FMOD_Studio_System_GetBus(FMOD_STUDIO_SYSTEM *system, const char *pathOrID, FMOD_STUDIO_BUS **bus);
FMOD_RESULT F_API FMOD_Studio_System_GetVCA(FMOD_STUDIO_SYSTEM *system, const char *pathOrID, FMOD_STUDIO_VCA **vca);
FMOD_RESULT F_API FMOD_Studio_System_GetBank(FMOD_STUDIO_SYSTEM *system, const char *pathOrID, FMOD_STUDIO_BANK **bank);
FMOD_RESULT F_API FMOD_Studio_System_GetEventByID(FMOD_STUDIO_SYSTEM *system, const FMOD_GUID *id, FMOD_STUDIO_EVENTDESCRIPTION **event);
FMOD_RESULT F_API FMOD_Studio_System_GetBusByID(FMOD_STUDIO_SYSTEM *system, const FMOD_GUID *id, FMOD_STUDIO_BUS **bus);
FMOD_RESULT F_API FMOD_Studio_System_GetVCAByID(FMOD_STUDIO_SYSTEM *system, const FMOD_GUID *id, FMOD_STUDIO_VCA **vca);
FMOD_RESULT F_API FMOD_Studio_System_GetBankByID(FMOD_STUDIO_SYSTEM *system, const FMOD_GUID *id, FMOD_STUDIO_BANK **bank);
FMOD_RESULT F_API FMOD_Studio_System_GetSoundInfo(FMOD_STUDIO_SYSTEM *system, const char *key, FMOD_STUDIO_SOUND_INFO *info);
FMOD_RESULT F_API FMOD_Studio_System_GetParameterDescriptionByName(FMOD_STUDIO_SYSTEM *system, const char *name, FMOD_STUDIO_PARAMETER_DESCRIPTION *parameter);
FMOD_RESULT F_API FMOD_Studio_System_GetParameterDescriptionByID(FMOD_STUDIO_SYSTEM *system, FMOD_STUDIO_PARAMETER_ID id, FMOD_STUDIO_PARAMETER_DESCRIPTION *parameter);
FMOD_RESULT F_API FMOD_Studio_System_GetParameterLabelByName(FMOD_STUDIO_SYSTEM *system, const char *name, int labelindex, char *label, int size, int *retrieved);
FMOD_RESULT F_API FMOD_Studio_System_GetParameterLabelByID(FMOD_STUDIO_SYSTEM *system, FMOD_STUDIO_PARAMETER_ID id, int labelindex, char *label, int size, int *retrieved);
FMOD_RESULT F_API FMOD_Studio_System_GetParameterByID(FMOD_STUDIO_SYSTEM *system, FMOD_STUDIO_PARAMETER_ID id, float *value, float *finalvalue);
FMOD_RESULT F_API FMOD_Studio_System_SetParameterByID(FMOD_STUDIO_SYSTEM *system, FMOD_STUDIO_PARAMETER_ID id, float value, FMOD_BOOL ignoreseekspeed);
FMOD_RESULT F_API FMOD_Studio_System_SetParameterByIDWithLabel(FMOD_STUDIO_SYSTEM *system, FMOD_STUDIO_PARAMETER_ID id, const char *label, FMOD_BOOL ignoreseekspeed);
FMOD_RESULT F_API FMOD_Studio_System_SetParametersByIDs(FMOD_STUDIO_SYSTEM *system, const FMOD_STUDIO_PARAMETER_ID *ids, float *values, int count, FMOD_BOOL ignoreseekspeed);
FMOD_RESULT F_API FMOD_Studio_System_GetParameterByName(FMOD_STUDIO_SYSTEM *system, const char *name, float *value, float *finalvalue);
FMOD_RESULT F_API FMOD_Studio_System_SetParameterByName(FMOD_STUDIO_SYSTEM *system, const char *name, float value, FMOD_BOOL ignoreseekspeed);
FMOD_RESULT F_API FMOD_Studio_System_SetParameterByNameWithLabel(FMOD_STUDIO_SYSTEM *system, const char *name, const char *label, FMOD_BOOL ignoreseekspeed);
FMOD_RESULT F_API FMOD_Studio_System_LookupID(FMOD_STUDIO_SYSTEM *system, const char *path, FMOD_GUID *id);
FMOD_RESULT F_API FMOD_Studio_System_LookupPath(FMOD_STUDIO_SYSTEM *system, const FMOD_GUID *id, char *path, int size, int *retrieved);
FMOD_RESULT F_API FMOD_Studio_System_GetNumListeners(FMOD_STUDIO_SYSTEM *system, int *numlisteners);
FMOD_RESULT F_API FMOD_Studio_System_SetNumListeners(FMOD_STUDIO_SYSTEM *system, int numlisteners);
FMOD_RESULT F_API FMOD_Studio_System_GetListenerAttributes(FMOD_STUDIO_SYSTEM *system, int index, FMOD_3D_ATTRIBUTES *attributes, FMOD_VECTOR *attenuationposition);
FMOD_RESULT F_API FMOD_Studio_System_SetListenerAttributes(FMOD_STUDIO_SYSTEM *system, int index, const FMOD_3D_ATTRIBUTES *attributes, const FMOD_VECTOR *attenuationposition);
FMOD_RESULT F_API FMOD_Studio_System_GetListenerWeight(FMOD_STUDIO_SYSTEM *system, int index, float *weight);
FMOD_RESULT F_API FMOD_Studio_System_SetListenerWeight(FMOD_STUDIO_SYSTEM *system, int index, float weight);
FMOD_RESULT F_API FMOD_Studio_System_LoadBankFile(FMOD_STUDIO_SYSTEM *system, const char *filename, FMOD_STUDIO_LOAD_BANK_FLAGS flags, FMOD_STUDIO_BANK **bank);
FMOD_RESULT F_API FMOD_Studio_System_LoadBankMemory(FMOD_STUDIO_SYSTEM *system, const char *buffer, int length, FMOD_STUDIO_LOAD_MEMORY_MODE mode, FMOD_STUDIO_LOAD_BANK_FLAGS flags, FMOD_STUDIO_BANK **bank);
FMOD_RESULT F_API FMOD_Studio_System_LoadBankCustom(FMOD_STUDIO_SYSTEM *system, const FMOD_STUDIO_BANK_INFO *info, FMOD_STUDIO_LOAD_BANK_FLAGS flags, FMOD_STUDIO_BANK **bank);
FMOD_RESULT F_API FMOD_Studio_System_RegisterPlugin(FMOD_STUDIO_SYSTEM *system, const FMOD_DSP_DESCRIPTION *description);
FMOD_RESULT F_API FMOD_Studio_System_UnregisterPlugin(FMOD_STUDIO_SYSTEM *system, const char *name);
FMOD_RESULT F_API FMOD_Studio_System_UnloadAll(FMOD_STUDIO_SYSTEM *system);
FMOD_RESULT F_API FMOD_Studio_System_FlushCommands(FMOD_STUDIO_SYSTEM *system);
FMOD_RESULT F_API FMOD_Studio_System_FlushSampleLoading(FMOD_STUDIO_SYSTEM *system);
FMOD_RESULT F_API FMOD_Studio_System_StartCommandCapture(FMOD_STUDIO_SYSTEM *system, const char *filename, FMOD_STUDIO_COMMANDCAPTURE_FLAGS flags);
FMOD_RESULT F_API FMOD_Studio_System_StopCommandCapture(FMOD_STUDIO_SYSTEM *system);
FMOD_RESULT F_API FMOD_Studio_System_LoadCommandReplay(FMOD_STUDIO_SYSTEM *system, const char *filename, FMOD_STUDIO_COMMANDREPLAY_FLAGS flags, FMOD_STUDIO_COMMANDREPLAY **replay);
FMOD_RESULT F_API FMOD_Studio_System_GetBankCount(FMOD_STUDIO_SYSTEM *system, int *count);
FMOD_RESULT F_API FMOD_Studio_System_GetBankList(FMOD_STUDIO_SYSTEM *system, FMOD_STUDIO_BANK **array, int capacity, int *count);
FMOD_RESULT F_API FMOD_Studio_System_GetParameterDescriptionCount(FMOD_STUDIO_SYSTEM *system, int *count);
FMOD_RESULT F_API FMOD_Studio_System_GetParameterDescriptionList(FMOD_STUDIO_SYSTEM *system, FMOD_STUDIO_PARAMETER_DESCRIPTION *array, int capacity, int *count);
FMOD_RESULT F_API FMOD_Studio_System_GetCPUUsage(FMOD_STUDIO_SYSTEM *system, FMOD_STUDIO_CPU_USAGE *usage, FMOD_CPU_USAGE *usage_core);
FMOD_RESULT F_API FMOD_Studio_System_GetBufferUsage(FMOD_STUDIO_SYSTEM *system, FMOD_STUDIO_BUFFER_USAGE *usage);
FMOD_RESULT F_API FMOD_Studio_System_ResetBufferUsage(FMOD_STUDIO_SYSTEM *system);
FMOD_RESULT F_API FMOD_Studio_System_SetCallback(FMOD_STUDIO_SYSTEM *system, FMOD_STUDIO_SYSTEM_CALLBACK callback, FMOD_STUDIO_SYSTEM_CALLBACK_TYPE callbackmask);
FMOD_RESULT F_API FMOD_Studio_System_SetUserData(FMOD_STUDIO_SYSTEM *system, void *userdata);
FMOD_RESULT F_API FMOD_Studio_System_GetUserData(FMOD_STUDIO_SYSTEM *system, void **userdata);
FMOD_RESULT F_API FMOD_Studio_System_GetMemoryUsage(FMOD_STUDIO_SYSTEM *system, FMOD_STUDIO_MEMORY_USAGE *memoryusage);

/*
    EventDescription
*/
FMOD_BOOL   F_API FMOD_Studio_EventDescription_IsValid(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription);
FMOD_RESULT F_API FMOD_Studio_EventDescription_GetID(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, FMOD_GUID *id);
FMOD_RESULT F_API FMOD_Studio_EventDescription_GetPath(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, char *path, int size, int *retrieved);
FMOD_RESULT F_API FMOD_Studio_EventDescription_GetParameterDescriptionCount(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, int *count);
FMOD_RESULT F_API FMOD_Studio_EventDescription_GetParameterDescriptionByIndex(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, int index, FMOD_STUDIO_PARAMETER_DESCRIPTION *parameter);
FMOD_RESULT F_API FMOD_Studio_EventDescription_GetParameterDescriptionByName(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, const char *name, FMOD_STUDIO_PARAMETER_DESCRIPTION *parameter);
FMOD_RESULT F_API FMOD_Studio_EventDescription_GetParameterDescriptionByID(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, FMOD_STUDIO_PARAMETER_ID id, FMOD_STUDIO_PARAMETER_DESCRIPTION *parameter);
FMOD_RESULT F_API FMOD_Studio_EventDescription_GetParameterLabelByIndex(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, int index, int labelindex, char *label, int size, int *retrieved);
FMOD_RESULT F_API FMOD_Studio_EventDescription_GetParameterLabelByName(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, const char *name, int labelindex, char *label, int size, int *retrieved);
FMOD_RESULT F_API FMOD_Studio_EventDescription_GetParameterLabelByID(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, FMOD_STUDIO_PARAMETER_ID id, int labelindex, char *label, int size, int *retrieved);
FMOD_RESULT F_API FMOD_Studio_EventDescription_GetUserPropertyCount(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, int *count);
FMOD_RESULT F_API FMOD_Studio_EventDescription_GetUserPropertyByIndex(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, int index, FMOD_STUDIO_USER_PROPERTY *property);
FMOD_RESULT F_API FMOD_Studio_EventDescription_GetUserProperty(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, const char *name, FMOD_STUDIO_USER_PROPERTY *property);
FMOD_RESULT F_API FMOD_Studio_EventDescription_GetLength(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, int *length);
FMOD_RESULT F_API FMOD_Studio_EventDescription_GetMinMaxDistance(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, float *min, float *max);
FMOD_RESULT F_API FMOD_Studio_EventDescription_GetSoundSize(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, float *size);
FMOD_RESULT F_API FMOD_Studio_EventDescription_IsSnapshot(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, FMOD_BOOL *snapshot);
FMOD_RESULT F_API FMOD_Studio_EventDescription_IsOneshot(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, FMOD_BOOL *oneshot);
FMOD_RESULT F_API FMOD_Studio_EventDescription_IsStream(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, FMOD_BOOL *isStream);
FMOD_RESULT F_API FMOD_Studio_EventDescription_Is3D(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, FMOD_BOOL *is3D);
FMOD_RESULT F_API FMOD_Studio_EventDescription_IsDopplerEnabled(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, FMOD_BOOL *doppler);
FMOD_RESULT F_API FMOD_Studio_EventDescription_HasSustainPoint(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, FMOD_BOOL *sustainPoint);
FMOD_RESULT F_API FMOD_Studio_EventDescription_CreateInstance(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, FMOD_STUDIO_EVENTINSTANCE **instance);
FMOD_RESULT F_API FMOD_Studio_EventDescription_GetInstanceCount(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, int *count);
FMOD_RESULT F_API FMOD_Studio_EventDescription_GetInstanceList(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, FMOD_STUDIO_EVENTINSTANCE **array, int capacity, int *count);
FMOD_RESULT F_API FMOD_Studio_EventDescription_LoadSampleData(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription);
FMOD_RESULT F_API FMOD_Studio_EventDescription_UnloadSampleData(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription);
FMOD_RESULT F_API FMOD_Studio_EventDescription_GetSampleLoadingState(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, FMOD_STUDIO_LOADING_STATE *state);
FMOD_RESULT F_API FMOD_Studio_EventDescription_ReleaseAllInstances(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription);
FMOD_RESULT F_API FMOD_Studio_EventDescription_SetCallback(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, FMOD_STUDIO_EVENT_CALLBACK callback, FMOD_STUDIO_EVENT_CALLBACK_TYPE callbackmask);
FMOD_RESULT F_API FMOD_Studio_EventDescription_GetUserData(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, void **userdata);
FMOD_RESULT F_API FMOD_Studio_EventDescription_SetUserData(FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, void *userdata);   

/*
    EventInstance
*/
FMOD_BOOL   F_API FMOD_Studio_EventInstance_IsValid(FMOD_STUDIO_EVENTINSTANCE *eventinstance);
FMOD_RESULT F_API FMOD_Studio_EventInstance_GetDescription(FMOD_STUDIO_EVENTINSTANCE *eventinstance, FMOD_STUDIO_EVENTDESCRIPTION **description);
FMOD_RESULT F_API FMOD_Studio_EventInstance_GetVolume(FMOD_STUDIO_EVENTINSTANCE *eventinstance, float *volume, float *finalvolume);
FMOD_RESULT F_API FMOD_Studio_EventInstance_SetVolume(FMOD_STUDIO_EVENTINSTANCE *eventinstance, float volume);
FMOD_RESULT F_API FMOD_Studio_EventInstance_GetPitch(FMOD_STUDIO_EVENTINSTANCE *eventinstance, float *pitch, float *finalpitch);
FMOD_RESULT F_API FMOD_Studio_EventInstance_SetPitch(FMOD_STUDIO_EVENTINSTANCE *eventinstance, float pitch);
FMOD_RESULT F_API FMOD_Studio_EventInstance_Get3DAttributes(FMOD_STUDIO_EVENTINSTANCE *eventinstance, FMOD_3D_ATTRIBUTES *attributes);
FMOD_RESULT F_API FMOD_Studio_EventInstance_Set3DAttributes(FMOD_STUDIO_EVENTINSTANCE *eventinstance, FMOD_3D_ATTRIBUTES *attributes);
FMOD_RESULT F_API FMOD_Studio_EventInstance_GetListenerMask(FMOD_STUDIO_EVENTINSTANCE *eventinstance, unsigned int *mask);
FMOD_RESULT F_API FMOD_Studio_EventInstance_SetListenerMask(FMOD_STUDIO_EVENTINSTANCE *eventinstance, unsigned int mask);
FMOD_RESULT F_API FMOD_Studio_EventInstance_GetProperty(FMOD_STUDIO_EVENTINSTANCE *eventinstance, FMOD_STUDIO_EVENT_PROPERTY index, float *value);
FMOD_RESULT F_API FMOD_Studio_EventInstance_SetProperty(FMOD_STUDIO_EVENTINSTANCE *eventinstance, FMOD_STUDIO_EVENT_PROPERTY index, float value);
FMOD_RESULT F_API FMOD_Studio_EventInstance_GetReverbLevel(FMOD_STUDIO_EVENTINSTANCE *eventinstance, int index, float *level);
FMOD_RESULT F_API FMOD_Studio_EventInstance_SetReverbLevel(FMOD_STUDIO_EVENTINSTANCE *eventinstance, int index, float level);
FMOD_RESULT F_API FMOD_Studio_EventInstance_GetPaused(FMOD_STUDIO_EVENTINSTANCE *eventinstance, FMOD_BOOL *paused);
FMOD_RESULT F_API FMOD_Studio_EventInstance_SetPaused(FMOD_STUDIO_EVENTINSTANCE *eventinstance, FMOD_BOOL paused);
FMOD_RESULT F_API FMOD_Studio_EventInstance_Start(FMOD_STUDIO_EVENTINSTANCE *eventinstance);
FMOD_RESULT F_API FMOD_Studio_EventInstance_Stop(FMOD_STUDIO_EVENTINSTANCE *eventinstance, FMOD_STUDIO_STOP_MODE mode);
FMOD_RESULT F_API FMOD_Studio_EventInstance_GetTimelinePosition(FMOD_STUDIO_EVENTINSTANCE *eventinstance, int *position);
FMOD_RESULT F_API FMOD_Studio_EventInstance_SetTimelinePosition(FMOD_STUDIO_EVENTINSTANCE *eventinstance, int position);
FMOD_RESULT F_API FMOD_Studio_EventInstance_GetPlaybackState(FMOD_STUDIO_EVENTINSTANCE *eventinstance, FMOD_STUDIO_PLAYBACK_STATE *state);
FMOD_RESULT F_API FMOD_Studio_EventInstance_GetChannelGroup(FMOD_STUDIO_EVENTINSTANCE *eventinstance, FMOD_CHANNELGROUP **group);
FMOD_RESULT F_API FMOD_Studio_EventInstance_GetMinMaxDistance(FMOD_STUDIO_EVENTINSTANCE *eventinstance, float *min, float *max);
FMOD_RESULT F_API FMOD_Studio_EventInstance_Release(FMOD_STUDIO_EVENTINSTANCE *eventinstance);
FMOD_RESULT F_API FMOD_Studio_EventInstance_IsVirtual(FMOD_STUDIO_EVENTINSTANCE *eventinstance, FMOD_BOOL *virtualstate);
FMOD_RESULT F_API FMOD_Studio_EventInstance_GetParameterByName(FMOD_STUDIO_EVENTINSTANCE *eventinstance, const char *name, float *value, float *finalvalue);
FMOD_RESULT F_API FMOD_Studio_EventInstance_SetParameterByName(FMOD_STUDIO_EVENTINSTANCE *eventinstance, const char *name, float value, FMOD_BOOL ignoreseekspeed);
FMOD_RESULT F_API FMOD_Studio_EventInstance_SetParameterByNameWithLabel(FMOD_STUDIO_EVENTINSTANCE *eventinstance, const char *name, const char *label, FMOD_BOOL ignoreseekspeed);
FMOD_RESULT F_API FMOD_Studio_EventInstance_GetParameterByID(FMOD_STUDIO_EVENTINSTANCE *eventinstance, FMOD_STUDIO_PARAMETER_ID id, float *value, float *finalvalue);
FMOD_RESULT F_API FMOD_Studio_EventInstance_SetParameterByID(FMOD_STUDIO_EVENTINSTANCE *eventinstance, FMOD_STUDIO_PARAMETER_ID id, float value, FMOD_BOOL ignoreseekspeed);
FMOD_RESULT F_API FMOD_Studio_EventInstance_SetParameterByIDWithLabel(FMOD_STUDIO_EVENTINSTANCE *eventinstance, FMOD_STUDIO_PARAMETER_ID id, const char *label, FMOD_BOOL ignoreseekspeed);
FMOD_RESULT F_API FMOD_Studio_EventInstance_SetParametersByIDs(FMOD_STUDIO_EVENTINSTANCE *eventinstance, const FMOD_STUDIO_PARAMETER_ID *ids, float *values, int count, FMOD_BOOL ignoreseekspeed);
FMOD_RESULT F_API FMOD_Studio_EventInstance_KeyOff(FMOD_STUDIO_EVENTINSTANCE *eventinstance);
FMOD_RESULT F_API FMOD_Studio_EventInstance_SetCallback(FMOD_STUDIO_EVENTINSTANCE *eventinstance, FMOD_STUDIO_EVENT_CALLBACK callback, FMOD_STUDIO_EVENT_CALLBACK_TYPE callbackmask);
FMOD_RESULT F_API FMOD_Studio_EventInstance_GetUserData(FMOD_STUDIO_EVENTINSTANCE *eventinstance, void **userdata);
FMOD_RESULT F_API FMOD_Studio_EventInstance_SetUserData(FMOD_STUDIO_EVENTINSTANCE *eventinstance, void *userdata);
FMOD_RESULT F_API FMOD_Studio_EventInstance_GetCPUUsage(FMOD_STUDIO_EVENTINSTANCE *eventinstance, unsigned int *exclusive, unsigned int *inclusive);
FMOD_RESULT F_API FMOD_Studio_EventInstance_GetMemoryUsage(FMOD_STUDIO_EVENTINSTANCE *eventinstance, FMOD_STUDIO_MEMORY_USAGE *memoryusage);

/*
    Bus
*/
FMOD_BOOL   F_API FMOD_Studio_Bus_IsValid(FMOD_STUDIO_BUS *bus);
FMOD_RESULT F_API FMOD_Studio_Bus_GetID(FMOD_STUDIO_BUS *bus, FMOD_GUID *id);
FMOD_RESULT F_API FMOD_Studio_Bus_GetPath(FMOD_STUDIO_BUS *bus, char *path, int size, int *retrieved);
FMOD_RESULT F_API FMOD_Studio_Bus_GetVolume(FMOD_STUDIO_BUS *bus, float *volume, float *finalvolume);
FMOD_RESULT F_API FMOD_Studio_Bus_SetVolume(FMOD_STUDIO_BUS *bus, float volume);
FMOD_RESULT F_API FMOD_Studio_Bus_GetPaused(FMOD_STUDIO_BUS *bus, FMOD_BOOL *paused);
FMOD_RESULT F_API FMOD_Studio_Bus_SetPaused(FMOD_STUDIO_BUS *bus, FMOD_BOOL paused);
FMOD_RESULT F_API FMOD_Studio_Bus_GetMute(FMOD_STUDIO_BUS *bus, FMOD_BOOL *mute);
FMOD_RESULT F_API FMOD_Studio_Bus_SetMute(FMOD_STUDIO_BUS *bus, FMOD_BOOL mute);
FMOD_RESULT F_API FMOD_Studio_Bus_StopAllEvents(FMOD_STUDIO_BUS *bus, FMOD_STUDIO_STOP_MODE mode);
FMOD_RESULT F_API FMOD_Studio_Bus_GetPortIndex(FMOD_STUDIO_BUS *bus, FMOD_PORT_INDEX *index);
FMOD_RESULT F_API FMOD_Studio_Bus_SetPortIndex(FMOD_STUDIO_BUS *bus, FMOD_PORT_INDEX index);
FMOD_RESULT F_API FMOD_Studio_Bus_LockChannelGroup(FMOD_STUDIO_BUS *bus);
FMOD_RESULT F_API FMOD_Studio_Bus_UnlockChannelGroup(FMOD_STUDIO_BUS *bus);
FMOD_RESULT F_API FMOD_Studio_Bus_GetChannelGroup(FMOD_STUDIO_BUS *bus, FMOD_CHANNELGROUP **group);
FMOD_RESULT F_API FMOD_Studio_Bus_GetCPUUsage(FMOD_STUDIO_BUS *bus, unsigned int *exclusive, unsigned int *inclusive);
FMOD_RESULT F_API FMOD_Studio_Bus_GetMemoryUsage(FMOD_STUDIO_BUS *bus, FMOD_STUDIO_MEMORY_USAGE *memoryusage);

/*
    VCA
*/
FMOD_BOOL   F_API FMOD_Studio_VCA_IsValid(FMOD_STUDIO_VCA *vca);
FMOD_RESULT F_API FMOD_Studio_VCA_GetID(FMOD_STUDIO_VCA *vca, FMOD_GUID *id);
FMOD_RESULT F_API FMOD_Studio_VCA_GetPath(FMOD_STUDIO_VCA *vca, char *path, int size, int *retrieved);
FMOD_RESULT F_API FMOD_Studio_VCA_GetVolume(FMOD_STUDIO_VCA *vca, float *volume, float *finalvolume);
FMOD_RESULT F_API FMOD_Studio_VCA_SetVolume(FMOD_STUDIO_VCA *vca, float volume);

/*
    Bank
*/
FMOD_BOOL   F_API FMOD_Studio_Bank_IsValid(FMOD_STUDIO_BANK *bank);
FMOD_RESULT F_API FMOD_Studio_Bank_GetID(FMOD_STUDIO_BANK *bank, FMOD_GUID *id);
FMOD_RESULT F_API FMOD_Studio_Bank_GetPath(FMOD_STUDIO_BANK *bank, char *path, int size, int *retrieved);
FMOD_RESULT F_API FMOD_Studio_Bank_Unload(FMOD_STUDIO_BANK *bank);
FMOD_RESULT F_API FMOD_Studio_Bank_LoadSampleData(FMOD_STUDIO_BANK *bank);
FMOD_RESULT F_API FMOD_Studio_Bank_UnloadSampleData(FMOD_STUDIO_BANK *bank);
FMOD_RESULT F_API FMOD_Studio_Bank_GetLoadingState(FMOD_STUDIO_BANK *bank, FMOD_STUDIO_LOADING_STATE *state);
FMOD_RESULT F_API FMOD_Studio_Bank_GetSampleLoadingState(FMOD_STUDIO_BANK *bank, FMOD_STUDIO_LOADING_STATE *state);
FMOD_RESULT F_API FMOD_Studio_Bank_GetStringCount(FMOD_STUDIO_BANK *bank, int *count);
FMOD_RESULT F_API FMOD_Studio_Bank_GetStringInfo(FMOD_STUDIO_BANK *bank, int index, FMOD_GUID *id, char *path, int size, int *retrieved);
FMOD_RESULT F_API FMOD_Studio_Bank_GetEventCount(FMOD_STUDIO_BANK *bank, int *count);
FMOD_RESULT F_API FMOD_Studio_Bank_GetEventList(FMOD_STUDIO_BANK *bank, FMOD_STUDIO_EVENTDESCRIPTION **array, int capacity, int *count);
FMOD_RESULT F_API FMOD_Studio_Bank_GetBusCount(FMOD_STUDIO_BANK *bank, int *count);
FMOD_RESULT F_API FMOD_Studio_Bank_GetBusList(FMOD_STUDIO_BANK *bank, FMOD_STUDIO_BUS **array, int capacity, int *count);
FMOD_RESULT F_API FMOD_Studio_Bank_GetVCACount(FMOD_STUDIO_BANK *bank, int *count);
FMOD_RESULT F_API FMOD_Studio_Bank_GetVCAList(FMOD_STUDIO_BANK *bank, FMOD_STUDIO_VCA **array, int capacity, int *count);
FMOD_RESULT F_API FMOD_Studio_Bank_GetUserData(FMOD_STUDIO_BANK *bank, void **userdata);
FMOD_RESULT F_API FMOD_Studio_Bank_SetUserData(FMOD_STUDIO_BANK *bank, void *userdata);

/*
    Command playback information
*/
FMOD_BOOL   F_API FMOD_Studio_CommandReplay_IsValid(FMOD_STUDIO_COMMANDREPLAY *replay);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_GetSystem(FMOD_STUDIO_COMMANDREPLAY *replay, FMOD_STUDIO_SYSTEM **system);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_GetLength(FMOD_STUDIO_COMMANDREPLAY *replay, float *length);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_GetCommandCount(FMOD_STUDIO_COMMANDREPLAY *replay, int *count);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_GetCommandInfo(FMOD_STUDIO_COMMANDREPLAY *replay, int commandindex, FMOD_STUDIO_COMMAND_INFO *info);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_GetCommandString(FMOD_STUDIO_COMMANDREPLAY *replay, int commandindex, char *buffer, int length);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_GetCommandAtTime(FMOD_STUDIO_COMMANDREPLAY *replay, float time, int *commandindex);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_SetBankPath(FMOD_STUDIO_COMMANDREPLAY *replay, const char *bankPath);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_Start(FMOD_STUDIO_COMMANDREPLAY *replay);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_Stop(FMOD_STUDIO_COMMANDREPLAY *replay);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_SeekToTime(FMOD_STUDIO_COMMANDREPLAY *replay, float time);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_SeekToCommand(FMOD_STUDIO_COMMANDREPLAY *replay, int commandindex);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_GetPaused(FMOD_STUDIO_COMMANDREPLAY *replay, FMOD_BOOL *paused);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_SetPaused(FMOD_STUDIO_COMMANDREPLAY *replay, FMOD_BOOL paused);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_GetPlaybackState(FMOD_STUDIO_COMMANDREPLAY *replay, FMOD_STUDIO_PLAYBACK_STATE *state);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_GetCurrentCommand(FMOD_STUDIO_COMMANDREPLAY *replay, int *commandindex, float *currenttime);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_Release(FMOD_STUDIO_COMMANDREPLAY *replay);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_SetFrameCallback(FMOD_STUDIO_COMMANDREPLAY *replay, FMOD_STUDIO_COMMANDREPLAY_FRAME_CALLBACK callback);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_SetLoadBankCallback(FMOD_STUDIO_COMMANDREPLAY *replay, FMOD_STUDIO_COMMANDREPLAY_LOAD_BANK_CALLBACK callback);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_SetCreateInstanceCallback(FMOD_STUDIO_COMMANDREPLAY *replay, FMOD_STUDIO_COMMANDREPLAY_CREATE_INSTANCE_CALLBACK callback);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_GetUserData(FMOD_STUDIO_COMMANDREPLAY *replay, void **userdata);
FMOD_RESULT F_API FMOD_Studio_CommandReplay_SetUserData(FMOD_STUDIO_COMMANDREPLAY *replay, void *userdata);

#ifdef __cplusplus
}
#endif

#endif /* FMOD_STUDIO_H */
