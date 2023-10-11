#ifndef _OBDBRIDGE_H_
#define _OBDBRIDGE_H_

#include "filesystem_impl.h"

#include "OBDataFile.h"

class OBDataFile; // Forward declaration to solve circular includes

class OBDataBridge {
  public:
	virtual ~OBDataBridge();

	virtual void OpenDocument(const OBDataFile &obdFile);
	virtual void CloseDocument();
	//virtual void DocumentSearch(const std::string &str, bool wholeWordsOnly, bool caseSensitive);
	//virtual bool HasNewSelection();
	//virtual std::string GetSelection() const;
};

#endif //_OBDBRIDGE_H_