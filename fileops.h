#ifndef FILEOPS_H
#define FILEOPS_H

#include <QString>

class FileOps
{
public:
	FileOps(QString LogDirectory);
	~FileOps();
	QString CheckVerboseValue();

private:
	void ReadXMLFile();
	void parseXML();
	void lineByLine(QString LogDirectory);
	bool VerboseLogging;
};

#endif // FILEOPS_H
