//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Generic in-game abuse reporting
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_ABUSE_REPORT_H
#define TF_ABUSE_REPORT_H
#ifdef _WIN32
#pragma once
#endif

#include <abuse_report.h>

/// Abuse report manager that knows about the TF2 specifics
class CTFAbuseReportManager : public CAbuseReportManager
{
public:
	CTFAbuseReportManager();
	virtual ~CTFAbuseReportManager();

	virtual void ActivateSubmitReportUI();
	virtual bool CreateAndPopulateIncident();

protected:

	//virtual bool CreateAndPopulateIncident();
};

#endif	// TF_ABUSE_REPORT_H
