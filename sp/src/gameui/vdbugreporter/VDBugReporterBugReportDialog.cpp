#include "VDBugReporterBugReportDialog.h"
#include "vdbugreporter/ivdbugreporter.h"
#include "IGameUI.h"
#include "vgui/IInput.h"
#include "vgui/IPanel.h"
#include "vgui/ISystem.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/MessageBox.h"
#include "vgui_controls/TextEntry.h"
#include "cdll_int.h"

#include "tier1/KeyValues.h"
#include "GameUI_Interface.h"

#include "EngineInterface.h"

#include "tier0/memdbgon.h"

using namespace vgui;

CVDBugRepBugReportDialog::CVDBugRepBugReportDialog(vgui::Panel* pParent) : BaseClass(pParent, "VDBugRepBugReportDialog")
{
	SetDeleteSelfOnClose(true);

	SetRoundedCorners(PANEL_ROUND_CORNER_NONE);

	m_pOk = new Button(this, "OKButton", "#GameUI_OK");
	m_pCancel = new Button(this, "CancelButton", "#GameUI_Cancel");

	m_pAttachScreeshot = new Button(this, "AttachSnapshot", "#GameUI_BugRepMenuAttachSnapshot");
	m_pSnapshotLabel = new Label(this, "BugSnapshotLabel", "#GameUI_BugRepMenuAttachSnapshotLabel");

	m_pTitle = new TextEntry(this, "BugTitle");
	m_pTitle->SetMaximumCharCount(256);
	
	m_pDescription = new TextEntry(this, "BugDesc");
	m_pDescription->SetMultiline(true);
	m_pDescription->SetVerticalScrollbar(true);
	m_pDescription->SetCatchEnterKey(true);
	m_pDescription->SetMaximumCharCount(8192);

	m_pSeverity = new vgui::ComboBox(this, "BugSeverity", 3, false);
	m_pSeverity->AddItem("#GameUI_BugRepMenuSeverity0", new KeyValues("BugSeverity", "severity", 0));
	m_pSeverity->AddItem("#GameUI_BugRepMenuSeverity1", new KeyValues("BugSeverity", "severity", 1));
	m_pSeverity->AddItem("#GameUI_BugRepMenuSeverity2", new KeyValues("BugSeverity", "severity", 2));
	m_pSeverity->AddItem("#GameUI_BugRepMenuSeverity3", new KeyValues("BugSeverity", "severity", 3));

	m_pCategory = new vgui::ComboBox(this, "BugCategory", 5, false);
	m_pCategory->AddItem("#GameUI_BugRepMenuBugCategory0", new KeyValues("BugCategory", "category", 0));
	m_pCategory->AddItem("#GameUI_BugRepMenuBugCategory1", new KeyValues("BugCategory", "category", 1));
	m_pCategory->AddItem("#GameUI_BugRepMenuBugCategory2", new KeyValues("BugCategory", "category", 2));
	m_pCategory->AddItem("#GameUI_BugRepMenuBugCategory3", new KeyValues("BugCategory", "category", 3));
	m_pCategory->AddItem("#GameUI_BugRepMenuBugCategory4", new KeyValues("BugCategory", "category", 4));

	SetSizeable(false);
	SetSize(680, 1000);
	SetTitle("#GameUI_BugReport", true);

	LoadControlSettings("Resource/VDBugRepBugReportDialog.res");

	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	m_pOk->SetEnabled(false);
}

CVDBugRepBugReportDialog::~CVDBugRepBugReportDialog()
{
	vgui::surface()->RestrictPaintToSinglePanel(NULL);
}

void CVDBugRepBugReportDialog::OnCommand(const char* command)
{
	if (!stricmp(command, "SendBugRep"))
	{
		IVDBugReporter* pBugRep = GameUI().GetBugReporter();
		pBugRep->StartBugReport();
		char title[256];
		char desc[8192];
		m_pTitle->GetText(title, 255);
		m_pDescription->GetText(desc, 8191);
		pBugRep->SetTitle(title);
		pBugRep->SetDescription(desc);
		pBugRep->SetSeverity(m_pSeverity->GetActiveItem());
		pBugRep->SetCategory(m_pCategory->GetActiveItem());
		pBugRep->SetMapname(engine->GetLevelName());
		BugLocalization_t* pLoc = new BugLocalization_t;
		pLoc->m_vecPosition = Vector(1, 6, 5);
		pLoc->m_angOrientation = QAngle(7, 15, 73);
		pBugRep->SetLocalization(pLoc);
		pBugRep->SubmitBugReport();
	}
	else if (!strcmp(command, "takesnapshot"))
	{
		engine->ClientCmd_Unrestricted("takesnapshot");
	}
	else if (!stricmp(command, "Cancel") || !stricmp(command, "Close"))
	{
		GameUI().GetBugReporter()->CancelBugReport();
		Close();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

void CVDBugRepBugReportDialog::OnTextChanged(Panel* entry)
{
	char title[2];
	char desc[2];
	m_pTitle->GetText(title, 2);
	m_pDescription->GetText(desc, 2);

	if (strlen(title) >= 1 && strlen(desc) >= 1)
	{
		m_pOk->SetEnabled(true);
	}
	else
	{
		m_pOk->SetEnabled(false);
	}
}

void CVDBugRepBugReportDialog::Activate()
{
	BaseClass::Activate();
	m_pTitle->RequestFocus();
}