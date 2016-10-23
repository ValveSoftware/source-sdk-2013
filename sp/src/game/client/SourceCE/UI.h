#ifndef _UI_H
#define _UI_H
#pragma once

#include "cbase.h"
#include <tinyxml2/tinyxml2.h>
#include <bitmap/bitmap.h>
#include "igamesystem.h"

using namespace tinyxml2;

void UIMsg(const char* msg, ...);
void UIDevMsg(const char* msg, ...);
void UIError(const char* msg, ...);

// Each UI panel is basically an HTML/XML document.
class UIPanel
{
	friend class CycloramaUI;
protected:
	const char* name;
	ushort width, height;
	XMLElement* root;
public:
	bool enabled = true;

	UIPanel(const char* name) : name(name) {};

	XMLElement* RootElement() { return root; };

	// Width in pixels
	ushort Width()	{ return width; };
	// Height in pixels
	ushort Height()	{ return height; };

	
};


// Panel rendered in 2D space on the monitor
class UIScreenPanel : public UIPanel
{
	friend class CycloramaUI;
protected:
	ushort x, y;
public:
	UIScreenPanel(const char* name) : UIPanel(name) {}
	// X pos on screen
	ushort X() { return x; }
	// Y pos on screen
	ushort Y() { return y; }
};

class CycloramaUI : public IGameSystemPerFrame
{
private:
	CUtlVector<UIPanel*> panels;
public:

	virtual char const *Name() { return "cyclorama"; }

	virtual bool Init();
	//virtual void PostInit();

	virtual void Shutdown();

	// Panel management
	virtual CUtlVector<UIPanel*>* GetPanels() { return &panels; }
	virtual int AddPanel(UIPanel* panel) { return panels.AddToTail(panel); }
	virtual int FindPanel(const char* name)
	{
		for (int i = 0; i < panels.Count(); i++) {
			if (panels[i]->name == name) {
				return i;
			}
		}
		return -1;
	}
	virtual UIPanel* GetPanel(int id) { return panels[id]; }
	virtual void RemovePanel(int panelNum) { panels.Remove(panelNum); }
	
	

	// Level init, shutdown
	//virtual void LevelInitPreEntity() {}
	//virtual void LevelInitPostEntity() {}
	//virtual void LevelShutdownPreClearSteamAPIContext() {}
	//virtual void LevelShutdownPreEntity() {}
	//virtual void LevelShutdownPostEntity() {}

	//virtual void OnSave() {}
	//virtual void OnRestore() {}
	//virtual void SafeRemoveIfDesired() {}

	virtual bool IsPerFrame() { return true; }
	// Called before rendering
	//virtual void PreRender() { }

	// Gets called each frame
	//virtual void Update(float frametime) { }

	// Called after rendering
	//virtual void PostRender() { }
};

#endif 