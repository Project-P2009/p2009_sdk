//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines a weighted storage cube.
//
//=====================================================================================//
#include "cbase.h"					// for pch
#include "prop_weightedcube.h"
#include "prop_laser_emitter.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar reflector_cube_disabled_think_rate("reflector_cube_disabled_think_rate", "0.1f", FCVAR_DEVELOPMENTONLY, "The rate at which the cube should think when it is disabled.");
ConVar reflector_cube_disabled_nudge_time("reflector_cube_disabled_nudge_time", "0.5f", FCVAR_DEVELOPMENTONLY, "The amount of time the cube needs to be touched before it gets enabled again.");
ConVar reflector_cube_disabled_use_touch_check("reflector_cube_disabled_use_touch_check", "0", FCVAR_DEVELOPMENTONLY, "Use touch checks to determine when to enable the cube.");

#define	CUBE_MODEL			"models/props/metal_box.mdl"
#define	REFLECTION_MODEL	"models/props/reflection_cube.mdl"
#define	SCHRODINGER_MODEL	"models/props/reflection_cube.mdl"
#define	SPHERE_MODEL		"models/props/sphere.mdl"
#define	ANTIQUE_MODEL		"models/props/p2/metal_box.mdl"

#define FIZZLE_SOUND		"Prop.Fizzled"

const char SCHRODINGER_THINK_CONTEXT[] = "Schrodinger Think Context";

LINK_ENTITY_TO_CLASS(prop_weighted_cube, CPropWeightedCube);


BEGIN_DATADESC(CPropWeightedCube)


// Save/load
DEFINE_USEFUNC(Use),

//DEFINE_KEYFIELD(m_oldSkin, FIELD_INTEGER, "skin"),
DEFINE_KEYFIELD(m_cubeType, FIELD_INTEGER, "CubeType"),
DEFINE_KEYFIELD(m_skinType, FIELD_INTEGER, "SkinType"),
DEFINE_KEYFIELD(m_paintPower, FIELD_INTEGER, "PaintPower"),
DEFINE_KEYFIELD(m_useNewSkins, FIELD_BOOLEAN, "NewSkins"),
DEFINE_KEYFIELD(m_allowFunnel, FIELD_BOOLEAN, "allowfunnel"),

DEFINE_FIELD(m_hSchrodingerTwin, FIELD_EHANDLE),
DEFINE_FIELD(m_hLaser, FIELD_EHANDLE),

DEFINE_INPUTFUNC(FIELD_VOID, "Dissolve", InputDissolve),
DEFINE_INPUTFUNC(FIELD_VOID, "SilentDissolve", InputSilentDissolve),
DEFINE_INPUTFUNC(FIELD_VOID, "PreDissolveJoke", InputPreDissolveJoke),
DEFINE_INPUTFUNC(FIELD_VOID, "ExitDisabledState", InputExitDisabledState),

// Output
//DEFINE_OUTPUT(m_OnPainted, "OnPainted"),

DEFINE_OUTPUT(m_OnFizzled, "OnFizzled"),
DEFINE_OUTPUT(m_OnOrangePickup, "OnOrangePickup"),
DEFINE_OUTPUT(m_OnBluePickup, "OnBluePickup"),
DEFINE_OUTPUT(m_OnPlayerPickup, "OnPlayerPickup"),
DEFINE_OUTPUT(m_OnPhysGunDrop, "OnPhysGunDrop"),

DEFINE_THINKFUNC(Think),
DEFINE_THINKFUNC(SchrodingerThink),

END_DATADESC()

CHandle< CPropWeightedCube > CPropWeightedCube::m_hSchrodingerDangling;

//-----------------------------------------------------------------------------
// Purpose: Precache
// Input  :  - 
//-----------------------------------------------------------------------------
void CPropWeightedCube::Precache()
{
	PrecacheModel(CUBE_MODEL);
	PrecacheModel(REFLECTION_MODEL);
	PrecacheModel(SCHRODINGER_MODEL);
	PrecacheModel(SPHERE_MODEL);
	PrecacheModel(ANTIQUE_MODEL);
	PrecacheScriptSound(FIZZLE_SOUND);

	BaseClass::Precache();

}

int CPropWeightedCube::ObjectCaps()
{
	int caps = BaseClass::ObjectCaps();

	caps |= FCAP_IMPULSE_USE;

	return caps;
}

