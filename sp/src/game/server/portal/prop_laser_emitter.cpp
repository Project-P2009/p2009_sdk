#include "cbase.h"

#include "beam_shared.h"
#include "props.h"
#include "ndebugoverlay.h"
#include "IEffects.h"
#include "portal_player.h"
#include "soundenvelope.h"

#include "npc_turret_floor.h"

#include "prop_laser_emitter.h"
#include "prop_laser_catcher.h"

#define LASER_BEAM_SPRITE "sprites/purplelaser1.vmt"//"sprites/xbeam2.vmt"
#define LASER_BEAM_COLOUR_CVAR "255 128 128"
#define LASER_BEAM_COLOUR 255, 128, 128
#define LASER_BEAM_WIDTH_CVAR "2"
#define LASER_BEAM_WIDTH 2

#define LASER_EMITTER_DEFAULT_SPRITE "sprites/light_glow02_add.vmt"//"sprites/purpleglow1.vmt"
#define LASER_SPRITE_COLOUR_CVAR "255 64 64"
#define LASER_SPRITE_COLOUR 255, 64, 64
#define LASER_SPRITE_SCALE_CVAR "0.75"
#define LASER_SPRITE_SCALE 0.75f

#define LASER_ACTIVATION_SOUND "Laser.Activate"
#define LASER_AMBIENCE_SOUND "vfx/laser_beam_lp_01.wav"

#define LASER_AMBIENCE_SOUND_VOLUME 0.1f

#define LASER_PUSHBACK_FORCE_CVAR "50"
#define LASER_PUSHBACK_FORCE 50
#define LASER_PUSHBACK_BOX_HALF_RADIUS_CVAR "4"
#define LASER_PUSHBACK_BOX_HALF_RADIUS 4

// CVar for visuals
// TODO: Finialize visuals and use macros/constants instead!
ConVar portal_laser_beam_colour("portal_laser_beam_colour", LASER_BEAM_COLOUR_CVAR, FCVAR_REPLICATED, "Set the colour of the laser beams. Note: You need to reload the map to apply changes!");
ConVar portal_laser_beam_texture("portal_laser_beam_texture", LASER_BEAM_SPRITE, FCVAR_REPLICATED, "Set the texture of the laser beams. Note: You need to reload the map to apply changes!");
ConVar portal_laser_beam_width("portal_laser_beam_width", LASER_BEAM_WIDTH_CVAR, FCVAR_REPLICATED, "Laser beam's width");

ConVar portal_laser_glow_sprite_colour("portal_laser_glow_sprite_colour", LASER_SPRITE_COLOUR_CVAR, FCVAR_REPLICATED, "Set the colour of the laser emitter/catcher glow sprite. Note: You need to reload the map to apply changes!");
ConVar portal_laser_glow_sprite("portal_laser_glow_sprite", LASER_EMITTER_DEFAULT_SPRITE, FCVAR_REPLICATED, "Sprite on the laser emitter/catcher.");
ConVar portal_laser_glow_sprite_scale("portal_laser_glow_sprite_scale", LASER_SPRITE_SCALE_CVAR, FCVAR_REPLICATED, "Laser emitter/catcher sprite scale.");

// ConVar portal_laser_sfx_volume("portal_laser_sfx_volume", "-1", FCVAR_REPLICATED, "Laser's buzzing sound volume. Use -1 for default volume.");

// CVar for player pushback.
ConVar portal_laser_push_force("portal_laser_push_force", "50", FCVAR_CHEAT, "Set the push force of the laser.");
ConVar portal_laser_push_radius("portal_laser_push_radius", "4", FCVAR_CHEAT, "Radius of the pusher box.");

// CVar controling debug mode for lasers.
ConVar portal_laser_debug("portal_laser_debug", "0", FCVAR_CHEAT, "Show laser debug informations.");

LINK_ENTITY_TO_CLASS(env_portal_laser, CEnvPortalLaser)

