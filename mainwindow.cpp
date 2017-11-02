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

	setupTableWidget();

	// our softwares version
	eliteLogVersion = "v1.1.7";
	setWindowTitle("Elite Log " + eliteLogVersion + " by PMC");

	savedHammers = 0;
	filePos = 0;
	numSessionSystems = 0;
	numAllSystems = 0;
	cmdrLogFileName = "EliteLog.log";

	readEliteCFG();

	// AppConfig.xml reading and adding VerboseLogging if its missing.
	// Elite v2.2 introduces Journal JSON files in stupid
	// C:\Users\USERNAME\Saved Games\Frontier Developments\Elite Dangerous\ directory
	// so we disable out appconfig.xml edits as they are obsolete
	//FileOps fo(logDirectory);

	// kind of debug thing, but still
	ui->textEdit->append("Log directory: " + logDirectory);

	// at start we read our log and latest / current Star System
	readCmdrLog();
	// then scan log directory for existing logs, extract system name which should be
	// the same as returned from previous function
	scanDirectoryLogs();
	ui->textEdit->append("Greetings commander! Current UTC time is: " + timeUTCtoString());

	// update GUI highscores and stuff
	updateSystemsVisited();

	// start the looping timer: void MainWindow::timerEvent(QTimerEvent *event)
	// was 5000, but lets try 10000 (10sec) now
	timerId = startTimer(2500);
}

MainWindow::~MainWindow()
{
	// timer stuff
	killTimer(timerId);
	delete ui;
}


void MainWindow::setupTableWidget()
{
	ui->tableWidget->setColumnCount(3);
	QStringList header;
	header << "UTC" << "Event" << "Detais";
	ui->tableWidget->setHorizontalHeaderLabels(header);

	QHeaderView* trueheader = ui->tableWidget->horizontalHeader();
	trueheader->setSectionResizeMode(QHeaderView::Fixed);
	trueheader->resizeSection(0, 200);
	trueheader->resizeSection(1, 150);
	trueheader->resizeSection(2, 800);
}


void MainWindow::updateTableView(const QString& date, const QString& event, const QString& details)
{
	int num = ui->tableWidget->rowCount();

	ui->tableWidget->insertRow(num);
	ui->tableWidget->setItem(num, 0, new QTableWidgetItem(date));
	ui->tableWidget->setItem(num, 1, new QTableWidgetItem(event));
	ui->tableWidget->setItem(num, 2, new QTableWidgetItem(details));

	// set focus on latest item added, ie bottom
	ui->tableWidget->scrollToBottom();
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
	scoopedTotal = tmp.toFloat(&ok);

	// jump distance shortest
	tmp = in.readLine();
	JumpDistShortest = tmp.toFloat(&ok);

	// jump distance longest
	tmp = in.readLine();
	JumpDistLongest = tmp.toFloat(&ok);

	// LS distance shortest
	tmp = in.readLine();
	DistanceFromArrivalLSMin = tmp.toFloat(&ok);

	// LS distance longest
	tmp = in.readLine();
	DistanceFromArrivalLSMax = tmp.toFloat(&ok);

	// planet radius smallest
	tmp = in.readLine();
	planetRadiusSmallest = tmp.toFloat(&ok);

	// planet radius largest
	tmp = in.readLine();
	planetRadiusLargest = tmp.toFloat(&ok);

	// planet surface gravity lowest
	tmp = in.readLine();
	surfaceGravityLowest = tmp.toFloat(&ok);

	// planet surface gravity highest
	tmp = in.readLine();
	surfaceGravityHighest = tmp.toFloat(&ok);

	// stellar mass lowest
	tmp = in.readLine();
	stellarMassLowest = tmp.toFloat(&ok);

	// stellar mass highest
	tmp = in.readLine();
	stellarMassHighest = tmp.toFloat(&ok);

	// stellar radius smallest
	tmp = in.readLine();
	stellarRadiusSmallest = tmp.toFloat(&ok);

	// stellar radius largest
	tmp = in.readLine();
	stellarRadiusLargest = tmp.toFloat(&ok);

	// landable planet smallest
	tmp = in.readLine();
	landableRadiusSmallest = tmp.toFloat(&ok);

	// landable planet largest
	tmp = in.readLine();
	landableRadiusLargest = tmp.toFloat(&ok);

	// landable gravity lowest
	tmp = in.readLine();
	landableGravityLowest = tmp.toFloat(&ok);

	// landable gravity highest
	tmp = in.readLine();
	landableGravityHighest = tmp.toFloat(&ok);

	// fuel level lowest
	tmp = in.readLine();
	fuelLevelLowest = tmp.toFloat(&ok);

	// star age youngest
	tmp = in.readLine();
	age_MYyoungest = tmp.toFloat(&ok);

	// star age oldest
	tmp = in.readLine();
	age_MYoldest = tmp.toFloat(&ok);

	ui->textEdit->append("EliteLog.cfg says the Journal dir is: " + logDirectory);
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
	out << numSessionSystemsRecord << "," << numSessionSystemsRecordDate;
	out << "\n";
	out << deaths;
	out << "\n";
	out << scoopedTotal;
	out << "\n";
	out << JumpDistShortest;
	out << "\n";
	out << JumpDistLongest;
	out << "\n";
	out << DistanceFromArrivalLSMin;
	out << "\n";
	out << DistanceFromArrivalLSMax;
	out << "\n";
	out << planetRadiusSmallest;
	out << "\n";
	out << planetRadiusLargest;
	out << "\n";
	out << surfaceGravityLowest;
	out << "\n";
	out << surfaceGravityHighest;
	out << "\n";
	out << stellarMassLowest;
	out << "\n";
	out << stellarMassHighest;
	out << "\n";
	out << stellarRadiusSmallest;
	out << "\n";
	out << stellarRadiusLargest;
	out << "\n";
	out << landableRadiusSmallest;
	out << "\n";
	out << landableRadiusLargest;
	out << "\n";
	out << landableGravityLowest;
	out << "\n";
	out << landableGravityHighest;
	out << "\n";
	out << fuelLevelLowest;
	out << "\n";
	out << age_MYyoungest;
	out << "\n";
	out << age_MYoldest;

	// some comments for easier elitelog.cfg reading
	out << "\n\nEliteLog.cfg line descriptions:\n";
	out << "1: Journal log directory\n";
	out << "2: Number of session system jumps high score + UTC date\n";
	out << "3: CMDR deaths\n";
	out << "4: Fueld scooped total\n";
	out << "5: Jump distance shortest\n";
	out << "6: Jump distance longest\n";
	out << "7: Planet LS shortest\n";
	out << "8: Planet LS longest\n";
	out << "9: Planet radius smallest\n";
	out << "10: Planet radius largest\n";
	out << "11: Planet surface gravity lowest\n";
	out << "12: Planet surface gravity highest\n";
	out << "13: Stellar mass lowest\n";
	out << "14: Stellar mass highest\n";
	out << "15: Stellar radius smallest\n";
	out << "16: Stellar radius largest\n";
	out << "17: Landable planet smallest\n";
	out << "18: Landable planet largest\n";
	out << "19: Landable gravity lowest\n";
	out << "20: Landable gravity highest\n";
	out << "21: Fuel level lowest\n";
	out << "22: Star age youngest\n";
	out << "23: Star age oldest\n";

	file.close();
}


