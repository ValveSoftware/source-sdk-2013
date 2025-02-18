//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================


#ifndef COMMANDMENU_H
#define COMMANDMENU_H

#include <vgui_controls/Menu.h>
#include <game/client/iviewport.h>
#include <filesystem.h>
#include "utlstack.h"
#include "utlvector.h"
#include <KeyValues.h>

using namespace vgui;

class CommandMenu : public Menu
{
private:
	DECLARE_CLASS_SIMPLE( CommandMenu, Menu );
	
		typedef struct
		{
			Menu *	menu;
			int		itemnr;
		} CommandMenuItem;

	public:
		CommandMenu( Panel *parent, const char *panelName, IViewPort * viewport );
		~CommandMenu();

		bool		LoadFromFile(const char * fileName);	// load menu from file (via KeyValues)
		void		UpdateMenu();	// call to update all menu items, check buttons etc
		void		RebuildMenu();  // rebuilds menu respecting changed game state (map, team etc)
		void		ClearMenu();	// destroy menu
	
	public:
		// overwrite these in your derived class
		// virtual CommandMenu * CommandMenu::Factory(Panel *parent, const char *panelName, IViewPort * viewport = NULL, IFileSystem * pFileSytem = NULL); // overwrite
		virtual int  AddCustomItem(KeyValues * params, Menu * menu) {return 0;} // return MenuItem nr
		virtual void UpdateCustomItem(KeyValues * params, MenuItem * item ) {}; // maybe change your item
		virtual void OnCustomItem(KeyValues * params) {}; // a custom item was pressed
		virtual bool CheckRules(const char *rule, const char *ruledata); // check a menu item rule
		virtual void SetVisible(bool state);

	// DON'T touch anything below !

	protected:
				
		void	OnMessage(const KeyValues *params, VPANEL fromPanel);
		void	StartNewSubMenu(KeyValues * params);
		void	FinishSubMenu();
		void	AddMenuCommandItem(KeyValues * params);
		void	AddMenuCustomItem(KeyValues * params);
		void	AddMenuToggleItem(KeyValues * params);
		bool	LoadFromKeyValuesInternal(KeyValues * key, int depth);
		bool	LoadFromKeyValues( KeyValues * key); // 
		
		KeyValues * GetKeyValues();	// returns keyValues for current menu or NULL

	
		IViewPort * m_ViewPort;		// viewport interface
		Menu *		m_CurrentMenu;	// Current menu while building CommandComoboBox
		char		m_CurrentTeam[4];
		char		m_CurrentMap[256];
		
		KeyValues*  m_MenuKeys;
		
		CUtlStack<vgui::Menu*>m_pMenuStack;		
		CUtlVector<CommandMenuItem>m_MenuItems;	
};

#endif // COMMANDMENU_H
