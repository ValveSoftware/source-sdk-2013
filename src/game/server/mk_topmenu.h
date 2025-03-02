#ifndef MK_TOPMENU_H
#define MK_TOPMENU_H

#ifdef _WIN32
#pragma once
#endif

#include "KeyValues.h"
#include "inetchannel.h"
#include "engine/iserverplugin.h"
#include "player.h"
#include "utlbuffer.h"

class CPluginMenu
{
private:
	int				m_DialogLevel;
	KeyValues*		m_KeyValues;
	DIALOG_TYPE		m_DialogType;
	void			ReleaseKeyValues( void )
	{
		if ( m_KeyValues )
		{
			m_KeyValues->deleteThis();
			m_KeyValues = NULL;
		}
	}
	void InitializeDialog( const DIALOG_TYPE type, const char* title, const char* msg, const Color& color, const int level, const int holdtime )
	{
		ReleaseKeyValues();
		m_DialogType = type;
		m_KeyValues = new KeyValues( "menu" );
		m_KeyValues->SetString( "title", title );
		m_KeyValues->SetInt( "level", level );
		m_KeyValues->SetColor( "color", color );
		m_KeyValues->SetInt( "time", holdtime );
		m_KeyValues->SetString( "msg", msg );
	}

	bool SendData( CBasePlayer* pPlayer )
	{
		if ( !m_KeyValues ) {
			UTIL_LogPrintf( "[CTopMenu] Error: KeyValues is null\n" );
			return false;
		}

		if ( !pPlayer || !pPlayer->edict() )
		{
			UTIL_LogPrintf( "[CTopMenu] Error: Invalid player entity\n" );
			return false;
		}

		INetChannel* pnetchan = static_cast<INetChannel*>(engine->GetPlayerNetInfo( pPlayer->edict()->m_EdictIndex ));

		if ( !pnetchan )
			return false;

		CUtlBuffer keyValuesBuffer;
		m_KeyValues->WriteAsBinary( keyValuesBuffer );

		// Size of KeyValues data
		const int keyValuesSize = keyValuesBuffer.TellPut();

		// Calculate header size in bits: 6 + 16 + 16
		const int headerBits = 6 + 16 + 16;

		// Convert bits to bytes with rounding up
		const int headerBytes = (headerBits + 7) / 8;

		// Total data size (without alignment)
		const int totalDataSize = headerBytes + keyValuesSize;

		// Align buffer size to 4 bytes
		const int alignedBufferSize = (totalDataSize + 3) & ~3;

		// Allocate aligned buffer
		char* tempBuffer = new char[alignedBufferSize](); // Zero-initialization
		bf_write bitBuffer( tempBuffer, alignedBufferSize );

		// Write data
		bitBuffer.WriteUBitLong( 29, 6 );
		bitBuffer.WriteShort( m_DialogType );
		bitBuffer.WriteWord( keyValuesSize );
		bitBuffer.WriteBytes( keyValuesBuffer.Base(), keyValuesSize );

		// Overflow check (must account for alignedBufferSize)
		if ( bitBuffer.IsOverflowed() ) {
			UTIL_LogPrintf( "[CTopMenu] Error: Net buffer overflow\n" );
			goto cleanup;
		}

		// Send data (only the written part will be sent, but buffer is aligned)
		bool sendResult = pnetchan->SendData( bitBuffer );

	cleanup:
		delete[] tempBuffer;
		return sendResult;
	}
	int GetLevel() const
	{
		return m_DialogLevel;
	}
	bool SetLevel( int level )
	{
		if ( m_KeyValues )
		{
			m_KeyValues->SetInt( "level", level );
			return true;
		}
		else
		{
			UTIL_LogPrintf( "[CTopMenu] Error: Invalid m_KeyValues (NULL)" );
			return false;
		}
	}
	bool DecreaseLevel()
	{
		int currentLevel = GetLevel();
		if ( currentLevel == 0 )
		{
			UTIL_LogPrintf( "[mk_topmenu] Can not decrease the current level!" );
			return false;
		}
		SetLevel( currentLevel - 1);
		return true;
	}
public:
	CPluginMenu()
	{
		m_KeyValues = NULL;
		m_DialogType = DIALOG_MSG;
		m_DialogLevel = INT_MAX - 1;
	}
	~CPluginMenu()
	{
		ReleaseKeyValues();
	}
	static CPluginMenu* CreateMenu( const char* title, const char* msg, Color color, int holdTime )
	{
		CPluginMenu* menu = new CPluginMenu();
		menu->InitializeDialog( DIALOG_MENU, title, msg, color, menu->GetLevel(), holdTime );
		return menu;
	}
	static CPluginMenu* CreateMessage( const char* title, const char* msg, Color color, int holdTime )
	{
		CPluginMenu* menu = new CPluginMenu();
		menu->InitializeDialog( DIALOG_MSG, title, "", color, menu->GetLevel(), holdTime );
		return menu;
	}
	static CPluginMenu* CreateRichText( const char* title, const char* msg, Color color, int holdTime )
	{
		CPluginMenu* menu = new CPluginMenu();
		menu->InitializeDialog( DIALOG_TEXT, title, msg, Color( 255, 100, 100, 150 ), menu->GetLevel(), holdTime );
		return menu;
	}
	static CPluginMenu* CreateEntry( const char* title, const char* msg, const char* command, Color color, int holdTime )
	{
		CPluginMenu* menu = new CPluginMenu();
		menu->InitializeDialog( DIALOG_ENTRY, title, msg, color, menu->GetLevel(), holdTime );
		menu->m_KeyValues->SetString( "command", command );
		return menu;
	}
	static CPluginMenu* CreateAskConnect( const char* serverIp, int holdTime )
	{
		CPluginMenu* menu = new CPluginMenu();
		menu->InitializeDialog( DIALOG_ASKCONNECT, serverIp, "", Color( 255, 100, 100, 150 ), 0, holdTime );
		return menu;
	}
	void AddMenuOption( const char* caption, const char* command )
	{
		if ( m_KeyValues )
		{
			KeyValues* item = NULL;
			item = m_KeyValues->FindKey( caption, true );
			item->SetString( "msg", caption );
			item->SetString( "command", command );
		}
	}
	void SendMenuToPlayer( CBasePlayer* pPlayer )
	{
		if ( pPlayer )
		{
			SendData( pPlayer );
		}
	}
	void BroadcastMenuToAllPlayers( void )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer* pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer != NULL && !pPlayer->IsBot() && !pPlayer->IsFakeClient() && pPlayer->IsConnected() )
			{
				SendData( pPlayer );
			}
		}
	}
	void BringToTop( void )
	{
		DecreaseLevel();
	}
};

CPluginMenu* GetPluginMenu()
{
	static CPluginMenu pluginmenu;
	return &pluginmenu;
}

#endif // MK_TOPMENU_H
