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

extern CUtlVector<CProp_Portal *> s_PortalLinkageGroups[256];

LINK_ENTITY_TO_CLASS(linked_portal_door, CLinkedPortalDoor);

IMPLEMENT_SERVERCLASS_ST(CLinkedPortalDoor, DT_LinkedPortalDoor)
END_SEND_TABLE();

BEGIN_DATADESC(CLinkedPortalDoor)
END_DATADESC();

CLinkedPortalDoor::CLinkedPortalDoor()
	: BaseClass()
{
	physcollision->DestroyCollide(m_pCollisionShape);
	m_pCollisionShape = nullptr;
}

CLinkedPortalDoor::~CLinkedPortalDoor()
{
}

void CLinkedPortalDoor::Spawn()
{
	BaseClass::Spawn();

	m_PortalSimulator.SetWidth(m_fWidth);
	m_PortalSimulator.SetHeight(m_fHeight);

#ifdef DEBUG
	Msg("---> World portal (%s) size -> (%f %f)\n", GetDebugName(), m_PortalSimulator.GetWidth(), m_PortalSimulator.GetHeight());
#endif
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

#ifdef DEBUG
	Msg("World portal (%s) size -> (%f %f)\n", GetDebugName(), m_PortalSimulator.GetWidth(), m_PortalSimulator.GetHeight());
#endif
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
}

void CLinkedPortalDoor::InputSetActivatedState( inputdata_t &inputdata )
{
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

	UpdatePortalTeleportMatrix();

	UpdatePortalLinkage();
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
	if(s_PortalLinkageGroups[m_iLinkageGroupID].Find( this ) == -1 )
		s_PortalLinkageGroups[m_iLinkageGroupID].AddToTail( this );

	if( m_pAttachedCloningArea == NULL )
		m_pAttachedCloningArea = CPhysicsCloneArea::CreatePhysicsCloneArea( this );

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