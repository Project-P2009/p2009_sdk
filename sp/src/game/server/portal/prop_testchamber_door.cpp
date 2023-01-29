#include "cbase.h"
#include "props.h"
#include "linked_portal_door.h"

#define PROP_TESTCHAMBER_DOOR "models/props/p1_door.mdl"

class CPropTestchamberDoor : public CDynamicProp {
public:
	DECLARE_CLASS(CPropTestchamberDoor, CDynamicProp);
	DECLARE_DATADESC();

	virtual void Spawn() override;
	virtual void Precache() override;

	void InputOpen(inputdata_t& value);
	void InputClose(inputdata_t& value);

	virtual void AnimBegun() override;
	virtual void AnimOver() override;

	virtual void OnFullyOpen();
	virtual void OnFullyClose();
	virtual void StartOpen();
	virtual void StartClose();
private:
	bool m_bIsOpen;

	COutputEvent m_OnOpen;
	COutputEvent m_OnFullyOpen;
	COutputEvent m_OnClose;
	COutputEvent m_OnFullyClosed;
};
LINK_ENTITY_TO_CLASS(prop_testchamber_door, CPropTestchamberDoor);

BEGIN_DATADESC(CPropTestchamberDoor)
// Fields
	DEFINE_FIELD(m_bIsOpen, FIELD_BOOLEAN),
// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "open", InputOpen),
	DEFINE_INPUTFUNC(FIELD_VOID, "close", InputClose),
// Outputs
	DEFINE_OUTPUT(m_OnOpen, "OnOpen"),
	DEFINE_OUTPUT(m_OnFullyOpen, "OnFullyOpen"),
	DEFINE_OUTPUT(m_OnClose, "OnClose"),
	DEFINE_OUTPUT(m_OnFullyClosed, "OnFullyClosed"),
END_DATADESC()

void CPropTestchamberDoor::Spawn()
{
	BaseClass::Spawn();
	SetModel(PROP_TESTCHAMBER_DOOR);

	m_bHoldAnimation = true;
}
void CPropTestchamberDoor::Precache()
{
	PrecacheScriptSound("Portal.doorclose");
	PrecacheModel(PROP_TESTCHAMBER_DOOR);
	BaseClass::Precache();
}

void CPropTestchamberDoor::InputOpen(inputdata_t& value)
{
	m_bIsOpen = true;
	PropSetAnim("open");
}

void CPropTestchamberDoor::InputClose(inputdata_t& value)
{
	m_bIsOpen = false;
	PropSetAnim("close");
}

void CPropTestchamberDoor::AnimBegun()
{
	if (m_bIsOpen) {
		StartOpen();
	} else {
		StartClose();
	}
}

void CPropTestchamberDoor::AnimOver()
{
	if (m_bIsOpen) {
		OnFullyOpen();
	} else {
		OnFullyClose();
	}

	EmitSound("Portal.doorclose");
}

void CPropTestchamberDoor::OnFullyOpen()
{
	m_OnFullyOpen.FireOutput(nullptr, this, 0);
}

void CPropTestchamberDoor::OnFullyClose()
{
	m_OnFullyClosed.FireOutput(nullptr, this, 0);
}

void CPropTestchamberDoor::StartOpen()
{
	m_OnOpen.FireOutput(nullptr, this, 0);
}

void CPropTestchamberDoor::StartClose()
{
	m_OnClose.FireOutput(nullptr, this, 0);
}

class CPropLinkedPortalDoor : public CPropTestchamberDoor {
public:
	DECLARE_CLASS(CPropLinkedPortalDoor, CPropTestchamberDoor);
	DECLARE_DATADESC();

	virtual ~CPropLinkedPortalDoor() override;

	virtual void Spawn() override;

	virtual void OnFullyOpen() override;
	virtual void OnFullyClose() override;
	virtual void StartOpen() override;
	virtual void StartClose() override;
private:
	CLinkedPortalDoor* m_pPortal;
};
LINK_ENTITY_TO_CLASS(prop_linked_portal_door, CPropLinkedPortalDoor);

BEGIN_DATADESC(CPropLinkedPortalDoor)
	DEFINE_FIELD(m_pPortal, FIELD_CLASSPTR),
END_DATADESC()

CPropLinkedPortalDoor::~CPropLinkedPortalDoor() {
	if (m_pPortal != nullptr) {
		UTIL_Remove(m_pPortal);
	}
}

void CPropLinkedPortalDoor::Spawn()
{
	m_pPortal = dynamic_cast<CLinkedPortalDoor*>(CreateEntityByName("linked_portal_door"));
}

void CPropLinkedPortalDoor::OnFullyOpen()
{
	BaseClass::OnFullyOpen();
}

void CPropLinkedPortalDoor::OnFullyClose()
{
	BaseClass::OnFullyClose();
	if (m_pPortal != nullptr) {
		inputdata_t value;
		value.value.SetBool(false);
		m_pPortal->InputSetActivatedState(value);
	}
}

void CPropLinkedPortalDoor::StartOpen()
{
	BaseClass::StartOpen();
	if (m_pPortal != nullptr) {
		inputdata_t value;
		value.value.SetBool(true);
		m_pPortal->InputSetActivatedState(value);
	}
}

void CPropLinkedPortalDoor::StartClose()
{
	BaseClass::StartClose();
}
