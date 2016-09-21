#include "fileops.h"
#include <QFile>
#include <QTextStream>
#include <QXmlStreamReader>
#include <QMessageBox>
#include <QDebug>
#include <QFileInfo>

FileOps::FileOps(QString LogDirectory)
{
	// initialize Verboselogging bool to false at start
	VerboseLogging = false;
	if (fileExists(LogDirectory + "//AppConfigLocal.xml"))
	{
		AppConfigLocal(LogDirectory, "//AppConfigLocal.xml");
	}
	else
	{
		AppConfigLocal(LogDirectory, "//AppConfig.xml");
	}
}

FileOps::~FileOps()
{

}


// check for verboselogging by reading XML line by line :)
void FileOps::AppConfigLocal(QString LogDirectory, QString ConfigName)
{
	QFile file (LogDirectory + ConfigName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox msgBox;
		msgBox.setText("Could not open " + ConfigName + " file\n\nCannot proceed, exiting...");
		msgBox.exec();
		exit(1);
	}
	// create buffer, read full XML file there, close file
	QString buffer,line;
	QTextStream in(&file);

	while(!in.atEnd())
	{
		line = in.readLine();

		// if we have verboselogging we simply exit
		if (line.contains("Verboselogging"))
		{
			VerboseLogging = true;
			return;
		}
		// if we encounter end of network xml (no verboselogging found), lets write verbose!
		if (line.contains("</Network"))
		{
			// remove >
			buffer.chop(2);
			// write our line PLUS the chopped > character
			buffer.append("\tVerboselogging=\"1\"\n\t>\n");
		}
		buffer.append(line + "\n");
	}
	file.close();

	// we reached here so it means verboselogging was not found, but lets check anyways hehe.
	if (!VerboseLogging)
	{
		QFile fout (LogDirectory + "//AppConfigLocal.xml");
		if (!fout.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QMessageBox msgBox;
			msgBox.setText("Error while creating a NEW AppConfigLocal.xml file (this should not happen, heh)\n\nCannot proceed, exiting...");
			msgBox.exec();
			exit(1);
		}

		// create buffer, read full XML file there, close file
		QTextStream out(&fout);
		// dump out buffer, ie XML with verboselogging into the file
		out << buffer;
		fout.close();

		// dunno why but lets set verboselogging on
		VerboseLogging = true;

		// informative message to user that hes AppConfig.xml has been edited
		QMessageBox msgBox;
		msgBox.setText(ConfigName + " did not include VerboseLogging, so it was added and\nAppConfigLocal.xml file created. All good."
			       "\n\nRemember to restart your Elite Launcher for verboselogging to take affect.");
		msgBox.exec();
	}
}


bool FileOps::fileExists(QString path)
{
	QFileInfo checkFile(path);
	// check if file exists and if yes: Is it really a file and no directory?
	if (checkFile.exists() && checkFile.isFile())
	{
		return true;
	}
	else
	{
		return false;
	}
}


QString FileOps::CheckVerboseValue()
{
	QString output;
	if (VerboseLogging)
		output = "Verboselogging = true";
	else
		output = "Verboselogging = false";
	return output;
}
