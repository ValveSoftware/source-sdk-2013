#pragma once

#include "postprocess_shared.h"

//=============================================================================
//
// Class Postprocess Controller:
//
class C_PostProcessController : public C_BaseEntity
{
	DECLARE_CLASS( C_PostProcessController, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();

	C_PostProcessController();
	virtual ~C_PostProcessController();

	virtual void PostDataUpdate( DataUpdateType_t updateType );

	static C_PostProcessController* GetMasterController() { return ms_pMasterController; }

	PostProcessParameters_t	m_PostProcessParameters;

#ifdef MAPBASE
	// Prevents fade time from being used in save/restore
	virtual void OnRestore();
#endif
	
private:
	bool m_bMaster;

	static C_PostProcessController* ms_pMasterController;
};
