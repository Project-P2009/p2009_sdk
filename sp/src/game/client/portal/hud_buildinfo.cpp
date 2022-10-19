//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "con_nprint.h"
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/Panel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CHudBuildInfo : public Panel, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudBuildInfo, Panel );

public:
	CHudBuildInfo( const char *pElementName );

	virtual void Paint( void );

};

DECLARE_HUDELEMENT( CHudBuildInfo );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudBuildInfo::CHudBuildInfo( const char *pElementName ) : BaseClass( NULL, "BuildInfo" ), CHudElement( pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	SetPaintBackgroundType( 0 );
	SetPaintBackgroundEnabled( false );
    SetPaintBorderEnabled(false);
    // SetBgColor(Color(0,0,0,1));

    // disabling buildinfo for now
    this->SetVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBuildInfo::Paint( void )
{
    // shit won't disable
	/*con_nprint_s nxPrn;
    nxPrn.time_to_live = gpGlobals->curtime;
    nxPrn.index = 0;
    nxPrn.fixed_width_font = true;
    nxPrn.color[0] = 1;
    nxPrn.color[1] = 1;
    nxPrn.color[2] = 0.5;
    engine->Con_NXPrintf( &nxPrn, "Portal: Project 2009" );
    nxPrn.index = 1;
    engine->Con_NXPrintf( &nxPrn, "DEVELOPMENT BUILD" );
    nxPrn.index = 2;
    nxPrn.color[0] = 0.5;
    nxPrn.color[1] = 1;
    nxPrn.color[2] = 0.5;
    engine->Con_NXPrintf( &nxPrn, "Not real beta content" );
    nxPrn.index = 3;
    engine->Con_NXPrintf( &nxPrn, "Code Build: %s", __DATE__);
    nxPrn.index = 4;
    engine->Con_NXPrintf( &nxPrn, "Codename: %s", "Quad" );
    nxPrn.index = 6;
	engine->Con_NXPrintf( &nxPrn, "Permissions: Developers Only" );
	nxPrn.index = 7;
	engine->Con_NXPrintf( &nxPrn, "Copyright Incruncensator Studios" );
    */
}