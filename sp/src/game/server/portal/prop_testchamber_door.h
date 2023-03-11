//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef PROP_TESTCHAMBER_DOOR_H
#define PROP_TESTCHAMBER_DOOR_H

#ifdef _WIN32
#pragma once
#endif

#include "func_areaportalwindow.h"
#include "physics_bone_follower.h"

#include "linked_portal_door.h"

//=============================================================================
// Non-animated linked portal door
//=============================================================================

class CPropTestChamberDoor : public CBaseAnimating
{
public:
	DECLARE_CLASS(CPropTestChamberDoor, CBaseAnimating);
	DECLARE_DATADESC();

	CPropTestChamberDoor(void);

	virtual void Precache(void);
	virtual void Spawn(void);
	virtual void Activate(void);
	virtual bool CreateVPhysics(void);

	virtual void UpdateOnRemove(void);

	virtual void Open(CBaseEntity* pActivator);
	virtual void Close(CBaseEntity* pActivator);

	virtual bool TestCollision(const Ray_t& ray, unsigned int mask, trace_t& trace);
	virtual void AnimateThink(void);

	virtual bool IsOpen();
	virtual bool IsClosed();
	bool IsAnimating() const { return m_bIsAnimating; }

protected:

	void CreateBoneFollowers(void);

	virtual void OnFullyOpened(void);
	virtual void OnFullyClosed(void);

	virtual void OnOpen(void);
	virtual void OnClose(void);

	void InputOpen(inputdata_t& input);
	void InputClose(inputdata_t& input);
	void InputLock(inputdata_t& input);
	void InputLockOpen(inputdata_t& input);
	void InputUnlock(inputdata_t& input);

	COutputEvent			m_OnOpen;
	COutputEvent			m_OnClose;
	COutputEvent			m_OnFullyOpen;
	COutputEvent			m_OnFullyClosed;

	int						m_nSequenceOpen;
	int						m_nSequenceOpenIdle;
	int						m_nSequenceClose;
	int						m_nSequenceCloseIdle;

	bool m_bIsOpen;
	bool m_bIsAnimating;
	bool m_bIsLocked;

	CBoneFollowerManager	m_BoneFollowerManager;

	//Area portal window
	void AreaPortalOpen(void);
	void AreaPortalClose(void);

	string_t m_strAreaPortalWindowName;
	CHandle<CFuncAreaPortalWindow> m_hAreaPortalWindow;
	bool m_bUseAreaPortalFade;
	float m_flAreaPortalFadeStartDistance;
	float m_flAreaPortalFadeEndDistance;

	const char* m_szOpenSound;
	const char* m_szOpenStopSound;
	const char* m_szCloseSound;
	const char* m_szCloseStopSound;
};

enum class ELinkedDoorState
{
	Open,
	Opening,
	Closing,
	Closed
};

class CPropLinkedPortalDoor : public CPropTestChamberDoor {
public:
	DECLARE_CLASS(CPropLinkedPortalDoor, CPropTestChamberDoor);
	DECLARE_DATADESC();

	virtual void Precache() override;
	virtual void Spawn() override;

	virtual void UpdateOnRemove(void);

protected:
	virtual void OnFullyClosed() override;
	virtual void OnFullyOpened() override;
	virtual void OnOpen() override;
	virtual void OnClose() override;

private:
	CLinkedPortalDoor* m_pLinkedPortalDoor1;
	CLinkedPortalDoor* m_pLinkedPortalDoor2;
	CPropLinkedPortalDoor* m_pLinkedPair;

	const char* m_szLinkedPair;

	int m_iLinkageGroupID;
	float m_fWidth, m_fHeight;
	ELinkedDoorState m_eState;
	bool m_bSecondary;
};

#endif // PROP_TESTCHAMBER_DOOR_H