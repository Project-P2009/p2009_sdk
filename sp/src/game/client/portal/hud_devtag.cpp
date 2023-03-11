#include "cbase.h"
#include "hud_devtag.h"
#include "iclientmode.h"
#include "hud_macros.h"
#include "vgui_controls/controls.h"
#include "vgui/ISurface.h"
#include "hud.h"
#include "tier0/memdbgon.h"
#include <string>
#include <wchar.h>

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
   // ITS LITERALLY SUPPOSED TO BE EXAMPLE CODE FOR A BETA WATERMARK
   // WHY WOULD IT WANT THE HEV SUIT
   // SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

void CHudDevtag::printText(const wchar_t* text, const int lineNum)
{
    surface()->SetFontGlyphSet(2, "Tahoma", 13, 400, 0, 0, 0x080); // 0x200 is outline, use 0x080 for drop shadow
    surface()->DrawSetTextColor(255, 255, 255, 255);
    surface()->DrawSetTextFont(2);
    surface()->DrawSetTextPos(0, (13 * lineNum) + 37);
    surface()->DrawPrintText(text, std::char_traits<wchar_t>::length(text)); // temp hardcode length
}

const wchar_t* CHudDevtag::WStr(const std::string str) {
    static std::wstring wstr;
    wstr = std::wstring(str.begin(), str.end());
    return wstr.c_str();
}
void CHudDevtag::Paint()
{
    SetPaintBorderEnabled(false);
    surface()->DrawSetTexture( m_nImport );
    surface()->DrawTexturedRect( 0, 0, 139, 34 );
    //printText(L"WIP - Development Build", 0);
    //printText(WStr(std::string("DEV BUILD: ") + __DATE__), 0);
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