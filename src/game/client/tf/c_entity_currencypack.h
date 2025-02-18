//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef C_ENTITY_CURRENCYPACK_H
#define C_ENTITY_CURRENCYPACK_H

class C_CurrencyPack : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_CurrencyPack, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

	C_CurrencyPack();
	~C_CurrencyPack();

	virtual void OnDataChanged( DataUpdateType_t updateType ) OVERRIDE;
	virtual void ClientThink();

private:

	void UpdateGlowEffect( void );
	void DestroyGlowEffect( void );
	CGlowObject *m_pGlowEffect;
	bool m_bShouldGlowForLocalPlayer;

	bool m_bDistributed;
};

#endif // C_ENTITY_CURRENCYPACK_H
