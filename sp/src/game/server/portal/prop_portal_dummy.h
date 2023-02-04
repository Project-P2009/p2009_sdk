#pragma once

#include "cbase.h"

class CPropPortalDummy : public CBaseAnimating
{
	DECLARE_CLASS(CPropPortalDummy, CBaseAnimating);
public:
	virtual void Precache() override;
	virtual void Spawn() override;
	
	static CPropPortalDummy* Create(CBaseEntity* pOwner, int skin, Vector mins, Vector maxs);
	static CPropPortalDummy* CreateTinted(CBaseEntity* pOwner, int r, int g, int b, Vector mins, Vector maxs);
};