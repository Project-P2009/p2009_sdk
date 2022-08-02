//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A entity casting an beam through portals and hurts everything in it's way
// Usage: Place infront of an prop
// TODO: Make it an standard prop that does this so that the level desginer does not have to make an seperate prop
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "env_portal_laser.h"
#include "Sprite.h"
#include "physicsshadowclone.h"		// For translating hit entities shadow clones to real ent
#include "props.h"				// CPhysicsProp base class
#include "baseanimating.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( env_portal_laser, CEnvPortalLaser );
#define LASER_ATTACHMENT 1

BEGIN_DATADESC( CEnvPortalLaser )

	DEFINE_KEYFIELD( m_iszLaserTarget, FIELD_STRING, "LaserTarget" ),
	DEFINE_FIELD(m_pSprite, FIELD_CLASSPTR),
	DEFINE_FIELD(m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( m_firePosition, FIELD_VECTOR ),
	DEFINE_KEYFIELD( m_flStartFrame, FIELD_FLOAT, "framestart" ),

	// Function Pointers
	DEFINE_FUNCTION( StrikeThink ),

	// Input functions
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

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


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvPortalLaser::Spawn( void )
{
	if ( !GetModelName() )
	{
		SetThink( &CEnvPortalLaser::SUB_Remove );
		return;
	}

	SetSolid( SOLID_NONE );							// Remove model & collisions
	SetThink( &CEnvPortalLaser::StrikeThink );

	SetEndWidth( GetWidth() );				// Note: EndWidth is not scaled

	Precache( );

	if ( GetEntityName() != NULL_STRING && !(m_spawnflags & SF_BEAM_STARTON) )
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
void CEnvPortalLaser::Precache( void )
{
	m_nSiteHalo = PrecacheModel("sprites/redglow2.vmt");

	SetModelIndex( PrecacheModel( STRING( GetModelName() ) ) );
	if ( m_iszSpriteName != NULL_STRING )
		PrecacheModel( STRING(m_iszSpriteName) );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEnvPortalLaser::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "width"))
	{
		SetWidth( atof(szValue) );
	}
	else if (FStrEq(szKeyName, "NoiseAmplitude"))
	{
		SetNoise( atoi(szValue) );
	}
	else if (FStrEq(szKeyName, "TextureScroll"))
	{
		SetScrollRate( atoi(szValue) );
	}
	else if (FStrEq(szKeyName, "texture"))
	{
		SetModelName( AllocPooledString(szValue) );
	}
	else
	{
		BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether the laser is currently active.
//-----------------------------------------------------------------------------
int CEnvPortalLaser::IsOn( void )
{
	if ( IsEffectActive( EF_NODRAW ) )
		return 0;
	return 1;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvPortalLaser::InputTurnOn( inputdata_t &inputdata )
{
	if (!IsOn())
	{
		TurnOn();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvPortalLaser::InputTurnOff( inputdata_t &inputdata )
{
	if (IsOn())
	{
		TurnOff();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvPortalLaser::InputToggle( inputdata_t &inputdata )
{
	if ( IsOn() )
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
void CEnvPortalLaser::TurnOff( void )
{
	AddEffects( EF_NODRAW );
	if (m_pBeam)
		m_pBeam->TurnOff();

	SetNextThink( TICK_NEVER_THINK );
	SetThink( NULL );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvPortalLaser::TurnOn( void )
{
	RemoveEffects( EF_NODRAW );
	if (m_pBeam)
		m_pBeam->TurnOn();

	m_flFireTime = gpGlobals->curtime;

	SetThink( &CEnvPortalLaser::StrikeThink );

	//
	// Call StrikeThink here to update the end position, otherwise we will see
	// the beam in the wrong place for one frame since we cleared the nodraw flag.
	//
	StrikeThink();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvPortalLaser::FireAtPoint( trace_t &tr )
{

	// Apply damage and do sparks every 1/10th of a second.
	if ( gpGlobals->curtime >= m_flFireTime + 0.1 )
	{
		BeamDamage( &tr );
		DoSparks( GetAbsStartPos(), tr.endpos );

		// Add laser impact sprite
		// Thanks to our method it works through portals as well :P
		// not in portal we don't -litevex
		// UTIL_DecalTrace(&tr, "FadingScorch");
	}
}


//-----------------------------------------------------------------------------
// Purpose: Does the damage through portals
//-----------------------------------------------------------------------------
void CEnvPortalLaser::StrikeThink( void )
{

	Vector vecMuzzle = GetAbsOrigin();
	QAngle angMuzzleDir = GetAbsAngles();

	QAngle angAimDir = GetAbsAngles();
	Vector vecAimDir;
	AngleVectors(angAimDir, &vecAimDir);

	if (!m_pBeam)
	{
		m_pBeam = CBeam::BeamCreate("effects/redlaser1.vmt", 0.1);
		m_pBeam->SetHaloTexture(m_nSiteHalo);
		m_pBeam->SetColor(100, 100, 255);
		m_pBeam->SetBrightness(100);
		m_pBeam->SetNoise(0);
		m_pBeam->SetWidth(2);
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
		vEndPointBeam = vecMuzzle + vecAimDir * LASER_RANGE;	// Trace went through portal and endpoint is unknown
	else
		vEndPointBeam = vecMuzzle + vecAimDir * LASER_RANGE * fEndFraction;	// Trace hit a wall

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
	SetNextThink( gpGlobals->curtime );
}
