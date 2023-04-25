#include "VDBugReporterBugReportDialog.h"
#include "vdbugreporter/ivdbugreporter.h"
#include "IGameUI.h"
#include "vgui/IInput.h"
#include "vgui/IPanel.h"
#include "vgui/ISystem.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/MessageBox.h"
#include "vgui_controls/TextEntry.h"

#include "GameUI_Interface.h"

#include "tier0/memdbgon.h"

using namespace vgui;

CVDBugRepBugReportDialog::CVDBugRepBugReportDialog(vgui::Panel* pParent) : BaseClass(pParent, "VDBugRepBugReportDialog")
{
	SetDeleteSelfOnClose(true);

	m_pOk = new Button(this, "OKButton", "#GameUI_OK");
	m_pCancel = new Button(this, "CancelButton", "#GameUI_Cancel");
	m_pBugTitle = new TextEntry(this, "BugTitle");
	m_pBugDescription = new TextEntry(this, "BugDesc");

	m_pBugTitle->SetMaximumCharCount(256);
	m_pBugDescription->SetMaximumCharCount(8191);

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
	CGameUI gameui = GameUI();
	if (!stricmp(command, "SendBugRep"))
	{
		gameui.GetBugReporter()->StartBugReport();
		char title[256];
		char desc[8192];
		m_pBugTitle->GetText(title, 255);
		m_pBugDescription->GetText(desc, 8191);
		gameui.GetBugReporter()->SetTitle(title);
		gameui.GetBugReporter()->SetDescription(desc);
		gameui.GetBugReporter()->SetSeverity(1);
		gameui.GetBugReporter()->SubmitBugReport();
	}
	else if (!stricmp(command, "Cancel") || !stricmp(command, "Close"))
	{
		gameui.GetBugReporter()->CancelBugReport();
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
	m_pBugTitle->GetText(title, 2);
	m_pBugDescription->GetText(desc, 2);

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
	m_pBugTitle->RequestFocus();
}