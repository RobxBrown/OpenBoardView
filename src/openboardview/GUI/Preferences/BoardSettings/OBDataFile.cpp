#include "OBDataFile.h"

#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

#include "platform.h"

namespace Preferences {

OBDataFile::OBDataFile(::OBDataFile &obdFile) : obdFile(obdFile), obdFileCopy(obdFile) {
}

void OBDataFile::save() {
	obdFile.writeToConfig(obdFile.configFilepath);
	obdFile.reload();
}

void OBDataFile::cancel() {
	obdFile = obdFileCopy;
}

void OBDataFile::clear() {
	obdFile.path = filesystem::path{};
	obdFile.close();
}

void OBDataFile::render(bool shown) {
	static bool wasShown = false;

	if (shown) {
		if (!wasShown) {           // Panel just got opened
			obdFileCopy = obdFile; // Make a copy to be able to restore if cancelled
		}

		ImGui::Text("OBData local file");

		std::string obdFilePath = obdFile.path.string();
		if (ImGui::InputText("OBD File", &obdFilePath)) {
			obdFile.path = obdFilePath;
		}
		ImGui::SameLine();
		if (ImGui::Button("Browse##obdFilePath")) {
			auto path = show_file_picker();
			if (!path.empty()) {
				obdFilePath  = path.string();
				obdFile.path = path;
			}
		}

		ImGui::Separator();
		std::string text = "When searching, use the minimum unique ID possible for the board.\nie, instead of 'J113 820-00165.brd', use '820-00165' only.\nAlso check https::\\\\openboarddata.org for a list of boards.";
		float font_size = ImGui::GetFontSize() * text.size() / 2;
		//ImGui::SameLine(ImGui::GetWindowSize().x / 2 - font_size + (font_size / 2));
		ImGui::SameLine(ImGui::GetWindowSize().x / 5.75f);
		ImGui::Text("%s", text.c_str());
		std::string searchString;
		std::string searchResult;
		ImGui::Text("Search string: ");
		ImGui::SameLine();
		ImGui::InputText("", &searchString);
		ImGui::SameLine();
		if(ImGui::Button("Search##searchString")) {
			
			//searchResult = something;
		}
		ImGui::Text("Search result: ");
		ImGui::SameLine();
		ImGui::InputText("", &searchResult);
		ImGui::SameLine();
		if(ImGui::Button("Select##searchResult")) {
			//do something with search result
		}
	}

	wasShown = shown;
}

} // namespace Preferences