//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Generic in-game abuse reporting
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_SHARED_CONTENT_MANAGER_H
#define TF_SHARED_CONTENT_MANAGER_H
#ifdef _WIN32
#pragma once
#endif

typedef struct 
{
	int iFlag;
	uint32 unAccountID;
} shared_vision_entry_t;

// Shared content manager
class C_TFSharedContentManager : public CBaseGameSystemPerFrame
{
public:
	C_TFSharedContentManager(){}

	void OfferSharedVision( int iFlag, uint32 unAccountID );
	bool IsSharedVisionAvailable( int iFlag ){ return ( m_iSharedVisionFlags & iFlag ); }

	// Methods of IGameSystem
	virtual char const *Name() { return "C_TFSharedContentManager"; }
	virtual bool Init();
	virtual void Update( float frametime );

private:
	bool CanOfferVision( int iFlag );
	void AddSharedVision( int iFlag ){ m_iSharedVisionFlags |= iFlag; }
	void OfferSharedVision_Internal( int iFlag, uint32 unAccountID );
	void PrintChatText( int iFlag, uint32 unAccountID );

private:
	int m_iSharedVisionFlags;
	CUtlVector< uint32 > m_PlayersWhoHaveOfferedVision;	// needs to be expanded when we offer more than Romevision...
	CUtlVector< shared_vision_entry_t > m_SharedVisionQueue;
};

extern C_TFSharedContentManager *TFSharedContentManager();	// get singleton

#endif	// TF_SHARED_CONTENT_MANAGER_H
