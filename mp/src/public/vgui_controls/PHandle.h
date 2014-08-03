//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PHANDLE_H
#define PHANDLE_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>

namespace vgui
{

class Panel;

//-----------------------------------------------------------------------------
// Purpose: Safe pointer class for handling Panel or derived panel classes
//-----------------------------------------------------------------------------
class PHandle
{
public:
	PHandle() : m_iPanelID(INVALID_PANEL) {} //m_iSerialNumber(0), m_pListEntry(0) {}

	Panel *Get();
	Panel *Set( Panel *pPanel );
	Panel *Set( HPanel hPanel );

	operator Panel *()						{ return Get(); }
	Panel * operator ->()					{ return Get(); }
	Panel * operator = (Panel *pPanel)		{ return Set(pPanel); }

	bool operator == (Panel *pPanel)		{ return (Get() == pPanel); }
	operator bool ()						{ return Get() != 0; }

private:
	HPanel m_iPanelID;
};

//-----------------------------------------------------------------------------
// Purpose: Safe pointer class to just convert between VPANEL's and PHandle
//-----------------------------------------------------------------------------
class VPanelHandle
{
public:
	VPanelHandle() : m_iPanelID(INVALID_PANEL) {}

	VPANEL Get();
	VPANEL Set( VPANEL pPanel );

	operator VPANEL ()						{ return Get(); }
	VPANEL operator = (VPANEL pPanel)		{ return Set(pPanel); }

	bool operator == (VPANEL pPanel)		{ return (Get() == pPanel); }
	operator bool ()						{ return Get() != 0; }

private:
	HPanel m_iPanelID;
};

//-----------------------------------------------------------------------------
// Purpose: DHANDLE is a templated version of PHandle
//-----------------------------------------------------------------------------
template< class PanelType >
class DHANDLE : public PHandle
{
public:
	PanelType *Get()					{ return (PanelType *)PHandle::Get(); }
	PanelType *Set( PanelType *pPanel )	{ return (PanelType *)PHandle::Set(pPanel); }
	PanelType *Set( HPanel hPanel )		{ return (PanelType *)PHandle::Set(hPanel); }

	operator PanelType *()						{ return (PanelType *)PHandle::Get(); }
	PanelType * operator ->()					{ return (PanelType *)PHandle::Get(); }
	PanelType * operator = (PanelType *pPanel)	{ return (PanelType *)PHandle::Set(pPanel); }
	bool operator == (Panel *pPanel)			{ return (PHandle::Get() == pPanel); }
	operator bool ()							{ return PHandle::Get() != NULL; }
};

};

#endif // PHANDLE_H
