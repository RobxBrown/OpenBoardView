#ifndef _OBDFILE_H_
#define _OBDFILE_H_

#include <array>
#include <string>
#include <glad/glad.h>

#include "imgui/imgui.h"

#include "Renderers/ImGuiRendererSDL.h"
#include "GUI/Image.h"
#include "filesystem_impl.h"

#include "OBDataBridge.h"

// Requires forward declaration in order to friend since it's from another namespace
namespace Preferences {
class OBDataFile;
}

class OBDataBridge; // Forward declaration to solve circular includes

// Manages background image configuration attached to boardview file and currently shown image depending on current board side
class OBDataFile {
  private:
	filesystem::path path;
	filesystem::path configFilepath{};

	OBDataBridge *obdBridge = nullptr;

  public:
	OBDataFile(OBDataBridge &obdBridge);

	void reload();
	void close();

	void loadFromConfig(const filesystem::path &filepath);
	void writeToConfig(const filesystem::path &filepath);
	filesystem::path getPath() const;

	friend Preferences::OBDataFile;
};

#endif