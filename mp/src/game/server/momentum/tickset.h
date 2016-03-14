#ifndef TICKSET_H
#define TICKSET_H

#include "cbase.h"
#include "momentum/mom_shareddefs.h"

class TickSet {
public:
    
    struct Tickrate
    {
        float fTickRate;
        const char* sType;

        Tickrate()
        {
            fTickRate = 0.0f;
            sType = '\0';
        }
        Tickrate(float f, const char * type)
        {
            fTickRate = f;
            sType = type;
        }
        Tickrate& operator =(const Tickrate &other)
        {
            this->fTickRate = other.fTickRate;
            this->sType = other.sType;
            return *this;
        }
        bool operator ==(const Tickrate &other)
        {
            return (other.fTickRate == fTickRate
                && !Q_strcmp(other.sType, sType));
        }
    };

    static const Tickrate s_DefinedRates[];

    enum
    {
        TICKRATE_66 = 0,
        TICKRATE_100 = 1    
    };

    static bool SetTickrate(int gameMode)
    {
        switch (gameMode)
        {
        case MOMGM_BHOP:
        case MOMGM_SCROLL:
            //MOM_TODO: add more gamemodes
            return SetTickrate(s_DefinedRates[TICKRATE_100]);

        case MOMGM_SURF:
        default:
            return SetTickrate(s_DefinedRates[TICKRATE_66]);
        }
        return false;
    }

    static Tickrate GetCurrentTickrate() { return (m_trCurrent.fTickRate > 0.0f ? m_trCurrent : s_DefinedRates[TICKRATE_66]); }
	static bool TickInit();
    static bool SetTickrate(Tickrate trNew)
    {
        if (trNew == m_trCurrent) return false;

        if (interval_per_tick)
        {
            DevLog("Testing: %f\n", trNew.fTickRate);
            *interval_per_tick = trNew.fTickRate;
            gpGlobals->interval_per_tick = *interval_per_tick;
            DevLog("Should have set the tickrate to %f\n", *interval_per_tick);
            m_trCurrent = trNew;
            return true;
        }
        return false;
    }
	static bool SetTickrate(float);
    static float GetTickrate() { return *interval_per_tick; }

private:
	static inline bool DataCompare(const unsigned char*, const unsigned char*, const char*);
	static void *FindPattern(const void*, size_t, const unsigned char*, const char*);
	static float *interval_per_tick;
    static Tickrate m_trCurrent;
};

#endif // TICKSET_H
