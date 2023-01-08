//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A entity casting an beam through portals and hurts everything in it's way
// Usage: Place infront of an prop
// TODO: Make it an standard prop that does this so that the level desginer does not have to make an seperate prop
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "dbg.h"
#include "env_portal_laser.h"
#include "Sprite.h"
#include "physicsshadowclone.h"		// For translating hit entities shadow clones to real ent
#include "props.h"				// CPhysicsProp base class
#include "baseanimating.h"
#include "prop_laser_catcher.h"
#include "prop_weightedcube.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(env_portal_laser, CEnvPortalLaser);
#define LASER_ATTACHMENT 1

BEGIN_DATADESC(CEnvPortalLaser)

DEFINE_KEYFIELD(m_iszLaserTarget, FIELD_STRING, "LaserTarget"),
//DEFINE_FIELD(m_pSprite, FIELD_CLASSPTR),
DEFINE_FIELD(m_pBeam, FIELD_CLASSPTR),
DEFINE_FIELD(m_firePosition, FIELD_VECTOR),
DEFINE_KEYFIELD(m_flStartFrame, FIELD_FLOAT, "framestart"),

// Function Pointers
DEFINE_FUNCTION(StrikeThink),

// Input functions
DEFINE_INPUTFUNC(FIELD_VOID, "TurnOn", InputTurnOn),
DEFINE_INPUTFUNC(FIELD_VOID, "TurnOff", InputTurnOff),
DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),

END_DATADESC()

#define LASER_RANGE		8192


CEnvPortalLaser::CEnvPortalLaser(void)
	: m_filterBeams(NULL, NULL, COLLISION_GROUP_DEBRIS)
{
	m_filterBeams.SetPassEntity(this);
	m_filterBeams.SetPassEntity2(UTIL_PlayerByIndex(1));
}

ITraceFilter* CEnvPortalLaser::GetBeamTraceFilter(void)
{
	return &m_filterBeams;
}

