
#include "cbase.h"
#include "c_gstring_player.h"


//CON_COMMAND( gstring_list_recvproj, "" )
//{
//	for ( C_BaseEntity *pEnt = ClientEntityList().FirstBaseEntity();
//		pEnt;
//		pEnt = ClientEntityList().NextBaseEntity( pEnt ) )
//	{
//		if ( !pEnt->ShouldReceiveProjectedTextures( SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK ) )
//			continue;
//
//		Msg( "%i: %s - %s\n", pEnt->entindex(), pEnt->GetClassName(), pEnt->GetClassname() );
//	}
//}


IMPLEMENT_CLIENTCLASS_DT( C_GstringPlayer, DT_CGstringPlayer, CGstringPlayer )

	RecvPropBool( RECVINFO( m_bNightvisionActive ) ),

END_RECV_TABLE()

C_GstringPlayer::C_GstringPlayer()
	: m_flNightvisionFraction( 0.0f )
{
}

bool C_GstringPlayer::IsNightvisionActive() const
{
	if ( render && render->GetViewEntity() > gpGlobals->maxClients )
		return false;

	return m_bNightvisionActive;
}

float C_GstringPlayer::GetNightvisionFraction() const
{
	return m_flNightvisionFraction;
}

void C_GstringPlayer::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
}

void C_GstringPlayer::ClientThink()
{
	BaseClass::ClientThink();

	const float flNightvisionTarget = IsNightvisionActive() ? 1.0f : 0.0f;

	if ( flNightvisionTarget != m_flNightvisionFraction )
	{
		m_flNightvisionFraction = Approach( flNightvisionTarget, m_flNightvisionFraction, gpGlobals->frametime * 5.0f );

		unsigned char r,g,b;
		g_pClientShadowMgr->GetShadowColor( &r, &g, &b );

		Vector v( r / 255.0f, g / 255.0f, b / 255.0f );
		v = Lerp( m_flNightvisionFraction * 0.7f, v, Vector( 1, 1, 1 ) );

		g_pClientShadowMgr->SetShadowColorMaterialsOnly( XYZ( v ) );
	}
}