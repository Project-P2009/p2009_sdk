#include "cbase.h"
#include "hud_maptag.h"
#include "iclientmode.h"
#include "hud_macros.h"
#include "vgui_controls/controls.h"
#include "vgui/ISurface.h"
#include "hud.h"
#include "tier0/memdbgon.h"
#include <string>
#include <wchar.h>
#include <ctime>
#include <iomanip>
#include <sstream>

// Project 2009: Maptag
// Mapname and time overlay

using namespace vgui;

DECLARE_HUDELEMENT( CHudMaptag );

static ConVar show_beta("p29_show_maptag", "1", 0, "Toggle Project 2009 Dev Map Info");

CHudMaptag::CHudMaptag( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudMaptag" )
{
   Panel *pParent = g_pClientMode->GetViewport();
   SetParent( pParent );   
   
   SetVisible( false );
   SetAlpha( 255 );

   //AW Create Texture for Looking around
   //m_nImport = surface()->CreateNewTextureID();
   //surface()->DrawSetTextureFile( m_nImport, "vgui/devtag" , true, true);

   // SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

void CHudMaptag::printText(const wchar_t* text, const int lineNum)
{
    surface()->SetFontGlyphSet(2, "Tahoma", 16, 400, 0, 0, 0x080); // 0x200 is outline, use 0x080 for drop shadow
    surface()->DrawSetTextColor(255, 255, 255, 255);
    surface()->DrawSetTextFont(2);
    surface()->DrawSetTextPos(0, (18 * lineNum) + 4);
    surface()->DrawPrintText(text, std::char_traits<wchar_t>::length(text)); // temp hardcode length
}

const wchar_t* CHudMaptag::WStr(const std::string str) {
    static std::wstring wstr;
    wstr = std::wstring(str.begin(), str.end());
    return wstr.c_str();
}
void CHudMaptag::Paint()
{
    SetPaintBorderEnabled(false);
    //surface()->DrawSetTexture( m_nImport );
    //surface()->DrawTexturedRect( 0, 0, 139, 34 );
	std::string dateTime = "";
	std::time_t t = std::time(nullptr);
	std::time(&t);
	std::time_t dateT = std::time(0);
	std::tm* now = std::localtime(&dateT);
	std::ostringstream ossDate;
	ossDate << (now->tm_year + 1900) << '/'
		<< (now->tm_mon + 1) << '/'
		<< now->tm_mday;
	dateTime = ossDate.str();
	std::ostringstream ossTime;
	ossTime << std::put_time(std::localtime(&t), "%X");
	dateTime = dateTime + " " + ossTime.str();
    printText(WStr(dateTime), 0);
	char const* mapName = IGameSystem::MapName();
	printText(WStr(mapName + std::string(".bsp")), 1);
}
void CHudMaptag::togglePrint()
{
   if (!show_beta.GetBool())
      this->SetVisible(false);
   else
      this->SetVisible(true);
}

void CHudMaptag::OnThink()
{
   togglePrint();

   BaseClass::OnThink();
}   