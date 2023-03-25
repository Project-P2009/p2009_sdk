#include "cbase.h"
#include "linked_portal_door.h"
#include "PhysicsCloneArea.h"
#include "envmicrophone.h"
#include "env_speaker.h"
#include "soundenvelope.h"
#include "portal_placement.h"
#include "physicsshadowclone.h"
#include "particle_parse.h"
#include "effect_dispatch_data.h"
#include "weapon_portalgun.h"
#include "rumble_shared.h"
#include "prop_portal_shared.h"
#include "tier0/memdbgon.h"

extern CUtlVector<CProp_Portal*> s_PortalLinkageGroups[256];

LINK_ENTITY_TO_CLASS(linked_portal_door, CLinkedPortalDoor);

IMPLEMENT_SERVERCLASS_ST(CLinkedPortalDoor, DT_LinkedPortalDoor)
END_SEND_TABLE();

BEGIN_DATADESC(CLinkedPortalDoor)
// Keyfields
	DEFINE_KEYFIELD(m_szLinkedPair, FIELD_STRING, "LinkedPair"),
// Fields
	DEFINE_FIELD(m_pLinkedPair, FIELD_CLASSPTR),
#ifdef DEBUG
	DEFINE_FIELD(m_pDebugMessage, FIELD_CLASSPTR),
#endif
// Inputs
	DEFINE_INPUTFUNC(FIELD_STRING,	"SetPartner",	InputSetPartner),
	DEFINE_INPUTFUNC(FIELD_VOID,	"Open",			InputOpen),
	DEFINE_INPUTFUNC(FIELD_VOID,	"Close",		InputClose),
END_DATADESC()

static bool _OccupiedLinkageGroups[128][2];

static int FindFreeGroup(bool isPortal2)
{
	for (int i = 0; i < 128; i++)
	{
		if (_OccupiedLinkageGroups[i][isPortal2 ? 1 : 0] == false)
		{
			return i + 127;
		}
	}

	return -1;
}

CLinkedPortalDoor::CLinkedPortalDoor()
	: BaseClass()
{
	physcollision->DestroyCollide(m_pCollisionShape);
	m_pCollisionShape = nullptr;
	m_bDontUpdatePair = false;
	m_iLinkageGroupID = 127;
}

CLinkedPortalDoor::~CLinkedPortalDoor()
{
}