//-----------------------------------------------------------------------------
// Purpose: Computes a local matrix for the player clamped to valid carry ranges
//-----------------------------------------------------------------------------
// when looking level, hold bottom of object 8 inches below eye level
#define PLAYER_HOLD_LEVEL_EYES	-8
// when looking down, hold bottom of object 0 inches from feet
#define PLAYER_HOLD_DOWN_FEET	2
// when looking up, hold bottom of object 24 inches above eye level
#define PLAYER_HOLD_UP_EYES		24
// use a +/-30 degree range for the entire range of motion of pitch
#define PLAYER_LOOK_PITCH_RANGE	30
// player can reach down 2ft below his feet (otherwise he'll hold the object above the bottom)
#define PLAYER_REACH_DOWN_DISTANCE	24
static void ComputePlayerMatrix( CBasePlayer *pPlayer, matrix3x4_t &out )
{
	if (!pPlayer) return;

	QAngle angles = pPlayer->EyeAngles();
	Vector origin = pPlayer->EyePosition();

	angles.x = 0;

	float feet = pPlayer->GetAbsOrigin().z + pPlayer->WorldAlignMins().z;
	float eyes = origin.z;
	float zoffset = 0;
	// moving up (negative pitch is up)
	if (angles.x < 0)
	{
		zoffset = RemapVal(angles.x, 0, -PLAYER_LOOK_PITCH_RANGE, PLAYER_HOLD_LEVEL_EYES, PLAYER_HOLD_UP_EYES);
	}
	else
	{
		zoffset = RemapVal(angles.x, 0, PLAYER_LOOK_PITCH_RANGE, PLAYER_HOLD_LEVEL_EYES, PLAYER_HOLD_DOWN_FEET + (feet - eyes));
	}
	origin.z += zoffset;
	angles.x = 0;
	AngleMatrix(angles, origin, out);
}

//-------------------------------------------------------------------------------------
// Purpose: Okay, it is me again because I am pretty much tasked with most of the
// prop_weighted_cube work. And this thing apart from CPropWeightedCube::ToggleLaser()
// is the most retarded thing I have ever seen. Sometimes I am wondering if being
// a good Source Programmer makes you a good Sphagetti cook...
// ~GabrielV
//-------------------------------------------------------------------------------------
QAngle CPropWeightedCube::PreferredCarryAngles(void)
{
	if (m_cubeType != Reflective) return BaseClass::PreferredCarryAngles();

	static QAngle ang_PrefAngels;

	CBasePlayer* pPlayer = UTIL_GetLocalPlayer();
	if (pPlayer)
	{
		Vector vecRight;
		pPlayer->GetVectors(NULL, &vecRight, NULL);

		Quaternion qRotation;
		AxisAngleQuaternion(vecRight, pPlayer->EyeAngles().x, qRotation);

		matrix3x4_t tmp;
		ComputePlayerMatrix(pPlayer, tmp);

		QAngle qTemp = TransformAnglesToWorldSpace(ang_PrefAngels, tmp);
		Quaternion qExisting;
		AngleQuaternion(qTemp, qExisting);
		Quaternion qFinal;
		QuaternionMult(qRotation, qExisting, qFinal);

		QuaternionAngles(qFinal, qTemp);
		ang_PrefAngels = TransformAnglesToLocalSpace(qTemp, tmp);
	}

	return ang_PrefAngels;
}

int CPropWeightedCube::UpdateTransmitState()
{
	if (m_cubeType == Reflective) return SetTransmitState(FL_EDICT_ALWAYS);

	return BaseClass::UpdateTransmitState();
}

