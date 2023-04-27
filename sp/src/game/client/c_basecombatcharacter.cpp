//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client's C_BaseCombatCharacter entity
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_basecombatcharacter.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined( CBaseCombatCharacter )
#undef CBaseCombatCharacter	
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter::C_BaseCombatCharacter()
{
	for ( int i=0; i < m_iAmmo.Count(); i++ )
	{
		m_iAmmo.Set( i, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter::~C_BaseCombatCharacter()
{
}

/*
//-----------------------------------------------------------------------------
// Purpose: Returns the amount of ammunition of the specified type the character's carrying
//-----------------------------------------------------------------------------
int	C_BaseCombatCharacter::GetAmmoCount( char *szName ) const
{
	return GetAmmoCount( g_pGameRules->GetAmmoDef()->Index(szName) );
}
*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
}

//-----------------------------------------------------------------------------
// Purpose: Overload our muzzle flash and send it to any actively held weapon
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::DoMuzzleFlash()
{
	// Our weapon takes our muzzle flash command
	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( pWeapon )
	{
		pWeapon->DoMuzzleFlash();
		//NOTENOTE: We do not chain to the base here
	}
	else
	{
		BaseClass::DoMuzzleFlash();
	}
}

IMPLEMENT_CLIENTCLASS(C_BaseCombatCharacter, DT_BaseCombatCharacter, CBaseCombatCharacter);

// Only send active weapon index to local player
BEGIN_RECV_TABLE_NOBASE( C_BaseCombatCharacter, DT_BCCLocalPlayerExclusive )
	RecvPropTime( RECVINFO( m_flNextAttack ) ),
END_RECV_TABLE();


BEGIN_RECV_TABLE(C_BaseCombatCharacter, DT_BaseCombatCharacter)
	RecvPropDataTable( "bcc_localdata", 0, 0, &REFERENCE_RECV_TABLE(DT_BCCLocalPlayerExclusive) ),
	RecvPropEHandle( RECVINFO( m_hActiveWeapon ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_hMyWeapons), RecvPropEHandle( RECVINFO( m_hMyWeapons[0] ) ) ),

#ifdef INVASION_CLIENT_DLL
	RecvPropInt( RECVINFO( m_iPowerups ) ),
#endif

END_RECV_TABLE()


BEGIN_PREDICTION_DATA( C_BaseCombatCharacter )

	DEFINE_PRED_ARRAY( m_iAmmo, FIELD_INTEGER,  MAX_AMMO_TYPES, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flNextAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_hActiveWeapon, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_ARRAY( m_hMyWeapons, FIELD_EHANDLE, MAX_WEAPONS, FTYPEDESC_INSENDTABLE ),

END_PREDICTION_DATA()

#ifdef MAPBASE_VSCRIPT

BEGIN_ENT_SCRIPTDESC( C_BaseCombatCharacter, CBaseEntity, "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetAmmoCount, "GetAmmoCount", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetActiveWeapon, "GetActiveWeapon", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetWeapon, "GetWeapon", "" )
END_SCRIPTDESC();


int C_BaseCombatCharacter::ScriptGetAmmoCount( int i )
{
	Assert( i == -1 || i < MAX_AMMO_SLOTS );

	if ( i < 0 || i >= MAX_AMMO_SLOTS )
		return NULL;

	return GetAmmoCount( i );
}

HSCRIPT C_BaseCombatCharacter::ScriptGetActiveWeapon()
{
	return ToHScript( GetActiveWeapon() );
}

HSCRIPT C_BaseCombatCharacter::ScriptGetWeapon( int i )
{
	Assert( i >= 0 && i < MAX_WEAPONS );

	if ( i < 0 || i >= MAX_WEAPONS )
		return NULL;

	return ToHScript( GetWeapon(i) );
}

#endif
