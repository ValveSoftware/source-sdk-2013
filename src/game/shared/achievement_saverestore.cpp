//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#ifdef GAME_DLL 

#include "isaverestore.h"
#include "saverestore_utlvector.h"
#include "achievement_saverestore.h"
#include "achievementmgr.h"
#include "baseachievement.h"
#include "utlmap.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static short ACHIEVEMENT_SAVE_RESTORE_VERSION = 2;

//-----------------------------------------------------------------------------

class CAchievementSaveRestoreBlockHandler : public CDefSaveRestoreBlockHandler
{
public:
	const char *GetBlockName()
	{
		return "Achievement";
	}

	//---------------------------------

	void Save( ISave *pSave )
	{
		CAchievementMgr *pAchievementMgr = dynamic_cast<CAchievementMgr *>( engine->GetAchievementMgr() );
		if ( !pAchievementMgr )
			return;

		// save global achievement mgr state to separate file if there have been any changes, so in case of a crash
		// the global state is consistent with last save game
		pAchievementMgr->SaveGlobalStateIfDirty( pSave->IsAsync() );

		pSave->StartBlock( "Achievements" );
		int iTotalAchievements = pAchievementMgr->GetAchievementCount();
		short nSaveCount = 0;
		// count how many achievements should be saved. 
		for ( int i = 0; i < iTotalAchievements; i++ )
		{
			IAchievement *pAchievement = pAchievementMgr->GetAchievementByIndex( i );
			if ( pAchievement->ShouldSaveWithGame() )
			{
				nSaveCount++;
			}
		}
		// Write # of saved achievements
		pSave->WriteShort( &nSaveCount );
		// Write out each achievement
		for ( int i = 0; i < iTotalAchievements; i++ )
		{
			IAchievement *pAchievement = pAchievementMgr->GetAchievementByIndex( i );
			if ( pAchievement->ShouldSaveWithGame() )
			{				
				CBaseAchievement *pBaseAchievement = dynamic_cast< CBaseAchievement * >( pAchievement );
				if ( pBaseAchievement )
				{
					short iAchievementID = (short) pBaseAchievement->GetAchievementID();
					// write the achievement ID
					pSave->WriteShort( &iAchievementID );
					// write the achievement data
					pSave->WriteAll( pBaseAchievement, pBaseAchievement->GetDataDescMap() );
				}
			}
		}
		pSave->EndBlock();
	}

	//---------------------------------

	void WriteSaveHeaders( ISave *pSave )
	{
		pSave->WriteShort( &ACHIEVEMENT_SAVE_RESTORE_VERSION );
	}
	
	//---------------------------------

	void ReadRestoreHeaders( IRestore *pRestore )
	{
		// No reason why any future version shouldn't try to retain backward compatability. The default here is to not do so.
		short version;
		pRestore->ReadShort( &version );
		// only load if version matches and if we are loading a game, not a transition
		m_fDoLoad = ( ( version == ACHIEVEMENT_SAVE_RESTORE_VERSION ) && 
			( ( MapLoad_LoadGame == gpGlobals->eLoadType ) || ( MapLoad_NewGame == gpGlobals->eLoadType )  ) 
		);
	}

	//---------------------------------

	void Restore( IRestore *pRestore, bool createPlayers )
	{
		CAchievementMgr *pAchievementMgr = dynamic_cast<CAchievementMgr *>( engine->GetAchievementMgr() );
		if ( !pAchievementMgr )
			return;

		if ( m_fDoLoad )
		{
			pAchievementMgr->PreRestoreSavedGame();

			pRestore->StartBlock();
			// read # of achievements
			int nSavedAchievements = pRestore->ReadShort();
			
			while ( nSavedAchievements-- )
			{
				// read achievement ID
				int iAchievementID = pRestore->ReadShort();
				// find the corresponding achievement object
				CBaseAchievement *pAchievement = pAchievementMgr->GetAchievementByID( iAchievementID );				
				Assert( pAchievement );		// It's a bug if we don't understand this achievement
				if ( pAchievement )
				{
					// read achievement data
					pRestore->ReadAll( pAchievement, pAchievement->GetDataDescMap() );
				}
				else
				{
					// if we don't recognize the achievement for some reason, read and discard the data and keep going
					CBaseAchievement ignored;
					pRestore->ReadAll( &ignored, ignored.GetDataDescMap() );
				}
			}
			pRestore->EndBlock();

			pAchievementMgr->PostRestoreSavedGame();
		}
	}

private:
	bool m_fDoLoad;
};

//-----------------------------------------------------------------------------

CAchievementSaveRestoreBlockHandler g_AchievementSaveRestoreBlockHandler;

//-------------------------------------

ISaveRestoreBlockHandler *GetAchievementSaveRestoreBlockHandler()
{
	return &g_AchievementSaveRestoreBlockHandler;
}


#endif // GAME_DLL
