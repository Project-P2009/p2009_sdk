#include "cbase.h"
#include "paintblob.h"

CPaintBlob::CPaintBlob()
{
	m_fRadius = 2.0f;
}

void CPaintBlob::Spawn()
{
	//AddSpawnFlags(SF_PHYSPROP_PREVENT_PICKUP);

	BaseClass::Spawn();
}

void CPaintBlob::Precache()
{
	BaseClass::Precache();
}

bool CPaintBlob::CreateVPhysics()
{
	SetSolid(SOLID_BBOX);

	SetCollisionBounds(-Vector(m_fRadius), Vector(m_fRadius));

	objectparams_t params = g_PhysDefaultObjectParams;

	params.pGameData = static_cast<void*>(this);

	IPhysicsObject* pPhysicsObject = physenv->CreateSphereObject(m_fRadius, GetModelPtr()->GetRenderHdr()->textureindex, GetAbsOrigin(), GetAbsAngles(), &params, false);

	if (pPhysicsObject)
	{
		VPhysicsSetObject(pPhysicsObject);
		SetMoveType(MOVETYPE_VPHYSICS);
		pPhysicsObject->Wake();
	}

	return true;
}

void CPaintBlob::BreakablePropTouch(CBaseEntity* pOther)
{
	if (pOther->GetGroundEntity() == this) {
		Vector splashVec = GetAbsOrigin();
		Msg("Splashing at (%.2f, %.2f, %.2f)\n", splashVec.x, splashVec.y, splashVec.z);
	}
}

LINK_ENTITY_TO_CLASS(paint_blob, CPaintBlob);

BEGIN_DATADESC(CPaintBlob)
DEFINE_KEYFIELD(m_fRadius, FIELD_FLOAT, "radius"),
END_DATADESC()


static void CreatePaintBlob(CBasePlayer* pPlayer)
{
	Vector vecForward;
	AngleVectors(pPlayer->EyeAngles(), &vecForward);
	Vector vecOrigin = pPlayer->GetAbsOrigin() + vecForward * 256 + Vector(0, 0, 64);
	QAngle vecAngles(0, pPlayer->GetAbsAngles().y - 90, 0);
	CBaseEntity* pJeep = (CBaseEntity*)CPaintBlob::Create("paint_blob", vecOrigin, vecAngles);
	if (pJeep)
	{
		pJeep->Activate();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CC_Ent_CreatePaintBlob(void)
{
	CBasePlayer* pPlayer = UTIL_GetCommandClient();
	if (!pPlayer)
		return;

	CreatePaintBlob(pPlayer);

}

static ConCommand ent_create_paint_blob("ent_create_paint_blob", CC_Ent_CreatePaintBlob, "Spawn paintblob in front of the player.", FCVAR_CHEAT);