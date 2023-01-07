#include "cbase.h"
#include "dbg.h"
#include "props.h"
#include "baseanimating.h"
#include "prop_laser_catcher.h"

#include "tier0/memdbgon.h"

// LASER TARGET
class CLaserTargetList : public CAutoGameSystem
{
public:
	CLaserTargetList(char const* name) : CAutoGameSystem(name)
	{
	}

	virtual void LevelShutdownPostEntity()
	{
		Clear();
	}

	void Clear()
	{
		m_list.Purge();
	}

	void AddToList(CEnvLaserTarget* pLaserTarget);
	void RemoveFromList(CEnvLaserTarget* pLaserTarget);

	CUtlVector<CEnvLaserTarget* >	m_list;
};

void CLaserTargetList::AddToList(CEnvLaserTarget* pLaserTarget)
{
	m_list.AddToTail(pLaserTarget);
}

void CLaserTargetList::RemoveFromList(CEnvLaserTarget* pLaserTarget)
{
	int index = m_list.Find(pLaserTarget);
	if (index != m_list.InvalidIndex())
	{
		m_list.FastRemove(index);
	}
}

CLaserTargetList g_LaserTargetList("CLaserTargetList");

BEGIN_DATADESC(CEnvLaserTarget)

DEFINE_OUTPUT(m_OnPowered, "OnPowered"),
DEFINE_OUTPUT(m_OnUnpowered, "OnUnpowered"),

END_DATADESC()

LINK_ENTITY_TO_CLASS(env_laser_target, CEnvLaserTarget);



//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CEnvLaserTarget::CEnvLaserTarget(void)
{
	g_LaserTargetList.AddToList(this);
}

