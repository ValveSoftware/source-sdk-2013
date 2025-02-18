//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Generic in-game abuse reporting
//
// $NoKeywords: $
//=============================================================================//

#ifndef ABUSE_REPORT_UI_H
#define ABUSE_REPORT_UI_H
#ifdef _WIN32
#pragma once
#endif

#include "abuse_report.h"
#include <vgui_controls/EditablePanel.h>

class CAvatarImagePanel;
class CCustomTextureImagePanel;
class CAbuseReportScreenShotPanel;

class CAbuseReportDlg : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CAbuseReportDlg, vgui::EditablePanel );

public:
	CAbuseReportDlg( vgui::Panel *parent, AbuseIncidentData_t *pIncidentData );
	~CAbuseReportDlg();

	virtual void OnCommand(const char *command);
	virtual void Close();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();

	virtual void MakeModal();

	bool IsAccusingGameServer();
	EAbuseReportContentType GetAbuseContentType();
	EAbuseReportType GetAbuseType();
	int GetAccusedPlayerIndex();
	const AbuseIncidentData_t::PlayerData_t *GetAccusedPlayerPtr();
	int GetUserImageIndex();
	int GetSelectedCustomImage();
	CUtlString GetAbuseDescription();
	bool GetAttachScreenShot();

protected:

	MESSAGE_FUNC_PTR( OnRadioButtonChecked, "RadioButtonChecked", panel );

	virtual const char *GetResFilename();

	vgui::Button *m_pSubmitButton;

	vgui::Button *m_pScreenShot;
	vgui::CheckButton *m_pScreenShotAttachCheckButton;

	vgui::Button *m_pCustomTextureNextButton;
	vgui::Button *m_pCustomTexturePrevButton;
	vgui::Button *m_pOffensiveImage;

	vgui::TextEntry		*m_pDescriptionTextEntry;
	vgui::Panel			*m_pPlayerLabel;
	vgui::RadioButton	*m_pPlayerRadio;
	vgui::RadioButton	*m_pGameServerRadio;
	vgui::ComboBox		*m_pPlayerCombo;
	vgui::Panel			*m_pAbuseContentLabel;
	vgui::ComboBox		*m_pAbuseContentCombo;
	vgui::Panel			*m_pAbuseTypeLabel;
	vgui::ComboBox		*m_pAbuseTypeCombo;

	CAbuseReportScreenShotPanel	*m_pScreenShotBitmap;

	CAvatarImagePanel	*m_pAvatarImage;
	vgui::Panel		*m_pNoAvatarLabel;

	CCustomTextureImagePanel *m_pCustomTextureImagePanel;
	vgui::Panel		*m_pNoCustomTexturesLabel;

	AbuseIncidentData_t *m_pIncidentData;

	int m_iUserImageIndex;

	MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel ); // send by combo box when it changes

	void PopulatePlayerList();
	void UpdateSubmitButton();
	void SetIsAccusingGameServer( bool bAccuseGameServer );
	void PlayerChanged();
	void ContentTypeChanged();
	void UpdateAvatarImage();
	void UpdateCustomTextures();

	virtual void OnSubmitReport();
};

/// Global pointer to the submission dialiog.
/// NULL if it's not displayed
extern vgui::DHANDLE<CAbuseReportDlg> g_AbuseReportDlg;

#endif	// ABUSE_REPORT_UI_H
