#include "cbase.h"
#include "prop_laser_catcher.h"
#include "soundenvelope.h"

#define CATCHER_ACTIVATE_SOUND "LaserCatcher.Activate"
#define CATCHER_DEACTIVATE_SOUND "LaserCatcher.Deactivate"
#define CATCHER_AMBIENCE_SOUND "world/laser_node_lp_01.wav"

#define LASER_EMITTER_DEFAULT_SPRITE "sprites/light_glow02_add.vmt"//"sprites/purpleglow1.vmt"
#define LASER_SPRITE_COLOUR 255, 64, 64

// CVar for visuals
// TODO: Finialize visuals and use macros/constants instead!
extern ConVar portal_laser_glow_sprite_colour;
extern ConVar portal_laser_glow_sprite;
extern ConVar portal_laser_glow_sprite_scale;

ConVar portal_laser_catcher_detector_size("portal_laser_catcher_detector_size", "12", FCVAR_CHEAT, "Set the laser catcher's detector size.");

LINK_ENTITY_TO_CLASS(func_laser_detect, CFuncLaserDetector)

BEGIN_DATADESC(CFuncLaserDetector)
// Fields
	DEFINE_FIELD(m_pProp, FIELD_EHANDLE),
// Key fields
	DEFINE_KEYFIELD(m_szIdleAnimation, FIELD_STRING, "idle_anim"),
	DEFINE_KEYFIELD(m_szActiveAnimation, FIELD_STRING, "active_anim"),
	DEFINE_KEYFIELD(m_bAnimateOnActive, FIELD_BOOLEAN, "animate_active"),
	DEFINE_KEYFIELD(m_szPropEntity, FIELD_STRING, "connected_prop"),
// Think functions
	DEFINE_THINKFUNC(DebugThink),
// Outputs
	DEFINE_OUTPUT(m_OnPowered, "OnPowered"),
	DEFINE_OUTPUT(m_OnUnpowered, "OnUnpowered"),
END_DATADESC()

extern ConVar portal_laser_debug;

void CFuncLaserDetector::Spawn() {
	BaseClass::Spawn();

	SetSolid(SOLID_OBB);

	if (m_szPropEntity != NULL) {
		m_pProp = gEntList.FindEntityByName(NULL, m_szPropEntity);
	}

	SetThink(&CFuncLaserDetector::DebugThink);
	SetNextThink(gpGlobals->curtime);
}

CFuncLaserDetector::~CFuncLaserDetector() {
	m_LaserList.Purge();
}

void CFuncLaserDetector::Precache() {
	PrecacheScriptSound(CATCHER_ACTIVATE_SOUND);
	PrecacheScriptSound(CATCHER_DEACTIVATE_SOUND);
	PrecacheSound(CATCHER_AMBIENCE_SOUND);

	BaseClass::Precache();
}

void CFuncLaserDetector::AddEmitter(CBaseEntity* emitter) {
	// Store previous list count
	int oldCount = m_LaserList.Count();

	// Check if the emitter has not been added already.
	if (!m_LaserList.HasElement(emitter)) {
		// Add laser emitter
		m_LaserList.AddToTail(emitter);
	}

	// Check if we added any laser emitters
	if (oldCount == 0 && m_LaserList.Count() > 0) {
		// Play activated sound
		EmitSound(CATCHER_ACTIVATE_SOUND);
		// Fire output event
		m_OnPowered.FireOutput(NULL, NULL);

		// Check if the detector has a parent catcher.
		if (m_pProp != NULL && FClassnameIs(m_pProp, "prop_laser_catcher")) {
			CPropLaserCatcher* catcher = dynamic_cast<CPropLaserCatcher*>(m_pProp.Get());
			if (catcher) {
				// Fire "PowerOn" output of the parent prop.
				catcher->FirePowerOnOutput();
			}
		}
		// Create looping "active" sound
		CreateSounds();
	}
}

void CFuncLaserDetector::RemoveEmitter(CBaseEntity* emitter) {
	// Check if the emitter is on the list, then remove it.
	if (m_LaserList.HasElement(emitter)) {
		m_LaserList.FindAndRemove(emitter);
	}

	if (m_LaserList.Count() == 0) {
		EmitSound(CATCHER_DEACTIVATE_SOUND);
		m_OnUnpowered.FireOutput(NULL, NULL);
		// Check if the detector has a parent catcher.
		if (m_pProp != NULL && FClassnameIs(m_pProp, "prop_laser_catcher")) {
			CPropLaserCatcher* catcher = dynamic_cast<CPropLaserCatcher*>(m_pProp.Get());
			if (catcher) {
				// Fire "PowerOff" output of the parent prop.
				catcher->FirePowerOffOutput();
			}
		}
		// Stop looping "active" sound
		DestroySounds();
	}
}

void CFuncLaserDetector::CreateSounds() {
	CSoundEnvelopeController& controller = CSoundEnvelopeController::GetController();

	CPASAttenuationFilter filter(this);
	if (!m_pActiveSound) {
		m_pActiveSound = controller.SoundCreate(filter, entindex(), CATCHER_AMBIENCE_SOUND);
	}

	controller.Play(m_pActiveSound, 1, 100);
}

void CFuncLaserDetector::DestroySounds() {
	CSoundEnvelopeController& controller = CSoundEnvelopeController::GetController();

	controller.SoundDestroy(m_pActiveSound);
	m_pActiveSound = NULL;
}

bool CFuncLaserDetector::IsActivated() const {
	return m_bActivated;
}