CEnvLaserTarget::~CEnvLaserTarget(void)
{
	g_LaserTargetList.RemoveFromList(this);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvLaserTarget::Precache(void)
{
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvLaserTarget::Spawn(void)
{
	Precache();

	// This is a dummy model that is never used!
	UTIL_SetSize(this, Vector(-14, -14, -14), Vector(14, 14, 14));

	SetNextThink(gpGlobals->curtime + 0.1f);

	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);

	AddEffects(EF_NODRAW);

	//Check our water level
	PhysicsCheckWater();

	m_iMaxHealth = GetHealth();
}

void CEnvLaserTarget::TurnOn(CBaseEntity* pLaser)
{
	m_OnPowered.FireOutput(pLaser, this);
}

void CEnvLaserTarget::TurnOff(CBaseEntity* pLaser)
{
	m_OnUnpowered.FireOutput(pLaser, this);
}

enum CatcherType {
	STANDARD = 0,
	RETAIL = 1
};

// CATCHER
LINK_ENTITY_TO_CLASS(prop_laser_catcher, CPropLaserCatcher);

#define CATCHER_RETAIL_MODEL "models/props/laser_catcher_center.mdl"
#define CATCHER_MODEL "models/props/combine_laser_catcher.mdl"
#define CATCHER_ENERGY_GLOW "materials/sprites/glow01.vmt"
#define CATCHER_ENERGY_GLOW2 "materials/sprites/physring1.vmt"
#define CATCHER_ON_SND "Portal.button_down"
#define CATCHER_OFF_SND "coast.combine_apc_shutdown"

BEGIN_DATADESC(CPropLaserCatcher)
	DEFINE_KEYFIELD(m_nCatcherType, FIELD_INTEGER, "CatcherType"),

	DEFINE_THINKFUNC(AnimateThink),

	DEFINE_OUTPUT(m_OnPowered, "OnPowered"),
	DEFINE_OUTPUT(m_OnUnpowered, "OnUnpowered"),
END_DATADESC()

CPropLaserCatcher::CPropLaserCatcher(void)
{

}

void CPropLaserCatcher::Precache(void)
{
	PrecacheModel(CATCHER_MODEL);
	PrecacheModel(CATCHER_RETAIL_MODEL);
	PrecacheMaterial(CATCHER_ENERGY_GLOW);
	PrecacheMaterial(CATCHER_ENERGY_GLOW2);
	PrecacheScriptSound(CATCHER_ON_SND);
	PrecacheScriptSound(CATCHER_OFF_SND);

	BaseClass::Precache();
}

void CPropLaserCatcher::Spawn(void)
{
	Precache();
	switch (m_nCatcherType) {
	case STANDARD:
		SetModel(CATCHER_MODEL);
		break;
	case RETAIL:
		SetModel(CATCHER_RETAIL_MODEL);
		break;
	}
	SetSolid(SOLID_VPHYSICS);

	SetPlaybackRate(0.5);
	ResetSequence(LookupSequence("idle"));

	m_nLaserTarget = LookupAttachment("laser_target");

	Vector laserTargetVec;
	QAngle laserTargetAng;
	GetAttachment(m_nLaserTarget, laserTargetVec, laserTargetAng);

	Msg("Creating env_laser_target at %.2f %.2f %.2f \n", laserTargetVec.x, laserTargetVec.y, laserTargetVec.z);
	m_pLaserTarget = (CEnvLaserTarget*)Create("env_laser_target", laserTargetVec, laserTargetAng);
	m_pLaserTarget->SetParent(this);
	
	if (m_nCatcherType == STANDARD) {
		m_pGlowSprite1 = (CSprite*)Create("env_sprite", laserTargetVec, laserTargetAng);
		m_pGlowSprite2 = (CSprite*)Create("env_sprite", laserTargetVec, laserTargetAng);
		
		m_pGlowSprite1->SetModel(CATCHER_ENERGY_GLOW);
		m_pGlowSprite1->SetRenderMode((RenderMode_t)7);
		m_pGlowSprite1->m_nRenderFX = 15;
		m_pGlowSprite1->SetScale(1);
		m_pGlowSprite1->m_flSpriteFramerate = 30;
		m_pGlowSprite1->SetParent(this);

		m_pGlowSprite2->SetModel(CATCHER_ENERGY_GLOW2);
		m_pGlowSprite2->SetRenderMode((RenderMode_t)9);
		m_pGlowSprite2->m_nRenderFX = 15;
		m_pGlowSprite2->SetScale(1);
		m_pGlowSprite2->m_flSpriteFramerate = 30;
		m_pGlowSprite2->SetParent(this);

		m_pGlowSprite1->TurnOff();
		m_pGlowSprite2->TurnOff();

		SetModelScale(0.65);
	}

	this->m_bIsPowered = false;
	Msg("Laser Catcher Spawned\n");

	BaseClass::Spawn();
}

void CPropLaserCatcher::Activate(void)
{
	BaseClass::Activate();

	SetThink(&CPropLaserCatcher::AnimateThink);
	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CPropLaserCatcher::AnimateThink(void)
{
	// Update our animation
	StudioFrameAdvance();
	DispatchAnimEvents(this);
	m_BoneFollowerManager.UpdateBoneFollowers(this);

	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CPropLaserCatcher::TurnOn(CBaseEntity* pLaser)
{
	EmitSound(CATCHER_ON_SND);

	if (m_nCatcherType == RETAIL) {
		SetSkin(1);
		ResetSequence(LookupSequence("spin"));
	}
	else {
		m_pGlowSprite1->TurnOn();
		m_pGlowSprite2->TurnOn();
	}

	m_OnPowered.FireOutput(pLaser, this);
}

void CPropLaserCatcher::TurnOff(CBaseEntity* pLaser)
{
	EmitSound(CATCHER_OFF_SND);
	if (m_nCatcherType == RETAIL) {
		SetSkin(0);
		ResetSequence(LookupSequence("idle"));
	}
	else {
		m_pGlowSprite1->TurnOff();
		m_pGlowSprite2->TurnOff();
	}
	m_OnUnpowered.FireOutput(pLaser, this);
}