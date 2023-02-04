#include "cbase.h"
#include "prop_portal_dummy.h"

#define PORTAL_DUMMY_MODEL "models/portals/portal_dummy.mdl"

LINK_ENTITY_TO_CLASS(prop_portal_dummy, CPropPortalDummy);

void CPropPortalDummy::Precache()
{
	PrecacheModel(PORTAL_DUMMY_MODEL);
	PrecacheMaterial("models/portals/dummy-blue.vmt");
	PrecacheMaterial("models/portals/dummy-orange.vmt");
	PrecacheMaterial("models/portals/dummy-gray.vmt");
	BaseClass::Precache();
}

void CPropPortalDummy::Spawn()
{
	BaseClass::Spawn();
	SetModel(PORTAL_DUMMY_MODEL);

	SetSolid(SOLID_NONE);
	AddEffects(EF_NOSHADOW);
}

CPropPortalDummy* CPropPortalDummy::Create(CBaseEntity* pOwner, int skin, Vector mins, Vector maxs)
{
	CPropPortalDummy* pDummy = dynamic_cast<CPropPortalDummy*>(CreateEntityByName("prop_portal_dummy"));
	if (pDummy != nullptr)
	{
		pDummy->Precache();
		DispatchSpawn(pDummy);
		pDummy->SetParent(pOwner);

		Vector vDir;
		AngleVectors(pOwner->GetAbsAngles(), &vDir);

		pDummy->SetAbsOrigin(pOwner->GetAbsOrigin() + vDir);
		pDummy->SetAbsAngles(pOwner->GetAbsAngles());
		pDummy->m_nSkin = skin;

		UTIL_SetSize(pDummy, mins, maxs);
	}

	return pDummy;
}

CPropPortalDummy* CPropPortalDummy::CreateTinted(CBaseEntity* pOwner, int r, int g, int b, Vector mins, Vector maxs)
{
	CPropPortalDummy* pDummy = Create(pOwner, 2, mins, maxs);
	if (pDummy) {
		pDummy->SetRenderColor(r, g, b);
	}

	return pDummy;
}
