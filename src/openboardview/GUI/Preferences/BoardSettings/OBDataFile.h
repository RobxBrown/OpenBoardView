#ifndef _PREFERENCES_OBDATAFILE_H_
#define _PREFERENCES_OBDATAFILE_H_

#include "OBDataBridge/OBDataFile.h"

#include <string>

namespace Preferences {

class OBDataFile {
  private:
	::OBDataFile &obdFile;
	::OBDataFile obdFileCopy;

  public:
	OBDataFile(::OBDataFile &obdFile);

	void render(bool shown);
	void save();
	void cancel();
	void clear();
};

} // namespace Preferences

#endif