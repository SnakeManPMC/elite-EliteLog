/*
directory:
C:\Users\PMCBitch\AppData\Local\Frontier_Developments\Products\FORC-FDEV-D-1001\Logs

file name:
netLog.1412190357.01.log
netLog.<YEAR><MONTH><DAY><HOUR><MINUTE>.<?>.log

Log output line for Star System:
{03:58:10} System:21(Aulin) Body:13 Pos:(-1857.98,2264.45,648.209)
{02:19:51} System:4(Pipe (stem) Sector FM-U b3-4) Body:0 Pos:(9.16107e+009,-8.00884e+009,9.83126e+009) cruising

QFile oldFile(outFileName);
QFileInfo fileInfo;
fileInfo.setFile(oldFile);
QDateTime created = fileInfo.lastModified();
*/

#include "widget.h"
#include "ui_widget.h"

#include <QtWidgets>

Widget::Widget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::Widget)
{
	ui->setupUi(this);

	savedHammers = 0;
	filePos = 0;

	// at start we read our log and latest / current Star System
	readCmdrLog();
	// then scan log directory for existing logs, extract system name which should be
	// the same as returned from previous function
	scanDirectoryLogs();
	ui->textEdit->append("Welcome, current UTC time is: " + timeUTCtoString());

	// start the looping timer: void Widget::timerEvent(QTimerEvent *event)
/*
Hyperjump times (ASP, FSD class 5 rating A. for elite log hammering)
FSD charging 15sec
Countdown 5sec (total 20sec)
Hyperjump 13-16sec (total 33-36sec) but this depends really how long the server connection takes...
FSD cooldown start delay 4sec (total 37-40sec)
FSD cooldown 4sec (total 41-44sec)
 */
	// was 5000, but lets try 10000 (10sec) now
	timerId = startTimer(10000);
}

Widget::~Widget()
{
	// timer stuff
	killTimer(timerId);
	delete ui;
}


// reads contents of LOG directory, checks the newest file
void Widget::scanDirectoryLogs()
{
	QString elite_path = "C:\\Users\\PMCBitch\\AppData\\Local\\Frontier_Developments\\Products\\FORC-FDEV-D-1001\\Logs";
	//QString elite_path = "D:\\coding\\Test_files\\Elite_Logs";
	QStringList nameFilter("netLog.*.log");
	QDir directory(elite_path);
	QStringList txtFilesAndDirectories = directory.entryList(nameFilter, QDir::NoFilter, QDir::Time);

	//ui->textEdit->append("Last created/modified(?) .log filename: " + txtFilesAndDirectories[0]);

	//QString fullFilePath = elite_path + "\\" + txtFilesAndDirectories[0];

	// check if we have a new log open, then make it to our current log
	if (CurrentLogName.compare(txtFilesAndDirectories[0]))
	{
		CurrentLogName = txtFilesAndDirectories[0];
		// and set file position to zero
		filePos = 0;

		//ui->textEdit->append("New log file found! CurrentLogName set to: " + CurrentLogName + ", filePos set to: 0");
	}

	// this doesnt work as windows is too lazy to update the lastModified time for a file :(
	if (fileChangedOrNot( elite_path + "\\" + txtFilesAndDirectories[0] ))
	{
		parseLog(elite_path + "\\" + txtFilesAndDirectories[0]);
	}
	//parseLog(elite_path + "\\" + txtFilesAndDirectories[0]);
}


// parses the Star System name from the LOG file
void Widget::parseLog(QString elite_path)
{
	//ui->textEdit->append("Trying to open path+filename of: " + elite_path);

	QFile file(elite_path);

	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::information(this, tr("Unable to open Elite netLog file"),
		file.errorString());
		return;
	}

	//ui->textEdit->append("Opened " + elite_path);
	QString line;
	QTextStream in(&file);

	// we have position changed, ie we have already read this file a bit
	if (filePos > 0)
	{
		file.seek(filePos);
		//ui->textEdit->append("Set file.seek -> filePos to " + QString::number(filePos));
	}

	while (!in.atEnd())
	{
		line = in.readLine();

		// we have matching line of star system name
		if (line.contains("System:"))
		{
			MySystem = extractSystemName(line);
			//ui->textEdit->append(MySystem);
		}
	}
	// mark the position our file
	filePos = file.pos();
	// close right away, this eventually is not what we want
	file.close();
	//ui->textEdit->append("File read, closed and we're done. filePos: " + QString::number(filePos));
	//ui->textEdit->append("Your are in: " + MySystem + " system.");
}


