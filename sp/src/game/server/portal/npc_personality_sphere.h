//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//
//=============================================================================//

#include "npc_playercompanion.h"
#include "ai_playerally.h"

enum SPHERESKIN
{
	SKIN_SPHERE01 = 0,
	SKIN_SPHERE02 = 1,
	SKIN_SPHERE03 = 2,
	SKIN_WHEATLEY_CRACKED = 0,
	SKIN_WHEATLEY = 1,
};

enum SPHERETYPE
{
	SPHERE01 = 0,
	SPHERE02 = 1,
	SPHERE03 = 2,
	WHEATLEY = 3,
};

//-----------------------------------------------------------------------------
// CPersonalitySphereController implementation
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Purpose: This class only implements the IMotionEvent-specific behavior
//			It keeps track of the forces so they can be integrated
//-----------------------------------------------------------------------------
class CPersonalitySphereController : public IMotionEvent
{
	DECLARE_SIMPLE_DATADESC();

public:
	IMotionEvent::simresult_e Simulate(IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular);

	AngularImpulse	m_vecAngular;
	Vector			m_vecLinear;

	void Off(void) { m_fIsStopped = true; }
	void On(void) { m_fIsStopped = false; }

	bool IsOn(void) { return !m_fIsStopped; }

	bool	m_fIsStopped;

};

BEGIN_SIMPLE_DATADESC(CPersonalitySphereController)

DEFINE_FIELD(m_vecAngular, FIELD_VECTOR),
DEFINE_FIELD(m_vecLinear, FIELD_VECTOR),
DEFINE_FIELD(m_fIsStopped, FIELD_BOOLEAN),

END_DATADESC()


//-----------------------------------------------------------------------------
IMotionEvent::simresult_e CPersonalitySphereController::Simulate(IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular)
{

	linear = m_vecLinear;
	angular = m_vecAngular;

	return IMotionEvent::SIM_LOCAL_ACCELERATION;
}
//-----------------------------------------------------------------------------

class CNPC_PersonalitySphere : public CNPCBaseInteractive<CAI_BaseNPC>, public CDefaultPlayerPickupVPhysics
{
public:

	DECLARE_CLASS(CNPC_PersonalitySphere, CNPCBaseInteractive<CAI_BaseNPC>);
	DECLARE_DATADESC();
	//	DECLARE_SERVERCLASS();

	CNPC_PersonalitySphere();
	~CNPC_PersonalitySphere();

	void			Precache();
	void			Spawn();

	void 			PlayAttach(animevent_t *pEvent);
	void 			PlayDettach(animevent_t *pEvent);
	virtual void	OnPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason);
	virtual void	OnPhysGunDrop(CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason);
	virtual int		OnTakeDamage(const CTakeDamageInfo &info);
	virtual int		VPhysicsTakeDamage(const CTakeDamageInfo &info);

	bool			BecomePhysical();

	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
		CBasePlayer *pPlayer = ToBasePlayer(pActivator);
		if (pPlayer)
		{
			pPlayer->PickupObject(this, false);
		}
	}

	void			SetSphereSkin(void);
	void			VPhysicsThink(void);

	bool			m_bAltModel;
	bool			m_bHeld;
	bool			m_bWheatleyFixed;

	SPHERETYPE m_iSphereType;

	CPersonalitySphereController			m_PersonalitySphereController;
	IPhysicsMotionController	*m_pMotionController;

	COutputEvent m_OnPhysGunDrop;
	COutputEvent m_OnPhysGunPickup;

};