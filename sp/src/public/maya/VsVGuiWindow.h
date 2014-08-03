//===== Copyright © 1996-2006, Valve Corporation, All rights reserved. ======//
//
// Base class for windows that draw vgui in Maya
//
//===========================================================================//

#ifndef VSVGUIWINDOW_H
#define VSVGUIWINDOW_H

#ifdef _WIN32
#pragma once
#endif


#include "imayavgui.h"
#include "vgui_controls/Frame.h"
#include "tier1/utlmap.h"
#include "valveMaya.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IMayaVGui;


//-----------------------------------------------------------------------------
// The singleton is defined here twice just so we don't have to include valvemaya.h also
//-----------------------------------------------------------------------------
extern IMayaVGui *g_pMayaVGui;


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
namespace vgui
{
	class EditablePanel;
}


class CVsVGuiWindowBase
{
public:
	virtual void SetPeriod( float flPeriod ) = 0;
	virtual void StartTick() = 0;
	virtual void StartTick( float flPeriod ) = 0;
	virtual void StopTick() = 0;
	virtual void Tick( float flElapsedTime ) = 0;
	virtual void NonTimerTick() = 0;
};


//-----------------------------------------------------------------------------
// Creates, destroys a maya vgui window
//-----------------------------------------------------------------------------
CVsVGuiWindowBase *CreateMayaVGuiWindow( vgui::EditablePanel *pRootPanel, const char *pPanelName );
void DestroyMayaVGuiWindow( const char *pPanelName );


//-----------------------------------------------------------------------------
// Factory used to install vgui windows easily
//-----------------------------------------------------------------------------
class CVsVguiWindowFactoryBase : public IMayaVguiWindowFactory
{
public:
	CVsVguiWindowFactoryBase( const char *pWindowTypeName, const char *pDccStartupCommand );

	// Returns the DCC command
	const char *GetDccStartupCommand() const;

	// Registers/deregisters all vgui windows
	static void RegisterAllVguiWindows( );
	static void UnregisterAllVguiWindows( );

protected:
	const char *m_pWindowTypeName;
	const char *m_pDccStartupCommand;

private:
	CVsVguiWindowFactoryBase *m_pNext;
	static CVsVguiWindowFactoryBase *s_pFirstCommandFactory;
};

template< class T >
class CVsVguiWindowFactory : public CVsVguiWindowFactoryBase
{
	typedef CVsVguiWindowFactoryBase BaseClass;

	static bool StringLessFunc( const CUtlString &a, const CUtlString &b )
	{
		return StringLessThan( a.Get(), b.Get() );
	}


public:
	CVsVguiWindowFactory( const char *pWindowTypeName, const char *pDccCommand )
	: BaseClass( pWindowTypeName, pDccCommand )
	, m_panelMap( StringLessFunc )
	{
	}

	struct PanelMapElem_s
	{
		CVsVGuiWindowBase *m_pVGuiWindow;
		T *m_pPanel;
	};

	typedef CUtlMap< CUtlString, PanelMapElem_s > PanelMap_t;

	virtual void CreateVguiWindow( const char *pPanelName )
	{
		T *pVGuiPanel = new T( NULL, pPanelName );
		vgui::Frame *pFrame = dynamic_cast< vgui::Frame * >( pVGuiPanel );

		if ( pFrame )
		{
			pFrame->SetSizeable( false );
			pFrame->SetCloseButtonVisible( false );
			pFrame->SetMoveable( false );

			CVsVGuiWindowBase *pVGuiWindow = CreateMayaVGuiWindow( pVGuiPanel, pPanelName );

			const CUtlString panelName( pPanelName );

			PanelMap_t::IndexType_t nIndex = m_panelMap.Find( panelName );
			if ( m_panelMap.IsValidIndex( nIndex ) )
			{
				merr << "[vsVguiWindow]: Panel \"" << pPanelName << "\" of Type: \"" << m_pWindowTypeName << "\" Already Exists!!!" << std::endl;
			}
			else
			{
				PanelMap_t::ElemType_t &element = m_panelMap.Element( m_panelMap.Insert( panelName ) );
				element.m_pVGuiWindow = pVGuiWindow;
				element.m_pPanel = pVGuiPanel;
			}
		}
	}

	virtual void DestroyVguiWindow( const char *pPanelName )
	{
		PanelMap_t::IndexType_t nIndex = m_panelMap.Find( pPanelName );
		if ( !m_panelMap.IsValidIndex( nIndex ) )
			return;

		PanelMap_t::ElemType_t &element = m_panelMap.Element( nIndex );
		delete element.m_pPanel;

		m_panelMap.Remove( CUtlString( pPanelName ) );
		DestroyMayaVGuiWindow( pPanelName );
	}

	virtual vgui::Frame *GetVGuiPanel( const char *pPanelName = NULL )
	{
		if ( pPanelName )
		{
			PanelMap_t::IndexType_t nPanelIndex = m_panelMap.Find( CUtlString( pPanelName ) );
			if ( m_panelMap.IsValidIndex( nPanelIndex ) )
				return dynamic_cast< vgui::Frame * >( m_panelMap.Element( nPanelIndex ).m_pPanel );
		}
		else if ( m_panelMap.Count() > 0 )
		{
				return dynamic_cast< vgui::Frame * >( m_panelMap.Element( m_panelMap.FirstInorder() ).m_pPanel );
		}

		return NULL;
	}

	virtual CVsVGuiWindowBase *GetVGuiWindow( const char *pPanelName = NULL )
	{
		if ( pPanelName )
		{
			PanelMap_t::IndexType_t nPanelIndex = m_panelMap.Find( CUtlString( pPanelName ) );
			if ( m_panelMap.IsValidIndex( nPanelIndex ) )
				return m_panelMap.Element( nPanelIndex ).m_pVGuiWindow;
		}
		else if ( m_panelMap.Count() > 0 )
		{
				return m_panelMap.Element( m_panelMap.FirstInorder() ).m_pVGuiWindow;
		}

		return NULL;
	}

private:

	PanelMap_t m_panelMap;
};


#define INSTALL_MAYA_VGUI_WINDOW( _className, _windowTypeName, _dccCommand )	\
	static CVsVguiWindowFactory< _className > s_VsVguiWindowFactory##_className##( _windowTypeName, _dccCommand )


#endif // VSVGUIWINDOW_H
