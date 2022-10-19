#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <string>

using namespace vgui;

class CHudDevtag : public CHudElement, public Panel
{
   DECLARE_CLASS_SIMPLE( CHudDevtag, Panel );

   public:
   CHudDevtag( const char *pElementName );
   void printText(const wchar_t *text, const int lineNum);
   const wchar_t* WStr(const std::string str);
   void togglePrint();
   virtual void OnThink();

   protected:
   virtual void Paint();
   int m_nImport;
};