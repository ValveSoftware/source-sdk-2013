#ifndef AUDIO_OUTPUT_PANEL_H
#define AUDIO_OUTPUT_PANEL_H
#ifdef _WIN64
#include "vgui_controls/EditablePanel.h"
#include "AudioOutputSelection.h"
namespace vgui { class Label; class Button; }

class CAudioOutputPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CAudioOutputPanel, vgui::EditablePanel );
public:
	CAudioOutputPanel( vgui::Panel *parent, ICvar *cvar, IVEngineClient *engine );
	bool Apply();
protected:
	virtual void OnCommand( const char *command );
	virtual void OnThink();
	virtual void PerformLayout();
private:
	void Reset();
	CAudioOutputSelection m_Selection;
	vgui::ComboBox *m_pDevices;
	vgui::Label *m_pLabel;
	vgui::Label *m_pStatus;
	vgui::Button *m_pRefresh;
	bool m_bReady;
};
#endif
#endif
