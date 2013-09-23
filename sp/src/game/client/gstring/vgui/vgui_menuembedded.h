#ifndef VGUI_MENU_EMBEDDED_H
#define VGUI_MENU_EMBEDDED_H

#include "cbase.h"

#include "vgui_controls/controls.h"
#include "vgui_controls/panel.h"
#include "vgui/IVGUI.h"

class CVGUIMenuLayer;
class CVGUIMenuButton;
class CVGUIMenuEmbedded;
class CViewSetup;

class CVGUIMenuEmbedded : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CVGUIMenuEmbedded, vgui::Panel );
public:

	CVGUIMenuEmbedded( Panel *parent );
	~CVGUIMenuEmbedded();

	void Render3D();
	void SetViewport( Panel *vp );

	virtual void OnThink();

	void SetActive( bool bActive );
	bool IsActive();

	vgui::HContext &GetContext();

	void SetViewXForms( Vector origin, QAngle angles, float fov );
	CViewSetup CalcView();

protected:

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual void PerformLayout();

	virtual void OnCommand( const char *cmd );

	virtual void Paint();

	void UpdateCursor( const CViewSetup &setup );

	MESSAGE_FUNC_PARAMS( OnButtonArmed, "OnButtonArmed", pKV );

private:

	vgui::HContext m_hContext;

	int m_iTexture_BG;
	int m_iTexture_Line;

	bool m_bActive;

	CUtlVector< CVGUIMenuButton* > m_hButtons_General;
	CUtlVector< CVGUIMenuButton* > m_hButtons_InGame;

	CVGUIParticleContainer *m_pParticleParent;

	vgui::Label *m_pTitle_0;
	vgui::Label *m_pTitle_1;

	int _line_offset;
	int _line_size_y;

	Vector viewOrigin;
	QAngle viewAngles;
	float viewFOV;
	Panel *m_pViewport;
};


class CVGUIMenu3DWrapper : public vgui::Panel
{
		DECLARE_CLASS_SIMPLE( CVGUIMenu3DWrapper, vgui::Panel );
public:

	CVGUIMenu3DWrapper( CVGUIMenuEmbedded *p3DPanel, vgui::Panel *parent );

	CVGUIMenuEmbedded *GetWrappedPanel();

protected:

	virtual void Paint();

	virtual void OnCursorEntered();
	virtual void OnCursorExited();

	virtual void OnMousePressed( vgui::MouseCode code );
	virtual void OnMouseDoublePressed( vgui::MouseCode code );
	virtual void OnMouseReleased( vgui::MouseCode code );

private:

	CVGUIMenuEmbedded *m_p3DPanel;
};


#endif