BEGIN_DATADESC(CEnvPortalLaser)
// Fields
	DEFINE_SOUNDPATCH(m_pLaserSound),
	DEFINE_FIELD(m_bStatus, FIELD_BOOLEAN),
	DEFINE_FIELD(m_pBeam, FIELD_CLASSPTR),
	DEFINE_FIELD(m_pBeamAfterPortal, FIELD_CLASSPTR),
	DEFINE_FIELD(m_pCatcher, FIELD_CLASSPTR),
	DEFINE_FIELD(m_iSprite, FIELD_INTEGER),
// Key fields
	DEFINE_KEYFIELD(m_bStartActive, FIELD_BOOLEAN, "startactive"),
	DEFINE_KEYFIELD(m_fPlayerDamage, FIELD_FLOAT, "playerdamage"),
// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "TurnOn", InputTurnOn),
	DEFINE_INPUTFUNC(FIELD_VOID, "TurnOff", InputTurnOff),
	DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),
// Think functions
	DEFINE_THINKFUNC(LaserThink),
END_DATADESC()

CEnvPortalLaser::~CEnvPortalLaser() {
	if (m_pBeam) {
		UTIL_Remove(m_pBeam);
	}
	if (m_pBeamAfterPortal) {
		UTIL_Remove(m_pBeamAfterPortal);
	}

	DestroySounds();
}

CEnvPortalLaser::CEnvPortalLaser() : m_pBeam(NULL), m_pBeamAfterPortal(NULL), m_pCatcher(NULL), m_fPlayerDamage(1), BaseClass() { }

void CEnvPortalLaser::Precache() {
	PrecacheScriptSound(LASER_ACTIVATION_SOUND);
	PrecacheScriptSound("Player.BurnPain");
	PrecacheSound(LASER_AMBIENCE_SOUND);

	BaseClass::Precache();
}

void CEnvPortalLaser::Spawn() {
	BaseClass::Spawn();

	if (m_bStartActive) {
		TurnOn();
	} else {
		TurnOff();
	}
}

