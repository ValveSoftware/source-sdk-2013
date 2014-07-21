//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef ENTITYAPI_H
#define ENTITYAPI_H


class SendTable;

extern void LoadMapEntities( const char *pMapEntities );
extern void	DispatchObjectCollisionBox( edict_t *pent );
extern float DispatchObjectPhysicsVelocity( edict_t *pent, float moveTime );
extern ServerClass* DispatchGetObjectServerClass(edict_t *pent);
extern ServerClass* GetAllServerClasses();
extern void SaveWriteFields( CSaveRestoreData *pSaveData, const char *pname, void *pBaseData, datamap_t *pMap, typedescription_t *pFields, int fieldCount );
extern void SaveReadFields( CSaveRestoreData *pSaveData, const char *pname, void *pBaseData, datamap_t *pMap, typedescription_t *pFields, int fieldCount );
extern void SaveGlobalState( CSaveRestoreData *pSaveData );
extern void RestoreGlobalState( CSaveRestoreData *pSaveData );
extern void ResetGlobalState( void );
extern CSaveRestoreData *SaveInit( int size  );
extern int CreateEntityTransitionList( CSaveRestoreData *pSaveData, int levelMask );
extern void ClearEntities( void );
extern void FreeContainingEntity( edict_t *ed );

class ISaveRestoreBlockHandler;
ISaveRestoreBlockHandler *GetEntitySaveRestoreBlockHandler();


#endif			// ENTITYAPI_H
