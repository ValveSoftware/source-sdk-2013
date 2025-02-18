//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_TRAINING_H
#define TF_HUD_TRAINING_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include "tf_controls.h"
#include "GameEventListener.h"
#include "c_tf_objective_resource.h"
#include "IconPanel.h"
#include "tf_gamerules.h"
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"

#define MAX_TRAINING_MSG_LENGTH 512

extern int Training_GetCompletedTrainingClasses();
extern void Training_MarkClassComplete( int iClass, int iStage );

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTFHudTraining : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTFHudTraining, vgui::EditablePanel );

public:

	CTFHudTraining( vgui::Panel *parent, const char *name );
	virtual ~CTFHudTraining();

	static bool   FormatTrainingText( const char* input, wchar_t* output );

	virtual void  ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void  Reset();
	virtual void  FireGameEvent( IGameEvent *event );
	virtual bool  IsVisible( void );
	virtual void  OnTick();

	void          SetTrainingText( char *msg );
	void          SetTrainingObjective( char *msg );

private:

	CExRichText		*m_pMsgLabel;
	CExLabel		*m_pPressSpacebarToContinueLabel;
};


#endif	// TF_HUD_TRAINING_H
