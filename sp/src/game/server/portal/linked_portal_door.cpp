#include "cbase.h"

#include "prop_portal.h"
#include "prop_portal_shared.h"
#include "PortalSimulation.h"
#include "PhysicsCloneArea.h"

class CLinkedPortalDoor : public CProp_Portal
{
public:
	DECLARE_CLASS(CLinkedPortalDoor, CProp_Portal);
	DECLARE_DATADESC();

	CLinkedPortalDoor();
	virtual ~CLinkedPortalDoor() override;

	virtual void Precache() override;
	virtual void Spawn() override;

	CLinkedPortalDoor* GetLinkedPair();

	virtual float GetWidth() override { return m_fWidth.Get(); }
	virtual float GetHeight() override { return m_fHeight.Get(); }
private:
	void SetLinkedPair(CLinkedPortalDoor* pPair);

	CLinkedPortalDoor* m_pPairEntity;
	const char* m_szPair;
};
LINK_ENTITY_TO_CLASS(linked_portal_door, CLinkedPortalDoor);

BEGIN_DATADESC(CLinkedPortalDoor)
// Keyfields
	DEFINE_KEYFIELD(m_fWidth, FIELD_FLOAT, "width"),
	DEFINE_KEYFIELD(m_fHeight, FIELD_FLOAT, "height"),
	DEFINE_KEYFIELD(m_szPair, FIELD_STRING, "linkedpair"),
END_DATADESC()

CLinkedPortalDoor::CLinkedPortalDoor() : BaseClass() { }
CLinkedPortalDoor::~CLinkedPortalDoor() = default;

void CLinkedPortalDoor::Precache()
{
	BaseClass::Precache();
}
void CLinkedPortalDoor::Spawn()
{
	BaseClass::Spawn();
	if (m_szPair != nullptr && Q_strlen(m_szPair) > 0)
	{
		m_pPairEntity = dynamic_cast<CLinkedPortalDoor*>(gEntList.FindEntityByName(nullptr, m_szPair));
		if (m_pPairEntity != nullptr)
		{
			m_pPairEntity->SetLinkedPair(this);
		}
	}
}
CLinkedPortalDoor* CLinkedPortalDoor::GetLinkedPair()
{
	return m_pPairEntity;
}

void CLinkedPortalDoor::SetLinkedPair(CLinkedPortalDoor* pPair)
{
	m_pPairEntity = pPair;
}
