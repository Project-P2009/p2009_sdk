#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <string>

using namespace vgui;

class CHudMaptag : public CHudElement, public Panel
{
   DECLARE_CLASS_SIMPLE( CHudMaptag, Panel );

   public:
   CHudMaptag( const char *pElementName );
   void printText(const wchar_t *text, const int lineNum);
   const wchar_t* WStr(const std::string str);
   void togglePrint();
   virtual void OnThink();

   protected:
   virtual void Paint();
   int m_nImport;
};