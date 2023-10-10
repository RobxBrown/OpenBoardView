
#include "OBData.h"
#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include <SDL.h>
#include <curl/curl.h>
#include "OBDataBridge/OBDataFile.h"

using namespace std;

string url_decode(std::string &str) {
	int length;
	const auto value = curl_easy_unescape(nullptr, str.c_str(), static_cast<int>(str.length()), &length);
	string result(value, length);
	curl_free(value);
	std::replace(result.begin(), result.end(), '+', ' ');
	return result;
}

string url_encode(std::string &str) {
	//int length;
	const auto value = curl_easy_escape(nullptr, str.c_str(), static_cast<int>(str.length()));
	string result(value);
	curl_free(value);
	return result;
}

int OBData::Load(const filesystem::path &filepath) {
	datapoints = 0;
	if (!exists(filepath)) {
		SDL_Log("Attempting to download file from openboarddata.org: %s", filepath.c_str());
		CURL *curl;
		CURLcode result;
		FILE *pfBuffer = NULL;
		const string url = "https://openboarddata.org/";
		const string path = "laptops/apple/820-00165";
		const string search = "?a=findboard&find=820-0016"; //J113
		const string postData = "a=generate&bpath=" + path;

		curl_global_init(CURL_GLOBAL_ALL); // In windows, this will init the winsock stuff
		curl = curl_easy_init();
		if (curl) {
			pfBuffer = fopen(filepath.c_str(), "wb");
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, pfBuffer);
			result = curl_easy_perform(curl);
			if (result != CURLE_OK) {
				fprintf(stderr, "Download from openboarddata.org failed with error: %s\n", curl_easy_strerror(result));
				curl_easy_cleanup(curl); // clean up windows winsock stuff
				curl_global_cleanup();
				fclose(pfBuffer);
				return -1;
			} else {
				curl_off_t numbytes;
				result = curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD_T, &numbytes);
				if (!result) {
					printf("Successfully downloaded %" CURL_FORMAT_CURL_OFF_T " bytes\n", numbytes);
				}
			}
		}
		curl_easy_cleanup(curl); // clean up windows winsock stuff
		curl_global_cleanup();
		fclose(pfBuffer);
	}

	if (exists(filepath)) {
		SDL_Log("OBData file is present on the local disk.");
		std::string line;
		std::ifstream obdatafile(filepath.c_str());
		current_filename = filepath;
		fileloaded = false;
		bool component_databegin = false;
		int  component_datacount = 0;
		bool network_databegin = false;
		int network_datacount = 0;

		if(!obdatafile.is_open()) {
			printf("Error! Could not open OBData file: %s\n", filepath.c_str());
			fileopenerror = true;
			fileloaded = false;
			return -1;
		} else {
			SDL_Log("Attempting to load OBData file from local disk...");
			while (getline(obdatafile, line) ) {

				if (line.length() == 1) continue; // found blank line, skip it.
				if (line.substr(0, 3) == "###") continue; // we've found a commented line, skip it.

				if (line.find("COMPONENTS_DATA_START") != std::string::npos) {
					component_databegin = true;
					printf("found beggining of the component data...\n");
					continue;
				}

				if (line.find("NETS_DATA_START") != std::string::npos) {
					network_databegin = true;
					printf("found beggining of the network data...\n");
					continue;
				}

				if (component_databegin) { // we found the component data
					if (line.find("COMPONENTS_DATA_END") != std::string::npos) {
						component_databegin = false;
						printf("found end of the component data...\n");
						continue;
					}
					OBDataComponentDatum obd_componentdatum;
					std::istringstream line_stream(line);
					getline(line_stream, obd_componentdatum.component_name, ' ');
					getline(line_stream, obd_componentdatum.valuetype, ' ');
					getline(line_stream, obd_componentdatum.value, ' ');
					obdatacomponents.push_back(obd_componentdatum);
					//printf("total = %lu\n", obdatacomponents.size());

					// printf("componentname = %s\nvaluetype = %s\nvalue = %s\n",
					//        obd_componentdatum.component_name.c_str(),
					//        obd_componentdatum.valuetype.c_str(),
					//        obd_componentdatum.value.c_str());

					component_datacount++;
				}

				if(network_databegin) { // we found the network data
					if (line.find("NETS_DATA_END") != std::string::npos) {
						network_databegin = false;
						printf("found end of the network data...\n");
						continue;
					}
					OBDataNetworkDatum obd_networkdatum;
					std::string temp;
					std::istringstream line_stream(line);
					getline(line_stream, temp, ' ');
					getline(line_stream, obd_networkdatum.valuetype, ' ');
					getline(line_stream, obd_networkdatum.value, ' ');
					getline(line_stream, obd_networkdatum.comment, ' ');

					std::istringstream temp_stream(temp);
					getline(temp_stream, obd_networkdatum.network, '/');
					getline(temp_stream, obd_networkdatum.condition, '/');
					obdatanetworks.push_back(obd_networkdatum);
					//printf("total = %lu\n", obdatanetworks.size());

					// printf("temp = %s\nnetwork = %s\ncondition = %s\ntype = %s\nvalue = %s\ncomment = %s\ncontent = %lu\n",
					//        temp.c_str(),
					//        obd_networkdatum.network.c_str(),
					//        obd_networkdatum.condition.c_str(),
					//        obd_networkdatum.valuetype.c_str(),
					//        obd_networkdatum.value.c_str(),
					//        obd_networkdatum.comment.c_str(),
					//        line.length());
					if(obd_networkdatum.network != last_network) {
						string str = obd_networkdatum.condition;
						std::transform(str.begin(), str.end(), str.begin(), ::toupper);
						if(str == "DEFAULT") network_datacount++;
						last_network = obd_networkdatum.network;
					}
					
				}
				line.clear();
			}
			fileloaded = true;
			datapoints = network_datacount;

			string currentcondition;
			for (auto network : obdatanetworks) {
				if (currentcondition != network.condition) {
					currentcondition = network.condition;
					conditions.push_back(currentcondition);
				}
			}
			//int countconditions = conditions.size();
			m_currentcondition = "Default";
			SDL_Log("Successfully loaded OBData file.");
		}
	}
	return 0;
}

