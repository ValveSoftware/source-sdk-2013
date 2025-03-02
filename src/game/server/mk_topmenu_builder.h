#ifndef MK_TOPMENU_BUILDER_H
#define MK_TOPMENU_BUILDER_H

#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"

class CMenuBuilder
{
private:
	static constexpr int DEFAULT_ITEMS_PER_PAGE = 8;
	static constexpr int MIN_ITEMS_PER_PAGE = 6;

	struct MenuItem
	{
		CUtlString title;
		CUtlString command;
	};

	struct MenuEntry {
		CUtlVector<MenuItem> items;
		CUtlString	title{ "" };
		CUtlString	description{ "" };
		Color		textColor{ 255, 255, 255 };
		int			holdTime{ 0 };
		float		endTime{ 0 };
		int			selectedIndex{ 0 };

		MenuEntry() = default;

		MenuEntry& operator=( const MenuEntry& entry )
		{
			if ( this == &entry ) return *this;
			title = entry.title;
			description = entry.description;
			textColor = entry.textColor;
			holdTime = entry.holdTime;
			items = entry.items;
			selectedIndex = entry.selectedIndex;
			return *this;
		}
	};

	CUtlString				menuName{ "" };
	MenuEntry				menuTemplate;
	MenuEntry				menuArray[MAX_PLAYERS];
	bool					alwaysOnTop{ true };

	void RenderMenuForClient( const int clientIndex );

	bool IsValidClientIndex( const int clientIndex ) const
	{
		return (clientIndex >= 0 && clientIndex < MAX_PLAYERS);
	}
	int GetMaxItemsPerPage( const int clientIndex ) const
	{
		return menuArray[clientIndex].items.Count() <= DEFAULT_ITEMS_PER_PAGE ? DEFAULT_ITEMS_PER_PAGE : MIN_ITEMS_PER_PAGE;
	}
	bool ShouldUsePagination( const int clientIndex ) const
	{
		return menuArray[clientIndex].items.Count() > DEFAULT_ITEMS_PER_PAGE;
	}
	int GetCurrentMenuPage( CBasePlayer* pPlayer ) const
	{
		int index = pPlayer->GetClientIndex();
		if ( !IsValidClientIndex( index ) )
			return 0;

		if ( !ShouldUsePagination( index ) )
			return 1;
		return ( menuArray[index].selectedIndex / MIN_ITEMS_PER_PAGE ) + 1;
	}
	int GetLastMenuPage( CBasePlayer* pPlayer ) const
	{
		int index = pPlayer->GetClientIndex();
		if ( !IsValidClientIndex( index ) )
			return 0;

		if ( !ShouldUsePagination( index ) )
			return 1;

		return (menuArray[index].items.Count() + MIN_ITEMS_PER_PAGE - 1) / MIN_ITEMS_PER_PAGE;
	}
	void RenderMenuPage( CBasePlayer* pPlayer, const int page )
	{
		int index = pPlayer->GetClientIndex();
		if ( IsValidClientIndex( index ) == false )
			return;

		menuArray[index].selectedIndex = page * GetMaxItemsPerPage( index );
		RenderMenuForClient( index );
	}
	void UpdateMenuExpiry();
	void NavigatePage( int clientIndex, int direction );
public:
	CMenuBuilder() = default;
	~CMenuBuilder()
	{
		menuTemplate.items.Purge();
	};
	void SetAlwaysOnTop( bool value )
	{
		alwaysOnTop = value;
	}
	void CreateMenu( const char* menu_name, const char* title, const char* msg, Color color, const int holdtime );
	bool AddMenuOption( const char* caption, const char* command );
	void NavigateToNextPage( const int clientIndex );
	void NavigateToPreviousPage( const int clientIndex );
	void SendMenuToPlayer( const int clientIndex );
	void BroadcastMenuToAllPlayers( void );
};

class CMenuManager
{
private:
	CUtlMap<const char*, CMenuBuilder*> m_menuPool;

	bool ExecuteMenuAction( const char* name, const std::function<void( CMenuBuilder* )>& action )
	{
		CMenuBuilder* menu = m_menuPool.FindElement( name, nullptr );
		if ( menu )
		{
			action( menu );
			return true;
		}
		return false;
	}
public:
	CMenuManager()
	{
		m_menuPool.SetLessFunc( StringLessThan );
	}
	~CMenuManager()
	{
		ClearAll();
	}
	CMenuBuilder* CreateMenu( const char* name, const char* title, const char* msg, Color color, int holdtime, int numCommands, ... )
	{
		if ( m_menuPool.HasElement( name ) )
		{
			m_menuPool.Remove( name );
		}
		CMenuBuilder* menubuilder = new CMenuBuilder;
		menubuilder->CreateMenu( name, title, msg, color, holdtime );
		va_list args;
		va_start( args, numCommands );
		for ( int i = 0; i < numCommands; ++i )
		{
			const char* caption = va_arg( args, const char* );
			const char* command = va_arg( args, const char* );
			menubuilder->AddMenuOption( caption, command );
		}
		va_end( args );
		m_menuPool.Insert( name, menubuilder );
		return menubuilder;
	}
	bool DeleteMenu( const char* name )
	{
		return m_menuPool.Remove( name );
	}
	void ClearAll()
	{
		for ( unsigned int i = 0; i < m_menuPool.Count(); ++i )
		{
			delete m_menuPool[i];
		}
		m_menuPool.RemoveAll();
	}
	bool SendMenuToPlayer( const char* name, int clientIndex )
	{
		return ExecuteMenuAction( name, [clientIndex]( CMenuBuilder* menu )
			{
				menu->SendMenuToPlayer( clientIndex );
			} );
	}
	bool BroadcastMenuToAllPlayers( const char* name, bool bringToTop = false )
	{
		return ExecuteMenuAction( name, []( CMenuBuilder* menu )
			{
				menu->BroadcastMenuToAllPlayers();
			} );
	}
	bool NavigateToNextPage( const char* name, int clientIndex ) const
	{
		CMenuBuilder* menu = m_menuPool.FindElement( name, nullptr );
		if ( menu )
		{
			menu->NavigateToNextPage( clientIndex );
			return true;
		}
		return false;
	}
	bool NavigateToPreviousPage( const char* name, int clientIndex ) const
	{
		CMenuBuilder* menu = m_menuPool.FindElement( name, nullptr );
		if ( menu )
		{
			menu->NavigateToPreviousPage( clientIndex );
			return true;
		}
		return false;
	}
	unsigned int GetMenuCount() const
	{
		return m_menuPool.Count();
	}
};

CMenuManager* GetTopMenuManager()
{
	static CMenuManager top_menu_pool;
	return &top_menu_pool;
}

#endif // MK_TOPMENU_BUILDER_H
