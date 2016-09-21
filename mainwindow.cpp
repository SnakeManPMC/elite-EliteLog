/*
directory:
C:\Users\<username>\AppData\Local\Frontier_Developments\Products\FORC-FDEV-D-1001\Logs

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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "fileops.h"

#include <QtWidgets>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	// our softwares version
	eliteLogVersion = "v1.0.4 build 4";
	setWindowTitle("Elite Log " + eliteLogVersion + " by PMC");

	savedHammers = 0;
	filePos = 0;
	numSessionSystems = 0;
	numAllSystems = 0;
	cmdrLogFileName = "EliteLog.log";

	readEliteCFG();

	// AppConfig.xml reading and adding VerboseLogging if its missing.
	FileOps fo(logDirectory);

	// kind of debug thing, but still
	ui->textEdit->append("Log directory: " + logDirectory + "\\Logs");

	// at start we read our log and latest / current Star System
	readCmdrLog();
	// then scan log directory for existing logs, extract system name which should be
	// the same as returned from previous function
	scanDirectoryLogs();
	ui->textEdit->append("Welcome, current UTC time is: " + timeUTCtoString());

	// start the looping timer: void MainWindow::timerEvent(QTimerEvent *event)
/*
Hyperjump times (ASP, FSD class 5 rating A. for elite log hammering)
FSD charging 15sec
Countdown 5sec (total 20sec)
Hyperjump 13-16sec (total 33-36sec) but this depends really how long the server connection takes...
FSD cooldown start delay 4sec (total 37-40sec)
FSD cooldown 4sec (total 41-44sec)
 */
	// was 5000, but lets try 10000 (10sec) now
	timerId = startTimer(5000);
}

MainWindow::~MainWindow()
{
	// timer stuff
	killTimer(timerId);
	delete ui;
}

void MainWindow::readEliteCFG()
{
	QFile file("EliteLog.cfg");

	// exit program if this file is not found, as we cannot really
	// proceed without the config info
	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::information(this, tr("Unable to open EliteLog.cfg file for reading"),
		file.errorString().append("\n\nUnable to open EliteLog.cfg file for reading, something is wrong, dude!\n\nExiting program..."));
		exit(1);
	}

	QTextStream in(&file);

	// log directory path
	logDirectory = in.readLine();

	// highest session jump record + date
	QString tmp = in.readLine();
	QStringList parsed = tmp.split(",");
	bool ok;
	numSessionSystemsRecord = parsed[0].toInt(&ok, 10);
	numSessionSystemsRecordDate = parsed[1];

	// CMDR deaths
	tmp = in.readLine();
	deaths = tmp.toInt(&ok, 10);

	// Fuel scooped total
	tmp = in.readLine();
	scoopedTotal = tmp.toDouble(&ok);

	// jump distance shortest
	tmp = in.readLine();
	JumpDistShortest = tmp.toDouble(&ok);

	// jump distance longest
	tmp = in.readLine();
	JumpDistLongest = tmp.toDouble(&ok);

	ui->textEdit->append("EliteLog.cfg says game dir is: " + logDirectory);
	file.close();
}


void MainWindow::saveEliteCFG()
{
	QFile file("EliteLog.cfg");

	if (!file.open(QIODevice::WriteOnly))
	{
		QMessageBox::information(this, tr("Unable to open EliteLog.cfg file for saving!"),
		file.errorString());
		return;
	}

	QTextStream out(&file);

	out << logDirectory;
	out << "\n";
	out << numSessionSystemsRecord;
	out << ",";
	out << numSessionSystemsRecordDate;
	out << "\n";
	out << deaths;
	out << "\n";
	out << scoopedTotal;
	out << "\n";
	out << JumpDistShortest;
	out << "\n";
	out << JumpDistLongest;

	file.close();
}


