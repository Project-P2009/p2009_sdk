//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The various Cores of Aperture Science.
//
//=====================================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "ai_baseactor.h"
#include "props.h"				// CPhysicsProp base class
#include "saverestore_utlvector.h"

#define GLADOS_CORE_MODEL_NAME "models/npcs/personality_sphere/personality_sphere_skins.mdl" 

static const char *s_pAnimateThinkContext = "Animate";

#define SPHERE01_LOOK_ANINAME			"core02_idle"
#define SPHERE02_LOOK_ANINAME			"core01_idle"
#define SPHERE03_LOOK_ANINAME			"core02_idle"

#define SPHERE01_SKIN					0
#define SPHERE02_SKIN					1
#define SPHERE03_SKIN					2

class CPropPersonalityCore : public CPhysicsProp
{
public:
	DECLARE_CLASS( CPropPersonalityCore, CPhysicsProp );
	DECLARE_DATADESC();

	CPropPersonalityCore();

	typedef enum 
	{
		CORETYPE_SPHERE01,
		CORETYPE_SPHERE02,
		CORETYPE_SPHERE03,
		CORETYPE_NONE,

	} CORETYPE;

	virtual void Spawn( void );
	virtual void Precache( void );

	virtual QAngle	PreferredCarryAngles( void ) { return QAngle( 180, -90, 180 ); }
	virtual bool	HasPreferredCarryAnglesForPlayer( CBasePlayer *pPlayer ) { return true; }

	void	TalkingThink(void);
	void	AnimateThink ( void );

	void	SetupVOList(void);
	
	void	OnPhysGunPickup( CBasePlayer* pPhysGunUser, PhysGunPickup_t reason );

private:
	bool m_bFirstPickup;

	string_t	m_iszLookAnimationName;		// Different animations for each personality

	CORETYPE m_iCoreType;
};

LINK_ENTITY_TO_CLASS( prop_personality_core, CPropPersonalityCore );

//-----------------------------------------------------------------------------
// Save/load 
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CPropPersonalityCore )

	DEFINE_FIELD( m_iszLookAnimationName,					FIELD_STRING ),
	DEFINE_FIELD( m_bFirstPickup,							FIELD_BOOLEAN ),

	DEFINE_KEYFIELD( m_iCoreType,			FIELD_INTEGER, "CoreType" ),
	
	DEFINE_THINKFUNC(TalkingThink),
	DEFINE_THINKFUNC( AnimateThink ),
	
END_DATADESC()

CPropPersonalityCore::CPropPersonalityCore()
{
	m_bFirstPickup = true;
}

void CPropPersonalityCore::Spawn( void )
{
	SetupVOList();

	Precache();
	KeyValue( "model", GLADOS_CORE_MODEL_NAME );
	BaseClass::Spawn();

	//Default to 'dropped' animation
	ResetSequence(LookupSequence(STRING(m_iszLookAnimationName)));
	SetCycle(1.0f);

	DisableAutoFade();

	SetContextThink( &CPropPersonalityCore::AnimateThink, gpGlobals->curtime + 0.1f, s_pAnimateThinkContext );
}

void CPropPersonalityCore::Precache( void )
{
	BaseClass::Precache();
	
	PrecacheModel( GLADOS_CORE_MODEL_NAME );
}

//-----------------------------------------------------------------------------
// Purpose: Start playing personality VO list
//-----------------------------------------------------------------------------
void CPropPersonalityCore::TalkingThink(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CPropPersonalityCore::AnimateThink()
{
	StudioFrameAdvance();
	SetContextThink( &CPropPersonalityCore::AnimateThink, gpGlobals->curtime + 0.1f, s_pAnimateThinkContext );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CPropPersonalityCore::SetupVOList(void)
{
	switch (m_iCoreType)
	{
	case CORETYPE_SPHERE01:
	{
		m_iszLookAnimationName = AllocPooledString(SPHERE01_LOOK_ANINAME);
		m_nSkin = SPHERE01_SKIN;
	}
	break;
	case CORETYPE_SPHERE02:
	{
		m_iszLookAnimationName = AllocPooledString(SPHERE02_LOOK_ANINAME);
		m_nSkin = SPHERE02_SKIN;
	}
	break;
	case CORETYPE_SPHERE03:
	{
		m_iszLookAnimationName = AllocPooledString(SPHERE03_LOOK_ANINAME);
		m_nSkin = SPHERE03_SKIN;
	}
	break;
	default:
	{
		m_iszLookAnimationName = AllocPooledString(SPHERE01_LOOK_ANINAME);
		m_nSkin = SPHERE01_SKIN;
	}
	break;
	};
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
//-----------------------------------------------------------------------------
void CPropPersonalityCore::OnPhysGunPickup( CBasePlayer* pPhysGunUser, PhysGunPickup_t reason )
{
	// +use always enables motion on these props
	EnableMotion();

	BaseClass::OnPhysGunPickup ( pPhysGunUser, reason );
}