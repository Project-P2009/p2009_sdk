#include "cbase.h"
#include "prop_laser_relay.h"

#include <io.h>

#include "soundenvelope.h"

#define RELAY_ACTIVATE_SOUND "LaserCatcher.Activate"
#define RELAY_DEACTIVATE_SOUND "LaserCatcher.Deactivate"
#define RELAY_AMBIENCE_SOUND "world/laser_node_lp_01.wav"
#define PROP_LASER_RELAY_MODEL "models/props/laser_receptacle.mdl"

#define LASER_EMITTER_DEFAULT_SPRITE "sprites/light_glow02_add.vmt"//"sprites/purpleglow1.vmt"
#define LASER_SPRITE_COLOUR 255, 64, 64

// CVar for visuals
// TODO: Finialize visuals and use macros/constants instead!
extern ConVar portal_laser_glow_sprite_colour;
extern ConVar portal_laser_glow_sprite;
extern ConVar portal_laser_glow_sprite_scale;
extern ConVar portal_laser_debug;

static const Vector RELAY_TARGET_RADIUS = { 8, 8, 8 };

LINK_ENTITY_TO_CLASS(func_laser_relay_target, CFuncLaserRelayTarget)
BEGIN_DATADESC(CFuncLaserRelayTarget)
// Think functions
#ifdef DEBUG
	DEFINE_THINKFUNC(DebugThink),
#endif
// Fields
	DEFINE_FIELD(m_pParentRelay, FIELD_CLASSPTR),
// Inputs
	DEFINE_INPUTFUNC(FIELD_BOOLEAN, "SetActive", InputSetActivated),
END_DATADESC()

CFuncLaserRelayTarget::CFuncLaserRelayTarget() { }

void CFuncLaserRelayTarget::Spawn()
{
	BaseClass::Spawn();

	SetBlocksLOS(false); // Disable Block LOS, so laser can go through
	SetSolid(SOLID_VPHYSICS);

	CreateVPhysics();
	SetThink(&CFuncLaserRelayTarget::DebugThink);
	SetNextThink(gpGlobals->curtime);
}

CFuncLaserRelayTarget* CFuncLaserRelayTarget::Create(CPropLaserRelay* parent, Vector origin)
{
	CFuncLaserRelayTarget* pTarget = dynamic_cast<CFuncLaserRelayTarget*>(CreateEntityByName("func_laser_relay_target", -1));
	if (pTarget != nullptr)
	{
		DispatchSpawn(pTarget);
		pTarget->m_pParentRelay = parent;
		char temp[128] = { 0 };
		Q_snprintf(temp, 128, "%s_target", parent->GetDebugName());
		pTarget->SetName(MAKE_STRING(temp));

		pTarget->SetSize(-RELAY_TARGET_RADIUS, RELAY_TARGET_RADIUS);
		pTarget->SetAbsOrigin(origin);
		pTarget->SetAbsAngles(parent->GetAbsAngles());
	}

	return pTarget;
}
void CFuncLaserRelayTarget::SetActivated(bool activated)
{
	if (m_pParentRelay)
	{
		m_pParentRelay->SetActivated(activated);
	}
}
void CFuncLaserRelayTarget::InputSetActivated(inputdata_t& data)
{
	SetActivated(data.value.Bool());
}

void CFuncLaserRelayTarget::DebugThink()
{
	if (portal_laser_debug.GetBool())
	{
		DrawBBoxOverlay();
		SetThink(&CFuncLaserRelayTarget::DebugThink);
		SetNextThink(gpGlobals->curtime);
	}
}

LINK_ENTITY_TO_CLASS(prop_laser_relay, CPropLaserRelay)
BEGIN_DATADESC(CPropLaserRelay)
// Fields
	DEFINE_FIELD(m_pTarget, FIELD_CLASSPTR),
	DEFINE_FIELD(m_pActivatedSprite, FIELD_CLASSPTR),
	DEFINE_FIELD(m_bActivated, FIELD_BOOLEAN),
	DEFINE_FIELD(m_iSpinSeq, FIELD_INTEGER),
	DEFINE_SOUNDPATCH(m_pActiveSound),
// Outputs
	DEFINE_OUTPUT(m_OnPowered, "OnPowered"),
	DEFINE_OUTPUT(m_OnUnpowered, "OnUnpowered"),
END_DATADESC()

void CPropLaserRelay::UpdateOnRemove()
{
	DestroySounds();
	CBaseAnimating::UpdateOnRemove();
}

void CPropLaserRelay::Precache()
{
	PrecacheScriptSound(RELAY_ACTIVATE_SOUND);
	PrecacheScriptSound(RELAY_DEACTIVATE_SOUND);
	PrecacheSound(RELAY_AMBIENCE_SOUND);

	string_t sModel = GetModelName();
	if (sModel != NULL_STRING) {
		PrecacheModel(STRING(sModel));
	}

	BaseClass::Precache();
}

