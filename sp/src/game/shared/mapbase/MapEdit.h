//========= Copyright Valve Corporation, All rights reserved. ============//
// 
// Purpose: Accessing MapEdit
// 
// $NoKeywords: $
//=============================================================================//

extern ConVar mapedit_enabled;
extern ConVar mapedit_stack;
extern ConVar mapedit_debug;

void MapEdit_MapReload( void );

void MapEdit_LoadFile( const char *pFile, bool bStack = mapedit_stack.GetBool() );
