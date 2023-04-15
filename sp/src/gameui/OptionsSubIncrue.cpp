#include "OptionsSubIncrue.h"
#include "CvarToggleCheckButton.h"
#include "vgui_controls/ComboBox.h"

#include <tier0/memdbgon.h>

using namespace vgui;

typedef struct {
	int id;
	float val;
} mousefiltertimepreset_t;

mousefiltertimepreset_t m_filterTimePresets[] = 
{
	{0, 0.006f},
	{1, 0.010f},
	{2, 0.015f},
	{3, 0.020f},
	{4, 0.030f},
};

COptionsSubIncrue::COptionsSubIncrue(vgui::Panel* pParent) : PropertyPage(pParent, NULL)
{
	m_pMouseFilteringCheckBox = new CCvarToggleCheckButton(this, "MouseFiltering", "#GameUI_MouseFiltering", "m_filter");
	m_pMouseFilteringTimeCombo = new ComboBox(this, "MouseFilterTime", 5, false);
	m_pMouseFilteringTimeCombo->AddItem("#GameUI_MouseFilterTime0", new KeyValues("MouseFilterTime", "time", "0.006"));
	m_pMouseFilteringTimeCombo->AddItem("#GameUI_MouseFilterTime1", new KeyValues("MouseFilterTime", "time", "0.010"));
	m_pMouseFilteringTimeCombo->AddItem("#GameUI_MouseFilterTime2", new KeyValues("MouseFilterTime", "time", "0.015"));
	m_pMouseFilteringTimeCombo->AddItem("#GameUI_MouseFilterTime3", new KeyValues("MouseFilterTime", "time", "0.020"));
	m_pMouseFilteringTimeCombo->AddItem("#GameUI_MouseFilterTime4", new KeyValues("MouseFilterTime", "time", "0.030"));

	LoadControlSettings("Resource\\optionssubincrue.res");
}

COptionsSubIncrue::~COptionsSubIncrue()
{
}

void COptionsSubIncrue::OnResetData()
{
	m_pMouseFilteringCheckBox->Reset();

	ConVarRef m_filter_time("m_filter_time");
	if (m_filter_time.IsValid()) {
		for (int i = 0; i < 5; i++) {
			mousefiltertimepreset_t pPreset = m_filterTimePresets[i];
			if (pPreset.val == m_filter_time.GetFloat()) {
				m_pMouseFilteringTimeCombo->ActivateItem(pPreset.id);
			}
			else {
				m_pMouseFilteringTimeCombo->ActivateItem(0);
			}
		}
	}
}

void COptionsSubIncrue::OnApplyChanges()
{
	m_pMouseFilteringCheckBox->ApplyChanges();

	if (m_pMouseFilteringTimeCombo->IsEnabled())
	{
		ConVarRef m_filter_time("m_filter_time");
		for (int i = 0; i < 5; i++) {
			mousefiltertimepreset_t pPreset = m_filterTimePresets[i];
			if (pPreset.id == m_pMouseFilteringTimeCombo->GetActiveItem()) {
				Msg("Active item: %d Value: %.3f\n", m_pMouseFilteringTimeCombo->GetActiveItem(), pPreset.val);
				m_filter_time.SetValue(pPreset.val);
			}
		}
	}
}

void COptionsSubIncrue::ApplySchemeSettings(IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}

void COptionsSubIncrue::OnControlModified()
{
	PostActionSignal(new KeyValues("ApplyButtonEnable"));
}