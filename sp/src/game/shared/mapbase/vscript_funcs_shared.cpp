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

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CScriptConvarLookup
{
public:

#ifndef CLIENT_DLL
	const char *GetClientConvarValue( const char *pszConVar, int entindex )
	{
		return engine->GetClientConVarValue( entindex, pszConVar );
	}
#endif

	const char *GetStr( const char *pszConVar )
	{
		ConVarRef cvar( pszConVar );
		return cvar.GetString();
	}

	float GetFloat( const char *pszConVar )
	{
		ConVarRef cvar( pszConVar );
		return cvar.GetFloat();
	}

	void SetValue( const char *pszConVar, const char *pszValue )
	{
		ConVarRef cvar( pszConVar );
		if (!cvar.IsValid())
			return;

		// FCVAR_NOT_CONNECTED can be used to protect specific convars from nefarious interference
		if (cvar.IsFlagSet(FCVAR_NOT_CONNECTED))
			return;

		cvar.SetValue( pszValue );
	}

private:
} g_ScriptConvarLookup;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptConvarLookup, "Convars", SCRIPT_SINGLETON "Provides an interface for getting and setting convars." )
#ifndef CLIENT_DLL
	DEFINE_SCRIPTFUNC( GetClientConvarValue, "Returns the convar value for the entindex as a string. Only works with client convars with the FCVAR_USERINFO flag." )
#endif
	DEFINE_SCRIPTFUNC( GetStr, "Returns the convar as a string. May return null if no such convar." )
	DEFINE_SCRIPTFUNC( GetFloat, "Returns the convar as a float. May return null if no such convar." )
	DEFINE_SCRIPTFUNC( SetValue, "Sets the value of the convar. Supported types are bool, int, float, string." )
END_SCRIPTDESC();

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

void ParseScriptTableKeyValues( CBaseEntity *pEntity, HSCRIPT hKV )
{
	int nIterator = -1;
	ScriptVariant_t varKey, varValue;
	while ((nIterator = g_pScriptVM->GetKeyValue( hKV, nIterator, &varKey, &varValue )) != -1)
	{
		switch (varValue.m_type)
		{
			case FIELD_CSTRING:		pEntity->KeyValue( varKey.m_pszString, varValue.m_pszString ); break;
			case FIELD_FLOAT:		pEntity->KeyValue( varKey.m_pszString, varValue.m_float ); break;
			case FIELD_VECTOR:		pEntity->KeyValue( varKey.m_pszString, *varValue.m_pVector ); break;
		}

		g_pScriptVM->ReleaseValue( varKey );
		g_pScriptVM->ReleaseValue( varValue );
	}
}

void PrecacheEntityFromTable( const char *pszClassname, HSCRIPT hKV )
{
	if ( IsEntityCreationAllowedInScripts() == false )
	{
		Warning( "VScript error: A script attempted to create an entity mid-game. Due to the server's settings, entity creation from scripts is only allowed during map init.\n" );
		return;
	}

	// This is similar to UTIL_PrecacheOther(), but we can't check if we can only precache it once.
	// Probably for the best anyway, as similar classes can still have different precachable properties.
	CBaseEntity *pEntity = CreateEntityByName( pszClassname );
	if (!pEntity)
	{
		Assert( !"PrecacheEntityFromTable: only works for CBaseEntities" );
		return;
	}

	ParseScriptTableKeyValues( pEntity, hKV );

	pEntity->Precache();

	UTIL_RemoveImmediate( pEntity );
}

HSCRIPT SpawnEntityFromTable( const char *pszClassname, HSCRIPT hKV )
{
	if ( IsEntityCreationAllowedInScripts() == false )
	{
		Warning( "VScript error: A script attempted to create an entity mid-game. Due to the server's settings, entity creation from scripts is only allowed during map init.\n" );
		return NULL;
	}

	CBaseEntity *pEntity = CreateEntityByName( pszClassname );
	if ( !pEntity )
	{
		Assert( !"SpawnEntityFromTable: only works for CBaseEntities" );
		return NULL;
	}

	gEntList.NotifyCreateEntity( pEntity );

	ParseScriptTableKeyValues( pEntity, hKV );

	DispatchSpawn( pEntity );

	return ToHScript( pEntity );
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

HSCRIPT SpawnEntityFromKeyValues( const char *pszClassname, HSCRIPT hKV )
{
	if ( IsEntityCreationAllowedInScripts() == false )
	{
		Warning( "VScript error: A script attempted to create an entity mid-game. Due to the server's settings, entity creation from scripts is only allowed during map init.\n" );
		return NULL;
	}

	CBaseEntity *pEntity = CreateEntityByName( pszClassname );
	if ( !pEntity )
	{
		Assert( !"SpawnEntityFromKeyValues: only works for CBaseEntities" );
		return NULL;
	}

	gEntList.NotifyCreateEntity( pEntity );

	CScriptKeyValues *pScriptKV = ToScriptKeyValues( hKV );
	if (pScriptKV)
	{
		KeyValues *pKV = pScriptKV->m_pKeyValues;
		for (pKV = pKV->GetFirstSubKey(); pKV != NULL; pKV = pKV->GetNextKey())
		{
			pEntity->KeyValue( pKV->GetName(), pKV->GetString() );
		}
	}

	DispatchSpawn( pEntity );

	return ToHScript( pEntity );
}

void ScriptDispatchSpawn( HSCRIPT hEntity )
{
	CBaseEntity *pEntity = ToEnt( hEntity );
	if (pEntity)
	{
		DispatchSpawn( pEntity );
	}
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
	ScriptRegisterFunction( g_pScriptVM, PrecacheEntityFromTable, "Precache an entity from KeyValues in a table." );
	ScriptRegisterFunction( g_pScriptVM, SpawnEntityFromTable, "Native function for entity spawning." );
#endif

	g_pScriptVM->RegisterInstance( &g_ScriptConvarLookup, "Convars" );

	// Functions unique to Mapbase
#ifndef CLIENT_DLL
	ScriptRegisterFunction( g_pScriptVM, SpawnEntityFromKeyValues, "Spawns an entity with the keyvalues in a CScriptKeyValues handle." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptDispatchSpawn, "DispatchSpawn", "Spawns an unspawned entity." );
#endif

#ifdef CLIENT_DLL
	VScriptRunScript( "vscript_client", true );
#else
	VScriptRunScript( "vscript_server", true );
#endif
}
