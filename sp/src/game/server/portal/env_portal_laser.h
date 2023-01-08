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
	DECLARE_CLASS(CEnvPortalLaser, CBeam);
public:
	CEnvPortalLaser(void);

	void	Spawn(void);
	void	Precache(void);
	bool	KeyValue(const char* szKeyName, const char* szValue);

	void	Toggle(void);
	void	TurnOn(void);
	void	TurnOff(void);
	bool		IsOn(void);

	virtual ITraceFilter* GetBeamTraceFilter(void);

	void	FireAtPoint(trace_t& point);
	void	StrikeThink(void);

	void InputTurnOn(inputdata_t& inputdata);
	void InputTurnOff(inputdata_t& inputdata);
	void InputToggle(inputdata_t& inputdata);

	DECLARE_DATADESC();

	string_t m_iszLaserTarget;	// Name of entity or entities to strike at, randomly picked if more than one match.
	//CSprite *m_pSprite;
	CBeam* m_pBeam;
	string_t m_iszSpriteName;
	Vector  m_firePosition;

	float	m_flStartFrame;

	CBaseEntity* m_pHitObject;
	int m_nSiteHalo;
private:
	CTraceFilterSkipTwoEntities		m_filterBeams;
	QAngle angAimDir;
	bool m_bActive = false;
	bool m_bLastCatcherActivated = false;
	bool m_bLastLaserTargetActivated = false;
	bool m_bLastCubeActivated = false;
};

// PROP LASER EMITTER
class CPropLaserEmitter : public CBaseAnimating
{
public:
	DECLARE_CLASS(CPropLaserEmitter, CBaseAnimating);

	CPropLaserEmitter(void);

	void Spawn(void);
	void Precache(void);

	int IsOn(void);

	DECLARE_DATADESC();
	void InputTurnOn(inputdata_t& inputdata);
	void InputTurnOff(inputdata_t& inputdata);
	void InputToggle(inputdata_t& inputdata);

private:
	int m_nEmitterType;
	int m_nLaserOrigin;
	CEnvPortalLaser* m_pLaser;
};

#endif // ENVPORTALLASER_H