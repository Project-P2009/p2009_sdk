#ifndef VDBUGREPBUGREPORTDIALOG_H
#define VDBUGREPBUGREPORTDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/Frame.h"

class CVDBugRepBugReportDialog : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CVDBugRepBugReportDialog, vgui::Frame);

public:
	CVDBugRepBugReportDialog(vgui::Panel* pParent);
	~CVDBugRepBugReportDialog();

	virtual void Activate();

private:
	virtual void OnCommand(const char* command);

	MESSAGE_FUNC_PTR(OnTextChanged, "TextChanged", panel);

	bool m_bSnapshotAttached = false;

	vgui::Button* m_pOk;
	vgui::Button* m_pCancel;
	vgui::Button* m_pAttachScreeshot;
	vgui::TextEntry* m_pTitle;
	vgui::TextEntry* m_pDescription;
	vgui::ComboBox* m_pCategory;
	vgui::ComboBox* m_pSeverity;
	vgui::Label* m_pSnapshotLabel;

	vgui::DHANDLE<vgui::MessageBox> m_hErrorBox;
};

#endif