CFuncLaserDetector* CFuncLaserDetector::Create(const Vector& origin, const QAngle& angles, const Vector& mins, const Vector& maxs, CBaseEntity* owner) {
	// Create detector entity
	CFuncLaserDetector* pEnt = dynamic_cast<CFuncLaserDetector*>(CreateEntityByName("func_laser_detect"));
	if (pEnt == NULL) {
		return NULL;
	}

	// Set detector bounds
	UTIL_SetSize(pEnt, mins, maxs);
	// Set parent
	pEnt->m_pProp = owner;
	pEnt->SetParent(owner);
	DispatchSpawn(pEnt);

	// Set location and rotation
	pEnt->SetAbsOrigin(origin);
	pEnt->SetAbsAngles(angles);

	return pEnt;
}

void CFuncLaserDetector::DebugThink() {
	if (portal_laser_debug.GetBool()) {
		DrawBBoxOverlay(1);

		NDebugOverlay::Axis(GetAbsOrigin(), GetAbsAngles(), 16, false, 1);
	}

	SetNextThink(gpGlobals->curtime + 1);
}

// ==== Catcher ====

LINK_ENTITY_TO_CLASS(prop_laser_catcher, CPropLaserCatcher);

BEGIN_DATADESC(CPropLaserCatcher)
// Fields
	DEFINE_FIELD(m_pLaserDetector, FIELD_CLASSPTR),
	DEFINE_FIELD(m_pActivatedSprite, FIELD_CLASSPTR),
// Key fields
	DEFINE_KEYFIELD(m_szIdleAnimation, FIELD_STRING, "idle_anim"),
	DEFINE_KEYFIELD(m_szActiveAnimation, FIELD_STRING, "active_anim"),
// Outputs
	DEFINE_OUTPUT(m_OnPowered, "OnPowered"),
	DEFINE_OUTPUT(m_OnUnpowered, "OnUnpowered"),
END_DATADESC()

void CPropLaserCatcher::Precache() {
	PrecacheScriptSound(CATCHER_ACTIVATE_SOUND);
	PrecacheScriptSound(CATCHER_DEACTIVATE_SOUND);
	PrecacheSound(CATCHER_AMBIENCE_SOUND);

	BaseClass::Precache();
}

CPropLaserCatcher::CPropLaserCatcher(): BaseClass() { }

void CPropLaserCatcher::Spawn() {
	BaseClass::Spawn();

	Vector vecOrigin;
	QAngle angDir;

	if (!GetAttachment("laser_target", vecOrigin, angDir)) {
		vecOrigin = GetAbsOrigin();
	}
	angDir = GetAbsAngles();

	Vector vecDir;
	AngleVectors(angDir, &vecDir);

	m_bHoldAnimation = true;

	m_pLaserDetector = CFuncLaserDetector::Create(
		vecOrigin - vecDir * portal_laser_catcher_detector_size.GetFloat() * 0.5f, angDir,
		Vector(-portal_laser_catcher_detector_size.GetFloat()), Vector(portal_laser_catcher_detector_size.GetFloat()),
		this
	);

	m_pActivatedSprite = dynamic_cast<CSprite*>(CreateEntityByName("env_sprite"));
	if (m_pActivatedSprite != NULL) {
		if (!GetAttachment("laser_target", vecOrigin, angDir)) {
			vecOrigin = GetAbsOrigin();
		}
		Vector vecDir;
		AngleVectors(GetAbsAngles(), &vecDir);
		vecOrigin += vecDir * 6; // Adjust position, so the sprite is not clipping into the model.

		const char* szSprite = portal_laser_glow_sprite.GetString();
		if (szSprite == NULL || Q_strlen(szSprite) == 0) {
			szSprite = LASER_EMITTER_DEFAULT_SPRITE;
		}
		m_pActivatedSprite->KeyValue("model", szSprite);
		m_pActivatedSprite->Precache();
		m_pActivatedSprite->SetParent(this);
		DispatchSpawn(m_pActivatedSprite);
		m_pActivatedSprite->SetAbsOrigin(vecOrigin);
		m_pActivatedSprite->SetRenderMode(kRenderWorldGlow);

		const char* szColor = portal_laser_glow_sprite_colour.GetString();
		if (szColor != NULL && Q_strlen(szColor) > 0) {
			int r, g, b;
			sscanf(szColor, "%i%i%i", &r, &g, &b);
			m_pActivatedSprite->SetRenderColor(r, g, b);
		} else {
			m_pActivatedSprite->SetRenderColor(LASER_SPRITE_COLOUR);
		}
		m_pActivatedSprite->SetScale(portal_laser_glow_sprite_scale.GetFloat());
		m_pActivatedSprite->TurnOff();
	}

	// Setup default animations if not specified
	if (m_szIdleAnimation == NULL) {
		m_szIdleAnimation = "idle";
	}
	if (m_szActiveAnimation == NULL) {
		m_szActiveAnimation = "spin";
	}

	SetSolid(SOLID_VPHYSICS);

	CreateVPhysics();
}

void CPropLaserCatcher::FirePowerOnOutput() {
	if (m_szActiveAnimation != NULL) {
		inputdata_t input;
		input.value.SetString(MAKE_STRING(m_szActiveAnimation));
		InputSetAnimation(input);
	}

	if (m_pActivatedSprite != NULL) {
		m_pActivatedSprite->TurnOn();
	}
	m_nSkin = 1;
	m_OnPowered.FireOutput(NULL, NULL);
}

void CPropLaserCatcher::FirePowerOffOutput() {
	if (m_szIdleAnimation != NULL) {
		inputdata_t input;
		input.value.SetString(MAKE_STRING(m_szIdleAnimation));
		InputSetAnimation(input);
	}

	if (m_pActivatedSprite != NULL) {
		m_pActivatedSprite->TurnOff();
	}
	m_nSkin = 0;
	m_OnUnpowered.FireOutput(NULL, NULL);
}