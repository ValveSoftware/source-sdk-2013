/* ======================================================================================== */
/* FMOD Studio API - C++ header file.                                                       */
/* Copyright (c), Firelight Technologies Pty, Ltd. 2004-2023.                               */
/*                                                                                          */
/* Use this header in conjunction with fmod_studio_common.h (which contains all the         */
/* constants / callbacks) to develop using the C++ language.                                */
/*                                                                                          */
/* For more detail visit:                                                                   */
/* https://fmod.com/docs/2.02/api/studio-api.html                                           */
/* ======================================================================================== */
#ifndef FMOD_STUDIO_HPP
#define FMOD_STUDIO_HPP

#include "fmod_studio_common.h"
#include "fmod_studio.h"

#include "fmod.hpp"

namespace FMOD
{

namespace Studio
{
    typedef FMOD_GUID ID; // Deprecated. Please use FMOD_GUID type.

    class System;
    class EventDescription;
    class EventInstance;
    class Bus;
    class VCA;
    class Bank;
    class CommandReplay;

    inline FMOD_RESULT parseID(const char *idstring, FMOD_GUID *id) { return FMOD_Studio_ParseID(idstring, id); }

    class System
    {
    private:
        // Constructor made private so user cannot statically instance a System class. System::create must be used.
        System();
        System(const System &);

    public:
        static FMOD_RESULT F_API create(System **system, unsigned int headerversion = FMOD_VERSION);
        FMOD_RESULT F_API setAdvancedSettings(FMOD_STUDIO_ADVANCEDSETTINGS *settings);
        FMOD_RESULT F_API getAdvancedSettings(FMOD_STUDIO_ADVANCEDSETTINGS *settings);
        FMOD_RESULT F_API initialize(int maxchannels, FMOD_STUDIO_INITFLAGS studioflags, FMOD_INITFLAGS flags, void *extradriverdata);
        FMOD_RESULT F_API release();

        // Handle validity
        bool F_API isValid() const;

        // Update processing
        FMOD_RESULT F_API update();
        FMOD_RESULT F_API flushCommands();
        FMOD_RESULT F_API flushSampleLoading();

        // Low-level API access
        FMOD_RESULT F_API getCoreSystem(FMOD::System **system) const;

        // Asset retrieval
        FMOD_RESULT F_API getEvent(const char *path, EventDescription **event) const;
        FMOD_RESULT F_API getBus(const char *path, Bus **bus) const;
        FMOD_RESULT F_API getVCA(const char *path, VCA **vca) const;
        FMOD_RESULT F_API getBank(const char *path, Bank **bank) const;
        FMOD_RESULT F_API getEventByID(const FMOD_GUID *id, EventDescription **event) const;
        FMOD_RESULT F_API getBusByID(const FMOD_GUID *id, Bus **bus) const;
        FMOD_RESULT F_API getVCAByID(const FMOD_GUID *id, VCA **vca) const;
        FMOD_RESULT F_API getBankByID(const FMOD_GUID *id, Bank **bank) const;
        FMOD_RESULT F_API getSoundInfo(const char *key, FMOD_STUDIO_SOUND_INFO *info) const;
        FMOD_RESULT F_API getParameterDescriptionByName(const char *name, FMOD_STUDIO_PARAMETER_DESCRIPTION *parameter) const;
        FMOD_RESULT F_API getParameterDescriptionByID(FMOD_STUDIO_PARAMETER_ID id, FMOD_STUDIO_PARAMETER_DESCRIPTION *parameter) const;
        FMOD_RESULT F_API getParameterLabelByName(const char *name, int labelindex, char *label, int size, int *retrieved) const;
        FMOD_RESULT F_API getParameterLabelByID(FMOD_STUDIO_PARAMETER_ID id, int labelindex, char *label, int size, int *retrieved) const;

        // Global parameter control
        FMOD_RESULT F_API getParameterByID(FMOD_STUDIO_PARAMETER_ID id, float *value, float *finalvalue = 0) const;
        FMOD_RESULT F_API setParameterByID(FMOD_STUDIO_PARAMETER_ID id, float value, bool ignoreseekspeed = false);
        FMOD_RESULT F_API setParameterByIDWithLabel(FMOD_STUDIO_PARAMETER_ID id, const char *label, bool ignoreseekspeed = false);
        FMOD_RESULT F_API setParametersByIDs(const FMOD_STUDIO_PARAMETER_ID *ids, float *values, int count, bool ignoreseekspeed = false);
        FMOD_RESULT F_API getParameterByName(const char *name, float *value, float *finalvalue = 0) const;
        FMOD_RESULT F_API setParameterByName(const char *name, float value, bool ignoreseekspeed = false);
        FMOD_RESULT F_API setParameterByNameWithLabel(const char *name, const char *label, bool ignoreseekspeed = false);

