//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IDLE_DIALOG_H
#define IDLE_DIALOG_H
#ifdef _WIN32
#pragma once
#endif


#include "win_idle.h"


//
// This is a base class that provides in-thread idle processing.
//
// To use it:
// 1. derive from it
// 2. Change your message map to point at this class instead of CDialog
// 3. Call StartIdleProcessing to begin receiving idle calls.
// 4. Override OnIdle().
//

class CIdleDialog : public CDialog
{
public:
					
					CIdleDialog( int id, CWnd *pParent );
	
	// Call this to start the idle processing.
	void			StartIdleProcessing( DWORD msInterval );
	
	virtual void	OnIdle() = 0;


private:
	DECLARE_MESSAGE_MAP()
	afx_msg LONG OnStartIdle(UINT, LONG);

	CWinIdle m_cWinIdle;
};


#endif // IDLE_DIALOG_H
