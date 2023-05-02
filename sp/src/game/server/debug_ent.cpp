#include "cbase.h"
#include "debug_ent.h"

LINK_ENTITY_TO_CLASS(point_debug_draw_bbox, CPointDebugBBox)
BEGIN_DATADESC(CPointDebugBBox)
	DEFINE_KEYFIELD(m_bAbsBox, FIELD_BOOLEAN, "drawabs"),
END_DATADESC()
CPointDebugBBox::CPointDebugBBox(): m_bAbsBox(false)
{
}

void CPointDebugBBox::Spawn()
{
	CPointEntity::Spawn();

	SetThink(&CPointDebugBBox::DebugThink);
	SetNextThink(gpGlobals->curtime);
}

void CPointDebugBBox::DebugThink()
{
	CBaseEntity* pParent = GetParent();
	if (pParent)
	{
		if (m_bAbsBox)
		{
			pParent->DrawAbsBoxOverlay();
		}
		else
		{
			pParent->DrawBBoxOverlay();
		}
	}
	SetNextThink(gpGlobals->curtime);
}
