#include "platform.h"

#include "OBDataFile.h"

#include "confparse.h"

OBDataFile::OBDataFile(OBDataBridge &obdBridge)
    : obdBridge(&obdBridge) {}

void OBDataFile::reload() {
	if (this->obdBridge == nullptr) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", "OBDataFile Could not reload: obdBridge == nullptr");
		return;
	}
	this->obdBridge->CloseDocument();
	this->obdBridge->OpenDocument(*this);
}

void OBDataFile::close() {
	if (this->obdBridge == nullptr) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", "OBDataFile Could not close: obdBridge == nullptr");
		return;
	}
	this->obdBridge->CloseDocument();
}

void OBDataFile::loadFromConfig(const filesystem::path &filepath) {
	configFilepath = filepath; // save filepath for latter use with writeToConfig

	// Use boardview file path as default PDF file path, with ext replaced with .pdf
	path = filepath;
	path.replace_extension("obd");

	if (!filesystem::exists(filepath)) // Config file doesn't exist, do not attempt to read or write it and load images
		return;

	auto configDir = filesystem::weakly_canonical(filepath).parent_path();

	Confparse confparse{};
	confparse.Load(filepath);

	std::string obdFilePathStr{confparse.ParseStr("OBDataFilePath", "")};
	if (!obdFilePathStr.empty()) path = configDir / filesystem::u8path(obdFilePathStr);

	writeToConfig(filepath);
}

void OBDataFile::writeToConfig(const filesystem::path &filepath) {
	if (filepath.empty()) // No destination file to save to
		return;

	std::error_code ec;
	auto confparse = Confparse{};
	confparse.Load(filepath);

	auto configDir = filesystem::weakly_canonical(filepath).parent_path();

	if (!path.empty()) {
		auto obdFilePath = filesystem::relative(path, configDir, ec);
		if (ec) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error writing OBData file path: %d - %s", ec.value(), ec.message().c_str());
		} else {
			confparse.WriteStr("OBDataFilePath", obdFilePath.string().c_str());
		}
	}
}

filesystem::path OBDataFile::getPath() const {
	return this->path;
}