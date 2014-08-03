//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef SEQUENCEPICKER_H
#define SEQUENCEPICKER_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlstring.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/ImageList.h"
#include "datacache/imdlcache.h"
#include "matsys_controls/mdlpanel.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
namespace vgui
{
	class Splitter;
}


//-----------------------------------------------------------------------------
// Purpose: Sequence picker panel
//-----------------------------------------------------------------------------
class CSequencePicker : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CSequencePicker, vgui::EditablePanel );

public:
	enum PickType_t
	{
		PICK_NONE		= 0,
		PICK_SEQUENCES	= 0x1,
		PICK_ACTIVITIES = 0x2,
		PICK_ALL		= 0xFFFFFFFF,
	};

	// Flags come from PickType_t
	CSequencePicker( vgui::Panel *pParent, int nFlags = PICK_ALL );
	~CSequencePicker();

	// overridden frame functions
	virtual void PerformLayout();

	// Sets the MDL to preview sequences for
	void SetMDL( const char *pMDLName );

	// Gets the selected activity/sequence
	PickType_t GetSelectedSequenceType();
	const char *GetSelectedSequenceName( );

private:
	void RefreshActivitiesAndSequencesList();

	// Plays the selected activity
	void PlayActivity( const char *pActivityName );

	// Plays the selected sequence
	void PlaySequence( const char *pSequenceName );

	MESSAGE_FUNC_PARAMS( OnItemSelected, "ItemSelected", kv );
	MESSAGE_FUNC( OnPageChanged, "PageChanged" );

	CMDLPanel *m_pMDLPreview;
	vgui::Splitter* m_pPreviewSplitter;
	vgui::PropertySheet *m_pViewsSheet;
	vgui::PropertyPage *m_pSequencesPage;
	vgui::PropertyPage *m_pActivitiesPage;
	vgui::ListPanel *m_pSequencesList;
	vgui::ListPanel *m_pActivitiesList;
	MDLHandle_t m_hSelectedMDL;
	CUtlString m_Filter;

	friend class CSequencePickerFrame;
};


//-----------------------------------------------------------------------------
// Purpose: Main app window
//-----------------------------------------------------------------------------
class CSequencePickerFrame : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CSequencePickerFrame, vgui::Frame );

public:
	CSequencePickerFrame( vgui::Panel *pParent, int nFlags );

	// Inherited from Frame
	virtual void OnCommand( const char *pCommand );

	// Purpose: Activate the dialog
	void DoModal( const char *pMDLName );

private:
	MESSAGE_FUNC_PARAMS( OnSequencePreviewChanged, "SequencePreviewChanged", kv );

	CSequencePicker *m_pPicker;
	vgui::Button *m_pOpenButton;
	vgui::Button *m_pCancelButton;
};


#endif // SEQUENCEPICKER_H