void CPropLaserRelay::Spawn()
{
	BaseClass::Spawn();

	SetMoveType(MOVETYPE_PUSH);
	SetSolid(SOLID_VPHYSICS);

	CreateVPhysics();

	Vector vecOrigin;
	QAngle angDir;

	// Get attachment "laser_target" location and angle
	if (!GetAttachment("laser_target", vecOrigin, angDir))
	{
		// If not found, use absolute origin and angle.
		vecOrigin = GetAbsOrigin();
		angDir = GetAbsAngles();
	}

	// Get direction vector
	Vector vecDir;
	AngleVectors(angDir, &vecDir);
	// Get animation id
	m_iSpinSeq = LookupSequence("spin");
	// Check if the id is valid
	if (m_iSpinSeq >= 0)
	{
		// Play animation
		PropSetSequence(m_iSpinSeq);
	}
	// Pause animation
	SetPlaybackRate(0);

	// Create glow sprite
	m_pActivatedSprite = dynamic_cast<CSprite*>(CreateEntityByName("env_sprite"));
	// Check if it was created successfully
	if (m_pActivatedSprite != nullptr)
	{
		// Get glow sprite path
		const char* szSprite = portal_laser_glow_sprite.GetString();
		// Check if it's valid, otherwise use a default sprite
		if (szSprite == nullptr || Q_strlen(szSprite) == 0)
		{
			szSprite = LASER_EMITTER_DEFAULT_SPRITE;
		}
		// Setup sprite
		m_pActivatedSprite->KeyValue("model", szSprite);
		m_pActivatedSprite->Precache();
		m_pActivatedSprite->SetParent(this);
		DispatchSpawn(m_pActivatedSprite);
		m_pActivatedSprite->SetAbsOrigin(vecOrigin);
		m_pActivatedSprite->SetRenderMode(kRenderWorldGlow);

		// Get colour
		const char* szColour = portal_laser_glow_sprite_colour.GetString();
		// Check if colour is correct, then apply it
		if (szColour != nullptr && Q_strlen(szColour) > 0)
		{
			int r, g, b;
			(void)sscanf(szColour, "%i%i%i", &r, &g, &b);
			m_pActivatedSprite->SetRenderColor(static_cast<byte>(r), static_cast<byte>(g), static_cast<byte>(b));
		}
		// If it's not correct, use a default colour
		else
		{
			m_pActivatedSprite->SetRenderColor(LASER_SPRITE_COLOUR);
		}
		m_pActivatedSprite->SetScale(portal_laser_glow_sprite_scale.GetFloat());
		// Turn if off by default
		m_pActivatedSprite->TurnOff();
	}

	// Create target entity, that is detected by a trigger attached to the laser emitter.
	m_pTarget = CFuncLaserRelayTarget::Create(this, vecOrigin);
	// Validate entity
	ASSERT(m_pTarget != nullptr);
	if (m_pTarget == nullptr)
	{
		Warning("Failed to create target for laser_relay \"%s\"\n", GetDebugName());
	}
}

void CPropLaserRelay::SetActivated(bool activated)
{
	if (activated)
	{
		EmitSound(RELAY_ACTIVATE_SOUND);
		CreateSounds();
		SetPlaybackRate(1);
		if (m_pActivatedSprite)
		{
			m_pActivatedSprite->TurnOn();
		}
		m_OnPowered.FireOutput(nullptr, nullptr);
	}
	else
	{
		EmitSound(RELAY_DEACTIVATE_SOUND);
		DestroySounds();
		SetPlaybackRate(0);
		if (m_pActivatedSprite)
		{
			m_pActivatedSprite->TurnOff();
		}
		m_OnUnpowered.FireOutput(nullptr, nullptr);
	}
}

void CPropLaserRelay::CreateSounds()
{
	CSoundEnvelopeController& controller = CSoundEnvelopeController::GetController();

	CPASAttenuationFilter filter(this);
	if (!m_pActiveSound)
	{
		m_pActiveSound = controller.SoundCreate(filter, entindex(), RELAY_AMBIENCE_SOUND);
	}

	controller.Play(m_pActiveSound, 1, 100);
}

void CPropLaserRelay::DestroySounds()
{
	CSoundEnvelopeController& controller = CSoundEnvelopeController::GetController();

	controller.SoundDestroy(m_pActiveSound);
	m_pActiveSound = nullptr;
}

bool CPropLaserRelay::CreateVPhysics()
{
	return BaseClass::CreateVPhysics();
}
