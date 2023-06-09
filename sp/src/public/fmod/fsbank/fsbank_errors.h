#ifndef _FSBANK_ERRORS_H
#define _FSBANK_ERRORS_H

#include "fsbank.h"

static const char *FSBank_ErrorString(FSBANK_RESULT result)
{
    switch (result)
    {
        case FSBANK_OK:                                 return "No errors.";
        case FSBANK_ERR_CACHE_CHUNKNOTFOUND:            return "An expected chunk is missing from the cache, perhaps try deleting cache files.";
        case FSBANK_ERR_CANCELLED:                      return "The build process was cancelled during compilation by the user.";
        case FSBANK_ERR_CANNOT_CONTINUE:                return "The build process cannot continue due to previously ignored errors.";
        case FSBANK_ERR_ENCODER:                        return "Encoder for chosen format has encountered an unexpected error.";
        case FSBANK_ERR_ENCODER_INIT:                   return "Encoder initialization failed.";
        case FSBANK_ERR_ENCODER_NOTSUPPORTED:           return "Encoder for chosen format is not supported on this platform.";
        case FSBANK_ERR_FILE_OS:                        return "An operating system based file error was encountered.";
        case FSBANK_ERR_FILE_NOTFOUND:                  return "A specified file could not be found.";
        case FSBANK_ERR_FMOD:                           return "Internal error from FMOD sub-system.";
        case FSBANK_ERR_INITIALIZED:                    return "Already initialized.";
        case FSBANK_ERR_INVALID_FORMAT:                 return "The format of the source file is invalid.";
        case FSBANK_ERR_INVALID_PARAM:                  return "An invalid parameter has been passed to this function.";
        case FSBANK_ERR_MEMORY:                         return "Run out of memory.";
        case FSBANK_ERR_UNINITIALIZED:                  return "Not initialized yet.";
        case FSBANK_ERR_WRITER_FORMAT:                  return "Chosen encode format is not supported by this FSB version.";
        case FSBANK_WARN_CANNOTLOOP:                    return "Source file is too short for seamless looping. Looping disabled.";
        case FSBANK_WARN_IGNORED_FILTERHIGHFREQ:        return "FSBANK_BUILD_FILTERHIGHFREQ flag ignored: feature only supported by XMA format.";
        case FSBANK_WARN_IGNORED_DISABLESEEKING:        return "FSBANK_BUILD_DISABLESEEKING flag ignored: feature only supported by XMA format.";
        case FSBANK_WARN_FORCED_DONTWRITENAMES:         return "FSBANK_BUILD_FSB5_DONTWRITENAMES flag forced: cannot write names when source is from memory.";
        case FSBANK_ERR_ENCODER_FILE_NOTFOUND:          return "External encoder dynamic library not found.";
        case FSBANK_ERR_ENCODER_FILE_BAD:               return "External encoder dynamic library could not be loaded, possibly incorrect binary format, incorrect architecture, or file corruption.";
        default:                                        return "Unknown error.";
    }
}

#endif // _FSBANK_ERRORS_H
