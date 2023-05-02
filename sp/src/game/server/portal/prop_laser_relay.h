#pragma once

#include "cbase.h"

#include "modelentities.h"
#include "props.h"
#include "Sprite.h"

#include "debug_ent.h"

class CPropLaserRelay;

class CFuncLaserRelayTarget : public CFuncBrush
{
public:
	DECLARE_DATADESC()
	DECLARE_CLASS(CFuncLaserRelayTarget, CFuncBrush)

	CFuncLaserRelayTarget();

	void Spawn() override;

	static CFuncLaserRelayTarget* Create(CPropLaserRelay* parent, Vector origin);

	virtual void SetActivated(bool activated);
	void InputSetActivated(inputdata_t& data);
	void DebugThink();
private:
	CPropLaserRelay* m_pParentRelay;
};

class CPropLaserRelay : public CDynamicProp
{
public:
	DECLARE_DATADESC()
	DECLARE_CLASS(CPropLaserRelay, CDynamicProp)

	void UpdateOnRemove() override;

	void Precache() override;
	void Spawn() override;

	virtual void SetActivated(bool activated);

	virtual void CreateSounds();
	void DestroySounds();

	bool CreateVPhysics() override;
private:
	CSoundPatch* m_pActiveSound;

	CFuncLaserRelayTarget* m_pTarget;
	CSprite* m_pActivatedSprite;

	bool m_bActivated;
	COutputEvent m_OnPowered;
	COutputEvent m_OnUnpowered;

	int m_iSpinSeq;
};