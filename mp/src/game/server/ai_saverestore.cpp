//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "isaverestore.h"
#include "saverestore_utlvector.h"
#include "ai_saverestore.h"
#include "ai_basenpc.h"
#include "ai_squad.h"
#include "ai_network.h"
#include "ai_networkmanager.h"

#ifdef HL2_DLL
#include "npc_playercompanion.h"
#endif // HL2_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static short AI_SAVE_RESTORE_VERSION = 2;

//-----------------------------------------------------------------------------

class CAI_SaveRestoreBlockHandler : public CDefSaveRestoreBlockHandler
{
public:
	const char *GetBlockName()
	{
		return "AI";
	}

	//---------------------------------

	void Save( ISave *pSave )
	{
		pSave->StartBlock( "Squads" );
		short nSquads = (short)g_AI_SquadManager.NumSquads();
		pSave->WriteShort( &nSquads );
		
		AISquadsIter_t iter;
		string_t squadName;
		CAI_Squad* pSquad = g_AI_SquadManager.GetFirstSquad( &iter );
		while (pSquad)
		{
			squadName = MAKE_STRING( pSquad->GetName() );
			pSave->WriteString( "", &squadName ); // Strings require a header to be read properly
			pSave->WriteAll( pSquad );
			pSquad = g_AI_SquadManager.GetNextSquad( &iter );
		}
		
		pSave->EndBlock();

		//---------------------------------
		
		pSave->StartBlock( "Enemies" );
		short nMemories = 0;
		
		CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
		int i;

		for ( i = 0; i < g_AI_Manager.NumAIs(); i++ )
		{
			if ( ppAIs[i]->GetEnemies() )
				nMemories++;
		}

		pSave->WriteShort( &nMemories );
		
		for ( i = 0; i < g_AI_Manager.NumAIs(); i++ )
		{
			if ( ppAIs[i]->GetEnemies() )
			{
				CBaseEntity *p = ppAIs[i];
				pSave->WriteEntityPtr( &p );
				pSave->WriteAll( ppAIs[i]->GetEnemies() );
			}
		}
		pSave->EndBlock();
	}

	//---------------------------------

	void WriteSaveHeaders( ISave *pSave )
	{
		pSave->WriteShort( &AI_SAVE_RESTORE_VERSION );
	}
	
	//---------------------------------

	void ReadRestoreHeaders( IRestore *pRestore )
	{
		// No reason why any future version shouldn't try to retain backward compatability. The default here is to not do so.
		short version;
		pRestore->ReadShort( &version );
		m_fDoLoad = ( version == AI_SAVE_RESTORE_VERSION );
	}

	//---------------------------------

	void Restore( IRestore *pRestore, bool createPlayers )
	{
		// Initialize the squads (as there's no spawn)
		CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
		int i;

		for ( i = 0; i < g_AI_Manager.NumAIs(); i++ )
		{
			ppAIs[i]->InitSquad();
		}
		
		if ( m_fDoLoad )
		{
			pRestore->StartBlock();
			// Fixup all the squads
			CAI_Squad  ignored;
			CAI_Squad *pSquad;
			string_t   squadName;
			int 	   nSavedSquads = pRestore->ReadShort();
			
			while ( nSavedSquads-- )
			{
				int sizeData = pRestore->SkipHeader();
				pRestore->ReadString( &squadName, 1, sizeData );
				pSquad = g_AI_SquadManager.FindSquad( squadName );
				if ( !pSquad )
					pSquad = &ignored; // if all of the AIs in a squad failed to spawn, there would be no squad
				pRestore->ReadAll( pSquad );
			}
			pRestore->EndBlock();
			
			//---------------------------------
			// Now load memories for unsquadded npcs
			
			pRestore->StartBlock();
			CAI_Enemies ignoredMem;
			short nMemories = pRestore->ReadShort();
			
			CBaseEntity *pAI; 
			
			while ( nMemories-- )
			{
				pRestore->ReadEntityPtr( &pAI );
				
				if ( pAI )
					pRestore->ReadAll( ((CAI_BaseNPC *)pAI)->GetEnemies() );
				else
					pRestore->ReadAll( &ignoredMem ); // AI probably failed to spawn
			}
			
			pRestore->EndBlock();
		}
		
		if ( g_AI_Manager.NumAIs() && g_pBigAINet->NumNodes() == 0 && !g_pAINetworkManager->NetworksLoaded() )
		{
			Msg( "***\n");
			Msg( "ERROR: Loaded save game with no node graph. Load map and build node graph first!\n");
			Msg( "***\n");
			CAI_BaseNPC::m_nDebugBits |= bits_debugDisableAI;
			g_pAINetworkManager->MarkDontSaveGraph();
		}
	}

	void PostRestore( void )
	{
#ifdef HL2_DLL
		// We need this list to be regenerated
		OverrideMoveCache_ForceRepopulateList();
#endif // HL2_DLL
	}

private:
	bool m_fDoLoad;
};

//-----------------------------------------------------------------------------

CAI_SaveRestoreBlockHandler g_AI_SaveRestoreBlockHandler;

//-------------------------------------

ISaveRestoreBlockHandler *GetAISaveRestoreBlockHandler()
{
	return &g_AI_SaveRestoreBlockHandler;
}

//=============================================================================
