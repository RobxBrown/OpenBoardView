#pragma once
#include "platform.h" // Should be kept first
#include <fstream>
#include "Board.h"

using namespace std;

struct OBDataComponentDatum {
	string component_name;
	string valuetype;
	string value;
	string comment;
};

struct OBDataNetworkDatum {
	string network;
	string condition;
	string valuetype;
	string value;
	string comment;
};

class OBData {
	string diode      = "-";
	string voltage    = "-";
	string resistance = "-";
	string alias      = "";
	string comment    = "";

    string value    = "";
    string package  = "";
    string mfg_code = "";
    string rating   = "";
    string misc     = "";
    string status   = "";

    bool fileopenerror = false;
    bool fileloaded = false;
    filesystem::path current_filename;
    string last_network;
	std::vector<string> conditions;
	string m_currentcondition;
    bool m_headersDrawn = false;

  public:
    virtual ~OBData()   {}
    int  Load(const filesystem::path &filename);
	//void BeginDataTabel() ;
	void DrawDataTable(std::vector<OBDataNetworkDatum> pins);
	void AppendPin(OBDataNetworkDatum pin);
    void AppendPart(OBDataComponentDatum part);
	void ResetPinValues();
    void ResetPartValues();
	void ShowTPTooltip(shared_ptr<Pin>pin);
	void ShowPinTooltip(shared_ptr<Pin> currentlyHoveredPin, shared_ptr<Component> currentlyHoveredPart);
	void ShowPartTooltip(shared_ptr<Component> currentlyHoveredPart);
	void DrawModalDialog();
	string GetPartValue(string partname);
    void DrawInfoPane();
    void AppendDataTable(OBDataNetworkDatum pin);
	void render(bool shown);
	//void DrawPartValue(ImDrawList *draw, shared_ptr<Component> part);

    uint datapoints = 0;
	std::vector<OBDataComponentDatum> obdatacomponents;
	std::vector<OBDataNetworkDatum> obdatanetworks;
};