// reads contents of LOG directory, checks the newest file
// if path or log file is not correct, it gives some index out of bounds error?
void MainWindow::scanDirectoryLogs()
{
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
	//qDebug() << "isArray: " << d.isArray() << ", isEmpty: " << d.isEmpty() << ", isObject: " << d.isObject();

	QJsonObject sett2 = d.object();
	QJsonValue value = sett2.value(QString("event"));
	//qDebug() << value;
	ui->textEdit->append("Event: " + value.toString());

	// Fileheader
	if (!value.toString().compare("Fileheader", Qt::CaseInsensitive))
	{
		// gameversion
		value = sett2.value(QString("gameversion"));
		ui->textEdit->append("gameversion: " + value.toString());
		// build
		value = sett2.value(QString("build"));
		ui->textEdit->append("build: " + value.toString());
	}

	// LoadGame
	if (!value.toString().compare("LoadGame", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Commander"));
		ui->textEdit->append("LoadGame, Commander: " + value.toString());
		value = sett2.value(QString("Ship"));
		ui->textEdit->append("-> Ship: " + value.toString());
		value = sett2.value(QString("ShipID"));
		ui->textEdit->append("-> ShipID: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("GameMode"));
		ui->textEdit->append("-> GameMode: " + value.toString());
		value = sett2.value(QString("Credits"));
		ui->textEdit->append("-> Credits: " + QString::number(value.toInt()));
		value = sett2.value(QString("Loan"));
		ui->textEdit->append("-> Loan: " + QString::number(value.toVariant().toFloat()));
	}

	// progress and rank are both numeric value from 0 to 100
	// Progress
	if (!value.toString().compare("Progress", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Combat"));
		ui->textEdit->append("Progress, Combat: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Trade"));
		ui->textEdit->append("-> Trade: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Explore"));
		ui->textEdit->append("-> Explore: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Empire"));
		ui->textEdit->append("-> Empire: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Federation"));
		ui->textEdit->append("-> Federation: " + QString::number(value.toVariant().toFloat()));
	}

	// Rank
	if (!value.toString().compare("Rank", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Combat"));
		ui->textEdit->append("Rank, Combat: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Trade"));
		ui->textEdit->append("-> Trade: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Explore"));
		ui->textEdit->append("-> Explore: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Empire"));
		ui->textEdit->append("-> Empire: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Federation"));
		ui->textEdit->append("-> Federation: " + QString::number(value.toVariant().toFloat()));
	}

	// FSDJump
	if (!value.toString().compare("FSDJump", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("StarSystem"));
		MySystem = value.toString();
		ui->SystemName->setText("System: " + MySystem);
		ui->textEdit->append("StarSystem: " + MySystem);

		// no station in immediate FSD jump range :)
		ui->StationName->setText("Station: -");

		// StarPos is array
		// "StarPos":[195.406,281.469,5.500]
		value = sett2.value(QString("StarPos"));
		//qDebug() << "value: " << value;
		/*
		qDebug() << "value.toArray(): " << value.toArray();
		qDebug() << "value.toArray().at(0): " << value.toArray().at(0);
		qDebug() << "value.toArray().at(0).toFloat(): " << value.toArray().at(0).toDouble();
		*/
		ui->textEdit->append("StarPos:[" + QString::number(value.toArray().at(0).toDouble()) + "," +
				     QString::number(value.toArray().at(1).toDouble()) + "," +
				     QString::number(value.toArray().at(2).toDouble()) + "]");

		// SystemAllegiance
		value = sett2.value(QString("SystemAllegiance"));
		ui->textEdit->append("SystemAllegiance: " + value.toString());

		// SystemEconomy
		value = sett2.value(QString("SystemEconomy"));
		ui->textEdit->append("SystemEconomy: " + value.toString());

		// SystemEconomy_Localised
		value = sett2.value(QString("SystemEconomy_Localised"));
		ui->textEdit->append("SystemEconomy_Localised: " + value.toString());

		// SystemGovernment
		value = sett2.value(QString("SystemGovernment"));
		ui->textEdit->append("SystemGovernment: " + value.toString());

		// SystemGovernment_Localised
		value = sett2.value(QString("SystemGovernment_Localised"));
		ui->textEdit->append("SystemGovernment_Localised: " + value.toString());

		// SystemSecurity
		value = sett2.value(QString("SystemSecurity"));
		ui->textEdit->append("SystemSecurity: " + value.toString());

		// SystemSecurity_Localised
		value = sett2.value(QString("SystemSecurity_Localised"));
		ui->textEdit->append("SystemSecurity_Localised: " + value.toString());

		value = sett2.value(QString("JumpDist"));
		//qDebug() << value;

		// new shortest jump record
		if (JumpDistShortest > value.toVariant().toFloat())
		{
			JumpDistShortest = value.toVariant().toFloat();
			ui->textEdit->append("New shortest jump distance record!");
			updateSystemsVisited();
			saveEliteCFG();
		}
		// new longest jump record
		if (JumpDistLongest < value.toVariant().toFloat())
		{
			JumpDistLongest = value.toVariant().toFloat();
			ui->textEdit->append("New longest jump distance record!");
			updateSystemsVisited();
			saveEliteCFG();
		}

		JumpDist = value.toVariant().toFloat();
		ui->JumpDistanceLast->setText("Last jump distance: " + QString::number(value.toVariant().toFloat()));

		value = sett2.value(QString("FuelUsed"));
		//qDebug() << value;
		FuelUsed = value.toVariant().toFloat();

		// tableview jump to system
		ttime = sett2.value(QString("timestamp")).toString();
		tevent = "Jump to System";
		tdetails = (MySystem + ", Jump distance: " + QString::number(sett2.value(QString("JumpDist")).toVariant().toFloat()) + "Ly, Fuel used: " + QString::number(sett2.value(QString("FuelUsed")).toVariant().toFloat()));
		updateTableView(ttime, tevent, tdetails);

		value = sett2.value(QString("FuelLevel"));
		//qDebug() << value;
		FuelLevel = value.toVariant().toFloat();
		// we have reached lowest fuel level ever!
		if (fuelLevelLowest > value.toVariant().toFloat())
		{
			fuelLevelLowest = value.toVariant().toFloat();
			ui->textEdit->append("Come on Maverick we're running on fumes! Lets land this sucker! Lowest fuel level record!");
			updateSystemsVisited();
			saveEliteCFG();
		}
		ui->textEdit->append("FuelUsed: " + QString::number(sett2.value(QString("FuelUsed")).toVariant().toFloat()) + ", FuelLevel: " + QString::number(sett2.value(QString("FuelLevel")).toVariant().toFloat()));
	}

	// Location
	if (!value.toString().compare("Location", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("StarSystem"));
		MySystem = value.toString();
		ui->SystemName->setText("System: " + MySystem);
		ui->textEdit->append("StarSystem: " + MySystem);

		value = sett2.value(QString("StationName"));
		MyStation = value.toString();
		ui->textEdit->append("StationName: " + MyStation);
		ui->StationName->setText("Station: " + MyStation);

		// missing lot of stuff now, Location is very similar to FSDJump ...

		ttime = sett2.value(QString("timestamp")).toString();
		tevent = "Location in System";
		tdetails = MySystem;
		updateTableView(ttime, tevent, tdetails);
	}

	/*
	 * this can be anywhere in the system.
	 * next to a planet OR a station...
	 * but how can you tell which is which as it just says "Body"
	 */
	// SupercruiseExit
	if (!value.toString().compare("SupercruiseExit", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Body"));
		//MyStation = value.toString();
		// lets just display debug for now
		ui->textEdit->append("SupercruiseExit, Body: " + value.toString());
		value = sett2.value(QString("BodyType"));
		ui->textEdit->append("-> BodyType: " + value.toString());
	}

	// DockingRequested
	if (!value.toString().compare("DockingRequested", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("StationName"));
		ui->textEdit->append("DockingRequested, StationName: " + value.toString());
	}

	// DockingDenied
	if (!value.toString().compare("DockingDenied", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Reason"));
		ui->textEdit->append("DockingDenied, Reason: " + value.toString());
		value = sett2.value(QString("StationName"));
		ui->textEdit->append("DockingDenied, StationName: " + value.toString());
	}

	// DockingGranted
	if (!value.toString().compare("DockingGranted", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("LandingPad"));
		ui->textEdit->append("DockingGranted, LandingPad: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("StationName"));
		ui->textEdit->append("DockingGranted, StationName: " + value.toString());
	}

	// Docked
	if (!value.toString().compare("Docked", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("StarSystem"));
		MySystem = value.toString();
		ui->SystemName->setText("System: " + MySystem);
		ui->textEdit->append("StarSystem: " + MySystem);

		value = sett2.value(QString("StationName"));
		MyStation = value.toString();
		ui->textEdit->append("StationName: " + MyStation);
		ui->StationName->setText("Station: " + MyStation);

		value = sett2.value(QString("StationType"));
		ui->textEdit->append("StationType: " + value.toString());

		// tableview docked to station
		ttime = sett2.value(QString("timestamp")).toString();
		tevent = "Docked to Station";
		tdetails = (MyStation + " at " + MySystem);
		updateTableView(ttime, tevent, tdetails);
	}

	// Undocked
	if (!value.toString().compare("Undocked", Qt::CaseInsensitive))
	{
		// star system is the same, we only left the station
		MyStation.clear();
		ui->textEdit->append("StationName: " + MyStation);
		ui->StationName->setText("Station: " + MyStation);
		value = sett2.value(QString("StationType"));
		ui->textEdit->append("StationType: " + value.toString());

		// tableview undocked from station
		ttime = sett2.value(QString("timestamp")).toString();
		tevent = "Undocked from Station";
		tdetails = (MyStation + " at " + MySystem);
		updateTableView(ttime, tevent, tdetails);
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
			ttime = sett2.value(QString("timestamp")).toString();
			tevent = "Scan: Star";
			tdetails = (sett2.value(QString("BodyName")).toString() + ", Type: " + sett2.value(QString("StarType")).toString() +
				    ", Age: " + QString::number(sett2.value(QString("Age_MY")).toVariant().toFloat()));
			updateTableView(ttime, tevent, tdetails);

			ui->textEdit->append("*** DEBUG 'SCAN' IF-> StarType DETECTED! ***");
			value = sett2.value(QString("BodyName"));
			ui->textEdit->append("BodyName: " + value.toString());

			value = sett2.value(QString("DistanceFromArrivalLS"));
			ui->textEdit->append("DistanceFromArrivalLS: " + QString::number(value.toVariant().toFloat()));

			value = sett2.value(QString("StarType"));
			ui->textEdit->append("StarType: " + value.toString());

			value = sett2.value(QString("StellarMass"));
			// stellarMass lowest highscore
			if (stellarMassLowest > value.toVariant().toFloat())
			{
				stellarMassLowest = value.toVariant().toFloat();
				ui->textEdit->append("New stellarMass highscore LOWEST!");
				updateSystemsVisited();
				saveEliteCFG();
			}
			// stellarMass highest highscore
			if (stellarMassHighest < value.toVariant().toFloat())
			{
				stellarMassHighest = value.toVariant().toFloat();
				ui->textEdit->append("New stellarMass highscore HIGHEST!");
				updateSystemsVisited();
				saveEliteCFG();
			}
			ui->textEdit->append("StellarMass: " + QString::number(value.toVariant().toFloat()));

			value = sett2.value(QString("Radius"));
			// stellar radius smallest highscore
			if (stellarRadiusSmallest > value.toVariant().toFloat())
			{
				stellarRadiusSmallest = value.toVariant().toFloat();
				ui->textEdit->append("New stellar radius smallest highscore!");
				updateSystemsVisited();
				saveEliteCFG();
			}
			// stellar radius largest highscore
			if (stellarRadiusLargest < value.toVariant().toFloat())
			{
				stellarRadiusLargest = value.toVariant().toFloat();
				ui->textEdit->append("New stellar radius largest highscore!");
				updateSystemsVisited();
				saveEliteCFG();
			}
			ui->textEdit->append("Radius: " + QString::number(value.toVariant().toFloat()));

			// AbsoluteMagnitude
			value = sett2.value(QString("AbsoluteMagnitude"));
			ui->textEdit->append("AbsoluteMagnitude: " + QString::number(value.toVariant().toFloat()));

			// Age_MY
			value = sett2.value(QString("Age_MY"));
			// star age youngest highscore
			if (age_MYyoungest > value.toVariant().toFloat())
			{
				age_MYyoungest = value.toVariant().toFloat();
				ui->textEdit->append("New star youngest highscore!");
				updateSystemsVisited();
				saveEliteCFG();
			}
			// star age oldest highscore
			if (age_MYoldest < value.toVariant().toFloat())
			{
				age_MYoldest = value.toVariant().toFloat();
				ui->textEdit->append("New star oldest highscore!");
				updateSystemsVisited();
				saveEliteCFG();
			}
			ui->textEdit->append("Age_MY: " + QString::number(value.toVariant().toFloat()));

			// SurfaceTemperature
			value = sett2.value(QString("SurfaceTemperature"));
			ui->textEdit->append("SurfaceTemperature: " + QString::number(value.toVariant().toFloat()));

			// SemiMajorAxis
			value = sett2.value(QString("SemiMajorAxis"));
			ui->textEdit->append("SemiMajorAxis: " + QString::number(value.toVariant().toFloat()));

			// Eccentricity
			value = sett2.value(QString("Eccentricity"));
			ui->textEdit->append("Eccentricity: " + QString::number(value.toVariant().toFloat()));

			// OrbitalInclination
			value = sett2.value(QString("OrbitalInclination"));
			ui->textEdit->append("OrbitalInclination: " + QString::number(value.toVariant().toFloat()));

			// Periapsis
			value = sett2.value(QString("Periapsis"));
			ui->textEdit->append("Periapsis: " + QString::number(value.toVariant().toFloat()));

			// OrbitalPeriod
			value = sett2.value(QString("OrbitalPeriod"));
			ui->textEdit->append("OrbitalPeriod: " + QString::number(value.toVariant().toFloat()));

			// RotationPeriod
			value = sett2.value(QString("RotationPeriod"));
			ui->textEdit->append("RotationPeriod: " + QString::number(value.toVariant().toFloat()));
		}

		// if its a PLANET it includes PlanetClass
		if (sett2.contains("PlanetClass"))
		{
			ttime = sett2.value(QString("timestamp")).toString();
			tevent = "Scan: Planet";
			tdetails = sett2.value(QString("BodyName")).toString();
			tdetails.append(", Class: " + sett2.value(QString("PlanetClass")).toString());
			updateTableView(ttime, tevent, tdetails);

			ui->textEdit->append("*** DEBUG 'SCAN' IF-> PlanetClass DETECTED! ***");
			value = sett2.value(QString("BodyName"));
			ui->textEdit->append("BodyName: " + value.toString());

			value = sett2.value(QString("DistanceFromArrivalLS"));
			// new minimum LS highscore
			if (DistanceFromArrivalLSMin > value.toVariant().toFloat())
			{
				DistanceFromArrivalLSMin = value.toVariant().toFloat();
				ui->textEdit->append("New DistanceFromArrivalLSMin highscore! " + QString::number(DistanceFromArrivalLSMin));
				updateSystemsVisited();
				saveEliteCFG();
			}
			// new maximum LS highscore
			if (DistanceFromArrivalLSMax < value.toVariant().toFloat())
			{
				DistanceFromArrivalLSMax = value.toVariant().toFloat();
				ui->textEdit->append("New DistanceFromArrivalLSMax highscore! " + QString::number(DistanceFromArrivalLSMax));
				updateSystemsVisited();
				saveEliteCFG();
			}
			ui->textEdit->append("DistanceFromArrivalLS: " + QString::number(value.toVariant().toFloat()));

			value = sett2.value(QString("TidalLock"));
			//qDebug() << "TidalLock: " << value;
			ui->textEdit->append("TidalLock: " + QString::number(value.toBool()));

			value = sett2.value(QString("TerraformState"));
			//qDebug() << "TerraformState: " << value;
			ui->textEdit->append("TerraformState: " + value.toString());

			value = sett2.value(QString("PlanetClass"));
			ui->textEdit->append("PlanetClass: " + value.toString());

			value = sett2.value(QString("Atmosphere"));
			//qDebug() << "Atmosphere: " << value;
			ui->textEdit->append("Atmosphere: " + value.toString());

			value = sett2.value(QString("Volcanism"));
			//qDebug() << "Volcanism: " << value;
			ui->textEdit->append("Volcanism: " + value.toString());

			value = sett2.value(QString("MassEM"));
			ui->textEdit->append("MassEM: " + QString::number(value.toVariant().toFloat()));

			// radius in JSON is in METERS, so divide by 1000 to get kilometers
			value = sett2.value(QString("Radius"));
			// planet radius smallest highscore
			if (planetRadiusSmallest > value.toVariant().toFloat())
			{
				planetRadiusSmallest = value.toVariant().toFloat();
				ui->textEdit->append("New highscore planet radius SMALLEST! " + QString::number(planetRadiusSmallest / 1000));
				updateSystemsVisited();
				saveEliteCFG();
			}
			// planet radius largest highscore
			if (planetRadiusLargest < value.toVariant().toFloat())
			{
				planetRadiusLargest = value.toVariant().toFloat();
				ui->textEdit->append("New highscore planet radius LARGEST! " + QString::number(planetRadiusLargest / 1000));
				updateSystemsVisited();
				saveEliteCFG();
			}
			ui->textEdit->append("Radius: " + QString::number(value.toVariant().toFloat()));

			value = sett2.value(QString("SurfaceGravity"));
			// planet gravity lowest highscore
			if (surfaceGravityLowest > value.toVariant().toFloat())
			{
				surfaceGravityLowest = value.toVariant().toFloat();
				ui->textEdit->append("New highscore planet surface gravity LOWEST: " + QString::number(surfaceGravityLowest / 100));
				updateSystemsVisited();
				saveEliteCFG();
			}
			// planet gravity highest highscore
			if (surfaceGravityHighest < value.toVariant().toFloat())
			{
				surfaceGravityHighest = value.toVariant().toFloat();
				ui->textEdit->append("New highscore planet surface gravity HIGHEST: " + QString::number(surfaceGravityHighest / 100));
				updateSystemsVisited();
				saveEliteCFG();
			}
			ui->textEdit->append("SurfaceGravity: " + QString::number(value.toVariant().toFloat() / 100));

			value = sett2.value(QString("SurfaceTemperature"));
			ui->textEdit->append("SurfaceTemperature: " + QString::number(value.toVariant().toFloat()));

			value = sett2.value(QString("SurfacePressure"));
			ui->textEdit->append("SurfacePressure: " + QString::number(value.toVariant().toFloat()));

			value = sett2.value(QString("Landable"));
			//qDebug() << "Landable: " << value;
			ui->textEdit->append("Landable: " + QString::number(value.toBool()));

			// ugly shuffle for landable planet highscores
			if (value.toBool())
			{
				// we have landabale planet
				// do some highscores specifically for landables, ie smallest, largest radius
				value = sett2.value(QString("Radius"));
				// planet radius smallest highscore
				if (landableRadiusSmallest > value.toVariant().toFloat())
				{
					landableRadiusSmallest = value.toVariant().toFloat();
					ui->textEdit->append("New highscore *landable* planet radius SMALLEST! " + QString::number(landableRadiusSmallest / 1000));
					updateSystemsVisited();
					saveEliteCFG();
				}
				// planet radius largest highscore
				if (landableRadiusLargest < value.toVariant().toFloat())
				{
					landableRadiusLargest = value.toVariant().toFloat();
					ui->textEdit->append("New highscore *landable* planet radius LARGEST! " + QString::number(landableRadiusLargest / 1000));
					updateSystemsVisited();
					saveEliteCFG();
				}

				value = sett2.value(QString("SurfaceGravity"));
				// planet gravity lowest highscore
				if (landableGravityLowest > value.toVariant().toFloat())
				{
					landableGravityLowest = value.toVariant().toFloat();
					ui->textEdit->append("New highscore *landable* planet surface gravity LOWEST: " + QString::number(landableGravityLowest / 100));
					updateSystemsVisited();
					saveEliteCFG();
				}
				// planet gravity highest highscore
				if (landableGravityHighest < value.toVariant().toFloat())
				{
					landableGravityHighest = value.toVariant().toFloat();
					ui->textEdit->append("New highscore *landable* planet surface gravity HIGHEST: " + QString::number(landableGravityHighest / 100));
					updateSystemsVisited();
					saveEliteCFG();
				}
				ui->textEdit->append("landableGravity: " + QString::number(value.toVariant().toFloat() / 100));
			}

			// SemiMajorAxis
			value = sett2.value(QString("SemiMajorAxis"));
			ui->textEdit->append("SemiMajorAxis: " + QString::number(value.toVariant().toFloat()));

			// Eccentricity
			value = sett2.value(QString("Eccentricity"));
			ui->textEdit->append("Eccentricity: " + QString::number(value.toVariant().toFloat()));

			// OrbitalInclination
			value = sett2.value(QString("OrbitalInclination"));
			ui->textEdit->append("OrbitalInclination: " + QString::number(value.toVariant().toFloat()));

			// Periapsis
			value = sett2.value(QString("Periapsis"));
			ui->textEdit->append("Periapsis: " + QString::number(value.toVariant().toFloat()));

			value = sett2.value(QString("OrbitalPeriod"));
			ui->textEdit->append("OrbitalPeriod: " + QString::number(value.toVariant().toFloat()));

			value = sett2.value(QString("RotationPeriod"));
			ui->textEdit->append("RotationPeriod: " + QString::number(value.toVariant().toFloat()));

			/*
			 * rings is an array of sorts :)
			 *
			 * "Rings":[ { "Name":"Eol Prou LW-L c8-76 A A Belt", "RingClass":"eRingClass_MetalRich", "MassMT":1.05e+14, "InnerRad":8.95e+08, "OuterRad":2.04e+09 } ]
			 *
			value = sett2.value(QString("Rings"));
			//qDebug() << "Rings: " << value;
			ui->textEdit->append("Rings: " + value.toBool());
			*/

			// Materials
			QJsonObject jmats = sett2.value(QString("Materials")).toObject();
			QString matName;
			float matValue;
			for(QJsonObject::const_iterator iter = jmats.begin(); iter != jmats.end(); ++iter)
			{
				matName = iter.key();
				matValue = iter.value().toVariant().toFloat();
				ui->textEdit->append(matName + ", " + QString::number(matValue));
			}
		}
	}
/*
	// SellExplorationData
{ "timestamp":"2016-06-10T14:32:03Z", "event":"SellExplorationData", "Systems":[ "HIP 78085", "Praea Euq NW-W b1-3" ], "Discovered":[ "HIP 78085 A", "Praea Euq NW-W b1-3", "Praea Euq NW-W b1-3 3 a", "Praea Euq NW-W b1-3 3" ], "BaseValue":10822, "Bonus":3959 }

// BuyExplorationData has System and Cost

*/
	// MaterialCollected
	if (!value.toString().compare("MaterialCollected", Qt::CaseInsensitive))
	{
		// if its a Encoded, Manufactured or Raw
		QString tmp = sett2.value(QString("Category")).toString();

		// below all have Name and DiscoveryNumber

		if (!tmp.compare("Encoded"))
		{
			value = sett2.value(QString("Name"));
			ui->textEdit->append("MaterialCollected, Encoded, Name: " + value.toString());
			value = sett2.value(QString("DiscoveryNumber"));
			ui->textEdit->append("DiscoveryNumber: " + QString::number(value.toVariant().toFloat()));
		}
		if (!tmp.compare("Manufactured"))
		{
			value = sett2.value(QString("Name"));
			ui->textEdit->append("MaterialCollected, Manufactured, Name: " + value.toString());
			value = sett2.value(QString("DiscoveryNumber"));
			ui->textEdit->append("DiscoveryNumber: " + QString::number(value.toVariant().toFloat()));
		}
		if (!tmp.compare("Raw"))
		{
			value = sett2.value(QString("Name"));
			ui->textEdit->append("MaterialCollected, Raw, Name: " + value.toString());
			value = sett2.value(QString("DiscoveryNumber"));
			ui->textEdit->append("DiscoveryNumber: " + QString::number(value.toVariant().toFloat()));
		}
	}

	// FuelScoop
	if (!value.toString().compare("FuelScoop", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Scooped"));
		scoopedTotal += value.toVariant().toFloat();
		ui->textEdit->append("Scooped: " + QString::number(value.toVariant().toFloat()) + ", total scooped: " + QString::number(scoopedTotal));
		ui->FuelScoopedTotal->setText("Fuel scooped total: " + QString::number(scoopedTotal));

		// EliteLog.CFG update, but damn you will hammer this saving a lot
		// when exploring and fuel scooping every few minutes :(
		saveEliteCFG();
	}

	// MarketBuy
	if (!value.toString().compare("MarketBuy", Qt::CaseInsensitive))
	{
		// commodity type
		value = sett2.value(QString("Type"));
		ui->textEdit->append("MarketBuy, Type: " + value.toString());
		// commodity count
		value = sett2.value(QString("Count"));
		ui->textEdit->append("-> Count: " + QString::number(value.toVariant().toFloat()));
		// commodity buy price
		value = sett2.value(QString("BuyPrice"));
		ui->textEdit->append("-> BuyPrice: " + QString::number(value.toVariant().toFloat()));
		// commodity total cost
		value = sett2.value(QString("TotalCost"));
		ui->textEdit->append("-> TotalCost: " + QString::number(value.toVariant().toFloat()));
	}

	// MarketSell
	if (!value.toString().compare("MarketSell", Qt::CaseInsensitive))
	{
		// commodity type
		value = sett2.value(QString("Type"));
		ui->textEdit->append("MarketSell, Type: " + value.toString());
		// commodity count
		value = sett2.value(QString("Count"));
		ui->textEdit->append("-> Count: " + QString::number(value.toVariant().toFloat()));
		// commodity sell price
		value = sett2.value(QString("SellPrice"));
		ui->textEdit->append("-> SellPrice: " + QString::number(value.toVariant().toFloat()));
		// commodity total sale
		value = sett2.value(QString("TotalSale"));
		ui->textEdit->append("-> TotalSale: " + QString::number(value.toVariant().toFloat()));
		// commodity AvgPricePaid
		value = sett2.value(QString("AvgPricePaid"));
		ui->textEdit->append("-> AvgPricePaid: " + QString::number(value.toVariant().toFloat()));
	}

	// ReceiveText
	if (!value.toString().compare("ReceiveText", Qt::CaseInsensitive))
	{
		// From, From_Localised, Message, Message_Localised, Channel
		value = sett2.value(QString("From"));
		ui->textEdit->append("ReceiveText, From: " + value.toString());
		value = sett2.value(QString("From_Localised"));
		ui->textEdit->append("-> From_Localised: " + value.toString());
		value = sett2.value(QString("Message"));
		ui->textEdit->append("-> Message: " + value.toString());
		value = sett2.value(QString("Message_Localised"));
		ui->textEdit->append("-> Message_Localised: " + value.toString());
		value = sett2.value(QString("Channel"));
		ui->textEdit->append("-> Channel: " + value.toString());
	}

	// SendText
	if (!value.toString().compare("SendText", Qt::CaseInsensitive))
	{
		// To, To_Localised, Message
		value = sett2.value(QString("To"));
		ui->textEdit->append("SendText, To: " + value.toString());
		value = sett2.value(QString("To_Localised"));
		ui->textEdit->append("-> To_Localised: " + value.toString());
		value = sett2.value(QString("Message"));
		ui->textEdit->append("-> Message: " + value.toString());
	}

	// RefuelAll
	if (!value.toString().compare("RefuelAll", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Cost"));
		ui->textEdit->append("RefuelAll, Cost: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Amount"));
		ui->textEdit->append("-> Amount: " + QString::number(value.toVariant().toFloat()));
	}

	// CommitCrime
	if (!value.toString().compare("CommitCrime", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("CrimeType"));
		ui->textEdit->append("CommitCrime, CrimeType: " + value.toString());
		value = sett2.value(QString("Faction"));
		ui->textEdit->append("-> Faction: " + value.toString());
		value = sett2.value(QString("Fine"));
		ui->textEdit->append("-> Fine: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Bounty"));
		ui->textEdit->append("-> Bounty: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Victim"));
		ui->textEdit->append("-> Victim: " + value.toString());
		value = sett2.value(QString("Victim_Localised"));
		ui->textEdit->append("-> Victim_Localised: " + value.toString());
	}

	/*
this is old code, please fix with new journal:
{ "timestamp":"2016-10-24T18:40:07Z", "event":"Bounty", "Rewards":[ { "Faction":"Surya PLC", "Reward":2400 }, { "Faction":"Federation", "Reward":98820 } ], "TotalReward":101220, "VictimFaction":"Surya Jet Drug Empire" }
{ "timestamp":"2016-10-24T18:41:01Z", "event":"Bounty", "Rewards":[ { "Faction":"Surya PLC", "Reward":26537 } ], "TotalReward":26537, "VictimFaction":"Felicia Winters" }
{ "timestamp":"2016-10-24T18:42:02Z", "event":"Bounty", "Rewards":[ { "Faction":"Surya PLC", "Reward":400 }, { "Faction":"Federation", "Reward":46882 } ], "TotalReward":47282, "VictimFaction":"Surya Jet Drug Empire" }
{ "timestamp":"2016-10-24T18:46:54Z", "event":"Bounty", "Rewards":[ { "Faction":"Surya PLC", "Reward":1200 }, { "Faction":"Federation", "Reward":62955 } ], "TotalReward":64155, "VictimFaction":"Surya Confederacy" }
	 */
	// Bounty
	if (!value.toString().compare("Bounty", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Faction"));
		ui->textEdit->append("Bounty, Faction: " + value.toString());
		value = sett2.value(QString("Faction_Localised"));
		ui->textEdit->append("-> Faction_Localised: " + value.toString());
		value = sett2.value(QString("VictimFaction"));
		ui->textEdit->append("-> VictimFaction: " + value.toString());
		value = sett2.value(QString("VictimFaction_Localised"));
		ui->textEdit->append("-> VictimFaction_Localised: " + value.toString());
		value = sett2.value(QString("Target"));
		ui->textEdit->append("-> Target: " + value.toString());
		value = sett2.value(QString("Reward"));
		ui->textEdit->append("-> Reward: " + QString::number(value.toVariant().toFloat()));

		// Rewards is an array, hmm?
	}

	// PayLegacyFines
	if (!value.toString().compare("PayLegacyFines", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Amount"));
		ui->textEdit->append("PayLegacyFines, Amount: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("BrokerPercentage"));
		ui->textEdit->append("-> BrokerPercentage: " + QString::number(value.toVariant().toFloat()));
	}

	// PayFines
	if (!value.toString().compare("PayFines", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Amount"));
		ui->textEdit->append("PayFines, Amount: " + QString::number(value.toVariant().toFloat()));
	}

	// ModuleStore
	if (!value.toString().compare("ModuleStore", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Slot"));
		ui->textEdit->append("ModuleStore, Slot: " + value.toString());
		value = sett2.value(QString("StoredItem"));
		ui->textEdit->append("-> StoredItem: " + value.toString());
		value = sett2.value(QString("StoredItem_Localised"));
		ui->textEdit->append("-> StoredItem_Localised: " + value.toString());
		value = sett2.value(QString("Ship"));
		ui->textEdit->append("-> Ship: " + value.toString());
		value = sett2.value(QString("ShipID"));
		ui->textEdit->append("-> ShipID: " + QString::number(value.toVariant().toFloat()));
	}

	// MassModuleStore
	if (!value.toString().compare("MassModuleStore", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Ship"));
		ui->textEdit->append("MassModuleStore, Ship: " + value.toString());
		value = sett2.value(QString("ShipID"));
		ui->textEdit->append("-> ShipID: " + QString::number(value.toVariant().toFloat()));
		// Items is array of all the modules stored
	}

	// ModuleSellRemote
	if (!value.toString().compare("ModuleSellRemote", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("StorageSlot"));
		ui->textEdit->append("ModuleSellRemote, StorageSlot: " + value.toString());
		value = sett2.value(QString("SellItem"));
		ui->textEdit->append("-> SellItem: " + value.toString());
		value = sett2.value(QString("SellItem_Localised"));
		ui->textEdit->append("-> SellItem_Localised: " + value.toString());
		value = sett2.value(QString("ServerId"));
		ui->textEdit->append("-> ServerId: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("SellPrice"));
		ui->textEdit->append("-> SellPrice: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Ship"));
		ui->textEdit->append("-> Ship: " + value.toString());
	}

	// ShipyardBuy
	if (!value.toString().compare("ShipyardBuy", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("ShipType"));
		ui->textEdit->append("ShipyardBuy, ShipType: " + value.toString());
		value = sett2.value(QString("ShipPrice"));
		ui->textEdit->append("-> ShipPrice: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("StoreOldShip"));
		ui->textEdit->append("-> StoreOldShip: " + value.toString());
		value = sett2.value(QString("StoreShipID"));
		ui->textEdit->append("-> StoreShipID: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("SellOldShip"));
		ui->textEdit->append("-> SellOldShip: " + value.toString());
		value = sett2.value(QString("SellShipID"));
		ui->textEdit->append("-> SellShipID: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("SellPrice"));
		ui->textEdit->append("-> SellPrice: " + QString::number(value.toVariant().toFloat()));
	}

	// ShipyardNew
	if (!value.toString().compare("ShipyardNew", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("ShipType"));
		ui->textEdit->append("ShipyardNew, ShipType: " + value.toString());
		value = sett2.value(QString("NewShipID"));
		ui->textEdit->append("-> NewShipID: " + QString::number(value.toVariant().toFloat()));
	}

	// ShipyardSell
	if (!value.toString().compare("ShipyardSell", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("ShipType"));
		ui->textEdit->append("ShipyardSell, ShipType: " + value.toString());
		value = sett2.value(QString("SellShipID"));
		ui->textEdit->append("-> SellShipID: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("ShipPrice"));
		ui->textEdit->append("-> ShipPrice: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("System"));
		ui->textEdit->append("-> System: " + value.toString());
	}

	// ShipyardSwap
	if (!value.toString().compare("ShipyardSwap", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("ShipType"));
		ui->textEdit->append("ShipyardSwap, ShipType: " + value.toString());
		value = sett2.value(QString("ShipID"));
		ui->textEdit->append("-> ShipID: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("StoreOldShip"));
		ui->textEdit->append("-> StoreOldShip: " + value.toString());
		value = sett2.value(QString("StoreShipID"));
		ui->textEdit->append("-> StoreShipID: " + QString::number(value.toVariant().toFloat()));
	}

	// ShipyardTransfer
	if (!value.toString().compare("ShipyardTransfer", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("ShipType"));
		ui->textEdit->append("ShipyardTransfer, ShipType: " + value.toString());
		value = sett2.value(QString("ShipID"));
		ui->textEdit->append("-> ShipID: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("System"));
		ui->textEdit->append("-> System: " + value.toString());
		value = sett2.value(QString("Distance"));
		ui->textEdit->append("-> Distance: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("TransferPrice"));
		ui->textEdit->append("-> TransferPrice: " + QString::number(value.toVariant().toFloat()));
	}

	// FetchRemoteModule
	if (!value.toString().compare("FetchRemoteModule", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("StorageSlot"));
		ui->textEdit->append("FetchRemoteModule, StorageSlot: " + value.toString());
		value = sett2.value(QString("StoredItem"));
		ui->textEdit->append("-> StoredItem: " + value.toString());
		value = sett2.value(QString("StoredItem_Localised"));
		ui->textEdit->append("-> StoredItem_Localised: " + value.toString());
		value = sett2.value(QString("ServerId"));
		ui->textEdit->append("-> ServerId: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Ship"));
		ui->textEdit->append("-> Ship: " + value.toString());
		value = sett2.value(QString("ShipID"));
		ui->textEdit->append("-> ShipID: " + QString::number(value.toVariant().toFloat()));
	}

	// ModuleBuy
	if (!value.toString().compare("ModuleBuy", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Slot"));
		ui->textEdit->append("ModuleBuy, Slot: " + value.toString());
		value = sett2.value(QString("SellItem"));
		ui->textEdit->append("-> SellItem: " + value.toString());
		value = sett2.value(QString("SellItem_Localised"));
		ui->textEdit->append("-> SellItem_Localised: " + value.toString());
		value = sett2.value(QString("SellPrice"));
		ui->textEdit->append("-> SellPrice: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("BuyItem"));
		ui->textEdit->append("-> BuyItem: " + value.toString());
		value = sett2.value(QString("BuyItem_Localised"));
		ui->textEdit->append("-> BuyItem_Localised: " + value.toString());
		value = sett2.value(QString("BuyPrice"));
		ui->textEdit->append("-> BuyPrice: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Ship"));
		ui->textEdit->append("-> Ship: " + value.toString());
		value = sett2.value(QString("ShipID"));
		ui->textEdit->append("-> ShipID: " + QString::number(value.toVariant().toFloat()));
	}

	// ModuleSell
	if (!value.toString().compare("ModuleSell", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Slot"));
		ui->textEdit->append("ModuleSell, Slot: " + value.toString());
		value = sett2.value(QString("SellItem"));
		ui->textEdit->append("-> SellItem: " + value.toString());
		value = sett2.value(QString("SellItem_Localised"));
		ui->textEdit->append("-> SellItem_Localised: " + value.toString());
		value = sett2.value(QString("SellPrice"));
		ui->textEdit->append("-> SellPrice: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Ship"));
		ui->textEdit->append("-> Ship: " + value.toString());
		value = sett2.value(QString("ShipID"));
		ui->textEdit->append("-> ShipID: " + QString::number(value.toVariant().toFloat()));
	}

	// ModuleSwap
	if (!value.toString().compare("ModuleSwap", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("FromSlot"));
		ui->textEdit->append("ModuleSwap, FromSlot: " + value.toString());
		value = sett2.value(QString("ToSlot"));
		ui->textEdit->append("-> ToSlot: " + value.toString());
		value = sett2.value(QString("FromItem"));
		ui->textEdit->append("-> FromItem: " + value.toString());
		value = sett2.value(QString("FromItem_Localised"));
		ui->textEdit->append("-> FromItem_Localised: " + value.toString());
		value = sett2.value(QString("ToItem"));
		ui->textEdit->append("-> ToItem: " + value.toString());
		value = sett2.value(QString("ToItem_Localised"));
		ui->textEdit->append("-> ToItem_Localised: " + value.toString());
		value = sett2.value(QString("Ship"));
		ui->textEdit->append("-> Ship: " + value.toString());
		value = sett2.value(QString("ShipID"));
		ui->textEdit->append("-> ShipID: " + QString::number(value.toVariant().toFloat()));
	}

	// ModuleSellRemote
	if (!value.toString().compare("ModuleSellRemote", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("StorageSlot"));
		ui->textEdit->append("ModuleSellRemote, StorageSlot: " + value.toString());
		value = sett2.value(QString("SellItem"));
		ui->textEdit->append("-> SellItem: " + value.toString());
		value = sett2.value(QString("SellItem_Localised"));
		ui->textEdit->append("-> SellItem_Localised: " + value.toString());
		value = sett2.value(QString("ServerId"));
		ui->textEdit->append("-> ServerId: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("SellPrice"));
		ui->textEdit->append("-> SellPrice: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Ship"));
		ui->textEdit->append("-> Ship: " + value.toString());
		value = sett2.value(QString("ShipID"));
		ui->textEdit->append("-> ShipID: " + QString::number(value.toVariant().toFloat()));
	}

	// EngineerProgress
	if (!value.toString().compare("EngineerProgress", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Engineer"));
		ui->textEdit->append("EngineerProgress, Engineer: " + value.toString());
		value = sett2.value(QString("Progress"));
		ui->textEdit->append("-> Progress: " + value.toString());
		value = sett2.value(QString("Rank"));
		ui->textEdit->append("-> Rank: " + value.toString());
	}

	// EngineerApply
	if (!value.toString().compare("EngineerApply", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Engineer"));
		ui->textEdit->append("EngineerApply, Engineer: " + value.toString());
		value = sett2.value(QString("Blueprint"));
		ui->textEdit->append("-> Blueprint: " + value.toString());
		value = sett2.value(QString("Level"));
		ui->textEdit->append("-> Level: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Override"));
		ui->textEdit->append("-> Override: " + value.toString());
	}

	// EngineerCraft
	if (!value.toString().compare("EngineerCraft", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Engineer"));
		ui->textEdit->append("EngineerCraft, Engineer: " + value.toString());
		value = sett2.value(QString("Blueprint"));
		ui->textEdit->append("-> Blueprint: " + value.toString());
		value = sett2.value(QString("Level"));
		ui->textEdit->append("-> Level: " + QString::number(value.toVariant().toFloat()));
		// Ingredients is an array
	}

	// CollectCargo
	if (!value.toString().compare("CollectCargo", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Type"));
		ui->textEdit->append("CollectCargo, Type: " + value.toString());
		value = sett2.value(QString("Stolen"));
		ui->textEdit->append("-> Stolen: " + QString::number(value.toBool()));
	}

	// EjectCargo
	if (!value.toString().compare("EjectCargo", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Type"));
		ui->textEdit->append("EjectCargo, Type: " + value.toString());
		value = sett2.value(QString("Count"));
		ui->textEdit->append("-> Count: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Abandoned"));
		ui->textEdit->append("-> Abandoned: " + QString::number(value.toBool()));
	}

	// RepairAll
	if (!value.toString().compare("RepairAll", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Cost"));
		ui->textEdit->append("RepairAll, Cost: " + QString::number(value.toVariant().toFloat()));
	}

	// Repair
	if (!value.toString().compare("Repair", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Item"));
		ui->textEdit->append("Repair, Item: " + value.toString());
		value = sett2.value(QString("Item_Localised"));
		ui->textEdit->append("-> Item_Localised: " + value.toString());
		value = sett2.value(QString("Cost"));
		ui->textEdit->append("-> Cost: " + QString::number(value.toVariant().toFloat()));
	}

	// RebootRepair even just has Modules which is an array

	// Synthesis
	if (!value.toString().compare("Synthesis", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Name"));
		ui->textEdit->append("Synthesis, Name: " + value.toString());
		// Materials is an array
	}

	// RedeemVoucher
	if (!value.toString().compare("RedeemVoucher", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Type"));
		ui->textEdit->append("RedeemVoucher, Type: " + value.toString());
		value = sett2.value(QString("Amount"));
		ui->textEdit->append("-> Amount: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("BrokerPercentage"));
		ui->textEdit->append("-> BrokerPercentage: " + QString::number(value.toVariant().toFloat()));
	}

	// MiningRefined
	if (!value.toString().compare("MiningRefined", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Type"));
		ui->textEdit->append("MiningRefined, Type: " + value.toString());
		value = sett2.value(QString("Type_Localised"));
		ui->textEdit->append("-> Type_Localised: " + value.toString());
	}

	// MaterialDiscovered
	if (!value.toString().compare("MaterialDiscovered", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Category"));
		ui->textEdit->append("MaterialDiscovered, Category: " + value.toString());
		value = sett2.value(QString("Name"));
		ui->textEdit->append("-> Name: " + value.toString());
		value = sett2.value(QString("DiscoveryNumber"));
		ui->textEdit->append("-> DiscoveryNumber: " + QString::number(value.toVariant().toFloat()));
	}

	// Touchdown
	if (!value.toString().compare("Touchdown", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Latitude"));
		ui->textEdit->append("Touchdown, Latitude: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Longitude"));
		ui->textEdit->append("-> Longitude: " + QString::number(value.toVariant().toFloat()));
	}

	// Liftoff
	if (!value.toString().compare("Liftoff", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Latitude"));
		ui->textEdit->append("Liftoff, Latitude: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Longitude"));
		ui->textEdit->append("-> Longitude: " + QString::number(value.toVariant().toFloat()));
	}

	// BuyDrones
	if (!value.toString().compare("BuyDrones", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Type"));
		ui->textEdit->append("BuyDrones, Type: " + value.toString());
		value = sett2.value(QString("Count"));
		ui->textEdit->append("-> Count: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("BuyPrice"));
		ui->textEdit->append("-> BuyPrice: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("TotalCost"));
		ui->textEdit->append("-> TotalCost: " + QString::number(value.toVariant().toFloat()));
	}

	// Interdicted
	if (!value.toString().compare("Interdicted", Qt::CaseInsensitive))
	{
		// boolean
		value = sett2.value(QString("Submitted"));
		ui->textEdit->append("Interdicted, Submitted: " + QString::number(value.toBool()));
		value = sett2.value(QString("Interdictor"));
		ui->textEdit->append("-> Interdictor: " + value.toString());
		// boolean
		value = sett2.value(QString("IsPlayer"));
		ui->textEdit->append("-> IsPlayer: " + QString::number(value.toBool()));
		value = sett2.value(QString("Faction"));
		ui->textEdit->append("-> Faction: " + value.toString());
	}

	// EscapeInterdiction
	if (!value.toString().compare("EscapeInterdiction", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Interdictor"));
		ui->textEdit->append("EscapeInterdiction, Interdictor: " + value.toString());
		value = sett2.value(QString("IsPlayer"));
		ui->textEdit->append("-> IsPlayer: " + QString::number(value.toBool()));
	}

	/* this might be obsolete as I never seen it?
	// interdiction
	if (!value.toString().compare("interdiction", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Success"));
		ui->textEdit->append("interdiction, Success: " + QString::number(value.toBool()));
		value = sett2.value(QString("Interdicted"));
		ui->textEdit->append("-> Interdicted: " + value.toString());
		value = sett2.value(QString("IsPlayer"));
		ui->textEdit->append("-> IsPlayer: " + QString::number(value.toBool()));
		value = sett2.value(QString("CombatRank"));
		ui->textEdit->append("-> CombatRank: " + QString::number(value.toVariant().toFloat());
	}
*/
	// FactionKillBond
	if (!value.toString().compare("FactionKillBond", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Reward"));
		ui->textEdit->append("FactionKillBond, Reward: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("AwardingFaction"));
		ui->textEdit->append("-> AwardingFaction: " + value.toString());
		value = sett2.value(QString("VictimFaction"));
		ui->textEdit->append("-> VictimFaction: " + value.toString());
	}

	// ApproachSettlement
	if (!value.toString().compare("ApproachSettlement", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Name"));
		ui->textEdit->append("ApproachSettlement, Name: " + value.toString());
		value = sett2.value(QString("Name_Localised"));
		ui->textEdit->append("-> Name_Localised: " + value.toString());
	}

	// DataScanned
	if (!value.toString().compare("DataScanned", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Type"));
		ui->textEdit->append("DataScanned, Type: " + value.toString());
	}

	// LaunchFighter
	if (!value.toString().compare("LaunchFighter", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Loadout"));
		ui->textEdit->append("LaunchFighter, Loadout: " + value.toString());
		// boolean
		value = sett2.value(QString("PlayerControlled"));
		ui->textEdit->append("-> PlayerControlled: " + QString::number(value.toBool()));
	}

	// HullDamage
	if (!value.toString().compare("HullDamage", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Health"));
		ui->textEdit->append("HullDamage, Health: " + QString::number(value.toVariant().toFloat()));
	}

	// ShieldState
	if (!value.toString().compare("ShieldState", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("ShieldsUp"));
		ui->textEdit->append("ShieldState, ShieldsUp: " + QString::number(value.toBool()));
	}

	// Resurrect
	if (!value.toString().compare("Resurrect", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Option"));
		ui->textEdit->append("Resurrect, Option: " + value.toString());
		value = sett2.value(QString("Cost"));
		ui->textEdit->append("-> Cost: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Bankrupt"));
		ui->textEdit->append("-> Bankrupt: " + QString::number(value.toBool()));
	}

	// RestockVehicle
	if (!value.toString().compare("RestockVehicle", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Type"));
		ui->textEdit->append("RestockVehicle, Type: " + value.toString());
		value = sett2.value(QString("Loadout"));
		ui->textEdit->append("-> Loadout: " + value.toString());
		value = sett2.value(QString("Cost"));
		ui->textEdit->append("-> Cost: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("Count"));
		ui->textEdit->append("-> Count: " + QString::number(value.toVariant().toFloat()));
	}

	// CrewHire
	if (!value.toString().compare("CrewHire", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Name"));
		ui->textEdit->append("CrewHire, Name: " + value.toString());
		value = sett2.value(QString("Faction"));
		ui->textEdit->append("-> Faction: " + value.toString());
		value = sett2.value(QString("Cost"));
		ui->textEdit->append("-> Cost: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("CombatRank"));
		ui->textEdit->append("-> CombatRank: " + QString::number(value.toVariant().toFloat()));
	}

	// CrewAssign
	if (!value.toString().compare("CrewAssign", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Name"));
		ui->textEdit->append("CrewAssign, Name: " + value.toString());
		value = sett2.value(QString("Role"));
		ui->textEdit->append("-> Role: " + value.toString());
	}

	// ClearSavedGame
	if (!value.toString().compare("ClearSavedGame", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Name"));
		ui->textEdit->append("ClearSavedGame, Name: " + value.toString());
	}

	// NewCommander
	if (!value.toString().compare("NewCommander", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Name"));
		ui->textEdit->append("NewCommander, Name: " + value.toString());
		value = sett2.value(QString("Package"));
		ui->textEdit->append("-> Package: " + value.toString());
	}

	// MissionAccepted
	if (!value.toString().compare("MissionAccepted", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Faction"));
		ui->textEdit->append("MissionAccepted, Faction: " + value.toString());
		value = sett2.value(QString("Name"));
		ui->textEdit->append("-> Name: " + value.toString());
		value = sett2.value(QString("DestinationSystem"));
		ui->textEdit->append("-> DestinationSystem: " + value.toString());
		value = sett2.value(QString("DestinationStation"));
		ui->textEdit->append("-> DestinationStation: " + value.toString());
		value = sett2.value(QString("Expiry"));
		ui->textEdit->append("-> Expiry: " + value.toString());
		value = sett2.value(QString("MissionID"));
		ui->textEdit->append("-> MissionID: " + QString::number(value.toVariant().toFloat()));
	}

	// MissionCompleted
	if (!value.toString().compare("MissionCompleted", Qt::CaseInsensitive))
	{
		value = sett2.value(QString("Faction"));
		ui->textEdit->append("MissionCompleted, Faction: " + value.toString());
		value = sett2.value(QString("Name"));
		ui->textEdit->append("-> Name: " + value.toString());
		value = sett2.value(QString("MissionID"));
		ui->textEdit->append("-> MissionID: " + QString::number(value.toVariant().toFloat()));
		value = sett2.value(QString("DestinationSystem"));
		ui->textEdit->append("-> DestinationSystem: " + value.toString());
		value = sett2.value(QString("DestinationStation"));
		ui->textEdit->append("-> DestinationStation: " + value.toString());
		value = sett2.value(QString("Reward"));
		ui->textEdit->append("-> Reward: " + QString::number(value.toVariant().toFloat()));
	}
}


// obsolete with new v2.2 Journal stuff
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


// obsolete with new v2.2 Journal stuff
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
	ui->FuelScoopedTotal->setText("Fuel scooped total: " + QString::number(scoopedTotal) + ", lowest fuel level reached: " + QString::number(fuelLevelLowest));

	// lets just damn update everything here :)
	ui->JumpDistanceRecords->setText("Jump distance shortest: " + QString::number(JumpDistShortest) + ", longest: " + QString::number(JumpDistLongest));
	ui->StarMassRecords->setText("Star mass lowest: " + QString::number(stellarMassLowest) + ", highest: " + QString::number(stellarMassHighest));
	ui->StarRadiusRecords->setText("Star radius smallest: " + QString::number(stellarRadiusSmallest) + ", largest: " + QString::number(stellarRadiusLargest));
	ui->DistanceLSRecords->setText("Planet Light Seconds from star closest: " + QString::number(DistanceFromArrivalLSMin) + ", furthest: " + QString::number(DistanceFromArrivalLSMax));
	ui->PlanetRadiusRecords->setText("Planet radius smallest: " + QString::number(planetRadiusSmallest / 1000) + ", largest: " + QString::number(planetRadiusLargest / 1000));
	ui->PlanetGravityRecords->setText("Planet gravity lowest: " + QString::number(surfaceGravityLowest / 100) + ", highest: " + QString::number(surfaceGravityHighest / 100));
	ui->LandableRadiusRecords->setText("Landable Planet radius smallest: " + QString::number(landableRadiusSmallest / 1000) + ", largest: " + QString::number(landableRadiusLargest / 1000));
	ui->LandableGravityRecords->setText("Landable Planet gravity lowest: " + QString::number(landableGravityLowest / 100) + ", highest: " + QString::number(landableGravityHighest / 100));
	ui->Age_MYRecords->setText("Star youngest: " + QString::number(age_MYyoungest) + ", oldest: " + QString::number(age_MYoldest));
}
