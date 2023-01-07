#ifndef PROP_LASER_CATCHER_H
#define PROP_LASER_CATCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "env_portal_laser.h"
#include "Sprite.h"
#include "props.h"

class CEnvLaserTarget : public CBaseAnimating
{
public:
	DECLARE_CLASS(CEnvLaserTarget, CBaseAnimating);
	DECLARE_DATADESC();

	CEnvLaserTarget(void);
	~CEnvLaserTarget();

	void Spawn(void);
	void Precache(void);

	void TurnOn(CBaseEntity* pLaser);
	void TurnOff(CBaseEntity* pLaser);

private:
	COutputEvent m_OnPowered;
	COutputEvent m_OnUnpowered;
};

class CPropLaserCatcher : public CDynamicProp
{
public:
	DECLARE_CLASS(CPropLaserCatcher, CDynamicProp);

	CPropLaserCatcher(void);

	void Spawn(void);
	void Precache(void);
	virtual void Activate(void);
	virtual void AnimateThink(void);
	void TurnOn(CBaseEntity* pLaser);
	void TurnOff(CBaseEntity* pLaser);



	DECLARE_DATADESC();
private:
	int m_nCatcherType;
	int m_nLaserTarget;
	CEnvLaserTarget* m_pLaserTarget;
	CSprite* m_pGlowSprite1;
	CSprite* m_pGlowSprite2;
	bool m_bIsPowered;
	// Inputs / Outputs
	COutputEvent m_OnPowered;
	COutputEvent m_OnUnpowered;
};

#endif //PROP_LASER_CATCHER_H