string OBData::GetPartValue(string partname) {
	if(!fileloaded) return "";
	if(obdatacomponents.empty()) return "";
	bool obdatafound = false;
	OBDataComponentDatum pt;
	ResetPartValues();
	for (auto &part : obdatacomponents) {
		if (part.component_name == partname) {
			AppendPart(part);
			pt = part;
			obdatafound = true;
		}
	}
	if (obdatafound) {
		return "\n" + pt.value;
	}
	return "";
}

// void OBData::DrawPartValue(ImDrawList *draw, shared_ptr<Component> part) {
// 	if(!fileloaded) return;
// 	if(obdatacomponents.empty()) return;
// 	bool obdatafound = false;
// 	OBDataComponentDatum pt;
// 	ResetPartValues();
// 	for (auto &p : obdatacomponents) {
// 		if (p.component_name == part->name) {
// 			AppendPart(p);
// 			pt = p;
// 			obdatafound = true;
// 		}
// 	}
// 	if (!obdatafound) {
// 		return;
// 	}

// 	ImVec2 a, b, c, d;

// 	a = ImVec2(CoordToScreen(part->outline[0].x, part->outline[0].y));
// 	b = ImVec2(CoordToScreen(part->outline[1].x, part->outline[1].y));
// 	c = ImVec2(CoordToScreen(part->outline[2].x, part->outline[2].y));
// 	d = ImVec2(CoordToScreen(part->outline[3].x, part->outline[3].y));

// 	string text                 = pt.value; //obdata.GetPartValue(part->name);
// 	ImFont *font                = ImGui::GetIO().Fonts->Fonts[0]; // Default font
// 	ImVec2 text_size_normalized = font->CalcTextSizeA(1.0f, FLT_MAX, 0.0f, text.c_str());

