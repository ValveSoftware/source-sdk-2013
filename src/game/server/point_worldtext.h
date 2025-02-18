//========= Copyright Valve Corporation, All rights reserved. ============//

#pragma once

//------------------------------------------------------------------------------
class CPointWorldText: public CBaseEntity
{
public:
	DECLARE_CLASS( CPointWorldText, CBaseEntity );

	CPointWorldText();
	virtual ~CPointWorldText();

	virtual void Spawn( void ) OVERRIDE;

	virtual bool KeyValue( const char *szKeyName, const char *szValue ) OVERRIDE;
	virtual int  UpdateTransmitState() OVERRIDE;

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	void InputSetText( inputdata_t &inputdata )
	{
		if ( inputdata.value.String() )
		{
			V_strncpy( m_szText.GetForModify(), inputdata.value.String(), sizeof(m_szText));
		}
	}

	void InputSetTextSize( inputdata_t &inputdata )
	{
		m_flTextSize = inputdata.value.Float();
	}

	void InputSetTextSpacingX( inputdata_t &inputdata )
	{
		m_flTextSpacingX = inputdata.value.Float();
	}

	void InputSetTextSpacingY( inputdata_t &inputdata )
	{
		m_flTextSpacingY = inputdata.value.Float();
	}

	void InputSetColor( inputdata_t &inputdata )
	{
		m_colTextColor = inputdata.value.Color32();
	}

	void InputSetOrientation( inputdata_t &inputdata )
	{
		m_nOrientation = inputdata.value.Int();
	}

	void InputSetFont( inputdata_t &inputdata )
	{
		m_nFont = inputdata.value.Int();
	}

	void InputSetRainbow( inputdata_t &inputdata )
	{
		m_bRainbow = inputdata.value.Bool();
	}

private:
	CNetworkString( m_szText, MAX_PATH );
	CNetworkVar( float, m_flTextSize );
	CNetworkVar( float, m_flTextSpacingX );
	CNetworkVar( float, m_flTextSpacingY );
	CNetworkColor32( m_colTextColor );
	CNetworkVar( int, m_nOrientation );
	CNetworkVar( int, m_nFont );
	CNetworkVar( bool, m_bRainbow );
};
