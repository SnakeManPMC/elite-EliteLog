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
	void AppConfigLocal(QString LogDirectory, QString ConfigName);
	bool fileExists(QString path);
	bool VerboseLogging;
};

#endif // FILEOPS_H
