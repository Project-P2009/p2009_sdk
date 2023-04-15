#ifndef OPTIONS_SUB_INCRUE_H
#define OPTIONS_SUB_INCRUE_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>

class CLabeledCommandComboBox;
class CCvarToggleCheckButton;

namespace vgui
{
	class Label;
	class Panel;
}

class COptionsSubIncrue : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE(COptionsSubIncrue, vgui::PropertyPage);
public:
	COptionsSubIncrue(vgui::Panel* pParent);
	~COptionsSubIncrue();

	virtual void OnResetData();
	virtual void OnApplyChanges();

protected:
	virtual void ApplySchemeSettings(vgui::IScheme* pScheme);

private:
	MESSAGE_FUNC(OnCheckButtonChecked, "CheckButtonChecked")
	{
		OnControlModified();
	}
	MESSAGE_FUNC(OnControlModified, "ControlModified");
	MESSAGE_FUNC(OnTextChanged, "TextChanged")
	{
		OnControlModified();
	}

	CCvarToggleCheckButton* m_pMouseFilteringCheckBox;
	vgui::ComboBox* m_pMouseFilteringTimeCombo;

	int m_nSelectedPreset = 0;
};

#endif