void CPropWeightedCube::SchrodingerThink(void)
{
	//Keep thinking
	SetContextThink(&CPropWeightedCube::SchrodingerThink, gpGlobals->curtime + reflector_cube_disabled_think_rate.GetFloat(), SCHRODINGER_THINK_CONTEXT);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CPropWeightedCube::Spawn()
{
	Precache();

	m_nSkin = 0;
	switch (m_cubeType)
	{
	case Standard:
		SetModel(CUBE_MODEL);
		if (m_skinType == Rusted)
			m_nSkin = 3;
		if (m_paintPower == Stick)
			m_nSkin = 8;
		if (m_paintPower == Speed)
			m_nSkin = 9;
		break;
	case Companion:
		SetModel(CUBE_MODEL);
		m_nSkin = 1;
		if (m_paintPower == Stick)
			m_nSkin = 10;
		if (m_paintPower == Speed)
			m_nSkin = 11;
		break;
	case Reflective:
		SetModel(REFLECTION_MODEL);
		if (m_skinType == Rusted)
			m_nSkin = 1;
		if (m_paintPower == Stick)
			m_nSkin = 2;
		if (m_paintPower == Speed)
			m_nSkin = 3;
		break;
	case Sphere:
		SetModel(SPHERE_MODEL);
		break;
	case Antique:
		SetModel(ANTIQUE_MODEL);
		if (m_skinType == Rusted)
			m_nSkin = 3;
		if (m_paintPower == Stick)
			m_nSkin = 8;
		if (m_paintPower == Speed)
			m_nSkin = 9;
		break;
	case Schrodinger:
		SetModel(SCHRODINGER_MODEL);
		break;
	}

	SetSolid(SOLID_VPHYSICS);

	// In order to pick it up, needs to be physics.
	CreateVPhysics();

	SetUse(&CPropWeightedCube::Use);

	if (m_cubeType == Schrodinger) {
		SetContextThink(&CPropWeightedCube::SchrodingerThink, gpGlobals->curtime + reflector_cube_disabled_think_rate.GetFloat(), SCHRODINGER_THINK_CONTEXT);
	}

	BaseClass::Spawn();

}

void CPropWeightedCube::UpdateOnRemove(void)
{
	BaseClass::UpdateOnRemove();

	CPropWeightedCube* pTwin = m_hSchrodingerTwin.Get();
	if (pTwin && !pTwin->IsMarkedForDeletion())
	{
		pTwin->Dissolve(NULL, gpGlobals->curtime, false, 0, pTwin->GetAbsOrigin(), 1);
		pTwin->EmitSound(FIZZLE_SOUND);
	}
}

void CPropWeightedCube::AddEmitter(CBaseEntity* emitter) {
	// Store previous list count
	int oldCount = m_LaserList.Count();

	// Check if the emitter has not been added already.
	if (!m_LaserList.HasElement(emitter)) {
		// Add laser emitter
		m_LaserList.AddToTail(emitter);
	}

	// Check if we added any laser emitters
	if (oldCount == 0 && m_LaserList.Count() > 0) {
		ToggleLaser(true);
	}
}

void CPropWeightedCube::RemoveEmitter(CBaseEntity* emitter) {
	// Check if the emitter is on the list, then remove it.
	if (m_LaserList.HasElement(emitter)) {
		m_LaserList.FindAndRemove(emitter);
	}

	if (m_LaserList.Count() == 0) {
		ToggleLaser(false);
	}
}

//-----------------------------------------------------------------------------------
// Purpose: Okay, so. I do not know why. I do not want to know why.
// I do not have a PHD in Source Engine Fuckery so I still do not understand how,
// but the only know method to not make the cubes laser lag is to put 0 as argument
// for SetParent() function. I have no idea why, or even how Source Engine treats it
// because setting it to GetAttachment() or -1 causes it to lag, so please...
// DO NOT FUCKING TOUCH THIS PART OF THE CODE.
// ~GabrielV
//-----------------------------------------------------------------------------------
void CPropWeightedCube::ToggleLaser(bool state)
{
	if (m_cubeType != Reflective) return;

	CEnvPortalLaser* pLaser = dynamic_cast<CEnvPortalLaser*>(m_hLaser.Get());

	if (!pLaser) {
		pLaser = (CEnvPortalLaser*)CreateEntityByName("env_portal_laser");
		if (pLaser) {
			Vector vecOrigin;
			QAngle angOrigin;
			GetAttachment("focus", vecOrigin, angOrigin);
			pLaser->Activate();
			pLaser->SetAbsOrigin(vecOrigin);
			pLaser->SetAbsAngles(angOrigin);
			pLaser->SetParent(this, 0);
			pLaser->TurnOff();
			DispatchSpawn(pLaser);
			m_hLaser = pLaser;
		}
	}

	if (state) {
		pLaser->TurnOn();
	}
	else {
		pLaser->TurnOff();
	}
}

void CPropWeightedCube::InputPreDissolveJoke(inputdata_t& data)
{
	// Sets some VO to do before fizzling.
}

void CPropWeightedCube::InputExitDisabledState(inputdata_t& data)
{
	// Exits the disabled state of a reflector cube.
}


void CPropWeightedCube::InputDissolve(inputdata_t& data)
{
	Dissolve(NULL, gpGlobals->curtime, false, 0, GetAbsOrigin(), 1);
	EmitSound(FIZZLE_SOUND);
}

void CPropWeightedCube::InputSilentDissolve(inputdata_t& data)
{
	Dissolve(NULL, gpGlobals->curtime, false, 0, GetAbsOrigin(), 1);
}

bool CPropWeightedCube::Dissolve(const char* materialName, float flStartTime, bool bNPCOnly, int nDissolveType, Vector vDissolverOrigin, int magnitude)
{
	m_OnFizzled.FireOutput(this, this);
	return BaseClass::Dissolve(materialName, flStartTime, bNPCOnly, nDissolveType, vDissolverOrigin, magnitude);
}

void CPropWeightedCube::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	CBasePlayer* pPlayer = ToBasePlayer(pActivator);
	if (pPlayer)
	{
		pPlayer->PickupObject(this);
	}
}

