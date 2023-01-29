#ifndef _LINKED_PORTAL_DOOR_H_
#define _LINKED_PORTAL_DOOR_H_

#pragma once

#include "prop_portal.h"

class CLinkedPortalDoor : public CProp_Portal
{
	public:
		DECLARE_CLASS(CLinkedPortalDoor, CProp_Portal);
		DECLARE_SERVERCLASS();
		DECLARE_DATADESC();

		CLinkedPortalDoor();
		virtual ~CLinkedPortalDoor() override;

		virtual void Spawn() override;

		virtual bool TestCollision(const Ray_t &ray, unsigned int fContentsMask, trace_t &tr) override;
		virtual void OnRestore() override;
		virtual void NewLocation(const Vector &vOrigin, const QAngle &qAngles) override;
		virtual void InputSetActivatedState(inputdata_t &inputdata) override;
		virtual void DoFizzleEffect(int iEffect, bool bDelayedPos) override;
		virtual void Activate() override;
		virtual void ResetModel() override;

		virtual void CreateSounds(void) override { /* Do nothing! */ }
	private:
		
};

#endif