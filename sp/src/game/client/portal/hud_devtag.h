#include "hudelement.h"
#include <vgui_controls/Panel.h>

using namespace vgui;

class CHudDevtag : public CHudElement, public Panel
{
   DECLARE_CLASS_SIMPLE( CHudDevtag, Panel );

   public:
   CHudDevtag( const char *pElementName );
   void togglePrint();
   virtual void OnThink();

   protected:
   virtual void Paint();
   int m_nImport;
};