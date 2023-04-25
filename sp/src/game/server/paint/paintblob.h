#ifndef PAINTBLOB_H
#define PAINTBLOB_H

#include "props.h"

class CPaintBlob : public CPhysicsProp
{
	DECLARE_CLASS(CPaintBlob, CPhysicsProp);
	DECLARE_DATADESC();
public:
	CPaintBlob();

	void Spawn();
	void Precache();


	bool CreateVPhysics();
	void BreakablePropTouch(CBaseEntity* pOther);

private:
	float m_fRadius;
};

#endif