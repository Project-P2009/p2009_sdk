#ifndef _C_LINKED_PORTAL_DOOR_H_
#define _C_LINKED_PORTAL_DOOR_H_

#pragma once

#include "c_prop_portal.h"

class C_LinkedPortalDoor : public C_Prop_Portal
{
public:
	DECLARE_CLASS(C_LinkedPortalDoor, C_Prop_Portal);
	DECLARE_CLIENTCLASS();

	C_LinkedPortalDoor();
	virtual ~C_LinkedPortalDoor() override;

	virtual void DrawSimplePortalMesh(const IMaterial *pMaterialOverride = NULL, float fForwardOffsetModifier = 0.25f) override;
};

#endif