//--------------------------------------------------------------------------------------------------------
// Copyright (c) 2007 Turtle Rock Studios, Inc. - All Rights Reserved

#ifndef FOG_VOLUME_H
#define FOG_VOLUME_H

#ifdef _WIN32
#pragma once
#endif


class CFogController;
class CPostProcessController;
class CColorCorrection;


//--------------------------------------------------------------------------------------------------------
// Fog volume entity
class CFogVolume : public CServerOnlyEntity
{
	DECLARE_CLASS( CFogVolume, CServerOnlyEntity );
	DECLARE_DATADESC();

public:
	CFogVolume();
	virtual ~CFogVolume();
	virtual void Spawn( void );
	virtual void Activate();

	static CFogVolume *FindFogVolumeForPosition( const Vector &position );

	const char *GetFogControllerName() const 
	{
		return STRING( m_fogName );
	}

	CFogController* GetFogController( ) const
	{
		return m_hFogController.Get();
	}

	CPostProcessController* GetPostProcessController( ) const
	{
		return m_hPostProcessController.Get();
	}

	CColorCorrection* GetColorCorrectionController( ) const
	{
		return m_hColorCorrectionController.Get();
	}

	void InputEnable( inputdata_t &data );
	void InputDisable( inputdata_t &data );

private:
	string_t m_fogName;
	string_t m_postProcessName;
	string_t m_colorCorrectionName;

	CHandle< CFogController > m_hFogController;
	CHandle< CPostProcessController > m_hPostProcessController;
	CHandle< CColorCorrection > m_hColorCorrectionController;

	bool m_bDisabled;
	bool m_bInFogVolumesList;

	void AddToGlobalList();
	void RemoveFromGlobalList();
};

extern CUtlVector< CFogVolume * > TheFogVolumes;


#endif // FOG_VOLUME_H