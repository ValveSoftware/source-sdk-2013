#include "cbase.h"

class CBaseDMStart : public CPointEntity
{
public:
	DECLARE_CLASS( CBaseDMStart, CPointEntity );

	bool IsTriggered( CBaseEntity *pEntity );

	DECLARE_DATADESC();

	string_t m_Master;

private:
};

// Peter: I don't see a reason why this could not use CBaseDMStart, 
// but as a precaution because team spawn points were set to use CPointEntity, 
// I will create a class for this.
class CBaseTeamSpawn : public CPointEntity
{
public:
	DECLARE_CLASS( CBaseTeamSpawn, CPointEntity );
};