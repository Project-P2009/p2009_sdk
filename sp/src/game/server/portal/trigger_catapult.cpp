//=========  ============//
//
// Purpose: Replica of Portal 2's entity by the same name.
// Code taken from Bob's portal, credit ASBob (https://github.com/ACBob/bobportal/blob/master/sp/src/game/server/bobportal/trigger_catapult.cpp)
//
//=============================================================================

#include "cbase.h"
#include "triggers.h"
#include "movevars_shared.h"
#include "ai_basenpc.h" // provides VecCheckToss(), which is used when lobbing physics objects
#include "player_pickup.h"
#include "const.h"

ConVar sv_debug_catapults( "sv_debug_catapults", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "Display some debug information for catapults." );

class CTriggerCatapult : public CBaseVPhysicsTrigger {
    public:
        DECLARE_CLASS(CTriggerCatapult, CBaseVPhysicsTrigger);
        DECLARE_DATADESC();

        CTriggerCatapult();

        void Spawn();
        void StartTouch(CBaseEntity *pOther);
        void BopIt(CBaseEntity *pOther);
        
        Vector CalcJumpLaunchVelocity(const Vector &startPos, const Vector &endPos, float flGravity, float *pminHeight, float maxHorzVelocity, Vector *pvecApex );

        Vector GenerateVelocity(Vector vecOther, Vector vecTarget, IPhysicsObject *physObject);

        bool VelocityThreshold(Vector velOther);

        void EnablePlayerMovement();

    protected:
        string_t m_jumpTarget;

        float m_PlayerLaunchSpeed;
        float m_PhysicsLaunchSpeed;

        float m_velThresholdLow;
        float m_velThresholdHigh;

        float m_flDisableMovementTime;

        CBasePlayer *m_lastPlayerHit; // FIXME: will not work in multiplayer, as if the next player triggers us, we will never re-enable the other player's movement

        COutputEvent m_OutputOnCatapulted;
        
        Vector m_vLaunchDirection;
        QAngle m_LaunchDirection;

        bool m_bLaunchByAngles;
        bool m_bApplyRandomRotation;
        bool m_bOnlyCheckVelocity;
        bool m_bTestVelocity;
};

LINK_ENTITY_TO_CLASS(trigger_catapult, CTriggerCatapult)

BEGIN_DATADESC( CTriggerCatapult )

    DEFINE_KEYFIELD(m_jumpTarget, FIELD_STRING, "launchTarget"),
    DEFINE_KEYFIELD(m_PlayerLaunchSpeed, FIELD_FLOAT, "playerSpeed"),
    DEFINE_KEYFIELD(m_PhysicsLaunchSpeed, FIELD_FLOAT, "physicsSpeed"),
    DEFINE_KEYFIELD(m_bApplyRandomRotation, FIELD_BOOLEAN, "applyAngularImpulse"),
    DEFINE_KEYFIELD(m_bOnlyCheckVelocity, FIELD_BOOLEAN, "onlyVelocityCheck"),
    DEFINE_KEYFIELD(m_vLaunchDirection, FIELD_VECTOR, "launchDirection"),
    DEFINE_KEYFIELD(m_velThresholdLow, FIELD_FLOAT, "lowerThreshold"),
    DEFINE_KEYFIELD(m_velThresholdHigh, FIELD_FLOAT, "upperThreshold"),
    DEFINE_KEYFIELD(m_bTestVelocity, FIELD_BOOLEAN, "useThresholdCheck"),
    DEFINE_KEYFIELD(m_flDisableMovementTime, FIELD_FLOAT, "AirCtrlSupressionTime"),

    DEFINE_OUTPUT(m_OutputOnCatapulted, "OnCatapulted")

END_DATADESC()

CTriggerCatapult::CTriggerCatapult(){}

void CTriggerCatapult::Spawn() {
    BaseClass::Spawn();

    SetSolid(SOLID_VPHYSICS);
    SetModel( STRING( GetModelName() ) );

    m_bLaunchByAngles = (m_jumpTarget == NULL_STRING);

    if (m_bLaunchByAngles) { // Copied from trigger_push
        // Convert launch direction from angles to a vector
        Vector vecAbsDir;
        QAngle angPushDir = QAngle(m_vLaunchDirection.x, m_vLaunchDirection.y, m_vLaunchDirection.z);
        AngleVectors(angPushDir, &vecAbsDir);

        // Convert it to entity space
        VectorIRotate( vecAbsDir, EntityToWorldTransform(), m_vLaunchDirection );
    }

    AddSolidFlags(FSOLID_NOT_SOLID | FSOLID_TRIGGER);

    VPhysicsInitShadow( false, false );

    if (m_flDisableMovementTime < 0) {
        m_flDisableMovementTime = 0.25; // Quarter second default if below -1
    }

    RegisterThinkContext("enableMovementContext");
}