void CEnvPortalLaser::LaserThink() {
	if (!m_bStatus) {
		TurnOff();
		return;
	}

	Vector vecStart, vecEnd;
	Vector vecPortalIn, vecPortalOut;
	Vector vecDir, traceDir;
	QAngle angDir = GetAbsAngles();
	trace_t tr;
	AngleVectors(GetAbsAngles(), &vecDir);

	if (portal_laser_debug.GetBool()) {
		NDebugOverlay::Axis(GetAbsOrigin(), angDir, 16, false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
	}

	// Try to turn on the lasers again if the beam failed to be created.
	if ((m_pBeam == NULL && m_bStatus) || (m_pBeamAfterPortal == NULL && m_bStatus)) {
		TurnOn();
	}

	m_pBeam->PointsInit(GetAbsOrigin(), GetAbsOrigin() + vecDir * MAX_TRACE_LENGTH);
	if (UTIL_Portal_Trace_Beam(m_pBeam, vecStart, vecEnd, vecPortalIn, vecPortalOut, MASK_BLOCKLOS, NULL)) {
		m_pBeam->PointsInit(vecStart, vecStart + vecDir * MAX_TRACE_LENGTH);

		traceDir = (vecEnd - vecPortalOut).Normalized();
		UTIL_TraceLine(vecPortalOut, vecPortalOut + (traceDir * MAX_TRACE_LENGTH), MASK_BLOCKLOS, NULL, &tr);
		HandlePlayerKnockback(vecDir, vecStart);
		HandlePlayerKnockback(traceDir, vecPortalOut);

		m_pBeam->PointsInit(vecStart, vecPortalIn);
		m_pBeamAfterPortal->PointsInit(vecPortalOut, vecEnd);
		m_pBeamAfterPortal->RemoveEffects(EF_NODRAW);

		if (portal_laser_debug.GetBool()) {
			NDebugOverlay::Line(vecStart, vecPortalIn, 0xFF, 0xFF, 0x00, false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
			NDebugOverlay::Line(vecPortalOut, vecEnd, 0xFF, 0xFF, 0x00, false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
		}
	} else {
		traceDir = vecDir;
		UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() + (traceDir * MAX_TRACE_LENGTH), MASK_BLOCKLOS, NULL, &tr);
		HandlePlayerKnockback(vecDir, GetAbsOrigin());

		m_pBeamAfterPortal->AddEffects(EF_NODRAW);
		m_pBeam->PointsInit(GetAbsOrigin(), tr.endpos);
		if (portal_laser_debug.GetBool()) {
			NDebugOverlay::Line(tr.startpos, tr.endpos, 0xFF, 0xFF, 0x00, false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
		}
	}

	bool bSparks = true;

	if (tr.m_pEnt) {
		// Check if we hit a laser detector
		if (FClassnameIs(tr.m_pEnt, "func_laser_detect")) {
			if (portal_laser_debug.GetBool()) {
				NDebugOverlay::Cross3D(tr.endpos, 16, 0xFF, 0x00, 0x00, false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
			}

			CFuncLaserDetector* pCatcher = dynamic_cast<CFuncLaserDetector*>(tr.m_pEnt);
			// Check if casting to catcher successed
			if (pCatcher) {
				// Check if the catcher is different
				if (m_pCatcher != pCatcher) {
					// Remove this emitter from the old catcher
					if (m_pCatcher != NULL) {
						m_pCatcher->RemoveEmitter(this);
					}
					// Add this emitter to the new catcher
					pCatcher->AddEmitter(this);
					// Keep track of the new catcher
					m_pCatcher = pCatcher;
				}
			}
			// Don't display impact sparks on catchers.
			bSparks = false;
		} else {
			// If we did not hit a laser detector, check if we did in the past and remove this emitter.
			if (m_pCatcher != NULL) {
				m_pCatcher->RemoveEmitter(this);
				m_pCatcher = NULL;
			}

			// Check if we hit a turret
			if (FClassnameIs(tr.m_pEnt, "npc_turret_floor") || FClassnameIs(tr.m_pEnt, "npc_portal_turret_floor")) {
				CNPC_FloorTurret* pTurret = dynamic_cast<CNPC_FloorTurret*>(tr.m_pEnt);
				if (pTurret != NULL) {
					pTurret->SelfDestruct();
				}
			} else if (FClassnameIs(tr.m_pEnt, "npc_personality_core")) {
				// TODO: Cast tr.m_pEnt to "CNPC_PersonalityCore", then call "ReactToLaser" function!
				// NOTE: Cores are not finsihed yet and part of master!
			}
		}

		// Display cross at hit position.
		if (portal_laser_debug.GetBool()) {
			NDebugOverlay::Cross3D(tr.endpos, 16, 0xFF, 0x00, 0x00, false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
		}
	}

	if (bSparks) {
		g_pEffects->MetalSparks(tr.endpos, -traceDir);
	}
	SetNextThink(gpGlobals->curtime);
}

void CEnvPortalLaser::TurnOn() {
	if (!m_bStatus) {
		Vector vecDir, vecOrigin = GetAbsOrigin();
		QAngle angDir = GetAbsAngles();
		AngleVectors(angDir, &vecDir);

		// Create laser beam
		if (m_pBeam == NULL) {
			m_pBeam = (CBeam*)CreateEntityByName("beam");
			if (m_pBeam) {
				m_pBeam->SetBeamTraceMask(MASK_BLOCKLOS);

				// Get laser beam sprite
				const char* szSprite = portal_laser_beam_texture.GetString();
				if (szSprite == NULL || Q_strlen(szSprite) == 0) {
					szSprite = LASER_BEAM_SPRITE;
				}
				m_pBeam->BeamInit(szSprite, portal_laser_beam_width.GetFloat());

				// Get laser colour
				int r = 0xFF, g = 0x00, b = 0x00;
				const char* szColours = portal_laser_beam_colour.GetString();
				if (szColours != NULL && Q_strlen(szColours) > 0) {
					sscanf(szColours, "%i%i%i", &r, &g, &b);
				}

				m_pBeam->SetColor(r, g, b);
				m_pBeam->SetBrightness(255);
				m_pBeam->SetCollisionGroup(COLLISION_GROUP_DEBRIS);
				m_pBeam->PointsInit(vecOrigin, vecOrigin + vecDir * MAX_TRACE_LENGTH);
				m_pBeam->SetStartEntity(this);
			} else {
				Warning("Failed to create beam!");
			}
		} else {
			m_pBeam->RemoveEffects(EF_NODRAW);
		}
		// Create a secondary beam that shows after the laser goes through a portal.
		// This wouldn't be nessecary, but if a laser goes through glass, the beam won't render beyond the portal.
		if (m_pBeamAfterPortal == NULL) {
			m_pBeamAfterPortal = (CBeam*)CreateEntityByName("beam");
			if (m_pBeamAfterPortal) {
				m_pBeamAfterPortal->SetBeamTraceMask(MASK_BLOCKLOS);

				// Get laser beam sprite
				const char* szSprite = portal_laser_beam_texture.GetString();
				if (szSprite == NULL || Q_strlen(szSprite) == 0) {
					szSprite = LASER_BEAM_SPRITE;
				}
				m_pBeamAfterPortal->BeamInit(szSprite, portal_laser_beam_width.GetFloat());

				// Get laser colour
				int r = 0xFF, g = 0x00, b = 0x00;
				const char* szColours = portal_laser_beam_colour.GetString();
				if (szColours != NULL && Q_strlen(szColours) > 0) {
					sscanf(szColours, "%i%i%i", &r, &g, &b);
				}

				m_pBeamAfterPortal->SetColor(r, g, b);
				m_pBeamAfterPortal->SetBrightness(255);
				m_pBeamAfterPortal->SetCollisionGroup(COLLISION_GROUP_DEBRIS);
				m_pBeamAfterPortal->PointsInit(vecOrigin, vecOrigin + vecDir * MAX_TRACE_LENGTH);
				m_pBeamAfterPortal->SetStartEntity(this);
				m_pBeamAfterPortal->AddEffects(EF_NODRAW);
			} else {
				Warning("Failed to create beam!");
			}
		}

		m_bStatus = true;

		EmitSound(LASER_ACTIVATION_SOUND);
		CreateSounds();

		SetThink(&CEnvPortalLaser::LaserThink);
		SetNextThink(gpGlobals->curtime);
	}
}

void CEnvPortalLaser::TurnOff() {
	if (m_bStatus) {
		if (m_pBeam) {
			m_pBeam->AddEffects(EF_NODRAW);
		}
		if (m_pBeamAfterPortal) {
			m_pBeamAfterPortal->AddEffects(EF_NODRAW);
		}

		if (m_pCatcher) {
			m_pCatcher->RemoveEmitter(this);
			m_pCatcher = NULL;
		}

		m_bStatus = false;
		DestroySounds();

		SetThink(NULL);
	}
}

void CEnvPortalLaser::Toggle() {
	if (m_bStatus) {
		TurnOff();
	} else {
		TurnOn();
	}
}

void CEnvPortalLaser::CreateSounds() {
	CSoundEnvelopeController& controller = CSoundEnvelopeController::GetController();

	CPASAttenuationFilter filter(this);
	if (!m_pLaserSound) {
		m_pLaserSound = controller.SoundCreate(filter, entindex(), LASER_AMBIENCE_SOUND);
	}

	//float value = portal_laser_sfx_volume.GetFloat();
	controller.Play(m_pLaserSound, /*value > -1 ? value :*/ LASER_AMBIENCE_SOUND_VOLUME, 100);
}

void CEnvPortalLaser::DestroySounds() {
	CSoundEnvelopeController& controller = CSoundEnvelopeController::GetController();

	controller.SoundDestroy(m_pLaserSound);
	m_pLaserSound = NULL;
}

void CEnvPortalLaser::InputTurnOn(inputdata_t& data) {
	TurnOn();
}

void CEnvPortalLaser::InputTurnOff(inputdata_t& data) {
	TurnOff();
}

void CEnvPortalLaser::InputToggle(inputdata_t& data) {
	Toggle();
}

#define LASER_PLAYER_PUSHER_TRACE_COUNT 8

void CEnvPortalLaser::HandlePlayerKnockback(const Vector& vecDir, const Vector& vecStart) {
	QAngle angDir;
	Vector vecForward, vecRight, vecUp;
	trace_t tr;

	// Get angle
	VectorAngles(vecDir, angDir);
	// Calculate the forward, right and up vector from the calculated angle.
	AngleVectors(angDir, &vecForward, &vecRight, &vecUp);

	// Precalculate trace origins
	Vector vecOrigins[LASER_PLAYER_PUSHER_TRACE_COUNT] = {
		vecStart + (vecRight * portal_laser_push_radius.GetFloat()) + (vecUp * portal_laser_push_radius.GetFloat()), // Top-right
		vecStart + (vecRight * portal_laser_push_radius.GetFloat()) - (vecUp * portal_laser_push_radius.GetFloat()), // Bottom-right
		vecStart - (vecRight * portal_laser_push_radius.GetFloat()) + (vecUp * portal_laser_push_radius.GetFloat()), // Top-left
		vecStart - (vecRight * portal_laser_push_radius.GetFloat()) - (vecUp * portal_laser_push_radius.GetFloat()), // Bottom-left

		vecStart + (vecRight * portal_laser_push_radius.GetFloat()) + (vecUp * (portal_laser_push_radius.GetFloat() + 4)), // Top-right
		vecStart + (vecRight * portal_laser_push_radius.GetFloat()) - (vecUp * (portal_laser_push_radius.GetFloat() + 4)), // Bottom-right
		vecStart - (vecRight * portal_laser_push_radius.GetFloat()) + (vecUp * (portal_laser_push_radius.GetFloat() + 4)), // Top-left
		vecStart - (vecRight * portal_laser_push_radius.GetFloat()) - (vecUp * (portal_laser_push_radius.GetFloat() + 4)), // Bottom-left
	};
	// Precalculate push directions
	Vector vecPushDir[LASER_PLAYER_PUSHER_TRACE_COUNT] = {
		vecRight + vecUp,
		vecRight - vecUp,
		-vecRight + vecUp,
		-vecRight - vecUp,

		vecRight + vecUp,
		vecRight - vecUp,
		-vecRight + vecUp,
		-vecRight - vecUp,
	};

	// Iterate though traces
	for (int i = 0; i < LASER_PLAYER_PUSHER_TRACE_COUNT; i++) {
		UTIL_TraceLine(vecOrigins[i], vecOrigins[i] + vecDir * MAX_TRACE_LENGTH, MASK_BLOCKLOS, NULL, &tr);
		// Look for a player
		if (tr.m_pEnt && tr.m_pEnt->IsPlayer()) {
			// Apply the push direction to the player. Remove Z component to not send the player flying.
			tr.m_pEnt->VelocityPunch(vecPushDir[i].RemoveZ() * portal_laser_push_force.GetFloat());
			// Apply damage in case the player gets stuck.
			tr.m_pEnt->TakeDamage(CTakeDamageInfo(this, this, m_fPlayerDamage, DMG_BURN));
			// Play hurt sound
			if (m_fHurnSoundTime < gpGlobals->curtime) {
				tr.m_pEnt->EmitSound("Player.BurnPain");
				m_fHurnSoundTime = gpGlobals->curtime + 0.5f;
			}
		}

		// Display traces if debuging is enabled.
		if (portal_laser_debug.GetBool()) {
			NDebugOverlay::Line(tr.startpos, tr.endpos, 0xFF, 0x00, 0x00, false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
			NDebugOverlay::Line(tr.startpos, tr.startpos + vecPushDir[i].RemoveZ() * 16, 0xFF, 0xFF, 0x00, false, NDEBUG_PERSIST_TILL_NEXT_SERVER);
		}
	}
}
void CEnvPortalLaser::SetPlayerDamage(float damage) {
	m_fPlayerDamage = damage;
}

// ==== Laser emitter prop ====
LINK_ENTITY_TO_CLASS(prop_laser_emitter, CPropLaserEmitter);

BEGIN_DATADESC(CPropLaserEmitter)
// Fields
	DEFINE_FIELD(m_pLaser, FIELD_CLASSPTR),
// Key fields
	DEFINE_KEYFIELD(m_bStartActive, FIELD_BOOLEAN, "startactive"),
	DEFINE_KEYFIELD(m_fPlayerDamage, FIELD_FLOAT, "playerdamage"),
// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "TurnOn", InputTurnOn),
	DEFINE_INPUTFUNC(FIELD_VOID, "TurnOff", InputTurnOff),
	DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),
END_DATADESC()

CPropLaserEmitter::CPropLaserEmitter() : m_pLaser(NULL), m_fPlayerDamage(1) { }

CPropLaserEmitter::~CPropLaserEmitter() {
	if (m_pLaser) {
		UTIL_Remove(m_pLaser);
	}
	if (m_pLaserSprite) {
		UTIL_Remove(m_pLaserSprite);
	}
}

void CPropLaserEmitter::Precache() {
	string_t sModel = GetModelName();
	if (sModel != NULL_STRING) {
		PrecacheModel(STRING(sModel));
	}

	BaseClass::Precache();
}
void CPropLaserEmitter::Spawn() {
	BaseClass::Spawn();

	// Create laser beam
	m_pLaser = dynamic_cast<CEnvPortalLaser*>(CreateEntityByName("env_portal_laser"));
	if (m_pLaser != NULL) {
		m_pLaser->KeyValue("playerdamage", m_fPlayerDamage);
		m_pLaser->Precache();
		m_pLaser->SetParent(this);
		m_pLaser->SetParentAttachment("SetParentAttachment", "laser_attachment", false);

		DispatchSpawn(m_pLaser);

		if (m_bStartActive) {
			m_pLaser->TurnOn();
		} else {
			m_pLaser->TurnOff();
		}
	}

	// Create glow
	m_pLaserSprite = dynamic_cast<CSprite*>(CreateEntityByName("env_sprite"));
	if (m_pLaserSprite != NULL) {
		const char* szSprite = portal_laser_glow_sprite.GetString();
		if (szSprite == NULL || Q_strlen(szSprite) == 0) {
			szSprite = LASER_EMITTER_DEFAULT_SPRITE;
		}
		m_pLaserSprite->KeyValue("model", szSprite);
		m_pLaserSprite->Precache();
		m_pLaserSprite->SetParent(this);
		m_pLaserSprite->SetParentAttachment("SetParentAttachment", "laser_attachment", false);
		DispatchSpawn(m_pLaserSprite);
		m_pLaserSprite->SetRenderMode(kRenderWorldGlow);
		const char* szColor = portal_laser_glow_sprite_colour.GetString();
		if (szColor != NULL && Q_strlen(szColor) > 0) {
			int r, g, b;
			sscanf(szColor, "%i%i%i", &r, &g, &b);
			m_pLaserSprite->SetRenderColor(r, g, b);
		} else {
			m_pLaserSprite->SetRenderColor(LASER_SPRITE_COLOUR);
		}
		m_pLaserSprite->SetScale(portal_laser_glow_sprite_scale.GetFloat());

		if (m_bStartActive) {
			m_pLaserSprite->TurnOn();
		} else {
			m_pLaserSprite->TurnOff();
		}
	}

	// Create collisions
	CreateVPhysics();
	SetSolid(SOLID_VPHYSICS);
}

void CPropLaserEmitter::TurnOn() {
	// Show laser beam and glow
	if (m_pLaser != NULL) {
		m_pLaser->TurnOn();
	}
	if (m_pLaserSprite != NULL) {
		m_pLaserSprite->TurnOn();
	}

	// Play activation sound
	EmitSound(LASER_ACTIVATION_SOUND);
}
void CPropLaserEmitter::TurnOff() {
	// Hide laser beam and glow
	if (m_pLaser != NULL) {
		m_pLaser->TurnOff();
	}
	if (m_pLaserSprite != NULL) {
		m_pLaserSprite->TurnOff();
	}
}
void CPropLaserEmitter::Toggle() {
	// Toggle laser beam and glow
	if (m_pLaser != NULL) {
		m_pLaser->Toggle();
	}
	if (m_pLaserSprite != NULL) {
		m_pLaserSprite->Toggle();
	}
}

void CPropLaserEmitter::InputTurnOn(inputdata_t& data) {
	TurnOn();
}
void CPropLaserEmitter::InputTurnOff(inputdata_t& data) {
	TurnOff();
}
void CPropLaserEmitter::InputToggle(inputdata_t& data) {
	Toggle();
}
