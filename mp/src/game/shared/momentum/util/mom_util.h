#ifndef MOM_UTIL_H
#define MOM_UTIL_H
#ifdef _WIN32
#pragma once
#endif

#include "filesystem.h"
#include "gason.h"

class MomentumUtil
{
public:

#ifdef GAME_DLL
    void PostTimeCallback(HTTPRequestCompleted_t*, bool);
    void DownloadCallback(HTTPRequestCompleted_t*, bool);
    void PostTime(const char* URL);
    void DownloadMap(const char*);

    void CreateAndSendHTTPReq(const char*, CCallResult<MomentumUtil, HTTPRequestCompleted_t>*,
        CCallResult<MomentumUtil, HTTPRequestCompleted_t>::func_t);

    CCallResult<MomentumUtil, HTTPRequestCompleted_t> cbDownloadCallback;
    CCallResult<MomentumUtil, HTTPRequestCompleted_t> cbPostTimeCallback;
#endif

    //Formats time in ticks by a given tickrate into
    //"HH:MM:SS.mmmm"
    void FormatTime(float ticks, float rate, char *pOut);

    
};

extern MomentumUtil mom_UTIL;

#endif //MOM_UTIL_H