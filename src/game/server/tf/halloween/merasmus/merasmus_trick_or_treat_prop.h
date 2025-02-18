//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef MERASMUS_TRICK_OR_TREAT_PROP_H
#define MERASMUS_TRICK_OR_TREAT_PROP_H

DECLARE_AUTO_LIST( ITFMerasmusTrickOrTreatProp );

class CTFMerasmusTrickOrTreatProp : public CBaseAnimating, public ITFMerasmusTrickOrTreatProp
{
	DECLARE_CLASS( CTFMerasmusTrickOrTreatProp, CBaseAnimating );

public:
	CTFMerasmusTrickOrTreatProp();
	~CTFMerasmusTrickOrTreatProp() {}

	virtual void	Spawn( void );
	virtual void	Event_Killed( const CTakeDamageInfo &info );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual void	Touch( CBaseEntity *pOther );
	virtual bool	IsProjectileCollisionTarget( void ) const OVERRIDE { return true; }

	static CTFMerasmusTrickOrTreatProp* Create( const Vector& vPosition, const QAngle& qAngles );

private:
	void SpawnTrickOrTreatItem();
};

#endif // MERASMUS_TRICK_OR_TREAT_PROP_H
