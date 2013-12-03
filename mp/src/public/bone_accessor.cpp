//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "bone_accessor.h"


#if defined( CLIENT_DLL ) && defined( _DEBUG )

	void CBoneAccessor::SanityCheckBone( int iBone, bool bReadable ) const
	{
		if ( m_pAnimating )
		{
			CStudioHdr *pHdr = m_pAnimating->GetModelPtr();
			if ( pHdr )
			{
				mstudiobone_t *pBone = pHdr->pBone( iBone );
				if ( bReadable )
				{
					AssertOnce( pBone->flags & m_ReadableBones );
				}
				else
				{
					AssertOnce( pBone->flags & m_WritableBones );
				}
			}
		}
	}

#endif

