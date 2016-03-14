#include "cbase.h"
#include "mom_util.h"
#include "filesystem.h"

#include "tier0/memdbgon.h"

extern IFileSystem* filesystem;

#ifdef GAME_DLL
void MomentumUtil::DownloadCallback(HTTPRequestCompleted_t *pCallback, bool bIOFailure)
{
    if (bIOFailure) return;

    FileHandle_t file;
    //MOM_TODO: Read the MOM_TODO DownloadMap(), we're going to need to save the zone files too
    file = filesystem->Open("testmapdownload.bsp", "w+b", "MOD");
    uint32 size;
    steamapicontext->SteamHTTP()->GetHTTPResponseBodySize(pCallback->m_hRequest, &size);
    DevLog("Size of body: %u\n", size);
    uint8 *pData = new uint8[size];
    steamapicontext->SteamHTTP()->GetHTTPResponseBodyData(pCallback->m_hRequest, pData, size);
    //write the file
    filesystem->Write(pData, size, file);
    //save the file
    filesystem->Close(file);
    DevLog("Successfully written file\n");

    //Free resources
    steamapicontext->SteamHTTP()->ReleaseHTTPRequest(pCallback->m_hRequest);
}

void MomentumUtil::PostTimeCallback(HTTPRequestCompleted_t *pCallback, bool bIOFailure)
{
    if (bIOFailure) return;
    uint32 size;
    steamapicontext->SteamHTTP()->GetHTTPResponseBodySize(pCallback->m_hRequest, &size);
    DevLog("Size of body: %u\n", size);
    uint8 *pData = new uint8[size];
    steamapicontext->SteamHTTP()->GetHTTPResponseBodyData(pCallback->m_hRequest, pData, size);

    JsonValue val;//Outer object
    JsonAllocator alloc;
    char* pDataPtr = reinterpret_cast<char*>(pData);
    DevLog("pDataPtr: %s\n", pDataPtr);
    char *endPtr;
    int status = jsonParse(pDataPtr, &endPtr, &val, alloc);

    if (status == JSON_OK)
    {
        DevLog("JSON Parsed!\n");
        if (val.getTag() == JSON_OBJECT)//Outer should be a JSON Object
        {
            //toNode() returns the >>payload<< of the JSON object !!!

            DevLog("Outer is JSON OBJECT!\n");
            JsonNode *node = val.toNode();
            DevLog("Outer has key %s with value %s\n", node->key, node->value.toString());

            if (node && node->value.getTag() == JSON_TRUE)
            {
                DevLog("RESPONSE WAS TRUE!\n");
                // Necesary so TimeDisplay scoreboard knows it has to update;
                IGameEvent *postEvent = gameeventmanager->CreateEvent("runtime_posted");
                if (postEvent)
                    gameeventmanager->FireEvent(postEvent);

                //MOM_TODO: Once the server updates this to contain more info, parse and do more with the response
            }
        }
    }
    else
    {
        Warning("%s at %zd\n", jsonStrError(status), endPtr - pDataPtr);
    }
    //Last but not least, free resources
    alloc.deallocate();
    steamapicontext->SteamHTTP()->ReleaseHTTPRequest(pCallback->m_hRequest);
}

void MomentumUtil::PostTime(const char* szURL)
{
    CreateAndSendHTTPReq(szURL, &cbPostTimeCallback, &MomentumUtil::PostTimeCallback);
}

void MomentumUtil::DownloadMap(const char* szMapname)
{
    if (!steamapicontext->SteamHTTP())
    {
        Warning("Failed to download map, cannot access HTTP!\n");
        return;
    }
    //MOM_TODO: 
    //This should only be called if the user has the outdated map version or
    //doesn't have the map at all

    //The two different URLs:
    //cdn.momentum-mod.org/maps/MAPNAME/MAPNAME.bsp
    //and
    //cdn.momentum-mod.org/maps/MAPNAME/MAPNAME.zon
    //We're going to need to build requests for and download both of these files

    //Uncomment the following when we build the URLS (MOM_TODO)
    //CreateAndSendHTTPReq(mapfileURL, &cbDownloadCallback, &MomentumUtil::DownloadCallback);
    //CreateAndSendHTTPReq(zonFileURL, &cbDownloadCallback, &MomentumUtil::DownloadCallback);
}


void MomentumUtil::CreateAndSendHTTPReq(const char* szURL, CCallResult<MomentumUtil, HTTPRequestCompleted_t>* callback,
    CCallResult<MomentumUtil, HTTPRequestCompleted_t>::func_t func)
{
    HTTPRequestHandle handle = steamapicontext->SteamHTTP()->CreateHTTPRequest(k_EHTTPMethodGET, szURL);
    SteamAPICall_t apiHandle;
    if (steamapicontext->SteamHTTP()->SendHTTPRequest(handle, &apiHandle))
    {
        callback->Set(apiHandle, this, func);
    }
    else
    {
        Warning("Failed to send HTTP Request to post scores online!\n");
        steamapicontext->SteamHTTP()->ReleaseHTTPRequest(handle);//GC
    }
}

#endif

void MomentumUtil::FormatTime(float ticks, float rate, char *pOut)
{
    float m_flSecondsTime = ticks * rate;

    int hours = m_flSecondsTime / (60.0f * 60.0f);
    int minutes = fmod(m_flSecondsTime / 60.0f, 60.0f);
    int seconds = fmod(m_flSecondsTime, 60.0f);
    int millis = fmod(m_flSecondsTime, 1.0f) * 1000.0f;

    Q_snprintf(pOut, 15, "%02d:%02d:%02d.%03d",
        hours,
        minutes,
        seconds,
        millis
        );
}

MomentumUtil mom_UTIL;