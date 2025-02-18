//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef C_ZOMBIE_H
#define C_ZOMBIE_H

#include "c_ai_basenpc.h"
#include "props_shared.h"

//--------------------------------------------------------------------------------------------------------
/**
 * The client-side implementation of the Halloween Zombie
 */
class C_Zombie : public C_NextBotCombatCharacter
{
public:
	DECLARE_CLASS( C_Zombie, C_NextBotCombatCharacter );
	DECLARE_CLIENTCLASS();

	C_Zombie();

	virtual bool IsNextBot() { return true; }

	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const;

	virtual void BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed ) OVERRIDE;

private:
	C_Zombie( const C_Zombie & );				// not defined, not accessible

	float m_flHeadScale;
};

#endif // C_EYEBALL_BOSS_H
