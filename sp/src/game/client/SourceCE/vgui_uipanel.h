
#include "cbase.h"

class IUIPanelInterface
{
public:
	virtual void Create(vgui::VPANEL parent) = 0;
	virtual void Destroy() = 0;
};

extern IUIPanelInterface* uipanel;
