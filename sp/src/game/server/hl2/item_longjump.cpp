//========= Copyright  1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//


#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "items.h"
#include "items.h"
#include "portal_player.h"


class CItemLongJump : public CItem
{
public:
	DECLARE_CLASS(CItemLongJump, CItem);

	void Spawn(void)
	{
		Precache();
		SetModel("models/w_longjump.mdl");
		BaseClass::Spawn();

		CollisionProp()->UseTriggerBounds(true, 16.0f);
	}
	void Precache(void)
	{
		PrecacheModel("models/w_longjump.mdl");
	}
	bool MyTouch(CPortal_Player *pPlayer)
	{
		if (pPlayer->m_bHasLongJump == true)
		{
			return false;
		}

		if (pPlayer->IsSuitEquipped())
		{
			pPlayer->m_bHasLongJump = true;// player now has longjump module

			CSingleUserRecipientFilter user(pPlayer);
			user.MakeReliable();

			UserMessageBegin(user, "ItemPickup");
			WRITE_STRING(STRING(m_iClassname));
			MessageEnd();

			UTIL_EmitSoundSuit(pPlayer->edict(), "!HEV_A1");	// Play the longjump sound UNDONE: Kelly? correct sound?
			return true;
		}
		return false;
	}
};

LINK_ENTITY_TO_CLASS(item_longjump, CItemLongJump);
PRECACHE_REGISTER(item_longjump);