void CTriggerCatapult::StartTouch(CBaseEntity *pOther) { // FIXME: We only launch if they have just started to touch us, but is that the behaviour we want?
    BaseClass::StartTouch(pOther);

    if (PassesTriggerFilters(pOther)) {
        if (!m_bOnlyCheckVelocity)
            BopIt(pOther);
        else {
            // We only check the velocity, so do so
            if (VelocityThreshold(pOther->GetAbsVelocity()))
                m_OutputOnCatapulted.FireOutput(this, this);
        }
    }

}

bool CTriggerCatapult::VelocityThreshold(Vector velOther) {

    if (!m_bTestVelocity) // If we're not set to test velocity, then we launch anyway.
        return true;

    // According to https://developer.valvesoftware.com/wiki/Faith_Plate#Tips, these aren't too simple - It also uses the player's speed!
    // Lower threshold is 100% - whatever the value
    // Upper threshold is 100 + whatever the value
    // It's confusing, but I'm going for a recreation of portal 2's entity.
    float speedLower = m_PlayerLaunchSpeed * (1.0 - m_velThresholdLow);
    float speedHigher = m_PlayerLaunchSpeed * (1.0 + m_velThresholdHigh);

    if (velOther.Length() >= speedLower && velOther.Length() <= speedHigher) {
        return true;
    }

    return false;

}

void CTriggerCatapult::BopIt(CBaseEntity *pOther) {

    if (!VelocityThreshold(pOther->GetAbsVelocity())) // Not going fast enough/too fast?
        return;
    
    CBaseEntity *pJumpTarget = NULL;

    // If we are willing to launch by target, we look for the target.
    if (m_jumpTarget != NULL_STRING) {
        // Get the entitiy
        pJumpTarget = gEntList.FindEntityByName(pJumpTarget, m_jumpTarget);

        // If we can't find it, we warn the stupid mapper and then choose to launch by angles anyway.
        if (pJumpTarget == NULL) {
            Warning("prop_catapult at %.0f %.0f %.0f has a target set, but we can't find a target by that name (%s).\nUsing angle launch instead...", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z, m_jumpTarget);
            m_bLaunchByAngles = true;
        }
    }
    else {
        m_bLaunchByAngles = true;
    }

    Vector velGoZoomZoom;

    // If we don't want to launch by angles, we do the cool launch
    if (!m_bLaunchByAngles) {
        velGoZoomZoom = GenerateVelocity(pOther->GetAbsOrigin(), pJumpTarget->GetAbsOrigin(),
            (!pOther->IsPlayer() ? pOther->VPhysicsGetObject() : NULL) ); // if we're not a player, send our vphysics object, else, send NULL
        if (sv_debug_catapults.GetInt())
            NDebugOverlay::Line( GetAbsOrigin(), pJumpTarget->GetAbsOrigin(), 0, 255, 0, 0, 5 ); // Green
    }
    // Else, we just launch by the angle.
    else {
        Vector LaunchDir;
        float LaunchSpeed = pOther->IsPlayer() ? m_PlayerLaunchSpeed : m_PhysicsLaunchSpeed;
	    VectorRotate( m_vLaunchDirection, EntityToWorldTransform(), LaunchDir ); // Convert our angle back to world space
        velGoZoomZoom = LaunchSpeed * LaunchDir; // go!
    }        


    pOther->SetGroundEntity(NULL); // Essential for allowing the stuff to yEET
    Pickup_ForcePlayerToDropThisObject(pOther); // make the player drop it

    // IF they're a player, AND we have a disable time, AND we aren't launching by angles
    if (pOther->IsPlayer() && m_flDisableMovementTime > 0 && !m_bLaunchByAngles) {

        pOther->AddFlag(FL_ATCONTROLS); // Disable this player's movement HACK: This flag disables the player moving, but not looking. There may be a better flag to use!
        m_lastPlayerHit = (CBasePlayer*) pOther;
        SetContextThink(&CTriggerCatapult::EnablePlayerMovement, gpGlobals->curtime + m_flDisableMovementTime, "enableMovementContext");
    }


    if (pOther->IsPlayer()) {
        pOther->SetAbsVelocity(velGoZoomZoom);
    }
    else if (pOther->GetMoveType() == MOVETYPE_VPHYSICS && pOther->VPhysicsGetObject()) { // We launch physics objects here
        IPhysicsObject *pPhysObject = pOther->VPhysicsGetObject();

        AngularImpulse velRotZoomZoom = Vector();
        if (m_bApplyRandomRotation) {
            velRotZoomZoom = RandomAngularImpulse( -250 , 250 ) / pPhysObject->GetMass();
        }
        else {
            velRotZoomZoom = RandomAngularImpulse( 0, 0 );
        }

        pPhysObject->SetVelocityInstantaneous(&velGoZoomZoom, &velRotZoomZoom);
    }
    // We have launched the thing!
    m_OutputOnCatapulted.FireOutput(this, this);
}

