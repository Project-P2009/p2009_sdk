//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ENVLASER_H
#define ENVLASER_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "beam_shared.h"
#include "entityoutput.h"
#include "props.h"				// CPhysicsProp base class


class CSprite;


class CEnvLaser : public CBeam
{
	DECLARE_CLASS( CEnvLaser, CBeam );
public:

	CEnvLaser(void);

	void	Spawn( void );
	void	Precache( void );
	bool	KeyValue( const char *szKeyName, const char *szValue );

	void	TurnOn( void );
	void	TurnOff( void );
	int		IsOn( void );

	virtual ITraceFilter* GetBeamTraceFilter(void);

	void	FireAtPoint( trace_t &point );
	void	StrikeThink( void );

	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

	DECLARE_DATADESC();

	string_t m_iszLaserTarget;	// Name of entity or entities to strike at, randomly picked if more than one match.
	CBeam* m_pBeam;
	string_t m_iszSpriteName;
	Vector  m_firePosition;

	float	m_flStartFrame;

	int m_nSiteHalo;
private:
	CTraceFilterSkipTwoEntities		m_filterBeams;
	QAngle angAimDir;
};

#endif // ENVLASER_H