// 	// Find max width and height of bounding box, not perfect for non-straight bounding box but good enough
// 	float minx      = std::min({a.x, b.x, c.x, d.x});
// 	float miny      = std::min({a.y, b.y, c.y, d.y});
// 	float maxx      = std::max({a.x, b.x, c.x, d.x});
// 	float maxy      = std::max({a.y, b.y, c.y, d.y});
// 	float maxwidth  = abs(maxx - minx) * 0.7; // Bounding box width with 30% padding
// 	float maxheight = abs(maxy - miny) * 0.7; // Bounding box height with 30% padding

// 	// Find max font size to fit text inside bounding box
// 	float maxfontwidth  = maxwidth / text_size_normalized.x;
// 	float maxfontheight = maxheight / text_size_normalized.y;
// 	float maxfontsize   = min(maxfontwidth, maxfontheight);

// 	ImVec2 text_size{text_size_normalized.x * maxfontsize, text_size_normalized.y * maxfontsize};

// 	// Center text
// 	ImVec2 pos = CoordToScreen(part->centerpoint.x,
// 	                           part->centerpoint.y); // Computed previously during bounding box generation
// 	pos.x -= text_size.x * 0.5f;
// 	pos.y -= text_size.y * 0.20f;

// 	if (maxfontsize < font->FontSize * 0.75) {
// 		font = ImGui::GetIO().Fonts->Fonts[2]; // Use smaller font for part name
// 	} else if (maxfontsize > font->FontSize * 1.5 && ImGui::GetIO().Fonts->Fonts[1]->FontSize > font->FontSize) {
// 		font = ImGui::GetIO().Fonts->Fonts[1]; // Use larger font for part name
// 	}

// 	draw->ChannelsSetCurrent(kChannelText);
// 	draw->AddText(font, maxfontsize, pos, m_colors.partTextColor, text.c_str());
// 	draw->ChannelsSetCurrent(kChannelPolylines);
// }

void OBData::AppendDataTable(OBDataNetworkDatum pin) {
	ImGui::TableNextColumn();
	ImVec4 tableBGColor = {0.1f, 0.1f, 0.1f, 1.0f};
	ImGui::PushStyleColor(ImGuiCol_Text, 0xffeeeeee);
	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(tableBGColor));
	ImGui::Text("%s", url_decode(pin.condition).c_str());
	ImGui::TableNextColumn();
	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(tableBGColor));
	if (isdigit(diode.c_str()[0])) {
		ImGui::Text("%.3f", stof(diode));
	} else {
		ImGui::Text("%s", diode.c_str());
	}
	ImGui::TableNextColumn();
	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(tableBGColor));
	if (isdigit(voltage.c_str()[0])) {
		ImGui::Text("%.3f", stof(voltage));
	} else {
		ImGui::Text("%s", voltage.c_str());
	}
	ImGui::TableNextColumn();
	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(tableBGColor));
	std::transform(resistance.begin(), resistance.end(), resistance.begin(), ::toupper);
	ImGui::Text("%s", resistance.c_str());
	ImGui::TableNextColumn();
	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(tableBGColor));
	ImGui::Text("%s", url_decode(comment).c_str());
	ImGui::PopStyleColor(1);
}