        // Path lookup
        FMOD_RESULT F_API lookupID(const char *path, FMOD_GUID *id) const;
        FMOD_RESULT F_API lookupPath(const FMOD_GUID *id, char *path, int size, int *retrieved) const;

        // Listener control
        FMOD_RESULT F_API getNumListeners(int *numlisteners);
        FMOD_RESULT F_API setNumListeners(int numlisteners);
        FMOD_RESULT F_API getListenerAttributes(int listener, FMOD_3D_ATTRIBUTES *attributes, FMOD_VECTOR *attenuationposition = 0) const;
        FMOD_RESULT F_API setListenerAttributes(int listener, const FMOD_3D_ATTRIBUTES *attributes, const FMOD_VECTOR *attenuationposition = 0);
        FMOD_RESULT F_API getListenerWeight(int listener, float *weight);
        FMOD_RESULT F_API setListenerWeight(int listener, float weight);

        // Bank control
        FMOD_RESULT F_API loadBankFile(const char *filename, FMOD_STUDIO_LOAD_BANK_FLAGS flags, Bank **bank);
        FMOD_RESULT F_API loadBankMemory(const char *buffer, int length, FMOD_STUDIO_LOAD_MEMORY_MODE mode, FMOD_STUDIO_LOAD_BANK_FLAGS flags, Bank **bank);
        FMOD_RESULT F_API loadBankCustom(const FMOD_STUDIO_BANK_INFO *info, FMOD_STUDIO_LOAD_BANK_FLAGS flags, Bank **bank);
        FMOD_RESULT F_API unloadAll();

        // General functionality
        FMOD_RESULT F_API getBufferUsage(FMOD_STUDIO_BUFFER_USAGE *usage) const;
        FMOD_RESULT F_API resetBufferUsage();
        FMOD_RESULT F_API registerPlugin(const FMOD_DSP_DESCRIPTION *description);
        FMOD_RESULT F_API unregisterPlugin(const char *name);

        // Enumeration
        FMOD_RESULT F_API getBankCount(int *count) const;
        FMOD_RESULT F_API getBankList(Bank **array, int capacity, int *count) const;
        FMOD_RESULT F_API getParameterDescriptionCount(int *count) const;
        FMOD_RESULT F_API getParameterDescriptionList(FMOD_STUDIO_PARAMETER_DESCRIPTION *array, int capacity, int *count) const;

        // Command capture and replay
        FMOD_RESULT F_API startCommandCapture(const char *filename, FMOD_STUDIO_COMMANDCAPTURE_FLAGS flags);
        FMOD_RESULT F_API stopCommandCapture();
        FMOD_RESULT F_API loadCommandReplay(const char *filename, FMOD_STUDIO_COMMANDREPLAY_FLAGS flags, CommandReplay **replay);

        // Callbacks
        FMOD_RESULT F_API setCallback(FMOD_STUDIO_SYSTEM_CALLBACK callback, FMOD_STUDIO_SYSTEM_CALLBACK_TYPE callbackmask = FMOD_STUDIO_SYSTEM_CALLBACK_ALL);
        FMOD_RESULT F_API getUserData(void **userdata) const;
        FMOD_RESULT F_API setUserData(void *userdata);

        // Monitoring
        FMOD_RESULT F_API getCPUUsage(FMOD_STUDIO_CPU_USAGE *usage, FMOD_CPU_USAGE *usage_core) const;
        FMOD_RESULT F_API getMemoryUsage(FMOD_STUDIO_MEMORY_USAGE *memoryusage) const;
    };

    class EventDescription
    {
    private:
        // Constructor made private so user cannot statically instance the class.
        EventDescription();
        EventDescription(const EventDescription &);

    public:
        // Handle validity
        bool F_API isValid() const;

