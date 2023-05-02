#pragma once

#include "cbase.h"

class CPointDebugBBox : public CPointEntity
{
public:
	DECLARE_DATADESC()
	DECLARE_CLASS(CPointDebugBBox, CBaseEntity)
	
	CPointDebugBBox();
	
	void Spawn() override;
	void DebugThink();
private:
	bool m_bAbsBox;
};