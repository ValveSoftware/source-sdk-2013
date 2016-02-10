#ifndef IMAPSELECTOR_H
#define IMAPSELECTOR_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui/IVGui.h"

class IMapSelector
{
public:
    virtual void Create(vgui::VPANEL) = 0;
    virtual void Destroy() = 0;
    virtual void Activate() = 0;
    virtual void Deactivate() = 0;
};

extern IMapSelector* mapselector;

#endif