CPropWeightedCube* m_pLastCube = nullptr;
CPropLaserCatcher* m_pLastCatcher = nullptr;
CEnvLaserTarget* m_pLastLaserTarget = nullptr;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvPortalLaser::Spawn(void)
{
	m_pLastCube = nullptr;
	m_pLastCatcher = nullptr;
	m_pLastLaserTarget = nullptr;

	if (!GetModelName())
	{
		SetThink(&CEnvPortalLaser::SUB_Remove);
		return;
	}

	SetSolid(SOLID_NONE);							// Remove model & collisions
	SetThink(&CEnvPortalLaser::StrikeThink);

	SetEndWidth(GetWidth());				// Note: EndWidth is not scaled\

	Precache();

	if (GetEntityName() != NULL_STRING && !(m_spawnflags & SF_BEAM_STARTON))
	{
		TurnOff();
	}
	else
	{
		TurnOn();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvPortalLaser::Precache(void)
{
	m_nSiteHalo = PrecacheModel("sprites/purpleglow1.vmt");
	PrecacheMaterial("sprites/purplelaser1.vmt");

	SetModelIndex(PrecacheModel(STRING(GetModelName())));
	if (m_iszSpriteName != NULL_STRING)
		PrecacheModel(STRING(m_iszSpriteName));
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEnvPortalLaser::KeyValue(const char* szKeyName, const char* szValue)
{
	if (FStrEq(szKeyName, "width"))
	{
		SetWidth(atof(szValue));
	}
	else if (FStrEq(szKeyName, "NoiseAmplitude"))
	{
		SetNoise(atoi(szValue));
	}
	else if (FStrEq(szKeyName, "TextureScroll"))
	{
		SetScrollRate(atoi(szValue));
	}
	else if (FStrEq(szKeyName, "texture"))
	{
		SetModelName(AllocPooledString(szValue));
	}
	else
	{
		BaseClass::KeyValue(szKeyName, szValue);
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether the laser is currently active.
//-----------------------------------------------------------------------------
bool CEnvPortalLaser::IsOn(void)
{
	return m_bActive;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvPortalLaser::InputTurnOn(inputdata_t& inputdata)
{
	if (!IsOn())
	{
		TurnOn();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvPortalLaser::InputTurnOff(inputdata_t& inputdata)
{
	if (IsOn())
	{
		TurnOff();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvPortalLaser::InputToggle(inputdata_t& inputdata)
{
	Toggle();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvPortalLaser::TurnOff(void)
{
	if (m_pBeam)
		m_pBeam->AddEffects(EF_NODRAW);

	m_bActive = false;
	SetNextThink(TICK_NEVER_THINK);
	SetThink(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvPortalLaser::TurnOn(void)
{
	//RemoveEffects( EF_NODRAW );
	if (m_pBeam)
		m_pBeam->RemoveEffects(EF_NODRAW);
	m_bActive = true;

	m_flFireTime = gpGlobals->curtime;

	SetThink(&CEnvPortalLaser::StrikeThink);

	//
	// Call StrikeThink here to update the end position, otherwise we will see
	// the beam in the wrong place for one frame since we cleared the nodraw flag.
	//
	StrikeThink();
}

void CEnvPortalLaser::Toggle()
{
	if (IsOn())
	{
		TurnOff();
	}
	else
	{
		TurnOn();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CEnvPortalLaser::FireAtPoint(trace_t& tr)
{

	// Apply damage and do sparks every 1/10th of a second.
	if (gpGlobals->curtime >= m_flFireTime + 0.1)
	{
		BeamDamage(&tr);
		DoSparks(GetAbsStartPos(), tr.endpos);
		CBaseEntity *pHit = tr.m_pEnt;
		m_pHitObject = pHit;
		if (dynamic_cast<CPropWeightedCube*>(pHit) != nullptr) {
			if (dynamic_cast<CPropWeightedCube*>(GetParent()) != nullptr) {
			}
			else {
				CPropWeightedCube* pCube = dynamic_cast<CPropWeightedCube*>(pHit);

				if (m_pLastCube != pCube) {
					if (m_pLastCube != nullptr) m_pLastCube->ToggleLaser(false);
					m_pLastCube = pCube;
				}

				if (m_bActive) {
					if (m_bLastCubeActivated == false) {
						m_bLastCubeActivated = true;
						m_pLastCube->ToggleLaser(true);
					}
				}
				else {
					if (m_bLastCubeActivated == true) {
						m_bLastCubeActivated = false;
						m_pLastCube->ToggleLaser(false);
					}
				}
			}
		}
		else {
			if (m_pLastCube != nullptr) {
				if (m_bLastCubeActivated == true) {
					m_bLastCubeActivated = false;
					m_pLastCube->ToggleLaser(false);
				}
			}
		}

		if (dynamic_cast<CEnvLaserTarget*>(pHit) != nullptr) {
			CEnvLaserTarget* pLaserTarget = dynamic_cast<CEnvLaserTarget*>(pHit);

			if (m_pLastLaserTarget != pLaserTarget) {
				if (m_pLastLaserTarget != nullptr) m_pLastLaserTarget->TurnOff(this);
				m_pLastLaserTarget = pLaserTarget;
			}
			
			if (m_bActive) {
				if (m_bLastLaserTargetActivated == false) {
					m_bLastLaserTargetActivated = true;
					m_pLastLaserTarget->TurnOn(this);
				}
			}
			else {
				if (m_bLastLaserTargetActivated == true) {
					m_bLastLaserTargetActivated = false;
					m_pLastLaserTarget->TurnOff(this);
				}
			}

			CPropLaserCatcher* catcher = dynamic_cast<CPropLaserCatcher*>(m_pLastLaserTarget->GetParent());

			if (m_pLastCatcher != catcher) {
				if (m_pLastCatcher != nullptr) m_pLastCatcher->TurnOff(this);
				m_pLastCatcher = catcher;
			}

			if (m_bActive) {
				if (m_bLastCatcherActivated == false) {
					m_bLastCatcherActivated = true;
					m_pLastCatcher->TurnOn(this);
				}
			}
			else {
				if (m_bLastCatcherActivated == true) {
					m_bLastCatcherActivated = false;
					m_pLastCatcher->TurnOff(this);
				}
			}

		}
		else {
			if (m_pLastLaserTarget != nullptr)
			{
				if (m_bLastLaserTargetActivated == false) {
					m_bLastLaserTargetActivated = true;
					m_pLastLaserTarget->TurnOff(this);
				}
			}

			if (m_pLastCatcher != nullptr) {
				if (m_bLastCatcherActivated == true) {
					m_bLastCatcherActivated = false;
					m_pLastCatcher->TurnOff(this);
				}
			}
		}
		// Add laser impact sprite
		// Thanks to our method it works through portals as well :P
		// not in portal we don't -litevex
		// UTIL_DecalTrace(&tr, "FadingScorch");
	}
}


//-----------------------------------------------------------------------------
// Purpose: Does the damage through portals
//-----------------------------------------------------------------------------
void CEnvPortalLaser::StrikeThink(void)
{
	Vector vecMuzzle = GetAbsOrigin();
	QAngle angMuzzleDir = GetAbsAngles();

	QAngle angAimDir = GetParent()->GetAbsAngles();
	Vector vecAimDir;
	AngleVectors(angAimDir, &vecAimDir);

	if (!m_pBeam)
	{
		m_pBeam = CBeam::BeamCreate("sprites/purplelaser1.vmt", 0.1);
		m_pBeam->SetHaloTexture(m_nSiteHalo);
		m_pBeam->SetColor(100, 100, 255);
		m_pBeam->SetBrightness(100);
		m_pBeam->SetNoise(0);
		m_pBeam->SetWidth(4);
		m_pBeam->SetEndWidth(0);
		m_pBeam->SetScrollRate(0);
		m_pBeam->SetFadeLength(0);
		m_pBeam->SetHaloScale(16.0f);
		m_pBeam->SetCollisionGroup(COLLISION_GROUP_NONE);
		m_pBeam->SetBeamFlag(FBEAM_REVERSED);
		m_pBeam->PointsInit(vecMuzzle + vecAimDir, vecMuzzle);
		m_pBeam->SetStartEntity(this);
	}
	else
	{
		m_pBeam->RemoveEffects(EF_NODRAW);
	}


	// Trace to find an endpoint (so the beam draws through portals)
	Vector vEndPointBeam;
	float fEndFraction;
	Ray_t rayPath;
	rayPath.Init(vecMuzzle, vecMuzzle + vecAimDir * LASER_RANGE);

	if (UTIL_Portal_TraceRay_Beam(rayPath, MASK_SHOT, &m_filterBeams, &fEndFraction))
	{
		vEndPointBeam = vecMuzzle + vecAimDir * LASER_RANGE;	// Trace went through portal and endpoint is unknown
	}
	else
	{
		vEndPointBeam = vecMuzzle + vecAimDir * LASER_RANGE * fEndFraction;	// Trace hit a wall
	}
	m_pBeam->PointsInit(vEndPointBeam, vecMuzzle);

	//m_pBeam->SetHaloScale(LaserEndPointSize());



	// If our facing direction hits our enemy, fire the beam
	Ray_t rayDmg;
	Vector vForward;
	AngleVectors(GetAbsAngles(), &vForward, NULL, NULL);
	Vector vEndPoint = GetAbsOrigin() + vForward * LASER_RANGE;
	rayDmg.Init(GetAbsOrigin(), vEndPoint);
	rayDmg.m_IsRay = true;
	trace_t traceDmg;

	// This version reorients through portals
	CTraceFilterSimple subfilter(this, COLLISION_GROUP_NONE);
	CTraceFilterTranslateClones filter(&subfilter);
	float flRequiredParameter = 2.0f;
	CProp_Portal* pFirstPortal = UTIL_Portal_FirstAlongRay(rayDmg, flRequiredParameter);
	UTIL_Portal_TraceRay_Bullets(pFirstPortal, rayDmg, MASK_VISIBLE_AND_NPCS, &filter, &traceDmg, false);


	//UTIL_TraceLine( GetAbsOrigin(), vecFireAt, MASK_SOLID, NULL, COLLISION_GROUP_NONE, &traceDmg );
	FireAtPoint(traceDmg);
	SetNextThink(gpGlobals->curtime);
}

// PROP LASER EMITTER
LINK_ENTITY_TO_CLASS(prop_laser_emitter, CPropLaserEmitter);

BEGIN_DATADESC(CPropLaserEmitter)

DEFINE_KEYFIELD(m_nEmitterType, FIELD_INTEGER, "EmitterType"),

DEFINE_INPUTFUNC(FIELD_VOID, "TurnOn", InputTurnOn),
DEFINE_INPUTFUNC(FIELD_VOID, "TurnOff", InputTurnOff),
DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),
END_DATADESC()

#define EMITTER_MODEL_RETAIL "models/props/laser_emitter_center.mdl"
#define EMITTER_MODEL "models/props/combine_laser_emitter.mdl"

enum EmitterType {
	STANDARD = 0,
	RETAIL = 1,
};

CPropLaserEmitter::CPropLaserEmitter(void) {}

void CPropLaserEmitter::Precache(void)
{
	PrecacheModel(EMITTER_MODEL);
	PrecacheModel(EMITTER_MODEL_RETAIL);

	BaseClass::Precache();
}

void CPropLaserEmitter::Spawn(void)
{
	Precache();
	switch (m_nEmitterType) {
	case STANDARD:
		SetModel(EMITTER_MODEL);
		SetModelScale(0.65);
		break;
	case RETAIL:
		SetModel(EMITTER_MODEL_RETAIL);
		break;
	}
	SetSolid(SOLID_VPHYSICS);

	m_nLaserOrigin = LookupAttachment("laser_attachment");

	Vector laserOriginVec;
	QAngle laserOriginAng;
	GetAttachment(m_nLaserOrigin, laserOriginVec, laserOriginAng);

	DevMsg("Creating env_portal_laser at %.2f %.2f %.2f \n", laserOriginVec.x, laserOriginVec.y, laserOriginVec.z);
	m_pLaser = dynamic_cast<CEnvPortalLaser*>(CreateEntityByName("env_portal_laser"));
	m_pLaser->SetParent(this);
	m_pLaser->KeyValue("damage", "100");
	m_pLaser->KeyValue("width", "2");
	m_pLaser->KeyValue("texture", "sprites/laserbeam.spr");
	m_pLaser->KeyValue("renderamt", "100");
	m_pLaser->KeyValue("TextureScroll", "35");
	DispatchSpawn(m_pLaser);
	m_pLaser->SetAbsOrigin(laserOriginVec);
	m_pLaser->SetAbsAngles(laserOriginAng);
	m_pLaser->Activate();
	m_pLaser->TurnOn();
}

int CPropLaserEmitter::IsOn(void)
{
	return m_pLaser->IsOn();
}

void CPropLaserEmitter::InputTurnOn(inputdata_t& inputdata)
{
	if (!IsOn()) m_pLaser->TurnOn();
}

void CPropLaserEmitter::InputTurnOff(inputdata_t& inputdata)
{
	if (IsOn()) m_pLaser->TurnOff();
}

void CPropLaserEmitter::InputToggle(inputdata_t& inputdata)
{
	if (IsOn())
	{
		m_pLaser->TurnOff();
	}
	else
	{
		m_pLaser->TurnOn();
	}
}