void OBData::DrawDataTable(std::vector<OBDataNetworkDatum> pins) {
	if (ImGui::BeginTable("OBData", 5,
	                      	ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders |
	                        ImGuiTableFlags_NoPadOuterX)) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{.9f, 0, 0, 1});
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 1));
		ImGui::TableSetBgColor(ImGuiTableRowFlags_Headers, ImGui::GetColorU32(ImVec4{0.5f, 0.5f, 0.5f, 1}));
		int columnCount                 = 5;
		string headers[columnCount + 1] = {"Condition", "D", "V", "R", "Note"};
		for (int column = 0; column < columnCount; column++) {
			ImGui::TableSetupColumn(
				headers[column].c_str(), ImGuiTableColumnFlags_WidthStretch, 0.0f, column / static_cast<float>(columnCount));
		}
		ImGui::TableHeadersRow();
		std::vector<string> conditionlist;
		OBDataNetworkDatum activepin = pins[0];
		ResetPinValues();
		for (auto &pin : pins) {
			if (std::find(conditionlist.begin(), conditionlist.end(), pin.condition) == conditionlist.end()) {
				conditionlist.push_back(pin.condition);
			}
		}
		for (auto &cond : conditionlist) {
			for (auto &pin : pins) {
				if(pin.condition == cond) {
					AppendPin(pin);
					activepin = pin;
				} 
			}
			// if (activepin.condition == "Default" && m_currentcondition == "Default") {
			// 	AppendDataTable(activepin);
			// }
			// if (cond == m_currentcondition)
			 AppendDataTable(activepin);
			ResetPinValues();
		}
		ImGui::EndTable();
		ImGui::PopStyleColor(2);
	}
}

void OBData::ResetPinValues() {
	diode      = "-";
	voltage    = "-";
	resistance = "-";
	alias      = "";
	comment    = "";
}

void OBData::AppendPin(OBDataNetworkDatum pin) {
	// ### d = diode, v = voltage, r = resistance, a = alias, t = net comment
	if (pin.valuetype == "d") diode = pin.value;
	if (pin.valuetype == "v") voltage = pin.value;
	if (pin.valuetype == "r") resistance = pin.value;
	if (pin.valuetype == "a") alias = pin.value;
	if (pin.valuetype == "t") comment = pin.value;
}

void OBData::ResetPartValues() {
	value    = "";
	package  = "";
	mfg_code = "";
	rating   = "";
	misc     = "";
	status   = "";
}

void OBData::AppendPart(OBDataComponentDatum part) {
	//## #v = value, p = package, c = manufacturer code, r = rating, m = misc, s = status
	if (part.valuetype == "v") value = part.value;
	if (part.valuetype == "p") package = part.value;
	if (part.valuetype == "c") mfg_code = part.value;
	if (part.valuetype == "r") rating = part.value;
	if (part.valuetype == "m") misc = part.value;
	if (part.valuetype == "s") status = part.value;
}

void OBData::ShowTPTooltip(shared_ptr<Pin> pin) {
	OBDataNetworkDatum activepin;
	bool obdatafound = false;
	std::vector<OBDataNetworkDatum> pins;
	ResetPinValues();
	for (auto &tp : obdatanetworks) {
		if (tp.network == pin->net->name) {
			AppendPin(tp);
			pins.push_back(tp);
			activepin   = tp;
			obdatafound = true;
		}
	}

	if (obdatafound) {
		ImGui::Text("Pin name: TP_%s\nTest pad: %s\nTP_%s / %s %s\n\nTest pad",
		            pin->net->name.c_str(),
		            pin->net->name.c_str(),
		            pin->net->name.c_str(),
		            pin->name.c_str(),
		            pin->net->name.c_str());
		DrawDataTable(pins);
	} else {
		ImGui::Text("No OBData present for %s", pin->net->name.c_str());
	}
}