        // Property access
        FMOD_RESULT F_API getID(FMOD_GUID *id) const;
        FMOD_RESULT F_API getPath(char *path, int size, int *retrieved) const;
        FMOD_RESULT F_API getParameterDescriptionCount(int *count) const;
        FMOD_RESULT F_API getParameterDescriptionByIndex(int index, FMOD_STUDIO_PARAMETER_DESCRIPTION *parameter) const;
        FMOD_RESULT F_API getParameterDescriptionByName(const char *name, FMOD_STUDIO_PARAMETER_DESCRIPTION *parameter) const;
        FMOD_RESULT F_API getParameterDescriptionByID(FMOD_STUDIO_PARAMETER_ID id, FMOD_STUDIO_PARAMETER_DESCRIPTION *parameter) const;
        FMOD_RESULT F_API getParameterLabelByIndex(int index, int labelindex, char *label, int size, int *retrieved) const;
        FMOD_RESULT F_API getParameterLabelByName(const char *name, int labelindex, char *label, int size, int *retrieved) const;
        FMOD_RESULT F_API getParameterLabelByID(FMOD_STUDIO_PARAMETER_ID id, int labelindex, char *label, int size, int *retrieved) const;
        FMOD_RESULT F_API getUserPropertyCount(int *count) const;
        FMOD_RESULT F_API getUserPropertyByIndex(int index, FMOD_STUDIO_USER_PROPERTY *property) const;
        FMOD_RESULT F_API getUserProperty(const char *name, FMOD_STUDIO_USER_PROPERTY *property) const;
        FMOD_RESULT F_API getLength(int *length) const;
        FMOD_RESULT F_API getMinMaxDistance(float *min, float *max) const;
        FMOD_RESULT F_API getSoundSize(float *size) const;

        FMOD_RESULT F_API isSnapshot(bool *snapshot) const;
        FMOD_RESULT F_API isOneshot(bool *oneshot) const;
        FMOD_RESULT F_API isStream(bool *isStream) const;
        FMOD_RESULT F_API is3D(bool *is3d) const;
        FMOD_RESULT F_API isDopplerEnabled(bool *doppler) const;
        FMOD_RESULT F_API hasSustainPoint(bool *sustainPoint) const;

        // Playback control
        FMOD_RESULT F_API createInstance(EventInstance **instance) const;
        FMOD_RESULT F_API getInstanceCount(int *count) const;
        FMOD_RESULT F_API getInstanceList(EventInstance **array, int capacity, int *count) const;

        // Sample data loading control
        FMOD_RESULT F_API loadSampleData();
        FMOD_RESULT F_API unloadSampleData();
        FMOD_RESULT F_API getSampleLoadingState(FMOD_STUDIO_LOADING_STATE *state) const;

        // Convenience functions
        FMOD_RESULT F_API releaseAllInstances();

        // Callbacks
        FMOD_RESULT F_API setCallback(FMOD_STUDIO_EVENT_CALLBACK callback, FMOD_STUDIO_EVENT_CALLBACK_TYPE callbackmask = FMOD_STUDIO_EVENT_CALLBACK_ALL);
        FMOD_RESULT F_API getUserData(void **userdata) const;
        FMOD_RESULT F_API setUserData(void *userdata);
    };

    class EventInstance
    {
    private:
        // Constructor made private so user cannot statically instance the class.
        EventInstance();
        EventInstance(const EventInstance &);

    public:
        // Handle validity
        bool F_API isValid() const;

        // Property access
        FMOD_RESULT F_API getDescription(EventDescription **description) const;

        // Playback control
        FMOD_RESULT F_API getVolume(float *volume, float *finalvolume = 0) const;
        FMOD_RESULT F_API setVolume(float volume);

        FMOD_RESULT F_API getPitch(float *pitch, float *finalpitch = 0) const;
        FMOD_RESULT F_API setPitch(float pitch);

        FMOD_RESULT F_API get3DAttributes(FMOD_3D_ATTRIBUTES *attributes) const;
        FMOD_RESULT F_API set3DAttributes(const FMOD_3D_ATTRIBUTES *attributes);

        FMOD_RESULT F_API getListenerMask(unsigned int *mask) const;
        FMOD_RESULT F_API setListenerMask(unsigned int mask);

        FMOD_RESULT F_API getProperty(FMOD_STUDIO_EVENT_PROPERTY index, float *value) const;
        FMOD_RESULT F_API setProperty(FMOD_STUDIO_EVENT_PROPERTY index, float value);