void CPropWeightedCube::OnPhysGunPickup(CBasePlayer* pPhysGunUser, PhysGunPickup_t reason)
{
	m_hPhysicsAttacker = pPhysGunUser;

	if (reason == PICKED_UP_BY_CANNON || reason == PICKED_UP_BY_PLAYER)
	{
		m_OnPlayerPickup.FireOutput(pPhysGunUser, this);
	}
}

void CPropWeightedCube::OnPhysGunDrop(CBasePlayer* pPhysGunUser, PhysGunDrop_t reason)
{
	m_OnPhysGunDrop.FireOutput(pPhysGunUser, this);
}

void CPropWeightedCube::SetActivated(bool active)
{
	if (m_useNewSkins)
	{
		if (active)
		{
			m_nSkin = 0;
		}
		else
		{
			m_nSkin = 2;
		}
		
		switch (m_cubeType)
		{
		case Standard:
			
			if (m_skinType == Rusted)
			{
				if (active)
				{
					m_nSkin = 5;
				}
				else
				{
					m_nSkin = 3;
				}
				
			}
			else
			{
				if (active)
				{
					m_nSkin = 2;
				}
				else
				{
					m_nSkin = 0;
				}
				
			}
				
			if (m_paintPower == Bounce)
			{
				if (active)
				{
					m_nSkin = 10;
				}
				else
				{
					m_nSkin = 6;
				}
			}
				
			if (m_paintPower == Speed)
			{
				if (active)
				{
					m_nSkin = 11;
				}
				else
				{
					m_nSkin = 7;
				}
			}
				
			break;
		case Companion:
			
			if (active)
			{
				m_nSkin = 4;
			}
			else
			{
				m_nSkin = 1;
			}
			
			if (m_paintPower == Bounce)
			{
				if (active)
				{
					m_nSkin = 10;
				}
				else
				{
					m_nSkin = 6;
				}
			}
				
			if (m_paintPower == Speed)
			{
				if (active)
				{
					m_nSkin = 9;
				}
				else
				{
					m_nSkin = 7;
				}
			}
			break;
		case Reflective:
			
			if (m_skinType == Rusted)
			{
				if (active)
				{
					m_nSkin = 5;
				}
				else
				{
					m_nSkin = 1;
				}

			}
			else
			{
				if (active)
				{
					m_nSkin = 6;
				}
				else
				{
					m_nSkin = 0;
				}

			}

			if (m_paintPower == Bounce)
			{
				if (active)
				{
					m_nSkin = 7;
				}
				else
				{
					m_nSkin = 2;
				}
			}

			if (m_paintPower == Speed)
			{
				if (active)
				{
					m_nSkin = 8;
				}
				else
				{
					m_nSkin = 3;
				}
			
			}

			break;
		case Sphere:
			
			break;
		case Antique:
			
			if (m_skinType == Rusted)
			{
				if (active)
				{
					m_nSkin = 5;
				}
				else
				{
					m_nSkin = 3;
				}
				
			}
			else
			{
				if (active)
				{
					m_nSkin = 2;
				}
				else
				{
					m_nSkin = 0;
				}
				
			}
				
			if (m_paintPower == Bounce)
			{
				if (active)
				{
					m_nSkin = 10;
				}
				else
				{
					m_nSkin = 6;
				}
			}
				
			if (m_paintPower == Speed)
			{
				if (active)
				{
					m_nSkin = 11;
				}
				else
				{
					m_nSkin = 7;
				}
			}
				
			break;
		}
	}
	else
	{
		switch (m_nSkin)
		{
		case OLDStandard:
			if (m_skinType == Rusted)
			{
				if (active)
				{
					m_nSkin = 5;
				}
				else
				{
					m_nSkin = 3;
				}

			}
			else
			{
				if (active)
				{
					m_nSkin = 2;
				}
				else
				{
					m_nSkin = 0;
				}

			}

			if (m_paintPower == Bounce)
			{
				if (active)
				{
					m_nSkin = 10;
				}
				else
				{
					m_nSkin = 6;
				}
			}

			if (m_paintPower == Speed)
			{
				if (active)
				{
					m_nSkin = 11;
				}
				else
				{
					m_nSkin = 7;
				}
			}
			break;
		case OLDCompanion:
			if (active)
			{
				m_nSkin = 4;
			}
			else
			{
				m_nSkin = 1;
			}

			if (m_paintPower == Bounce)
			{
				if (active)
				{
					m_nSkin = 10;
				}
				else
				{
					m_nSkin = 6;
				}
			}

			if (m_paintPower == Speed)
			{
				if (active)
				{
					m_nSkin = 9;
				}
				else
				{
					m_nSkin = 7;
				}
			}
			break;
		case OLDReflective:
			if (m_skinType == Rusted)
			{
				if (active)
				{
					m_nSkin = 5;
				}
				else
				{
					m_nSkin = 1;
				}

			}
			else
			{
				if (active)
				{
					m_nSkin = 6;
				}
				else
				{
					m_nSkin = 0;
				}

			}

			if (m_paintPower == Bounce)
			{
				if (active)
				{
					m_nSkin = 7;
				}
				else
				{
					m_nSkin = 2;
				}
			}

			if (m_paintPower == Speed)
			{
				if (active)
				{
					m_nSkin = 8;
				}
				else
				{
					m_nSkin = 3;
				}

			}
			break;
		case OLDSphere:

			break;
		case OLDAntique:
			if (m_skinType == Rusted)
			{
				if (active)
				{
					m_nSkin = 5;
				}
				else
				{
					m_nSkin = 3;
				}

			}
			else
			{
				if (active)
				{
					m_nSkin = 2;
				}
				else
				{
					m_nSkin = 0;
				}

			}

			if (m_paintPower == Bounce)
			{
				if (active)
				{
					m_nSkin = 10;
				}
				else
				{
					m_nSkin = 6;
				}
			}

			if (m_paintPower == Speed)
			{
				if (active)
				{
					m_nSkin = 11;
				}
				else
				{
					m_nSkin = 7;
				}
			}
			break;
		}
	}
}