Vector CTriggerCatapult::GenerateVelocity(Vector vecOther, Vector vecTarget, IPhysicsObject *physObject = NULL) {
    /** Copied parts from the antlion, bodged the rest **/

    bool isPlayer = (physObject == NULL);

    float minJumpHeight;
    if (vecOther.z > vecTarget.z)
        minJumpHeight = GetAbsOrigin().z - vecTarget.z; // It must be ATLEAST high enough for our target
    else if (vecOther.z < vecTarget.z)
        minJumpHeight = GetAbsOrigin().z + vecTarget.z; // It must be ATLEAST high enough for our target

    float maxHorzVel;
    if (isPlayer)
    	maxHorzVel = m_PlayerLaunchSpeed;
    else
        maxHorzVel = m_PhysicsLaunchSpeed;

    Vector vecApex;
    // Calculate it from the entity, doing it from OUR origin is silly.
	Vector rawJumpVel = CalcJumpLaunchVelocity(vecOther, vecTarget, GetCurrentGravity(), &minJumpHeight, maxHorzVel, &vecApex ); 

    // We're not done yet if we're a physics object!
    if (!isPlayer) {
        Vector filteredJumpVel = rawJumpVel;
        Vector velUnit = filteredJumpVel;
        VectorNormalize(velUnit); // Turn it into length(?)

		float flTest = 1000 / filteredJumpVel.Length();

        float flDrag = physObject->CalculateLinearDrag(filteredJumpVel);

        // Add the drag squared to the velocity -
        // Velocity += Direction * flDrag squared
        filteredJumpVel = filteredJumpVel + ( velUnit * ( flDrag * flDrag ) ) / flTest;

        // Set this to the velocity we'll use
        rawJumpVel = filteredJumpVel;
    }

	if ( sv_debug_catapults.GetInt())
	{
		NDebugOverlay::Line( GetAbsOrigin(), vecApex, 255, 0, 255, 0, 5 ); // Magenta

        // Draw the path we SHOULD take
        // FIXME: Inaccurate if we're a phys object
		NDebugOverlay::Line( GetAbsOrigin(), vecApex, 255, 0, 0, 0, 5 ); // Red
		NDebugOverlay::Line( vecApex, vecTarget, 255, 0, 0, 0, 5 ); // Red

        // Display the velocity
        NDebugOverlay::Line( GetAbsOrigin(), rawJumpVel, 255, 255, 0, 0, 5 ); // Yellow
	}

    return rawJumpVel;
}

/** Copied from CAI_MoveProbe, TODO: Is there a better way of accessing this function? **/
Vector CTriggerCatapult::CalcJumpLaunchVelocity(const Vector &startPos, const Vector &endPos, float flGravity, float *pminHeight, float maxHorzVelocity, Vector *pvecApex )
{
	// Get the height I have to jump to get to the target
	float	stepHeight = endPos.z - startPos.z;

	// get horizontal distance to target
	Vector targetDir2D	= endPos - startPos;
	targetDir2D.z = 0;
	float distance = VectorNormalize(targetDir2D);

	Assert( maxHorzVelocity > 0 );

	// get minimum times and heights to meet ideal horz velocity
	float minHorzTime = distance / maxHorzVelocity;
	float minHorzHeight = 0.5 * flGravity * (minHorzTime * 0.5) * (minHorzTime * 0.5);

	// jump height must be enough to hang in the air
	*pminHeight = MAX( *pminHeight, minHorzHeight );
	// jump height must be enough to cover the step up
	*pminHeight = MAX( *pminHeight, stepHeight );

	// time from start to apex
	float t0 = sqrt( ( 2.0 * *pminHeight) / flGravity );
	// time from apex to end
	float t1 = sqrt( ( 2.0 * fabs( *pminHeight - stepHeight) ) / flGravity );

	float velHorz = distance / (t0 + t1);

	Vector jumpVel = targetDir2D * velHorz;

	jumpVel.z = (float)sqrt(2.0f * flGravity * (*pminHeight));

	if (pvecApex)
	{
		*pvecApex = startPos + targetDir2D * velHorz * t0 + Vector( 0, 0, *pminHeight );
	}

	// -----------------------------------------------------------
	// Make the horizontal jump vector and add vertical component
	// -----------------------------------------------------------

	return jumpVel;
}

void CTriggerCatapult::EnablePlayerMovement() {
    if (m_lastPlayerHit != NULL)
        m_lastPlayerHit->RemoveFlag(FL_ATCONTROLS);
}