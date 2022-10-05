#include "baseentity.h"
#include "cbase.h"
#include "datamap.h"
#include "dbg.h"
#include "env_portal_laser.h"
#include "prop_laser_catcher.h"
#include "props.h"				// CPhysicsProp base class
#include "baseanimating.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
#include "util.h"

LINK_ENTITY_TO_CLASS( prop_laser_catcher, CPropLaserCatcher );

BEGIN_DATADESC( CPropLaserCatcher )

	DEFINE_KEYFIELD(, FIELD_STRING, "" ),
	DEFINE_KEYFIELD(, FIELD_STRING, "" ),
	DEFINE_FIELD(name, fieldtype),	

	// Function Pointers
	DEFINE_ENTITYFUNC( TouchThink ),

	// Outputs
	DEFINE_OUTPUT(m_OnPowered, "OnPowered" ),
	DEFINE_OUTPUT(m_OnUnpowered, "OnUnpowered" )
	
END_DATADESC()

CPropLaserCatcher::CPropLaserCatcher(void)
{

}

void CPropLaserCatcher::Spawn(void)
{

}

void CPropLaserCatcher::Precache(void)
{

}

bool CPropLaserCatcher::KeyValue( const char *szKeyName, const char *szValue )
{

}

void CPropLaserCatcher::TouchThink( void )
{

}