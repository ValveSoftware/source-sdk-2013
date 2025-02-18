//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Play VCD on taunt prop
//
// $NoKeywords: $
//=============================================================================//
#ifndef TF_TAUNT_PROP_H
#define TF_TAUNT_PROP_H
#ifdef _WIN32
#pragma once
#endif

class CTFTauntProp : public CBaseCombatCharacter
{
	DECLARE_CLASS( CTFTauntProp, CBaseCombatCharacter );
public:
	DECLARE_SERVERCLASS();

	CTFTauntProp();

	virtual bool StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget ) OVERRIDE;

	virtual bool ProcessSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event ) OVERRIDE;

	virtual float PlayScene( const char *pszScene, float flDelay = 0.0f, AI_Response *response = NULL, IRecipientFilter *filter = NULL ) OVERRIDE;

	virtual void UpdateOnRemove() OVERRIDE;

	void SetAutoRemove( bool bAutoRemove ) { m_bAutoRemove = bAutoRemove; }
	bool ShouldSelfRemove() const { return m_bAutoRemove; }

	CBaseEntity *GetSceneEntity() { return m_hScene.Get(); }

private:
	EHANDLE m_hScene;
	bool m_bAutoRemove;
};

#endif // TF_TAUNT_PROP_H