void CLinkedPortalDoor::Spawn()
{
	BaseClass::Spawn();

	m_PortalSimulator.SetWidth(m_fWidth);
	m_PortalSimulator.SetHeight(m_fHeight);

	CBaseEntity* pParent = gEntList.FindEntityByName(nullptr, MAKE_STRING(m_szLinkedPair));
	if (pParent != nullptr)
	{
		CLinkedPortalDoor* pDoor = dynamic_cast<CLinkedPortalDoor*>(pParent);
		if (pDoor)
		{
			m_pLinkedPair = pDoor;
		}
		else
		{
			DEBUG_WARN("%s ==> Failed to get pair: Pair is not a linked_portal_pair!\n", GetDebugName());
		}
	}
	else
	{
		DEBUG_WARN("%s ==> Failed to get pair: No pair specified!\n", GetDebugName());
	}

	// Find free linkage group
	// NOTE: We use the top 128 values, to not interfier with prop_portal.
	int iLinkageGroup = FindFreeGroup(IsPortal2());

	if (iLinkageGroup < 0)
	{
		Warning("No free linkage group id found for linked_portal_door at (%f %f %f)!\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z);
		UTIL_Remove(this);
	}
	else if (iLinkageGroup > 255 || iLinkageGroup < 127)
	{
		Warning("Index out of bounds! Expected ineger between 128 and 255, but got %i!\n", iLinkageGroup);
		UTIL_Remove(this);
	}
	else
	{
		m_iLinkageGroupID = (unsigned char)iLinkageGroup;
		if (m_pLinkedPair != nullptr && IsPortal2() == false)
		{
			if (m_pLinkedPair->m_iLinkageGroupID >= 127)
			{
				_OccupiedLinkageGroups[m_pLinkedPair->m_iLinkageGroupID - 127][m_pLinkedPair->IsPortal2() ? 1 : 0] = false;
			}

			m_pLinkedPair->SetIsPortal2(true);
			m_pLinkedPair->SetLinkageGroup(m_iLinkageGroupID);
			// m_pLinkedPair->UnlinkPair();
			m_pLinkedPair->m_pLinkedPair = this;
		}

		_OccupiedLinkageGroups[iLinkageGroup - 127][IsPortal2() ? 1 : 0] = true;

		DEBUG_MSG("%s ---> World portal (%s) size -> (%f %f), location -> (%f %f %f), linkage id -> (%u), pair -> (%s), secondary -> (%u)\n",
			GetClassname(), GetDebugName(),
			m_PortalSimulator.GetWidth(), m_PortalSimulator.GetHeight(),
			GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z,
			m_iLinkageGroupID,
			m_pLinkedPair ? m_pLinkedPair->GetDebugName() : "(none)",
			IsPortal2()
		);

#if DEBUG
		m_pDebugMessage = dynamic_cast<CMessageEntity*>(CreateEntityByName("point_message"));
		if (m_pDebugMessage != nullptr)
		{
			m_pDebugMessage->SetAbsOrigin(GetAbsOrigin());
			m_pDebugMessage->KeyValue("radius", "128");

			memset(m_szDebugMessage, 0, 512);
			Q_snprintf(m_szDebugMessage, 512,
				"Name: %s, Pair name: %s Size: %i * %i Status: %u Linkage Group: %u Secondary: %u",
				GetDebugName(), (m_pLinkedPair ? m_pLinkedPair->GetDebugName() : "(none)"),
				(int)m_fWidth, (int)m_fHeight,
				m_bActivated,
				m_iLinkageGroupID,
				IsPortal2()
			);
			m_pDebugMessage->SetMessageText(MAKE_STRING(m_szDebugMessage));
			DispatchSpawn(m_pDebugMessage);
		}
#endif
		UpdatePortalLinkage();
	}
}

void CLinkedPortalDoor::UpdateOnRemove()
{
	if (_OccupiedLinkageGroups[m_iLinkageGroupID - 127][IsPortal2() ? 1 : 0])
	{
		_OccupiedLinkageGroups[m_iLinkageGroupID - 127][IsPortal2() ? 1 : 0] = false;
	}

#if DEBUG
	if (m_pDebugMessage != nullptr)
	{
		UTIL_Remove(m_pDebugMessage);
		m_pDebugMessage = nullptr;
	}
#endif

	DEBUG_MSG("!!! %s ---> Entity destroyed!\n", GetDebugName());

	BaseClass::UpdateOnRemove();
}

bool CLinkedPortalDoor::TestCollision(const Ray_t &ray, unsigned int fContentsMask, trace_t &tr)
{
	CTraceFilterSimple filter(this, GetCollisionGroup(), nullptr);
	enginetrace->TraceRay(ray, fContentsMask, &filter, &tr);
	return tr.DidHit();
}

void CLinkedPortalDoor::OnRestore()
{
	UpdateCorners();

	Assert(m_pAttachedCloningArea == NULL);
	m_pAttachedCloningArea = CPhysicsCloneArea::CreatePhysicsCloneArea(this);

	BaseClass::BaseClass::OnRestore();
}

void CLinkedPortalDoor::NewLocation( const Vector &vOrigin, const QAngle &qAngles )
{
	// Tell our physics environment to stop simulating it's entities.
	// Fast moving objects can pass through the hole this frame while it's in the old location.
	m_PortalSimulator.ReleaseAllEntityOwnership();
	m_PortalSimulator.SetWidth(m_fWidth);
	m_PortalSimulator.SetHeight(m_fHeight);

	DEBUG_MSG("%s ---> World portal (%s) size -> (%f %f), location -> (%f %f %f), linkage id -> (%i)\n", GetClassname(), GetDebugName(),
		m_PortalSimulator.GetWidth(), m_PortalSimulator.GetHeight(),
		GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z,
		m_iLinkageGroupID
	);
	Vector vOldForward;
	GetVectors( &vOldForward, 0, 0 );

	m_vPrevForward = vOldForward;

	WakeNearbyEntities();

	Teleport( &vOrigin, &qAngles, 0 );

	if ( m_hMicrophone )
	{
		CEnvMicrophone *pMicrophone = static_cast<CEnvMicrophone*>( m_hMicrophone.Get() );
		pMicrophone->Teleport( &vOrigin, &qAngles, 0 );
		pMicrophone->InputEnable( inputdata_t() );
	}

	if ( m_hSpeaker )
	{
		CSpeaker *pSpeaker = static_cast<CSpeaker*>( m_hSpeaker.Get() );
		pSpeaker->Teleport( &vOrigin, &qAngles, 0 );
		pSpeaker->InputTurnOn( inputdata_t() );
	}

	//if the other portal should be static, let's not punch stuff resting on it
	bool bOtherShouldBeStatic = false;
	if( !m_hLinkedPortal )
		bOtherShouldBeStatic = true;

	m_bActivated = true;

	UpdatePortalLinkage();
	UpdatePortalTeleportMatrix();

	// Update the four corners of this portal for faster reference
	UpdateCorners();

	WakeNearbyEntities();

	if ( m_hLinkedPortal )
	{
		m_hLinkedPortal->WakeNearbyEntities();
		if( !bOtherShouldBeStatic ) 
		{
			m_hLinkedPortal->PunchAllPenetratingPlayers();
		}
	}

	UpdateCollision();
}

void CLinkedPortalDoor::InputSetActivatedState( inputdata_t &inputdata )
{
	if (m_pLinkedPair == nullptr && m_bActivated == false)
	{
		Warning("Cannot activate portal '%s'! Portal has no pair assigned!\n", GetDebugName());
		return;
	}

	m_bActivated = inputdata.value.Bool();
	m_hPlacedBy = NULL;

	if ( m_bActivated )
	{
		Vector vOrigin;
		vOrigin = GetAbsOrigin();

		Vector vForward, vUp;
		GetVectors( &vForward, 0, &vUp );

		CTraceFilterSimpleClassnameList baseFilter( this, COLLISION_GROUP_NONE );
		UTIL_Portal_Trace_Filter( &baseFilter );
		CTraceFilterTranslateClones traceFilterPortalShot( &baseFilter );

		trace_t tr;
		UTIL_TraceLine( vOrigin + vForward, vOrigin + vForward * -8.0f, MASK_SHOT_PORTAL, &traceFilterPortalShot, &tr );

		QAngle qAngles;
		VectorAngles( tr.plane.normal, vUp, qAngles );

		float fPlacementSuccess = VerifyPortalPlacement( this, tr.endpos, qAngles, PORTAL_PLACED_BY_FIXED );
		PlacePortal( tr.endpos, qAngles, fPlacementSuccess );

		// If the fixed portal is overlapping a portal that was placed before it... kill it!
		if ( fPlacementSuccess )
		{
			IsPortalOverlappingOtherPortals( this, vOrigin, GetAbsAngles(), true );
		}
	}
	else
	{
		StopParticleEffects( this );
	}

	if (m_pLinkedPair && m_bDontUpdatePair == false)
	{
		m_pLinkedPair->m_bDontUpdatePair = true; // This flag prevents endless looping
		m_pLinkedPair->InputSetActivatedState(inputdata);
	}

	UpdatePortalTeleportMatrix();
	UpdatePortalLinkage();

	if (m_bDontUpdatePair)
	{
		m_bDontUpdatePair = false;
	}
}

void CLinkedPortalDoor::DoFizzleEffect( int iEffect, bool bDelayedPos /*= true*/ )
{
	// Rumble effects on the firing player (if one exists)
	CWeaponPortalgun *pPortalGun = dynamic_cast<CWeaponPortalgun*>( m_hPlacedBy.Get() );

	if ( pPortalGun && (iEffect != PORTAL_FIZZLE_CLOSE ) 
				    && (iEffect != PORTAL_FIZZLE_SUCCESS )
				    && (iEffect != PORTAL_FIZZLE_NONE )		)
	{
		CBasePlayer* pPlayer = (CBasePlayer*)pPortalGun->GetOwner();
		if ( pPlayer )
		{
			pPlayer->RumbleEffect( RUMBLE_PORTAL_PLACEMENT_FAILURE, 0, RUMBLE_FLAGS_NONE );
		}
	}
}

void CLinkedPortalDoor::Activate()
{
	if ( s_PortalLinkageGroups[m_iLinkageGroupID].Find(this) == -1 )
	{
		s_PortalLinkageGroups[m_iLinkageGroupID].AddToTail(this);
	}

	if ( m_pAttachedCloningArea == NULL )
	{
		m_pAttachedCloningArea = CPhysicsCloneArea::CreatePhysicsCloneArea(this);
	}

	UpdatePortalTeleportMatrix();
	
	UpdatePortalLinkage();

	BaseClass::BaseClass::Activate();

	AddEffects( EF_NOSHADOW | EF_NORECEIVESHADOW );

	if( m_bActivated && (m_hLinkedPortal.Get() != NULL) )
	{
		Vector ptCenter = GetAbsOrigin();
		QAngle qAngles = GetAbsAngles();
		m_PortalSimulator.MoveTo( ptCenter, qAngles );

		//resimulate everything we're touching
		touchlink_t *root = ( touchlink_t * )GetDataObject( TOUCHLINK );
		if( root )
		{
			for( touchlink_t *link = root->nextLink; link != root; link = link->nextLink )
			{
				CBaseEntity *pOther = link->entityTouched;
				if( CProp_Portal_Shared::IsEntityTeleportable( pOther ) )
				{
					CCollisionProperty *pOtherCollision = pOther->CollisionProp();
					Vector vWorldMins, vWorldMaxs;
					pOtherCollision->WorldSpaceAABB( &vWorldMins, &vWorldMaxs );
					Vector ptOtherCenter = (vWorldMins + vWorldMaxs) / 2.0f;

					if( m_plane_Origin.normal.Dot( ptOtherCenter ) > m_plane_Origin.dist )
					{
						//we should be interacting with this object, add it to our environment
						if( SharedEnvironmentCheck( pOther ) )
						{
							Assert( ((m_PortalSimulator.GetLinkedPortalSimulator() == NULL) && (m_hLinkedPortal.Get() == NULL)) || 
								(m_PortalSimulator.GetLinkedPortalSimulator() == &m_hLinkedPortal->m_PortalSimulator) ); //make sure this entity is linked to the same portal as our simulator

							CPortalSimulator *pOwningSimulator = CPortalSimulator::GetSimulatorThatOwnsEntity( pOther );
							if( pOwningSimulator && (pOwningSimulator != &m_PortalSimulator) )
								pOwningSimulator->ReleaseOwnershipOfEntity( pOther );

							m_PortalSimulator.TakeOwnershipOfEntity( pOther );
						}
					}
				}
			}
		}
	}
}

void CLinkedPortalDoor::ResetModel()
{
	if(!m_bIsPortal2)
		SetModel("models/portals/portal1.mdl");
	else
		SetModel("models/portals/portal2.mdl");

	SetSize(GetMins(), GetMaxs());

	SetSolid(SOLID_OBB);
	SetSolidFlags(FSOLID_TRIGGER | FSOLID_NOT_SOLID | FSOLID_CUSTOMBOXTEST | FSOLID_CUSTOMRAYTEST);
}
void CLinkedPortalDoor::UpdatePortalLinkage()
{
	if (m_pLinkedPair != nullptr && IsPortal2() == false)
	{
		m_pLinkedPair->SetLinkageGroup(m_iLinkageGroupID);
		if (m_pLinkedPair->IsPortal2() == false)
		{
			m_pLinkedPair->SetIsPortal2(true);
		}
	}

#if DEBUG
	if (m_pDebugMessage != nullptr)
	{
		memset(m_szDebugMessage, 0, 512);
		Q_snprintf(m_szDebugMessage, 512,
			"Name: %s, Pair name: %s Size: %i * %i Status: %u Linkage Group: %u Secondary: %u",
			GetDebugName(), (m_pLinkedPair ? m_pLinkedPair->GetDebugName() : "(none)"),
			(int)m_fWidth, (int)m_fHeight,
			m_bActivated,
			m_iLinkageGroupID,
			IsPortal2()
		);
		m_pDebugMessage->SetMessageText(MAKE_STRING(m_szDebugMessage));
		m_pDebugMessage->SetAbsOrigin(GetAbsOrigin());
	}
#endif

	BaseClass::UpdatePortalLinkage();
}

void CLinkedPortalDoor::SetPartner(CLinkedPortalDoor* pNewPair)
{
	inputdata_t data;
	if (pNewPair != nullptr && pNewPair != m_pLinkedPair)
	{
		DEBUG_MSG("%s (InputSetPartner) ==> Setting up new pair '%s'!\n", GetDebugName(), m_pLinkedPair->GetDebugName());
		// Find a new linkage group for the old pair
		int i = FindFreeGroup(false);
		if (i < 0)
		{
			Warning("%s (InputSetPartner) ==> Failed to find empty linkage group for old pair '%s'!\n",
				GetDebugName(), m_pLinkedPair->GetDebugName()
			);
			return;
		}

		// Check if the portal had a linked pair previously
		if (m_pLinkedPair != nullptr)
		{
			// Set the "secondary" flag and group id of the new pair.
			m_pLinkedPair->SetIsPortal2(pNewPair->IsPortal2());
			m_pLinkedPair->m_iLinkageGroupID = pNewPair->m_iLinkageGroupID;
			// Remove old pair
			m_pLinkedPair->m_pLinkedPair = nullptr;

			// Deactivate old pair
			data.value.SetBool(false);
			m_pLinkedPair->InputSetActivatedState(data);
			// Update linkage
			m_pLinkedPair->UpdatePortalLinkage();
		}

		if (IsPortal2())
		{
			SetIsPortal2(false);
		}

		// Assign new pair
		m_pLinkedPair = pNewPair;
		// Update linkage group
		m_pLinkedPair->m_iLinkageGroupID = m_iLinkageGroupID;
		// Update "secondary" flag
		if (m_pLinkedPair->IsPortal2() == false)
		{
			m_pLinkedPair->SetIsPortal2(false);
		}
		// Set pair
		m_pLinkedPair->m_pLinkedPair = this;

		// Update active state of the new pair
		data.value.SetBool(m_bActivated);
		m_pLinkedPair->InputSetActivatedState(data);
		// Update linkage
		m_pLinkedPair->UpdatePortalLinkage();
		UpdatePortalLinkage();
	}
}

void CLinkedPortalDoor::Open()
{
	SetActiveState(true);
}

void CLinkedPortalDoor::Close()
{
	SetActiveState(false);
}

void CLinkedPortalDoor::InputOpen(inputdata_t& inputdata)
{
	Open();
}

void CLinkedPortalDoor::InputClose(inputdata_t& inputdata)
{
	Close();
}

void CLinkedPortalDoor::SetActiveState(bool active)
{
	inputdata_t data;
	data.value.SetBool(active);
	InputSetActivatedState(data);
}

void CLinkedPortalDoor::UnlinkPair()
{
	SetActiveState(false);
	if (m_pLinkedPair != nullptr)
	{
		m_pLinkedPair->SetActiveState(false);
		CLinkedPortalDoor* pPair = m_pLinkedPair;
		m_pLinkedPair = nullptr;
		pPair->UnlinkPair();
	}
}

void CLinkedPortalDoor::SetPortalSize(float fWidth, float fHeight)
{
	m_fWidth = fWidth;
	m_fHeight = fHeight;
}

void CLinkedPortalDoor::SetLinkageGroup(int iLinkageGroup)
{
	m_iLinkageGroupID = iLinkageGroup;
}

void CLinkedPortalDoor::InputSetPartner(inputdata_t& inputdata)
{
	const char* szNewPair = inputdata.value.String();
	CBaseEntity* pNewPairBase = gEntList.FindEntityByName(nullptr, MAKE_STRING(szNewPair));

	if (pNewPairBase == nullptr)
	{
		Warning("%s (InputSetPartner) ==> Failed to find new pair '%s'!\n",
			GetDebugName(), szNewPair
		);
		return;
	}

	CLinkedPortalDoor* pNewPair = dynamic_cast<CLinkedPortalDoor*>(pNewPairBase);
	// Check if conversion has succeeded.
	if (pNewPair == nullptr)
	{
		Warning("%s (InputSetPartner) ==> Failed to assign new pair '%s': Pair must be a 'linked_portal_door' entity!\n",
			GetDebugName(), pNewPair
		);
	}
	// Show debug message if the new pair is the same as the old one.
	if (pNewPair == m_pLinkedPair)
	{
		Warning("%s (InputSetPartner) ==> Old pair is reassigned! (%s)\n",
			GetDebugName(), szNewPair
		);
	}

	SetPartner(pNewPair);
}

bool CLinkedPortalDoor::IsLinkedTo(CLinkedPortalDoor* pPortalDoor)
{
	return m_pLinkedPair == pPortalDoor;
}

CLinkedPortalDoor* CLinkedPortalDoor::GetPartner()
{
	return m_pLinkedPair;
}