// extracts the actual star system name from last system: entry
QString Widget::extractSystemName(QString line)
{
	// regexp reads "<ANYTHING>System:<ANY_NUMBER_OF_DIGITS>(" for match
	QStringList parsed = line.split(QRegExp("(.*)System:?\\d+\\("));
	QStringList finale = parsed[1].split(") Body:");
	return finale[0];
}


// returns current UTC time as string
QString Widget::timeUTCtoString()
{
	QDateTime date = QDateTime::currentDateTimeUtc();
	QString utctime = date.toString(Qt::ISODate);

	return utctime;
}


// at program start read cmdr.log for our last Star System we were in
void Widget::readCmdrLog()
{
	QFile CmdrLog("Cmdr.log");

	if (!CmdrLog.open(QIODevice::ReadOnly))
	{
		QMessageBox::information(this, tr("Unable to open Cmdr.log file for reading"),
		CmdrLog.errorString().append("\n\nUnable to open Cmdr.log text file for reading, something is wrong, dude..."));
		return;
	}

	QString line;
	QStringList tempsystem;
	QTextStream in(&CmdrLog);

	while (!in.atEnd())
	{
		line = in.readLine();
		tempsystem = line.split(",");
		MySystem = tempsystem[0];
	}
	// close right away, this eventually is not what we want
	CmdrLog.close();
	ui->textEdit->append("Your current system: " + MySystem);
}


// write new Star System name and UTC time string into cmdr.log file
void Widget::writeCmdrLog()
{
	QFile CmdrLog("Cmdr.log");

	if (!CmdrLog.open(QIODevice::Append))
	{
		QMessageBox::information(this, tr("Unable to open Cmdr.log file for writing"),
		CmdrLog.errorString());
		return;
	}

	QTextStream in(&CmdrLog);

	in << MySystem << "," << timeUTCtoString() << "\n";
	CmdrLog.close();
}


// this runs between seconds of as specificed in Widget constructor
void Widget::timerEvent(QTimerEvent *event)
{
	// this updates according to: timerId = startTimer(5000);
	QString oldsystem = MySystem;
	scanDirectoryLogs();
/*
	if (!oldsystem.compare(MySystem)) ui->textEdit->append(timeUTCtoString() + " - Systems are the same, no update, sorry buddy.");
	else
	{
		writeCmdrLog();
		ui->textEdit->append("Wrote our current system + time into Cmdr.log file!");
	}
*/
	if (oldsystem.compare(MySystem))
	{
		writeCmdrLog();
		ui->textEdit->append(MySystem + ", " + timeUTCtoString());
	}
}


// copy star system name to clipboard (so it can be pasted to IRC etc hehe)
void Widget::on_pushButton_clicked()
{
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(MySystem);
}


bool Widget::fileChangedOrNot(QString elite_file)
{
	QFile elite_log(elite_file);
	QFileInfo fileInfo;
	fileInfo.setFile(elite_log);
	// attempting to make the info as accurate as possible as there
	// was some odd delay on getting last modified data to show.
	fileInfo.refresh();
	created = fileInfo.lastModified();
	// get file size
	sizeOfLog = fileInfo.size();

	// first run check
	if (oldfiletime.isNull())
	{
		oldfiletime = created;
		sizeOfOldLog = fileInfo.size();
		ui->textEdit->append("Initialized file date/time...");
		return true;
	}

	// too heavy debug text every (timerEvent) seconds ;)
	//ui->textEdit->append("File last modified: " + created.toString() + ", oldfiletime: " + oldfiletime.toString() + ", sizeOfLog: "
	//		     + QString::number(sizeOfLog) + ", sizeOfOldLog: " + QString::number(sizeOfOldLog));

	if (created.operator >(oldfiletime) || sizeOfLog > sizeOfOldLog)
	{
		//ui->textEdit->append("created > oldfiletime || sizeOfLog > sizeOfOldLog; " + timeUTCtoString() + ", savedHammers: " + QString::number(savedHammers));
		savedHammers = 0;
		oldfiletime = created;
		sizeOfOldLog = sizeOfLog;
		return true;
	}
	else
	{
		savedHammers++;
		return false;
	}
}