void OBData::ShowPinTooltip(shared_ptr<Pin> currentlyHoveredPin, shared_ptr<Component> currentlyHoveredPart) {
	bool obdatafound = false;
	//OBDataNetworkDatum activepin;
	std::vector<OBDataNetworkDatum> pins;
	//ResetPinValues();
	for (auto &pin : obdatanetworks) {
		if (pin.network == currentlyHoveredPin->net->name) {
			if(m_currentcondition != "Default") {
				printf("pin.condition = %s\nm_currentcondition = %s\n", pin.condition.c_str(), m_currentcondition.c_str());
				if(url_decode(pin.condition) == m_currentcondition || (currentlyHoveredPin->net->name == pin.network && pin.condition != "Default")) {
					pins.push_back(pin);
					obdatafound = true;
				} else {
				
				}
			} else {
				pins.push_back(pin);
				obdatafound = true;
			}
		}
	}
	if (obdatafound) {
		ImGui::Text("%s:%s %s\n\nNormal pin",
		            currentlyHoveredPart->name.c_str(),
		            (currentlyHoveredPin ? currentlyHoveredPin->name.c_str() : " "),
		            (currentlyHoveredPin ? currentlyHoveredPin->net->name.c_str() : " "));
		DrawDataTable(pins);
				
	} else {
		string strnocon = "Normal pin\n";
		if (currentlyHoveredPin->net->name == "UNCONNECTED") strnocon = "Not Connected\n";
		ImGui::Text("%s:%s %s\n\n%sNo OBData present for '%s'",
		            currentlyHoveredPart->name.c_str(),
		            (currentlyHoveredPin ? currentlyHoveredPin->name.c_str() : " "),
		            (currentlyHoveredPin ? currentlyHoveredPin->net->name.c_str() : " "),
		            strnocon.c_str(),
		            currentlyHoveredPin->net->name.c_str());
	}	
}

void OBData::ShowPartTooltip(shared_ptr<Component> currentlyHoveredPart) {
	bool obdatafound = false;
	ResetPartValues();
	for (auto &part : obdatacomponents) {
		if (part.component_name == currentlyHoveredPart->name) {
			AppendPart(part);
			obdatafound = true;
		}
	}
	//printf("%i\n", (int)currentlyHoveredPart->component_type);
	// if (currentlyHoveredPart->component_type == currentlyHoveredPart->kComponentTypeUnknown) printf("isDummy\n");
	if (obdatafound) { // ## #v = value, p = package, c = manufacturer code, r = rating, m = misc, s = status
	string str_tooltip = "";
	if (value.length() 		> 1) 	str_tooltip += "Value: "	+ value		+ "\n";
	//if (value.empty()		   ) 	str_tooltip += "Value: " + currentlyHoveredPart->name + "\n";
	if (package.length() 	> 1) 	str_tooltip += "Package: " 	+ package 	+ "\n";
	if (mfg_code.length() 	> 1) 	str_tooltip += "Mfg_Code: " + mfg_code 	+ "\n";
	if (rating.length() 	> 1)	str_tooltip += "Rating: " 	+ rating 	+ "\n";
	if (misc.length() 		> 1)	str_tooltip += "Misc: " 	+ misc 		+ "\n";
	if (status.length() 	> 2) 	str_tooltip += "Status: " 	+ status 	+ "\n";	

	ImGui::Text("%s\n%s", currentlyHoveredPart->name.c_str(), str_tooltip.c_str());
	} else {
		ImGui::Text("%s", currentlyHoveredPart->name.c_str());
	}
}

void OBData::DrawModalDialog() {
	if (ImGui::BeginPopupModal("Error opening OBDatafile")) {
		ImGui::Text("There was an error opening the file: %s", current_filename.c_str());
		// if (!m_error_msg.empty()) {
		// 	ImGui::Text("%s", m_error_msg.c_str());
		// }
		// if (obdatafile && !obdatafile->error_msg.empty()) {
		// 	ImGui::Text("%s", obdatafile->error_msg.c_str());
		// }
		if (ImGui::Button("OK")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	if (fileopenerror) {
		ImGui::OpenPopup("Error opening OBDatafile");
		fileopenerror = false;
	}
}
void OBData::DrawInfoPane() {
	ImGui::Separator();
	ImGui::Columns(1);
	string text = "OBData condition [" + m_currentcondition + "]";
	if (ImGui::TreeNode(text.c_str())) {
		if (ImGui::BeginCombo("##combo", m_currentcondition.c_str())) {
			for (auto condition : conditions) {
				bool is_selected = (m_currentcondition == condition);
				if (ImGui::Selectable(url_decode(condition).c_str(), is_selected)) {
					m_currentcondition = url_decode(condition);
				}
				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::Text("Condition: %s", m_currentcondition.c_str());
		ImGui::Text("Data points: %lu", obdatanetworks.size());
		ImGui::TreePop();
	}
}