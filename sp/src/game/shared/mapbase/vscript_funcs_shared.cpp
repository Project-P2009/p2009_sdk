//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Due to this being a custom integration of VScript based on the Alien Swarm SDK, we don't have access to
//			some of the code normally available in games like L4D2 or Valve's original VScript DLL.
//			Instead, that code is recreated here, shared between server and client.
//
//			It also contains other functions unique to Mapbase.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#ifndef CLIENT_DLL
#include "globalstate.h"
#include "vscript_server.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL
extern ConVar sv_script_think_interval;

void AddThinkToEnt( HSCRIPT entity, const char *pszFuncName )
{
	CBaseEntity *pEntity = ToEnt( entity );
	if (!pEntity)
		return;

	if (pszFuncName == NULL || pszFuncName[0] == '\0')
		pEntity->m_iszScriptThinkFunction = NULL_STRING;
	else
		pEntity->m_iszScriptThinkFunction = AllocPooledString(pszFuncName);

	pEntity->SetContextThink( &CBaseEntity::ScriptThink, gpGlobals->curtime + sv_script_think_interval.GetFloat(), "ScriptThink" );
}

HSCRIPT EntIndexToHScript( int index )
{
	return ToHScript( UTIL_EntityByIndex( index ) );
}
#endif

//-----------------------------------------------------------------------------
// Mapbase-specific functions start here
//-----------------------------------------------------------------------------

#ifndef CLIENT_DLL
inline CScriptKeyValues *ToScriptKeyValues( HSCRIPT hKV )
{
	return (hKV) ? (CScriptKeyValues*)g_pScriptVM->GetInstanceValue( hKV, GetScriptDescForClass( CScriptKeyValues ) ) : NULL;
}

// Since IScriptVM::GetKeyValue() is unsupported, Mapbase uses this CScriptKeyValues version
HSCRIPT SpawnEntityFromKeyValues( const char *pszClassname, HSCRIPT hKV )
{
	CBaseEntity *pEntity = CreateEntityByName( pszClassname );
	if ( !pEntity )
	{
		Assert( !"SpawnEntityFromTable: only works for CBaseEntities" );
		return NULL;
	}

	gEntList.NotifyCreateEntity( pEntity );

	CScriptKeyValues *pScriptKV = ToScriptKeyValues( hKV );
	if (pScriptKV)
	{
		KeyValues *pKV = pScriptKV->m_pKeyValues;
		for (pKV = pKV->GetFirstSubKey(); pKV != NULL; pKV = pKV->GetNextKey())
		{
			//g_pScriptVM->GetKeyValue( hKV, i, &varKey, &varValue );
			pEntity->KeyValue( pKV->GetName(), pKV->GetString() );
		}
	}

	DispatchSpawn( pEntity );

	return ToHScript( pEntity );
}
#endif

void RegisterSharedScriptFunctions()
{
	// 
	// Due to this being a custom integration of VScript based on the Alien Swarm SDK, we don't have access to
	// some of the code normally available in games like L4D2 or Valve's original VScript DLL.
	// Instead, that code is recreated here, shared between server and client.
	// 
	ScriptRegisterFunction( g_pScriptVM, RandomFloat, "Generate a random floating point number within a range, inclusive." );
	ScriptRegisterFunction( g_pScriptVM, RandomInt, "Generate a random integer within a range, inclusive." );

#ifndef CLIENT_DLL
	ScriptRegisterFunctionNamed( g_pScriptVM, NDebugOverlay::BoxDirection, "DebugDrawBoxDirection", "Draw a debug forward box" );
	ScriptRegisterFunctionNamed( g_pScriptVM, NDebugOverlay::Text, "DebugDrawText", "Draw a debug overlay text" );

	ScriptRegisterFunction( g_pScriptVM, AddThinkToEnt, "This will put a think function onto an entity, or pass null to remove it. This is NOT chained, so be careful." );
	ScriptRegisterFunction( g_pScriptVM, EntIndexToHScript, "Returns the script handle for the given entity index." );
#endif

	// Functions unique to Mapbase
#ifndef CLIENT_DLL
	ScriptRegisterFunction( g_pScriptVM, SpawnEntityFromKeyValues, "Spawns an entity with the keyvalues in a CScriptKeyValues handle." );
#endif

#ifdef CLIENT_DLL
	VScriptRunScript( "vscript_client", true );
#else
	VScriptRunScript( "vscript_server", true );
#endif
}
