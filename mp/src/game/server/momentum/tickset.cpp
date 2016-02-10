#ifdef _WIN32
#include "Windows.h"
#include "Psapi.h"
#pragma comment(lib, "psapi.lib")
#elif defined (__linux__)
#elif defined (__APPLE__)
#endif

#include "tickset.h"
#include "tier0/platform.h"

float* TickSet::interval_per_tick = NULL;
const TickSet::Tickrate TickSet::s_DefinedRates[] = {
    { 0.015f, "66" },
    { 0.01f, "100" }
};
TickSet::Tickrate TickSet::m_trCurrent = TickSet::s_DefinedRates[TickSet::TICKRATE_66];

#ifdef __linux__
int GetModuleInformation_Linux(const char *name, void **base, size_t *length)
{
    // this is the only way to do this on linux, lol
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f)
        return 1;

    char buf[PATH_MAX+100];
    while (!feof(f))
    {
        if (!fgets(buf, sizeof(buf), f))
            break;

	char *tmp = strrchr(buf, '\n');
	if (tmp)
            *tmp = '\0';

	char *mapname = strchr(buf, '/');
	if (!mapname)
	    continue;

        char perm[5];
        unsigned long begin, end;
        sscanf(buf, "%lx-%lx %4s", &begin, &end, perm);

        if (strcmp(basename(mapname), name) == 0 && perm[0] == 'r' && perm[2] == 'x')
        {
            *base = (void*)begin;
            *length = (size_t)end-begin;
            fclose(f);
            return 0;
        }
    }

    fclose(f);
    return 2;
}
#endif // __linux__

inline bool TickSet::DataCompare(const unsigned char* data, const unsigned char* pattern, const char* mask)
{
    for (; *mask != 0; ++data, ++pattern, ++mask)
        if (*mask == 'x' && *data != *pattern)
            return false;

    return (*mask == 0);
}

void* TickSet::FindPattern(const void* start, size_t length, const unsigned char* pattern, const char* mask)
{
    auto maskLength = strlen(mask);
    for (size_t i = 0; i <= length - maskLength; ++i)
    {
        auto addr = reinterpret_cast<const unsigned char*>(start)+i;
        if (DataCompare(addr, pattern, mask))
            return const_cast<void*>(reinterpret_cast<const void*>(addr));
    }

    return NULL;
}

bool TickSet::TickInit()
{
#ifdef _WIN32
    HMODULE handle = GetModuleHandleA("engine.dll");
    if (!handle)
        return false;    
    
    MODULEINFO info;
    GetModuleInformation(GetCurrentProcess(), handle, &info, sizeof(info));

    auto moduleBase = info.lpBaseOfDll;
    auto moduleSize = info.SizeOfImage;

    unsigned char pattern[] = { 0x8B, 0x0D, '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', 0xFF, '?', 0xD9, 0x15, '?', '?', '?', '?', 0xDD, 0x05, '?', '?', '?', '?', 0xDB, 0xF1, 0xDD, 0x05, '?', '?', '?', '?', 0x77, 0x08, 0xD9, 0xCA, 0xDB, 0xF2, 0x76, 0x1F, 0xD9, 0xCA };
    auto p = reinterpret_cast<uintptr_t>(FindPattern(moduleBase, moduleSize, pattern, "xx????????????x?xx????xx????xxxx????xxxxxxxxxx"));
    if (p)
        interval_per_tick = *reinterpret_cast<float**>(p + 18);

#elif defined (__linux__)
    void *base;
    size_t length;
    if (GetModuleInformation_Linux("engine.so", &base, &length))
	return false;
    
    // mov ds:interval_per_tick, 3C75C28Fh         <-- float for 0.015
    unsigned char pattern[] = { 0xC7,0x05, '?','?','?','?', 0x8F,0xC2,0x75,0x3C, 0xE8 };
    void* addr = FindPattern(base, length, pattern, "xx????xxxxx");
    if (addr)
        interval_per_tick = *(float**)(addr + 2);

#elif defined (__APPLE__)

#endif

    return (interval_per_tick ? true : false);
}

bool TickSet::SetTickrate(float tickrate)
{
    /*if (interval_per_tick)
    {
        *interval_per_tick = tickrate;
        gpGlobals->interval_per_tick = tickrate;
        
        return true;
    }
    else return false;*/
    if (m_trCurrent.fTickRate != tickrate)
    {
        Tickrate tr;
        if (tickrate == 0.01f) tr = s_DefinedRates[TICKRATE_100];
        else if (tickrate == 0.015f) tr = s_DefinedRates[TICKRATE_66];
        else
        {
            tr.fTickRate = tickrate;
            tr.sType = "CUSTOM";
        }
        return SetTickrate(tr);
    }
    else return false;
}
