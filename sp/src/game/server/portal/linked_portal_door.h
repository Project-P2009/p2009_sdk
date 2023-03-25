#ifndef _LINKED_PORTAL_DOOR_H_
#define _LINKED_PORTAL_DOOR_H_

#pragma once

#include "prop_portal.h"

#ifdef DEBUG
#include "message_entity.h"
#endif

class CLinkedPortalDoor : public CProp_Portal
{
public:
	DECLARE_CLASS(CLinkedPortalDoor, CProp_Portal);
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CLinkedPortalDoor();
	virtual ~CLinkedPortalDoor() override;

	virtual void Spawn() override;

	virtual void UpdateOnRemove() override;

	virtual bool TestCollision(const Ray_t &ray, unsigned int fContentsMask, trace_t &tr) override;
	virtual void OnRestore() override;
	virtual void NewLocation(const Vector &vOrigin, const QAngle &qAngles) override;
	virtual void InputSetActivatedState(inputdata_t& inputdata) override;
	virtual void DoFizzleEffect(int iEffect, bool bDelayedPos) override;
	virtual void Activate() override;
	virtual void ResetModel() override;
	virtual void UpdatePortalLinkage() override;

	void SetPartner(CLinkedPortalDoor* newPair);

	void Open();
	void Close();
	void InputOpen(inputdata_t& inputdata);
	void InputClose(inputdata_t& inputdata);

	void SetActiveState(bool active);

	void UnlinkPair();

	virtual void CreateSounds(void) override { /* Do nothing! */ }

	virtual bool IsWorldPortal() const override { return true; }

	void SetPortalSize(float fWidth, float fHeight);
	void SetLinkageGroup(int iLinkageGroup);

	void InputSetPartner(inputdata_t& inputdata);

	bool IsLinkedTo(CLinkedPortalDoor* pPortalDoor);
	CLinkedPortalDoor* GetPartner();
private:
	CLinkedPortalDoor* m_pLinkedPair;
	const char* m_szLinkedPair;
	bool m_bDontUpdatePair;
#ifdef DEBUG
	CMessageEntity* m_pDebugMessage;
	char m_szDebugMessage[512];
#endif
};

#endif