// reads contents of LOG directory, checks the newest file
// if path or log file is not correct, it gives some index out of bounds error?
void MainWindow::scanDirectoryLogs()
{
	//QString elite_path = logDirectory + "\\Logs";
	QString elite_path = logDirectory;
	QStringList nameFilter("Journal.*.log");
	QDir directory(elite_path);
	QStringList txtFilesAndDirectories = directory.entryList(nameFilter, QDir::NoFilter, QDir::Time);

	if (txtFilesAndDirectories.count() == 0)
	{
		// exit
		ui->textEdit->append("Directory listed in EliteLog.cfg does not exist, please check the config.");
		exit(1);
	}
	//ui->textEdit->append("Last created/modified(?) .log filename: " + txtFilesAndDirectories[0]);

	//QString fullFilePath = elite_path + "\\" + txtFilesAndDirectories[0];

	// check if we have a new log open, then make it to our current log
	if (CurrentLogName.compare(txtFilesAndDirectories[0]))
	{
		CurrentLogName = txtFilesAndDirectories[0];
		// and set file position to zero
		filePos = 0;

		ui->textEdit->append("New log file found! CurrentLogName set to: " + CurrentLogName + ", filePos set to: 0");
	}

	// this doesnt work as windows is too lazy to update the lastModified time for a file :(
	if (fileChangedOrNot( elite_path + "\\" + txtFilesAndDirectories[0] ))
	{
		parseLog(elite_path + "\\" + txtFilesAndDirectories[0]);
	}
	//parseLog(elite_path + "\\" + txtFilesAndDirectories[0]);
}


// parses the Star System name from the LOG file
void MainWindow::parseLog(QString elite_path)
{
	//ui->textEdit->append("Trying to open path+filename of: " + elite_path);
	QFile file(elite_path);

	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::information(this, tr("Unable to open Elite netLog file"),
		file.errorString());
		return;
	}

	// we have position changed, ie we have already read this file a bit
	if (filePos > 0)
	{
		file.seek(filePos);
		//ui->textEdit->append("Set file.seek -> filePos to " + QString::number(filePos));
	}

	while (!file.atEnd())
	{
		parseSystemsJSON(file.readLine());
	}

	// mark the position our file
	filePos = file.pos();
	// close right away, this eventually is not what we want
	file.close();
	//ui->textEdit->append("File read, closed and we're done. filePos: " + QString::number(filePos));
	//ui->textEdit->append("Your are in: " + MySystem + " system.");
}


