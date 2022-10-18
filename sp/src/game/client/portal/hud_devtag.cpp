#include "cbase.h"
#include "hud_devtag.h"
#include "iclientmode.h"
#include "hud_macros.h"
#include "vgui_controls/controls.h"
#include "vgui/ISurface.h"
#include "hud.h"
#include "tier0/memdbgon.h"

// Project 2009: Developer Watermark
// Indicates that this is not real beta

using namespace vgui;

DECLARE_HUDELEMENT( CHudDevtag );

static ConVar show_beta("p29_show_devtag", "1", 0, "Toggle Project 2009 Watermark");

CHudDevtag::CHudDevtag( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudDevtag" )
{
   Panel *pParent = g_pClientMode->GetViewport();
   SetParent( pParent );   
   
   SetVisible( false );
   SetAlpha( 255 );

   //AW Create Texture for Looking around
   m_nImport = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImport, "vgui/devtag" , true, true);

   // fuck you whoever put this in vdc and wasted like 20 minutes of my life
   // there IS NO HEV SUIT IN PORTAL
   // ITS LITERALLY CODE FOR A BETA WATERMARK
   // WHY WOULD IT WANT THE HEV SUIT
   // SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

void CHudDevtag::Paint()
{
    SetPaintBorderEnabled(false);
    surface()->DrawSetTexture( m_nImport );
    surface()->DrawTexturedRect( 0, 0, 139, 34 );
}

void CHudDevtag::togglePrint()
{
   if (!show_beta.GetBool())
      this->SetVisible(false);
   else
      this->SetVisible(true);
}

void CHudDevtag::OnThink()
{
   togglePrint();

   BaseClass::OnThink();
}   