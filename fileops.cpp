#include "fileops.h"
#include <QFile>
#include <QTextStream>
#include <QXmlStreamReader>
#include <QMessageBox>
#include <QDebug>

FileOps::FileOps(QString LogDirectory)
{
	// initialize Verboselogging bool to false at start
	VerboseLogging = false;
	lineByLine(LogDirectory);
}

FileOps::~FileOps()
{

}

// check for verboselogging by reading XML line by line :)
void FileOps::lineByLine(QString LogDirectory)
{
	QFile file (LogDirectory + "//AppConfig.xml");
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox msgBox;
		msgBox.setText("Could not open AppConfig.xml file\n\nCannot proceed, exiting...");
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
		QFile fout (LogDirectory + "//AppConfig.xml");
		if (!fout.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QMessageBox msgBox;
			msgBox.setText("Error while creating a NEW AppConfig.xml file (this should not happen, heh)\n\nCannot proceed, exiting...");
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
	}
}


// this is just testing some code, not working or anything...
void FileOps::parseXML()
{
	QFile* file = new QFile("d://elite.dangerous//EliteGameDirSim//AppConfig.xml");
	if (!file->open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() << "Error opening XML file!";
		return;
	}

	// create buffer, read full XML file there, close file
	QString xmlbuffer;
	xmlbuffer = file->readAll();
	file->close();

	// start XML reader for our buffer
	QXmlStreamReader xml(xmlbuffer);

	while(!xml.atEnd() && !xml.hasError())
	{
		QXmlStreamReader::TokenType token = xml.readNext();

		if(token == QXmlStreamReader::StartElement)
		{
			if(xml.name() == "Network")
			{
				//qDebug() << "Network xml value found!";
				QXmlStreamAttributes attributes = xml.attributes();
				/*
				if (attributes.hasAttribute("LogFile"))
				{
					qDebug() << "LogFile: " << attributes.value("LogFile").toString();
				}
				*/
				if(attributes.hasAttribute("Verboselogging"))
				{
					//qDebug() << "Verboselogging ==" << attributes.value("Verboselogging").toString();
					VerboseLogging = true;
				}
				else
				{

				}
			}
		}
	}

	if(xml.hasError())
	{
		qDebug() << "XML error while reading the file!";
	}
	xml.clear();
}


// this is just testing some code, not working or anything...
void FileOps::ReadXMLFile()
{
	QFile file("d://elite.dangerous//EliteGameDirSim//AppConfig.xml");

	if (!file.open(QIODevice::ReadOnly))
	{
		return;
	}

	QString line;
	QTextStream in(&file);

	while (!in.atEnd())
	{
		line = in.readLine();

		// we have matching line of star system name
		if (line.contains("<Network"))
		{
			// read until file ends or we find </Network> string
			while (!in.atEnd() && !line.contains("</Network"))
			{
				line = in.readLine();
				// is this line: Verboselogging="1"
				if (line.contains("Verboselogging")) VerboseLogging = true;
			}
		}
	}
	file.close();
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