void CPropWeightedCube::UpdatePreferredAngles(CBasePlayer* pPlayer)
{
	m_vecCarryAngles = QAngle(0, 0, 0);

	if (HasPreferredCarryAnglesForPlayer(pPlayer))
	{
		m_qPreferredPlayerCarryAngles = m_vecCarryAngles;
	}
	else
	{
		if (m_qPreferredPlayerCarryAngles.Get().x < FLT_MAX)
		{
			m_qPreferredPlayerCarryAngles.GetForModify().Init(FLT_MAX, FLT_MAX, FLT_MAX);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool UTIL_IsWeightedCube(CBaseEntity* pEntity)
{
	if (pEntity == NULL)
		return false;

	return (FClassnameIs(pEntity, "prop_weighted_cube"));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool UTIL_IsReflectiveCube(CBaseEntity* pEntity)
{
	if (UTIL_IsWeightedCube(pEntity) == false)
		return false;

	CPropWeightedCube* pCube = assert_cast<CPropWeightedCube*>(pEntity);
	return (pCube && pCube->GetCubeType() == Reflective);
}

bool UTIL_IsSchrodinger(CBaseEntity* pEntity)
{
	if (!UTIL_IsWeightedCube(pEntity))
		return false;

	CPropWeightedCube* pCube = assert_cast<CPropWeightedCube*>(pEntity);
	if (!pCube)
		return false;

	return pCube->GetCubeType() == Schrodinger;
}

CPropWeightedCube* UTIL_GetSchrodingerTwin(CBaseEntity* pEntity)
{
	if (!UTIL_IsSchrodinger(pEntity))
		return NULL;

	CPropWeightedCube* pCube = assert_cast<CPropWeightedCube*>(pEntity);
	if (!pCube)
		return NULL;

	return pCube->GetSchrodingerTwin();
}