        FMOD_RESULT F_API getReverbLevel(int index, float *level) const;
        FMOD_RESULT F_API setReverbLevel(int index, float level);

        FMOD_RESULT F_API getPaused(bool *paused) const;
        FMOD_RESULT F_API setPaused(bool paused);

        FMOD_RESULT F_API start();
        FMOD_RESULT F_API stop(FMOD_STUDIO_STOP_MODE mode);

        FMOD_RESULT F_API getTimelinePosition(int *position) const;
        FMOD_RESULT F_API setTimelinePosition(int position);

        FMOD_RESULT F_API getPlaybackState(FMOD_STUDIO_PLAYBACK_STATE *state) const;

        FMOD_RESULT F_API getChannelGroup(ChannelGroup **group) const;

        FMOD_RESULT F_API getMinMaxDistance(float *min, float *max) const;

        FMOD_RESULT F_API release();

        FMOD_RESULT F_API isVirtual(bool *virtualstate) const;

        FMOD_RESULT F_API getParameterByID(FMOD_STUDIO_PARAMETER_ID id, float *value, float *finalvalue = 0) const;
        FMOD_RESULT F_API setParameterByID(FMOD_STUDIO_PARAMETER_ID id, float value, bool ignoreseekspeed = false);
        FMOD_RESULT F_API setParameterByIDWithLabel(FMOD_STUDIO_PARAMETER_ID id, const char* label, bool ignoreseekspeed = false);
        FMOD_RESULT F_API setParametersByIDs(const FMOD_STUDIO_PARAMETER_ID *ids, float *values, int count, bool ignoreseekspeed = false);

        FMOD_RESULT F_API getParameterByName(const char *name, float *value, float *finalvalue = 0) const;
        FMOD_RESULT F_API setParameterByName(const char *name, float value, bool ignoreseekspeed = false);
        FMOD_RESULT F_API setParameterByNameWithLabel(const char *name, const char* label, bool ignoreseekspeed = false);

        FMOD_RESULT F_API keyOff();

        // Monitoring
        FMOD_RESULT F_API getCPUUsage(unsigned int *exclusive, unsigned int *inclusive) const;
        FMOD_RESULT F_API getMemoryUsage(FMOD_STUDIO_MEMORY_USAGE *memoryusage) const;

        // Callbacks
        FMOD_RESULT F_API setCallback(FMOD_STUDIO_EVENT_CALLBACK callback, FMOD_STUDIO_EVENT_CALLBACK_TYPE callbackmask = FMOD_STUDIO_EVENT_CALLBACK_ALL);
        FMOD_RESULT F_API getUserData(void **userdata) const;
        FMOD_RESULT F_API setUserData(void *userdata);
    };

    class Bus
    {
    private:
        // Constructor made private so user cannot statically instance the class.
        Bus();
        Bus(const Bus &);

    public:
        // Handle validity
        bool F_API isValid() const;

        // Property access
        FMOD_RESULT F_API getID(FMOD_GUID *id) const;
        FMOD_RESULT F_API getPath(char *path, int size, int *retrieved) const;

        // Playback control
        FMOD_RESULT F_API getVolume(float *volume, float *finalvolume = 0) const;
        FMOD_RESULT F_API setVolume(float volume);

        FMOD_RESULT F_API getPaused(bool *paused) const;
        FMOD_RESULT F_API setPaused(bool paused);

        FMOD_RESULT F_API getMute(bool *mute) const;
        FMOD_RESULT F_API setMute(bool mute);

        FMOD_RESULT F_API stopAllEvents(FMOD_STUDIO_STOP_MODE mode);

        // Output port
        FMOD_RESULT F_API getPortIndex(FMOD_PORT_INDEX *index) const;
        FMOD_RESULT F_API setPortIndex(FMOD_PORT_INDEX index);

        // Low-level API access
        FMOD_RESULT F_API lockChannelGroup();
        FMOD_RESULT F_API unlockChannelGroup();
        FMOD_RESULT F_API getChannelGroup(FMOD::ChannelGroup **group) const;

        // Monitoring
        FMOD_RESULT F_API getCPUUsage(unsigned int *exclusive, unsigned int *inclusive) const;
        FMOD_RESULT F_API getMemoryUsage(FMOD_STUDIO_MEMORY_USAGE *memoryusage) const;
    };

