//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef TF_BOT_HINT_ENTITY_H
#define TF_BOT_HINT_ENTITY_H

DECLARE_AUTO_LIST( ITFBotHintEntityAutoList );

class CBaseTFBotHintEntity : public CPointEntity, public ITFBotHintEntityAutoList
{
	DECLARE_CLASS( CBaseTFBotHintEntity, CPointEntity );
public:
	DECLARE_DATADESC();

	CBaseTFBotHintEntity( void );
	virtual ~CBaseTFBotHintEntity() { }

	enum HintType
	{
		HINT_INVALID = -1,
		HINT_TELEPORTER_EXIT,
		HINT_SENTRYGUN,
		HINT_ENGINEER_NEST,
	};
	virtual HintType GetHintType() const = 0;
	bool IsHintType( HintType hintType ) { return GetHintType() == hintType; }

	bool OwnerObjectHasNoOwner() const;
	bool OwnerObjectFinishBuilding() const;

	bool IsEnabled() const;
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

private:

	bool m_isDisabled;
	HintType m_hintType;
};


inline void CBaseTFBotHintEntity::InputEnable( inputdata_t &inputdata )
{
	m_isDisabled = false;
}

inline void CBaseTFBotHintEntity::InputDisable( inputdata_t &inputdata )
{
	m_isDisabled = true;
}

inline bool CBaseTFBotHintEntity::IsEnabled() const
{
	return !m_isDisabled;
}

#endif // TF_BOT_HINT_ENTITY_H
