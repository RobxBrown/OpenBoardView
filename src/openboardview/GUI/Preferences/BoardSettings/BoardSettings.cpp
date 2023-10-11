#include "BoardSettings.h"

#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

#include "platform.h"
// #include "PDFFile.h"
// #include "OBDataFile.h"

namespace Preferences
{

	BoardSettings::BoardSettings(const KeyBindings &keybindings, ::BackgroundImage &backgroundImage, ::PDFFile &pdfFile, ::OBDataFile &obdFile) : keybindings(keybindings), backgroundImagePreferences(backgroundImage), pdfFilePreferences(pdfFile), obdFilePreferences(obdFile)
	{
	}

	void BoardSettings::menuItem()
	{
		if (ImGui::MenuItem("Board Settings"))
		{
			shown = true;
		}
	}

	void BoardSettings::render()
	{
		if (shown)
		{
			ImGui::Begin("Board Settings", &shown, ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::Separator();
			pdfFilePreferences.render(true);

			ImGui::Separator();
			obdFilePreferences.render(true);

			ImGui::Separator();
			backgroundImagePreferences.render(true);

			ImGui::Separator();
			ImGui::Text("%s", "Note: board settings are stored in the .conf file associated with the boardview file.");

			if (!shown)
			{ // modal just closed after title bar close button clicked, Save/Cancel modify shown so this must stay above
				pdfFilePreferences.cancel();
				backgroundImagePreferences.cancel();
				obdFilePreferences.cancel();
			}

			if (ImGui::Button("Save"))
			{
				shown = false;
				pdfFilePreferences.save();
				backgroundImagePreferences.save();
				obdFilePreferences.save();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel") || this->keybindings.isPressed("CloseDialog"))
			{
				shown = false;
				pdfFilePreferences.cancel();
				backgroundImagePreferences.cancel();
				obdFilePreferences.cancel();
			}
			ImGui::SameLine();
			if (ImGui::Button("Clear"))
			{
				pdfFilePreferences.clear();
				backgroundImagePreferences.clear();
				obdFilePreferences.clear();
			}

			ImGui::End();
		}
		else
		{
			pdfFilePreferences.render(false);
			backgroundImagePreferences.render(false);
			obdFilePreferences.render(false);
		}
	}

} // namespace Preferences
