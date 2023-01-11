//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "ai_criteria.h"
#include "ai_default.h"
#include "ai_hint.h"
#include "ai_hull.h"
#include "ai_interactions.h"
#include "ai_memory.h"
#include "ai_navigator.h"
#include "ai_schedule.h"
#include "ai_senses.h"
#include "ai_squad.h"
#include "ai_squadslot.h"
#include "ai_task.h"
#include "ndebugoverlay.h"
#include "explode.h"
#include "bitstring.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "decals.h"
#include "antlion_dust.h"
#include "beam_shared.h"
#include "iservervehicle.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "physics_saverestore.h"
#include "vphysics/constraints.h"
#include "vehicle_base.h"
#include "eventqueue.h"
#include "te_effect_dispatch.h"
#include "npc_personality_sphere.h"
#include "npc_rollermine.h"
#include "func_break.h"
#include "soundenvelope.h"
#include "mapentities.h"
#include "RagdollBoogie.h"
#include "physics_collisionevent.h"
#include "sceneentity.h"

#include "ai_speech_new.cpp"
#include "ai_speech_new.h"
#include "ai_speech.cpp"
#include "ai_speech.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define NORMAL_CORE_MODEL_NAME "models/npcs/personality_sphere/personality_sphere.mdl" 
#define ALTERNATE_CORE_MODEL_NAME "models/npcs/personality_sphere/personality_sphere_skins.mdl" 

LINK_ENTITY_TO_CLASS(npc_personality_sphere, CNPC_PersonalitySphere);

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CNPC_PersonalitySphere )

	DEFINE_FIELD( m_iSphereType, FIELD_INTEGER ),
	DEFINE_FIELD( m_bWheatleyFixed, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_bAltModel, FIELD_BOOLEAN, "UseAltModel" ),

	DEFINE_THINKFUNC( VPhysicsThink ),

	DEFINE_OUTPUT( m_OnPhysGunDrop, "OnPhysGunDrop" ),
	DEFINE_OUTPUT( m_OnPhysGunPickup, "OnPhysGunPickup" ),

	DEFINE_BASENPCINTERACTABLE_DATADESC(),

END_DATADESC()

CNPC_PersonalitySphere::CNPC_PersonalitySphere()
{
	m_bAltModel = true; // Force it True since we don't want the Normal Model, Unforce for a more P2-Styled System
}

CNPC_PersonalitySphere::~CNPC_PersonalitySphere()
{
}


void CNPC_PersonalitySphere::Precache(void)
{
	PrecacheModel(NORMAL_CORE_MODEL_NAME);
	PrecacheModel(ALTERNATE_CORE_MODEL_NAME);

	BaseClass::Precache();
}

void CNPC_PersonalitySphere::Spawn(void)
{

	Precache();

	SetSolid(SOLID_VPHYSICS);
	AddSolidFlags(FSOLID_FORCE_WORLD_ALIGNED | FSOLID_NOT_STANDABLE);

	BaseClass::Spawn();

	m_takedamage = DAMAGE_EVENTS_ONLY;
	m_iHealth = -1;
	m_iMaxHealth = -1;

	SetPoseParameter(m_poseAim_Yaw, 0);
	SetPoseParameter(m_poseAim_Pitch, 0);

	SetModel(ALTERNATE_CORE_MODEL_NAME);
	SetSphereSkin();
	SetMass(0.1);

	CapabilitiesClear();
	CapabilitiesAdd(bits_CAP_SQUAD);

	m_flFieldOfView = -1.0f;
	m_bloodColor = DONT_BLEED;

	SetHullType(HULL_SMALL_CENTERED);

	SetHullSizeNormal();

	NPCInit();

	m_takedamage = DAMAGE_EVENTS_ONLY;
	SetDistLook(512);

	m_NPCState = NPC_STATE_NONE;

	BecomePhysical();

	SetThink(&CNPC_PersonalitySphere::VPhysicsThink);

	SetState(NPC_STATE_NONE);

}

void CNPC_PersonalitySphere::SetSphereSkin(void)
{
	switch (m_iSphereType)
	{
	case SPHERE01:
	{
		m_nSkin = (int)SKIN_SPHERE01;
	}
	case SPHERE02:
	{
		m_nSkin = (int)SKIN_SPHERE02;
	}
	case SPHERE03:
	{
		m_nSkin = (int)SKIN_SPHERE01;
	}
	case WHEATLEY:
	{
		if (m_bWheatleyFixed)
		{
			m_nSkin = (int)SKIN_WHEATLEY;
		}
		else
		{
			m_nSkin = (int)SKIN_WHEATLEY_CRACKED;
		}
	}
	}
}

bool CNPC_PersonalitySphere::BecomePhysical(void)
{
	VPhysicsDestroyObject();

	RemoveSolidFlags(FSOLID_NOT_SOLID);

	//Setup the physics controller on the roller
	IPhysicsObject *pPhysicsObject = VPhysicsInitNormal(SOLID_VPHYSICS, GetSolidFlags(), false);

	if (pPhysicsObject == NULL)
		return false;

	m_pMotionController = physenv->CreateMotionController(&m_PersonalitySphereController);
	m_pMotionController->AttachObject(pPhysicsObject, true);

	SetMoveType(MOVETYPE_VPHYSICS);

	return true;
}

void CNPC_PersonalitySphere::VPhysicsThink()
{
	if (m_MoveType != MOVETYPE_VPHYSICS)
	{
		BecomePhysical();
	}
	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CNPC_PersonalitySphere::OnPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason)
{
	// Are we just being punted?
	if (reason == PUNTED_BY_CANNON)
	{
		return;
	}

	Speak(TLK_ANSWER, NULL); //Future Me! Put this response loop stuff you were planning into the ThinkFunc and use a Bool to toggle the Held State!

	//Stop turning
	m_PersonalitySphereController.m_vecAngular = vec3_origin;

	m_OnPhysGunPickup.FireOutput(pPhysGunUser, this);
	m_bHeld = true;
	m_PersonalitySphereController.Off();
	EmitSound("NPC_RollerMine.Held");
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPhysGunUser - 
//-----------------------------------------------------------------------------
void CNPC_PersonalitySphere::OnPhysGunDrop(CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason)
{
	m_bHeld = false;
	m_PersonalitySphereController.On();

	// explode on contact if launched from the physgun
	if (Reason == LAUNCHED_BY_CANNON)
	{
		EmitSound("NPC_RollerMine.Tossed");
	}

	m_OnPhysGunDrop.FireOutput(pPhysGunUser, this);
}

int CNPC_PersonalitySphere::OnTakeDamage(const CTakeDamageInfo &info)
{
	if (!(info.GetDamageType() & DMG_BURN))
	{
		if (GetMoveType() == MOVETYPE_VPHYSICS)
		{
			AngularImpulse	angVel;
			angVel.Random(-400.0f, 400.0f);
			VPhysicsGetObject()->AddVelocity(NULL, &angVel);
			m_PersonalitySphereController.m_vecAngular *= 0.8f;

			VPhysicsTakeDamage(info);
		}
		SetCondition(COND_LIGHT_DAMAGE);
	}

	return 0;
}

int CNPC_PersonalitySphere::VPhysicsTakeDamage(const CTakeDamageInfo &info)
{
	return 0;
}