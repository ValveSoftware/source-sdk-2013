//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Play VCD on taunt prop
//
// $NoKeywords: $
//=============================================================================//
#ifndef C_TF_TAUNT_PROP_H
#define C_TF_TAUNT_PROP_H
#ifdef _WIN32
#pragma once
#endif

class C_TFTauntProp : public C_BaseCombatCharacter
{
	DECLARE_CLASS( C_TFTauntProp, C_BaseCombatCharacter );
public:
	DECLARE_CLIENTCLASS();

	virtual bool	StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, C_BaseEntity *pTarget ) OVERRIDE;
	virtual	bool	ClearSceneEvent( CSceneEventInfo *info, bool fastKill, bool canceled ) OVERRIDE;
	virtual void	UpdateOnRemove() OVERRIDE;
};

#endif // C_TF_TAUNT_PROP_H