void MainWindow::parseSystemsJSON(QByteArray line)
{
	QJsonDocument d = QJsonDocument::fromJson(line);
	qDebug() << "isArray: " << d.isArray() << ", isEmpty: " << d.isEmpty() << ", isObject: " << d.isObject();

	QJsonObject sett2 = d.object();
	QJsonValue value = sett2.value(QString("event"));
	qDebug() << value;
	ui->textEdit->append("Event: " + value.toString());

	// FSDJump
	if (!value.toString().compare("FSDJump", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("StarSystem"));
		MySystem = value.toString();
		ui->textEdit->append("StarSystem: " + MySystem);

		// no station in immediate FSD jump range :)
		ui->StationName->setText("Station: -");

		value = sett2.value(QString("JumpDist"));
		qDebug() << value;

		// new shortest jump record
		if (JumpDistShortest > value.toDouble())
		{
			JumpDistShortest = value.toDouble();
			ui->textEdit->append("New shortest jump distance record!");
			ui->JumpDistanceRecords->setText("Jump distance shortest: " + QString::number(JumpDistShortest) + ", longest: " + QString::number(JumpDistLongest));
			saveEliteCFG();
		}
		// new longest jump record
		if (JumpDistLongest < value.toDouble())
		{
			JumpDistLongest = value.toDouble();
			ui->textEdit->append("New longest jump distance record!");
			ui->JumpDistanceRecords->setText("Jump distance shortest: " + QString::number(JumpDistShortest) + ", longest: " + QString::number(JumpDistLongest));
			saveEliteCFG();
		}

		JumpDist = value.toDouble();
		ui->JumpDistanceLast->setText("Last jump distance: " + QString::number(value.toDouble()));

		value = sett2.value(QString("FuelUsed"));
		qDebug() << value;
		FuelUsed = value.toDouble();
		ui->textEdit->append("FuelUsed: " + QString::number(value.toDouble()));

		value = sett2.value(QString("FuelLevel"));
		qDebug() << value;
		FuelLevel = value.toDouble();
		ui->textEdit->append("FuelLevel: " + QString::number(value.toDouble()));
	}

	// Location
	if (!value.toString().compare("location", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("StarSystem"));
		MySystem = value.toString();
		ui->textEdit->append("StarSystem: " + MySystem);

		value = sett2.value(QString("StationName"));
		MyStation = value.toString();
		ui->textEdit->append("StationName: " + MyStation);
		ui->StationName->setText("Station: " + MyStation);
	}

	// Docked
	if (!value.toString().compare("docked", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("StarSystem"));
		MySystem = value.toString();
		ui->textEdit->append("StarSystem: " + MySystem);

		value = sett2.value(QString("StationName"));
		MyStation = value.toString();
		ui->textEdit->append("StationName: " + MyStation);
		ui->StationName->setText("Station: " + MyStation);
	}

	// Undocked
	if (!value.toString().compare("Undocked", Qt::CaseInsensitive))
	{
		// star system is the same, we only left the station
		MyStation.clear();
		ui->textEdit->append("StationName: " + MyStation);
		ui->StationName->setText("Station: " + MyStation);
	}

	// Died
	if (!value.toString().compare("Died", Qt::CaseInsensitive))
	{
		deaths++;
		ui->Deaths->setText("CMDR Deaths: " + QString::number(deaths));
		ui->textEdit->append("You died! :) You have died " + QString::number(deaths) + " times!");
	}

	// Scan (exploration object)
	if (!value.toString().compare("Scan", Qt::CaseInsensitive))
	{
		ui->textEdit->append("*** DEBUG 'SCAN' DETECTED! ***");
		// if its a STAR it includes StarType value
		if (sett2.contains("StarType"))
		{
			ui->textEdit->append("*** DEBUG 'SCAN' IF-> StarType DETECTED! ***");
			value = sett2.value(QString("BodyName"));
			ui->textEdit->append("BodyName: " + value.toString());

			value = sett2.value(QString("StarType"));
			ui->textEdit->append("Star type: " + value.toString());

			value = sett2.value(QString("DistanceFromArrivalLS"));
			ui->textEdit->append("DistanceFromArrivalLS: " + QString::number(value.toDouble()));

			value = sett2.value(QString("StellarMass"));
			ui->textEdit->append("StellarMass: " + QString::number(value.toDouble()));

			value = sett2.value(QString("Radius"));
			ui->textEdit->append("Radius: " + QString::number(value.toDouble()));
		}

		// if its a PLANET it includes PlanetClass
		if (sett2.contains("PlanetClass"))
		{
			ui->textEdit->append("*** DEBUG 'SCAN' IF-> PlanetClass DETECTED! ***");
			value = sett2.value(QString("BodyName"));
			ui->textEdit->append("BodyName: " + value.toString());

			value = sett2.value(QString("TerraformState"));
			ui->textEdit->append("TerraformState: " + value.toString());

			value = sett2.value(QString("PlanetClass"));
			ui->textEdit->append("PlanetClass: " + value.toString());

			value = sett2.value(QString("Atmosphere"));
			ui->textEdit->append("Atmosphere: " + value.toString());

			value = sett2.value(QString("Volcanism"));
			ui->textEdit->append("Volcanism: " + value.toString());

			value = sett2.value(QString("DistanceFromArrivalLS"));
			ui->textEdit->append("DistanceFromArrivalLS: " + QString::number(value.toDouble()));

			value = sett2.value(QString("TidalLock"));
			ui->textEdit->append("TidalLock: " + QString::number(value.toDouble()));

			value = sett2.value(QString("MassEM"));
			ui->textEdit->append("MassEM: " + QString::number(value.toDouble()));

			value = sett2.value(QString("Radius"));
			ui->textEdit->append("Radius: " + QString::number(value.toDouble()));

			value = sett2.value(QString("SurfaceGravity"));
			ui->textEdit->append("SurfaceGravity: " + QString::number(value.toDouble()));

			value = sett2.value(QString("SurfaceTemperature"));
			ui->textEdit->append("SurfaceTemperature: " + QString::number(value.toDouble()));

			value = sett2.value(QString("SurfacePressure"));
			ui->textEdit->append("SurfacePressure: " + QString::number(value.toDouble()));

			value = sett2.value(QString("Landable"));
			ui->textEdit->append("Landable: " + QString::number(value.toDouble()));

			value = sett2.value(QString("OrbitalPeriod"));
			ui->textEdit->append("OrbitalPeriod: " + QString::number(value.toDouble()));

			value = sett2.value(QString("RotationPeriod"));
			ui->textEdit->append("RotationPeriod: " + QString::number(value.toDouble()));

			value = sett2.value(QString("Rings"));
			ui->textEdit->append("Rings: " + QString::number(value.toDouble()));

			// Materials
			QJsonObject fucker = sett2.value(QString("Materials")).toObject();
			for(QJsonObject::const_iterator iter = fucker.begin(); iter != fucker.end(); ++iter)
			{
				qDebug() << iter.key() << iter.value();
			}
		}
	}
	/*
"Materials":{ "iron":35.1, "nickel":26.5, "chromium":15.8, "manganese":14.5, "niobium":2.4, "yttrium":2.1, "tungsten":1.9, "arsenic":1.7 },
	 */

	// SellExplorationData
	/*
{ "timestamp":"2016-06-10T14:32:03Z", "event":"SellExplorationData", "Systems":[ "HIP 78085", "Praea Euq NW-W b1-3" ], "Discovered":[ "HIP 78085 A", "Praea Euq NW-W b1-3", "Praea Euq NW-W b1-3 3 a", "Praea Euq NW-W b1-3 3" ], "BaseValue":10822, "Bonus":3959 }
*/
	// MaterialCollected
	if (!value.toString().compare("MaterialCollected", Qt::CaseInsensitive))
	{
		// if its a RAW
		QString tmp = sett2.value(QString("Category")).toString();

		if (!tmp.compare("Raw"))
		{
			value = sett2.value(QString("Name"));
			ui->textEdit->append("MaterialCollected, Raw, Name: " + value.toString());
		}

		if (!tmp.compare("Encoded"))
		{
			value = sett2.value(QString("Name"));
			ui->textEdit->append("MaterialCollected, Encoded, Name: " + value.toString());
		}
	}

	// FuelScoop
	if (!value.toString().compare("FuelScoop", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Scooped"));
		scoopedTotal += value.toDouble();
		ui->textEdit->append("Scooped: " + QString::number(value.toDouble()) + ", total scooped: " + QString::number(scoopedTotal));
		ui->FuelScoopedTotal->setText("Fuel scooped total: " + QString::number(scoopedTotal));

		// EliteLog.CFG update, but damn you will hammer this saving a lot
		// when exploring and fuel scooping every few minutes :(
		saveEliteCFG();
	}
}


// extracts the actual star system name from last system: entry
QString MainWindow::extractSystemName(QString line)
{
	QString tmpSystemName = MySystem;
	// regexp reads "<ANYTHING>System:<ANY_NUMBER_OF_DIGITS>(" for match
	//QStringList parsed = line.split(QRegExp("(.*)System:?\\d+\\("));

	// v2.1 the engineers update change:
	// {07:31:19} System:"Shinrarta Dezhra" StarPos:(55.719,17.594,27.156)ly  NormalFlight
	QStringList parsed = line.split(QRegExp("(.*)System:\""));
	QStringList finale = parsed[1].split("\" StarPos:");

	// check if we ignore Training system (playing in training missions)
	if ((finale[0] == "Training" || finale[0] == "Destination") && (ui->IgnoreTraining->isChecked()))
		return tmpSystemName;
	else
		return finale[0];
}


QString MainWindow::extractStationName(QString line)
{
	QString final;
	QStringList parsed = line.split(":");
	// if we are NOT in station, if the name is just system name, we make it "-"
	if (parsed[5].contains(parsed[6]))
		final = "-";
	else
		final = parsed[5];
	return final;
}


// returns current UTC time as string
QString MainWindow::timeUTCtoString()
{
	QDateTime date = QDateTime::currentDateTimeUtc();
	QString utctime = date.toString(Qt::ISODate);

	return utctime;
}


// at program start read LOG for our last Star System we were in
void MainWindow::readCmdrLog()
{
	QFile CmdrLog(cmdrLogFileName);

	if (!CmdrLog.open(QIODevice::ReadOnly))
	{
		QMessageBox::information(this, tr("Unable to open your personal LOG file for reading"),
		CmdrLog.errorString().append("\n\nUnable to open your personal LOG text file for reading, something is wrong, dude..."));
		exit(1);
	}

	QString line;
	QStringList tempsystem;
	QTextStream in(&CmdrLog);

	while (!in.atEnd())
	{
		line = in.readLine();
		tempsystem = line.split(",");
		MySystem = tempsystem[0];

		numAllSystems++;
		checkUniqueSystem(MySystem);
	}
	// close right away, this eventually is not what we want
	CmdrLog.close();

	updateSystemsVisited();
	// basic info for our text edit
	ui->textEdit->append("Your current system: " + MySystem + ". You've visited " + QString::number(uniqueSystems.count()) + " unique systems.");
}


// write new Star System name and UTC time string into LOG file
void MainWindow::writeCmdrLog()
{
	QFile CmdrLog(cmdrLogFileName);

	if (!CmdrLog.open(QIODevice::Append))
	{
		QMessageBox::information(this, tr("Unable to open LOG file for writing"),
		CmdrLog.errorString());
		return;
	}

	QTextStream in(&CmdrLog);

	in << MySystem << "," << timeUTCtoString() << "\n";
	CmdrLog.close();
}


// this runs between seconds of as specificed in MainWindow constructor
void MainWindow::timerEvent(QTimerEvent *event)
{
	// this updates according to: timerId = startTimer(5000);
	QString oldsystem = MySystem;
	scanDirectoryLogs();
/*
	if (!oldsystem.compare(MySystem)) ui->textEdit->append(timeUTCtoString() + " - Systems are the same, no update, sorry buddy.");
	else
	{
		writeCmdrLog();
		ui->textEdit->append("Wrote our current system + time into LOG file!");
	}
*/
	if (oldsystem.compare(MySystem))
	{
		writeCmdrLog();
		ui->textEdit->append(MySystem + ", " + timeUTCtoString());

		// if we automatically clipboard system name
		if (ui->SysNamAutoClipboard->isChecked()) on_pushButton_clicked();

		// add one new system visited counter
		numSessionSystems++;
		numAllSystems++;
		// check for new high score
		if (numSessionSystemsRecord < numSessionSystems)
		{
			// increment this record
			numSessionSystemsRecord = numSessionSystems;
			numSessionSystemsRecordDate = timeUTCtoString();
			// then save it to file, ouch, lot of saving in long gaming session huh?
			saveEliteCFG();
		}

		// check unique system
		checkUniqueSystem(MySystem);
		// update systems visited label
		updateSystemsVisited();
	}
}


// copy star system name to clipboard (so it can be pasted to IRC etc hehe)
void MainWindow::on_pushButton_clicked()
{
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(MySystem);
}


// copy station name to clipboard
void MainWindow::on_pushButton_2_clicked()
{
	// station to clipboard
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(MyStation);
}


// utc timestamp to clipboard
void MainWindow::on_pushButton_3_clicked()
{
	// station to clipboard
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(timeUTCtoString());
}


bool MainWindow::fileChangedOrNot(QString elite_file)
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


// check for unique never before visited system, if it is, add it to the list
void MainWindow::checkUniqueSystem(QString MySystem)
{
	if (!uniqueSystems.contains(MySystem)) uniqueSystems.push_back(MySystem);
}


// updates the Label on UI for unique systems visited
void MainWindow::updateSystemsVisited()
{
	// update label which shows number of unique systems
	ui->SystemsVisited->setText("Unique Systems: " + QString::number(uniqueSystems.count()));
	ui->SessionSystemVisits->setText("Session Systems: " + QString::number(numSessionSystems) + ", Record: " + QString::number(numSessionSystemsRecord)
					 + " at " + numSessionSystemsRecordDate);
	ui->totalSystemsVisited->setText("Total Systems: " + QString::number(numAllSystems));
	// this doesnt need to be here, but I dont know where else
	// will it be updated on program start?
	ui->Deaths->setText("CMDR Deaths: " + QString::number(deaths));
	ui->FuelScoopedTotal->setText("Fuel scooped total: " + QString::number(scoopedTotal));
}
