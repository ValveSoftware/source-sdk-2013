//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=============================================================================

#ifndef HELPERINFO_H
#define HELPERINFO_H
#pragma once

#include <tier0/dbg.h>
#include <utlvector.h>


#define MAX_HELPER_NAME_LEN			256


typedef CUtlVector<char *> CParameterList;


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHelperInfo
{
	public:

		inline CHelperInfo(void);
		inline ~CHelperInfo(void);

		inline const char *GetName(void) const { return(m_szName); }
		inline void SetName(const char *pszName);

		inline bool AddParameter(const char *pszParameter);

		inline int GetParameterCount(void) const { return(m_Parameters.Count()); }
		inline const char *GetParameter(int nIndex) const;

	protected:

		char m_szName[MAX_HELPER_NAME_LEN];
		CParameterList m_Parameters;
};



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CHelperInfo::CHelperInfo(void)
{
	m_szName[0] = '\0';
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CHelperInfo::~CHelperInfo(void)
{
	int nCount = m_Parameters.Count();
	for (int i = 0; i < nCount; i++)
	{
		char *pszParam = m_Parameters.Element(i);
		Assert(pszParam != NULL);
		if (pszParam != NULL)
		{
			delete [] pszParam;
		}
	}

	m_Parameters.RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : char *pszParameter - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
inline bool CHelperInfo::AddParameter(const char *pszParameter)
{
	if ((pszParameter != NULL) && (pszParameter[0] != '\0'))
	{
		int nLen = strlen(pszParameter);
		
		if (nLen > 0)
		{
			char *pszNew = new char [nLen + 1];
			if (pszNew != NULL)
			{
				strcpy(pszNew, pszParameter);
				m_Parameters.AddToTail(pszNew);
				return(true);
			}
		}
	}

	return(false);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline const char *CHelperInfo::GetParameter(int nIndex) const
{
	if (nIndex >= m_Parameters.Count())
		return NULL;
	
	return m_Parameters.Element(nIndex); 
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : char *pszName - 
//-----------------------------------------------------------------------------
inline void CHelperInfo::SetName(const char *pszName)
{
	if (pszName != NULL)
	{	
		strcpy(m_szName, pszName);
	}
}


typedef CUtlVector<CHelperInfo *> CHelperInfoList;


#endif // HELPERINFO_H