    class VCA
    {
    private:
        // Constructor made private so user cannot statically instance the class.
        VCA();
        VCA(const VCA &);

    public:
        // Handle validity
        bool F_API isValid() const;

        // Property access
        FMOD_RESULT F_API getID(FMOD_GUID *id) const;
        FMOD_RESULT F_API getPath(char *path, int size, int *retrieved) const;

        // Playback control
        FMOD_RESULT F_API getVolume(float *volume, float *finalvolume = 0) const;
        FMOD_RESULT F_API setVolume(float volume);
    };

    class Bank
    {
    private:
        // Constructor made private so user cannot statically instance the class.
        Bank();
        Bank(const Bank &);

    public:
        // Handle validity
        bool F_API isValid() const;

        // Property access
        FMOD_RESULT F_API getID(FMOD_GUID *id) const;
        FMOD_RESULT F_API getPath(char *path, int size, int *retrieved) const;

        // Loading control
        FMOD_RESULT F_API unload();
        FMOD_RESULT F_API loadSampleData();
        FMOD_RESULT F_API unloadSampleData();

        FMOD_RESULT F_API getLoadingState(FMOD_STUDIO_LOADING_STATE *state) const;
        FMOD_RESULT F_API getSampleLoadingState(FMOD_STUDIO_LOADING_STATE *state) const;

        // Enumeration
        FMOD_RESULT F_API getStringCount(int *count) const;
        FMOD_RESULT F_API getStringInfo(int index, FMOD_GUID *id, char *path, int size, int *retrieved) const;
        FMOD_RESULT F_API getEventCount(int *count) const;
        FMOD_RESULT F_API getEventList(EventDescription **array, int capacity, int *count) const;
        FMOD_RESULT F_API getBusCount(int *count) const;
        FMOD_RESULT F_API getBusList(Bus **array, int capacity, int *count) const;
        FMOD_RESULT F_API getVCACount(int *count) const;
        FMOD_RESULT F_API getVCAList(VCA **array, int capacity, int *count) const;

        FMOD_RESULT F_API getUserData(void **userdata) const;
        FMOD_RESULT F_API setUserData(void *userdata);
    };

    class CommandReplay
    {
    private:
        // Constructor made private so user cannot statically instance the class.
        CommandReplay();
        CommandReplay(const CommandReplay &);

    public:
        // Handle validity
        bool F_API isValid() const;

        // Information query
        FMOD_RESULT F_API getSystem(System **system) const;
        FMOD_RESULT F_API getLength(float *length) const;

        FMOD_RESULT F_API getCommandCount(int *count) const;
        FMOD_RESULT F_API getCommandInfo(int commandindex, FMOD_STUDIO_COMMAND_INFO *info) const;
        FMOD_RESULT F_API getCommandString(int commandindex, char *buffer, int length) const;
        FMOD_RESULT F_API getCommandAtTime(float time, int *commandindex) const;

        // Playback
        FMOD_RESULT F_API setBankPath(const char *bankPath);
        FMOD_RESULT F_API start();
        FMOD_RESULT F_API stop();
        FMOD_RESULT F_API seekToTime(float time);
        FMOD_RESULT F_API seekToCommand(int commandindex);
        FMOD_RESULT F_API getPaused(bool *paused) const;
        FMOD_RESULT F_API setPaused(bool paused);
        FMOD_RESULT F_API getPlaybackState(FMOD_STUDIO_PLAYBACK_STATE *state) const;
        FMOD_RESULT F_API getCurrentCommand(int *commandindex, float *currenttime) const;

        // Release
        FMOD_RESULT F_API release();

        // Callbacks
        FMOD_RESULT F_API setFrameCallback(FMOD_STUDIO_COMMANDREPLAY_FRAME_CALLBACK callback);
        FMOD_RESULT F_API setLoadBankCallback(FMOD_STUDIO_COMMANDREPLAY_LOAD_BANK_CALLBACK callback);
        FMOD_RESULT F_API setCreateInstanceCallback(FMOD_STUDIO_COMMANDREPLAY_CREATE_INSTANCE_CALLBACK callback);

        FMOD_RESULT F_API getUserData(void **userdata) const;
        FMOD_RESULT F_API setUserData(void *userdata);
    };

} // namespace Studio

} // namespace FMOD

#endif //FMOD_STUDIO_HPP
