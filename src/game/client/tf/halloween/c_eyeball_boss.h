//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef C_EYEBALL_BOSS_H
#define C_EYEBALL_BOSS_H

#include "c_ai_basenpc.h"

#define EYEBALL_ANGRY	2
#define EYEBALL_GRUMPY	1
#define EYEBALL_CALM	0

//--------------------------------------------------------------------------------------------------------
/**
 * The client-side implementation of the Halloween Eyeball Boss
 */
class C_EyeballBoss : public C_NextBotCombatCharacter
{
public:
	DECLARE_CLASS( C_EyeballBoss, C_NextBotCombatCharacter );
	DECLARE_CLIENTCLASS();

	C_EyeballBoss();
	virtual ~C_EyeballBoss();

	virtual void Spawn( void );
	virtual void OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual bool IsNextBot() { return true; }

	virtual void FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );
	virtual void ClientThink();
	virtual void SetDormant( bool bDormant );

	virtual QAngle const &GetRenderAngles( void );
	virtual int	InternalDrawModel( int flags );

private:
	C_EyeballBoss( const C_EyeballBoss & );				// not defined, not accessible

	Vector m_lookAtSpot;
	int m_attitude;
	int m_priorAttitude;

	QAngle m_myAngles;

	int m_leftRightPoseParameter;
	int m_upDownPoseParameter;

	HPARTICLEFFECT		m_ghostEffect;

	HPARTICLEFFECT		m_auraEffect;

	CMaterialReference	m_InvulnerableMaterial;
};

#endif // C_EYEBALL_BOSS_H
