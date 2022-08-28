//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ENVPORTALLASER_H
#define ENVPORTALLASER_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "beam_shared.h"
#include "entityoutput.h"
#include "props.h"				// CPhysicsProp base class


class CSprite;


class CEnvPortalLaser : public CBeam
{
	DECLARE_CLASS( CEnvPortalLaser, CBeam );
public:
	CEnvPortalLaser(void);

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
#ifdef MAPBASE
	void InputSetTarget( inputdata_t &inputdata ) { m_iszLaserTarget = inputdata.value.StringID(); }
#endif
	DECLARE_DATADESC();

	string_t m_iszLaserTarget;	// Name of entity or entities to strike at, randomly picked if more than one match.
	CSprite *m_pSprite;
	CBeam* m_pBeam;
	string_t m_iszSpriteName;
	Vector  m_firePosition;

	float	m_flStartFrame;

	int m_nSiteHalo;
private:
	CTraceFilterSkipTwoEntities		m_filterBeams;
	QAngle angAimDir;
};

#endif // ENVPORTALLASER_H
